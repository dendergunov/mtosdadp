#ifndef SISD_LFSR_HPP
#define SISD_LFSR_HPP

#include "parameters.hpp"
#include <array>
#include <iostream>
#include <bitset>

template<std::size_t width>
class sisd_lfsr
{
public:
    sisd_lfsr()
        : bit_width_(width*64),
        polynom_{0},
        state_{0}
    {
        polynom_[width-1] = maximum_cycle_polynom_head[width-1];
        for(auto& x: state_)
            x = uniform_random(0, std::numeric_limits<std::uint64_t>::max());
        std::cout << "Initialized sisd_lfsr!\n";
        std::cout << "Polynom:\n";
        for (auto i = polynom_.rbegin(); i != polynom_.rend(); ++i)
            std::cout << std::bitset<64>(*i);
        std::cout << "\nState:\n";
        for (auto i = state_.rbegin(); i != state_.rend(); ++i)
            std::cout << std::bitset<64>(*i);
        std::cout << std::endl;
    };
private:
    std::size_t bit_width_;
    std::array<std::uint64_t, width> polynom_;
    std::array<std::uint64_t, width> state_;
};


#endif // SISD_LFSR_HPP
