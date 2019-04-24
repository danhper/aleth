#pragma once

#include <sys/resource.h>
#include <cstdint>
#include <ctime>
#include <memory>

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
};

}  // namespace eth
}  // namespace dev
