

















#include "asmjs/AsmJSSignalHandlers.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/PodOperations.h"

#include "asmjs/AsmJSModule.h"
#include "jit/Disassembler.h"
#include "vm/Runtime.h"

using namespace js;
using namespace js::jit;

using JS::GenericNaN;
using mozilla::DebugOnly;
using mozilla::PodArrayZero;

#if defined(ANDROID)
# include <sys/system_properties.h>
# if defined(MOZ_LINKER)
extern "C" MFBT_API bool IsSignalHandlingBroken();
# endif
#endif




static JSRuntime*
RuntimeForCurrentThread()
{
    PerThreadData* threadData = TlsPerThreadData.get();
    if (!threadData)
        return nullptr;

    return threadData->runtimeIfOnOwnerThread();
}






class AutoSetHandlingSignal
{
    JSRuntime* rt;

  public:
    explicit AutoSetHandlingSignal(JSRuntime* rt)
      : rt(rt)
    {
        MOZ_ASSERT(!rt->handlingSignal);
        rt->handlingSignal = true;
    }

    ~AutoSetHandlingSignal()
    {
        MOZ_ASSERT(rt->handlingSignal);
        rt->handlingSignal = false;
    }
};

#if defined(XP_WIN)
# define XMM_sig(p,i) ((p)->Xmm##i)
# define EIP_sig(p) ((p)->Eip)
# define RIP_sig(p) ((p)->Rip)
# define RAX_sig(p) ((p)->Rax)
# define RCX_sig(p) ((p)->Rcx)
# define RDX_sig(p) ((p)->Rdx)
# define RBX_sig(p) ((p)->Rbx)
# define RSP_sig(p) ((p)->Rsp)
# define RBP_sig(p) ((p)->Rbp)
# define RSI_sig(p) ((p)->Rsi)
# define RDI_sig(p) ((p)->Rdi)
# define R8_sig(p) ((p)->R8)
# define R9_sig(p) ((p)->R9)
# define R10_sig(p) ((p)->R10)
# define R11_sig(p) ((p)->R11)
# define R12_sig(p) ((p)->R12)
# define R13_sig(p) ((p)->R13)
# define R14_sig(p) ((p)->R14)
# define R15_sig(p) ((p)->R15)
#elif defined(__OpenBSD__)
# define XMM_sig(p,i) ((p)->sc_fpstate->fx_xmm[i])
# define EIP_sig(p) ((p)->sc_eip)
# define RIP_sig(p) ((p)->sc_rip)
# define RAX_sig(p) ((p)->sc_rax)
# define RCX_sig(p) ((p)->sc_rcx)
# define RDX_sig(p) ((p)->sc_rdx)
# define RBX_sig(p) ((p)->sc_rbx)
# define RSP_sig(p) ((p)->sc_rsp)
# define RBP_sig(p) ((p)->sc_rbp)
# define RSI_sig(p) ((p)->sc_rsi)
# define RDI_sig(p) ((p)->sc_rdi)
# define R8_sig(p) ((p)->sc_r8)
# define R9_sig(p) ((p)->sc_r9)
# define R10_sig(p) ((p)->sc_r10)
# define R11_sig(p) ((p)->sc_r11)
# define R12_sig(p) ((p)->sc_r12)
# define R13_sig(p) ((p)->sc_r13)
# define R14_sig(p) ((p)->sc_r14)
# define R15_sig(p) ((p)->sc_r15)
#elif defined(__linux__) || defined(SOLARIS)
# if defined(__linux__)
#  define XMM_sig(p,i) ((p)->uc_mcontext.fpregs->_xmm[i])
#  define EIP_sig(p) ((p)->uc_mcontext.gregs[REG_EIP])
# else
#  define XMM_sig(p,i) ((p)->uc_mcontext.fpregs.fp_reg_set.fpchip_state.xmm[i])
#  define EIP_sig(p) ((p)->uc_mcontext.gregs[REG_PC])
# endif
# define RIP_sig(p) ((p)->uc_mcontext.gregs[REG_RIP])
# define RAX_sig(p) ((p)->uc_mcontext.gregs[REG_RAX])
# define RCX_sig(p) ((p)->uc_mcontext.gregs[REG_RCX])
# define RDX_sig(p) ((p)->uc_mcontext.gregs[REG_RDX])
# define RBX_sig(p) ((p)->uc_mcontext.gregs[REG_RBX])
# define RSP_sig(p) ((p)->uc_mcontext.gregs[REG_RSP])
# define RBP_sig(p) ((p)->uc_mcontext.gregs[REG_RBP])
# define RSI_sig(p) ((p)->uc_mcontext.gregs[REG_RSI])
# define RDI_sig(p) ((p)->uc_mcontext.gregs[REG_RDI])
# define R8_sig(p) ((p)->uc_mcontext.gregs[REG_R8])
# define R9_sig(p) ((p)->uc_mcontext.gregs[REG_R9])
# define R10_sig(p) ((p)->uc_mcontext.gregs[REG_R10])
# define R11_sig(p) ((p)->uc_mcontext.gregs[REG_R11])
# define R12_sig(p) ((p)->uc_mcontext.gregs[REG_R12])
# define R13_sig(p) ((p)->uc_mcontext.gregs[REG_R13])
# define R14_sig(p) ((p)->uc_mcontext.gregs[REG_R14])
# if defined(__linux__) && defined(__arm__)
#  define R15_sig(p) ((p)->uc_mcontext.arm_pc)
# else
#  define R15_sig(p) ((p)->uc_mcontext.gregs[REG_R15])
# endif
# if defined(__linux__) && defined(__mips__)
#  define EPC_sig(p) ((p)->uc_mcontext.pc)
#  define RSP_sig(p) ((p)->uc_mcontext.gregs[29])
#  define RFP_sig(p) ((p)->uc_mcontext.gregs[30])
# endif
#elif defined(__NetBSD__)
# define XMM_sig(p,i) (((struct fxsave64*)(p)->uc_mcontext.__fpregs)->fx_xmm[i])
# define EIP_sig(p) ((p)->uc_mcontext.__gregs[_REG_EIP])
# define RIP_sig(p) ((p)->uc_mcontext.__gregs[_REG_RIP])
# define RAX_sig(p) ((p)->uc_mcontext.__gregs[_REG_RAX])
# define RCX_sig(p) ((p)->uc_mcontext.__gregs[_REG_RCX])
# define RDX_sig(p) ((p)->uc_mcontext.__gregs[_REG_RDX])
# define RBX_sig(p) ((p)->uc_mcontext.__gregs[_REG_RBX])
# define RSP_sig(p) ((p)->uc_mcontext.__gregs[_REG_RSP])
# define RBP_sig(p) ((p)->uc_mcontext.__gregs[_REG_RBP])
# define RSI_sig(p) ((p)->uc_mcontext.__gregs[_REG_RSI])
# define RDI_sig(p) ((p)->uc_mcontext.__gregs[_REG_RDI])
# define R8_sig(p) ((p)->uc_mcontext.__gregs[_REG_R8])
# define R9_sig(p) ((p)->uc_mcontext.__gregs[_REG_R9])
# define R10_sig(p) ((p)->uc_mcontext.__gregs[_REG_R10])
# define R11_sig(p) ((p)->uc_mcontext.__gregs[_REG_R11])
# define R12_sig(p) ((p)->uc_mcontext.__gregs[_REG_R12])
# define R13_sig(p) ((p)->uc_mcontext.__gregs[_REG_R13])
# define R14_sig(p) ((p)->uc_mcontext.__gregs[_REG_R14])
# define R15_sig(p) ((p)->uc_mcontext.__gregs[_REG_R15])
#elif defined(__DragonFly__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
# if defined(__DragonFly__)
#  define XMM_sig(p,i) (((union savefpu*)(p)->uc_mcontext.mc_fpregs)->sv_xmm.sv_xmm[i])
# else
#  define XMM_sig(p,i) (((struct savefpu*)(p)->uc_mcontext.mc_fpstate)->sv_xmm[i])
# endif
# define EIP_sig(p) ((p)->uc_mcontext.mc_eip)
# define RIP_sig(p) ((p)->uc_mcontext.mc_rip)
# define RAX_sig(p) ((p)->uc_mcontext.mc_rax)
# define RCX_sig(p) ((p)->uc_mcontext.mc_rcx)
# define RDX_sig(p) ((p)->uc_mcontext.mc_rdx)
# define RBX_sig(p) ((p)->uc_mcontext.mc_rbx)
# define RSP_sig(p) ((p)->uc_mcontext.mc_rsp)
# define RBP_sig(p) ((p)->uc_mcontext.mc_rbp)
# define RSI_sig(p) ((p)->uc_mcontext.mc_rsi)
# define RDI_sig(p) ((p)->uc_mcontext.mc_rdi)
# define R8_sig(p) ((p)->uc_mcontext.mc_r8)
# define R9_sig(p) ((p)->uc_mcontext.mc_r9)
# define R10_sig(p) ((p)->uc_mcontext.mc_r10)
# define R11_sig(p) ((p)->uc_mcontext.mc_r11)
# define R12_sig(p) ((p)->uc_mcontext.mc_r12)
# define R13_sig(p) ((p)->uc_mcontext.mc_r13)
# define R14_sig(p) ((p)->uc_mcontext.mc_r14)
# if defined(__FreeBSD__) && defined(__arm__)
#  define R15_sig(p) ((p)->uc_mcontext.__gregs[_REG_R15])
# else
#  define R15_sig(p) ((p)->uc_mcontext.mc_r15)
# endif
#elif defined(XP_MACOSX)
# define EIP_sig(p) ((p)->uc_mcontext->__ss.__eip)
# define RIP_sig(p) ((p)->uc_mcontext->__ss.__rip)
#else
# error "Don't know how to read/write to the thread state via the mcontext_t."
#endif

