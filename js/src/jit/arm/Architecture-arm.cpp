





#include "jit/arm/Architecture-arm.h"

#ifndef JS_ARM_SIMULATOR
#include <elf.h>
#endif

#include <fcntl.h>
#include <unistd.h>

#include "jit/arm/Assembler-arm.h"
#include "jit/RegisterSets.h"

#define HWCAP_USE_HARDFP_ABI (1 << 27)

#if !(defined(ANDROID) || defined(MOZ_B2G)) && !defined(JS_ARM_SIMULATOR)
#define HWCAP_ARMv7 (1 << 28)
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
namespace jit {



static uint32_t armHwCapFlags = 0;

bool
ParseARMHwCapFlags(const char *armHwCap)
{
    uint32_t flags = 0;

    if (!armHwCap || !armHwCap[0])
        return false;

#ifdef JS_CODEGEN_ARM_HARDFP
    flags |= HWCAP_USE_HARDFP_ABI;
#endif

    if (strstr(armHwCap, "help")) {
        fflush(NULL);
        printf(
               "\n"
               "usage: ARMHWCAP=option,option,option,... where options can be:\n"
               "\n"
               "  armv7    \n"
               "  vfp      \n"
               "  neon     \n"
               "  vfpv3    \n"
               "  vfpv3d16 \n"
               "  vfpv4    \n"
               "  idiva    \n"
               "  idivt    \n"
#if defined(JS_ARM_SIMULATOR)
               "  hardfp   \n"
#endif
               "\n"
               );
        exit(0);
        
    }

    
    const char *start = armHwCap;  
    for (;;) {
        char  ch = *start;
        if (!ch) {
            
            break;
        }
        if (ch == ' ' || ch == ',') {
            
            start++;
            continue;
        }
        
        const char *end = start + 1;
        for (; ; end++) {
            ch = *end;
            if (!ch || ch == ' ' || ch == ',')
                break;
        }
        size_t count = end - start;
        if (count == 3 && strncmp(start, "vfp", 3) == 0)
            flags |= HWCAP_VFP;
        else if (count == 5 && strncmp(start, "vfpv3", 5) == 0)
            flags |= HWCAP_VFPv3;
        else if (count == 8 && strncmp(start, "vfpv3d16", 8) == 0)
            flags |= HWCAP_VFPv3D16;
        else if (count == 5 && strncmp(start, "vfpv4", 5) == 0)
            flags |= HWCAP_VFPv4;
        else if (count == 5 && strncmp(start, "idiva", 5) == 0)
            flags |= HWCAP_IDIVA;
        else if (count == 5 && strncmp(start, "idivt", 5) == 0)
            flags |= HWCAP_IDIVT;
        else if (count == 4 && strncmp(start, "neon", 4) == 0)
            flags |= HWCAP_NEON;
        else if (count == 5 && strncmp(start, "armv7", 5) == 0)
            flags |= HWCAP_ARMv7;
#if defined(JS_ARM_SIMULATOR)
        else if (count == 6 && strncmp(start, "hardfp", 6) == 0)
            flags |= HWCAP_USE_HARDFP_ABI;
#endif
        else
            fprintf(stderr, "Warning: unexpected ARMHWCAP flag at: %s\n", start);
        start = end;
    }
#ifdef DEBUG
    IonSpew(IonSpew_Codegen, "ARMHWCAP: '%s'\n   flags: 0x%x\n", armHwCap, flags);
#endif
    armHwCapFlags = flags;
    return true;
}

uint32_t GetARMFlags()
{
    static bool isSet = false;
    static uint32_t flags = 0;
    if (isSet)
        return flags;

    const char *env = getenv("ARMHWCAP");
    if (ParseARMHwCapFlags(env) || armHwCapFlags) {
        isSet = true;
        flags = armHwCapFlags;
        return flags;
    }

#ifdef JS_CODEGEN_ARM_HARDFP
    flags |= HWCAP_USE_HARDFP_ABI;
#endif

#ifdef JS_ARM_SIMULATOR
    isSet = true;
    flags = HWCAP_ARMv7 | HWCAP_VFP | HWCAP_VFPv3 | HWCAP_VFPv4 | HWCAP_NEON;
    return flags;
#else

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

#if defined(__ARM_ARCH_7__) || defined (__ARM_ARCH_7A__)
    flags = HWCAP_ARMv7;
#endif
    isSet = true;
    return flags;

#elif defined(WTF_OS_ANDROID) || defined(MOZ_B2G)
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp)
        return false;

