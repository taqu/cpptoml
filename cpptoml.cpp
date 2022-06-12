// clang-format off
/*
# License
This software is distributed under two licenses, choose whichever you like.

## MIT License
Copyright (c) 2022 Takuro Sakai

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

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
// clang-format on
/**
@author t-sakai
*/
#include "cpptoml.h"
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <limits>

#ifdef CPPTOML_DEBUG
#    include <cassert>
#    define CPPTOML_ASSERT(exp) assert((exp))
#else
#    define CPPTOML_ASSERT(exp)
#endif

namespace cpptoml
{
namespace
{
    void* cpptoml_default_malloc(size_t size, void*)
    {
        return ::malloc(size);
    }

    void cpptoml_default_free(void* mem, void*)
    {
        ::free(mem);
    }
} // namespace

//---------------------------------------
TomlType TomlValueProxy::type() const
{
    return parser_->get_value(value_).type_;
}

TomlStringProxy TomlKeyValueProxy::key() const
{
    const TomlValue& key = parser_->get_value(key_);
    return {true, key.positionLength_.length_, parser_->str(key.positionLength_.position_)};
}

TomlValueProxy TomlKeyValueProxy::value() const
{
    TomlValueProxy value;
    value.parser_ = parser_;
    value.value_ = value_;
    return value;
}

u32 TomlArrayProxy::size() const
{
    return parser_->get_value(index_).array_.size_;
}

TomlArrayProxy::Iterator TomlArrayProxy::begin() const
{
    return parser_->get_value(index_).array_.head_;
}

TomlArrayProxy::Iterator TomlArrayProxy::next(Iterator iterator) const
{
    return parser_->get_value(iterator).next_;
}

TomlArrayProxy::Iterator TomlArrayProxy::end() const
{
    return Invalid;
}

TomlValueProxy TomlArrayProxy::operator[](Iterator iterator) const
{
    TomlValueProxy proxy;
    proxy.parser_ = parser_;
    proxy.value_ = iterator;
    return proxy;
}

u32 TomlTableProxy::size() const
{
    return parser_->get_value(index_).table_.size_;
}

TomlTableProxy::Iterator TomlTableProxy::begin() const
{
    return parser_->get_value(index_).table_.head_;
}

TomlTableProxy::Iterator TomlTableProxy::next(Iterator iterator) const
{
    return parser_->get_value(iterator).next_;
}

TomlTableProxy::Iterator TomlTableProxy::end() const
{
    return Invalid;
}

TomlKeyValueProxy TomlTableProxy::operator[](Iterator iterator) const
{
    const TomlKeyValue& keyvalue = parser_->get_value(iterator).keyvalue_;
    TomlKeyValueProxy proxy;
    proxy.parser_ = parser_;
    proxy.key_ = keyvalue.key_;
    proxy.value_ = keyvalue.value_;
    return proxy;
}

u32 TomlArrayTableProxy::size() const
{
    return parser_->get_value(index_).table_.size_;
}

TomlArrayTableProxy::Iterator TomlArrayTableProxy::begin() const
{
    return parser_->get_value(index_).table_.head_;
}

TomlArrayTableProxy::Iterator TomlArrayTableProxy::next(Iterator iterator) const
{
    return parser_->get_value(iterator).next_;
}

TomlArrayTableProxy::Iterator TomlArrayTableProxy::end() const
{
    return Invalid;
}

TomlValueProxy TomlArrayTableProxy::operator[](Iterator iterator) const
{
    TomlValueProxy proxy;
    proxy.parser_ = parser_;
    proxy.value_ = iterator;
    return proxy;
}

//---------------------------------------
TomlParser::TomlParser(cpptoml_malloc allocator, cpptoml_free deallocator, void* user_data)
    : allocator_(allocator)
    , deallocator_(deallocator)
    , user_data_(user_data)
    , begin_(CPPTOML_NULL)
    , current_(CPPTOML_NULL)
    , end_(CPPTOML_NULL)
    , capacity_(0)
    , size_(0)
    , buffer_(CPPTOML_NULL)
    , table_(0)
{
    CPPTOML_ASSERT((CPPTOML_NULL != allocator && CPPTOML_NULL != deallocator) || (CPPTOML_NULL == allocator && CPPTOML_NULL == deallocator));
    if(CPPTOML_NULL == allocator_) {
        allocator_ = cpptoml_default_malloc;
    }
    if(CPPTOML_NULL == deallocator_) {
        deallocator_ = cpptoml_default_free;
    }
}

TomlParser::~TomlParser()
{
    deallocator_(buffer_, user_data_);
}

bool TomlParser::parse(cursor head, cursor end)
{
    CPPTOML_ASSERT(head <= end);
    begin_ = current_ = head;
    end_ = end;
    size_ = 0;
    current_ = skip_bom(current_);
    table_ = 0;
    {
        u32 index = create_table();
        CPPTOML_ASSERT(0 == index);
        (void)index;
        reset_table();
    }
    while(current_ < end_) {
        current_ = parse_expression(current_);
        if(CPPTOML_NULL == current_) {
            return false;
        }
        current_ = skip_newline(current_);
    }
    reset_table();
    return end_ <= current_;
}

void TomlParser::clear()
{
    size_ = 0;
}

TomlTableProxy TomlParser::top() const
{
    return {this, table_};
}

bool TomlParser::is_alpha(UChar c)
{
    return (0x41U <= c && c <= 0x5AU) || (0x61U <= c && c <= 0x7AU);
}

bool TomlParser::is_digit(UChar c)
{
    return 0x30U <= c && c <= 0x39U;
}

bool TomlParser::is_hexdigit(UChar c)
{
    return is_digit(c) || (0x41U <= c && c <= 0x46U) || (0x61U <= c && c <= 0x66U);
}

bool TomlParser::is_digit19(UChar c)
{
    return 0x31U <= c && c <= 0x39U;
}

bool TomlParser::is_digit07(UChar c)
{
    return 0x30U <= c && c <= 0x37U;
}

bool TomlParser::is_digit01(UChar c)
{
    return 0x30U <= c && c <= 0x31U;
}

bool TomlParser::is_whitespace(UChar c)
{
    return 0x20U == c || 0x09U == c;
}

bool TomlParser::is_basicchar(UChar c)
{
    if(is_whitespace(c)
       || 0x21U == c
       || (0x23U <= c && c <= 0x5BU)
       || (0x5DU <= c && c <= 0x7EU)) {
        return true;
    }
    return false;
}

bool TomlParser::is_newline(UChar c)
{
    return 0x0AU == c || 0x0DU == c;
}

bool TomlParser::is_quated_key(UChar c)
{
    if(0x22U == c
       || 0x27U == c) {
        return true;
    }
    return false;
}

bool TomlParser::is_unquated_key(UChar c)
{
    if(is_alpha(c)
       || is_digit(c)
       || 0x2DU == c
       || 0x5FU == c) {
        return true;
    }
    return false;
}

bool TomlParser::is_table(UChar c)
{
    return 0x5BU == c;
}

bool TomlParser::is_minus(f64 x)
{
    return x < 0.0;
}

bool TomlParser::check_overflow(s64 x0, s64 x1, s64 x2)
{
    if(0 <= x1 && x0 < x2) {
        return false;
    }
    if(x1 < 0 && x2 < x0) {
        return false;
    }
    return true;
}

bool TomlParser::mul(s64& x, s64 base)
{
    s64 t = x;
    x *= base;
    if(0 <= t) {
        return t <= x;
    } else {
        return x <= t;
    }
}

bool TomlParser::add(s64& x, s64 x0, s64 x1)
{
    if(0 <= x0) {
        x = x0 + x1;
    } else {
        x = x0 - x1;
    }
    if(0 <= x0 && x < x1) {
        return false;
    }
    if(x0 < 0 && x1 < x) {
        return false;
    }
    return true;
}

s64 TomlParser::from_digit(UChar c)
{
    return c - 0x30U;
}

s64 TomlParser::from_hex(UChar c)
{
    if(is_digit(c)) {
        return c - 0x30U;
    }
    if(0x41U <= c && c <= 0x46U) {
        return c - 0x41U;
    }
    if(0x61U <= c && c <= 0x66U) {
        return c - 0x61U;
    }
    return 0;
}

TomlParser::cursor TomlParser::skip_bom(cursor str)
{
#ifdef CPPTOML_CPP
    size_t size = std::distance(str, end_);
#else
    size_t size = (size_t)(str - end_);
#endif
    if(3 <= size) {
        u8 c0 = *reinterpret_cast<const u8*>(&str[0]);
        u8 c1 = *reinterpret_cast<const u8*>(&str[1]);
        u8 c2 = *reinterpret_cast<const u8*>(&str[2]);
        if(0xEFU == c0 && 0xBBU == c1 && 0xBFU == c2) {
            str += 3;
        }
    }
    return str;
}

TomlParser::cursor TomlParser::skip_newline(cursor str)
{
    if(str < end_) {
        switch(str[0]) {
        case 0x0AU:
            ++str;
            break;
        case 0x0DU:
            ++str;
            if(str < end_ && 0x0AU == str[0]) {
                ++str;
            }
            break;
        default:
            return str;
        }
    }
    return str;
}

TomlParser::cursor TomlParser::skip_spaces(cursor str)
{
    while(str < end_) {
        switch(str[0]) {
        case 0x09U:
        case 0x20U:
            ++str;
            break;
        default:
            return str;
        }
    }
    return str;
}

TomlParser::cursor TomlParser::skip_utf8_1(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    static const u8 mask0 = 0b11000000U;
    static const u8 value0 = 0b10000000U;
    if(end_ <= (str + 1)) {
        return CPPTOML_NULL;
    }
    ++str;
    return value0 == (str[0] & mask0) ? str : end_;
}

TomlParser::cursor TomlParser::skip_utf8_2(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    static const u8 mask0 = 0b11000000U;
    static const u8 value0 = 0b10000000U;
    if(end_ <= (str + 2)) {
        return CPPTOML_NULL;
    }
    ++str;
    if(value0 != (str[0] & mask0)) {
        return CPPTOML_NULL;
    }
    ++str;
    return value0 == (str[0] & mask0) ? str : end_;
}

TomlParser::cursor TomlParser::skip_utf8_3(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    static const u8 mask0 = 0b11000000U;
    static const u8 value0 = 0b10000000U;
    if(end_ <= (str + 3)) {
        return CPPTOML_NULL;
    }
    ++str;
    if(value0 != (str[0] & mask0)) {
        return CPPTOML_NULL;
    }
    ++str;
    if(value0 != (str[0] & mask0)) {
        return CPPTOML_NULL;
    }
    ++str;
    return value0 == (str[0] & mask0) ? str : end_;
}

TomlParser::cursor TomlParser::skip_comment(cursor str)
{
    if(end_ <= str || '#' != str[0]) {
        return str;
    }
    ++str;
    while(str < end_) {
        cursor end = parse_non_eol(str);
        if(CPPTOML_NULL == end) {
            break;
        }
        str = end;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_expression(cursor str)
{
    str = skip_spaces(str);
    if(end_ <= str) {
        return str;
    }
    if(is_quated_key(str[0]) || is_unquated_key(str[0])) {
        str = parse_keyval(str);
        if(CPPTOML_NULL == str) {
            return str;
        }
    } else if(is_table(str[0])) {
        reset_table();
        str = parse_table(str);
        if(CPPTOML_NULL == str) {
            return str;
        }
    }
    str = skip_spaces(str);
    str = skip_comment(str);
    if(end_ <= str || is_newline(str[0])) {
        return str;
    } else {
        return CPPTOML_NULL;
    }
}

TomlParser::cursor TomlParser::parse_keyval(cursor str)
{
    Result k = parse_key(str);
    if(CPPTOML_NULL == k.cursor_) {
        return CPPTOML_NULL;
    }
    u32 table = table_;
    k.index_ = traverse_tables(k.index_, TomlType::None);
    if(Invalid == k.index_) {
        return CPPTOML_NULL;
    }
    str = parse_keyval_sep(k.cursor_);
    if(CPPTOML_NULL == str) {
        return CPPTOML_NULL;
    }
    Result v = parse_val(str);
    if(CPPTOML_NULL == v.cursor_) {
        return CPPTOML_NULL;
    }
    table_ = table;
    add_table_value(table_, create_keyvalue(k.index_, v.index_));
    return v.cursor_;
}

TomlParser::Result TomlParser::parse_key(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    cursor begin = str;

    if(is_quated_key(str[0])) {
        str = parse_quated_key(str);
    } else if(is_unquated_key(str[0])) {
        str = parse_unquated_key(str);
    } else {
        return {CPPTOML_NULL, 0};
    }
    if(CPPTOML_NULL == str) {
        return {CPPTOML_NULL, 0};
    }
    u32 index = create_key(begin, str);
    cursor dot_sep = parse_dot_sep(str);
    if(CPPTOML_NULL != dot_sep) {
        TomlParser::Result result = parse_key(dot_sep);
        if(CPPTOML_NULL == result.cursor_) {
            return result;
        }
        str = result.cursor_;
        get_value(index).next_ = result.index_;
    }
    return {str, index};
}

TomlParser::cursor TomlParser::parse_dot_sep(cursor str)
{
    str = skip_spaces(str);
    if(end_ <= str || 0x2EU != str[0]) {
        return CPPTOML_NULL;
    }
    ++str;
    str = skip_spaces(str);
    return str;
}

TomlParser::cursor TomlParser::parse_keyval_sep(cursor str)
{
    str = skip_spaces(str);
    if(end_ <= str || 0x3DU != str[0]) {
        return CPPTOML_NULL;
    }
    ++str;
    str = skip_spaces(str);
    return str;
}

//
TomlParser::Result TomlParser::parse_val(cursor str)
{
    CPPTOML_ASSERT(str < end_);

    cursor val;
    val = parse_string(str);
    if(CPPTOML_NULL != val) {
        u32 index = create_string(str, val);
        return {val, index};
    }

    val = parse_boolean(str);
    if(CPPTOML_NULL != val) {
        u32 index = create_value(str, val, TomlType::Boolean);
        return {val, index};
    }

    Result result;
    result = parse_array(str);
    if(CPPTOML_NULL != result.cursor_) {
        return result;
    }

    result = parse_inline_table(str);
    if(CPPTOML_NULL != result.cursor_) {
        return result;
    }

    val = parse_date_time(str);
    if(CPPTOML_NULL != val) {
        u32 index = create_value(str, val, TomlType::DateTime);
        return {val, index};
    }

    ResultFloat fval = parse_float(str);
    if(CPPTOML_NULL != fval.cursor_) {
        u32 index = create_float(fval.value_);
        return {fval.cursor_, index};
    }

    ResultInt ival = parse_integer(str);
    if(CPPTOML_NULL != ival.cursor_) {
        u32 index = create_int(ival.value_);
        return {ival.cursor_, index};
    }
    return {CPPTOML_NULL, 0};
}

TomlParser::cursor TomlParser::parse_quated_key(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x22U == str[0]) {
        cursor next = parse_basic_string(str);
        if(CPPTOML_NULL != next) {
            return next;
        }
    } else if(0x27U == str[0]) {
        cursor next = parse_literal_string(str);
        if(CPPTOML_NULL != next) {
            return next;
        }
    }
    return CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_unquated_key(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    cursor begin = str;
    while(str < end_) {
        if(is_alpha(str[0])
           || is_digit(str[0])
           || 0x2DU == str[0]
           || 0x5FU == str[0]) {
            ++str;
        } else {
            break;
        }
    }
    return (begin < str) ? str : CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_basic_string(cursor str)
{
    CPPTOML_ASSERT(0x22U == str[0]);
    ++str;
    while(str < end_) {
        if(0x22U == str[0]) {
            ++str;
            return str;
        }
        str = parse_basic_char(str);
        if(CPPTOML_NULL == str) {
            break;
        }
    }
    return CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_basic_char(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x5CU == str[0]) {
        return parse_escaped(str);
    }
    if(is_whitespace(str[0])
       || 0x21U == str[0]
       || (0x23U <= str[0] && str[0] <= 0x5BU)
       || (0x5DU <= str[0] && str[0] <= 0x7EU)) {
        ++str;
        return str;
    }
    return parse_non_ascii(str);
}

TomlParser::cursor TomlParser::parse_non_ascii(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    static const u8 mask1 = 0b11100000U;
    static const u8 mask2 = 0b11110000U;
    static const u8 mask3 = 0b11111000U;
    static const u8 value1 = 0b11000000U;
    static const u8 value2 = 0b11100000U;
    static const u8 value3 = 0b11110000U;

    if(value1 == (str[0] & mask1)) {
        str = skip_utf8_1(str);
        if(CPPTOML_NULL == str) {
            return CPPTOML_NULL;
        }
        ++str;
    } else if(value2 == (*current_ & mask2)) {
        str = skip_utf8_2(str);
        if(CPPTOML_NULL == str) {
            return CPPTOML_NULL;
        }
        ++str;
    } else if(value3 == (*current_ & mask3)) {
        str = skip_utf8_3(str);
        if(CPPTOML_NULL == str) {
            return CPPTOML_NULL;
        }
        ++str;
    } else {
        str = CPPTOML_NULL;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_non_eol(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x09U == str[0]) {
        ++str;
    } else if(0x20U <= str[0] && str[0] <= 0x7FU) {
        ++str;
    } else {
        str = parse_non_ascii(str);
    }
    return str;
}

TomlParser::cursor TomlParser::parse_escaped(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    CPPTOML_ASSERT(0x5CU == str[0]);
    ++str;
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    switch(str[0]) {
    case 0x22U:
    case 0x5CU:
    case 0x62U:
    case 0x65U:
    case 0x66U:
    case 0x6EU:
    case 0x72U:
    case 0x74U:
        ++str;
        break;
    case 0x75U:
        str = parse_4HEXDIG(str);
        break;
    case 0x55U:
        str = parse_8HEXDIG(str);
        break;
    default:
        str = CPPTOML_NULL;
        break;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_4HEXDIG(cursor str)
{
    CPPTOML_ASSERT(0x75U == str[0]);
    ++str;
    if(end_ <= (str + 4)) {
        return CPPTOML_NULL;
    }
    for(u32 i = 0; i < 4; ++i, ++str) {
        if(!is_hexdigit(str[0])) {
            return CPPTOML_NULL;
        }
    }
    return str;
}

TomlParser::cursor TomlParser::parse_8HEXDIG(cursor str)
{
    CPPTOML_ASSERT(0x55U == str[0]);
    ++str;
    if(end_ <= (str + 8)) {
        return CPPTOML_NULL;
    }
    for(u32 i = 0; i < 8; ++i, ++str) {
        if(!is_hexdigit(str[0])) {
            return CPPTOML_NULL;
        }
    }
    return str;
}

TomlParser::cursor TomlParser::parse_literal_string(cursor str)
{
    CPPTOML_ASSERT(0x27U == str[0]);
    ++str;
    while(str < end_) {
        if(0x27U == str[0]) {
            ++str;
            return str;
        }
        str = parse_literal_char(str);
        if(CPPTOML_NULL == str) {
            break;
        }
    }
    return CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_literal_char(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x09U == str[0]
       || (0x20U <= str[0] && str[0] <= 0x26U)
       || (0x28U <= str[0] && str[0] <= 0x7EU)) {
        ++str;
        return str;
    }
    return parse_non_ascii(str);
}

TomlParser::cursor TomlParser::parse_string(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x22U == str[0]) {
        if((str + 2) < end_ && 0x22U == str[1] && 0x22U == str[2]) {
            return parse_ml_basic_string(str);
        } else {
            return parse_basic_string(str);
        }
    }
    if(0x27U == str[0]) {
        if((str + 2) < end_ && 0x27U == str[1] && 0x27U == str[2]) {
            return parse_ml_literal_string(str);
        } else {
            return parse_literal_string(str);
        }
    }
    return CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_ml_basic_string(cursor str)
{
    str += 3;
    str = skip_newline(str);
    str = parse_ml_basic_body(str);
    if(CPPTOML_NULL == str) {
        return str;
    }
    if(end_ <= (str + 3)) {
        return CPPTOML_NULL;
    }
    if(0x22U != str[0] || 0x22U != str[1] || 0x22U != str[2]) {
        return CPPTOML_NULL;
    }
    return str + 3;
}

TomlParser::cursor TomlParser::parse_ml_basic_body(cursor str)
{
    for(;;) {
        cursor content = parse_mlb_content(str);
        if(CPPTOML_NULL == content || end_ <= str || 0x22U == str[0]) {
            break;
        }
        str = content;
    }

    for(;;) {
        cursor quotes = parse_mlb_quotes(str);
        if(CPPTOML_NULL == quotes) {
            break;
        }
        str = quotes;
        cursor content = parse_mlb_content(str);
        if(CPPTOML_NULL == content) {
            return CPPTOML_NULL;
        }
        for(;;) {
            content = parse_mlb_content(str);
            if(CPPTOML_NULL == content) {
                break;
            }
            str = content;
        }
    }
    {
        cursor quotes = parse_mlb_quotes(str);
        if(CPPTOML_NULL != quotes) {
            str = quotes;
        }
    }
    return str;
}

TomlParser::cursor TomlParser::parse_mlb_quotes(cursor str)
{
    u32 count = 0;
    while(str < end_) {
        if(0x22U != str[0]) {
            break;
        }
        ++count;
        ++str;
    }
    return 1 <= count && count <= 2 ? str : CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_mlb_content(cursor str)
{
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    switch(str[0]) {
    case 0x0AU:
    case 0x0DU:
        return skip_newline(str);
    case 0x5CU:
        return parse_mlb_escaped_nl(str);
    default:
        return parse_basic_char(str);
    }
}

TomlParser::cursor TomlParser::parse_mlb_escaped_nl(cursor str)
{
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    if(0x5CU != str[0]) {
        return CPPTOML_NULL;
    }
    ++str;
    str = skip_spaces(str);
    str = skip_newline(str);

    while(str < end_) {
        switch(str[0]) {
        case 0x09U:
        case 0x20U:
        case 0x0AU:
        case 0x0DU:
            ++str;
            break;
        default:
            return str;
        }
    }
    return str;
}

TomlParser::cursor TomlParser::parse_ml_literal_string(cursor str)
{
    str += 3;
    str = skip_newline(str);
    str = parse_ml_literal_body(str);
    if(CPPTOML_NULL == str) {
        return str;
    }
    if(end_ <= (str + 3)) {
        return CPPTOML_NULL;
    }
    if(0x27U != str[0] || 0x27U != str[1] || 0x27U != str[2]) {
        return CPPTOML_NULL;
    }
    return str + 3;
}

TomlParser::cursor TomlParser::parse_ml_literal_body(cursor str)
{
    for(;;) {
        cursor content = parse_mll_content(str);
        if(CPPTOML_NULL == content || end_ <= str || 0x27U == str[0]) {
            break;
        }
        str = content;
    }

    for(;;) {
        cursor quotes = parse_mll_quotes(str);
        if(CPPTOML_NULL == quotes) {
            break;
        }
        str = quotes;
        cursor content = parse_mll_content(str);
        if(CPPTOML_NULL == content) {
            return CPPTOML_NULL;
        }
        for(;;) {
            content = parse_mll_content(str);
            if(CPPTOML_NULL == content) {
                break;
            }
            str = content;
        }
    }
    {
        cursor quotes = parse_mll_quotes(str);
        if(CPPTOML_NULL != quotes) {
            str = quotes;
        }
    }
    return str;
}

TomlParser::cursor TomlParser::parse_mll_quotes(cursor str)
{
    u32 count = 0;
    while(str < end_) {
        if(0x27U != str[0]) {
            break;
        }
        ++count;
        ++str;
    }
    return 1 <= count && count <= 2 ? str : CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_mll_content(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    switch(str[0]) {
    case 0x0AU:
    case 0x0DU:
        return skip_newline(str);
    default:
        return parse_literal_char(str);
    }
}

TomlParser::cursor TomlParser::parse_boolean(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x74U == str[0]) {
        if((str + 4) < end_ && 0x72U == str[1] && 0x75U == str[2] && 0x65U == str[3]) {
            return str + 4;
        }
        return CPPTOML_NULL;
    }
    if(0x66U == str[0]) {
        if((str + 5) < end_ && 0x61U == str[1] && 0x6CU == str[2] && 0x73U == str[3] && 0x65U == str[4]) {
            return str + 5;
        }
        return CPPTOML_NULL;
    }
    return CPPTOML_NULL;
}

TomlParser::Result TomlParser::parse_array(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x5BU != str[0]) {
        return {CPPTOML_NULL, 0};
    }
    ++str;
    u32 array = create_array();
    cursor val;
    val = parse_array_values(str, array);
    if(CPPTOML_NULL != val) {
        str = val;
    }
    str = parse_ws_comment_newline(str);
    if(CPPTOML_NULL == str || end_ <= str || 0x5DU != str[0]) {
        return {CPPTOML_NULL, 0};
    }
    if(!check_array_mixed_types(array)) {
        return {CPPTOML_NULL, 0};
    }
    return {str + 1, array};
}

TomlParser::cursor TomlParser::parse_array_values(cursor str, u32 array)
{
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    str = parse_ws_comment_newline(str);
    if(CPPTOML_NULL == str) {
        return CPPTOML_NULL;
    }

    Result next;
    next = parse_val(str);
    if(CPPTOML_NULL != next.cursor_) {
        str = next.cursor_;
        add_array_value(array, next.index_);
    }
    str = parse_ws_comment_newline(str);
    if(CPPTOML_NULL != str && str < end_ && 0x2CU == str[0]) {
        ++str;
        return parse_array_values(str, array);
    }
    return str;
}

TomlParser::cursor TomlParser::parse_ws_comment_newline(cursor str)
{
    while(str < end_) {
        switch(str[0]) {
        case 0x09U:
        case 0x20U:
            ++str;
            break;
        case 0x23U:
            str = skip_comment(str);
            break;
        case 0x0AU:
        case 0x0DU: {
            cursor newline = skip_newline(str);
            if(str == newline) {
                return CPPTOML_NULL;
            }
            str = newline;
        } break;
        default:
            return str;
        }
    }
    return str;
}

TomlParser::Result TomlParser::parse_inline_table(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x7BU != str[0]) {
        return {nullptr, 0};
    }
    ++str;
    str = skip_spaces(str);

    u32 prev_table = table_;
    u32 table = create_table();
    table_ = table;

    cursor val;
    val = parse_inline_table_keyvals(str);
    if(CPPTOML_NULL != val) {
        str = val;
    }
    table_ = prev_table;
    str = skip_spaces(str);
    if(end_ <= str || 0x7DU != str[0]) {
        return {nullptr, 0};
    }
    return {str + 1, table};
}

TomlParser::cursor TomlParser::parse_inline_table_keyvals(cursor str)
{
    if(end_ <= str) {
        return str;
    }
    if(!is_quated_key(str[0]) && !is_unquated_key(str[0])) {
        return str;
    }
    str = parse_keyval(str);
    if(CPPTOML_NULL == str) {
        return str;
    }
    str = skip_spaces(str);
    if(str < end_ && 0x2CU == str[0]) {
        ++str;
        str = skip_spaces(str);
        return parse_inline_table_keyvals(str);
    }
    return str;
}

TomlParser::cursor TomlParser::parse_table(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    CPPTOML_ASSERT(is_table(str[0]));
    if(end_ <= (str + 1)) {
        return CPPTOML_NULL;
    }
    if(0x5B == str[1]) {
        return parse_array_table(str);
    }
    return parse_std_table(str);
}

TomlParser::cursor TomlParser::parse_date_time(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    cursor val;
    val = parse_offset_date_time(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    val = parse_local_date_time(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    val = parse_local_date(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    val = parse_local_time(str);
    if(CPPTOML_NULL != val) {
        return val;
    }
    return CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_offset_date_time(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    str = parse_full_date(str);
    if(CPPTOML_NULL == str) {
        return CPPTOML_NULL;
    }
    str = parse_time_delim(str);
    if(CPPTOML_NULL == str) {
        return CPPTOML_NULL;
    }
    return parse_full_time(str);
}

TomlParser::cursor TomlParser::parse_local_date_time(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    str = parse_full_date(str);
    if(CPPTOML_NULL == str) {
        return CPPTOML_NULL;
    }
    str = parse_time_delim(str);
    if(CPPTOML_NULL == str) {
        return CPPTOML_NULL;
    }
    return parse_partial_time(str);
}

TomlParser::cursor TomlParser::parse_local_date(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    return parse_full_date(str);
}

TomlParser::cursor TomlParser::parse_local_time(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    return parse_partial_time(str);
}

TomlParser::cursor TomlParser::parse_time_delim(cursor str)
{
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    switch(str[0]) {
    case 0x20U:
    case 'T':
    case 't':
        ++str;
        break;
    default:
        return CPPTOML_NULL;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_full_date(cursor str)
{
    str = parse_date_fullyear(str);
    if(str == CPPTOML_NULL || end_ <= str || '-' != str[0]) {
        return CPPTOML_NULL;
    }
    ++str;

    str = parse_date_month(str);
    if(str == CPPTOML_NULL || end_ <= str || '-' != str[0]) {
        return CPPTOML_NULL;
    }
    ++str;

    return parse_date_mday(str);
}

TomlParser::cursor TomlParser::parse_full_time(cursor str)
{
    str = parse_partial_time(str);
    if(str == CPPTOML_NULL || end_ <= str) {
        return CPPTOML_NULL;
    }
    switch(str[0]) {
    case 'Z':
    case 'z':
        ++str;
        break;
    case '+':
    case '-': {
        ++str;
        str = parse_time_hour(str);
        if(str == CPPTOML_NULL || end_ <= str || ':' != str[0]) {
            return CPPTOML_NULL;
        }
        ++str;
        str = parse_time_minute(str);
    } break;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_partial_time(cursor str)
{
    str = parse_time_hour(str);
    if(str == CPPTOML_NULL || end_ <= str || ':' != str[0]) {
        return CPPTOML_NULL;
    }
    ++str;

    str = parse_time_minute(str);
    if(str == CPPTOML_NULL || end_ <= str || ':' != str[0]) {
        return CPPTOML_NULL;
    }
    ++str;

    str = parse_time_second(str);
    if(str == CPPTOML_NULL || end_ <= str) {
        return str;
    }
    if('.' == str[0]) {
        ++str;
        return parse_time_secfrac(str);
    }
    return str;
}

TomlParser::cursor TomlParser::parse_date_fullyear(cursor str)
{
    if(end_ <= (str + 4U)) {
        return CPPTOML_NULL;
    }
    for(u32 i = 0; i < 4U; ++i, ++str) {
        if(!is_digit(str[0])) {
            return CPPTOML_NULL;
        }
    }
    return str;
}

TomlParser::cursor TomlParser::parse_date_month(cursor str)
{
    if(end_ <= (str + 2U)) {
        return CPPTOML_NULL;
    }
    if(str[0] < '0' || '1' < str[0]) {
        return CPPTOML_NULL;
    }
    if(str[1] < '0' || '9' < str[1]) {
        return CPPTOML_NULL;
    }
    return str + 2U;
}

TomlParser::cursor TomlParser::parse_date_mday(cursor str)
{
    if(end_ <= (str + 2U)) {
        return CPPTOML_NULL;
    }
    if(str[0] < '0' || '3' < str[0]) {
        return CPPTOML_NULL;
    }
    if(str[1] < '0' || '9' < str[1]) {
        return CPPTOML_NULL;
    }
    return str + 2U;
}

TomlParser::cursor TomlParser::parse_time_hour(cursor str)
{
    if(end_ <= (str + 2U)) {
        return CPPTOML_NULL;
    }
    if(str[0] < '0' || '2' < str[0]) {
        return CPPTOML_NULL;
    }
    if(str[1] < '0' || '9' < str[1]) {
        return CPPTOML_NULL;
    }
    return str + 2U;
}

TomlParser::cursor TomlParser::parse_time_minute(cursor str)
{
    if(end_ <= (str + 2U)) {
        return CPPTOML_NULL;
    }
    if(str[0] < '0' || '5' < str[0]) {
        return CPPTOML_NULL;
    }
    if(str[1] < '0' || '9' < str[1]) {
        return CPPTOML_NULL;
    }
    return str + 2U;
}

TomlParser::cursor TomlParser::parse_time_second(cursor str)
{
    if(end_ <= (str + 2U)) {
        return CPPTOML_NULL;
    }
    if(str[0] < '0' || '6' < str[0]) {
        return CPPTOML_NULL;
    }
    if(str[1] < '0' || '9' < str[1]) {
        return CPPTOML_NULL;
    }
    return str + 2U;
}

TomlParser::cursor TomlParser::parse_time_secfrac(cursor str)
{
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    if(!is_digit(str[1])) {
        return CPPTOML_NULL;
    }
    ++str;
    while(str < end_) {
        if(!is_digit(str[0])) {
            break;
        }
        ++str;
    }
    return str;
}

TomlParser::ResultFloat TomlParser::parse_float(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    ResultFloat val;
    val = parse_special_float(str);
    if(CPPTOML_NULL != val.cursor_) {
        return val;
    }

    val = parse_float_int_part(str);
    if(CPPTOML_NULL == val.cursor_ || end_ <= val.cursor_) {
        return {CPPTOML_NULL, 0.0};
    }
    str = val.cursor_;
    f64 value = val.value_;
    switch(str[0]) {
    case 'e':
    case 'E': {
        ResultInt exp = parse_exp(str);
        if(CPPTOML_NULL == exp.cursor_) {
            return {CPPTOML_NULL, 0.0};
        }
        value *= ::pow(10.0, exp.value_);
        return {exp.cursor_, value};
    }
    case 0x2EU: {
        val = parse_frac(str);
        if(CPPTOML_NULL == val.cursor_) {
            return {CPPTOML_NULL, 0.0};
        }
        if(is_minus(value)) {
            value -= val.value_;
        } else {
            value += val.value_;
        }
        str = val.cursor_;
        if(str < end_) {
            switch(str[0]) {
            case 'e':
            case 'E': {
                ResultInt exp = parse_exp(str);
                if(CPPTOML_NULL == exp.cursor_) {
                    return {CPPTOML_NULL, 0.0};
                }
                str = exp.cursor_;
                value *= ::pow(10.0, exp.value_);
                break;
            }
            };
        }
        return {str, value};
    }
    }
    return {CPPTOML_NULL, 0.0};
}

TomlParser::ResultFloat TomlParser::parse_special_float(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    bool minus = false;
    switch(str[0]) {
    case 0x2BU:
        ++str;
        break;
    case 0x2DU:
        minus = true;
        ++str;
        break;
    default:
        break;
    }
    if(end_ <= (str + 2)) {
        return {CPPTOML_NULL, 0.0};
    }
    if(0x69U == str[0] || 0x6EU == str[1] || 0x66U == str[2]) {
        return {str + 3, minus ? -std::numeric_limits<f64>::infinity() : std::numeric_limits<f64>::infinity()};
    }
    if(0x6EU == str[0] || 0x61U == str[1] || 0x6EU == str[2]) {
        return {str + 3, std::numeric_limits<f64>::quiet_NaN()};
    }
    return {CPPTOML_NULL, 0.0};
}

TomlParser::ResultFloat TomlParser::parse_float_int_part(cursor str)
{
    ResultInt ivalue = parse_dec_int(str);
    if(CPPTOML_NULL == ivalue.cursor_) {
        return {CPPTOML_NULL, 0.0};
    }
    return {ivalue.cursor_, static_cast<f64>(ivalue.value_)};
}

TomlParser::ResultInt TomlParser::parse_dec_int(cursor str)
{
    if(end_ <= str) {
        return {CPPTOML_NULL, 0};
    }
    bool minus = false;
    switch(str[0]) {
    case 0x2BU:
        ++str;
        break;
    case 0x2DU:
        minus = true;
        ++str;
        break;
    }
    return parse_unsigned_dec_int(str, minus);
}

TomlParser::ResultInt TomlParser::parse_unsigned_dec_int(cursor str, bool minus)
{
    if(end_ <= str) {
        return {CPPTOML_NULL, 0};
    }
    s64 value = 0;
    switch(str[0]) {
    case 0x30U:
        ++str;
        if(end_ <= str) {
            return {CPPTOML_NULL, 0};
        }
        if(is_digit(str[0]) || 0x5FU == str[0]) {
            return {CPPTOML_NULL, 0};
        }
        return {str, value};
    default:
        if(!is_digit19(str[0])) {
            return {CPPTOML_NULL, 0};
        }
        value = from_digit(str[0]);
        if(minus) {
            value = -value;
        }
        ++str;
        break;
    }
    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return {CPPTOML_NULL, 0};
            }
            if(!is_digit(str[0])) {
                return {CPPTOML_NULL, 0};
            }
        } else if(!is_digit(str[0])) {
            break;
        }
        s64 x = from_digit(str[0]);
        if(!mul(value, 10) || !add(value, value, x)) {
            return {CPPTOML_NULL, 0};
        }
        ++str;
    }
    return {str, value};
}

TomlParser::ResultInt TomlParser::parse_exp(cursor str)
{
    CPPTOML_ASSERT('e' == str[0] || 'E' == str[0]);
    ++str;
    return parse_float_exp_part(str);
}

TomlParser::ResultInt TomlParser::parse_float_exp_part(cursor str)
{
    if(end_ <= str) {
        return {CPPTOML_NULL, 0};
    }
    bool minus = false;
    switch(str[0]) {
    case 0x2BU:
        ++str;
        break;
    case 0x2DU:
        minus = true;
        ++str;
        break;
    }
    ResultInt value = parse_zero_prefixable_int(str);
    if(CPPTOML_NULL != value.cursor_ && minus) {
        value.value_ = -value.value_;
    }
    return value;
}

TomlParser::ResultInt TomlParser::parse_zero_prefixable_int(cursor str)
{
    if(end_ <= str || !is_digit(str[0])) {
        return {CPPTOML_NULL, 0};
    }
    s64 value = from_digit(str[0]);
    ++str;
    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return {CPPTOML_NULL, 0};
            }
            if(!is_digit(str[0])) {
                return {CPPTOML_NULL, 0};
            }
        } else if(!is_digit(str[0])) {
            break;
        }
        s64 x = from_digit(str[0]);
        if(!add(value, value, x)) {
            return {CPPTOML_NULL, 0};
        }
        ++str;
    }
    if(-1022 <= value && value <= 1023) {
        return {str, value};
    }
    return {CPPTOML_NULL, 0};
}

TomlParser::ResultFloat TomlParser::parse_frac(cursor str)
{
    CPPTOML_ASSERT(0x2EU == str[0]);
    ++str;
    if(end_ <= str) {
        return {CPPTOML_NULL, 0};
    }
    f64 frac = 1.0;
    f64 value = 0.0;
    u32 count = 0;
    while(str < end_) {
        if(0x5FU == str[0]) {
            if(count <= 0) { // invalid underscore after point
                return {CPPTOML_NULL, 0};
            }
            ++str;
            if(end_ <= str) {
                return {CPPTOML_NULL, 0};
            }
            if(!is_digit(str[0])) {
                return {CPPTOML_NULL, 0};
            }
        } else if(!is_digit(str[0])) {
            if(count <= 0) {
                return {CPPTOML_NULL, 0};
            }
            break;
        }
        frac *= 0.1;
        value += frac * from_digit(str[0]);
        ++str;
        ++count;
    }
    return {str, value};
}

TomlParser::ResultInt TomlParser::parse_integer(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    switch(str[0]) {
    case 0x30U:
        if((str + 1) < end_) {
            ResultInt result;
            switch(str[1]) {
            case 0x78U:
                result = parse_hex_prefix(str);
                break;
            case 0x6FU:
                result = parse_oct_prefix(str);
                break;
            case 0x62U:
                result = parse_bin_prefix(str);
                break;
            default:
                return parse_dec_int(str);
            }
            return result;
        }
        break;
    }
    return parse_dec_int(str);
}

TomlParser::ResultInt TomlParser::parse_hex_prefix(cursor str)
{
    CPPTOML_ASSERT(str < end_ && 0x78U != str[0]);
    ++str;
    if(end_ <= str) {
        return {CPPTOML_NULL, 0};
    }
    if(!is_hexdigit(str[0])) {
        return {CPPTOML_NULL, 0};
    }

    s64 value = 0;
    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return {CPPTOML_NULL, 0};
            }
            if(!is_hexdigit(str[0])) {
                return {CPPTOML_NULL, 0};
            }
        } else if(!is_hexdigit(str[0])) {
            break;
        }
        s64 x = from_hex(str[0]);
        if(!mul(value, 16) || !add(value, value, x)) {
            return {CPPTOML_NULL, 0};
        }
        ++str;
    }
    return {str, value};
}

TomlParser::ResultInt TomlParser::parse_oct_prefix(cursor str)
{
    CPPTOML_ASSERT(str < end_ && 0x6FU != str[0]);
    ++str;
    if(end_ <= str) {
        return {CPPTOML_NULL, 0};
    }
    if(!is_digit07(str[0])) {
        return {CPPTOML_NULL, 0};
    }

    s64 value = 0;
    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return {CPPTOML_NULL, 0};
            }
            if(!is_digit07(str[0])) {
                return {CPPTOML_NULL, 0};
            }
        } else if(!is_digit07(str[0])) {
            break;
        }
        s64 x = from_digit(str[0]);
        if(!mul(value, 8) || !add(value, value, x)) {
            return {CPPTOML_NULL, 0};
        }
        ++str;
    }
    return {str, value};
}

TomlParser::ResultInt TomlParser::parse_bin_prefix(cursor str)
{
    CPPTOML_ASSERT(str < end_ && 0x62U != str[0]);
    ++str;
    if(end_ <= str) {
        return {CPPTOML_NULL, 0};
    }
    if(!is_digit01(str[0])) {
        return {CPPTOML_NULL, 0};
    }

    s64 value = 0;
    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return {CPPTOML_NULL, 0};
            }
            if(!is_digit01(str[0])) {
                return {CPPTOML_NULL, 0};
            }
        } else if(!is_digit01(str[0])) {
            break;
        }
        s64 x = from_digit(str[0]);
        if(!mul(value, 2) || !add(value, value, x)) {
            return {CPPTOML_NULL, 0};
        }
        ++str;
    }
    return {str, value};
}

