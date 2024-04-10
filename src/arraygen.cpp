#include "../include/arraygen.h"

#include <stdexcept>
#include <cstdlib>
#include <ctime>

int Generator::array_generate(long** array, const size_t length) {
    if (array == nullptr || length == 0) {
        throw std::invalid_argument("Invalid input parameters");
    }

    if (*array == nullptr) {
        *array = new long[length];
    }
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    for (size_t i = 0; i < length; ++i) {
        (*array)[i] = std::rand();
    }

    return 0;
}

int Generator::array_destroy(long** array) {
    if (array == nullptr || *array == nullptr) {
        throw std::invalid_argument("Invalid input parameter");
    }

    delete[] *array;
    *array = nullptr;
    return 0;
}
