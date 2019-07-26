





#include "ion/arm/Architecture-arm.h"

#include "mozilla/StandardInteger.h"

#include <elf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define HWCAP_ARMv7 (1 << 31)



#if !(defined(ANDROID) || defined(MOZ_B2G))
#include <asm/hwcap.h>
#else
#define HWCAP_VFP      (1<<0)
#define HWCAP_VFPv3    (1<<1)
#define HWCAP_VFPv3D16 (1<<2)
#define HWCAP_VFPv4    (1<<3)
#define HWCAP_IDIVA    (1<<4)
#define HWCAP_IDIVT    (1<<5)
#define HWCAP_NEON     (1<<6)
#define HWCAP_ARMv7    (1<<7)
#endif

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
#if defined(__ARM_ARCH_7__) || defined (__ARM_ARCH_7A__)
                
                
                
                
                
                flags |= HWCAP_ARMv7;
#endif
                return flags;
            }
        }
        close(fd);
    }
#elif defined(WTF_OS_ANDROID) || defined(MOZ_B2G)
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
        flags |= HWCAP_VFPv3D16;

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

bool has32DP()
{
    return !(js::ion::getFlags() & HWCAP_VFPv3D16 && !(js::ion::getFlags() & HWCAP_NEON));
}
bool useConvReg()
{
    return has32DP();
}

bool hasIDIV()
{
#if defined HWCAP_IDIVA
    return js::ion::getFlags() & HWCAP_IDIVA;
#else
    return false;
#endif
}

} 
} 

