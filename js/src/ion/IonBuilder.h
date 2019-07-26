





#ifndef jsion_bytecode_analyzer_h__
#define jsion_bytecode_analyzer_h__




#include "MIR.h"
#include "MIRGraph.h"

namespace js {
namespace ion {

class CodeGenerator;
class CallInfo;
class BaselineInspector;

class IonBuilder : public MIRGenerator
{
    enum ControlStatus {
        ControlStatus_Error,
        ControlStatus_Abort,
        ControlStatus_Ended,        
        ControlStatus_Joined,       
        ControlStatus_Jumped,       
        ControlStatus_None          
    };

    struct DeferredEdge : public TempObject
    {
        MBasicBlock *block;
        DeferredEdge *next;

        DeferredEdge(MBasicBlock *block, DeferredEdge *next)
          : block(block), next(next)
        { }
    };

    struct ControlFlowInfo {
        
        uint32_t cfgEntry;

        
        jsbytecode *continuepc;

        ControlFlowInfo(uint32_t cfgEntry, jsbytecode *continuepc)
          : cfgEntry(cfgEntry),
            continuepc(continuepc)
        { }
    };

    
    
    
    
    struct CFGState {
        enum State {
            IF_TRUE,            
            IF_TRUE_EMPTY_ELSE, 
            IF_ELSE_TRUE,       
            IF_ELSE_FALSE,      
            DO_WHILE_LOOP_BODY, 
            DO_WHILE_LOOP_COND, 
            WHILE_LOOP_COND,    
            WHILE_LOOP_BODY,    
            FOR_LOOP_COND,      
            FOR_LOOP_BODY,      
            FOR_LOOP_UPDATE,    
            TABLE_SWITCH,       
            COND_SWITCH_CASE,   
            COND_SWITCH_BODY,   
            AND_OR,             
            LABEL               
        };

        State state;            
        jsbytecode *stopAt;     

        
        union {
            struct {
                MBasicBlock *ifFalse;
                jsbytecode *falseEnd;
                MBasicBlock *ifTrue;    
            } branch;
            struct {
                
                MBasicBlock *entry;

                
                bool osr;

                
                jsbytecode *bodyStart;
                jsbytecode *bodyEnd;

                
                jsbytecode *exitpc;

                
                jsbytecode *continuepc;

                
                MBasicBlock *successor;

                
                DeferredEdge *breaks;
                DeferredEdge *continues;

                
                State initialState;
                jsbytecode *initialPc;
                jsbytecode *initialStopAt;
                jsbytecode *loopHead;

                
                jsbytecode *condpc;
                jsbytecode *updatepc;
                jsbytecode *updateEnd;
            } loop;
            struct {
                
                jsbytecode *exitpc;

                
                DeferredEdge *breaks;

                
                MTableSwitch *ins;

                
                uint32_t currentBlock;

            } tableswitch;
            struct {
                
                FixedList<MBasicBlock *> *bodies;

                
                
                
                uint32_t currentIdx;

                
                jsbytecode *defaultTarget;
                uint32_t defaultIdx;

                
                jsbytecode *exitpc;
                DeferredEdge *breaks;
            } condswitch;
            struct {
                DeferredEdge *breaks;
            } label;
        };

        inline bool isLoop() const {
            switch (state) {
              case DO_WHILE_LOOP_COND:
              case DO_WHILE_LOOP_BODY:
              case WHILE_LOOP_COND:
              case WHILE_LOOP_BODY:
              case FOR_LOOP_COND:
              case FOR_LOOP_BODY:
              case FOR_LOOP_UPDATE:
                return true;
              default:
                return false;
            }
        }

        static CFGState If(jsbytecode *join, MBasicBlock *ifFalse);
        static CFGState IfElse(jsbytecode *trueEnd, jsbytecode *falseEnd, MBasicBlock *ifFalse);
        static CFGState AndOr(jsbytecode *join, MBasicBlock *joinStart);
        static CFGState TableSwitch(jsbytecode *exitpc, MTableSwitch *ins);
        static CFGState CondSwitch(jsbytecode *exitpc, jsbytecode *defaultTarget);
        static CFGState Label(jsbytecode *exitpc);
    };

    static int CmpSuccessors(const void *a, const void *b);

  public:
    IonBuilder(JSContext *cx, TempAllocator *temp, MIRGraph *graph,
               BaselineInspector *inspector, CompileInfo *info, AbstractFramePtr fp,
               size_t inliningDepth = 0, uint32_t loopDepth = 0);

