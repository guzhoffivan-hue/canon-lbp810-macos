#pragma once
#include <stdexcept>

namespace Capt::Compression {
    class DecoderError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}
