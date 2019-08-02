#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <utility>

#include <json/json.h>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/multiprecision/float128.hpp>
#include <boost/multiprecision/number.hpp>

#include <libevmanalysis/ExtendedInstruction.h>
#include <libevm/Instruction.h>

namespace
{
    const uint64_t defaultGranularity = 100;
}

namespace dev
{
template <typename T>
std::string keyToString(T value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

template <>
std::string keyToString<eth::Instruction>(eth::Instruction instruction);

template <>
std::string keyToString<eth::ExtendedInstruction>(eth::ExtendedInstruction einstruction);


namespace eth
{
struct BenchmarkMeasurement
{
    BenchmarkMeasurement(double _time, bigint _gas) : time(_time), gas(_gas) {}

    double time;
    bigint gas;

    BenchmarkMeasurement& operator+=(const BenchmarkMeasurement& rhs)
    {
        time += rhs.time;
        gas += rhs.gas;
        return *this;  // return the result by reference
    }

    template <typename T>
    BenchmarkMeasurement& operator/=(T value)
    {
        time = static_cast<double>(time) / value;
        gas /= value;
        return *this;
    }

    template <typename T>
    friend BenchmarkMeasurement operator/(BenchmarkMeasurement lhs, T value)
    {
        lhs /= value;
        return lhs;
    }
};

class BenchmarkResults
{
public:
    BenchmarkResults();
    explicit BenchmarkResults(uint64_t granularity);

    void addMeasurement(BenchmarkMeasurement measurement);

    uint64_t granularity() const { return m_granularity; }

    uint64_t count() const { return boost::accumulators::count(m_accTime); }
    double mean() const { return boost::accumulators::mean(m_accTime); }
    double variance() const { return boost::accumulators::variance(m_accTime); }
    double stdev() const { return std::sqrt(variance()); }

    double gasMean() const { return boost::accumulators::mean(m_accGas); }
    double gasStdev() const { return std::sqrt(boost::accumulators::variance(m_accGas)); }

    Json::Value toJson(bool full = false) const;

private:
    std::vector<BenchmarkMeasurement> m_measurements;
    boost::accumulators::accumulator_set<double,
        boost::accumulators::features<boost::accumulators::tag::count,
            boost::accumulators::tag::mean, boost::accumulators::tag::variance>>
        m_accTime;

    boost::accumulators::accumulator_set<double,
        boost::accumulators::features<boost::accumulators::tag::mean,
            boost::accumulators::tag::variance>>
        m_accGas;

    const uint64_t m_granularity;
    BenchmarkMeasurement m_currentMeasurement;
    uint64_t m_currentMeasurementCount;

    void commitMeasurements();
};

template <typename T>
class BenchmarkResultsMap
{
public:
    explicit BenchmarkResultsMap() : m_granularity(defaultGranularity) {}
    explicit BenchmarkResultsMap(uint64_t granularity) : m_granularity(granularity) {}
    uint64_t totalCount() const { return m_totalCount; }

    void addMeasurement(const T& key, BenchmarkMeasurement measurement);

    Json::Value toJson(bool full = false) const;

private:
    std::map<T, BenchmarkResults> m_results;
    uint64_t m_totalCount = 0;
    const uint64_t m_granularity;
};

template <typename T>
void BenchmarkResultsMap<T>::addMeasurement(const T& key, BenchmarkMeasurement measurement)
{
    m_totalCount += 1;
    auto result = m_results.find(key);
    if (result == m_results.end())
    {
        auto pair = std::pair<T, BenchmarkResults>(key, BenchmarkResults(m_granularity));
        auto inserted = m_results.insert(pair);
        result = inserted.first;
    }
    result->second.addMeasurement(measurement);
}


template <typename T>
Json::Value BenchmarkResultsMap<T>::toJson(bool full) const
{
    Json::Value result;
    result["granularity"] = m_granularity;
    result["total_count"] = m_totalCount;
    result["stats"] = Json::Value();
    for (auto& kv : m_results)
    {
        result["stats"][keyToString(kv.first)] = kv.second.toJson(full);
    }
    return result;
}


template <typename T>
std::ostream& operator<<(std::ostream& os, const BenchmarkResultsMap<T>& v)
{
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(v.toJson(), &os);
    return os;
}


using InstructionsBenchmark = BenchmarkResultsMap<eth::ExtendedInstruction>;


}  // namespace eth
}  // namespace dev
