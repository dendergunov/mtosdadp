#include "parameters.hpp"
#include "sisd_lfsr.hpp"

#include <immintrin.h>
#include <benchmark/benchmark.h>

#include <iostream>

template<std::size_t width>
static void BM_SISD(benchmark::State& state)
{
    sisd_lfsr<width> lfsr(maximum_cycle_polynom_head[width-1]);
//    std::cout << "sisd:\n";
    for(auto _ : state){
        lfsr.clock();
    }
}

template<std::size_t bitwidth>
static void BM_BITSET(benchmark::State& state)
{
    bitset_lfsr<bitwidth> lfsr(maximum_cycle_tap_indicies[(bitwidth/64)-1]);
//    std::cout << "bitset:\n";
    for(auto _ : state){
        lfsr.clock();
    }
}

BENCHMARK_TEMPLATE(BM_SISD, 1);
BENCHMARK_TEMPLATE(BM_SISD, 2);
BENCHMARK_TEMPLATE(BM_SISD, 3);
BENCHMARK_TEMPLATE(BM_SISD, 4);
BENCHMARK_TEMPLATE(BM_BITSET, 64);
BENCHMARK_TEMPLATE(BM_BITSET, 128);
BENCHMARK_TEMPLATE(BM_BITSET, 192);
BENCHMARK_TEMPLATE(BM_BITSET, 256);

BENCHMARK_MAIN();

//int main()
//{


//    lfsr.clock();
//    lfsr.print_state();

//    bitset_lfsr<64> blfsr(maximum_cycle_tap_indicies[0]);
//    blfsr.clock();
//    blfsr.print_state();
//    return 0;
//}

