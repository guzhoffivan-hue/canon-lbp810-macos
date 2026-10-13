#include "StreamPacket.hpp"
#include <cassert>
#include <utility>

namespace Capt {
    StreamPacket::StreamPacket(std::istream& stream, PacketHeader header) noexcept
        : stream(&stream), remain(header.PayloadSize), Header(header) {}

    StreamPacket::~StreamPacket() {
        this->Discard();
    }

    StreamPacket::StreamPacket(StreamPacket&& other) noexcept
        : stream(other.stream), remain(other.remain), Header(std::move(other.Header)) {
        other.stream = nullptr;
        other.remain = 0;
    }

    StreamPacket& StreamPacket::operator=(StreamPacket&& other) noexcept {
        this->stream = other.stream;
        this->remain = other.remain;
        this->Header = std::move(other.Header);
        other.stream = nullptr;
        other.remain = 0;
        return *this;
    }

    void StreamPacket::ReadBytes(std::span<uint8_t> dest) {
        assert(this->stream != nullptr);
        if (dest.size() > this->remain) {
            throw std::out_of_range("packet payload EOF");
        }
        this->stream->read(reinterpret_cast<char*>(dest.data()), dest.size());
        this->remain -= dest.size();
    }

    void StreamPacket::Discard() {
        if (this->remain == 0) {
            return;
        }
        if (this->stream != nullptr && this->stream->good()) {
            // std::istream::ignore() calls unnecessary underflow
            for (std::size_t i = 0; i < this->remain; i++) {
                this->stream->rdbuf()->sbumpc();
            }
        }
        this->remain = 0;
    }

    std::istream& operator>>(std::istream& stream, StreamPacket& reader) {
        reader.Discard();
        stream >> reader.Header;
        reader.remain = reader.Header.PayloadSize;
        reader.stream = &stream;
        return stream;
    }
}
