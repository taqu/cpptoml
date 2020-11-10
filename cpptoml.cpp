/**
@file cpptoml.cpp
@author t-sakai

# License
This software is distributed under two licenses, choose whichever you like.

## MIT License
Copyright (c) 2020 Takuro Sakai

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Public Domain
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/
#include "cpptoml.h"

#include <cstdlib>

#ifdef CPPTOML_DEBUG
#    include <cassert>
#    include <cstdio>
#    include <string>
#    define CPPTOML_ASSERT(exp) assert((exp))
#else
#    define CPPTOML_ASSERT(exp)
#endif

namespace cpptoml
{
const char* EmptyString = "";

//----------------------------------------------------------------------
TomlParser::Buffer::Buffer(Allocator allocator, Deallocator deallocator)
    : allocator_(allocator)
    , deallocator_(deallocator)
    , capacity_(0)
    , size_(0)
    , chunks_(CPPTOML_NULL)
{
    CPPTOML_ASSERT((CPPTOML_NULL == allocator_ && CPPTOML_NULL == deallocator_)
                   || (CPPTOML_NULL != allocator_ && CPPTOML_NULL != deallocator_));

    if(CPPTOML_NULL == allocator_) {
        allocator_ = malloc;
    }
    if(CPPTOML_NULL == deallocator_) {
        deallocator_ = free;
    }
}

TomlParser::Buffer::~Buffer()
{
    deallocator_(chunks_);
}

void TomlParser::Buffer::clear()
{
    size_ = 0;
}

template<>
u16 TomlParser::Buffer::create<Element>()
{
    expand();
    u16 result = size_;
    ++size_;
    chunks_[result].element_ = {};
    return result;
}

template<>
u16 TomlParser::Buffer::create<Array>()
{
    expand();
    u16 result = size_;
    ++size_;
    chunks_[result].array_ = {0, 0, static_cast<u16>(Type::Array), End, End, End, End};
    return result;
}

template<>
u16 TomlParser::Buffer::create<KeyValue>()
{
    expand();
    u16 result = size_;
    ++size_;
    chunks_[result].keyvalue_ = {0, 0, 0, End, End, End, End};
    return result;
}

template<>
u16 TomlParser::Buffer::create<Table>()
{
    expand();
    u16 result = size_;
    ++size_;
    chunks_[result].table_ = {0, 0, static_cast<u16>(Type::Table), End, End, End, End};
    return result;
}

void TomlParser::Buffer::expand()
{
    if(capacity_ <= size_) {
        u16 capacity = capacity_ + AllocStep;
        Chunk* chunks = reinterpret_cast<Chunk*>(allocator_(sizeof(Chunk) * capacity));
        CPPTOML_ASSERT(CPPTOML_NULL != chunks);
        memcpy(chunks, chunks_, sizeof(Chunk) * capacity_);
        deallocator_(chunks_);
        capacity_ = capacity;
        chunks_ = chunks;
    }
}

const TomlParser::Chunk& TomlParser::Buffer::operator[](u16 index) const
{
    return chunks_[index];
}

TomlParser::Chunk& TomlParser::Buffer::operator[](u16 index)
{
    return chunks_[index];
}

namespace
{
    struct Line
    {
        u32 size_;
        const_iterator begin_;
    };

#ifdef CPPTOML_DEBUG
#    define CPPTOML_PRINT(fmt, ...) std::printf(fmt, __VA_ARGS__)
#    define CPPTOML_PRINTLINE(line) print(line)

    void print(const Line& line)
    {
        std::string str(line.begin_, line.size_);
        std::printf("%s\n", str.c_str());
    }

#else
#    define CPPTOML_PRINT(fmt, ...)
#    define CPPTOML_PRINTLINE(line)
#endif

    static const char LF = 0x0A;
    static const char CR = 0x0D;
    static const char CommentStart = 0x23;
    static const char Space = 0x20;
    static const char HorizontalTab = 0x09;
    static const char KeyValueSep = 0x3D;
    static const char Quotation = 0x22;
    static const char Apostrophe = 0x27;
    static const char Escape = 0x5C;
    static const char Underscore = 0x5F;
    static const char ArrayOpen = 0x5B;
    static const char ArrayClose = 0x5D;
    static const char Comma = 0x2C;
    static const char StdTableOpen = 0x5B;
    static const char StdTableClose = 0x5D;
    static const char InlineTableOpen = 0x7B;
    static const char InlineTableClose = 0x7D;

    void skipNewlines(const_iterator& current, const_iterator end);
    void skipWhitespaces(const_iterator& current, const_iterator end);

    bool isNewline(const_iterator iterator)
    {
        char c = *iterator;
        return LF == c || CR == c;
    }

    bool isWhitespace(const_iterator iterator)
    {
        char c = *iterator;
        return ' ' == c || '\t' == c;
    }

    s32 getUTF8Size(u8 utf8)
    {
        if(utf8 < 0x80U) {
            return 1;
        } else if(0xC2U <= utf8 && utf8 < 0xE0U) {
            return 2;
        } else if(0xE0U <= utf8 && utf8 < 0xF0U) {
            return 3;
        } else if(0xF0U <= utf8 && utf8 < 0xF8U) {
            return 4;
        }
        return 0;
    }

    bool isAlpha(const_iterator iterator)
    {
        char c = *iterator;
        return (0x41 <= c && c <= 0x5A) || (0x61 <= c && c <= 0x7A);
    }

    bool isDigit(const_iterator iterator)
    {
        char c = *iterator;
        return (0x30 <= c && c <= 0x39);
    }
    bool isNonZero(const_iterator iterator)
    {
        char c = *iterator;
        return (0x31 <= c && c <= 0x39);
    }

    bool isHexDigit(const_iterator iterator)
    {
        char c = *iterator;
        return (0x30 <= c && c <= 0x39) || (0x41 <= c && c <= 0x46);
    }

    bool isOctDigit(const_iterator iterator)
    {
        char c = *iterator;
        return (0x30 <= c && c <= 0x37);
    }

    bool isBinDigit(const_iterator iterator)
    {
        char c = *iterator;
        return (0x30 <= c && c <= 0x31);
    }

    bool isNonAscii(const_iterator iterator)
    {
        u8 c = *reinterpret_cast<const u8*>(iterator);
        return 0x80U <= c;
    }

    s32 isNonEol(const_iterator iterator)
    {
        char c = *iterator;
        if(0x09 == c
           || (0x20 <= c && c <= 0x7F)) {
            return true;
        }
        return isNonAscii(iterator);
    }

    bool isBasicUnescaped(const_iterator iterator)
    {
        char c = *iterator;
        switch(c) {
        case Space:
        case HorizontalTab:
        case 0x21:
            return true;
        default:
            return (0x23 <= c && c <= 0x5B)
                   || (0x5D <= c && c <= 0x7E)
                   || isNonEol(iterator);
        }
    }

    bool isLiteralChar(const_iterator iterator)
    {
        char c = *iterator;
        switch(c) {
        case 0x09:
            return true;
        default:
            return (0x20 <= c && c <= 0x26)
                   || (0x28 <= c && c <= 0x7E)
                   || isNonAscii(iterator);
        }
    }

    bool isMlbUnescaped(const_iterator iterator)
    {
        char c = *iterator;
        switch(c) {
        case Space:
        case HorizontalTab:
        case 0x21:
            return true;
        default:
            return (0x23 <= c && c <= 0x5B)
                   || (0x5D <= c && c <= 0x7E)
                   || isNonAscii(iterator);
        }
    }

    bool isMllChar(const_iterator iterator)
    {
        char c = *iterator;
        switch(c) {
        case 0x09:
            return true;
        default:
            return (0x20 <= c && c <= 0x26)
                   || (0x28 <= c && c <= 0x7E)
                   || isNonAscii(iterator);
        }
    }

    s32 isMlbEscapedNl(const_iterator iterator, const_iterator end)
    {
        CPPTOML_ASSERT(Quotation == *iterator);
        const_iterator start = iterator;
        ++iterator;
        if(end <= iterator) {
            return 0;
        }
        skipWhitespaces(iterator, end);
        if(end <= iterator) {
            return static_cast<s32>(iterator - start);
        }
        if(*iterator == LF || *iterator == CR) {
            skipNewlines(iterator, end);
            return static_cast<s32>(iterator - start);
        }
        return 0;
    }

    bool isTimeDelim(const_iterator iterator)
    {
        char c = *iterator;
        return 'T' == c || 't' == c || 0x20 == c;
    }

    bool isEndOfValue(const_iterator iterator, const_iterator end, bool isEndOfValue)
    {
        if(end <= iterator) {
            return true;
        }
        char c = *iterator;
        return LF == c
               || CR == c
               || Space == c
               || HorizontalTab == c
               || CommentStart == c
               || Comma == c
               || ArrayClose == c
               || InlineTableClose == c
               || (isEndOfValue && (0x2E == c || 'E' == c || 'e' == c));
    }

    s32 trailingChars(const_iterator iterator, const_iterator end, char c)
    {
        CPPTOML_ASSERT(c == *iterator);
        s32 count = 1;
        ++iterator;
        if(end <= iterator || (c != *iterator)) {
            return count;
        }
        ++count;
        ++iterator;
        if(end <= iterator || (c != *iterator)) {
            return count;
        }
        ++count;
        return count;
    }

    s32 countDigit(const_iterator iterator, const_iterator end)
    {
        s32 count = 0;
        while(iterator < end) {
            if(!isDigit(iterator)) {
                break;
            }
            ++count;
            ++iterator;
        }
        return count;
    }

    s32 getBase(const_iterator iterator, const_iterator end)
    {
        CPPTOML_ASSERT(0x30 == *iterator);
        ++iterator;
        if(end <= iterator) {
            return 10;
        }
        switch(*iterator) {
        case 0x78:
            return 16;
        case 0x6F:
            return 8;
        case 0x62:
            return 2;
        default:
            return 10;
        }
    }

    u32 hexDigitsToU32(char c)
    {
        if(0x30 <= c && c <= 0x39) {
            return static_cast<u32>(c - 0x30);
        }
        if(0x41 <= c && c <= 0x46) {
            return static_cast<u32>(c - 0x41);
        }
        return 0;
    }

    u32 octDigitsToU32(char c)
    {
        return (0x30 <= c && c <= 0x37)
                   ? static_cast<u32>(c - 0x30)
                   : 0;
    }

    u32 binDigitsToU32(char c)
    {
        return (0x30 <= c && c <= 0x31)
                   ? static_cast<u32>(c - 0x30)
                   : 0;
    }

    const_iterator skipBOM(u32 size, const_iterator begin)
    {
        if(size < 3) {
            return begin;
        }
        const u8* head = reinterpret_cast<const u8*>(begin);
        if(0xEFU == head[0]
           && 0xBBU == head[1]
           && 0xBFU == head[2]) {
            return begin + 3;
        }
        return begin;
    }

    void skipNewlines(const_iterator& current, const_iterator end)
    {
        while(current < end && isNewline(current)) {
            ++current;
        }
    }

    void skipWhitespaces(const_iterator& current, const_iterator end)
    {
        while(current < end && isWhitespace(current)) {
            ++current;
        }
    }

    void skipComment(const_iterator& current, const_iterator end)
    {
        while(current < end) {
            if(isNewline(current)) {
                break;
            }
            ++current;
        }
    }

    void skipWhitespaceCommentNewline(const_iterator& current, const_iterator end)
    {
        while(current < end) {
            switch(*current) {
            case Space:
            case HorizontalTab:
                skipWhitespaces(current, end);
                break;
            case CommentStart:
                skipComment(current, end);
                break;
            case LF:
            case CR:
                skipNewlines(current, end);
                break;
            default:
                return;
            }
        }
    }

    s32 toInt(char c)
    {
        CPPTOML_ASSERT(isDigit(&c));
        return c - 0x30;
    }

    s32 toInt(const_iterator iterator, s32 ranks)
    {
        s32 x = toInt(iterator[0]);
        for(s32 i = 1; i < ranks; ++i) {
            x = (x * 10) + toInt(iterator[i]);
        }
        return x;
    }
} // namespace

ArrayAccessor asArray(const ValueAccessor& value)
{
    return Type::Array == value.type()
               ? ArrayAccessor(value.parser_, value.value_)
               : ArrayAccessor();
}

//----------------------------------------------------------------------
ValueAccessor::ValueAccessor()
    : parser_(CPPTOML_NULL)
    , value_(End)
{
}

ValueAccessor::ValueAccessor(const TomlParser* parser, u16 value)
    : parser_(parser)
    , value_(value)
{
}

bool ValueAccessor::valid() const
{
    return End != value_;
}

Type ValueAccessor::type() const
{
    return static_cast<Type>(parser_->chunks_[value_].element_.type_);
}

bool ValueAccessor::getBoolean(bool defaultValue) const
{
    bool value;
    return tryGet(value) ? value : defaultValue;
}

s32 ValueAccessor::getInt(s32 defaultValue) const
{
    s32 value;
    return tryGet(value) ? value : defaultValue;
}

u32 ValueAccessor::getUInt(u32 defaultValue) const
{
    u32 value;
    return tryGet(value) ? value : defaultValue;
}

f32 ValueAccessor::getFloat(f32 defaultValue) const
{
    f32 value;
    return tryGet(value) ? value : defaultValue;
}

f64 ValueAccessor::getDouble(f64 defaultValue) const
{
    f64 value;
    return tryGet(value) ? value : defaultValue;
}

s32 ValueAccessor::getStringLength() const
{
    const Element& element = parser_->chunks_[value_].element_;
    switch(static_cast<Type>(element.type_)) {
    case Type::BasicString:
    case Type::LiteralString:
    case Type::MultiLineBasicString:
    case Type::MultiLineLiteralString:
        return element.size_;
    default:
        return 0;
    }
}

s32 ValueAccessor::getStringBufferSize() const
{
    const Element& element = parser_->chunks_[value_].element_;
    switch(static_cast<Type>(element.type_)) {
    case Type::BasicString:
    case Type::LiteralString:
    case Type::MultiLineBasicString:
    case Type::MultiLineLiteralString:
        return element.size_ + 1;
    default:
        return 0;
    }
}

const char* ValueAccessor::getString(s32 size, char* buffer, const char* defaultValue) const
{
    return tryGet(size, buffer) ? buffer : defaultValue;
}

bool ValueAccessor::tryGet(bool& value) const
{
    const Element& element = parser_->chunks_[value_].element_;
    if(static_cast<u16>(Type::Boolean) == element.type_) {
        value = element.boolean_;
        return true;
    }
    return false;
}

bool ValueAccessor::tryGet(s32& value) const
{
    const Element& element = parser_->chunks_[value_].element_;
    if(static_cast<u16>(Type::Int64) == element.type_
       || static_cast<u16>(Type::UInt64) == element.type_) {
        value = static_cast<s32>(element.s64_);
        return true;
    }
    return false;
}

bool ValueAccessor::tryGet(u32& value) const
{
    const Element& element = parser_->chunks_[value_].element_;
    if(static_cast<u16>(Type::UInt64) == element.type_
       || static_cast<u16>(Type::Int64) == element.type_) {
        value = static_cast<u32>(element.u64_);
        return true;
    }
    return false;
}

bool ValueAccessor::tryGet(f32& value) const
{
    const Element& element = parser_->chunks_[value_].element_;
    switch(static_cast<Type>(element.type_)) {
    case Type::Float:
        value = static_cast<f32>(element.f64_);
        return true;
    case Type::FloatInf:
        value = 0.0 < element.f64_ ? std::numeric_limits<f32>::infinity() : -std::numeric_limits<f32>::infinity();
        return true;
    case Type::FloatNan:
        value = std::numeric_limits<f32>::quiet_NaN();
        return true;
    }
    return false;
}

bool ValueAccessor::tryGet(f64& value) const
{
    const Element& element = parser_->chunks_[value_].element_;
    switch(static_cast<Type>(element.type_)) {
    case Type::Float:
    case Type::FloatInf:
    case Type::FloatNan:
        value = element.f64_;
        return true;
    }
    return false;
}

bool ValueAccessor::tryGet(s32 size, char* buffer) const
{
    //TODO: fix multi lines
    const Element& element = parser_->chunks_[value_].element_;
    if(size <= element.size_) {
        return false;
    }

    const char* str = parser_->toml_ + element.position_;
    switch(static_cast<Type>(element.type_)) {
    case Type::BasicString:
    case Type::LiteralString:
        strncpy_s(buffer, size, str, element.size_);
        return true;
    case Type::MultiLineBasicString:
    case Type::MultiLineLiteralString:
        strncpy_s(buffer, size, str, element.size_);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------
ArrayAccessor::Iterator::Iterator()
    :parser_(CPPTOML_NULL)
    ,index_(End)
    ,array_(End)
{
}

bool ArrayAccessor::Iterator::next()
{
    if(index_ == End){
        index_ = 0;
    }else{
        ++index_;
    }
    if(End != array_){
        if(3<=index_){
            index_ = 0;
            array_ = parser_->chunks_[array_].array_.next_;
            if(End == array_){
                return false;
            }
        }
        const u16* values = parser_->chunks_[array_].array_.values();
        return End != values[index_];
    }
    return false;
}

ValueAccessor ArrayAccessor::Iterator::get()
{
    const u16* values = parser_->chunks_[array_].array_.values();
    return ValueAccessor(parser_, values[index_]);
}

ArrayAccessor::Iterator::Iterator(const TomlParser* parser, u16 array)
    :parser_(parser)
    ,index_(End)
    ,array_(array)
{
}

ArrayAccessor::ArrayAccessor()
    : parser_(CPPTOML_NULL)
    , array_(End)
    , size_(0)
{
}

ArrayAccessor::ArrayAccessor(const TomlParser* parser, u16 array)
    : parser_(parser)
    , array_(array)
    , size_(0)
{
    u16 current = array_;
    for(;;) {
        const Array& a = parser_->chunks_[current].array_;
        const u16* values = a.values();
        for(s32 i = 0; i < 3; ++i) {
            if(End == values[i]) {
                break;
            }
            ++size_;
        }
        if(End == a.next_) {
            break;
        }
        current = a.next_;
    }
}

bool ArrayAccessor::valid() const
{
    return End != array_;
}

Type ArrayAccessor::type() const
{
    return static_cast<Type>(parser_->chunks_[array_].array_.type_);
}

u16 ArrayAccessor::size() const
{
    return size_;
}

ArrayAccessor::Iterator ArrayAccessor::begin() const
{
    return Iterator(parser_, array_);
}

//----------------------------------------------------------------------
TableAccessor::TableAccessor(const TomlParser* parser, u16 table)
    : parser_(parser)
    , table_(table)
{
}

bool TableAccessor::valid() const
{
    return End != table_;
}

TableAccessor TableAccessor::findTable(const char* name) const
{
    CPPTOML_ASSERT(CPPTOML_NULL != parser_ && End != table_);
    CPPTOML_ASSERT(CPPTOML_NULL != name);
    const Table& table = parser_->chunks_[table_].table_;
    u16 child = table.child_;
    while(End != child) {
        const KeyValue& keyvalue = parser_->chunks_[child].keyvalue_;
        if(static_cast<u16>(Type::Table) != keyvalue.type_) {
            child = keyvalue.next_;
            continue;
        }

        const char* tableName = parser_->toml_ + keyvalue.position_;
        if(0 == strncmp(tableName, name, keyvalue.size_)) {
            return TableAccessor(parser_, keyvalue.value_);
        }
        child = keyvalue.next_;
    }
    return TableAccessor(CPPTOML_NULL, End);
}

ArrayAccessor TableAccessor::findArray(const char* name) const
{
    CPPTOML_ASSERT(CPPTOML_NULL != parser_ && End != table_);
    CPPTOML_ASSERT(CPPTOML_NULL != name);
    const Table& table = parser_->chunks_[table_].table_;
    u16 child = table.child_;
    while(End != child) {
        const KeyValue& keyvalue = parser_->chunks_[child].keyvalue_;
        if(static_cast<u16>(Type::Array) != keyvalue.type_) {
            child = keyvalue.next_;
            continue;
        }

        const char* arrayName = parser_->toml_ + keyvalue.position_;
        if(0 == strncmp(arrayName, name, keyvalue.size_)) {
            return ArrayAccessor(parser_, keyvalue.value_);
        }
        child = keyvalue.next_;
    }
    return ArrayAccessor();
}

ValueAccessor TableAccessor::findValue(const char* name) const
{
    CPPTOML_ASSERT(CPPTOML_NULL != parser_ && End != table_);
    CPPTOML_ASSERT(CPPTOML_NULL != name);

    const Table& table = parser_->chunks_[table_].table_;
    u16 child = table.child_;
    while(End != child) {
        const KeyValue& keyvalue = parser_->chunks_[child].keyvalue_;
        if(static_cast<u16>(Type::Table) == keyvalue.type_) {
            child = keyvalue.next_;
            continue;
        }

        const char* keyname = parser_->toml_ + keyvalue.position_;
        if(0 == strncmp(keyname, name, keyvalue.size_)) {
            return ValueAccessor(parser_, keyvalue.value_);
        }
        child = keyvalue.next_;
    }

    return ValueAccessor();
}

bool TableAccessor::tryGet(TableAccessor& table, const char* name) const
{
    table = findTable(name);
    return table.valid();
}

bool TableAccessor::tryGet(ValueAccessor& value, const char* name) const
{
    value = findValue(name);
    return value.valid();
}

//----------------------------------------------------------------------
TomlParser::TomlParser(Allocator allocator, Deallocator deallocator) noexcept
    : currentTable_(0)
    , currentKeyTable_(0)
    , lastElement_(End)
    , lastKeySize_(0)
    , lastKeyPosition_(0)
    , chunks_(allocator, deallocator)
    , toml_(CPPTOML_NULL)
    , end_(CPPTOML_NULL)
{
}

TomlParser::~TomlParser() noexcept
{
}

TableAccessor TomlParser::getTop() const
{
    return TableAccessor(this, 0);
}

bool TomlParser::parse(u32 size, const_iterator toml) noexcept
{
    CPPTOML_ASSERT((0 < size && CPPTOML_NULL != toml) || (0 == size));
    currentTable_ = 0;
    chunks_.clear();

    chunks_.create<Table>();

    toml_ = toml;
    const_iterator current = skipBOM(size, toml_);
    end_ = toml_ + size;
    while(current < end_) {
        currentKeyTable_ = currentTable_;
        current = expression(current);
        if(CPPTOML_NULL == current) {
            return false;
        }
        skipNewlines(current, end_);
    }
    return true;
}

const_iterator TomlParser::expression(const_iterator current)
{
    skipWhitespaces(current, end_);
    if(end_ <= current) {
        return current;
    }
    switch(*current) {
    case CommentStart: //Comment start
        skipComment(current, end_);
        return current;

    case StdTableOpen:
        currentTable_ = currentKeyTable_ = 0; //reset current table
        current = table(current);
        if(CPPTOML_NULL == current) {
            return CPPTOML_NULL;
        }
        skipWhitespaces(current, end_);
        if(current < end_ && CommentStart == *current) {
            skipComment(current, end_);
        }
        return current;
    default: {
        u16 elementKey, elementValue;
        current = keyvalue(elementKey, elementValue, current);
        if(CPPTOML_NULL == current) {
            return CPPTOML_NULL;
        }
        linkKeyValueToTable(currentTable_, elementKey, elementValue);
        skipWhitespaces(current, end_);
        if(current < end_ && CommentStart == *current) {
            skipComment(current, end_);
        }
        return current;
    }
    }
}

const_iterator TomlParser::keyvalue(u16& elementKey, u16& elementValue, const_iterator current)
{
    current = key(current);
    if(CPPTOML_NULL == current) {
        return CPPTOML_NULL;
    }
    elementKey = lastElement_;

    //keyval-sep
    skipWhitespaces(current, end_);
    if(KeyValueSep != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    skipWhitespaces(current, end_);
    if(end_ <= current) {
        return CPPTOML_NULL;
    }

    current = value(current);
    if(CPPTOML_NULL != current) {
        elementValue = lastElement_;
    }
    return current;
}

bool TomlParser::skipDotSep(const_iterator& current)
{
    if(end_ <= current) {
        return false;
    }
    const_iterator next = current;
    skipWhitespaces(next, end_);
    if(end_ <= next || 0x2E != *next) {
        current = next;
        return false;
    }
    ++next;
    skipWhitespaces(next, end_);
    current = next;
    return true;
}

const_iterator TomlParser::key(const_iterator current)
{
    while(current < end_) {
        switch(*current) {
        case Quotation:
            current = basic_string(current);
            break;
        case Apostrophe:
            current = literal_string(current);
        default:
            current = unquoted_key(current);
            break;
        }
        if(CPPTOML_NULL == current) {
            break;
        }
        lastKeySize_ = chunks_[lastElement_].element_.size_;
        lastKeyPosition_ = chunks_[lastElement_].element_.position_;

        if(!skipDotSep(current)) {
            return current;
        }
        u16 nextTable = createTable(currentKeyTable_, lastKeySize_, lastKeyPosition_);
        if(End == nextTable) {
            return CPPTOML_NULL;
        }
        currentTable_ = currentKeyTable_ = nextTable;
    }
    return CPPTOML_NULL;
}

const_iterator TomlParser::basic_string(const_iterator current)
{
    CPPTOML_ASSERT(Quotation == *current);
    ++current;
    const_iterator start = current;
    while(current < end_) {
        char c = *current;
        switch(c) {
        case Quotation:
            lastElement_ = createString(Type::BasicString, start, current);
            ++current;
            return current;
        case Escape:
            current = escaped(current);
            if(CPPTOML_NULL == current) {
                return CPPTOML_NULL;
            }
            continue;
        default:
            if(!isBasicUnescaped(current)) {
                return CPPTOML_NULL;
            }
            break;
        }
        ++current;
    }
    return CPPTOML_NULL;
}

const_iterator TomlParser::escaped(const_iterator current)
{
    CPPTOML_ASSERT(Escape == *current);
    ++current;
    if(end_ <= current) {
        return CPPTOML_NULL;
    }
    char c = *current;
    switch(c) {
    case 0x22:
    case 0x5C:
    case 0x62:
    case 0x66:
    case 0x6E:
    case 0x72:
    case 0x74:
        ++current;
        return current;
    case 0x75: {
        s32 count = 0;
        while(current < end_) {
            if(!isHexDigit(current)) {
                break;
            }
            ++count;
            ++current;
        }
        return (4 == count) ? current : CPPTOML_NULL;
    }
    case 0x55: {
        s32 count = 0;
        while(current < end_) {
            if(!isHexDigit(current)) {
                break;
            }
            ++count;
            ++current;
        }
        return (8 == count) ? current : CPPTOML_NULL;
    }
    default:
        return CPPTOML_NULL;
    }
}

const_iterator TomlParser::literal_string(const_iterator current)
{
    CPPTOML_ASSERT(Apostrophe == *current);
    ++current;
    const_iterator start = current;
    while(current < end_) {
        char c = *current;
        switch(c) {
        case Apostrophe:
            lastElement_ = createString(Type::LiteralString, start, current);
            ++current;
            return current;
        default:
            if(!isLiteralChar(current)) {
                return CPPTOML_NULL;
            }
        }
        ++current;
    }
    return CPPTOML_NULL;
}

const_iterator TomlParser::unquoted_key(const_iterator current)
{
    const_iterator start = current;
    while(current < end_) {
        char c = *current;
        if(isAlpha(current)
           || isDigit(current)
           || (0x30 <= c && c <= 0x39)
           || (0x2D == c)
           || (0x5F == c)) {
            ++current;
        } else {
            lastElement_ = createString(Type::UnquotedKey, start, current);
            return current;
        }
    }
    return CPPTOML_NULL;
}

const_iterator TomlParser::value(const_iterator current)
{
    switch(*current) {
    case Quotation: //basic string
        current = value_basic_string(current);
        break;
    case Apostrophe: //literal string
        current = value_literal_string(current);
        break;
    case 0x74: //true
        current = value_true(current);
        break;
    case 0x66: //false
        current = value_false(current);
        break;
    case ArrayOpen: //array
        current = value_array(current);
        break;
    case InlineTableOpen: //inline-array
        current = inline_table(current);
        break;
    default: //date-time / float / integer
        current = value_number(current);
        break;
    }
    return current;
}

const_iterator TomlParser::value_basic_string(const_iterator current)
{
    CPPTOML_ASSERT(Quotation == *current);
    const_iterator next = current + 1;
    if(end_ <= next) {
        return CPPTOML_NULL;
    }
    switch(*next) {
    case Quotation:
        return ml_basic_string(next);
    default:
        return basic_string(current);
    }
}

const_iterator TomlParser::ml_basic_string(const_iterator current)
{
    CPPTOML_ASSERT(Quotation == *current);
    ++current;
    if(end_ <= current || Quotation != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    skipNewlines(current, end_);
    const_iterator start = current;
    while(current < end_) {
        char c = *current;
        switch(c) {
        case Quotation: {
            s32 quotations = trailingChars(current, end_, Quotation);
            if(3 == quotations) {
                lastElement_ = createString(Type::MultiLineBasicString, start, current);
                current += 3;
                return current;
            }
            current += quotations;
        } break;
        default:
            mlb_content(current);
            break;
        }
    }
    return CPPTOML_NULL;
}

const_iterator TomlParser::mlb_content(const_iterator current)
{
    while(current < end_) {
        char c = *current;
        switch(c) {
        case Quotation:
            return current;
        case LF:
        case CR:
            skipNewlines(current, end_);
            continue;
        case Escape: {
            s32 escapedNl = isMlbEscapedNl(current, end_);
            if(0 < escapedNl) {
                current += escapedNl;
                current = mlb_escaped_nl(current);
                if(CPPTOML_NULL == current) {
                    return CPPTOML_NULL;
                }
            } else {
                current = escaped(current);
                if(CPPTOML_NULL == current) {
                    return CPPTOML_NULL;
                }
            }
        }
            continue;
        default:
            if(!isMlbUnescaped(current)) {
                return CPPTOML_NULL;
            }
            break;
        }
        ++current;
    }
    return CPPTOML_NULL;
}

const_iterator TomlParser::mlb_escaped_nl(const_iterator current)
{
    CPPTOML_ASSERT(Quotation == *current);
    ++current;
    skipWhitespaces(current, end_);
    if(end_ <= current || (LF != *current && CR != *current)) {
        return CPPTOML_NULL;
    }
    skipNewlines(current, end_);
    while(current < end_) {
        char c = *current;
        switch(c) {
        case CR:
        case LF:
        case Space:
        case HorizontalTab:
            break;
        default:
            return CPPTOML_NULL;
        }
        ++current;
    }
    return CPPTOML_NULL;
}

const_iterator TomlParser::value_literal_string(const_iterator current)
{
    CPPTOML_ASSERT(Apostrophe == *current);
    ++current;
    if(end_ <= current) {
        return CPPTOML_NULL;
    }
    switch(*current) {
    case Apostrophe:
        return ml_literal_string(current);
    default:
        return literal_string(current - 1);
    }
}

const_iterator TomlParser::ml_literal_string(const_iterator current)
{
    CPPTOML_ASSERT(Apostrophe == *current);
    ++current;
    if(end_ <= current || Apostrophe != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    skipNewlines(current, end_);
    const_iterator start = current;
    while(current < end_) {
        char c = *current;
        switch(c) {
        case Apostrophe: {
            s32 apostrophes = trailingChars(current, end_, Apostrophe);
            if(3 == apostrophes) {
                lastElement_ = createString(Type::MultiLineLiteralString, start, current);
                current += 3;
                return current;
            }
            current += apostrophes;
        } break;
        default:
            mll_content(current);
            break;
        }
    }
    return CPPTOML_NULL;
}

const_iterator TomlParser::mll_content(const_iterator current)
{
    while(current < end_) {
        char c = *current;
        switch(c) {
        case Apostrophe:
            return current;
        case LF:
        case CR:
            skipNewlines(current, end_);
            continue;
        default:
            if(!isMllChar(current)) {
                return CPPTOML_NULL;
            }
            break;
        }
        ++current;
    }
    return CPPTOML_NULL;
}

const_iterator TomlParser::value_true(const_iterator current)
{
    CPPTOML_ASSERT(0x74 == *current);
    const_iterator start = current;
    ++current;
    if(end_ <= current || 0x72 != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    if(end_ <= current || 0x75 != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    if(end_ <= current || 0x65 != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    lastElement_ = createBoolean(start, current);
    return current;
}

const_iterator TomlParser::value_false(const_iterator current)
{
    CPPTOML_ASSERT(0x66 == *current);
    const_iterator start = current;
    ++current;
    if(end_ <= current || 0x61 != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    if(end_ <= current || 0x6C != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    if(end_ <= current || 0x73 != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    if(end_ <= current || 0x65 != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    lastElement_ = createBoolean(start, current);
    return current;
}

const_iterator TomlParser::value_array(const_iterator current)
{
    CPPTOML_ASSERT(ArrayOpen == *current);
    ++current;

    if(end_ <= current) {
        return CPPTOML_NULL;
    }

    current = array_values(current);
    if(CPPTOML_NULL == current) {
        return CPPTOML_NULL;
    }
    skipWhitespaceCommentNewline(current, end_);
    if(end_ <= current) {
        return CPPTOML_NULL;
    }
    if(ArrayClose != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    return current;
}

const_iterator TomlParser::array_values(const_iterator current)
{
    u16 elementArray = chunks_.create<Array>();
    for(;;) {
        skipWhitespaceCommentNewline(current, end_);
        if(end_ <= current) {
            return current;
        }

        if(ArrayClose == *current) {
            lastElement_ = elementArray;
            return current;
        }

        current = value(current);
        if(CPPTOML_NULL == current) {
            return CPPTOML_NULL;
        }
        addToArray(elementArray, lastElement_);
        skipWhitespaceCommentNewline(current, end_);
        if(end_ <= current || 0x2C != *current) {
            lastElement_ = elementArray;
            return current;
        }
        ++current;
        if(end_ <= current) {
            return current;
        }
    }
}

const_iterator TomlParser::inline_table(const_iterator current)
{
    CPPTOML_ASSERT(InlineTableOpen == *current);
    ++current;
    skipWhitespaces(current, end_);

    u16 currentKeyTable = currentKeyTable_;
    currentKeyTable_ = createInlineTable();
    current = inline_table_keyvalues(current);
    lastElement_ = currentKeyTable_;
    currentKeyTable_ = currentKeyTable;

    if(end_ <= current || InlineTableClose != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    return current;
}

const_iterator TomlParser::inline_table_keyvalues(const_iterator current)
{
    for(;;) {
        u16 elementKey, elementValue;
        current = keyvalue(elementKey, elementValue, current);
        if(CPPTOML_NULL == current) {
            return CPPTOML_NULL;
        }
        linkKeyValueToTable(currentKeyTable_, elementKey, elementValue);
        skipWhitespaces(current, end_);
        if(end_ <= current || 0x2C != *current) {
            return current;
        }
        ++current;
        skipWhitespaces(current, end_);
        if(end_ <= current) {
            return CPPTOML_NULL;
        }
    }
}

const_iterator TomlParser::value_number(const_iterator current)
{
    //TODO:
    const_iterator start = current;
    switch(*current) {
    case 0x2D:
    case 0x2B:
        if(try_integer(current, false)) {
            lastElement_ = createDecInt(start, current);
            return current;
        }
        if(try_float(current)) {
            lastElement_ = createFloat(start, current);
            return current;
        }
        if(try_special_float(current)) {
            lastElement_ = createSpecialFloat(start, current);
            return current;
        }
        return CPPTOML_NULL;
    case 0x30: {
        s32 base = getBase(current, end_);
        switch(base) {
        case 16:
            current = hex_oct_bin_int(current, isHexDigit);
            break;
        case 10:
            if(try_integer(current, false)) {
                lastElement_ = createDecInt(start, current);
                return current;
            }
            if(try_float(current)) {
                lastElement_ = createFloat(start, current);
                return current;
            }
            if(try_datetime(current)) {
                lastElement_ = createSpecialFloat(start, current);
                return current;
            }
            return CPPTOML_NULL;
        case 8:
            current = hex_oct_bin_int(current, isOctDigit);
            break;
        case 2:
            current = hex_oct_bin_int(current, isBinDigit);
            break;
        default:
            return CPPTOML_NULL;
        }
    }
    case 0x69:
        if(try_inf(current)) {
            lastElement_ = createSpecialFloat(start, current);
            return current;
        }
        return CPPTOML_NULL;
    case 0x6E:
        if(try_nan(current)) {
            lastElement_ = createSpecialFloat(start, current);
            return current;
        }
        return CPPTOML_NULL;
    default:
        if(isDigit(current)) {
            if(try_integer(current, false)) {
                lastElement_ = createDecInt(start, current);
                return current;
            }
            if(try_float(current)) {
                lastElement_ = createFloat(start, current);
                return current;
            }
            if(try_datetime(current)) {
                lastElement_ = createDateTime(start, current);
                return current;
            }
        }
        return CPPTOML_NULL;
    }
}

bool TomlParser::try_integer(const_iterator& current, bool float_int_part)
{
    switch(*current) {
    case 0x2B:
    case 0x2D: {
        const_iterator next = current + 1;
        if(end_ <= next) {
            return false;
        }
        if(try_unsigned_dec(next, float_int_part)) {
            current = next;
            return true;
        }
        return false;
    }
    default:
        return try_unsigned_dec(current, float_int_part);
    }
}

bool TomlParser::try_unsigned_dec(const_iterator& current, bool float_int_part)
{
    const_iterator next = current;
    switch(*next) {
    case 0x30: {
        ++next;
        if(isEndOfValue(next, end_, float_int_part)) {
            current = next;
            return true;
        }
        return false;
    }
    default:
        if(!isNonZero(next)) {
            return false;
        }
        break;
    }

    ++next;
    while(next < end_) {
        if(Underscore == *next) {
            ++next;
            if(end_ <= next || !isDigit(next)) {
                return false;
            }
            continue;

        } else if(isEndOfValue(next, end_, float_int_part)) {
            break;
        } else if(!isDigit(next)) {
            return false;
        }
        ++next;
    }
    current = next;
    return true;
}

bool TomlParser::try_float(const_iterator& current)
{
    const_iterator next = current;
    // integer part
    if(!try_integer(next, true)) {
        return false;
    }
    if(end_ <= next) {
        current = next;
        return true;
    }

    switch(*next) {
    case 'E':
    case 'e': {
        ++next;
        if(try_float_exp_part(next)) {
            current = next;
            return true;
        } else {
            return false;
        }
    }
    case 0x2E: {
        ++next;
        if(try_zero_prefixable_int(next)) {
            current = next;
            return true;
        } else {
            return false;
        }
    }
    default:
        if(isEndOfValue(next, end_, false)) {
            current = next;
            return true;
        }
        return false;
    }
}

bool TomlParser::try_float_exp_part(const_iterator& current)
{
    const_iterator next = current;
    switch(*next) {
    case 0x2B:
    case 0x2D:
        ++next;
        if(end_ <= next) {
            return false;
        }
        break;
    }
    if(try_zero_prefixable_int(next)) {
        current = next;
        return true;
    }
    return false;
}

bool TomlParser::try_zero_prefixable_int(const_iterator& current)
{
    const_iterator next = current;
    while(next < end_) {
        if(Underscore == *next) {
            ++next;
            if(end_ <= next || !isDigit(next)) {
                return false;
            }

        } else if(!isDigit(next)) {
            break;
        }
        ++next;
    }
    current = next;
    return true;
}

bool TomlParser::try_datetime(const_iterator& current)
{
    const_iterator next = current;
    if(!try_fulldate(next)) {
        if(try_partialtime(next)) {
            current = next;
            return true;
        }
        return false;
    }
    if(end_ <= next || !isTimeDelim(next)) {
        return true;
    }
    ++next;
    if(end_ <= next) {
        return false;
    }
    if(try_fulltime(next)) {
        current = next;
        return true;
    }
    if(try_partialtime(next)) {
        current = next;
        return true;
    }
    return false;
}

bool TomlParser::try_fulldate(const_iterator& current)
{
    const_iterator next = current;
    if(!try_year(next)) {
        return false;
    }
    if(end_ <= next) {
        return false;
    }
    if('-' != *next) {
        return false;
    }
    ++next;
    if(end_ <= next) {
        return false;
    }
    if(!try_month(next)) {
        return false;
    }
    if(end_ <= next) {
        return false;
    }
    if('-' != *next) {
        return false;
    }
    ++next;
    if(end_ <= next) {
        return false;
    }
    if(!try_day(next)) {
        return false;
    }
    current = next;
    return true;
}

bool TomlParser::try_fulltime(const_iterator& current)
{
    const_iterator next = current;
    if(!try_partialtime(next)) {
        return false;
    }
    if(!try_timeoffset(next)) {
        return false;
    }
    current = next;
    return true;
}

bool TomlParser::try_partialtime(const_iterator& current)
{
    const_iterator next = current;
    if(!try_hour(next)) {
        return false;
    }
    if(end_ <= next) {
        return false;
    }
    if(':' != *next) {
        return false;
    }
    ++next;
    if(end_ <= next) {
        return false;
    }
    if(!try_minute(next)) {
        return false;
    }
    if(end_ <= next) {
        return false;
    }
    if(':' != *next) {
        return false;
    }
    ++next;
    if(end_ <= next) {
        return false;
    }
    if(!try_second(next)) {
        return false;
    }
    if(end_ <= next || '.' != *next) {
        current = next;
        return true;
    }
    ++next;
    s32 count = 0;
    while(next < end_) {
        if(!isDigit(next)) {
            break;
        }
        ++count;
        ++next;
    }
    if(count <= 0) {
        return false;
    }
    current = next;
    return true;
}

bool TomlParser::try_year(const_iterator& current)
{
    s32 count = countDigit(current, end_);
    if(4 != count) {
        return false;
    }
    current += 4;
    return true;
}

bool TomlParser::try_month(const_iterator& current)
{
    s32 count = countDigit(current, end_);
    if(2 != count) {
        return false;
    }
    s32 month = toInt(current, 2);
    if(month < 1 || 12 < month) {
        return false;
    }
    current += 2;
    return true;
}

bool TomlParser::try_day(const_iterator& current)
{
    s32 count = countDigit(current, end_);
    if(2 != count) {
        return false;
    }
    s32 day = toInt(current, 2);
    if(day < 1 || 31 < day) {
        return false;
    }
    current += 2;
    return true;
}

bool TomlParser::try_hour(const_iterator& current)
{
    s32 count = countDigit(current, end_);
    if(2 != count) {
        return false;
    }
    s32 hour = toInt(current, 2);
    if(23 < hour) {
        return false;
    }
    current += 2;
    return true;
}

bool TomlParser::try_minute(const_iterator& current)
{
    s32 count = countDigit(current, end_);
    if(2 != count) {
        return false;
    }
    s32 minute = toInt(current, 2);
    if(59 < minute) {
        return false;
    }
    current += 2;
    return true;
}

bool TomlParser::try_second(const_iterator& current)
{
    s32 count = countDigit(current, end_);
    if(2 != count) {
        return false;
    }
    s32 second = toInt(current, 2);
    if(59 < second) {
        return false;
    }
    current += 2;
    return true;
}

bool TomlParser::try_timeoffset(const_iterator& current)
{
    if('Z' == *current) {
        ++current;
        return true;
    }
    const_iterator next = current;
    if('+' != *next && '-' != *next) {
        return false;
    }
    ++next;
    if(end_ <= next) {
        return false;
    }
    if(!try_hour(next)) {
        return false;
    }
    if(end_ <= next) {
        return false;
    }
    if(':' != *next) {
        return false;
    }
    ++next;
    if(end_ <= next) {
        return false;
    }
    if(!try_minute(next)) {
        return false;
    }
    current = next;
    return true;
}

bool TomlParser::try_special_float(const_iterator& current)
{
    switch(*current) {
    case 0x2B:
    case 0x2D:
        break;
    case 0x69:
        return try_inf(current);
    case 0x6E:
        return try_nan(current);
    default:
        return false;
    }
    const_iterator next = current + 1;
    if(end_ <= next) {
        return false;
    }
    switch(*next) {
    case 0x69:
        if(try_inf(next)) {
            current = next;
            return true;
        } else {
            return false;
        }
    case 0x6E:
        if(try_nan(next)) {
            current = next;
            return true;
        } else {
            return false;
        }
    default:
        return false;
    }
}

bool TomlParser::try_inf(const_iterator& current)
{
    CPPTOML_ASSERT(0x69 == *current);
    const_iterator next = current + 1;
    if(end_ <= next || 0x6E != *next) {
        return false;
    }
    if(end_ <= next || 0x66 != *next) {
        return false;
    }
    current = next;
    return true;
}

bool TomlParser::try_nan(const_iterator& current)
{
    CPPTOML_ASSERT(0x6E == *current);
    const_iterator next = current + 1;
    if(end_ <= next || 0x61 != *next) {
        return false;
    }
    if(end_ <= next || 0x6E != *next) {
        return false;
    }
    current = next;
    return true;
}

const_iterator TomlParser::hex_oct_bin_int(const_iterator current, NumericalPredicate predicate)
{
    CPPTOML_ASSERT(0x30 == *current);
    CPPTOML_ASSERT((current + 1) < end_);
    char type = current[1];
    const_iterator start = current;
    current += 2;
    if(end_ <= current || !predicate(current)) {
        return CPPTOML_NULL;
    }
    while(current < end_) {
        if(Underscore == *current) {
            ++current;
            if(end_ <= current || !predicate(current)) {
                return CPPTOML_NULL;
            }

        } else if(!predicate(current)) {
            break;
        }
        ++current;
    }
    switch(type) {
    case 0x78:
        lastElement_ = createHexInt(start, current);
        break;
    case 0x6F:
        lastElement_ = createOctInt(start, current);
        break;
    case 0x62:
        lastElement_ = createBinInt(start, current);
        break;
    }
    return current;
}

const_iterator TomlParser::table(const_iterator current)
{
    CPPTOML_ASSERT(StdTableOpen == *current);
    const_iterator next = current + 1;
    if(end_ <= next) {
        return CPPTOML_NULL;
    }
    if(StdTableOpen == *next) {
        return array_table(next);
    } else {
        return std_table(current);
    }
}

const_iterator TomlParser::std_table(const_iterator current)
{
    CPPTOML_ASSERT(StdTableOpen == *current);
    ++current;

    current = key(current);
    if(end_ <= current || current == CPPTOML_NULL || StdTableClose != *current) {
        return CPPTOML_NULL;
    }

    if(currentKeyTable_ == currentTable_){
        currentKeyTable_ = createTable(currentKeyTable_, lastKeySize_, lastKeyPosition_);
        if(End == currentKeyTable_) {
            return CPPTOML_NULL;
        }
    }
    currentTable_ = currentKeyTable_;
    ++current;
    return current;
}

const_iterator TomlParser::array_table(const_iterator current)
{
    CPPTOML_ASSERT(StdTableOpen == *current);
    ++current;

    current = key(current);
    if(end_ <= current || current == CPPTOML_NULL || StdTableClose != *current) {
        return CPPTOML_NULL;
    }
    ++current;
    if(end_ <= current || StdTableClose != *current) {
        return CPPTOML_NULL;
    }

    if(currentKeyTable_ == currentTable_){
        currentKeyTable_ = createArrayTable(currentKeyTable_, lastKeySize_, lastKeyPosition_);
        if(End == currentKeyTable_) {
            return CPPTOML_NULL;
        }
    }
    currentTable_ = currentKeyTable_;
    ++current;
    return current;
}

u16 TomlParser::getTopTable() const
{
    return 0;
}

u32 TomlParser::getPosition(const_iterator current) const
{
    CPPTOML_ASSERT(toml_ <= current);
    return static_cast<u32>(current - toml_);
}

u16 TomlParser::getSize(const_iterator start, const_iterator end)
{
    CPPTOML_ASSERT(start <= end);
    return static_cast<u16>(end - start);
}

u16 TomlParser::createDecInt(const_iterator start, const_iterator end)
{
    u16 index = chunks_.create<Element>();
    Element& element = chunks_[index].element_;
    element.position_ = getPosition(start);
    element.size_ = getSize(start, end);

    //NOTE: check overflow ?
    //TODO: detect 32bits or 64 bits, and unsigned
    const char* str = toml_ + element.position_;
    bool sign = false;
    s64 value = 0;
    for(u16 i = 0; i < element.size_; ++i) {
        switch(str[i]) {
        case Underscore:
            break;
        case 0x2B:
            CPPTOML_ASSERT(i <= 0);
            break;
        case 0x2D:
            CPPTOML_ASSERT(i <= 0);
            sign = true;
            break;
        default:
            CPPTOML_ASSERT(isDigit(str + i));
            value = (value * 10) + static_cast<s64>(str[i] - '0');
            break;
        }
    }
    element.type_ = static_cast<u16>(Type::Int64);
    element.s64_ = (sign) ? -value : value;
    return index;
}

u16 TomlParser::createHexInt(const_iterator start, const_iterator end)
{
    u16 index = chunks_.create<Element>();
    Element& element = chunks_[index].element_;
    element.position_ = getPosition(start);
    element.size_ = getSize(start, end);

    const char* str = toml_ + element.position_;
    CPPTOML_ASSERT(2 < element.size_);
    u64 value = 0;
    for(u16 i = 2; i < element.size_; ++i) {
        switch(str[i]) {
        case Underscore:
            break;
        default:
            CPPTOML_ASSERT(isHexDigit(str + i));
            value = (value << 4) + hexDigitsToU32(str[i]);
            break;
        }
    }
    element.type_ = static_cast<u16>(Type::UInt64);
    element.u64_ = value;
    return index;
}

u16 TomlParser::createOctInt(const_iterator start, const_iterator end)
{
    u16 index = chunks_.create<Element>();
    Element& element = chunks_[index].element_;
    element.position_ = getPosition(start);
    element.size_ = getSize(start, end);

    const char* str = toml_ + element.position_;
    CPPTOML_ASSERT(2 < element.size_);
    u64 value = 0;
    for(u16 i = 2; i < element.size_; ++i) {
        switch(str[i]) {
        case Underscore:
            break;
        default:
            CPPTOML_ASSERT(isOctDigit(str + i));
            value = (value << 3) + octDigitsToU32(str[i]);
            break;
        }
    }
    element.type_ = static_cast<u16>(Type::UInt64);
    element.u64_ = value;
    return index;
}

u16 TomlParser::createBinInt(const_iterator start, const_iterator end)
{
    u16 index = chunks_.create<Element>();
    Element& element = chunks_[index].element_;
    element.position_ = getPosition(start);
    element.size_ = getSize(start, end);

    const char* str = toml_ + element.position_;
    CPPTOML_ASSERT(2 < element.size_);
    u64 value = 0;
    for(u16 i = 2; i < element.size_; ++i) {
        switch(str[i]) {
        case Underscore:
            break;
        default:
            CPPTOML_ASSERT(isBinDigit(str + i));
            value = (value << 1) + binDigitsToU32(str[i]);
            break;
        }
    }
    element.type_ = static_cast<u16>(Type::UInt64);
    element.u64_ = value;
    return index;
}

u16 TomlParser::createFloat(const_iterator start, const_iterator end)
{
    u16 index = chunks_.create<Element>();
    Element& element = chunks_[index].element_;
    element.position_ = getPosition(start);
    element.size_ = getSize(start, end);

    //NOTE: check overflow ?
    //TODO: detect 32bits or 64 bits
    char buffer[32]; //max 24 characters
    const char* str = toml_ + element.position_;
    bool sign = false;
    s32 count = 0;
    for(u16 i = 0; i < element.size_ && count < 24; ++i) {
        switch(str[i]) {
        case Underscore:
            break;
        case 0x2B:
            CPPTOML_ASSERT(i <= 0);
            break;
        case 0x2D:
            CPPTOML_ASSERT(i <= 0);
            sign = true;
            ++count;
            break;
        default:
            buffer[count++] = str[i];
            break;
        }
    }
    buffer[count] = '\0';
    element.type_ = static_cast<u16>(Type::Float);
    element.f64_ = atof(buffer);
    return index;
}

u16 TomlParser::createSpecialFloat(const_iterator start, const_iterator end)
{
    u16 index = chunks_.create<Element>();
    Element& element = chunks_[index].element_;
    element.position_ = getPosition(start);
    element.size_ = getSize(start, end);

    const char* str = toml_ + element.position_;
    s32 count = 0;
    bool sign = false;
    switch(str[0]) {
    case 0x2B:
        ++count;
        break;
    case 0x2D:
        ++count;
        sign = true;
        break;
    }
    switch(str[count]) {
    case 0x69:
        element.type_ = static_cast<u16>(Type::FloatInf);
        element.f64_ = sign
                           ? -std::numeric_limits<f64>::infinity()
                           : std::numeric_limits<f64>::infinity();
        break;
    case 0x6E:
        element.type_ = static_cast<u16>(Type::FloatNan);
        element.f64_ = sign
                           ? -std::numeric_limits<f64>::quiet_NaN()
                           : std::numeric_limits<f64>::quiet_NaN();
        break;
    default:
        CPPTOML_ASSERT(false);
        return End;
    }
    return index;
}

u16 TomlParser::createBoolean(const_iterator start, const_iterator end)
{
    u16 index = chunks_.create<Element>();
    Element& element = chunks_[index].element_;
    element.position_ = getPosition(start);
    element.size_ = getSize(start, end);

    const char* str = toml_ + element.position_;
    switch(str[0]) {
    case 0x74:
        element.type_ = static_cast<u16>(Type::Boolean);
        element.boolean_ = true;
        break;
    case 0x66:
        element.type_ = static_cast<u16>(Type::Boolean);
        element.boolean_ = false;
        break;
    default:
        CPPTOML_ASSERT(false);
        return End;
    }
    return index;
}

u16 TomlParser::createDateTime(const_iterator start, const_iterator end)
{
    u16 index = chunks_.create<Element>();
    Element& element = chunks_[index].element_;
    element.position_ = getPosition(start);
    element.size_ = getSize(start, end);

    element.type_ = static_cast<u16>(Type::DateTime);
    //TODO:
    element.u64_ = 0;
    //const char* str = toml_ + element.position_;
    return index;
}

u16 TomlParser::createString(Type type, const_iterator start, const_iterator end)
{
    u16 index = chunks_.create<Element>();
    Element& element = chunks_[index].element_;
    element.position_ = getPosition(start);
    element.size_ = getSize(start, end);
    element.type_ = static_cast<u16>(type);
    return index;
}

u16 TomlParser::createTable(u16 parent, u16 size, u32 position)
{
    Table& parentTable = chunks_[parent].table_;
    CPPTOML_ASSERT(parentTable.type_ == static_cast<u16>(Type::Table));
    u16 child = parentTable.child_;
    while(End != child) {
        Table& table = chunks_[child].table_;
        const char* name0 = toml_ + position;
        const char* name1 = toml_ + table.position_;
        if(table.size_ == size
           && 0 == strncmp(name0, name1, table.size_)) {

            if(static_cast<u16>(Type::Table) != chunks_[child].keyvalue_.type_
                || End != table.child_) { //Empty table
                return End;
            }
            return child;
        }
        child = table.next_;
    }
    u16 t = chunks_.create<Table>();
    if(End == t) {
        return End;
    }
    Table& newTable = chunks_[t].table_;
    newTable.parent_ = parent;
    newTable.position_ = position;
    newTable.size_ = size;
    linkKeyValueToTable(parent, t, t);
    return t;
}

u16 TomlParser::createInlineTable()
{
    u16 t = chunks_.create<Table>();
    if(End == t) {
        return End;
    }
    Table& newTable = chunks_[t].table_;
    newTable.parent_ = End;
    newTable.position_ = 0;
    newTable.size_ = 0;
    return t;
}

u16 TomlParser::createArrayTable(u16 parent, u16 size, u32 position)
{
    Table& parentTable = chunks_[parent].table_;
    CPPTOML_ASSERT(parentTable.type_ == static_cast<u16>(Type::Table));
    u16 child = parentTable.child_;
    while(End != child) {
        Table& table = chunks_[child].table_;
        const char* name0 = toml_ + position;
        const char* name1 = toml_ + table.position_;
        if(table.size_ == size
           && 0 == strncmp(name0, name1, table.size_)) {

            if(static_cast<u16>(Type::Array) != chunks_[child].keyvalue_.type_) { //expect array
                return End;
            }
            break;
        }
        child = table.next_;
    }
    if(End == child){
        u16 a = chunks_.create<Array>();
        if(End == a) {
            return End;
        }
        Array& newArray = chunks_[a].array_;
        newArray.position_ = position;
        newArray.size_ = size;
        linkKeyValueToTable(parent, a, a);
        child = a;
    }
    u16 t = chunks_.create<Table>();
    if(End == t){
        return End;
    }

    Table& newTable = chunks_[t].table_;
    newTable.parent_ = parent;
    newTable.position_ = position;
    newTable.size_ = size;
    addToArray(child, t);
    return t;
}

void TomlParser::linkKeyValueToTable(u16 parent, u16 key, u16 value)
{
    Table& parentTable = chunks_[parent].table_;
    CPPTOML_ASSERT(parentTable.type_ == static_cast<u16>(Type::Table));

    Element& elementKey = chunks_[key].element_;
    Element& elementValue = chunks_[value].element_;

    u16 child = parentTable.child_;
    while(End != child) {
        KeyValue& elementChild = chunks_[child].keyvalue_;
        const char* name0 = toml_ + elementChild.position_;
        const char* name1 = toml_ + elementKey.position_;
        if(elementChild.size_ == elementKey.size_
           && 0 == strncmp(name0, name1, elementChild.size_)) {
            return;
        }
        child = elementChild.next_;
    }

    u16 index = chunks_.create<KeyValue>();
    KeyValue& keyvalue = chunks_[index].keyvalue_;
    keyvalue.position_ = elementKey.position_;
    keyvalue.size_ = elementKey.size_;
    keyvalue.type_ = elementValue.type_;
    keyvalue.value_ = value;
    keyvalue.parent_ = parent;

    if(End == parentTable.child_) {
        parentTable.child_ = index;
        return;
    }
    child = parentTable.child_;
    while(End != chunks_[child].keyvalue_.next_) {
        child = chunks_[child].keyvalue_.next_;
    }
    chunks_[child].keyvalue_.next_ = index;
}

void TomlParser::addToArray(u16 parent, u16 element)
{
    CPPTOML_ASSERT(Type::Array == static_cast<Type>(chunks_[parent].array_.type_));
    while(End != chunks_[parent].array_.next_) {
        parent = chunks_[parent].array_.next_;
    }
    Array& array = chunks_[parent].array_;
    u16* values = array.values();
    for(s32 i = 0; i < 3; ++i) {
        if(End == values[i]) {
            values[i] = element;
            return;
        }
    }
    u16 newArray = chunks_.create<Array>();
    array.next_ = newArray;
    addToArray(newArray, element);
}

const_iterator TomlParser::setElement(Element& element, const_iterator start, const_iterator current)
{
    element.position_ = getPosition(start);
    element.size_ = getSize(start, current);
    return current;
}

} // namespace cpptoml
