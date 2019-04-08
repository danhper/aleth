#pragma once

#include <map>

#include <json/json.h>

#include <libethcore/Common.h>

namespace dev
{
namespace eth
{


struct StoreKeyStats {
    u256 initialValue;
    u256 newValue;
    uint64_t changesCount = 0;
    uint64_t writesCount = 0;
    uint64_t readsCount = 0;
    bool initialValueSet = false;

    StoreKeyStats();
};

class StoreStats {
public:
    void recordWrite(const u256 &key, u256 originalValue, const u256 &currentValue, const u256 &newValue);
    void recordRead(const u256 &key);
    Json::Value toJson() const;

private:
    std::map<u256, StoreKeyStats> m_Changes;
};

}
}
