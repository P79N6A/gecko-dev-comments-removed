





#include "jit/mips/Architecture-mips.h"

#include <fcntl.h>
#include <unistd.h>

#include "jit/RegisterSets.h"

#define HWCAP_MIPS (1 << 31)
#define HWCAP_FPU (1 << 0)

namespace js {
namespace jit {

uint32_t GetMIPSFlags()
{
    static bool isSet = false;
    static uint32_t flags = 0;
    if (isSet)
        return flags;
#ifdef JS_MIPS_SIMULATOR
    isSet = true;
    flags |= HWCAP_FPU;
    return flags;
#else

#if WTF_OS_LINUX
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp)
        return false;

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    fread(buf, sizeof(char), sizeof(buf) - 1, fp);
    fclose(fp);
    if (strstr(buf, "FPU"))
        flags |= HWCAP_FPU;

    isSet = true;
    return flags;
#endif

    return false;
#endif 
}

bool hasFPU()
{
    return js::jit::GetMIPSFlags() & HWCAP_FPU;
}

Registers::Code
Registers::FromName(const char *name)
{
    for (size_t i = 0; i < Total; i++) {
        if (strcmp(GetName(i), name) == 0)
            return Code(i);
    }

    return Invalid;
}

FloatRegisters::Code
FloatRegisters::FromName(const char *name)
{
    for (size_t i = 0; i < Total; i++) {
        if (strcmp(GetName(i), name) == 0)
            return Code(i);
    }

    return Invalid;
}



} 
} 

