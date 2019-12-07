#include <array>
#include <vector>
#include <random>
#include <limits>
#include <future>
#include <iostream>

#include <boost/make_unique.hpp>
#include <boost/core/ignore_unused.hpp>

#include <benchmark/benchmark.h>

struct ReferenceSolution
{
    __attribute__((always_inline))
    static constexpr uint32_t Count (uint32_t n) noexcept
    {
        uint32_t i = 0;

        while (n)
        {
            n &= n - 1;
            ++i;
        }

        return i;
    }
};

#pragma GCC target("popcnt") 
struct AsmSolution
{
    __attribute__((always_inline))
    static constexpr uint32_t Count (uint32_t n) noexcept
    {
        return __builtin_popcount (n);
    }
};

struct MagicSolution
{
    __attribute__((always_inline))
    static constexpr uint32_t Count (uint32_t v) noexcept
    {
        v = v - ((v >> 1) & 0x55555555);
        v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
        return static_cast<uint16_t> ((((v + (v >> 4)) & 0xf0f0f0f) * 0x1010101) >> 24);
    }
};

template <size_t TableSize>
constexpr static auto CountTable() noexcept
{
    std::array<uint32_t, TableSize> res = {0,};
    for (size_t ii = 0; ii < res.size(); ++ii)
        res[ii] = ReferenceSolution::Count (ii);
    return res;
}

struct ByteTableSolution
{
    static constexpr uint32_t Count (uint32_t n) noexcept
    {
        uint32_t res = 0;

        res += g_byteTable[n & 0xFF];
        res += g_byteTable[(n >>= 8) & 0xFF];
        res += g_byteTable[(n >>= 8) & 0xFF];
        res += g_byteTable[(n >> 8) & 0xFF];

        return res;
    }

private:
    constexpr static auto g_byteTable = CountTable<256>();
};

struct ElevenBitsTableSolution
{
    static constexpr uint32_t Count (uint32_t n) noexcept
    {
        uint32_t res = 0;

        res += g_byteTable[n & 0b11111111111];
        res += g_byteTable[(n >>= 11) & 0b11111111111];
        res += g_byteTable[(n >> 11)];

        return res;
    }

private:
    inline const static auto g_byteTable = CountTable<2048>();
};

struct WordsTableSolution
{
    static constexpr uint32_t Count (uint32_t n) noexcept
    {
        return g_byteTable[n & 0xFFFF] +
            g_byteTable[n >> 16];
    }

private:
    inline const static auto g_byteTable = CountTable<65536>();
};

struct FullTableSolution
{
private:
    static auto CountFullTable() 
    {
        auto table = boost::make_unique_noinit<uint32_t[]> (1ull << 32);

        std::array<std::future<void>, 4> workers;
        const auto batch = (1ull << 32)/workers.size();
        for (size_t ii = 0; ii < workers.size(); ++ii)
            workers[ii] = std::async (std::launch::async, [&table, start = ii*batch, finish = (ii + 1)*batch]()
                {
                    for (auto ii = start; ii < finish; ++ii)
                        table[ii] = ElevenBitsTableSolution::Count (ii);
                });

        return table;
    }

    static auto& GetTable()
    {
        static const auto table = CountFullTable();
        return table;
    }

public:
    static uint32_t Count (uint32_t n) noexcept
    {
        return GetTable()[n];
    }
};

auto GenerateNumbers()
{
    std::array<uint32_t, 100000> res;

    std::mt19937 gen(42);
    std::uniform_int_distribution<uint32_t> dist(0, std::numeric_limits<uint32_t>::max());

    const auto generator = [&gen, &dist]() { return dist(gen); };

    std::generate (res.begin(), res.end(), generator);

    return res;
}

template <class Solution>
void BM_Count (benchmark::State &state)
{
    const auto nums = GenerateNumbers();
    Solution::Count (42); // Heatup table

    for (auto _ : state)
    {
        boost::ignore_unused (_);
        for (auto num : nums)
            benchmark::DoNotOptimize (Solution::Count (num));
        state.SetItemsProcessed (state.items_processed() + nums.size());
    }
}

BENCHMARK_TEMPLATE(BM_Count, ReferenceSolution);
BENCHMARK_TEMPLATE(BM_Count, AsmSolution);
BENCHMARK_TEMPLATE(BM_Count, MagicSolution);
BENCHMARK_TEMPLATE(BM_Count, ByteTableSolution);
BENCHMARK_TEMPLATE(BM_Count, ElevenBitsTableSolution);
BENCHMARK_TEMPLATE(BM_Count, WordsTableSolution);
BENCHMARK_TEMPLATE(BM_Count, FullTableSolution);

BENCHMARK_TEMPLATE(BM_Count, ReferenceSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, AsmSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, MagicSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, ByteTableSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, ElevenBitsTableSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, WordsTableSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, FullTableSolution)->Threads (2);

BENCHMARK_TEMPLATE(BM_Count, ReferenceSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, AsmSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, MagicSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, ByteTableSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, ElevenBitsTableSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, WordsTableSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, FullTableSolution)->Threads (4);

BENCHMARK_TEMPLATE(BM_Count, ReferenceSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, AsmSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, MagicSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, ByteTableSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, ElevenBitsTableSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, WordsTableSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, FullTableSolution)->Threads (8);

void BM_CountCheck (benchmark::State &state)
{
    const auto nums = GenerateNumbers();

    for (auto _ : state)
    {
        boost::ignore_unused (_);
        for (auto num : nums)
        {
            const auto etalon = ReferenceSolution::Count (num);
            const uint32_t ress[] = 
                {
                    AsmSolution::Count (num),
                    ByteTableSolution::Count (num),
                    ElevenBitsTableSolution::Count (num),
                    WordsTableSolution::Count (num),
                    MagicSolution::Count (num),
                    FullTableSolution::Count (num)
                };

            if (std::count (std::cbegin (ress), std::cend (ress), etalon) != std::size (ress)) 
                throw std::runtime_error ("test");
        }

        state.SetItemsProcessed (state.items_processed() + nums.size());
    }
}

BENCHMARK (BM_CountCheck);