TomlParser::cursor TomlParser::parse_std_table(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    CPPTOML_ASSERT(is_table(str[0]));
    ++str;
    str = skip_spaces(str);
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    Result next = parse_key(str);
    if(CPPTOML_NULL == next.cursor_) {
        return CPPTOML_NULL;
    }
    if(!validate_table_key_chain(next.index_)) {
        return CPPTOML_NULL;
    }

    str = skip_spaces(next.cursor_);
    if(end_ <= str || 0x5DU != str[0]) {
        return CPPTOML_NULL;
    }
    if(Invalid == traverse_tables(next.index_, TomlType::Table)) {
        return CPPTOML_NULL;
    }
    return str + 1;
}

TomlParser::cursor TomlParser::parse_array_table(cursor str)
{
    CPPTOML_ASSERT((str + 1) < end_);
    CPPTOML_ASSERT(is_table(str[0]) && is_table(str[1]));
    str += 2;
    str = skip_spaces(str);
    Result next = parse_key(str);
    if(CPPTOML_NULL == next.cursor_) {
        return CPPTOML_NULL;
    }
    if(!validate_array_key_chain(next.index_)) {
        return CPPTOML_NULL;
    }

    str = skip_spaces(next.cursor_);
    if(end_ <= str || 0x5DU != str[0]) {
        return CPPTOML_NULL;
    }
    ++str;
    if(end_ <= str || 0x5DU != str[0]) {
        return CPPTOML_NULL;
    }
    if(Invalid == traverse_tables(next.index_, TomlType::ArrayTable)) {
        return CPPTOML_NULL;
    }
    return str + 1;
}

