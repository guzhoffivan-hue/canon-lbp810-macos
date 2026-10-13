#pragma once
#include <algorithm>
#include <cstddef>

namespace Capt::Utility {
    [[nodiscard]] constexpr std::size_t CropLineSize(std::size_t lineSize) noexcept {
        return lineSize - (lineSize % 4);
    }

    [[nodiscard]] constexpr std::size_t CropLineSize(std::size_t lineSize, std::size_t paperWidth) noexcept {
        return CropLineSize(std::min(lineSize, paperWidth / 8));
    }

    [[nodiscard]] constexpr std::size_t CropLinesCount(std::size_t count) noexcept {
        return count - (count % 32);
    }

    [[nodiscard]] constexpr std::size_t CropLinesCount(std::size_t count, std::size_t paperHeight) noexcept {
        return CropLinesCount(std::min(count, paperHeight));
    }
}
