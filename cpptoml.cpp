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

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include <charconv>
#include <iterator>
#include <limits>

#ifdef CPPTOML_DEBUG
#    define CPPTOML_ASSERT(exp) assert((exp))
#else
#    define CPPTOML_ASSERT(exp)
#endif

namespace cpptoml
{
namespace
{
    bool validate_object(TomlProxy proxy);
    bool validate_array(TomlProxy proxy);
    bool validate_keyvalue(TomlProxy proxy);
    bool validate_string(TomlProxy proxy);
    bool validate_integer(TomlProxy proxy);
    bool validate_hex(TomlProxy proxy);
    bool validate_oct(TomlProxy proxy);
    bool validate_bin(TomlProxy proxy);
    bool validate_float(TomlProxy proxy);
    bool validate_inf(TomlProxy proxy);
    bool validate_nan(TomlProxy proxy);
    bool validate_datetime(TomlProxy proxy);
    bool validate_true(TomlProxy proxy);
    bool validate_false(TomlProxy proxy);

    bool validate(TomlProxy proxy)
    {
        switch(proxy.type()) {
        case TomlType::Table:
            return validate_object(proxy);
        case TomlType::Array:
            return validate_array(proxy);
        case TomlType::KeyValue:
            return validate_keyvalue(proxy);
        case TomlType::String:
            return validate_string(proxy);
        case TomlType::Integer:
            return validate_integer(proxy);
        case TomlType::Hex:
            return validate_hex(proxy);
        case TomlType::Oct:
            return validate_oct(proxy);
        case TomlType::Bin:
            return validate_bin(proxy);
        case TomlType::Float:
            return validate_float(proxy);
        case TomlType::Inf:
            return validate_inf(proxy);
        case TomlType::NaN:
            return validate_nan(proxy);
        case TomlType::DateTime:
            return validate_datetime(proxy);
        case TomlType::True:
            return validate_true(proxy);
        case TomlType::False:
            return validate_false(proxy);
        default:
            return false;
        }
    }

    bool validate_object(TomlProxy proxy)
    {
        for(TomlProxy i = proxy.begin(); i; i = i.next()) {
            if (!validate(i.value())) {
                return false;
            }
        }
        return true;
    }

    bool validate_array(TomlProxy proxy)
    {
        TomlType type = TomlType::Invalid;
        for(TomlProxy i = proxy.begin(); i; i = i.next()) {
            if(!validate(i)) {
                return false;
            }
            if (TomlType::Invalid == i.type()) {
                return false;
            }
            if(TomlType::Invalid == type) {
                type = i.type();
            } else if(type != i.type()) {
                return false;
            }
        }
        return true;
    }

    bool validate_keyvalue(TomlProxy proxy)
    {
        return validate(proxy.value());
    }

    bool validate_string(cpptoml::TomlProxy /*proxy*/)
    {
        return true;
    }

    bool validate_integer(cpptoml::TomlProxy /*proxy*/)
    {
        return true;
    }

    bool validate_hex(TomlProxy /*proxy*/)
    {
        return true;
    }

    bool validate_oct(TomlProxy /*proxy*/)
    {
        return true;
    }

    bool validate_bin(TomlProxy /*proxy*/)
    {
        return true;
    }

    bool validate_float(cpptoml::TomlProxy /*proxy*/)
    {
        return true;
    }

    bool validate_inf(TomlProxy /*proxy*/)
    {
        return true;
    }

    bool validate_nan(TomlProxy /*proxy*/)
    {
        return true;
    }

    bool validate_datetime(TomlProxy /*proxy*/)
    {
        return true;
    }

    bool validate_true(cpptoml::TomlProxy)
    {
        return true;
    }

