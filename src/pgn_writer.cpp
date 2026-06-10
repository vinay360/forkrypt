#include "pgn_writer.hpp"

#include "codec_error.hpp"

#include <ctime>
#include <iomanip>
#include <optional>

namespace {
std::optional<std::tm> safeLocalTime(std::time_t now) {
    std::tm tm{};
#if defined(_WIN32)
    if (localtime_s(&tm, &now) != 0) {
        return std::nullopt;
    }
#else
    if (localtime_r(&now, &tm) == nullptr) {
        return std::nullopt;
    }
#endif
    return tm;
}
}

PGNWriter::PGNWriter(const std::string& path) : output_(path) {
    if (!output_) {
        throw CodecError("Failed to open PGN output file: " + path);
    }
}

PGNWriter::~PGNWriter() {
    if (gameOpen_) {
        endGame();
    }
}

void PGNWriter::beginGame() {
    if (gameOpen_) {
        endGame();
    }

    const std::time_t now = std::time(nullptr);
    const auto tm = safeLocalTime(now);

    output_ << "[Event \"Binary File Encoding\"]\n";
    output_ << "[Site \"?\"]\n";
    if (tm) {
        output_ << "[Date \"" << std::put_time(&*tm, "%Y.%m.%d") << "\"]\n";
    } else {
        output_ << "[Date \"????.??.??\"]\n";
    }
    output_ << "[Round \"-\"]\n";
    output_ << "[White \"Encoder\"]\n";
    output_ << "[Black \"Encoder\"]\n";
    output_ << "[Result \"*\"]\n\n";

    if (!output_) {
        throw CodecError("Failed while writing PGN headers");
    }

    gameOpen_ = true;
    lineHasMoves_ = false;
}

void PGNWriter::writeMove(const std::string& san, bool whiteToMove, uint32_t fullMoveNumber) {
    if (!gameOpen_) {
        beginGame();
    }

    if (whiteToMove) {
        if (lineHasMoves_) {
            output_ << ' ';
        }
        output_ << fullMoveNumber << ". " << san;
    } else {
        if (!lineHasMoves_) {
            output_ << fullMoveNumber << "... ";
        } else {
            output_ << ' ';
        }
        output_ << san;
    }

    lineHasMoves_ = true;
    if (!output_) {
        throw CodecError("Failed while writing PGN move");
    }
}

void PGNWriter::endGame() {
    if (!gameOpen_) {
        return;
    }

    if (lineHasMoves_) {
        output_ << ' ';
    }
    output_ << "*\n\n";
    if (!output_) {
        throw CodecError("Failed while ending PGN game");
    }

    gameOpen_ = false;
    lineHasMoves_ = false;
}
