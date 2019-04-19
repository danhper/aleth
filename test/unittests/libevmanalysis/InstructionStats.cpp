#include <libevmanalysis/InstructionStats.h>

#include <gtest/gtest.h>

using namespace std;
using namespace dev;
using namespace eth;

TEST(InstructionStats, recordInstruction)
{
    auto stats = InstructionStats();
    stats.recordInstruction(Instruction::ADD);
    stats.recordInstruction(Instruction::ADD);
    stats.recordInstruction(Instruction::SUB);
    auto json = stats.toJson();
    EXPECT_EQ(json["calls"].size(), 2);
    EXPECT_EQ(json["calls"]["ADD"].asUInt64(), 2);
    EXPECT_EQ(json["calls"]["SUB"].asUInt64(), 1);
}

TEST(InstructionStats, recordRead)
{
    auto stats = InstructionStats();
    stats.recordRead(1);
    stats.recordRead(2);
    stats.recordRead(1);
    auto json = stats.toJson();
    EXPECT_EQ(json["storage.readsCount"].asUInt64(), 3);
}


TEST(InstructionStats, recordWrite)
{
    auto stats = InstructionStats();
    stats.recordWrite(1, 2, 2, 4);
    stats.recordWrite(1, 2, 4, 4);
    stats.recordWrite(1, 2, 4, 8);
    stats.recordWrite(2, 0, 0, 4);
    auto json = stats.toJson();
    EXPECT_EQ(json["storage.writesCount"].asUInt64(), 4);
    EXPECT_EQ(json["storage.changesCount"].asUInt64(), 3);
    EXPECT_EQ(json["storage.allocated"].asUInt64(), 1);
}
