#pragma once
#include <streambuf>
#include <iostream>
#include <vector>
#include <cstdint>

class MemoryStreambuf : public std::streambuf {
private:
    int_type overflow(int_type c = traits_type::eof()) override {
        if (!traits_type::eq_int_type(c, traits_type::eof())){
            this->Buffer.push_back(traits_type::to_char_type(c));
        }
        return traits_type::not_eof(c);
    }

    int_type underflow() override {
        if (this->Buffer.empty()) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(Buffer[0]);
    }

    int_type uflow() override {
        int_type val = this->underflow();
        if (!traits_type::eq_int_type(val, traits_type::eof())) {
            this->Buffer.erase(this->Buffer.begin());
        }
        return val;
    }
public:
    std::vector<uint8_t> Buffer;

    MemoryStreambuf() = default;
};

class MemoryStream : public std::iostream {
private:
    MemoryStreambuf buf;
public:
    MemoryStream() : std::iostream(&buf) {}
    explicit MemoryStream(const std::vector<uint8_t>& buffer) : std::iostream(&buf) {
        this->buf.Buffer = buffer;
    }

    std::vector<uint8_t>& Buffer() noexcept {
        return this->buf.Buffer;
    }
};
