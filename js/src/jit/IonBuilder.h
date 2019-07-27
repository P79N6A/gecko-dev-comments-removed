





#ifndef jit_IonBuilder_h
#define jit_IonBuilder_h




#include "mozilla/LinkedList.h"

#include "jit/BaselineInspector.h"
#include "jit/BytecodeAnalysis.h"
#include "jit/IonAnalysis.h"
#include "jit/IonOptimizationLevels.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/MIRGraph.h"
#include "jit/OptimizationTracking.h"

namespace js {
namespace jit {

class CodeGenerator;
class CallInfo;
class BaselineFrameInspector;



BaselineFrameInspector*
NewBaselineFrameInspector(TempAllocator* temp, BaselineFrame* frame, CompileInfo* info);

class IonBuilder
  : public MIRGenerator,
    public mozilla::LinkedListElement<IonBuilder>
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
        MBasicBlock* block;
        DeferredEdge* next;

        DeferredEdge(MBasicBlock* block, DeferredEdge* next)
          : block(block), next(next)
        { }
    };

    struct ControlFlowInfo {
        
        uint32_t cfgEntry;

        
        jsbytecode* continuepc;

        ControlFlowInfo(uint32_t cfgEntry, jsbytecode* continuepc)
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
        jsbytecode* stopAt;     

        
        union {
            struct {
                MBasicBlock* ifFalse;
                jsbytecode* falseEnd;
                MBasicBlock* ifTrue;    
                MTest* test;
            } branch;
            struct {
                
                MBasicBlock* entry;

                
                bool osr;

                
                jsbytecode* bodyStart;
                jsbytecode* bodyEnd;

                
                jsbytecode* exitpc;

                
                jsbytecode* continuepc;

                
                MBasicBlock* successor;

                
                DeferredEdge* breaks;
                DeferredEdge* continues;

                
                State initialState;
                jsbytecode* initialPc;
                jsbytecode* initialStopAt;
                jsbytecode* loopHead;

                
                jsbytecode* condpc;
                jsbytecode* updatepc;
                jsbytecode* updateEnd;
            } loop;
            struct {
                
                jsbytecode* exitpc;

                
                DeferredEdge* breaks;

                
                MTableSwitch* ins;

                
                uint32_t currentBlock;

            } tableswitch;
            struct {
                
                FixedList<MBasicBlock*>* bodies;

                
                
                
                uint32_t currentIdx;

                
                jsbytecode* defaultTarget;
                uint32_t defaultIdx;

                
                jsbytecode* exitpc;
                DeferredEdge* breaks;
            } condswitch;
            struct {
                DeferredEdge* breaks;
            } label;
            struct {
                MBasicBlock* successor;
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

        static CFGState If(jsbytecode* join, MTest* test);
        static CFGState IfElse(jsbytecode* trueEnd, jsbytecode* falseEnd, MTest* test);
        static CFGState AndOr(jsbytecode* join, MBasicBlock* lhs);
        static CFGState TableSwitch(jsbytecode* exitpc, MTableSwitch* ins);
        static CFGState CondSwitch(IonBuilder* builder, jsbytecode* exitpc, jsbytecode* defaultTarget);
        static CFGState Label(jsbytecode* exitpc);
        static CFGState Try(jsbytecode* exitpc, MBasicBlock* successor);
    };

    static int CmpSuccessors(const void* a, const void* b);

  public:
    IonBuilder(JSContext* analysisContext, CompileCompartment* comp,
               const JitCompileOptions& options, TempAllocator* temp,
               MIRGraph* graph, CompilerConstraintList* constraints,
               BaselineInspector* inspector, CompileInfo* info,
               const OptimizationInfo* optimizationInfo, BaselineFrameInspector* baselineFrame,
               size_t inliningDepth = 0, uint32_t loopDepth = 0);

    bool build();
    bool buildInline(IonBuilder* callerBuilder, MResumePoint* callerResumePoint,
                     CallInfo& callInfo);

  private:
    bool traverseBytecode();
    ControlStatus snoopControlFlow(JSOp op);
    bool processIterators();
    bool inspectOpcode(JSOp op);
    uint32_t readIndex(jsbytecode* pc);
    JSAtom* readAtom(jsbytecode* pc);
    bool abort(const char* message, ...);
    void trackActionableAbort(const char* message);
    void spew(const char* message);

    MInstruction* constantMaybeNursery(JSObject* obj);

    JSFunction* getSingleCallTarget(TemporaryTypeSet* calleeTypes);
    bool getPolyCallTargets(TemporaryTypeSet* calleeTypes, bool constructing,
                            ObjectVector& targets, uint32_t maxTargets);

    void popCfgStack();
    DeferredEdge* filterDeadDeferredEdges(DeferredEdge* edge);
    bool processDeferredContinues(CFGState& state);
    ControlStatus processControlEnd();
    ControlStatus processCfgStack();
    ControlStatus processCfgEntry(CFGState& state);
    ControlStatus processIfEnd(CFGState& state);
    ControlStatus processIfElseTrueEnd(CFGState& state);
    ControlStatus processIfElseFalseEnd(CFGState& state);
    ControlStatus processDoWhileBodyEnd(CFGState& state);
    ControlStatus processDoWhileCondEnd(CFGState& state);
    ControlStatus processWhileCondEnd(CFGState& state);
    ControlStatus processWhileBodyEnd(CFGState& state);
    ControlStatus processForCondEnd(CFGState& state);
    ControlStatus processForBodyEnd(CFGState& state);
    ControlStatus processForUpdateEnd(CFGState& state);
    ControlStatus processNextTableSwitchCase(CFGState& state);
    ControlStatus processCondSwitchCase(CFGState& state);
    ControlStatus processCondSwitchBody(CFGState& state);
    ControlStatus processSwitchBreak(JSOp op);
    ControlStatus processSwitchEnd(DeferredEdge* breaks, jsbytecode* exitpc);
    ControlStatus processAndOrEnd(CFGState& state);
    ControlStatus processLabelEnd(CFGState& state);
    ControlStatus processTryEnd(CFGState& state);
    ControlStatus processReturn(JSOp op);
    ControlStatus processThrow();
    ControlStatus processContinue(JSOp op);
    ControlStatus processBreak(JSOp op, jssrcnote* sn);
    ControlStatus maybeLoop(JSOp op, jssrcnote* sn);
    bool pushLoop(CFGState::State state, jsbytecode* stopAt, MBasicBlock* entry, bool osr,
                  jsbytecode* loopHead, jsbytecode* initialPc,
                  jsbytecode* bodyStart, jsbytecode* bodyEnd, jsbytecode* exitpc,
                  jsbytecode* continuepc = nullptr);
    bool analyzeNewLoopTypes(MBasicBlock* entry, jsbytecode* start, jsbytecode* end);

