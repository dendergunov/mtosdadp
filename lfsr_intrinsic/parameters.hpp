#ifndef PARAMETERS_HPP
#define PARAMETERS_HPP

#include <array>
#include <random>

std::uint64_t uniform_random(std::uint64_t min, std::uint64_t max)
{
    static std::mt19937 prng(std::random_device{}());
    return std::uniform_int_distribution<std::uint64_t>(min, max)(prng);
}

constexpr std::array<std::uint64_t, 8> maximum_cycle_polynom_head =
    {0xD800000000000000, //64-bit: 64, 63, 61, 60
     0xE100000000000000, //128-bit:128, 127, 126, 121
     0xA003000000000000, //192-bit:192, 190, 178, 177
     0xA420000000000000, //256-bit:256, 254, 251, 246
     0xD800000000000000,
     0x8201800000000000,
     0x8A10000000000000,
     0xA480000000000000
};

constexpr std::array<std::uint64_t, 1> poly64 =
    {0xD800000000000000};
constexpr std::array<std::uint64_t, 2> poly128 =
    {0xD800000000000000, 0xD80010300006000};
constexpr std::array<std::uint64_t, 3> poly192 =
    {0xD800000000000000, 0xD80010300006000, 0xD526781452412642};
constexpr std::array<std::uint64_t, 4> poly256 =
    {0xD800000000000000, 0xD80010300006000, 0xD526781452412642, 0x492352638268ABCD};
constexpr std::array<std::uint64_t, 5> poly320 =
    {0xD800000000000000, 0xD80010300006000, 0xD526781452412642, 0x492352638268ABCD, 0xFE};
constexpr std::array<std::uint64_t, 6> poly384 =
    {0xD800000000000000, 0xD80010300006000, 0xD526781452412642, 0x492352638268ABCD, 0xFE, 0x12};
constexpr std::array<std::uint64_t, 7> poly448 =
    {0xD800000000000000, 0xD80010300006000, 0xD526781452412642, 0x492352638268ABCD, 0xFE, 0x12, 0xABCDEF9274236};
constexpr std::array<std::uint64_t, 8> poly512 =
    {0xD800000000000000, 0xD80010300006000, 0xD526781452412642, 0x492352638268ABCD, 0xFE, 0x12, 0xABCDEF9274236, 0xCEC};


constexpr std::array<std::array<std::size_t, 4>, 8> maximum_cycle_tap_indicies
{{ {63, 62, 60, 59},
    {127, 126, 125, 120},
    {191, 189, 177, 176},
    {255, 253, 250, 245},
    {319, 318, 316, 315},
    {383, 377, 368, 367},
    {447, 443, 441, 436},
    {511, 509, 506, 503}
}};


#endif // PARAMETERS_HPP
