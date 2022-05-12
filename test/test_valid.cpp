#include <Windows.h>

#include "../cpptoml.h"
#include "catch_wrap.hpp"

namespace
{
bool test(const CHAR* filepath)
{
    printf("%s\n", filepath);
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
    bool result = parser.parse(buffer, buffer+size);
    ::free(buffer);
    return result;
}
} // namespace

TEST_CASE("TestToml::Valid")
{
    WIN32_FIND_DATA findFileData;
    HANDLE file = FindFirstFileA("..\\toml-test\\tests\\valid\\*.toml", &findFileData);
    if(INVALID_HANDLE_VALUE == file) {
        return;
    }
    int32_t count = 0;
    static const int32_t skip = 38;
    do {
        ++count;
        if(count<=skip){
            continue;
        }
        std::string path = "..\\toml-test\\tests\\valid\\";
        path += findFileData.cFileName;
        EXPECT_TRUE(test(path.c_str()));
    } while(FindNextFileA(file, &findFileData));
    FindClose(file);
}

TEST_CASE("TestToml::InValid")
{
    WIN32_FIND_DATA findFileData;
    HANDLE file = FindFirstFileA("..\\toml-test\\tests\\invalid\\*.toml", &findFileData);
    if(INVALID_HANDLE_VALUE == file) {
        return;
    }
    int32_t count = 0;
    static const int32_t skip = 3;
    do {
        ++count;
        if(count<=skip){
            continue;
        }
        std::string path = "..\\toml-test\\tests\\invalid\\";
        path += findFileData.cFileName;
        EXPECT_FALSE(test(path.c_str()));
    } while(FindNextFileA(file, &findFileData));
    FindClose(file);
}