    MBasicBlock* addBlock(MBasicBlock* block, uint32_t loopDepth);
    MBasicBlock* newBlock(MBasicBlock* predecessor, jsbytecode* pc);
    MBasicBlock* newBlock(MBasicBlock* predecessor, jsbytecode* pc, uint32_t loopDepth);
    MBasicBlock* newBlock(MBasicBlock* predecessor, jsbytecode* pc, MResumePoint* priorResumePoint);
    MBasicBlock* newBlockPopN(MBasicBlock* predecessor, jsbytecode* pc, uint32_t popped);
    MBasicBlock* newBlockAfter(MBasicBlock* at, MBasicBlock* predecessor, jsbytecode* pc);
    MBasicBlock* newOsrPreheader(MBasicBlock* header, jsbytecode* loopEntry);
    MBasicBlock* newPendingLoopHeader(MBasicBlock* predecessor, jsbytecode* pc, bool osr, bool canOsr,
                                      unsigned stackPhiCount);
    MBasicBlock* newBlock(jsbytecode* pc) {
        return newBlock(nullptr, pc);
    }
    MBasicBlock* newBlockAfter(MBasicBlock* at, jsbytecode* pc) {
        return newBlockAfter(at, nullptr, pc);
    }

    
    
    
    
    
    
    MTest* newTest(MDefinition* ins, MBasicBlock* ifTrue, MBasicBlock* ifFalse);

    
    
    MBasicBlock* createBreakCatchBlock(DeferredEdge* edge, jsbytecode* pc);

    
    
    ControlStatus processBrokenLoop(CFGState& state);

    
    
    ControlStatus finishLoop(CFGState& state, MBasicBlock* successor);

    
    
    bool addOsrValueTypeBarrier(uint32_t slot, MInstruction** def,
                                MIRType type, TemporaryTypeSet* typeSet);
    bool maybeAddOsrTypeBarriers();

    
    
    ControlStatus restartLoop(CFGState state);

    void assertValidLoopHeadOp(jsbytecode* pc);

    ControlStatus forLoop(JSOp op, jssrcnote* sn);
    ControlStatus whileOrForInLoop(jssrcnote* sn);
    ControlStatus doWhileLoop(JSOp op, jssrcnote* sn);
    ControlStatus tableSwitch(JSOp op, jssrcnote* sn);
    ControlStatus condSwitch(JSOp op, jssrcnote* sn);

    
    
    bool resume(MInstruction* ins, jsbytecode* pc, MResumePoint::Mode mode);
    bool resumeAt(MInstruction* ins, jsbytecode* pc);
    bool resumeAfter(MInstruction* ins);
    bool maybeInsertResume();

    void insertRecompileCheck();

    void initParameters();
    void initLocals();
    void rewriteParameter(uint32_t slotIdx, MDefinition* param, int32_t argIndex);
    void rewriteParameters();
    bool initScopeChain(MDefinition* callee = nullptr);
    bool initArgumentsObject();
    bool pushConstant(const Value& v);

    MConstant* constant(const Value& v);
    MConstant* constantInt(int32_t i);

    
    MConstant* constantMaybeAtomize(const Value& v);

    
    bool improveTypesAtTest(MDefinition* ins, bool trueBranch, MTest* test);
    bool improveTypesAtCompare(MCompare* ins, bool trueBranch, MTest* test);
    bool improveTypesAtNullOrUndefinedCompare(MCompare* ins, bool trueBranch, MTest* test);
    bool improveTypesAtTypeOfCompare(MCompare* ins, bool trueBranch, MTest* test);

    
    bool detectAndOrStructure(MPhi* ins, bool* branchIsTrue);
    bool replaceTypeSet(MDefinition* subject, TemporaryTypeSet* type, MTest* test);

    
    
    MDefinition* addTypeBarrier(MDefinition* def, TemporaryTypeSet* observed,
                                BarrierKind kind, MTypeBarrier** pbarrier = nullptr);
    bool pushTypeBarrier(MDefinition* def, TemporaryTypeSet* observed, BarrierKind kind);

    
    
    
    bool pushDOMTypeBarrier(MInstruction* ins, TemporaryTypeSet* observed, JSFunction* func);

    
    
    
    
    MDefinition* ensureDefiniteType(MDefinition* def, MIRType definiteType);

    
    MDefinition* ensureDefiniteTypeSet(MDefinition* def, TemporaryTypeSet* types);

    JSObject* getSingletonPrototype(JSFunction* target);

    MDefinition* createThisScripted(MDefinition* callee);
    MDefinition* createThisScriptedSingleton(JSFunction* target, MDefinition* callee);
    MDefinition* createThisScriptedBaseline(MDefinition* callee);
    MDefinition* createThis(JSFunction* target, MDefinition* callee);
    MInstruction* createDeclEnvObject(MDefinition* callee, MDefinition* scopeObj);
    MInstruction* createCallObject(MDefinition* callee, MDefinition* scopeObj);

    MDefinition* walkScopeChain(unsigned hops);

    MInstruction* addConvertElementsToDoubles(MDefinition* elements);
    MDefinition* addMaybeCopyElementsForWrite(MDefinition* object);
    MInstruction* addBoundsCheck(MDefinition* index, MDefinition* length);
    MInstruction* addShapeGuard(MDefinition* obj, Shape* const shape, BailoutKind bailoutKind);
    MInstruction* addGroupGuard(MDefinition* obj, ObjectGroup* group, BailoutKind bailoutKind);
    MInstruction* addUnboxedExpandoGuard(MDefinition* obj, bool hasExpando, BailoutKind bailoutKind);

    MInstruction*
    addGuardReceiverPolymorphic(MDefinition* obj, const BaselineInspector::ReceiverVector& receivers);

    MDefinition* convertShiftToMaskForStaticTypedArray(MDefinition* id,
                                                       Scalar::Type viewType);

    bool invalidatedIdempotentCache();