    bool build();
    bool buildInline(IonBuilder *callerBuilder, MResumePoint *callerResumePoint,
                     CallInfo &callInfo);

  private:
    bool traverseBytecode();
    ControlStatus snoopControlFlow(JSOp op);
    bool processIterators();
    bool inspectOpcode(JSOp op);
    uint32_t readIndex(jsbytecode *pc);
    JSAtom *readAtom(jsbytecode *pc);
    bool abort(const char *message, ...);
    void spew(const char *message);

    static bool inliningEnabled() {
        return js_IonOptions.inlining;
    }

    JSFunction *getSingleCallTarget(types::StackTypeSet *calleeTypes);
    bool getPolyCallTargets(types::StackTypeSet *calleeTypes,
                            AutoObjectVector &targets, uint32_t maxTargets);
    bool canInlineTarget(JSFunction *target, CallInfo &callInfo);

    void popCfgStack();
    DeferredEdge *filterDeadDeferredEdges(DeferredEdge *edge);
    bool processDeferredContinues(CFGState &state);
    ControlStatus processControlEnd();
    ControlStatus processCfgStack();
    ControlStatus processCfgEntry(CFGState &state);
    ControlStatus processIfEnd(CFGState &state);
    ControlStatus processIfElseTrueEnd(CFGState &state);
    ControlStatus processIfElseFalseEnd(CFGState &state);
    ControlStatus processDoWhileBodyEnd(CFGState &state);
    ControlStatus processDoWhileCondEnd(CFGState &state);
    ControlStatus processWhileCondEnd(CFGState &state);
    ControlStatus processWhileBodyEnd(CFGState &state);
    ControlStatus processForCondEnd(CFGState &state);
    ControlStatus processForBodyEnd(CFGState &state);
    ControlStatus processForUpdateEnd(CFGState &state);
    ControlStatus processNextTableSwitchCase(CFGState &state);
    ControlStatus processCondSwitchCase(CFGState &state);
    ControlStatus processCondSwitchBody(CFGState &state);
    ControlStatus processSwitchBreak(JSOp op);
    ControlStatus processSwitchEnd(DeferredEdge *breaks, jsbytecode *exitpc);
    ControlStatus processAndOrEnd(CFGState &state);
    ControlStatus processLabelEnd(CFGState &state);
    ControlStatus processReturn(JSOp op);
    ControlStatus processThrow();
    ControlStatus processContinue(JSOp op);
    ControlStatus processBreak(JSOp op, jssrcnote *sn);
    ControlStatus maybeLoop(JSOp op, jssrcnote *sn);
    bool pushLoop(CFGState::State state, jsbytecode *stopAt, MBasicBlock *entry, bool osr,
                  jsbytecode *loopHead, jsbytecode *initialPc,
                  jsbytecode *bodyStart, jsbytecode *bodyEnd, jsbytecode *exitpc,
                  jsbytecode *continuepc = NULL);
    void analyzeNewLoopTypes(MBasicBlock *entry, jsbytecode *start, jsbytecode *end);

