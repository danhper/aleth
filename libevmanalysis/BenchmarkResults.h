#pragma once

#include <cstdint>
#include <utility>
#include <map>
#include <memory>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <json/json.h>

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

    uint64_t granularity() const { return m_granularity; }

    uint64_t count() const;
    double mean() const;
    double variance() const;
    double stdev() const;

    Json::Value toJson() const;

private:
    boost::accumulators::accumulator_set<double,
        boost::accumulators::features<boost::accumulators::tag::count,
                                      boost::accumulators::tag::mean,
                                      boost::accumulators::tag::variance>> m_acc;

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

    Json::Value toJson() const;
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

template <typename T>
Json::Value BenchmarkResultsMap<T>::toJson() const
{
    Json::Value result;
    result["granularity"] = m_granularity;
    result["total_count"] = m_totalCount;
    result["stats"] = Json::Value();
    for (auto& kv : m_results)
    {
        result["stats"][instructionInfo(kv.first).name] = kv.second.toJson();
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


using InstructionsBenchmark = BenchmarkResultsMap<eth::Instruction>;


}
}