#if defined(XP_WIN)
# include "jswin.h"
#else
# include <signal.h>
# include <sys/mman.h>
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
# include <sys/ucontext.h> 
#endif

#if defined(JS_CPU_X64)
# if defined(__DragonFly__)
#  include <machine/npx.h> 
# elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
       defined(__NetBSD__) || defined(__OpenBSD__)
#  include <machine/fpu.h> 
# endif
#endif

#if defined(ANDROID)







# if !defined(__BIONIC_HAVE_UCONTEXT_T)
#  if defined(__arm__)



#   if !defined(__BIONIC_HAVE_STRUCT_SIGCONTEXT)
#    include <asm/sigcontext.h>
#   endif

typedef struct sigcontext mcontext_t;

typedef struct ucontext {
    uint32_t uc_flags;
    struct ucontext* uc_link;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
    
} ucontext_t;

#  elif defined(__mips__)

typedef struct {
    uint32_t regmask;
    uint32_t status;
    uint64_t pc;
    uint64_t gregs[32];
    uint64_t fpregs[32];
    uint32_t acx;
    uint32_t fpc_csr;
    uint32_t fpc_eir;
    uint32_t used_math;
    uint32_t dsp;
    uint64_t mdhi;
    uint64_t mdlo;
    uint32_t hi1;
    uint32_t lo1;
    uint32_t hi2;
    uint32_t lo2;
    uint32_t hi3;
    uint32_t lo3;
} mcontext_t;

