






#include "jscntxt.h"

#include "jstypedarrayinlines.h"

#include "ion/AsmJS.h"
#include "ion/AsmJSModule.h"
#include "assembler/assembler/MacroAssembler.h"

using namespace js;
using namespace js::ion;

#ifdef JS_ASMJS




#if !defined(XP_MACOSX)
static AsmJSActivation *
InnermostAsmJSActivation()
{
    PerThreadData *threadData = TlsPerThreadData.get();
    if (!threadData)
        return NULL;

    return threadData->asmJSActivationStackFromOwnerThread();
}
#endif




#if !defined(XP_MACOSX)
# ifdef JS_THREADSAFE
#  include "jslock.h"

class InstallSignalHandlersMutex
{
    PRLock *mutex_;

  public:
    InstallSignalHandlersMutex() {
        mutex_ = PR_NewLock();
        if (!mutex_)
            MOZ_CRASH();
    }
    ~InstallSignalHandlersMutex() {
        PR_DestroyLock(mutex_);
    }
    class Lock {
        static bool sHandlersInstalled;
      public:
        Lock();
        ~Lock();
        bool handlersInstalled() const { return sHandlersInstalled; }
        void setHandlersInstalled() { sHandlersInstalled = true; }
    };
} signalMutex;

bool InstallSignalHandlersMutex::Lock::sHandlersInstalled = false;

InstallSignalHandlersMutex::Lock::Lock()
{
    PR_Lock(signalMutex.mutex_);
}

InstallSignalHandlersMutex::Lock::~Lock()
{
    PR_Unlock(signalMutex.mutex_);
}
# else  
struct InstallSignalHandlersMutex
{
    class Lock {
        static bool sHandlersInstalled;
      public:
        Lock() { (void)this; }
        bool handlersInstalled() const { return sHandlersInstalled; }
        void setHandlersInstalled() { sHandlersInstalled = true; }
    };
};

bool InstallSignalHandlersMutex::Lock::sHandlersInstalled = false;
# endif  
#endif   

# if defined(JS_CPU_X64)
template <class T>
static void
SetXMMRegToNaN(bool isFloat32, T *xmm_reg)
{
    if (isFloat32) {
        JS_STATIC_ASSERT(sizeof(T) == 4 * sizeof(float));
        float *floats = reinterpret_cast<float*>(xmm_reg);
        floats[0] = js_NaN;
        floats[1] = 0;
        floats[2] = 0;
        floats[3] = 0;
    } else {
        JS_STATIC_ASSERT(sizeof(T) == 2 * sizeof(double));
        double *dbls = reinterpret_cast<double*>(xmm_reg);
        dbls[0] = js_NaN;
        dbls[1] = 0;
    }
}



static const AsmJSHeapAccess *
LookupHeapAccess(const AsmJSModule &module, uint8_t *pc)
{
    JS_ASSERT(module.containsPC(pc));
    size_t targetOffset = pc - module.functionCode();

    if (module.numHeapAccesses() == 0)
        return NULL;

    size_t low = 0;
    size_t high = module.numHeapAccesses() - 1;
    while (high - low >= 2) {
        size_t mid = low + (high - low) / 2;
        uint32_t midOffset = module.heapAccess(mid).offset();
        if (targetOffset == midOffset)
            return &module.heapAccess(mid);
        if (targetOffset < midOffset)
            high = mid;
        else
            low = mid;
    }
    if (targetOffset == module.heapAccess(low).offset())
        return &module.heapAccess(low);
    if (targetOffset == module.heapAccess(high).offset())
        return &module.heapAccess(high);

    return NULL;
}
# endif

# if defined(XP_WIN)
#  include "jswin.h"

static uint8_t **
ContextToPC(PCONTEXT context)
{
#  if defined(JS_CPU_X64)
    JS_STATIC_ASSERT(sizeof(context->Rip) == sizeof(void*));
    return reinterpret_cast<uint8_t**>(&context->Rip);
#  else
    JS_STATIC_ASSERT(sizeof(context->Eip) == sizeof(void*));
    return reinterpret_cast<uint8_t**>(&context->Eip);
#  endif
}