    bool hasStaticScopeObject(ScopeCoordinate sc, JSObject** pcall);
    bool loadSlot(MDefinition* obj, size_t slot, size_t nfixed, MIRType rvalType,
                  BarrierKind barrier, TemporaryTypeSet* types);
    bool loadSlot(MDefinition* obj, Shape* shape, MIRType rvalType,
                  BarrierKind barrier, TemporaryTypeSet* types);
    bool storeSlot(MDefinition* obj, size_t slot, size_t nfixed,
                   MDefinition* value, bool needsBarrier,
                   MIRType slotType = MIRType_None);
    bool storeSlot(MDefinition* obj, Shape* shape, MDefinition* value, bool needsBarrier,
                   MIRType slotType = MIRType_None);

    MDefinition* tryInnerizeWindow(MDefinition* obj);

    
    bool checkIsDefinitelyOptimizedArguments(MDefinition* obj, bool* isOptimizedArgs);
    bool getPropTryInferredConstant(bool* emitted, MDefinition* obj, PropertyName* name,
                                    TemporaryTypeSet* types);
    bool getPropTryArgumentsLength(bool* emitted, MDefinition* obj);
    bool getPropTryArgumentsCallee(bool* emitted, MDefinition* obj, PropertyName* name);
    bool getPropTryConstant(bool* emitted, MDefinition* obj, PropertyName* name,
                            TemporaryTypeSet* types);
    bool getPropTryDefiniteSlot(bool* emitted, MDefinition* obj, PropertyName* name,
                                BarrierKind barrier, TemporaryTypeSet* types);
    bool getPropTryUnboxed(bool* emitted, MDefinition* obj, PropertyName* name,
                           BarrierKind barrier, TemporaryTypeSet* types);
    bool getPropTryCommonGetter(bool* emitted, MDefinition* obj, PropertyName* name,
                                TemporaryTypeSet* types);
    bool getPropTryInlineAccess(bool* emitted, MDefinition* obj, PropertyName* name,
                                BarrierKind barrier, TemporaryTypeSet* types);
    bool getPropTrySimdGetter(bool* emitted, MDefinition* obj, PropertyName* name);
    bool getPropTryTypedObject(bool* emitted, MDefinition* obj, PropertyName* name);
    bool getPropTryScalarPropOfTypedObject(bool* emitted, MDefinition* typedObj,
                                           int32_t fieldOffset,
                                           TypedObjectPrediction fieldTypeReprs);
    bool getPropTryReferencePropOfTypedObject(bool* emitted, MDefinition* typedObj,
                                              int32_t fieldOffset,
                                              TypedObjectPrediction fieldPrediction,
                                              PropertyName* name);
    bool getPropTryComplexPropOfTypedObject(bool* emitted, MDefinition* typedObj,
                                            int32_t fieldOffset,
                                            TypedObjectPrediction fieldTypeReprs,
                                            size_t fieldIndex);
    bool getPropTryInnerize(bool* emitted, MDefinition* obj, PropertyName* name,
                            TemporaryTypeSet* types);
    bool getPropTryCache(bool* emitted, MDefinition* obj, PropertyName* name,
                         BarrierKind barrier, TemporaryTypeSet* types);

    
    bool setPropTryCommonSetter(bool* emitted, MDefinition* obj,
                                PropertyName* name, MDefinition* value);
    bool setPropTryCommonDOMSetter(bool* emitted, MDefinition* obj,
                                   MDefinition* value, JSFunction* setter,
                                   TemporaryTypeSet* objTypes);
    bool setPropTryDefiniteSlot(bool* emitted, MDefinition* obj,
                                PropertyName* name, MDefinition* value,
                                bool barrier, TemporaryTypeSet* objTypes);
    bool setPropTryUnboxed(bool* emitted, MDefinition* obj,
                           PropertyName* name, MDefinition* value,
                           bool barrier, TemporaryTypeSet* objTypes);
    bool setPropTryInlineAccess(bool* emitted, MDefinition* obj,
                                PropertyName* name, MDefinition* value,
                                bool barrier, TemporaryTypeSet* objTypes);
    bool setPropTryTypedObject(bool* emitted, MDefinition* obj,
                               PropertyName* name, MDefinition* value);
    bool setPropTryReferencePropOfTypedObject(bool* emitted,
                                              MDefinition* obj,
                                              int32_t fieldOffset,
                                              MDefinition* value,
                                              TypedObjectPrediction fieldPrediction,
                                              PropertyName* name);
    bool setPropTryScalarPropOfTypedObject(bool* emitted,
                                           MDefinition* obj,
                                           int32_t fieldOffset,
                                           MDefinition* value,
                                           TypedObjectPrediction fieldTypeReprs);
    bool setPropTryCache(bool* emitted, MDefinition* obj,
                         PropertyName* name, MDefinition* value,
                         bool barrier, TemporaryTypeSet* objTypes);

    
    TypedObjectPrediction typedObjectPrediction(MDefinition* typedObj);
    TypedObjectPrediction typedObjectPrediction(TemporaryTypeSet* types);
    bool typedObjectHasField(MDefinition* typedObj,
                             PropertyName* name,
                             size_t* fieldOffset,
                             TypedObjectPrediction* fieldTypeReprs,
                             size_t* fieldIndex);
    MDefinition* loadTypedObjectType(MDefinition* value);
    void loadTypedObjectData(MDefinition* typedObj,
                             MDefinition** owner,
                             LinearSum* ownerOffset);
    void loadTypedObjectElements(MDefinition* typedObj,
                                 const LinearSum& byteOffset,
                                 int32_t scale,
                                 MDefinition** ownerElements,
                                 MDefinition** ownerScaledOffset,
                                 int32_t* ownerByteAdjustment);
    MDefinition* typeObjectForElementFromArrayStructType(MDefinition* typedObj);
    MDefinition* typeObjectForFieldFromStructType(MDefinition* type,
                                                  size_t fieldIndex);
    bool storeReferenceTypedObjectValue(MDefinition* typedObj,
                                        const LinearSum& byteOffset,
                                        ReferenceTypeDescr::Type type,
                                        MDefinition* value,
                                        PropertyName* name);
    bool storeScalarTypedObjectValue(MDefinition* typedObj,
                                     const LinearSum& byteOffset,
                                     ScalarTypeDescr::Type type,
                                     MDefinition* value);
    bool checkTypedObjectIndexInBounds(int32_t elemSize,
                                       MDefinition* obj,
                                       MDefinition* index,
                                       TypedObjectPrediction objTypeDescrs,
                                       LinearSum* indexAsByteOffset);
    bool pushDerivedTypedObject(bool* emitted,
                                MDefinition* obj,
                                const LinearSum& byteOffset,
                                TypedObjectPrediction derivedTypeDescrs,
                                MDefinition* derivedTypeObj);
    bool pushScalarLoadFromTypedObject(MDefinition* obj,
                                       const LinearSum& byteoffset,
                                       ScalarTypeDescr::Type type);
    bool pushReferenceLoadFromTypedObject(MDefinition* typedObj,
                                          const LinearSum& byteOffset,
                                          ReferenceTypeDescr::Type type,
                                          PropertyName* name);
    MDefinition* neuterCheck(MDefinition* obj);
    JSObject* getStaticTypedArrayObject(MDefinition* obj, MDefinition* index);

    
    bool setElemTryTypedArray(bool* emitted, MDefinition* object,
                         MDefinition* index, MDefinition* value);
    bool setElemTryTypedObject(bool* emitted, MDefinition* obj,
                               MDefinition* index, MDefinition* value);
    bool setElemTryTypedStatic(bool* emitted, MDefinition* object,
                               MDefinition* index, MDefinition* value);
    bool setElemTryDense(bool* emitted, MDefinition* object,
                         MDefinition* index, MDefinition* value);
    bool setElemTryArguments(bool* emitted, MDefinition* object,
                             MDefinition* index, MDefinition* value);
    bool setElemTryCache(bool* emitted, MDefinition* object,
                         MDefinition* index, MDefinition* value);
    bool setElemTryReferenceElemOfTypedObject(bool* emitted,
                                              MDefinition* obj,
                                              MDefinition* index,
                                              TypedObjectPrediction objPrediction,
                                              MDefinition* value,
                                              TypedObjectPrediction elemPrediction);
    bool setElemTryScalarElemOfTypedObject(bool* emitted,
                                           MDefinition* obj,
                                           MDefinition* index,
                                           TypedObjectPrediction objTypeReprs,
                                           MDefinition* value,
                                           TypedObjectPrediction elemTypeReprs,
                                           int32_t elemSize);

    
    bool getElemTryDense(bool* emitted, MDefinition* obj, MDefinition* index);
    bool getElemTryTypedStatic(bool* emitted, MDefinition* obj, MDefinition* index);
    bool getElemTryTypedArray(bool* emitted, MDefinition* obj, MDefinition* index);
    bool getElemTryTypedObject(bool* emitted, MDefinition* obj, MDefinition* index);
    bool getElemTryString(bool* emitted, MDefinition* obj, MDefinition* index);
    bool getElemTryArguments(bool* emitted, MDefinition* obj, MDefinition* index);
    bool getElemTryArgumentsInlined(bool* emitted, MDefinition* obj, MDefinition* index);
    bool getElemTryCache(bool* emitted, MDefinition* obj, MDefinition* index);
    bool getElemTryScalarElemOfTypedObject(bool* emitted,
                                           MDefinition* obj,
                                           MDefinition* index,
                                           TypedObjectPrediction objTypeReprs,
                                           TypedObjectPrediction elemTypeReprs,
                                           int32_t elemSize);
    bool getElemTryReferenceElemOfTypedObject(bool* emitted,
                                              MDefinition* obj,
                                              MDefinition* index,
                                              TypedObjectPrediction objPrediction,
                                              TypedObjectPrediction elemPrediction);
    bool getElemTryComplexElemOfTypedObject(bool* emitted,
                                            MDefinition* obj,
                                            MDefinition* index,
                                            TypedObjectPrediction objTypeReprs,
                                            TypedObjectPrediction elemTypeReprs,
                                            int32_t elemSize);

