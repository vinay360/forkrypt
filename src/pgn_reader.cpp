#include "pgn_reader.hpp"

#include "chess.hpp"
#include "codec_error.hpp"

#include <fstream>

namespace {
class Visitor final : public chess::pgn::Visitor {
public:
    Visitor(PGNReader::BeginGameHandler beginGame,
            PGNReader::MoveHandler move,
            PGNReader::EndGameHandler endGame)
        : beginGame_(std::move(beginGame)), move_(std::move(move)), endGame_(std::move(endGame)) {}

    void startPgn() override {
        startedMoves_ = false;
    }

    void header(std::string_view, std::string_view) override {}

    void startMoves() override {
        beginGame_();
        startedMoves_ = true;
    }

    void move(std::string_view san, std::string_view) override {
        if (!san.empty()) {
            move_(san);
        }
    }

    void endPgn() override {
        if (startedMoves_) {
            endGame_();
        }
        startedMoves_ = false;
    }

private:
    PGNReader::BeginGameHandler beginGame_;
    PGNReader::MoveHandler move_;
    PGNReader::EndGameHandler endGame_;
    bool startedMoves_ = false;
};
}

void PGNReader::read(const std::string& path,
                     const BeginGameHandler& beginGame,
                     const MoveHandler& move,
                     const EndGameHandler& endGame) const {
    std::ifstream input(path);
    if (!input) {
        throw CodecError("Failed to open PGN input file: " + path);
    }

    Visitor visitor(beginGame, move, endGame);
    chess::pgn::StreamParser<> parser(input);
    const auto error = parser.readGames(visitor);
    if (error.hasError()) {
        throw CodecError("Invalid PGN: " + error.message());
    }
}
