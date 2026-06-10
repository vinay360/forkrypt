#include "file_io.hpp"

#include "codec_error.hpp"

#include <limits>

FileReader::FileReader(const std::string& path) : input_(path, std::ios::binary) {
    if (!input_) {
        throw CodecError("Failed to open input file: " + path);
    }

    input_.seekg(0, std::ios::end);
    const auto end = input_.tellg();
    if (end < 0) {
        throw CodecError("Failed to determine input file size: " + path);
    }

    const auto unsignedEnd = static_cast<unsigned long long>(end);
    if (unsignedEnd > std::numeric_limits<uint64_t>::max()) {
        throw CodecError("Input file is too large: " + path);
    }

    size_ = static_cast<uint64_t>(unsignedEnd);
    input_.seekg(0, std::ios::beg);
}

bool FileReader::readByte(uint8_t& byte) {
    if (metadataIndex_ < 8) {
        const uint32_t shift = (7U - metadataIndex_) * 8U;
        byte = static_cast<uint8_t>((size_ >> shift) & 0xffU);
        ++metadataIndex_;
        return true;
    }

    char c = 0;
    if (!input_.get(c)) {
        return false;
    }

    byte = static_cast<uint8_t>(static_cast<unsigned char>(c));
    return true;
}

uint64_t FileReader::size() const noexcept {
    return size_;
}

FileWriter::FileWriter(const std::string& path) : output_(path, std::ios::binary) {
    if (!output_) {
        throw CodecError("Failed to open output file: " + path);
    }
}

void FileWriter::writeByte(uint8_t byte) {
    const char c = static_cast<char>(byte);
    output_.write(&c, 1);
    if (!output_) {
        throw CodecError("Failed while writing output file");
    }
}

void FileWriter::flush() {
    output_.flush();
    if (!output_) {
        throw CodecError("Failed while flushing output file");
    }
}
