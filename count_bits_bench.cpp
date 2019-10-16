#include <array>
#include <vector>
#include <random>
#include <limits>
#include <future>
#include <iostream>

#include <boost/make_unique.hpp>

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

struct AsmSolution
{
    __attribute__((always_inline))
    static constexpr uint32_t Count (uint32_t n) noexcept
    {
        return __builtin_popcount (n);
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
        return g_byteTable[n & 0xFF] +
            g_byteTable[(n >>= 8) & 0xFF] +
            g_byteTable[(n >>= 8) & 0xFF] +
            g_byteTable[(n >> 8) & 0xFF];
    }

private:
    constexpr static auto g_byteTable = CountTable<256>();
};

struct ElevenBitsTableSolution
{
    static constexpr uint32_t Count (uint32_t n) noexcept
    {
        return g_byteTable[n & 0b11111111111] +
            g_byteTable[(n >> 11) & 0b11111111111] +
            g_byteTable[(n >> 22)];
    }

private:
    constexpr static auto g_byteTable = CountTable<1 << 11>();
};

struct WordsTableSolution
{
    static constexpr uint32_t Count (uint32_t n) noexcept
    {
        return g_byteTable[n & 0xFFFF] +
            g_byteTable[n >> 16];
    }

private:
    constexpr static auto g_byteTable = CountTable<1 << 16>();
};

struct FullTableSolution
{
private:
    static auto CountFullTable() 
    {
        auto table = boost::make_unique_noinit<uint32_t[]> (1l << 32);

        std::array<std::future<void>, 4> workers;
        constexpr auto batch = (1l << 32)/workers.size();
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

auto GenerateNUmbers()
{
    std::array<uint32_t, 10000> res;

    std::mt19937 gen(42);
    std::uniform_int_distribution<uint32_t> dist(0, std::numeric_limits<uint32_t>::max());

    const auto generator = [&gen, &dist]() { return dist(gen); };

    std::generate (res.begin(), res.end(), generator);

    return res;
}

template <class Solution>
void BM_Count (benchmark::State &state)
{
    const auto nums = GenerateNUmbers();
    Solution::Count (42); // Heatup table

    size_t curr = 0;
    for (auto _ : state)
    {
        for (auto num : nums)
            benchmark::DoNotOptimize (Solution::Count (num));
        state.SetItemsProcessed (state.items_processed() + nums.size());
    }
}

BENCHMARK_TEMPLATE(BM_Count, ReferenceSolution);
BENCHMARK_TEMPLATE(BM_Count, AsmSolution);
BENCHMARK_TEMPLATE(BM_Count, ByteTableSolution);
BENCHMARK_TEMPLATE(BM_Count, ElevenBitsTableSolution);
BENCHMARK_TEMPLATE(BM_Count, WordsTableSolution);
BENCHMARK_TEMPLATE(BM_Count, FullTableSolution);

BENCHMARK_TEMPLATE(BM_Count, ReferenceSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, AsmSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, ByteTableSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, ElevenBitsTableSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, WordsTableSolution)->Threads (2);
BENCHMARK_TEMPLATE(BM_Count, FullTableSolution)->Threads (2);

BENCHMARK_TEMPLATE(BM_Count, ReferenceSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, AsmSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, ByteTableSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, ElevenBitsTableSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, WordsTableSolution)->Threads (4);
BENCHMARK_TEMPLATE(BM_Count, FullTableSolution)->Threads (4);

BENCHMARK_TEMPLATE(BM_Count, ReferenceSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, AsmSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, ByteTableSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, ElevenBitsTableSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, WordsTableSolution)->Threads (8);
BENCHMARK_TEMPLATE(BM_Count, FullTableSolution)->Threads (8);

void BM_CountCheck (benchmark::State &state)
{
    const auto nums = GenerateNUmbers();

    size_t curr = 0;
    for (auto _ : state)
    {
        for (auto num : nums)
        {
            const auto etalon = ReferenceSolution::Count (num);
            const uint32_t ress[] = 
                {
                    AsmSolution::Count (num),
                    ByteTableSolution::Count (num),
                    ElevenBitsTableSolution::Count (num),
                    WordsTableSolution::Count (num),
                    FullTableSolution::Count (num)
                };

            if (std::count (std::cbegin (ress), std::cend (ress), etalon) != std::size (ress)) 
                throw std::runtime_error ("test");
        }

        state.SetItemsProcessed (state.items_processed() + nums.size());
    }
}

BENCHMARK (BM_CountCheck);