#pragma once

#include <sys/resource.h>
#include <ctime>
#include <cstdint>


struct SystemUsageStat {
    float clock_time;
    float user_time;
    float system_time;
    uint64_t memory_usage;
};


class SystemUsageStatCollector {
public:
    SystemUsageStatCollector();
    void reset();
    SystemUsageStat get_system_stat() const;
private:
    clock_t start_clock;
    rusage start_usage;
    uint64_t start_memory;
};
