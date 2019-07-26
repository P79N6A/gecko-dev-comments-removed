





#ifndef jit_IonBuilder_h
#define jit_IonBuilder_h

#ifdef JS_ION




#include "jit/BytecodeAnalysis.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "jit/TypeRepresentationSet.h"

namespace js {
namespace jit {

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

    enum SetElemSafety {
        
        SetElem_Normal,

        
        
        
        SetElem_Unsafe,
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
            LABEL,              
            TRY                 
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
            struct {
                MBasicBlock *successor;
            } try_;
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
        static CFGState CondSwitch(IonBuilder *builder, jsbytecode *exitpc, jsbytecode *defaultTarget);
        static CFGState Label(jsbytecode *exitpc);
        static CFGState Try(jsbytecode *exitpc, MBasicBlock *successor);
    };

    static int CmpSuccessors(const void *a, const void *b);

  public:
    IonBuilder(JSContext *cx, TempAllocator *temp, MIRGraph *graph,
               types::CompilerConstraintList *constraints,
               BaselineInspector *inspector, CompileInfo *info, BaselineFrame *baselineFrame,
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

    JSFunction *getSingleCallTarget(types::TemporaryTypeSet *calleeTypes);
    bool getPolyCallTargets(types::TemporaryTypeSet *calleeTypes, bool constructing,
                            ObjectVector &targets, uint32_t maxTargets, bool *gotLambda);
    bool canInlineTarget(JSFunction *target, bool constructing);

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
    ControlStatus processTryEnd(CFGState &state);
    ControlStatus processReturn(JSOp op);
    ControlStatus processThrow();
    ControlStatus processContinue(JSOp op);
    ControlStatus processBreak(JSOp op, jssrcnote *sn);
    ControlStatus maybeLoop(JSOp op, jssrcnote *sn);
    bool pushLoop(CFGState::State state, jsbytecode *stopAt, MBasicBlock *entry, bool osr,
                  jsbytecode *loopHead, jsbytecode *initialPc,
                  jsbytecode *bodyStart, jsbytecode *bodyEnd, jsbytecode *exitpc,
                  jsbytecode *continuepc = nullptr);
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
        return newBlock(nullptr, pc);
    }
    MBasicBlock *newBlockAfter(MBasicBlock *at, jsbytecode *pc) {
        return newBlockAfter(at, nullptr, pc);
    }

    
    
    MBasicBlock *createBreakCatchBlock(DeferredEdge *edge, jsbytecode *pc);

    
    
    ControlStatus processBrokenLoop(CFGState &state);

    
    
    ControlStatus finishLoop(CFGState &state, MBasicBlock *successor);

    
    
    bool addOsrValueTypeBarrier(uint32_t slot, MInstruction **def,
                                MIRType type, types::TemporaryTypeSet *typeSet);
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
    bool initScopeChain(MDefinition *callee = nullptr);
    bool initArgumentsObject();
    bool pushConstant(const Value &v);

    
    
    bool pushTypeBarrier(MInstruction *ins, types::TemporaryTypeSet *observed, bool needBarrier);

    JSObject *getSingletonPrototype(JSFunction *target);

    MDefinition *createThisScripted(MDefinition *callee);
    MDefinition *createThisScriptedSingleton(JSFunction *target, MDefinition *callee);
    MDefinition *createThis(JSFunction *target, MDefinition *callee);
    MInstruction *createDeclEnvObject(MDefinition *callee, MDefinition *scopeObj);
    MInstruction *createCallObject(MDefinition *callee, MDefinition *scopeObj);

    MDefinition *walkScopeChain(unsigned hops);

    MInstruction *addConvertElementsToDoubles(MDefinition *elements);
    MInstruction *addBoundsCheck(MDefinition *index, MDefinition *length);
    MInstruction *addShapeGuard(MDefinition *obj, Shape *const shape, BailoutKind bailoutKind);

    MDefinition *convertShiftToMaskForStaticTypedArray(MDefinition *id,
                                                       ArrayBufferView::ViewType viewType);

    bool invalidatedIdempotentCache();

