





#ifndef vm_Stack_h
#define vm_Stack_h

#include "mozilla/Atomics.h"
#include "mozilla/MemoryReporting.h"

#include "jsfun.h"
#include "jsscript.h"

#include "asmjs/AsmJSFrameIterator.h"
#include "jit/JitFrameIterator.h"
#ifdef CHECK_OSIPOINT_REGISTERS
#include "jit/Registers.h" 
#endif

struct JSCompartment;

namespace js {

class ArgumentsObject;
class AsmJSModule;
class InterpreterRegs;
class CallObject;
class ScopeObject;
class ScriptFrameIter;
class SPSProfiler;
class InterpreterFrame;
class StaticBlockObject;

class ScopeCoordinate;

class SavedFrame;































enum MaybeCheckAliasing { CHECK_ALIASING = true, DONT_CHECK_ALIASING = false };
enum MaybeCheckLexical { CheckLexical = true, DontCheckLexical = false };



namespace jit {
    class BaselineFrame;
    class RematerializedFrame;
}




















class AbstractFramePtr
{
    friend class FrameIter;

    uintptr_t ptr_;

    enum {
        Tag_ScriptFrameIterData = 0x0,
        Tag_InterpreterFrame = 0x1,
        Tag_BaselineFrame = 0x2,
        Tag_RematerializedFrame = 0x3,
        TagMask = 0x3
    };

  public:
    AbstractFramePtr()
      : ptr_(0)
    {}

    MOZ_IMPLICIT AbstractFramePtr(InterpreterFrame* fp)
      : ptr_(fp ? uintptr_t(fp) | Tag_InterpreterFrame : 0)
    {
        MOZ_ASSERT_IF(fp, asInterpreterFrame() == fp);
    }

    MOZ_IMPLICIT AbstractFramePtr(jit::BaselineFrame* fp)
      : ptr_(fp ? uintptr_t(fp) | Tag_BaselineFrame : 0)
    {
        MOZ_ASSERT_IF(fp, asBaselineFrame() == fp);
    }

    MOZ_IMPLICIT AbstractFramePtr(jit::RematerializedFrame* fp)
      : ptr_(fp ? uintptr_t(fp) | Tag_RematerializedFrame : 0)
    {
        MOZ_ASSERT_IF(fp, asRematerializedFrame() == fp);
    }

    static AbstractFramePtr FromRaw(void* raw) {
        AbstractFramePtr frame;
        frame.ptr_ = uintptr_t(raw);
        return frame;
    }

    bool isScriptFrameIterData() const {
        return !!ptr_ && (ptr_ & TagMask) == Tag_ScriptFrameIterData;
    }
    bool isInterpreterFrame() const {
        return (ptr_ & TagMask) == Tag_InterpreterFrame;
    }
    InterpreterFrame* asInterpreterFrame() const {
        MOZ_ASSERT(isInterpreterFrame());
        InterpreterFrame* res = (InterpreterFrame*)(ptr_ & ~TagMask);
        MOZ_ASSERT(res);
        return res;
    }
    bool isBaselineFrame() const {
        return (ptr_ & TagMask) == Tag_BaselineFrame;
    }
    jit::BaselineFrame* asBaselineFrame() const {
        MOZ_ASSERT(isBaselineFrame());
        jit::BaselineFrame* res = (jit::BaselineFrame*)(ptr_ & ~TagMask);
        MOZ_ASSERT(res);
        return res;
    }
    bool isRematerializedFrame() const {
        return (ptr_ & TagMask) == Tag_RematerializedFrame;
    }
    jit::RematerializedFrame* asRematerializedFrame() const {
        MOZ_ASSERT(isRematerializedFrame());
        jit::RematerializedFrame* res = (jit::RematerializedFrame*)(ptr_ & ~TagMask);
        MOZ_ASSERT(res);
        return res;
    }

    void* raw() const { return reinterpret_cast<void*>(ptr_); }

    bool operator ==(const AbstractFramePtr& other) const { return ptr_ == other.ptr_; }
    bool operator !=(const AbstractFramePtr& other) const { return ptr_ != other.ptr_; }

    explicit operator bool() const { return !!ptr_; }

    inline JSObject* scopeChain() const;
    inline CallObject& callObj() const;
    inline bool initFunctionScopeObjects(JSContext* cx);
    inline void pushOnScopeChain(ScopeObject& scope);

    inline JSCompartment* compartment() const;

    inline bool hasCallObj() const;
    inline bool isFunctionFrame() const;
    inline bool isGlobalFrame() const;
    inline bool isEvalFrame() const;
    inline bool isDebuggerEvalFrame() const;

    inline JSScript* script() const;
    inline JSFunction* fun() const;
    inline JSFunction* maybeFun() const;
    inline JSFunction* callee() const;
    inline Value calleev() const;
    inline Value& thisValue() const;

    inline bool isNonEvalFunctionFrame() const;
    inline bool isNonStrictDirectEvalFrame() const;
    inline bool isStrictEvalFrame() const;

    inline unsigned numActualArgs() const;
    inline unsigned numFormalArgs() const;

    inline Value* argv() const;

    inline bool hasArgs() const;
    inline bool hasArgsObj() const;
    inline ArgumentsObject& argsObj() const;
    inline void initArgsObj(ArgumentsObject& argsobj) const;
    inline bool createSingleton() const;

    inline bool copyRawFrameSlots(AutoValueVector* vec) const;

    inline Value& unaliasedLocal(uint32_t i);
    inline Value& unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value& unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    template <class Op> inline void unaliasedForEachActual(JSContext* cx, Op op);

    inline bool prevUpToDate() const;
    inline void setPrevUpToDate() const;
    inline void unsetPrevUpToDate() const;

