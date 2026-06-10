#pragma once

#include <cstdint>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "barkeep.hpp"
#include "file_io.hpp"

class ChessDecoder {
public:
    void decode(const std::string& inputPgnPath, const std::string& outputPath, bool showProgress = false);

private:
    class PayloadSink {
    public:
        PayloadSink(FileWriter& writer, bool showProgress);

        void writeBits(uint64_t value, uint32_t count);
        bool complete() const noexcept;
        void finish();
        uint64_t expectedSize() const noexcept;
        uint64_t bytesWritten() const noexcept;

    private:
        void consumeByte(uint8_t byte);

        FileWriter& writer_;
        bool showProgress_ = false;
        std::atomic<uint64_t> progress_{0};
        std::shared_ptr<barkeep::BaseDisplay> bar_;
        uint8_t currentByte_ = 0;
        uint32_t bitsFilled_ = 0;
        std::vector<uint8_t> metadata_;
        bool haveSize_ = false;
        uint64_t expectedSize_ = 0;
        uint64_t bytesWritten_ = 0;
    };
};
