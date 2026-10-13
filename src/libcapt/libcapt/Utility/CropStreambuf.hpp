#pragma once
#include <streambuf>

namespace Capt::Utility {
    class CropStreambuf : public std::streambuf {
    private:
        std::streambuf& rasterStream;
        unsigned origLineSize;
        unsigned origLines;
        unsigned cropLineSize;
        unsigned cropLines;

        unsigned currentLine = 0;
        unsigned lineRead = 0;

        void discardLines();
        void endLine();
        int_type underflow() override;
        int_type uflow() override;
        std::streamsize xsgetn(char_type* s, std::streamsize n) override;
    public:
        explicit CropStreambuf(std::streambuf& rasterStream, unsigned origLineSize, unsigned origLines, unsigned cropLineSize, unsigned cropLines) noexcept;
    };
}