    enum BoundsChecking { DoBoundsCheck, SkipBoundsCheck };

    
    
    
    
    
    void addTypedArrayLengthAndData(MDefinition* obj,
                                    BoundsChecking checking,
                                    MDefinition** index,
                                    MInstruction** length, MInstruction** elements);

    
    
    
    MInstruction* addTypedArrayLength(MDefinition* obj) {
        MInstruction* length;
        addTypedArrayLengthAndData(obj, SkipBoundsCheck, nullptr, &length, nullptr);
        return length;
    }

    bool improveThisTypesForCall();

    MDefinition* getCallee();
    MDefinition* getAliasedVar(ScopeCoordinate sc);
    MDefinition* addLexicalCheck(MDefinition* input);

    bool tryFoldInstanceOf(MDefinition* lhs, JSObject* protoObject);

    bool jsop_add(MDefinition* left, MDefinition* right);
    bool jsop_bitnot();
    bool jsop_bitop(JSOp op);
    bool jsop_binary(JSOp op);
    bool jsop_binary(JSOp op, MDefinition* left, MDefinition* right);
    bool jsop_pos();
    bool jsop_neg();
    bool jsop_setarg(uint32_t arg);
    bool jsop_defvar(uint32_t index);
    bool jsop_deffun(uint32_t index);
    bool jsop_notearg();
    bool jsop_checklexical();
    bool jsop_checkaliasedlet(ScopeCoordinate sc);
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
    bool jsop_loophead(jsbytecode* pc);
    bool jsop_compare(JSOp op);
    bool getStaticName(JSObject* staticObject, PropertyName* name, bool* psucceeded,
                       MDefinition* lexicalCheck = nullptr);
    bool setStaticName(JSObject* staticObject, PropertyName* name);
    bool jsop_getgname(PropertyName* name);
    bool jsop_getname(PropertyName* name);
    bool jsop_intrinsic(PropertyName* name);
    bool jsop_bindname(PropertyName* name);
    bool jsop_getelem();
    bool jsop_getelem_dense(MDefinition* obj, MDefinition* index);
    bool jsop_getelem_typed(MDefinition* obj, MDefinition* index, ScalarTypeDescr::Type arrayType);
    bool jsop_setelem();
    bool jsop_setelem_dense(TemporaryTypeSet::DoubleConversion conversion,
                            SetElemSafety safety,
                            MDefinition* object, MDefinition* index, MDefinition* value);
    bool jsop_setelem_typed(ScalarTypeDescr::Type arrayType,
                            SetElemSafety safety,
                            MDefinition* object, MDefinition* index, MDefinition* value);
    bool jsop_setelem_typed_object(ScalarTypeDescr::Type arrayType,
                                   SetElemSafety safety,
                                   MDefinition* object, MDefinition* index, MDefinition* value);
    bool jsop_length();
    bool jsop_length_fastPath();
    bool jsop_arguments();
    bool jsop_arguments_getelem();
    bool jsop_runonce();
    bool jsop_rest();
    bool jsop_not();
    bool jsop_getprop(PropertyName* name);
    bool jsop_setprop(PropertyName* name);
    bool jsop_delprop(PropertyName* name);
    bool jsop_delelem();
    bool jsop_newarray(uint32_t count);
    bool jsop_newarray_copyonwrite();
    bool jsop_newobject();
    bool jsop_initelem();
    bool jsop_initelem_array();
    bool jsop_initelem_getter_setter();
    bool jsop_mutateproto();
    bool jsop_initprop(PropertyName* name);
    bool jsop_initprop_getter_setter(PropertyName* name);
    bool jsop_regexp(RegExpObject* reobj);
    bool jsop_object(JSObject* obj);
    bool jsop_lambda(JSFunction* fun);
    bool jsop_lambda_arrow(JSFunction* fun);
    bool jsop_this();
    bool jsop_typeof();
    bool jsop_toid();
    bool jsop_iter(uint8_t flags);
    bool jsop_itermore();
    bool jsop_isnoiter();
    bool jsop_iterend();
    bool jsop_in();
    bool jsop_in_dense();
    bool jsop_instanceof();
    bool jsop_getaliasedvar(ScopeCoordinate sc);
    bool jsop_setaliasedvar(ScopeCoordinate sc);
    bool jsop_debugger();

    

