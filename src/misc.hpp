
#ifndef MISC_H
#define MISC_H

#include <vector>
#include <string>
#include <cstdint>

std::vector<std::string> tokenize(const std::string& str, char delim);

uint64_t GetTimeMS();

#endif // MISC_H