u32 TomlParser::length(cursor begin, cursor end) const
{
    return static_cast<u32>(std::distance(begin, end));
}

u32 TomlParser::create_value(cursor begin, cursor end, TomlType type)
{
    u32 index = allocate();
    TomlValue& value = get_value(index);
    value.next_ = Invalid;
    value.type_ = type;
    value.positionLength_.position_ = length(begin_, begin);
    value.positionLength_.length_ = length(begin, end);
    return index;
}

u32 TomlParser::create_key(cursor begin, cursor end)
{
    if(0x22U == begin[0] || 0x27U == begin[0]) {
        CPPTOML_ASSERT(0x22U == end[-1] || 0x27U == end[-1]);
        ++begin;
        --end;
    }
    u32 index = allocate();
    TomlValue& value = get_value(index);
    value.next_ = Invalid;
    value.type_ = TomlType::Key;
    value.positionLength_.position_ = length(begin_, begin);
    value.positionLength_.length_ = length(begin, end);
    return index;
}

u32 TomlParser::create_keyvalue(u32 key, u32 value)
{
    u32 index = allocate();
    TomlValue& x = get_value(index);
    x.next_ = Invalid;
    x.type_ = TomlType::KeyValue;
    x.keyvalue_.key_ = static_cast<u16>(key);
    x.keyvalue_.value_ = static_cast<u16>(value);
    return index;
}

