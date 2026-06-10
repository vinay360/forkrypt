#include "decoder.hpp"

#include "codec_error.hpp"
#include "game_state.hpp"
#include "pgn_reader.hpp"

#include <algorithm>
#include <iostream>

ChessDecoder::PayloadSink::PayloadSink(FileWriter& writer, bool showProgress)
    : writer_(writer), showProgress_(showProgress) {
    metadata_.reserve(8);
}

void ChessDecoder::PayloadSink::writeBits(uint64_t value, uint32_t count) {
    if (complete()) {
        return;
    }

    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t shift = count - i - 1;
        currentByte_ = static_cast<uint8_t>((currentByte_ << 1) | ((value >> shift) & 1ULL));
        ++bitsFilled_;

        if (bitsFilled_ == 8) {
            consumeByte(currentByte_);
            currentByte_ = 0;
            bitsFilled_ = 0;
            if (complete()) {
                return;
            }
        }
    }
}

bool ChessDecoder::PayloadSink::complete() const noexcept {
    return haveSize_ && bytesWritten_ >= expectedSize_;
}

void ChessDecoder::PayloadSink::finish() {
    if (!haveSize_) {
        throw CodecError("Corrupt metadata: PGN ended before file size could be decoded");
    }
    if (bytesWritten_ != expectedSize_) {
        throw CodecError("Decoding size mismatch: PGN ended before output file was complete");
    }
    progress_.store(expectedSize_, std::memory_order_relaxed);
    if (bar_) {
        bar_->done();
    }
    writer_.flush();
}

void ChessDecoder::PayloadSink::consumeByte(uint8_t byte) {
    if (!haveSize_) {
        metadata_.push_back(byte);
        if (metadata_.size() == 8) {
            expectedSize_ = 0;
            for (uint8_t b : metadata_) {
                expectedSize_ = (expectedSize_ << 8U) | b;
            }
            haveSize_ = true;
            if (showProgress_) {
                barkeep::ProgressBarConfig<uint64_t> cfg;
                cfg.out = &std::cerr;
                cfg.total = expectedSize_;
                cfg.message = "Decoding";
                cfg.speed = 0.2;
                cfg.speed_unit = "B/s";
                bar_ = barkeep::ProgressBar(&progress_, cfg);
            }
        }
        return;
    }

    if (bytesWritten_ < expectedSize_) {
        writer_.writeByte(byte);
        ++bytesWritten_;
        progress_.store(bytesWritten_, std::memory_order_relaxed);
    }
}

uint64_t ChessDecoder::PayloadSink::expectedSize() const noexcept {
    return expectedSize_;
}

uint64_t ChessDecoder::PayloadSink::bytesWritten() const noexcept {
    return bytesWritten_;
}

void ChessDecoder::decode(const std::string& inputPgnPath, const std::string& outputPath, bool showProgress) {
    FileWriter file(outputPath);
    PayloadSink sink(file, showProgress);
    GameState game;
    PGNReader reader;
    uint64_t movesRead = 0;
    uint64_t gamesRead = 0;

    reader.read(
        inputPgnPath,
        [&game]() {
            game.reset();
        },
        [&game, &sink, &movesRead](std::string_view san) {
            const auto legalMoves = game.legalMoves();
            if (legalMoves.empty()) {
                throw CodecError("Illegal PGN: move encountered in a terminal position");
            }

            chess::Move played;
            try {
                played = chess::uci::parseSan(game.board(), san);
            } catch (const std::exception& ex) {
                throw CodecError(std::string("Invalid SAN move '") + std::string(san) + "': " + ex.what());
            }

            const auto found = std::find(legalMoves.begin(), legalMoves.end(), played);
            if (found == legalMoves.end()) {
                throw CodecError("Illegal move encountered while decoding PGN: " + std::string(san));
            }

            const size_t index = static_cast<size_t>(std::distance(legalMoves.begin(), found));
            const uint32_t usable = GameState::usableBits(legalMoves.size());
            const size_t subsetSize = size_t{1} << usable;
            if (index >= subsetSize) {
                throw CodecError("Corrupt PGN: move uses discarded branch outside encoded subset: " + std::string(san));
            }

            sink.writeBits(static_cast<uint64_t>(index), usable);
            game.play(played);
            ++movesRead;
        },
        [&gamesRead]() {
            ++gamesRead;
        });

    sink.finish();
    if (showProgress) {
        std::cerr << "Decoded " << sink.bytesWritten() << " data bytes from " << movesRead
                  << " moves across " << gamesRead << " PGN game(s).\n";
    }
}
