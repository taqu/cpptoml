#include <Windows.h>

#include "../cpptoml.h"
#include "catch_wrap.hpp"

namespace
{
void print(const cpptoml::TomlTableProxy& table, cpptoml::u32 indent);

void put_indent(cpptoml::u32 indent)
{
    indent *= 4;
    for(cpptoml::u32 i = 0; i < indent; ++i) {
        putchar(' ');
    }
}

void puts(cpptoml::u32 indent, cpptoml::u32 length, const char* str)
{
    put_indent(indent);
    for(cpptoml::u32 i = 0; i < length; ++i) {
        putchar(str[i]);
    }
}

void print(cpptoml::u32 indent, const cpptoml::TomlStringProxy& value)
{
    puts(indent, value.length_, value.str_);
}

void print(cpptoml::u32 indent, const cpptoml::TomlDateTimeProxy& value)
{
    put_indent(indent);
    printf("%d-%d-%d", value.year_, value.month_, value.day_);
}

void print(cpptoml::u32 indent, const cpptoml::TomlFloatProxy& value)
{
    put_indent(indent);
    printf("%f", value.value_);
}

void print(cpptoml::u32 indent, const cpptoml::TomlIntProxy& value)
{
    put_indent(indent);
    printf("%lld", value.value_);
}

void print(cpptoml::u32 indent, const cpptoml::TomlBoolProxy& value)
{
    put_indent(indent);
    printf("%s", value.value_ ? "true" : "false");
}

void print(const cpptoml::TomlArrayProxy& array, cpptoml::u32 indent)
{
    ::printf("[\n");
    for(cpptoml::u32 itr = array.begin(); itr != array.end(); itr = array.next(itr)) {
        cpptoml::TomlValueProxy value = array[itr];
        switch(value.type()) {
        case cpptoml::TomlType::Table:
            print(value.value<cpptoml::TomlTableProxy>(), indent + 1);
            break;
        case cpptoml::TomlType::Array:
        case cpptoml::TomlType::ArrayTable:
            print(value.value<cpptoml::TomlArrayProxy>(), indent + 1);
            break;
        case cpptoml::TomlType::String:
            print(indent + 1, value.value<cpptoml::TomlStringProxy>());
            ::printf(",\n");
            break;
        case cpptoml::TomlType::DateTime:
            print(indent + 1, value.value<cpptoml::TomlDateTimeProxy>());
            ::printf(",\n");
            break;
        case cpptoml::TomlType::Float:
            print(indent + 1, value.value<cpptoml::TomlFloatProxy>());
            ::printf(",\n");
            break;
        case cpptoml::TomlType::Integer:
            print(indent + 1, value.value<cpptoml::TomlIntProxy>());
            ::printf(",\n");
            break;
        case cpptoml::TomlType::Boolean:
            print(indent + 1, value.value<cpptoml::TomlBoolProxy>());
            ::printf(",\n");
            break;
        default:
            assert(false);
            break;
        }
    }
    put_indent(indent);
    ::printf("]\n");
}

void print(const cpptoml::TomlTableProxy& table, cpptoml::u32 indent)
{
    ::printf("{\n");
    for(cpptoml::u32 itr = table.begin(); itr != table.end(); itr = table.next(itr)) {
        cpptoml::TomlKeyValueProxy keyvalue = table[itr];
        cpptoml::TomlStringProxy key = keyvalue.key();
        cpptoml::TomlValueProxy value = keyvalue.value();
        puts(indent, key.length_, key.str_);
        ::printf(" = ");
        switch(value.type()) {
        case cpptoml::TomlType::Table:
            print(value.value<cpptoml::TomlTableProxy>(), indent + 1);
            break;
        case cpptoml::TomlType::Array:
        case cpptoml::TomlType::ArrayTable:
            print(value.value<cpptoml::TomlArrayProxy>(), indent + 1);
            break;
        case cpptoml::TomlType::String:
            print(0, value.value<cpptoml::TomlStringProxy>());
            break;
        case cpptoml::TomlType::DateTime:
            print(0, value.value<cpptoml::TomlDateTimeProxy>());
            break;
        case cpptoml::TomlType::Float:
            print(0, value.value<cpptoml::TomlFloatProxy>());
            break;
        case cpptoml::TomlType::Integer:
            print(0, value.value<cpptoml::TomlIntProxy>());
            break;
        case cpptoml::TomlType::Boolean:
            print(0, value.value<cpptoml::TomlBoolProxy>());
            break;
        default:
            assert(false);
            break;
        }
        putchar('\n');
    }
    put_indent(indent);
    ::printf("}\n");
}

bool test(const CHAR* filepath, bool print_result)
{
    printf("%s\n", filepath);
#if defined(__unix__) || defined(__linux__)
#else
    HANDLE file = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(NULL == file) {
        return false;
    }
    DWORD size = 0;
    size = GetFileSize(file, NULL);
    cpptoml::u8* buffer = reinterpret_cast<cpptoml::u8*>(::malloc(sizeof(cpptoml::u8) * size));
    if(!ReadFile(file, buffer, size, &size, NULL)) {
        CloseHandle(file);
        return false;
    }
    CloseHandle(file);

    cpptoml::TomlParser parser;
    bool result = parser.parse(buffer, buffer + size);
    if(result) {
        if(print_result) {
            print(parser.top(), 0);
        }
    }
    ::free(buffer);
    return result;
#endif
}
} // namespace

TEST_CASE("TestToml::Valid")
{
#if defined(__unix__) || defined(__linux__)
#else
    WIN32_FIND_DATA findFileData;
    HANDLE file = FindFirstFileA("..\\toml-test\\tests\\valid\\*.toml", &findFileData);
    if(INVALID_HANDLE_VALUE == file) {
        return;
    }
    int32_t count = 0;
    static const int32_t skip = 10;
    do {
        ++count;
        if(count <= skip) {
            continue;
        }
        std::string path = "..\\toml-test\\tests\\valid\\";
        path += findFileData.cFileName;
        EXPECT_TRUE(test(path.c_str(), false));
    } while(FindNextFileA(file, &findFileData));
    FindClose(file);
#endif
}

TEST_CASE("TestToml::InValid")
{
    WIN32_FIND_DATA findFileData;
    HANDLE file = FindFirstFileA("..\\toml-test\\tests\\invalid\\*.toml", &findFileData);
    if(INVALID_HANDLE_VALUE == file) {
        return;
    }
    int32_t count = 0;
    static const int32_t skip = 0;
    do {
        ++count;
        if(count <= skip) {
            continue;
        }
        std::string path = "..\\toml-test\\tests\\invalid\\";
        path += findFileData.cFileName;
        EXPECT_FALSE(test(path.c_str(), false));
    } while(FindNextFileA(file, &findFileData));
    FindClose(file);
}

TEST_CASE("TestToml::PrintValues")
{
    std::string path;
    path = "..\\toml-test\\tests\\valid\\string-simple.toml";
    EXPECT_TRUE(test(path.c_str(), true));
    path = "..\\toml-test\\tests\\valid\\string-with-pound.toml";
    EXPECT_TRUE(test(path.c_str(), true));
}
