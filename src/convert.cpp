#include <iostream>
#include <string>
#include <sstream>
#include <limits>
#include "../include/macro.h"
#include "../include/convert.h"
bool int_convert(int& number, const std::string& candidate) {
    try {
        size_t pos;
        number = std::stoi(candidate, &pos);
        if (pos != candidate.size()) {
            return false;  // Not a valid integer string
        }
    } catch (const std::out_of_range&) {
        return false;  // Overflow
    } catch (const std::invalid_argument&) {
        return false;  // Not a valid integer string
    }
    return true;
}

bool unsigned_convert(unsigned int& number, const std::string& candidate) {
    try {
        size_t pos;
        number = std::stoul(candidate, &pos);
        if (pos != candidate.size()) {
            return false;  // Not a valid unsigned integer string
        }
    } catch (const std::out_of_range&) {
        return false;  // Overflow
    } catch (const std::invalid_argument&) {
        return false;  // Not a valid unsigned integer string
    }
    return true;
}

bool sizet_convert(size_t& size, const std::string& candidate) {
    try {
        size_t pos;
        size = std::stoull(candidate, &pos);
        if (pos != candidate.size()) {
            return false;  // Not a valid size_t string
        }
    } catch (const std::out_of_range&) {
        return false;  // Overflow
    } catch (const std::invalid_argument&) {
        return false;  // Not a valid size_t string
    }
    return true;
}
