






#if !defined jsjaeger_h__ && defined JS_METHODJIT
#define jsjaeger_h__

#include "mozilla/PodOperations.h"

#ifdef JSGC_INCREMENTAL
#define JSGC_INCREMENTAL_MJ
#endif

#include "jscntxt.h"
#include "jscompartment.h"

#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/assembler/CodeLocation.h"

#if !defined JS_CPU_X64 && \
    !defined JS_CPU_X86 && \
    !defined JS_CPU_SPARC && \
    !defined JS_CPU_ARM && \
    !defined JS_CPU_MIPS
# error "Oh no, you should define a platform so this compiles."
#endif

#if !defined(JS_NUNBOX32) && !defined(JS_PUNBOX64)
# error "No boxing format selected."
#endif

namespace js {

namespace mjit {
    struct JITChunk;
    struct JITScript;
}

namespace analyze {
    struct ScriptLiveness;
}

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

#elif defined(JS_CPU_MIPS)
    
    void         *unused0;
    void         *unused1;
    void         *unused2;
    void         *unused3;
#endif

    union Arguments {
        struct {
            void *ptr;
            void *ptr2;
        } x;
        struct {
            uint32_t dynamicArgc;
        } call;
    } u;

    static size_t offsetOfDynamicArgc() {
        return offsetof(VMFrame, u.call.dynamicArgc);
    }

    VMFrame      *previous;
    void         *scratch;
    FrameRegs    regs;

    static size_t offsetOfRegsSp() {
        return offsetof(VMFrame, regs.sp);
    }

    static size_t offsetOfRegsPc() {
        return offsetof(VMFrame, regs.pc);
    }

    JSContext    *cx;
    Value        *stackLimit;
    StackFrame   *entryfp;
    FrameRegs    *oldregs;
    FrameRejoinState stubRejoin;  

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

    
    static const uint32_t STACK_BASE_DIFFERENCE = 0x38;

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
#elif defined(JS_CPU_MIPS)
    void *savedS0;
    void *savedS1;
    void *savedS2;
    void *savedS3;
    void *savedS4;
    void *savedS5;
    void *savedS6;
    void *savedS7;
    void *savedGP;
    void *savedRA;
    void *unused4;  

    inline void** returnAddressLocation() {
        return reinterpret_cast<void**>(this) - 1;
    }
#else
# error "The VMFrame layout isn't defined for your processor architecture!"
#endif

    JSRuntime *runtime() { return cx->runtime; }

    





    StackFrame *fp() { return regs.fp(); }
    mjit::JITScript *jit() { return fp()->jit(); }

    inline mjit::JITChunk *chunk();
    inline unsigned chunkIndex();

    
    inline JSScript *script();
    inline jsbytecode *pc();

#if defined(JS_CPU_SPARC)
    static const size_t offsetOfFp = 30 * sizeof(void *) + FrameRegs::offsetOfFp;
    static const size_t offsetOfInlined = 30 * sizeof(void *) + FrameRegs::offsetOfInlined;
#elif defined(JS_CPU_MIPS)
    static const size_t offsetOfFp = 8 * sizeof(void *) + FrameRegs::offsetOfFp;
    static const size_t offsetOfInlined = 8 * sizeof(void *) + FrameRegs::offsetOfInlined;
#else
    static const size_t offsetOfFp = 4 * sizeof(void *) + FrameRegs::offsetOfFp;
    static const size_t offsetOfInlined = 4 * sizeof(void *) + FrameRegs::offsetOfInlined;
#endif

    static void staticAssert() {
        JS_STATIC_ASSERT(offsetOfFp == offsetof(VMFrame, regs) + FrameRegs::offsetOfFp);
        JS_STATIC_ASSERT(offsetOfInlined == offsetof(VMFrame, regs) + FrameRegs::offsetOfInlined);
    }
};

#if defined(JS_CPU_ARM) || defined(JS_CPU_SPARC) || defined(JS_CPU_MIPS)

extern "C" void JaegerStubVeneer(void);
# if defined(JS_CPU_ARM)
extern "C" void IonVeneer(void);
# endif
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

    



    REJOIN_THIS_PROTOTYPE,

    
    REJOIN_THIS_CREATED,

    



    REJOIN_CHECK_ARGUMENTS,

    



    REJOIN_EVAL_PROLOGUE,
    REJOIN_FUNCTION_PROLOGUE,

    



    REJOIN_CALL_PROLOGUE,
    REJOIN_CALL_PROLOGUE_LOWERED_CALL,
    REJOIN_CALL_PROLOGUE_LOWERED_APPLY,

    
    REJOIN_CALL_SPLAT,

    
    REJOIN_GETTER,

    



    REJOIN_BRANCH
};