    char buf[1024];
    memset(buf, 0, sizeof(buf));
    size_t len = fread(buf, sizeof(char), sizeof(buf) - 2, fp);
    fclose(fp);
    
    buf[len] = ' ';
    buf[len + 1] = '\0';
    for (size_t i = 0; i < len; i++) {
        char  ch = buf[i];
        if (!ch)
            break;
        else if (ch == '\n')
            buf[i] = 0x20;
        else
            buf[i] = ch;
    }

    if (strstr(buf, " vfp "))
        flags |= HWCAP_VFP;

    if (strstr(buf, " vfpv3 "))
        flags |= HWCAP_VFPv3;

    if (strstr(buf, " vfpv3d16 "))
        flags |= HWCAP_VFPv3D16;

    if (strstr(buf, " vfpv4 "))
        flags |= HWCAP_VFPv4;

    if (strstr(buf, " idiva "))
        flags |= HWCAP_IDIVA;

    if (strstr(buf, " idivt "))
        flags |= HWCAP_IDIVT;

    if (strstr(buf, " neon "))
        flags |= HWCAP_NEON;

    
    
    if (strstr(buf, "ARMv7"))
        flags |= HWCAP_ARMv7;

#ifdef DEBUG
    IonSpew(IonSpew_Codegen, "ARMHWCAP: '%s'\n   flags: 0x%x\n", buf, flags);
#endif

    isSet = true;
    return flags;
#endif

    return 0;
#endif 
}

bool HasMOVWT()
{
    return GetARMFlags() & HWCAP_ARMv7;
}
bool HasVFPv3()
{
    return GetARMFlags() & HWCAP_VFPv3;
}
bool HasVFP()
{
    return GetARMFlags() & HWCAP_VFP;
}

bool Has32DP()
{
    return (GetARMFlags() & HWCAP_VFPv3) && !(GetARMFlags() & HWCAP_VFPv3D16);
}
bool UseConvReg()
{
    return Has32DP();
}

bool HasIDIV()
{
#if defined HWCAP_IDIVA
    return GetARMFlags() & HWCAP_IDIVA;
#else
    return false;
#endif
}


#if defined(JS_ARM_SIMULATOR)
bool UseHardFpABI()
{
    return GetARMFlags() & HWCAP_USE_HARDFP_ABI;
}
#endif

Registers::Code
Registers::FromName(const char *name)
{
    
    if (strcmp(name, "ip") == 0)
        return ip;
    if (strcmp(name, "r13") == 0)
        return r13;
    if (strcmp(name, "lr") == 0)
        return lr;
    if (strcmp(name, "r15") == 0)
        return r15;

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

FloatRegisterSet
VFPRegister::ReduceSetForPush(const FloatRegisterSet &s)
{
    FloatRegisterSet mod;
    for (TypedRegisterIterator<FloatRegister> iter(s); iter.more(); iter++) {
        if ((*iter).isSingle()) {
            
            mod.addUnchecked(*iter);
        } else if ((*iter).id() < 16) {
            
            mod.addUnchecked((*iter).singleOverlay(0));
            mod.addUnchecked((*iter).singleOverlay(1));
        } else {
            
            mod.addUnchecked(*iter);
        }
    }
    return mod;
}

uint32_t
VFPRegister::GetSizeInBytes(const FloatRegisterSet &s)
{
    uint64_t bits = s.bits();
    uint32_t ret = mozilla::CountPopulation32(bits&0xffffffff) * sizeof(float);
    ret +=  mozilla::CountPopulation32(bits >> 32) * sizeof(double);
    return ret;
}
uint32_t
VFPRegister::GetPushSizeInBytes(const FloatRegisterSet &s)
{
    FloatRegisterSet ss = s.reduceSetForPush();
    uint64_t bits = ss.bits();
    uint32_t ret = mozilla::CountPopulation32(bits&0xffffffff) * sizeof(float);
    ret +=  mozilla::CountPopulation32(bits >> 32) * sizeof(double);
    return ret;
}
uint32_t
VFPRegister::getRegisterDumpOffsetInBytes()
{
    if (isSingle())
        return id() * sizeof(float);
    if (isDouble())
        return id() * sizeof(double);
    MOZ_ASSUME_UNREACHABLE();
}

uint32_t
FloatRegisters::ActualTotalPhys()
{
    if (Has32DP())
        return 32;
    return 16;
}


} 
} 

