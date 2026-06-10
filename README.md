# chesscodec

`chesscodec` losslessly encodes binary files into legal, playable chess PGN and decodes those PGNs back to the original bytes.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Usage

```bash
./build/chesscodec encode input.bin output.pgn
./build/chesscodec decode output.pgn recovered.bin
sha256sum input.bin recovered.bin
```

## Encoding Math

For each position, the codec asks Disservin's chess library for legal moves. If there are `N` legal moves, it can encode `floor(log2(N))` bits because only a power-of-two subset can map uniformly to bit patterns. For example, `37` legal moves gives `5` usable bits, so only the first `32` sorted legal moves are used.

Moves are sorted by the library's stable internal move value before encoding and decoding. This makes the mapping deterministic.

## Edge Cases

The payload begins with an 8-byte big-endian file size, followed by file bytes. The final bit group is zero-padded when fewer bits remain than the current position can carry. During decoding, the stored size controls exactly how many bytes are written, so padding bits are ignored.

If only one legal move exists, zero bits are encoded and the forced move is played. If a game reaches checkmate, stalemate, insufficient material, repetition, or the fifty-move rule, the writer closes that PGN game and starts another from the normal initial position.

Invalid PGN, illegal SAN moves, corrupt move choices outside the encoded subset, missing files, bad metadata, and output size mismatches are reported as exceptions and CLI errors.

## Tests

The test binary round-trips empty data, a one-byte file, random 1 KB data, and small PNG/ZIP/executable-style binary fixtures using SHA-256 comparison. The 1 MB random test is available with:

```bash
CHESSCODEC_LARGE_TESTS=1 ctest --test-dir build
```