u32 TomlParser::create_table()
{
    u32 index = allocate();
    TomlValue& value = get_value(index);
    value.next_ = Invalid;
    value.type_ = TomlType::Table;
    value.table_.size_ = 0;
    value.table_.head_ = Invalid;
    return index;
}

u32 TomlParser::create_array_table()
{
    u32 index = allocate();
    TomlValue& value = get_value(index);
    value.next_ = Invalid;
    value.type_ = TomlType::ArrayTable;
    value.arrayTable_.size_ = 0;
    value.arrayTable_.head_ = Invalid;
    return index;
}

u32 TomlParser::create_array()
{
    u32 index = allocate();
    TomlValue& value = get_value(index);
    value.next_ = Invalid;
    value.type_ = TomlType::Array;
    value.array_.size_ = 0;
    value.array_.head_ = Invalid;
    return index;
}

u32 TomlParser::create_string(cursor begin, cursor end)
{
    while(begin < end && (0x22U == begin[0] || 0x27U == begin[0])) {
        ++begin;
        --end;
    }
    u32 index = allocate();
    TomlValue& value = get_value(index);
    value.next_ = Invalid;
    value.type_ = TomlType::String;
    value.positionLength_.position_ = length(begin_, begin);
    value.positionLength_.length_ = length(begin, end);
    return index;
}

