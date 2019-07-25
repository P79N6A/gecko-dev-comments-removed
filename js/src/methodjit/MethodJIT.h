





































#if !defined jsjaeger_h__ && defined JS_METHODJIT
#define jsjaeger_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/assembler/CodeLocation.h"

#if !defined JS_CPU_X64 && \
    !defined JS_CPU_X86 && \
    !defined JS_CPU_SPARC && \
    !defined JS_CPU_ARM
# error "Oh no, you should define a platform so this compiles."
#endif

#if !defined(JS_NUNBOX32) && !defined(JS_PUNBOX64)
# error "No boxing format selected."
#endif

namespace js {

namespace mjit { struct JITScript; }

struct VMFrame
{
#if defined(JS_CPU_SPARC)
    void *savedL0;
    void *savedL1;
    void *savedL2;
    void *savedL3;
    void *savedL4;
    void *savedL5;
    void *savedL6;
    void *savedL7;
    void *savedI0;
    void *savedI1;
    void *savedI2;
    void *savedI3;
    void *savedI4;
    void *savedI5;
    void *savedI6;
    void *savedI7;

    void *str_p;

    void *outgoing_p0;
    void *outgoing_p1;
    void *outgoing_p2;
    void *outgoing_p3;
    void *outgoing_p4;
    void *outgoing_p5;

    void *outgoing_p6;

    void *reserve_0;
    void *reserve_1;
#endif

    union Arguments {
        struct {
            void *ptr;
            void *ptr2;
        } x;
        struct {
            uint32 lazyArgsObj;
            uint32 dynamicArgc;
        } call;
    } u;

    VMFrame      *previous;
    void         *scratch;
    FrameRegs    regs;
    JSContext    *cx;
    Value        *stackLimit;
    StackFrame   *entryfp;
    FrameRegs    *oldregs;
    JSRejoinState stubRejoin;  

#if defined(JS_CPU_X86)
    void         *unused0, *unused1;  
#endif

#if defined(JS_CPU_X86)
    void *savedEBX;
    void *savedEDI;
    void *savedESI;
    void *savedEBP;
    void *savedEIP;

# ifdef JS_NO_FASTCALL
    inline void** returnAddressLocation() {
        return reinterpret_cast<void**>(this) - 5;
    }
# else
    inline void** returnAddressLocation() {
        return reinterpret_cast<void**>(this) - 1;
    }
# endif

    
    static const uint32 STACK_BASE_DIFFERENCE = 0x38;

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
#elif defined(JS_CPU_SPARC)
    JSStackFrame *topRetrunAddr;
    void* veneerReturn;
    void* _align;
    inline void** returnAddressLocation() {
        return reinterpret_cast<void**>(&this->veneerReturn);
    }
#else
# error "The VMFrame layout isn't defined for your processor architecture!"
#endif

    JSRuntime *runtime() { return cx->runtime; }

    





    StackFrame *fp() { return regs.fp(); }
    mjit::JITScript *jit() { return fp()->jit(); }

    
    inline JSScript *script();
    inline jsbytecode *pc();

#if defined(JS_CPU_SPARC)
    static const size_t offsetOfFp = 30 * sizeof(void *) + FrameRegs::offsetOfFp;
    static const size_t offsetOfInlined = 30 * sizeof(void *) + FrameRegs::offsetOfInlined;
#else
    static const size_t offsetOfFp = 4 * sizeof(void *) + FrameRegs::offsetOfFp;
    static const size_t offsetOfInlined = 4 * sizeof(void *) + FrameRegs::offsetOfInlined;
#endif

    static void staticAssert() {
        JS_STATIC_ASSERT(offsetOfFp == offsetof(VMFrame, regs) + FrameRegs::offsetOfFp);
        JS_STATIC_ASSERT(offsetOfInlined == offsetof(VMFrame, regs) + FrameRegs::offsetOfInlined);
    }
};

#if defined(JS_CPU_ARM) || defined(JS_CPU_SPARC)

extern "C" void JaegerStubVeneer(void);
#endif

namespace mjit {






enum RejoinState {
    








    REJOIN_SCRIPTED = 1,

    
    REJOIN_NONE,

    
    REJOIN_RESUME,

    



    REJOIN_TRAP,

    
    REJOIN_FALLTHROUGH,

    





    REJOIN_NATIVE,
    REJOIN_NATIVE_LOWERED,
    REJOIN_NATIVE_GETTER,

    




