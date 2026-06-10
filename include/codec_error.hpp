#pragma once

#include <stdexcept>
#include <string>

class CodecError : public std::runtime_error {
public:
    explicit CodecError(const std::string& message) : std::runtime_error(message) {}
};
