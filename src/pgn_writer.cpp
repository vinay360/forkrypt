#include "pgn_writer.hpp"

#include "codec_error.hpp"

#include <ctime>
#include <iomanip>

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
    const std::tm* tm = std::localtime(&now);

    output_ << "[Event \"Binary File Encoding\"]\n";
    output_ << "[Site \"?\"]\n";
    if (tm) {
        output_ << "[Date \"" << std::put_time(tm, "%Y.%m.%d") << "\"]\n";
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
