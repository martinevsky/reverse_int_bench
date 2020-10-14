#include <benchmark/benchmark.h>

#include <functional>

#include <boost/range.hpp>
#include <boost/core/ignore_unused.hpp>

template <size_t TableSize>
constexpr static auto CountTable2() noexcept
{
    std::array<uint32_t, TableSize> res = {0,};
    for (size_t ii = 0; ii < res.size(); ++ii)
        res[ii] = ii%42;
    return res;
}

using range = boost::iterator_range<const uint32_t*>;

template <class T>
struct ByT
{
    static uint32_t Test (T r, const std::function<void()>& f) 
    {
        uint32_t res = 0;
        for (const auto& v : r)
        {
            benchmark::DoNotOptimize (res += v);
            f();
        }
        return res;
    }
};

using ByVal = ByT<range>;
using ByRef = ByT<const range&>;

template <class Solution, size_t RangeSize>
void BM_PassRange (benchmark::State &state)
{
    constexpr const auto table = CountTable2<RangeSize>();
    const auto r = range (std::cbegin (table), std::cend (table));
    for (auto _ : state)
    {
        boost::ignore_unused (_);
        benchmark::DoNotOptimize (Solution::Test (r, [](){}));
    }
}

BENCHMARK_TEMPLATE(BM_PassRange, ByVal, 1000);
BENCHMARK_TEMPLATE(BM_PassRange, ByRef, 1000);

BENCHMARK_TEMPLATE(BM_PassRange, ByVal, 10000);
BENCHMARK_TEMPLATE(BM_PassRange, ByRef, 10000);

BENCHMARK_TEMPLATE(BM_PassRange, ByVal, 100000);
BENCHMARK_TEMPLATE(BM_PassRange, ByRef, 100000);