typedef struct ucontext {
    uint32_t uc_flags;
    struct ucontext* uc_link;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
    
} ucontext_t;

#  elif defined(__i386__)

typedef struct {
    uint32_t gregs[19];
    void* fpregs;
    uint32_t oldmask;
    uint32_t cr2;
} mcontext_t;

typedef uint32_t kernel_sigset_t[2];  
typedef struct ucontext {
    uint32_t uc_flags;
    struct ucontext* uc_link;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
    
} ucontext_t;
enum { REG_EIP = 14 };
#  endif  
# endif  
#endif 

#if !defined(XP_WIN)
# define CONTEXT ucontext_t
#endif





#if defined(XP_MACOSX) && defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
# if defined(JS_CODEGEN_X64)
struct macos_x64_context {
    x86_thread_state64_t thread;
    x86_float_state64_t float_;
};
#  define EMULATOR_CONTEXT macos_x64_context
# else
struct macos_x86_context {
    x86_thread_state_t thread;
    x86_float_state_t float_;
};
#  define EMULATOR_CONTEXT macos_x86_context
# endif
#else
# define EMULATOR_CONTEXT CONTEXT
#endif

#if defined(JS_CPU_X64)
# define PC_sig(p) RIP_sig(p)
#elif defined(JS_CPU_X86)
# define PC_sig(p) EIP_sig(p)
#elif defined(JS_CPU_ARM)
# define PC_sig(p) R15_sig(p)
#elif defined(JS_CPU_MIPS)
# define PC_sig(p) EPC_sig(p)
#endif

static uint8_t**
ContextToPC(CONTEXT* context)
{
#ifdef JS_CODEGEN_NONE
    MOZ_CRASH();
#else
     return reinterpret_cast<uint8_t**>(&PC_sig(context));
#endif
}

#if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)

#if defined(JS_CODEGEN_X64)
MOZ_COLD static void
SetFPRegToNaN(size_t size, void* fp_reg)
{
    MOZ_RELEASE_ASSERT(size <= Simd128DataSize);
    memset(fp_reg, 0, Simd128DataSize);
    switch (size) {
      case 4: *static_cast<float*>(fp_reg) = GenericNaN(); break;
      case 8: *static_cast<double*>(fp_reg) = GenericNaN(); break;
      default:
        
        MOZ_CRASH("unexpected size in SetFPRegToNaN");
    }
}

MOZ_COLD static void
SetGPRegToZero(void* gp_reg)
{
    memset(gp_reg, 0, sizeof(intptr_t));
}

MOZ_COLD static void
SetFPRegToLoadedValue(const void* addr, size_t size, void* fp_reg)
{
    MOZ_RELEASE_ASSERT(size <= Simd128DataSize);
    memset(fp_reg, 0, Simd128DataSize);
    memcpy(fp_reg, addr, size);
}

MOZ_COLD static void
SetGPRegToLoadedValue(const void* addr, size_t size, void* gp_reg)
{
    MOZ_RELEASE_ASSERT(size <= sizeof(void*));
    memset(gp_reg, 0, sizeof(void*));
    memcpy(gp_reg, addr, size);
}

MOZ_COLD static void
SetGPRegToLoadedValueSext32(const void* addr, size_t size, void* gp_reg)
{
    MOZ_RELEASE_ASSERT(size <= sizeof(int32_t));
    int8_t msb = static_cast<const int8_t*>(addr)[size - 1];
    memset(gp_reg, 0, sizeof(void*));
    memset(gp_reg, msb >> 7, sizeof(int32_t));
    memcpy(gp_reg, addr, size);
}

MOZ_COLD static void
StoreValueFromFPReg(void* addr, size_t size, const void* fp_reg)
{
    MOZ_RELEASE_ASSERT(size <= Simd128DataSize);
    memcpy(addr, fp_reg, size);
}

MOZ_COLD static void
StoreValueFromGPReg(void* addr, size_t size, const void* gp_reg)
{
    MOZ_RELEASE_ASSERT(size <= sizeof(void*));
    memcpy(addr, gp_reg, size);
}

MOZ_COLD static void
StoreValueFromGPImm(void* addr, size_t size, int32_t imm)
{
    MOZ_RELEASE_ASSERT(size <= sizeof(imm));
    memcpy(addr, &imm, size);
}

# if !defined(XP_MACOSX)
MOZ_COLD static void*
AddressOfFPRegisterSlot(CONTEXT* context, FloatRegisters::Encoding encoding)
{
    switch (encoding) {
      case X86Encoding::xmm0:  return &XMM_sig(context, 0);
      case X86Encoding::xmm1:  return &XMM_sig(context, 1);
      case X86Encoding::xmm2:  return &XMM_sig(context, 2);
      case X86Encoding::xmm3:  return &XMM_sig(context, 3);
      case X86Encoding::xmm4:  return &XMM_sig(context, 4);
      case X86Encoding::xmm5:  return &XMM_sig(context, 5);
      case X86Encoding::xmm6:  return &XMM_sig(context, 6);
      case X86Encoding::xmm7:  return &XMM_sig(context, 7);
      case X86Encoding::xmm8:  return &XMM_sig(context, 8);
      case X86Encoding::xmm9:  return &XMM_sig(context, 9);
      case X86Encoding::xmm10: return &XMM_sig(context, 10);
      case X86Encoding::xmm11: return &XMM_sig(context, 11);
      case X86Encoding::xmm12: return &XMM_sig(context, 12);
      case X86Encoding::xmm13: return &XMM_sig(context, 13);
      case X86Encoding::xmm14: return &XMM_sig(context, 14);
      case X86Encoding::xmm15: return &XMM_sig(context, 15);
      default: break;
    }
    MOZ_CRASH();
}