static inline FrameRejoinState
ScriptedRejoin(uint32_t pcOffset)
{
    return REJOIN_SCRIPTED | (pcOffset << 1);
}


static inline FrameRejoinState
StubRejoin(RejoinState rejoin)
{
    return rejoin << 1;
}


struct RecompilationMonitor
{
    JSContext *cx;

    




    unsigned recompilations;
    unsigned frameExpansions;

    
    uint64_t gcNumber;

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

    




    Jaeger_UnfinishedAtTrap = 3,

    



    Jaeger_ThrowBeforeEnter = 4
};

static inline bool
JaegerStatusToSuccess(JaegerStatus status)
{
    JS_ASSERT(status != Jaeger_Unfinished);
    JS_ASSERT(status != Jaeger_UnfinishedAtTrap);
    return status == Jaeger_Returned;
}


class JaegerRuntime
{
    Trampolines              trampolines;    
    VMFrame                  *activeFrame_;  
    JaegerStatus             lastUnfinished_;
                                             

    void finish();

  public:
    bool init(JSContext *cx);

    JaegerRuntime();
    ~JaegerRuntime() { finish(); }

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
    struct CallICInfo;
# endif
}
}

typedef void (JS_FASTCALL *VoidStub)(VMFrame &);
typedef void (JS_FASTCALL *VoidVpStub)(VMFrame &, Value *);
typedef void (JS_FASTCALL *VoidStubUInt32)(VMFrame &, uint32_t);
typedef void (JS_FASTCALL *VoidStubInt32)(VMFrame &, int32_t);
typedef JSBool (JS_FASTCALL *BoolStub)(VMFrame &);
typedef void * (JS_FASTCALL *VoidPtrStub)(VMFrame &);
typedef void * (JS_FASTCALL *VoidPtrStubPC)(VMFrame &, jsbytecode *);
typedef void * (JS_FASTCALL *VoidPtrStubUInt32)(VMFrame &, uint32_t);
typedef JSObject * (JS_FASTCALL *JSObjStub)(VMFrame &);
typedef JSObject * (JS_FASTCALL *JSObjStubUInt32)(VMFrame &, uint32_t);
typedef JSObject * (JS_FASTCALL *JSObjStubFun)(VMFrame &, JSFunction *);
typedef void (JS_FASTCALL *VoidStubFun)(VMFrame &, JSFunction *);
typedef JSObject * (JS_FASTCALL *JSObjStubJSObj)(VMFrame &, JSObject *);
typedef void (JS_FASTCALL *VoidStubName)(VMFrame &, PropertyName *);
typedef JSString * (JS_FASTCALL *JSStrStub)(VMFrame &);
typedef JSString * (JS_FASTCALL *JSStrStubUInt32)(VMFrame &, uint32_t);
typedef void (JS_FASTCALL *VoidStubJSObj)(VMFrame &, JSObject *);
typedef void (JS_FASTCALL *VoidStubPC)(VMFrame &, jsbytecode *);
typedef JSBool (JS_FASTCALL *BoolStubUInt32)(VMFrame &f, uint32_t);
#ifdef JS_MONOIC
typedef void (JS_FASTCALL *VoidStubCallIC)(VMFrame &, js::mjit::ic::CallICInfo *);
typedef void * (JS_FASTCALL *VoidPtrStubCallIC)(VMFrame &, js::mjit::ic::CallICInfo *);
typedef void (JS_FASTCALL *VoidStubGetGlobal)(VMFrame &, js::mjit::ic::GetGlobalNameIC *);
typedef void (JS_FASTCALL *VoidStubSetGlobal)(VMFrame &, js::mjit::ic::SetGlobalNameIC *);
typedef JSBool (JS_FASTCALL *BoolStubEqualityIC)(VMFrame &, js::mjit::ic::EqualityICInfo *);
#endif
#ifdef JS_POLYIC
typedef void (JS_FASTCALL *VoidStubPIC)(VMFrame &, js::mjit::ic::PICInfo *);
typedef void (JS_FASTCALL *VoidStubGetElemIC)(VMFrame &, js::mjit::ic::GetElementIC *);
typedef void (JS_FASTCALL *VoidStubSetElemIC)(VMFrame &f, js::mjit::ic::SetElementIC *);
#endif

namespace mjit {

struct InlineFrame;
struct CallSite;
struct CompileTrigger;

struct NativeMapEntry {
    size_t          bcOff;  
    void            *ncode; 
};


struct PCLengthEntry {
    double          inlineLength; 
    double          picsLength;   
    double          stubLength;   
    double          codeLengthAugment; 



};






struct NativeCallStub {
    
    jsbytecode *pc;

    
    JSC::ExecutablePool *pool;

    




#ifdef JS_CPU_X64
    JSC::CodeLocationDataLabelPtr jump;
#else
    JSC::CodeLocationJump jump;
#endif
};

struct JITChunk
{
    typedef JSC::MacroAssemblerCodeRef CodeRef;
    CodeRef         code;       

