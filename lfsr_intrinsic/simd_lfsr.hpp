#ifndef SIMD_LFSR_HPP
#define SIMD_LFSR_HPP

#include "parameters.hpp"
#include <immintrin.h>
#include <array>
#include <vector>
#include <iostream>
#include <bitset>

#if defined (__SSE4_1__)
template <std::size_t bit_width>
class simd_lfsr_128
{
public:
    simd_lfsr_128(const std::array<std::uint64_t, bit_width/64>& poly)
        : bit_width_(bit_width),
            width_(bit_width_/128)
    {
        static_assert(bit_width>0 && !(bit_width%128), "Number of bits has to be a multiple of 128!");
        for(auto& x: state_){
            x = _mm_set_epi64x(uniform_random(0, std::numeric_limits<std::uint64_t>::max()),
                              uniform_random(0, std::numeric_limits<std::uint64_t>::max()));
        }
        for(int i = 0; i < width_; ++i){
            polynom_[i] = _mm_set_epi64x(poly[2*i+1], poly[2*i]);
        }
    }

    simd_lfsr_128(const std::vector<std::uint64_t>& poly)
        : bit_width_(bit_width),
        width_(bit_width_/128)
    {
        static_assert(bit_width>0 && !(bit_width%128), "Number of bits has to be a multiple of 128!");
        if(poly.size()<width_*2)
            throw std::runtime_error("Vector with coefficients is too small!");

        for(auto& x: state_){
            x = _mm_set_epi64x(uniform_random(0, std::numeric_limits<std::uint64_t>::max()),
                               uniform_random(0, std::numeric_limits<std::uint64_t>::max()));
        }
        for(int i = 0; i < width_; ++i){
            polynom_[i] = _mm_set_epi64x(poly[2*i+1], poly[2*i]);
        }
    }

    void print_state()
    {
        std::cout << "State\n";
        for (auto i = state_.rbegin(); i != state_.rend(); ++i){
            std::cout << std::bitset<64>(_mm_extract_epi64(*i, 1))
            << '|' << std::bitset<64>(_mm_extract_epi64(*i, 0))
            << std::endl;
        }
    }

    void print_polynom()
    {
        std::cout << "State\n";
        for (auto i = polynom_.rbegin(); i != polynom_.rend(); ++i){
            std::cout << std::bitset<64>(_mm_extract_epi64(*i, 1))
                      << '|' << std::bitset<64>(_mm_extract_epi64(*i, 0))
                      << std::endl;
        }
    }

    bool clock()
    {
        bool output_bit = _mm_extract_epi8(state_[0], 0) & 0x1;
        bool ob = output_bit;
        bool carrier_bit = output_bit;

        __m128i least_bits;
        __m128i mask = _mm_set_epi64x(0x1, 0x1);
        for(int i = width_-1; i >= 0; --i){
            if(ob){
                state_[i] = _mm_xor_si128(state_[i], polynom_[i]);
            }
            least_bits = _mm_and_si128(state_[i], mask);
            output_bit = _mm_extract_epi8(least_bits, 0);
            least_bits = _mm_srli_si128(least_bits, 8);
            least_bits = _mm_insert_epi64(least_bits, carrier_bit, 1);
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
    std::array<__m128i, bit_width/128> polynom_;
};
#endif


#if defined (__AVX2__)
template <std::size_t bit_width>
class simd_lfsr_256
{
public:
    simd_lfsr_256(const std::array<std::uint64_t, bit_width/64>& poly)
        : bit_width_(bit_width),
        width_(bit_width_/256)
    {
        static_assert(bit_width>0 && !(bit_width%256), "Number of bits has to be a multiple of 256!");
        for(auto& x: state_){
            x = _mm256_set_epi64x(uniform_random(0, std::numeric_limits<std::uint64_t>::max()),
                                   uniform_random(0, std::numeric_limits<std::uint64_t>::max()),
                                   uniform_random(0, std::numeric_limits<std::uint64_t>::max()),
                                   uniform_random(0, std::numeric_limits<std::uint64_t>::max()));
        }
        for(int i = 0; i < width_; ++i){
            polynom_[i] = _mm256_set_epi64x(poly[4*i+3], poly[4*i+2],
                                         poly[4*i+1], poly[4*i]);
        }
    }

    simd_lfsr_256(const std::vector<std::uint64_t>& poly)
        : bit_width_(bit_width),
        width_(bit_width_/256)
    {
        static_assert(bit_width>0 && !(bit_width%256), "Number of bits has to be a multiple of 256!");
        if(poly.size()<width_*4)
            throw std::runtime_error("Vector with coefficients is too small!");
        for(auto& x: state_){
            x = _mm256_set_epi64x(uniform_random(0, std::numeric_limits<std::uint64_t>::max()),
                                  uniform_random(0, std::numeric_limits<std::uint64_t>::max()),
                                  uniform_random(0, std::numeric_limits<std::uint64_t>::max()),
                                  uniform_random(0, std::numeric_limits<std::uint64_t>::max()));
        }
        for(int i = 0; i < width_; ++i){
            polynom_[i] = _mm256_set_epi64x(poly[4*i+3], poly[4*i+2],
                                         poly[4*i+1], poly[4*i]);
        }
    }

    void print_state()
    {
        std::cout << "State\n";
        for (auto i = state_.rbegin(); i != state_.rend(); ++i){
            std::cout << std::bitset<64>(_mm256_extract_epi64(*i, 3))
                      << '|' << std::bitset<64>(_mm256_extract_epi64(*i, 2))
                      << '|' << std::bitset<64>(_mm256_extract_epi64(*i, 1))
                      << '|' << std::bitset<64>(_mm256_extract_epi64(*i, 0))
                      << std::endl;
        }
    }

    void print_polynom()
    {
        std::cout << "Polynom\n";
        for (auto i = polynom_.rbegin(); i != polynom_.rend(); ++i){
            std::cout << std::bitset<64>(_mm256_extract_epi64(*i, 3))
                      << '|' << std::bitset<64>(_mm256_extract_epi64(*i, 2))
                      << '|' << std::bitset<64>(_mm256_extract_epi64(*i, 1))
                      << '|' << std::bitset<64>(_mm256_extract_epi64(*i, 0))
                      << std::endl;
        }
    }

    bool clock()
    {
        bool output_bit = _mm256_extract_epi8(state_[0], 0) & 0x1;
        bool ob = output_bit;
        bool carrier_bit = output_bit;
        __m256i least_bits;
        __m256i mask = _mm256_set_epi64x(0x1, 0x1, 0x1, 0x1);
        for(int i = width_-1; i >= 0; --i){
            if(ob){
                state_[i] = _mm256_xor_si256(state_[i], polynom_[i]);
            }
            least_bits = _mm256_and_si256(state_[i], mask);
            output_bit = _mm256_extract_epi8(least_bits, 0);
            least_bits = _mm256_permute4x64_epi64(least_bits, 0b00111001);
            least_bits = _mm256_insert_epi64(least_bits, carrier_bit, 3);
            least_bits = _mm256_slli_epi64(least_bits, 63);
            state_[i] = _mm256_srli_epi64(state_[i], 1);
            state_[i] = _mm256_or_si256(state_[i], least_bits);
            carrier_bit = output_bit;
        }

        return ob;
    }

private:
    std::size_t bit_width_;
    std::size_t width_;
    std::array<__m256i, bit_width/256> state_;
    std::array<__m256i, bit_width/256> polynom_;
};
#endif


#endif // SIMD_LFSR_HPP
