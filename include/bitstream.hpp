#pragma once

#include "file_io.hpp"

#include <cstdint>
#include <vector>

class BitReader {
public:
    explicit BitReader(FileReader& reader);

    // Reads count bits, MSB first. If EOF occurs after at least one bit,
    // remaining bits are zero-padded and true is returned.
    bool readBits(uint32_t count, uint64_t& value);

private:
    FileReader& reader_;
    uint8_t currentByte_ = 0;
    uint32_t bitsRemaining_ = 0;
    bool eof_ = false;
};

class BitWriter {
public:
    void writeBits(uint64_t value, uint32_t count);
    std::vector<uint8_t> takeBytes();
    void flushZeroPaddedByte();

private:
    std::vector<uint8_t> bytes_;
    uint8_t currentByte_ = 0;
    uint32_t bitsFilled_ = 0;
};