    bool validate_false(cpptoml::TomlProxy)
    {
        return true;
    }
}

//--- TomlProxy
//---------------------------------------
TomlProxy::operator bool() const
{
    return TomlParser::Invalid != value_;
}

TomlType TomlProxy::type() const
{
    if(TomlParser::Invalid != value_) {
        return static_cast<TomlType>(values_[value_].type_);
    }
    return TomlType::Invalid;
}

uint64_t TomlProxy::size() const
{
    CPPTOML_ASSERT(TomlParser::Invalid != value_);
    return values_[value_].size_;
}

TomlProxy TomlProxy::begin() const
{
    CPPTOML_ASSERT(TomlParser::Invalid != value_);
    if(static_cast<uint32_t>(TomlType::Table) != values_[value_].type_
       && static_cast<uint32_t>(TomlType::Array) != values_[value_].type_) {
        return {TomlParser::Invalid, CPPTOML_NULL, CPPTOML_NULL};
    }
    return {static_cast<uint32_t>(values_[value_].start_), data_, values_};
}

TomlProxy TomlProxy::next() const
{
    CPPTOML_ASSERT(TomlParser::Invalid != value_);
    return {values_[value_].next_, data_, values_};
}

TomlProxy TomlProxy::key() const
{
    CPPTOML_ASSERT(TomlParser::Invalid != value_);
    if(TomlType::KeyValue != type()) {
        return {TomlParser::Invalid, CPPTOML_NULL, CPPTOML_NULL};
    }
    return {values_[value_].start_, data_, values_};
}

TomlProxy TomlProxy::value() const
{
    CPPTOML_ASSERT(TomlParser::Invalid != value_);
    if(TomlType::KeyValue != type()) {
        return {TomlParser::Invalid, CPPTOML_NULL, CPPTOML_NULL};
    }
    return {values_[value_].size_, data_, values_};
}

namespace
{
    bool iswhitespace(const char c)
    {
        switch(c) {
        case 0x09:
        case 0x20:
            return true;
        default:
            return false;
        }
    }

}

uint64_t TomlProxy::getTableName(uint32_t len, char* str) const
{
    CPPTOML_ASSERT(0<len);
    CPPTOML_ASSERT(nullptr != str);
    if(TomlType::Table != type()) {
        str[0] = '\0';
        return 0;
    }
    CPPTOML_ASSERT(data_[0] == '[');
    const char* data = data_ + 1;
    uint32_t i = 0;
    for(; i<len; ++i){
        if(!iswhitespace(data[i])){
            break;
        }
    }
    --len;
    uint32_t o = 0;
    for(; o<len; ++i){
        if(data[i]==']'){
            break;
        }
        str[o] = data[i];
        ++o;
    }
    str[o] = '\0';
    if(o<=0){
        return o;
    }
    for(uint32_t j=o;0<j; --j){
        if(!iswhitespace(str[j-1])){
            o = j;
            str[o] = '\0';
            break;
        }
    }
    return o;
}

uint64_t TomlProxy::getString(char* str) const
{
    ::memcpy(str, data_ + values_[value_].start_, values_[value_].size_);
    str[values_[value_].size_] = '\0';
    return values_[value_].size_;
}

uint64_t TomlProxy::getStrLen() const
{
    return values_[value_].size_;
}

int64_t TomlProxy::getInt64() const
{
    const char* first = data_ + values_[value_].start_;
    const char* last = first + values_[value_].size_;
    int64_t value = 0;
    std::from_chars(first, last, value);
    return value;
}

double TomlProxy::getFloat64() const
{
    if (values_[value_].size_ <= 0) {
        return 0.0;
    }
    const char* first = data_ + values_[value_].start_;
#if _MSC_VER
    const char* last = first + values_[value_].size_;
    double value = 0.0;
    std::from_chars(first, last, value);
#else
    char buffer[127];
    uint64_t size = values_[value_].size_ < 128ULL ? values_[value_].size_ : 127ULL;
    ::memcpy(buffer, first, size);
    buffer[size] = '\0';
    double value = strtod(buffer, CPPTOML_NULL);
#endif
    return value;
}

bool TomlProxy::getBool() const
{
    if(equalsString("true")){
        return true;
    }
    if(equalsString("false")){
		return false;
	}
    return false;
}

bool TomlProxy::equalsString(const char* str) const
{
    return 0==::strncmp(str, data_ + values_[value_].start_, values_[value_].size_);
}

//--- TomlParser
//---------------------------------------
TomlParser::TomlParser(CPPTOML_MALLOC_TYPE allocator, CPPTOML_FREE_TYPE deallocator)
    : allocator_(allocator)
    , deallocator_(deallocator)
    , begin_(CPPTOML_NULL)
    , end_(CPPTOML_NULL)
    , current_(Invalid)
    , capacity_(0)
    , size_(0)
    , values_(CPPTOML_NULL)
{
    if(CPPTOML_NULL == allocator_ || CPPTOML_NULL == deallocator_) {
        allocator_ = ::malloc;
        deallocator_ = ::free;
    }
}

TomlParser::~TomlParser()
{
    deallocator_(values_);
}

bool TomlParser::parse(const char* begin, const char* end)
{
    CPPTOML_ASSERT(CPPTOML_NULL != begin);
    CPPTOML_ASSERT(CPPTOML_NULL != end);
    CPPTOML_ASSERT(begin <= end);
    begin_ = begin;
    end_ = end;
    current_ = Invalid;
    size_ = 0;
    current_ = add_table();
    const char* str = bom(begin_);
    while(str < end_) {
        str = parse_expression(str);
        if(CPPTOML_NULL == str) {
            return false;
        }
        const char* next = newline(str);
        if(str == next && str < end_) {
            return false;
        }
        str = next;
    }
    if (str < end_) {
        return false;
    }
    return validate(root());
}

TomlProxy TomlParser::root() const
{
    if(size_ <= 0) {
        return {Invalid, CPPTOML_NULL, CPPTOML_NULL};
    }
    return {0, begin_, values_};
}

int64_t TomlParser::next_symbol(const char*& str) const
{
    const uint8_t* u = reinterpret_cast<const uint8_t*>(str);
    if(u[0] <= 0x7FU) {
        str += 1;
        return u[0];
    }
    if(0b1100'0000U == (u[0] & 0b1110'0000U)) {
        if(end_ <= (str + 1)) {
            return -1;
        }
        if(0b1000'0000U != (u[1] & 0b1100'0000U)) {
            return -1;
        }
        str += 2;
        return ((static_cast<int64_t>(u[0]) & 0b1'1111U) << 6)
               + (static_cast<int64_t>(u[1]) & 0b11'1111U);
    }
    if(0b1110'0000U == (u[0] & 0b1111'0000U)) {
        if(end_ <= (str + 2)) {
            return -1;
        }
        if(0b1000'0000U != (u[1] & 0b1100'0000U)) {
            return -1;
        }
        if(0b1000'0000U != (u[2] & 0b1100'0000U)) {
            return -1;
        }
        str += 3;
        return ((static_cast<int64_t>(u[0]) & 0b1111U) << 12)
               + ((static_cast<int64_t>(u[1]) & 0b11'1111U) << 6)
               + ((static_cast<int64_t>(u[2]) & 0b11'1111U));
    }
    if(0b1111000U == (u[0] & 0b1111100U)) {
        if(end_ <= (str + 3)) {
            return -1;
        }
        if(0b1000'0000U != (u[1] & 0b1100'0000U)) {
            return -1;
        }
        if(0b1000'0000U != (u[2] & 0b1100'0000U)) {
            return -1;
        }
        if(0b1000'0000U != (u[3] & 0b1100'0000U)) {
            return -1;
        }
        return ((static_cast<int64_t>(u[0]) & 0b111U) << 18)
               + ((static_cast<int64_t>(u[1]) & 0b11'1111U) << 12)
               + ((static_cast<int64_t>(u[2]) & 0b11'1111U) << 6)
               + ((static_cast<int64_t>(u[3]) & 0b11'1111U));
    }
    return -1;
}

bool TomlParser::parse_unquated_key_char(const char*& str) const
{
    const char* next = str;
    int64_t c = next_symbol(next);
    if('a' <= c && c <= 'z') {
        str = next;
        return true;
    }
    if('A' <= c && c <= 'Z') {
        str = next;
        return true;
    }
    if('0' <= c && c <= '9') {
        str = next;
        return true;
    }
    switch(c) {
    case 0x2D:
    case 0x5F:
    case 0xB2:
    case 0xB3:
    case 0xB9:
    case 0xBC:
    case 0xBD:
    case 0xBE:
        str = next;
        return true;
    }
    if(0xC0 <= c && c <= 0xD6) {
        str = next;
        return true;
    }
    if(0xD8 <= c && c <= 0xF6) {
        str = next;
        return true;
    }
    if(0xF8 <= c && c <= 0x37D) {
        str = next;
        return true;
    }
    if(0x37F <= c && c <= 0x1FFF) {
        str = next;
        return true;
    }
    if((0x200C <= c && c <= 0x200D) || (0x203F <= c && c <= 0x2040)) {
        str = next;
        return true;
    }
    if((0x2070 <= c && c <= 0x218F) || (0x2460 <= c && c <= 0x24FF)) {
        str = next;
        return true;
    }
    if((0x2C00 <= c && c <= 0x2FEF) || (0x3001 <= c && c <= 0xD7FF)) {
        str = next;
        return true;
    }
    if((0xF900 <= c && c <= 0xFDCF) || (0xFDF0 <= c && c <= 0xFFFD)) {
        str = next;
        return true;
    }
    if(0x10000 <= c && c <= 0xEFFFF) {
        str = next;
        return true;
    }
    return false;
}

bool TomlParser::basic_char(const char*& str) const
{
    switch(str[0]) {
    case 0x09:
    case 0x20:
    case 0x21:
        ++str;
        return true;
    case 0x5C: // espace
        return escaped(str);
    }
    if(0x23 <= str[0] && str[0] <= 0x7E) {
        ++str;
        return true;
    }
    const char* next = parse_non_ascii(str);
    if(CPPTOML_NULL != next && next != str) {
        str = next;
        return true;
    }
    return false;
}

bool TomlParser::literal_char(const char*& str) const
{
    switch(str[0]) {
    case 0x09:
    case 0x20:
        ++str;
        return true;
    }
    if(0x20 <= str[0] && str[0] <= 0x26) {
        ++str;
        return true;
    }
    if(0x28 <= str[0] && str[0] <= 0x7E) {
        ++str;
        return true;
    }
    const char* next = parse_non_ascii(str);
    if(CPPTOML_NULL != next && next != str) {
        str = next;
        return true;
    }
    return false;
}

bool TomlParser::escaped(const char*& str) const
{
    CPPTOML_ASSERT(0x5C == str[0]);
    const char* next = str + 1;
    if(end_ <= next) {
        return false;
    }
    switch(next[0]) {
    case 0x22:
    case 0x5C:
    case 0x62:
    case 0x65:
    case 0x66:
    case 0x6E:
    case 0x72:
    case 0x74:
        str = next + 1;
        return true;
    case 0x55:
        str = next + 1;
        return parse_8hexdig(str);
    case 0x75:
        str = next + 1;
        return parse_4hexdig(str);
    //case 0x78:
    //    str = next + 1;
    //    return parse_2hexdig(str);
    }
    return false;
}

bool TomlParser::parse_2hexdig(const char*& str) const
{
    if(end_ <= (str + 1)) {
        return false;
    }
    if(!hexgidit(str[0]) || !hexgidit(str[1])) {
        return false;
    }
    str += 2;
    return true;
}

bool TomlParser::parse_4hexdig(const char*& str) const
{
    if(end_ <= (str + 3)) {
        return false;
    }
    if(!hexgidit(str[0]) || !hexgidit(str[1]) || !hexgidit(str[2]) || !hexgidit(str[3])) {
        return false;
    }
    str += 4;
    return true;
}

bool TomlParser::parse_8hexdig(const char*& str) const
{
    if(end_ <= (str + 7)) {
        return false;
    }
    for(uint32_t i = 0; i < 8; ++i) {
        if(!hexgidit(str[i])) {
            return false;
        }
    }
    str += 8;
    return true;
}

bool TomlParser::hexgidit(char c)
{
    switch(c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return true;
    default:
        return false;
    }
}

bool TomlParser::digit(char c)
{
    return '0' <= c && c <= '9';
}

const char* TomlParser::bom(const char* str) const
{
    std::ptrdiff_t size = std::distance(str, end_);
    if(3 <= size) {
        if(0xEF == str[0] && 0xBB == str[1] && 0xBF == str[2]) {
            str += 3;
        }
    }
    return str;
}

const char* TomlParser::newline(const char* str) const
{
    while(str < end_) {
        switch(str[0]) {
        case 0x0A:
        case 0x0D:
            ++str;
            break;
        default:
            return str;
        }
    }
    return str;
}

const char* TomlParser::whitespace(const char* str) const
{
    while(str < end_) {
        switch(str[0]) {
        case 0x09:
        case 0x20:
            ++str;
            break;
        default:
            return str;
        }
    }
    return str;
}

const char* TomlParser::comment(const char* str) const
{
    if(end_ <= str || '#' != str[0]) {
        return str;
    }
    ++str;
    while(str < end_) {
        // non-eol
        if(0x09 == str[0] || (0x20 <= str[0] && str[0] <= 0x7E)) {
            ++str;
            continue;
        }
        // non-ascii
        const char* next = parse_non_ascii(str);
        if(CPPTOML_NULL == next) {
            return CPPTOML_NULL;
        }
        if(next == str) {
            break;
        }
        str = next;
    }
    return str;
}

const char* TomlParser::ws_comment_newline(const char* str) const
{
    while(str < end_) {
        switch(str[0]) {
        case 0x09:
        case 0x20:
            str = whitespace(str);
            break;
        case 0x0A:
        case 0x0D:
            str = newline(str);
            break;
        case 0x23:
            str = comment(str);
            break;
        default:
            return str;
        }
    }
    return str;
}

const char* TomlParser::parse_non_ascii(const char* str) const
{
    const char* next = str;
    int64_t c = next_symbol(next);
    if(c < 0) {
        return CPPTOML_NULL;
    }
    if(0x80 <= c && c <= 0xD7FF) {
        return next;
    }
    if(0xE000 <= c && c <= 0x10FFFF) {
        return next;
    }
    return str;
}

const char* TomlParser::parse_expression(const char* str)
{
    std::tuple<const char*, uint32_t> pair;
    str = whitespace(str);
    switch(str[0]) {
    case '"': // quated-key
        pair = parse_keyvalue(str);
        break;
    case '\'': // quated-key
        pair = parse_keyvalue(str);
        break;
    case '[': // table
        pair = parse_table(str);
        break;
    case '#':
        str = comment(str);
        pair = {str, Invalid};
        break;
    default: {
        const char* next = str;
        if(!parse_unquated_key_char(next)) {
            return str;
        }
        pair = parse_keyvalue(str);
    } break;
    }
    if(CPPTOML_NULL == std::get<0>(pair)) {
        return CPPTOML_NULL;
    }
    str = std::get<0>(pair);
    str = whitespace(str);
    str = comment(str);
    return str;
}

std::tuple<const char*, uint32_t> TomlParser::parse_keyvalue(const char* str)
{
    std::tuple<const char*, uint32_t, uint32_t> keyvalue = parse_key(str, current_, KeyPlace::KeyValue);
    if(CPPTOML_NULL == std::get<0>(keyvalue) || end_ <= std::get<0>(keyvalue)) {
        return InvalidPair;
    }
    str = std::get<0>(keyvalue);
    if(0x3D != str[0]) {
        return InvalidPair;
    }
    str = whitespace(str + 1);
    if(end_ <= str) {
        return InvalidPair;
    }
    std::tuple<const char*, uint32_t> value = parse_value(str);
    if(CPPTOML_NULL == std::get<0>(value)) {
        return InvalidPair;
    }
    values_[std::get<1>(keyvalue)].size_ = std::get<1>(value);
    append(std::get<2>(keyvalue), std::get<1>(keyvalue));
    return {std::get<0>(value), std::get<1>(keyvalue)};
}

bool TomlParser::keyvalue(const char* str) const
{
    if(end_ <= str) {
        return false;
    }
    switch(str[0]) {
    case '"':
    case '\'':
        return true;
    default: {
        const char* next = str;
        if(parse_unquated_key_char(next)) {
            return true;
        }
    } break;
    }
    return false;
}

std::tuple<const char*, uint32_t, uint32_t> TomlParser::parse_key(const char* str, uint32_t current, KeyPlace place)
{
    for(;;) {
        const char* begin = str;
        switch(str[0]) {
        case '"': // quated-key
            str = parse_basic_string(str);
            break;
        case '\'': // quated-key
            str = parse_literal_string(str);
            break;
        default:
            const char* next = str;
            if(!parse_unquated_key_char(next)) {
                return InvalidTuple;
            }
            str = parse_unquated_key(str);
            break;
        }
        const char* end = str;
        uint32_t exist = find_keyvalue(current, begin, end);
        str = whitespace(str);
        if(end_ <= str) {
            return InvalidTuple;
        }
        if(str[0] != '.') {
            if(Invalid != exist) {
                switch(place) {
                case KeyPlace::KeyValue: {
                    uint64_t table = values_[exist].size_;
                    if(static_cast<uint32_t>(TomlType::Table) != values_[table].type_) {
                        return InvalidTuple;
                    }
                    // if (!has_child_table(table)) {
                    //     return InvalidTuple;
                    // }
                    return {str, exist, current};
                }
                case KeyPlace::Table: {
                    uint64_t table = values_[exist].size_;
                    if(static_cast<uint32_t>(TomlType::Table) != values_[table].type_) {
                        return InvalidTuple;
                    }
                    if(!has_child_table(static_cast<uint32_t>(table))) {
                        return InvalidTuple;
                    }
                    return {str, exist, current};
                }
                case KeyPlace::ArrayTable: {
                    uint32_t array = static_cast<uint32_t>(values_[exist].size_);
                    if(static_cast<uint32_t>(TomlType::Array) != values_[array].type_) {
                        return InvalidTuple;
                    }
                    return {str, exist, current};
                }
                default:
                    return InvalidTuple;
                }
            }
            uint32_t keyvalue = add_keyvalue(begin, end);
            return {str, keyvalue, current};
        }
        if(Invalid != exist) {
            current = static_cast<uint32_t>(values_[exist].size_);
            if(Invalid == current) {
                return InvalidTuple;
            }
            if (static_cast<uint32_t>(TomlType::Table) == values_[current].type_) {
            } else if(static_cast<uint32_t>(TomlType::Array) == values_[current].type_) {
                uint32_t table = Invalid;
                if(values_[current].size_ <= 0) {
                    table = add_table();
                    append(current, table);
                } else {
                    table = find_table(current);
                    if(Invalid == table) {
                        table = add_table();
                        append(current, table);
                    }
                }
                if(Invalid == table) {
                    return InvalidTuple;
                }
                current = table;
            } else {
                return InvalidTuple;
            }
        } else {
            uint32_t keyvalue = add_keyvalue(begin, end);
            if(KeyPlace::ArrayTable == place) {
                //uint32_t array = add_array();
                //values_[keyvalue].size_ = array;
                //append(current, keyvalue);

                uint32_t table = add_table();
                values_[keyvalue].size_ = table;
                append(current, keyvalue);
                current = table;

            } else {
                uint32_t table = add_table();
                values_[keyvalue].size_ = table;
                append(current, keyvalue);
                current = table;
            }
        }
        str = whitespace(str + 1);
        if(end_ <= str) {
            return InvalidTuple;
        }
    }
}

const char* TomlParser::parse_unquated_key(const char* str) const
{
    while(str < end_) {
        if(!parse_unquated_key_char(str)) {
            return str;
        }
    }
    return str;
}

std::tuple<const char*, uint32_t> TomlParser::parse_value(const char* str)
{
    const char* next = CPPTOML_NULL;
    TomlType type = TomlType::Invalid;
    switch(str[0]) {
    case '"':
        type = TomlType::String;
        if((str + 2) < end_ && '"' == str[1] && '"' == str[2]) {
            next = parse_ml_basic_string(str);
        } else {
            next = parse_basic_string(str);
        }
        break;
    case '\'':
        type = TomlType::String;
        if((str + 2) < end_ && '\'' == str[1] && '\'' == str[2]) {
            next = parse_ml_literal_string(str);
        } else {
            next = parse_literal_string(str);
        }
        break;
    case 't':
        type = TomlType::True;
        next = parse_true(str);
        break;
    case 'f':
        type = TomlType::False;
        next = parse_false(str);
        break;
    case '[':
        return parse_array(str);
    case '{':
        return parse_inline_table(str);
    default:
        if(('0' <= str[0] && str[0] <= '9') || '-' == str[0] || '+' == str[0]) {
            auto [n, v] = parse_number(str);
            if(CPPTOML_NULL != n) {
                return {n, v};
            }
        }
        break;
    }
    if(CPPTOML_NULL != next) {
        uint32_t value = add_value(type, str, next);
        return {next, value};
    }
    return InvalidPair;
}

bool TomlParser::value(const char* str) const
{
    if(end_ <= str) {
        return false;
    }
    switch(str[0]) {
    case '"':
    case '\'':
    case 't':
    case 'f':
    case '[':
    case '{':
        return true;
    default:
        if('0' <= str[0] && str[0] <= '9') {
            return true;
        }
        break;
    }
    return false;
}

const char* TomlParser::parse_basic_string(const char* str)
{
    CPPTOML_ASSERT('"' == str[0]);
    ++str;
    while(str < end_) {
        if('"' == str[0]) {
            return str + 1;
        }
        if(basic_char(str)) {
            continue;
        }
        break;
    }
    return CPPTOML_NULL;
}

const char* TomlParser::parse_literal_string(const char* str)
{
    CPPTOML_ASSERT('\'' == str[0]);
    ++str;
    while(str < end_) {
        if('\'' == str[0]) {
            return str + 1;
        }
        if(literal_char(str)) {
            continue;
        }
        break;
    }
    return CPPTOML_NULL;
}

const char* TomlParser::parse_ml_basic_string(const char* str)
{
    CPPTOML_ASSERT('"' == str[0] && '"' == str[1] && '"' == str[2]);
    str += 3;
    str = newline(str);

    for(;;) {
        const char* next = parse_mlb_content(str);
        if(next == str) {
            break;
        }
        str = next;
    }

    while(str < end_) {
        if(!parse_mlb_quotes(str)) {
            break;
        }
        while(str < end_) {
            const char* next = parse_mlb_content(str);
            if(next == str) {
                break;
            }
            str = next;
        }
    }
    parse_mlb_quotes(str);
    if(end_ <= (str + 2)) {
        return CPPTOML_NULL;
    }
    if('"' == str[0] && '"' == str[1] && '"' == str[2]) {
        return str + 3;
    }
    return CPPTOML_NULL;
}

const char* TomlParser::parse_mlb_content(const char* str)
{
    switch(str[0]) {
    case 0x0A:
    case 0x0D:
        return newline(str);
    case 0x5C: // escape
        return parse_mlb_escaped_nl(str);
    default:
        if(basic_char(str)) {
            return str;
        }
        break;
    }
    return str;
}

const char* TomlParser::parse_mlb_escaped_nl(const char* str)
{
    CPPTOML_ASSERT(0x5C == str[0]);
    str = whitespace(str + 1);
    str = newline(str);
    while(str < end_) {
        switch(str[0]) {
        case 0x0A:
        case 0x0D:
            str = newline(str);
            break;
        case 0x09:
        case 0x20:
            str = whitespace(str);
            break;
        default:
            return str;
        }
    }
    return str;
}

bool TomlParser::parse_mlb_quotes(const char*& str)
{
    const char* next = str;
    uint32_t count = 0;
    while(next < end_) {
        if('"' == next[0]) {
            ++count;
            ++next;
            continue;
        }
        break;
    }
    switch(count) {
    case 0:
        return false;
    case 1:
        return false;
    case 2:
        str += 2;
        return true;
    case 3:
        return false;
    case 4:
        return false;
    default:
        next = str;
        while(5 <= count) {
            count -= 2;
            next += 2;
        }
        if(3 == count || 0 == count) {
            str = next;
            return true;
        }
        return false;
    }
}

const char* TomlParser::parse_ml_literal_string(const char* str)
{
    CPPTOML_ASSERT('\'' == str[0] && '\'' == str[1] && '\'' == str[2]);
    str += 3;
    str = newline(str);

    for(;;) {
        const char* next = parse_mll_content(str);
        if(next == str) {
            break;
        }
        str = next;
    }

    while(str < end_) {
        if(!parse_mll_quotes(str)) {
            break;
        }
        while(str < end_) {
            const char* next = parse_mll_content(str);
            if(next == str) {
                break;
            }
            str = next;
        }
    }
    parse_mll_quotes(str);
    if(end_ <= (str + 2)) {
        return CPPTOML_NULL;
    }
    if('\'' == str[0] && '\'' == str[1] && '\'' == str[2]) {
        return str + 3;
    }
    return CPPTOML_NULL;
}

const char* TomlParser::parse_mll_content(const char* str)
{
    switch(str[0]) {
    case 0x0A:
    case 0x0D:
        return newline(str);
    default:
        if(literal_char(str)) {
            return str;
        }
        break;
    }
    return str;
}

bool TomlParser::parse_mll_quotes(const char*& str)
{
    const char* next = str;
    uint32_t count = 0;
    while(next < end_) {
        if('\'' == next[0]) {
            ++count;
            ++next;
            continue;
        }
        break;
    }
    switch(count) {
    case 0:
        return false;
    case 1:
        str += 1;
        return true;
    case 2:
        str += 2;
        return true;
    case 3:
        return false;
    case 4:
        str += 1;
        return true;
    default:
        next = str;
        while(5 <= count) {
            count -= 2;
            next += 2;
        }
        if(3 == count || 0 == count) {
            str = next;
            return true;
        }
        return false;
    }
}

std::tuple<const char*, uint32_t> TomlParser::parse_table(const char* str)
{
    CPPTOML_ASSERT('[' == str[0]);
    if(end_ <= (str + 1)) {
        return InvalidPair;
    }
    current_ = 0;
    if(str[1] == '[') {
        return parse_array_table(str);
    } else {
        return parse_std_table(str);
    }
}

std::tuple<const char*, uint32_t> TomlParser::parse_std_table(const char* str)
{
    CPPTOML_ASSERT('[' == str[0]);
    str = whitespace(str + 1);
    if(end_ <= str) {
        return InvalidPair;
    }
    std::tuple<const char*, uint32_t, uint32_t> keyvalue = parse_key(str, current_, KeyPlace::Table);
    str = std::get<0>(keyvalue);
    if(CPPTOML_NULL == str) {
        return InvalidPair;
    }
    str = whitespace(str);
    if(end_ <= str || str[0] != ']') {
        return InvalidPair;
    }
    uint32_t table = add_table();
    values_[std::get<1>(keyvalue)].size_ = table;
    append(std::get<2>(keyvalue), std::get<1>(keyvalue));
    current_ = table;
    return {str + 1, table};
}

std::tuple<const char*, uint32_t> TomlParser::parse_array_table(const char* str)
{
    CPPTOML_ASSERT('[' == str[0] && '[' == str[1]);
    str = whitespace(str + 2);
    if(end_ <= str) {
        return InvalidPair;
    }
    std::tuple<const char*, uint32_t, uint32_t> keyvalue = parse_key(str, current_, KeyPlace::ArrayTable);
    str = std::get<0>(keyvalue);
    if(CPPTOML_NULL == str) {
        return InvalidPair;
    }
    str = whitespace(str);
    if(end_ <= (str + 1) || (str[0] != ']' || str[1] != ']')) {
        return InvalidPair;
    }
    CPPTOML_ASSERT(static_cast<uint32_t>(TomlType::KeyValue) == values_[std::get<1>(keyvalue)].type_);
    uint32_t array = static_cast < uint32_t>(values_[std::get<1>(keyvalue)].size_);
    if(Invalid == array) {
        array = add_array();
        values_[std::get<1>(keyvalue)].size_ = array;
        append(std::get<2>(keyvalue), std::get<1>(keyvalue));
    }
    CPPTOML_ASSERT(static_cast<uint32_t>(TomlType::Array) == values_[array].type_);
    uint32_t table = add_table();
    append(array, table);
    current_ = table;
    return {str + 2, current_};
}

const char* TomlParser::parse_true(const char* str)
{
    CPPTOML_ASSERT('t' == str[0]);
    if(end_ <= (str + 3)) {
        return CPPTOML_NULL;
    }
    if('r' == str[1] && 'u' == str[2] && 'e' == str[3]) {
        return str + 4;
    }
    return CPPTOML_NULL;
}

const char* TomlParser::parse_false(const char* str)
{
    if(end_ <= (str + 4)) {
        return CPPTOML_NULL;
    }
    if('a' == str[1] && 'l' == str[2] && 's' == str[3] && 'e' == str[4]) {
        return str + 5;
    }
    return CPPTOML_NULL;
}

std::tuple<const char*, uint32_t> TomlParser::parse_array(const char* str)
{
    CPPTOML_ASSERT('[' == str[0]);
    ++str;

    uint32_t array = add_array();

    bool sep = false;
    while(str < end_) {
        str = ws_comment_newline(str);
        if(value(str)) {
            auto [n, v] = parse_value(str);
            if(CPPTOML_NULL == n) {
                return InvalidPair;
            } else {
                append(array, v);
            }
            str = n;
            sep = false;
        }
        str = ws_comment_newline(str);
        if(end_ <= str) {
            break;
        }
        if(']' == str[0]) {
            return {str + 1, array};
        }
        if(',' == str[0]) {
            if(sep) {
                return InvalidPair;
            }
            ++str;
            sep = true;
            continue;
        }
        break;
    }
    return InvalidPair;
}

std::tuple<const char*, uint32_t> TomlParser::parse_inline_table(const char* str)
{
    CPPTOML_ASSERT('{' == str[0]);
    uint32_t previous = current_;
    uint32_t table = add_table();
    current_ = table;
    str = whitespace(str + 1);
    while(str < end_) {
        if(keyvalue(str)) {
            auto [n, v] = parse_keyvalue(str);
            if(CPPTOML_NULL == n) {
                return InvalidPair;
            }
            append(table, v);
            str = n;
        }
        str = whitespace(str);
        if(end_ <= str) {
            break;
        }
        if('}' == str[0]) {
            current_ = previous;
            return {str + 1, table};
        }
        if(',' == str[0]) {
            str = whitespace(str + 1);
            continue;
        }
        break;
    }
    return InvalidPair;
}

std::tuple<const char*, uint32_t> TomlParser::parse_number(const char* str)
{
    switch(number_type(str)) {
    case TomlType::Integer:
        return parse_integer(str);
    case TomlType::Hex:
        return parse_hex(str);
    case TomlType::Oct:
        return parse_oct(str);
    case TomlType::Bin:
        return parse_bin(str);
    case TomlType::Float:
        return parse_float(str);
    case TomlType::Inf:
        return parse_inf(str);
    case TomlType::NaN:
        return parse_nan(str);
    case TomlType::DateTime:
        return parse_datetime(str);
    default:
        return InvalidPair;
    }
}

TomlType TomlParser::number_type(const char* str)
{
    CPPTOML_ASSERT(str < end_);
    bool sign = false;
    if('-' == str[0] || '+' == str[0]) {
        sign = true;
        ++str;
        if(end_ <= str) {
            return TomlType::Invalid;
        }
    }
    switch(str[0]) {
    case '0':
        if((str + 1) < end_) {
            switch(str[1]) {
            case 'x':
                return !sign ? TomlType::Hex : TomlType::Invalid;
            case 'o':
                return !sign ? TomlType::Oct : TomlType::Invalid;
            case 'b':
                return !sign ? TomlType::Bin : TomlType::Invalid;
            }
        }
        break;
    case 'i':
        return TomlType::Inf;
    case 'n':
        return TomlType::NaN;
    case '_':
        return TomlType::Invalid;
    default:
        if(!digit(str[0])) {
            return TomlType::Invalid;
        }
        break;
    }
    uint32_t rank = 0;
    bool exp = false;
    while(str < end_) {
        switch(str[0]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            ++rank;
            break;
        case '_':
            break;
        case '.':
            return 0 < rank ? TomlType::Float : TomlType::Invalid;
        case '-':
            return exp ? TomlType::Float : TomlType::DateTime;
        case '+':
            return exp ? TomlType::Float : TomlType::Invalid;
        case 'e':
        case 'E':
            exp = true;
            rank = 0;
            break;
        case 't':
        case 'T':
        case 'Z':
        case ':':
            return TomlType::DateTime;
        default:
            if(exp) {
                return 0 < rank ? TomlType::Float : TomlType::Invalid;
            } else {
                return TomlType::Integer;
            }
        }
        ++str;
    }
    return 0 < rank ? TomlType::Integer : TomlType::Invalid;
}

std::tuple<const char*, uint32_t> TomlParser::parse_integer(const char* str)
{
    CPPTOML_ASSERT(str < end_);
    bool top_zero = false;
    uint32_t rank = 0;
    bool sign = false;
    bool sep = false;
    bool loop = true;
    const char* begin = str;
    while(str < end_ && loop) {
        switch(str[0]) {
        case '0':
            if(rank <= 0) {
                top_zero = true;
            }
            [[fallthrough]];
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            ++rank;
            ++str;
            sep = false;
            break;
        case '_':
            if(rank <= 0 || sep) {
                return InvalidPair;
            }
            ++str;
            sep = true;
            break;
        case '-':
        case '+':
            if(0 < rank) {
                return InvalidPair;
            }
            if(sign) {
                return InvalidPair;
            }
            sign = true;
            ++str;
            break;
        default:
            loop = false;
            break;
        }
    }
    if(sep) {
        return InvalidPair;
    }
    if(rank <= 0) {
        return InvalidPair;
    }
    if(top_zero && 1 < rank) {
        return InvalidPair;
    }
    uint32_t value = add_value(TomlType::Integer, begin, str);
    return {str, value};
}

std::tuple<const char*, uint32_t> TomlParser::parse_hex(const char* str)
{
    CPPTOML_ASSERT(str < end_);
    if(end_ <= (str + 1) || '0' != str[0] || 'x' != str[1]) {
        return InvalidPair;
    }
    const char* begin = str;
    str += 2;
    uint32_t rank = 0;
    bool sep = false;
    while(str < end_) {
        if(hexgidit(str[0])) {
            ++rank;
            ++str;
            sep = false;
            continue;
        }
        if('_' == str[0]) {
            if(rank <= 0 || sep) {
                return InvalidPair;
            }
            ++str;
            sep = true;
            continue;
        }
        break;
    }
    if(sep) {
        return InvalidPair;
    }
    if(rank <= 0) {
        return InvalidPair;
    }
    uint32_t value = add_value(TomlType::Hex, begin, str);
    return {str, value};
}

std::tuple<const char*, uint32_t> TomlParser::parse_oct(const char* str)
{
    CPPTOML_ASSERT(str < end_);
    if(end_ <= (str + 1) || '0' != str[0] || 'o' != str[1]) {
        return InvalidPair;
    }
    const char* begin = str;
    str += 2;
    uint32_t rank = 0;
    bool sep = false;
    while(str < end_) {
        if('0' <= str[0] && str[0] <= '7') {
            ++rank;
            ++str;
            sep = false;
            continue;
        }
        if('_' == str[0]) {
            if(rank <= 0 || sep) {
                return InvalidPair;
            }
            ++str;
            sep = true;
            continue;
        }
        break;
    }
    if(sep) {
        return InvalidPair;
    }
    if(rank <= 0) {
        return InvalidPair;
    }
    uint32_t value = add_value(TomlType::Oct, begin, str);
    return {str, value};
}

std::tuple<const char*, uint32_t> TomlParser::parse_bin(const char* str)
{
    CPPTOML_ASSERT(str < end_);
    if(end_ <= (str + 1) || '0' != str[0] || 'b' != str[1]) {
        return InvalidPair;
    }
    const char* begin = str;
    str += 2;
    uint32_t rank = 0;
    bool sep = false;
    while(str < end_) {
        if('0' <= str[0] && str[0] <= '1') {
            ++rank;
            ++str;
            sep = false;
            continue;
        }
        if('_' == str[0]) {
            if(rank <= 0 || sep) {
                return InvalidPair;
            }
            ++str;
            sep = true;
            continue;
        }
        break;
    }
    if(sep) {
        return InvalidPair;
    }
    if(rank <= 0) {
        return InvalidPair;
    }
    uint32_t value = add_value(TomlType::Bin, begin, str);
    return {str, value};
}

std::tuple<const char*, uint32_t> TomlParser::parse_float(const char* str)
{
    auto [n, v] = parse_integer(str);
    if(CPPTOML_NULL == n || end_ <= n) {
        return InvalidPair;
    }
    switch(n[0]) {
    case '.':
        n = parse_frac(n);
        break;
    case 'e':
    case 'E':
        n = parse_exp(n);
        break;
    default:
        break;
    }
    if(CPPTOML_NULL == n) {
        return InvalidPair;
    }
    values_[v].type_ = static_cast<uint32_t>(TomlType::Float);
    values_[v].size_ = static_cast<uint64_t>(n - str);
    return {n, v};
}

const char* TomlParser::parse_frac(const char* str)
{
    CPPTOML_ASSERT('.' == str[0]);
    ++str;
    str = parse_zero_prefixable_int(str);
    if(str == CPPTOML_NULL) {
        return CPPTOML_NULL;
    }
    if(str < end_ && ('e' == str[0] || 'E' == str[0])) {
        return parse_exp(str);
    }
    return str;
}

const char* TomlParser::parse_exp(const char* str)
{
    CPPTOML_ASSERT('e' == str[0] || 'E' == str[0]);
    ++str;
    if(end_ <= str) {
        return CPPTOML_NULL;
    }
    if('-' == str[0] || '+' == str[0]) {
        ++str;
    }
    return parse_zero_prefixable_int(str);
}

const char* TomlParser::parse_zero_prefixable_int(const char* str)
{
    uint32_t rank = 0;
    bool sep = false;
    while(str < end_) {
        if(digit(str[0])) {
            ++rank;
            ++str;
            sep = false;
            continue;
        }
        if('_' == str[0]) {
            if(rank <= 0 || sep) {
                return CPPTOML_NULL;
            }
            ++str;
            sep = true;
            continue;
        }
        break;
    }
    if(sep) {
        return CPPTOML_NULL;
    }
    if(rank <= 0) {
        return CPPTOML_NULL;
    }
    return str;
}

std::tuple<const char*, uint32_t> TomlParser::parse_inf(const char* str)
{
    const char* begin = str;
    if('-' == str[0] || '+' == str[0]) {
        ++str;
    }
    if(end_ <= (str + 2) || 'i' != str[0] || 'n' != str[1] || 'f' != str[2]) {
        return InvalidPair;
    }
    str += 3;
    uint32_t value = add_value(TomlType::Inf, begin, str);
    return {str, value};
}

std::tuple<const char*, uint32_t> TomlParser::parse_nan(const char* str)
{
    const char* begin = str;
    if('-' == str[0] || '+' == str[0]) {
        ++str;
    }
    if(end_ <= (str + 2) || 'n' != str[0] || 'a' != str[1] || 'n' != str[2]) {
        return InvalidPair;
    }
    str += 3;
    uint32_t value = add_value(TomlType::Inf, begin, str);
    return {str, value};
}

std::tuple<const char*, uint32_t> TomlParser::parse_datetime(const char* str)
{
    const char* begin = str;
    if(end_ <= (str + 2)) {
        return InvalidPair;
    }
    if(':' == str[2]) {
        const char* n = parse_partial_time(str);
        if(CPPTOML_NULL == n) {
            return InvalidPair;
        }
        uint32_t v = add_value(TomlType::DateTime, begin, n);
        return {n, v};
    }

    str = parse_fulldate(str);
    if(CPPTOML_NULL == str) {
        return InvalidPair;
    }
    if(end_ <= (str + 3) || ':' != str[3]) {
        uint32_t v = add_value(TomlType::DateTime, begin, str);
        return {str, v};
    }
    if('T' != str[0] && 't' != str[0] && ' ' != str[0]) {
        return InvalidPair;
    }
    str = parse_partial_time(str + 1);
    if(CPPTOML_NULL == str) {
        return InvalidPair;
    }
    if((str + 1) < end_ && ('z' == str[0] || 'Z' == str[0])) {
        uint32_t v = add_value(TomlType::DateTime, begin, str + 1);
        return {str + 1, v};
    }
    if(end_ <= (str + 3) || (':' != str[2] && ':' != str[3])) {
        uint32_t v = add_value(TomlType::DateTime, begin, str);
        return {str, v};
    }
    str = parse_timeoffset(str);
    if(CPPTOML_NULL == str) {
        return InvalidPair;
    }
    uint32_t v = add_value(TomlType::DateTime, begin, str);
    return {str, v};
}

const char* TomlParser::parse_fulldate(const char* str)
{
    if(end_ <= (str + 9)) {
        return CPPTOML_NULL;
    }
    if(!digit(str[0]) || !digit(str[1]) || !digit(str[2]) || !digit(str[3])) {
        return CPPTOML_NULL;
    }
    if('-' != str[4]) {
        return CPPTOML_NULL;
    }
    if(!digit(str[5]) || !digit(str[6])) {
        return CPPTOML_NULL;
    }
    if('-' != str[7]) {
        return CPPTOML_NULL;
    }
    if(!digit(str[8]) || !digit(str[9])) {
        return CPPTOML_NULL;
    }
    return str + 10;
}

const char* TomlParser::parse_partial_time(const char* str)
{
    if(end_ <= (str + 4)) {
        return CPPTOML_NULL;
    }
    if(!digit(str[0]) || !digit(str[1])) {
        return CPPTOML_NULL;
    }
    if(':' != str[2]) {
        return CPPTOML_NULL;
    }
    if(!digit(str[3]) || !digit(str[4])) {
        return CPPTOML_NULL;
    }
    str += 5;
    if(end_ <= str || ':' != str[0]) {
        return CPPTOML_NULL;
    }
    if(end_ <= (str + 2)) {
        return CPPTOML_NULL;
    }
    if(!digit(str[1]) || !digit(str[2])) {
        return CPPTOML_NULL;
    }
    str += 3;
    if(end_ <= str || '.' != str[0]) {
        return str;
    }
    ++str;
    uint32_t count = 0;
    while(str < end_) {
        if(!digit(str[0])) {
            break;
        }
        ++count;
        ++str;
    }
    return (0 < count) ? str : CPPTOML_NULL;
}

const char* TomlParser::parse_timeoffset(const char* str)
{
    if('-' == str[0] || '+' == str[0]) {
        ++str;
    }
    if(end_ <= (str + 4)) {
        return CPPTOML_NULL;
    }
    if(!digit(str[0]) || !digit(str[1])) {
        return CPPTOML_NULL;
    }
    if(':' != str[2]) {
        return CPPTOML_NULL;
    }
    if(!digit(str[3]) || !digit(str[4])) {
        return CPPTOML_NULL;
    }
    return str + 5;
}

bool TomlParser::strcmp(const char* s0, const char* e0, const char* s1, const char* e1)
{
    while(s0 < e0 && s1 < e1) {
        if(s0[0] != s1[0]) {
            return false;
        }
        ++s0;
        ++s1;
    }
    return e0<=s0 && e1<=s1;
}

uint32_t TomlParser::find_keyvalue(uint32_t table, const char* begin, const char* end) const
{
    CPPTOML_ASSERT(Invalid != table);
    CPPTOML_ASSERT(CPPTOML_NULL != begin);
    CPPTOML_ASSERT(CPPTOML_NULL != end);
    uint32_t node = static_cast<uint32_t>(values_[table].start_);
    while(Invalid != node) {
        CPPTOML_ASSERT(static_cast<uint32_t>(TomlType::KeyValue) == values_[node].type_);
        uint64_t key = values_[node].start_;
        const char* s1 = begin_ + values_[key].start_;
        const char* e1 = s1 + values_[key].size_;
        if(TomlParser::strcmp(begin, end, s1, e1)) {
            return node;
        }
        node = values_[node].next_;
    }
    return Invalid;
}

uint32_t TomlParser::find_table(uint32_t array) const
{
    CPPTOML_ASSERT(Invalid != array);
    uint32_t node = static_cast<uint32_t>(values_[array].start_);
    if (Invalid == node) {
        return Invalid;
    }
    return find_table_node(node);
}

uint32_t TomlParser::find_table_node(uint32_t node) const
{
    CPPTOML_ASSERT(Invalid != node);
    if (Invalid == values_[node].next_) {
        return static_cast<uint32_t>(TomlType::Table) == values_[node].type_? node : Invalid;
    }
    uint32_t next = find_table_node(values_[node].next_);
    if (Invalid == next) {
        return static_cast<uint32_t>(TomlType::Table) == values_[node].type_ ? node : Invalid; 
    } else {
        return next;
    }
}

bool TomlParser::has_child_table(uint32_t table) const
{
    CPPTOML_ASSERT(Invalid != table);
    if(Invalid == values_[table].start_) {
        return false;
    }
    uint32_t node = static_cast<uint32_t>(values_[table].start_);
    while(Invalid != node) {
        CPPTOML_ASSERT(static_cast<uint32_t>(TomlType::KeyValue) == values_[node].type_);
        if (static_cast<uint32_t>(TomlType::Table) == values_[values_[node].size_].type_) {
            return true;
        }
        node = values_[node].next_;
    }
    return false;
}

void TomlParser::clear()
{
    current_ = 0;
}

uint32_t TomlParser::add()
{
    if(capacity_ <= size_) {
        uint32_t capacity = capacity_ + Expand;
        TomlValue* values = reinterpret_cast<TomlValue*>(allocator_(sizeof(TomlValue) * capacity));
        if(0 < capacity_) {
            ::memcpy(values, values_, sizeof(TomlValue) * capacity_);
        }
        deallocator_(values_);
        capacity_ = capacity;
        values_ = values;
    }
    uint32_t current = size_;
    ++size_;
    return current;
}

uint32_t TomlParser::add_keyvalue(const char* str, const char* end)
{
    uint32_t key = add_value(TomlType::Key, str, end);
    uint32_t value = add();
    values_[value].start_ = key;
    values_[value].size_ = Invalid;
    values_[value].next_ = Invalid;
    values_[value].type_ = static_cast<uint32_t>(TomlType::KeyValue);
    return value;
}

uint32_t TomlParser::add_value(TomlType type, const char* str, const char* end)
{
    uint32_t value = add();
    values_[value].start_ = static_cast<uint64_t>(str - begin_);
    values_[value].size_ = static_cast<uint64_t>(end - str);
    values_[value].next_ = Invalid;
    values_[value].type_ = static_cast<uint32_t>(type);
    return value;
}

uint32_t TomlParser::add_table()
{
    uint32_t value = add();
    values_[value].start_ = Invalid;
    values_[value].size_ = 0;
    values_[value].next_ = Invalid;
    values_[value].type_ = static_cast<uint32_t>(TomlType::Table);
    return value;
}

uint32_t TomlParser::add_array()
{
    uint32_t value = add();
    values_[value].start_ = Invalid;
    values_[value].size_ = 0;
    values_[value].next_ = Invalid;
    values_[value].type_ = static_cast<uint32_t>(TomlType::Array);
    return value;
}

void TomlParser::append(uint32_t parent, uint32_t value)
{
    values_[parent].size_ += 1;
    if(Invalid == values_[parent].start_ || value == values_[parent].start_) {
        values_[parent].start_ = value;
        return;
    }
    uint32_t node = static_cast<uint32_t>(values_[parent].start_);
    for (;;) {
        if (node == value) {
            return;
        }
        if (Invalid == values_[node].next_) {
            values_[node].next_ = value;
            return;
        }
        node = values_[node].next_;
    }
}
} // namespace cpptoml
