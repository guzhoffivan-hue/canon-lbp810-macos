#pragma once
#include "ScoaFunc.hpp"
#include <cassert>
#include <cstdint>
#include <optional>
#include <span>

namespace Capt::Compression {
    class ScoaState {
    private:
        ScoaFunc last;
        ScoaFunc curr;
        std::span<const uint8_t> line;
        std::span<const uint8_t> prevLine;
        std::size_t index = 0;

        constexpr std::optional<ScoaFunc> NextFunc(std::span<const uint8_t> prevLine, std::span<const uint8_t> line, std::size_t idx, ScoaFunc* last) const {
            if (idx >= 1 && line[idx] == line[idx - 1] && last->Type != ScoaFuncType::Copy) {
                assert(last != nullptr);
                if (last->Type == ScoaFuncType::Repeat) {
                    last->Count++;
                    return std::nullopt;
                } else if (last->Count == 1) {
                    last->Type = ScoaFuncType::Repeat;
                    last->Count = 2;
                    return std::nullopt;
                }
                last->Count--;
                return ScoaFunc{ScoaFuncType::Repeat, idx - 1, 2};
            } else if (!prevLine.empty() && line[idx] == prevLine[idx]) {
                if (last != nullptr && last->Type == ScoaFuncType::Copy) {
                    last->Count++;
                    return std::nullopt;
                }
                return ScoaFunc{ScoaFuncType::Copy, idx, 1};
            } else if (last != nullptr && last->Type == ScoaFuncType::Raw) {
                last->Count++;
                return std::nullopt;
            }
            return ScoaFunc{ScoaFuncType::Raw, idx, 1};
        }
    public:
        constexpr explicit ScoaState(std::span<const uint8_t> line, std::span<const uint8_t> prevLine) noexcept : line(line), prevLine(prevLine) {
            this->Next();
        }

        [[nodiscard]] constexpr ScoaFunc Peek() const noexcept {
            assert(this->index != 0);
            return this->curr;
        }

        constexpr ScoaFunc Get() noexcept {
            ScoaFunc f = this->Peek();
            this->Next();
            return f;
        }

        [[nodiscard]] constexpr bool Empty() const noexcept {
            return this->index > (line.size() + 1);
        }

        constexpr bool Next() noexcept {
            while (this->index < line.size()) {
                std::optional<ScoaFunc> next = this->NextFunc(this->prevLine, line, this->index, this->index == 0 ? nullptr : &this->last);
                this->index++;
                if (next.has_value()) {
                    if (this->index > 1) {
                        this->curr = this->last;
                        this->last = *next;
                        return true;
                    }
                    this->last = *next;
                }
            }
            if (this->index != 0 && this->index <= (line.size() + 1)) {
                this->curr = this->last;
                this->index++;
                return true;
            }
            return false;
        }
    };
}