MOZ_COLD static void*
AddressOfGPRegisterSlot(EMULATOR_CONTEXT* context, Registers::Code code)
{
    switch (code) {
      case X86Encoding::rax: return &RAX_sig(context);
      case X86Encoding::rcx: return &RCX_sig(context);
      case X86Encoding::rdx: return &RDX_sig(context);
      case X86Encoding::rbx: return &RBX_sig(context);
      case X86Encoding::rsp: return &RSP_sig(context);
      case X86Encoding::rbp: return &RBP_sig(context);
      case X86Encoding::rsi: return &RSI_sig(context);
      case X86Encoding::rdi: return &RDI_sig(context);
      case X86Encoding::r8:  return &R8_sig(context);
      case X86Encoding::r9:  return &R9_sig(context);
      case X86Encoding::r10: return &R10_sig(context);
      case X86Encoding::r11: return &R11_sig(context);
      case X86Encoding::r12: return &R12_sig(context);
      case X86Encoding::r13: return &R13_sig(context);
      case X86Encoding::r14: return &R14_sig(context);
      case X86Encoding::r15: return &R15_sig(context);
      default: break;
    }
    MOZ_CRASH();
}
# else
MOZ_COLD static void*
AddressOfFPRegisterSlot(EMULATOR_CONTEXT* context, FloatRegisters::Encoding encoding)
{
    switch (encoding) {
      case X86Encoding::xmm0:  return &context->float_.__fpu_xmm0;
      case X86Encoding::xmm1:  return &context->float_.__fpu_xmm1;
      case X86Encoding::xmm2:  return &context->float_.__fpu_xmm2;
      case X86Encoding::xmm3:  return &context->float_.__fpu_xmm3;
      case X86Encoding::xmm4:  return &context->float_.__fpu_xmm4;
      case X86Encoding::xmm5:  return &context->float_.__fpu_xmm5;
      case X86Encoding::xmm6:  return &context->float_.__fpu_xmm6;
      case X86Encoding::xmm7:  return &context->float_.__fpu_xmm7;
      case X86Encoding::xmm8:  return &context->float_.__fpu_xmm8;
      case X86Encoding::xmm9:  return &context->float_.__fpu_xmm9;
      case X86Encoding::xmm10: return &context->float_.__fpu_xmm10;
      case X86Encoding::xmm11: return &context->float_.__fpu_xmm11;
      case X86Encoding::xmm12: return &context->float_.__fpu_xmm12;
      case X86Encoding::xmm13: return &context->float_.__fpu_xmm13;
      case X86Encoding::xmm14: return &context->float_.__fpu_xmm14;
      case X86Encoding::xmm15: return &context->float_.__fpu_xmm15;
      default: break;
    }
    MOZ_CRASH();
}

MOZ_COLD static void*
AddressOfGPRegisterSlot(EMULATOR_CONTEXT* context, Registers::Code code)
{
    switch (code) {
      case X86Encoding::rax: return &context->thread.__rax;
      case X86Encoding::rcx: return &context->thread.__rcx;
      case X86Encoding::rdx: return &context->thread.__rdx;
      case X86Encoding::rbx: return &context->thread.__rbx;
      case X86Encoding::rsp: return &context->thread.__rsp;
      case X86Encoding::rbp: return &context->thread.__rbp;
      case X86Encoding::rsi: return &context->thread.__rsi;
      case X86Encoding::rdi: return &context->thread.__rdi;
      case X86Encoding::r8:  return &context->thread.__r8;
      case X86Encoding::r9:  return &context->thread.__r9;
      case X86Encoding::r10: return &context->thread.__r10;
      case X86Encoding::r11: return &context->thread.__r11;
      case X86Encoding::r12: return &context->thread.__r12;
      case X86Encoding::r13: return &context->thread.__r13;
      case X86Encoding::r14: return &context->thread.__r14;
      case X86Encoding::r15: return &context->thread.__r15;
      default: break;
    }
    MOZ_CRASH();
}
# endif  
#endif 

MOZ_COLD static void
SetRegisterToCoercedUndefined(EMULATOR_CONTEXT* context, size_t size,
                              const Disassembler::OtherOperand& value)
{
    if (value.kind() == Disassembler::OtherOperand::FPR)
        SetFPRegToNaN(size, AddressOfFPRegisterSlot(context, value.fpr()));
    else
        SetGPRegToZero(AddressOfGPRegisterSlot(context, value.gpr()));
}

MOZ_COLD static void
SetRegisterToLoadedValue(EMULATOR_CONTEXT* context, const void* addr, size_t size,
                         const Disassembler::OtherOperand& value)
{
    if (value.kind() == Disassembler::OtherOperand::FPR)
        SetFPRegToLoadedValue(addr, size, AddressOfFPRegisterSlot(context, value.fpr()));
    else
        SetGPRegToLoadedValue(addr, size, AddressOfGPRegisterSlot(context, value.gpr()));
}

MOZ_COLD static void
SetRegisterToLoadedValueSext32(EMULATOR_CONTEXT* context, const void* addr, size_t size,
                               const Disassembler::OtherOperand& value)
{
    SetGPRegToLoadedValueSext32(addr, size, AddressOfGPRegisterSlot(context, value.gpr()));
}

MOZ_COLD static void
StoreValueFromRegister(EMULATOR_CONTEXT* context, void* addr, size_t size,
                       const Disassembler::OtherOperand& value)
{
    if (value.kind() == Disassembler::OtherOperand::FPR)
        StoreValueFromFPReg(addr, size, AddressOfFPRegisterSlot(context, value.fpr()));
    else if (value.kind() == Disassembler::OtherOperand::GPR)
        StoreValueFromGPReg(addr, size, AddressOfGPRegisterSlot(context, value.gpr()));
    else
        StoreValueFromGPImm(addr, size, value.imm());
}

