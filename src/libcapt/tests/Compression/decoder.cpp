#include "libcapt/Compression/Decoder/DecoderError.hpp"
#include "libcapt/Core/StreamPacket.hpp"
#include "libcapt/Compression/Decoder/DecoderStreambuf.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <streambuf>

using namespace Capt;

class VideoDataStreambuf : public std::streambuf {
private:
    std::vector<uint8_t> buffer;
    std::istream& stream;
public:
    unsigned LineSize = 0;
    unsigned LinesCount = 0;

protected:
    int_type underflow() override {
        if (!this->stream.good()) {
            return traits_type::eof();
        }
        if (this->gptr() < this->egptr()) {
            return traits_type::to_int_type(*this->gptr());
        }

        assert(this->LineSize != 0 && this->LinesCount != 0);
        while (true) {
            StreamPacket packet;
            if (!(this->stream >> packet)) {
                return traits_type::eof();
            }
            if (packet.Header.Opcode == 3 || packet.Header.Opcode == 0xD0A2) {
                return traits_type::eof();
            } else if (packet.Header.Opcode == 0xC0A0) {
                this->buffer.resize(packet.Header.PayloadSize);
                packet.ReadBytes(this->buffer);
                if (!this->stream.good()) {
                    return traits_type::eof();
                }
                break;
            }
        }

        char_type* start = reinterpret_cast<char_type*>(this->buffer.data());
        this->setg(start, start, start + this->buffer.size());
        return traits_type::to_int_type(*this->gptr());
    }

public:
    explicit VideoDataStreambuf(std::istream& reader) noexcept : stream(reader) {}

    bool ReadHeader() {
        StreamPacket packet;
        while (this->stream >> packet) {
            if (packet.Header.Opcode == 0xD0A0) {
                if (packet.Header.PayloadSize < 33) {
                    throw Compression::DecoderError("invalid packet size");
                }
                std::vector<uint8_t> payload(packet.Header.PayloadSize);
                packet.ReadBytes(payload);
                if (!this->stream.good()) {
                    return false;
                }
                this->LineSize = (static_cast<unsigned>(payload[31-4]) << 8) | static_cast<unsigned>(payload[30-4]);
                this->LinesCount = (static_cast<unsigned>(payload[33-4]) << 8) | static_cast<unsigned>(payload[32-4]);
                return true;
            }
        }
        return false;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::fprintf(stderr, "Usage: %s filterout outfile cmdfile\n", argv[0]);
        return 1;
    }
    std::ifstream filterInput(argv[1], std::ios_base::in | std::ios_base::binary);
    if (!filterInput.is_open()) {
        std::fputs("Failed to open filterout file\n", stderr);
        return 1;
    }

    std::ofstream cmdout(argv[3], std::ios_base::out);
    if (!cmdout.is_open()) {
        std::fputs("Failed to open cmdfile\n", stderr);
        return 1;
    }
    std::ofstream outfile(argv[2], std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
    if (!outfile.is_open()) {
        std::fputs("Failed to open outfile\n", stderr);
        return 1;
    }

    unsigned page = 0;
    while (true) {
        VideoDataStreambuf videoStreambuf(filterInput);
        if (!videoStreambuf.ReadHeader()) {
            break;
        }
        Capt::Compression::DecoderStreambuf dec(videoStreambuf, videoStreambuf.LineSize, &cmdout);

        outfile << "P4\n" << (videoStreambuf.LineSize * 8) << " " << videoStreambuf.LinesCount << "\n";

        std::size_t decodedSize = 0;
        const std::size_t expectedSize = videoStreambuf.LineSize * videoStreambuf.LinesCount;
        std::vector<char> buffer(videoStreambuf.LineSize);
        while (true) {
            std::streamsize read = dec.sgetn(buffer.data(), buffer.size());
            if (read == 0) {
                break;
            }
            outfile.write(buffer.data(), read);
            decodedSize += read;
        }

        std::fprintf(stderr, "Page: %u\n", page + 1);
        std::fprintf(stderr, "Decoded size  = %zu\n", decodedSize);
        std::fprintf(stderr, "Expected size = %zu\n", expectedSize);
        if (decodedSize != expectedSize) {
            throw Compression::DecoderError("Raster size mismatch");
        }
        page++;
    }
    std::fprintf(stderr, "Pages decoded: %u\n", page);
    return 0;
}
