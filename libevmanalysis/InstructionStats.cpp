#include "InstructionStats.h"


namespace dev
{
namespace eth
{
StoreKeyStats::StoreKeyStats() = default;


void InstructionStats::recordWrite(
    const u256& key, u256 originalValue, const u256& currentValue, const u256& newValue)
{
    auto res = m_changes.insert(std::make_pair(key, StoreKeyStats()));
    auto& stats = res.first->second;
    if (!stats.initialValueSet)
    {
        stats.initialValueSet = true;
        stats.initialValue = std::move(originalValue);
    }
    stats.newValue = newValue;
    stats.writesCount++;
    if (currentValue != newValue)
    {
        stats.changesCount++;
    }
}

void InstructionStats::recordRead(const u256& key)
{
    auto res = m_changes.insert(std::make_pair(key, StoreKeyStats()));
    auto& stats = res.first->second;
    stats.readsCount++;
}

void InstructionStats::recordCreate(const u256& size)
{
    m_createCalls.push_back(size);
}

void InstructionStats::recordSuicide()
{
    m_suicideCallsCount++;
}

void InstructionStats::recordInstruction(Instruction instruction)
{
    m_instructionCounts[std::move(instruction)]++;
}


Json::Value InstructionStats::toJson() const
{
    uint64_t changesCount = 0;
    uint64_t writesCount = 0;
    uint64_t readsCount = 0;
    uint64_t storageAllocated = 0;
    for (auto& changeKv : m_changes)
    {
        auto& change = changeKv.second;
        changesCount += change.changesCount;
        writesCount += change.writesCount;
        readsCount += change.readsCount;
        if (change.initialValue == 0 && change.newValue != 0)
        {
            storageAllocated++;
        }
        else if (change.initialValue != 0 && change.newValue == 0)
        {
            storageAllocated--;
        }
    }

    uint64_t creationSize = 0;
    for (const u256& size : m_createCalls)
    {
        creationSize += size.convert_to<uint64_t>();
    }

    Json::Value result;
    result["storage.changesCount"] = changesCount;
    result["storage.writesCount"] = writesCount;
    result["storage.readsCount"] = readsCount;
    result["storage.allocated"] = storageAllocated;
    result["contracts.creationCount"] = m_createCalls.size();
    result["contracts.creationSize"] = creationSize;
    result["suicideCount"] = m_suicideCallsCount;

    result["calls"] = Json::Value();
    for (auto& kv : m_instructionCounts)
    {
        auto info = instructionInfo(kv.first);
        result["calls"][info.name] = kv.second;
    }

    return result;
}

}  // namespace eth
}  // namespace dev
