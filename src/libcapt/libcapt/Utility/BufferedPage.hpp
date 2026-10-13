#pragma once
#include "../Protocol/PageParams.hpp"
#include <streambuf>
#include <vector>
#include <cstddef>
#include <ios>

namespace Capt::Utility {
    class BufferedPage : public std::streambuf {
    private:
        std::vector<char_type> buffer;
        std::streambuf* videoStream = nullptr;
        std::size_t blockSize = 4096;

        int_type underflow() override;
        pos_type seekpos(pos_type pos, std::ios_base::openmode which) override;
        pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override;
    public:
        unsigned PageNumber = 0;
        PageParams Params;

        BufferedPage() = default;
        explicit BufferedPage(unsigned page, const PageParams& params, std::streambuf* stream, std::size_t blockSize = 4096) noexcept;

        BufferedPage(const BufferedPage&) = delete;
        BufferedPage& operator=(const BufferedPage&) = delete;

        BufferedPage(BufferedPage&& other) noexcept;
        BufferedPage& operator=(BufferedPage&& other) noexcept;
    };
}
