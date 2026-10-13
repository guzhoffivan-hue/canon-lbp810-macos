#pragma once
#include <cstdint>
#include <span>
#include <cassert>

namespace Capt::Compression {
    enum class ScoaFuncType {
        Copy = 0,
        Repeat,
        Raw,
    };

    struct ScoaFunc {
        ScoaFuncType Type;
        std::size_t Index;
        std::size_t Count;

        constexpr std::span<const uint8_t> Payload(std::span<const uint8_t> line) const noexcept {
            assert(this->Index < line.size());
            assert(this->Index + this->Count <= line.size());
            return {line.data() + this->Index, this->Count};
        }

        constexpr bool operator==(const ScoaFunc& other) const noexcept {
            return this->Type == other.Type && this->Index == other.Index && this->Count == other.Count;
        }

        constexpr bool operator!=(const ScoaFunc& other) const noexcept {
            return !(*this == other);
        }
    };
}
