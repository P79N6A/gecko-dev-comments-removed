





#include "jit/arm64/Architecture-arm64.h"

#include <cstring>

#include "jit/RegisterSets.h"

namespace js {
namespace jit {

Registers::Code
Registers::FromName(const char* name)
{
    
    if (strcmp(name, "ip0") == 0)
        return ip0;
    if (strcmp(name, "ip1") == 0)
        return ip1;
    if (strcmp(name, "fp") == 0)
        return fp;

    for (uint32_t i = 0; i < Total; i++) {
        if (strcmp(GetName(Code(i)), name) == 0)
            return Code(i);
    }

    return invalid_reg;
}

FloatRegisters::Code
FloatRegisters::FromName(const char* name)
{
    for (size_t i = 0; i < Total; i++) {
        if (strcmp(GetName(Code(i)), name) == 0)
            return Code(i);
    }

    return invalid_fpreg;
}

FloatRegisterSet
FloatRegister::ReduceSetForPush(const FloatRegisterSet& s)
{
    LiveFloatRegisterSet ret;
    for (FloatRegisterIterator iter(s); iter.more(); ++iter)
        ret.addUnchecked(FromCode((*iter).encoding()));
    return ret.set();
}

uint32_t
FloatRegister::GetSizeInBytes(const FloatRegisterSet& s)
{
    return s.size() * sizeof(double);
}

uint32_t
FloatRegister::GetPushSizeInBytes(const FloatRegisterSet& s)
{
    return s.size() * sizeof(double);
}

uint32_t
FloatRegister::getRegisterDumpOffsetInBytes()
{
    
    return encoding() * sizeof(double);
}

} 
} 
