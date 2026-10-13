#include "DecoderStreambuf.hpp"
#include "DecoderError.hpp"
#include <cassert>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace Capt::Compression {
    #define CLOG(STR) if (this->commandLog) *this->commandLog << STR
    #define CHECK_BOUNDS(PARAM, MIN, MAX) checkBounds(PARAM, MIN, MAX, #PARAM)
    static inline void checkBounds(int value, int min, int max, std::string_view paramName) {
        if (value < min || value > max) {
            std::ostringstream ss;
            ss << paramName << " must be in range [" << min << ';' << max << "], got " << value;
            throw DecoderError(ss.str());
        }
    }

    static inline uint8_t mustGet(std::streambuf& stream) {
        using traits_type = std::streambuf::traits_type;
        std::streambuf::int_type v = stream.sbumpc();
        if (traits_type::eq_int_type(v, traits_type::eof())) {
            throw DecoderError("unexpected EOF");
        }
        return traits_type::to_char_type(v);
    }

    static inline std::streamsize mustRead(std::streambuf& stream, std::streambuf::char_type* s, std::streamsize n) {
        std::streamsize read = stream.sgetn(s, n);
        if (read != n) {
            throw DecoderError("unexpected EOF");
        }
        return read;
    }

    DecoderStreambuf::DecoderStreambuf(std::streambuf& stream, unsigned lineSize, std::ostream* commandLog) noexcept
        : stream(stream), commandLog(commandLog), lineSize(lineSize), videoSize(0) {}

    void DecoderStreambuf::repeat(unsigned count, uint8_t repeatByte) {
        if (count == 0) {
            return;
        }
        this->buffer.insert(this->buffer.end(), count, repeatByte);
    }

    void DecoderStreambuf::repeatX(unsigned count) {
        this->repeat(count, 0x43);
    }

    void DecoderStreambuf::copy(unsigned count) {
        if (count == 0) {
            return;
        }
        if (this->buffer.size() < this->lineSize) {
            throw DecoderError("insufficient buffer size for copying");
        }
        auto start = this->buffer.end() - this->lineSize;
        this->buffer.insert(this->buffer.end(), start, start + count);
    }

    void DecoderStreambuf::raw(const std::vector<uint8_t>& data) {
        if (data.size() == 0) {
            return;
        }
        this->buffer.insert(this->buffer.end(), data.begin(), data.end());
    }

    void DecoderStreambuf::endLine() {
        unsigned count = this->lineSize - (this->buffer.size() % this->lineSize);
        this->copy(count);
    }

    int DecoderStreambuf::decodeNext() {
        uint8_t cmd = mustGet(this->stream);
        if (cmd == 0x42) { // EOP
            CLOG("EOP") << std::endl;
            return 0;
        } else if (cmd == 0x41) { // EOL
            CLOG("EOL") << std::endl;
            this->endLine();
            return 1;
        } else if (cmd == 0x40) { // NOP
            CLOG("NOP") << std::endl;
            return 1;
        } else if ((cmd & 0xe0) == 0xa0) { // Extend
            uint8_t extendCount = (cmd & 0x1f) << 3;
            CLOG("Extend ") << static_cast<int>(extendCount) << std::endl;
            cmd = mustGet(this->stream);
            if ((cmd & 0xc0) == 0) { // RepeatXLong
                uint8_t repeatCount = extendCount + ((cmd >> 3) & 7);
                CLOG("RepeatXLong ") << static_cast<int>(repeatCount) << std::endl;
                CHECK_BOUNDS(repeatCount, 8, 255);
                this->repeatX(repeatCount);
                return 2;
            } else if ((cmd & 0xc0) == 0x80) { // CopyThenRepeatLong
                uint8_t copyCount = (cmd & 7);
                uint8_t repeatCount = extendCount + ((cmd >> 3) & 7);
                uint8_t repeatByte = mustGet(this->stream);
                CLOG("CopyThenRepeatLong ") << static_cast<int>(copyCount) << ' ' << static_cast<int>(repeatCount) << std::endl;
                CHECK_BOUNDS(copyCount, 0, 7);
                CHECK_BOUNDS(repeatCount, 8, 255);
                this->copy(copyCount);
                this->repeat(repeatCount, repeatByte);
                return 3;
            } else if ((cmd & 0xc0) == 0x40) { // RepeatThenRawLong
                uint8_t rawCount = extendCount + (cmd & 7);
                uint8_t repeatCount = ((cmd >> 3) & 7);
                uint8_t repeatByte = mustGet(this->stream);
                CLOG("RepeatThenRawLong ") << static_cast<int>(repeatCount) << ' ' << static_cast<int>(rawCount) << std::endl;
                std::vector<uint8_t> rawData(rawCount);
                mustRead(this->stream, reinterpret_cast<char*>(rawData.data()), rawCount);
                CHECK_BOUNDS(repeatCount, 2, 7);
                CHECK_BOUNDS(rawCount, 8, 255);
                this->repeat(repeatCount, repeatByte);
                this->raw(rawData);
                return 3 + rawCount;
            } else if ((cmd & 0xc0) == 0xc0) { // CopyThenRawLong
                uint8_t copyCount = (cmd & 7);
                uint8_t rawCount = extendCount + ((cmd >> 3) & 7);
                CLOG("CopyThenRawLong ") << static_cast<int>(copyCount) << ' ' << static_cast<int>(rawCount) << std::endl;
                std::vector<uint8_t> rawData(rawCount);
                mustRead(this->stream, reinterpret_cast<char*>(rawData.data()), rawCount);
                CHECK_BOUNDS(copyCount, 0, 7);
                CHECK_BOUNDS(rawCount, 8, 255);
                this->copy(copyCount);
                this->raw(rawData);
                return 2 + rawCount;
            }
        } else if ((cmd & 0xe0) == 0x80) { // CopyLong
            uint8_t copyCount = (cmd & 0x1f) << 3;
            CLOG("CopyLong ") << static_cast<int>(copyCount) << std::endl;
            CHECK_BOUNDS(copyCount, 8, 248);
            this->copy(copyCount);
            return 1;
        } else if ((cmd & 0xc0) == 0x40) { // CopyThenRepeat
            uint8_t copyCount = (cmd & 7);
            uint8_t repeatCount = ((cmd >> 3) & 7);
            uint8_t repeatByte = mustGet(this->stream);
            CLOG("CopyThenRepeat ") << static_cast<int>(copyCount) << ' ' << static_cast<int>(repeatCount) << std::endl;
            CHECK_BOUNDS(copyCount, 0, 7);
            CHECK_BOUNDS(repeatCount, 2, 7);
            this->copy(copyCount);
            this->repeat(repeatCount, repeatByte);
            return 2;
        } else if ((cmd & 0xc0) == 0) { // CopyThenRaw
            uint8_t copyCount = (cmd & 7);
            uint8_t rawCount = ((cmd >> 3) & 7);
            CLOG("CopyThenRaw ") << static_cast<int>(copyCount) << ' ' << static_cast<int>(rawCount) << std::endl;
            std::vector<uint8_t> rawData(rawCount);
            mustRead(this->stream, reinterpret_cast<char*>(rawData.data()), rawCount);
            CHECK_BOUNDS(copyCount, 0, 7);
            CHECK_BOUNDS(rawCount, 1, 7);
            this->copy(copyCount);
            this->raw(rawData);
            return 1 + rawCount;
        } else if ((cmd & 0xc0) == 0xc0) { // RepeatX | CopyShort | RepeatThenRaw
            uint8_t copyCount = (cmd & 7);
            uint8_t repeatCount = ((cmd >> 3) & 7);
            if (copyCount != 0 && repeatCount == 0) { // CopyShort
                CLOG("CopyShort ") << static_cast<int>(copyCount) << std::endl;
                this->copy(copyCount);
                return 1;
            } else if (copyCount == 0 && repeatCount != 0) { // RepeatX
                CLOG("RepeatX ") << static_cast<int>(repeatCount) << std::endl;
                CHECK_BOUNDS(repeatCount, 1, 7);
                this->repeatX(repeatCount);
                return 1;
            } else if (copyCount != 0 && repeatCount != 0) { // RepeatThenRaw
                uint8_t rawCount = copyCount;
                uint8_t repeatByte = mustGet(this->stream);
                CLOG("RepeatThenRaw ") << static_cast<int>(repeatCount) << ' ' << static_cast<int>(rawCount) << std::endl;
                std::vector<uint8_t> rawData(rawCount);
                mustRead(this->stream, reinterpret_cast<char*>(rawData.data()), rawCount);
                CHECK_BOUNDS(repeatCount, 2, 7);
                CHECK_BOUNDS(rawCount, 1, 7);
                this->repeat(repeatCount, repeatByte);
                this->raw(rawData);
                return 2 + rawCount;
            }
        }

        std::ostringstream ss;
        ss << "unknown command: 0x" << std::hex << std::setfill('0') << std::setw(2) << cmd;
        throw DecoderError(ss.str());
    }

    bool DecoderStreambuf::decodeLine() {
        unsigned start = this->buffer.size();
        while (this->buffer.size() < start + this->lineSize) {
            int c = this->decodeNext();
            this->videoSize += c;
            if (c == 0) {
                if (((this->videoSize + 1) % 2) != 0) {
                    throw DecoderError("video size must be even");
                }
                this->videoSize = 0;
                return false;
            }
        }
        return true;
    }

    DecoderStreambuf::int_type DecoderStreambuf::underflow() {
        if (this->gptr() < this->egptr()) {
            return traits_type::to_int_type(*this->gptr());
        }

        if (!this->decodeLine()) {
            return traits_type::eof();
        }

        assert(this->buffer.size() >= this->lineSize);
        char_type* start = reinterpret_cast<char_type*>(this->buffer.data() + this->buffer.size() - this->lineSize);
        this->setg(start, start, start + this->lineSize);
        return traits_type::to_int_type(*this->gptr());
    }
}