    bool hasStaticScopeObject(ScopeCoordinate sc, JSObject **pcall);
    bool loadSlot(MDefinition *obj, Shape *shape, MIRType rvalType,
                  bool barrier, types::TemporaryTypeSet *types);
    bool storeSlot(MDefinition *obj, Shape *shape, MDefinition *value, bool needsBarrier,
                   MIRType slotType = MIRType_None);

    
    bool getPropTryArgumentsLength(bool *emitted);
    bool getPropTryConstant(bool *emitted, PropertyName *name,
                            types::TemporaryTypeSet *types);
    bool getPropTryDefiniteSlot(bool *emitted, PropertyName *name,
                                bool barrier, types::TemporaryTypeSet *types);
    bool getPropTryCommonGetter(bool *emitted, PropertyName *name,
                                types::TemporaryTypeSet *types);
    bool getPropTryInlineAccess(bool *emitted, PropertyName *name,
                                bool barrier, types::TemporaryTypeSet *types);
    bool getPropTryTypedObject(bool *emitted, PropertyName *name,
                               types::TemporaryTypeSet *resultTypes);
    bool getPropTryScalarPropOfTypedObject(bool *emitted,
                                           int32_t fieldOffset,
                                           TypeRepresentationSet fieldTypeReprs,
                                           types::TemporaryTypeSet *resultTypes);
    bool getPropTryComplexPropOfTypedObject(bool *emitted,
                                            int32_t fieldOffset,
                                            TypeRepresentationSet fieldTypeReprs,
                                            size_t fieldIndex,
                                            types::TemporaryTypeSet *resultTypes);
    bool getPropTryCache(bool *emitted, PropertyName *name,
                         bool barrier, types::TemporaryTypeSet *types);
    bool needsToMonitorMissingProperties(types::TemporaryTypeSet *types);

    
    bool setPropTryCommonSetter(bool *emitted, MDefinition *obj,
                                PropertyName *name, MDefinition *value);
    bool setPropTryCommonDOMSetter(bool *emitted, MDefinition *obj,
                                   MDefinition *value, JSFunction *setter,
                                   bool isDOM);
    bool setPropTryDefiniteSlot(bool *emitted, MDefinition *obj,
                                PropertyName *name, MDefinition *value,
                                bool barrier, types::TemporaryTypeSet *objTypes);
    bool setPropTryInlineAccess(bool *emitted, MDefinition *obj,
                                PropertyName *name, MDefinition *value, bool barrier,
                                types::TemporaryTypeSet *objTypes);
    bool setPropTryTypedObject(bool *emitted, MDefinition *obj,
                               PropertyName *name, MDefinition *value);
    bool setPropTryCache(bool *emitted, MDefinition *obj,
                         PropertyName *name, MDefinition *value,
                         bool barrier, types::TemporaryTypeSet *objTypes);

    
    bool lookupTypeRepresentationSet(MDefinition *typedObj,
                                     TypeRepresentationSet *out);
    bool lookupTypedObjectField(MDefinition *typedObj,
                                PropertyName *name,
                                int32_t *fieldOffset,
                                TypeRepresentationSet *fieldTypeReprs,
                                size_t *fieldIndex);
    MDefinition *loadTypedObjectType(MDefinition *value);
    void loadTypedObjectData(MDefinition *inOwner,
                             int32_t inOffset,
                             MDefinition **outOwner,
                             MDefinition **outOffset);
    MDefinition *typeObjectForFieldFromStructType(MDefinition *type,
                                                  size_t fieldIndex);

    
    bool setElemTryTyped(bool *emitted, MDefinition *object,
                         MDefinition *index, MDefinition *value);
    bool setElemTryTypedStatic(bool *emitted, MDefinition *object,
                               MDefinition *index, MDefinition *value);
    bool setElemTryDense(bool *emitted, MDefinition *object,
                         MDefinition *index, MDefinition *value);
    bool setElemTryArguments(bool *emitted, MDefinition *object,
                             MDefinition *index, MDefinition *value);
    bool setElemTryCache(bool *emitted, MDefinition *object,
                         MDefinition *index, MDefinition *value);

    
    bool getElemTryDense(bool *emitted, MDefinition *obj, MDefinition *index);
    bool getElemTryTypedStatic(bool *emitted, MDefinition *obj, MDefinition *index);
    bool getElemTryTyped(bool *emitted, MDefinition *obj, MDefinition *index);
    bool getElemTryString(bool *emitted, MDefinition *obj, MDefinition *index);
    bool getElemTryArguments(bool *emitted, MDefinition *obj, MDefinition *index);
    bool getElemTryArgumentsInlined(bool *emitted, MDefinition *obj, MDefinition *index);
    bool getElemTryCache(bool *emitted, MDefinition *obj, MDefinition *index);

    
    MInstruction *getTypedArrayLength(MDefinition *obj);
    MInstruction *getTypedArrayElements(MDefinition *obj);

