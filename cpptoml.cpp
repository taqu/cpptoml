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
#include <cstdlib>
#include <iterator>

#ifdef CPPTOML_DEBUG
#    include <cassert>
#    define CPPTOML_ASSERT(exp) assert((exp))
#else
#    define CPPTOML_ASSERT(exp)
#endif
CPPTOML_NAMESPAFCE_BEGIN

//--- Buffer
TomlParser::Buffer::Buffer(cpptoml_malloc allocator, cpptoml_free deallocator)
    : allocator_(allocator)
    , deallocator_(deallocator)
    , capacity_(0)
    , size_(0)
{
    CPPTOML_ASSERT((CPPTOML_NULL != allocator && CPPTOML_NULL != deallocator) || (CPPTOML_NULL == allocator && CPPTOML_NULL == deallocator));
    if(CPPTOML_NULL == allocator_) {
        allocator_ = ::malloc;
    }
    if(CPPTOML_NULL == deallocator_) {
        deallocator_ = ::free;
    }
}

TomlParser::Buffer::~Buffer()
{
}

u32 TomlParser::Buffer::capacity() const
{
    return capacity_;
}

u32 TomlParser::Buffer::size() const
{
    return size_;
}

void TomlParser::Buffer::clear()
{
    size_ = 0;
}

TomlParser::TomlParser(cpptoml_malloc allocator, cpptoml_free deallocator)
    : allocator_(allocator)
    , deallocator_(deallocator)
    , begin_(CPPTOML_NULL)
    , current_(CPPTOML_NULL)
    , end_(CPPTOML_NULL)
    , buffer_(allocator, deallocator)
{
    CPPTOML_ASSERT((CPPTOML_NULL != allocator && CPPTOML_NULL != deallocator) || (CPPTOML_NULL == allocator && CPPTOML_NULL == deallocator));
    if(CPPTOML_NULL == allocator_) {
        allocator_ = ::malloc;
    }
    if(CPPTOML_NULL == deallocator_) {
        deallocator_ = ::free;
    }
}

TomlParser::~TomlParser()
{
}

bool TomlParser::parse(cursor head, cursor end)
{
    CPPTOML_ASSERT(head <= end);
    begin_ = current_ = head;
    end_ = end;
    buffer_.clear();
    current_ = skip_bom(current_);
    while(current_ < end_) {
        current_ = parse_expression(current_);
        if(CPPTOML_NULL == current_) {
            return false;
        }
        current_ = skip_newline(current_);
    }
    return end_ <= current_;
}

u32 TomlParser::size() const
{
    return 0;
}

bool TomlParser::is_alpha(Char c)
{
    return (0x41U <= c && c <= 0x5AU) || (0x61U <= c && c <= 0x7AU);
}

bool TomlParser::is_digit(Char c)
{
    return 0x30U <= c && c <= 0x39U;
}

bool TomlParser::is_hexdigit(Char c)
{
    return is_digit(c) || ('A' <= c && c <= 'F');
}

bool TomlParser::is_digit19(Char c)
{
    return 0x31U <= c && c <= 0x39U;
}

bool TomlParser::is_digit07(Char c)
{
    return 0x30U <= c && c <= 0x37U;
}

bool TomlParser::is_digit01(Char c)
{
    return 0x30U <= c && c <= 0x31U;
}

bool TomlParser::is_whitespace(Char c)
{
    return 0x20U == c || 0x09U == c;
}

bool TomlParser::is_basicchar(Char c)
{
    if(is_whitespace(c)
       || 0x21U == c
       || 0x23U <= c && c <= 0x5BU
       || 0x5DU <= c && c <= 0x7EU) {
        return true;
    }
    return false;
}

bool TomlParser::is_newline(Char c)
{
    return 0x0AU == c || 0x0DU == c;
}

bool TomlParser::is_quated_key(Char c)
{
    if(0x22U == c
       || 0x27U == c) {
        return true;
    }
    return false;
}

bool TomlParser::is_unquated_key(Char c)
{
    if(is_alpha(c)
       || is_digit(c)
       || 0x2DU == c
       || 0x5FU == c) {
        return true;
    }
    return false;
}

