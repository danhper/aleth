#include "BenchmarkResults.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace dev
{

template <>
std::string keyToString<eth::Instruction>(eth::Instruction instruction)
{
    return instructionInfo(instruction).name;
}

template <>
std::string keyToString<eth::ExtendedInstruction>(eth::ExtendedInstruction einstruction)
{
    return instructionName(einstruction);
}


namespace eth
{
BenchmarkResults::BenchmarkResults() : BenchmarkResults(1) {}

BenchmarkResults::BenchmarkResults(uint64_t granularity)
  : m_granularity(granularity),
    m_currentMeasurement(BenchmarkMeasurement(0, 0)),
    m_currentMeasurementCount(0)
{}

void BenchmarkResults::addMeasurement(BenchmarkMeasurement measurement)
{
    if (m_currentMeasurementCount + 1 >= m_granularity)
    {
        m_currentMeasurement += measurement;
        auto normalizedMeasurement = m_currentMeasurement / m_granularity;
        m_measurements.push_back(normalizedMeasurement);
        m_accTime(normalizedMeasurement.time);
        m_accGas(normalizedMeasurement.gas.convert_to<double>());
        m_currentMeasurement = BenchmarkMeasurement(0, 0);
        m_currentMeasurementCount = 0;
    }
    else
    {
        m_currentMeasurement += measurement;
        m_currentMeasurementCount += 1;
    }
}


Json::Value BenchmarkResults::toJson(bool full) const
{
    Json::Value result;
    result["count"] = count();
    result["granularity"] = granularity();
    result["timeMean"] = mean();
    result["timeVariance"] = variance();
    result["timeStdev"] = stdev();
    result["gasMean"] = gasMean();
    result["gasStdev"] = gasStdev();
    result["throughputMean"] = gasMean() / mean();
    if (full)
    {
        auto times = Json::Value(Json::arrayValue);
        auto gas = Json::Value(Json::arrayValue);
        for (auto measurement : m_measurements)
        {
            times.append(measurement.time);
            gas.append(measurement.gas.convert_to<double>());
        }
        result["times"] = times;
        result["gas"] = times;
    }
    return result;
}

}  // namespace eth
}  // namespace dev