    enum InliningStatus
    {
        InliningStatus_Error,
        InliningStatus_NotInlined,
        InliningStatus_WarmUpCountTooLow,
        InliningStatus_Inlined
    };

    enum InliningDecision
    {
        InliningDecision_Error,
        InliningDecision_Inline,
        InliningDecision_DontInline,
        InliningDecision_WarmUpCountTooLow
    };

    static InliningDecision DontInline(JSScript* targetScript, const char* reason);

    
    InliningDecision canInlineTarget(JSFunction* target, CallInfo& callInfo);
    InliningDecision makeInliningDecision(JSObject* target, CallInfo& callInfo);
    bool selectInliningTargets(const ObjectVector& targets, CallInfo& callInfo,
                               BoolVector& choiceSet, uint32_t* numInlineable);

    
    
    
    TemporaryTypeSet* getInlineReturnTypeSet();
    
    MIRType getInlineReturnType();

    
    InliningStatus inlineArray(CallInfo& callInfo);
    InliningStatus inlineArrayPopShift(CallInfo& callInfo, MArrayPopShift::Mode mode);
    InliningStatus inlineArrayPush(CallInfo& callInfo);
    InliningStatus inlineArrayConcat(CallInfo& callInfo);
    InliningStatus inlineArrayJoin(CallInfo& callInfo);
    InliningStatus inlineArraySplice(CallInfo& callInfo);

    
    InliningStatus inlineMathAbs(CallInfo& callInfo);
    InliningStatus inlineMathFloor(CallInfo& callInfo);
    InliningStatus inlineMathCeil(CallInfo& callInfo);
    InliningStatus inlineMathClz32(CallInfo& callInfo);
    InliningStatus inlineMathRound(CallInfo& callInfo);
    InliningStatus inlineMathSqrt(CallInfo& callInfo);
    InliningStatus inlineMathAtan2(CallInfo& callInfo);
    InliningStatus inlineMathHypot(CallInfo& callInfo);
    InliningStatus inlineMathMinMax(CallInfo& callInfo, bool max);
    InliningStatus inlineMathPow(CallInfo& callInfo);
    InliningStatus inlineMathRandom(CallInfo& callInfo);
    InliningStatus inlineMathImul(CallInfo& callInfo);
    InliningStatus inlineMathFRound(CallInfo& callInfo);
    InliningStatus inlineMathFunction(CallInfo& callInfo, MMathFunction::Function function);

    
    InliningStatus inlineStringObject(CallInfo& callInfo);
    InliningStatus inlineConstantStringSplit(CallInfo& callInfo);
    InliningStatus inlineStringSplit(CallInfo& callInfo);
    InliningStatus inlineStrCharCodeAt(CallInfo& callInfo);
    InliningStatus inlineConstantCharCodeAt(CallInfo& callInfo);
    InliningStatus inlineStrFromCharCode(CallInfo& callInfo);
    InliningStatus inlineStrCharAt(CallInfo& callInfo);
    InliningStatus inlineStrReplace(CallInfo& callInfo);

    
    InliningStatus inlineRegExpExec(CallInfo& callInfo);
    InliningStatus inlineRegExpTest(CallInfo& callInfo);

    
    InliningStatus inlineObjectCreate(CallInfo& callInfo);

    
    InliningStatus inlineAtomicsCompareExchange(CallInfo& callInfo);
    InliningStatus inlineAtomicsLoad(CallInfo& callInfo);
    InliningStatus inlineAtomicsStore(CallInfo& callInfo);
    InliningStatus inlineAtomicsFence(CallInfo& callInfo);
    InliningStatus inlineAtomicsBinop(CallInfo& callInfo, JSFunction* target);

    
    InliningStatus inlineUnsafePutElements(CallInfo& callInfo);
    bool inlineUnsafeSetDenseArrayElement(CallInfo& callInfo, uint32_t base);
    bool inlineUnsafeSetTypedArrayElement(CallInfo& callInfo, uint32_t base,
                                          ScalarTypeDescr::Type arrayType);
    bool inlineUnsafeSetTypedObjectArrayElement(CallInfo& callInfo, uint32_t base,
                                                ScalarTypeDescr::Type arrayType);

    
    InliningStatus inlineUnsafeSetReservedSlot(CallInfo& callInfo);
    InliningStatus inlineUnsafeGetReservedSlot(CallInfo& callInfo,
                                               MIRType knownValueType);

    
    enum WrappingBehavior { AllowWrappedTypedArrays, RejectWrappedTypedArrays };
    InliningStatus inlineIsTypedArrayHelper(CallInfo& callInfo, WrappingBehavior wrappingBehavior);
    InliningStatus inlineIsTypedArray(CallInfo& callInfo);
    InliningStatus inlineIsPossiblyWrappedTypedArray(CallInfo& callInfo);
    InliningStatus inlineTypedArrayLength(CallInfo& callInfo);
    InliningStatus inlineSetDisjointTypedElements(CallInfo& callInfo);

    
    InliningStatus inlineObjectIsTypeDescr(CallInfo& callInfo);
    InliningStatus inlineSetTypedObjectOffset(CallInfo& callInfo);
    bool elementAccessIsTypedObjectArrayOfScalarType(MDefinition* obj, MDefinition* id,
                                                     ScalarTypeDescr::Type* arrayType);
    InliningStatus inlineConstructTypedObject(CallInfo& callInfo, TypeDescr* target);

    
    InliningStatus inlineConstructSimdObject(CallInfo& callInfo, SimdTypeDescr* target);

    
    static MIRType SimdTypeDescrToMIRType(SimdTypeDescr::Type type);
    bool checkInlineSimd(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type,
                         unsigned numArgs, InlineTypedObject** templateObj);
    IonBuilder::InliningStatus boxSimd(CallInfo& callInfo, MInstruction* ins,
                                       InlineTypedObject* templateObj);

