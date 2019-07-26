





#ifndef Stack_h__
#define Stack_h__

#include "jsfun.h"
#include "jsscript.h"
#include "ion/IonFrameIterator.h"
#include "jsautooplen.h"

struct JSContext;
struct JSCompartment;

namespace js {

class StackFrame;
class FrameRegs;
class StackSegment;
class StackSpace;
class ContextStack;

class InvokeArgsGuard;
class InvokeFrameGuard;
class FrameGuard;
class ExecuteFrameGuard;
class GeneratorFrameGuard;

class CallIter;
class ScriptFrameIter;
class AllFramesIter;

class ArgumentsObject;
class ScopeObject;
class StaticBlockObject;

struct ScopeCoordinate;









































































enum MaybeCheckAliasing { CHECK_ALIASING = true, DONT_CHECK_ALIASING = false };



#ifdef DEBUG
extern void
CheckLocalUnaliased(MaybeCheckAliasing checkAliasing, JSScript *script,
                    StaticBlockObject *maybeBlock, unsigned i);
#endif

namespace ion {
    class BaselineFrame;
}


class AbstractFramePtr
{
    uintptr_t ptr_;

  public:
    AbstractFramePtr()
      : ptr_(0)
    {}

    AbstractFramePtr(StackFrame *fp)
        : ptr_(fp ? uintptr_t(fp) | 0x1 : 0)
    {
        JS_ASSERT((uintptr_t(fp) & 1) == 0);
    }

    AbstractFramePtr(ion::BaselineFrame *fp)
      : ptr_(uintptr_t(fp))
    {
        JS_ASSERT((uintptr_t(fp) & 1) == 0);
    }

    explicit AbstractFramePtr(JSAbstractFramePtr frame)
        : ptr_(uintptr_t(frame.raw()))
    {
    }

    bool isStackFrame() const {
        return ptr_ & 0x1;
    }
    StackFrame *asStackFrame() const {
        JS_ASSERT(isStackFrame());
        StackFrame *res = (StackFrame *)(ptr_ & ~0x1);
        JS_ASSERT(res);
        return res;
    }
    bool isBaselineFrame() const {
        return ptr_ && !isStackFrame();
    }
    ion::BaselineFrame *asBaselineFrame() const {
        JS_ASSERT(isBaselineFrame());
        ion::BaselineFrame *res = (ion::BaselineFrame *)ptr_;
        JS_ASSERT(res);
        return res;
    }

    void *raw() const { return reinterpret_cast<void *>(ptr_); }

    bool operator ==(const AbstractFramePtr &other) const { return ptr_ == other.ptr_; }
    bool operator !=(const AbstractFramePtr &other) const { return ptr_ != other.ptr_; }

    operator bool() const { return !!ptr_; }

    inline JSGenerator *maybeSuspendedGenerator(JSRuntime *rt) const;

    inline JSObject *scopeChain() const;
    inline CallObject &callObj() const;
    inline bool initFunctionScopeObjects(JSContext *cx);
    inline JSCompartment *compartment() const;

    inline StaticBlockObject *maybeBlockChain() const;
    inline bool hasCallObj() const;
    inline bool isGeneratorFrame() const;
    inline bool isYielding() const;
    inline bool isFunctionFrame() const;
    inline bool isGlobalFrame() const;
    inline bool isEvalFrame() const;
    inline bool isFramePushedByExecute() const;
    inline bool isDebuggerFrame() const;

    inline JSScript *script() const;
    inline JSFunction *fun() const;
    inline JSFunction *maybeFun() const;
    inline JSFunction *callee() const;
    inline Value calleev() const;
    inline Value &thisValue() const;

    inline bool isNonEvalFunctionFrame() const;
    inline bool isNonStrictDirectEvalFrame() const;
    inline bool isStrictEvalFrame() const;

    inline unsigned numActualArgs() const;
    inline unsigned numFormalArgs() const;

    inline Value *argv() const;

    inline bool hasArgsObj() const;
    inline ArgumentsObject &argsObj() const;
    inline void initArgsObj(ArgumentsObject &argsobj) const;
    inline bool useNewType() const;

    inline bool copyRawFrameSlots(AutoValueVector *vec) const;

    inline Value &unaliasedVar(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value &unaliasedLocal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value &unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);
    inline Value &unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING);

    inline bool prevUpToDate() const;
    inline void setPrevUpToDate() const;

    JSObject *evalPrevScopeChain(JSContext *cx) const;

    inline void *maybeHookData() const;
    inline void setHookData(void *data) const;
    inline Value returnValue() const;
    inline void setReturnValue(const Value &rval) const;

    bool hasPushedSPSFrame() const;

    inline void popBlock(JSContext *cx) const;
    inline void popWith(JSContext *cx) const;
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
    INITIAL_CONSTRUCT      =       0x20, 
};

enum ExecuteType {
    EXECUTE_GLOBAL         =        0x1, 
    EXECUTE_DIRECT_EVAL    =        0x4, 
    EXECUTE_INDIRECT_EVAL  =        0x5, 
    EXECUTE_DEBUG          =        0xc, 
    EXECUTE_DEBUG_GLOBAL   =        0xd  
};