    REJOIN_NATIVE_PATCHED,

    
    REJOIN_PUSH_BOOLEAN,
    REJOIN_PUSH_OBJECT,

    
    REJOIN_DEFLOCALFUN,

    



    REJOIN_THIS_PROTOTYPE,

    



    REJOIN_CHECK_ARGUMENTS,

    



    REJOIN_FUNCTION_PROLOGUE,

    



    REJOIN_CALL_PROLOGUE,
    REJOIN_CALL_PROLOGUE_LOWERED_CALL,
    REJOIN_CALL_PROLOGUE_LOWERED_APPLY,

    
    REJOIN_CALL_SPLAT,

    
    REJOIN_GETTER,
    REJOIN_POS,
    REJOIN_BINARY,

    



    REJOIN_BRANCH
};


struct RecompilationMonitor
{
    JSContext *cx;

    




    unsigned recompilations;
    unsigned frameExpansions;

    
    unsigned gcNumber;

    RecompilationMonitor(JSContext *cx)
        : cx(cx),
          recompilations(cx->compartment->types.recompilations),
          frameExpansions(cx->compartment->types.frameExpansions),
          gcNumber(cx->runtime->gcNumber)
    {}

    bool recompiled() {
        return cx->compartment->types.recompilations != recompilations
            || cx->compartment->types.frameExpansions != frameExpansions
            || cx->runtime->gcNumber != gcNumber;
    }
};





struct Trampolines {
    typedef void (*TrampolinePtr)();

    TrampolinePtr       forceReturn;
    JSC::ExecutablePool *forceReturnPool;

#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
    TrampolinePtr       forceReturnFast;
    JSC::ExecutablePool *forceReturnFastPool;
#endif
};


enum JaegerStatus
{
    
    Jaeger_Throwing = 0,

    
    Jaeger_Returned = 1,

    



    Jaeger_Unfinished = 2,

    




    Jaeger_UnfinishedAtTrap = 3
};







class JaegerCompartment {
    JSC::ExecutableAllocator *execAlloc_;    
    Trampolines              trampolines;    
    VMFrame                  *activeFrame_;  
    JaegerStatus             lastUnfinished_;
                                             

    void Finish();

  public:
    bool Initialize();

    JaegerCompartment();
    ~JaegerCompartment() { Finish(); }

    JSC::ExecutableAllocator *execAlloc() {
        return execAlloc_;
    }

    VMFrame *activeFrame() {
        return activeFrame_;
    }

    void pushActiveFrame(VMFrame *f) {
        JS_ASSERT(!lastUnfinished_);
        f->previous = activeFrame_;
        f->scratch = NULL;
        activeFrame_ = f;
    }

    void popActiveFrame() {
        JS_ASSERT(activeFrame_);
        activeFrame_ = activeFrame_->previous;
    }

    void setLastUnfinished(JaegerStatus status) {
        JS_ASSERT(!lastUnfinished_);
        lastUnfinished_ = status;
    }

    JaegerStatus lastUnfinished() {
        JaegerStatus result = lastUnfinished_;
        lastUnfinished_ = (JaegerStatus) 0;
        return result;
    }

    void *forceReturnFromExternC() const {
        return JS_FUNC_TO_DATA_PTR(void *, trampolines.forceReturn);
    }

    void *forceReturnFromFastCall() const {
#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
        return JS_FUNC_TO_DATA_PTR(void *, trampolines.forceReturnFast);
#else
        return JS_FUNC_TO_DATA_PTR(void *, trampolines.forceReturn);
#endif
    }

    




    Vector<StackFrame *, 8, SystemAllocPolicy> orphanedNativeFrames;
    Vector<JSC::ExecutablePool *, 8, SystemAllocPolicy> orphanedNativePools;
};








class CompilerAllocPolicy : public TempAllocPolicy
{
    bool *oomFlag;

    void *checkAlloc(void *p) {
        if (!p)
            *oomFlag = true;
        return p;
    }

  public:
    CompilerAllocPolicy(JSContext *cx, bool *oomFlag)
    : TempAllocPolicy(cx), oomFlag(oomFlag) {}
    CompilerAllocPolicy(JSContext *cx, Compiler &compiler);

