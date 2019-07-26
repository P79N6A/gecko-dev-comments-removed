





#define HWCAP_ARMv7 (1 << 31)
#include <mozilla/StandardInteger.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>




#include <asm/hwcap.h>
#include "ion/arm/Architecture-arm.h"
#include "ion/arm/Assembler-arm.h"
namespace js {
namespace ion {

uint32_t getFlags()
{
    static bool isSet = false;
    static uint32_t flags = 0;
    if (isSet)
        return flags;

#if WTF_OS_LINUX
    int fd = open("/proc/self/auxv", O_RDONLY);
    if (fd > 0) {
        Elf32_auxv_t aux;
        while (read(fd, &aux, sizeof(Elf32_auxv_t))) {
            if (aux.a_type == AT_HWCAP) {
                close(fd);
                flags = aux.a_un.a_val;
                isSet = true;
#ifdef __ARM_ARCH_7__
                
                
                
                
                
                flags |= HWCAP_ARMv7;
#endif
                return flags;
            }
        }
        close(fd);
    }
#elif defined(WTF_OS_ANDROID)
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp)
        return false;

    char buf[1024];
    fread(buf, sizeof(char), sizeof(buf), fp);
    fclose(fp);
    if (strstr(buf, "vfp"))
        flags |= HWCAP_VFP;

    if (strstr(buf, "vfpv3"))
        flags |= HWCAP_VFPv3;

    if (strstr(buf, "vfpv3d16"))
        flags |= HWCAP_VFPv3;

    if (strstr(buf, "vfpv4"))
        flags |= HWCAP_VFPv4;

    if (strstr(buf, "idiva"))
        flags |= HWCAP_IDIVA;

    if (strstr(buf, "idivt"))
        flags |= HWCAP_IDIVT;

    if (strstr(buf, "neon"))
        flags |= HWCAP_NEON;

    
    
    if (strstr(buf, "ARMv7"))
        flags |= HWCAP_ARMv7;

    isSet = true;
    return flags;
#endif

    return false;
}

bool hasMOVWT()
{
    return js::ion::getFlags() & HWCAP_ARMv7;
}
bool hasVFPv3()
{
    return js::ion::getFlags() & HWCAP_VFPv3;
}
bool hasVFP()
{
    return js::ion::getFlags() & HWCAP_VFP;
}

bool has16DP()
{
    return js::ion::getFlags() & HWCAP_VFPv3D16 && !(js::ion::getFlags() & HWCAP_NEON);
}
bool useConvReg()
{
    return !has16DP();
}

} 
} 

