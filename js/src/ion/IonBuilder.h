






#ifndef jsion_bytecode_analyzer_h__
#define jsion_bytecode_analyzer_h__




#include "MIR.h"
#include "MIRGraph.h"

namespace js {
namespace ion {

class CodeGenerator;
class CallInfo;

class IonBuilder : public MIRGenerator
{
    enum ControlStatus {
        ControlStatus_Error,
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
            AND_OR              
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

                
                jsbytecode *bodyStart;
                jsbytecode *bodyEnd;

                
                jsbytecode *exitpc;

                
                MBasicBlock *successor;

                
                DeferredEdge *breaks;
                DeferredEdge *continues;

                
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
    };

    static int CmpSuccessors(const void *a, const void *b);

  public:
    IonBuilder(JSContext *cx, TempAllocator *temp, MIRGraph *graph,
               TypeOracle *oracle, CompileInfo *info, size_t inliningDepth = 0, uint32_t loopDepth = 0);

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
    bool canInlineTarget(JSFunction *target);

    void popCfgStack();
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
    ControlStatus processSwitchBreak(JSOp op, jssrcnote *sn);
    ControlStatus processSwitchEnd(DeferredEdge *breaks, jsbytecode *exitpc);
    ControlStatus processAndOrEnd(CFGState &state);
    ControlStatus processReturn(JSOp op);
    ControlStatus processThrow();
    ControlStatus processContinue(JSOp op, jssrcnote *sn);
    ControlStatus processBreak(JSOp op, jssrcnote *sn);
    ControlStatus maybeLoop(JSOp op, jssrcnote *sn);
    bool pushLoop(CFGState::State state, jsbytecode *stopAt, MBasicBlock *entry,
                  jsbytecode *bodyStart, jsbytecode *bodyEnd, jsbytecode *exitpc,
                  jsbytecode *continuepc = NULL);