    bool jsop_add(MDefinition *left, MDefinition *right);
    bool jsop_bitnot();
    bool jsop_bitop(JSOp op);
    bool jsop_binary(JSOp op);
    bool jsop_binary(JSOp op, MDefinition *left, MDefinition *right);
    bool jsop_pos();
    bool jsop_neg();
    bool jsop_setarg(uint32_t arg);
    bool jsop_defvar(uint32_t index);
    bool jsop_deffun(uint32_t index);
    bool jsop_notearg();
    bool jsop_funcall(uint32_t argc);
    bool jsop_funapply(uint32_t argc);
    bool jsop_funapplyarguments(uint32_t argc);
    bool jsop_call(uint32_t argc, bool constructing);
    bool jsop_eval(uint32_t argc);
    bool jsop_ifeq(JSOp op);
    bool jsop_try();
    bool jsop_label();
    bool jsop_condswitch();
    bool jsop_andor(JSOp op);
    bool jsop_dup2();
    bool jsop_loophead(jsbytecode *pc);
    bool jsop_compare(JSOp op);
    bool getStaticName(JSObject *staticObject, PropertyName *name, bool *psucceeded);
    bool setStaticName(JSObject *staticObject, PropertyName *name);
    bool jsop_getname(PropertyName *name);
    bool jsop_intrinsic(PropertyName *name);
    bool jsop_bindname(PropertyName *name);
    bool jsop_getelem();
    bool jsop_getelem_dense(MDefinition *obj, MDefinition *index);
    bool jsop_getelem_typed(MDefinition *obj, MDefinition *index, ScalarTypeRepresentation::Type arrayType);
    bool jsop_setelem();
    bool jsop_setelem_dense(types::TemporaryTypeSet::DoubleConversion conversion,
                            SetElemSafety safety,
                            MDefinition *object, MDefinition *index, MDefinition *value);
    bool jsop_setelem_typed(ScalarTypeRepresentation::Type arrayType,
                            SetElemSafety safety,
                            MDefinition *object, MDefinition *index, MDefinition *value);
    bool jsop_length();
    bool jsop_length_fastPath();
    bool jsop_arguments();
    bool jsop_arguments_length();
    bool jsop_arguments_getelem();
    bool jsop_runonce();
    bool jsop_rest();
    bool jsop_not();
    bool jsop_getprop(PropertyName *name);
    bool jsop_setprop(PropertyName *name);
    bool jsop_delprop(PropertyName *name);
    bool jsop_delelem();
    bool jsop_newarray(uint32_t count);
    bool jsop_newobject();
    bool jsop_initelem();
    bool jsop_initelem_array();
    bool jsop_initelem_getter_setter();
    bool jsop_initprop(PropertyName *name);
    bool jsop_initprop_getter_setter(PropertyName *name);
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
    uint32_t selectInliningTargets(ObjectVector &targets, CallInfo &callInfo,
                                   BoolVector &choiceSet);

    
    types::TemporaryTypeSet *getInlineReturnTypeSet();
    MIRType getInlineReturnType();

    
    InliningStatus inlineArray(CallInfo &callInfo);
    InliningStatus inlineArrayPopShift(CallInfo &callInfo, MArrayPopShift::Mode mode);
    InliningStatus inlineArrayPush(CallInfo &callInfo);
    InliningStatus inlineArrayConcat(CallInfo &callInfo);

    
    InliningStatus inlineMathAbs(CallInfo &callInfo);
    InliningStatus inlineMathFloor(CallInfo &callInfo);
    InliningStatus inlineMathRound(CallInfo &callInfo);
    InliningStatus inlineMathSqrt(CallInfo &callInfo);
    InliningStatus inlineMathAtan2(CallInfo &callInfo);
    InliningStatus inlineMathMinMax(CallInfo &callInfo, bool max);
    InliningStatus inlineMathPow(CallInfo &callInfo);
    InliningStatus inlineMathRandom(CallInfo &callInfo);
    InliningStatus inlineMathImul(CallInfo &callInfo);
    InliningStatus inlineMathFRound(CallInfo &callInfo);
    InliningStatus inlineMathFunction(CallInfo &callInfo, MMathFunction::Function function);

    
    InliningStatus inlineStringObject(CallInfo &callInfo);
    InliningStatus inlineStrCharCodeAt(CallInfo &callInfo);
    InliningStatus inlineStrFromCharCode(CallInfo &callInfo);
    InliningStatus inlineStrCharAt(CallInfo &callInfo);

    
    InliningStatus inlineRegExpTest(CallInfo &callInfo);

    
    InliningStatus inlineUnsafePutElements(CallInfo &callInfo);
    bool inlineUnsafeSetDenseArrayElement(CallInfo &callInfo, uint32_t base);
    bool inlineUnsafeSetTypedArrayElement(CallInfo &callInfo, uint32_t base,
                                          ScalarTypeRepresentation::Type arrayType);
    InliningStatus inlineNewDenseArray(CallInfo &callInfo);
    InliningStatus inlineNewDenseArrayForSequentialExecution(CallInfo &callInfo);
    InliningStatus inlineNewDenseArrayForParallelExecution(CallInfo &callInfo);

    
    InliningStatus inlineUnsafeSetReservedSlot(CallInfo &callInfo);
    InliningStatus inlineUnsafeGetReservedSlot(CallInfo &callInfo);

    
    InliningStatus inlineNewParallelArray(CallInfo &callInfo);
    InliningStatus inlineParallelArray(CallInfo &callInfo);
    InliningStatus inlineParallelArrayTail(CallInfo &callInfo,
                                           HandleFunction target,
                                           MDefinition *ctor,
                                           types::TemporaryTypeSet *ctorTypes,
                                           uint32_t discards);

    
    InliningStatus inlineIsCallable(CallInfo &callInfo);
    InliningStatus inlineHaveSameClass(CallInfo &callInfo);
    InliningStatus inlineToObject(CallInfo &callInfo);
    InliningStatus inlineDump(CallInfo &callInfo);

    
    InliningStatus inlineForceSequentialOrInParallelSection(CallInfo &callInfo);
    InliningStatus inlineBailout(CallInfo &callInfo);
    InliningStatus inlineAssertFloat32(CallInfo &callInfo);

    
    InliningStatus inlineNativeCall(CallInfo &callInfo, JSNative native);
    bool inlineScriptedCall(CallInfo &callInfo, JSFunction *target);
    InliningStatus inlineSingleCall(CallInfo &callInfo, JSFunction *target);

    
    InliningStatus inlineCallsite(ObjectVector &targets, ObjectVector &originals,
                                  bool lambda, CallInfo &callInfo);
    bool inlineCalls(CallInfo &callInfo, ObjectVector &targets, ObjectVector &originals,
                     BoolVector &choiceSet, MGetPropertyCache *maybeCache);

    
    bool inlineGenericFallback(JSFunction *target, CallInfo &callInfo, MBasicBlock *dispatchBlock,
                               bool clonedAtCallsite);
    bool inlineTypeObjectFallback(CallInfo &callInfo, MBasicBlock *dispatchBlock,
                                  MTypeObjectDispatch *dispatch, MGetPropertyCache *cache,
                                  MBasicBlock **fallbackTarget);

