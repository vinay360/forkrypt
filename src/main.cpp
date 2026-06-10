#include "decoder.hpp"
#include "encoder.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {
void usage() {
  std::cerr << "Usage:\n"
            << "  chesscodec encode <input.bin> <output.pgn>\n"
            << "  chesscodec decode <input.pgn> <output.bin>\n";
}
} // namespace

int main(int argc, char **argv) {
  if (argc != 4) {
    usage();
    return 2;
  }

  try {
    const std::string mode = argv[1];
    if (mode == "encode") {
      ChessEncoder{}.encode(argv[2], argv[3], true);
    } else if (mode == "decode") {
      ChessDecoder{}.decode(argv[2], argv[3], true);
    } else {
      usage();
      return 2;
    }
  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << '\n';
    return 1;
  }

  return 0;
}
