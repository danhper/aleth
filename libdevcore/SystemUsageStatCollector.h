#pragma once

#include <sys/resource.h>
#include <ctime>
#include <cstdint>
#include <memory>


template<typename T>
struct CAllocator : public std::allocator<T> {
    inline typename std::allocator<T>::pointer allocate(typename std::allocator<T>::size_type n, typename std::allocator<void>::const_pointer = 0) {
        return static_cast<T*>(std::malloc(n * sizeof(T)));
    }

    inline void deallocate(typename std::allocator<T>::pointer p, __attribute__((unused)) typename std::allocator<T>::size_type n) {
        std::free(p);
    }

    template<typename U>
    struct rebind {
        typedef CAllocator<U> other;
    };
};


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
