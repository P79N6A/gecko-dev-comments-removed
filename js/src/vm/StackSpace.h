







































#ifndef StackSpace_h__
#define StackSpace_h__

#include "jsprvtd.h"

namespace js {


class FrameGuard;
class DummyFrameGuard;
class ExecuteFrameGuard;
class GeneratorFrameGuard;


enum InitialFrameFlags {
    INITIAL_NONE           =          0,
    INITIAL_CONSTRUCT      =       0x80, 
    INITIAL_LOWERED        =   0x200000  
};

enum ExecuteType {
    EXECUTE_GLOBAL         =        0x1, 
    EXECUTE_DIRECT_EVAL    =        0x8, 
    EXECUTE_INDIRECT_EVAL  =        0x9, 
    EXECUTE_DEBUG          =       0x18  
};



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

    







    static const size_t CX_COMPARTMENT = 0xc;

    inline bool ensureSpace(JSContext *cx, MaybeReportError report,
                            Value *from, ptrdiff_t nvals,
                            JSCompartment *dest = (JSCompartment *)CX_COMPARTMENT) const;
    JS_FRIEND_API(bool) ensureSpaceSlow(JSContext *cx, MaybeReportError report,
                                        Value *from, ptrdiff_t nvals,
                                        JSCompartment *dest) const;

    StackSegment &findContainingSegment(const StackFrame *target) const;

  public:
    StackSpace();
    bool init();
    ~StackSpace();

    










    static const uintN ARGS_LENGTH_MAX = CAPACITY_VALS - (2 * BUFFER_VALS);

    
    inline Value *firstUnused() const;

    StackSegment &containingSegment(const StackFrame *target) const;

    







    static const size_t STACK_JIT_EXTRA = ( 8 + 18) * 10;

    













    inline Value *getStackLimit(JSContext *cx, MaybeReportError report);
    bool tryBumpLimit(JSContext *cx, Value *from, uintN nvals, Value **limit);

    
    void mark(JSTracer *trc);

    
    JS_FRIEND_API(size_t) committedSize();
};



class ContextStack
{
    StackSegment *seg_;
    StackSpace *space_;
    JSContext *cx_;

    







    bool onTop() const;

#ifdef DEBUG
    void assertSpaceInSync() const;
#else
    void assertSpaceInSync() const {}
#endif

    
    StackSegment *pushSegment(JSContext *cx);
    enum MaybeExtend { CAN_EXTEND = true, CANT_EXTEND = false };
    Value *ensureOnTop(JSContext *cx, MaybeReportError report, uintN nvars,
                       MaybeExtend extend, bool *pushedSeg,
                       JSCompartment *dest = (JSCompartment *)StackSpace::CX_COMPARTMENT);

    inline StackFrame *
    getCallFrame(JSContext *cx, MaybeReportError report, const CallArgs &args,
                 JSFunction *fun, JSScript *script,  uint32 *pflags) const;

    
    void popSegment();
    friend class InvokeArgsGuard;
    void popInvokeArgs(const InvokeArgsGuard &iag);
    friend class FrameGuard;
    void popFrame(const FrameGuard &fg);
    friend class GeneratorFrameGuard;
    void popGeneratorFrame(const GeneratorFrameGuard &gfg);

    friend class StackIter;

  public:
    ContextStack(JSContext *cx);
    ~ContextStack();

    

    



    bool empty() const                { return !seg_; }

    





    inline bool hasfp() const;

    



    inline FrameRegs *maybeRegs() const;
    inline StackFrame *maybefp() const;

    
    inline FrameRegs &regs() const;
    inline StackFrame *fp() const;

    
    StackSpace &space() const    { assertSpaceInSync(); return *space_; }

    
    bool containsSlow(const StackFrame *target) const;

    

    





    bool pushInvokeArgs(JSContext *cx, uintN argc, InvokeArgsGuard *ag);

    
    bool pushInvokeFrame(JSContext *cx, const CallArgs &args,
                         InitialFrameFlags initial, InvokeFrameGuard *ifg);

    
    bool pushExecuteFrame(JSContext *cx, JSScript *script, const Value &thisv,
                          JSObject &scopeChain, ExecuteType type,
                          StackFrame *evalInFrame, ExecuteFrameGuard *efg);

    





    bool pushGeneratorFrame(JSContext *cx, JSGenerator *gen, GeneratorFrameGuard *gfg);

    








    bool pushDummyFrame(JSContext *cx, JSCompartment *dest, JSObject &scopeChain, DummyFrameGuard *dfg);

    




    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         JSFunction &callee, JSScript *script,
                         InitialFrameFlags initial);
    bool pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                         JSFunction &callee, JSScript *script,
                         InitialFrameFlags initial, Value **stackLimit);
    void popInlineFrame(FrameRegs &regs);

    
    void popFrameAfterOverflow();

    
    inline JSScript *currentScript(jsbytecode **pc = NULL) const;

    
    inline JSObject *currentScriptedScopeChain() const;

    







    StackFrame *getFixupFrame(JSContext *cx, MaybeReportError report,
                              const CallArgs &args, JSFunction *fun, JSScript *script,
                              void *ncode, InitialFrameFlags initial, Value **stackLimit);

    bool saveFrameChain();
    void restoreFrameChain();

    



    inline void repointRegs(FrameRegs *regs);

    

    




    void threadReset();

    

    static size_t offsetOfSeg() { return offsetof(ContextStack, seg_); }
};

} 

#endif 
