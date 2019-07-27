

















#include "asmjs/AsmJSSignalHandlers.h"

#include "mozilla/DebugOnly.h"

#include "asmjs/AsmJSModule.h"
#include "vm/Runtime.h"

using namespace js;
using namespace js::jit;

using JS::GenericNaN;
using mozilla::DebugOnly;

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
# define XMM_sig(p,i) (((struct fxsave64 *)(p)->uc_mcontext.__fpregs)->fx_xmm[i])
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
#  define XMM_sig(p,i) (((union savefpu *)(p)->uc_mcontext.mc_fpregs)->sv_xmm.sv_xmm[i])
# else
#  define XMM_sig(p,i) (((struct savefpu *)(p)->uc_mcontext.mc_fpstate)->sv_xmm[i])
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

#else
# error "Don't know how to read/write to the thread state via the mcontext_t."
#endif




#if !defined(XP_MACOSX)
static JSRuntime *
RuntimeForCurrentThread()
{
    PerThreadData *threadData = TlsPerThreadData.get();
    if (!threadData)
        return nullptr;

    return threadData->runtimeIfOnOwnerThread();
}
#endif 






class AutoSetHandlingSignal
{
    JSRuntime *rt;

  public:
    explicit AutoSetHandlingSignal(JSRuntime *rt)
      : rt(rt)
    {
        JS_ASSERT(!rt->handlingSignal);
        rt->handlingSignal = true;
    }

    ~AutoSetHandlingSignal()
    {
        JS_ASSERT(rt->handlingSignal);
        rt->handlingSignal = false;
    }
};

#if defined(JS_CODEGEN_X64)
template <class T>
static void
SetXMMRegToNaN(bool isFloat32, T *xmm_reg)
{
    if (isFloat32) {
        JS_STATIC_ASSERT(sizeof(T) == 4 * sizeof(float));
        float *floats = reinterpret_cast<float*>(xmm_reg);
        floats[0] = GenericNaN();
        floats[1] = 0;
        floats[2] = 0;
        floats[3] = 0;
    } else {
        JS_STATIC_ASSERT(sizeof(T) == 2 * sizeof(double));
        double *dbls = reinterpret_cast<double*>(xmm_reg);
        dbls[0] = GenericNaN();
        dbls[1] = 0;
    }
}
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

#if defined(JS_CODEGEN_X64)
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

#if defined(ANDROID) && defined(MOZ_LINKER)






extern "C" MFBT_API bool IsSignalHandlingBroken();
#else
static bool IsSignalHandlingBroken() { return false; }
#endif 

#if !defined(XP_WIN)
# define CONTEXT ucontext_t
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

static bool
HandleSimulatorInterrupt(JSRuntime *rt, AsmJSActivation *activation, void *faultingAddress)
{
    
    
    
    
    

#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    const AsmJSModule &module = activation->module();
    if (module.containsFunctionPC((void *)rt->mainThread.simulator()->get_pc()) &&
        module.containsFunctionPC(faultingAddress))
    {
        activation->setResumePC(nullptr);
        int32_t nextpc = int32_t(module.interruptExit());
        rt->mainThread.simulator()->set_resume_pc(nextpc);
        return true;
    }
#endif
    return false;
}

#if !defined(XP_MACOSX)
static uint8_t **
ContextToPC(CONTEXT *context)
{
#ifdef JS_CODEGEN_NONE
    MOZ_CRASH();
#else
    return reinterpret_cast<uint8_t**>(&PC_sig(context));
#endif
}