    PCLengthEntry   *pcLengths;         

    







    uint32_t        nNmapPairs : 31;    

    uint32_t        nInlineFrames;
    uint32_t        nCallSites;
    uint32_t        nCompileTriggers;
    uint32_t        nRootedTemplates;
    uint32_t        nRootedRegExps;
    uint32_t        nMonitoredBytecodes;
    uint32_t        nTypeBarrierBytecodes;
#ifdef JS_MONOIC
    uint32_t        nGetGlobalNames;
    uint32_t        nSetGlobalNames;
    uint32_t        nCallICs;
    uint32_t        nEqualityICs;
#endif
#ifdef JS_POLYIC
    uint32_t        nGetElems;
    uint32_t        nSetElems;
    uint32_t        nPICs;
#endif

#ifdef JS_MONOIC
    
    typedef Vector<JSC::ExecutablePool *, 0, SystemAllocPolicy> ExecPoolVector;
    ExecPoolVector execPools;
#endif

    types::RecompileInfo recompileInfo;

    
    Vector<NativeCallStub, 0, SystemAllocPolicy> nativeCallStubs;

    NativeMapEntry *nmap() const;
    js::mjit::InlineFrame *inlineFrames() const;
    js::mjit::CallSite *callSites() const;
    js::mjit::CompileTrigger *compileTriggers() const;
    JSObject **rootedTemplates() const;
    RegExpShared **rootedRegExps() const;

    




    uint32_t *monitoredBytecodes() const;
    uint32_t *typeBarrierBytecodes() const;

#ifdef JS_MONOIC
    ic::GetGlobalNameIC *getGlobalNames() const;
    ic::SetGlobalNameIC *setGlobalNames() const;
    ic::CallICInfo *callICs() const;
    ic::EqualityICInfo *equalityICs() const;
#endif
#ifdef JS_POLYIC
    ic::GetElementIC *getElems() const;
    ic::SetElementIC *setElems() const;
    ic::PICInfo     *pics() const;
#endif

    bool isValidCode(void *ptr) {
        char *jitcode = (char *)code.m_code.executableAddress();
        char *jcheck = (char *)ptr;
        return jcheck >= jitcode && jcheck < jitcode + code.m_size;
    }

    size_t computedSizeOfIncludingThis();
    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf);

    ~JITChunk();

    void trace(JSTracer *trc);
    void purgeCaches();

  private:
    
    char *commonSectionLimit() const;
    char *monoICSectionsLimit() const;
    char *polyICSectionsLimit() const;
};

void
SetChunkLimit(uint32_t limit);


struct ChunkDescriptor
{
    
    uint32_t begin;
    uint32_t end;

    
    uint32_t counter;

    
    JITChunk *chunk;

    ChunkDescriptor() { mozilla::PodZero(this); }
};


struct CrossChunkEdge
{
    
    uint32_t source;
    uint32_t target;

    
    void *sourceJump1;
    void *sourceJump2;

#ifdef JS_CPU_X64
    



    void *sourceTrampoline;
#endif

    
    typedef Vector<void**,4,SystemAllocPolicy> JumpTableEntryVector;
    JumpTableEntryVector *jumpTableEntries;

    
    void *targetLabel;

    




    void *shimLabel;

    CrossChunkEdge() { mozilla::PodZero(this); }
};

struct JITScript
{
    JSScript        *script;

    void            *invokeEntry;       
    void            *fastEntry;         
    void            *arityCheckEntry;   
    void            *argsCheckEntry;    

    
    JSCList         callers;

    uint32_t        nchunks;
    uint32_t        nedges;

    



    JSC::ExecutablePool *shimPool;

    



    analyze::ScriptLiveness *liveness;

    



    uint32_t        ionCalls;

    




    bool mustDestroyEntryChunk;

#ifdef JS_MONOIC
    
    JSC::CodeLocationLabel argsCheckStub;
    JSC::CodeLocationLabel argsCheckFallthrough;
    JSC::CodeLocationJump  argsCheckJump;
    JSC::ExecutablePool *argsCheckPool;
    void resetArgsCheck();
#endif

    ChunkDescriptor &chunkDescriptor(unsigned i) {
        JS_ASSERT(i < nchunks);
        ChunkDescriptor *descs = (ChunkDescriptor *) ((char *) this + sizeof(JITScript));
        return descs[i];
    }

    unsigned chunkIndex(jsbytecode *pc) {
        unsigned offset = pc - script->code;
        JS_ASSERT(offset < script->length);
        for (unsigned i = 0; i < nchunks; i++) {
            const ChunkDescriptor &desc = chunkDescriptor(i);
            JS_ASSERT(desc.begin <= offset);
            if (offset < desc.end)
                return i;
        }
        JS_NOT_REACHED("Bad chunk layout");
        return 0;
    }

