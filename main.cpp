#include <cstdint>
#include <cstring>
#include <charconv>
#include <functional>
#include <chrono>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <nmmintrin.h>

namespace
{
    inline uint32_t rotr32(uint32_t x, uint32_t r)
    {
        return (x>>r) | (x<<((~r+1)&31U));
    }
}

uint32_t pcg32()
{
    static constexpr uint64_t Increment = 0x14057B7EF767814FULL;
    static constexpr uint64_t  Multiplier = 0x5851F42D4C957F2DULL;
    static uint64_t state = 123456U;
    uint64_t x = state;
    uint32_t count = static_cast<uint32_t>(x>>59);
    state = x*Multiplier + Increment;
    x ^= x>>18;
    uint32_t y = rotr32(static_cast<uint32_t>(x>>27), count);
    return y & 0xFFFFFFFEUL;
}

void test(std::function<uint32_t(void)> func, uint32_t count, const char* name)
{
    std::vector<uint32_t> values;
    values.resize(count);
    std::chrono::time_point start = std::chrono::high_resolution_clock::now();
    for(uint32_t i=0; i<count; ++i){
        values[i] = func();
    }
    std::chrono::duration duration = std::chrono::high_resolution_clock::now() - start;
    std::ostringstream oss;
    oss << name << ".bin";
    std::string filename = oss.str();
    std::ofstream file(filename.c_str(), std::ios::binary);
    file.write(reinterpret_cast<char*>(&values[0]), sizeof(uint32_t)*values.size());
    file.close();
    std::cout << name << ": " << std::chrono::duration_cast<std::chrono::microseconds>(duration).count() << " microseconds" << std::endl;
}

int main(int /*arc*/, char** argv)
{
    int32_t count = 0;
    std::string arg = argv[1];
    std::from_chars(arg.c_str(), arg.c_str()+arg.length(), count); 
    test(pcg32, static_cast<uint32_t>(count), "pcg32");
    return 0;
}