# if defined(JS_CODEGEN_X64)
static void
SetRegisterToCoercedUndefined(CONTEXT *context, bool isFloat32, AnyRegister reg)
{
    if (reg.isFloat()) {
        switch (reg.fpu().code()) {
          case JSC::X86Registers::xmm0:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 0)); break;
          case JSC::X86Registers::xmm1:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 1)); break;
          case JSC::X86Registers::xmm2:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 2)); break;
          case JSC::X86Registers::xmm3:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 3)); break;
          case JSC::X86Registers::xmm4:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 4)); break;
          case JSC::X86Registers::xmm5:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 5)); break;
          case JSC::X86Registers::xmm6:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 6)); break;
          case JSC::X86Registers::xmm7:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 7)); break;
          case JSC::X86Registers::xmm8:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 8)); break;
          case JSC::X86Registers::xmm9:  SetXMMRegToNaN(isFloat32, &XMM_sig(context, 9)); break;
          case JSC::X86Registers::xmm10: SetXMMRegToNaN(isFloat32, &XMM_sig(context, 10)); break;
          case JSC::X86Registers::xmm11: SetXMMRegToNaN(isFloat32, &XMM_sig(context, 11)); break;
          case JSC::X86Registers::xmm12: SetXMMRegToNaN(isFloat32, &XMM_sig(context, 12)); break;
          case JSC::X86Registers::xmm13: SetXMMRegToNaN(isFloat32, &XMM_sig(context, 13)); break;
          case JSC::X86Registers::xmm14: SetXMMRegToNaN(isFloat32, &XMM_sig(context, 14)); break;
          case JSC::X86Registers::xmm15: SetXMMRegToNaN(isFloat32, &XMM_sig(context, 15)); break;
          default: MOZ_CRASH();
        }
    } else {
        switch (reg.gpr().code()) {
          case JSC::X86Registers::eax: RAX_sig(context) = 0; break;
          case JSC::X86Registers::ecx: RCX_sig(context) = 0; break;
          case JSC::X86Registers::edx: RDX_sig(context) = 0; break;
          case JSC::X86Registers::ebx: RBX_sig(context) = 0; break;
          case JSC::X86Registers::esp: RSP_sig(context) = 0; break;
          case JSC::X86Registers::ebp: RBP_sig(context) = 0; break;
          case JSC::X86Registers::esi: RSI_sig(context) = 0; break;
          case JSC::X86Registers::edi: RDI_sig(context) = 0; break;
          case JSC::X86Registers::r8:  R8_sig(context)  = 0; break;
          case JSC::X86Registers::r9:  R9_sig(context)  = 0; break;
          case JSC::X86Registers::r10: R10_sig(context) = 0; break;
          case JSC::X86Registers::r11: R11_sig(context) = 0; break;
          case JSC::X86Registers::r12: R12_sig(context) = 0; break;
          case JSC::X86Registers::r13: R13_sig(context) = 0; break;
          case JSC::X86Registers::r14: R14_sig(context) = 0; break;
          case JSC::X86Registers::r15: R15_sig(context) = 0; break;
          default: MOZ_CRASH();
        }
    }
}
# endif  
#endif   

#if defined(XP_WIN)

static bool
HandleException(PEXCEPTION_POINTERS exception)
{
    EXCEPTION_RECORD *record = exception->ExceptionRecord;
    CONTEXT *context = exception->ContextRecord;

    if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return false;

    uint8_t **ppc = ContextToPC(context);
    uint8_t *pc = *ppc;
    JS_ASSERT(pc == record->ExceptionAddress);

    if (record->NumberParameters < 2)
        return false;

    void *faultingAddress = (void*)record->ExceptionInformation[1];

    JSRuntime *rt = RuntimeForCurrentThread();

    
    if (!rt || rt->handlingSignal)
        return false;
    AutoSetHandlingSignal handling(rt);

    if (rt->jitRuntime() && rt->jitRuntime()->handleAccessViolation(rt, faultingAddress))
        return true;

    AsmJSActivation *activation = PerThreadData::innermostAsmJSActivation();
    if (!activation)
        return false;

    const AsmJSModule &module = activation->module();
    if (!module.containsFunctionPC(pc))
        return false;

    
    
    
    
    
    if (module.containsFunctionPC(faultingAddress)) {
        activation->setResumePC(pc);
        *ppc = module.interruptExit();

        JSRuntime::AutoLockForInterrupt lock(rt);
        module.unprotectCode(rt);
        return true;
    }

# if defined(JS_CODEGEN_X64)
    
    
    if (!module.maybeHeap() ||
        faultingAddress < module.maybeHeap() ||
        faultingAddress >= module.maybeHeap() + AsmJSBufferProtectedSize)
    {
        return false;
    }

    const AsmJSHeapAccess *heapAccess = module.lookupHeapAccess(pc);
    if (!heapAccess)
        return false;

    
    if (heapAccess->isLoad() != !record->ExceptionInformation[0])
        return false;

    
    
    
    
    
    
    if (heapAccess->isLoad())
        SetRegisterToCoercedUndefined(context, heapAccess->isFloat32Load(), heapAccess->loadedReg());
    *ppc += heapAccess->opLength();
    return true;
# else
    return false;
# endif
}

