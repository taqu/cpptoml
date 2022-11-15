# cpptoml
A minimal reader of [Tom's Obvious, Minimal Language v1.0.0](https://toml.io/en/v1.0.0).
"Minimal" means that this doesn't have kindful supports like, friendness for STL, error messages for incorrects,
but aim for minimal memory footpoints, supports of yourown allocators.

# Usage

<details>
<summary>Example</summary>

```cpp
#include <sys/types.h>
#include <sys/stat.h>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>
#include "cpptoml.h"

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
    default:
        break;
    }
}

void traverse_object(cpptoml::TomlProxy proxy)
{
    using namespace cpptoml;
    printf("{");
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

int main(void)
{
    using namespace cpptoml;
    std::string path = "../../test00.toml";
    FILE* f = fopen(path.c_str(), "rb");
    if(NULL == f) {
        return 0;
    }
    struct stat s;
    fstat(fileno(f), &s);
    size_t size = s.st_size;
    char* data = (char*)::malloc(size);
    if(NULL == data || (0 < size && fread(data, size, 1, f) <= 0)) {
        fclose(f);
        ::free(data);
        return 0;
    }
    fclose(f);
    TomlParser parser;
    bool result = parser.parse(data, data + size);
    assert(result);
    TomlProxy proxy = parser.root();
    traverse(proxy);
    ::free(data);
    return 0;
}
```

</details>

# Limitations
- Uncheck the number of nests, so deep nests of tables or arrays will cause stackoverflow.
- Uncheck bad Unicode codepoints.

# Test
Use test cases [BurntSushi/toml-test](https://github.com/BurntSushi/toml-test).

# ToDo

- Pass the test of bad Unicode's codepoints
- Write comments and document

# License
This software is distributed under two licenses 'The MIT License' or 'Public Domain', choose whichever you like.

