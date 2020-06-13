#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

#include <array>

const std::array<std::uint64_t, 4> maximum_cycle_polynom_head =
    {0xD800000000000000, //64-bit: 64, 63, 61, 60
        0xE100000000000000, //128-bit:128, 127, 126, 121
        0xA003000000000000, //192-bit:192, 190, 178, 177
        0xA420000000000000, //256-bit:256, 254, 251, 246
};


#endif // PARAMETERS_HPP