u32 TomlParser::create_float(f64 x)
{
    u32 index = allocate();
    TomlValue& value = get_value(index);
    value.next_ = Invalid;
    value.type_ = TomlType::Float;
    value.float_ = x;
    return index;
}

u32 TomlParser::create_int(s64 x)
{
    u32 index = allocate();
    TomlValue& value = get_value(index);
    value.next_ = Invalid;
    value.type_ = TomlType::Integer;
    value.int_ = x;
    return index;
}

const TomlValue& TomlParser::get_value(u32 index) const
{
    CPPTOML_ASSERT(index < size_);
    return buffer_[index];
}

TomlValue& TomlParser::get_value(u32 index)
{
    CPPTOML_ASSERT(index < size_);
    return buffer_[index];
}

const char* TomlParser::str(u32 position) const
{
    return reinterpret_cast<const char*>(begin_ + position);
}

u32 TomlParser::allocate()
{
    u32 capacity = (size_ + 1) * sizeof(TomlValue);
    if(capacity_ < capacity) {
        while(capacity_ < capacity) {
            capacity_ += ExpandSize;
        }
        TomlValue* buffer = reinterpret_cast<TomlValue*>(allocator_(capacity_, user_data_));
        ::memcpy(buffer, buffer_, size_);
        deallocator_(buffer_, user_data_);
        buffer_ = buffer;
    }
    u32 result = size_;
    ++size_;
    return result;
}

