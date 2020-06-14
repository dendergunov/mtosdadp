#ifndef SIMD_LFSR_HPP
#define SIMD_LFSR_HPP

#include "parameters.hpp"
#include <immintrin.h>
#include <array>
#include <iostream>
#include <bitset>

#if defined (__AVX__)
template <std::size_t bit_width>
class simd_lfsr_128
{
public:
    simd_lfsr_128(std::uint64_t polynom_head)
        : bit_width_(bit_width),
            width_(bit_width_/128),
        polynom_head_(polynom_head)
    {
        static_assert(bit_width>0 && !(bit_width%128), "Number of bits has to be a multiple of 128!");
        for(auto& x: state_){
            x[0] = uniform_random(0, std::numeric_limits<std::uint64_t>::max());
            x[1] = uniform_random(0, std::numeric_limits<std::uint64_t>::max());
        }
    }

    void print_state()
    {
        std::cout << "State\n";
        for (auto i = state_.rbegin(); i != state_.rend(); ++i){
            std::cout << std::bitset<64>((*i)[1]) << std::bitset<64>((*i)[0]) << std::endl;
        }
    }

    bool clock()
    {
        bool output_bit = state_[0][0]&0x1ull;
        bool ob = output_bit;
        bool carrier_bit = output_bit;
        if(output_bit){
            state_[width_-1][1] ^= polynom_head_;
        }
        __m128i least_bits;
        constexpr __m128i mask = {0x1, 0x1};
        for(int i = width_-1; i >= 0; --i){
            least_bits = _mm_and_si128(state_[i], mask);
            output_bit = least_bits[0];
            least_bits = _mm_srli_si128(least_bits, 7);
            least_bits[1] = carrier_bit;
            least_bits = _mm_slli_epi64(least_bits, 63);
            state_[i] = _mm_srli_epi64(state_[i], 1);
            state_[i] = _mm_or_si128(state_[i], least_bits);
            carrier_bit = output_bit;
        }

        return ob;
    }

private:
    std::size_t bit_width_;
    std::size_t width_;
    std::array<__m128i, bit_width/128> state_;
    std::uint64_t polynom_head_;
};
#endif


#endif // SIMD_LFSR_HPP
