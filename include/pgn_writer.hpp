#pragma once

#include <cstdint>
#include <fstream>
#include <string>

class PGNWriter {
public:
    explicit PGNWriter(const std::string& path);
    ~PGNWriter();

    void beginGame();
    void writeMove(const std::string& san, bool whiteToMove, uint32_t fullMoveNumber);
    void endGame();

private:
    std::ofstream output_;
    bool gameOpen_ = false;
    bool lineHasMoves_ = false;
};
