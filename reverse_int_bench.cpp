#include <limits.h>

#include <algorithm>
#include <iostream>
#include <random>
#include <set>
#include <unordered_set>
#include <vector>

#include <benchmark/benchmark.h>

class MySolution
{
public:
    static int reverse(int x)
    {
        if (x > 0)
        {
            const auto res = reverse(-x);
            if (res == INT_MIN)
                return 0;
            return -res;
        }

        int res = 0;

        while (x)
        {
            const int sub = x % 10;
            x = x / 10;
            if (res < INT_MIN / 10)
                return 0;
            if (res == INT_MIN / 10 && sub < -8)
                return 0;
            res = res * 10 + sub;
        }

        return res;
    }
};

class ReferenceSolution
{
public:
    static int reverse(int x)
    {
        int rev = 0;
        while (x != 0)
        {
            int pop = x % 10;
            x /= 10;
            if (rev > INT_MAX / 10 || (rev == INT_MAX / 10 && pop > 7))
                return 0;
            if (rev < INT_MIN / 10 || (rev == INT_MIN / 10 && pop < -8))
                return 0;
            rev = rev * 10 + pop;
        }
        return rev;
    }
};

template <class Solution>
void BM_Find(benchmark::State &state)
{
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);

    const auto generator = [&gen, &dist]() { return dist(gen); };

    for (auto _ : state)
    {
        const constexpr size_t iterations = 1'000;
        const auto val = generator();
        for (size_t ii = 0; ii < iterations; ++ii)
            benchmark::DoNotOptimize(Solution::reverse (val));

        state.SetItemsProcessed(state.items_processed() + iterations);
    }
}

BENCHMARK_TEMPLATE(BM_Find, ReferenceSolution);
BENCHMARK_TEMPLATE(BM_Find, MySolution);

BENCHMARK_MAIN();