    MBasicBlock *addBlock(MBasicBlock *block, uint32_t loopDepth);
    MBasicBlock *newBlock(MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newBlock(MBasicBlock *predecessor, jsbytecode *pc, uint32_t loopDepth);
    MBasicBlock *newBlock(MBasicBlock *predecessor, jsbytecode *pc, MResumePoint *priorResumePoint);
    MBasicBlock *newBlockPopN(MBasicBlock *predecessor, jsbytecode *pc, uint32_t popped);
    MBasicBlock *newBlockAfter(MBasicBlock *at, MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newOsrPreheader(MBasicBlock *header, jsbytecode *loopEntry);
    MBasicBlock *newPendingLoopHeader(MBasicBlock *predecessor, jsbytecode *pc, bool osr);
    MBasicBlock *newBlock(jsbytecode *pc) {
        return newBlock(NULL, pc);
    }
    MBasicBlock *newBlockAfter(MBasicBlock *at, jsbytecode *pc) {
        return newBlockAfter(at, NULL, pc);
    }

    
    
    MBasicBlock *createBreakCatchBlock(DeferredEdge *edge, jsbytecode *pc);

    
    
    ControlStatus processBrokenLoop(CFGState &state);

    
    
    ControlStatus finishLoop(CFGState &state, MBasicBlock *successor);

    
    
    bool addOsrValueTypeBarrier(uint32_t slot, MInstruction **def,
                                MIRType type, types::StackTypeSet *typeSet);
    bool maybeAddOsrTypeBarriers();

    
    
    ControlStatus restartLoop(CFGState state);

    void assertValidLoopHeadOp(jsbytecode *pc);

    ControlStatus forLoop(JSOp op, jssrcnote *sn);
    ControlStatus whileOrForInLoop(jssrcnote *sn);
    ControlStatus doWhileLoop(JSOp op, jssrcnote *sn);
    ControlStatus tableSwitch(JSOp op, jssrcnote *sn);
    ControlStatus condSwitch(JSOp op, jssrcnote *sn);

    
    
    bool resume(MInstruction *ins, jsbytecode *pc, MResumePoint::Mode mode);
    bool resumeAt(MInstruction *ins, jsbytecode *pc);
    bool resumeAfter(MInstruction *ins);
    bool maybeInsertResume();

    bool initParameters();
    void rewriteParameter(uint32_t slotIdx, MDefinition *param, int32_t argIndex);
    void rewriteParameters();
    bool initScopeChain();
    bool initArgumentsObject();
    bool pushConstant(const Value &v);

    
    
    bool pushTypeBarrier(MInstruction *ins, types::StackTypeSet *observed, bool needBarrier);

    JSObject *getSingletonPrototype(JSFunction *target);

    MDefinition *createThisScripted(MDefinition *callee);
    MDefinition *createThisScriptedSingleton(HandleFunction target, MDefinition *callee);
    MDefinition *createThis(HandleFunction target, MDefinition *callee);
    MInstruction *createDeclEnvObject(MDefinition *callee, MDefinition *scopeObj);
    MInstruction *createCallObject(MDefinition *callee, MDefinition *scopeObj);

    MDefinition *walkScopeChain(unsigned hops);

    MInstruction *addConvertElementsToDoubles(MDefinition *elements);
    MInstruction *addBoundsCheck(MDefinition *index, MDefinition *length);
    MInstruction *addShapeGuard(MDefinition *obj, Shape *const shape, BailoutKind bailoutKind);

    JSObject *getNewArrayTemplateObject(uint32_t count);
    MDefinition *convertShiftToMaskForStaticTypedArray(MDefinition *id,
                                                       ArrayBufferView::ViewType viewType);

    bool invalidatedIdempotentCache();

    bool loadSlot(MDefinition *obj, Shape *shape, MIRType rvalType,
                  bool barrier, types::StackTypeSet *types);
    bool storeSlot(MDefinition *obj, Shape *shape, MDefinition *value, bool needsBarrier);

    
    bool getPropTryArgumentsLength(bool *emitted);
    bool getPropTryConstant(bool *emitted, HandleId id, types::StackTypeSet *types);
    bool getPropTryDefiniteSlot(bool *emitted, HandlePropertyName name,
                                bool barrier, types::StackTypeSet *types);
    bool getPropTryCommonGetter(bool *emitted, HandleId id,
                                bool barrier, types::StackTypeSet *types);
    bool getPropTryInlineAccess(bool *emitted, HandlePropertyName name, HandleId id,
                                bool barrier, types::StackTypeSet *types);
    bool getPropTryCache(bool *emitted, HandlePropertyName name, HandleId id,
                         bool barrier, types::StackTypeSet *types);

    
    MInstruction *getTypedArrayLength(MDefinition *obj);
    MInstruction *getTypedArrayElements(MDefinition *obj);

    bool jsop_add(MDefinition *left, MDefinition *right);
    bool jsop_bitnot();
    bool jsop_bitop(JSOp op);
    bool jsop_binary(JSOp op);
    bool jsop_binary(JSOp op, MDefinition *left, MDefinition *right);
    bool jsop_pos();
    bool jsop_neg();
    bool jsop_defvar(uint32_t index);
    bool jsop_deffun(uint32_t index);
    bool jsop_notearg();
    bool jsop_funcall(uint32_t argc);
    bool jsop_funapply(uint32_t argc);
    bool jsop_funapplyarguments(uint32_t argc);
    bool jsop_call(uint32_t argc, bool constructing);
    bool jsop_eval(uint32_t argc);
    bool jsop_ifeq(JSOp op);
    bool jsop_label();
    bool jsop_condswitch();
    bool jsop_andor(JSOp op);
    bool jsop_dup2();
    bool jsop_loophead(jsbytecode *pc);
    bool jsop_compare(JSOp op);
    bool jsop_getgname(HandlePropertyName name);
    bool jsop_setgname(HandlePropertyName name);
    bool jsop_getname(HandlePropertyName name);
    bool jsop_intrinsic(HandlePropertyName name);
    bool jsop_bindname(PropertyName *name);
    bool jsop_getelem();
    bool jsop_getelem_dense();
    bool jsop_getelem_typed(int arrayType);
    bool jsop_getelem_typed_static(bool *psucceeded);
    bool jsop_getelem_string();
    bool jsop_setelem();
    bool jsop_setelem_dense(types::StackTypeSet::DoubleConversion conversion);
    bool jsop_setelem_typed(int arrayType);
    bool jsop_setelem_typed_static(bool *psucceeded);
    bool jsop_length();
    bool jsop_length_fastPath();
    bool jsop_arguments();
    bool jsop_arguments_length();
    bool jsop_arguments_getelem();
    bool jsop_arguments_setelem();
    bool jsop_not();
    bool jsop_getprop(HandlePropertyName name);
    bool jsop_setprop(HandlePropertyName name);
    bool jsop_delprop(HandlePropertyName name);
    bool jsop_newarray(uint32_t count);
    bool jsop_newobject(HandleObject baseObj);
    bool jsop_initelem();
    bool jsop_initelem_array();
    bool jsop_initprop(HandlePropertyName name);
    bool jsop_regexp(RegExpObject *reobj);
    bool jsop_object(JSObject *obj);
    bool jsop_lambda(JSFunction *fun);
    bool jsop_this();
    bool jsop_typeof();
    bool jsop_toid();
    bool jsop_iter(uint8_t flags);
    bool jsop_iternext();
    bool jsop_itermore();
    bool jsop_iterend();
    bool jsop_in();
    bool jsop_in_dense();
    bool jsop_instanceof();
    bool jsop_getaliasedvar(ScopeCoordinate sc);
    bool jsop_setaliasedvar(ScopeCoordinate sc);

    

    enum InliningStatus
    {
        InliningStatus_Error,
        InliningStatus_NotInlined,
        InliningStatus_Inlined
    };

    
    bool canEnterInlinedFunction(JSFunction *target);
    bool makeInliningDecision(JSFunction *target, CallInfo &callInfo);
    uint32_t selectInliningTargets(AutoObjectVector &targets, CallInfo &callInfo, Vector<bool> &choiceSet);

    
    types::StackTypeSet *getInlineReturnTypeSet();
    MIRType getInlineReturnType();

    
    InliningStatus inlineArray(CallInfo &callInfo);
    InliningStatus inlineArrayPopShift(CallInfo &callInfo, MArrayPopShift::Mode mode);
    InliningStatus inlineArrayPush(CallInfo &callInfo);
    InliningStatus inlineArrayConcat(CallInfo &callInfo);

    
    InliningStatus inlineMathAbs(CallInfo &callInfo);
    InliningStatus inlineMathFloor(CallInfo &callInfo);
    InliningStatus inlineMathRound(CallInfo &callInfo);
    InliningStatus inlineMathSqrt(CallInfo &callInfo);
    InliningStatus inlineMathMinMax(CallInfo &callInfo, bool max);
    InliningStatus inlineMathPow(CallInfo &callInfo);
    InliningStatus inlineMathRandom(CallInfo &callInfo);
    InliningStatus inlineMathImul(CallInfo &callInfo);
    InliningStatus inlineMathFunction(CallInfo &callInfo, MMathFunction::Function function);

    
    InliningStatus inlineStringObject(CallInfo &callInfo);
    InliningStatus inlineStrCharCodeAt(CallInfo &callInfo);
    InliningStatus inlineStrFromCharCode(CallInfo &callInfo);
    InliningStatus inlineStrCharAt(CallInfo &callInfo);

    
    InliningStatus inlineRegExpTest(CallInfo &callInfo);

    
    InliningStatus inlineUnsafeSetElement(CallInfo &callInfo);
    bool inlineUnsafeSetDenseArrayElement(CallInfo &callInfo, uint32_t base);
    bool inlineUnsafeSetTypedArrayElement(CallInfo &callInfo, uint32_t base, int arrayType);
    InliningStatus inlineShouldForceSequentialOrInParallelSection(CallInfo &callInfo);
    InliningStatus inlineNewDenseArray(CallInfo &callInfo);
    InliningStatus inlineNewDenseArrayForSequentialExecution(CallInfo &callInfo);
    InliningStatus inlineNewDenseArrayForParallelExecution(CallInfo &callInfo);
    InliningStatus inlineNewParallelArray(CallInfo &callInfo);
    InliningStatus inlineParallelArray(CallInfo &callInfo);
    InliningStatus inlineParallelArrayTail(CallInfo &callInfo,
                                           HandleFunction target,
                                           MDefinition *ctor,
                                           types::StackTypeSet *ctorTypes,
                                           uint32_t discards);

    InliningStatus inlineThrowError(CallInfo &callInfo);
    InliningStatus inlineIsCallable(CallInfo &callInfo);
    InliningStatus inlineToObject(CallInfo &callInfo);
    InliningStatus inlineDump(CallInfo &callInfo);

    
    InliningStatus inlineNativeCall(CallInfo &callInfo, JSNative native);
    bool inlineScriptedCall(CallInfo &callInfo, JSFunction *target);
    InliningStatus inlineSingleCall(CallInfo &callInfo, JSFunction *target);

    
    InliningStatus inlineCallsite(AutoObjectVector &targets, AutoObjectVector &originals,
                                  CallInfo &callInfo);
    bool inlineCalls(CallInfo &callInfo, AutoObjectVector &targets, AutoObjectVector &originals,
                     Vector<bool> &choiceSet, MGetPropertyCache *maybeCache);

    
    bool inlineGenericFallback(JSFunction *target, CallInfo &callInfo, MBasicBlock *dispatchBlock,
                               bool clonedAtCallsite);
    bool inlineTypeObjectFallback(CallInfo &callInfo, MBasicBlock *dispatchBlock,
                                  MTypeObjectDispatch *dispatch, MGetPropertyCache *cache,
                                  MBasicBlock **fallbackTarget);

    bool anyFunctionIsCloneAtCallsite(types::StackTypeSet *funTypes);
    MDefinition *makeCallsiteClone(HandleFunction target, MDefinition *fun);
    MCall *makeCallHelper(HandleFunction target, CallInfo &callInfo, bool cloneAtCallsite);
    bool makeCall(HandleFunction target, CallInfo &callInfo, bool cloneAtCallsite);

    MDefinition *patchInlinedReturn(CallInfo &callInfo, MBasicBlock *exit, MBasicBlock *bottom);
    MDefinition *patchInlinedReturns(CallInfo &callInfo, MIRGraphExits &exits, MBasicBlock *bottom);

    inline bool TestCommonPropFunc(JSContext *cx, types::StackTypeSet *types,
                                   HandleId id, JSFunction **funcp,
                                   bool isGetter, bool *isDOM,
                                   MDefinition **guardOut);

    bool annotateGetPropertyCache(JSContext *cx, MDefinition *obj, MGetPropertyCache *getPropCache,
                                  types::StackTypeSet *objTypes, types::StackTypeSet *pushedTypes);

    MGetPropertyCache *getInlineableGetPropertyCache(CallInfo &callInfo);

    MPolyInlineDispatch *
    makePolyInlineDispatch(JSContext *cx, CallInfo &callInfo,
                           MGetPropertyCache *getPropCache, MBasicBlock *bottom,
                           Vector<MDefinition *, 8, IonAllocPolicy> &retvalDefns);

    types::StackTypeSet *cloneTypeSet(types::StackTypeSet *types);

    
    
    

    void setCurrentAndSpecializePhis(MBasicBlock *block) {
        if (block)
            block->specializePhis();
        setCurrent(block);
    }

    void setCurrent(MBasicBlock *block) {
        current = block;
    }

    
    HeapPtrScript script_;

    
    
    
    
    CodeGenerator *backgroundCodegen_;

  public:
    
    types::RecompileInfo const recompileInfo;

    void clearForBackEnd();

    JSScript *script() const { return script_.get(); }

    CodeGenerator *backgroundCodegen() const { return backgroundCodegen_; }
    void setBackgroundCodegen(CodeGenerator *codegen) { backgroundCodegen_ = codegen; }

    AbortReason abortReason() { return abortReason_; }

  private:
    JSContext *cx;
    AbstractFramePtr fp;
    AbortReason abortReason_;

    jsbytecode *pc;
    MBasicBlock *current;
    uint32_t loopDepth_;

    
    MResumePoint *callerResumePoint_;
    jsbytecode *callerPC() {
        return callerResumePoint_ ? callerResumePoint_->pc() : NULL;
    }
    IonBuilder *callerBuilder_;

    Vector<CFGState, 8, IonAllocPolicy> cfgStack_;
    Vector<ControlFlowInfo, 4, IonAllocPolicy> loops_;
    Vector<ControlFlowInfo, 0, IonAllocPolicy> switches_;
    Vector<ControlFlowInfo, 2, IonAllocPolicy> labels_;
    Vector<MInstruction *, 2, IonAllocPolicy> iterators_;
    BaselineInspector *inspector;

    size_t inliningDepth_;
    Vector<MDefinition *, 0, IonAllocPolicy> inlinedArguments_;

    
    
    static const size_t MAX_LOOP_RESTARTS = 20;
    size_t numLoopRestarts_;

    
    
    bool failedBoundsCheck_;

    
    
    bool failedShapeGuard_;

    
    bool nonStringIteration_;

    
    
    MInstruction *lazyArguments_;
};

class CallInfo
{
    MDefinition *fun_;
    MDefinition *thisArg_;
    Vector<MDefinition *> args_;

    bool constructing_;

  public:
    CallInfo(JSContext *cx, bool constructing)
      : fun_(NULL),
        thisArg_(NULL),
        args_(cx),
        constructing_(constructing)
    { }

    bool init(CallInfo &callInfo) {
        JS_ASSERT(constructing_ == callInfo.constructing());

        fun_ = callInfo.fun();
        thisArg_ = callInfo.thisArg();

        if (!args_.append(callInfo.argv().begin(), callInfo.argv().end()))
            return false;

        return true;
    }

    bool init(MBasicBlock *current, uint32_t argc) {
        JS_ASSERT(args_.length() == 0);

        
        if (!args_.reserve(argc))
            return false;
        for (int32_t i = argc; i > 0; i--)
            args_.infallibleAppend(current->peek(-i));
        current->popn(argc);

        
        setThis(current->pop());
        setFun(current->pop());

        return true;
    }

    void popFormals(MBasicBlock *current) {
        current->popn(numFormals());
    }

    void pushFormals(MBasicBlock *current) {
        current->push(fun());
        current->push(thisArg());

        for (uint32_t i = 0; i < argc(); i++)
            current->push(getArg(i));
    }

    uint32_t argc() const {
        return args_.length();
    }
    uint32_t numFormals() const {
        return argc() + 2;
    }

    void setArgs(Vector<MDefinition *> *args) {
        JS_ASSERT(args_.length() == 0);
        args_.append(args->begin(), args->end());
    }

    Vector<MDefinition *> &argv() {
        return args_;
    }

    MDefinition *getArg(uint32_t i) {
        JS_ASSERT(i < argc());
        return args_[i];
    }

    void setArg(uint32_t i, MDefinition *def) {
        JS_ASSERT(i < argc());
        args_[i] = def;
    }

    MDefinition *thisArg() {
        JS_ASSERT(thisArg_);
        return thisArg_;
    }

    void setThis(MDefinition *thisArg) {
        thisArg_ = thisArg;
    }

    bool constructing() {
        return constructing_;
    }

    void wrapArgs(MBasicBlock *current) {
        thisArg_ = wrap(current, thisArg_);
        for (uint32_t i = 0; i < argc(); i++)
            args_[i] = wrap(current, args_[i]);
    }

    void unwrapArgs() {
        thisArg_ = unwrap(thisArg_);
        for (uint32_t i = 0; i < argc(); i++)
            args_[i] = unwrap(args_[i]);
    }

    MDefinition *fun() const {
        JS_ASSERT(fun_);
        return fun_;
    }

    void setFun(MDefinition *fun) {
        JS_ASSERT(!fun->isPassArg());
        fun_ = fun;
    }

    bool isWrapped() {
        bool wrapped = thisArg()->isPassArg();

#if DEBUG
        for (uint32_t i = 0; i < argc(); i++)
            JS_ASSERT(args_[i]->isPassArg() == wrapped);
#endif

        return wrapped;
    }

  private:
    static MDefinition *unwrap(MDefinition *arg) {
        JS_ASSERT(arg->isPassArg());
        MPassArg *passArg = arg->toPassArg();
        MBasicBlock *block = passArg->block();
        MDefinition *wrapped = passArg->getArgument();
        wrapped->setFoldedUnchecked();
        passArg->replaceAllUsesWith(wrapped);
        block->discard(passArg);
        return wrapped;
    }
    static MDefinition *wrap(MBasicBlock *current, MDefinition *arg) {
        JS_ASSERT(!arg->isPassArg());
        MPassArg *passArg = MPassArg::New(arg);
        current->add(passArg);
        return passArg;
    }
};

bool TypeSetIncludes(types::TypeSet *types, MIRType input, types::TypeSet *inputTypes);

} 
} 

#endif 
