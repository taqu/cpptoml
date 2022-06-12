#ifndef INC_CPPTOML_H_
#define INC_CPPTOML_H_
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
#include <cstdint>
#include <ctime>

namespace cpptoml
{

#ifndef CPPTOML_TYPES
#    define CPPTOML_TYPES
#    ifdef __cplusplus
#        define CPPTOML_CPP
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using Char = char;
using UChar = unsigned char;

#    else
#    endif
#endif // CPPTOML_TYPES

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
#endif // CPPTOML_NULL

typedef void* (*cpptoml_malloc)(size_t, void*);
typedef void (*cpptoml_free)(void*, void*);

static constexpr u32 Invalid = 0xFFFFFFFFU;

class TomlParser;

//--- TomlType
//---------------------------------------
enum class TomlType : u32
{
    None = 0,
    Table,
    Array,
    ArrayTable,
    String,
    DateTime,
    Float,
    Integer,
    Boolean,
    Key,
    KeyValue,
};

struct TomlTable
{
    u32 size_;
    u32 head_;
};

struct TomlArray
{
    u32 size_;
    u32 head_;
};

struct TomlArrayTable
{
    u32 size_;
    u32 head_;
};

struct TomlKeyValue
{
    u32 key_;
    u32 value_;
};

struct TomlPositionLength
{
    u32 position_;
    u32 length_;
};

struct TomlValue
{
    u32 next_;
    TomlType type_;
    union
    {
        TomlTable table_;
        TomlArray array_;
        TomlArrayTable arrayTable_;
        TomlKeyValue keyvalue_;
        TomlPositionLength positionLength_;
        s64 int_;
        f64 float_;
    };
};

//--- TomlProxy
//---------------------------------------
struct TomlStringProxy
{
    bool valid_;
    u32 length_;
    const char* str_;

    bool equals(u32 length, const char* str) const;
};

struct TomlDateTimeProxy
{
    enum class Type : s8
    {
        OffsetDateTime,
        LocalDateTime,
        LocalDate,
        LocalTime,
    };

    bool valid_;
    Type type_;
    s32 year_;
    s32 month_;
    s32 day_;
    s32 hour_;
    s32 minute_;
    s32 second_;
    s32 millisecond_;
    s32 offset_; //!< an UTC offset
};

struct TomlFloatProxy
{
    bool valid_;
    f64 value_;
};

struct TomlIntProxy
{
    bool valid_;
    s64 value_;
};

struct TomlBoolProxy
{
    bool valid_;
    bool value_;
};

class TomlValueProxy
{
public:
    TomlType type() const;
    template<class T>
    T value() const {}

private:
    friend class TomlKeyValueProxy;
    friend class TomlArrayProxy;
    friend class TomlArrayTableProxy;

    const TomlParser* parser_;
    u32 value_;
};

class TomlKeyValueProxy
{
public:
    TomlStringProxy key() const;
    TomlValueProxy value() const;

    inline u32 getKeyIndex() const
    {
        return key_;
    }
    inline u32 getValueIndex() const
    {
        return value_;
    }

private:
    friend class TomlTableProxy;

    const TomlParser* parser_;
    u32 key_;
    u32 value_;
};

class TomlArrayProxy
{
public:
    using Iterator = u32;

    u32 size() const;
    Iterator begin() const;
    Iterator next(Iterator iterator) const;
    Iterator end() const;
    TomlValueProxy operator[](Iterator iterator) const;

private:
    friend class TomlValueProxy;
    friend class TomlParser;

    TomlArrayProxy(const TomlParser* parser, u32 index)
        : parser_(parser)
        , index_(index)
    {
    }
    const TomlParser* parser_;
    u32 index_;
};

class TomlTableProxy
{
public:
    using Iterator = u32;
    TomlTableProxy()
        : parser_(CPPTOML_NULL)
        , index_(Invalid)
    {
    }

    ~TomlTableProxy()
    {
    }

    u32 size() const;
    Iterator begin() const;
    Iterator next(Iterator iterator) const;
    Iterator end() const;
    TomlKeyValueProxy operator[](Iterator iterator) const;

    template<class T>
    bool tryGet(T& x, const char* name) const;

private:
    friend class TomlValueProxy;
    friend class TomlParser;

    TomlTableProxy(const TomlParser* parser, u32 index)
        : parser_(parser)
        , index_(index)
    {
    }

    const TomlParser* parser_;
    u32 index_;
};

class TomlArrayTableProxy
{
public:
    using Iterator = u32;

    u32 size() const;
    Iterator begin() const;
    Iterator next(Iterator iterator) const;
    Iterator end() const;
    TomlValueProxy operator[](Iterator iterator) const;

private:
    friend class TomlValueProxy;
    friend class TomlParser;

