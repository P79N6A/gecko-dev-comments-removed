





































#if !defined jsjaeger_h__ && defined JS_METHODJIT
#define jsjaeger_h__

#include "jscntxt.h"

#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/assembler/CodeLocation.h"

#if !defined JS_CPU_X64 && \
    !defined JS_CPU_X86 && \
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
    union Arguments {
        struct {
            uint32 lazyArgsObj;
            uint32 dynamicArgc;
        } call;
    } u;

    VMFrame      *previous;
    void         *scratch;
    JSFrameRegs  regs;
    JSContext    *cx;
    Value        *stackLimit;
    JSStackFrame *entryfp;







#define NATIVE_CALL_SCRATCH_VALUE (void *) 0x1

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
    mjit::JITScript *jit() { return fp()->jit(); }

    
    inline JSScript *script();
    inline jsbytecode *pc();
};

#ifdef JS_CPU_ARM

extern "C" void JaegerStubVeneer(void);
#endif

namespace mjit {





struct Trampolines {
    typedef void (*TrampolinePtr)();

    TrampolinePtr       forceReturn;
    JSC::ExecutablePool *forceReturnPool;

#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
    TrampolinePtr       forceReturnFast;
    JSC::ExecutablePool *forceReturnFastPool;
#endif
};







class JaegerCompartment {
    JSC::ExecutableAllocator *execAlloc_;    
    Trampolines              trampolines;    
    VMFrame                  *activeFrame_;  

    void Finish();

  public:
    bool Initialize();

    ~JaegerCompartment() { Finish(); }

    JSC::ExecutableAllocator *execAlloc() {
        return execAlloc_;
    }

    VMFrame *activeFrame() {
        return activeFrame_;
    }

    void pushActiveFrame(VMFrame *f) {
        f->previous = activeFrame_;
        f->scratch = NULL;
        activeFrame_ = f;
    }

    void popActiveFrame() {
        JS_ASSERT(activeFrame_);
        activeFrame_ = activeFrame_->previous;
    }

    Trampolines::TrampolinePtr forceReturnTrampoline() const {
        return trampolines.forceReturn;
    }

#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
    Trampolines::TrampolinePtr forceReturnFastTrampoline() const {
        return trampolines.forceReturnFast;
    }
#endif
};








class CompilerAllocPolicy : public ContextAllocPolicy
{
    bool *oomFlag;

    void *checkAlloc(void *p) {
        if (!p)
            *oomFlag = true;
        return p;
    }

  public:
    CompilerAllocPolicy(JSContext *cx, bool *oomFlag)
    : ContextAllocPolicy(cx), oomFlag(oomFlag) {}
    CompilerAllocPolicy(JSContext *cx, Compiler &compiler);

    void *malloc(size_t bytes) { return checkAlloc(ContextAllocPolicy::malloc(bytes)); }
    void *realloc(void *p, size_t bytes) {
        return checkAlloc(ContextAllocPolicy::realloc(p, bytes));
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

struct JITScript {
    typedef JSC::MacroAssemblerCodeRef CodeRef;
    CodeRef         code;       

    JSScript        *script;

    void            *invokeEntry;       
    void            *fastEntry;         
    void            *arityCheckEntry;   

    







    uint32          nNmapPairs:30;      

    bool            singleStepMode:1;   
    bool            rejoinPoints:1;     
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

    




    uint32          recompilations;

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
    void sweepCallICs(JSContext *cx, bool purgeAll);
    void purgeMICs();
    void purgePICs();

    size_t scriptDataSize();

    size_t mainCodeSize() { return code.m_size; } 

    jsbytecode *nativeToPC(void *returnAddress, CallSite **pinline) const;

    void trace(JSTracer *trc);

  private:
    
    char *nmapSectionLimit() const;
    char *monoICSectionsLimit() const;
    char *polyICSectionsLimit() const;
};





JSBool EnterMethodJIT(JSContext *cx, JSStackFrame *fp, void *code, Value *stackLimit);


JSBool JaegerShot(JSContext *cx);


JSBool JaegerShotAtSafePoint(JSContext *cx, void *safePoint);

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
TryCompile(JSContext *cx, JSStackFrame *fp);

void
ReleaseScriptCode(JSContext *cx, JSScript *script);


void
ExpandInlineFrames(JSContext *cx, bool all);


struct UnsyncedEntry
{
    
    uint32 offset;

    bool copy : 1;
    bool constant : 1;
    bool knownType : 1;
    union {
        uint32 copiedOffset;
        Value value;
        JSValueType type;
    } u;
};


struct InlineFrame
{
    InlineFrame *parent;
    jsbytecode *parentpc;
    JSFunction *fun;

    
    
    uint32 depth;

    
    
    
    
    
    
    
    uint32 nUnsyncedEntries;
    UnsyncedEntry *unsyncedEntries;
};

struct CallSite
{
    uint32 codeOffset;
    uint32 inlineIndex;
    uint32 pcOffset;
    size_t id;

    
    
    

    
    
    
    static const size_t MAGIC_TRAP_ID = 0;

    
    static const size_t NCODE_RETURN_ID = 1;

    void initialize(uint32 codeOffset, uint32 inlineIndex, uint32 pcOffset, size_t id) {
        this->codeOffset = codeOffset;
        this->inlineIndex = inlineIndex;
        this->pcOffset = pcOffset;
        this->id = id;
    }

    bool isTrap() const {
        return id == MAGIC_TRAP_ID;
    }
};





void
ResetTraceHint(JSScript *script, jsbytecode *pc, uint16_t index, bool full);

uintN
GetCallTargetCount(JSScript *script, jsbytecode *pc);

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
    if (regs.inlined)
        return jit()->inlineFrames()[regs.inlined->inlineIndex].fun->script();
    return fp()->script();
}

inline jsbytecode *
VMFrame::pc()
{
    if (regs.inlined)
        return script()->code + regs.inlined->pcOffset;
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

#if defined(_MSC_VER) || defined(_WIN64)
extern "C" void *JaegerThrowpoline(js::VMFrame *vmFrame);
#else
extern "C" void JaegerThrowpoline();
#endif
extern "C" void InjectJaegerReturn();

#endif 

