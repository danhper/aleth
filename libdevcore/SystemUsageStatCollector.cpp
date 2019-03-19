#include "SystemUsageStatCollector.h"

#include <cstdio>
#include <memory>
#include <map>


static thread_local size_t memoryAllocated = 0;
static thread_local size_t memoryDeallocated = 0;

#ifdef ETH_MEASURE_GAS
static thread_local std::map<intptr_t, size_t, std::less<intptr_t>, CAllocator<std::pair<const intptr_t, size_t>>> memoryMapping;
void* operator new(std::size_t sz) {
    // NOTE: if this overflows we probably have bigger problems
    memoryAllocated += sz;
    auto ptr = std::malloc(sz);
    memoryMapping[reinterpret_cast<intptr_t>(ptr)] = sz;
    return ptr;
}

void operator delete(void* ptr) noexcept {
    auto key = reinterpret_cast<intptr_t>(ptr);
    if (memoryMapping.find(key) != memoryMapping.end()) {
        memoryDeallocated += memoryMapping[key];
        memoryMapping.erase(key);
    }
    std::free(ptr);
}
#endif

static float getEllapsedSecs(timeval start, timeval end) {
    time_t ellapsed_us = end.tv_usec - start.tv_usec;
    return (float)ellapsed_us / 1000000;
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
    rusage end_usage = getCurrentUsage();
    auto totalMemoryAllocated = memoryAllocated - startMemoryAllocated;
    auto totalMemoryDeallocated = memoryDeallocated - startMemoryDeallocated;
    return {
        .clockTime = getClockEllapsedSecs(startClock, clock()),
        .userTime = getEllapsedSecs(startUsage.ru_utime, end_usage.ru_utime),
        .systemTime = getEllapsedSecs(startUsage.ru_stime, end_usage.ru_stime),
        .memoryAllocated = totalMemoryAllocated,
        .extraMemoryAllocated = static_cast<int64_t>(totalMemoryAllocated - totalMemoryDeallocated)
    };
}