class StackFrame
{
  public:
    enum Flags {
        
        GLOBAL             =        0x1,  
        FUNCTION           =        0x2,  

        
        EVAL               =        0x4,  
        DEBUGGER           =        0x8,  
        GENERATOR          =       0x10,  
        CONSTRUCTING       =       0x20,  

        
        YIELDING           =       0x40,  
        FINISHED_IN_INTERP =       0x80,  

        
        HAS_CALL_OBJ       =      0x100,  
        HAS_ARGS_OBJ       =      0x200,  

        
        HAS_HOOK_DATA      =      0x400,  
        HAS_RVAL           =      0x800,  
        HAS_SCOPECHAIN     =     0x1000,  
        HAS_PREVPC         =     0x2000,  
        HAS_BLOCKCHAIN     =     0x4000,  

        
        PREV_UP_TO_DATE    =     0x8000,  

        
        HAS_PUSHED_SPS_FRAME =  0x10000,  

        



        RUNNING_IN_JIT     =    0x20000,

        
        USE_NEW_TYPE       =    0x40000   
    };

  private:
    mutable uint32_t    flags_;         
    union {                             
        JSScript        *script;        
        JSFunction      *fun;           
    } exec;
    union {                             
        unsigned        nactual;        
        JSScript        *evalScript;    
    } u;
    mutable JSObject    *scopeChain_;   
    StackFrame          *prev_;         
    Value               rval_;          
    StaticBlockObject   *blockChain_;   
    ArgumentsObject     *argsObj_;      
    jsbytecode          *prevpc_;       
    void                *hookData_;     
    AbstractFramePtr    evalInFramePrev_; 

    static void staticAsserts() {
        JS_STATIC_ASSERT(offsetof(StackFrame, rval_) % sizeof(Value) == 0);
        JS_STATIC_ASSERT(sizeof(StackFrame) % sizeof(Value) == 0);
    }

    inline void initPrev(JSContext *cx);
    void writeBarrierPost();

    






  public:
    Value *slots() const { return (Value *)(this + 1); }
    Value *base() const { return slots() + script()->nfixed; }
    Value *argv() const { return (Value *)this - Max(numActualArgs(), numFormalArgs()); }

  private:
    friend class FrameRegs;
    friend class ContextStack;
    friend class StackSpace;
    friend class ScriptFrameIter;
    friend class CallObject;
    friend class ClonedBlockObject;
    friend class ArgumentsObject;

    




    
    void initCallFrame(JSContext *cx, JSFunction &callee,
                       JSScript *script, uint32_t nactual, StackFrame::Flags flags);

    
    void initExecuteFrame(JSScript *script, StackFrame *prevLink, AbstractFramePtr prev,
                          FrameRegs *regs, const Value &thisv, JSObject &scopeChain,
                          ExecuteType type);

  public:
    


















    bool prologue(JSContext *cx);
    void epilogue(JSContext *cx);

