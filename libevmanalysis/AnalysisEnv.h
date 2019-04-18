#pragma once

#include <ostream>
#include <iostream>

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
            InstructionsBenchmark& instructionsBenchmark
        )
        : m_statStream(statStream),
          m_instructionsBenchmark(instructionsBenchmark)
         {}
    AnalysisEnv(const AnalysisEnv&) = delete;

    std::ostream& statStream() { return m_statStream; }
    InstructionsBenchmark& instructionsBenchmark() { return m_instructionsBenchmark; }

private:
    std::ostream& m_statStream = std::cout;
    InstructionsBenchmark& m_instructionsBenchmark;
};


}
}
