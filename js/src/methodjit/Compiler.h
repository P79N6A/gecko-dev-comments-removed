





#if !defined jsjaeger_compiler_h__ && defined JS_METHODJIT
#define jsjaeger_compiler_h__

#include "jsanalyze.h"
#include "jscntxt.h"
#include "MethodJIT.h"
#include "CodeGenIncludes.h"
#include "BaseCompiler.h"
#include "StubCompiler.h"
#include "MonoIC.h"
#include "PolyIC.h"

namespace js {
namespace mjit {





struct InvariantCodePatch {
    bool hasPatch;
    JSC::MacroAssembler::DataLabelPtr codePatch;
    InvariantCodePatch() : hasPatch(false) {}
};

struct JSActiveFrame {
    JSActiveFrame *parent;
    jsbytecode *parentPC;
    JSScript *script;

    



    uint32_t inlineIndex;

    
    size_t mainCodeStart;
    size_t stubCodeStart;
    size_t mainCodeEnd;
    size_t stubCodeEnd;
    size_t inlinePCOffset;

    JSActiveFrame();
};

class Compiler : public BaseCompiler
{
    friend class StubCompiler;

    struct BranchPatch {
        BranchPatch(const Jump &j, jsbytecode *pc, uint32_t inlineIndex)
          : jump(j), pc(pc), inlineIndex(inlineIndex)
        { }

        Jump jump;
        jsbytecode *pc;
        uint32_t inlineIndex;
    };

#if defined JS_MONOIC
    struct GlobalNameICInfo {
        Label fastPathStart;
        Call slowPathCall;
        DataLabelPtr shape;
        DataLabelPtr addrLabel;

        void copyTo(ic::GlobalNameIC &to, JSC::LinkBuffer &full, JSC::LinkBuffer &stub) {
            to.fastPathStart = full.locationOf(fastPathStart);

            int offset = full.locationOf(shape) - to.fastPathStart;
            to.shapeOffset = offset;
            JS_ASSERT(to.shapeOffset == offset);

            to.slowPathCall = stub.locationOf(slowPathCall);
        }
    };

    struct GetGlobalNameICInfo : public GlobalNameICInfo {
        Label load;
    };

    struct SetGlobalNameICInfo : public GlobalNameICInfo {
        Label slowPathStart;
        Label fastPathRejoin;
        DataLabel32 store;
        Jump shapeGuardJump;
        ValueRemat vr;
        RegisterID objReg;
        RegisterID shapeReg;
        bool objConst;
    };

    struct EqualityGenInfo {
        DataLabelPtr addrLabel;
        Label stubEntry;
        Call stubCall;
        BoolStub stub;
        MaybeJump jumpToStub;
        Label fallThrough;
        jsbytecode *jumpTarget;
        bool trampoline;
        Label trampolineStart;
        ValueRemat lvr, rvr;
        Assembler::Condition cond;
        JSC::MacroAssembler::RegisterID tempReg;
    };

    
  public:
    struct CallGenInfo {
        



        uint32_t     callIndex;
        Label        funGuardLabel;
        DataLabelPtr funGuard;
        Jump         funJump;
        Jump         hotJump;
        Call         oolCall;
        Label        joinPoint;
        Label        slowJoinPoint;
        Label        slowPathStart;
        Label        hotPathLabel;
        Label        ionJoinPoint;
        DataLabelPtr addrLabel1;
        DataLabelPtr addrLabel2;
        Jump         oolJump;
        Label        icCall;
        RegisterID   funObjReg;
        FrameSize    frameSize;
        bool         typeMonitored;
    };

  private:
#endif





    struct CallPatchInfo {
        CallPatchInfo() : hasFastNcode(false), hasSlowNcode(false), joinSlow(false) {}
        Label joinPoint;
        DataLabelPtr fastNcodePatch;
        DataLabelPtr slowNcodePatch;
        bool hasFastNcode;
        bool hasSlowNcode;
        bool joinSlow;
    };

    struct BaseICInfo {
        BaseICInfo() : canCallHook(false), forcedTypeBarrier(false)
        { }
        Label fastPathStart;
        Label fastPathRejoin;
        Label slowPathStart;
        Call slowPathCall;
        DataLabelPtr paramAddr;
        bool canCallHook;
        bool forcedTypeBarrier;

