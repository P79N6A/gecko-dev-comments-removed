





































#if !defined jsjaeger_h__ && defined JS_METHODJIT
#define jsjaeger_h__

#include "jscntxt.h"

#include "assembler/assembler/MacroAssemblerCodeRef.h"

#if !defined JS_CPU_X64 && \
    !defined JS_CPU_X86 && \
    !defined JS_CPU_ARM
# error "Oh no, you should define a platform so this compiles."
#endif

#if !defined(JS_32BIT) && !defined(JS_64BIT)
# error "This processor is UNKNOWN."
#endif

namespace js {

struct VMFrame
{
#if defined(JS_CPU_ARM)
    JSC::ReturnAddressPtr   veneerReturn;
#endif

    
    void *scriptedReturn;

#if defined(JS_CPU_X86)
    uintptr_t               padding[2];
#elif defined(JS_CPU_ARM)
    uintptr_t               padding;
#endif

    union Arguments {
        struct DEFVAR {
            jsatomid index;
            uint32   op;
        } defvar;
        struct LAMBDA {
            uint32 index;
            JSOp   op;
        } lambda;
        struct INITPROP {
            uint32   index;
            unsigned defineHow;
        } initprop;
        struct DEFLOCALFUN {
            uint32   slot;
            uint32   index;
        } deflocalfun;
        struct TRACER {
            uint32 traceId;
            uint32 offs;
        } tracer;
        struct SETTER {
            jsatomid index;
            JSOp     op;
        } setter;
    } u;

    JSFrameRegs  *oldRegs;
    JSFrameRegs  regs;
    JSStackFrame *fp;
    JSContext    *cx;
    uintptr_t    inlineCallCount;

#if defined(JS_CPU_X86)
    void *savedEBX;
    void *savedEDI;
    void *savedESI;
    void *savedEBP;
    void *savedEIP;

    inline void setReturnAddress(JSC::ReturnAddressPtr addr) {
        *(reinterpret_cast<JSC::ReturnAddressPtr*>(this)-1) = addr;
    }

    inline JSC::ReturnAddressPtr getReturnAddress() const {
        return *(reinterpret_cast<const JSC::ReturnAddressPtr*>(this)-1);
    }

#elif defined(JS_CPU_X64)
    void *savedRBX;
#ifdef _MSC_VER
    void *savedRSI;
    void *savedRDI;
#endif
    void *savedR15;
    void *savedR14;
    void *savedR13;
    void *savedR12;
    void *savedRBP;
    void *savedRIP;

#ifdef _MSC_VER
    inline void setReturnAddress(JSC::ReturnAddressPtr addr) {
        *(reinterpret_cast<JSC::ReturnAddressPtr*>(this)-5) = addr;
    }

    inline JSC::ReturnAddressPtr getReturnAddress() const {
        return *(reinterpret_cast<const JSC::ReturnAddressPtr*>(this)-5);
    }
#else
    inline void setReturnAddress(JSC::ReturnAddressPtr addr) {
        *(reinterpret_cast<JSC::ReturnAddressPtr*>(this)-1) = addr;
    }

    inline JSC::ReturnAddressPtr getReturnAddress() const {
        return *(reinterpret_cast<const JSC::ReturnAddressPtr*>(this)-1);
    }
#endif

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

    inline void setReturnAddress(JSC::ReturnAddressPtr addr) {
        this->veneerReturn = addr;
    }

    inline JSC::ReturnAddressPtr getReturnAddress() {
        return this->veneerReturn;
    }

#else
# error "The VMFrame layout isn't defined for your processor architecture!"
#endif

    JSRuntime *runtime() { return cx->runtime; }
};

#ifdef JS_CPU_ARM
extern "C" void JaegerStubVeneer(void);
#endif

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
typedef JSObject * (JS_FASTCALL *JSObjStubJSObj)(VMFrame &f, JSObject *);
typedef void (JS_FASTCALL *VoidStubAtom)(VMFrame &f, JSAtom *);

#define JS_UNJITTABLE_METHOD (reinterpret_cast<void*>(-1))

namespace mjit {

JSBool
JaegerShot(JSContext *cx);

enum CompileStatus
{
    Compile_Okay,
    Compile_Abort,
    Compile_Error
};

CompileStatus
TryCompile(JSContext *cx, JSScript *script, JSFunction *fun, JSObject *scopeChain);

void
ReleaseScriptCode(JSContext *cx, JSScript *script);

static inline CompileStatus
CanMethodJIT(JSContext *cx, JSScript *script, JSFunction *fun, JSObject *scopeChain)
{
    if (!(cx->options & JSOPTION_METHODJIT) || script->ncode == JS_UNJITTABLE_METHOD)
        return Compile_Abort;
    if (script->ncode == NULL)
        return TryCompile(cx, script, fun, scopeChain);
    return Compile_Okay;
}

} 

} 

#ifdef _MSC_VER
extern "C" void *JaegerThrowpoline(js::VMFrame *vmFrame);
#else
extern "C" void JaegerThrowpoline();
#endif
extern "C" void JaegerFromTracer();

#endif 
