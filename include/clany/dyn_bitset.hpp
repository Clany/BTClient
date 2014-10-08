/////////////////////////////////////////////////////////////////////////////////
// The MIT License(MIT)
// 
// Copyright (c) 2014 Tiangang Song
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
/////////////////////////////////////////////////////////////////////////////////

#ifndef DYN_BITSET_HPP
#define DYN_BITSET_HPP

#include <vector>
#include <bitset>
#include <algorithm>
#include <numeric>
#include "byte_array.hpp"

_CLANY_BEGIN
class DynBitset {
protected:
    static const size_t BYTESIZE = 8;
    using Byte = bitset<BYTESIZE>;

public:
    DynBitset() = default;
    DynBitset(size_t n)
        : bit_field((n + BYTESIZE - 1) / BYTESIZE),
          bit_size(n),
          offset(BYTESIZE*bit_field.size() - n), 
          last_size(n - BYTESIZE*(bit_field.size() - 1)) {}
    DynBitset(size_t n, const string& val)
        : DynBitset(n) {
        size_t idx = 0;
        auto byte = bit_field.begin();
        do {
            *byte++ = Byte(val.substr(idx, BYTESIZE));
        } while ((idx += BYTESIZE) < val.length());
        bit_field.back() <<= BYTESIZE - last_size;
    }

    DynBitset(size_t n, const ByteArray& data)
        : DynBitset(n) {
        for (auto idx = 0u; idx < bit_field.size(); ++idx) {
            bit_field[idx] = static_cast<ullong>(data[idx]);
        }
    }

    // Conversion to byte array and string
    auto toByteArray() const -> ByteArray {
        ByteArray data(bit_field.size());
        for (auto i = 0u; i < bit_field.size(); ++i) {
            data[i] = static_cast<char>(bit_field[i].to_ulong());
        }
        return data;
    }

    string to_string() const {
        string str;
        for (const auto& byte : bit_field) {
            str += byte.to_string();
        }
        return str.substr(0, bit_size);
    }

    // Bit access
    auto operator[](size_t idx) -> Byte::reference {
        idx += offset;
        auto vec_idx = bit_field.size() - 1 - idx / BYTESIZE;
        return bit_field[vec_idx][idx % BYTESIZE];
    }

    bool operator[](size_t idx) const {
        idx += offset;
        auto vec_idx = bit_field.size() - 1 - idx / BYTESIZE;
        return bit_field[vec_idx][idx % BYTESIZE];
    }

    size_t count() const {
        return accumulate(bit_field.begin(), bit_field.end(), 0,
                          [](int init, const Byte& byte) {
            return init + byte.count();
        });
    }

    size_t size() const { return bit_size; }

    bool test(size_t idx) const {
        if (idx >= bit_size) throw range_error("bitset subscript out of range");
        return (*this)[idx];
    }

    bool any() const {
        return any_of(bit_field.begin(), bit_field.end(), 
                      [](const Byte& byte) {
            return byte.any();
        });
    }

    bool none() const {
        return !any();
    }

    bool all() const {
        bool all_set =  all_of(bit_field.begin(), bit_field.end()-1,
                      [](const Byte& byte) {
            return byte.all();
        });
        return all_set && bit_field.back().count() == last_size;
    }

    // Bit operation
    DynBitset& set() {
        for_each(bit_field.begin(), bit_field.end() - 1, [](Byte& byte) {
            byte.set();
        });
        bit_field.back() = Byte(string(last_size, '1'));
        bit_field.back() <<= BYTESIZE - bit_field.back().count();
        return (*this);
    }

    DynBitset& reset() {
        for_each(bit_field.begin(), bit_field.end(), [](Byte& byte) {
            byte.reset();
        });
        return (*this);
    }

//     void resize(size_t n) {
//         bit_field.resize((n + BYTESIZE - 1) / BYTESIZE);
//         bit_size = n;
//         offset = BYTESIZE*bit_field.size() - n;
//         last_size = n - BYTESIZE*(bit_field.size() - 1);
//         bit_field.back() = Byte(bit_field.back().to_string().substr(0, last_size));
//         bit_field.back() <<= BYTESIZE - last_size;
//     }

protected:
    vector<Byte> bit_field;
    size_t bit_size;
    size_t offset;
    size_t last_size;
};

// This class access the bit from left to right
struct BitField : DynBitset {
#if __cplusplus >= 201103L
    using DynBitset::DynBitset;
#else
    BitField() = default;
    BitField(size_t n) : DynBitset(n) {}
    BitField(size_t n, const string& val) : DynBitset(n, val) {}
    BitField(size_t n, const ByteArray& data) : DynBitset(n, data) {}
#endif

    auto operator[](size_t idx) -> Byte::reference {
        auto vec_idx = idx / BYTESIZE;
        return bit_field[vec_idx][BYTESIZE - 1 - idx % BYTESIZE];
    }

    bool operator[](size_t idx) const {
        auto vec_idx = idx / BYTESIZE;
        return bit_field[vec_idx][BYTESIZE - 1 - idx % BYTESIZE];
    }

    void resize(size_t n) {
        bit_field.resize((n + BYTESIZE - 1) / BYTESIZE);
        bit_size  = n;
        offset    = BYTESIZE*bit_field.size() - n;
        last_size = n - BYTESIZE*(bit_field.size() - 1);
        bit_field.back() = Byte(bit_field.back().to_string().substr(0, last_size));
        bit_field.back() <<= BYTESIZE - last_size;
    }
};

inline ostream& operator<<(ostream& os, const DynBitset& data)
{
    os << data.to_string();
    return os;
}

inline bool operator==(const DynBitset& left, const DynBitset& right)
{
    return left.to_string() == right.to_string();
}

inline bool operator!=(const DynBitset& left, const DynBitset& right)
{
    return !(left == right);
}
_CLANY_END

#endif // DYN_BITSET_HPP