        void copyTo(ic::BaseIC &to, JSC::LinkBuffer &full, JSC::LinkBuffer &stub) {
            to.fastPathStart = full.locationOf(fastPathStart);
            to.fastPathRejoin = full.locationOf(fastPathRejoin);
            to.slowPathStart = stub.locationOf(slowPathStart);
            to.slowPathCall = stub.locationOf(slowPathCall);
            to.canCallHook = canCallHook;
            to.forcedTypeBarrier = forcedTypeBarrier;
        }
    };

    struct GetElementICInfo : public BaseICInfo {
        RegisterID  typeReg;
        RegisterID  objReg;
        ValueRemat  id;
        MaybeJump   typeGuard;
        Jump        shapeGuard;
    };

    struct SetElementICInfo : public BaseICInfo {
        RegisterID  objReg;
        StateRemat  objRemat;
        ValueRemat  vr;
        Jump        capacityGuard;
        Jump        shapeGuard;
        Jump        holeGuard;
        Int32Key    key;
        uint32_t    volatileMask;
    };

    struct PICGenInfo : public BaseICInfo {
        PICGenInfo(ic::PICInfo::Kind kind, jsbytecode *pc)
          : kind(kind), pc(pc), typeMonitored(false)
        { }
        ic::PICInfo::Kind kind;
        Label typeCheck;
        RegisterID shapeReg;
        RegisterID objReg;
        RegisterID typeReg;
        Label shapeGuard;
        jsbytecode *pc;
        PropertyName *name;
        bool hasTypeCheck;
        bool typeMonitored;
        bool cached;
        ValueRemat vr;
        union {
            ic::GetPropLabels getPropLabels_;
            ic::SetPropLabels setPropLabels_;
            ic::BindNameLabels bindNameLabels_;
            ic::ScopeNameLabels scopeNameLabels_;
        };

        ic::GetPropLabels &getPropLabels() {
            JS_ASSERT(kind == ic::PICInfo::GET);
            return getPropLabels_;
        }
        ic::SetPropLabels &setPropLabels() {
            JS_ASSERT(kind == ic::PICInfo::SET);
            return setPropLabels_;
        }
        ic::BindNameLabels &bindNameLabels() {
            JS_ASSERT(kind == ic::PICInfo::BIND);
            return bindNameLabels_;
        }
        ic::ScopeNameLabels &scopeNameLabels() {
            JS_ASSERT(kind == ic::PICInfo::NAME ||
                      kind == ic::PICInfo::XNAME);
            return scopeNameLabels_;
        }

        void copySimpleMembersTo(ic::PICInfo &ic) {
            ic.kind = kind;
            ic.shapeReg = shapeReg;
            ic.objReg = objReg;
            ic.name = name;
            if (ic.isSet()) {
                ic.u.vr = vr;
            } else if (ic.isGet()) {
                ic.u.get.typeReg = typeReg;
                ic.u.get.hasTypeCheck = hasTypeCheck;
            }
            ic.typeMonitored = typeMonitored;
            ic.cached = cached;
            if (ic.isGet())
                ic.setLabels(getPropLabels());
            else if (ic.isSet())
                ic.setLabels(setPropLabels());
            else if (ic.isBind())
                ic.setLabels(bindNameLabels());
            else if (ic.isScopeName())
                ic.setLabels(scopeNameLabels());
        }

    };

    struct Defs {
        Defs(uint32_t ndefs)
          : ndefs(ndefs)
        { }
        uint32_t ndefs;
    };

    struct InternalCallSite {
        uint32_t returnOffset;
        DataLabelPtr inlinePatch;
        uint32_t inlineIndex;
        jsbytecode *inlinepc;
        RejoinState rejoin;
        bool ool;
        Label loopJumpLabel;
        InvariantCodePatch loopPatch;

        InternalCallSite(uint32_t returnOffset,
                         uint32_t inlineIndex, jsbytecode *inlinepc,
                         RejoinState rejoin, bool ool)
          : returnOffset(returnOffset),
            inlineIndex(inlineIndex), inlinepc(inlinepc),
            rejoin(rejoin), ool(ool)
        { }
    };

    struct InternalCompileTrigger {
        jsbytecode *pc;
        Jump inlineJump;
        Label stubLabel;
    };

    struct DoublePatch {
        double d;
        DataLabelPtr label;
        bool ool;
    };

    struct JumpTable {
        DataLabelPtr label;
        size_t offsetIndex;
    };