    template <typename T>
    InliningStatus inlineBinarySimd(CallInfo& callInfo, JSNative native,
                                    typename T::Operation op, SimdTypeDescr::Type type);
    InliningStatus inlineCompSimd(CallInfo& callInfo, JSNative native,
                                  MSimdBinaryComp::Operation op, SimdTypeDescr::Type compType);
    InliningStatus inlineUnarySimd(CallInfo& callInfo, JSNative native,
                                   MSimdUnaryArith::Operation op, SimdTypeDescr::Type type);
    InliningStatus inlineSimdWith(CallInfo& callInfo, JSNative native, SimdLane lane,
                                  SimdTypeDescr::Type type);
    InliningStatus inlineSimdSplat(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type);
    InliningStatus inlineSimdShuffle(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type,
                                     unsigned numVectors, unsigned numLanes);
    InliningStatus inlineSimdCheck(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type);
    InliningStatus inlineSimdConvert(CallInfo& callInfo, JSNative native, bool isCast,
                                     SimdTypeDescr::Type from, SimdTypeDescr::Type to);
    InliningStatus inlineSimdSelect(CallInfo& callInfo, JSNative native, bool isElementWise,
                                    SimdTypeDescr::Type type);

    bool prepareForSimdLoadStore(CallInfo& callInfo, Scalar::Type simdType, MInstruction** elements,
                                 MDefinition** index, Scalar::Type* arrayType);
    InliningStatus inlineSimdLoad(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type,
                                  unsigned numElems);
    InliningStatus inlineSimdStore(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type,
                                   unsigned numElems);

    InliningStatus inlineSimdBool(CallInfo& callInfo, JSNative native, SimdTypeDescr::Type type);

    
    InliningStatus inlineIsCallable(CallInfo& callInfo);
    InliningStatus inlineIsObject(CallInfo& callInfo);
    InliningStatus inlineToObject(CallInfo& callInfo);
    InliningStatus inlineToInteger(CallInfo& callInfo);
    InliningStatus inlineToString(CallInfo& callInfo);
    InliningStatus inlineDump(CallInfo& callInfo);
    InliningStatus inlineHasClass(CallInfo& callInfo, const Class* clasp,
                                  const Class* clasp2 = nullptr,
                                  const Class* clasp3 = nullptr,
                                  const Class* clasp4 = nullptr);
    InliningStatus inlineIsConstructing(CallInfo& callInfo);
    InliningStatus inlineSubstringKernel(CallInfo& callInfo);

    
    InliningStatus inlineBailout(CallInfo& callInfo);
    InliningStatus inlineAssertFloat32(CallInfo& callInfo);
    InliningStatus inlineAssertRecoveredOnBailout(CallInfo& callInfo);
    InliningStatus inlineTrue(CallInfo& callInfo);

    
    InliningStatus inlineBoundFunction(CallInfo& callInfo, JSFunction* target);

    
    InliningStatus inlineNativeCall(CallInfo& callInfo, JSFunction* target);
    InliningStatus inlineNativeGetter(CallInfo& callInfo, JSFunction* target);
    InliningStatus inlineNonFunctionCall(CallInfo& callInfo, JSObject* target);
    bool inlineScriptedCall(CallInfo& callInfo, JSFunction* target);
    InliningStatus inlineSingleCall(CallInfo& callInfo, JSObject* target);

    
    InliningStatus inlineCallsite(const ObjectVector& targets, CallInfo& callInfo);
    bool inlineCalls(CallInfo& callInfo, const ObjectVector& targets, BoolVector& choiceSet,
                     MGetPropertyCache* maybeCache);

    
    bool inlineGenericFallback(JSFunction* target, CallInfo& callInfo, MBasicBlock* dispatchBlock);
    bool inlineObjectGroupFallback(CallInfo& callInfo, MBasicBlock* dispatchBlock,
                                   MObjectGroupDispatch* dispatch, MGetPropertyCache* cache,
                                   MBasicBlock** fallbackTarget);

    enum AtomicCheckResult {
        DontCheckAtomicResult,
        DoCheckAtomicResult
    };

    bool atomicsMeetsPreconditions(CallInfo& callInfo, Scalar::Type* arrayElementType,
                                   AtomicCheckResult checkResult=DoCheckAtomicResult);
    void atomicsCheckBounds(CallInfo& callInfo, MInstruction** elements, MDefinition** index);

    bool testNeedsArgumentCheck(JSFunction* target, CallInfo& callInfo);

    MCall* makeCallHelper(JSFunction* target, CallInfo& callInfo);
    bool makeCall(JSFunction* target, CallInfo& callInfo);

    MDefinition* patchInlinedReturn(CallInfo& callInfo, MBasicBlock* exit, MBasicBlock* bottom);
    MDefinition* patchInlinedReturns(CallInfo& callInfo, MIRGraphReturns& returns,
                                     MBasicBlock* bottom);
    MDefinition* specializeInlinedReturn(MDefinition* rdef, MBasicBlock* exit);

    bool objectsHaveCommonPrototype(TemporaryTypeSet* types, PropertyName* name,
                                    bool isGetter, JSObject* foundProto, bool* guardGlobal);
    void freezePropertiesForCommonPrototype(TemporaryTypeSet* types, PropertyName* name,
                                            JSObject* foundProto, bool allowEmptyTypesForGlobal = false);
    


