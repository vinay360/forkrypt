#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "file_io.hpp"

class ChessDecoder {
public:
    void decode(const std::string& inputPgnPath, const std::string& outputPath);

private:
    class PayloadSink {
    public:
        explicit PayloadSink(FileWriter& writer);

        void writeBits(uint64_t value, uint32_t count);
        bool complete() const noexcept;
        void finish();

    private:
        void consumeByte(uint8_t byte);

        FileWriter& writer_;
        uint8_t currentByte_ = 0;
        uint32_t bitsFilled_ = 0;
        std::vector<uint8_t> metadata_;
        bool haveSize_ = false;
        uint64_t expectedSize_ = 0;
        uint64_t bytesWritten_ = 0;
    };
};
