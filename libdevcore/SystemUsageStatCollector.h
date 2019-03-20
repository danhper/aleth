#pragma once

#include <sys/resource.h>
#include <ctime>
#include <cstdint>
#include <memory>

namespace dev
{

struct SystemUsageStat {
    float clockTime;
    float userTime;
    float systemTime;
    size_t memoryAllocated;
    int64_t extraMemoryAllocated;
};


class SystemUsageStatCollector {
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

}
