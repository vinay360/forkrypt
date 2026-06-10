#pragma once

#include <functional>
#include <string>
#include <string_view>

class PGNReader {
public:
    using BeginGameHandler = std::function<void()>;
    using MoveHandler = std::function<void(std::string_view)>;
    using EndGameHandler = std::function<void()>;

    void read(const std::string& path,
              const BeginGameHandler& beginGame,
              const MoveHandler& move,
              const EndGameHandler& endGame) const;
};