    bool initFunctionScopeObjects(JSContext *cx);

    
    void initVarsToUndefined();

    









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
        return isEvalFrame() && script()->strict;
    }

    bool isNonStrictEvalFrame() const {
        return isEvalFrame() && !script()->strict;
    }

    bool isDirectEvalFrame() const {
        return isEvalFrame() && script()->staticLevel > 0;
    }

    bool isNonStrictDirectEvalFrame() const {
        return isNonStrictEvalFrame() && isDirectEvalFrame();
    }

    












    StackFrame *prev() const {
        return prev_;
    }

    AbstractFramePtr evalInFramePrev() const {
        JS_ASSERT(isEvalFrame());
        return evalInFramePrev_;
    }

    inline void resetGeneratorPrev(JSContext *cx);

    

















    inline Value &unaliasedVar(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    inline Value &unaliasedLocal(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);

    bool hasArgs() const { return isNonEvalFunctionFrame(); }
    inline Value &unaliasedFormal(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    inline Value &unaliasedActual(unsigned i, MaybeCheckAliasing = CHECK_ALIASING);
    template <class Op> inline void forEachUnaliasedActual(Op op);

    bool copyRawFrameSlots(AutoValueVector *v);

    unsigned numFormalArgs() const { JS_ASSERT(hasArgs()); return fun()->nargs; }
    unsigned numActualArgs() const { JS_ASSERT(hasArgs()); return u.nactual; }

    inline Value &canonicalActualArg(unsigned i) const;
    template <class Op>
    inline bool forEachCanonicalActualArg(Op op, unsigned start = 0, unsigned count = unsigned(-1));
    template <class Op> inline bool forEachFormalArg(Op op);

    










    ArgumentsObject &argsObj() const;
    void initArgsObj(ArgumentsObject &argsobj);

    JSObject *createRestParameter(JSContext *cx);

    


















    inline HandleObject scopeChain() const;

    inline ScopeObject &aliasedVarScope(ScopeCoordinate sc) const;
    inline GlobalObject &global() const;
    inline CallObject &callObj() const;
    inline JSObject &varObj();

    inline void pushOnScopeChain(ScopeObject &scope);
    inline void popOffScopeChain();

    










    bool hasBlockChain() const {
        return (flags_ & HAS_BLOCKCHAIN) && blockChain_;
    }

    StaticBlockObject *maybeBlockChain() {
        return (flags_ & HAS_BLOCKCHAIN) ? blockChain_ : NULL;
    }

    StaticBlockObject &blockChain() const {
        JS_ASSERT(hasBlockChain());
        return *blockChain_;
    }

    bool pushBlock(JSContext *cx, StaticBlockObject &block);
    void popBlock(JSContext *cx);

    






    void popWith(JSContext *cx);

    














    JSScript *script() const {
        return isFunctionFrame()
               ? isEvalFrame()
                 ? u.evalScript
                 : fun()->nonLazyScript()
               : exec.script;
    }

    














    jsbytecode *pcQuadratic(const ContextStack &stack, size_t maxDepth = SIZE_MAX);

    
    jsbytecode *prevpc() {
        JS_ASSERT(flags_ & HAS_PREVPC);
        return prevpc_;
    }

    








    JSFunction* fun() const {
        JS_ASSERT(isFunctionFrame());
        return exec.fun;
    }

    JSFunction* maybeFun() const {
        return isFunctionFrame() ? fun() : NULL;
    }

    












    Value &functionThis() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((Value *)this)[-1];
        return argv()[-1];
    }

    JSObject &constructorThis() const {
        JS_ASSERT(hasArgs());
        return argv()[-1].toObject();
    }

    Value &thisValue() const {
        if (flags_ & (EVAL | GLOBAL))
            return ((Value *)this)[-1];
        return argv()[-1];
    }

    








    JSFunction &callee() const {
        JS_ASSERT(isFunctionFrame());
        return *calleev().toObject().toFunction();
    }

    const Value &calleev() const {
        JS_ASSERT(isFunctionFrame());
        return mutableCalleev();
    }

    const Value &maybeCalleev() const {
        Value &calleev = flags_ & (EVAL | GLOBAL)
                         ? ((Value *)this)[-2]
                         : argv()[-2];
        JS_ASSERT(calleev.isObjectOrNull());
        return calleev;
    }

    Value &mutableCalleev() const {
        JS_ASSERT(isFunctionFrame());
        if (isEvalFrame())
            return ((Value *)this)[-2];
        return argv()[-2];
    }

    CallReceiver callReceiver() const {
        return CallReceiverFromArgv(argv());
    }

    






    inline JSCompartment *compartment() const;

    

    bool hasHookData() const {
        return !!(flags_ & HAS_HOOK_DATA);
    }

    void* hookData() const {
        JS_ASSERT(hasHookData());
        return hookData_;
    }

    void* maybeHookData() const {
        return hasHookData() ? hookData_ : NULL;
    }

    void setHookData(void *v) {
        hookData_ = v;
        flags_ |= HAS_HOOK_DATA;
    }

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
        return !!(flags_ & HAS_RVAL);
    }

    Value &returnValue() {
        if (!(flags_ & HAS_RVAL))
            rval_.setUndefined();
        return rval_;
    }

    void markReturnValue() {
        flags_ |= HAS_RVAL;
    }

    void setReturnValue(const Value &v) {
        rval_ = v;
        markReturnValue();
    }

    void clearReturnValue() {
        rval_.setUndefined();
        markReturnValue();
    }

    










    bool isGeneratorFrame() const {
        bool ret = flags_ & GENERATOR;
        JS_ASSERT_IF(ret, isNonEvalFunctionFrame());
        return ret;
    }

    void initGeneratorFrame() const {
        JS_ASSERT(!isGeneratorFrame());
        JS_ASSERT(isNonEvalFunctionFrame());
        flags_ |= GENERATOR;
    }

    Value *generatorArgsSnapshotBegin() const {
        JS_ASSERT(isGeneratorFrame());
        return argv() - 2;
    }

    Value *generatorArgsSnapshotEnd() const {
        JS_ASSERT(isGeneratorFrame());
        return (Value *)this;
    }

    Value *generatorSlotsSnapshotBegin() const {
        JS_ASSERT(isGeneratorFrame());
        return (Value *)(this + 1);
    }

    enum TriggerPostBarriers {
        DoPostBarrier = true,
        NoPostBarrier = false
    };
    template <TriggerPostBarriers doPostBarrier>
    void copyFrameAndValues(JSContext *cx, Value *vp, StackFrame *otherfp,
                            const Value *othervp, Value *othersp);

    JSGenerator *maybeSuspendedGenerator(JSRuntime *rt);

    






    bool isFramePushedByExecute() const {
        return !!(flags_ & (GLOBAL | EVAL));
    }

    



    InitialFrameFlags initialFlags() const {
        JS_STATIC_ASSERT((int)INITIAL_NONE == 0);
        JS_STATIC_ASSERT((int)INITIAL_CONSTRUCT == (int)CONSTRUCTING);
        uint32_t mask = CONSTRUCTING;
        JS_ASSERT((flags_ & mask) != mask);
        return InitialFrameFlags(flags_ & mask);
    }

    void setConstructing() {
        flags_ |= CONSTRUCTING;
    }

    bool isConstructing() const {
        return !!(flags_ & CONSTRUCTING);
    }

    







    bool hasCallObj() const {
        JS_ASSERT(isStrictEvalFrame() || fun()->isHeavyweight());
        return flags_ & HAS_CALL_OBJ;
    }

    bool hasCallObjUnchecked() const {
        return flags_ & HAS_CALL_OBJ;
    }

    bool hasArgsObj() const {
        JS_ASSERT(script()->needsArgsObj());
        return flags_ & HAS_ARGS_OBJ;
    }

    void setUseNewType() {
        JS_ASSERT(isConstructing());
        flags_ |= USE_NEW_TYPE;
    }
    bool useNewType() const {
        JS_ASSERT(isConstructing());
        return flags_ & USE_NEW_TYPE;
    }

    bool isDebuggerFrame() const {
        return !!(flags_ & DEBUGGER);
    }

    bool prevUpToDate() const {
        return !!(flags_ & PREV_UP_TO_DATE);
    }

    void setPrevUpToDate() {
        flags_ |= PREV_UP_TO_DATE;
    }

    bool isYielding() {
        return !!(flags_ & YIELDING);
    }

    void setYielding() {
        flags_ |= YIELDING;
    }

    void clearYielding() {
        flags_ &= ~YIELDING;
    }

    void setFinishedInInterpreter() {
        flags_ |= FINISHED_IN_INTERP;
    }

    bool finishedInInterpreter() const {
        return !!(flags_ & FINISHED_IN_INTERP);
    }

  public:
    static size_t offsetOfFlags() {
        return offsetof(StackFrame, flags_);
    }

    static size_t offsetOfExec() {
        return offsetof(StackFrame, exec);
    }

    static size_t offsetOfNumActual() {
        return offsetof(StackFrame, u.nactual);
    }

    static size_t offsetOfScopeChain() {
        return offsetof(StackFrame, scopeChain_);
    }

    static ptrdiff_t offsetOfThis(JSFunction *fun) {
        return fun == NULL
               ? -1 * ptrdiff_t(sizeof(Value))
               : -(fun->nargs + 1) * ptrdiff_t(sizeof(Value));
    }

    static ptrdiff_t offsetOfFormalArg(JSFunction *fun, unsigned i) {
        JS_ASSERT(i < fun->nargs);
        return (-(int)fun->nargs + i) * sizeof(Value);
    }

    static size_t offsetOfFixed(unsigned i) {
        return sizeof(StackFrame) + i * sizeof(Value);
    }

  public:
    void mark(JSTracer *trc);

    
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