    TomlArrayTableProxy(const TomlParser* parser, u32 index)
        : parser_(parser)
        , index_(index)
    {
    }

    const TomlParser* parser_;
    u32 index_;
};

//--- TomlParser
//---------------------------------------
class TomlParser
{
public:
    static constexpr u32 MaxNests = 64;
    static constexpr u32 ExpandSize = 4 * 4096;
    using cursor = const UChar*;

    TomlParser(cpptoml_malloc allocator = CPPTOML_NULL, cpptoml_free deallocator = CPPTOML_NULL, void* user_data = CPPTOML_NULL);
    ~TomlParser();

    bool parse(const char* head, const char* end)
    {
        return parse(reinterpret_cast<cursor>(head), reinterpret_cast<cursor>(end));
    }

    bool parse(cursor head, cursor end);
    void clear();
    TomlTableProxy top() const;

private:
    TomlParser(const TomlParser&) = delete;
    TomlParser& operator=(const TomlParser&) = delete;

    friend class TomlValueProxy;
    friend class TomlKeyValueProxy;
    friend class TomlArrayProxy;
    friend class TomlTableProxy;
    friend class TomlArrayTableProxy;

    struct Result
    {
        cursor cursor_;
        u32 index_;
    };

    struct ResultFloat
    {
        cursor cursor_;
        f64 value_;
    };

    struct ResultInt
    {
        cursor cursor_;
        s64 value_;
    };

    static bool is_alpha(UChar c);
    static bool is_digit(UChar c);
    static bool is_hexdigit(UChar c);
    static bool is_digit19(UChar c);
    static bool is_digit07(UChar c);
    static bool is_digit01(UChar c);
    static bool is_whitespace(UChar c);
    static bool is_basicchar(UChar c);
    static bool is_newline(UChar c);

    static bool is_quated_key(UChar c);
    static bool is_unquated_key(UChar c);
    static bool is_table(UChar c);

    static bool is_minus(f64 x);

    static bool check_overflow(s64 x0, s64 x1, s64 x2);
    static bool mul(s64& x, s64 base);
    static bool add(s64& x, s64 x0, s64 x1);
    static s64 from_digit(UChar c);
    static s64 from_hex(UChar c);

    cursor skip_bom(cursor str);
    cursor skip_newline(cursor str);
    cursor skip_spaces(cursor str);
    cursor skip_utf8_1(cursor str);
    cursor skip_utf8_2(cursor str);
    cursor skip_utf8_3(cursor str);
    cursor skip_comment(cursor str);

    cursor parse_expression(cursor str);
    cursor parse_keyval(cursor str);
    cursor parse_table(cursor str);

    Result parse_key(cursor str);
    cursor parse_quated_key(cursor str);
    cursor parse_unquated_key(cursor str);
    cursor parse_dot_sep(cursor str);
    cursor parse_keyval_sep(cursor str);
    Result parse_val(cursor str);
    cursor parse_basic_string(cursor str);
    cursor parse_basic_char(cursor str);
    cursor parse_non_ascii(cursor str);
    cursor parse_non_eol(cursor str);
    cursor parse_escaped(cursor str);
    cursor parse_4HEXDIG(cursor str);
    cursor parse_8HEXDIG(cursor str);
    cursor parse_literal_string(cursor str);
    cursor parse_literal_char(cursor str);

    cursor parse_string(cursor str);
    cursor parse_ml_basic_string(cursor str);
    cursor parse_ml_basic_body(cursor str);
    cursor parse_mlb_quotes(cursor str);
    cursor parse_mlb_content(cursor str);
    cursor parse_mlb_escaped_nl(cursor str);
    cursor parse_ml_literal_string(cursor str);
    cursor parse_ml_literal_body(cursor str);
    cursor parse_mll_quotes(cursor str);
    cursor parse_mll_content(cursor str);

    cursor parse_boolean(cursor str);

    Result parse_array(cursor str);
    cursor parse_array_values(cursor str, u32 array);
    cursor parse_ws_comment_newline(cursor str);

    Result parse_inline_table(cursor str);
    cursor parse_inline_table_keyvals(cursor str);

    cursor parse_date_time(cursor str);
    cursor parse_offset_date_time(cursor str);
    cursor parse_local_date_time(cursor str);
    cursor parse_local_date(cursor str);
    cursor parse_local_time(cursor str);
    cursor parse_time_delim(cursor str);
    cursor parse_full_date(cursor str);
    cursor parse_full_time(cursor str);
    cursor parse_partial_time(cursor str);
    cursor parse_date_fullyear(cursor str);
    cursor parse_date_month(cursor str);
    cursor parse_date_mday(cursor str);
    cursor parse_time_hour(cursor str);
    cursor parse_time_minute(cursor str);
    cursor parse_time_second(cursor str);
    cursor parse_time_secfrac(cursor str);

