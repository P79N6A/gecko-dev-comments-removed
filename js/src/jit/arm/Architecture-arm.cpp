





#include "jit/arm/Architecture-arm.h"

#ifndef JS_ARM_SIMULATOR
#include <elf.h>
#endif

#include <fcntl.h>
#include <unistd.h>

#include "jit/arm/Assembler-arm.h"
#include "jit/RegisterSets.h"

#if defined(ANDROID) || defined(JS_ARM_SIMULATOR)


# define HWCAP_VFP        (1 << 6)
# define HWCAP_NEON       (1 << 12)
# define HWCAP_VFPv3      (1 << 13)
# define HWCAP_VFPv3D16   (1 << 14) /* also set for VFPv4-D16 */
# define HWCAP_VFPv4      (1 << 16)
# define HWCAP_IDIVA      (1 << 17)
# define HWCAP_IDIVT      (1 << 18)
# define HWCAP_VFPD32     (1 << 19) /* set if VFP has 32 regs (not 16) */
# define AT_HWCAP 16
#else
# include <asm/hwcap.h>
# if !defined(HWCAP_IDIVA)
#  define HWCAP_IDIVA     (1 << 17)
# endif
# if !defined(HWCAP_VFPD32)
#  define HWCAP_VFPD32    (1 << 19) /* set if VFP has 32 regs (not 16) */
# endif
#endif




#define HWCAP_ARMv7 (1 << 28)


#define HWCAP_USE_HARDFP_ABI (1 << 27)


#define HWCAP_ALIGNMENT_FAULT (1 << 26)


#define HWCAP_UNINITIALIZED (1 << 25)