    inline bool isDebuggee() const;
    inline void setIsDebuggee();
    inline void unsetIsDebuggee();

    JSObject* evalPrevScopeChain(JSContext* cx) const;

    inline HandleValue returnValue() const;
    inline void setReturnValue(const Value& rval) const;

    bool hasPushedSPSFrame() const;

    inline bool freshenBlock(JSContext* cx) const;

    inline void popBlock(JSContext* cx) const;
    inline void popWith(JSContext* cx) const;

    friend void GDBTestInitAbstractFramePtr(AbstractFramePtr&, void*);
    friend void GDBTestInitAbstractFramePtr(AbstractFramePtr&, InterpreterFrame*);
    friend void GDBTestInitAbstractFramePtr(AbstractFramePtr&, jit::BaselineFrame*);
    friend void GDBTestInitAbstractFramePtr(AbstractFramePtr&, jit::RematerializedFrame*);
};

class NullFramePtr : public AbstractFramePtr
{
  public:
    NullFramePtr()
      : AbstractFramePtr()
    { }
};




enum InitialFrameFlags {
    INITIAL_NONE           =          0,
    INITIAL_CONSTRUCT      =       0x10, 
};

enum ExecuteType {
    EXECUTE_GLOBAL         =        0x1, 
    EXECUTE_DIRECT_EVAL    =        0x4, 
    EXECUTE_INDIRECT_EVAL  =        0x5, 
    EXECUTE_DEBUG          =        0xc, 
    EXECUTE_DEBUG_GLOBAL   =        0xd  
};



class InterpreterFrame
{
  public:
    enum Flags {
        
        GLOBAL             =        0x1,  
        FUNCTION           =        0x2,  

        
        EVAL               =        0x4,  


        











        DEBUGGER_EVAL      =        0x8,

        CONSTRUCTING       =       0x10,  

        

        
        HAS_CALL_OBJ       =      0x100,  
        HAS_ARGS_OBJ       =      0x200,  

        
        HAS_RVAL           =      0x800,  
        HAS_SCOPECHAIN     =     0x1000,  

        
        PREV_UP_TO_DATE    =     0x4000,  

        



        DEBUGGEE           =     0x8000,  

        
        HAS_PUSHED_SPS_FRAME =   0x10000, 

        



        RUNNING_IN_JIT     =    0x20000,

        
        CREATE_SINGLETON   =    0x40000   
    };

  private:
    mutable uint32_t    flags_;         
    union {                             
        JSScript*       script;        
        JSFunction*     fun;           
    } exec;
    union {                             
        unsigned        nactual;        
        JSScript*       evalScript;    
    } u;
    mutable JSObject*   scopeChain_;   
    Value               rval_;          
    ArgumentsObject*    argsObj_;      

    




    InterpreterFrame*   prev_;
    jsbytecode*         prevpc_;
    Value*              prevsp_;

    void*               unused;

    



    AbstractFramePtr    evalInFramePrev_;