#  if defined(JS_CPU_X64)
static void
SetRegisterToCoercedUndefined(CONTEXT *context, bool isFloat32, AnyRegister reg)
{
    if (reg.isFloat()) {
        switch (reg.fpu().code()) {
          case JSC::X86Registers::xmm0:  SetXMMRegToNaN(isFloat32, &context->Xmm0); break;
          case JSC::X86Registers::xmm1:  SetXMMRegToNaN(isFloat32, &context->Xmm1); break;
          case JSC::X86Registers::xmm2:  SetXMMRegToNaN(isFloat32, &context->Xmm2); break;
          case JSC::X86Registers::xmm3:  SetXMMRegToNaN(isFloat32, &context->Xmm3); break;
          case JSC::X86Registers::xmm4:  SetXMMRegToNaN(isFloat32, &context->Xmm4); break;
          case JSC::X86Registers::xmm5:  SetXMMRegToNaN(isFloat32, &context->Xmm5); break;
          case JSC::X86Registers::xmm6:  SetXMMRegToNaN(isFloat32, &context->Xmm6); break;
          case JSC::X86Registers::xmm7:  SetXMMRegToNaN(isFloat32, &context->Xmm7); break;
          case JSC::X86Registers::xmm8:  SetXMMRegToNaN(isFloat32, &context->Xmm8); break;
          case JSC::X86Registers::xmm9:  SetXMMRegToNaN(isFloat32, &context->Xmm9); break;
          case JSC::X86Registers::xmm10: SetXMMRegToNaN(isFloat32, &context->Xmm10); break;
          case JSC::X86Registers::xmm11: SetXMMRegToNaN(isFloat32, &context->Xmm11); break;
          case JSC::X86Registers::xmm12: SetXMMRegToNaN(isFloat32, &context->Xmm12); break;
          case JSC::X86Registers::xmm13: SetXMMRegToNaN(isFloat32, &context->Xmm13); break;
          case JSC::X86Registers::xmm14: SetXMMRegToNaN(isFloat32, &context->Xmm14); break;
          case JSC::X86Registers::xmm15: SetXMMRegToNaN(isFloat32, &context->Xmm15); break;
          default: MOZ_CRASH();
        }
    } else {
        switch (reg.gpr().code()) {
          case JSC::X86Registers::eax: context->Rax = 0; break;
          case JSC::X86Registers::ecx: context->Rcx = 0; break;
          case JSC::X86Registers::edx: context->Rdx = 0; break;
          case JSC::X86Registers::ebx: context->Rbx = 0; break;
          case JSC::X86Registers::esp: context->Rsp = 0; break;
          case JSC::X86Registers::ebp: context->Rbp = 0; break;
          case JSC::X86Registers::esi: context->Rsi = 0; break;
          case JSC::X86Registers::edi: context->Rdi = 0; break;
          case JSC::X86Registers::r8:  context->R8  = 0; break;
          case JSC::X86Registers::r9:  context->R9  = 0; break;
          case JSC::X86Registers::r10: context->R10 = 0; break;
          case JSC::X86Registers::r11: context->R11 = 0; break;
          case JSC::X86Registers::r12: context->R12 = 0; break;
          case JSC::X86Registers::r13: context->R13 = 0; break;
          case JSC::X86Registers::r14: context->R14 = 0; break;
          case JSC::X86Registers::r15: context->R15 = 0; break;
          default: MOZ_CRASH();
        }
    }
}
#  endif

