#include "game_state.hpp"

#include <algorithm>

GameState::GameState() : board_(chess::constants::STARTPOS) {}

void GameState::reset() {
    board_ = chess::Board(chess::constants::STARTPOS);
}

const chess::Board& GameState::board() const noexcept {
    return board_;
}

chess::Board& GameState::board() noexcept {
    return board_;
}

std::vector<chess::Move> GameState::legalMoves() const {
    chess::Movelist list;
    chess::movegen::legalmoves(list, board_);

    std::vector<chess::Move> moves(list.begin(), list.end());
    std::sort(moves.begin(), moves.end(), [](const chess::Move& a, const chess::Move& b) {
        return a.move() < b.move();
    });
    return moves;
}

void GameState::play(chess::Move move) {
    board_.makeMove(move);
}

bool GameState::isGameOver() const {
    return board_.isGameOver().first != chess::GameResultReason::NONE;
}

uint32_t GameState::usableBits(size_t legalMoveCount) {
    uint32_t bits = 0;
    while (legalMoveCount > 1) {
        legalMoveCount >>= 1U;
        ++bits;
    }
    return bits;
}