    Value*              argv_;         
    LifoAlloc::Mark     mark_;          

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(InterpreterFrame, rval_) % sizeof(Value) == 0);
        JS_STATIC_ASSERT(sizeof(InterpreterFrame) % sizeof(Value) == 0);
    }

    void writeBarrierPost();

    





    Value* slots() const { return (Value*)(this + 1); }
    Value* base() const { return slots() + script()->nfixed(); }

    friend class FrameIter;
    friend class InterpreterRegs;
    friend class InterpreterStack;
    friend class jit::BaselineFrame;

    




    
    void initCallFrame(JSContext* cx, InterpreterFrame* prev, jsbytecode* prevpc, Value* prevsp,
                       JSFunction& callee, JSScript* script, Value* argv, uint32_t nactual,
                       InterpreterFrame::Flags flags);

    
    void initExecuteFrame(JSContext* cx, HandleScript script, AbstractFramePtr prev,
                          const Value& thisv, HandleObject scopeChain, ExecuteType type);

  public:
    


















    bool prologue(JSContext* cx);
    void epilogue(JSContext* cx);

    bool initFunctionScopeObjects(JSContext* cx);

    




    void initLocals();

    









    bool isFunctionFrame() const {
        return !!(flags_ & FUNCTION);
    }

    bool isGlobalFrame() const {
        return !!(flags_ & GLOBAL);
    }

    











    bool isEvalFrame() const {
        return flags_ & EVAL;
    }

    bool isEvalInFunction() const {
        return (flags_ & (EVAL | FUNCTION)) == (EVAL | FUNCTION);
    }

    bool isNonEvalFunctionFrame() const {
        return (flags_ & (FUNCTION | EVAL)) == FUNCTION;
    }

    inline bool isStrictEvalFrame() const {
        return isEvalFrame() && script()->strict();
    }

    bool isNonStrictEvalFrame() const {
        return isEvalFrame() && !script()->strict();
    }

    bool isDirectEvalFrame() const {
        return isEvalFrame() && script()->staticLevel() > 0;
    }

    bool isNonStrictDirectEvalFrame() const {
        return isNonStrictEvalFrame() && isDirectEvalFrame();
    }

    












    InterpreterFrame* prev() const {
        return prev_;
    }

    AbstractFramePtr evalInFramePrev() const {
        MOZ_ASSERT(isEvalFrame());
        return evalInFramePrev_;
    }

    

















    inline Value& unaliasedLocal(uint32_t i);

    bool hasArgs() const { return isNonEvalFunctionFrame(); }
    inline Value& unaliasedFormal(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    inline Value& unaliasedActual(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    template <class Op> inline void unaliasedForEachActual(Op op);

    bool copyRawFrameSlots(AutoValueVector* v);

    unsigned numFormalArgs() const { MOZ_ASSERT(hasArgs()); return fun()->nargs(); }
    unsigned numActualArgs() const { MOZ_ASSERT(hasArgs()); return u.nactual; }

    
    Value* argv() const { return argv_; }

    










    ArgumentsObject& argsObj() const;
    void initArgsObj(ArgumentsObject& argsobj);

    JSObject* createRestParameter(JSContext* cx);

    


















    inline HandleObject scopeChain() const;

    inline ScopeObject& aliasedVarScope(ScopeCoordinate sc) const;
    inline GlobalObject& global() const;
    inline CallObject& callObj() const;
    inline JSObject& varObj();

    inline void pushOnScopeChain(ScopeObject& scope);
    inline void popOffScopeChain();
    inline void replaceInnermostScope(ScopeObject& scope);

    






    bool pushBlock(JSContext* cx, StaticBlockObject& block);
    void popBlock(JSContext* cx);
    bool freshenBlock(JSContext* cx);

    






    void popWith(JSContext* cx);

    














    JSScript* script() const {
        return isFunctionFrame()
               ? isEvalFrame()
                 ? u.evalScript
                 : fun()->nonLazyScript()
               : exec.script;
    }

    
    jsbytecode* prevpc() {
        MOZ_ASSERT(prev_);
        return prevpc_;
    }

    
    Value* prevsp() {
        MOZ_ASSERT(prev_);
        return prevsp_;
    }

    








    JSFunction* fun() const {
        MOZ_ASSERT(isFunctionFrame());
        return exec.fun;
    }

    JSFunction* maybeFun() const {
        return isFunctionFrame() ? fun() : nullptr;
    }

    












    Value& functionThis() const {
        MOZ_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((Value*)this)[-1];
        return argv()[-1];
    }

    JSObject& constructorThis() const {
        MOZ_ASSERT(hasArgs());
        return argv()[-1].toObject();
    }

    Value& thisValue() const {
        if (flags_ & (EVAL | GLOBAL))
            return ((Value*)this)[-1];
        return argv()[-1];
    }

    








    JSFunction& callee() const {
        MOZ_ASSERT(isFunctionFrame());
        return calleev().toObject().as<JSFunction>();
    }

    const Value& calleev() const {
        MOZ_ASSERT(isFunctionFrame());
        return mutableCalleev();
    }

    const Value& maybeCalleev() const {
        Value& calleev = flags_ & (EVAL | GLOBAL)
                         ? ((Value*)this)[-2]
                         : argv()[-2];
        MOZ_ASSERT(calleev.isObjectOrNull());
        return calleev;
    }

    Value& mutableCalleev() const {
        MOZ_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((Value*)this)[-2];
        return argv()[-2];
    }

    CallReceiver callReceiver() const {
        return CallReceiverFromArgv(argv());
    }

    






    inline JSCompartment* compartment() const;

    

    bool hasPushedSPSFrame() {
        return !!(flags_ & HAS_PUSHED_SPS_FRAME);
    }

    void setPushedSPSFrame() {
        flags_ |= HAS_PUSHED_SPS_FRAME;
    }

    void unsetPushedSPSFrame() {
        flags_ &= ~HAS_PUSHED_SPS_FRAME;
    }

    

    bool hasReturnValue() const {
        return flags_ & HAS_RVAL;
    }

    MutableHandleValue returnValue() {
        if (!hasReturnValue())
            rval_.setUndefined();
        return MutableHandleValue::fromMarkedLocation(&rval_);
    }

    void markReturnValue() {
        flags_ |= HAS_RVAL;
    }

    void setReturnValue(const Value& v) {
        rval_ = v;
        markReturnValue();
    }

    void clearReturnValue() {
        rval_.setUndefined();
        markReturnValue();
    }

    void resumeGeneratorFrame(JSObject* scopeChain) {
        MOZ_ASSERT(script()->isGenerator());
        MOZ_ASSERT(isNonEvalFunctionFrame());
        flags_ |= HAS_CALL_OBJ | HAS_SCOPECHAIN;
        scopeChain_ = scopeChain;
    }

    



    InitialFrameFlags initialFlags() const {
        JS_STATIC_ASSERT((int)INITIAL_NONE == 0);
        JS_STATIC_ASSERT((int)INITIAL_CONSTRUCT == (int)CONSTRUCTING);
        uint32_t mask = CONSTRUCTING;
        MOZ_ASSERT((flags_ & mask) != mask);
        return InitialFrameFlags(flags_ & mask);
    }

    void setConstructing() {
        flags_ |= CONSTRUCTING;
    }

    bool isConstructing() const {
        return !!(flags_ & CONSTRUCTING);
    }

    







    inline bool hasCallObj() const;

    bool hasCallObjUnchecked() const {
        return flags_ & HAS_CALL_OBJ;
    }

    bool hasArgsObj() const {
        MOZ_ASSERT(script()->needsArgsObj());
        return flags_ & HAS_ARGS_OBJ;
    }

    void setCreateSingleton() {
        MOZ_ASSERT(isConstructing());
        flags_ |= CREATE_SINGLETON;
    }
    bool createSingleton() const {
        MOZ_ASSERT(isConstructing());
        return flags_ & CREATE_SINGLETON;
    }

    bool isDebuggerEvalFrame() const {
        return !!(flags_ & DEBUGGER_EVAL);
    }

    bool prevUpToDate() const {
        return !!(flags_ & PREV_UP_TO_DATE);
    }

    void setPrevUpToDate() {
        flags_ |= PREV_UP_TO_DATE;
    }

    void unsetPrevUpToDate() {
        flags_ &= ~PREV_UP_TO_DATE;
    }

    bool isDebuggee() const {
        return !!(flags_ & DEBUGGEE);
    }

    void setIsDebuggee() {
        flags_ |= DEBUGGEE;
    }

    inline void unsetIsDebuggee();

  public:
    void mark(JSTracer* trc);
    void markValues(JSTracer* trc, unsigned start, unsigned end);
    void markValues(JSTracer* trc, Value* sp, jsbytecode* pc);

    
    bool runningInJit() const {
        return !!(flags_ & RUNNING_IN_JIT);
    }
    void setRunningInJit() {
        flags_ |= RUNNING_IN_JIT;
    }
    void clearRunningInJit() {
        flags_ &= ~RUNNING_IN_JIT;
    }
};

static const size_t VALUES_PER_STACK_FRAME = sizeof(InterpreterFrame) / sizeof(Value);

static inline InterpreterFrame::Flags
ToFrameFlags(InitialFrameFlags initial)
{
    return InterpreterFrame::Flags(initial);
}

static inline InitialFrameFlags
InitialFrameFlagsFromConstructing(bool b)
{
    return b ? INITIAL_CONSTRUCT : INITIAL_NONE;
}

static inline bool
InitialFrameFlagsAreConstructing(InitialFrameFlags initial)
{
    return !!(initial & INITIAL_CONSTRUCT);
}



class InterpreterRegs
{
  public:
    Value* sp;
    jsbytecode* pc;
  private:
    InterpreterFrame* fp_;
  public:
    InterpreterFrame* fp() const { return fp_; }

    unsigned stackDepth() const {
        MOZ_ASSERT(sp >= fp_->base());
        return sp - fp_->base();
    }

    Value* spForStackDepth(unsigned depth) const {
        MOZ_ASSERT(fp_->script()->nfixed() + depth <= fp_->script()->nslots());
        return fp_->base() + depth;
    }

    
    void rebaseFromTo(const InterpreterRegs& from, InterpreterFrame& to) {
        fp_ = &to;
        sp = to.slots() + (from.sp - from.fp_->slots());
        pc = from.pc;
        MOZ_ASSERT(fp_);
    }

    void popInlineFrame() {
        pc = fp_->prevpc();
        sp = fp_->prevsp() - fp_->numActualArgs() - 1;
        fp_ = fp_->prev();
        MOZ_ASSERT(fp_);
    }
    void prepareToRun(InterpreterFrame& fp, JSScript* script) {
        pc = script->code();
        sp = fp.slots() + script->nfixed();
        fp_ = &fp;
    }

    void setToEndOfScript();

    MutableHandleValue stackHandleAt(int i) {
        return MutableHandleValue::fromMarkedLocation(&sp[i]);
    }

    HandleValue stackHandleAt(int i) const {
        return HandleValue::fromMarkedLocation(&sp[i]);
    }

    friend void GDBTestInitInterpreterRegs(InterpreterRegs&, js::InterpreterFrame*,
                                           JS::Value*, uint8_t*);
};



class InterpreterStack
{
    friend class InterpreterActivation;

    static const size_t DEFAULT_CHUNK_SIZE = 4 * 1024;
    LifoAlloc allocator_;

    
    static const size_t MAX_FRAMES = 50 * 1000;
    static const size_t MAX_FRAMES_TRUSTED = MAX_FRAMES + 1000;
    size_t frameCount_;

    inline uint8_t* allocateFrame(JSContext* cx, size_t size);

    inline InterpreterFrame*
    getCallFrame(JSContext* cx, const CallArgs& args, HandleScript script,
                 InterpreterFrame::Flags* pflags, Value** pargv);

    void releaseFrame(InterpreterFrame* fp) {
        frameCount_--;
        allocator_.release(fp->mark_);
    }

  public:
    InterpreterStack()
      : allocator_(DEFAULT_CHUNK_SIZE),
        frameCount_(0)
    { }

    ~InterpreterStack() {
        MOZ_ASSERT(frameCount_ == 0);
    }

    
    InterpreterFrame* pushExecuteFrame(JSContext* cx, HandleScript script, const Value& thisv,
                                 HandleObject scopeChain, ExecuteType type,
                                 AbstractFramePtr evalInFrame);

    
    InterpreterFrame* pushInvokeFrame(JSContext* cx, const CallArgs& args,
                                      InitialFrameFlags initial);

    
    
    bool pushInlineFrame(JSContext* cx, InterpreterRegs& regs, const CallArgs& args,
                         HandleScript script, InitialFrameFlags initial);

    void popInlineFrame(InterpreterRegs& regs);

    bool resumeGeneratorCallFrame(JSContext* cx, InterpreterRegs& regs,
                                  HandleFunction callee, HandleValue thisv,
                                  HandleObject scopeChain);

    inline void purge(JSRuntime* rt);

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return allocator_.sizeOfExcludingThis(mallocSizeOf);
    }
};

