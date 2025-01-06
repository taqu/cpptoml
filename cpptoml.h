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
#include <cstddef>
#include <tuple>

namespace cpptoml
{

#ifndef CPPTOML_TYPES
#    define CPPTOML_TYPES
#    ifndef CPPTOML_NULL
#        define CPPTOML_NULL nullptr
#    endif // CPPTOML_NULL
#endif // CPPTOML_TYPES

#ifdef _DEBUG
#    define CPPTOML_DEBUG
#endif

typedef void* (*CPPTOML_MALLOC_TYPE)(size_t);
typedef void (*CPPTOML_FREE_TYPE)(void*);

class TomlParser;

/**
 * @brief Toml Type
 */
enum class TomlType : uint32_t
{
    Table =0,
    Array,
    String,
    Integer,
    Hex,
    Oct,
    Bin,
    Float,
    Inf,
    NaN,
    DateTime,
    True,
    False,
    Key,
    KeyValue,
    Invalid,
};

/**
 * @brief value type
 */
struct TomlValue
{
    uint64_t start_; //!< the start position of element
    uint64_t size_;  //!< the size of element
    uint32_t next_;  //!< the next element of aggregations
    uint32_t type_;  //!< the type of element
};

/**
 * @brief Toml proxy
 */
struct TomlProxy
{
    /**
     * @brief validate this element
     * @return true if this is valid
     */
    operator bool() const;

    /**
     * @return type of this
     */
    TomlType type() const;

    /**
     * Indicates the size of element. In a case of the string type, it shows the length of the string. For excample "test", the size equals to 4, excepts characters `"`.
     * @return size of this
     */
    uint64_t size() const;

    /**
     * In typical usage, you can iterate children,
     *
     * ```cpp
     * for(TomlProxy i=aggregation.begin(); i; i=i.next())
     * ```
     * @return the first element of aggregations, like the object or array
     */
    TomlProxy begin() const;

    /**
     * @return the next element of aggregations
     */
    TomlProxy next() const;

    /**
     * @return key of an object's entry
     */
    TomlProxy key() const;
    /**
     * @return value of an object's entry
     */
    TomlProxy value() const;

    /**
     * @brief Get the table name 
     * @param [in] len ... buffer size of str with including null termination
     * @param [out] str ... the result
     * @return size of the result
     */
    uint64_t getTableName(uint32_t len, char* str) const;

    /**
     * @brief Get the value as string
     * @param [out] str ... the result
     * @return size of the result
     */
    uint64_t getString(char* str) const;

    /**
     * @brief Get the value as integer
     * @return the value as integer
     */
    int64_t getInt64() const;
    
    /**
     * @brief Get the value as float
     * @return the value as float
     */
    double getFloat64() const;

    uint64_t value_;
    const char* data_;
    const TomlValue* values_;
};

/**
 * @brief Toml Parser
 */
class TomlParser
{
public:
    static constexpr uint32_t Invalid = static_cast<uint32_t>(-1);
    static constexpr std::tuple<const char*, uint32_t> InvalidPair = {CPPTOML_NULL, Invalid};
    static constexpr std::tuple<const char*, uint32_t, uint32_t> InvalidTuple = {CPPTOML_NULL, Invalid, Invalid};
    static constexpr uint32_t Expand = 128;
    static constexpr int32_t MaxNesting = 128;

    /**
     * @param [in] allocator ... custom allocator
     * @param [in] deallocator ... custom deallocator
     */
    TomlParser(CPPTOML_MALLOC_TYPE allocator = CPPTOML_NULL, CPPTOML_FREE_TYPE deallocator = CPPTOML_NULL);
    
    ~TomlParser();

    /**
     * @return true if succeeded
     * @param [in] begin ...
     * @param [in] end ...
     */
    bool parse(const char* begin, const char* end);

    /**
     * @return root object of the document
     */
    TomlProxy root() const;
private:
    TomlParser(const TomlParser&) = delete;
    TomlParser& operator=(const TomlParser&) = delete;

    enum class KeyPlace
    {
        KeyValue,
        Table,
        ArrayTable,
    };

