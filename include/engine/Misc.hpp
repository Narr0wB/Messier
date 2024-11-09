
#ifndef MISC_H
#define MISC_H

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <chrono>

std::vector<std::string> tokenize(std::string& str, char delim);

uint64_t GetTimeMS();

#endif // MISC_H