bool TomlParser::is_table(Char c)
{
    return 0x5BU == c;
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
        switch(*str) {
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
    return value0 == (*str & mask0) ? str : end_;
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
    if(value0 != (*str & mask0)) {
        return CPPTOML_NULL;
    }
    ++str;
    return value0 == (*str & mask0) ? str : end_;
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
    if(value0 != (*str & mask0)) {
        return CPPTOML_NULL;
    }
    ++str;
    if(value0 != (*str & mask0)) {
        return CPPTOML_NULL;
    }
    ++str;
    return value0 == (*str & mask0) ? str : end_;
}

TomlParser::cursor TomlParser::skip_comment(cursor str)
{
    if(end_ <= str || '#' != *str) {
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
    if(is_quated_key(*str) || is_unquated_key(*str)) {
        str = parse_keyval(str);
        if(CPPTOML_NULL == str) {
            return str;
        }
    } else if(is_table(*str)) {
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
    str = parse_key(str);
    if(CPPTOML_NULL == str) {
        return str;
    }
    str = parse_keyval_sep(str);
    if(CPPTOML_NULL == str) {
        return str;
    }
    str = parse_val(str);
    if(CPPTOML_NULL == str) {
        return str;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_key(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(is_quated_key(str[0])) {
        str = parse_quated_key(str);
    } else if(is_unquated_key(str[0])) {
        str = parse_unquated_key(str);
    } else {
        return CPPTOML_NULL;
    }
    if(CPPTOML_NULL == str) {
        return str;
    }
    cursor dot_sep = parse_dot_sep(str);
    if(CPPTOML_NULL != dot_sep) {
        return parse_key(dot_sep);
    }
    return str;
}

TomlParser::cursor TomlParser::parse_dot_sep(cursor str)
{
    str = skip_spaces(str);
    if(end_ <= str || 0x2EU != *str) {
        return CPPTOML_NULL;
    }
    ++str;
    str = skip_spaces(str);
    return str;
}

TomlParser::cursor TomlParser::parse_keyval_sep(cursor str)
{
    str = skip_spaces(str);
    if(end_ <= str || 0x3DU != *str) {
        return CPPTOML_NULL;
    }
    ++str;
    str = skip_spaces(str);
    return str;
}

TomlParser::cursor TomlParser::parse_val(cursor str)
{
    CPPTOML_ASSERT(str < end_);

    cursor val;
    val = parse_string(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    val = parse_boolean(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    val = parse_array(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    val = parse_inline_table(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    val = parse_date_time(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    val = parse_float(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    val = parse_integer(str);
    if(CPPTOML_NULL != val) {
        return val;
    }
    return CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_quated_key(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x22U == *str) {
        cursor next = parse_basic_string(str);
        if(CPPTOML_NULL != next) {
            return next;
        }
    } else if(0x27U == *str) {
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
        if(is_alpha(*str)
           || is_digit(*str)
           || 0x2DU == *str
           || 0x5FU == *str) {
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
    if(0x5CU == *str) {
        return parse_escaped(str);
    }
    if(is_whitespace(*str)
       || 0x21U == *str
       || 0x23U <= *str && *str <= 0x5BU
       || 0x5DU <= *str && *str <= 0x7EU) {
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

    if(value1 == (*str & mask1)) {
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
    if(0x09U == *str) {
        ++str;
    } else if(0x20U <= *str && *str <= 0x7FU) {
        ++str;
    } else {
        str = parse_non_ascii(str);
    }
    return str;
}

TomlParser::cursor TomlParser::parse_escaped(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    CPPTOML_ASSERT(0x5CU == *str);
    ++str;
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    switch(*str) {
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
    CPPTOML_ASSERT(0x75U == *str);
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
    CPPTOML_ASSERT(0x55U == *str);
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
    if(0x09U == *str
       || 0x20U <= *str && *str <= 0x26U
       || 0x28U <= *str && *str <= 0x7EU) {
        ++str;
        return str;
    }
    return parse_non_ascii(str);
}

TomlParser::cursor TomlParser::parse_string(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x22U == *str) {
        if((str + 2) < end_ && 0x22U == str[1] && 0x22U == str[2]) {
            return parse_ml_basic_string(str);
        } else {
            return parse_basic_string(str);
        }
    }
    if(0x27U == *str) {
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
    if(0x5CU != *str) {
        return CPPTOML_NULL;
    }
    ++str;
    str = skip_spaces(str);
    str = skip_newline(str);

    while(str < end_) {
        switch(*str) {
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
    if(0x74U == *str) {
        if((str + 4) < end_ && 0x72U == str[1] && 0x75U == str[2] && 0x65U == str[3]) {
            return str + 4;
        }
        return CPPTOML_NULL;
    }
    if(0x66U == *str) {
        if((str + 5) < end_ && 0x61U == str[1] && 0x6CU == str[2] && 0x73U == str[3] && 0x65U == str[4]) {
            return str + 5;
        }
        return CPPTOML_NULL;
    }
    return CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_array(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x5BU != *str) {
        return CPPTOML_NULL;
    }
    ++str;
    cursor val;
    val = parse_array_values(str);
    if(CPPTOML_NULL != val) {
        str = val;
    }
    str = parse_ws_comment_newline(str);
    if(CPPTOML_NULL == str || end_ <= str || 0x5DU != str[0]) {
        return CPPTOML_NULL;
    }
    return str + 1;
}

TomlParser::cursor TomlParser::parse_array_values(cursor str)
{
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    str = parse_ws_comment_newline(str);
    if(CPPTOML_NULL == str) {
        return CPPTOML_NULL;
    }

    cursor val;
    val = parse_val(str);
    if(CPPTOML_NULL != val) {
        str = val;
    }
    str = parse_ws_comment_newline(str);
    if(CPPTOML_NULL != str && str < end_ && 0x2CU == str[0]) {
        ++str;
        return parse_array_values(str);
    }
    return str;
}

TomlParser::cursor TomlParser::parse_ws_comment_newline(cursor str)
{
    while(str < end_) {
        switch(*str) {
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

TomlParser::cursor TomlParser::parse_inline_table(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    if(0x7BU != *str) {
        return CPPTOML_NULL;
    }
    ++str;
    str = skip_spaces(str);

    cursor val;
    val = parse_inline_table_keyvals(str);
    if(CPPTOML_NULL != val) {
        str = val;
    }
    str = skip_spaces(str);
    if(end_ <= str || 0x7DU != str[0]) {
        return CPPTOML_NULL;
    }
    return str + 1;
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
    if(str < end_ && 0x2CU == *str) {
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
    switch(*str) {
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
    if(str == CPPTOML_NULL || end_ <= str || '-' != *str) {
        return CPPTOML_NULL;
    }
    ++str;

    str = parse_date_month(str);
    if(str == CPPTOML_NULL || end_ <= str || '-' != *str) {
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
    switch(*str) {
    case 'Z':
    case 'z':
        ++str;
        break;
    case '+':
    case '-': {
        ++str;
        str = parse_time_hour(str);
        if(str == CPPTOML_NULL || end_ <= str || ':' != *str) {
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
    if(str == CPPTOML_NULL || end_ <= str || ':' != *str) {
        return CPPTOML_NULL;
    }
    ++str;

    str = parse_time_minute(str);
    if(str == CPPTOML_NULL || end_ <= str || ':' != *str) {
        return CPPTOML_NULL;
    }
    ++str;

    str = parse_time_second(str);
    if(str == CPPTOML_NULL || end_ <= str) {
        return str;
    }
    if('.' == *str) {
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

TomlParser::cursor TomlParser::parse_float(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    cursor val;
    val = parse_special_float(str);
    if(CPPTOML_NULL != val) {
        return val;
    }

    str = parse_float_int_part(str);
    if(CPPTOML_NULL == str || end_ <= str) {
        return CPPTOML_NULL;
    }
    switch(str[0]) {
    case 'e':
    case 'E':
        str = parse_exp(str);
        return str;
    case 0x2EU:
        str = parse_frac(str);
        if(CPPTOML_NULL == str) {
            return CPPTOML_NULL;
        }
        if(str < end_) {
            switch(str[0]) {
            case 'e':
            case 'E':
                str = parse_exp(str);
                break;
            };
        }
        return str;
    }
    return CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_special_float(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    switch(str[0]) {
    case 0x2DU:
    case 0x2BU:
        break;
    default:
        return CPPTOML_NULL;
    }
    if(end_ <= (str + 3)) {
        return CPPTOML_NULL;
    }
    if(0x69U == str[0] || 0x6EU == str[1] || 0x66U == str[2]) {
        return str + 3;
    }
    if(0x6EU == str[0] || 0x61U == str[1] || 0x6EU == str[2]) {
        return str + 3;
    }
    return CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_float_int_part(cursor str)
{
    return parse_dec_int(str);
}

TomlParser::cursor TomlParser::parse_dec_int(cursor str)
{
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    switch(str[0]) {
    case 0x2BU:
    case 0x2DU:
        ++str;
        break;
    }
    return parse_unsigned_dec_int(str);
}

TomlParser::cursor TomlParser::parse_unsigned_dec_int(cursor str)
{
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    switch(str[0]) {
    case 0x30U:
        ++str;
        if(end_ <= str) {
            return str;
        }
        if(is_digit(str[0]) || 0x5FU == str[0]) {
            return CPPTOML_NULL;
        }
        return str;
    default:
        if(!is_digit19(str[0])) {
            return CPPTOML_NULL;
        }
        ++str;
        break;
    }
    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return CPPTOML_NULL;
            }
            if(!is_digit(str[0])) {
                return CPPTOML_NULL;
            }
        } else if(!is_digit(str[0])) {
            break;
        }
        ++str;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_exp(cursor str)
{
    CPPTOML_ASSERT('e' == str[0] || 'E' == str[0]);
    ++str;
    return parse_float_exp_part(str);
}

TomlParser::cursor TomlParser::parse_float_exp_part(cursor str)
{
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    switch(str[0]) {
    case 0x2BU:
    case 0x2DU:
        ++str;
        break;
    }
    return parse_zero_prefixable_int(str);
}

TomlParser::cursor TomlParser::parse_zero_prefixable_int(cursor str)
{
    if(end_ <= str || !is_digit(str[0])) {
        return CPPTOML_NULL;
    }
    ++str;

    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return CPPTOML_NULL;
            }
            if(!is_digit(str[0])) {
                return CPPTOML_NULL;
            }
        } else if(!is_digit(str[0])) {
            break;
        }
        ++str;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_frac(cursor str)
{
    CPPTOML_ASSERT(0x2EU == str[0]);
    ++str;
    return parse_zero_prefixable_int(str);
}

TomlParser::cursor TomlParser::parse_integer(cursor str)
{
    CPPTOML_ASSERT(str < end_);
    switch(str[0]) {
    case 0x30U:
        if((str + 1) < end_) {
            switch(str[1]) {
            case 0x78U:
                str = parse_hex_prefix(str);
                if(CPPTOML_NULL == str) {
                    return CPPTOML_NULL;
                }
                break;
            case 0x6FU:
                str = parse_oct_prefix(str);
                if(CPPTOML_NULL == str) {
                    return CPPTOML_NULL;
                }
                break;
            case 0x62U:
                str = parse_bin_prefix(str);
                if(CPPTOML_NULL == str) {
                    return CPPTOML_NULL;
                }
                break;
            default:
                break;
            }
        }
        break;
    }
    return parse_dec_int(str);
}

TomlParser::cursor TomlParser::parse_hex_prefix(cursor str)
{
    CPPTOML_ASSERT(str < end_ && 0x78U != str[0]);
    ++str;
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    if(!is_hexdigit(str[0])) {
        return CPPTOML_NULL;
    }

    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return CPPTOML_NULL;
            }
            if(!is_hexdigit(str[0])) {
                return CPPTOML_NULL;
            }
        } else if(!is_hexdigit(str[0])) {
            break;
        }
        ++str;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_oct_prefix(cursor str)
{
    CPPTOML_ASSERT(str < end_ && 0x6FU != str[0]);
    ++str;
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    if(!is_digit07(str[0])) {
        return CPPTOML_NULL;
    }

    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return CPPTOML_NULL;
            }
            if(!is_digit07(str[0])) {
                return CPPTOML_NULL;
            }
        } else if(!is_digit07(str[0])) {
            break;
        }
        ++str;
    }
    return str;
}

TomlParser::cursor TomlParser::parse_bin_prefix(cursor str)
{
    CPPTOML_ASSERT(str < end_ && 0x62U != str[0]);
    ++str;
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    if(!is_digit01(str[0])) {
        return CPPTOML_NULL;
    }

    while(str < end_) {
        if(0x5FU == str[0]) {
            ++str;
            if(end_ <= str) {
                return CPPTOML_NULL;
            }
            if(!is_digit01(str[0])) {
                return CPPTOML_NULL;
            }
        } else if(!is_digit01(str[0])) {
            break;
        }
        ++str;
    }
    return str;
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
    str = parse_key(str);
    if(CPPTOML_NULL == str) {
        return CPPTOML_NULL;
    }
    str = skip_spaces(str);
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    return 0x5DU == str[0] ? str + 1 : CPPTOML_NULL;
}

TomlParser::cursor TomlParser::parse_array_table(cursor str)
{
    CPPTOML_ASSERT((str + 1) < end_);
    CPPTOML_ASSERT(is_table(str[0]) && is_table(str[1]));
    str += 2;
    str = skip_spaces(str);
    str = parse_key(str);
    if(CPPTOML_NULL == str) {
        return CPPTOML_NULL;
    }

    str = skip_spaces(str);
    if(end_ <= str || 0x5DU != str[0]) {
        return CPPTOML_NULL;
    }
    ++str;
    if(end_ <= str || 0x5DU != str[0]) {
        return CPPTOML_NULL;
    }
    return str + 1;
}
CPPTOML_NAMESPAFCE_END
