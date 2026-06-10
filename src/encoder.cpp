#include "encoder.hpp"

#include "bitstream.hpp"
#include "codec_error.hpp"
#include "file_io.hpp"
#include "game_state.hpp"
#include "pgn_writer.hpp"

#include <cstdint>

void ChessEncoder::encode(const std::string& inputPath, const std::string& outputPgnPath) {
    FileReader file(inputPath);
    BitReader bits(file);
    PGNWriter pgn(outputPgnPath);
    GameState game;

    pgn.beginGame();

    while (true) {
        if (game.isGameOver()) {
            pgn.endGame();
            game.reset();
            pgn.beginGame();
        }

        const auto moves = game.legalMoves();
        if (moves.empty()) {
            pgn.endGame();
            game.reset();
            pgn.beginGame();
            continue;
        }

        const uint32_t usable = GameState::usableBits(moves.size());
        uint64_t index = 0;

        if (usable == 0) {
            index = 0;
        } else if (!bits.readBits(usable, index)) {
            break;
        }

        if (index >= moves.size()) {
            throw CodecError("Internal encoder error: move index outside legal move list");
        }

        const auto move = moves[static_cast<size_t>(index)];
        const bool whiteToMove = game.board().sideToMove() == chess::Color::WHITE;
        const uint32_t fullMoveNumber = game.board().fullMoveNumber();
        const std::string san = chess::uci::moveToSan(game.board(), move);
        pgn.writeMove(san, whiteToMove, fullMoveNumber);
        game.play(move);
    }

    pgn.endGame();
}
