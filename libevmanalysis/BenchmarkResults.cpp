#include "BenchmarkResults.h"

#include <cmath>
#include <stdexcept>
#include <sstream>

namespace dev
{
namespace eth
{


BenchmarkResults::BenchmarkResults(): BenchmarkResults(1) {}

BenchmarkResults::BenchmarkResults(uint64_t granularity)
    : m_count(0),
      m_sum(0),
      m_squaredSum(0),
      m_granularity(granularity),
      m_currentMeasurement(0),
      m_currentMeasurementCount(0) {}

void BenchmarkResults::addMeasurement(uint64_t measurement)
{
    if (m_currentMeasurementCount + 1 >= m_granularity)
    {
        m_currentMeasurement += measurement;
        m_count += 1;
        m_sum += m_currentMeasurement;
        m_squaredSum += m_currentMeasurement * m_currentMeasurement;
        m_currentMeasurement = 0;
        m_currentMeasurementCount = 0;
    }
    else
    {
        m_currentMeasurement += measurement;
        m_currentMeasurementCount += 1;
    }
}

void BenchmarkResults::merge(const BenchmarkResults& other)
{
    if (granularity() != other.granularity())
    {
        std::stringstream ss;
        ss << "merging two benchmarks with different granularity: "
           << granularity() << " != " << other.granularity()
           << " this might lead to unexpected results";
        throw std::runtime_error(ss.str());
    }
    m_count += other.count();
    m_sum += other.sum();
    m_squaredSum += other.squaredSum();
}

double BenchmarkResults::mean() const
{
    return static_cast<double>(m_sum) / m_count;
}

double BenchmarkResults::variance() const
{
    auto dataMean = mean();
    auto squaredMean = static_cast<double>(m_squaredSum) / m_count;
    return squaredMean - (dataMean * dataMean);
}

double BenchmarkResults::stdev() const
{
    return std::sqrt(variance());
}

Json::Value BenchmarkResults::toJson() const
{
    Json::Value result;
    result["count"] = count();
    result["sum"] = sum();
    result["squared_sum"] = squaredSum();
    result["granularity"] = granularity();
    result["mean"] = mean();
    result["variance"] = variance();
    result["stdev"] = stdev();
    return result;
}

}
}