    bool testCommonGetterSetter(TemporaryTypeSet* types, PropertyName* name,
                                bool isGetter, JSObject* foundProto, Shape* lastProperty,
                                JSFunction* getterOrSetter,
                                MDefinition** guard, Shape* globalShape = nullptr,
                                MDefinition** globalGuard = nullptr);
    bool testShouldDOMCall(TypeSet* inTypes,
                           JSFunction* func, JSJitInfo::OpType opType);

    MDefinition*
    addShapeGuardsForGetterSetter(MDefinition* obj, JSObject* holder, Shape* holderShape,
                                  const BaselineInspector::ReceiverVector& receivers,
                                  const BaselineInspector::ObjectGroupVector& convertUnboxedGroups,
                                  bool isOwnProperty);

    bool annotateGetPropertyCache(MDefinition* obj, MGetPropertyCache* getPropCache,
                                  TemporaryTypeSet* objTypes,
                                  TemporaryTypeSet* pushedTypes);

    MGetPropertyCache* getInlineableGetPropertyCache(CallInfo& callInfo);

    JSObject* testSingletonProperty(JSObject* obj, PropertyName* name);
    bool testSingletonPropertyTypes(MDefinition* obj, JSObject* singleton, PropertyName* name,
                                    bool* testObject, bool* testString);
    uint32_t getDefiniteSlot(TemporaryTypeSet* types, PropertyName* name, uint32_t* pnfixed,
                             BaselineInspector::ObjectGroupVector& convertUnboxedGroups);
    MDefinition* convertUnboxedObjects(MDefinition* obj,
                                       const BaselineInspector::ObjectGroupVector& list);
    uint32_t getUnboxedOffset(TemporaryTypeSet* types, PropertyName* name,
                              JSValueType* punboxedType);
    MInstruction* loadUnboxedProperty(MDefinition* obj, size_t offset, JSValueType unboxedType,
                                      BarrierKind barrier, TemporaryTypeSet* types);
    MInstruction* storeUnboxedProperty(MDefinition* obj, size_t offset, JSValueType unboxedType,
                                       MDefinition* value);
    bool freezePropTypeSets(TemporaryTypeSet* types,
                            JSObject* foundProto, PropertyName* name);
    bool canInlinePropertyOpShapes(const BaselineInspector::ReceiverVector& receivers);

    TemporaryTypeSet* bytecodeTypes(jsbytecode* pc);

    
    
    

    bool setCurrentAndSpecializePhis(MBasicBlock* block) {
        if (block) {
            if (!block->specializePhis())
                return false;
        }
        setCurrent(block);
        return true;
    }

    void setCurrent(MBasicBlock* block) {
        current = block;
    }

    
    JSScript* script_;

    
    
    
    
    CodeGenerator* backgroundCodegen_;

    
    
    
    
    JSScript* actionableAbortScript_;
    jsbytecode* actionableAbortPc_;
    const char* actionableAbortMessage_;

  public:
    void clearForBackEnd();

    JSScript* script() const { return script_; }

    CodeGenerator* backgroundCodegen() const { return backgroundCodegen_; }
    void setBackgroundCodegen(CodeGenerator* codegen) { backgroundCodegen_ = codegen; }

    CompilerConstraintList* constraints() {
        return constraints_;
    }

    bool isInlineBuilder() const {
        return callerBuilder_ != nullptr;
    }

    const JSAtomState& names() { return compartment->runtime()->names(); }

    bool hadActionableAbort() const {
        MOZ_ASSERT(!actionableAbortScript_ ||
                   (actionableAbortPc_ && actionableAbortMessage_));
        return actionableAbortScript_ != nullptr;
    }

    void actionableAbortLocationAndMessage(JSScript** abortScript, jsbytecode** abortPc,
                                           const char** abortMessage)
    {
        MOZ_ASSERT(hadActionableAbort());
        *abortScript = actionableAbortScript_;
        *abortPc = actionableAbortPc_;
        *abortMessage = actionableAbortMessage_;
    }

  private:
    bool init();

    JSContext* analysisContext;
    BaselineFrameInspector* baselineFrame_;

    
    CompilerConstraintList* constraints_;

    
    BytecodeAnalysis analysis_;
    BytecodeAnalysis& analysis() {
        return analysis_;
    }

    TemporaryTypeSet* thisTypes;
    TemporaryTypeSet* argTypes;
    TemporaryTypeSet* typeArray;
    uint32_t typeArrayHint;
    uint32_t* bytecodeTypeMap;

    GSNCache gsn;
    ScopeCoordinateNameCache scopeCoordinateNameCache;

    jsbytecode* pc;
    MBasicBlock* current;
    uint32_t loopDepth_;

    Vector<BytecodeSite*, 0, JitAllocPolicy> trackedOptimizationSites_;

    BytecodeSite* bytecodeSite(jsbytecode* pc) {
        MOZ_ASSERT(info().inlineScriptTree()->script()->containsPC(pc));
        
        if (isOptimizationTrackingEnabled()) {
            if (BytecodeSite* site = maybeTrackedOptimizationSite(pc))
                return site;
        }
        return new(alloc()) BytecodeSite(info().inlineScriptTree(), pc);
    }

    BytecodeSite* maybeTrackedOptimizationSite(jsbytecode* pc);

    MDefinition* lexicalCheck_;

    void setLexicalCheck(MDefinition* lexical) {
        MOZ_ASSERT(!lexicalCheck_);
        lexicalCheck_ = lexical;
    }
    MDefinition* takeLexicalCheck() {
        MDefinition* lexical = lexicalCheck_;
        lexicalCheck_ = nullptr;
        return lexical;
    }

    
    MResumePoint* callerResumePoint_;
    jsbytecode* callerPC() {
        return callerResumePoint_ ? callerResumePoint_->pc() : nullptr;
    }
    IonBuilder* callerBuilder_;

    IonBuilder* outermostBuilder();

    bool oom() {
        abortReason_ = AbortReason_Alloc;
        return false;
    }

    struct LoopHeader {
        jsbytecode* pc;
        MBasicBlock* header;

        LoopHeader(jsbytecode* pc, MBasicBlock* header)
          : pc(pc), header(header)
        {}
    };

    Vector<CFGState, 8, JitAllocPolicy> cfgStack_;
    Vector<ControlFlowInfo, 4, JitAllocPolicy> loops_;
    Vector<ControlFlowInfo, 0, JitAllocPolicy> switches_;
    Vector<ControlFlowInfo, 2, JitAllocPolicy> labels_;
    Vector<MInstruction*, 2, JitAllocPolicy> iterators_;
    Vector<LoopHeader, 0, JitAllocPolicy> loopHeaders_;
    BaselineInspector* inspector;

