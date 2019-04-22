#include "BenchmarkResults.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace dev
{
namespace eth
{
BenchmarkResults::BenchmarkResults() : BenchmarkResults(1) {}

BenchmarkResults::BenchmarkResults(uint64_t granularity)
  : m_granularity(granularity), m_currentMeasurement(0), m_currentMeasurementCount(0)
{}

void BenchmarkResults::addMeasurement(uint64_t measurement)
{
    if (m_currentMeasurementCount + 1 >= m_granularity)
    {
        m_currentMeasurement += measurement;
        auto normalizedMeasurement = static_cast<double>(m_currentMeasurement) / m_granularity;
        m_measurements.push_back(normalizedMeasurement);
        m_acc(normalizedMeasurement);
        m_currentMeasurement = 0;
        m_currentMeasurementCount = 0;
    }
    else
    {
        m_currentMeasurement += measurement;
        m_currentMeasurementCount += 1;
    }
}

uint64_t BenchmarkResults::count() const
{
    return boost::accumulators::count(m_acc);
}

double BenchmarkResults::mean() const
{
    return boost::accumulators::mean(m_acc);
}

double BenchmarkResults::variance() const
{
    return boost::accumulators::variance(m_acc);
}

double BenchmarkResults::stdev() const
{
    return std::sqrt(variance());
}

Json::Value BenchmarkResults::toJson(bool full) const
{
    Json::Value result;
    result["count"] = count();
    result["granularity"] = granularity();
    result["mean"] = mean();
    result["variance"] = variance();
    result["stdev"] = stdev();
    if (full)
    {
        auto measurements = Json::Value(Json::arrayValue);
        for (auto measurement : m_measurements)
        {
            measurements.append(measurement);
        }
        result["measurements"] = measurements;
    }
    return result;
}

}  // namespace eth
}  // namespace dev
