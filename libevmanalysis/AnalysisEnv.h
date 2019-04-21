#pragma once

#include <ostream>
#include <iostream>

#include <boost/thread/mutex.hpp>

#include "BenchmarkResults.h"

namespace dev
{
namespace eth
{

class AnalysisEnv
{
public:
    AnalysisEnv() = default;
    explicit AnalysisEnv(
            std::ostream& statStream,
            std::ostream& benchmarkStream,
            InstructionsBenchmark& instructionsBenchmark,
            int64_t benchmarkInterval = 1000
        )
        : m_statStream(statStream),
          m_benchmarkStream(benchmarkStream),
          m_benchmarkInterval(benchmarkInterval),
          m_instructionsBenchmark(instructionsBenchmark)
         {}
    AnalysisEnv(const AnalysisEnv&) = delete;

    std::ostream& statStream() { return m_statStream; }
    InstructionsBenchmark& instructionsBenchmark() { return m_instructionsBenchmark; }
    boost::mutex& statStreamLock() { return m_statStreamLock; }
    int64_t benchmarkInteval() const { return m_benchmarkInterval; }
    void outputInstructionsBenchmark(int64_t blockNumber);

private:
    std::ostream& m_statStream = std::cout;
    std::ostream& m_benchmarkStream = std::cout;

    int64_t m_benchmarkInterval = 1000;

    uint64_t m_lastInstructionsBenchmarkCount = 0;

    InstructionsBenchmark& m_instructionsBenchmark;

    boost::mutex m_statStreamLock;
    boost::mutex m_benchmarkStreamLock;
};


}
}
