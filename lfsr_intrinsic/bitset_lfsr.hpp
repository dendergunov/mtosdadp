#ifndef BITSET_LFSR_HPP
#define BITSET_LFSR_HPP

#include "parameters.hpp"
#include <iostream>

template<std::size_t bit_width>
class bitset_lfsr
{
public:
    bitset_lfsr(std::array<std::size_t, 4> tap_indicies)
        : bit_width_(bit_width),
        width_(bit_width / 64)
    {
        static_assert(bit_width>0 && !(bit_width%64), "Number of bits has to be a multiple of 64!");
        for(auto x: tap_indicies)
            polynom_.set(x);
        for(int i = 0; i < bit_width_; ++i)
            state_.set(i, uniform_random(0, 1));
    }

    void print_state(){
        std::cout << "State:\n";
        std::cout << state_ << std::endl;
    }

    bool clock(){
        bool to_tap = state_[0];
        if(to_tap)
            state_ ^= polynom_;
        state_ = state_ >> 1;
        state_[bit_width_-1] = to_tap;
        return to_tap;
    }

private:
    std::size_t bit_width_;
    std::size_t width_;
    std::bitset<bit_width> polynom_;
    std::bitset<bit_width> state_;
};

#endif // BITSET_LFSR_HPP