static const size_t VALUES_PER_STACK_FRAME = sizeof(StackFrame) / sizeof(Value);

static inline StackFrame::Flags
ToFrameFlags(InitialFrameFlags initial)
{
    return StackFrame::Flags(initial);
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

inline AbstractFramePtr Valueify(JSAbstractFramePtr frame) { return AbstractFramePtr(frame); }
static inline JSAbstractFramePtr Jsvalify(AbstractFramePtr frame)   { return JSAbstractFramePtr(frame.raw()); }



class FrameRegs
{
  public:
    Value *sp;
    jsbytecode *pc;
  private:
    StackFrame *fp_;
  public:
    StackFrame *fp() const { return fp_; }

    unsigned stackDepth() const {
        JS_ASSERT(sp >= fp_->base());
        return sp - fp_->base();
    }

    Value *spForStackDepth(unsigned depth) const {
        JS_ASSERT(fp_->script()->nfixed + depth <= fp_->script()->nslots);
        return fp_->base() + depth;
    }

    
    void rebaseFromTo(const FrameRegs &from, StackFrame &to) {
        fp_ = &to;
        sp = to.slots() + (from.sp - from.fp_->slots());
        pc = from.pc;
        JS_ASSERT(fp_);
    }

    
    void popFrame(Value *newsp) {
        pc = fp_->prevpc();
        sp = newsp;
        fp_ = fp_->prev();
        JS_ASSERT(fp_);
    }

    
    void prepareToRun(StackFrame &fp, JSScript *script) {
        pc = script->code;
        sp = fp.slots() + script->nfixed;
        fp_ = &fp;
    }

    void setToEndOfScript() {
        JSScript *script = fp()->script();
        sp = fp()->base();
        pc = script->code + script->length - JSOP_STOP_LENGTH;
        JS_ASSERT(*pc == JSOP_STOP);
    }
};



class StackSegment
{
    JSContext *cx_;

    
    StackSegment *const prevInContext_;

    
    StackSegment *const prevInMemory_;

    
    FrameRegs *regs_;

    
    Value *invokeArgsEnd_;

#if JS_BITS_PER_WORD == 32
    



  protected:
    uint32_t padding_;
#endif

  public:
    StackSegment(JSContext *cx,
                 StackSegment *prevInContext,
                 StackSegment *prevInMemory,
                 FrameRegs *regs)
      : cx_(cx),
        prevInContext_(prevInContext),
        prevInMemory_(prevInMemory),
        regs_(regs),
        invokeArgsEnd_(NULL)
    {}

    

    Value *slotsBegin() const {
        return (Value *)(this + 1);
    }

    

    FrameRegs &regs() const {
        JS_ASSERT(regs_);
        return *regs_;
    }

    FrameRegs *maybeRegs() const {
        return regs_;
    }

    StackFrame *fp() const {
        return regs_->fp();
    }

    StackFrame *maybefp() const {
        return regs_ ? regs_->fp() : NULL;
    }

    jsbytecode *maybepc() const {
        return regs_ ? regs_->pc : NULL;
    }

    JSContext *cx() const {
        return cx_;
    }

    StackSegment *prevInContext() const {
        return prevInContext_;
    }

    StackSegment *prevInMemory() const {
        return prevInMemory_;
    }

    void repointRegs(FrameRegs *regs) {
        regs_ = regs;
    }

    bool isEmpty() const {
        return !regs_;
    }

    bool contains(const StackFrame *fp) const;
    bool contains(const FrameRegs *regs) const;

    StackFrame *computeNextFrame(const StackFrame *fp, size_t maxDepth) const;

    Value *end() const;

    FrameRegs *pushRegs(FrameRegs &regs);
    void popRegs(FrameRegs *regs);

    Value *invokeArgsEnd() const {
        return invokeArgsEnd_;
    }
    void pushInvokeArgsEnd(Value *end, Value **prev) {
        *prev = invokeArgsEnd_;
        invokeArgsEnd_ = end;
    }
    void popInvokeArgsEnd(Value *prev) {
        invokeArgsEnd_ = prev;
    }
};

static const size_t VALUES_PER_STACK_SEGMENT = sizeof(StackSegment) / sizeof(Value);
JS_STATIC_ASSERT(sizeof(StackSegment) % sizeof(Value) == 0);



class StackSpace
{
    StackSegment  *seg_;
    Value         *base_;
    mutable Value *conservativeEnd_;
#ifdef XP_WIN
    mutable Value *commitEnd_;
#endif
    Value         *defaultEnd_;
    Value         *trustedEnd_;

    void assertInvariants() const {
        JS_ASSERT(base_ <= conservativeEnd_);
#ifdef XP_WIN
        JS_ASSERT(conservativeEnd_ <= commitEnd_);
        JS_ASSERT(commitEnd_ <= trustedEnd_);
#endif
        JS_ASSERT(conservativeEnd_ <= defaultEnd_);
        JS_ASSERT(defaultEnd_ <= trustedEnd_);
    }

    
    static const size_t CAPACITY_VALS  = 512 * 1024;
    static const size_t CAPACITY_BYTES = CAPACITY_VALS * sizeof(Value);

    
    static const size_t COMMIT_VALS    = 16 * 1024;
    static const size_t COMMIT_BYTES   = COMMIT_VALS * sizeof(Value);

    
    static const size_t BUFFER_VALS    = 16 * 1024;
    static const size_t BUFFER_BYTES   = BUFFER_VALS * sizeof(Value);

    static void staticAsserts() {
        JS_STATIC_ASSERT(CAPACITY_VALS % COMMIT_VALS == 0);
    }

    friend class AllFramesIter;
    friend class ContextStack;
    friend class StackFrame;

    inline bool ensureSpace(JSContext *cx, MaybeReportError report,
                            Value *from, ptrdiff_t nvals) const;
    JS_FRIEND_API(bool) ensureSpaceSlow(JSContext *cx, MaybeReportError report,
                                        Value *from, ptrdiff_t nvals) const;

    StackSegment &findContainingSegment(const StackFrame *target) const;

    bool containsFast(StackFrame *fp) {
        return (Value *)fp >= base_ && (Value *)fp <= trustedEnd_;
    }

    void markFrame(JSTracer *trc, StackFrame *fp, Value *slotsEnd);

  public:
    StackSpace();
    bool init();
    ~StackSpace();

    










    static const unsigned ARGS_LENGTH_MAX = CAPACITY_VALS - (2 * BUFFER_VALS);

    
    inline Value *firstUnused() const { return seg_ ? seg_->end() : base_; }

    StackSegment &containingSegment(const StackFrame *target) const;

    
    void mark(JSTracer *trc);

    
    void markActiveCompartments();

    




    JS_FRIEND_API(size_t) sizeOf();

#ifdef DEBUG
    
    bool containsSlow(StackFrame *fp);
#endif
};



class ContextStack
{
    StackSegment *seg_;
    StackSpace *const space_;
    JSContext *cx_;

    







    bool onTop() const;

#ifdef DEBUG
    void assertSpaceInSync() const;
#else
    void assertSpaceInSync() const {}
#endif

    
    StackSegment *pushSegment(JSContext *cx);
    enum MaybeExtend { CAN_EXTEND = true, CANT_EXTEND = false };
    Value *ensureOnTop(JSContext *cx, MaybeReportError report, unsigned nvars,
                       MaybeExtend extend, bool *pushedSeg);

    inline StackFrame *
    getCallFrame(JSContext *cx, MaybeReportError report, const CallArgs &args,
                 JSFunction *fun, HandleScript script, StackFrame::Flags *pflags) const;

    
    void popSegment();
    friend class InvokeArgsGuard;
    void popInvokeArgs(const InvokeArgsGuard &iag);
    friend class FrameGuard;
    void popFrame(const FrameGuard &fg);
    friend class GeneratorFrameGuard;
    void popGeneratorFrame(const GeneratorFrameGuard &gfg);

    friend class ScriptFrameIter;

  public:
    ContextStack(JSContext *cx);
    ~ContextStack();

    

    



    bool empty() const                { return !seg_; }

    




    inline bool hasfp() const { return seg_ && seg_->maybeRegs(); }

    



    inline FrameRegs *maybeRegs() const { return seg_ ? seg_->maybeRegs() : NULL; }
    inline StackFrame *maybefp() const { return seg_ ? seg_->maybefp() : NULL; }

    
    inline FrameRegs &regs() const { JS_ASSERT(hasfp()); return seg_->regs(); }
    inline StackFrame *fp() const { JS_ASSERT(hasfp()); return seg_->fp(); }

    
    StackSpace &space() const { return *space_; }

    

    





    bool pushInvokeArgs(JSContext *cx, unsigned argc, InvokeArgsGuard *ag,
                        MaybeReportError report = REPORT_ERROR);

    StackFrame *pushInvokeFrame(JSContext *cx, MaybeReportError report,
                                const CallArgs &args, JSFunction *fun,
                                InitialFrameFlags initial, FrameGuard *fg);

    
    bool pushInvokeFrame(JSContext *cx, const CallArgs &args,
                         InitialFrameFlags initial, InvokeFrameGuard *ifg);

    
    bool pushExecuteFrame(JSContext *cx, HandleScript script, const Value &thisv,
                          HandleObject scopeChain, ExecuteType type,
                          AbstractFramePtr evalInFrame, ExecuteFrameGuard *efg);

    





    bool pushGeneratorFrame(JSContext *cx, JSGenerator *gen, GeneratorFrameGuard *gfg);

    




    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         HandleFunction callee, HandleScript script,
                         InitialFrameFlags initial,
                         MaybeReportError report = REPORT_ERROR);
    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         HandleFunction callee, HandleScript script,
                         InitialFrameFlags initial, Value **stackLimit);
    void popInlineFrame(FrameRegs &regs);

    





    enum MaybeAllowCrossCompartment {
        DONT_ALLOW_CROSS_COMPARTMENT = false,
        ALLOW_CROSS_COMPARTMENT = true
    };
    inline JSScript *currentScript(jsbytecode **pc = NULL,
                                   MaybeAllowCrossCompartment = DONT_ALLOW_CROSS_COMPARTMENT) const;

    
    inline HandleObject currentScriptedScopeChain() const;

    bool saveFrameChain();
    void restoreFrameChain();

    



    inline void repointRegs(FrameRegs *regs) { JS_ASSERT(hasfp()); seg_->repointRegs(regs); }
};