    void *malloc_(size_t bytes) { return checkAlloc(TempAllocPolicy::malloc_(bytes)); }
    void *realloc_(void *p, size_t oldBytes, size_t bytes) {
        return checkAlloc(TempAllocPolicy::realloc_(p, oldBytes, bytes));
    }
};

namespace ic {
# if defined JS_POLYIC
    struct PICInfo;
    struct GetElementIC;
    struct SetElementIC;
# endif
# if defined JS_MONOIC
    struct GetGlobalNameIC;
    struct SetGlobalNameIC;
    struct EqualityICInfo;
    struct TraceICInfo;
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
typedef void (JS_FASTCALL *VoidStubGetGlobal)(VMFrame &, js::mjit::ic::GetGlobalNameIC *);
typedef void (JS_FASTCALL *VoidStubSetGlobal)(VMFrame &, js::mjit::ic::SetGlobalNameIC *);
typedef JSBool (JS_FASTCALL *BoolStubEqualityIC)(VMFrame &, js::mjit::ic::EqualityICInfo *);
typedef void * (JS_FASTCALL *VoidPtrStubTraceIC)(VMFrame &, js::mjit::ic::TraceICInfo *);
#endif
#ifdef JS_POLYIC
typedef void (JS_FASTCALL *VoidStubPIC)(VMFrame &, js::mjit::ic::PICInfo *);
typedef void (JS_FASTCALL *VoidStubGetElemIC)(VMFrame &, js::mjit::ic::GetElementIC *);
typedef void (JS_FASTCALL *VoidStubSetElemIC)(VMFrame &f, js::mjit::ic::SetElementIC *);
#endif

namespace mjit {

struct InlineFrame;
struct CallSite;

struct NativeMapEntry {
    size_t          bcOff;  
    void            *ncode; 
};


struct PCLengthEntry {
    double          codeLength; 
    double          picsLength; 
};






struct NativeCallStub {
    
    jsbytecode *pc;
    CallSite *inlined;

    
    JSC::ExecutablePool *pool;

    




#ifdef JS_CPU_X64
    JSC::CodeLocationDataLabelPtr jump;
#else
    JSC::CodeLocationJump jump;
#endif
};

struct JITScript {
    typedef JSC::MacroAssemblerCodeRef CodeRef;
    CodeRef         code;       

    JSScript        *script;

    void            *invokeEntry;       
    void            *fastEntry;         
    void            *arityCheckEntry;   
    void            *argsCheckEntry;    

    PCLengthEntry   *pcLengths;         

    







    uint32          nNmapPairs:31;      

    bool            singleStepMode:1;   
    uint32          nInlineFrames;
    uint32          nCallSites;
#ifdef JS_MONOIC
    uint32          nGetGlobalNames;
    uint32          nSetGlobalNames;
    uint32          nCallICs;
    uint32          nEqualityICs;
    uint32          nTraceICs;
#endif
#ifdef JS_POLYIC
    uint32          nGetElems;
    uint32          nSetElems;
    uint32          nPICs;
#endif

#ifdef JS_MONOIC
    
    JSC::CodeLocationLabel argsCheckStub;
    JSC::CodeLocationLabel argsCheckFallthrough;
    JSC::CodeLocationJump  argsCheckJump;
    JSC::ExecutablePool *argsCheckPool;
    void resetArgsCheck();
#endif

    
    JSCList          callers;

#ifdef JS_MONOIC
    
    typedef Vector<JSC::ExecutablePool *, 0, SystemAllocPolicy> ExecPoolVector;
    ExecPoolVector execPools;
#endif

    
    Vector<NativeCallStub, 0, SystemAllocPolicy> nativeCallStubs;

    NativeMapEntry *nmap() const;
    js::mjit::InlineFrame *inlineFrames() const;
    js::mjit::CallSite *callSites() const;
#ifdef JS_MONOIC
    ic::GetGlobalNameIC *getGlobalNames() const;
    ic::SetGlobalNameIC *setGlobalNames() const;
    ic::CallICInfo *callICs() const;
    ic::EqualityICInfo *equalityICs() const;
    ic::TraceICInfo *traceICs() const;
#endif
#ifdef JS_POLYIC
    ic::GetElementIC *getElems() const;
    ic::SetElementIC *setElems() const;
    ic::PICInfo     *pics() const;
#endif

    ~JITScript();

    bool isValidCode(void *ptr) {
        char *jitcode = (char *)code.m_code.executableAddress();
        char *jcheck = (char *)ptr;
        return jcheck >= jitcode && jcheck < jitcode + code.m_size;
    }

    void nukeScriptDependentICs();
    void purgeGetterPICs();

    
    size_t scriptDataSize(JSUsableSizeFun usf);