void TomlParser::reset_table()
{
    table_ = 0;
    array_table_ = Invalid;
}

void TomlParser::add_table_value(u32 table, u32 value)
{
    TomlValue& target = get_value(table);
    ++target.table_.size_;
    u32 current = target.table_.head_;
    while(current != Invalid) {
        TomlValue& x = get_value(current);
        if(Invalid == x.next_) {
            x.next_ = static_cast<u16>(value);
            return;
        }
        current = x.next_;
    }
    target.table_.head_ = static_cast<u16>(value);
}

void TomlParser::add_array_value(u32 array, u32 value)
{
    TomlValue& target = get_value(array);
    ++target.array_.size_;
    u32 current = target.array_.head_;
    while(current != Invalid) {
        TomlValue& x = get_value(current);
        if(Invalid == x.next_) {
            x.next_ = static_cast<u16>(value);
            return;
        }
        current = x.next_;
    }
    target.array_.head_ = static_cast<u16>(value);
}

TomlStringProxy TomlParser::get_string(u32 index) const
{
    const TomlValue& value = get_value(index);
    if(TomlType::String == value.type_) {
        return {true, value.positionLength_.length_, str(value.positionLength_.position_)};
    }
    return {false, 0, CPPTOML_NULL};
}

namespace
{
    s32 c_decimal_int(s32 c)
    {
        return c - 0x30U;
    }

