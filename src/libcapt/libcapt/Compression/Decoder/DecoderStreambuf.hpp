#pragma once
#include <ostream>
#include <cstddef>
#include <vector>
#include <cstdint>

namespace Capt::Compression {
    class DecoderStreambuf : public std::streambuf {
    private:
        std::streambuf& stream;
        std::ostream* commandLog;

        std::vector<uint8_t> buffer;
        unsigned lineSize;
        std::size_t videoSize;

        void repeat(unsigned count, uint8_t repeatByte);
        void repeatX(unsigned count);
        void copy(unsigned count);
        void raw(const std::vector<uint8_t>& data);
        void endLine();

        int decodeNext();
        bool decodeLine();

        int_type underflow() override;
    public:
        explicit DecoderStreambuf(std::streambuf& stream, unsigned lineSize, std::ostream* commandLog = nullptr) noexcept;
    };
}