    jsbytecode *nativeToPC(void *returnAddress, CallSite **pinline) const;

  private:
    
    char *commonSectionLimit() const;
    char *monoICSectionsLimit() const;
    char *polyICSectionsLimit() const;
};





JaegerStatus EnterMethodJIT(JSContext *cx, StackFrame *fp, void *code, Value *stackLimit,
                            bool partial);


JaegerStatus JaegerShot(JSContext *cx, bool partial);


JaegerStatus JaegerShotAtSafePoint(JSContext *cx, void *safePoint, bool partial);

enum CompileStatus
{
    Compile_Okay,
    Compile_Abort,        
    Compile_InlineAbort,  
    Compile_Retry,        
    Compile_Error,        
    Compile_Skipped
};

void JS_FASTCALL
ProfileStubCall(VMFrame &f);

CompileStatus JS_NEVER_INLINE
TryCompile(JSContext *cx, JSScript *script, bool construct);

void
ReleaseScriptCode(JSContext *cx, JSScript *script, bool construct);

inline void
ReleaseScriptCode(JSContext *cx, JSScript *script)
{
    if (script->jitCtor)
        mjit::ReleaseScriptCode(cx, script, true);
    if (script->jitNormal)
        mjit::ReleaseScriptCode(cx, script, false);
}


void
ExpandInlineFrames(JSCompartment *compartment);



void
ClearAllFrames(JSCompartment *compartment);


struct InlineFrame
{
    InlineFrame *parent;
    jsbytecode *parentpc;
    JSFunction *fun;

    
    
    uint32 depth;
};

struct CallSite
{
    uint32 codeOffset;
    uint32 inlineIndex;
    uint32 pcOffset;
    RejoinState rejoin;

    void initialize(uint32 codeOffset, uint32 inlineIndex, uint32 pcOffset, RejoinState rejoin) {
        this->codeOffset = codeOffset;
        this->inlineIndex = inlineIndex;
        this->pcOffset = pcOffset;
        this->rejoin = rejoin;
    }

    bool isTrap() const {
        return rejoin == REJOIN_TRAP;
    }
};





void
ResetTraceHint(JSScript *script, jsbytecode *pc, uint16_t index, bool full);

uintN
GetCallTargetCount(JSScript *script, jsbytecode *pc);

void
DumpAllProfiles(JSContext *cx);

inline void * bsearch_nmap(NativeMapEntry *nmap, size_t nPairs, size_t bcOff)
{
    size_t lo = 1, hi = nPairs;
    while (1) {
        
        if (lo > hi)
            return NULL; 
        size_t mid       = (lo + hi) / 2;
        size_t bcOff_mid = nmap[mid-1].bcOff;
        if (bcOff < bcOff_mid) {
            hi = mid-1;
            continue;
        } 
        if (bcOff > bcOff_mid) {
            lo = mid+1;
            continue;
        }
        return nmap[mid-1].ncode;
    }
}

} 

inline JSScript *
VMFrame::script()
{
    if (regs.inlined())
        return jit()->inlineFrames()[regs.inlined()->inlineIndex].fun->script();
    return fp()->script();
}

inline jsbytecode *
VMFrame::pc()
{
    if (regs.inlined())
        return script()->code + regs.inlined()->pcOffset;
    return regs.pc;
}

} 

inline void *
JSScript::maybeNativeCodeForPC(bool constructing, jsbytecode *pc)
{
    js::mjit::JITScript *jit = getJIT(constructing);
    if (!jit)
        return NULL;
    JS_ASSERT(pc >= code && pc < code + length);
    return bsearch_nmap(jit->nmap(), jit->nNmapPairs, (size_t)(pc - code));
}

inline void *
JSScript::nativeCodeForPC(bool constructing, jsbytecode *pc)
{
    js::mjit::JITScript *jit = getJIT(constructing);
    JS_ASSERT(pc >= code && pc < code + length);
    void* native = bsearch_nmap(jit->nmap(), jit->nNmapPairs, (size_t)(pc - code));
    JS_ASSERT(native);
    return native;
}

extern "C" void JaegerTrampolineReturn();
extern "C" void JaegerInterpoline();
extern "C" void JaegerInterpolineScripted();

#if defined(_MSC_VER) || defined(_WIN64)
extern "C" void *JaegerThrowpoline(js::VMFrame *vmFrame);
#else
extern "C" void JaegerThrowpoline();
#endif

#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
extern "C" void JaegerInterpolinePatched();
#endif

#endif 

