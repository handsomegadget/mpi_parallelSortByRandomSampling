#ifndef CONVERT_H
#define CONVERT_H

#include <stddef.h>
#include <string>
bool int_convert(int& number, const std::string& candidate);
bool unsigned_convert(unsigned int& number, const std::string& candidate);
bool sizet_convert(size_t& size, const std::string& candidate);
#endif /* CONVERT_H */
