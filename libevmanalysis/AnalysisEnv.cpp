#include "AnalysisEnv.h"

namespace dev
{
namespace eth
{

void AnalysisEnv::outputInstructionsBenchmark(int64_t blockNumber, bool full)
{
    auto root = m_instructionsBenchmark.toJson(full);
    root["block_number"] = blockNumber;
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    boost::mutex::scoped_lock scoped_lock(m_benchmarkStreamLock);

    writer->write(root, &m_benchmarkStream);
    m_benchmarkStream << std::endl;
    m_lastInstructionsBenchmarkCount = m_instructionsBenchmark.totalCount();
}


}
}
