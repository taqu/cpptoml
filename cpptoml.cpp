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

TomlParser::TomlParser(cpptoml_malloc allocator, cpptoml_free deallocator, u32 max_nests)
    : allocator_(allocator)
    , deallocator_(deallocator)
    , max_nests_(max_nests)
    , nest_count_(0)
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
    nest_count_ = 0;
    begin_ = current_ = head;
    end_ = end;
    buffer_.clear();
    skip_bom();
    CppTomlResult result;
    while(current_ < end_) {
        result = parse_expression();
        if(!result.success_) {
            return false;
        }
        skip_newline();
    }
    return end_ <= current_;
}

u32 TomlParser::size() const
{
    return 0;
}

bool TomlParser::isAlpha(Char c)
{
    return (0x41U<=c&&c<=0x5AU) || (0x61U<=c&&c<=0x7AU);
}

bool TomlParser::isDigit(Char c)
{
    return 0x30U<=c&&c<=0x39U;
}

bool TomlParser::isHexDigit(Char c)
{
    return isDigit(c) || ('A'<=c&&c<='F');
}

bool TomlParser::isEnd() const
{
    return end_ <= current_;
}

bool TomlParser::isQuotedKey() const
{
    CPPTOML_ASSERT(current_ < end_);
    if(0x22U == *current_
       || 0x27U == *current_) {
        return true;
    }
    return false;
}

bool TomlParser::isUnquotedKey() const
{
    CPPTOML_ASSERT(current_ < end_);
    if(isAlpha(*current_)
       || isDigit(*current_)
       || 0x2DU == *current_
       || 0x5FU == *current_) {
        return true;
    }
    return false;
}

bool TomlParser::isTable() const
{
    CPPTOML_ASSERT(current_ < end_);
    return 0x5BU == *current_;
}

void TomlParser::skip_bom()
{
#ifdef CPPTOML_CPP
    size_t size = std::distance(current_, end_);
#else
    size_t size = (size_t)(current_ - end_);
#endif
    if(3 <= size) {
        u8 c0 = *reinterpret_cast<const u8*>(&current_[0]);
        u8 c1 = *reinterpret_cast<const u8*>(&current_[1]);
        u8 c2 = *reinterpret_cast<const u8*>(&current_[2]);
        if(0xEFU == c0 && 0xBBU == c1 && 0xBFU == c2) {
            current_ += 3;
        }
    }
}

void TomlParser::skip_space()
{
    while(current_ < end_) {
        switch(*current_) {
        case 0x09:
        case 0x20:
            ++current_;
            break;
        default:
            return;
        }
    }
}

void TomlParser::skip_newline()
{
    while(current_ < end_) {
        switch(*current_) {
        case 0x0A:
        case 0x0D:
            ++current_;
            break;
        default:
            return;
        }
    }
}

void TomlParser::skip_spacenewline()
{
    while(current_ < end_) {
        switch(*current_) {
        case 0x09:
        case 0x20:
        case 0x0A:
        case 0x0D:
            ++current_;
            break;
        default:
            return;
        }
    }
}

bool TomlParser::skip_utf8_1()
{
    CPPTOML_ASSERT(current_ < end_);
    static const u8 mask0 = 0b11000000U;
    static const u8 value0 = 0b10000000U;
    if(end_ <= (current_ + 1)) {
        return false;
    }
    ++current_;
    return value0 == (*current_ & mask0);
}

bool TomlParser::skip_utf8_2()
{
    CPPTOML_ASSERT(current_ < end_);
    static const u8 mask0 = 0b11000000U;
    static const u8 value0 = 0b10000000U;
    if(end_ <= (current_ + 2)) {
        return false;
    }
    ++current_;
    if(value0 != (*current_ & mask0)) {
        return false;
    }
    ++current_;
    return value0 == (*current_ & mask0);
}

bool TomlParser::skip_utf8_3()
{
    CPPTOML_ASSERT(current_ < end_);
    static const u8 mask0 = 0b11000000U;
    static const u8 value0 = 0b10000000U;
    if(end_ <= (current_ + 3)) {
        return false;
    }
    ++current_;
    if(value0 != (*current_ & mask0)) {
        return false;
    }
    ++current_;
    if(value0 != (*current_ & mask0)) {
        return false;
    }
    ++current_;
    return value0 == (*current_ & mask0);
}

bool TomlParser::skip_noneol()
{
    static const u8 mask1 = 0b11100000U;
    static const u8 mask2 = 0b11110000U;
    static const u8 mask3 = 0b11111000U;
    static const u8 value1 = 0b11000000U;
    static const u8 value2 = 0b11100000U;
    static const u8 value3 = 0b11110000U;

    while(current_ < end_) {
        if(0x09U == *current_) {
            ++current_;
        } else if(0x20U <= *current_ && *current_ <= 0x7FU) {
            ++current_;
        } else if(value1 == (*current_ & mask1)) {
            if(!skip_utf8_1()) {
                current_ = end_;
                return false;
            }
            ++current_;
        } else if(value2 == (*current_ & mask2)) {
            if(!skip_utf8_2()) {
                current_ = end_;
                return false;
            }
            ++current_;
        } else if(value3 == (*current_ & mask3)) {
            if(!skip_utf8_3()) {
                current_ = end_;
                return false;
            }
            ++current_;
        } else {
            current_ = end_;
            return false;
        }
    } // while
    return true;
}

void TomlParser::skip_comment()
{
    if('#' != *current_) {
        return;
    }
    ++current_;
    skip_noneol();
}

CppTomlResult TomlParser::parse_expression()
{
    skip_space();
    CppTomlResult result = {true, 0};
    if(isEnd()) {
        return result;
    }
    if(isQuotedKey() || isUnquotedKey()) {
        result = parse_keyval();
    } else if(isTable()) {
        result = parse_table();
    }
    skip_space();
    skip_comment();
    return result;
}

CppTomlResult TomlParser::parse_keyval()
{
    CPPTOML_ASSERT(isQuotedKey() || isUnquotedKey());
    CppTomlResult result;
    result = parse_key();
}
CPPTOML_NAMESPAFCE_END