void MarkInterpreterActivations(JSRuntime* rt, JSTracer* trc);



class InvokeArgs : public JS::CallArgs
{
    AutoValueVector v_;

  public:
    explicit InvokeArgs(JSContext* cx) : v_(cx) {}

    bool init(unsigned argc) {
        if (!v_.resize(2 + argc))
            return false;
        ImplicitCast<CallArgs>(*this) = CallArgsFromVp(argc, v_.begin());
        return true;
    }
};

template <>
struct DefaultHasher<AbstractFramePtr> {
    typedef AbstractFramePtr Lookup;

    static js::HashNumber hash(const Lookup& key) {
        return size_t(key.raw());
    }

    static bool match(const AbstractFramePtr& k, const Lookup& l) {
        return k == l;
    }
};



class InterpreterActivation;
class AsmJSActivation;

namespace jit {
    class JitActivation;
};

class Activation
{
  protected:
    JSContext* cx_;
    JSCompartment* compartment_;
    Activation* prev_;
    Activation* prevProfiling_;

    
    
    
    
    size_t savedFrameChain_;

    
    
    
    
    
    size_t hideScriptedCallerCount_;

    
    
    
    
    
    
    Rooted<SavedFrame*> asyncStack_;

    
    RootedString asyncCause_;

    enum Kind { Interpreter, Jit, AsmJS };
    Kind kind_;