static bool
HandleException(PEXCEPTION_POINTERS exception)
{
    EXCEPTION_RECORD *record = exception->ExceptionRecord;
    CONTEXT *context = exception->ContextRecord;

    if (record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return false;

    AsmJSActivation *activation = InnermostAsmJSActivation();
    if (!activation)
        return false;

    uint8_t **ppc = ContextToPC(context);
    uint8_t *pc = *ppc;
	JS_ASSERT(pc == record->ExceptionAddress);

    const AsmJSModule &module = activation->module();
    if (!module.containsPC(pc))
        return false;

	if (record->NumberParameters < 2)
		return false;

    void *faultingAddress = (void*)record->ExceptionInformation[1];

    
    
    
    
    
    if (module.containsPC(faultingAddress)) {
        activation->setResumePC(pc);
        *ppc = module.operationCallbackExit();
        DWORD oldProtect;
        if (!VirtualProtect(module.functionCode(), module.functionBytes(), PAGE_EXECUTE, &oldProtect))
            MOZ_CRASH();
        return true;
    }

# if defined(JS_CPU_X64)
    
    
    if (!module.maybeHeap() ||
        faultingAddress < module.maybeHeap() ||
        faultingAddress >= module.maybeHeap() + AsmJSBufferProtectedSize)
    {
        return false;
    }

    const AsmJSHeapAccess *heapAccess = LookupHeapAccess(module, pc);
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

# elif defined(XP_MACOSX)
#  include <sys/mman.h>
#  include <mach/exc.h>

static uint8_t **
ContextToPC(x86_thread_state_t &state)
{
#  if defined(JS_CPU_X64)
    JS_STATIC_ASSERT(sizeof(state.uts.ts64.__rip) == sizeof(void*));
    return reinterpret_cast<uint8_t**>(&state.uts.ts64.__rip);
#  else
    JS_STATIC_ASSERT(sizeof(state.uts.ts32.__eip) == sizeof(void*));
    return reinterpret_cast<uint8_t**>(&state.uts.ts32.__eip);
#  endif
}

#  if defined(JS_CPU_X64)
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
#  endif



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
    
    mach_port_t rtThread = request.body.thread.name;

    
    x86_thread_state_t state;
    unsigned int count = x86_THREAD_STATE_COUNT;
    kern_return_t kret;
    kret = thread_get_state(rtThread, x86_THREAD_STATE, (thread_state_t)&state, &count);
    if (kret != KERN_SUCCESS)
        return false;

    AsmJSActivation *activation = rt->mainThread.asmJSActivationStackFromAnyThread();
    if (!activation)
        return false;

    uint8_t **ppc = ContextToPC(state);
    uint8_t *pc = *ppc;

    const AsmJSModule &module = activation->module();
    if (!module.containsPC(pc))
        return false;

    if (request.body.exception != EXC_BAD_ACCESS || request.body.codeCnt != 2)
        return false;

    void *faultingAddress = (void*)request.body.code[1];

    
    
    
    
    
    if (module.containsPC(faultingAddress)) {
        activation->setResumePC(pc);
        *ppc = module.operationCallbackExit();
        mprotect(module.functionCode(), module.functionBytes(), PROT_EXEC);

        
        kret = thread_set_state(rtThread, x86_THREAD_STATE, (thread_state_t)&state, x86_THREAD_STATE_COUNT);
        return kret == KERN_SUCCESS;
    }

#  if defined(JS_CPU_X64)
    
    
    if (!module.maybeHeap() ||
        faultingAddress < module.maybeHeap() ||
        faultingAddress >= module.maybeHeap() + AsmJSBufferProtectedSize)
    {
        return false;
    }

    const AsmJSHeapAccess *heapAccess = LookupHeapAccess(module, pc);
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
#  else
    return false;
#  endif
}

void *
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

    JS_NOT_REACHED("The exception handler thread should never return");
    return NULL;
}

AsmJSMachExceptionHandler::AsmJSMachExceptionHandler()
  : installed_(false),
    thread_(NULL),
    port_(MACH_PORT_NULL)
{}

void
AsmJSMachExceptionHandler::release()
{
    if (installed_) {
        clearCurrentThread();
        installed_ = false;
    }
    if (thread_ != NULL) {
        pthread_cancel(thread_);
        thread_ = NULL;
    }
    if (port_ != MACH_PORT_NULL) {
        mach_port_deallocate(mach_task_self(), port_);
        port_ = MACH_PORT_NULL;
    }
}

void
AsmJSMachExceptionHandler::clearCurrentThread()
{
    if (!installed_)
        return;

    kern_return_t kret = thread_set_exception_ports(mach_thread_self(),
                                                    EXC_MASK_BAD_ACCESS,
                                                    MACH_PORT_NULL,
                                                    EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
                                                    THREAD_STATE_NONE);
    if (kret != KERN_SUCCESS)
        MOZ_CRASH();
}

