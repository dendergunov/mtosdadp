#include "parameters.hpp"
#include "sisd_lfsr.hpp"

#include <immintrin.h>
#include <benchmark/benchmark.h>

#include <iostream>


int main()
{
    sisd_lfsr<1> lfsr(maximum_cycle_polynom_head[3-1]);

    lfsr.clock();
    lfsr.print_state();

    return 0;
}