    JITChunk *chunk(jsbytecode *pc) {
        return chunkDescriptor(chunkIndex(pc)).chunk;
    }

    JITChunk *findCodeChunk(void *addr);

    CrossChunkEdge *edges() {
        return (CrossChunkEdge *) (&chunkDescriptor(0) + nchunks);
    }

    
    void patchEdge(const CrossChunkEdge &edge, void *label);

    jsbytecode *nativeToPC(void *returnAddress, CallSite **pinline);

    size_t sizeOfIncludingThis(JSMallocSizeOfFun mallocSizeOf);

    void destroy(FreeOp *fop);
    void destroyChunk(FreeOp *fop, unsigned chunkIndex, bool resetUses = true);

    void trace(JSTracer *trc);
    void purgeCaches();

    void disableScriptEntry();
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

enum CompileRequest
{
    CompileRequest_Interpreter,
    CompileRequest_JIT
};

CompileStatus
CanMethodJIT(JSContext *cx, JSScript *script, jsbytecode *pc,
             bool construct, CompileRequest request, StackFrame *sp);

inline void
ReleaseScriptCode(FreeOp *fop, JSScript *script)
{
    if (!script->hasMJITInfo())
        return;

    for (int constructing = 0; constructing <= 1; constructing++) {
        for (int barriers = 0; barriers <= 1; barriers++) {
            JSScript::JITScriptHandle *jith = script->jitHandle((bool) constructing, (bool) barriers);
            if (jith && jith->isValid())
                JSScript::ReleaseCode(fop, jith);
        }
    }

    script->destroyMJITInfo(fop);
}




void
DisableScriptCodeForIon(JSScript *script, jsbytecode *osrPC);


void
ExpandInlineFrames(JS::Zone *zone);



void
ClearAllFrames(JS::Zone *zone);


struct InlineFrame
{
    InlineFrame *parent;
    jsbytecode *parentpc;
    HeapPtrFunction fun;

    
    
    uint32_t depth;
};

struct CallSite
{
    uint32_t codeOffset;
    uint32_t inlineIndex;
    uint32_t pcOffset;
    RejoinState rejoin;

    void initialize(uint32_t codeOffset, uint32_t inlineIndex, uint32_t pcOffset,
                    RejoinState rejoin) {
        this->codeOffset = codeOffset;
        this->inlineIndex = inlineIndex;
        this->pcOffset = pcOffset;
        this->rejoin = rejoin;
    }

    bool isTrap() const {
        return rejoin == REJOIN_TRAP;
    }
};



struct CompileTrigger
{
    uint32_t pcOffset;

    
    
    
    JSC::CodeLocationJump inlineJump;
    JSC::CodeLocationLabel stubLabel;

    void initialize(uint32_t pcOffset, JSC::CodeLocationJump inlineJump, JSC::CodeLocationLabel stubLabel) {
        this->pcOffset = pcOffset;
        this->inlineJump = inlineJump;
        this->stubLabel = stubLabel;
    }
};

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

static inline bool
IsLowerableFunCallOrApply(jsbytecode *pc)
{
#ifdef JS_MONOIC
    return (*pc == JSOP_FUNCALL && GET_ARGC(pc) >= 1) ||
           (*pc == JSOP_FUNAPPLY && GET_ARGC(pc) == 2);
#else
    return false;
#endif
}

RawShape
GetPICSingleShape(JSContext *cx, JSScript *script, jsbytecode *pc, bool constructing);

static inline void
PurgeCaches(JSScript *script)
{
    for (int constructing = 0; constructing <= 1; constructing++) {
        for (int barriers = 0; barriers <= 1; barriers++) {
            mjit::JITScript *jit = script->getJIT((bool) constructing, (bool) barriers);
            if (jit)
                jit->purgeCaches();
        }
    }
}

} 

inline mjit::JITChunk *
VMFrame::chunk()
{
    return jit()->chunk(regs.pc);
}

inline unsigned
VMFrame::chunkIndex()
{
    return jit()->chunkIndex(regs.pc);
}

inline JSScript *
VMFrame::script()
{
    if (regs.inlined())
        return chunk()->inlineFrames()[regs.inlined()->inlineIndex].fun->nonLazyScript();
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
JSScript::nativeCodeForPC(bool constructing, jsbytecode *pc)
{
    js::mjit::JITScript *jit = getJIT(constructing, zone()->compileBarriers());
    if (!jit)
        return NULL;
    js::mjit::JITChunk *chunk = jit->chunk(pc);
    if (!chunk)
        return NULL;
    return bsearch_nmap(chunk->nmap(), chunk->nNmapPairs, (size_t)(pc - code));
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

