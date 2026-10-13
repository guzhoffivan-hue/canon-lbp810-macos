#pragma once
#include "Internal/Integer.hpp"
#include "PacketHeader.hpp"
#include <cassert>
#include <concepts>
#include <istream>
#include <cstdint>
#include <cstddef>
#include <span>
#include <stdexcept>

namespace Capt {
    class StreamPacket {
    private:
        std::istream* stream = nullptr;
        std::size_t remain = 0;
    public:
        PacketHeader Header;

        StreamPacket() noexcept = default;
        explicit StreamPacket(std::istream& stream, PacketHeader header) noexcept;
        ~StreamPacket();

        StreamPacket(const StreamPacket&) = delete;
        StreamPacket& operator=(const StreamPacket&) = delete;

        StreamPacket(StreamPacket&& other) noexcept;
        StreamPacket& operator=(StreamPacket&& other) noexcept;

        [[nodiscard]] inline std::size_t Remain() const noexcept {
            return this->remain;
        }

        template<std::integral T>
        T ReadInt() {
            assert(this->stream != nullptr);
            if (this->remain < sizeof(T)) {
                throw std::out_of_range("packet payload EOF");
            }
            this->remain -= sizeof(T);
            return Internal::ReadLittle<T>(*this->stream);
        }

        inline uint8_t ReadByte() { return this->ReadInt<uint8_t>(); }
        inline uint16_t ReadUint16() { return this->ReadInt<uint16_t>(); }
        inline uint32_t ReadUint32() { return this->ReadInt<uint32_t>(); }
        void ReadBytes(std::span<uint8_t> dest);

        void Discard();

        friend std::istream& operator>>(std::istream& stream, StreamPacket& reader);
    };
};
