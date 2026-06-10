#include "bitstream.hpp"

BitReader::BitReader(FileReader& reader) : reader_(reader) {}

bool BitReader::readBits(uint32_t count, uint64_t& value) {
    value = 0;
    bool consumedAny = false;

    for (uint32_t i = 0; i < count; ++i) {
        if (bitsRemaining_ == 0) {
            if (eof_ || !reader_.readByte(currentByte_)) {
                eof_ = true;
                value <<= (count - i);
                return consumedAny;
            }
            bitsRemaining_ = 8;
        }

        value = (value << 1) | ((currentByte_ >> 7) & 1U);
        currentByte_ <<= 1;
        --bitsRemaining_;
        consumedAny = true;
    }

    return true;
}

void BitWriter::writeBits(uint64_t value, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t shift = count - i - 1;
        currentByte_ = static_cast<uint8_t>((currentByte_ << 1) | ((value >> shift) & 1ULL));
        ++bitsFilled_;

        if (bitsFilled_ == 8) {
            bytes_.push_back(currentByte_);
            currentByte_ = 0;
            bitsFilled_ = 0;
        }
    }
}

std::vector<uint8_t> BitWriter::takeBytes() {
    std::vector<uint8_t> out;
    out.swap(bytes_);
    return out;
}

void BitWriter::flushZeroPaddedByte() {
    if (bitsFilled_ == 0) {
        return;
    }

    currentByte_ <<= (8 - bitsFilled_);
    bytes_.push_back(currentByte_);
    currentByte_ = 0;
    bitsFilled_ = 0;
}
