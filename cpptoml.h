#ifndef INC_CPPTOML_H_
#define INC_CPPTOML_H_
/**
@file cpptoml.h
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
#include <cstdint>
#include <vector>

namespace cpptoml
{
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#ifdef _DEBUG
#    define CPPTOML_DEBUG
#endif

#ifndef CPPTOML_NULL
#    ifdef __cplusplus
#        if 201103L <= __cplusplus || 1700 <= _MSC_VER
#            define CPPTOML_NULL nullptr
#        else
#            define CPPTOML_NULL 0
#        endif
#    else
#        define CPPTOML_NULL (void*)0
#    endif
#endif

typedef void* (*Allocator)(size_t);
typedef void (*Deallocator)(void*);
typedef const char* const_iterator;

extern const char* EmptyString; //="";
static const u16 End = 0xFFFFU;

enum class Type : u16
{
    Unknown = 0,
    DottedKey,
    UnquotedKey,
    Array,
    Table,
    BasicString,
    LiteralString,
    MultiLineBasicString,
    MultiLineLiteralString,
    Boolean,
    DateTime,
    Float,
    FloatInf,
    FloatNan,
    Int32,
    UInt32,
    Int64,
    UInt64,
};

struct DateTime
{
    s64 utc_;
};

struct Element
{
    u32 position_;
    u16 size_;
    u16 type_;
    //u16 next_;
    //u16 prev_;
    //u16 child_;
    //u16 padding_;
    union
    {
        s64 s64_;
        u64 u64_;
        f64 f64_;
        s32 s32_;
        u32 u32_;
        f32 f32_;
        bool boolean_;
    };
};

struct Array
{
    static const s32 OffsetValue = 5;
    const u16* values() const
    {
        return reinterpret_cast<const u16*>(this) + OffsetValue;
    }

    u16* values()
    {
        return reinterpret_cast<u16*>(this) + OffsetValue;
    }

    u32 position_;
    u16 size_;
    u16 type_;
    u16 next_;
    u16 value0_;
    u16 value1_;
    u16 value2_;
};

struct KeyValue
{
    u32 position_; //key name
    u16 size_;
    u16 type_; //value type
    u16 next_;
    u16 parent_;
    u16 value_;
    u16 padding_;
};

struct Table
{
    u32 position_;
    u16 size_;
    u16 type_;
    u16 next_;
    u16 parent_;
    u16 child_;
    u16 padding_;
};

class ValueAccessor;
class ArrayAccessor;
class TomlParser;

ArrayAccessor asArray(const ValueAccessor& value);

//----------------------------------------------------------------------
class ValueAccessor
{
public:
    ValueAccessor();
    bool valid() const;

    Type type() const;

    bool getBoolean(bool defaultValue = false) const;

    s32 getInt(s32 defaultValue = 0) const;
    u32 getUInt(u32 defaultValue = 0) const;

    f32 getFloat(f32 defaultValue = 0.0f) const;
    f64 getDouble(f64 defaultValue = 0.0) const;

    s32 getStringLength() const;
    s32 getStringBufferSize() const;
    const char* getString(s32 size, char* buffer, const char* defaultValue = EmptyString) const;

    bool tryGet(bool& value) const;

    bool tryGet(s32& value) const;
    bool tryGet(u32& value) const;

    bool tryGet(f32& value) const;
    bool tryGet(f64& value) const;

    bool tryGet(s32 size, char* buffer) const;
private:
    friend class ArrayAccessor;
    friend class TableAccessor;
    friend ArrayAccessor asArray(const ValueAccessor& value);

    ValueAccessor(const TomlParser* parser, u16 value);

    const TomlParser* parser_;
    u16 value_;
};

//----------------------------------------------------------------------
class ArrayAccessor
{
public:
    class Iterator
    {
    public:
        Iterator();
        bool next();
        ValueAccessor get();
    private:
        friend class ArrayAccessor;
        Iterator(const TomlParser* parser, u16 array);

        const TomlParser* parser_;
        u16 index_;
        u16 array_;
    };

    ArrayAccessor();
    bool valid() const;

    Type type() const;

    u16 size() const;
    Iterator begin() const;
private:
    friend class TableAccessor;
    friend ArrayAccessor asArray(const ValueAccessor& value);

    ArrayAccessor(const TomlParser* parser, u16 array);

    const TomlParser* parser_;
    u16 array_;
    u16 size_;
};

//----------------------------------------------------------------------
class TableAccessor
{
public:
    bool valid() const;

    TableAccessor findTable(const char* name) const;
    ArrayAccessor findArray(const char* name) const;
    ValueAccessor findValue(const char* name) const;

    bool tryGet(TableAccessor& table, const char* name) const;
    bool tryGet(ValueAccessor& value, const char* name) const;

private:
    friend class TomlParser;

    TableAccessor(const TomlParser* parser, u16 table);

    const TomlParser* parser_;
    u16 table_;
};

//----------------------------------------------------------------------
class TomlParser
{
private:
public:
    TomlParser(Allocator allocator = CPPTOML_NULL, Deallocator deallocator = CPPTOML_NULL) noexcept;
    ~TomlParser() noexcept;

    TableAccessor getTop() const;
    bool parse(u32 size, const_iterator toml) noexcept;

private:
    TomlParser(const TomlParser&) = delete;
    TomlParser& operator=(const TomlParser&) = delete;

    friend class ValueAccessor;
    friend class ArrayAccessor;
    friend class TableAccessor;

    const_iterator expression(const_iterator current);
    const_iterator keyvalue(u16& elementKey, u16& elementValue, const_iterator current);

    const_iterator key(const_iterator current);
    bool skipDotSep(const_iterator& current);

    const_iterator basic_string(const_iterator current);
    const_iterator escaped(const_iterator current);
    const_iterator literal_string(const_iterator current);
    const_iterator unquoted_key(const_iterator current);

    const_iterator value(const_iterator current);

    const_iterator value_basic_string(const_iterator current);
    const_iterator ml_basic_string(const_iterator current);
    const_iterator mlb_content(const_iterator current);
    const_iterator mlb_escaped_nl(const_iterator current);

    const_iterator value_literal_string(const_iterator current);
    const_iterator ml_literal_string(const_iterator current);
    const_iterator mll_content(const_iterator current);

    const_iterator value_true(const_iterator current);
    const_iterator value_false(const_iterator current);
    const_iterator value_array(const_iterator current);
    const_iterator array_values(const_iterator current);

    const_iterator inline_table(const_iterator current);
    const_iterator inline_table_keyvalues(const_iterator current);

    const_iterator value_number(const_iterator current);
    bool try_integer(const_iterator& current, bool float_int_part);
    bool try_unsigned_dec(const_iterator& current, bool float_int_part);
    bool try_float(const_iterator& current);
    bool try_float_exp_part(const_iterator& current);
    bool try_zero_prefixable_int(const_iterator& current);

    bool try_datetime(const_iterator& current);
    bool try_fulldate(const_iterator& current);
    bool try_fulltime(const_iterator& current);
    bool try_partialtime(const_iterator& current);
    bool try_year(const_iterator& current);
    bool try_month(const_iterator& current);
    bool try_day(const_iterator& current);
    bool try_hour(const_iterator& current);
    bool try_minute(const_iterator& current);
    bool try_second(const_iterator& current);
    bool try_timeoffset(const_iterator& current);

    bool try_special_float(const_iterator& current);
    bool try_inf(const_iterator& current);
    bool try_nan(const_iterator& current);

    typedef bool (*NumericalPredicate)(const_iterator iterator);

    const_iterator hex_oct_bin_int(const_iterator current, NumericalPredicate predicate);
    const_iterator table(const_iterator current);
    const_iterator std_table(const_iterator current);
    const_iterator array_table(const_iterator current);

    u16 getTopTable() const;
    u32 getPosition(const_iterator current) const;
    static u16 getSize(const_iterator start, const_iterator end);
    u16 createDecInt(const_iterator start, const_iterator end);
    u16 createHexInt(const_iterator start, const_iterator end);
    u16 createOctInt(const_iterator start, const_iterator end);
    u16 createBinInt(const_iterator start, const_iterator end);
    u16 createFloat(const_iterator start, const_iterator end);
    u16 createSpecialFloat(const_iterator start, const_iterator end);
    u16 createBoolean(const_iterator start, const_iterator end);
    u16 createDateTime(const_iterator start, const_iterator end);
    u16 createString(Type type, const_iterator start, const_iterator end);
    u16 createTable(u16 parent, u16 size, u32 position);
    u16 createInlineTable();
    u16 createArrayTable(u16 parent, u16 size, u32 position);
    void linkKeyValueToTable(u16 parent, u16 key, u16 value);
    void addToArray(u16 parent, u16 element);
    const_iterator setElement(Element& element, const_iterator start, const_iterator current);

    union Chunk
    {
        Element element_;
        Array array_;
        KeyValue keyvalue_;
        Table table_;
    };

    class Buffer
    {
    public:
        static const u32 ChunkSize = sizeof(Chunk);
        static const u16 AllocStep = 128;

        Buffer(Allocator allocator, Deallocator deallocator);
        ~Buffer();

        void clear();

        template<class T>
        u16 create();

        template<>
        u16 create<Element>();
        template<>
        u16 create<Array>();
        template<>
        u16 create<KeyValue>();
        template<>
        u16 create<Table>();

        void expand();

        const Chunk& operator[](u16 index) const;
        Chunk& operator[](u16 index);

        Allocator allocator_;
        Deallocator deallocator_;

        u16 capacity_;
        u16 size_;
        Chunk* chunks_;
    };

    u16 currentTable_;
    u16 currentKeyTable_;
    u16 lastElement_;
    u16 lastKeySize_;
    u32 lastKeyPosition_;

    Buffer chunks_;
    const_iterator toml_;
    const_iterator end_;
};

} // namespace cpptoml
#endif //INC_CPPTOML_H_
