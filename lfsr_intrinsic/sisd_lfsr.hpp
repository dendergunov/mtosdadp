#ifndef SISD_LFSR_HPP
#define SISD_LFSR_HPP

#include "parameters.hpp"
#include <array>
#include <iostream>
#include <bitset>

template<std::size_t bit_width>
class sisd_lfsr
{
public:
    sisd_lfsr(std::uint64_t polynom_head)
        : bit_width_(bit_width),
        width_(bit_width/64),
        polynom_{0},
        state_{0}
    {
        static_assert(bit_width>0 && !(bit_width%64), "Number of bits has to be a multiple of 64!");

        polynom_[width_-1] = polynom_head;
        for(auto& x: state_)
            x = uniform_random(0, std::numeric_limits<std::uint64_t>::max());
    };

    sisd_lfsr(const std::array<std::uint64_t, bit_width/64>& poly)
        : bit_width_(bit_width),
        width_(bit_width/64)
    {
        static_assert(bit_width>0 && !(bit_width%64), "Number of bits has to be a multiple of 64!");

        for(int i = width_-1; i>=0; --i)
            polynom_[i]=poly[i];
        for(auto& x: state_)
            x = uniform_random(0, std::numeric_limits<std::uint64_t>::max());
    };

    sisd_lfsr(const std::vector<std::uint64_t>& poly)
        : bit_width_(bit_width),
        width_(bit_width/64)
    {
        static_assert(bit_width>0 && !(bit_width%64), "Number of bits has to be a multiple of 64!");
        if(poly.size()<width_)
            throw std::runtime_error("Vector with coefficients is too small!");

        for(int i = width_-1; i>=0; --i)
            polynom_[i]=poly[i];
        for(auto& x: state_)
            x = uniform_random(0, std::numeric_limits<std::uint64_t>::max());
    };


    void print_state()
    {
        std::cout << "\nState:\n";
        for (auto i = state_.rbegin(); i != state_.rend(); ++i)
            std::cout << std::bitset<64>(*i);
        std::cout << std::endl;
    }

    bool clock()
    {
        bool carrier_bit = 0;
        bool to_flip = state_[0] & 0x1ull;
        bool least_bit;
        for(int i = width_-1; i >= 0; --i){
            least_bit = state_[i]&0x1ull;
            if(to_flip)
                state_[i] ^= polynom_[i];
            state_[i] >>= 1;
            if(carrier_bit)
                state_[i] |= (1ull << 63);
            carrier_bit = least_bit;
        }
        if(to_flip)
            state_[width_-1] = state_[width_-1] | (1ull << 63);

        return to_flip;
    }
private:
    std::size_t bit_width_;
    std::size_t width_;
    std::array<std::uint64_t, bit_width/64> polynom_;
    std::array<std::uint64_t, bit_width/64> state_;
};




#endif // SISD_LFSR_HPP
