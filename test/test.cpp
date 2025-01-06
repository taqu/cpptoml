
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
void traverse_object(cpptoml::TomlProxy proxy);
void traverse_array(cpptoml::TomlProxy proxy);
void traverse_keyvalue(cpptoml::TomlProxy proxy);
void traverse_string(cpptoml::TomlProxy proxy);
void traverse_number(cpptoml::TomlProxy proxy);
void traverse_integer(cpptoml::TomlProxy proxy);
void traverse_true(cpptoml::TomlProxy proxy);
void traverse_false(cpptoml::TomlProxy proxy);

void traverse(cpptoml::TomlProxy proxy)
{
    using namespace cpptoml;
    switch(proxy.type()) {
    case TomlType::Table:
        traverse_object(proxy);
        break;
    case TomlType::Array:
        traverse_array(proxy);
        break;
    case TomlType::KeyValue:
        traverse_keyvalue(proxy);
        break;
    case TomlType::String:
        traverse_string(proxy);
        break;
    case TomlType::Float:
        traverse_number(proxy);
        break;
    case TomlType::Integer:
        traverse_integer(proxy);
        break;
    case TomlType::True:
        traverse_true(proxy);
        break;
    case TomlType::False:
        traverse_false(proxy);
        break;
        break;
    default:
        break;
    }
}

void traverse_object(cpptoml::TomlProxy proxy)
{
    using namespace cpptoml;
    char name[16];
    proxy.getTableName(16, name);
    printf("%s{", name);
    for(TomlProxy i = proxy.begin(); i; i = i.next()) {
        char key[128];
        i.key().getString(key);
        printf("%s: ", key);
        traverse(i.value());
        printf(", ");
    }
    printf("}");
}

void traverse_array(cpptoml::TomlProxy proxy)
{
    using namespace cpptoml;
    printf("[");
    for(TomlProxy i = proxy.begin(); i; i = i.next()) {
        traverse(i);
        printf(", ");
    }
    printf("]");
}

void traverse_keyvalue(cpptoml::TomlProxy proxy)
{
    char key[128];
    proxy.key().getString(key);
    printf("%s: ", key);
    traverse(proxy.value());
}

void traverse_string(cpptoml::TomlProxy proxy)
{
    // correct way to acuire a string
    uint64_t length = proxy.size();
    char* value = reinterpret_cast<char*>(::malloc(length + 1));
    if(NULL == value) {
        return;
    }
    proxy.getString(value);
    printf("%s", value);
    ::free(value);
}

void traverse_number(cpptoml::TomlProxy proxy)
{
    double value = proxy.getFloat64();
    printf("%lf", value);
}

void traverse_integer(cpptoml::TomlProxy proxy)
{
    int64_t value = proxy.getInt64();
#ifdef _MSC_VER
    printf("%lld", value);
#else
    printf("%ld", value);
#endif
}

void traverse_true(cpptoml::TomlProxy)
{
    printf("true");
}

void traverse_false(cpptoml::TomlProxy)
{
    printf("false");
}

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

bool test_valid(uint32_t no, const char* filepath, bool )
{
    printf("[%d] %s\n", no, filepath);
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
    char* buffer = reinterpret_cast<char*>(::malloc(sizeof(char) * size));

#ifdef _WIN32
    if(!ReadFile(file, buffer, size, &size, NULL)) {
        CloseHandle(file);
        return false;
    }
    CloseHandle(file);
#else
    if(0<size && fread(buffer, size, 1, file) < 1) {
        fclose(file);
        return false;
    }
    fclose(file);
#endif

    cpptoml::TomlParser parser;
    bool result = parser.parse(buffer, buffer + size);
    ::free(buffer);
    return result;
}

bool test_invalid(uint32_t no, const char* filepath)
{
    printf("[%d] %s\n", no, filepath);
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
    char* buffer = reinterpret_cast<char*>(::malloc(sizeof(char) * size));

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
    ::free(buffer);
    return false == result;
}

} // namespace

#if 1
TEST_CASE("TestToml::Valid")
{
    Directory directory;
#ifdef _WIN32
    if(!directory.open("..\\..\\toml-test\\tests\\valid\\", ".toml")) {
        return;
    }
#else
    if(!directory.open("../../toml-test/tests/valid/", ".toml")) {
        return;
    }
#endif
    uint32_t count = 0;
    static const uint32_t skip = 0;
    do {
        if(count < skip) {
            ++count;
            continue;
        }
        std::string path = directory.path();
        bool result = test_valid(count, path.c_str(), false);
        EXPECT_TRUE(result);
        ++count;
    } while(directory.next());
    directory.close();
}
#endif

#if 1
TEST_CASE("TestToml::InValid")
{
    Directory directory;
#ifdef _WIN32
    if(!directory.open("..\\..\\toml-test\\tests\\invalid\\", ".toml")) {
        return;
    }
#else
    if(!directory.open("../../toml-test/tests/invalid/", ".toml")) {
        return;
    }
#endif

    int32_t count = 0;
    static const int32_t skip = 0;
    do {
        if(count < skip) {
            ++count;
            continue;
        }
        std::string path = directory.path();
        bool result = test_invalid(count, path.c_str());
        EXPECT_TRUE(result);
        ++count;
    } while(directory.next());
    directory.close();
}
#endif

TEST_CASE("TestToml::PrintValues")
{
    std::string path = "../../test00.toml";
    FILE* f = fopen(path.c_str(), "rb");
    if(NULL == f) {
        return;
    }
    struct stat s;
    fstat(fileno(f), &s);
    size_t size = s.st_size;
    char* data = (char*)::malloc(size);
    if(NULL == data || (0 < size && fread(data, size, 1, f) <= 0)) {
        fclose(f);
        ::free(data);
        return;
    }
    fclose(f);
    cpptoml::TomlParser parser;
    bool result = parser.parse(data, data + size);
    EXPECT_TRUE(result);
    cpptoml::TomlProxy proxy = parser.root();
    traverse(proxy);
    ::free(data);
}