void
AsmJSMachExceptionHandler::setCurrentThread()
{
    if (!installed_)
        return;

    kern_return_t kret = thread_set_exception_ports(mach_thread_self(),
                                                    EXC_MASK_BAD_ACCESS,
                                                    port_,
                                                    EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
                                                    THREAD_STATE_NONE);
    if (kret != KERN_SUCCESS)
        MOZ_CRASH();
}

bool
AsmJSMachExceptionHandler::install(JSRuntime *rt)
{
    JS_ASSERT(!installed());
    kern_return_t kret;

    
    kret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port_);
    if (kret != KERN_SUCCESS)
        goto error;
    kret = mach_port_insert_right(mach_task_self(), port_, port_, MACH_MSG_TYPE_MAKE_SEND);
    if (kret != KERN_SUCCESS)
        goto error;

    
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread_, &attrs, AsmJSMachExceptionHandlerThread, rt))
        goto error;

    
    
    
    
    
    kret = thread_set_exception_ports(mach_thread_self(),
                                      EXC_MASK_BAD_ACCESS,
                                      port_,
                                      EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
                                      THREAD_STATE_NONE);
    if (kret != KERN_SUCCESS)
        goto error;

    installed_ = true;
    return true;

  error:
    release();
    return false;
}

# else  
#  include <signal.h>
#  include <sys/mman.h>



static uint8_t **
ContextToPC(mcontext_t &context)
{
#  if defined(JS_CPU_X86)
    JS_STATIC_ASSERT(sizeof(context.gregs[REG_EIP]) == sizeof(void*));
    return reinterpret_cast<uint8_t**>(&context.gregs[REG_EIP]);
#  else
    JS_STATIC_ASSERT(sizeof(context.gregs[REG_RIP]) == sizeof(void*));
    return reinterpret_cast<uint8_t**>(&context.gregs[REG_RIP]);
#  endif
}

#  if defined(JS_CPU_X64)
static void
SetRegisterToCoercedUndefined(mcontext_t &context, bool isFloat32, AnyRegister reg)
{
    if (reg.isFloat()) {
        switch (reg.fpu().code()) {
          case JSC::X86Registers::xmm0:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[0]); break;
          case JSC::X86Registers::xmm1:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[1]); break;
          case JSC::X86Registers::xmm2:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[2]); break;
          case JSC::X86Registers::xmm3:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[3]); break;
          case JSC::X86Registers::xmm4:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[4]); break;
          case JSC::X86Registers::xmm5:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[5]); break;
          case JSC::X86Registers::xmm6:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[6]); break;
          case JSC::X86Registers::xmm7:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[7]); break;
          case JSC::X86Registers::xmm8:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[8]); break;
          case JSC::X86Registers::xmm9:  SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[9]); break;
          case JSC::X86Registers::xmm10: SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[10]); break;
          case JSC::X86Registers::xmm11: SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[11]); break;
          case JSC::X86Registers::xmm12: SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[12]); break;
          case JSC::X86Registers::xmm13: SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[13]); break;
          case JSC::X86Registers::xmm14: SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[14]); break;
          case JSC::X86Registers::xmm15: SetXMMRegToNaN(isFloat32, &context.fpregs->_xmm[15]); break;
          default: MOZ_CRASH();
        }
    } else {
        switch (reg.gpr().code()) {
          case JSC::X86Registers::eax: context.gregs[REG_RAX] = 0; break;
          case JSC::X86Registers::ecx: context.gregs[REG_RCX] = 0; break;
          case JSC::X86Registers::edx: context.gregs[REG_RDX] = 0; break;
          case JSC::X86Registers::ebx: context.gregs[REG_RBX] = 0; break;
          case JSC::X86Registers::esp: context.gregs[REG_RSP] = 0; break;
          case JSC::X86Registers::ebp: context.gregs[REG_RBP] = 0; break;
          case JSC::X86Registers::esi: context.gregs[REG_RSI] = 0; break;
          case JSC::X86Registers::edi: context.gregs[REG_RDI] = 0; break;
          case JSC::X86Registers::r8:  context.gregs[REG_R8]  = 0; break;
          case JSC::X86Registers::r9:  context.gregs[REG_R9]  = 0; break;
          case JSC::X86Registers::r10: context.gregs[REG_R10] = 0; break;
          case JSC::X86Registers::r11: context.gregs[REG_R11] = 0; break;
          case JSC::X86Registers::r12: context.gregs[REG_R12] = 0; break;
          case JSC::X86Registers::r13: context.gregs[REG_R13] = 0; break;
          case JSC::X86Registers::r14: context.gregs[REG_R14] = 0; break;
          case JSC::X86Registers::r15: context.gregs[REG_R15] = 0; break;
          default: MOZ_CRASH();
        }
    }
}
#  endif



