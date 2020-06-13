#include "parameters.hpp"
#include "sisd_lfsr.hpp"

#include <immintrin.h>
#include <benchmark/benchmark.h>

#include <iostream>


int main()
{
    sisd_lfsr<1> lfsr(maximum_cycle_polynom_head[1-1]);

    lfsr.clock();
    lfsr.print_state();

    bitset_lfsr<64> blfsr(maximum_cycle_tap_indicies[0]);
    blfsr.clock();
    blfsr.print_state();
    return 0;
}