    inline Activation(JSContext* cx, Kind kind_);
    inline ~Activation();

  public:
    JSContext* cx() const {
        return cx_;
    }
    JSCompartment* compartment() const {
        return compartment_;
    }
    Activation* prev() const {
        return prev_;
    }
    Activation* prevProfiling() const { return prevProfiling_; }
    inline Activation* mostRecentProfiling();

    bool isInterpreter() const {
        return kind_ == Interpreter;
    }
    bool isJit() const {
        return kind_ == Jit;
    }
    bool isAsmJS() const {
        return kind_ == AsmJS;
    }

    inline bool isProfiling() const;
    void registerProfiling();
    void unregisterProfiling();

    InterpreterActivation* asInterpreter() const {
        MOZ_ASSERT(isInterpreter());
        return (InterpreterActivation*)this;
    }
    jit::JitActivation* asJit() const {
        MOZ_ASSERT(isJit());
        return (jit::JitActivation*)this;
    }
    AsmJSActivation* asAsmJS() const {
        MOZ_ASSERT(isAsmJS());
        return (AsmJSActivation*)this;
    }

    void saveFrameChain() {
        savedFrameChain_++;
    }
    void restoreFrameChain() {
        MOZ_ASSERT(savedFrameChain_ > 0);
        savedFrameChain_--;
    }
    bool hasSavedFrameChain() const {
        return savedFrameChain_ > 0;
    }

    void hideScriptedCaller() {
        hideScriptedCallerCount_++;
    }
    void unhideScriptedCaller() {
        MOZ_ASSERT(hideScriptedCallerCount_ > 0);
        hideScriptedCallerCount_--;
    }
    bool scriptedCallerIsHidden() const {
        return hideScriptedCallerCount_ > 0;
    }

    static size_t offsetOfPrevProfiling() {
        return offsetof(Activation, prevProfiling_);
    }

    SavedFrame* asyncStack() {
        return asyncStack_;
    }

    JSString* asyncCause() {
        return asyncCause_;
    }

  private:
    Activation(const Activation& other) = delete;
    void operator=(const Activation& other) = delete;
};




static const jsbytecode EnableInterruptsPseudoOpcode = -1;

static_assert(EnableInterruptsPseudoOpcode >= JSOP_LIMIT,
              "EnableInterruptsPseudoOpcode must be greater than any opcode");
static_assert(EnableInterruptsPseudoOpcode == jsbytecode(-1),
              "EnableInterruptsPseudoOpcode must be the maximum jsbytecode value");

class InterpreterFrameIterator;
class RunState;

class InterpreterActivation : public Activation
{
    friend class js::InterpreterFrameIterator;

    InterpreterRegs regs_;
    InterpreterFrame* entryFrame_;
    size_t opMask_; 

#ifdef DEBUG
    size_t oldFrameCount_;
#endif

  public:
    inline InterpreterActivation(RunState& state, JSContext* cx, InterpreterFrame* entryFrame);
    inline ~InterpreterActivation();

    inline bool pushInlineFrame(const CallArgs& args, HandleScript script,
                                InitialFrameFlags initial);
    inline void popInlineFrame(InterpreterFrame* frame);

    inline bool resumeGeneratorFrame(HandleFunction callee, HandleValue thisv,
                                     HandleObject scopeChain);

    InterpreterFrame* current() const {
        return regs_.fp();
    }
    InterpreterRegs& regs() {
        return regs_;
    }
    InterpreterFrame* entryFrame() const {
        return entryFrame_;
    }
    size_t opMask() const {
        return opMask_;
    }

    bool isProfiling() const {
        return false;
    }

    
    void enableInterruptsIfRunning(JSScript* script) {
        if (regs_.fp()->script() == script)
            enableInterruptsUnconditionally();
    }
    void enableInterruptsUnconditionally() {
        opMask_ = EnableInterruptsPseudoOpcode;
    }
    void clearInterruptsMask() {
        opMask_ = 0;
    }
};



class ActivationIterator
{
    uint8_t* jitTop_;

  protected:
    Activation* activation_;

  private:
    void settle();

  public:
    explicit ActivationIterator(JSRuntime* rt);

    ActivationIterator& operator++();

    Activation* operator->() const {
        return activation_;
    }
    Activation* activation() const {
        return activation_;
    }
    uint8_t* jitTop() const {
        MOZ_ASSERT(activation_->isJit());
        return jitTop_;
    }
    bool done() const {
        return activation_ == nullptr;
    }
};

namespace jit {

class BailoutFrameInfo;


class JitActivation : public Activation
{
    uint8_t* prevJitTop_;
    JitActivation* prevJitActivation_;
    JSContext* prevJitJSContext_;
    bool active_;

    
    
    
    
    
    typedef Vector<RematerializedFrame*> RematerializedFrameVector;
    typedef HashMap<uint8_t*, RematerializedFrameVector> RematerializedFrameTable;
    RematerializedFrameTable* rematerializedFrames_;

    
    
    
    
    
    
    
    typedef Vector<RInstructionResults, 1> IonRecoveryMap;
    IonRecoveryMap ionRecovery_;

    
    
    
    
    
    BailoutFrameInfo* bailoutData_;

    
    
    
    mozilla::Atomic<void*, mozilla::Relaxed> lastProfilingFrame_;
    mozilla::Atomic<void*, mozilla::Relaxed> lastProfilingCallSite_;
    static_assert(sizeof(mozilla::Atomic<void*, mozilla::Relaxed>) == sizeof(void*),
                  "Atomic should have same memory format as underlying type.");