MOZ_COLD static uint8_t*
ComputeAccessAddress(EMULATOR_CONTEXT* context, const Disassembler::ComplexAddress& address)
{
    MOZ_RELEASE_ASSERT(!address.isPCRelative(), "PC-relative addresses not supported yet");

    uintptr_t result = address.disp();

    if (address.hasBase()) {
        uintptr_t base;
        StoreValueFromGPReg(&base, sizeof(uintptr_t),
                            AddressOfGPRegisterSlot(context, address.base()));
        result += base;
    }

    if (address.hasIndex()) {
        uintptr_t index;
        StoreValueFromGPReg(&index, sizeof(uintptr_t),
                            AddressOfGPRegisterSlot(context, address.index()));
        result += index * (1 << address.scale());
    }

    return reinterpret_cast<uint8_t*>(result);
}

MOZ_COLD static uint8_t*
EmulateHeapAccess(EMULATOR_CONTEXT* context, uint8_t* pc, uint8_t* faultingAddress,
                  const AsmJSHeapAccess* heapAccess, const AsmJSModule& module)
{
    MOZ_RELEASE_ASSERT(module.containsFunctionPC(pc));
    MOZ_RELEASE_ASSERT(module.usesSignalHandlersForOOB());
    MOZ_RELEASE_ASSERT(!heapAccess->hasLengthCheck());
    MOZ_RELEASE_ASSERT(heapAccess->insnOffset() == (pc - module.codeBase()));

    
    
    Disassembler::HeapAccess access;
    uint8_t* end = Disassembler::DisassembleHeapAccess(pc, &access);
    const Disassembler::ComplexAddress& address = access.address();
    MOZ_RELEASE_ASSERT(end > pc);
    MOZ_RELEASE_ASSERT(module.containsFunctionPC(end));

#if defined(JS_CODEGEN_X64)
    
    MOZ_RELEASE_ASSERT(address.disp() >= 0);
    MOZ_RELEASE_ASSERT(address.base() == HeapReg.code());
    MOZ_RELEASE_ASSERT(!address.hasIndex() || address.index() != HeapReg.code());
    MOZ_RELEASE_ASSERT(address.scale() == 0);
    if (address.hasBase()) {
        uintptr_t base;
        StoreValueFromGPReg(&base, sizeof(uintptr_t),
                            AddressOfGPRegisterSlot(context, address.base()));
        MOZ_RELEASE_ASSERT(reinterpret_cast<uint8_t*>(base) == module.maybeHeap());
    }
    if (address.hasIndex()) {
        uintptr_t index;
        StoreValueFromGPReg(&index, sizeof(uintptr_t),
                            AddressOfGPRegisterSlot(context, address.index()));
        MOZ_RELEASE_ASSERT(uint32_t(index) == index);
    }
#endif

    
    
    
    
    uint8_t* accessAddress = ComputeAccessAddress(context, address);
    MOZ_RELEASE_ASSERT(size_t(faultingAddress - accessAddress) < access.size(),
                       "Given faulting address does not appear to be within computed "
                       "faulting address range");
    MOZ_RELEASE_ASSERT(accessAddress >= module.maybeHeap(),
                       "Access begins outside the asm.js heap");
    MOZ_RELEASE_ASSERT(accessAddress + access.size() <= module.maybeHeap() + AsmJSMappedSize,
                       "Access extends beyond the asm.js heap guard region");
    MOZ_RELEASE_ASSERT(accessAddress + access.size() > module.maybeHeap() + module.heapLength(),
                       "Computed access address is not actually out of bounds");

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    intptr_t unwrappedOffset = accessAddress - module.maybeHeap();
    uint32_t wrappedOffset = uint32_t(unwrappedOffset);
    size_t size = access.size();
    MOZ_RELEASE_ASSERT(wrappedOffset + size > wrappedOffset);
    bool inBounds = wrappedOffset < module.heapLength() &&
                    wrappedOffset + size < module.heapLength();

    
    
    MOZ_RELEASE_ASSERT(unwrappedOffset > heapAccess->offsetWithinWholeSimdVector());
    uint32_t wrappedBaseOffset = uint32_t(unwrappedOffset - heapAccess->offsetWithinWholeSimdVector());
    if (wrappedBaseOffset >= module.heapLength())
        inBounds = false;

    if (inBounds) {
        
        
        
        uint8_t* wrappedAddress = module.maybeHeap() + wrappedOffset;
        MOZ_RELEASE_ASSERT(wrappedAddress >= module.maybeHeap());
        MOZ_RELEASE_ASSERT(wrappedAddress + size > wrappedAddress);
        MOZ_RELEASE_ASSERT(wrappedAddress + size <= module.maybeHeap() + module.heapLength());
        switch (access.kind()) {
          case Disassembler::HeapAccess::Load:
            SetRegisterToLoadedValue(context, wrappedAddress, size, access.otherOperand());
            break;
          case Disassembler::HeapAccess::LoadSext32:
            SetRegisterToLoadedValueSext32(context, wrappedAddress, size, access.otherOperand());
            break;
          case Disassembler::HeapAccess::Store:
            StoreValueFromRegister(context, wrappedAddress, size, access.otherOperand());
            break;
          case Disassembler::HeapAccess::Unknown:
            MOZ_CRASH("Failed to disassemble instruction");
        }
    } else {
        
        

        if (heapAccess->throwOnOOB())
            return module.outOfBoundsExit();

        switch (access.kind()) {
          case Disassembler::HeapAccess::Load:
          case Disassembler::HeapAccess::LoadSext32:
            
            
            
            
            
            
            SetRegisterToCoercedUndefined(context, access.size(), access.otherOperand());
            break;
          case Disassembler::HeapAccess::Store:
            
            break;
          case Disassembler::HeapAccess::Unknown:
            MOZ_CRASH("Failed to disassemble instruction");
        }
    }

    return end;
}

