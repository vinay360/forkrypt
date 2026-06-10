#pragma once

#include "chess.hpp"

#include <cstdint>
#include <vector>

class GameState {
public:
    GameState();

    void reset();
    const chess::Board& board() const noexcept;
    chess::Board& board() noexcept;

    std::vector<chess::Move> legalMoves() const;
    void play(chess::Move move);
    bool isGameOver() const;

    static uint32_t usableBits(size_t legalMoveCount);

private:
    chess::Board board_;
};
