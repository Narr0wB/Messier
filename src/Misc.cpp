
#include <engine/Misc.hpp>

std::vector<std::string> tokenize(std::string& str, char delim) {
    std::stringstream stream(str);
    std::string tmp;
    std::vector<std::string> tokens;

    while (std::getline(stream, tmp, delim)) {
        tokens.push_back(tmp);
    }

    return tokens;
}

uint64_t GetTimeMS() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}