    int64_t next_symbol(const char*& str) const;
    bool parse_unquated_key_char(const char*& str) const;
    bool basic_char(const char*& str) const;
    bool literal_char(const char*& str) const;
    bool escaped(const char*& str) const;
    bool parse_2hexdig(const char*& str) const;
    bool parse_4hexdig(const char*& str) const;
    bool parse_8hexdig(const char*& str) const;
    static bool hexgidit(char c);
    static bool digit(char c);

    const char* bom(const char* str) const;
    const char* newline(const char* str) const;
    const char* whitespace(const char* str) const;
    const char* comment(const char* str) const;
    const char* ws_comment_newline(const char* str) const;

    const char* parse_non_ascii(const char* str) const;

    const char* parse_expression(const char* str);
    std::tuple<const char*, uint32_t> parse_keyvalue(const char* str);
    bool keyvalue(const char* str) const;
    std::tuple<const char*, uint32_t, uint32_t> parse_key(const char* str, uint32_t current, KeyPlace place);
    const char* parse_unquated_key(const char* str) const;

    std::tuple<const char*, uint32_t> parse_value(const char* str);
    bool value(const char* str) const;

    const char* parse_basic_string(const char* str);
    const char* parse_literal_string(const char* str);

    const char* parse_ml_basic_string(const char* str);
    const char* parse_mlb_content(const char* str);
    const char* parse_mlb_escaped_nl(const char* str);
    bool parse_mlb_quotes(const char*& str);

    const char* parse_ml_literal_string(const char* str);
    const char* parse_mll_content(const char* str);
    bool parse_mll_quotes(const char*& str);

    std::tuple<const char*, uint32_t> parse_table(const char* str);
    std::tuple<const char*, uint32_t> parse_std_table(const char* str);
    std::tuple<const char*, uint32_t> parse_array_table(const char* str);

    const char* parse_true(const char* str);
    const char* parse_false(const char* str);

    std::tuple<const char*, uint32_t> parse_array(const char* str);
    std::tuple<const char*, uint32_t> parse_inline_table(const char* str);

    std::tuple<const char*, uint32_t> parse_number(const char* str);
    TomlType number_type(const char* str);
    std::tuple<const char*, uint32_t> parse_integer(const char* str);
    std::tuple<const char*, uint32_t> parse_hex(const char* str);
    std::tuple<const char*, uint32_t> parse_oct(const char* str);
    std::tuple<const char*, uint32_t> parse_bin(const char* str);
    std::tuple<const char*, uint32_t> parse_float(const char* str);
    const char* parse_frac(const char* str);
    const char* parse_exp(const char* str);
    const char* parse_zero_prefixable_int(const char* str);
    std::tuple<const char*, uint32_t> parse_inf(const char* str);
    std::tuple<const char*, uint32_t> parse_nan(const char* str);
    std::tuple<const char*, uint32_t> parse_datetime(const char* str);
    const char* parse_fulldate(const char* str);
    const char* parse_partial_time(const char* str);
    const char* parse_timeoffset(const char* str);

    static bool strcmp(const char* s0, const char* e0, const char* s1, const char* e1);
    uint32_t find_keyvalue(uint32_t table, const char* begin, const char* end) const;
    uint32_t find_table(uint32_t array) const;
    uint32_t find_table_node(uint32_t node) const;
    bool has_child_table(uint32_t table) const;

    CPPTOML_MALLOC_TYPE allocator_;
    CPPTOML_FREE_TYPE deallocator_;
    const char* begin_;
    const char* end_;

    void clear();
    uint32_t add();
    uint32_t add_keyvalue(const char* str, const char* end);
    uint32_t add_value(TomlType type, const char* str, const char* end);
    uint32_t add_table();
    uint32_t add_array();
    void append(uint32_t parent, uint32_t value);

    uint32_t current_; //!< current table
    uint32_t capacity_; //!< capacity of buffer
    uint32_t size_;     //!< current size of buffer
    TomlValue* values_; //!< elements of Json
};

} // namespace cpptoml
#endif // INC_CPPTOML_H_
