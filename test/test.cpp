
#ifdef _WIN32
#    include <Windows.h>
#else
#    include <dirent.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#endif
#include "../cpptoml.h"
#include "catch_wrap.hpp"

namespace
{
class Directory
{
public:
    bool open(const char* path, const char* pattern);
    void close();
    bool next();

    std::string path() const;

private:
    std::string path_;
    std::string pattern_;
#ifdef _WIN32
    WIN32_FIND_DATA findFileData_;
    HANDLE file_ = INVALID_HANDLE_VALUE;
#else
    DIR* directory_ = nullptr;
    std::string name_;
#endif
};

bool Directory::open(const char* path, const char* pattern)
{
    path_ = path;
#ifdef _WIN32
    pattern_ = "*";
    pattern_ += pattern;
    std::string search_string = path_ + pattern_;
    file_ = FindFirstFileA(search_string.c_str(), &findFileData_);
    return INVALID_HANDLE_VALUE != file_;
#else
    pattern_ = pattern;
    directory_ = opendir(path_.c_str());
    return nullptr != directory_ ? next() : false;
#endif
}

void Directory::close()
{
#ifdef _WIN32
    if(INVALID_HANDLE_VALUE != file_) {
        FindClose(file_);
        file_ = INVALID_HANDLE_VALUE;
    }
#else
    if(nullptr != directory_) {
        closedir(directory_);
        directory_ = nullptr;
    }
#endif
}

bool Directory::next()
{
#ifdef _WIN32
    return FindNextFileA(file_, &findFileData_);
#else
    for(;;) {
        struct dirent* info = readdir(directory_);
        if(nullptr == info) {
            return false;
        }
        std::string name = info->d_name;
        bool found = std::equal(std::crbegin(pattern_), std::crend(pattern_), std::crbegin(name));
        if(found) {
            name_ = name;
            return true;
        }
    }
#endif
}

std::string Directory::path() const
{
#ifdef _WIN32
    return path_ + findFileData_.cFileName;
#else
    return path_ + name_;
#endif
}

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
#ifdef _WIN32
    printf("%lld", value.value_);
#else
    printf("%ld", value.value_);
#endif
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

bool test_toml(const char* filepath, bool print_result)
{
    printf("%s\n", filepath);
#ifdef _WIN32
    HANDLE file = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(nullptr == file) {
        return false;
    }
    DWORD size = 0;
    size = GetFileSize(file, NULL);
#else
    FILE* file = fopen(filepath, "rb");
    if(nullptr == file) {
        return false;
    }
    std::size_t size;
    {
        int fd = fileno(file);
        if(fd<0) {
            fclose(file);
            return false;
        }
        struct stat s;
        if(0 != fstat(fd, &s)) {
            fclose(file);
            return false;
        }
        size = s.st_size;
    }
#endif
    cpptoml::u8* buffer = reinterpret_cast<cpptoml::u8*>(::malloc(sizeof(cpptoml::u8) * size));

#ifdef _WIN32
    if(!ReadFile(file, buffer, size, &size, NULL)) {
        CloseHandle(file);
        return false;
    }
    CloseHandle(file);
#else

    if(fread(buffer, size, 1, file) < 1) {
        fclose(file);
        return false;
    }
    fclose(file);
#endif

    cpptoml::TomlParser parser;
    bool result = parser.parse(buffer, buffer + size);
    if(result) {
        if(print_result) {
            print(parser.top(), 0);
        }
    }
    ::free(buffer);
    return result;
}
} // namespace

TEST_CASE("TestToml::Valid")
{
    Directory directory;
#ifdef _WIN32
    if(!directory.open("..\\toml-test\\tests\\valid\\", ".toml")) {
        return;
    }
#else
    if(!directory.open("../toml-test/tests/valid/", ".toml")) {
        return;
    }
#endif
    int32_t count = 0;
    static const int32_t skip = 0;
    do {
        ++count;
        if(count < skip) {
            continue;
        }
        std::string path = directory.path();
        bool result = test_toml(path.c_str(), false);
        EXPECT_TRUE(result);
    } while(directory.next());
    directory.close();
}

TEST_CASE("TestToml::InValid")
{
    Directory directory;
#ifdef _WIN32
    if(!directory.open("..\\toml-test\\tests\\invalid\\", ".toml")) {
        return;
    }
#else
    if(!directory.open("../toml-test/tests/invalid/", ".toml")) {
        return;
    }
#endif

    int32_t count = 0;
    static const int32_t skip = 0;
    do {
        ++count;
        if(count <= skip) {
            continue;
        }
        std::string path = directory.path();
        bool result = test_toml(path.c_str(), false);
        EXPECT_FALSE(result);
    } while(directory.next());
    directory.close();
}

#if 0
TEST_CASE("TestToml::PrintValues")
{
    std::string path;
    path = "../toml-test/tests/valid/string-simple.toml";
    bool result;
    result = test_toml(path.c_str(), true);
    EXPECT_TRUE(result);
    path = "../toml-test/tests/valid/string-with-pound.toml";
    result = test_toml(path.c_str(), true);
    EXPECT_TRUE(result);
}
#endif
