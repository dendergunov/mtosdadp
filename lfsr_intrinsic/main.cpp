#include <immintrin.h>
#include <benchmark/benchmark.h>

#include <iostream>

static void BM_SomeFunction(benchmark::State& state)
{
    int i=0;
    for(auto _ : state)
        ++i;
}

BENCHMARK(BM_SomeFunction);
//BENCHMARK_MAIN();

int main()
{
    //Выбор размера регистра?

    //Выбор характеристического многочлена?

    //Задание начального состояния?

    //Отдельные функции для каждого размера? (64,128,256,512)

    //Что и как выводить?

    //А что собственно нужно? Замерить время сколько тактов в секунду?

    //Гугл бенчмарк?

    return 0;
}

