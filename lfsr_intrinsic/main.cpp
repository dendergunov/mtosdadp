#include "parameters.hpp"
#include "sisd_lfsr.hpp"
#include "simd_lfsr.hpp"

#include <immintrin.h>
#include <benchmark/benchmark.h>

#include <iostream>

template<std::size_t bit_width>
static void BM_SISD(benchmark::State& state)
{
    sisd_lfsr<bit_width> lfsr(maximum_cycle_polynom_head.at(bit_width/64-1));
//    std::cout << "sisd:\n";
    for(auto _ : state){
        lfsr.clock();
    }
}
BENCHMARK_TEMPLATE(BM_SISD, 64);
BENCHMARK_TEMPLATE(BM_SISD, 128);
BENCHMARK_TEMPLATE(BM_SISD, 192);
BENCHMARK_TEMPLATE(BM_SISD, 256);
BENCHMARK_TEMPLATE(BM_SISD, 320);
BENCHMARK_TEMPLATE(BM_SISD, 384);
BENCHMARK_TEMPLATE(BM_SISD, 448);
BENCHMARK_TEMPLATE(BM_SISD, 512);

template<std::size_t bit_width>
static void BM_BITSET(benchmark::State& state)
{
    bitset_lfsr<bit_width> lfsr(maximum_cycle_tap_indicies.at(bit_width/64-1));
//    std::cout << "bitset:\n";
    for(auto _ : state){
        lfsr.clock();
    }
}
BENCHMARK_TEMPLATE(BM_BITSET, 64);
BENCHMARK_TEMPLATE(BM_BITSET, 128);
BENCHMARK_TEMPLATE(BM_BITSET, 192);
BENCHMARK_TEMPLATE(BM_BITSET, 256);
BENCHMARK_TEMPLATE(BM_BITSET, 320);
BENCHMARK_TEMPLATE(BM_BITSET, 384);
BENCHMARK_TEMPLATE(BM_BITSET, 448);
BENCHMARK_TEMPLATE(BM_BITSET, 512);

template<std::size_t bit_width>
static void BM_AVX(benchmark::State& state)
{
    simd_lfsr_128<bit_width> lfsr(maximum_cycle_polynom_head.at(bit_width/64-1));
    //    std::cout << "bitset:\n";
    for(auto _ : state){
        lfsr.clock();
    }
}
BENCHMARK_TEMPLATE(BM_AVX, 128);
BENCHMARK_TEMPLATE(BM_AVX, 256);
BENCHMARK_TEMPLATE(BM_AVX, 384);
BENCHMARK_TEMPLATE(BM_AVX, 512);


BENCHMARK_MAIN();

//int main()
//{

//    simd_lfsr_128<128> lfsr_128(maximum_cycle_polynom_head.at(1));
//    lfsr_128.print_state();
//    lfsr_128.clock();
//    lfsr_128.print_state();

//    return 0;
//}

