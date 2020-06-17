#include "parameters.hpp"
#include "sisd_lfsr.hpp"
#include "simd_lfsr.hpp"
#include "bitset_lfsr.hpp"

#include <immintrin.h>
#include <benchmark/benchmark.h>

#include <iostream>

template<std::size_t bit_width>
static void BM_SISD(benchmark::State& state)
{
    std::vector<std::uint64_t> polynom = {0x1010101010101010,
                                          0x2121212121212121,
                                          0x3232323232323232,
                                          0x4343434343434343,
                                          0x5454545454545454,
                                          0x6565656565656565,
                                          0x7676767676767676,
                                          0x8787878787878787};
    sisd_lfsr<bit_width> lfsr(polynom);
    for(auto _ : state){
        lfsr.clock();
    }
}

#if defined (__SSE4_1__)
template<std::size_t bit_width>
static void BM_SSE(benchmark::State& state)
{
    std::vector<std::uint64_t> polynom = {0x1010101010101010,
                                          0x2121212121212121,
                                          0x3232323232323232,
                                          0x4343434343434343,
                                          0x5454545454545454,
                                          0x6565656565656565,
                                          0x7676767676767676,
                                          0x8787878787878787};
    simd_lfsr_128<bit_width> lfsr(polynom);
    for(auto _ : state){
        lfsr.clock();
    }
}
#endif

#if defined (__AVX2__)
template<std::size_t bit_width>
static void BM_AVX_2(benchmark::State& state)
{
    std::vector<std::uint64_t> polynom = {0x1010101010101010,
                                          0x2121212121212121,
                                          0x3232323232323232,
                                          0x4343434343434343,
                                          0x5454545454545454,
                                          0x6565656565656565,
                                          0x7676767676767676,
                                          0x8787878787878787};
    simd_lfsr_256<bit_width> lfsr(polynom);
    for(auto _ : state){
        lfsr.clock();
    }
}
#endif

template<std::size_t bit_width>
static void BM_BITSET(benchmark::State& state)
{
    std::vector<std::uint64_t> polynom = {510, 496, 432, 365, 219, 278, 109, 165, 187, 34, 67, 53};
    bitset_lfsr<bit_width> lfsr(polynom);
    for(auto _ : state){
        lfsr.clock();
    }
}

BENCHMARK_TEMPLATE(BM_BITSET, 64);
BENCHMARK_TEMPLATE(BM_SISD, 64);

BENCHMARK_TEMPLATE(BM_BITSET, 128);
BENCHMARK_TEMPLATE(BM_SISD, 128);
BENCHMARK_TEMPLATE(BM_SSE, 128);

BENCHMARK_TEMPLATE(BM_BITSET, 192);
BENCHMARK_TEMPLATE(BM_SISD, 192);

BENCHMARK_TEMPLATE(BM_BITSET, 256);
BENCHMARK_TEMPLATE(BM_SISD, 256);
BENCHMARK_TEMPLATE(BM_SSE, 256);
BENCHMARK_TEMPLATE(BM_AVX_2, 256);

BENCHMARK_TEMPLATE(BM_BITSET, 320);
BENCHMARK_TEMPLATE(BM_SISD, 320);

BENCHMARK_TEMPLATE(BM_BITSET, 384);
BENCHMARK_TEMPLATE(BM_SISD, 384);
BENCHMARK_TEMPLATE(BM_SSE, 384);

BENCHMARK_TEMPLATE(BM_BITSET, 448);
BENCHMARK_TEMPLATE(BM_SISD, 448);

BENCHMARK_TEMPLATE(BM_BITSET, 512);
BENCHMARK_TEMPLATE(BM_SISD, 512);
BENCHMARK_TEMPLATE(BM_SSE, 512);
BENCHMARK_TEMPLATE(BM_AVX_2, 512);

BENCHMARK_MAIN();