static bool
HandleSignal(int signum, siginfo_t *info, void *ctx)
{
    AsmJSActivation *activation = InnermostAsmJSActivation();
    if (!activation)
        return false;

    mcontext_t &context = reinterpret_cast<ucontext_t*>(ctx)->uc_mcontext;
    uint8_t **ppc = ContextToPC(context);
    uint8_t *pc = *ppc;

    const AsmJSModule &module = activation->module();
    if (!module.containsPC(pc))
        return false;

    void *faultingAddress = info->si_addr;

    
    
    
    
    
    if (module.containsPC(faultingAddress)) {
        activation->setResumePC(pc);
        *ppc = module.operationCallbackExit();
        mprotect(module.functionCode(), module.functionBytes(), PROT_EXEC);
        return true;
    }

#  if defined(JS_CPU_X64)
    
    
    if (!module.maybeHeap() ||
        faultingAddress < module.maybeHeap() ||
        faultingAddress >= module.maybeHeap() + AsmJSBufferProtectedSize)
    {
        return false;
    }

    const AsmJSHeapAccess *heapAccess = LookupHeapAccess(module, pc);
    if (!heapAccess)
        return false;

    
    
    
    
    
    
    if (heapAccess->isLoad())
        SetRegisterToCoercedUndefined(context, heapAccess->isFloat32Load(), heapAccess->loadedReg());
    *ppc += heapAccess->opLength();
    return true;
#  else
    return false;
#  endif
}

static struct sigaction sPrevHandler;

static void
AsmJSFaultHandler(int signum, siginfo_t *info, void *context)
{
    if (HandleSignal(signum, info, context))
        return;

    
    
    
    
    
    
    
    if (sPrevHandler.sa_flags & SA_SIGINFO) {
        sPrevHandler.sa_sigaction(signum, info, context);
        exit(signum);  
    } else if (sPrevHandler.sa_handler == SIG_DFL || sPrevHandler.sa_handler == SIG_IGN) {
        sigaction(signum, &sPrevHandler, NULL);
    } else {
        sPrevHandler.sa_handler(signum);
        exit(signum);  
    }
}
# endif
#endif 

bool
EnsureAsmJSSignalHandlersInstalled(JSRuntime *rt)
{
#if defined(JS_ASMJS)
# if defined(XP_MACOSX)
    
    return rt->asmJSMachExceptionHandler.installed() || rt->asmJSMachExceptionHandler.install(rt);
# else
    
    
    InstallSignalHandlersMutex::Lock lock;
    if (lock.handlersInstalled())
        return true;

#  if defined(XP_WIN)
    if (!AddVectoredExceptionHandler(true, AsmJSExceptionHandler))
        return false;
#  else  
    struct sigaction sigAction;
    sigAction.sa_sigaction = &AsmJSFaultHandler;
    sigemptyset(&sigAction.sa_mask);
    sigAction.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &sigAction, &sPrevHandler))
        return false;
    if (sigaction(SIGBUS, &sigAction, &sPrevHandler))
        return false;
#  endif

    lock.setHandlersInstalled();
# endif
#endif
    return true;
}











void
js::TriggerOperationCallbackForAsmJSCode(JSRuntime *rt)
{
#if defined(JS_ASMJS)
    PerThreadData::AsmJSActivationStackLock lock(rt->mainThread);

    AsmJSActivation *activation = rt->mainThread.asmJSActivationStackFromAnyThread();
    if (!activation)
        return;

    const AsmJSModule &module = activation->module();

# if defined(XP_WIN)
    DWORD oldProtect;
    if (!VirtualProtect(module.functionCode(), 4096, PAGE_NOACCESS, &oldProtect))
        MOZ_CRASH();
# else  
    if (mprotect(module.functionCode(), module.functionBytes(), PROT_NONE))
        MOZ_CRASH();
# endif
#endif
}