    bool testNeedsArgumentCheck(JSContext *cx, JSFunction *target, CallInfo &callInfo);

    MDefinition *makeCallsiteClone(JSFunction *target, MDefinition *fun);
    MCall *makeCallHelper(JSFunction *target, CallInfo &callInfo, bool cloneAtCallsite);
    bool makeCall(JSFunction *target, CallInfo &callInfo, bool cloneAtCallsite);

    MDefinition *patchInlinedReturn(CallInfo &callInfo, MBasicBlock *exit, MBasicBlock *bottom);
    MDefinition *patchInlinedReturns(CallInfo &callInfo, MIRGraphExits &exits, MBasicBlock *bottom);

    inline bool testCommonPropFunc(JSContext *cx, types::TemporaryTypeSet *types,
                                   PropertyName *name, JSFunction **funcp,
                                   bool isGetter, bool *isDOM,
                                   MDefinition **guardOut);

    bool annotateGetPropertyCache(JSContext *cx, MDefinition *obj, MGetPropertyCache *getPropCache,
                                  types::TemporaryTypeSet *objTypes, types::TemporaryTypeSet *pushedTypes);

    MGetPropertyCache *getInlineableGetPropertyCache(CallInfo &callInfo);

    bool testSingletonProperty(JSObject *obj, JSObject *singleton, PropertyName *name,
                               bool *isKnownConstant);
    bool testSingletonPropertyTypes(MDefinition *obj, JSObject *singleton,
                                    JSObject *globalObj, PropertyName *name,
                                    bool *isKnownConstant, bool *testObject,
                                    bool *testString);
    bool getDefiniteSlot(types::TemporaryTypeSet *types, PropertyName *name,
                         types::HeapTypeSetKey *property);
    bool freezePropTypeSets(types::TemporaryTypeSet *types,
                            JSObject *foundProto, PropertyName *name);