    void clearRematerializedFrames();

#ifdef CHECK_OSIPOINT_REGISTERS
  protected:
    
    
    uint32_t checkRegs_;
    RegisterDump regs_;
#endif

  public:
    explicit JitActivation(JSContext* cx, bool active = true);
    ~JitActivation();

    bool isActive() const {
        return active_;
    }
    void setActive(JSContext* cx, bool active = true);

    bool isProfiling() const;

    uint8_t* prevJitTop() const {
        return prevJitTop_;
    }
    JitActivation* prevJitActivation() const {
        return prevJitActivation_;
    }
    static size_t offsetOfPrevJitTop() {
        return offsetof(JitActivation, prevJitTop_);
    }
    static size_t offsetOfPrevJitJSContext() {
        return offsetof(JitActivation, prevJitJSContext_);
    }
    static size_t offsetOfPrevJitActivation() {
        return offsetof(JitActivation, prevJitActivation_);
    }
    static size_t offsetOfActiveUint8() {
        MOZ_ASSERT(sizeof(bool) == 1);
        return offsetof(JitActivation, active_);
    }

#ifdef CHECK_OSIPOINT_REGISTERS
    void setCheckRegs(bool check) {
        checkRegs_ = check;
    }
    static size_t offsetOfCheckRegs() {
        return offsetof(JitActivation, checkRegs_);
    }
    static size_t offsetOfRegs() {
        return offsetof(JitActivation, regs_);
    }
#endif

    
    
    
    
    
    
    RematerializedFrame* getRematerializedFrame(JSContext* cx, const JitFrameIterator& iter,
                                                size_t inlineDepth = 0);

    
    
    RematerializedFrame* lookupRematerializedFrame(uint8_t* top, size_t inlineDepth = 0);

    bool hasRematerializedFrame(uint8_t* top, size_t inlineDepth = 0) {
        return !!lookupRematerializedFrame(top, inlineDepth);
    }

    
    void removeRematerializedFrame(uint8_t* top);

    void markRematerializedFrames(JSTracer* trc);


    
    bool registerIonFrameRecovery(RInstructionResults&& results);

    
    RInstructionResults* maybeIonFrameRecovery(JitFrameLayout* fp);

    
    
    void removeIonFrameRecovery(JitFrameLayout* fp);

    void markIonRecovery(JSTracer* trc);

    
    const BailoutFrameInfo* bailoutData() const { return bailoutData_; }

    
    void setBailoutData(BailoutFrameInfo* bailoutData);

    
    void cleanBailoutData();

    static size_t offsetOfLastProfilingFrame() {
        return offsetof(JitActivation, lastProfilingFrame_);
    }
    void* lastProfilingFrame() {
        return lastProfilingFrame_;
    }
    void setLastProfilingFrame(void* ptr) {
        lastProfilingFrame_ = ptr;
    }

    static size_t offsetOfLastProfilingCallSite() {
        return offsetof(JitActivation, lastProfilingCallSite_);
    }
    void* lastProfilingCallSite() {
        return lastProfilingCallSite_;
    }
    void setLastProfilingCallSite(void* ptr) {
        lastProfilingCallSite_ = ptr;
    }
};


class JitActivationIterator : public ActivationIterator
{
    void settle() {
        while (!done() && !activation_->isJit())
            ActivationIterator::operator++();
    }

  public:
    explicit JitActivationIterator(JSRuntime* rt)
      : ActivationIterator(rt)
    {
        settle();
    }

    JitActivationIterator& operator++() {
        ActivationIterator::operator++();
        settle();
        return *this;
    }

    
    void jitStackRange(uintptr_t*& min, uintptr_t*& end);
};

} 


class InterpreterFrameIterator
{
    InterpreterActivation* activation_;
    InterpreterFrame* fp_;
    jsbytecode* pc_;
    Value* sp_;

  public:
    explicit InterpreterFrameIterator(InterpreterActivation* activation)
      : activation_(activation),
        fp_(nullptr),
        pc_(nullptr),
        sp_(nullptr)
    {
        if (activation) {
            fp_ = activation->current();
            pc_ = activation->regs().pc;
            sp_ = activation->regs().sp;
        }
    }

    InterpreterFrame* frame() const {
        MOZ_ASSERT(!done());
        return fp_;
    }
    jsbytecode* pc() const {
        MOZ_ASSERT(!done());
        return pc_;
    }
    Value* sp() const {
        MOZ_ASSERT(!done());
        return sp_;
    }

    InterpreterFrameIterator& operator++();

    bool done() const {
        return fp_ == nullptr;
    }
};










class AsmJSActivation : public Activation
{
    AsmJSModule& module_;
    AsmJSActivation* prevAsmJS_;
    AsmJSActivation* prevAsmJSForModule_;
    void* entrySP_;
    void* resumePC_;
    uint8_t* fp_;
    AsmJSExit::Reason exitReason_;

  public:
    AsmJSActivation(JSContext* cx, AsmJSModule& module);
    ~AsmJSActivation();

    inline JSContext* cx();
    AsmJSModule& module() const { return module_; }
    AsmJSActivation* prevAsmJS() const { return prevAsmJS_; }

    bool isProfiling() const {
        return true;
    }

    
    
