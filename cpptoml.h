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

#ifdef __cplusplus
#    define CPPTOML_NAMESPAFCE_BEGIN \
        namespace cpptoml \
        {
#    define CPPTOML_NAMESPAFCE_END }
#else
#    define CPPTOML_NAMESPAFCE_BEGIN
#    define CPPTOML_NAMESPAFCE_END
#endif

CPPTOML_NAMESPAFCE_BEGIN

#ifndef CPPTOML_TYPES
#    define CPPTOML_TYPES
#    ifdef __cplusplus
#define CPPTOML_CPP
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

static constexpr u32 CppTomlDefaultMaxNest = 512;
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

typedef void* (*cpptoml_malloc)(size_t);
typedef void (*cpptoml_free)(void*);

enum CppTomlType
{
    CppTomlType_None = 0,
    CppTomlType_Object,
    CppTomlType_Array,
    CppTomlType_String,
    CppTomlType_Number,
    CppTomlType_Integer,
    CppTomlType_Boolean,
    CppTomlType_Null,
    CppTomlType_KeyValue,
};

struct CppTomlResult
{
    bool success_;
    u32 index_;
};

struct CppTomlObject
{
    u32 size_;
    u32 head_;
};

struct CppTomlArray
{
    u32 size_;
    u32 head_;
};

struct CppTomlKeyValue
{
    u32 key_;
    u32 value_;
};

struct CppTomlValue
{
    u32 position_;
    u32 length_;
    u32 next_;
    CppTomlType type_;
    union
    {
        CppTomlObject object_;
        CppTomlArray array_;
        CppTomlKeyValue key_value_;
    };
};

//--- TomlParser
//---------------------------------------
class TomlParser
{
public:
    using cursor = const UChar*;

    TomlParser(cpptoml_malloc allocator = CPPTOML_NULL, cpptoml_free deallocator = CPPTOML_NULL, u32 max_nests = CppTomlDefaultMaxNest);
    ~TomlParser();

    bool parse(cursor head, cursor end);
    u32 size() const;
private:
    TomlParser(const TomlParser&) = delete;
    TomlParser& operator=(const TomlParser&) = delete;

    struct Buffer
    {
    public:
        static constexpr u32 ExpandSize = 4*4096;

        Buffer(cpptoml_malloc allocator = CPPTOML_NULL, cpptoml_free deallocator = CPPTOML_NULL);
        ~Buffer();

        u32 capacity() const;
        u32 size() const;
        void clear();
    private:
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        cpptoml_malloc allocator_;
        cpptoml_free deallocator_;
        u32 capacity_;
        u32 size_;
    };

    static bool isAlpha(Char c);
    static bool isDigit(Char c);
    static bool isHexDigit(Char c);

    bool isEnd() const;
    bool isQuotedKey() const;
    bool isUnquotedKey() const;
    bool isTable() const;

    void skip_bom();
    void skip_space();
    void skip_newline();
    void skip_spacenewline();
    bool skip_utf8_1();
    bool skip_utf8_2();
    bool skip_utf8_3();
    bool skip_noneol();
    void skip_comment();

    CppTomlResult parse_expression();
    CppTomlResult parse_keyval();
    CppTomlResult parse_table();

    cpptoml_malloc allocator_;
    cpptoml_free deallocator_;
    u32 max_nests_;
    u32 nest_count_;
    cursor begin_ = CPPTOML_NULL;
    cursor current_ = CPPTOML_NULL;
    cursor end_ = CPPTOML_NULL;
    Buffer buffer_;
};

CPPTOML_NAMESPAFCE_END
#endif // INC_CPPTOML_H_
