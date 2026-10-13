#include "ScoaStreambuf.hpp"
#include "ScoaState.hpp"
#include "ScoaCmd.hpp"
#include "ScoaFunc.hpp"
#include <cassert>
#include <iostream>
#include <span>
#include <algorithm>

namespace Capt::Compression {
    using int_type = ScoaStreambuf::int_type;

    ScoaStreambuf::ScoaStreambuf(std::size_t bufferReserve) {
        this->buffer.reserve(bufferReserve);
    }

    ScoaStreambuf::ScoaStreambuf(std::streambuf& rasterStream, unsigned lineSize, unsigned lines, std::size_t bufferReserve)
        : rasterStream(&rasterStream), currLine(lineSize), linesRemain(lines) {
        this->buffer.reserve(bufferReserve);
    }

    void ScoaStreambuf::Reset(std::streambuf& rasterStream, unsigned lineSize, unsigned lines) {
        this->rasterStream = &rasterStream;
        this->buffer.clear();
        this->linesRemain = lines;
        this->currLine.resize(lineSize);
        this->prevLine.resize(lineSize);
        this->videoSize = 0;
    }

    inline std::size_t cmd_Copy(std::vector<uint8_t>& buffer, unsigned copyCount) {
        assert(copyCount != 0);
        std::size_t vsize = 0;
        while (copyCount >= 8) {
            unsigned copy = std::min(copyCount, 248u);
            copy = copy - (copy % 8);
            vsize += ScoaCmd::CopyLong(buffer, copy);
            copyCount -= copy;
        }
        if (copyCount != 0) {
            vsize += ScoaCmd::CopyShort(buffer, copyCount);
        }
        return vsize;
    }

    inline std::size_t cmd_CopyThenRaw(std::vector<uint8_t>& buffer, unsigned copyCount, std::span<const uint8_t> rawData) {
        assert(rawData.size() != 0);
        std::size_t vsize = 0;
        if (copyCount <= 7) {
            while (rawData.size() >= 8) {
                std::size_t count = std::min(rawData.size(), 255uz);
                vsize += ScoaCmd::CopyThenRawLong(buffer, copyCount, rawData.subspan(0, count));
                copyCount = 0;
                rawData = rawData.subspan(count);
            }
            if (copyCount != 0 || rawData.size() != 0) {
                vsize += ScoaCmd::CopyThenRaw(buffer, copyCount, rawData);
            }
        } else {
            while (copyCount >= 8) {
                unsigned copy = std::min(copyCount, 248u);
                copy = copy - (copy % 8);
                vsize += ScoaCmd::CopyLong(buffer, copy);
                copyCount -= copy;
            }
            return vsize + cmd_CopyThenRaw(buffer, copyCount, rawData);
        }
        return vsize;
    }

    inline std::size_t cmd_WriteRaw(std::vector<uint8_t>& buffer, std::span<const uint8_t> rawData) {
        assert(rawData.size() != 0);
        std::size_t vsize = 0;
        if (rawData.size() <= 7) {
            vsize += ScoaCmd::CopyThenRaw(buffer, 0, rawData);
        } else {
            while (rawData.size() >= 8) {
                std::size_t count = std::min(rawData.size(), 255uz);
                vsize += ScoaCmd::CopyThenRawLong(buffer, 0, rawData.subspan(0, count));
                rawData = rawData.subspan(count);
            }
            if (rawData.size() != 0) {
                vsize += ScoaCmd::CopyThenRaw(buffer, 0, rawData);
            }
        }
        return vsize;
    }

    inline std::size_t cmd_CopyThenRepeat(std::vector<uint8_t>& buffer, unsigned copyCount, unsigned repeatCount, uint8_t repeatByte) {
        assert(copyCount != 0 || repeatCount != 0);
        std::size_t vsize = 0;
        if (copyCount <= 7) {
            while (repeatCount >= 8) {
                unsigned rep = std::min(repeatCount, 255u);
                vsize += ScoaCmd::CopyThenRepeatLong(buffer, copyCount, rep, repeatByte);
                copyCount = 0;
                repeatCount -= rep;
            }
            if (repeatCount >= 2) {
                vsize += ScoaCmd::CopyThenRepeat(buffer, copyCount, repeatCount, repeatByte);
            } else if (repeatCount == 1) {
                vsize += ScoaCmd::CopyThenRaw(buffer, copyCount, {&repeatByte, 1});
            } else if (copyCount != 0) {
                vsize += ScoaCmd::CopyShort(buffer, copyCount);
            }
        } else {
            while (copyCount >= 8) {
                unsigned copy = std::min(copyCount, 248u);
                copy = copy - (copy % 8);
                vsize += ScoaCmd::CopyLong(buffer, copy);
                copyCount -= copy;
            }
            return vsize + cmd_CopyThenRepeat(buffer, copyCount, repeatCount, repeatByte);
        }
        return vsize;
    }

    inline std::size_t cmd_RepeatX(std::vector<uint8_t>& buffer, unsigned repeatCount) {
        assert(repeatCount != 0);
        std::size_t vsize = 0;
        while (repeatCount >= 8) {
            unsigned rep = std::min(repeatCount, 255u);
            vsize += ScoaCmd::RepeatXLong(buffer, rep);
            repeatCount -= rep;
        }
        if (repeatCount != 0) {
            vsize += ScoaCmd::RepeatX(buffer, repeatCount);
        }
        return vsize;
    }