class InvokeArgsGuard : public JS::CallArgs
{
    friend class ContextStack;
    ContextStack *stack_;
    Value *prevInvokeArgsEnd_;
    bool pushedSeg_;
    void setPushed(ContextStack &stack) { JS_ASSERT(!pushed()); stack_ = &stack; }
  public:
    InvokeArgsGuard() : CallArgs(), stack_(NULL), prevInvokeArgsEnd_(NULL), pushedSeg_(false) {}
    ~InvokeArgsGuard() { if (pushed()) stack_->popInvokeArgs(*this); }
    bool pushed() const { return !!stack_; }
    void pop() { stack_->popInvokeArgs(*this); stack_ = NULL; }
};

class FrameGuard
{
  protected:
    friend class ContextStack;
    ContextStack *stack_;
    bool pushedSeg_;
    FrameRegs regs_;
    FrameRegs *prevRegs_;
    void setPushed(ContextStack &stack) { stack_ = &stack; }
  public:
    FrameGuard() : stack_(NULL), pushedSeg_(false) {}
    ~FrameGuard() { if (pushed()) stack_->popFrame(*this); }
    bool pushed() const { return !!stack_; }
    void pop() { stack_->popFrame(*this); stack_ = NULL; }

    StackFrame *fp() const { return regs_.fp(); }
};

