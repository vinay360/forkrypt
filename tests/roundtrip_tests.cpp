#include "decoder.hpp"
#include "encoder.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <string>
#include <vector>

namespace {
class Sha256 {
public:
    void update(const uint8_t* data, size_t len) {
        bitLen_ += static_cast<uint64_t>(len) * 8ULL;
        for (size_t i = 0; i < len; ++i) {
            buffer_[bufferLen_++] = data[i];
            if (bufferLen_ == 64) {
                transform(buffer_.data());
                bufferLen_ = 0;
            }
        }
    }

    std::array<uint8_t, 32> final() {
        buffer_[bufferLen_++] = 0x80;
        if (bufferLen_ > 56) {
            while (bufferLen_ < 64) {
                buffer_[bufferLen_++] = 0;
            }
            transform(buffer_.data());
            bufferLen_ = 0;
        }
        while (bufferLen_ < 56) {
            buffer_[bufferLen_++] = 0;
        }
        for (int i = 7; i >= 0; --i) {
            buffer_[bufferLen_++] = static_cast<uint8_t>((bitLen_ >> (i * 8)) & 0xffU);
        }
        transform(buffer_.data());

        std::array<uint8_t, 32> digest{};
        for (size_t i = 0; i < state_.size(); ++i) {
            digest[i * 4] = static_cast<uint8_t>((state_[i] >> 24) & 0xffU);
            digest[i * 4 + 1] = static_cast<uint8_t>((state_[i] >> 16) & 0xffU);
            digest[i * 4 + 2] = static_cast<uint8_t>((state_[i] >> 8) & 0xffU);
            digest[i * 4 + 3] = static_cast<uint8_t>(state_[i] & 0xffU);
        }
        return digest;
    }

private:
    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
    static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
    static uint32_t bsig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    static uint32_t bsig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    static uint32_t ssig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static uint32_t ssig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    void transform(const uint8_t* chunk) {
        static constexpr std::array<uint32_t, 64> k = {
            0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
            0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
            0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
            0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
            0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
            0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
            0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
            0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

        std::array<uint32_t, 64> w{};
        for (size_t i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(chunk[i * 4]) << 24) |
                   (static_cast<uint32_t>(chunk[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(chunk[i * 4 + 2]) << 8) |
                   static_cast<uint32_t>(chunk[i * 4 + 3]);
        }
        for (size_t i = 16; i < 64; ++i) {
            w[i] = ssig1(w[i - 2]) + w[i - 7] + ssig0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = state_[0];
        uint32_t b = state_[1];
        uint32_t c = state_[2];
        uint32_t d = state_[3];
        uint32_t e = state_[4];
        uint32_t f = state_[5];
        uint32_t g = state_[6];
        uint32_t h = state_[7];

        for (size_t i = 0; i < 64; ++i) {
            const uint32_t t1 = h + bsig1(e) + ch(e, f, g) + k[i] + w[i];
            const uint32_t t2 = bsig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    std::array<uint32_t, 8> state_ = {0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
                                      0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
    std::array<uint8_t, 64> buffer_{};
    size_t bufferLen_ = 0;
    uint64_t bitLen_ = 0;
};

std::array<uint8_t, 32> sha256File(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open file for hashing: " + path.string());
    }

    Sha256 sha;
    std::array<uint8_t, 4096> buffer{};
    while (input) {
        input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const auto got = input.gcount();
        if (got > 0) {
            sha.update(buffer.data(), static_cast<size_t>(got));
        }
    }
    return sha.final();
}

void writeFile(const std::filesystem::path& path, const std::vector<uint8_t>& data) {
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!output) {
        throw std::runtime_error("failed to write fixture: " + path.string());
    }
}

std::vector<uint8_t> randomBytes(size_t size) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> data(size);
    for (auto& b : data) {
        b = static_cast<uint8_t>(dist(rng));
    }
    return data;
}

void roundTrip(const std::filesystem::path& dir, const std::string& name, const std::vector<uint8_t>& data) {
    const auto input = dir / (name + ".bin");
    const auto pgn = dir / (name + ".pgn");
    const auto output = dir / (name + ".out");

    writeFile(input, data);
    ChessEncoder{}.encode(input.string(), pgn.string());
    ChessDecoder{}.decode(pgn.string(), output.string());

    if (sha256File(input) != sha256File(output)) {
        throw std::runtime_error("SHA-256 mismatch for " + name);
    }
}

void roundTripInParallel(const std::filesystem::path& dir) {
    constexpr size_t threadCount = 4;
    std::mutex exceptionMutex;
    std::exception_ptr firstFailure;
    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (size_t i = 0; i < threadCount; ++i) {
        workers.emplace_back([&, i]() {
            try {
                roundTrip(dir, "parallel_" + std::to_string(i), randomBytes(2048 + i * 512));
            } catch (...) {
                std::lock_guard<std::mutex> lock(exceptionMutex);
                if (!firstFailure) {
                    firstFailure = std::current_exception();
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    if (firstFailure) {
        std::rethrow_exception(firstFailure);
    }
}
}

int main() {
    try {
        const auto dir = std::filesystem::temp_directory_path() / "chesscodec_roundtrip_tests";
        std::filesystem::create_directories(dir);

        roundTrip(dir, "empty", {});
        roundTrip(dir, "one_byte", {0xa5});
        roundTrip(dir, "random_1kb", randomBytes(1024));
        roundTrip(dir, "png_fixture", {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n', 0, 0, 0, 0x0d, 'I', 'H', 'D', 'R'});
        roundTrip(dir, "zip_fixture", {'P', 'K', 3, 4, 20, 0, 0, 0, 8, 0, 1, 2, 3, 4, 0xaa, 0xbb});
        roundTrip(dir, "exe_fixture", {'M', 'Z', 0x90, 0, 3, 0, 0, 0, 4, 0, 0, 0xff, 0xff, 0, 0xb8, 0});
        roundTripInParallel(dir);

        if (std::getenv("CHESSCODEC_LARGE_TESTS")) {
            roundTrip(dir, "random_1mb", randomBytes(1024 * 1024));
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }

    return 0;
}