    ResultFloat parse_float(cursor str);
    ResultFloat parse_special_float(cursor str);
    ResultFloat parse_float_int_part(cursor str);
    ResultInt parse_dec_int(cursor str);
    ResultInt parse_unsigned_dec_int(cursor str, bool minus);
    ResultInt parse_exp(cursor str);
    ResultInt parse_float_exp_part(cursor str);
    ResultInt parse_zero_prefixable_int(cursor str);
    ResultFloat parse_frac(cursor str);

    ResultInt parse_integer(cursor str);
    ResultInt parse_hex_prefix(cursor str);
    ResultInt parse_oct_prefix(cursor str);
    ResultInt parse_bin_prefix(cursor str);

    cursor parse_std_table(cursor str);
    cursor parse_array_table(cursor str);

    u32 length(cursor begin, cursor end) const;
    u32 create_value(cursor begin, cursor end, TomlType type);
    u32 create_key(cursor begin, cursor end);
    u32 create_keyvalue(u32 key, u32 value);
    u32 create_table();
    u32 create_array_table();
    u32 create_array();
    u32 create_string(cursor begin, cursor end);
    u32 create_float(f64 value);
    u32 create_int(s64 value);

    const TomlValue& get_value(u32 index) const;
    TomlValue& get_value(u32 index);

    const char* str(u32 position) const;
    u32 allocate();
    void reset_table();
    void add_table_value(u32 table, u32 value);
    void add_array_value(u32 array, u32 value);

    TomlStringProxy get_string(u32 value) const;
    TomlDateTimeProxy get_datetime(u32 value) const;
    TomlFloatProxy get_float(u32 value) const;
    TomlIntProxy get_int(u32 value) const;
    TomlBoolProxy get_bool(u32 value) const;

    u32 traverse_tables(u32 key_chain, TomlType type);
    u32 get_last_table(u32 array_table);

    u32 find_array_table(u32 key_chain) const;
    bool equal(const TomlPositionLength& x0, const TomlPositionLength& x1) const;
    bool compare_key_chain(u32 key0, u32 key1) const;
    bool validate_table_key_chain(u32 key_chain) const;
    bool validate_array_key_chain(u32 key_chain) const;
    bool check_array_mixed_types(u32 array) const;

    cpptoml_malloc allocator_;
    cpptoml_free deallocator_;
    void* user_data_;
    cursor begin_;
    cursor current_;
    cursor end_;

    u32 capacity_;
    u32 size_;
    TomlValue* buffer_;
    u32 table_;
    u32 array_table_;
};

template<>
TomlStringProxy TomlValueProxy::value<TomlStringProxy>() const;

template<>
TomlDateTimeProxy TomlValueProxy::value<TomlDateTimeProxy>() const;

template<>
TomlFloatProxy TomlValueProxy::value<TomlFloatProxy>() const;

template<>
TomlIntProxy TomlValueProxy::value<TomlIntProxy>() const;

template<>
TomlBoolProxy TomlValueProxy::value<TomlBoolProxy>() const;

template<>
TomlArrayProxy TomlValueProxy::value<TomlArrayProxy>() const;

template<>
TomlTableProxy TomlValueProxy::value<TomlTableProxy>() const;

template<>
TomlArrayTableProxy TomlValueProxy::value<TomlArrayTableProxy>() const;

//
template<>
bool TomlTableProxy::tryGet<s8>(s8& x, const char* name) const;
template<>
bool TomlTableProxy::tryGet<s16>(s16& x, const char* name) const;
template<>
bool TomlTableProxy::tryGet<s32>(s32& x, const char* name) const;
template<>
bool TomlTableProxy::tryGet<s64>(s64& x, const char* name) const;

template<>
bool TomlTableProxy::tryGet<u8>(u8& x, const char* name) const;
template<>
bool TomlTableProxy::tryGet<u16>(u16& x, const char* name) const;
template<>
bool TomlTableProxy::tryGet<u32>(u32& x, const char* name) const;
template<>
bool TomlTableProxy::tryGet<u64>(u64& x, const char* name) const;

template<>
bool TomlTableProxy::tryGet<f32>(f32& x, const char* name) const;
template<>
bool TomlTableProxy::tryGet<f64>(f64& x, const char* name) const;

template<>
bool TomlTableProxy::tryGet<TomlStringProxy>(TomlStringProxy& x, const char* name) const;
template<>
bool TomlTableProxy::tryGet<TomlTableProxy>(TomlTableProxy& x, const char* name) const;
} // namespace cpptoml
#endif // INC_CPPTOML_H_
