#pragma once

#include <sys/resource.h>
#include <cstdint>
#include <ctime>
#include <memory>

namespace dev
{
namespace eth
{
struct SystemUsageStat
{
    float clockTime;
    float userTime;
    float systemTime;
    size_t memoryAllocated;
    int64_t extraMemoryAllocated;
};


class SystemUsageStatCollector
{
public:
    SystemUsageStatCollector();
    void reset();
    SystemUsageStat getSystemStat() const;

private:
    clock_t startClock;
    rusage startUsage;
    size_t startMemoryAllocated;
    size_t startMemoryDeallocated;
};

}  // namespace eth
}  // namespace dev