class InvokeFrameGuard : public FrameGuard
{};

class ExecuteFrameGuard : public FrameGuard
{};

class DummyFrameGuard : public FrameGuard
{};

class GeneratorFrameGuard : public FrameGuard
{
    friend class ContextStack;
    JSGenerator *gen_;
    Value *stackvp_;
  public:
    ~GeneratorFrameGuard() { if (pushed()) stack_->popGeneratorFrame(*this); }
};

template <>
struct DefaultHasher<AbstractFramePtr> {
    typedef AbstractFramePtr Lookup;

    static js::HashNumber hash(const Lookup &key) {
        return size_t(key.raw());
    }

    static bool match(const AbstractFramePtr &k, const Lookup &l) {
        return k == l;
    }
};



class InterpreterActivation;

namespace ion {
    class JitActivation;
};










class Activation
{
  protected:
    JSContext *cx_;
    JSCompartment *compartment_;
    Activation *prev_;
    bool active_;

    
    
    
    
    size_t savedFrameChain_;

    enum Kind { Interpreter, Jit };
    Kind kind_;

    inline Activation(JSContext *cx, Kind kind_, bool active = true);
    inline ~Activation();

  public:
    JSContext *cx() const {
        return cx_;
    }
    JSCompartment *compartment() const {
        return compartment_;
    }
    Activation *prev() const {
        return prev_;
    }
    bool isActive() const {
        return active_;
    }
    void setActive(bool active = true) {
        active_ = active;
    }

