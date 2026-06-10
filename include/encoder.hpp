#pragma once

#include <string>

class ChessEncoder {
public:
    void encode(const std::string& inputPath, const std::string& outputPgnPath, bool showProgress = false);
};
