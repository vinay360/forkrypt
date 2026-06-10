#pragma once

#include <cstdint>
#include <fstream>
#include <string>

class FileReader {
public:
    explicit FileReader(const std::string& path);

    bool readByte(uint8_t& byte);
    uint64_t size() const noexcept;
    uint64_t bytesEmitted() const noexcept;

private:
    std::ifstream input_;
    uint64_t size_ = 0;
    uint64_t bytesEmitted_ = 0;
    uint32_t metadataIndex_ = 0;
};

class FileWriter {
public:
    explicit FileWriter(const std::string& path);

    void writeByte(uint8_t byte);
    void flush();

private:
    std::ofstream output_;
};
