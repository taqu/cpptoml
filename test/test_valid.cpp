#include <Windows.h>

#include "../cpptoml.h"
#include "catch_wrap.hpp"

TEST_CASE("TestToml::Valid")
{
    WIN32_FIND_DATA findFileData;
    HANDLE file = FindFirstFileA("..\\toml-test\\tests\\valid\\*.toml", &findFileData);
    if(INVALID_HANDLE_VALUE == file){
        return;
    }
    do{
        OutputDebugStringA(findFileData.cFileName);
        OutputDebugStringA("\n");
    }while(FindNextFileA(file, & findFileData));
    FindClose(file);
}

