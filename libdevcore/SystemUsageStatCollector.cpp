#include "SystemUsageStatCollector.h"

#include <cstdio>

static uint64_t get_memory_usage() {
    FILE* stat_file = fopen("/proc/self/stat", "r");
    if (stat_file == NULL) {
        return 0;
    }
    uint64_t used_memory;
    // NOTE: vsize is field number 23 and is the virtual memory size in bytes
    // see 'man 5 proc'
    fscanf(stat_file,
        "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u"
        "%*u %*u %*u %*u %*u %*d %*d %*d %*d %*d"
        "%*d %*d %lu",
        &used_memory);
    fclose(stat_file);
    return used_memory;
}

static float get_ellapsed_secs(timeval start, timeval end) {
    time_t ellapsed_us = end.tv_usec - start.tv_usec;
    return (float)ellapsed_us / 1000000;
}

static float get_clock_ellapsed_secs(clock_t start, clock_t stop) {
    clock_t ellapsed_clock = stop - start;
    return ellapsed_clock / (float)CLOCKS_PER_SEC;
}

static rusage get_current_usage() {
    rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage;
}

SystemUsageStatCollector::SystemUsageStatCollector() {
    reset();
}

void SystemUsageStatCollector::reset() {
    start_memory = get_memory_usage();
    start_clock = clock();
    start_usage = get_current_usage();
}

SystemUsageStat SystemUsageStatCollector::get_system_stat() const {
    rusage end_usage = get_current_usage();
    return {
        .clock_time = get_clock_ellapsed_secs(start_clock, clock()),
        .user_time = get_ellapsed_secs(start_usage.ru_utime, end_usage.ru_utime),
        .system_time = get_ellapsed_secs(start_usage.ru_stime, end_usage.ru_stime),
        .memory_usage = get_memory_usage() - start_memory,
    };
}
