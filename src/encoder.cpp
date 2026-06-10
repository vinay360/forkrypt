#include "encoder.hpp"

#include "bitstream.hpp"
#include "barkeep.hpp"
#include "codec_error.hpp"
#include "file_io.hpp"
#include "game_state.hpp"
#include "pgn_writer.hpp"

#include <atomic>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t metadataBytes = 8;
}

void ChessEncoder::encode(const std::string& inputPath, const std::string& outputPgnPath, bool showProgress) {
    FileReader file(inputPath);
    BitReader bits(file);
    PGNWriter pgn(outputPgnPath);
    GameState game;
    const uint64_t totalBytes = file.size() + metadataBytes;
    uint64_t movesWritten = 0;
    uint64_t gamesWritten = 1;

    std::atomic<uint64_t> progress{0};
    std::shared_ptr<barkeep::BaseDisplay> bar;
    if (showProgress) {
        barkeep::ProgressBarConfig<uint64_t> cfg;
        cfg.out = &std::cerr;
        cfg.total = totalBytes;
        cfg.message = "Encoding";
        cfg.speed = 0.2;
        cfg.speed_unit = "B/s";
        bar = barkeep::ProgressBar(&progress, cfg);
    }

    pgn.beginGame();

    while (true) {
        if (game.isGameOver()) {
            pgn.endGame();
            game.reset();
            pgn.beginGame();
            ++gamesWritten;
        }

        const auto moves = game.legalMoves();
        if (moves.empty()) {
            pgn.endGame();
            game.reset();
            pgn.beginGame();
            ++gamesWritten;
            continue;
        }

        const uint32_t usable = GameState::usableBits(moves.size());
        uint64_t index = 0;

        if (usable == 0) {
            index = 0;
        } else if (!bits.readBits(usable, index)) {
            break;
        }
        progress.store(file.bytesEmitted(), std::memory_order_relaxed);

        if (index >= moves.size()) {
            throw CodecError("Internal encoder error: move index outside legal move list");
        }

        const auto move = moves[static_cast<size_t>(index)];
        const bool whiteToMove = game.board().sideToMove() == chess::Color::WHITE;
        const uint32_t fullMoveNumber = game.board().fullMoveNumber();
        const std::string san = chess::uci::moveToSan(game.board(), move);
        pgn.writeMove(san, whiteToMove, fullMoveNumber);
        game.play(move);
        ++movesWritten;
    }

    progress.store(totalBytes, std::memory_order_relaxed);
    if (bar) {
        bar->done();
        std::cerr << "Encoded " << file.size() << " data bytes (" << totalBytes
                  << " bytes including metadata) into " << movesWritten << " moves across "
                  << gamesWritten << " PGN game(s).\n";
    }
    pgn.endGame();
}
