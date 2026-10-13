#pragma once
#include <iostream>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Capt::Compression {
    class ScoaStreambuf : public std::streambuf {
    private:
        std::streambuf* rasterStream = nullptr;
        std::vector<uint8_t> buffer;
        std::vector<uint8_t> currLine;
        std::vector<uint8_t> prevLine;

        unsigned linesRemain = 0;
        std::size_t videoSize = 0;

        int_type underflow() override;
    public:
        explicit ScoaStreambuf(std::size_t bufferReserve = 2048);
        explicit ScoaStreambuf(std::streambuf& rasterStream, unsigned lineSize, unsigned lines, std::size_t bufferReserve = 2048);

        void Reset(std::streambuf& rasterStream, unsigned lineSize, unsigned lines);
    };
}