#if defined(XP_WIN)

static bool
HandleFault(PEXCEPTION_POINTERS exception)
{
    EXCEPTION_RECORD* record = exception->ExceptionRecord;
    CONTEXT* context = exception->ContextRecord;

    if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return false;

    uint8_t** ppc = ContextToPC(context);
    uint8_t* pc = *ppc;

    if (record->NumberParameters < 2)
        return false;

    
    JSRuntime* rt = RuntimeForCurrentThread();
    if (!rt || rt->handlingSignal)
        return false;
    AutoSetHandlingSignal handling(rt);

    AsmJSActivation* activation = rt->asmJSActivationStack();
    if (!activation)
        return false;

    const AsmJSModule& module = activation->module();

    
    
    uint8_t* faultingAddress = reinterpret_cast<uint8_t*>(record->ExceptionInformation[1]);
    if (!module.maybeHeap() ||
        faultingAddress < module.maybeHeap() ||
        faultingAddress >= module.maybeHeap() + AsmJSMappedSize)
    {
        return false;
    }

    if (!module.containsFunctionPC(pc)) {
        
        
        
        
        
        
        
        
        
        if (pc == module.interruptExit() &&
            module.containsFunctionPC(activation->resumePC()) &&
            module.lookupHeapAccess(activation->resumePC()))
        {
            return true;
        }
        return false;
    }

    const AsmJSHeapAccess* heapAccess = module.lookupHeapAccess(pc);
    if (!heapAccess)
        return false;

    *ppc = EmulateHeapAccess(context, pc, faultingAddress, heapAccess, module);
    return true;
}

static LONG WINAPI
AsmJSFaultHandler(LPEXCEPTION_POINTERS exception)
{
    if (HandleFault(exception))
        return EXCEPTION_CONTINUE_EXECUTION;

    
    return EXCEPTION_CONTINUE_SEARCH;
}

#elif defined(XP_MACOSX)
# include <mach/exc.h>

static uint8_t**
ContextToPC(EMULATOR_CONTEXT* context)
{
# if defined(JS_CPU_X64)
    static_assert(sizeof(context->thread.__rip) == sizeof(void*),
                  "stored IP should be compile-time pointer-sized");
    return reinterpret_cast<uint8_t**>(&context->thread.__rip);
# else
    static_assert(sizeof(context->thread.uts.ts32.__eip) == sizeof(void*),
                  "stored IP should be compile-time pointer-sized");
    return reinterpret_cast<uint8_t**>(&context->thread.uts.ts32.__eip);
# endif
}



#pragma pack(4)
typedef struct {
    mach_msg_header_t Head;
    
    mach_msg_body_t msgh_body;
    mach_msg_port_descriptor_t thread;
    mach_msg_port_descriptor_t task;
    
    NDR_record_t NDR;
    exception_type_t exception;
    mach_msg_type_number_t codeCnt;
    int64_t code[2];
} Request__mach_exception_raise_t;
#pragma pack()


struct ExceptionRequest
{
    Request__mach_exception_raise_t body;
    mach_msg_trailer_t trailer;
};

static bool
HandleMachException(JSRuntime* rt, const ExceptionRequest& request)
{
    
    if (rt->handlingSignal)
        return false;
    AutoSetHandlingSignal handling(rt);

    
    mach_port_t rtThread = request.body.thread.name;

    
    EMULATOR_CONTEXT context;
# if defined(JS_CODEGEN_X64)
    unsigned int thread_state_count = x86_THREAD_STATE64_COUNT;
    unsigned int float_state_count = x86_FLOAT_STATE64_COUNT;
    int thread_state = x86_THREAD_STATE64;
    int float_state = x86_FLOAT_STATE64;
# else
    unsigned int thread_state_count = x86_THREAD_STATE_COUNT;
    unsigned int float_state_count = x86_FLOAT_STATE_COUNT;
    int thread_state = x86_THREAD_STATE;
    int float_state = x86_FLOAT_STATE;
# endif
    kern_return_t kret;
    kret = thread_get_state(rtThread, thread_state,
                            (thread_state_t)&context.thread, &thread_state_count);
    if (kret != KERN_SUCCESS)
        return false;
    kret = thread_get_state(rtThread, float_state,
                            (thread_state_t)&context.float_, &float_state_count);
    if (kret != KERN_SUCCESS)
        return false;

    uint8_t** ppc = ContextToPC(&context);
    uint8_t* pc = *ppc;

    if (request.body.exception != EXC_BAD_ACCESS || request.body.codeCnt != 2)
        return false;

    AsmJSActivation* activation = rt->asmJSActivationStack();
    if (!activation)
        return false;

    const AsmJSModule& module = activation->module();
    if (!module.containsFunctionPC(pc))
        return false;

    
    
    uint8_t* faultingAddress = reinterpret_cast<uint8_t*>(request.body.code[1]);
    if (!module.maybeHeap() ||
        faultingAddress < module.maybeHeap() ||
        faultingAddress >= module.maybeHeap() + AsmJSMappedSize)
    {
        return false;
    }

    const AsmJSHeapAccess* heapAccess = module.lookupHeapAccess(pc);
    if (!heapAccess)
        return false;

    *ppc = EmulateHeapAccess(&context, pc, faultingAddress, heapAccess, module);

    
    kret = thread_set_state(rtThread, float_state, (thread_state_t)&context.float_, float_state_count);
    if (kret != KERN_SUCCESS)
        return false;
    kret = thread_set_state(rtThread, thread_state, (thread_state_t)&context.thread, thread_state_count);
    if (kret != KERN_SUCCESS)
        return false;

    return true;
}


