#include "SystemUsageStatCollector.h"

#include <cstdio>
#include <map>
#include <memory>

static thread_local size_t memoryAllocated = 0;
static thread_local size_t memoryDeallocated = 0;

#define MAX_SIZE 1000003
static thread_local intptr_t memoryMapping[MAX_SIZE];

void* operator new(std::size_t sz)
{
    // NOTE: if this overflows we probably have bigger problems
    memoryAllocated += sz;
    auto ptr = std::malloc(sz);
    intptr_t key = reinterpret_cast<intptr_t>(ptr) % MAX_SIZE;
    memoryMapping[key] = sz;
    return ptr;
}

void operator delete(void* ptr) noexcept
{
    intptr_t key = reinterpret_cast<intptr_t>(ptr) % MAX_SIZE;
    memoryDeallocated += memoryMapping[key];
    memoryMapping[key] = 0;
    std::free(ptr);
}

namespace
{
const uint64_t microsecondsPerSeconds = 1000000;
const uint64_t nanosecondsPerSeconds = 1000 * microsecondsPerSeconds;
}

namespace dev
{
namespace eth
{
static float getEllapsedSecs(timeval start, timeval end)
{
    time_t ellapsed_us = end.tv_usec - start.tv_usec;
    return static_cast<float>(ellapsed_us) / microsecondsPerSeconds;
}

static float getClockEllapsedSecs(clock_t start, clock_t stop)
{
    clock_t ellapsed_clock = stop - start;
    return ellapsed_clock / static_cast<float>(CLOCKS_PER_SEC);
}

static float getTimespecEllapsedSecs(timespec start, timespec stop)
{
    auto startNs = start.tv_nsec * nanosecondsPerSeconds + start.tv_nsec;
    auto endNs = stop.tv_nsec * nanosecondsPerSeconds + stop.tv_nsec;
    auto ellapsed = endNs - startNs;
    return static_cast<double>(ellapsed) / nanosecondsPerSeconds;
}


Json::Value SystemUsageStat::toJson() const
{
    Json::Value root;
    root["clock_time"] = clockTime;
    root["user_time"] = userTime;
    root["monotonic_time"] = monotonicTime;
    root["system_time"] = systemTime;
    root["memory_allocated"] = memoryAllocated;
    root["extra_memory_allocated"] = extraMemoryAllocated;
    return root;
}

SystemUsageStatCollector::SystemUsageStatCollector()
{
    reset();
}

void SystemUsageStatCollector::reset()
{
    m_startMemoryAllocated = memoryAllocated;
    m_startMemoryDeallocated = memoryDeallocated;
    m_startClock = clock();
    getrusage(RUSAGE_SELF, &m_startUsage);
}

SystemUsageStat SystemUsageStatCollector::getSystemStat() const
{
    auto totalMemoryAllocated = memoryAllocated - m_startMemoryAllocated;
    auto totalMemoryDeallocated = memoryDeallocated - m_startMemoryDeallocated;

    timespec endTimespec;
    clock_gettime(CLOCK_MONOTONIC, &endTimespec);

    rusage endUsage;
    getrusage(RUSAGE_SELF, &endUsage);
    return {.clockTime = getClockEllapsedSecs(m_startClock, clock()),
        .userTime = getEllapsedSecs(m_startUsage.ru_utime, endUsage.ru_utime),
        .systemTime = getEllapsedSecs(m_startUsage.ru_stime, endUsage.ru_stime),
        .monotonicTime = getTimespecEllapsedSecs(m_startTimespec, endTimespec),
        .memoryAllocated = totalMemoryAllocated,
        .extraMemoryAllocated =
            static_cast<int64_t>(totalMemoryAllocated - totalMemoryDeallocated)};
}

}  // namespace eth
}  // namespace dev
