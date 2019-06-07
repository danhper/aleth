#pragma once

#include <sys/resource.h>
#include <cstdint>
#include <ctime>
#include <memory>
#include <chrono>

#include <json/json.h>

namespace dev
{
namespace eth
{
struct SystemUsageStat
{
    float clockTime;
    float userTime;
    float systemTime;
    float monotonicTime;
    float chronoTime;
    size_t memoryAllocated;
    int64_t extraMemoryAllocated;

    Json::Value toJson() const;
};


class SystemUsageStatCollector
{
public:
    SystemUsageStatCollector();
    void reset();
    SystemUsageStat getSystemStat() const;

private:
    clock_t m_startClock;
    timespec m_startTimespec;
    rusage m_startUsage;
    size_t m_startMemoryAllocated;
    size_t m_startMemoryDeallocated;
    std::chrono::high_resolution_clock::time_point m_startChrono;
};

}  // namespace eth
}  // namespace dev
