#pragma once

#include <cstdint>
#include <utility>
#include <map>

#include <libevm/Instruction.h>

namespace dev
{
namespace eth
{

class BenchmarkResults
{
public:
    BenchmarkResults();
    explicit BenchmarkResults(uint64_t granularity);

    void addMeasurement(uint64_t measurement);
    void merge(const BenchmarkResults& other);

    uint64_t count() const { return m_count; }
    uint64_t sum() const { return m_sum; }
    uint64_t squaredSum() const { return m_squaredSum; }
    uint64_t granularity() const { return m_granularity; }

    double mean() const;
    double variance() const;
    double stdev() const;

private:
    uint64_t m_count;
    uint64_t m_sum;
    uint64_t m_squaredSum;

    const uint64_t m_granularity;
    uint64_t m_currentMeasurement;
    uint64_t m_currentMeasurementCount;

    void commitMeasurements();
};

template <typename T>
class BenchmarkResultsMap
{
public:
    explicit BenchmarkResultsMap(uint64_t granularity)
        : m_granularity(granularity) {}
    uint64_t totalCount() const { return m_totalCount; }

    void addMeasurement(const T& key, uint64_t value);
private:
    std::map<T, BenchmarkResults> m_results;
    uint64_t m_totalCount = 0;
    const uint64_t m_granularity;
};

template <typename T>
void BenchmarkResultsMap<T>::addMeasurement(const T& key, uint64_t value)
{
    m_totalCount += 1;
    auto result = m_results.find(key);
    if (result == m_results.end())
    {
        auto pair = std::pair<T, BenchmarkResults>(key, BenchmarkResults(m_granularity));
        auto inserted = m_results.insert(pair);
        result = inserted.first;
    }
    result->second.addMeasurement(value);
}

using InstructionsBenchmark = BenchmarkResultsMap<eth::Instruction>;


}
}