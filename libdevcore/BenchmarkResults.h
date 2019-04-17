#pragma once

#include <cstdint>

namespace dev
{

class BenchmarkResults {
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

}
