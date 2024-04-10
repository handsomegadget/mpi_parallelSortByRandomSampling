#ifndef ARRAYGEN_H
#define ARRAYGEN_H

#include "macro.h"

#include <cstddef>

class Generator {
public:
    Generator() = default;
    ~Generator() = default;

    int array_generate(long** array, const size_t length);
    int array_destroy(long** array);
};

#endif /* ARRAYGEN_H */