    uint8_t* fp() const { return fp_; }

    
    AsmJSExit::Reason exitReason() const { return exitReason_; }

    
    static unsigned offsetOfContext() { return offsetof(AsmJSActivation, cx_); }
    static unsigned offsetOfResumePC() { return offsetof(AsmJSActivation, resumePC_); }

    
    static unsigned offsetOfEntrySP() { return offsetof(AsmJSActivation, entrySP_); }
    static unsigned offsetOfFP() { return offsetof(AsmJSActivation, fp_); }
    static unsigned offsetOfExitReason() { return offsetof(AsmJSActivation, exitReason_); }

    
    void setResumePC(void* pc) { resumePC_ = pc; }
    void* resumePC() const { return resumePC_; }
};























class FrameIter
{
  public:
    enum SavedOption { STOP_AT_SAVED, GO_THROUGH_SAVED };
    enum ContextOption { CURRENT_CONTEXT, ALL_CONTEXTS };
    enum DebuggerEvalOption { FOLLOW_DEBUGGER_EVAL_PREV_LINK,
                              IGNORE_DEBUGGER_EVAL_PREV_LINK };
    enum State { DONE, INTERP, JIT, ASMJS };

    
    
    struct Data
    {
        JSContext * cx_;
        SavedOption         savedOption_;
        ContextOption       contextOption_;
        DebuggerEvalOption  debuggerEvalOption_;
        JSPrincipals *      principals_;

        State               state_;

        jsbytecode *        pc_;

        InterpreterFrameIterator interpFrames_;
        ActivationIterator activations_;

        jit::JitFrameIterator jitFrames_;
        unsigned ionInlineFrameNo_;
        AsmJSFrameIterator asmJSFrames_;

        Data(JSContext* cx, SavedOption savedOption, ContextOption contextOption,
             DebuggerEvalOption debuggerEvalOption, JSPrincipals* principals);
        Data(const Data& other);
    };

    MOZ_IMPLICIT FrameIter(JSContext* cx, SavedOption = STOP_AT_SAVED);
    FrameIter(JSContext* cx, ContextOption, SavedOption,
              DebuggerEvalOption = FOLLOW_DEBUGGER_EVAL_PREV_LINK);
    FrameIter(JSContext* cx, ContextOption, SavedOption, DebuggerEvalOption, JSPrincipals*);
    FrameIter(const FrameIter& iter);
    MOZ_IMPLICIT FrameIter(const Data& data);
    MOZ_IMPLICIT FrameIter(AbstractFramePtr frame);

    bool done() const { return data_.state_ == DONE; }

    
    
    

    FrameIter& operator++();

    JSCompartment* compartment() const;
    Activation* activation() const { return data_.activations_.activation(); }

    bool isInterp() const { MOZ_ASSERT(!done()); return data_.state_ == INTERP;  }
    bool isJit() const { MOZ_ASSERT(!done()); return data_.state_ == JIT; }
    bool isAsmJS() const { MOZ_ASSERT(!done()); return data_.state_ == ASMJS; }
    inline bool isIon() const;
    inline bool isBaseline() const;

    bool isFunctionFrame() const;
    bool isGlobalFrame() const;
    bool isEvalFrame() const;
    bool isNonEvalFunctionFrame() const;
    bool hasArgs() const { return isNonEvalFunctionFrame(); }

    ScriptSource* scriptSource() const;
    const char* scriptFilename() const;
    const char16_t* scriptDisplayURL() const;
    unsigned computeLine(uint32_t* column = nullptr) const;
    JSAtom* functionDisplayAtom() const;
    bool mutedErrors() const;

    bool hasScript() const { return !isAsmJS(); }

    
    
    

    inline JSScript* script() const;

    bool        isConstructing() const;
    jsbytecode* pc() const { MOZ_ASSERT(!done()); return data_.pc_; }
    void        updatePcQuadratic();

    
    
    
    
    
    
    JSFunction* calleeTemplate() const;
    JSFunction* callee(JSContext* cx) const;

    JSFunction* maybeCallee(JSContext* cx) const {
        return isFunctionFrame() ? callee(cx) : nullptr;
    }

    bool        matchCallee(JSContext* cx, HandleFunction fun) const;

    unsigned    numActualArgs() const;
    unsigned    numFormalArgs() const;
    Value       unaliasedActual(unsigned i, MaybeCheckAliasing = CHECK_ALIASING) const;
    template <class Op> inline void unaliasedForEachActual(JSContext* cx, Op op);

    JSObject*  scopeChain(JSContext* cx) const;
    CallObject& callObj(JSContext* cx) const;

    bool        hasArgsObj() const;
    ArgumentsObject& argsObj() const;

    
    bool        computeThis(JSContext* cx) const;
    
    
    
    
    
    
    
    Value       computedThisValue() const;
    Value       thisv(JSContext* cx) const;

    Value       returnValue() const;
    void        setReturnValue(const Value& v);

    
    size_t      numFrameSlots() const;
    Value       frameSlotValue(size_t index) const;

    
    
    bool ensureHasRematerializedFrame(JSContext* cx);

    
    
    bool hasUsableAbstractFramePtr() const;

    
    
    
    
    

    AbstractFramePtr abstractFramePtr() const;
    AbstractFramePtr copyDataAsAbstractFramePtr() const;
    Data* copyData() const;

    
    inline InterpreterFrame* interpFrame() const;

  private:
    Data data_;
    jit::InlineFrameIterator ionInlineFrames_;

    void popActivation();
    void popInterpreterFrame();
    void nextJitFrame();
    void popJitFrame();
    void popAsmJSFrame();
    void settleOnActivation();
};