    types::TemporaryTypeSet *bytecodeTypes(jsbytecode *pc);

    
    
    

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
    void clearForBackEnd();

    JSScript *script() const { return script_.get(); }

    CodeGenerator *backgroundCodegen() const { return backgroundCodegen_; }
    void setBackgroundCodegen(CodeGenerator *codegen) { backgroundCodegen_ = codegen; }

    AbortReason abortReason() { return abortReason_; }

    TypeRepresentationSetHash *getOrCreateReprSetHash(); 

    types::CompilerConstraintList *constraints() {
        return constraints_;
    }

    bool isInlineBuilder() const {
        return callerBuilder_ != nullptr;
    }

    JSContext *context() {
        
        
        
        
        if (info().executionMode() == DefinitePropertiesAnalysis)
            return cx;
        return nullptr;
    }

    JSAtomState &names() { return compartment->runtimeFromAnyThread()->atomState; }

  private:
    bool init();

    JSContext *cx;
    BaselineFrame *baselineFrame_;
    AbortReason abortReason_;
    ScopedJSDeletePtr<TypeRepresentationSetHash> reprSetHash_;

    
    types::CompilerConstraintList *constraints_;

    
    BytecodeAnalysis analysis_;
    BytecodeAnalysis &analysis() {
        return analysis_;
    }

    types::TemporaryTypeSet *thisTypes, *argTypes, *typeArray;
    uint32_t typeArrayHint;

    GSNCache gsn;

    jsbytecode *pc;
    MBasicBlock *current;
    uint32_t loopDepth_;

    
    MResumePoint *callerResumePoint_;
    jsbytecode *callerPC() {
        return callerResumePoint_ ? callerResumePoint_->pc() : nullptr;
    }
    IonBuilder *callerBuilder_;

    struct LoopHeader {
        jsbytecode *pc;
        MBasicBlock *header;

        LoopHeader(jsbytecode *pc, MBasicBlock *header)
          : pc(pc), header(header)
        {}
    };

    Vector<CFGState, 8, IonAllocPolicy> cfgStack_;
    Vector<ControlFlowInfo, 4, IonAllocPolicy> loops_;
    Vector<ControlFlowInfo, 0, IonAllocPolicy> switches_;
    Vector<ControlFlowInfo, 2, IonAllocPolicy> labels_;
    Vector<MInstruction *, 2, IonAllocPolicy> iterators_;
    Vector<LoopHeader, 0, IonAllocPolicy> loopHeaders_;
    BaselineInspector *inspector;

    size_t inliningDepth_;

    
    
    static const size_t MAX_LOOP_RESTARTS = 40;
    size_t numLoopRestarts_;

    
    
    bool failedBoundsCheck_;

    
    
    bool failedShapeGuard_;

    
    bool nonStringIteration_;

    
    
    MInstruction *lazyArguments_;

    
    const CallInfo *inlineCallInfo_;
};

class CallInfo
{
    MDefinition *fun_;
    MDefinition *thisArg_;
    MDefinitionVector args_;

    bool constructing_;
    bool setter_;

  public:
    CallInfo(bool constructing)
      : fun_(nullptr),
        thisArg_(nullptr),
        constructing_(constructing),
        setter_(false)
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

    void setArgs(MDefinitionVector *args) {
        JS_ASSERT(args_.length() == 0);
        args_.append(args->begin(), args->end());
    }

    MDefinitionVector &argv() {
        return args_;
    }

    const MDefinitionVector &argv() const {
        return args_;
    }

    MDefinition *getArg(uint32_t i) const {
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

    bool isSetter() const {
        return setter_;
    }
    void markAsSetter() {
        setter_ = true;
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

bool NeedsPostBarrier(CompileInfo &info, MDefinition *value);

} 
} 

#endif

#endif
