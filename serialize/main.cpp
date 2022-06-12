#include "../cpptoml.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>

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

//--- Serializer
//------------------------
class Serializer
{
public:
    Serializer();
    ~Serializer();

private:
    template<class T>
    friend void serialize(Serializer& s, const char* name, const T& x);

    Serializer(const Serializer&) = delete;
    Serializer& operator=(const Serializer&) = delete;
};

Serializer::Serializer()
{
}

Serializer::~Serializer()
{
}

template<class T>
void serialize(Serializer& s, const char* name, const T& x)
{
    assert(nullptr != name);
}

template<>
void serialize<s32>(Serializer& s, const char* name, const s32& x)
{
    assert(nullptr != name);
}

//--- Deserializer
//------------------------
class Deserializer
{
public:
    explicit Deserializer(cpptoml::TomlParser& toml);
    ~Deserializer();

private:
    template<class T>
    friend void deserialize(Deserializer& s, const char* name, T& x);

    Deserializer(const Deserializer&) = delete;
    Deserializer& operator=(const Deserializer&) = delete;

    explicit Deserializer(cpptoml::TomlTableProxy& table);

    cpptoml::TomlTableProxy table_;
};

Deserializer::Deserializer(cpptoml::TomlParser& toml)
    : table_(toml.top())
{
}

Deserializer::Deserializer(cpptoml::TomlTableProxy& table)
    : table_(table)
{
}

Deserializer::~Deserializer()
{
}

template<class T>
void deserialize(Deserializer& s, T& x);

template<class T>
void deserialize(Deserializer& s, const char* name, T& x)
{
    cpptoml::TomlTableProxy table;
    if(s.table_.tryGet(table, name)) {
        Deserializer child(table);
        deserialize(child, x);
    }
}

template<>
void deserialize<s8>(Deserializer& s, const char* name, s8& x)
{
    s.table_.tryGet(x, name);
}

template<>
void deserialize<s16>(Deserializer& s, const char* name, s16& x)
{
    s.table_.tryGet(x, name);
}

template<>
void deserialize<s32>(Deserializer& s, const char* name, s32& x)
{
    s.table_.tryGet(x, name);
}

template<>
void deserialize<s64>(Deserializer& s, const char* name, s64& x)
{
    s.table_.tryGet(x, name);
}

template<>
void deserialize<u8>(Deserializer& s, const char* name, u8& x)
{
    s.table_.tryGet(x, name);
}

template<>
void deserialize<u16>(Deserializer& s, const char* name, u16& x)
{
    s.table_.tryGet(x, name);
}

template<>
void deserialize<u32>(Deserializer& s, const char* name, u32& x)
{
    s.table_.tryGet(x, name);
}

template<>
void deserialize<u64>(Deserializer& s, const char* name, u64& x)
{
    s.table_.tryGet(x, name);
}

template<>
void deserialize<f32>(Deserializer& s, const char* name, f32& x)
{
    s.table_.tryGet(x, name);
}

template<>
void deserialize<f64>(Deserializer& s, const char* name, f64& x)
{
    s.table_.tryGet(x, name);
}

bool read(std::string& content, const char* path)
{
    std::ifstream file(path);
    if(!file.is_open()) {
        return false;
    }
    content = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

struct SubConfig
{
    s32 i0_;
    s32 i1_;
    f32 f0_;
    f32 f1_;
};

struct Config
{
    s32 i0_;
    s32 i1_;
    f32 f0_;
    f32 f1_;
    SubConfig sub_;
};

template<>
void deserialize<SubConfig>(Deserializer& s, SubConfig& config)
{
    deserialize(s, "i0", config.i0_);
    deserialize(s, "i1", config.i1_);
    deserialize(s, "f0", config.f0_);
    deserialize(s, "f1", config.f1_);
}

template<>
void deserialize<Config>(Deserializer& s, Config& config)
{
    deserialize(s, "i0", config.i0_);
    deserialize(s, "i1", config.i1_);
    deserialize(s, "f0", config.f0_);
    deserialize(s, "f1", config.f1_);
    deserialize(s, "sub", config.sub_);
}

int main(void)
{
    std::string content;
    read(content, "config.toml");
    cpptoml::TomlParser parser;
    if(!parser.parse(content.c_str(), content.c_str() + content.length())) {
        return 0;
    }
    Deserializer deserializer(parser);
    Config config;
    deserialize(deserializer, config);
    return 0;
}