    inline std::size_t cmd_Repeat(std::vector<uint8_t>& buffer, unsigned repeatCount, uint8_t repeatByte) {
        assert(repeatCount != 0);
        if (repeatByte == ScoaCmd::RepeatXByte) {
            return cmd_RepeatX(buffer, repeatCount);
        }
        return cmd_CopyThenRepeat(buffer, 0, repeatCount, repeatByte);
    }

    inline std::size_t cmd_RepeatThenRaw(std::vector<uint8_t>& buffer, unsigned repeatCount, uint8_t repeatByte, std::span<const uint8_t> rawData) {
        assert(repeatCount != 0 || rawData.size() != 0);
        std::size_t vsize = 0;
        if (repeatCount <= 7) {
            if (rawData.size() <= 7) {
                if (rawData.size() == 0) {
                    vsize += cmd_CopyThenRepeat(buffer, 0, repeatCount, repeatByte);
                } else if (repeatCount == 0) {
                    vsize += ScoaCmd::CopyThenRaw(buffer, 0, rawData);
                } else {
                    if (repeatCount >= 2) {
                        vsize += ScoaCmd::RepeatThenRaw(buffer, repeatCount, repeatByte, rawData);
                    } else {
                        vsize += cmd_CopyThenRepeat(buffer, 0, repeatCount, repeatByte);
                        vsize += ScoaCmd::CopyThenRaw(buffer, 0, rawData);
                    }
                }
            } else {
                if (repeatCount >= 2) {
                    std::size_t count = std::min(rawData.size(), 255uz);
                    vsize += ScoaCmd::RepeatThenRawLong(buffer, repeatCount, repeatByte, rawData.subspan(0, count));
                    rawData = rawData.subspan(count);
                } else if (repeatCount != 0) {
                    vsize += cmd_CopyThenRepeat(buffer, 0, repeatCount, repeatByte);
                }
                while (rawData.size() >= 8) {
                    std::size_t count = std::min(rawData.size(), 255uz);
                    vsize += ScoaCmd::CopyThenRawLong(buffer, 0, rawData.subspan(0, count));
                    rawData = rawData.subspan(count);
                }
                if (rawData.size() != 0) {
                    vsize += ScoaCmd::CopyThenRaw(buffer, 0, rawData);
                }
            }
        } else {
            while (repeatCount >= 8) {
                unsigned count = std::min(repeatCount, 255u);
                vsize += ScoaCmd::CopyThenRepeatLong(buffer, 0, count, repeatByte);
                repeatCount -= count;
            }
            return vsize + cmd_RepeatThenRaw(buffer, repeatCount, repeatByte, rawData);
        }
        return vsize;
    }

    inline std::size_t nextCmd(std::vector<uint8_t>& buffer, std::span<const uint8_t> line, ScoaState& state) {
        assert(!state.Empty());
        ScoaFunc curr = state.Get();
        if (curr.Type == ScoaFuncType::Copy) {
            if ((curr.Index + curr.Count) == line.size()) {
                return ScoaCmd::EOL(buffer);
            }
            if (!state.Empty()) {
                ScoaFunc next = state.Get();
                if (next.Type == ScoaFuncType::Repeat) {
                    assert(next.Count >= 2);
                    return cmd_CopyThenRepeat(buffer, curr.Count, next.Count, line[next.Index]);
                } else {
                    assert(next.Type == ScoaFuncType::Raw);
                    return cmd_CopyThenRaw(buffer, curr.Count, next.Payload(line));
                }
            }
            return cmd_Copy(buffer, curr.Count);
        } else if (curr.Type == ScoaFuncType::Repeat) {
            assert(curr.Count >= 2);
            if (!state.Empty() && state.Peek().Type == ScoaFuncType::Raw) {
                ScoaFunc next = state.Get();
                return cmd_RepeatThenRaw(buffer, curr.Count, line[curr.Index], next.Payload(line));
            }
            return cmd_Repeat(buffer, curr.Count, line[curr.Index]);
        }
        return cmd_WriteRaw(buffer, curr.Payload(line));
    }

    int_type ScoaStreambuf::underflow() {
        assert(this->rasterStream != nullptr);
        if (this->gptr() < this->egptr()) {
            return traits_type::to_int_type(*this->gptr());
        }
        if (this->linesRemain == 0) {
            return traits_type::eof();
        }

        std::streamsize read = this->rasterStream->sgetn(reinterpret_cast<char*>(this->currLine.data()), this->currLine.size());
        if (read < static_cast<std::streamsize>(this->currLine.size())) {
            return traits_type::eof();
        }

        this->buffer.clear();

        ScoaState state(this->currLine, this->videoSize == 0 ? std::span<const uint8_t>() : this->prevLine);
        while (!state.Empty()) {
            this->videoSize += nextCmd(this->buffer, this->currLine, state);
        }

        this->prevLine.resize(this->currLine.size());
        this->prevLine.swap(this->currLine);

        this->linesRemain--;
        if (this->linesRemain == 0) {
            if ((this->videoSize + 1) % 2 != 0) {
                ScoaCmd::NOP(this->buffer);
            }
            ScoaCmd::EOP(this->buffer);
        }

        char_type* start = reinterpret_cast<char_type*>(this->buffer.data());
        char_type* end = start + this->buffer.size();
        this->setg(start, start, end);
        return traits_type::to_int_type(*this->gptr());
    }
}
