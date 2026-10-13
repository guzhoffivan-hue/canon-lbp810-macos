#include "CropStreambuf.hpp"
#include <algorithm>
#include <cassert>
#include <streambuf>

namespace Capt::Utility {
    using int_type = CropStreambuf::int_type;

    static std::streamsize discard(std::streambuf& stream, std::streamsize count) {
        using traits_type = std::streambuf::traits_type;
        std::streamsize read = 0;
        while (read != count) {
            std::streambuf::int_type c = stream.sbumpc();
            read++;
            if (traits_type::eq_int_type(c, traits_type::eof())) {
                break;
            }
        }
        return read;
    }

    void CropStreambuf::discardLines() {
        assert(this->currentLine <= this->origLines);
        while (this->currentLine != this->origLines) {
            if (discard(this->rasterStream, this->origLineSize) == 0) {
                break;
            }
            this->currentLine++;
        }
        this->currentLine = 0;
    }

    void CropStreambuf::endLine() {
        discard(this->rasterStream, this->origLineSize - this->cropLineSize);
        this->lineRead = 0;
        this->currentLine++;
        if (this->currentLine == this->cropLines) {
            this->discardLines();
        }
    }

    int_type CropStreambuf::underflow() {
        assert(this->lineRead < this->cropLineSize);
        return this->rasterStream.sgetc();
    }

    int_type CropStreambuf::uflow() {
        char c;
        if (this->xsgetn(&c, 1) == 0) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(c);
    }

    std::streamsize CropStreambuf::xsgetn(char_type* s, std::streamsize n) {
        if (n <= 0) {
            return n;
        }
        std::streamsize result = 0;
        while (static_cast<unsigned>(n) > this->cropLineSize) {
            std::streamsize read = this->xsgetn(s, this->cropLineSize);
            result += read;
            if (read == 0) {
                return result;
            }
            n -= read;
            s += read;
        }

        unsigned count = std::min(static_cast<unsigned>(n), this->cropLineSize - this->lineRead);
        std::streamsize read = this->rasterStream.sgetn(s, count);
        this->lineRead += read;
        assert(this->lineRead <= this->cropLineSize);
        if (this->lineRead == this->cropLineSize) {
            this->endLine();
        }
        return result + read;
    }

    CropStreambuf::CropStreambuf(std::streambuf& rasterStream, unsigned origLineSize, unsigned origLines, unsigned cropLineSize, unsigned cropLines) noexcept
        : rasterStream(rasterStream), origLineSize(origLineSize), origLines(origLines), cropLineSize(cropLineSize), cropLines(cropLines) {}
}
