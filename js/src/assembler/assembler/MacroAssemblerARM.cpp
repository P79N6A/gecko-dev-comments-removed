





























#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER && WTF_CPU_ARM_TRADITIONAL

#include "assembler/assembler/MacroAssemblerARM.h"

#if (WTF_OS_LINUX || WTF_OS_ANDROID) && !defined(JS_ARM_SIMULATOR)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <stdio.h>



#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
#include <asm/procinfo.h>
#else
#include <asm/hwcap.h>
#endif

#endif

namespace JSC {

static bool isVFPPresent()
{
#ifdef JS_ARM_SIMULATOR
    return true;
#else
#if WTF_OS_LINUX
    int fd = open("/proc/self/auxv", O_RDONLY);
    if (fd > 0) {
        Elf32_auxv_t aux;
        while (read(fd, &aux, sizeof(Elf32_auxv_t))) {
            if (aux.a_type == AT_HWCAP) {
                close(fd);
                return aux.a_un.a_val & HWCAP_VFP;
            }
        }
        close(fd);
    }
#endif

#if defined(__GNUC__) && defined(__VFP_FP__)
    return true;
#endif

#ifdef WTF_OS_ANDROID
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp)
        return false;

    char buf[1024];
    fread(buf, sizeof(char), sizeof(buf), fp);
    fclose(fp);
    if (strstr(buf, "vfp"))
        return true;
#endif
    return false;
#endif 
}

const bool MacroAssemblerARM::s_isVFPPresent = isVFPPresent();

}

#endif 
