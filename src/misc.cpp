
#include "misc.hpp" 

#include <iostream>
#include <sstream>
#include <chrono>

std::vector<std::string> tokenize(const std::string& str, char delim) {
    std::stringstream stream(str);
    std::string tmp;
    std::vector<std::string> tokens;

    while (std::getline(stream, tmp, delim)) {
        tokens.push_back(tmp);
    }

    return tokens;
}

uint64_t time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}