    struct JumpTableEdge {
        uint32_t source;
        uint32_t target;
    };

    struct ChunkJumpTableEdge {
        JumpTableEdge edge;
        void **jumpTableEntry;
    };

    struct LoopEntry {
        uint32_t pcOffset;
        Label label;
    };

    




    class VarType {
        JSValueType type;
        types::StackTypeSet *types;

      public:
        void setTypes(types::StackTypeSet *types) {
            this->types = types;
            this->type = JSVAL_TYPE_MISSING;
        }

        types::TypeSet *getTypes() { return types; }

        JSValueType getTypeTag() {
            if (type == JSVAL_TYPE_MISSING)
                type = types ? types->getKnownTypeTag() : JSVAL_TYPE_UNKNOWN;
            return type;
        }
    };

    struct OutgoingChunkEdge {
        uint32_t source;
        uint32_t target;

#ifdef JS_CPU_X64
        Label sourceTrampoline;
#endif

        Jump fastJump;
        MaybeJump slowJump;
    };

    struct SlotType
    {
        uint32_t slot;
        VarType vt;
        SlotType(uint32_t slot, VarType vt) : slot(slot), vt(vt) {}
    };

    RootedScript outerScript;
    unsigned chunkIndex;
    bool isConstructing;
    ChunkDescriptor outerChunk;

    
    analyze::CrossScriptSSA ssa;

    Rooted<GlobalObject*> globalObj;
    const HeapSlot *globalSlots;  

    MJITInstrumentation sps;
    Assembler masm;
    FrameState frame;

    




public:
    struct ActiveFrame : public JSActiveFrame {
        Label *jumpMap;

        
        VarType *varTypes;

        
        bool needReturnValue;          
        bool syncReturnValue;          
        bool returnValueDouble;        
        bool returnSet;                
        AnyRegisterID returnRegister;  
        const FrameEntry *returnEntry; 
        Vector<Jump, 4, CompilerAllocPolicy> *returnJumps;

        



        RegisterAllocation *exitState;

        ActiveFrame(JSContext *cx);
        ~ActiveFrame();
    };

private:
    ActiveFrame *a;
    ActiveFrame *outer;

    RootedScript script_;
    analyze::ScriptAnalysis *analysis;
    jsbytecode *PC;

    LoopState *loop;

    

    js::Vector<ActiveFrame*, 4, CompilerAllocPolicy> inlineFrames;
    js::Vector<BranchPatch, 64, CompilerAllocPolicy> branchPatches;
#if defined JS_MONOIC
    js::Vector<GetGlobalNameICInfo, 16, CompilerAllocPolicy> getGlobalNames;
    js::Vector<SetGlobalNameICInfo, 16, CompilerAllocPolicy> setGlobalNames;
    js::Vector<CallGenInfo, 64, CompilerAllocPolicy> callICs;
    js::Vector<EqualityGenInfo, 64, CompilerAllocPolicy> equalityICs;
#endif
#if defined JS_POLYIC
    js::Vector<PICGenInfo, 16, CompilerAllocPolicy> pics;
    js::Vector<GetElementICInfo, 16, CompilerAllocPolicy> getElemICs;
    js::Vector<SetElementICInfo, 16, CompilerAllocPolicy> setElemICs;
#endif
    js::Vector<CallPatchInfo, 64, CompilerAllocPolicy> callPatches;
    js::Vector<InternalCallSite, 64, CompilerAllocPolicy> callSites;
    js::Vector<InternalCompileTrigger, 4, CompilerAllocPolicy> compileTriggers;
    js::Vector<DoublePatch, 16, CompilerAllocPolicy> doubleList;
    js::Vector<JSObject*, 0, CompilerAllocPolicy> rootedTemplates;
    js::Vector<RegExpShared*, 0, CompilerAllocPolicy> rootedRegExps;
    js::Vector<uint32_t> monitoredBytecodes;
    js::Vector<uint32_t> typeBarrierBytecodes;
    js::Vector<uint32_t> fixedIntToDoubleEntries;
    js::Vector<uint32_t> fixedDoubleToAnyEntries;
    js::Vector<JumpTable, 16> jumpTables;
    js::Vector<JumpTableEdge, 16> jumpTableEdges;
    js::Vector<LoopEntry, 16> loopEntries;
    js::Vector<OutgoingChunkEdge, 16> chunkEdges;
    StubCompiler stubcc;
    Label fastEntryLabel;
    Label arityLabel;
    Label argsCheckLabel;
#ifdef JS_MONOIC
    Label argsCheckStub;
    Label argsCheckFallthrough;
    Jump argsCheckJump;
#endif
    bool debugMode_;
    bool inlining_;
    bool hasGlobalReallocation;
    bool oomInVector;       
    bool overflowICSpace;   
    uint64_t gcNumber;
    PCLengthEntry *pcLengths;

