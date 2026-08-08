
#ifndef MISC_H
#define MISC_H

#include <vector>
#include <string>
#include <cstdint>

#define SIZE_MB 1000000

std::vector<std::string> tokenize(const std::string& str, char delim);
uint64_t time_ms();

inline uint64_t mul_hi64(uint64_t h, uint64_t c)
{
    using uint128_t = __uint128_t;
    return (uint128_t(h) * uint128_t(c)) >> 64;
}

#endif // MISC_H