    bool isInterpreter() const {
        return kind_ == Interpreter;
    }
    bool isJit() const {
        return kind_ == Jit;
    }

    InterpreterActivation *asInterpreter() const {
        JS_ASSERT(isInterpreter());
        return (InterpreterActivation *)this;
    }
    ion::JitActivation *asJit() const {
        JS_ASSERT(isJit());
        return (ion::JitActivation *)this;
    }

    void saveFrameChain() {
        savedFrameChain_++;
    }
    void restoreFrameChain() {
        JS_ASSERT(savedFrameChain_ > 0);
        savedFrameChain_--;
    }
    bool hasSavedFrameChain() const {
        return savedFrameChain_ > 0;
    }

  private:
    Activation(const Activation &other) MOZ_DELETE;
    void operator=(const Activation &other) MOZ_DELETE;
};

class InterpreterFrameIterator;

class InterpreterActivation : public Activation
{
    friend class js::InterpreterFrameIterator;

    StackFrame *const entry_; 
    StackFrame *current_;     
    FrameRegs &regs_;

  public:
    inline InterpreterActivation(JSContext *cx, StackFrame *entry, FrameRegs &regs);
    inline ~InterpreterActivation();

    void pushFrame(StackFrame *frame) {
        JS_ASSERT(frame->script()->compartment() == compartment_);
        current_ = frame;
    }
    void popFrame(StackFrame *frame) {
        JS_ASSERT(current_ == frame);
        JS_ASSERT(current_ != entry_);

        current_ = frame->prev();
        JS_ASSERT(current_);
    }
    StackFrame *current() const {
        JS_ASSERT(current_);
        return current_;
    }
};


class ActivationIterator
{
    uint8_t *jitTop_;

  protected:
    Activation *activation_;

  private:
    void settle();

  public:
    explicit ActivationIterator(JSRuntime *rt);

    ActivationIterator &operator++();

    Activation *activation() const {
        return activation_;
    }
    uint8_t *jitTop() const {
        JS_ASSERT(activation_->isJit());
        return jitTop_;
    }
    bool done() const {
        return activation_ == NULL;
    }
};

namespace ion {


class JitActivation : public Activation
{
    uint8_t *prevIonTop_;
    JSContext *prevIonJSContext_;
    bool firstFrameIsConstructing_;

  public:
    JitActivation(JSContext *cx, bool firstFrameIsConstructing, bool active = true);
    ~JitActivation();

    uint8_t *prevIonTop() const {
        return prevIonTop_;
    }
    JSCompartment *compartment() const {
        return compartment_;
    }
    bool firstFrameIsConstructing() const {
        return firstFrameIsConstructing_;
    }
};


class JitActivationIterator : public ActivationIterator
{
    void settle() {
        while (!done() && !activation_->isJit())
            ActivationIterator::operator++();
    }

  public:
    explicit JitActivationIterator(JSRuntime *rt)
      : ActivationIterator(rt)
    {
        settle();
    }

    JitActivationIterator &operator++() {
        ActivationIterator::operator++();
        settle();
        return *this;
    }

    
    void jitStackRange(uintptr_t *&min, uintptr_t *&end);
};

} 


class InterpreterFrameIterator
{
    InterpreterActivation *activation_;
    StackFrame *fp_;
    jsbytecode *pc_;

  public:
    explicit InterpreterFrameIterator(InterpreterActivation *activation)
      : activation_(activation),
        fp_(NULL),
        pc_(NULL)
    {
        if (activation) {
            fp_ = activation->current();
            pc_ = activation->regs_.pc;
        }
    }

    StackFrame *frame() const {
        JS_ASSERT(!done());
        return fp_;
    }
    jsbytecode *pc() const {
        JS_ASSERT(!done());
        return pc_;
    }

    InterpreterFrameIterator &operator++();

    bool done() const {
        return fp_ == NULL;
    }
};



