    Compiler *thisFromCtor() { return this; }

    friend class CompilerAllocPolicy;
  public:
    Compiler(JSContext *cx, JSScript *outerScript, unsigned chunkIndex, bool isConstructing);
    ~Compiler();

    CompileStatus compile();

    Label getLabel() { return masm.label(); }
    bool knownJump(jsbytecode *pc);
    Label labelOf(jsbytecode *target, uint32_t inlineIndex);
    void addCallSite(const InternalCallSite &callSite);
    void addReturnSite();
    void inlineStubCall(void *stub, RejoinState rejoin, Uses uses);

    bool debugMode() { return debugMode_; }
    bool inlining() { return inlining_; }
    bool constructing() { return isConstructing; }

    jsbytecode *outerPC() {
        if (a == outer)
            return PC;
        ActiveFrame *scan = a;
        while (scan && scan->parent != outer)
            scan = static_cast<ActiveFrame *>(scan->parent);
        return scan->parentPC;
    }

    JITScript *outerJIT() {
        return outerScript->getJIT(isConstructing, cx->zone()->compileBarriers());
    }

    ChunkDescriptor &outerChunkRef() {
        return outerJIT()->chunkDescriptor(chunkIndex);
    }

    bool bytecodeInChunk(jsbytecode *pc) {
        return (unsigned(pc - outerScript->code) >= outerChunk.begin)
            && (unsigned(pc - outerScript->code) < outerChunk.end);
    }

    jsbytecode *inlinePC() { return PC; }
    uint32_t inlineIndex() { return a->inlineIndex; }

    Assembler &getAssembler(bool ool) { return ool ? stubcc.masm : masm; }

    InvariantCodePatch *getInvariantPatch(unsigned index) {
        return &callSites[index].loopPatch;
    }
    jsbytecode *getInvariantPC(unsigned index) {
        return callSites[index].inlinepc;
    }

    bool activeFrameHasMultipleExits() {
        ActiveFrame *na = a;
        while (na->parent) {
            if (na->exitState)
                return true;
            na = static_cast<ActiveFrame *>(na->parent);
        }
        return false;
    }

  private:
    CompileStatus performCompilation();
    CompileStatus generatePrologue();
    CompileStatus generateMethod();
    CompileStatus generateEpilogue();
    CompileStatus finishThisUp();
    CompileStatus pushActiveFrame(JSScript *script, uint32_t argc);
    void popActiveFrame();
    void updatePCCounts(jsbytecode *pc, bool *updated);
    void updatePCTypes(jsbytecode *pc, FrameEntry *fe);
    void updateArithCounts(jsbytecode *pc, FrameEntry *fe,
                             JSValueType firstUseType, JSValueType secondUseType);
    void updateElemCounts(jsbytecode *pc, FrameEntry *obj, FrameEntry *id);
    void bumpPropCount(jsbytecode *pc, int count);

    
    CompileStatus prepareInferenceTypes(JSScript *script, ActiveFrame *a);
    void ensureDoubleArguments();
    void markUndefinedLocal(uint32_t offset, uint32_t i);
    void markUndefinedLocals();
    void fixDoubleTypes(jsbytecode *target);
    bool watchGlobalReallocation();
    void updateVarType();
    void updateJoinVarTypes();
    void restoreVarType();
    JSValueType knownPushedType(uint32_t pushed);
    bool mayPushUndefined(uint32_t pushed);
    types::StackTypeSet *pushedTypeSet(uint32_t which);
    bool monitored(jsbytecode *pc);
    bool hasTypeBarriers(jsbytecode *pc);
    bool testSingletonProperty(HandleObject obj, HandleId id);
    bool testSingletonPropertyTypes(FrameEntry *top, HandleId id, bool *testObject);
    CompileStatus addInlineFrame(HandleScript script, uint32_t depth, uint32_t parent, jsbytecode *parentpc);
    CompileStatus scanInlineCalls(uint32_t index, uint32_t depth);
    CompileStatus checkAnalysis(HandleScript script);