static const mach_msg_id_t sExceptionId = 2405;


static const mach_msg_id_t sQuitId = 42;

void
AsmJSMachExceptionHandlerThread(void* threadArg)
{
    JSRuntime* rt = reinterpret_cast<JSRuntime*>(threadArg);
    mach_port_t port = rt->asmJSMachExceptionHandler.port();
    kern_return_t kret;

    while(true) {
        ExceptionRequest request;
        kret = mach_msg(&request.body.Head, MACH_RCV_MSG, 0, sizeof(request),
                        port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

        
        
        if (kret != KERN_SUCCESS) {
            fprintf(stderr, "AsmJSMachExceptionHandlerThread: mach_msg failed with %d\n", (int)kret);
            MOZ_CRASH();
        }

        
        
        
        if (request.body.Head.msgh_id == sQuitId)
            break;
        if (request.body.Head.msgh_id != sExceptionId) {
            fprintf(stderr, "Unexpected msg header id %d\n", (int)request.body.Head.msgh_bits);
            MOZ_CRASH();
        }

        
        
        
        
        
        
        bool handled = HandleMachException(rt, request);
        kern_return_t replyCode = handled ? KERN_SUCCESS : KERN_FAILURE;

        
        
        __Reply__exception_raise_t reply;
        reply.Head.msgh_bits = MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(request.body.Head.msgh_bits), 0);
        reply.Head.msgh_size = sizeof(reply);
        reply.Head.msgh_remote_port = request.body.Head.msgh_remote_port;
        reply.Head.msgh_local_port = MACH_PORT_NULL;
        reply.Head.msgh_id = request.body.Head.msgh_id + 100;
        reply.NDR = NDR_record;
        reply.RetCode = replyCode;
        mach_msg(&reply.Head, MACH_SEND_MSG, sizeof(reply), 0, MACH_PORT_NULL,
                 MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    }
}

AsmJSMachExceptionHandler::AsmJSMachExceptionHandler()
  : installed_(false),
    thread_(nullptr),
    port_(MACH_PORT_NULL)
{}

void
AsmJSMachExceptionHandler::uninstall()
{
    if (installed_) {
        thread_port_t thread = mach_thread_self();
        kern_return_t kret = thread_set_exception_ports(thread,
                                                        EXC_MASK_BAD_ACCESS,
                                                        MACH_PORT_NULL,
                                                        EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
                                                        THREAD_STATE_NONE);
        mach_port_deallocate(mach_task_self(), thread);
        if (kret != KERN_SUCCESS)
            MOZ_CRASH();
        installed_ = false;
    }
    if (thread_ != nullptr) {
        
        mach_msg_header_t msg;
        msg.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
        msg.msgh_size = sizeof(msg);
        msg.msgh_remote_port = port_;
        msg.msgh_local_port = MACH_PORT_NULL;
        msg.msgh_reserved = 0;
        msg.msgh_id = sQuitId;
        kern_return_t kret = mach_msg(&msg, MACH_SEND_MSG, sizeof(msg), 0, MACH_PORT_NULL,
                                      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
        if (kret != KERN_SUCCESS) {
            fprintf(stderr, "AsmJSMachExceptionHandler: failed to send quit message: %d\n", (int)kret);
            MOZ_CRASH();
        }

        
        PR_JoinThread(thread_);
        thread_ = nullptr;
    }
    if (port_ != MACH_PORT_NULL) {
        DebugOnly<kern_return_t> kret = mach_port_destroy(mach_task_self(), port_);
        MOZ_ASSERT(kret == KERN_SUCCESS);
        port_ = MACH_PORT_NULL;
    }
}

bool
AsmJSMachExceptionHandler::install(JSRuntime* rt)
{
    MOZ_ASSERT(!installed());
    kern_return_t kret;
    mach_port_t thread;

    
    kret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port_);
    if (kret != KERN_SUCCESS)
        goto error;
    kret = mach_port_insert_right(mach_task_self(), port_, port_, MACH_MSG_TYPE_MAKE_SEND);
    if (kret != KERN_SUCCESS)
        goto error;

    
    thread_ = PR_CreateThread(PR_USER_THREAD, AsmJSMachExceptionHandlerThread, rt,
                              PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (!thread_)
        goto error;

    
    
    
    
    
    thread = mach_thread_self();
    kret = thread_set_exception_ports(thread,
                                      EXC_MASK_BAD_ACCESS,
                                      port_,
                                      EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
                                      THREAD_STATE_NONE);
    mach_port_deallocate(mach_task_self(), thread);
    if (kret != KERN_SUCCESS)
        goto error;

    installed_ = true;
    return true;

  error:
    uninstall();
    return false;
}

#else  



static bool
HandleFault(int signum, siginfo_t* info, void* ctx)
{
    
    
    
    MOZ_RELEASE_ASSERT(signum == SIGSEGV);
    if (info->si_code != SEGV_ACCERR)
        return false;

    CONTEXT* context = (CONTEXT*)ctx;
    uint8_t** ppc = ContextToPC(context);
    uint8_t* pc = *ppc;

    
    JSRuntime* rt = RuntimeForCurrentThread();
    if (!rt || rt->handlingSignal)
        return false;
    AutoSetHandlingSignal handling(rt);

    AsmJSActivation* activation = rt->asmJSActivationStack();
    if (!activation)
        return false;

    const AsmJSModule& module = activation->module();
    if (!module.containsFunctionPC(pc))
        return false;

    
    
    uint8_t* faultingAddress = reinterpret_cast<uint8_t*>(info->si_addr);
    if (!module.maybeHeap() ||
        faultingAddress < module.maybeHeap() ||
        faultingAddress >= module.maybeHeap() + AsmJSMappedSize)
    {
        return false;
    }

    const AsmJSHeapAccess* heapAccess = module.lookupHeapAccess(pc);
    if (!heapAccess)
        return false;

    *ppc = EmulateHeapAccess(context, pc, faultingAddress, heapAccess, module);

    return true;
}

static struct sigaction sPrevSEGVHandler;

static void
AsmJSFaultHandler(int signum, siginfo_t* info, void* context)
{
    if (HandleFault(signum, info, context))
        return;

    
    
    
    
    
    
    
    
    
    
    
    
    if (sPrevSEGVHandler.sa_flags & SA_SIGINFO)
        sPrevSEGVHandler.sa_sigaction(signum, info, context);
    else if (sPrevSEGVHandler.sa_handler == SIG_DFL || sPrevSEGVHandler.sa_handler == SIG_IGN)
        sigaction(signum, &sPrevSEGVHandler, nullptr);
    else
        sPrevSEGVHandler.sa_handler(signum);
}
#endif

#endif 

static void
RedirectIonBackedgesToInterruptCheck(JSRuntime* rt)
{
    if (jit::JitRuntime* jitRuntime = rt->jitRuntime()) {
        
        
        
        
        if (!jitRuntime->mutatingBackedgeList())
            jitRuntime->patchIonBackedges(rt, jit::JitRuntime::BackedgeInterruptCheck);
    }
}

static bool
RedirectJitCodeToInterruptCheck(JSRuntime* rt, CONTEXT* context)
{
    RedirectIonBackedgesToInterruptCheck(rt);

    if (AsmJSActivation* activation = rt->asmJSActivationStack()) {
        const AsmJSModule& module = activation->module();

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
        if (module.containsFunctionPC((void*)rt->simulator()->get_pc()))
            rt->simulator()->set_resume_pc(int32_t(module.interruptExit()));
#endif

        uint8_t** ppc = ContextToPC(context);
        uint8_t* pc = *ppc;
        if (module.containsFunctionPC(pc)) {
            activation->setResumePC(pc);
            *ppc = module.interruptExit();
            return true;
        }
    }

    return false;
}

#if !defined(XP_WIN)





static const int sInterruptSignal = SIGVTALRM;

static void
JitInterruptHandler(int signum, siginfo_t* info, void* context)
{
    if (JSRuntime* rt = RuntimeForCurrentThread())
        RedirectJitCodeToInterruptCheck(rt, (CONTEXT*)context);
}
#endif

bool
js::EnsureSignalHandlersInstalled(JSRuntime* rt)
{
#if defined(XP_MACOSX) && defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
    
    if (!rt->asmJSMachExceptionHandler.installed() && !rt->asmJSMachExceptionHandler.install(rt))
        return false;
#endif

    
    
    
    static bool sTried = false;
    static bool sResult = false;
    if (sTried)
        return sResult;
    sTried = true;

#if defined(ANDROID)
    
    
    
    
    
    char version_string[PROP_VALUE_MAX];
    PodArrayZero(version_string);
    if (__system_property_get("ro.build.version.sdk", version_string) > 0) {
        if (atol(version_string) < 19)
            return false;
    }
# if defined(MOZ_LINKER)
    
    if (IsSignalHandlingBroken())
        return false;
# endif
#endif

    
    
#if defined(XP_WIN)
    
#else
    struct sigaction interruptHandler;
    interruptHandler.sa_flags = SA_SIGINFO;
    interruptHandler.sa_sigaction = &JitInterruptHandler;
    sigemptyset(&interruptHandler.sa_mask);
    struct sigaction prev;
    if (sigaction(sInterruptSignal, &interruptHandler, &prev))
        MOZ_CRASH("unable to install interrupt handler");

    
    
    
    if ((prev.sa_flags & SA_SIGINFO && prev.sa_sigaction) ||
        (prev.sa_handler != SIG_DFL && prev.sa_handler != SIG_IGN))
    {
        MOZ_CRASH("contention for interrupt signal");
    }
#endif 

#if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
    
    
# if defined(XP_WIN)
    if (!AddVectoredExceptionHandler( true, AsmJSFaultHandler))
        return false;
# elif defined(XP_MACOSX)
    
    
# else
    
    
    
    struct sigaction faultHandler;
    faultHandler.sa_flags = SA_SIGINFO | SA_NODEFER;
    faultHandler.sa_sigaction = &AsmJSFaultHandler;
    sigemptyset(&faultHandler.sa_mask);
    if (sigaction(SIGSEGV, &faultHandler, &sPrevSEGVHandler))
        MOZ_CRASH("unable to install segv handler");
# endif
#endif 

    sResult = true;
    return true;
}










void
js::InterruptRunningJitCode(JSRuntime* rt)
{
    
    
    if (!rt->canUseSignalHandlers())
        return;

    
    
    
    if (rt == RuntimeForCurrentThread()) {
        RedirectIonBackedgesToInterruptCheck(rt);
        return;
    }

    
    
#if defined(XP_WIN)
    
    
    
    
    HANDLE thread = (HANDLE)rt->ownerThreadNative();
    if (SuspendThread(thread) != -1) {
        CONTEXT context;
        context.ContextFlags = CONTEXT_CONTROL;
        if (GetThreadContext(thread, &context)) {
            if (RedirectJitCodeToInterruptCheck(rt, &context))
                SetThreadContext(thread, &context);
        }
        ResumeThread(thread);
    }
#else
    
    
    
    pthread_t thread = (pthread_t)rt->ownerThreadNative();
    pthread_kill(thread, sInterruptSignal);
#endif
}
