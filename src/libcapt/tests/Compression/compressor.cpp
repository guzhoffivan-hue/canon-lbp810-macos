#include "libcapt/Compression/ScoaStreambuf.hpp"
#include "libcapt/Protocol/Protocol.hpp"
#include <cctype>
#include <fstream>
#include <istream>
#include <stdexcept>
#include <vector>

using namespace Capt;

static bool readPbmHeader(std::istream& stream, unsigned& width, unsigned& height) {
    char buffer[3];
    stream.read(buffer, sizeof(buffer));
    if (stream.eof()) {
        return false;
    }
    if (buffer[0] != 'P' || buffer[1] != '4' || !std::isspace(buffer[2])) {
        throw std::runtime_error("PBM: invalid magic");
    }
    while (stream.peek() == '#') {
        while (stream.get() != '\n') {
            if (stream.eof()) {
                throw std::runtime_error("PBM: unexpected EOF");
            }
        }
    }
    if (!(stream >> width)) {
        throw std::runtime_error("PBM: failed to read width");
    }
    if (!std::isspace(stream.get())) {
        throw std::runtime_error("PBM: unexpected char");
    }
    if (!(stream >> height)) {
        throw std::runtime_error("PBM: failed to read height");
    }
    while (!std::isspace(stream.get()) && stream.good());
    if (stream.eof()) {
        throw std::runtime_error("PBM: unexpected EOF");
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::fprintf(stderr, "Usage %s pbmfile outfile\n", argv[0]);
        return 1;
    }

    std::ifstream pbmStream(argv[1], std::ios_base::in | std::ios_base::binary);
    if (!pbmStream.is_open()) {
        std::fputs("Failed to open pbmfile\n", stderr);
        return 1;
    }

    std::ofstream outStream(argv[2], std::ios_base::trunc | std::ios_base::out | std::ios_base::binary);
    if (!outStream.is_open()) {
        std::fputs("Failed to open outfile\n", stderr);
        return 1;
    }

    unsigned page = 0;
    Compression::ScoaStreambuf scoaStreambuf;
    const std::size_t maxsize = 4096;
    std::vector<uint8_t> buffer(maxsize);
    while (true) {
        unsigned lineSize;
        unsigned lines;
        if (!readPbmHeader(pbmStream, lineSize, lines)) {
            break;
        }
        if (lineSize % 8 != 0) {
            throw std::runtime_error("PBM width must be a multiple of 8");
        }
        lineSize /= 8;
        scoaStreambuf.Reset(*pbmStream.rdbuf(), lineSize, lines);

        PageParams pp;
        pp.ImageLineSize = static_cast<uint16_t>(lineSize);
        pp.ImageLines = static_cast<uint16_t>(lines);
        std::fprintf(stderr, "Page: %u\n", page + 1);
        std::fprintf(stderr, "LineSize = %u\n", pp.ImageLineSize);
        std::fprintf(stderr, "Lines = %u\n", pp.ImageLines);

        Protocol::IC_BEGIN_PAGE(outStream, pp);
        Protocol::IC_BEGIN_DATA(outStream);
        while (true) {
            std::streamsize read = scoaStreambuf.sgetn(reinterpret_cast<char*>(buffer.data()), buffer.size());
            if (read <= 0) {
                break;
            }
            Protocol::IC_VIDEO_DATA(outStream, {buffer.data(), static_cast<std::size_t>(read)});
        }
        Protocol::IC_END_PAGE(outStream);
        page++;
    }
    std::fprintf(stderr, "Pages compressed: %u\n", page);
    return 0;
}