    struct BarrierState {
        MaybeJump jump;
        RegisterID typeReg;
        RegisterID dataReg;
    };

    MaybeJump trySingleTypeTest(types::StackTypeSet *types, RegisterID typeReg);
    Jump addTypeTest(types::StackTypeSet *types, RegisterID typeReg, RegisterID dataReg);
    BarrierState pushAddressMaybeBarrier(Address address, JSValueType type, bool reuseBase,
                                         bool testUndefined = false);
    BarrierState testBarrier(RegisterID typeReg, RegisterID dataReg,
                             bool testUndefined = false, bool testReturn = false,
                             bool force = false);
    void finishBarrier(const BarrierState &barrier, RejoinState rejoin, uint32_t which);

    void testPushedType(RejoinState rejoin, int which, bool ool = true);

    
    void pushSyncedEntry(uint32_t pushed);
    bool jumpInScript(Jump j, jsbytecode *pc);
    bool compareTwoValues(JSContext *cx, JSOp op, const Value &lhs, const Value &rhs);

    
    bool constantFoldBranch(jsbytecode *target, bool taken);
    bool emitStubCmpOp(BoolStub stub, jsbytecode *target, JSOp fused);
    bool iter(unsigned flags);
    void iterNext();
    bool iterMore(jsbytecode *target);
    void iterEnd();
    MaybeJump loadDouble(FrameEntry *fe, FPRegisterID *fpReg, bool *allocated);
#ifdef JS_POLYIC
    void passICAddress(BaseICInfo *ic);
#endif
#ifdef JS_MONOIC
    void passMICAddress(GlobalNameICInfo &mic);
#endif
    bool constructThis();
    void ensureDouble(FrameEntry *fe);

    



    void ensureInteger(FrameEntry *fe, Uses uses);

    
    void truncateDoubleToInt32(FrameEntry *fe, Uses uses);

    