class ScriptFrameIter : public FrameIter
{
    void settle() {
        while (!done() && !hasScript())
            FrameIter::operator++();
    }

  public:
    explicit ScriptFrameIter(JSContext* cx, SavedOption savedOption = STOP_AT_SAVED)
      : FrameIter(cx, savedOption)
    {
        settle();
    }

    ScriptFrameIter(JSContext* cx,
                    ContextOption cxOption,
                    SavedOption savedOption,
                    DebuggerEvalOption debuggerEvalOption = FOLLOW_DEBUGGER_EVAL_PREV_LINK)
      : FrameIter(cx, cxOption, savedOption, debuggerEvalOption)
    {
        settle();
    }

    ScriptFrameIter(JSContext* cx,
                    ContextOption cxOption,
                    SavedOption savedOption,
                    DebuggerEvalOption debuggerEvalOption,
                    JSPrincipals* prin)
      : FrameIter(cx, cxOption, savedOption, debuggerEvalOption, prin)
    {
        settle();
    }

    ScriptFrameIter(const ScriptFrameIter& iter) : FrameIter(iter) { settle(); }
    explicit ScriptFrameIter(const FrameIter::Data& data) : FrameIter(data) { settle(); }
    explicit ScriptFrameIter(AbstractFramePtr frame) : FrameIter(frame) { settle(); }

    ScriptFrameIter& operator++() {
        FrameIter::operator++();
        settle();
        return *this;
    }
};

#ifdef DEBUG
bool SelfHostedFramesVisible();
#else
static inline bool
SelfHostedFramesVisible()
{
    return false;
}
#endif


class NonBuiltinFrameIter : public FrameIter
{
    void settle();

  public:
    explicit NonBuiltinFrameIter(JSContext* cx,
                                 FrameIter::SavedOption opt = FrameIter::STOP_AT_SAVED)
      : FrameIter(cx, opt)
    {
        settle();
    }

    NonBuiltinFrameIter(JSContext* cx,
                        FrameIter::ContextOption contextOption,
                        FrameIter::SavedOption savedOption,
                        FrameIter::DebuggerEvalOption debuggerEvalOption =
                        FrameIter::FOLLOW_DEBUGGER_EVAL_PREV_LINK)
      : FrameIter(cx, contextOption, savedOption, debuggerEvalOption)
    {
        settle();
    }

    NonBuiltinFrameIter(JSContext* cx,
                        FrameIter::ContextOption contextOption,
                        FrameIter::SavedOption savedOption,
                        FrameIter::DebuggerEvalOption debuggerEvalOption,
                        JSPrincipals* principals)
      : FrameIter(cx, contextOption, savedOption, debuggerEvalOption, principals)
    {
        settle();
    }

    explicit NonBuiltinFrameIter(const FrameIter::Data& data)
      : FrameIter(data)
    {}

    NonBuiltinFrameIter& operator++() {
        FrameIter::operator++();
        settle();
        return *this;
    }
};


class NonBuiltinScriptFrameIter : public ScriptFrameIter
{
    void settle();

  public:
    explicit NonBuiltinScriptFrameIter(JSContext* cx,
                                       ScriptFrameIter::SavedOption opt =
                                       ScriptFrameIter::STOP_AT_SAVED)
      : ScriptFrameIter(cx, opt)
    {
        settle();
    }

    NonBuiltinScriptFrameIter(JSContext* cx,
                              ScriptFrameIter::ContextOption contextOption,
                              ScriptFrameIter::SavedOption savedOption,
                              ScriptFrameIter::DebuggerEvalOption debuggerEvalOption =
                              ScriptFrameIter::FOLLOW_DEBUGGER_EVAL_PREV_LINK)
      : ScriptFrameIter(cx, contextOption, savedOption, debuggerEvalOption)
    {
        settle();
    }

    NonBuiltinScriptFrameIter(JSContext* cx,
                              ScriptFrameIter::ContextOption contextOption,
                              ScriptFrameIter::SavedOption savedOption,
                              ScriptFrameIter::DebuggerEvalOption debuggerEvalOption,
                              JSPrincipals* principals)
      : ScriptFrameIter(cx, contextOption, savedOption, debuggerEvalOption, principals)
    {
        settle();
    }

    explicit NonBuiltinScriptFrameIter(const ScriptFrameIter::Data& data)
      : ScriptFrameIter(data)
    {}

    NonBuiltinScriptFrameIter& operator++() {
        ScriptFrameIter::operator++();
        settle();
        return *this;
    }
};





class AllFramesIter : public ScriptFrameIter
{
  public:
    explicit AllFramesIter(JSContext* cx)
      : ScriptFrameIter(cx, ScriptFrameIter::ALL_CONTEXTS, ScriptFrameIter::GO_THROUGH_SAVED,
                        ScriptFrameIter::IGNORE_DEBUGGER_EVAL_PREV_LINK)
    {}
};



inline JSScript*
FrameIter::script() const
{
    MOZ_ASSERT(!done());
    if (data_.state_ == INTERP)
        return interpFrame()->script();
    MOZ_ASSERT(data_.state_ == JIT);
    if (data_.jitFrames_.isIonJS())
        return ionInlineFrames_.script();
    return data_.jitFrames_.script();
}

inline bool
FrameIter::isIon() const
{
    return isJit() && data_.jitFrames_.isIonJS();
}

inline bool
FrameIter::isBaseline() const
{
    return isJit() && data_.jitFrames_.isBaselineJS();
}

inline InterpreterFrame*
FrameIter::interpFrame() const
{
    MOZ_ASSERT(data_.state_ == INTERP);
    return data_.interpFrames_.frame();
}

}  
#endif 