class ScriptFrameIter
{
  public:
    enum SavedOption { STOP_AT_SAVED, GO_THROUGH_SAVED };
    enum ContextOption { CURRENT_CONTEXT, ALL_CONTEXTS };
    enum State { DONE, SCRIPTED, JIT };

    



    struct Data
    {
        PerThreadData *perThread_;
        JSContext    *cx_;
        SavedOption  savedOption_;
        ContextOption contextOption_;

        State        state_;

        jsbytecode   *pc_;

        InterpreterFrameIterator interpFrames_;
        ActivationIterator activations_;

#ifdef JS_ION
        ion::IonFrameIterator ionFrames_;
#endif

        Data(JSContext *cx, PerThreadData *perThread, SavedOption savedOption,
             ContextOption contextOption);
        Data(const Data &other);
    };

    friend class ContextStack;
    friend class ::JSBrokenFrameIterator;
  private:
    Data data_;
#ifdef JS_ION
    ion::InlineFrameIterator ionInlineFrames_;
#endif

    void popActivation();
    void popInterpreterFrame();
#ifdef JS_ION
    void nextJitFrame();
    void popJitFrame();
#endif
    void settleOnActivation();

  public:
    ScriptFrameIter(JSContext *cx, SavedOption = STOP_AT_SAVED);
    ScriptFrameIter(JSContext *cx, ContextOption, SavedOption);
    ScriptFrameIter(const ScriptFrameIter &iter);
    ScriptFrameIter(const Data &data);

    bool done() const { return data_.state_ == DONE; }
    ScriptFrameIter &operator++();

    Data *copyData() const;

    JSCompartment *compartment() const;

    JSScript *script() const {
        JS_ASSERT(!done());
        if (data_.state_ == SCRIPTED)
            return interpFrame()->script();
#ifdef JS_ION
        JS_ASSERT(data_.state_ == JIT);
        if (data_.ionFrames_.isOptimizedJS())
            return ionInlineFrames_.script();
        return data_.ionFrames_.script();
#else
        return NULL;
#endif
    }
    bool isJit() const {
        JS_ASSERT(!done());
        return data_.state_ == JIT;
    }

    bool isIon() const {
#ifdef JS_ION
        return isJit() && data_.ionFrames_.isOptimizedJS();
#else
        return false;
#endif
    }

    bool isBaseline() const {
#ifdef JS_ION
        return isJit() && data_.ionFrames_.isBaselineJS();
#else
        return false;
#endif
    }

    bool isFunctionFrame() const;
    bool isGlobalFrame() const;
    bool isEvalFrame() const;
    bool isNonEvalFunctionFrame() const;
    bool isGeneratorFrame() const;
    bool isConstructing() const;

    bool hasArgs() const { return isNonEvalFunctionFrame(); }

    AbstractFramePtr abstractFramePtr() const;

    





    StackFrame *interpFrame() const {
        JS_ASSERT(data_.state_ == SCRIPTED);
        return data_.interpFrames_.frame();
    }

    Activation *activation() const { return data_.activations_.activation(); }

    jsbytecode *pc() const { JS_ASSERT(!done()); return data_.pc_; }
    void        updatePcQuadratic();
    JSFunction *callee() const;
    Value       calleev() const;
    unsigned    numActualArgs() const;
    unsigned    numFormalArgs() const { return script()->function()->nargs; }
    Value       unaliasedActual(unsigned i, MaybeCheckAliasing = CHECK_ALIASING) const;

    JSObject   *scopeChain() const;
    CallObject &callObj() const;

    bool        hasArgsObj() const;
    ArgumentsObject &argsObj() const;

    
    bool        computeThis() const;
    Value       thisv() const;

    Value       returnValue() const;
    void        setReturnValue(const Value &v);

    JSFunction *maybeCallee() const {
        return isFunctionFrame() ? callee() : NULL;
    }

    
    size_t      numFrameSlots() const;
    Value       frameSlotValue(size_t index) const;

    template <class Op>
    inline void ionForEachCanonicalActualArg(JSContext *cx, Op op);
};


class NonBuiltinScriptFrameIter : public ScriptFrameIter
{
    void settle() {
        while (!done() && script()->selfHosted)
            ScriptFrameIter::operator++();
    }

  public:
    NonBuiltinScriptFrameIter(JSContext *cx, ScriptFrameIter::SavedOption opt = ScriptFrameIter::STOP_AT_SAVED)
      : ScriptFrameIter(cx, opt) { settle(); }

    NonBuiltinScriptFrameIter(const ScriptFrameIter::Data &data)
      : ScriptFrameIter(data)
    {}

    NonBuiltinScriptFrameIter &operator++() { ScriptFrameIter::operator++(); settle(); return *this; }
};





class AllFramesIter : public ScriptFrameIter
{
  public:
    AllFramesIter(JSContext *cx)
      : ScriptFrameIter(cx, ScriptFrameIter::ALL_CONTEXTS, ScriptFrameIter::GO_THROUGH_SAVED)
    {}
};

}  
#endif 