    void tryConvertInteger(FrameEntry *fe, Uses uses);

    
    bool jumpAndRun(Jump j, jsbytecode *target,
                    Jump *slow = NULL, bool *trampoline = NULL,
                    bool fallthrough = false);
    bool startLoop(jsbytecode *head, Jump entry, jsbytecode *entryTarget);
    bool finishLoop(jsbytecode *head);
    inline bool shouldStartLoop(jsbytecode *head);
    void jsop_bindname(HandlePropertyName name);
    void jsop_setglobal(uint32_t index);
    void jsop_getprop_slow(HandlePropertyName name, bool forPrototype = false);
    void jsop_aliasedArg(unsigned i, bool get, bool poppedAfter = false);
    void jsop_aliasedVar(ScopeCoordinate sc, bool get, bool poppedAfter = false);
    void jsop_this();
    void emitReturn(FrameEntry *fe);
    void emitFinalReturn(Assembler &masm);
    void loadReturnValue(Assembler *masm, FrameEntry *fe);
    void emitReturnValue(Assembler *masm, FrameEntry *fe);
    void emitInlineReturnValue(FrameEntry *fe);
    void dispatchCall(VoidPtrStubUInt32 stub, uint32_t argc);
    void interruptCheckHelper();
    void ionCompileHelper();
    void inliningCompileHelper();
    CompileStatus methodEntryHelper();
    CompileStatus profilingPushHelper();
    void profilingPopHelper();
    void emitUncachedCall(uint32_t argc, bool callingNew);
    void checkCallApplySpeculation(uint32_t argc, FrameEntry *origCallee, FrameEntry *origThis,
                                   MaybeRegisterID origCalleeType, RegisterID origCalleeData,
                                   MaybeRegisterID origThisType, RegisterID origThisData,
                                   Jump *uncachedCallSlowRejoin, CallPatchInfo *uncachedCallPatch);
    bool inlineCallHelper(uint32_t argc, bool callingNew, FrameSize &callFrameSize);
    void fixPrimitiveReturn(Assembler *masm, FrameEntry *fe);
    bool jsop_getgname(uint32_t index);
    void jsop_getgname_slow(uint32_t index);
    bool jsop_setgname(HandlePropertyName name, bool popGuaranteed);
    void jsop_setgname_slow(HandlePropertyName name);
    void jsop_bindgname();
    void jsop_setelem_slow();
    void jsop_getelem_slow();
    bool jsop_getprop(HandlePropertyName name, JSValueType type,
                      bool typeCheck = true, bool forPrototype = false);
    bool jsop_getprop_dispatch(HandlePropertyName name);
    bool jsop_setprop(HandlePropertyName name, bool popGuaranteed);
    void jsop_setprop_slow(HandlePropertyName name);
    bool jsop_instanceof();
    bool jsop_intrinsic(HandlePropertyName name, JSValueType type);
    void jsop_name(HandlePropertyName name, JSValueType type);
    bool jsop_xname(HandlePropertyName name);
    void enterBlock(StaticBlockObject *block);
    void leaveBlock();
    void emitEval(uint32_t argc);
    bool jsop_tableswitch(jsbytecode *pc);
    Jump getNewObject(JSContext *cx, RegisterID result, JSObject *templateObject);

    
    bool jsop_binary_slow(JSOp op, VoidStub stub, JSValueType type, FrameEntry *lhs, FrameEntry *rhs);
    bool jsop_binary(JSOp op, VoidStub stub, JSValueType type, types::TypeSet *typeSet);
    void jsop_binary_full(FrameEntry *lhs, FrameEntry *rhs, JSOp op, VoidStub stub,
                          JSValueType type, bool cannotOverflow, bool ignoreOverflow);
    void jsop_binary_full_simple(FrameEntry *fe, JSOp op, VoidStub stub,
                                 JSValueType type);
    void jsop_binary_double(FrameEntry *lhs, FrameEntry *rhs, JSOp op, VoidStub stub,
                            JSValueType type);
    void slowLoadConstantDouble(Assembler &masm, FrameEntry *fe,
                                FPRegisterID fpreg);
    void maybeJumpIfNotInt32(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                             MaybeRegisterID &mreg);
    void maybeJumpIfNotDouble(Assembler &masm, MaybeJump &mj, FrameEntry *fe,
                              MaybeRegisterID &mreg);
    bool jsop_relational(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    bool jsop_relational_full(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    bool jsop_relational_double(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    bool jsop_relational_int(JSOp op, jsbytecode *target, JSOp fused);

    void emitLeftDoublePath(FrameEntry *lhs, FrameEntry *rhs, FrameState::BinaryAlloc &regs,
                            MaybeJump &lhsNotDouble, MaybeJump &rhsNotNumber,
                            MaybeJump &lhsUnknownDone);
    void emitRightDoublePath(FrameEntry *lhs, FrameEntry *rhs, FrameState::BinaryAlloc &regs,
                             MaybeJump &rhsNotNumber2);
    bool tryBinaryConstantFold(JSContext *cx, FrameState &frame, JSOp op,
                               FrameEntry *lhs, FrameEntry *rhs, Value *vp);

    
    void jsop_bitop(JSOp op);
    bool jsop_mod();
    void jsop_neg();
    void jsop_bitnot();
    void jsop_not();
    void jsop_typeof();
    bool booleanJumpScript(JSOp op, jsbytecode *target);
    bool jsop_ifneq(JSOp op, jsbytecode *target);
    bool jsop_andor(JSOp op, jsbytecode *target);
    bool jsop_newinit();
    bool jsop_regexp();
    void jsop_initmethod();
    void jsop_initprop();
    void jsop_initelem_array();
    void jsop_setelem_dense(types::StackTypeSet::DoubleConversion conversion);
#ifdef JS_METHODJIT_TYPED_ARRAY
    void jsop_setelem_typed(int atype);
    void convertForTypedArray(int atype, ValueRemat *vr, bool *allocated);
#endif
    bool jsop_setelem(bool popGuaranteed);
    bool jsop_getelem();
    void jsop_getelem_dense(bool isPacked);
    void jsop_getelem_args();
#ifdef JS_METHODJIT_TYPED_ARRAY
    bool jsop_getelem_typed(int atype);
#endif
    void jsop_toid();
    bool isCacheableBaseAndIndex(FrameEntry *obj, FrameEntry *id);
    void jsop_stricteq(JSOp op);
    bool jsop_equality(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    CompileStatus jsop_equality_obj_obj(JSOp op, jsbytecode *target, JSOp fused);
    bool jsop_equality_int_string(JSOp op, BoolStub stub, jsbytecode *target, JSOp fused);
    void jsop_pos();
    void jsop_in();

    static inline Assembler::Condition
    GetCompareCondition(JSOp op, JSOp fused)
    {
        bool ifeq = fused == JSOP_IFEQ;
        switch (op) {
          case JSOP_GT:
            return ifeq ? Assembler::LessThanOrEqual : Assembler::GreaterThan;
          case JSOP_GE:
            return ifeq ? Assembler::LessThan : Assembler::GreaterThanOrEqual;
          case JSOP_LT:
            return ifeq ? Assembler::GreaterThanOrEqual : Assembler::LessThan;
          case JSOP_LE:
            return ifeq ? Assembler::GreaterThan : Assembler::LessThanOrEqual;
          case JSOP_STRICTEQ:
          case JSOP_EQ:
            return ifeq ? Assembler::NotEqual : Assembler::Equal;
          case JSOP_STRICTNE:
          case JSOP_NE:
            return ifeq ? Assembler::Equal : Assembler::NotEqual;
          default:
            JS_NOT_REACHED("unrecognized op");
            return Assembler::Equal;
        }
    }

    static inline Assembler::Condition
    GetStubCompareCondition(JSOp fused)
    {
        return fused == JSOP_IFEQ ? Assembler::Zero : Assembler::NonZero;
    }

    
    JSObject *pushedSingleton(unsigned pushed);
    CompileStatus inlineNativeFunction(uint32_t argc, bool callingNew);
    CompileStatus inlineScriptedFunction(uint32_t argc, bool callingNew);
    CompileStatus compileMathAbsInt(FrameEntry *arg);
    CompileStatus compileMathAbsDouble(FrameEntry *arg);
    CompileStatus compileMathSqrt(FrameEntry *arg);
    CompileStatus compileMathMinMaxDouble(FrameEntry *arg1, FrameEntry *arg2,
                                          Assembler::DoubleCondition cond);
    CompileStatus compileMathMinMaxInt(FrameEntry *arg1, FrameEntry *arg2,
                                       Assembler::Condition cond);
    CompileStatus compileMathPowSimple(FrameEntry *arg1, FrameEntry *arg2);
    CompileStatus compileArrayPush(FrameEntry *thisv, FrameEntry *arg,
                                   types::StackTypeSet::DoubleConversion conversion);
    CompileStatus compileArrayConcat(types::TypeSet *thisTypes, types::TypeSet *argTypes,
                                     FrameEntry *thisValue, FrameEntry *argValue);
    CompileStatus compileArrayPopShift(FrameEntry *thisv, bool isPacked, bool isArrayPop);
    CompileStatus compileArrayWithLength(uint32_t argc);
    CompileStatus compileArrayWithArgs(uint32_t argc);

    enum RoundingMode { Floor, Round };
    CompileStatus compileRound(FrameEntry *arg, RoundingMode mode);

    enum GetCharMode { GetChar, GetCharCode };
    CompileStatus compileGetChar(FrameEntry *thisValue, FrameEntry *arg, GetCharMode mode);

    CompileStatus compileStringFromCode(FrameEntry *arg);
    CompileStatus compileParseInt(JSValueType argType, uint32_t argc);

    void prepareStubCall(Uses uses);
    Call emitStubCall(void *ptr, DataLabelPtr *pinline);
};



#define INLINE_STUBCALL(stub, rejoin)                                       \
    inlineStubCall(JS_FUNC_TO_DATA_PTR(void *, (stub)), rejoin, Uses(0))
#define INLINE_STUBCALL_USES(stub, rejoin, uses)                            \
    inlineStubCall(JS_FUNC_TO_DATA_PTR(void *, (stub)), rejoin, uses)



#define OOL_STUBCALL(stub, rejoin)                                          \
    stubcc.emitStubCall(JS_FUNC_TO_DATA_PTR(void *, (stub)), rejoin, Uses(0))
#define OOL_STUBCALL_USES(stub, rejoin, uses)                               \
    stubcc.emitStubCall(JS_FUNC_TO_DATA_PTR(void *, (stub)), rejoin, uses)


#define OOL_STUBCALL_LOCAL_SLOTS(stub, rejoin, slots)                       \
    stubcc.emitStubCall(JS_FUNC_TO_DATA_PTR(void *, (stub)), rejoin, Uses(0), (slots))

} 
} 

#endif