namespace js {
namespace jit {




static uint32_t
ParseARMCpuFeatures(const char *features, bool override = false)
{
    uint32_t flags = 0;

    for (;;) {
        char  ch = *features;
        if (!ch) {
            
            break;
        }
        if (ch == ' ' || ch == ',') {
            
            features++;
            continue;
        }
        
        const char *end = features + 1;
        for (; ; end++) {
            ch = *end;
            if (!ch || ch == ' ' || ch == ',')
                break;
        }
        size_t count = end - features;
        if (count == 3 && strncmp(features, "vfp", 3) == 0)
            flags |= HWCAP_VFP;
        else if (count == 4 && strncmp(features, "neon", 4) == 0)
            flags |= HWCAP_NEON;
        else if (count == 5 && strncmp(features, "vfpv3", 5) == 0)
            flags |= HWCAP_VFPv3;
        else if (count == 8 && strncmp(features, "vfpv3d16", 8) == 0)
            flags |= HWCAP_VFPv3D16;
        else if (count == 5 && strncmp(features, "vfpv4", 5) == 0)
            flags |= HWCAP_VFPv4;
        else if (count == 5 && strncmp(features, "idiva", 5) == 0)
            flags |= HWCAP_IDIVA;
        else if (count == 5 && strncmp(features, "idivt", 5) == 0)
            flags |= HWCAP_IDIVT;
        else if (count == 6 && strncmp(features, "vfpd32", 6) == 0)
            flags |= HWCAP_VFPD32;
        else if (count == 5 && strncmp(features, "armv7", 5) == 0)
            flags |= HWCAP_ARMv7;
        else if (count == 5 && strncmp(features, "align", 5) == 0)
            flags |= HWCAP_ALIGNMENT_FAULT;
#if defined(JS_ARM_SIMULATOR)
        else if (count == 6 && strncmp(features, "hardfp", 6) == 0)
            flags |= HWCAP_USE_HARDFP_ABI;
#endif
        else if (override)
            fprintf(stderr, "Warning: unexpected ARM feature at: %s\n", features);
        features = end;
    }
    return flags;
}

static uint32_t
CanonicalizeARMHwCapFlags(uint32_t flags)
{
    
    

    
    
    if (flags & HWCAP_VFPv3D16)
        flags |= HWCAP_VFPv3;

    
    if (flags & (HWCAP_VFPv3 | HWCAP_NEON))
        flags |= HWCAP_ARMv7;

    
    
    if (flags & HWCAP_VFP && flags & HWCAP_ARMv7)
        flags |= HWCAP_VFPv3;

    
    if ((flags & HWCAP_VFPv3) && !(flags & HWCAP_VFPv3D16))
        flags |= HWCAP_VFPD32;

    return flags;
}



volatile static uint32_t armHwCapFlags = HWCAP_UNINITIALIZED;

bool
ParseARMHwCapFlags(const char *armHwCap)
{
    uint32_t flags = 0;

    if (!armHwCap)
        return false;

    if (strstr(armHwCap, "help")) {
        fflush(NULL);
        printf(
               "\n"
               "usage: ARMHWCAP=option,option,option,... where options can be:\n"
               "\n"
               "  vfp      \n"
               "  neon     \n"
               "  vfpv3    \n"
               "  vfpv3d16 \n"
               "  vfpv4    \n"
               "  idiva    \n"
               "  idivt    \n"
               "  vfpd32   \n"
               "  armv7    \n"
               "  align    \n"
#if defined(JS_ARM_SIMULATOR)
               "  hardfp   \n"
#endif
               "\n"
               );
        exit(0);
        
    }

    flags = ParseARMCpuFeatures(armHwCap,  true);

#ifdef JS_CODEGEN_ARM_HARDFP
    flags |= HWCAP_USE_HARDFP_ABI;
#endif

    armHwCapFlags = CanonicalizeARMHwCapFlags(flags);
    JitSpew(JitSpew_Codegen, "ARM HWCAP: 0x%x\n", armHwCapFlags);
    return true;
}

void
InitARMFlags()
{
    uint32_t flags = 0;

    if (armHwCapFlags != HWCAP_UNINITIALIZED)
        return;

    const char *env = getenv("ARMHWCAP");
    if (ParseARMHwCapFlags(env))
        return;

#ifdef JS_ARM_SIMULATOR
    flags = HWCAP_ARMv7 | HWCAP_VFP | HWCAP_VFPv3 | HWCAP_VFPv4 | HWCAP_NEON;
#else

#if defined(__linux__)
    
    bool readAuxv = false;
    int fd = open("/proc/self/auxv", O_RDONLY);
    if (fd > 0) {
        struct { uint32_t a_type; uint32_t a_val; } aux;
        while (read(fd, &aux, sizeof(aux))) {
            if (aux.a_type == AT_HWCAP) {
                flags = aux.a_val;
                readAuxv = true;
                break;
            }
        }
        close(fd);
    }

    if (!readAuxv) {
        
        FILE *fp = fopen("/proc/cpuinfo", "r");
        if (fp) {
            char buf[1024];
            memset(buf, 0, sizeof(buf));
            size_t len = fread(buf, sizeof(char), sizeof(buf) - 1, fp);
            fclose(fp);
            buf[len] = '\0';
            char *featureList = strstr(buf, "Features");
            if (featureList) {
                if (char *featuresEnd = strstr(featureList, "\n"))
                    *featuresEnd = '\0';
                flags = ParseARMCpuFeatures(featureList + 8);
            }
            if (strstr(buf, "ARMv7"))
                flags |= HWCAP_ARMv7;
        }
    }
#endif

    
    

#ifdef JS_CODEGEN_ARM_HARDFP
    
    flags |= HWCAP_USE_HARDFP_ABI;
#endif

#if defined(__VFP_FP__) && !defined(__SOFTFP__)
    
    flags |= HWCAP_VFP;
#endif

#if defined(__ARM_ARCH_7__) || defined (__ARM_ARCH_7A__)
    
    flags |= HWCAP_ARMv7;
#endif

#endif 

    armHwCapFlags = CanonicalizeARMHwCapFlags(flags);

    JitSpew(JitSpew_Codegen, "ARM HWCAP: 0x%x\n", armHwCapFlags);
    return;
}

uint32_t
GetARMFlags()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags;
}

bool HasMOVWT()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_ARMv7;
}

bool HasLDSTREXBHD()
{
    
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_ARMv7;
}

bool HasDMBDSBISB()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_ARMv7;
}

bool HasVFPv3()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_VFPv3;
}

bool HasVFP()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_VFP;
}

bool Has32DP()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_VFPD32;
}

bool HasIDIV()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_IDIVA;
}



bool HasAlignmentFault()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_ALIGNMENT_FAULT;
}


#if defined(JS_ARM_SIMULATOR)
bool UseHardFpABI()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_USE_HARDFP_ABI;
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
    MOZ_CRASH("not Single or Double");
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

