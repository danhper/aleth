#include "ExtendedInstruction.h"


namespace dev
{
namespace eth
{
std::string instructionName(ExtendedInstruction einstruction)
{
    switch (einstruction)
    {
    case ExtendedInstruction::PRECOMPILED_ECRECOVER:
        return "PRECOMPILED_ECRECOVER";
    case ExtendedInstruction::PRECOMPILED_SHA256:
        return "PRECOMPILED_SHA256";
    case ExtendedInstruction::PRECOMPILED_RIPEMD160:
        return "PRECOMPILED_RIPEMD160";
    default:
        auto info = instructionInfo(static_cast<Instruction>(einstruction));
        return std::string(info.name);
    }
}

ExtendedInstruction fromInstruction(Instruction instruction)
{
    return static_cast<ExtendedInstruction>(instruction);
}


ExtendedInstruction fromInstruction(Instruction instruction, u256s stack)
{
    if (instruction == Instruction::CALL && stack.size() >= 2 && stack[1] <= 3)
    {
        auto address = stack[1].convert_to<uint8_t>();
        switch (address)
        {
        case 1:
            return ExtendedInstruction::PRECOMPILED_ECRECOVER;
        case 2:
            return ExtendedInstruction::PRECOMPILED_SHA256;
        case 3:
            return ExtendedInstruction::PRECOMPILED_RIPEMD160;
        default:
            break;
        }
    }
    return static_cast<ExtendedInstruction>(instruction);
}

}  // namespace eth
}  // namespace dev
