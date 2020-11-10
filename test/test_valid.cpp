#include <fstream>

#include "../cpptoml.h"
#include "catch_wrap.hpp"

namespace cpptoml
{
namespace
{
    const char* test_root = "../toml-test/tests/valid/";

    struct Data
    {
        Data()
            : size_(0)
            , data_(nullptr)
        {
        }
        Data(cpptoml::u32 size, char* data)
            : size_(size)
            , data_(data)
        {
        }

        Data(Data&& other)
            : size_(other.size_)
            , data_(other.data_)
        {
            other.size_ = 0;
            other.data_ = nullptr;
        }

        ~Data()
        {
            free(data_);
        }

        bool empty() const
        {
            return size_ <= 0 || nullptr == data_;
        }

        Data& operator=(Data&& other)
        {
            if(this == &other) {
                return *this;
            }
            size_ = other.size_;
            data_ = other.data_;
            other.size_ = 0;
            other.data_ = nullptr;
            return *this;
        }
        cpptoml::u32 size_;
        char* data_;
    };
    Data readfile(const char* root, const char* filename)
    {
        char path[128];
        sprintf_s(path, "%s%s", root, filename);

        std::ifstream file(path, std::ios::binary);
        if(!file.is_open()) {
            return Data();
        }
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        size -= file.tellg();
        char* buffer = reinterpret_cast<char*>(malloc(size));
        file.read(buffer, size);
        file.close();
        Data data = {static_cast<cpptoml::u32>(size), buffer};
        return std::move(data);
    }

    const char* test_simple_files[] = {
        "simple.toml",
    };

    const char* test_array_files[] = {
        //"array-empty.toml",
        "array-nospaces.toml",
        //"arrays.toml",
        //"arrays-hetergeneous.toml",
        //"arrays-nested.toml",
        //"array-string-quote-comma.toml",
        //"array-string-quote-comma-2.toml",
        //"array-string-with-comma.toml",
        //"array-table-array-string-backslash.toml",
    };
} // namespace

TEST_CASE("TestToml::TestSimple")
{
    int size = sizeof(test_simple_files) / sizeof(test_simple_files[0]);
    for(int i = 0; i < size; ++i) {
        Data data = std::move(readfile("../", test_simple_files[i]));
        TomlParser parser;
        bool result = parser.parse(data.size_, data.data_);
        if(result) {
            CHECK(true);

            ValueAccessor valueAccessor;
            s32 int32;
            char buffer[32];

            // top
            //------------------------------------------------------
            TableAccessor topTableAccessor = parser.getTop();

            valueAccessor = topTableAccessor.findValue("int0");
            CHECK(valueAccessor.valid());
            CHECK(valueAccessor.type() == Type::Int64);
            if(valueAccessor.tryGet(int32)){
                std::cout << "int0=" << int32 << std::endl;
            }

            valueAccessor = topTableAccessor.findValue("int1");
            CHECK(valueAccessor.valid());
            CHECK(valueAccessor.type() == Type::Int64);
            if(valueAccessor.tryGet(int32)){
                std::cout << "int1=" << int32 << std::endl;
            }

            valueAccessor = topTableAccessor.findValue("str0");
            CHECK(valueAccessor.valid());
            CHECK(valueAccessor.type() == Type::BasicString);
            if(valueAccessor.tryGet(31, buffer)){
                std::cout << "str0=" << buffer << std::endl;
            }

            valueAccessor = topTableAccessor.findValue("str1");
            CHECK(valueAccessor.valid());
            CHECK(valueAccessor.type() == Type::LiteralString);
            if(valueAccessor.tryGet(31, buffer)){
                std::cout << "str1=" << buffer << std::endl;
            }

            ArrayAccessor arrayAccessor;
            ArrayAccessor::Iterator arrayIterator;
            arrayAccessor = topTableAccessor.findArray("ints");
            arrayIterator = arrayAccessor.begin();
            std::cout << "ints=";
            while(arrayIterator.next()){
                if(arrayIterator.get().tryGet(int32)){
                    std::cout << int32 << ", ";
                }
            }
            std::cout << std::endl;

            // table
            //------------------------------------------------------
            TableAccessor childTableAccessor = topTableAccessor.findTable("table");
            if(childTableAccessor.findValue("key").tryGet(31, buffer)){
                std::cout << "key=" << buffer << std::endl;
            }
            if(childTableAccessor.findValue("bare_key").tryGet(31, buffer)){
                std::cout << "bare_key=" << buffer << std::endl;
            }
            if(childTableAccessor.findValue("bare-key").tryGet(31, buffer)){
                std::cout << "bare-key=" << buffer << std::endl;
            }
            if(childTableAccessor.findValue("127.0.0.1").tryGet(31, buffer)){
                std::cout << "127.0.0.1=" << buffer << std::endl;
            }
            if(childTableAccessor.findValue("character encoding").tryGet(31, buffer)){
                std::cout << "character encoding=" << buffer << std::endl;
            }

            // inline-table
            //------------------------------------------------------
            cpptoml::TableAccessor inlineTableAccessor = childTableAccessor.findTable("name");

            if(inlineTableAccessor.findValue("first").tryGet(31, buffer)){
                std::cout << "first=" << buffer << std::endl;
            }

            if(inlineTableAccessor.findValue("last").tryGet(31, buffer)){
                std::cout << "last=" << buffer << std::endl;
            }

            // hierarchical-table
            //------------------------------------------------------
            cpptoml::TableAccessor hierarchicalTableAccessor = topTableAccessor.findTable("a").findTable("b");
            inlineTableAccessor = hierarchicalTableAccessor.findTable("name");
            if(inlineTableAccessor.findValue("first").tryGet(31, buffer)){
                std::cout << "first=" << buffer << std::endl;
            }

            if(inlineTableAccessor.findValue("last").tryGet(31, buffer)){
                std::cout << "last=" << buffer << std::endl;
            }
        } else {
            printf("[%d] false\n", i);
            CHECK(false);
        }
    }
}

//TEST_CASE("TestToml::TestArray")
//{
//    int size = sizeof(test_array_files) / sizeof(test_array_files[0]);
//    for(int i = 0; i < size; ++i) {
//        Data data = std::move(readfile(test_array_files[i]));
//        cpptoml::TomlParser parser;
//        bool result = parser.parse(data.size_, data.data_);
//        if(result) {
//            CHECK(true);
//        } else {
//            printf("[%d] false\n", i);
//            CHECK(false);
//        }
//    }
//}
} // namespace cpptoml