static LONG WINAPI
AsmJSExceptionHandler(LPEXCEPTION_POINTERS exception)
{
    if (HandleException(exception))
        return EXCEPTION_CONTINUE_EXECUTION;

    
    return EXCEPTION_CONTINUE_SEARCH;
}

#elif defined(XP_MACOSX)
# include <mach/exc.h>

static uint8_t **
ContextToPC(x86_thread_state_t &state)
{
# if defined(JS_CODEGEN_X64)
    JS_STATIC_ASSERT(sizeof(state.uts.ts64.__rip) == sizeof(void*));
    return reinterpret_cast<uint8_t**>(&state.uts.ts64.__rip);
# elif defined(JS_CODEGEN_NONE)
    MOZ_CRASH();
# else
    JS_STATIC_ASSERT(sizeof(state.uts.ts32.__eip) == sizeof(void*));
    return reinterpret_cast<uint8_t**>(&state.uts.ts32.__eip);
# endif
}

# if defined(JS_CODEGEN_X64)
static bool
SetRegisterToCoercedUndefined(mach_port_t rtThread, x86_thread_state64_t &state,
                              const AsmJSHeapAccess &heapAccess)
{
    if (heapAccess.loadedReg().isFloat()) {
        kern_return_t kret;

        x86_float_state64_t fstate;
        unsigned int count = x86_FLOAT_STATE64_COUNT;
        kret = thread_get_state(rtThread, x86_FLOAT_STATE64, (thread_state_t) &fstate, &count);
        if (kret != KERN_SUCCESS)
            return false;

        bool f32 = heapAccess.isFloat32Load();
        switch (heapAccess.loadedReg().fpu().code()) {
          case JSC::X86Registers::xmm0:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm0); break;
          case JSC::X86Registers::xmm1:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm1); break;
          case JSC::X86Registers::xmm2:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm2); break;
          case JSC::X86Registers::xmm3:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm3); break;
          case JSC::X86Registers::xmm4:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm4); break;
          case JSC::X86Registers::xmm5:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm5); break;
          case JSC::X86Registers::xmm6:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm6); break;
          case JSC::X86Registers::xmm7:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm7); break;
          case JSC::X86Registers::xmm8:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm8); break;
          case JSC::X86Registers::xmm9:  SetXMMRegToNaN(f32, &fstate.__fpu_xmm9); break;
          case JSC::X86Registers::xmm10: SetXMMRegToNaN(f32, &fstate.__fpu_xmm10); break;
          case JSC::X86Registers::xmm11: SetXMMRegToNaN(f32, &fstate.__fpu_xmm11); break;
          case JSC::X86Registers::xmm12: SetXMMRegToNaN(f32, &fstate.__fpu_xmm12); break;
          case JSC::X86Registers::xmm13: SetXMMRegToNaN(f32, &fstate.__fpu_xmm13); break;
          case JSC::X86Registers::xmm14: SetXMMRegToNaN(f32, &fstate.__fpu_xmm14); break;
          case JSC::X86Registers::xmm15: SetXMMRegToNaN(f32, &fstate.__fpu_xmm15); break;
          default: MOZ_CRASH();
        }

        kret = thread_set_state(rtThread, x86_FLOAT_STATE64, (thread_state_t)&fstate, x86_FLOAT_STATE64_COUNT);
        if (kret != KERN_SUCCESS)
            return false;
    } else {
        switch (heapAccess.loadedReg().gpr().code()) {
          case JSC::X86Registers::eax: state.__rax = 0; break;
          case JSC::X86Registers::ecx: state.__rcx = 0; break;
          case JSC::X86Registers::edx: state.__rdx = 0; break;
          case JSC::X86Registers::ebx: state.__rbx = 0; break;
          case JSC::X86Registers::esp: state.__rsp = 0; break;
          case JSC::X86Registers::ebp: state.__rbp = 0; break;
          case JSC::X86Registers::esi: state.__rsi = 0; break;
          case JSC::X86Registers::edi: state.__rdi = 0; break;
          case JSC::X86Registers::r8:  state.__r8  = 0; break;
          case JSC::X86Registers::r9:  state.__r9  = 0; break;
          case JSC::X86Registers::r10: state.__r10 = 0; break;
          case JSC::X86Registers::r11: state.__r11 = 0; break;
          case JSC::X86Registers::r12: state.__r12 = 0; break;
          case JSC::X86Registers::r13: state.__r13 = 0; break;
          case JSC::X86Registers::r14: state.__r14 = 0; break;
          case JSC::X86Registers::r15: state.__r15 = 0; break;
          default: MOZ_CRASH();
        }
    }
    return true;
}
# endif



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
HandleMachException(JSRuntime *rt, const ExceptionRequest &request)
{
    
    if (rt->handlingSignal)
        return false;
    AutoSetHandlingSignal handling(rt);

    
    mach_port_t rtThread = request.body.thread.name;

    
    x86_thread_state_t state;
    unsigned int count = x86_THREAD_STATE_COUNT;
    kern_return_t kret;
    kret = thread_get_state(rtThread, x86_THREAD_STATE, (thread_state_t)&state, &count);
    if (kret != KERN_SUCCESS)
        return false;

    uint8_t **ppc = ContextToPC(state);
    uint8_t *pc = *ppc;

    if (request.body.exception != EXC_BAD_ACCESS || request.body.codeCnt != 2)
        return false;

    void *faultingAddress = (void*)request.body.code[1];

    if (rt->jitRuntime() && rt->jitRuntime()->handleAccessViolation(rt, faultingAddress))
        return true;

    AsmJSActivation *activation = rt->mainThread.asmJSActivationStack();
    if (!activation)
        return false;

    const AsmJSModule &module = activation->module();
    if (HandleSimulatorInterrupt(rt, activation, faultingAddress)) {
        JSRuntime::AutoLockForInterrupt lock(rt);
        module.unprotectCode(rt);
        return true;
    }

    if (!module.containsFunctionPC(pc))
        return false;

    
    
    
    
    
    if (module.containsFunctionPC(faultingAddress)) {
        activation->setResumePC(pc);
        *ppc = module.interruptExit();

        JSRuntime::AutoLockForInterrupt lock(rt);
        module.unprotectCode(rt);

        
        kret = thread_set_state(rtThread, x86_THREAD_STATE, (thread_state_t)&state, x86_THREAD_STATE_COUNT);
        return kret == KERN_SUCCESS;
    }

# if defined(JS_CODEGEN_X64)
    
    
    if (!module.maybeHeap() ||
        faultingAddress < module.maybeHeap() ||
        faultingAddress >= module.maybeHeap() + AsmJSBufferProtectedSize)
    {
        return false;
    }

    const AsmJSHeapAccess *heapAccess = module.lookupHeapAccess(pc);
    if (!heapAccess)
        return false;

    
    
    
    
    
    
    if (heapAccess->isLoad()) {
        if (!SetRegisterToCoercedUndefined(rtThread, state.uts.ts64, *heapAccess))
            return false;
    }
    *ppc += heapAccess->opLength();

    
    kret = thread_set_state(rtThread, x86_THREAD_STATE, (thread_state_t)&state, x86_THREAD_STATE_COUNT);
    if (kret != KERN_SUCCESS)
        return false;

    return true;
# else
    return false;
# endif
}


