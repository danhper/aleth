#include "SystemUsageStatCollector.h"

#include <cstdio>
#include <memory>
#include <map>

static thread_local size_t memoryAllocated = 0;
static thread_local size_t memoryDeallocated = 0;

#define MAX_SIZE 1000003
static thread_local intptr_t memoryMapping[MAX_SIZE];

void* operator new(std::size_t sz) {
    // NOTE: if this overflows we probably have bigger problems
    memoryAllocated += sz;
    auto ptr = std::malloc(sz);
    intptr_t key = reinterpret_cast<intptr_t>(ptr) % MAX_SIZE;
    memoryMapping[key] = sz;
    return ptr;
}

void operator delete(void* ptr) noexcept {
    intptr_t key = reinterpret_cast<intptr_t>(ptr) % MAX_SIZE;
    memoryDeallocated += memoryMapping[key];
    memoryMapping[key] = 0;
    std::free(ptr);
}

namespace
{
    const uint64_t microsecondsPerSeconds = 1000000;
}

namespace dev
{

namespace eth
{

static float getEllapsedSecs(timeval start, timeval end) {
    time_t ellapsed_us = end.tv_usec - start.tv_usec;
    return (float)ellapsed_us / microsecondsPerSeconds;
}

static float getClockEllapsedSecs(clock_t start, clock_t stop) {
    clock_t ellapsed_clock = stop - start;
    return ellapsed_clock / (float)CLOCKS_PER_SEC;
}

static rusage getCurrentUsage() {
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage;
}

SystemUsageStatCollector::SystemUsageStatCollector() {
    reset();
}

void SystemUsageStatCollector::reset() {
    startMemoryAllocated = memoryAllocated;
    startMemoryDeallocated = memoryDeallocated;
    startClock = clock();
    startUsage = getCurrentUsage();
}

SystemUsageStat SystemUsageStatCollector::getSystemStat() const {
    auto totalMemoryAllocated = memoryAllocated - startMemoryAllocated;
    auto totalMemoryDeallocated = memoryDeallocated - startMemoryDeallocated;
    rusage end_usage = getCurrentUsage();
    return {
        .clockTime = getClockEllapsedSecs(startClock, clock()),
        .userTime = getEllapsedSecs(startUsage.ru_utime, end_usage.ru_utime),
        .systemTime = getEllapsedSecs(startUsage.ru_stime, end_usage.ru_stime),
        .memoryAllocated = totalMemoryAllocated,
        .extraMemoryAllocated = static_cast<int64_t>(totalMemoryAllocated - totalMemoryDeallocated)
    };
}

}
}
