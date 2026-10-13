#pragma once
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <span>
#include <utility>
#include <array>
#include <ostream>
#include "PacketHeader.hpp"

namespace Capt {
    template<std::size_t Size = 0>
    struct PacketBuilder {
        static_assert((Size + 4) <= UINT16_MAX, "packet size overflow");

        std::array<uint8_t, Size> Payload;

        [[nodiscard]] constexpr auto AppendByte(uint8_t value) noexcept {
            PacketBuilder<Size + 1> res;
            std::move(this->Payload.begin(), this->Payload.end(), res.Payload.begin());
            res.Payload[Size] = value;
            return res;
        }

        [[nodiscard]] constexpr auto AppendUint16(uint16_t value) noexcept {
            PacketBuilder<Size + 2> res;
            std::move(this->Payload.begin(), this->Payload.end(), res.Payload.begin());
            res.Payload[Size] = value & 0xff;
            res.Payload[Size + 1] = (value >> 8) & 0xff;
            return res;
        }

        [[nodiscard]] constexpr auto AppendUint32(uint32_t value) noexcept {
            PacketBuilder<Size + 4> res;
            std::move(this->Payload.begin(), this->Payload.end(), res.Payload.begin());
            res.Payload[Size] = value & 0xff;
            res.Payload[Size + 1] = (value >> 8) & 0xff;
            res.Payload[Size + 2] = (value >> 16) & 0xff;
            res.Payload[Size + 3] = (value >> 24) & 0xff;
            return res;
        }

        template<std::size_t DataSize>
        [[nodiscard]] constexpr auto AppendBytes(std::span<const uint8_t, DataSize> data) noexcept {
            PacketBuilder<Size + data.size()> res;
            std::move(this->Payload.begin(), this->Payload.end(), res.Payload.begin());
            std::copy_n(data.begin(), data.size(), res.Payload.begin() + Size);
            return res;
        }

        template<std::size_t DataSize>
        [[nodiscard]] constexpr auto AppendBytes(const std::array<uint8_t, DataSize>& data) noexcept {
            return this->AppendBytes(std::span<const uint8_t, DataSize>(data));
        }

        inline std::ostream& WriteTo(std::ostream& stream, uint16_t opcode) const {
            return PacketHeader::WriteTo(stream, opcode, this->Payload);
        }
    };
}
