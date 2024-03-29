cmake_minimum_required(VERSION 3.2)

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")

set(ProjectName cpptoml)
project(${ProjectName})

########################################################################
# Sources
set(HEADERS "../cpptoml.h;catch_amalgamated.hpp")
set(SOURCES "../cpptoml.cpp;main.cpp;test.cpp;catch_amalgamated.cpp")

source_group("include" FILES ${HEADERS})
source_group("src" FILES ${SOURCES})

set(FILES ${HEADERS} ${SOURCES})

set(OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

add_executable(${ProjectName} ${FILES})

if(MSVC)
    set(DEFAULT_CXX_FLAGS "/DWIN32 /D_WINDOWS /D_MSBC /W4 /WX- /nologo /fp:precise /arch:AVX2 /Zc:wchar_t /TP /Gd /std:c++17")
    if("1800" VERSION_LESS MSVC_VERSION)
        set(DEFAULT_CXX_FLAGS "${DEFAULT_CXX_FLAGS} /EHsc")
    endif()

    set(CMAKE_CXX_FLAGS "${DEFAULT_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MDd /Zi /Ob0 /Od /RTC1 /Gy /GR- /GS /Gm-")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /GL /GR- /DNDEBUG")

elseif(UNIX)
    set(DEFAULT_CXX_FLAGS "-Wall -O0 -g -std=c++17 -std=gnu++17 -march=native")
    set(CMAKE_CXX_FLAGS "${DEFAULT_CXX_FLAGS}")
elseif(APPLE)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${ProjectName})
set_target_properties(${ProjectName}
    PROPERTIES
        OUTPUT_NAME_DEBUG "${ProjectName}" OUTPUT_NAME_RELEASE "${ProjectName}"
        VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_DIRECTORY}")