static const mach_msg_id_t sExceptionId = 2405;


static const mach_msg_id_t sQuitId = 42;

void
AsmJSMachExceptionHandlerThread(void *threadArg)
{
    JSRuntime *rt = reinterpret_cast<JSRuntime*>(threadArg);
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
        JS_ASSERT(kret == KERN_SUCCESS);
        port_ = MACH_PORT_NULL;
    }
}

bool
AsmJSMachExceptionHandler::install(JSRuntime *rt)
{
    JS_ASSERT(!installed());
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
HandleSignal(int signum, siginfo_t *info, void *ctx)
{
    CONTEXT *context = (CONTEXT *)ctx;
    uint8_t **ppc = ContextToPC(context);
    uint8_t *pc = *ppc;

    void *faultingAddress = info->si_addr;

    JSRuntime *rt = RuntimeForCurrentThread();

    
    if (!rt || rt->handlingSignal)
        return false;
    AutoSetHandlingSignal handling(rt);

    if (rt->jitRuntime() && rt->jitRuntime()->handleAccessViolation(rt, faultingAddress))
        return true;

    AsmJSActivation *activation = PerThreadData::innermostAsmJSActivation();
    if (!activation)
        return false;

    const AsmJSModule &module = activation->module();
    if (HandleSimulatorInterrupt(rt, activation, faultingAddress)) {
        JSRuntime::AutoLockForInterrupt lock(rt);
        module.unprotectCode(rt);
        return true;
    }

    if (!module.containsFunctionPC(pc))
        return false;

    
    
    
    
    
    if (module.containsFunctionPC(faultingAddress)) {
        activation->setResumePC(pc);
        *ppc = module.interruptExit();

        JSRuntime::AutoLockForInterrupt lock(rt);
        module.unprotectCode(rt);
        return true;
    }

# if defined(JS_CODEGEN_X64)
    
    
    if (!module.maybeHeap() ||
        faultingAddress < module.maybeHeap() ||
        faultingAddress >= module.maybeHeap() + AsmJSBufferProtectedSize)
    {
        return false;
    }

    const AsmJSHeapAccess *heapAccess = module.lookupHeapAccess(pc);
    if (!heapAccess)
        return false;

    
    
    
    
    
    
    if (heapAccess->isLoad())
        SetRegisterToCoercedUndefined(context, heapAccess->isFloat32Load(), heapAccess->loadedReg());
    *ppc += heapAccess->opLength();
    return true;
# else
    return false;
# endif
}

static struct sigaction sPrevHandler;

static void
AsmJSFaultHandler(int signum, siginfo_t *info, void *context)
{
    if (HandleSignal(signum, info, context))
        return;

    
    
    
    
    
    
    
    
    
    
    
    
    if (sPrevHandler.sa_flags & SA_SIGINFO)
        sPrevHandler.sa_sigaction(signum, info, context);
    else if (sPrevHandler.sa_handler == SIG_DFL || sPrevHandler.sa_handler == SIG_IGN)
        sigaction(signum, &sPrevHandler, nullptr);
    else
        sPrevHandler.sa_handler(signum);
}
#endif

#if !defined(XP_MACOSX)
static bool sInstalledHandlers = false;
#endif

bool
js::EnsureAsmJSSignalHandlersInstalled(JSRuntime *rt)
{
#ifdef JS_CODEGEN_NONE
    
    return false;
#endif

    if (IsSignalHandlingBroken())
        return false;

#if defined(XP_MACOSX)
    
    return rt->asmJSMachExceptionHandler.installed() || rt->asmJSMachExceptionHandler.install(rt);
#else
    
    
    if (sInstalledHandlers)
        return true;

# if defined(XP_WIN)
    if (!AddVectoredExceptionHandler(true, AsmJSExceptionHandler))
        return false;
# else
    
    
    
    struct sigaction sigAction;
    sigAction.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigAction.sa_sigaction = &AsmJSFaultHandler;
    sigemptyset(&sigAction.sa_mask);
    if (sigaction(SIGSEGV, &sigAction, &sPrevHandler))
        return false;
# endif

    sInstalledHandlers = true;
#endif
    return true;
}











void
js::RequestInterruptForAsmJSCode(JSRuntime *rt, int interruptModeRaw)
{
    switch (JSRuntime::InterruptMode(interruptModeRaw)) {
      case JSRuntime::RequestInterruptMainThread:
      case JSRuntime::RequestInterruptAnyThread:
        break;
      case JSRuntime::RequestInterruptAnyThreadDontStopIon:
      case JSRuntime::RequestInterruptAnyThreadForkJoin:
        
        
        
        return;
    }

    AsmJSActivation *activation = rt->mainThread.asmJSActivationStack();
    if (!activation)
        return;

    JS_ASSERT(rt->currentThreadOwnsInterruptLock());
    activation->module().protectCode(rt);
}


#if defined(MOZ_ASAN) && defined(JS_STANDALONE) && !defined(_MSC_VER)



extern "C" MOZ_ASAN_BLACKLIST
const char* __asan_default_options() {
    return "allow_user_segv_handler=1";
}
#endif