    bool parse_date_time_internal(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_offset_date_time(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_local_date_time(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_local_date(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_local_time(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_time_delim(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_full_date(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_full_time(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_partial_time(TomlDateTimeProxy& datetime, const char*& current, const char* end);

    bool parse_date_fullyear(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_date_month(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_date_mday(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_time_hour(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_time_minute(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_time_second(TomlDateTimeProxy& datetime, const char*& current, const char* end);
    bool parse_time_secfrac(TomlDateTimeProxy& datetime, const char*& current, const char* end);

    bool parse_date_time_internal(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(end <= current) {
            return false;
        }
        if(parse_offset_date_time(datetime, current, end)) {
            datetime.type_ = TomlDateTimeProxy::Type::OffsetDateTime;
            return true;
        }

        if(parse_local_date_time(datetime, current, end)) {
            datetime.type_ = TomlDateTimeProxy::Type::LocalDateTime;
            return true;
        }

        if(parse_local_date(datetime, current, end)) {
            datetime.type_ = TomlDateTimeProxy::Type::LocalDate;
            return true;
        }

        if(parse_local_time(datetime, current, end)) {
            datetime.type_ = TomlDateTimeProxy::Type::LocalTime;
            return true;
        }
        return false;
    }

    bool parse_offset_date_time(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        CPPTOML_ASSERT(current <= end);
        if(!parse_full_date(datetime, current, end)) {
            return false;
        }
        if(!parse_time_delim(datetime, current, end)) {
            return false;
        }
        return parse_full_time(datetime, current, end);
    }

    bool parse_local_date_time(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        CPPTOML_ASSERT(current <= end);
        if(!parse_full_date(datetime, current, end)) {
            return false;
        }
        if(!parse_time_delim(datetime, current, end)) {
            return false;
        }
        return parse_partial_time(datetime, current, end);
    }

    bool parse_local_date(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        return parse_full_date(datetime, current, end);
    }

    bool parse_local_time(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        return parse_partial_time(datetime, current, end);
    }

    bool parse_time_delim(TomlDateTimeProxy&, const char*& current, const char* end)
    {
        if(end <= current) {
            return false;
        }
        switch(current[0]) {
        case 0x20U:
        case 'T':
        case 't':
            ++current;
            return true;
        default:
            return false;
        }
    }

    bool parse_full_date(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(!parse_date_fullyear(datetime, current, end)) {
            return false;
        }
        if(end <= current || '-' != *current) {
            return false;
        }
        ++current;

        if(!parse_date_month(datetime, current, end)) {
            return false;
        }
        if(end <= current || '-' != *current) {
            return false;
        }
        ++current;
        return parse_date_mday(datetime, current, end);
    }

    bool parse_full_time(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(!parse_partial_time(datetime, current, end) || end <= current) {
            return false;
        }
        switch(current[0]) {
        case 'Z':
        case 'z':
            ++current;
            break;
        case '+':
        case '-':
            ++current;
            parse_time_hour(datetime, current, end);
            if(!parse_time_hour(datetime, current, end) || end <= current || ':' != current[0]) {
                return false;
            }
            ++current;
            return parse_time_minute(datetime, current, end);
        default:
            break;
        }
        return true;
    }

    bool parse_partial_time(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(!parse_time_hour(datetime, current, end) || end <= current || ':' != current[0]) {
            return false;
        }
        ++current;

        if(!parse_time_minute(datetime, current, end) || end <= current || ':' != current[0]) {
            return false;
        }
        ++current;

        if(!parse_time_second(datetime, current, end)) {
            return false;
        }
        ++current;
        if(end <= current) {
            return true;
        }
        if('.' == current[0]) {
            ++current;
            return parse_time_secfrac(datetime, current, end);
        }
        return true;
    }

    bool parse_date_fullyear(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(end <= (current + 4U)) {
            return false;
        }
        s32 year = 0;
        for(u32 i = 0; i < 4U; ++i, ++current) {
            if(0x30 <= current[i] && current[i] <= 0x39) {
                year += (3 - i) * 10 * c_decimal_int(current[i]);
                continue;
            }
            return false;
        }
        datetime.year_ = static_cast<s16>(year);
        return true;
    }

    bool parse_date_month(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(end <= (current + 2U)) {
            return false;
        }
        if(current[0] < '0' || '1' < current[0]) {
            return false;
        }
        datetime.month_ = 10 * c_decimal_int(current[0]);
        if(current[1] < '0' || '9' < current[1]) {
            return false;
        }
        datetime.month_ += c_decimal_int(current[1]);
        current += 2;
        return true;
    }

    bool parse_date_mday(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(end <= (current + 2U)) {
            return false;
        }
        if(current[0] < '0' || '3' < current[0]) {
            return false;
        }
        datetime.day_ = 10 * c_decimal_int(current[0]);
        if(current[1] < '0' || '9' < current[1]) {
            return false;
        }
        datetime.day_ += c_decimal_int(current[1]);
        current += 2;
        return true;
    }

    bool parse_time_hour(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(end <= (current + 2U)) {
            return false;
        }
        if(current[0] < '0' || '2' < current[0]) {
            return false;
        }
        datetime.hour_ = 10 * c_decimal_int(current[0]);
        if(current[1] < '0' || '9' < current[1]) {
            return false;
        }
        datetime.hour_ += c_decimal_int(current[1]);
        current += 2;
        return true;
    }

    bool parse_time_minute(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(end <= (current + 2U)) {
            return false;
        }
        if(current[0] < '0' || '5' < current[0]) {
            return false;
        }
        datetime.minute_ = 10 * c_decimal_int(current[0]);
        if(current[1] < '0' || '9' < current[1]) {
            return false;
        }
        datetime.minute_ += c_decimal_int(current[1]);
        current += 2;
        return true;
    }

    bool parse_time_second(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(end <= (current + 2U)) {
            return false;
        }
        if(current[0] < '0' || '6' < current[0]) {
            return false;
        }
        datetime.second_ = 10 * c_decimal_int(current[0]);
        if(current[1] < '0' || '9' < current[1]) {
            return false;
        }
        datetime.second_ += c_decimal_int(current[1]);
        current += 2;
        return true;
    }

    bool parse_time_secfrac(TomlDateTimeProxy& datetime, const char*& current, const char* end)
    {
        if(end <= current) {
            return false;
        }
        s32 millisecond = 0;
        s32 unit = 1000'000;
        while(current < end) {
            if(current[0] < '0' || '9' < current[0]) {
                break;
            }
            if(unit <= 0) {
                break;
            }
            unit /= 10;
            millisecond += unit * (c_decimal_int(current[0]));
            ++current;
        }
        datetime.millisecond_ = millisecond;
        return true;
    }
} // namespace

TomlDateTimeProxy TomlParser::get_datetime(u32 index) const
{
    const TomlValue& value = get_value(index);
    if(TomlType::DateTime != value.type_) {
        return {false};
    }
    const char* begin = str(value.positionLength_.position_);
    const char* end = begin + value.positionLength_.length_;
    TomlDateTimeProxy datetime = {};
    datetime.valid_ = parse_date_time_internal(datetime, begin, end);
    return datetime;
}

TomlFloatProxy TomlParser::get_float(u32 index) const
{
    const TomlValue& value = get_value(index);
    if(TomlType::Float != value.type_) {
        return {false, 0.0};
    }
    return {true, value.float_};
}

TomlIntProxy TomlParser::get_int(u32 index) const
{
    const TomlValue& value = get_value(index);
    if(TomlType::Integer != value.type_) {
        return {false, 0};
    }
    return {true, value.int_};
}

TomlBoolProxy TomlParser::get_bool(u32 index) const
{
    const TomlValue& value = get_value(index);
    if(TomlType::Boolean != value.type_ || value.positionLength_.length_ < 4) {
        return {false, false};
    }
    const char* begin = str(value.positionLength_.position_);
    switch(begin[0]) {
    case 0x74U:
        if(0x72U == begin[1] && 0x75U == begin[2] && 0x65U == begin[3]) {
            return {true, true};
        }
        break;
    case 0x66U:
        if(5 <= value.positionLength_.length_ && 0x61U == begin[1] && 0x6CU == begin[2] && 0x73U == begin[3] && 0x65U == begin[4]) {
            return {true, false};
        }
        break;
    default:
        break;
    }
    return {false, false};
}

bool TomlParser::equal(const TomlPositionLength& x0, const TomlPositionLength& x1) const
{
    if(x0.length_ != x1.length_) {
        return false;
    }
    const char* s0 = str(x0.position_);
    const char* s1 = str(x1.position_);
    return 0 == ::strncmp(s0, s1, x0.length_);
}

bool TomlParser::compare_key_chain(u32 key0, u32 key1) const
{
    CPPTOML_ASSERT(Invalid != key0);
    CPPTOML_ASSERT(Invalid != key1);
    for(;;) {
        const TomlValue& x0 = get_value(key0);
        const TomlValue& x1 = get_value(key1);
        if(!equal(x0.positionLength_, x1.positionLength_)) {
            return false;
        }
        key0 = x0.next_;
        key1 = x1.next_;
        if(Invalid != key0 || Invalid != key1) {
            break;
        }
        if(Invalid == key0 && Invalid == key1) {
            return true;
        }
    }
    return false;
}

u32 TomlParser::traverse_tables(u32 key_chain, TomlType type)
{
    const TomlValue& key = get_value(key_chain);
    TomlTableProxy table = top();
    for(u32 itr = table.begin(); itr != table.end(); itr = table.next(itr)) {
        CPPTOML_ASSERT(TomlType::KeyValue == get_value(itr).type_);
        TomlKeyValueProxy keyvalue = table[itr];
        switch(keyvalue.value().type()) {
        case TomlType::Table: {
            const TomlValue& k = get_value(keyvalue.getKeyIndex());
            if(equal(k.positionLength_, key.positionLength_)) {
                if(type != TomlType::Table) {
                    return Invalid;
                } else if(Invalid == key.next_) {
                    return key_chain;
                } else {
                    table_ = keyvalue.getValueIndex();
                    return traverse_tables(key.next_, type);
                }
            }
        } break;
        case TomlType::ArrayTable: {
            const TomlValue& k = get_value(keyvalue.getKeyIndex());
            if(equal(k.positionLength_, key.positionLength_)) {
                if(Invalid == key.next_) {
                    array_table_ = keyvalue.getValueIndex();
                    table_ = create_table();
                    add_array_value(array_table_, table_);
                    return key_chain;
                } else {
                    array_table_ = keyvalue.getValueIndex();
                    table_ = get_last_table(array_table_);
                    return traverse_tables(key.next_, type);
                }
            }
        } break;
        default: {
            const TomlValue& x0 = get_value(keyvalue.getKeyIndex());
            if(equal(x0.positionLength_, key.positionLength_)) {
                return Invalid;
            }
        } break;
        }
    } // for(u32 itr
    switch(type) {
    case TomlType::Table: {
        u32 new_table = create_table();
        add_table_value(table_, create_keyvalue(key_chain, new_table));
        table_ = new_table;
    } break;
    case TomlType::ArrayTable:
        if(Invalid == key.next_) {
            array_table_ = create_array_table();
            add_table_value(table_, create_keyvalue(key_chain, array_table_));
            table_ = create_table();
            add_array_value(array_table_, table_);
        } else {
            u32 new_table = create_table();
            add_table_value(table_, create_keyvalue(key_chain, new_table));
            table_ = new_table;
        }
        break;
    default:
        break;
    }
    return Invalid != key.next_ ? traverse_tables(key.next_, type) : key_chain;
}

u32 TomlParser::get_last_table(u32 index)
{
    TomlArrayTableProxy array_table = {this, index};
    u32 result = index;
    for(u32 itr = array_table.begin(); itr != array_table.end(); itr = array_table.next(itr)) {
        result = itr;
    }
    return result;
}

u32 TomlParser::find_array_table(u32 key_chain) const
{
    CPPTOML_ASSERT(Invalid != key_chain);
    TomlTableProxy table = top();
    for(u32 itr = table.begin(); itr != table.end(); itr = table.next(itr)) {
        TomlKeyValueProxy value = table[itr];
        if(TomlType::ArrayTable != value.value().type()) {
            continue;
        }
        if(compare_key_chain(value.getKeyIndex(), key_chain)) {
            return itr;
        }
    }
    return Invalid;
}

bool TomlParser::validate_table_key_chain(u32 key_chain) const
{
    TomlTableProxy table = top();
    for(u32 itr = table.begin(); itr != table.end(); itr = table.next(itr)) {
        TomlKeyValueProxy value = table[itr];
        if(compare_key_chain(value.getKeyIndex(), key_chain)) {
            return false;
        }
    }
    return true;
}

bool TomlParser::validate_array_key_chain(u32 key_chain) const
{
    TomlTableProxy table = top();
    for(u32 itr = table.begin(); itr != table.end(); itr = table.next(itr)) {
        TomlKeyValueProxy value = table[itr];
        if(value.value().type() == TomlType::ArrayTable) {
            continue;
        }
        if(compare_key_chain(value.getKeyIndex(), key_chain)) {
            return false;
        }
    }
    return true;
}

bool TomlParser::check_array_mixed_types(u32 index) const
{
    TomlArrayProxy array = {this, index};
    TomlType type = TomlType::None;
    for(u32 itr = array.begin(); itr != array.end(); itr = array.next(itr)) {
        TomlValueProxy value = array[itr];
        if(type == TomlType::None) {
            type = value.type();
        } else if(type != value.type()) {
            return false;
        }
    }
    return true;
}

//--- TomlValueProxy
//---------------------------------------
template<>
TomlStringProxy TomlValueProxy::value<TomlStringProxy>() const
{
    return parser_->get_string(value_);
}

template<>
TomlDateTimeProxy TomlValueProxy::value<TomlDateTimeProxy>() const
{
    return parser_->get_datetime(value_);
}

template<>
TomlFloatProxy TomlValueProxy::value<TomlFloatProxy>() const
{
    return parser_->get_float(value_);
}

template<>
TomlIntProxy TomlValueProxy::value<TomlIntProxy>() const
{
    return parser_->get_int(value_);
}

template<>
TomlBoolProxy TomlValueProxy::value<TomlBoolProxy>() const
{
    return parser_->get_bool(value_);
}

template<>
TomlArrayProxy TomlValueProxy::value<TomlArrayProxy>() const
{
    return {parser_, value_};
}

template<>
TomlTableProxy TomlValueProxy::value<TomlTableProxy>() const
{
    return {parser_, value_};
}

template<>
TomlArrayTableProxy TomlValueProxy::value<TomlArrayTableProxy>() const
{
    return {parser_, value_};
}

//--- TomlProxy
//---------------------------------------
bool TomlStringProxy::equals(u32 length, const char* str) const
{
    CPPTOML_ASSERT(CPPTOML_NULL != str);
    return (length == length_) && (0 == ::strncmp(str, str_, length_));
}

//
template<>
bool TomlTableProxy::tryGet<s8>(s8& x, const char* name) const
{
    s64 t;
    if(tryGet(t, name)) {
        x = static_cast<s8>(t);
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<s16>(s16& x, const char* name) const
{
    s64 t;
    if(tryGet(t, name)) {
        x = static_cast<s16>(t);
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<s32>(s32& x, const char* name) const
{
    s64 t;
    if(tryGet(t, name)) {
        x = static_cast<s32>(t);
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<s64>(s64& x, const char* name) const
{
    CPPTOML_ASSERT(CPPTOML_NULL != name);
    u32 length = static_cast<u32>(::strlen(name));
    for(TomlTableProxy::Iterator itr = begin(); itr != end(); itr = next(itr)) {
        TomlKeyValueProxy keyvalue = (*this)[itr];
        if(!keyvalue.key().equals(length, name)) {
            continue;
        }
        TomlValueProxy value = keyvalue.value();
        if(TomlType::Integer != value.type()) {
            continue;
        }
        TomlIntProxy i = value.value<TomlIntProxy>();
        if(!i.valid_) {
            return false;
        }
        x = i.value_;
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<u8>(u8& x, const char* name) const
{
    s64 t;
    if(tryGet(t, name)) {
        x = static_cast<u8>(t);
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<u16>(u16& x, const char* name) const
{
    s64 t;
    if(tryGet(t, name)) {
        x = static_cast<u16>(t);
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<u32>(u32& x, const char* name) const
{
    s64 t;
    if(tryGet(t, name)) {
        x = static_cast<u32>(t);
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<u64>(u64& x, const char* name) const
{
    s64 t;
    if(tryGet(t, name)) {
        x = static_cast<u64>(t);
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<f32>(f32& x, const char* name) const
{
    f64 t;
    if(tryGet(t, name)) {
        x = static_cast<f32>(t);
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<f64>(f64& x, const char* name) const
{
    CPPTOML_ASSERT(CPPTOML_NULL != name);
    u32 length = static_cast<u32>(::strlen(name));
    for(TomlTableProxy::Iterator itr = begin(); itr != end(); itr = next(itr)) {
        TomlKeyValueProxy keyvalue = (*this)[itr];
        if(!keyvalue.key().equals(length, name)) {
            continue;
        }
        TomlValueProxy value = keyvalue.value();
        if(TomlType::Float == value.type()) {
            TomlFloatProxy f = value.value<TomlFloatProxy>();
            if(!f.valid_) {
                return false;
            }
            x = f.value_;
            return true;
        }
        if(TomlType::Integer == value.type()) {
            TomlIntProxy i = value.value<TomlIntProxy>();
            if(!i.valid_) {
                return false;
            }
            x = static_cast<f64>(i.value_);
            return true;
        }
        return false;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<TomlStringProxy>(TomlStringProxy& x, const char* name) const
{
    CPPTOML_ASSERT(CPPTOML_NULL != name);
    u32 length = static_cast<u32>(::strlen(name));
    for(TomlTableProxy::Iterator itr = begin(); itr != end(); itr = next(itr)) {
        TomlKeyValueProxy keyvalue = (*this)[itr];
        if(!keyvalue.key().equals(length, name)) {
            continue;
        }
        TomlValueProxy value = keyvalue.value();
        if(TomlType::String != value.type()) {
            continue;
        }
        TomlStringProxy s = value.value<TomlStringProxy>();
        if(!s.valid_) {
            return false;
        }
        x = s;
        return true;
    }
    return false;
}

template<>
bool TomlTableProxy::tryGet<TomlTableProxy>(TomlTableProxy& x, const char* name) const
{
    CPPTOML_ASSERT(CPPTOML_NULL != name);
    u32 length = static_cast<u32>(::strlen(name));
    for(TomlTableProxy::Iterator itr = begin(); itr != end(); itr = next(itr)) {
        TomlKeyValueProxy keyvalue = (*this)[itr];
        if(!keyvalue.key().equals(length, name)) {
            continue;
        }
        TomlValueProxy value = keyvalue.value();
        if(TomlType::Table != value.type()) {
            continue;
        }
        x = value.value<TomlTableProxy>();
        return true;
    }
    return false;
}
} // namespace cpptoml
