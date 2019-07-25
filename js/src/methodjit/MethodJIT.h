





































#if !defined jsjaeger_h__ && defined JS_METHODJIT
#define jsjaeger_h__

#include "jscntxt.h"

#include "assembler/assembler/MacroAssemblerCodeRef.h"

#if !defined JS_CPU_X64 && \
    !defined JS_CPU_X86 && \
    !defined JS_CPU_ARM
# error "Oh no, you should define a platform so this compiles."
#endif

#if !defined(JS_NUNBOX32) && !defined(JS_PUNBOX64)
# error "No boxing format selected."
#endif

namespace js {

struct VMFrame
{
    union Arguments {
        struct {
            void *ptr;
            void *ptr2;
            void *ptr3;
        } x;
    } u;

    VMFrame      *previous;
    void         *unused;
    JSFrameRegs  regs;
    JSContext    *cx;
    Value        *stackLimit;
    JSStackFrame *entryFp;

#if defined(JS_CPU_X86)
    void *savedEBX;
    void *savedEDI;
    void *savedESI;
    void *savedEBP;
    void *savedEIP;

# ifdef JS_NO_FASTCALL
    inline void** returnAddressLocation() {
        return reinterpret_cast<void**>(this) - 3;
    }
# else
    inline void** returnAddressLocation() {
        return reinterpret_cast<void**>(this) - 1;
    }
# endif
#elif defined(JS_CPU_X64)
    void *savedRBX;
# ifdef _WIN64
    void *savedRSI;
    void *savedRDI;
# endif
    void *savedR15;
    void *savedR14;
    void *savedR13;
    void *savedR12;
    void *savedRBP;
    void *savedRIP;

# ifdef _WIN64
    inline void** returnAddressLocation() {
        return reinterpret_cast<void**>(this) - 5;
    }
# else
    inline void** returnAddressLocation() {
        return reinterpret_cast<void**>(this) - 1;
    }
# endif

#elif defined(JS_CPU_ARM)
    void *savedR4;
    void *savedR5;
    void *savedR6;
    void *savedR7;
    void *savedR8;
    void *savedR9;
    void *savedR10;
    void *savedR11;
    void *savedLR;

    inline void** returnAddressLocation() {
        return reinterpret_cast<void**>(this) - 1;
    }
#else
# error "The VMFrame layout isn't defined for your processor architecture!"
#endif

    JSRuntime *runtime() { return cx->runtime; }

    JSStackFrame *&fp() { return regs.fp; }
};

#ifdef JS_CPU_ARM

extern "C" void JaegerStubVeneer(void);
#endif

namespace mjit {
namespace ic {
# if defined JS_POLYIC
    struct PICInfo;
# endif
# if defined JS_MONOIC
    struct MICInfo;
    struct CallICInfo;
# endif
}
}

typedef void (JS_FASTCALL *VoidStub)(VMFrame &);
typedef void (JS_FASTCALL *VoidVpStub)(VMFrame &, Value *);
typedef void (JS_FASTCALL *VoidStubUInt32)(VMFrame &, uint32);
typedef void (JS_FASTCALL *VoidStubInt32)(VMFrame &, int32);
typedef JSBool (JS_FASTCALL *BoolStub)(VMFrame &);
typedef void * (JS_FASTCALL *VoidPtrStub)(VMFrame &);
typedef void * (JS_FASTCALL *VoidPtrStubPC)(VMFrame &, jsbytecode *);
typedef void * (JS_FASTCALL *VoidPtrStubUInt32)(VMFrame &, uint32);
typedef JSObject * (JS_FASTCALL *JSObjStub)(VMFrame &);
typedef JSObject * (JS_FASTCALL *JSObjStubUInt32)(VMFrame &, uint32);
typedef JSObject * (JS_FASTCALL *JSObjStubFun)(VMFrame &, JSFunction *);
typedef void (JS_FASTCALL *VoidStubFun)(VMFrame &, JSFunction *);
typedef JSObject * (JS_FASTCALL *JSObjStubJSObj)(VMFrame &, JSObject *);
typedef void (JS_FASTCALL *VoidStubAtom)(VMFrame &, JSAtom *);
typedef JSString * (JS_FASTCALL *JSStrStub)(VMFrame &);
typedef JSString * (JS_FASTCALL *JSStrStubUInt32)(VMFrame &, uint32);
typedef void (JS_FASTCALL *VoidStubJSObj)(VMFrame &, JSObject *);
typedef void (JS_FASTCALL *VoidStubPC)(VMFrame &, jsbytecode *);
typedef JSBool (JS_FASTCALL *BoolStubUInt32)(VMFrame &f, uint32);
#ifdef JS_MONOIC
typedef void (JS_FASTCALL *VoidStubCallIC)(VMFrame &, js::mjit::ic::CallICInfo *);
typedef void * (JS_FASTCALL *VoidPtrStubCallIC)(VMFrame &, js::mjit::ic::CallICInfo *);
typedef void (JS_FASTCALL *VoidStubMIC)(VMFrame &, js::mjit::ic::MICInfo *);
typedef void * (JS_FASTCALL *VoidPtrStubMIC)(VMFrame &, js::mjit::ic::MICInfo *);
#endif
#ifdef JS_POLYIC
typedef void (JS_FASTCALL *VoidStubPIC)(VMFrame &, js::mjit::ic::PICInfo *);
#endif

namespace mjit {

struct CallSite;

struct JITScript {
    typedef JSC::MacroAssemblerCodeRef CodeRef;
    CodeRef         code;       