    size_t inliningDepth_;

    
    
    size_t inlinedBytecodeLength_;

    
    
    static const size_t MAX_LOOP_RESTARTS = 40;
    size_t numLoopRestarts_;

    
    
    bool failedBoundsCheck_;

    
    
    bool failedShapeGuard_;

    
    
    bool failedLexicalCheck_;

    
    bool nonStringIteration_;

    
    
    MInstruction* lazyArguments_;

    
    const CallInfo* inlineCallInfo_;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    void replaceMaybeFallbackFunctionGetter(MGetPropertyCache* cache);

    
    void keepFallbackFunctionGetter(MGetPropertyCache* cache) {
        if (cache == maybeFallbackFunctionGetter_)
            maybeFallbackFunctionGetter_ = nullptr;
    }

    MGetPropertyCache* maybeFallbackFunctionGetter_;

    
    void startTrackingOptimizations();

    
    
    
    void trackTypeInfo(JS::TrackedTypeSite site, MIRType mirType,
                       TemporaryTypeSet* typeSet)
    {
        if (MOZ_UNLIKELY(current->trackedSite()->hasOptimizations()))
            trackTypeInfoUnchecked(site, mirType, typeSet);
    }
    void trackTypeInfo(JS::TrackedTypeSite site, JSObject* obj) {
        if (MOZ_UNLIKELY(current->trackedSite()->hasOptimizations()))
            trackTypeInfoUnchecked(site, obj);
    }
    void trackTypeInfo(CallInfo& callInfo) {
        if (MOZ_UNLIKELY(current->trackedSite()->hasOptimizations()))
            trackTypeInfoUnchecked(callInfo);
    }
    void trackOptimizationAttempt(JS::TrackedStrategy strategy) {
        if (MOZ_UNLIKELY(current->trackedSite()->hasOptimizations()))
            trackOptimizationAttemptUnchecked(strategy);
    }
    void amendOptimizationAttempt(uint32_t index) {
        if (MOZ_UNLIKELY(current->trackedSite()->hasOptimizations()))
            amendOptimizationAttemptUnchecked(index);
    }
    void trackOptimizationOutcome(JS::TrackedOutcome outcome) {
        if (MOZ_UNLIKELY(current->trackedSite()->hasOptimizations()))
            trackOptimizationOutcomeUnchecked(outcome);
    }
    void trackOptimizationSuccess() {
        if (MOZ_UNLIKELY(current->trackedSite()->hasOptimizations()))
            trackOptimizationSuccessUnchecked();
    }
    void trackInlineSuccess(InliningStatus status = InliningStatus_Inlined) {
        if (MOZ_UNLIKELY(current->trackedSite()->hasOptimizations()))
            trackInlineSuccessUnchecked(status);
    }

    
    
    void trackTypeInfoUnchecked(JS::TrackedTypeSite site, MIRType mirType,
                                TemporaryTypeSet* typeSet);
    void trackTypeInfoUnchecked(JS::TrackedTypeSite site, JSObject* obj);
    void trackTypeInfoUnchecked(CallInfo& callInfo);
    void trackOptimizationAttemptUnchecked(JS::TrackedStrategy strategy);
    void amendOptimizationAttemptUnchecked(uint32_t index);
    void trackOptimizationOutcomeUnchecked(JS::TrackedOutcome outcome);
    void trackOptimizationSuccessUnchecked();
    void trackInlineSuccessUnchecked(InliningStatus status);
};

class CallInfo
{
    MDefinition* fun_;
    MDefinition* thisArg_;
    MDefinitionVector args_;

    bool constructing_;
    bool setter_;

  public:
    CallInfo(TempAllocator& alloc, bool constructing)
      : fun_(nullptr),
        thisArg_(nullptr),
        args_(alloc),
        constructing_(constructing),
        setter_(false)
    { }

    bool init(CallInfo& callInfo) {
        MOZ_ASSERT(constructing_ == callInfo.constructing());

        fun_ = callInfo.fun();
        thisArg_ = callInfo.thisArg();

        if (!args_.appendAll(callInfo.argv()))
            return false;

        return true;
    }

    bool init(MBasicBlock* current, uint32_t argc) {
        MOZ_ASSERT(args_.empty());

        
        if (!args_.reserve(argc))
            return false;
        for (int32_t i = argc; i > 0; i--)
            args_.infallibleAppend(current->peek(-i));
        current->popn(argc);

        
        setThis(current->pop());
        setFun(current->pop());

        return true;
    }

    void popFormals(MBasicBlock* current) {
        current->popn(numFormals());
    }

    void pushFormals(MBasicBlock* current) {
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

    bool setArgs(const MDefinitionVector& args) {
        MOZ_ASSERT(args_.empty());
        return args_.appendAll(args);
    }

    MDefinitionVector& argv() {
        return args_;
    }

    const MDefinitionVector& argv() const {
        return args_;
    }

    MDefinition* getArg(uint32_t i) const {
        MOZ_ASSERT(i < argc());
        return args_[i];
    }

    MDefinition* getArgWithDefault(uint32_t i, MDefinition* defaultValue) const {
        if (i < argc())
            return args_[i];

        return defaultValue;
    }

    void setArg(uint32_t i, MDefinition* def) {
        MOZ_ASSERT(i < argc());
        args_[i] = def;
    }

    MDefinition* thisArg() const {
        MOZ_ASSERT(thisArg_);
        return thisArg_;
    }

    void setThis(MDefinition* thisArg) {
        thisArg_ = thisArg;
    }

    bool constructing() const {
        return constructing_;
    }

    bool isSetter() const {
        return setter_;
    }
    void markAsSetter() {
        setter_ = true;
    }

    MDefinition* fun() const {
        MOZ_ASSERT(fun_);
        return fun_;
    }

    void setFun(MDefinition* fun) {
        fun_ = fun;
    }

    void setImplicitlyUsedUnchecked() {
        fun_->setImplicitlyUsedUnchecked();
        thisArg_->setImplicitlyUsedUnchecked();
        for (uint32_t i = 0; i < argc(); i++)
            getArg(i)->setImplicitlyUsedUnchecked();
    }
};

bool NeedsPostBarrier(CompileInfo& info, MDefinition* value);

} 
} 

#endif 
