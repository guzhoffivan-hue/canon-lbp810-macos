#pragma once
#include <stdexcept>

namespace Capt {
    class ProtocolError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
}