    js::mjit::CallSite *callSites;
    uint32          nCallSites;
    void            **nmap;     
#ifdef JS_MONOIC
    ic::MICInfo     *mics;      
    uint32          nMICs;      
    ic::CallICInfo  *callICs;   
    uint32          nCallICs;   
#endif
#ifdef JS_POLYIC
    ic::PICInfo     *pics;      
    uint32          nPICs;      
#endif
    void            *invokeEntry;       
    void            *fastEntry;         
    void            *arityCheckEntry;   

    bool isValidCode(void *ptr) {
        char *jitcode = (char *)code.m_code.executableAddress();
        char *jcheck = (char *)ptr;
        return jcheck >= jitcode && jcheck < jitcode + code.m_size;
    }

    void sweepCallICs();
    void purgeMICs();
    void purgePICs();
    void release();
};


JSBool JaegerShot(JSContext *cx);


JSBool JaegerShotAtSafePoint(JSContext *cx, void *safePoint);

enum CompileStatus
{
    Compile_Okay,
    Compile_Abort,
    Compile_Error
};

void JS_FASTCALL
ProfileStubCall(VMFrame &f);

CompileStatus JS_NEVER_INLINE
TryCompile(JSContext *cx, JSStackFrame *fp);

void
ReleaseScriptCode(JSContext *cx, JSScript *script);

static inline CompileStatus
CanMethodJIT(JSContext *cx, JSScript *script, JSStackFrame *fp)
{
    if (!cx->methodJitEnabled)
        return Compile_Abort;
    JITScriptStatus status = script->getJITStatus(fp->isConstructing());
    if (status == JITScript_Invalid)
        return Compile_Abort;
    if (status == JITScript_None)
        return TryCompile(cx, fp);
    return Compile_Okay;
}

struct CallSite
{
    uint32 codeOffset;
    uint32 pcOffset;
    uint32 id;
};

} 

} 

inline void **
JSScript::maybeNativeMap(bool constructing)
{
    js::mjit::JITScript *jit = constructing ? jitCtor : jitNormal;
    if (!jit)
        return NULL;
    return jit->nmap;
}

inline bool
JSScript::hasNativeCodeForPC(bool constructing, jsbytecode *pc)
{
    js::mjit::JITScript *jit = getJIT(constructing);
    if (!jit)
        return false;
    JS_ASSERT(pc >= code && pc < code + length);
    return !!jit->nmap[pc - code];
}

#ifdef _MSC_VER
extern "C" void *JaegerThrowpoline(js::VMFrame *vmFrame);
#else
extern "C" void JaegerThrowpoline();
#endif
extern "C" void InjectJaegerReturn();

#endif 