    MBasicBlock *addBlock(MBasicBlock *block, uint32_t loopDepth);
    MBasicBlock *newBlock(MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newBlock(MBasicBlock *predecessor, jsbytecode *pc, uint32_t loopDepth);
    MBasicBlock *newBlock(MBasicBlock *predecessor, jsbytecode *pc, MResumePoint *priorResumePoint);
    MBasicBlock *newBlockPopN(MBasicBlock *predecessor, jsbytecode *pc, uint32_t popped);
    MBasicBlock *newBlockAfter(MBasicBlock *at, MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newOsrPreheader(MBasicBlock *header, jsbytecode *loopEntry);
    MBasicBlock *newPendingLoopHeader(MBasicBlock *predecessor, jsbytecode *pc);
    MBasicBlock *newBlock(jsbytecode *pc) {
        return newBlock(NULL, pc);
    }
    MBasicBlock *newBlockAfter(MBasicBlock *at, jsbytecode *pc) {
        return newBlockAfter(at, NULL, pc);
    }

    
    
    MBasicBlock *createBreakCatchBlock(DeferredEdge *edge, jsbytecode *pc);

    
    
    ControlStatus processBrokenLoop(CFGState &state);

    
    
    ControlStatus finishLoop(CFGState &state, MBasicBlock *successor);

    void assertValidLoopHeadOp(jsbytecode *pc);

    ControlStatus forLoop(JSOp op, jssrcnote *sn);
    ControlStatus whileOrForInLoop(JSOp op, jssrcnote *sn);
    ControlStatus doWhileLoop(JSOp op, jssrcnote *sn);
    ControlStatus tableSwitch(JSOp op, jssrcnote *sn);
    ControlStatus condSwitch(JSOp op, jssrcnote *sn);

    
    
    bool resume(MInstruction *ins, jsbytecode *pc, MResumePoint::Mode mode);
    bool resumeAt(MInstruction *ins, jsbytecode *pc);
    bool resumeAfter(MInstruction *ins);
    bool maybeInsertResume();

    void insertRecompileCheck();

    bool initParameters();
    void rewriteParameters();
    bool initScopeChain();
    bool pushConstant(const Value &v);
    bool pushTypeBarrier(MInstruction *ins, types::StackTypeSet *actual, types::StackTypeSet *observed);
    void monitorResult(MInstruction *ins, types::TypeSet *barrier, types::StackTypeSet *types);

    JSObject *getSingletonPrototype(JSFunction *target);

    MDefinition *createThisScripted(MDefinition *callee);
    MDefinition *createThisScriptedSingleton(HandleFunction target, MDefinition *callee);
    MDefinition *createThis(HandleFunction target, MDefinition *callee);
    MInstruction *createDeclEnvObject(MDefinition *callee, MDefinition *scopeObj);
    MInstruction *createCallObject(MDefinition *callee, MDefinition *scopeObj);

    MDefinition *walkScopeChain(unsigned hops);

    MInstruction *addConvertElementsToDoubles(MDefinition *elements);
    MInstruction *addBoundsCheck(MDefinition *index, MDefinition *length);
    MInstruction *addShapeGuard(MDefinition *obj, const UnrootedShape shape, BailoutKind bailoutKind);

    JSObject *getNewArrayTemplateObject(uint32_t count);

    bool invalidatedIdempotentCache();

    bool loadSlot(MDefinition *obj, HandleShape shape, MIRType rvalType);
    bool storeSlot(MDefinition *obj, UnrootedShape shape, MDefinition *value, bool needsBarrier);

    
    bool getPropTryArgumentsLength(bool *emitted);
    bool getPropTryConstant(bool *emitted, HandleId id, types::StackTypeSet *barrier,
                            types::StackTypeSet *types, TypeOracle::UnaryTypes unaryTypes);
    bool getPropTryDefiniteSlot(bool *emitted, HandlePropertyName name,
                            types::StackTypeSet *barrier, types::StackTypeSet *types,
                            TypeOracle::Unary unary, TypeOracle::UnaryTypes unaryTypes);
    bool getPropTryCommonGetter(bool *emitted, HandleId id, types::StackTypeSet *barrier,
                                types::StackTypeSet *types, TypeOracle::UnaryTypes unaryTypes);
    bool getPropTryMonomorphic(bool *emitted, HandleId id, types::StackTypeSet *barrier,
                               TypeOracle::Unary unary, TypeOracle::UnaryTypes unaryTypes);
    bool getPropTryPolymorphic(bool *emitted, HandlePropertyName name, HandleId id,
                               types::StackTypeSet *barrier, types::StackTypeSet *types,
                               TypeOracle::Unary unary, TypeOracle::UnaryTypes unaryTypes);

    
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
    bool jsop_ifeq(JSOp op);
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
    bool jsop_getelem_string();
    bool jsop_setelem();
    bool jsop_setelem_dense();
    bool jsop_setelem_typed(int arrayType);
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

    
    types::StackTypeSet *getInlineReturnTypeSet();
    MIRType getInlineReturnType();
    types::StackTypeSet *getInlineThisTypeSet(CallInfo &callInfo);
    MIRType getInlineThisType(CallInfo &callInfo);
    types::StackTypeSet *getInlineArgTypeSet(CallInfo &callInfo, uint32_t arg);
    MIRType getInlineArgType(CallInfo &callInfo, uint32_t arg);

    
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
    InliningStatus inlineForceSequentialOrInParallelSection(CallInfo &callInfo);
    InliningStatus inlineNewDenseArray(CallInfo &callInfo);
    InliningStatus inlineNewDenseArrayForSequentialExecution(CallInfo &callInfo);
    InliningStatus inlineNewDenseArrayForParallelExecution(CallInfo &callInfo);

    InliningStatus inlineThrowError(CallInfo &callInfo);
    InliningStatus inlineDump(CallInfo &callInfo);

    InliningStatus inlineNativeCall(CallInfo &callInfo, JSNative native);

    
    bool jsop_call_inline(HandleFunction callee, CallInfo &callInfo, MBasicBlock *bottom,
                          Vector<MDefinition *, 8, IonAllocPolicy> &retvalDefns);
    bool inlineScriptedCalls(AutoObjectVector &targets, AutoObjectVector &originals,
                             CallInfo &callInfo);
    bool inlineScriptedCall(HandleFunction target, CallInfo &callInfo);
    bool makeInliningDecision(AutoObjectVector &targets, uint32_t argc);

    bool anyFunctionIsCloneAtCallsite(types::StackTypeSet *funTypes);
    MDefinition *makeCallsiteClone(HandleFunction target, MDefinition *fun);
    MCall *makeCallHelper(HandleFunction target, CallInfo &callInfo,
                          types::StackTypeSet *calleeTypes, bool cloneAtCallsite);
    bool makeCallBarrier(HandleFunction target,  CallInfo &callInfo,
                         types::StackTypeSet *calleeTypes, bool cloneAtCallsite);
    bool makeCall(HandleFunction target, CallInfo &callInfo, 
                  types::StackTypeSet *calleeTypes, bool cloneAtCallsite);

    MDefinition *patchInlinedReturns(CallInfo &callInfo, MIRGraphExits &exits, MBasicBlock *bottom);

    inline bool TestCommonPropFunc(JSContext *cx, types::StackTypeSet *types,
                                   HandleId id, JSFunction **funcp,
                                   bool isGetter, bool *isDOM,
                                   MDefinition **guardOut);

    bool annotateGetPropertyCache(JSContext *cx, MDefinition *obj, MGetPropertyCache *getPropCache,
                                  types::StackTypeSet *objTypes, types::StackTypeSet *pushedTypes);

    MGetPropertyCache *checkInlineableGetPropertyCache(uint32_t argc);

    MPolyInlineDispatch *
    makePolyInlineDispatch(JSContext *cx, CallInfo &callInfo,
                           MGetPropertyCache *getPropCache, MBasicBlock *bottom,
                           Vector<MDefinition *, 8, IonAllocPolicy> &retvalDefns);

    const types::StackTypeSet *cloneTypeSet(const types::StackTypeSet *types);

    
    HeapPtrScript script_;

    
    
    
    CodeGenerator *backgroundCodegen_;

  public:
    
    types::RecompileInfo const recompileInfo;

    void clearForBackEnd();

    UnrootedScript script() const { return script_.get(); }

    CodeGenerator *backgroundCodegen() const { return backgroundCodegen_; }
    void setBackgroundCodegen(CodeGenerator *codegen) { backgroundCodegen_ = codegen; }

    AbortReason abortReason() { return abortReason_; }

  private:
    JSContext *cx;
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
    Vector<MInstruction *, 2, IonAllocPolicy> iterators_;
    TypeOracle *oracle;

    size_t inliningDepth;
    Vector<MDefinition *, 0, IonAllocPolicy> inlinedArguments_;

    
    
    bool failedBoundsCheck_;

    
    
    bool failedShapeGuard_;

    
    
    MInstruction *lazyArguments_;
};

class CallInfo
{
    types::StackTypeSet *barrier_;
    types::StackTypeSet *types_;

    MDefinition *fun_;
    MDefinition *thisArg_;
    Vector<MDefinition *> args_;

    bool constructing_;

  public:
    CallInfo(JSContext *cx, bool constructing)
      : barrier_(NULL),
        types_(NULL),
        fun_(NULL),
        thisArg_(NULL),
        args_(cx),
        constructing_(constructing)
    { }

    CallInfo(JSContext *cx, bool constructing,
             types::StackTypeSet *types, types::StackTypeSet *barrier)
      : barrier_(barrier),
        types_(types),
        fun_(NULL),
        thisArg_(NULL),
        args_(cx),
        constructing_(constructing)
    { }

    bool init(CallInfo &callInfo) {
        JS_ASSERT(constructing_ == callInfo.constructing());

        fun_ = callInfo.fun();
        thisArg_ = callInfo.thisArg();

        if (!args_.append(callInfo.argv()->begin(), callInfo.argv()->end()))
            return false;

        if (callInfo.hasTypeInfo())
            setTypeInfo(callInfo.types(), callInfo.barrier());

        return true;
    }

    bool init(MBasicBlock *current, uint32_t argc) {
        JS_ASSERT(args_.length() == 0);

        
        if (!args_.reserve(argc))
            return false;
        for (int32_t i = argc; i > 0; i--) {
            if (!args_.append(current->peek(-i)))
                return false;
        }
        current->popn(argc);

        
        setThis(current->pop());
        setFun(current->pop());

        return true;
    }

    void popFormals(MBasicBlock *current) {
        current->popn(argc() + 2);
    }

    void pushFormals(MBasicBlock *current) {
        current->push(fun());
        current->push(thisArg());

        for (uint32_t i = 0; i < argc(); i++)
            current->push(getArg(i));
    }

    void setTypeInfo(types::StackTypeSet *types, types::StackTypeSet *barrier) {
        types_ = types;
        barrier_ = barrier;
    }

    bool hasTypeInfo() const {
        JS_ASSERT_IF(barrier_, types_);
        return types_;
    }

    uint32_t argc() {
        return args_.length();
    }

    void setArgs(Vector<MDefinition *> *args) {
        JS_ASSERT(args_.length() == 0);
        args_.append(args->begin(), args->end());
    }

    Vector<MDefinition *> *argv() {
        return &args_;
    }

    MDefinition *getArg(uint32_t i) {
        JS_ASSERT(i < argc());
        return args_[i];
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

    types::StackTypeSet *types() {
        return types_;
    }

    types::StackTypeSet *barrier() {
        return barrier_;
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

} 
} 

#endif 
