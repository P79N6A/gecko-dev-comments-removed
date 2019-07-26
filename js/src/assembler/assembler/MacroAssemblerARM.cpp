





























#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER && WTF_CPU_ARM_TRADITIONAL

#include "assembler/assembler/MacroAssemblerARM.h"

#if (WTF_OS_LINUX || WTF_OS_ANDROID) && !defined(JS_ARM_SIMULATOR)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>



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

#if WTF_CPU_ARMV5_OR_LOWER

void MacroAssemblerARM::load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
{
    ARMWord op2;

    ASSERT(address.scale >= 0 && address.scale <= 3);
    op2 = m_assembler.lsl(address.index, static_cast<int>(address.scale));

    if (address.offset >= 0 && address.offset + 0x2 <= 0xff) {
        m_assembler.add_r(ARMRegisters::S0, address.base, op2);
        m_assembler.ldrh_u(dest, ARMRegisters::S0, ARMAssembler::getOp2Byte(address.offset));
        m_assembler.ldrh_u(ARMRegisters::S0, ARMRegisters::S0, ARMAssembler::getOp2Byte(address.offset + 0x2));
    } else if (address.offset < 0 && address.offset >= -0xff) {
        m_assembler.add_r(ARMRegisters::S0, address.base, op2);
        m_assembler.ldrh_d(dest, ARMRegisters::S0, ARMAssembler::getOp2Byte(-address.offset));
        m_assembler.ldrh_d(ARMRegisters::S0, ARMRegisters::S0, ARMAssembler::getOp2Byte(-address.offset - 0x2));
    } else {
        m_assembler.ldr_un_imm(ARMRegisters::S0, address.offset);
        m_assembler.add_r(ARMRegisters::S0, ARMRegisters::S0, op2);
        m_assembler.ldrh_r(dest, address.base, ARMRegisters::S0);
        m_assembler.add_r(ARMRegisters::S0, ARMRegisters::S0, ARMAssembler::OP2_IMM | 0x2);
        m_assembler.ldrh_r(ARMRegisters::S0, address.base, ARMRegisters::S0);
    }
    m_assembler.orr_r(dest, dest, m_assembler.lsl(ARMRegisters::S0, 16));
}
#endif

}

#endif 
