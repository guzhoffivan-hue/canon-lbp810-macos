#include "BufferedPage.hpp"
#include <utility>

namespace Capt::Utility {
    using int_type = BufferedPage::int_type;
    using pos_type = BufferedPage::pos_type;
    using off_type = BufferedPage::off_type;

    int_type BufferedPage::underflow() {
        if (this->gptr() < this->egptr()) {
            return traits_type::to_int_type(*this->gptr());
        }
        if (this->videoStream == nullptr) {
            return traits_type::eof();
        }

        std::size_t oldSize = this->buffer.size();
        this->buffer.resize(oldSize + blockSize);

        std::streamsize read = this->videoStream->sgetn(this->buffer.data() + oldSize, blockSize);
        this->buffer.resize(oldSize + read);

        char_type* start = this->buffer.data();
        char_type* end = start + this->buffer.size();
        this->setg(start, start + oldSize, end);
        return read == 0 ? traits_type::eof() : traits_type::to_int_type(*this->gptr());
    }

    pos_type BufferedPage::seekpos(pos_type pos, std::ios_base::openmode which) {
        if ((which & std::ios_base::in) == 0) {
            return -1;
        }
        this->setg(this->eback(), this->eback() + pos, this->egptr());
        return this->gptr() - this->eback();
    }

    pos_type BufferedPage::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) {
        if ((which & std::ios_base::in) == 0) {
            return -1;
        }
        if (dir == std::ios_base::cur) {
            this->gbump(off);
        } else if (dir == std::ios_base::beg) {
            this->setg(this->eback(), this->eback() + off, this->egptr());
        } else if (dir == std::ios_base::end) {
            this->setg(this->eback(), this->egptr() + off, this->egptr());
        }
        return this->gptr() - this->eback();
    }

    BufferedPage::BufferedPage(unsigned page, const PageParams& params, std::streambuf* stream, std::size_t blockSize) noexcept
        : videoStream(stream), blockSize(blockSize), PageNumber(page), Params(params) {}

    BufferedPage::BufferedPage(BufferedPage&& other) noexcept : buffer(std::move(other.buffer)), videoStream(nullptr), PageNumber(other.PageNumber), Params(std::move(other.Params)) {
        char_type* start = this->buffer.data();
        char_type* end = start + this->buffer.size();
        this->setg(start, start, end);
    }

    BufferedPage& BufferedPage::operator=(BufferedPage&& other) noexcept {
        this->buffer = std::move(other.buffer);
        this->PageNumber = other.PageNumber;
        this->Params = std::move(other.Params);

        char_type* start = this->buffer.data();
        char_type* end = start + this->buffer.size();
        this->setg(start, start, end);
        return *this;
    }
}
