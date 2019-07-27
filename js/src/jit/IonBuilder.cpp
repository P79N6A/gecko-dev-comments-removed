





#include "jit/IonBuilder.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/SizePrintfMacros.h"

#include "builtin/Eval.h"
#include "builtin/TypedObject.h"
#include "frontend/SourceNotes.h"
#include "jit/BaselineFrame.h"
#include "jit/BaselineInspector.h"
#include "jit/Ion.h"
#include "jit/IonOptimizationLevels.h"
#include "jit/JitSpewer.h"
#include "jit/Lowering.h"
#include "jit/MIRGraph.h"
#include "vm/ArgumentsObject.h"
#include "vm/Opcodes.h"
#include "vm/RegExpStatics.h"
#include "vm/TraceLogging.h"

#include "jsopcodeinlines.h"
#include "jsscriptinlines.h"

#include "jit/CompileInfo-inl.h"
#include "vm/NativeObject-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::AssertedCast;
using mozilla::DebugOnly;
using mozilla::Maybe;

using JS::TrackedStrategy;
using JS::TrackedOutcome;
using JS::TrackedTypeSite;

class jit::BaselineFrameInspector
{
  public:
    TypeSet::Type thisType;
    JSObject* singletonScopeChain;

    Vector<TypeSet::Type, 4, JitAllocPolicy> argTypes;
    Vector<TypeSet::Type, 4, JitAllocPolicy> varTypes;

    explicit BaselineFrameInspector(TempAllocator* temp)
      : thisType(TypeSet::UndefinedType()),
        singletonScopeChain(nullptr),
        argTypes(*temp),
        varTypes(*temp)
    {}
};

BaselineFrameInspector*
jit::NewBaselineFrameInspector(TempAllocator* temp, BaselineFrame* frame, CompileInfo* info)
{
    MOZ_ASSERT(frame);

    BaselineFrameInspector* inspector = temp->lifoAlloc()->new_<BaselineFrameInspector>(temp);
    if (!inspector)
        return nullptr;

    
    
    

    inspector->thisType = TypeSet::GetMaybeUntrackedValueType(frame->thisValue());

    if (frame->scopeChain()->isSingleton())
        inspector->singletonScopeChain = frame->scopeChain();

    JSScript* script = frame->script();

    if (script->functionNonDelazifying()) {
        if (!inspector->argTypes.reserve(frame->numFormalArgs()))
            return nullptr;
        for (size_t i = 0; i < frame->numFormalArgs(); i++) {
            if (script->formalIsAliased(i)) {
                inspector->argTypes.infallibleAppend(TypeSet::UndefinedType());
            } else if (!script->argsObjAliasesFormals()) {
                TypeSet::Type type =
                    TypeSet::GetMaybeUntrackedValueType(frame->unaliasedFormal(i));
                inspector->argTypes.infallibleAppend(type);
            } else if (frame->hasArgsObj()) {
                TypeSet::Type type =
                    TypeSet::GetMaybeUntrackedValueType(frame->argsObj().arg(i));
                inspector->argTypes.infallibleAppend(type);
            } else {
                inspector->argTypes.infallibleAppend(TypeSet::UndefinedType());
            }
        }
    }

    if (!inspector->varTypes.reserve(frame->script()->nfixed()))
        return nullptr;
    for (size_t i = 0; i < frame->script()->nfixed(); i++) {
        if (info->isSlotAliasedAtOsr(i + info->firstLocalSlot())) {
            inspector->varTypes.infallibleAppend(TypeSet::UndefinedType());
        } else {
            TypeSet::Type type = TypeSet::GetMaybeUntrackedValueType(frame->unaliasedLocal(i));
            inspector->varTypes.infallibleAppend(type);
        }
    }

    return inspector;
}

IonBuilder::IonBuilder(JSContext* analysisContext, CompileCompartment* comp,
                       const JitCompileOptions& options, TempAllocator* temp,
                       MIRGraph* graph, CompilerConstraintList* constraints,
                       BaselineInspector* inspector, CompileInfo* info,
                       const OptimizationInfo* optimizationInfo,
                       BaselineFrameInspector* baselineFrame, size_t inliningDepth,
                       uint32_t loopDepth)
  : MIRGenerator(comp, options, temp, graph, info, optimizationInfo),
    backgroundCodegen_(nullptr),
    actionableAbortScript_(nullptr),
    actionableAbortPc_(nullptr),
    actionableAbortMessage_(nullptr),
    analysisContext(analysisContext),
    baselineFrame_(baselineFrame),
    constraints_(constraints),
    analysis_(*temp, info->script()),
    thisTypes(nullptr),
    argTypes(nullptr),
    typeArray(nullptr),
    typeArrayHint(0),
    bytecodeTypeMap(nullptr),
    loopDepth_(loopDepth),
    trackedOptimizationSites_(*temp),
    lexicalCheck_(nullptr),
    callerResumePoint_(nullptr),
    callerBuilder_(nullptr),
    cfgStack_(*temp),
    loops_(*temp),
    switches_(*temp),
    labels_(*temp),
    iterators_(*temp),
    loopHeaders_(*temp),
    inspector(inspector),
    inliningDepth_(inliningDepth),
    inlinedBytecodeLength_(0),
    numLoopRestarts_(0),
    failedBoundsCheck_(info->script()->failedBoundsCheck()),
    failedShapeGuard_(info->script()->failedShapeGuard()),
    failedLexicalCheck_(info->script()->failedLexicalCheck()),
    nonStringIteration_(false),
    lazyArguments_(nullptr),
    inlineCallInfo_(nullptr),
    maybeFallbackFunctionGetter_(nullptr)
{
    script_ = info->script();
    pc = info->startPC();
    abortReason_ = AbortReason_Disable;

    MOZ_ASSERT(script()->hasBaselineScript() == (info->analysisMode() != Analysis_ArgumentsUsage));
    MOZ_ASSERT(!!analysisContext == (info->analysisMode() == Analysis_DefiniteProperties));

    if (!info->isAnalysis())
        script()->baselineScript()->setIonCompiledOrInlined();
}

void
IonBuilder::clearForBackEnd()
{
    MOZ_ASSERT(!analysisContext);
    baselineFrame_ = nullptr;

    
    
    
    
    gsn.purge();
    scopeCoordinateNameCache.purge();
}

bool
IonBuilder::abort(const char* message, ...)
{
    
#ifdef DEBUG
    va_list ap;
    va_start(ap, message);
    abortFmt(message, ap);
    va_end(ap);
    JitSpew(JitSpew_IonAbort, "aborted @ %s:%d", script()->filename(), PCToLineNumber(script(), pc));
#endif
    trackActionableAbort(message);
    return false;
}

IonBuilder*
IonBuilder::outermostBuilder()
{
    IonBuilder* builder = this;
    while (builder->callerBuilder_)
        builder = builder->callerBuilder_;
    return builder;
}

void
IonBuilder::trackActionableAbort(const char* message)
{
    if (!isOptimizationTrackingEnabled())
        return;

    IonBuilder* topBuilder = outermostBuilder();
    if (topBuilder->hadActionableAbort())
        return;

    topBuilder->actionableAbortScript_ = script();
    topBuilder->actionableAbortPc_ = pc;
    topBuilder->actionableAbortMessage_ = message;
}

void
IonBuilder::spew(const char* message)
{
    
#ifdef DEBUG
    JitSpew(JitSpew_IonMIR, "%s @ %s:%d", message, script()->filename(), PCToLineNumber(script(), pc));
#endif
}

MInstruction*
IonBuilder::constantMaybeNursery(JSObject* obj)
{
    MOZ_ASSERT(obj);
    if (!IsInsideNursery(obj))
        return constant(ObjectValue(*obj));

    
    
    
    

    ObjectVector& nurseryObjects = outermostBuilder()->nurseryObjects_;

    size_t index = UINT32_MAX;
    for (size_t i = 0, len = nurseryObjects.length(); i < len; i++) {
        if (nurseryObjects[i] == obj) {
            index = i;
            break;
        }
    }

    if (index == UINT32_MAX) {
        if (!nurseryObjects.append(obj))
            return nullptr;
        index = nurseryObjects.length() - 1;
    }

    MNurseryObject* ins = MNurseryObject::New(alloc(), obj, index, constraints());
    current->add(ins);
    return ins;
}

static inline int32_t
GetJumpOffset(jsbytecode* pc)
{
    MOZ_ASSERT(js_CodeSpec[JSOp(*pc)].type() == JOF_JUMP);
    return GET_JUMP_OFFSET(pc);
}

IonBuilder::CFGState
IonBuilder::CFGState::If(jsbytecode* join, MTest* test)
{
    CFGState state;
    state.state = IF_TRUE;
    state.stopAt = join;
    state.branch.ifFalse = test->ifFalse();
    state.branch.test = test;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::IfElse(jsbytecode* trueEnd, jsbytecode* falseEnd, MTest* test)
{
    MBasicBlock* ifFalse = test->ifFalse();

    CFGState state;
    
    
    
    
    
    state.state = (falseEnd == ifFalse->pc())
                  ? IF_TRUE_EMPTY_ELSE
                  : IF_ELSE_TRUE;
    state.stopAt = trueEnd;
    state.branch.falseEnd = falseEnd;
    state.branch.ifFalse = ifFalse;
    state.branch.test = test;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::AndOr(jsbytecode* join, MBasicBlock* lhs)
{
    CFGState state;
    state.state = AND_OR;
    state.stopAt = join;
    state.branch.ifFalse = lhs;
    state.branch.test = nullptr;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::TableSwitch(jsbytecode* exitpc, MTableSwitch* ins)
{
    CFGState state;
    state.state = TABLE_SWITCH;
    state.stopAt = exitpc;
    state.tableswitch.exitpc = exitpc;
    state.tableswitch.breaks = nullptr;
    state.tableswitch.ins = ins;
    state.tableswitch.currentBlock = 0;
    return state;
}

JSFunction*
IonBuilder::getSingleCallTarget(TemporaryTypeSet* calleeTypes)
{
    if (!calleeTypes)
        return nullptr;

    JSObject* obj = calleeTypes->maybeSingleton();
    if (!obj || !obj->is<JSFunction>())
        return nullptr;

    return &obj->as<JSFunction>();
}

bool
IonBuilder::getPolyCallTargets(TemporaryTypeSet* calleeTypes, bool constructing,
                               ObjectVector& targets, uint32_t maxTargets)
{
    MOZ_ASSERT(targets.empty());

    if (!calleeTypes)
        return true;

    if (calleeTypes->baseFlags() != 0)
        return true;

    unsigned objCount = calleeTypes->getObjectCount();

    if (objCount == 0 || objCount > maxTargets)
        return true;

    if (!targets.reserve(objCount))
        return false;
    for (unsigned i = 0; i < objCount; i++) {
        JSObject* obj = calleeTypes->getSingleton(i);
        if (obj) {
            MOZ_ASSERT(obj->isSingleton());
        } else {
            ObjectGroup* group = calleeTypes->getGroup(i);
            if (!group)
                continue;

            obj = group->maybeInterpretedFunction();
            if (!obj) {
                targets.clear();
                return true;
            }

            MOZ_ASSERT(!obj->isSingleton());
        }

        
        
        
        if (constructing ? !obj->isConstructor() : !obj->isCallable()) {
            targets.clear();
            return true;
        }

        targets.infallibleAppend(obj);
    }

    return true;
}

IonBuilder::InliningDecision
IonBuilder::DontInline(JSScript* targetScript, const char* reason)
{
    if (targetScript) {
        JitSpew(JitSpew_Inlining, "Cannot inline %s:%" PRIuSIZE ": %s",
                targetScript->filename(), targetScript->lineno(), reason);
    } else {
        JitSpew(JitSpew_Inlining, "Cannot inline: %s", reason);
    }

    return InliningDecision_DontInline;
}

IonBuilder::InliningDecision
IonBuilder::canInlineTarget(JSFunction* target, CallInfo& callInfo)
{
    if (!optimizationInfo().inlineInterpreted()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineGeneric);
        return InliningDecision_DontInline;
    }

    if (TraceLogTextIdEnabled(TraceLogger_InlinedScripts)) {
        return DontInline(nullptr, "Tracelogging of inlined scripts is enabled"
                                   "but Tracelogger cannot do that yet.");
    }

    if (!target->isInterpreted()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNotInterpreted);
        return DontInline(nullptr, "Non-interpreted target");
    }

    if (info().analysisMode() != Analysis_DefiniteProperties) {
        
        
        
        

        if (callInfo.thisArg()->emptyResultTypeSet()) {
            trackOptimizationOutcome(TrackedOutcome::CantInlineUnreachable);
            return DontInline(nullptr, "Empty TypeSet for |this|");
        }

        for (size_t i = 0; i < callInfo.argc(); i++) {
            if (callInfo.getArg(i)->emptyResultTypeSet()) {
                trackOptimizationOutcome(TrackedOutcome::CantInlineUnreachable);
                return DontInline(nullptr, "Empty TypeSet for argument");
            }
        }
    }

    
    
    if (target->isInterpreted() && info().analysisMode() == Analysis_DefiniteProperties) {
        RootedScript script(analysisContext, target->getOrCreateScript(analysisContext));
        if (!script)
            return InliningDecision_Error;

        if (!script->hasBaselineScript() && script->canBaselineCompile()) {
            MethodStatus status = BaselineCompile(analysisContext, script);
            if (status == Method_Error)
                return InliningDecision_Error;
            if (status != Method_Compiled) {
                trackOptimizationOutcome(TrackedOutcome::CantInlineNoBaseline);
                return InliningDecision_DontInline;
            }
        }
    }

    if (!target->hasScript()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineLazy);
        return DontInline(nullptr, "Lazy script");
    }

    JSScript* inlineScript = target->nonLazyScript();
    if (callInfo.constructing() && !target->isInterpretedConstructor()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNotConstructor);
        return DontInline(inlineScript, "Callee is not a constructor");
    }

    AnalysisMode analysisMode = info().analysisMode();
    if (!CanIonCompile(inlineScript, analysisMode)) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineDisabledIon);
        return DontInline(inlineScript, "Disabled Ion compilation");
    }

    
    if (!inlineScript->hasBaselineScript()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNoBaseline);
        return DontInline(inlineScript, "No baseline jitcode");
    }

    if (TooManyFormalArguments(target->nargs())) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineTooManyArgs);
        return DontInline(inlineScript, "Too many args");
    }

    
    
    
    if (TooManyFormalArguments(callInfo.argc())) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineTooManyArgs);
        return DontInline(inlineScript, "Too many actual args");
    }

    
    IonBuilder* builder = callerBuilder_;
    while (builder) {
        if (builder->script() == inlineScript) {
            trackOptimizationOutcome(TrackedOutcome::CantInlineRecursive);
            return DontInline(inlineScript, "Recursive call");
        }
        builder = builder->callerBuilder_;
    }

    if (target->isHeavyweight()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineHeavyweight);
        return DontInline(inlineScript, "Heavyweight function");
    }

    if (inlineScript->uninlineable()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineGeneric);
        return DontInline(inlineScript, "Uninlineable script");
    }

    if (inlineScript->needsArgsObj()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNeedsArgsObj);
        return DontInline(inlineScript, "Script that needs an arguments object");
    }

    if (inlineScript->isDebuggee()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineDebuggee);
        return DontInline(inlineScript, "Script is debuggee");
    }

    TypeSet::ObjectKey* targetKey = TypeSet::ObjectKey::get(target);
    if (targetKey->unknownProperties()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineUnknownProps);
        return DontInline(inlineScript, "Target type has unknown properties");
    }

    return InliningDecision_Inline;
}

void
IonBuilder::popCfgStack()
{
    if (cfgStack_.back().isLoop())
        loops_.popBack();
    if (cfgStack_.back().state == CFGState::LABEL)
        labels_.popBack();
    cfgStack_.popBack();
}

bool
IonBuilder::analyzeNewLoopTypes(MBasicBlock* entry, jsbytecode* start, jsbytecode* end)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    for (size_t i = 0; i < loopHeaders_.length(); i++) {
        if (loopHeaders_[i].pc == start) {
            MBasicBlock* oldEntry = loopHeaders_[i].header;

            
            
            if (!oldEntry->isDead()) {
                MResumePoint* oldEntryRp = oldEntry->entryResumePoint();
                size_t stackDepth = oldEntryRp->stackDepth();
                for (size_t slot = 0; slot < stackDepth; slot++) {
                    MDefinition* oldDef = oldEntryRp->getOperand(slot);
                    if (!oldDef->isPhi()) {
                        MOZ_ASSERT(oldDef->block()->id() < oldEntry->id());
                        MOZ_ASSERT(oldDef == entry->getSlot(slot));
                        continue;
                    }
                    MPhi* oldPhi = oldDef->toPhi();
                    MPhi* newPhi = entry->getSlot(slot)->toPhi();
                    if (!newPhi->addBackedgeType(oldPhi->type(), oldPhi->resultTypeSet()))
                        return false;
                }
            }

            
            
            
            loopHeaders_[i].header = entry;
            return true;
        }
    }
    loopHeaders_.append(LoopHeader(start, entry));

    jsbytecode* last = nullptr;
    jsbytecode* earlier = nullptr;
    for (jsbytecode* pc = start; pc != end; earlier = last, last = pc, pc += GetBytecodeLength(pc)) {
        uint32_t slot;
        if (*pc == JSOP_SETLOCAL)
            slot = info().localSlot(GET_LOCALNO(pc));
        else if (*pc == JSOP_SETARG)
            slot = info().argSlotUnchecked(GET_ARGNO(pc));
        else
            continue;
        if (slot >= info().firstStackSlot())
            continue;
        if (!analysis().maybeInfo(pc))
            continue;
        if (!last)
            continue;

        MPhi* phi = entry->getSlot(slot)->toPhi();

        if (*last == JSOP_POS)
            last = earlier;

        if (js_CodeSpec[*last].format & JOF_TYPESET) {
            TemporaryTypeSet* typeSet = bytecodeTypes(last);
            if (!typeSet->empty()) {
                MIRType type = typeSet->getKnownMIRType();
                if (!phi->addBackedgeType(type, typeSet))
                    return false;
            }
        } else if (*last == JSOP_GETLOCAL || *last == JSOP_GETARG) {
            uint32_t slot = (*last == JSOP_GETLOCAL)
                            ? info().localSlot(GET_LOCALNO(last))
                            : info().argSlotUnchecked(GET_ARGNO(last));
            if (slot < info().firstStackSlot()) {
                MPhi* otherPhi = entry->getSlot(slot)->toPhi();
                if (otherPhi->hasBackedgeType()) {
                    if (!phi->addBackedgeType(otherPhi->type(), otherPhi->resultTypeSet()))
                        return false;
                }
            }
        } else {
            MIRType type = MIRType_None;
            switch (*last) {
              case JSOP_VOID:
              case JSOP_UNDEFINED:
                type = MIRType_Undefined;
                break;
              case JSOP_GIMPLICITTHIS:
                if (!script()->hasPollutedGlobalScope())
                    type = MIRType_Undefined;
                break;
              case JSOP_NULL:
                type = MIRType_Null;
                break;
              case JSOP_ZERO:
              case JSOP_ONE:
              case JSOP_INT8:
              case JSOP_INT32:
              case JSOP_UINT16:
              case JSOP_UINT24:
              case JSOP_BITAND:
              case JSOP_BITOR:
              case JSOP_BITXOR:
              case JSOP_BITNOT:
              case JSOP_RSH:
              case JSOP_LSH:
              case JSOP_URSH:
                type = MIRType_Int32;
                break;
              case JSOP_FALSE:
              case JSOP_TRUE:
              case JSOP_EQ:
              case JSOP_NE:
              case JSOP_LT:
              case JSOP_LE:
              case JSOP_GT:
              case JSOP_GE:
              case JSOP_NOT:
              case JSOP_STRICTEQ:
              case JSOP_STRICTNE:
              case JSOP_IN:
              case JSOP_INSTANCEOF:
                type = MIRType_Boolean;
                break;
              case JSOP_DOUBLE:
                type = MIRType_Double;
                break;
              case JSOP_STRING:
              case JSOP_TYPEOF:
              case JSOP_TYPEOFEXPR:
                type = MIRType_String;
                break;
              case JSOP_SYMBOL:
                type = MIRType_Symbol;
                break;
              case JSOP_ADD:
              case JSOP_SUB:
              case JSOP_MUL:
              case JSOP_DIV:
              case JSOP_MOD:
              case JSOP_NEG:
                type = inspector->expectedResultType(last);
              default:
                break;
            }
            if (type != MIRType_None) {
                if (!phi->addBackedgeType(type, nullptr))
                    return false;
            }
        }
    }
    return true;
}

bool
IonBuilder::pushLoop(CFGState::State initial, jsbytecode* stopAt, MBasicBlock* entry, bool osr,
                     jsbytecode* loopHead, jsbytecode* initialPc,
                     jsbytecode* bodyStart, jsbytecode* bodyEnd, jsbytecode* exitpc,
                     jsbytecode* continuepc)
{
    if (!continuepc)
        continuepc = entry->pc();

    ControlFlowInfo loop(cfgStack_.length(), continuepc);
    if (!loops_.append(loop))
        return false;

    CFGState state;
    state.state = initial;
    state.stopAt = stopAt;
    state.loop.bodyStart = bodyStart;
    state.loop.bodyEnd = bodyEnd;
    state.loop.exitpc = exitpc;
    state.loop.continuepc = continuepc;
    state.loop.entry = entry;
    state.loop.osr = osr;
    state.loop.successor = nullptr;
    state.loop.breaks = nullptr;
    state.loop.continues = nullptr;
    state.loop.initialState = initial;
    state.loop.initialPc = initialPc;
    state.loop.initialStopAt = stopAt;
    state.loop.loopHead = loopHead;
    return cfgStack_.append(state);
}

bool
IonBuilder::init()
{
    if (!TypeScript::FreezeTypeSets(constraints(), script(), &thisTypes, &argTypes, &typeArray))
        return false;

    if (inlineCallInfo_) {
        
        
        
        thisTypes = inlineCallInfo_->thisArg()->resultTypeSet();
        argTypes = nullptr;
    }

    if (!analysis().init(alloc(), gsn))
        return false;

    
    
    if (script()->hasBaselineScript()) {
        bytecodeTypeMap = script()->baselineScript()->bytecodeTypeMap();
    } else {
        bytecodeTypeMap = alloc_->lifoAlloc()->newArrayUninitialized<uint32_t>(script()->nTypeSets());
        if (!bytecodeTypeMap)
            return false;
        FillBytecodeTypeMap(script(), bytecodeTypeMap);
    }

    return true;
}

bool
IonBuilder::build()
{
    if (!init())
        return false;

    if (script()->hasBaselineScript())
        script()->baselineScript()->resetMaxInliningDepth();

    if (!setCurrentAndSpecializePhis(newBlock(pc)))
        return false;
    if (!current)
        return false;

#ifdef DEBUG
    if (info().isAnalysis()) {
        JitSpew(JitSpew_IonScripts, "Analyzing script %s:%" PRIuSIZE " (%p) %s",
                script()->filename(), script()->lineno(), (void*)script(),
                AnalysisModeString(info().analysisMode()));
    } else {
        JitSpew(JitSpew_IonScripts, "%sompiling script %s:%" PRIuSIZE " (%p) (warmup-counter=%" PRIuSIZE ", level=%s)",
                (script()->hasIonScript() ? "Rec" : "C"),
                script()->filename(), script()->lineno(), (void*)script(),
                script()->getWarmUpCount(), OptimizationLevelString(optimizationInfo().level()));
    }
#endif

    initParameters();
    initLocals();

    
    
    
    
    MInstruction* scope = MConstant::New(alloc(), UndefinedValue());
    current->add(scope);
    current->initSlot(info().scopeChainSlot(), scope);

    
    MInstruction* returnValue = MConstant::New(alloc(), UndefinedValue());
    current->add(returnValue);
    current->initSlot(info().returnValueSlot(), returnValue);

    
    if (info().hasArguments()) {
        MInstruction* argsObj = MConstant::New(alloc(), UndefinedValue());
        current->add(argsObj);
        current->initSlot(info().argsObjSlot(), argsObj);
    }

    
    current->add(MStart::New(alloc(), MStart::StartType_Default));

    
    
    
    
    MCheckOverRecursed* check = MCheckOverRecursed::New(alloc());
    current->add(check);
    MResumePoint* entryRpCopy = MResumePoint::Copy(alloc(), current->entryResumePoint());
    if (!entryRpCopy)
        return false;
    check->setResumePoint(entryRpCopy);

    
    
    rewriteParameters();

    
    if (!initScopeChain())
        return false;

    if (info().needsArgsObj() && !initArgumentsObject())
        return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (uint32_t i = 0; i < info().endArgSlot(); i++) {
        MInstruction* ins = current->getEntrySlot(i)->toInstruction();
        if (ins->type() != MIRType_Value)
            continue;

        MResumePoint* entryRpCopy = MResumePoint::Copy(alloc(), current->entryResumePoint());
        if (!entryRpCopy)
            return false;
        ins->setResumePoint(entryRpCopy);
    }

    
    if (info().hasArguments() && !info().argsObjAliasesFormals()) {
        lazyArguments_ = MConstant::New(alloc(), MagicValue(JS_OPTIMIZED_ARGUMENTS));
        current->add(lazyArguments_);
    }

    insertRecompileCheck();

    if (!traverseBytecode())
        return false;

    
    replaceMaybeFallbackFunctionGetter(nullptr);

    if (script_->hasBaselineScript() &&
        inlinedBytecodeLength_ > script_->baselineScript()->inlinedBytecodeLength())
    {
        script_->baselineScript()->setInlinedBytecodeLength(inlinedBytecodeLength_);
    }

    if (!maybeAddOsrTypeBarriers())
        return false;

    if (!processIterators())
        return false;

    if (!abortedPreliminaryGroups().empty()) {
        MOZ_ASSERT(!info().isAnalysis());
        abortReason_ = AbortReason_PreliminaryObjects;
        return false;
    }

    if (shouldForceAbort()) {
        abortReason_ = AbortReason_Disable;
        return false;
    }

    MOZ_ASSERT(loopDepth_ == 0);
    abortReason_ = AbortReason_NoAbort;
    return true;
}

bool
IonBuilder::processIterators()
{
    
    Vector<MPhi*, 0, SystemAllocPolicy> worklist;
    for (size_t i = 0; i < iterators_.length(); i++) {
        MInstruction* ins = iterators_[i];
        for (MUseDefIterator iter(ins); iter; iter++) {
            if (iter.def()->isPhi()) {
                if (!worklist.append(iter.def()->toPhi()))
                    return false;
            }
        }
    }

    
    
    while (!worklist.empty()) {
        MPhi* phi = worklist.popCopy();
        phi->setIterator();
        phi->setImplicitlyUsedUnchecked();

        for (MUseDefIterator iter(phi); iter; iter++) {
            if (iter.def()->isPhi()) {
                MPhi* other = iter.def()->toPhi();
                if (!other->isIterator() && !worklist.append(other))
                    return false;
            }
        }
    }

    return true;
}

bool
IonBuilder::buildInline(IonBuilder* callerBuilder, MResumePoint* callerResumePoint,
                        CallInfo& callInfo)
{
    inlineCallInfo_ = &callInfo;

    if (!init())
        return false;

    JitSpew(JitSpew_IonScripts, "Inlining script %s:%" PRIuSIZE " (%p)",
            script()->filename(), script()->lineno(), (void*)script());

    callerBuilder_ = callerBuilder;
    callerResumePoint_ = callerResumePoint;

    if (callerBuilder->failedBoundsCheck_)
        failedBoundsCheck_ = true;

    if (callerBuilder->failedShapeGuard_)
        failedShapeGuard_ = true;

    if (callerBuilder->failedLexicalCheck_)
        failedLexicalCheck_ = true;

    
    if (!setCurrentAndSpecializePhis(newBlock(pc)))
        return false;
    if (!current)
        return false;

    current->setCallerResumePoint(callerResumePoint);

    
    MBasicBlock* predecessor = callerBuilder->current;
    MOZ_ASSERT(predecessor == callerResumePoint->block());

    predecessor->end(MGoto::New(alloc(), current));
    if (!current->addPredecessorWithoutPhis(predecessor))
        return false;

    
    MInstruction* scope = MConstant::New(alloc(), UndefinedValue());
    current->add(scope);
    current->initSlot(info().scopeChainSlot(), scope);

    
    MInstruction* returnValue = MConstant::New(alloc(), UndefinedValue());
    current->add(returnValue);
    current->initSlot(info().returnValueSlot(), returnValue);

    
    if (info().hasArguments()) {
        MInstruction* argsObj = MConstant::New(alloc(), UndefinedValue());
        current->add(argsObj);
        current->initSlot(info().argsObjSlot(), argsObj);
    }

    
    current->initSlot(info().thisSlot(), callInfo.thisArg());

    JitSpew(JitSpew_Inlining, "Initializing %u arg slots", info().nargs());

    
    
    MOZ_ASSERT(!info().needsArgsObj());

    
    uint32_t existing_args = Min<uint32_t>(callInfo.argc(), info().nargs());
    for (size_t i = 0; i < existing_args; ++i) {
        MDefinition* arg = callInfo.getArg(i);
        current->initSlot(info().argSlot(i), arg);
    }

    
    for (size_t i = callInfo.argc(); i < info().nargs(); ++i) {
        MConstant* arg = MConstant::New(alloc(), UndefinedValue());
        current->add(arg);
        current->initSlot(info().argSlot(i), arg);
    }

    
    if (!initScopeChain(callInfo.fun()))
        return false;

    JitSpew(JitSpew_Inlining, "Initializing %u local slots; fixed lexicals begin at %u",
            info().nlocals(), info().fixedLexicalBegin());

    initLocals();

    JitSpew(JitSpew_Inlining, "Inline entry block MResumePoint %p, %u stack slots",
            (void*) current->entryResumePoint(), current->entryResumePoint()->stackDepth());

    
    MOZ_ASSERT(current->entryResumePoint()->stackDepth() == info().totalSlots());

    if (script_->argumentsHasVarBinding()) {
        lazyArguments_ = MConstant::New(alloc(), MagicValue(JS_OPTIMIZED_ARGUMENTS));
        current->add(lazyArguments_);
    }

    insertRecompileCheck();

    if (!traverseBytecode())
        return false;

    
    replaceMaybeFallbackFunctionGetter(nullptr);

    if (!abortedPreliminaryGroups().empty()) {
        MOZ_ASSERT(!info().isAnalysis());
        abortReason_ = AbortReason_PreliminaryObjects;
        return false;
    }

    if (shouldForceAbort()) {
        abortReason_ = AbortReason_Disable;
        return false;
    }

    return true;
}

void
IonBuilder::rewriteParameter(uint32_t slotIdx, MDefinition* param, int32_t argIndex)
{
    MOZ_ASSERT(param->isParameter() || param->isGetArgumentsObjectArg());

    TemporaryTypeSet* types = param->resultTypeSet();
    MDefinition* actual = ensureDefiniteType(param, types->getKnownMIRType());
    if (actual == param)
        return;

    
    
    
    
    
    
    
    
    
    current->rewriteSlot(slotIdx, actual);
}




void
IonBuilder::rewriteParameters()
{
    MOZ_ASSERT(info().scopeChainSlot() == 0);

    if (!info().funMaybeLazy())
        return;

    for (uint32_t i = info().startArgSlot(); i < info().endArgSlot(); i++) {
        MDefinition* param = current->getSlot(i);
        rewriteParameter(i, param, param->toParameter()->index());
    }
}

void
IonBuilder::initParameters()
{
    if (!info().funMaybeLazy())
        return;

    
    
    

    if (thisTypes->empty() && baselineFrame_)
        thisTypes->addType(baselineFrame_->thisType, alloc_->lifoAlloc());

    MParameter* param = MParameter::New(alloc(), MParameter::THIS_SLOT, thisTypes);
    current->add(param);
    current->initSlot(info().thisSlot(), param);

    for (uint32_t i = 0; i < info().nargs(); i++) {
        TemporaryTypeSet* types = &argTypes[i];
        if (types->empty() && baselineFrame_ &&
            !script_->baselineScript()->modifiesArguments())
        {
            types->addType(baselineFrame_->argTypes[i], alloc_->lifoAlloc());
        }

        param = MParameter::New(alloc(), i, types);
        current->add(param);
        current->initSlot(info().argSlotUnchecked(i), param);
    }
}

void
IonBuilder::initLocals()
{
    if (info().nlocals() == 0)
        return;

    MConstant* undef = nullptr;
    if (info().fixedLexicalBegin() > 0) {
        undef = MConstant::New(alloc(), UndefinedValue());
        current->add(undef);
    }

    MConstant* uninitLexical = nullptr;
    if (info().fixedLexicalBegin() < info().nlocals()) {
        uninitLexical = MConstant::New(alloc(), MagicValue(JS_UNINITIALIZED_LEXICAL));
        current->add(uninitLexical);
    }

    for (uint32_t i = 0; i < info().nlocals(); i++) {
        current->initSlot(info().localSlot(i), (i < info().fixedLexicalBegin()
                                                ? undef
                                                : uninitLexical));
    }
}

bool
IonBuilder::initScopeChain(MDefinition* callee)
{
    MInstruction* scope = nullptr;

    
    
    
    
    if (!info().needsArgsObj() && !analysis().usesScopeChain())
        return true;

    
    
    
    

    if (JSFunction* fun = info().funMaybeLazy()) {
        if (!callee) {
            MCallee* calleeIns = MCallee::New(alloc());
            current->add(calleeIns);
            callee = calleeIns;
        }
        scope = MFunctionEnvironment::New(alloc(), callee);
        current->add(scope);

        
        
        
        if (fun->isHeavyweight() && !info().isAnalysis()) {
            if (fun->isNamedLambda()) {
                scope = createDeclEnvObject(callee, scope);
                if (!scope)
                    return false;
            }

            scope = createCallObject(callee, scope);
            if (!scope)
                return false;
        }
    } else {
        
        
        MOZ_ASSERT(!script()->isForEval());
        MOZ_ASSERT(!script()->hasPollutedGlobalScope());
        scope = constant(ObjectValue(script()->global()));
    }

    current->setScopeChain(scope);
    return true;
}

bool
IonBuilder::initArgumentsObject()
{
    JitSpew(JitSpew_IonMIR, "%s:%" PRIuSIZE " - Emitting code to initialize arguments object! block=%p",
                              script()->filename(), script()->lineno(), current);
    MOZ_ASSERT(info().needsArgsObj());
    MCreateArgumentsObject* argsObj = MCreateArgumentsObject::New(alloc(), current->scopeChain());
    current->add(argsObj);
    current->setArgumentsObject(argsObj);
    return true;
}

bool
IonBuilder::addOsrValueTypeBarrier(uint32_t slot, MInstruction** def_,
                                   MIRType type, TemporaryTypeSet* typeSet)
{
    MInstruction*& def = *def_;
    MBasicBlock* osrBlock = def->block();

    
    def->setResultType(MIRType_Value);
    def->setResultTypeSet(nullptr);

    if (typeSet && !typeSet->unknown()) {
        MInstruction* barrier = MTypeBarrier::New(alloc(), def, typeSet);
        osrBlock->insertBefore(osrBlock->lastIns(), barrier);
        osrBlock->rewriteSlot(slot, barrier);
        def = barrier;
    } else if (type == MIRType_Null ||
               type == MIRType_Undefined ||
               type == MIRType_MagicOptimizedArguments)
    {
        
        
        TypeSet::Type ntype = TypeSet::PrimitiveType(ValueTypeFromMIRType(type));
        LifoAlloc* lifoAlloc = alloc().lifoAlloc();
        typeSet = lifoAlloc->new_<TemporaryTypeSet>(lifoAlloc, ntype);
        if (!typeSet)
            return false;
        MInstruction* barrier = MTypeBarrier::New(alloc(), def, typeSet);
        osrBlock->insertBefore(osrBlock->lastIns(), barrier);
        osrBlock->rewriteSlot(slot, barrier);
        def = barrier;
    }

    switch (type) {
      case MIRType_Boolean:
      case MIRType_Int32:
      case MIRType_Double:
      case MIRType_String:
      case MIRType_Symbol:
      case MIRType_Object:
        if (type != def->type()) {
            MUnbox* unbox = MUnbox::New(alloc(), def, type, MUnbox::Fallible);
            osrBlock->insertBefore(osrBlock->lastIns(), unbox);
            osrBlock->rewriteSlot(slot, unbox);
            def = unbox;
        }
        break;

      case MIRType_Null:
      {
        MConstant* c = MConstant::New(alloc(), NullValue());
        osrBlock->insertBefore(osrBlock->lastIns(), c);
        osrBlock->rewriteSlot(slot, c);
        def = c;
        break;
      }

      case MIRType_Undefined:
      {
        MConstant* c = MConstant::New(alloc(), UndefinedValue());
        osrBlock->insertBefore(osrBlock->lastIns(), c);
        osrBlock->rewriteSlot(slot, c);
        def = c;
        break;
      }

      case MIRType_MagicOptimizedArguments:
        MOZ_ASSERT(lazyArguments_);
        osrBlock->rewriteSlot(slot, lazyArguments_);
        def = lazyArguments_;
        break;

      default:
        break;
    }

    MOZ_ASSERT(def == osrBlock->getSlot(slot));
    return true;
}

bool
IonBuilder::maybeAddOsrTypeBarriers()
{
    if (!info().osrPc())
        return true;

    
    
    
    

    MBasicBlock* osrBlock = graph().osrBlock();
    if (!osrBlock) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        MOZ_ASSERT(graph().hasTryBlock());
        return abort("OSR block only reachable through catch block");
    }

    MBasicBlock* preheader = osrBlock->getSuccessor(0);
    MBasicBlock* header = preheader->getSuccessor(0);
    static const size_t OSR_PHI_POSITION = 1;
    MOZ_ASSERT(preheader->getPredecessor(OSR_PHI_POSITION) == osrBlock);

    MResumePoint* headerRp = header->entryResumePoint();
    size_t stackDepth = headerRp->stackDepth();
    MOZ_ASSERT(stackDepth == osrBlock->stackDepth());
    for (uint32_t slot = info().startArgSlot(); slot < stackDepth; slot++) {
        
        
        
        if (info().isSlotAliasedAtOsr(slot))
            continue;

        MInstruction* def = osrBlock->getSlot(slot)->toInstruction();
        MPhi* preheaderPhi = preheader->getSlot(slot)->toPhi();
        MPhi* headerPhi = headerRp->getOperand(slot)->toPhi();

        MIRType type = headerPhi->type();
        TemporaryTypeSet* typeSet = headerPhi->resultTypeSet();

        if (!addOsrValueTypeBarrier(slot, &def, type, typeSet))
            return false;

        preheaderPhi->replaceOperand(OSR_PHI_POSITION, def);
        preheaderPhi->setResultType(type);
        preheaderPhi->setResultTypeSet(typeSet);
    }

    return true;
}




























bool
IonBuilder::traverseBytecode()
{
    for (;;) {
        MOZ_ASSERT(pc < info().limitPC());

        for (;;) {
            if (!alloc().ensureBallast())
                return false;

            
            
            
            if (!cfgStack_.empty() && cfgStack_.back().stopAt == pc) {
                ControlStatus status = processCfgStack();
                if (status == ControlStatus_Error)
                    return false;
                if (status == ControlStatus_Abort)
                    return abort("Aborted while processing control flow");
                if (!current)
                    return true;
                continue;
            }

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            ControlStatus status;
            if ((status = snoopControlFlow(JSOp(*pc))) == ControlStatus_None)
                break;
            if (status == ControlStatus_Error)
                return false;
            if (status == ControlStatus_Abort)
                return abort("Aborted while processing control flow");
            if (!current)
                return true;
        }

#ifdef DEBUG
        
        
        
        
        
        
        
        
        
        Vector<MDefinition*, 4, JitAllocPolicy> popped(alloc());
        Vector<size_t, 4, JitAllocPolicy> poppedUses(alloc());
        unsigned nuses = GetUseCount(script_, script_->pcToOffset(pc));

        for (unsigned i = 0; i < nuses; i++) {
            MDefinition* def = current->peek(-int32_t(i + 1));
            if (!popped.append(def) || !poppedUses.append(def->defUseCount()))
                return false;
        }
#endif

        
        JSOp op = JSOp(*pc);
        if (!inspectOpcode(op))
            return false;

#ifdef DEBUG
        for (size_t i = 0; i < popped.length(); i++) {
            switch (op) {
              case JSOP_POP:
              case JSOP_POPN:
              case JSOP_DUPAT:
              case JSOP_DUP:
              case JSOP_DUP2:
              case JSOP_PICK:
              case JSOP_SWAP:
              case JSOP_SETARG:
              case JSOP_SETLOCAL:
              case JSOP_INITLEXICAL:
              case JSOP_SETRVAL:
              case JSOP_VOID:
                
                break;

              case JSOP_POS:
              case JSOP_TOID:
                
                
                
                
                MOZ_ASSERT(i == 0);
                if (current->peek(-1) == popped[0])
                    break;
                

              default:
                MOZ_ASSERT(popped[i]->isImplicitlyUsed() ||

                           
                           
                           
                           
                           popped[i]->isNewDerivedTypedObject() ||

                           popped[i]->defUseCount() > poppedUses[i]);
                break;
            }
        }
#endif

        pc += js_CodeSpec[op].length;
        current->updateTrackedSite(bytecodeSite(pc));
    }

    return true;
}

IonBuilder::ControlStatus
IonBuilder::snoopControlFlow(JSOp op)
{
    switch (op) {
      case JSOP_NOP:
        return maybeLoop(op, info().getNote(gsn, pc));

      case JSOP_POP:
        return maybeLoop(op, info().getNote(gsn, pc));

      case JSOP_RETURN:
      case JSOP_RETRVAL:
        return processReturn(op);

      case JSOP_THROW:
        return processThrow();

      case JSOP_GOTO:
      {
        jssrcnote* sn = info().getNote(gsn, pc);
        switch (sn ? SN_TYPE(sn) : SRC_NULL) {
          case SRC_BREAK:
          case SRC_BREAK2LABEL:
            return processBreak(op, sn);

          case SRC_CONTINUE:
            return processContinue(op);

          case SRC_SWITCHBREAK:
            return processSwitchBreak(op);

          case SRC_WHILE:
          case SRC_FOR_IN:
          case SRC_FOR_OF:
            
            return whileOrForInLoop(sn);

          default:
            
            MOZ_CRASH("unknown goto case");
        }
        break;
      }

      case JSOP_TABLESWITCH:
        return tableSwitch(op, info().getNote(gsn, pc));

      case JSOP_IFNE:
        
        
        MOZ_CRASH("we should never reach an ifne!");

      default:
        break;
    }
    return ControlStatus_None;
}

bool
IonBuilder::inspectOpcode(JSOp op)
{
    switch (op) {
      case JSOP_NOP:
      case JSOP_LINENO:
      case JSOP_LOOPENTRY:
        return true;

      case JSOP_LABEL:
        return jsop_label();

      case JSOP_UNDEFINED:
        
        return pushConstant(UndefinedValue());

      case JSOP_IFEQ:
        return jsop_ifeq(JSOP_IFEQ);

      case JSOP_TRY:
        return jsop_try();

      case JSOP_CONDSWITCH:
        return jsop_condswitch();

      case JSOP_BITNOT:
        return jsop_bitnot();

      case JSOP_BITAND:
      case JSOP_BITOR:
      case JSOP_BITXOR:
      case JSOP_LSH:
      case JSOP_RSH:
      case JSOP_URSH:
        return jsop_bitop(op);

      case JSOP_ADD:
      case JSOP_SUB:
      case JSOP_MUL:
      case JSOP_DIV:
      case JSOP_MOD:
        return jsop_binary(op);

      case JSOP_POS:
        return jsop_pos();

      case JSOP_NEG:
        return jsop_neg();

      case JSOP_AND:
      case JSOP_OR:
        return jsop_andor(op);

      case JSOP_DEFVAR:
      case JSOP_DEFCONST:
        return jsop_defvar(GET_UINT32_INDEX(pc));

      case JSOP_DEFFUN:
        return jsop_deffun(GET_UINT32_INDEX(pc));

      case JSOP_EQ:
      case JSOP_NE:
      case JSOP_STRICTEQ:
      case JSOP_STRICTNE:
      case JSOP_LT:
      case JSOP_LE:
      case JSOP_GT:
      case JSOP_GE:
        return jsop_compare(op);

      case JSOP_DOUBLE:
        return pushConstant(info().getConst(pc));

      case JSOP_STRING:
        return pushConstant(StringValue(info().getAtom(pc)));

      case JSOP_SYMBOL: {
        unsigned which = GET_UINT8(pc);
        JS::Symbol* sym = compartment->runtime()->wellKnownSymbols().get(which);
        return pushConstant(SymbolValue(sym));
      }

      case JSOP_ZERO:
        return pushConstant(Int32Value(0));

      case JSOP_ONE:
        return pushConstant(Int32Value(1));

      case JSOP_NULL:
        return pushConstant(NullValue());

      case JSOP_VOID:
        current->pop();
        return pushConstant(UndefinedValue());

      case JSOP_HOLE:
        return pushConstant(MagicValue(JS_ELEMENTS_HOLE));

      case JSOP_FALSE:
        return pushConstant(BooleanValue(false));

      case JSOP_TRUE:
        return pushConstant(BooleanValue(true));

      case JSOP_ARGUMENTS:
        return jsop_arguments();

      case JSOP_RUNONCE:
        return jsop_runonce();

      case JSOP_REST:
        return jsop_rest();

      case JSOP_GETARG:
        if (info().argsObjAliasesFormals()) {
            MGetArgumentsObjectArg* getArg = MGetArgumentsObjectArg::New(alloc(),
                                                                         current->argumentsObject(),
                                                                         GET_ARGNO(pc));
            current->add(getArg);
            current->push(getArg);
        } else {
            current->pushArg(GET_ARGNO(pc));
        }
        return true;

      case JSOP_SETARG:
        return jsop_setarg(GET_ARGNO(pc));

      case JSOP_GETLOCAL:
        current->pushLocal(GET_LOCALNO(pc));
        return true;

      case JSOP_SETLOCAL:
        current->setLocal(GET_LOCALNO(pc));
        return true;

      case JSOP_CHECKLEXICAL:
        return jsop_checklexical();

      case JSOP_INITLEXICAL:
        current->setLocal(GET_LOCALNO(pc));
        return true;

      case JSOP_CHECKALIASEDLEXICAL:
        return jsop_checkaliasedlet(ScopeCoordinate(pc));

      case JSOP_INITALIASEDLEXICAL:
        return jsop_setaliasedvar(ScopeCoordinate(pc));

      case JSOP_UNINITIALIZED:
        return pushConstant(MagicValue(JS_UNINITIALIZED_LEXICAL));

      case JSOP_POP:
        current->pop();

        
        
        
        
        if (pc[JSOP_POP_LENGTH] == JSOP_POP)
            return true;
        return maybeInsertResume();

      case JSOP_POPN:
        for (uint32_t i = 0, n = GET_UINT16(pc); i < n; i++)
            current->pop();
        return true;

      case JSOP_DUPAT:
        current->pushSlot(current->stackDepth() - 1 - GET_UINT24(pc));
        return true;

      case JSOP_NEWINIT:
        if (GET_UINT8(pc) == JSProto_Array)
            return jsop_newarray(0);
        return jsop_newobject();

      case JSOP_NEWARRAY:
        return jsop_newarray(GET_UINT24(pc));

      case JSOP_NEWARRAY_COPYONWRITE:
        return jsop_newarray_copyonwrite();

      case JSOP_NEWOBJECT:
        return jsop_newobject();

      case JSOP_INITELEM:
        return jsop_initelem();

      case JSOP_INITELEM_ARRAY:
        return jsop_initelem_array();

      case JSOP_INITPROP:
      case JSOP_INITLOCKEDPROP:
      case JSOP_INITHIDDENPROP:
      {
        PropertyName* name = info().getAtom(pc)->asPropertyName();
        return jsop_initprop(name);
      }

      case JSOP_MUTATEPROTO:
      {
        return jsop_mutateproto();
      }

      case JSOP_INITPROP_GETTER:
      case JSOP_INITPROP_SETTER: {
        PropertyName* name = info().getAtom(pc)->asPropertyName();
        return jsop_initprop_getter_setter(name);
      }

      case JSOP_INITELEM_GETTER:
      case JSOP_INITELEM_SETTER:
        return jsop_initelem_getter_setter();

      case JSOP_FUNCALL:
        return jsop_funcall(GET_ARGC(pc));

      case JSOP_FUNAPPLY:
        return jsop_funapply(GET_ARGC(pc));

      case JSOP_CALL:
      case JSOP_NEW:
        return jsop_call(GET_ARGC(pc), (JSOp)*pc == JSOP_NEW);

      case JSOP_EVAL:
      case JSOP_STRICTEVAL:
        return jsop_eval(GET_ARGC(pc));

      case JSOP_INT8:
        return pushConstant(Int32Value(GET_INT8(pc)));

      case JSOP_UINT16:
        return pushConstant(Int32Value(GET_UINT16(pc)));

      case JSOP_GETGNAME:
      {
        PropertyName* name = info().getAtom(pc)->asPropertyName();
        if (!script()->hasPollutedGlobalScope())
            return jsop_getgname(name);
        return jsop_getname(name);
      }

      case JSOP_SETGNAME:
      case JSOP_STRICTSETGNAME:
      {
        PropertyName* name = info().getAtom(pc)->asPropertyName();
        if (script()->hasPollutedGlobalScope())
            return jsop_setprop(name);
        JSObject* obj = &script()->global();
        return setStaticName(obj, name);
      }

      case JSOP_GETNAME:
      {
        PropertyName* name = info().getAtom(pc)->asPropertyName();
        return jsop_getname(name);
      }

      case JSOP_GETINTRINSIC:
      {
        PropertyName* name = info().getAtom(pc)->asPropertyName();
        return jsop_intrinsic(name);
      }

      case JSOP_BINDGNAME:
        if (!script()->hasPollutedGlobalScope())
            return pushConstant(ObjectValue(script()->global()));
        
      case JSOP_BINDNAME:
        return jsop_bindname(info().getName(pc));

      case JSOP_DUP:
        current->pushSlot(current->stackDepth() - 1);
        return true;

      case JSOP_DUP2:
        return jsop_dup2();

      case JSOP_SWAP:
        current->swapAt(-1);
        return true;

      case JSOP_PICK:
        current->pick(-GET_INT8(pc));
        return true;

      case JSOP_GETALIASEDVAR:
        return jsop_getaliasedvar(ScopeCoordinate(pc));

      case JSOP_SETALIASEDVAR:
        return jsop_setaliasedvar(ScopeCoordinate(pc));

      case JSOP_UINT24:
        return pushConstant(Int32Value(GET_UINT24(pc)));

      case JSOP_INT32:
        return pushConstant(Int32Value(GET_INT32(pc)));

      case JSOP_LOOPHEAD:
        
        MOZ_CRASH("JSOP_LOOPHEAD outside loop");

      case JSOP_GETELEM:
      case JSOP_CALLELEM:
        if (!jsop_getelem())
            return false;
        if (op == JSOP_CALLELEM && !improveThisTypesForCall())
            return false;
        return true;

      case JSOP_SETELEM:
      case JSOP_STRICTSETELEM:
        return jsop_setelem();

      case JSOP_LENGTH:
        return jsop_length();

      case JSOP_NOT:
        return jsop_not();

      case JSOP_THIS:
        return jsop_this();

      case JSOP_CALLEE: {
         MDefinition* callee = getCallee();
         current->push(callee);
         return true;
      }

      case JSOP_GETPROP:
      case JSOP_CALLPROP:
      {
        PropertyName* name = info().getAtom(pc)->asPropertyName();
        if (!jsop_getprop(name))
            return false;
        if (op == JSOP_CALLPROP && !improveThisTypesForCall())
            return false;
        return true;
      }

      case JSOP_SETPROP:
      case JSOP_STRICTSETPROP:
      case JSOP_SETNAME:
      case JSOP_STRICTSETNAME:
      {
        PropertyName* name = info().getAtom(pc)->asPropertyName();
        return jsop_setprop(name);
      }

      case JSOP_DELPROP:
      case JSOP_STRICTDELPROP:
      {
        PropertyName* name = info().getAtom(pc)->asPropertyName();
        return jsop_delprop(name);
      }

      case JSOP_DELELEM:
      case JSOP_STRICTDELELEM:
        return jsop_delelem();

      case JSOP_REGEXP:
        return jsop_regexp(info().getRegExp(pc));

      case JSOP_CALLSITEOBJ:
        return pushConstant(ObjectValue(*(info().getObject(pc))));

      case JSOP_OBJECT:
        return jsop_object(info().getObject(pc));

      case JSOP_TYPEOF:
      case JSOP_TYPEOFEXPR:
        return jsop_typeof();

      case JSOP_TOID:
        return jsop_toid();

      case JSOP_LAMBDA:
        return jsop_lambda(info().getFunction(pc));

      case JSOP_LAMBDA_ARROW:
        return jsop_lambda_arrow(info().getFunction(pc));

      case JSOP_ITER:
        return jsop_iter(GET_INT8(pc));

      case JSOP_MOREITER:
        return jsop_itermore();

      case JSOP_ISNOITER:
        return jsop_isnoiter();

      case JSOP_ENDITER:
        return jsop_iterend();

      case JSOP_IN:
        return jsop_in();

      case JSOP_SETRVAL:
        MOZ_ASSERT(!script()->noScriptRval());
        current->setSlot(info().returnValueSlot(), current->pop());
        return true;

      case JSOP_INSTANCEOF:
        return jsop_instanceof();

      case JSOP_DEBUGLEAVEBLOCK:
        return true;

      case JSOP_DEBUGGER:
        return jsop_debugger();

      case JSOP_GIMPLICITTHIS:
        if (!script()->hasPollutedGlobalScope())
            return pushConstant(UndefinedValue());

        
        break;

#ifdef DEBUG
      case JSOP_PUSHBLOCKSCOPE:
      case JSOP_FRESHENBLOCKSCOPE:
      case JSOP_POPBLOCKSCOPE:
        
        
        
        
        
        
#endif
      default:
        break;
    }

    
    
    
    trackActionableAbort("Unsupported bytecode");
#ifdef DEBUG
    return abort("Unsupported opcode: %s", js_CodeName[op]);
#else
    return abort("Unsupported opcode: %d", op);
#endif
}














IonBuilder::ControlStatus
IonBuilder::processControlEnd()
{
    MOZ_ASSERT(!current);

    if (cfgStack_.empty()) {
        
        
        return ControlStatus_Ended;
    }

    return processCfgStack();
}







IonBuilder::ControlStatus
IonBuilder::processCfgStack()
{
    ControlStatus status = processCfgEntry(cfgStack_.back());

    
    
    while (status == ControlStatus_Ended) {
        popCfgStack();
        if (cfgStack_.empty())
            return status;
        status = processCfgEntry(cfgStack_.back());
    }

    
    if (status == ControlStatus_Joined)
        popCfgStack();

    return status;
}

IonBuilder::ControlStatus
IonBuilder::processCfgEntry(CFGState& state)
{
    switch (state.state) {
      case CFGState::IF_TRUE:
      case CFGState::IF_TRUE_EMPTY_ELSE:
        return processIfEnd(state);

      case CFGState::IF_ELSE_TRUE:
        return processIfElseTrueEnd(state);

      case CFGState::IF_ELSE_FALSE:
        return processIfElseFalseEnd(state);

      case CFGState::DO_WHILE_LOOP_BODY:
        return processDoWhileBodyEnd(state);

      case CFGState::DO_WHILE_LOOP_COND:
        return processDoWhileCondEnd(state);

      case CFGState::WHILE_LOOP_COND:
        return processWhileCondEnd(state);

      case CFGState::WHILE_LOOP_BODY:
        return processWhileBodyEnd(state);

      case CFGState::FOR_LOOP_COND:
        return processForCondEnd(state);

      case CFGState::FOR_LOOP_BODY:
        return processForBodyEnd(state);

      case CFGState::FOR_LOOP_UPDATE:
        return processForUpdateEnd(state);

      case CFGState::TABLE_SWITCH:
        return processNextTableSwitchCase(state);

      case CFGState::COND_SWITCH_CASE:
        return processCondSwitchCase(state);

      case CFGState::COND_SWITCH_BODY:
        return processCondSwitchBody(state);

      case CFGState::AND_OR:
        return processAndOrEnd(state);

      case CFGState::LABEL:
        return processLabelEnd(state);

      case CFGState::TRY:
        return processTryEnd(state);

      default:
        MOZ_CRASH("unknown cfgstate");
    }
}

IonBuilder::ControlStatus
IonBuilder::processIfEnd(CFGState& state)
{
    bool thenBranchTerminated = !current;
    if (!thenBranchTerminated) {
        
        
        
        current->end(MGoto::New(alloc(), state.branch.ifFalse));

        if (!state.branch.ifFalse->addPredecessor(alloc(), current))
            return ControlStatus_Error;
    }

    if (!setCurrentAndSpecializePhis(state.branch.ifFalse))
        return ControlStatus_Error;
    graph().moveBlockToEnd(current);
    pc = current->pc();

    if (thenBranchTerminated) {
        
        
        MTest* test = state.branch.test;
        if (!improveTypesAtTest(test->getOperand(0), test->ifTrue() == current, test))
            return ControlStatus_Error;
    }

    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processIfElseTrueEnd(CFGState& state)
{
    
    
    state.state = CFGState::IF_ELSE_FALSE;
    state.branch.ifTrue = current;
    state.stopAt = state.branch.falseEnd;
    pc = state.branch.ifFalse->pc();
    if (!setCurrentAndSpecializePhis(state.branch.ifFalse))
        return ControlStatus_Error;
    graph().moveBlockToEnd(current);

    MTest* test = state.branch.test;
    if (!improveTypesAtTest(test->getOperand(0), test->ifTrue() == current, test))
        return ControlStatus_Error;

    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processIfElseFalseEnd(CFGState& state)
{
    
    state.branch.ifFalse = current;

    
    
    MBasicBlock* pred = state.branch.ifTrue
                        ? state.branch.ifTrue
                        : state.branch.ifFalse;
    MBasicBlock* other = (pred == state.branch.ifTrue) ? state.branch.ifFalse : state.branch.ifTrue;

    if (!pred)
        return ControlStatus_Ended;

    
    MBasicBlock* join = newBlock(pred, state.branch.falseEnd);
    if (!join)
        return ControlStatus_Error;

    
    pred->end(MGoto::New(alloc(), join));

    if (other) {
        other->end(MGoto::New(alloc(), join));
        if (!join->addPredecessor(alloc(), other))
            return ControlStatus_Error;
    }

    
    if (!setCurrentAndSpecializePhis(join))
        return ControlStatus_Error;
    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processBrokenLoop(CFGState& state)
{
    MOZ_ASSERT(!current);

    MOZ_ASSERT(loopDepth_);
    loopDepth_--;

    
    
    for (MBasicBlockIterator i(graph().begin(state.loop.entry)); i != graph().end(); i++) {
        if (i->loopDepth() > loopDepth_)
            i->setLoopDepth(i->loopDepth() - 1);
    }

    
    
    
    if (!setCurrentAndSpecializePhis(state.loop.successor))
        return ControlStatus_Error;
    if (current) {
        MOZ_ASSERT(current->loopDepth() == loopDepth_);
        graph().moveBlockToEnd(current);
    }

    
    if (state.loop.breaks) {
        MBasicBlock* block = createBreakCatchBlock(state.loop.breaks, state.loop.exitpc);
        if (!block)
            return ControlStatus_Error;

        if (current) {
            current->end(MGoto::New(alloc(), block));
            if (!block->addPredecessor(alloc(), current))
                return ControlStatus_Error;
        }

        if (!setCurrentAndSpecializePhis(block))
            return ControlStatus_Error;
    }

    
    
    
    if (!current)
        return ControlStatus_Ended;

    
    
    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::finishLoop(CFGState& state, MBasicBlock* successor)
{
    MOZ_ASSERT(current);

    MOZ_ASSERT(loopDepth_);
    loopDepth_--;
    MOZ_ASSERT_IF(successor, successor->loopDepth() == loopDepth_);

    
    
    AbortReason r = state.loop.entry->setBackedge(current);
    if (r == AbortReason_Alloc)
        return ControlStatus_Error;
    if (r == AbortReason_Disable) {
        
        
        
        
        
        return restartLoop(state);
    }

    if (successor) {
        graph().moveBlockToEnd(successor);
        successor->inheritPhis(state.loop.entry);
    }

    if (state.loop.breaks) {
        
        DeferredEdge* edge = state.loop.breaks;
        while (edge) {
            edge->block->inheritPhis(state.loop.entry);
            edge = edge->next;
        }

        
        MBasicBlock* block = createBreakCatchBlock(state.loop.breaks, state.loop.exitpc);
        if (!block)
            return ControlStatus_Error;

        if (successor) {
            
            
            successor->end(MGoto::New(alloc(), block));
            if (!block->addPredecessor(alloc(), successor))
                return ControlStatus_Error;
        }
        successor = block;
    }

    if (!setCurrentAndSpecializePhis(successor))
        return ControlStatus_Error;

    
    if (!current)
        return ControlStatus_Ended;

    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::restartLoop(CFGState state)
{
    spew("New types at loop header, restarting loop body");

    if (js_JitOptions.limitScriptSize) {
        if (++numLoopRestarts_ >= MAX_LOOP_RESTARTS)
            return ControlStatus_Abort;
    }

    MBasicBlock* header = state.loop.entry;

    
    replaceMaybeFallbackFunctionGetter(nullptr);

    
    
    graph().removeBlocksAfter(header);

    
    
    header->discardAllInstructions();
    header->discardAllResumePoints( false);
    header->setStackDepth(header->getPredecessor(0)->stackDepth());

    popCfgStack();

    loopDepth_++;

    if (!pushLoop(state.loop.initialState, state.loop.initialStopAt, header, state.loop.osr,
                  state.loop.loopHead, state.loop.initialPc,
                  state.loop.bodyStart, state.loop.bodyEnd,
                  state.loop.exitpc, state.loop.continuepc))
    {
        return ControlStatus_Error;
    }

    CFGState& nstate = cfgStack_.back();

    nstate.loop.condpc = state.loop.condpc;
    nstate.loop.updatepc = state.loop.updatepc;
    nstate.loop.updateEnd = state.loop.updateEnd;

    
    
    setCurrent(header);

    if (!jsop_loophead(nstate.loop.loopHead))
        return ControlStatus_Error;

    pc = nstate.loop.initialPc;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processDoWhileBodyEnd(CFGState& state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    
    
    if (!current)
        return processBrokenLoop(state);

    MBasicBlock* header = newBlock(current, state.loop.updatepc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(alloc(), header));

    state.state = CFGState::DO_WHILE_LOOP_COND;
    state.stopAt = state.loop.updateEnd;
    pc = state.loop.updatepc;
    if (!setCurrentAndSpecializePhis(header))
        return ControlStatus_Error;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processDoWhileCondEnd(CFGState& state)
{
    MOZ_ASSERT(JSOp(*pc) == JSOP_IFNE);

    
    
    MOZ_ASSERT(current);

    
    MDefinition* vins = current->pop();
    MBasicBlock* successor = newBlock(current, GetNextPc(pc), loopDepth_ - 1);
    if (!successor)
        return ControlStatus_Error;

    
    if (vins->isConstantValue() && !vins->constantValue().isMagic()) {
        if (!vins->constantToBoolean()) {
            current->end(MGoto::New(alloc(), successor));
            current = nullptr;

            state.loop.successor = successor;
            return processBrokenLoop(state);
        }
    }

    
    MTest* test = newTest(vins, state.loop.entry, successor);
    current->end(test);
    return finishLoop(state, successor);
}

IonBuilder::ControlStatus
IonBuilder::processWhileCondEnd(CFGState& state)
{
    MOZ_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFEQ);

    
    MDefinition* ins = current->pop();

    
    MBasicBlock* body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc, loopDepth_ - 1);
    if (!body || !state.loop.successor)
        return ControlStatus_Error;

    MTest* test;
    if (JSOp(*pc) == JSOP_IFNE)
        test = newTest(ins, body, state.loop.successor);
    else
        test = newTest(ins, state.loop.successor, body);
    current->end(test);

    state.state = CFGState::WHILE_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    if (!setCurrentAndSpecializePhis(body))
        return ControlStatus_Error;

    
    if (!improveTypesAtTest(test->getOperand(0), test->ifTrue() == current, test))
        return ControlStatus_Error;

    
    if (ins->isIsNoIter()) {
        MIteratorMore* iterMore = ins->toIsNoIter()->input()->toIteratorMore();
        jsbytecode* iterMorePc = iterMore->resumePoint()->pc();
        MOZ_ASSERT(*iterMorePc == JSOP_MOREITER);

        if (!nonStringIteration_ && !inspector->hasSeenNonStringIterMore(iterMorePc)) {
            MDefinition* val = current->peek(-1);
            MOZ_ASSERT(val == iterMore);
            MInstruction* ins = MUnbox::New(alloc(), val, MIRType_String, MUnbox::Fallible,
                                            Bailout_NonStringInputInvalidate);
            current->add(ins);
            current->rewriteAtDepth(-1, ins);
        }
    }

    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processWhileBodyEnd(CFGState& state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    if (!current)
        return processBrokenLoop(state);

    current->end(MGoto::New(alloc(), state.loop.entry));
    return finishLoop(state, state.loop.successor);
}

IonBuilder::ControlStatus
IonBuilder::processForCondEnd(CFGState& state)
{
    MOZ_ASSERT(JSOp(*pc) == JSOP_IFNE);

    
    MDefinition* ins = current->pop();

    
    MBasicBlock* body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc, loopDepth_ - 1);
    if (!body || !state.loop.successor)
        return ControlStatus_Error;

    MTest* test = newTest(ins, body, state.loop.successor);
    current->end(test);

    state.state = CFGState::FOR_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    if (!setCurrentAndSpecializePhis(body))
        return ControlStatus_Error;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processForBodyEnd(CFGState& state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    
    
    
    if (!state.loop.updatepc || !current)
        return processForUpdateEnd(state);

    pc = state.loop.updatepc;

    state.state = CFGState::FOR_LOOP_UPDATE;
    state.stopAt = state.loop.updateEnd;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processForUpdateEnd(CFGState& state)
{
    
    
    if (!current)
        return processBrokenLoop(state);

    current->end(MGoto::New(alloc(), state.loop.entry));
    return finishLoop(state, state.loop.successor);
}

IonBuilder::DeferredEdge*
IonBuilder::filterDeadDeferredEdges(DeferredEdge* edge)
{
    DeferredEdge* head = edge;
    DeferredEdge* prev = nullptr;

    while (edge) {
        if (edge->block->isDead()) {
            if (prev)
                prev->next = edge->next;
            else
                head = edge->next;
        } else {
            prev = edge;
        }
        edge = edge->next;
    }

    
    
    
    MOZ_ASSERT(head);

    return head;
}

bool
IonBuilder::processDeferredContinues(CFGState& state)
{
    
    
    if (state.loop.continues) {
        DeferredEdge* edge = filterDeadDeferredEdges(state.loop.continues);

        MBasicBlock* update = newBlock(edge->block, loops_.back().continuepc);
        if (!update)
            return false;

        if (current) {
            current->end(MGoto::New(alloc(), update));
            if (!update->addPredecessor(alloc(), current))
                return false;
        }

        
        
        edge->block->end(MGoto::New(alloc(), update));
        edge = edge->next;

        
        while (edge) {
            edge->block->end(MGoto::New(alloc(), update));
            if (!update->addPredecessor(alloc(), edge->block))
                return false;
            edge = edge->next;
        }
        state.loop.continues = nullptr;

        if (!setCurrentAndSpecializePhis(update))
            return ControlStatus_Error;
    }

    return true;
}

MBasicBlock*
IonBuilder::createBreakCatchBlock(DeferredEdge* edge, jsbytecode* pc)
{
    edge = filterDeadDeferredEdges(edge);

    
    MBasicBlock* successor = newBlock(edge->block, pc);
    if (!successor)
        return nullptr;

    
    
    edge->block->end(MGoto::New(alloc(), successor));
    edge = edge->next;

    
    while (edge) {
        edge->block->end(MGoto::New(alloc(), successor));
        if (!successor->addPredecessor(alloc(), edge->block))
            return nullptr;
        edge = edge->next;
    }

    return successor;
}

IonBuilder::ControlStatus
IonBuilder::processNextTableSwitchCase(CFGState& state)
{
    MOZ_ASSERT(state.state == CFGState::TABLE_SWITCH);

    state.tableswitch.currentBlock++;

    
    if (state.tableswitch.currentBlock >= state.tableswitch.ins->numBlocks())
        return processSwitchEnd(state.tableswitch.breaks, state.tableswitch.exitpc);

    
    MBasicBlock* successor = state.tableswitch.ins->getBlock(state.tableswitch.currentBlock);

    
    
    
    if (current) {
        current->end(MGoto::New(alloc(), successor));
        if (!successor->addPredecessor(alloc(), current))
            return ControlStatus_Error;
    }

    
    graph().moveBlockToEnd(successor);

    
    
    if (state.tableswitch.currentBlock+1 < state.tableswitch.ins->numBlocks())
        state.stopAt = state.tableswitch.ins->getBlock(state.tableswitch.currentBlock+1)->pc();
    else
        state.stopAt = state.tableswitch.exitpc;

    if (!setCurrentAndSpecializePhis(successor))
        return ControlStatus_Error;
    pc = current->pc();
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processAndOrEnd(CFGState& state)
{
    MOZ_ASSERT(current);
    MBasicBlock* lhs = state.branch.ifFalse;

    
    MBasicBlock* join = newBlock(current, state.stopAt);
    if (!join)
        return ControlStatus_Error;

    
    current->end(MGoto::New(alloc(), join));

    
    lhs->end(MGoto::New(alloc(), join));
    if (!join->addPredecessor(alloc(), state.branch.ifFalse))
        return ControlStatus_Error;

    
    if (!setCurrentAndSpecializePhis(join))
        return ControlStatus_Error;
    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processLabelEnd(CFGState& state)
{
    MOZ_ASSERT(state.state == CFGState::LABEL);

    
    if (!state.label.breaks && !current)
        return ControlStatus_Ended;

    
    if (!state.label.breaks)
        return ControlStatus_Joined;

    MBasicBlock* successor = createBreakCatchBlock(state.label.breaks, state.stopAt);
    if (!successor)
        return ControlStatus_Error;

    if (current) {
        current->end(MGoto::New(alloc(), successor));
        if (!successor->addPredecessor(alloc(), current))
            return ControlStatus_Error;
    }

    pc = state.stopAt;
    if (!setCurrentAndSpecializePhis(successor))
        return ControlStatus_Error;
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processTryEnd(CFGState& state)
{
    MOZ_ASSERT(state.state == CFGState::TRY);

    if (!state.try_.successor) {
        MOZ_ASSERT(!current);
        return ControlStatus_Ended;
    }

    if (current) {
        current->end(MGoto::New(alloc(), state.try_.successor));

        if (!state.try_.successor->addPredecessor(alloc(), current))
            return ControlStatus_Error;
    }

    
    if (!setCurrentAndSpecializePhis(state.try_.successor))
        return ControlStatus_Error;
    graph().moveBlockToEnd(current);
    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processBreak(JSOp op, jssrcnote* sn)
{
    MOZ_ASSERT(op == JSOP_GOTO);

    MOZ_ASSERT(SN_TYPE(sn) == SRC_BREAK ||
               SN_TYPE(sn) == SRC_BREAK2LABEL);

    
    jsbytecode* target = pc + GetJumpOffset(pc);
    DebugOnly<bool> found = false;

    if (SN_TYPE(sn) == SRC_BREAK2LABEL) {
        for (size_t i = labels_.length() - 1; i < labels_.length(); i--) {
            CFGState& cfg = cfgStack_[labels_[i].cfgEntry];
            MOZ_ASSERT(cfg.state == CFGState::LABEL);
            if (cfg.stopAt == target) {
                cfg.label.breaks = new(alloc()) DeferredEdge(current, cfg.label.breaks);
                found = true;
                break;
            }
        }
    } else {
        for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
            CFGState& cfg = cfgStack_[loops_[i].cfgEntry];
            MOZ_ASSERT(cfg.isLoop());
            if (cfg.loop.exitpc == target) {
                cfg.loop.breaks = new(alloc()) DeferredEdge(current, cfg.loop.breaks);
                found = true;
                break;
            }
        }
    }

    MOZ_ASSERT(found);

    setCurrent(nullptr);
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

static inline jsbytecode*
EffectiveContinue(jsbytecode* pc)
{
    if (JSOp(*pc) == JSOP_GOTO)
        return pc + GetJumpOffset(pc);
    return pc;
}

IonBuilder::ControlStatus
IonBuilder::processContinue(JSOp op)
{
    MOZ_ASSERT(op == JSOP_GOTO);

    
    CFGState* found = nullptr;
    jsbytecode* target = pc + GetJumpOffset(pc);
    for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
        if (loops_[i].continuepc == target ||
            EffectiveContinue(loops_[i].continuepc) == target)
        {
            found = &cfgStack_[loops_[i].cfgEntry];
            break;
        }
    }

    
    
    MOZ_ASSERT(found);
    CFGState& state = *found;

    state.loop.continues = new(alloc()) DeferredEdge(current, state.loop.continues);

    setCurrent(nullptr);
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processSwitchBreak(JSOp op)
{
    MOZ_ASSERT(op == JSOP_GOTO);

    
    CFGState* found = nullptr;
    jsbytecode* target = pc + GetJumpOffset(pc);
    for (size_t i = switches_.length() - 1; i < switches_.length(); i--) {
        if (switches_[i].continuepc == target) {
            found = &cfgStack_[switches_[i].cfgEntry];
            break;
        }
    }

    
    
    MOZ_ASSERT(found);
    CFGState& state = *found;

    DeferredEdge** breaks = nullptr;
    switch (state.state) {
      case CFGState::TABLE_SWITCH:
        breaks = &state.tableswitch.breaks;
        break;
      case CFGState::COND_SWITCH_BODY:
        breaks = &state.condswitch.breaks;
        break;
      default:
        MOZ_CRASH("Unexpected switch state.");
    }

    *breaks = new(alloc()) DeferredEdge(current, *breaks);

    setCurrent(nullptr);
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processSwitchEnd(DeferredEdge* breaks, jsbytecode* exitpc)
{
    
    
    
    if (!breaks && !current)
        return ControlStatus_Ended;

    
    
    
    MBasicBlock* successor = nullptr;
    if (breaks)
        successor = createBreakCatchBlock(breaks, exitpc);
    else
        successor = newBlock(current, exitpc);

    if (!successor)
        return ControlStatus_Ended;

    
    
    if (current) {
        current->end(MGoto::New(alloc(), successor));
        if (breaks) {
            if (!successor->addPredecessor(alloc(), current))
                return ControlStatus_Error;
        }
    }

    pc = exitpc;
    if (!setCurrentAndSpecializePhis(successor))
        return ControlStatus_Error;
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::maybeLoop(JSOp op, jssrcnote* sn)
{
    
    
    
    
    
    switch (op) {
      case JSOP_POP:
        
        if (sn && SN_TYPE(sn) == SRC_FOR) {
            current->pop();
            return forLoop(op, sn);
        }
        break;

      case JSOP_NOP:
        if (sn) {
            
            if (SN_TYPE(sn) == SRC_WHILE)
                return doWhileLoop(op, sn);
            
            

            
            if (SN_TYPE(sn) == SRC_FOR)
                return forLoop(op, sn);
        }
        break;

      default:
        MOZ_CRASH("unexpected opcode");
    }

    return ControlStatus_None;
}

void
IonBuilder::assertValidLoopHeadOp(jsbytecode* pc)
{
#ifdef DEBUG
    MOZ_ASSERT(JSOp(*pc) == JSOP_LOOPHEAD);

    
    
    CFGState& state = cfgStack_.back();
    MOZ_ASSERT_IF((JSOp)*(state.loop.entry->pc()) == JSOP_GOTO,
         GetNextPc(state.loop.entry->pc()) == pc);

    
    jssrcnote* sn = info().getNote(gsn, pc);
    if (sn) {
        jsbytecode* ifne = pc + GetSrcNoteOffset(sn, 0);

        jsbytecode* expected_ifne;
        switch (state.state) {
          case CFGState::DO_WHILE_LOOP_BODY:
            expected_ifne = state.loop.updateEnd;
            break;

          default:
            MOZ_CRASH("JSOP_LOOPHEAD unexpected source note");
        }

        
        
        MOZ_ASSERT(ifne == expected_ifne);
    } else {
        MOZ_ASSERT(state.state != CFGState::DO_WHILE_LOOP_BODY);
    }
#endif
}

IonBuilder::ControlStatus
IonBuilder::doWhileLoop(JSOp op, jssrcnote* sn)
{
    
    
    
    
    
    
    
    
    
    int condition_offset = GetSrcNoteOffset(sn, 0);
    jsbytecode* conditionpc = pc + condition_offset;

    jssrcnote* sn2 = info().getNote(gsn, pc+1);
    int offset = GetSrcNoteOffset(sn2, 0);
    jsbytecode* ifne = pc + offset + 1;
    MOZ_ASSERT(ifne > pc);

    
    jsbytecode* loopHead = GetNextPc(pc);
    MOZ_ASSERT(JSOp(*loopHead) == JSOP_LOOPHEAD);
    MOZ_ASSERT(loopHead == ifne + GetJumpOffset(ifne));

    jsbytecode* loopEntry = GetNextPc(loopHead);
    bool canOsr = LoopEntryCanIonOsr(loopEntry);
    bool osr = info().hasOsrAt(loopEntry);

    if (osr) {
        MBasicBlock* preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(alloc(), preheader));
        if (!setCurrentAndSpecializePhis(preheader))
            return ControlStatus_Error;
    }

    unsigned stackPhiCount = 0;
    MBasicBlock* header = newPendingLoopHeader(current, pc, osr, canOsr, stackPhiCount);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(alloc(), header));

    jsbytecode* loophead = GetNextPc(pc);
    jsbytecode* bodyStart = GetNextPc(loophead);
    jsbytecode* bodyEnd = conditionpc;
    jsbytecode* exitpc = GetNextPc(ifne);
    if (!analyzeNewLoopTypes(header, bodyStart, exitpc))
        return ControlStatus_Error;
    if (!pushLoop(CFGState::DO_WHILE_LOOP_BODY, conditionpc, header, osr,
                  loopHead, bodyStart, bodyStart, bodyEnd, exitpc, conditionpc))
    {
        return ControlStatus_Error;
    }

    CFGState& state = cfgStack_.back();
    state.loop.updatepc = conditionpc;
    state.loop.updateEnd = ifne;

    if (!setCurrentAndSpecializePhis(header))
        return ControlStatus_Error;
    if (!jsop_loophead(loophead))
        return ControlStatus_Error;

    pc = bodyStart;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::whileOrForInLoop(jssrcnote* sn)
{
    
    
    
    
    
    
    
    
    
    MOZ_ASSERT(SN_TYPE(sn) == SRC_FOR_OF || SN_TYPE(sn) == SRC_FOR_IN || SN_TYPE(sn) == SRC_WHILE);
    int ifneOffset = GetSrcNoteOffset(sn, 0);
    jsbytecode* ifne = pc + ifneOffset;
    MOZ_ASSERT(ifne > pc);

    
    MOZ_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_LOOPHEAD);
    MOZ_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    jsbytecode* loopEntry = pc + GetJumpOffset(pc);
    bool canOsr = LoopEntryCanIonOsr(loopEntry);
    bool osr = info().hasOsrAt(loopEntry);

    if (osr) {
        MBasicBlock* preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(alloc(), preheader));
        if (!setCurrentAndSpecializePhis(preheader))
            return ControlStatus_Error;
    }

    unsigned stackPhiCount;
    if (SN_TYPE(sn) == SRC_FOR_OF)
        stackPhiCount = 2;
    else if (SN_TYPE(sn) == SRC_FOR_IN)
        stackPhiCount = 1;
    else
        stackPhiCount = 0;

    MBasicBlock* header = newPendingLoopHeader(current, pc, osr, canOsr, stackPhiCount);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(alloc(), header));

    
    jsbytecode* loopHead = GetNextPc(pc);
    jsbytecode* bodyStart = GetNextPc(loopHead);
    jsbytecode* bodyEnd = pc + GetJumpOffset(pc);
    jsbytecode* exitpc = GetNextPc(ifne);
    if (!analyzeNewLoopTypes(header, bodyStart, exitpc))
        return ControlStatus_Error;
    if (!pushLoop(CFGState::WHILE_LOOP_COND, ifne, header, osr,
                  loopHead, bodyEnd, bodyStart, bodyEnd, exitpc))
    {
        return ControlStatus_Error;
    }

    
    if (!setCurrentAndSpecializePhis(header))
        return ControlStatus_Error;
    if (!jsop_loophead(loopHead))
        return ControlStatus_Error;

    pc = bodyEnd;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::forLoop(JSOp op, jssrcnote* sn)
{
    
    MOZ_ASSERT(op == JSOP_POP || op == JSOP_NOP);
    pc = GetNextPc(pc);

    jsbytecode* condpc = pc + GetSrcNoteOffset(sn, 0);
    jsbytecode* updatepc = pc + GetSrcNoteOffset(sn, 1);
    jsbytecode* ifne = pc + GetSrcNoteOffset(sn, 2);
    jsbytecode* exitpc = GetNextPc(ifne);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    jsbytecode* bodyStart = pc;
    jsbytecode* bodyEnd = updatepc;
    jsbytecode* loopEntry = condpc;
    if (condpc != ifne) {
        MOZ_ASSERT(JSOp(*bodyStart) == JSOP_GOTO);
        MOZ_ASSERT(bodyStart + GetJumpOffset(bodyStart) == condpc);
        bodyStart = GetNextPc(bodyStart);
    } else {
        
        if (op != JSOP_NOP) {
            
            MOZ_ASSERT(JSOp(*bodyStart) == JSOP_NOP);
            bodyStart = GetNextPc(bodyStart);
        }
        loopEntry = GetNextPc(bodyStart);
    }
    jsbytecode* loopHead = bodyStart;
    MOZ_ASSERT(JSOp(*bodyStart) == JSOP_LOOPHEAD);
    MOZ_ASSERT(ifne + GetJumpOffset(ifne) == bodyStart);
    bodyStart = GetNextPc(bodyStart);

    bool osr = info().hasOsrAt(loopEntry);
    bool canOsr = LoopEntryCanIonOsr(loopEntry);

    if (osr) {
        MBasicBlock* preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(alloc(), preheader));
        if (!setCurrentAndSpecializePhis(preheader))
            return ControlStatus_Error;
    }

    unsigned stackPhiCount = 0;
    MBasicBlock* header = newPendingLoopHeader(current, pc, osr, canOsr, stackPhiCount);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(alloc(), header));

    
    
    jsbytecode* stopAt;
    CFGState::State initial;
    if (condpc != ifne) {
        pc = condpc;
        stopAt = ifne;
        initial = CFGState::FOR_LOOP_COND;
    } else {
        pc = bodyStart;
        stopAt = bodyEnd;
        initial = CFGState::FOR_LOOP_BODY;
    }

    if (!analyzeNewLoopTypes(header, bodyStart, exitpc))
        return ControlStatus_Error;
    if (!pushLoop(initial, stopAt, header, osr,
                  loopHead, pc, bodyStart, bodyEnd, exitpc, updatepc))
    {
        return ControlStatus_Error;
    }

    CFGState& state = cfgStack_.back();
    state.loop.condpc = (condpc != ifne) ? condpc : nullptr;
    state.loop.updatepc = (updatepc != condpc) ? updatepc : nullptr;
    if (state.loop.updatepc)
        state.loop.updateEnd = condpc;

    if (!setCurrentAndSpecializePhis(header))
        return ControlStatus_Error;
    if (!jsop_loophead(loopHead))
        return ControlStatus_Error;

    return ControlStatus_Jumped;
}

int
IonBuilder::CmpSuccessors(const void* a, const void* b)
{
    const MBasicBlock* a0 = * (MBasicBlock * const*)a;
    const MBasicBlock* b0 = * (MBasicBlock * const*)b;
    if (a0->pc() == b0->pc())
        return 0;

    return (a0->pc() > b0->pc()) ? 1 : -1;
}

IonBuilder::ControlStatus
IonBuilder::tableSwitch(JSOp op, jssrcnote* sn)
{
    
    
    
    
    
    
    
    
    
    

    MOZ_ASSERT(op == JSOP_TABLESWITCH);
    MOZ_ASSERT(SN_TYPE(sn) == SRC_TABLESWITCH);

    
    MDefinition* ins = current->pop();

    
    jsbytecode* exitpc = pc + GetSrcNoteOffset(sn, 0);
    jsbytecode* defaultpc = pc + GET_JUMP_OFFSET(pc);

    MOZ_ASSERT(defaultpc > pc && defaultpc <= exitpc);

    
    jsbytecode* pc2 = pc;
    pc2 += JUMP_OFFSET_LEN;
    int low = GET_JUMP_OFFSET(pc2);
    pc2 += JUMP_OFFSET_LEN;
    int high = GET_JUMP_OFFSET(pc2);
    pc2 += JUMP_OFFSET_LEN;

    
    MTableSwitch* tableswitch = MTableSwitch::New(alloc(), ins, low, high);

    
    MBasicBlock* defaultcase = newBlock(current, defaultpc);
    if (!defaultcase)
        return ControlStatus_Error;
    tableswitch->addDefault(defaultcase);
    tableswitch->addBlock(defaultcase);

    
    jsbytecode* casepc = nullptr;
    for (int i = 0; i < high-low+1; i++) {
        casepc = pc + GET_JUMP_OFFSET(pc2);

        MOZ_ASSERT(casepc >= pc && casepc <= exitpc);

        MBasicBlock* caseblock = newBlock(current, casepc);
        if (!caseblock)
            return ControlStatus_Error;

        
        
        
        
        if (casepc == pc) {
            caseblock->end(MGoto::New(alloc(), defaultcase));
            if (!defaultcase->addPredecessor(alloc(), caseblock))
                return ControlStatus_Error;
        }

        tableswitch->addCase(tableswitch->addSuccessor(caseblock));

        
        
        if (casepc != pc)
            tableswitch->addBlock(caseblock);

        pc2 += JUMP_OFFSET_LEN;
    }

    
    graph().moveBlockToEnd(defaultcase);

    MOZ_ASSERT(tableswitch->numCases() == (uint32_t)(high - low + 1));
    MOZ_ASSERT(tableswitch->numSuccessors() > 0);

    
    qsort(tableswitch->blocks(), tableswitch->numBlocks(),
          sizeof(MBasicBlock*), CmpSuccessors);

    
    ControlFlowInfo switchinfo(cfgStack_.length(), exitpc);
    if (!switches_.append(switchinfo))
        return ControlStatus_Error;

    
    CFGState state = CFGState::TableSwitch(exitpc, tableswitch);

    
    current->end(tableswitch);

    
    
    if (tableswitch->numBlocks() > 1)
        state.stopAt = tableswitch->getBlock(1)->pc();
    if (!setCurrentAndSpecializePhis(tableswitch->getBlock(0)))
        return ControlStatus_Error;

    if (!cfgStack_.append(state))
        return ControlStatus_Error;

    pc = current->pc();
    return ControlStatus_Jumped;
}

bool
IonBuilder::replaceTypeSet(MDefinition* subject, TemporaryTypeSet* type, MTest* test)
{
    if (type->unknown())
        return true;

    if (subject->resultTypeSet() && type->equals(subject->resultTypeSet()))
        return true;

    MInstruction* replace = nullptr;
    MDefinition* ins;

    for (uint32_t i = 0; i < current->stackDepth(); i++) {
        ins = current->getSlot(i);

        
        if (ins->isFilterTypeSet() && ins->getOperand(0) == subject &&
            ins->dependency() == test)
        {
            TemporaryTypeSet* intersect =
                TypeSet::intersectSets(ins->resultTypeSet(), type, alloc_->lifoAlloc());
            if (!intersect)
                return false;

            ins->toFilterTypeSet()->setResultType(intersect->getKnownMIRType());
            ins->toFilterTypeSet()->setResultTypeSet(intersect);

            if (ins->type() == MIRType_Undefined)
                current->setSlot(i, constant(UndefinedValue()));
            if (ins->type() == MIRType_Null)
                current->setSlot(i, constant(NullValue()));
            continue;
        }

        if (ins == subject) {
            if (!replace) {
                replace = MFilterTypeSet::New(alloc(), subject, type);
                if (!replace)
                    return false;

                current->add(replace);

                
                
                
                
                replace->setDependency(test);

                if (replace->type() == MIRType_Undefined)
                    replace = constant(UndefinedValue());
                if (replace->type() == MIRType_Null)
                    replace = constant(NullValue());
            }
            current->setSlot(i, replace);
        }
    }
    return true;
}

bool
IonBuilder::detectAndOrStructure(MPhi* ins, bool* branchIsAnd)
{
    
    
    
    
    
    
    
    
    
    

    if (ins->numOperands() != 2)
        return false;

    MBasicBlock* testBlock = ins->block();
    MOZ_ASSERT(testBlock->numPredecessors() == 2);

    MBasicBlock* initialBlock;
    MBasicBlock* branchBlock;
    if (testBlock->getPredecessor(0)->lastIns()->isTest()) {
        initialBlock = testBlock->getPredecessor(0);
        branchBlock = testBlock->getPredecessor(1);
    } else if (testBlock->getPredecessor(1)->lastIns()->isTest()) {
        initialBlock = testBlock->getPredecessor(1);
        branchBlock = testBlock->getPredecessor(0);
    } else {
        return false;
    }

    if (branchBlock->numSuccessors() != 1)
        return false;

    if (branchBlock->numPredecessors() != 1 || branchBlock->getPredecessor(0) != initialBlock)
        return false;

    if (initialBlock->numSuccessors() != 2)
        return false;

    MDefinition* branchResult = ins->getOperand(testBlock->indexForPredecessor(branchBlock));
    MDefinition* initialResult = ins->getOperand(testBlock->indexForPredecessor(initialBlock));

    if (branchBlock->stackDepth() != initialBlock->stackDepth())
        return false;
    if (branchBlock->stackDepth() != testBlock->stackDepth() + 1)
        return false;
    if (branchResult != branchBlock->peek(-1) || initialResult != initialBlock->peek(-1))
        return false;

    MTest* initialTest = initialBlock->lastIns()->toTest();
    bool branchIsTrue = branchBlock == initialTest->ifTrue();
    if (initialTest->input() == ins->getOperand(0))
        *branchIsAnd = branchIsTrue != (testBlock->getPredecessor(0) == branchBlock);
    else if (initialTest->input() == ins->getOperand(1))
        *branchIsAnd = branchIsTrue != (testBlock->getPredecessor(1) == branchBlock);
    else
        return false;

    return true;
}

bool
IonBuilder::improveTypesAtCompare(MCompare* ins, bool trueBranch, MTest* test)
{
    if (ins->compareType() == MCompare::Compare_Undefined ||
        ins->compareType() == MCompare::Compare_Null)
    {
        return improveTypesAtNullOrUndefinedCompare(ins, trueBranch, test);
    }

    if ((ins->lhs()->isTypeOf() || ins->rhs()->isTypeOf()) &&
        (ins->lhs()->isConstantValue() || ins->rhs()->isConstantValue()))
    {
        return improveTypesAtTypeOfCompare(ins, trueBranch, test);
    }

    return true;
}

bool
IonBuilder::improveTypesAtTypeOfCompare(MCompare* ins, bool trueBranch, MTest* test)
{
    MTypeOf* typeOf = ins->lhs()->isTypeOf() ? ins->lhs()->toTypeOf() : ins->rhs()->toTypeOf();
    const Value* constant =
        ins->lhs()->isConstant() ? ins->lhs()->constantVp() : ins->rhs()->constantVp();

    if (!constant->isString())
        return true;

    bool equal = ins->jsop() == JSOP_EQ || ins->jsop() == JSOP_STRICTEQ;
    bool notEqual = ins->jsop() == JSOP_NE || ins->jsop() == JSOP_STRICTNE;

    if (notEqual)
        trueBranch = !trueBranch;

    
    if (!equal && !notEqual)
        return true;

    MDefinition* subject = typeOf->input();
    TemporaryTypeSet* inputTypes = subject->resultTypeSet();

    
    TemporaryTypeSet tmp;
    if (!inputTypes) {
        if (subject->type() == MIRType_Value)
            return true;
        inputTypes = &tmp;
        tmp.addType(TypeSet::PrimitiveType(ValueTypeFromMIRType(subject->type())), alloc_->lifoAlloc());
    }

    if (inputTypes->unknown())
        return true;

    
    
    
    TemporaryTypeSet filter;
    const JSAtomState& names = GetJitContext()->runtime->names();
    if (constant->toString() == TypeName(JSTYPE_VOID, names)) {
        filter.addType(TypeSet::UndefinedType(), alloc_->lifoAlloc());
        if (typeOf->inputMaybeCallableOrEmulatesUndefined() && trueBranch)
            filter.addType(TypeSet::AnyObjectType(), alloc_->lifoAlloc());
    } else if (constant->toString() == TypeName(JSTYPE_BOOLEAN, names)) {
        filter.addType(TypeSet::BooleanType(), alloc_->lifoAlloc());
    } else if (constant->toString() == TypeName(JSTYPE_NUMBER, names)) {
        filter.addType(TypeSet::Int32Type(), alloc_->lifoAlloc());
        filter.addType(TypeSet::DoubleType(), alloc_->lifoAlloc());
    } else if (constant->toString() == TypeName(JSTYPE_STRING, names)) {
        filter.addType(TypeSet::StringType(), alloc_->lifoAlloc());
    } else if (constant->toString() == TypeName(JSTYPE_SYMBOL, names)) {
        filter.addType(TypeSet::SymbolType(), alloc_->lifoAlloc());
    } else if (constant->toString() == TypeName(JSTYPE_OBJECT, names)) {
        filter.addType(TypeSet::NullType(), alloc_->lifoAlloc());
        if (trueBranch)
            filter.addType(TypeSet::AnyObjectType(), alloc_->lifoAlloc());
    } else if (constant->toString() == TypeName(JSTYPE_FUNCTION, names)) {
        if (typeOf->inputMaybeCallableOrEmulatesUndefined() && trueBranch)
            filter.addType(TypeSet::AnyObjectType(), alloc_->lifoAlloc());
    } else {
        return true;
    }

    TemporaryTypeSet* type;
    if (trueBranch)
        type = TypeSet::intersectSets(&filter, inputTypes, alloc_->lifoAlloc());
    else
        type = TypeSet::removeSet(inputTypes, &filter, alloc_->lifoAlloc());

    if (!type)
        return false;

    return replaceTypeSet(subject, type, test);
}

bool
IonBuilder::improveTypesAtNullOrUndefinedCompare(MCompare* ins, bool trueBranch, MTest* test)
{
    MOZ_ASSERT(ins->compareType() == MCompare::Compare_Undefined ||
               ins->compareType() == MCompare::Compare_Null);

    
    bool altersUndefined, altersNull;
    JSOp op = ins->jsop();

    switch(op) {
      case JSOP_STRICTNE:
      case JSOP_STRICTEQ:
        altersUndefined = ins->compareType() == MCompare::Compare_Undefined;
        altersNull = ins->compareType() == MCompare::Compare_Null;
        break;
      case JSOP_NE:
      case JSOP_EQ:
        altersUndefined = altersNull = true;
        break;
      default:
        MOZ_CRASH("Relational compares not supported");
    }

    MDefinition* subject = ins->lhs();
    TemporaryTypeSet* inputTypes = subject->resultTypeSet();

    MOZ_ASSERT(IsNullOrUndefined(ins->rhs()->type()));

    
    TemporaryTypeSet tmp;
    if (!inputTypes) {
        if (subject->type() == MIRType_Value)
            return true;
        inputTypes = &tmp;
        tmp.addType(TypeSet::PrimitiveType(ValueTypeFromMIRType(subject->type())), alloc_->lifoAlloc());
    }

    if (inputTypes->unknown())
        return true;

    TemporaryTypeSet* type;

    
    if ((op == JSOP_STRICTEQ || op == JSOP_EQ) ^ trueBranch) {
        
        TemporaryTypeSet remove;
        if (altersUndefined)
            remove.addType(TypeSet::UndefinedType(), alloc_->lifoAlloc());
        if (altersNull)
            remove.addType(TypeSet::NullType(), alloc_->lifoAlloc());

        type = TypeSet::removeSet(inputTypes, &remove, alloc_->lifoAlloc());
    } else {
        
        TemporaryTypeSet base;
        if (altersUndefined) {
            base.addType(TypeSet::UndefinedType(), alloc_->lifoAlloc());
            
            if (inputTypes->maybeEmulatesUndefined(constraints()))
                base.addType(TypeSet::AnyObjectType(), alloc_->lifoAlloc());
        }

        if (altersNull)
            base.addType(TypeSet::NullType(), alloc_->lifoAlloc());

        type = TypeSet::intersectSets(&base, inputTypes, alloc_->lifoAlloc());
    }

    if (!type)
        return false;

    return replaceTypeSet(subject, type, test);
}

bool
IonBuilder::improveTypesAtTest(MDefinition* ins, bool trueBranch, MTest* test)
{
    
    

    
    
    
    
    
    switch (ins->op()) {
      case MDefinition::Op_Not:
        return improveTypesAtTest(ins->toNot()->getOperand(0), !trueBranch, test);
      case MDefinition::Op_IsObject: {
        MDefinition* subject = ins->getOperand(0);
        TemporaryTypeSet* oldType = subject->resultTypeSet();

        
        TemporaryTypeSet tmp;
        if (!oldType) {
            if (subject->type() == MIRType_Value)
                return true;
            oldType = &tmp;
            tmp.addType(TypeSet::PrimitiveType(ValueTypeFromMIRType(subject->type())), alloc_->lifoAlloc());
        }

        if (oldType->unknown())
            return true;

        TemporaryTypeSet* type = nullptr;
        if (trueBranch)
            type = oldType->cloneObjectsOnly(alloc_->lifoAlloc());
        else
            type = oldType->cloneWithoutObjects(alloc_->lifoAlloc());

        if (!type)
            return false;

        return replaceTypeSet(subject, type, test);
      }
      case MDefinition::Op_Phi: {
        bool branchIsAnd = true;
        if (!detectAndOrStructure(ins->toPhi(), &branchIsAnd)) {
            
            break;
        }

        
        
        if (branchIsAnd) {
            if (trueBranch) {
                if (!improveTypesAtTest(ins->toPhi()->getOperand(0), true, test))
                    return false;
                if (!improveTypesAtTest(ins->toPhi()->getOperand(1), true, test))
                    return false;
            }
        } else {
            














            if (!trueBranch) {
                if (!improveTypesAtTest(ins->toPhi()->getOperand(0), false, test))
                    return false;
                if (!improveTypesAtTest(ins->toPhi()->getOperand(1), false, test))
                    return false;
            }
        }
        return true;
      }

      case MDefinition::Op_Compare:
        return improveTypesAtCompare(ins->toCompare(), trueBranch, test);

      default:
        break;
    }

    
    
    

    TemporaryTypeSet* oldType = ins->resultTypeSet();
    TemporaryTypeSet* type;

    
    TemporaryTypeSet tmp;
    if (!oldType) {
        if (ins->type() == MIRType_Value)
            return true;
        oldType = &tmp;
        tmp.addType(TypeSet::PrimitiveType(ValueTypeFromMIRType(ins->type())), alloc_->lifoAlloc());
    }

    
    if (oldType->unknown())
        return true;

    
    if (trueBranch) {
        TemporaryTypeSet remove;
        remove.addType(TypeSet::UndefinedType(), alloc_->lifoAlloc());
        remove.addType(TypeSet::NullType(), alloc_->lifoAlloc());
        type = TypeSet::removeSet(oldType, &remove, alloc_->lifoAlloc());
    } else {
        TemporaryTypeSet base;
        base.addType(TypeSet::UndefinedType(), alloc_->lifoAlloc()); 
        base.addType(TypeSet::NullType(), alloc_->lifoAlloc()); 
        base.addType(TypeSet::BooleanType(), alloc_->lifoAlloc()); 
        base.addType(TypeSet::Int32Type(), alloc_->lifoAlloc()); 
        base.addType(TypeSet::DoubleType(), alloc_->lifoAlloc()); 
        base.addType(TypeSet::StringType(), alloc_->lifoAlloc()); 

        
        
        if (oldType->maybeEmulatesUndefined(constraints()))
            base.addType(TypeSet::AnyObjectType(), alloc_->lifoAlloc());

        type = TypeSet::intersectSets(&base, oldType, alloc_->lifoAlloc());
    }

    return replaceTypeSet(ins, type, test);
}

bool
IonBuilder::jsop_label()
{
    MOZ_ASSERT(JSOp(*pc) == JSOP_LABEL);

    jsbytecode* endpc = pc + GET_JUMP_OFFSET(pc);
    MOZ_ASSERT(endpc > pc);

    ControlFlowInfo label(cfgStack_.length(), endpc);
    if (!labels_.append(label))
        return false;

    return cfgStack_.append(CFGState::Label(endpc));
}

bool
IonBuilder::jsop_condswitch()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    MOZ_ASSERT(JSOp(*pc) == JSOP_CONDSWITCH);
    jssrcnote* sn = info().getNote(gsn, pc);
    MOZ_ASSERT(SN_TYPE(sn) == SRC_CONDSWITCH);

    
    jsbytecode* exitpc = pc + GetSrcNoteOffset(sn, 0);
    jsbytecode* firstCase = pc + GetSrcNoteOffset(sn, 1);

    
    
    
    
    jsbytecode* curCase = firstCase;
    jsbytecode* lastTarget = GetJumpOffset(curCase) + curCase;
    size_t nbBodies = 2; 

    MOZ_ASSERT(pc < curCase && curCase <= exitpc);
    while (JSOp(*curCase) == JSOP_CASE) {
        
        jssrcnote* caseSn = info().getNote(gsn, curCase);
        MOZ_ASSERT(caseSn && SN_TYPE(caseSn) == SRC_NEXTCASE);
        ptrdiff_t off = GetSrcNoteOffset(caseSn, 0);
        curCase = off ? curCase + off : GetNextPc(curCase);
        MOZ_ASSERT(pc < curCase && curCase <= exitpc);

        
        jsbytecode* curTarget = GetJumpOffset(curCase) + curCase;
        if (lastTarget < curTarget)
            nbBodies++;
        lastTarget = curTarget;
    }

    
    
    MOZ_ASSERT(JSOp(*curCase) == JSOP_DEFAULT);
    jsbytecode* defaultTarget = GetJumpOffset(curCase) + curCase;
    MOZ_ASSERT(curCase < defaultTarget && defaultTarget <= exitpc);

    
    CFGState state = CFGState::CondSwitch(this, exitpc, defaultTarget);
    if (!state.condswitch.bodies || !state.condswitch.bodies->init(alloc(), nbBodies))
        return ControlStatus_Error;

    
    MOZ_ASSERT(JSOp(*firstCase) == JSOP_CASE);
    state.stopAt = firstCase;
    state.state = CFGState::COND_SWITCH_CASE;

    return cfgStack_.append(state);
}

IonBuilder::CFGState
IonBuilder::CFGState::CondSwitch(IonBuilder* builder, jsbytecode* exitpc, jsbytecode* defaultTarget)
{
    CFGState state;
    state.state = COND_SWITCH_CASE;
    state.stopAt = nullptr;
    state.condswitch.bodies = (FixedList<MBasicBlock*>*)builder->alloc_->allocate(
        sizeof(FixedList<MBasicBlock*>));
    state.condswitch.currentIdx = 0;
    state.condswitch.defaultTarget = defaultTarget;
    state.condswitch.defaultIdx = uint32_t(-1);
    state.condswitch.exitpc = exitpc;
    state.condswitch.breaks = nullptr;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::Label(jsbytecode* exitpc)
{
    CFGState state;
    state.state = LABEL;
    state.stopAt = exitpc;
    state.label.breaks = nullptr;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::Try(jsbytecode* exitpc, MBasicBlock* successor)
{
    CFGState state;
    state.state = TRY;
    state.stopAt = exitpc;
    state.try_.successor = successor;
    return state;
}

IonBuilder::ControlStatus
IonBuilder::processCondSwitchCase(CFGState& state)
{
    MOZ_ASSERT(state.state == CFGState::COND_SWITCH_CASE);
    MOZ_ASSERT(!state.condswitch.breaks);
    MOZ_ASSERT(current);
    MOZ_ASSERT(JSOp(*pc) == JSOP_CASE);
    FixedList<MBasicBlock*>& bodies = *state.condswitch.bodies;
    jsbytecode* defaultTarget = state.condswitch.defaultTarget;
    uint32_t& currentIdx = state.condswitch.currentIdx;
    jsbytecode* lastTarget = currentIdx ? bodies[currentIdx - 1]->pc() : nullptr;

    
    jssrcnote* sn = info().getNote(gsn, pc);
    ptrdiff_t off = GetSrcNoteOffset(sn, 0);
    jsbytecode* casePc = off ? pc + off : GetNextPc(pc);
    bool caseIsDefault = JSOp(*casePc) == JSOP_DEFAULT;
    MOZ_ASSERT(JSOp(*casePc) == JSOP_CASE || caseIsDefault);

    
    bool bodyIsNew = false;
    MBasicBlock* bodyBlock = nullptr;
    jsbytecode* bodyTarget = pc + GetJumpOffset(pc);
    if (lastTarget < bodyTarget) {
        
        if (lastTarget < defaultTarget && defaultTarget <= bodyTarget) {
            MOZ_ASSERT(state.condswitch.defaultIdx == uint32_t(-1));
            state.condswitch.defaultIdx = currentIdx;
            bodies[currentIdx] = nullptr;
            
            
            if (defaultTarget < bodyTarget)
                currentIdx++;
        }

        bodyIsNew = true;
        
        bodyBlock = newBlockPopN(current, bodyTarget, 2);
        bodies[currentIdx++] = bodyBlock;
    } else {
        
        MOZ_ASSERT(lastTarget == bodyTarget);
        MOZ_ASSERT(currentIdx > 0);
        bodyBlock = bodies[currentIdx - 1];
    }

    if (!bodyBlock)
        return ControlStatus_Error;

    lastTarget = bodyTarget;

    
    
    bool caseIsNew = false;
    MBasicBlock* caseBlock = nullptr;
    if (!caseIsDefault) {
        caseIsNew = true;
        
        caseBlock = newBlockPopN(current, GetNextPc(pc), 1);
    } else {
        
        
        

        if (state.condswitch.defaultIdx == uint32_t(-1)) {
            
            MOZ_ASSERT(lastTarget < defaultTarget);
            state.condswitch.defaultIdx = currentIdx++;
            caseIsNew = true;
        } else if (bodies[state.condswitch.defaultIdx] == nullptr) {
            
            
            MOZ_ASSERT(defaultTarget < lastTarget);
            caseIsNew = true;
        } else {
            
            MOZ_ASSERT(defaultTarget <= lastTarget);
            caseBlock = bodies[state.condswitch.defaultIdx];
        }

        
        if (caseIsNew) {
            
            caseBlock = newBlockPopN(current, defaultTarget, 2);
            bodies[state.condswitch.defaultIdx] = caseBlock;
        }
    }

    if (!caseBlock)
        return ControlStatus_Error;

    
    
    if (bodyBlock != caseBlock) {
        MDefinition* caseOperand = current->pop();
        MDefinition* switchOperand = current->peek(-1);
        MCompare* cmpResult = MCompare::New(alloc(), switchOperand, caseOperand, JSOP_STRICTEQ);
        cmpResult->infer(constraints(), inspector, pc);
        MOZ_ASSERT(!cmpResult->isEffectful());
        current->add(cmpResult);
        current->end(newTest(cmpResult, bodyBlock, caseBlock));

        
        
        if (!bodyIsNew && !bodyBlock->addPredecessorPopN(alloc(), current, 1))
            return ControlStatus_Error;

        
        
        
        
        MOZ_ASSERT_IF(!caseIsNew, caseIsDefault);
        if (!caseIsNew && !caseBlock->addPredecessorPopN(alloc(), current, 1))
            return ControlStatus_Error;
    } else {
        
        MOZ_ASSERT(caseIsDefault);
        current->pop(); 
        current->pop(); 
        current->end(MGoto::New(alloc(), bodyBlock));
        if (!bodyIsNew && !bodyBlock->addPredecessor(alloc(), current))
            return ControlStatus_Error;
    }

    if (caseIsDefault) {
        
        
        
        
        MOZ_ASSERT(currentIdx == bodies.length() || currentIdx + 1 == bodies.length());
        bodies.shrink(bodies.length() - currentIdx);

        
        
        ControlFlowInfo breakInfo(cfgStack_.length() - 1, state.condswitch.exitpc);
        if (!switches_.append(breakInfo))
            return ControlStatus_Error;

        
        currentIdx = 0;
        setCurrent(nullptr);
        state.state = CFGState::COND_SWITCH_BODY;
        return processCondSwitchBody(state);
    }

    
    if (!setCurrentAndSpecializePhis(caseBlock))
        return ControlStatus_Error;
    pc = current->pc();
    state.stopAt = casePc;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processCondSwitchBody(CFGState& state)
{
    MOZ_ASSERT(state.state == CFGState::COND_SWITCH_BODY);
    MOZ_ASSERT(pc <= state.condswitch.exitpc);
    FixedList<MBasicBlock*>& bodies = *state.condswitch.bodies;
    uint32_t& currentIdx = state.condswitch.currentIdx;

    MOZ_ASSERT(currentIdx <= bodies.length());
    if (currentIdx == bodies.length()) {
        MOZ_ASSERT_IF(current, pc == state.condswitch.exitpc);
        return processSwitchEnd(state.condswitch.breaks, state.condswitch.exitpc);
    }

    
    MBasicBlock* nextBody = bodies[currentIdx++];
    MOZ_ASSERT_IF(current, pc == nextBody->pc());

    
    graph().moveBlockToEnd(nextBody);

    
    if (current) {
        current->end(MGoto::New(alloc(), nextBody));
        if (!nextBody->addPredecessor(alloc(), current))
            return ControlStatus_Error;
    }

    
    if (!setCurrentAndSpecializePhis(nextBody))
        return ControlStatus_Error;
    pc = current->pc();

    if (currentIdx < bodies.length())
        state.stopAt = bodies[currentIdx]->pc();
    else
        state.stopAt = state.condswitch.exitpc;
    return ControlStatus_Jumped;
}

bool
IonBuilder::jsop_andor(JSOp op)
{
    MOZ_ASSERT(op == JSOP_AND || op == JSOP_OR);

    jsbytecode* rhsStart = pc + js_CodeSpec[op].length;
    jsbytecode* joinStart = pc + GetJumpOffset(pc);
    MOZ_ASSERT(joinStart > pc);

    
    MDefinition* lhs = current->peek(-1);

    MBasicBlock* evalLhs = newBlock(current, joinStart);
    MBasicBlock* evalRhs = newBlock(current, rhsStart);
    if (!evalLhs || !evalRhs)
        return false;

    MTest* test = (op == JSOP_AND)
                  ? newTest(lhs, evalRhs, evalLhs)
                  : newTest(lhs, evalLhs, evalRhs);
    current->end(test);

    
    if (!setCurrentAndSpecializePhis(evalLhs))
        return false;

    if (!improveTypesAtTest(test->getOperand(0), test->ifTrue() == current, test))
        return false;

    
    if (!cfgStack_.append(CFGState::AndOr(joinStart, evalLhs)))
        return false;

    if (!setCurrentAndSpecializePhis(evalRhs))
        return false;

    if (!improveTypesAtTest(test->getOperand(0), test->ifTrue() == current, test))
        return false;

    return true;
}

bool
IonBuilder::jsop_dup2()
{
    uint32_t lhsSlot = current->stackDepth() - 2;
    uint32_t rhsSlot = current->stackDepth() - 1;
    current->pushSlot(lhsSlot);
    current->pushSlot(rhsSlot);
    return true;
}

bool
IonBuilder::jsop_loophead(jsbytecode* pc)
{
    assertValidLoopHeadOp(pc);

    current->add(MInterruptCheck::New(alloc()));
    insertRecompileCheck();

    return true;
}

bool
IonBuilder::jsop_ifeq(JSOp op)
{
    
    jsbytecode* trueStart = pc + js_CodeSpec[op].length;
    jsbytecode* falseStart = pc + GetJumpOffset(pc);
    MOZ_ASSERT(falseStart > pc);

    
    jssrcnote* sn = info().getNote(gsn, pc);
    if (!sn)
        return abort("expected sourcenote");

    MDefinition* ins = current->pop();

    
    MBasicBlock* ifTrue = newBlock(current, trueStart);
    MBasicBlock* ifFalse = newBlock(current, falseStart);
    if (!ifTrue || !ifFalse)
        return false;

    MTest* test = newTest(ins, ifTrue, ifFalse);
    current->end(test);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    switch (SN_TYPE(sn)) {
      case SRC_IF:
        if (!cfgStack_.append(CFGState::If(falseStart, test)))
            return false;
        break;

      case SRC_IF_ELSE:
      case SRC_COND:
      {
        
        
        jsbytecode* trueEnd = pc + GetSrcNoteOffset(sn, 0);
        MOZ_ASSERT(trueEnd > pc);
        MOZ_ASSERT(trueEnd < falseStart);
        MOZ_ASSERT(JSOp(*trueEnd) == JSOP_GOTO);
        MOZ_ASSERT(!info().getNote(gsn, trueEnd));

        jsbytecode* falseEnd = trueEnd + GetJumpOffset(trueEnd);
        MOZ_ASSERT(falseEnd > trueEnd);
        MOZ_ASSERT(falseEnd >= falseStart);

        if (!cfgStack_.append(CFGState::IfElse(trueEnd, falseEnd, test)))
            return false;
        break;
      }

      default:
        MOZ_CRASH("unexpected source note type");
    }

    
    
    if (!setCurrentAndSpecializePhis(ifTrue))
        return false;

    
    if (!improveTypesAtTest(test->getOperand(0), test->ifTrue() == current, test))
        return false;

    return true;
}

bool
IonBuilder::jsop_try()
{
    MOZ_ASSERT(JSOp(*pc) == JSOP_TRY);

    
    if (analysis().hasTryFinally())
        return abort("Has try-finally");

    
    MOZ_ASSERT(!isInlineBuilder());

    
    
    if (info().analysisMode() == Analysis_ArgumentsUsage)
        return abort("Try-catch during arguments usage analysis");

    graph().setHasTryBlock();

    jssrcnote* sn = info().getNote(gsn, pc);
    MOZ_ASSERT(SN_TYPE(sn) == SRC_TRY);

    
    
    jsbytecode* endpc = pc + GetSrcNoteOffset(sn, 0);
    MOZ_ASSERT(JSOp(*endpc) == JSOP_GOTO);
    MOZ_ASSERT(GetJumpOffset(endpc) > 0);

    jsbytecode* afterTry = endpc + GetJumpOffset(endpc);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    MBasicBlock* tryBlock = newBlock(current, GetNextPc(pc));
    if (!tryBlock)
        return false;

    MBasicBlock* successor;
    if (analysis().maybeInfo(afterTry)) {
        successor = newBlock(current, afterTry);
        if (!successor)
            return false;

        current->end(MGotoWithFake::New(alloc(), tryBlock, successor));
    } else {
        successor = nullptr;
        current->end(MGoto::New(alloc(), tryBlock));
    }

    if (!cfgStack_.append(CFGState::Try(endpc, successor)))
        return false;

    
    
    MOZ_ASSERT(info().osrPc() < endpc || info().osrPc() >= afterTry);

    
    return setCurrentAndSpecializePhis(tryBlock);
}

IonBuilder::ControlStatus
IonBuilder::processReturn(JSOp op)
{
    MDefinition* def;
    switch (op) {
      case JSOP_RETURN:
        
        def = current->pop();
        break;

      case JSOP_RETRVAL:
        
        if (script()->noScriptRval()) {
            MInstruction* ins = MConstant::New(alloc(), UndefinedValue());
            current->add(ins);
            def = ins;
            break;
        }

        def = current->getSlot(info().returnValueSlot());
        break;

      default:
        def = nullptr;
        MOZ_CRASH("unknown return op");
    }

    MReturn* ret = MReturn::New(alloc(), def);
    current->end(ret);

    if (!graph().addReturn(current))
        return ControlStatus_Error;

    
    setCurrent(nullptr);
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processThrow()
{
    MDefinition* def = current->pop();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    MNop* nop = MNop::New(alloc());
    current->add(nop);

    if (!resumeAfter(nop))
        return ControlStatus_Error;

    MThrow* ins = MThrow::New(alloc(), def);
    current->end(ins);

    
    setCurrent(nullptr);
    return processControlEnd();
}

bool
IonBuilder::pushConstant(const Value& v)
{
    current->push(constant(v));
    return true;
}

bool
IonBuilder::jsop_bitnot()
{
    MDefinition* input = current->pop();
    MBitNot* ins = MBitNot::New(alloc(), input);

    current->add(ins);
    ins->infer();

    current->push(ins);
    if (ins->isEffectful() && !resumeAfter(ins))
        return false;
    return true;
}
bool
IonBuilder::jsop_bitop(JSOp op)
{
    
    MDefinition* right = current->pop();
    MDefinition* left = current->pop();

    MBinaryBitwiseInstruction* ins;
    switch (op) {
      case JSOP_BITAND:
        ins = MBitAnd::New(alloc(), left, right);
        break;

      case JSOP_BITOR:
        ins = MBitOr::New(alloc(), left, right);
        break;

      case JSOP_BITXOR:
        ins = MBitXor::New(alloc(), left, right);
        break;

      case JSOP_LSH:
        ins = MLsh::New(alloc(), left, right);
        break;

      case JSOP_RSH:
        ins = MRsh::New(alloc(), left, right);
        break;

      case JSOP_URSH:
        ins = MUrsh::New(alloc(), left, right);
        break;

      default:
        MOZ_CRASH("unexpected bitop");
    }

    current->add(ins);
    ins->infer(inspector, pc);

    current->push(ins);
    if (ins->isEffectful() && !resumeAfter(ins))
        return false;

    return true;
}

bool
IonBuilder::jsop_binary(JSOp op, MDefinition* left, MDefinition* right)
{
    
    
    if (op == JSOP_ADD &&
        ((left->type() == MIRType_String &&
          (right->type() == MIRType_String ||
           right->type() == MIRType_Int32 ||
           right->type() == MIRType_Double)) ||
         (left->type() == MIRType_Int32 &&
          right->type() == MIRType_String) ||
         (left->type() == MIRType_Double &&
          right->type() == MIRType_String)))
    {
        MConcat* ins = MConcat::New(alloc(), left, right);
        current->add(ins);
        current->push(ins);
        return maybeInsertResume();
    }

    MBinaryArithInstruction* ins;
    switch (op) {
      case JSOP_ADD:
        ins = MAdd::New(alloc(), left, right);
        break;

      case JSOP_SUB:
        ins = MSub::New(alloc(), left, right);
        break;

      case JSOP_MUL:
        ins = MMul::New(alloc(), left, right);
        break;

      case JSOP_DIV:
        ins = MDiv::New(alloc(), left, right);
        break;

      case JSOP_MOD:
        ins = MMod::New(alloc(), left, right);
        break;

      default:
        MOZ_CRASH("unexpected binary opcode");
    }

    current->add(ins);
    ins->infer(alloc(), inspector, pc);
    current->push(ins);

    if (ins->isEffectful())
        return resumeAfter(ins);
    return maybeInsertResume();
}

bool
IonBuilder::jsop_binary(JSOp op)
{
    MDefinition* right = current->pop();
    MDefinition* left = current->pop();

    return jsop_binary(op, left, right);
}

bool
IonBuilder::jsop_pos()
{
    if (IsNumberType(current->peek(-1)->type())) {
        
        
        
        current->peek(-1)->setImplicitlyUsedUnchecked();
        return true;
    }

    
    MDefinition* value = current->pop();
    MConstant* one = MConstant::New(alloc(), Int32Value(1));
    current->add(one);

    return jsop_binary(JSOP_MUL, value, one);
}

bool
IonBuilder::jsop_neg()
{
    
    
    MConstant* negator = MConstant::New(alloc(), Int32Value(-1));
    current->add(negator);

    MDefinition* right = current->pop();

    if (!jsop_binary(JSOP_MUL, negator, right))
        return false;
    return true;
}

class AutoAccumulateReturns
{
    MIRGraph& graph_;
    MIRGraphReturns* prev_;

  public:
    AutoAccumulateReturns(MIRGraph& graph, MIRGraphReturns& returns)
      : graph_(graph)
    {
        prev_ = graph_.returnAccumulator();
        graph_.setReturnAccumulator(&returns);
    }
    ~AutoAccumulateReturns() {
        graph_.setReturnAccumulator(prev_);
    }
};

bool
IonBuilder::inlineScriptedCall(CallInfo& callInfo, JSFunction* target)
{
    MOZ_ASSERT(target->hasScript());
    MOZ_ASSERT(IsIonInlinablePC(pc));

    callInfo.setImplicitlyUsedUnchecked();

    
    uint32_t depth = current->stackDepth() + callInfo.numFormals();
    if (depth > current->nslots()) {
        if (!current->increaseSlots(depth - current->nslots()))
            return false;
    }

    
    if (callInfo.constructing()) {
        MDefinition* thisDefn = createThis(target, callInfo.fun());
        if (!thisDefn)
            return false;
        callInfo.setThis(thisDefn);
    }

    
    callInfo.pushFormals(current);

    MResumePoint* outerResumePoint =
        MResumePoint::New(alloc(), current, pc, MResumePoint::Outer);
    if (!outerResumePoint)
        return false;
    current->setOuterResumePoint(outerResumePoint);

    
    callInfo.popFormals(current);
    current->push(callInfo.fun());

    JSScript* calleeScript = target->nonLazyScript();
    BaselineInspector inspector(calleeScript);

    
    if (callInfo.constructing() &&
        !callInfo.thisArg()->resultTypeSet())
    {
        StackTypeSet* types = TypeScript::ThisTypes(calleeScript);
        if (types && !types->unknown()) {
            TemporaryTypeSet* clonedTypes = types->clone(alloc_->lifoAlloc());
            if (!clonedTypes)
                return oom();
            MTypeBarrier* barrier = MTypeBarrier::New(alloc(), callInfo.thisArg(), clonedTypes);
            current->add(barrier);
            if (barrier->type() == MIRType_Undefined)
                callInfo.setThis(constant(UndefinedValue()));
            else if (barrier->type() == MIRType_Null)
                callInfo.setThis(constant(NullValue()));
            else
                callInfo.setThis(barrier);
        }
    }

    
    LifoAlloc* lifoAlloc = alloc_->lifoAlloc();
    InlineScriptTree* inlineScriptTree =
        info().inlineScriptTree()->addCallee(alloc_, pc, calleeScript);
    if (!inlineScriptTree)
        return false;
    CompileInfo* info = lifoAlloc->new_<CompileInfo>(calleeScript, target,
                                                     (jsbytecode*)nullptr, callInfo.constructing(),
                                                     this->info().analysisMode(),
                                                      false,
                                                     inlineScriptTree);
    if (!info)
        return false;

    MIRGraphReturns returns(alloc());
    AutoAccumulateReturns aar(graph(), returns);

    
    IonBuilder inlineBuilder(analysisContext, compartment, options, &alloc(), &graph(), constraints(),
                             &inspector, info, &optimizationInfo(), nullptr, inliningDepth_ + 1,
                             loopDepth_);
    if (!inlineBuilder.buildInline(this, outerResumePoint, callInfo)) {
        if (analysisContext && analysisContext->isExceptionPending()) {
            JitSpew(JitSpew_IonAbort, "Inline builder raised exception.");
            abortReason_ = AbortReason_Error;
            return false;
        }

        
        
        if (inlineBuilder.abortReason_ == AbortReason_Disable) {
            calleeScript->setUninlineable();
            abortReason_ = AbortReason_Inlining;
        } else if (inlineBuilder.abortReason_ == AbortReason_Inlining) {
            abortReason_ = AbortReason_Inlining;
        } else if (inlineBuilder.abortReason_ == AbortReason_PreliminaryObjects) {
            const ObjectGroupVector& groups = inlineBuilder.abortedPreliminaryGroups();
            MOZ_ASSERT(!groups.empty());
            for (size_t i = 0; i < groups.length(); i++)
                addAbortedPreliminaryGroup(groups[i]);
            abortReason_ = AbortReason_PreliminaryObjects;
        }

        return false;
    }

    MOZ_ASSERT(inlineBuilder.nurseryObjects_.empty(),
               "Nursery objects should be added to outer builder");

    
    jsbytecode* postCall = GetNextPc(pc);
    MBasicBlock* returnBlock = newBlock(nullptr, postCall);
    if (!returnBlock)
        return false;
    returnBlock->setCallerResumePoint(callerResumePoint_);

    
    returnBlock->inheritSlots(current);
    returnBlock->pop();

    
    if (returns.empty()) {
        
        calleeScript->setUninlineable();
        abortReason_ = AbortReason_Inlining;
        return false;
    }
    MDefinition* retvalDefn = patchInlinedReturns(callInfo, returns, returnBlock);
    if (!retvalDefn)
        return false;
    returnBlock->push(retvalDefn);

    
    if (!returnBlock->initEntrySlots(alloc()))
        return false;

    return setCurrentAndSpecializePhis(returnBlock);
}

MDefinition*
IonBuilder::patchInlinedReturn(CallInfo& callInfo, MBasicBlock* exit, MBasicBlock* bottom)
{
    
    MDefinition* rdef = exit->lastIns()->toReturn()->input();
    exit->discardLastIns();

    
    if (callInfo.constructing()) {
        if (rdef->type() == MIRType_Value) {
            
            MReturnFromCtor* filter = MReturnFromCtor::New(alloc(), rdef, callInfo.thisArg());
            exit->add(filter);
            rdef = filter;
        } else if (rdef->type() != MIRType_Object) {
            
            rdef = callInfo.thisArg();
        }
    } else if (callInfo.isSetter()) {
        
        rdef = callInfo.getArg(0);
    }

    if (!callInfo.isSetter())
        rdef = specializeInlinedReturn(rdef, exit);

    MGoto* replacement = MGoto::New(alloc(), bottom);
    exit->end(replacement);
    if (!bottom->addPredecessorWithoutPhis(exit))
        return nullptr;

    return rdef;
}

MDefinition*
IonBuilder::specializeInlinedReturn(MDefinition* rdef, MBasicBlock* exit)
{
    
    TemporaryTypeSet* types = bytecodeTypes(pc);

    
    if (types->empty() || types->unknown())
        return rdef;

    
    

    if (rdef->resultTypeSet()) {
        
        
        if (rdef->resultTypeSet()->isSubset(types))
            return rdef;
    } else {
        MIRType observedType = types->getKnownMIRType();

        
        
        if (observedType == MIRType_Double && rdef->type() == MIRType_Float32)
            return rdef;

        
        
        
        if (observedType == rdef->type() &&
            observedType != MIRType_Value &&
            (observedType != MIRType_Object || types->unknownObject()))
        {
            return rdef;
        }
    }

    setCurrent(exit);

    MTypeBarrier* barrier = nullptr;
    rdef = addTypeBarrier(rdef, types, BarrierKind::TypeSet, &barrier);
    if (barrier)
        barrier->setNotMovable();

    return rdef;
}

MDefinition*
IonBuilder::patchInlinedReturns(CallInfo& callInfo, MIRGraphReturns& returns, MBasicBlock* bottom)
{
    
    
    MOZ_ASSERT(returns.length() > 0);

    if (returns.length() == 1)
        return patchInlinedReturn(callInfo, returns[0], bottom);

    
    MPhi* phi = MPhi::New(alloc());
    if (!phi->reserveLength(returns.length()))
        return nullptr;

    for (size_t i = 0; i < returns.length(); i++) {
        MDefinition* rdef = patchInlinedReturn(callInfo, returns[i], bottom);
        if (!rdef)
            return nullptr;
        phi->addInput(rdef);
    }

    bottom->addPhi(phi);
    return phi;
}

IonBuilder::InliningDecision
IonBuilder::makeInliningDecision(JSObject* targetArg, CallInfo& callInfo)
{
    
    if (targetArg == nullptr) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNoTarget);
        return InliningDecision_DontInline;
    }

    
    if (!targetArg->is<JSFunction>())
        return InliningDecision_Inline;

    JSFunction* target = &targetArg->as<JSFunction>();

    
    if (info().analysisMode() == Analysis_ArgumentsUsage)
        return InliningDecision_DontInline;

    
    if (target->isNative())
        return InliningDecision_Inline;

    
    InliningDecision decision = canInlineTarget(target, callInfo);
    if (decision != InliningDecision_Inline)
        return decision;

    
    JSScript* targetScript = target->nonLazyScript();

    
    
    bool offThread = options.offThreadCompilationAvailable();
    if (targetScript->length() > optimizationInfo().inlineMaxBytecodePerCallSite(offThread)) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineBigCallee);
        return DontInline(targetScript, "Vetoed: callee excessively large");
    }

    
    
    
    if (targetScript->getWarmUpCount() < optimizationInfo().inliningWarmUpThreshold() &&
        !targetScript->baselineScript()->ionCompiledOrInlined() &&
        info().analysisMode() != Analysis_DefiniteProperties)
    {
        trackOptimizationOutcome(TrackedOutcome::CantInlineNotHot);
        JitSpew(JitSpew_Inlining, "Cannot inline %s:%" PRIuSIZE ": callee is insufficiently hot.",
                targetScript->filename(), targetScript->lineno());
        return InliningDecision_WarmUpCountTooLow;
    }

    
    
    uint32_t inlinedBytecodeLength = targetScript->baselineScript()->inlinedBytecodeLength();
    if (inlinedBytecodeLength > optimizationInfo().inlineMaxCalleeInlinedBytecodeLength()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineBigCalleeInlinedBytecodeLength);
        return DontInline(targetScript, "Vetoed: callee inlinedBytecodeLength is too big");
    }

    IonBuilder* outerBuilder = outermostBuilder();

    
    
    size_t totalBytecodeLength = outerBuilder->inlinedBytecodeLength_ + targetScript->length();
    if (totalBytecodeLength > optimizationInfo().inlineMaxTotalBytecodeLength()) {
        trackOptimizationOutcome(TrackedOutcome::CantInlineExceededTotalBytecodeLength);
        return DontInline(targetScript, "Vetoed: exceeding max total bytecode length");
    }

    

    uint32_t maxInlineDepth;
    if (js_JitOptions.isSmallFunction(targetScript)) {
        maxInlineDepth = optimizationInfo().smallFunctionMaxInlineDepth();
    } else {
        maxInlineDepth = optimizationInfo().maxInlineDepth();

        
        if (script()->length() >= optimizationInfo().inliningMaxCallerBytecodeLength()) {
            trackOptimizationOutcome(TrackedOutcome::CantInlineBigCaller);
            return DontInline(targetScript, "Vetoed: caller excessively large");
        }
    }

    BaselineScript* outerBaseline = outermostBuilder()->script()->baselineScript();
    if (inliningDepth_ >= maxInlineDepth) {
        
        
        
        
        outerBaseline->setMaxInliningDepth(0);

        trackOptimizationOutcome(TrackedOutcome::CantInlineExceededDepth);
        return DontInline(targetScript, "Vetoed: exceeding allowed inline depth");
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (targetScript->hasLoops() &&
        inliningDepth_ >= targetScript->baselineScript()->maxInliningDepth())
    {
        trackOptimizationOutcome(TrackedOutcome::CantInlineExceededDepth);
        return DontInline(targetScript, "Vetoed: exceeding allowed script inline depth");
    }

    
    MOZ_ASSERT(maxInlineDepth > inliningDepth_);
    uint32_t scriptInlineDepth = maxInlineDepth - inliningDepth_ - 1;
    if (scriptInlineDepth < outerBaseline->maxInliningDepth())
        outerBaseline->setMaxInliningDepth(scriptInlineDepth);

    

    
    TypeSet::ObjectKey* targetKey = TypeSet::ObjectKey::get(target);
    targetKey->watchStateChangeForInlinedCall(constraints());

    outerBuilder->inlinedBytecodeLength_ += targetScript->length();

    return InliningDecision_Inline;
}

bool
IonBuilder::selectInliningTargets(const ObjectVector& targets, CallInfo& callInfo, BoolVector& choiceSet,
                                  uint32_t* numInlineable)
{
    *numInlineable = 0;
    uint32_t totalSize = 0;

    
    if (!choiceSet.reserve(targets.length()))
        return false;

    
    
    if (info().analysisMode() == Analysis_DefiniteProperties && targets.length() > 1)
        return true;

    for (size_t i = 0; i < targets.length(); i++) {
        JSObject* target = targets[i];

        trackOptimizationAttempt(TrackedStrategy::Call_Inline);
        trackTypeInfo(TrackedTypeSite::Call_Target, target);

        bool inlineable;
        InliningDecision decision = makeInliningDecision(target, callInfo);
        switch (decision) {
          case InliningDecision_Error:
            return false;
          case InliningDecision_DontInline:
          case InliningDecision_WarmUpCountTooLow:
            inlineable = false;
            break;
          case InliningDecision_Inline:
            inlineable = true;
            break;
          default:
            MOZ_CRASH("Unhandled InliningDecision value!");
        }

        if (target->is<JSFunction>()) {
            
            if (inlineable && target->as<JSFunction>().isInterpreted()) {
                totalSize += target->as<JSFunction>().nonLazyScript()->length();
                bool offThread = options.offThreadCompilationAvailable();
                if (totalSize > optimizationInfo().inlineMaxBytecodePerCallSite(offThread))
                    inlineable = false;
            }
        } else {
            
            inlineable = false;
        }

        choiceSet.append(inlineable);
        if (inlineable)
            *numInlineable += 1;
    }

    
    
    
    if (isOptimizationTrackingEnabled()) {
        for (size_t i = 0; i < targets.length(); i++) {
            if (choiceSet[i] && targets[i]->as<JSFunction>().isNative()) {
                trackTypeInfo(callInfo);
                break;
            }
        }
    }

    MOZ_ASSERT(choiceSet.length() == targets.length());
    return true;
}

static bool
CanInlineGetPropertyCache(MGetPropertyCache* cache, MDefinition* thisDef)
{
    MOZ_ASSERT(cache->object()->type() == MIRType_Object);
    if (cache->object() != thisDef)
        return false;

    InlinePropertyTable* table = cache->propTable();
    if (!table)
        return false;
    if (table->numEntries() == 0)
        return false;
    return true;
}

class WrapMGetPropertyCache
{
    MGetPropertyCache* cache_;

  private:
    void discardPriorResumePoint() {
        if (!cache_)
            return;

        InlinePropertyTable* propTable = cache_->propTable();
        if (!propTable)
            return;
        MResumePoint* rp = propTable->takePriorResumePoint();
        if (!rp)
            return;
        cache_->block()->discardPreAllocatedResumePoint(rp);
    }

  public:
    explicit WrapMGetPropertyCache(MGetPropertyCache* cache)
      : cache_(cache)
    { }

    ~WrapMGetPropertyCache() {
        discardPriorResumePoint();
    }

    MGetPropertyCache* get() {
        return cache_;
    }
    MGetPropertyCache* operator->() {
        return get();
    }

    
    
    MGetPropertyCache* moveableCache(bool hasTypeBarrier, MDefinition* thisDef) {
        
        
        if (!hasTypeBarrier) {
            if (cache_->hasUses())
                return nullptr;
        } else {
            
            
            MOZ_ASSERT(cache_->hasUses());
            if (!cache_->hasOneUse())
                return nullptr;
        }

        
        
        
        if (!CanInlineGetPropertyCache(cache_, thisDef))
            return nullptr;

        MGetPropertyCache* ret = cache_;
        cache_ = nullptr;
        return ret;
    }
};

MGetPropertyCache*
IonBuilder::getInlineableGetPropertyCache(CallInfo& callInfo)
{
    if (callInfo.constructing())
        return nullptr;

    MDefinition* thisDef = callInfo.thisArg();
    if (thisDef->type() != MIRType_Object)
        return nullptr;

    MDefinition* funcDef = callInfo.fun();
    if (funcDef->type() != MIRType_Object)
        return nullptr;

    
    if (funcDef->isGetPropertyCache()) {
        WrapMGetPropertyCache cache(funcDef->toGetPropertyCache());
        return cache.moveableCache( false, thisDef);
    }

    
    
    if (funcDef->isTypeBarrier()) {
        MTypeBarrier* barrier = funcDef->toTypeBarrier();
        if (barrier->hasUses())
            return nullptr;
        if (barrier->type() != MIRType_Object)
            return nullptr;
        if (!barrier->input()->isGetPropertyCache())
            return nullptr;

        WrapMGetPropertyCache cache(barrier->input()->toGetPropertyCache());
        return cache.moveableCache( true, thisDef);
    }

    return nullptr;
}

IonBuilder::InliningStatus
IonBuilder::inlineSingleCall(CallInfo& callInfo, JSObject* targetArg)
{
    if (!targetArg->is<JSFunction>()) {
        InliningStatus status = inlineNonFunctionCall(callInfo, targetArg);
        trackInlineSuccess(status);
        return status;
    }

    JSFunction* target = &targetArg->as<JSFunction>();
    if (target->isNative()) {
        InliningStatus status = inlineNativeCall(callInfo, target);
        trackInlineSuccess(status);
        return status;
    }

    
    
    trackInlineSuccess();
    if (!inlineScriptedCall(callInfo, target))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineCallsite(const ObjectVector& targets, CallInfo& callInfo)
{
    if (targets.empty()) {
        trackOptimizationAttempt(TrackedStrategy::Call_Inline);
        trackOptimizationOutcome(TrackedOutcome::CantInlineNoTarget);
        return InliningStatus_NotInlined;
    }

    
    
    
    WrapMGetPropertyCache propCache(getInlineableGetPropertyCache(callInfo));
    keepFallbackFunctionGetter(propCache.get());

    
    
    if (!propCache.get() && targets.length() == 1) {
        JSObject* target = targets[0];

        trackOptimizationAttempt(TrackedStrategy::Call_Inline);
        trackTypeInfo(TrackedTypeSite::Call_Target, target);

        InliningDecision decision = makeInliningDecision(target, callInfo);
        switch (decision) {
          case InliningDecision_Error:
            return InliningStatus_Error;
          case InliningDecision_DontInline:
            return InliningStatus_NotInlined;
          case InliningDecision_WarmUpCountTooLow:
            return InliningStatus_WarmUpCountTooLow;
          case InliningDecision_Inline:
            break;
        }

        
        
        
        callInfo.fun()->setImplicitlyUsedUnchecked();

        
        
        
        if (target->isSingleton()) {
            
            MConstant* constFun = constant(ObjectValue(*target));
            callInfo.setFun(constFun);
        }

        return inlineSingleCall(callInfo, target);
    }

    
    BoolVector choiceSet(alloc());
    uint32_t numInlined;
    if (!selectInliningTargets(targets, callInfo, choiceSet, &numInlined))
        return InliningStatus_Error;
    if (numInlined == 0)
        return InliningStatus_NotInlined;

    
    if (!inlineCalls(callInfo, targets, choiceSet, propCache.get()))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

bool
IonBuilder::inlineGenericFallback(JSFunction* target, CallInfo& callInfo, MBasicBlock* dispatchBlock)
{
    
    MBasicBlock* fallbackBlock = newBlock(dispatchBlock, pc);
    if (!fallbackBlock)
        return false;

    
    CallInfo fallbackInfo(alloc(), callInfo.constructing());
    if (!fallbackInfo.init(callInfo))
        return false;
    fallbackInfo.popFormals(fallbackBlock);

    
    if (!setCurrentAndSpecializePhis(fallbackBlock))
        return false;
    if (!makeCall(target, fallbackInfo))
        return false;

    
    return true;
}

bool
IonBuilder::inlineObjectGroupFallback(CallInfo& callInfo, MBasicBlock* dispatchBlock,
                                     MObjectGroupDispatch* dispatch, MGetPropertyCache* cache,
                                     MBasicBlock** fallbackTarget)
{
    
    
    
    MOZ_ASSERT(callInfo.fun()->isGetPropertyCache() || callInfo.fun()->isTypeBarrier());

    
    MOZ_ASSERT(dispatch->numCases() > 0);

    
    
    MOZ_ASSERT_IF(callInfo.fun()->isGetPropertyCache(), !cache->hasUses());
    MOZ_ASSERT_IF(callInfo.fun()->isTypeBarrier(), cache->hasOneUse());

    
    
    
    MOZ_ASSERT(cache->idempotent());

    
    CallInfo fallbackInfo(alloc(), callInfo.constructing());
    if (!fallbackInfo.init(callInfo))
        return false;

    
    MResumePoint* preCallResumePoint =
        MResumePoint::New(alloc(), dispatchBlock, pc, MResumePoint::ResumeAt);
    if (!preCallResumePoint)
        return false;

    DebugOnly<size_t> preCallFuncIndex = preCallResumePoint->stackDepth() - callInfo.numFormals();
    MOZ_ASSERT(preCallResumePoint->getOperand(preCallFuncIndex) == fallbackInfo.fun());

    
    MConstant* undefined = MConstant::New(alloc(), UndefinedValue());
    dispatchBlock->add(undefined);
    dispatchBlock->rewriteAtDepth(-int(callInfo.numFormals()), undefined);

    
    
    MBasicBlock* prepBlock = newBlock(dispatchBlock, pc);
    if (!prepBlock)
        return false;
    fallbackInfo.popFormals(prepBlock);

    
    
    InlinePropertyTable* propTable = cache->propTable();
    MResumePoint* priorResumePoint = propTable->takePriorResumePoint();
    MOZ_ASSERT(propTable->pc() != nullptr);
    MOZ_ASSERT(priorResumePoint != nullptr);
    MBasicBlock* getPropBlock = newBlock(prepBlock, propTable->pc(), priorResumePoint);
    if (!getPropBlock)
        return false;

    prepBlock->end(MGoto::New(alloc(), getPropBlock));

    
    
    DebugOnly<MDefinition*> checkObject = getPropBlock->pop();
    MOZ_ASSERT(checkObject == cache->object());

    
    if (fallbackInfo.fun()->isGetPropertyCache()) {
        MOZ_ASSERT(fallbackInfo.fun()->toGetPropertyCache() == cache);
        getPropBlock->addFromElsewhere(cache);
        getPropBlock->push(cache);
    } else {
        MTypeBarrier* barrier = callInfo.fun()->toTypeBarrier();
        MOZ_ASSERT(barrier->type() == MIRType_Object);
        MOZ_ASSERT(barrier->input()->isGetPropertyCache());
        MOZ_ASSERT(barrier->input()->toGetPropertyCache() == cache);

        getPropBlock->addFromElsewhere(cache);
        getPropBlock->addFromElsewhere(barrier);
        getPropBlock->push(barrier);
    }

    
    MBasicBlock* preCallBlock = newBlock(getPropBlock, pc, preCallResumePoint);
    if (!preCallBlock)
        return false;
    getPropBlock->end(MGoto::New(alloc(), preCallBlock));

    
    if (!inlineGenericFallback(nullptr, fallbackInfo, preCallBlock))
        return false;

    
    preCallBlock->end(MGoto::New(alloc(), current));
    *fallbackTarget = prepBlock;
    return true;
}

bool
IonBuilder::inlineCalls(CallInfo& callInfo, const ObjectVector& targets, BoolVector& choiceSet,
                        MGetPropertyCache* maybeCache)
{
    
    MOZ_ASSERT(IsIonInlinablePC(pc));
    MOZ_ASSERT(choiceSet.length() == targets.length());
    MOZ_ASSERT_IF(!maybeCache, targets.length() >= 2);
    MOZ_ASSERT_IF(maybeCache, targets.length() >= 1);

    MBasicBlock* dispatchBlock = current;
    callInfo.setImplicitlyUsedUnchecked();
    callInfo.pushFormals(dispatchBlock);

    
    
    
    if (maybeCache) {
        InlinePropertyTable* propTable = maybeCache->propTable();
        propTable->trimToTargets(targets);
        if (propTable->numEntries() == 0)
            maybeCache = nullptr;
    }

    
    MDispatchInstruction* dispatch;
    if (maybeCache) {
        dispatch = MObjectGroupDispatch::New(alloc(), maybeCache->object(), maybeCache->propTable());
        callInfo.fun()->setImplicitlyUsedUnchecked();
    } else {
        dispatch = MFunctionDispatch::New(alloc(), callInfo.fun());
    }

    
    jsbytecode* postCall = GetNextPc(pc);
    MBasicBlock* returnBlock = newBlock(nullptr, postCall);
    if (!returnBlock)
        return false;
    returnBlock->setCallerResumePoint(callerResumePoint_);

    
    returnBlock->inheritSlots(dispatchBlock);
    callInfo.popFormals(returnBlock);

    MPhi* retPhi = MPhi::New(alloc());
    returnBlock->addPhi(retPhi);
    returnBlock->push(retPhi);

    
    returnBlock->initEntrySlots(alloc());

    
    
    uint32_t count = 1; 
    for (uint32_t i = 0; i < targets.length(); i++) {
        if (choiceSet[i])
            count++;
    }
    retPhi->reserveLength(count);

    
    for (uint32_t i = 0; i < targets.length(); i++) {
        
        if (!choiceSet[i])
            continue;

        
        
        amendOptimizationAttempt(i);

        
        JSFunction* target = &targets[i]->as<JSFunction>();
        if (maybeCache && !maybeCache->propTable()->hasFunction(target)) {
            choiceSet[i] = false;
            trackOptimizationOutcome(TrackedOutcome::CantInlineNotInDispatch);
            continue;
        }

        MBasicBlock* inlineBlock = newBlock(dispatchBlock, pc);
        if (!inlineBlock)
            return false;

        
        
        
        MInstruction* funcDef;
        if (target->isSingleton())
            funcDef = MConstant::New(alloc(), ObjectValue(*target), constraints());
        else
            funcDef = MPolyInlineGuard::New(alloc(), callInfo.fun());

        funcDef->setImplicitlyUsedUnchecked();
        dispatchBlock->add(funcDef);

        
        int funIndex = inlineBlock->entryResumePoint()->stackDepth() - callInfo.numFormals();
        inlineBlock->entryResumePoint()->replaceOperand(funIndex, funcDef);
        inlineBlock->rewriteSlot(funIndex, funcDef);

        
        CallInfo inlineInfo(alloc(), callInfo.constructing());
        if (!inlineInfo.init(callInfo))
            return false;
        inlineInfo.popFormals(inlineBlock);
        inlineInfo.setFun(funcDef);

        if (maybeCache) {
            
            
            MOZ_ASSERT(callInfo.thisArg() == maybeCache->object());
            TemporaryTypeSet* thisTypes = maybeCache->propTable()->buildTypeSetForFunction(target);
            if (!thisTypes)
                return false;

            MFilterTypeSet* filter = MFilterTypeSet::New(alloc(), inlineInfo.thisArg(), thisTypes);
            inlineBlock->add(filter);
            inlineInfo.setThis(filter);
        }

        
        if (!setCurrentAndSpecializePhis(inlineBlock))
            return false;
        InliningStatus status = inlineSingleCall(inlineInfo, target);
        if (status == InliningStatus_Error)
            return false;

        
        if (status == InliningStatus_NotInlined) {
            MOZ_ASSERT(target->isNative());
            MOZ_ASSERT(current == inlineBlock);
            graph().removeBlock(inlineBlock);
            choiceSet[i] = false;
            continue;
        }

        
        MBasicBlock* inlineReturnBlock = current;
        setCurrent(dispatchBlock);

        
        ObjectGroup* funcGroup = target->isSingleton() ? nullptr : target->group();
        dispatch->addCase(target, funcGroup, inlineBlock);

        MDefinition* retVal = inlineReturnBlock->peek(-1);
        retPhi->addInput(retVal);
        inlineReturnBlock->end(MGoto::New(alloc(), returnBlock));
        if (!returnBlock->addPredecessorWithoutPhis(inlineReturnBlock))
            return false;
    }

    
    bool useFallback;
    if (maybeCache) {
        InlinePropertyTable* propTable = maybeCache->propTable();
        propTable->trimTo(targets, choiceSet);

        if (propTable->numEntries() == 0) {
            
            MOZ_ASSERT(dispatch->numCases() == 0);
            maybeCache = nullptr;
            useFallback = true;
        } else {
            
            
            useFallback = false;
            TemporaryTypeSet* objectTypes = maybeCache->object()->resultTypeSet();
            for (uint32_t i = 0; i < objectTypes->getObjectCount(); i++) {
                TypeSet::ObjectKey* obj = objectTypes->getObject(i);
                if (!obj)
                    continue;

                if (!obj->isGroup()) {
                    useFallback = true;
                    break;
                }

                if (!propTable->hasObjectGroup(obj->group())) {
                    useFallback = true;
                    break;
                }
            }

            if (!useFallback) {
                
                
                
                if (callInfo.fun()->isGetPropertyCache()) {
                    MOZ_ASSERT(callInfo.fun() == maybeCache);
                } else {
                    MTypeBarrier* barrier = callInfo.fun()->toTypeBarrier();
                    MOZ_ASSERT(!barrier->hasUses());
                    MOZ_ASSERT(barrier->type() == MIRType_Object);
                    MOZ_ASSERT(barrier->input()->isGetPropertyCache());
                    MOZ_ASSERT(barrier->input()->toGetPropertyCache() == maybeCache);
                    barrier->block()->discard(barrier);
                }

                MOZ_ASSERT(!maybeCache->hasUses());
                maybeCache->block()->discard(maybeCache);
            }
        }
    } else {
        useFallback = dispatch->numCases() < targets.length();
    }

    
    if (useFallback) {
        
        if (maybeCache) {
            MBasicBlock* fallbackTarget;
            if (!inlineObjectGroupFallback(callInfo, dispatchBlock,
                                           dispatch->toObjectGroupDispatch(),
                                           maybeCache, &fallbackTarget))
            {
                return false;
            }
            dispatch->addFallback(fallbackTarget);
        } else {
            JSFunction* remaining = nullptr;

            
            
            if (dispatch->numCases() + 1 == targets.length()) {
                for (uint32_t i = 0; i < targets.length(); i++) {
                    if (choiceSet[i])
                        continue;

                    MOZ_ASSERT(!remaining);
                    if (targets[i]->is<JSFunction>() && targets[i]->as<JSFunction>().isSingleton())
                        remaining = &targets[i]->as<JSFunction>();
                    break;
                }
            }

            if (!inlineGenericFallback(remaining, callInfo, dispatchBlock))
                return false;
            dispatch->addFallback(current);
        }

        MBasicBlock* fallbackReturnBlock = current;

        
        MDefinition* retVal = fallbackReturnBlock->peek(-1);
        retPhi->addInput(retVal);
        fallbackReturnBlock->end(MGoto::New(alloc(), returnBlock));
        if (!returnBlock->addPredecessorWithoutPhis(fallbackReturnBlock))
            return false;
    }

    
    
    dispatchBlock->end(dispatch);

    
    MOZ_ASSERT(returnBlock->stackDepth() == dispatchBlock->stackDepth() - callInfo.numFormals() + 1);

    graph().moveBlockToEnd(returnBlock);
    return setCurrentAndSpecializePhis(returnBlock);
}

MInstruction*
IonBuilder::createDeclEnvObject(MDefinition* callee, MDefinition* scope)
{
    
    
    DeclEnvObject* templateObj = inspector->templateDeclEnvObject();

    
    
    MOZ_ASSERT(!templateObj->hasDynamicSlots());

    
    
    
    MInstruction* declEnvObj = MNewDeclEnvObject::New(alloc(), templateObj);
    current->add(declEnvObj);

    
    
    
    
    current->add(MStoreFixedSlot::New(alloc(), declEnvObj, DeclEnvObject::enclosingScopeSlot(), scope));
    current->add(MStoreFixedSlot::New(alloc(), declEnvObj, DeclEnvObject::lambdaSlot(), callee));

    return declEnvObj;
}

MInstruction*
IonBuilder::createCallObject(MDefinition* callee, MDefinition* scope)
{
    
    
    CallObject* templateObj = inspector->templateCallObject();

    
    
    MNullaryInstruction* callObj;
    if (script()->treatAsRunOnce())
        callObj = MNewRunOnceCallObject::New(alloc(), templateObj);
    else
        callObj = MNewCallObject::New(alloc(), templateObj);
    current->add(callObj);

    
    
    current->add(MStoreFixedSlot::New(alloc(), callObj, CallObject::enclosingScopeSlot(), scope));
    current->add(MStoreFixedSlot::New(alloc(), callObj, CallObject::calleeSlot(), callee));

    
    MSlots* slots = nullptr;
    for (AliasedFormalIter i(script()); i; i++) {
        unsigned slot = i.scopeSlot();
        unsigned formal = i.frameIndex();
        MDefinition* param = current->getSlot(info().argSlotUnchecked(formal));
        if (slot >= templateObj->numFixedSlots()) {
            if (!slots) {
                slots = MSlots::New(alloc(), callObj);
                current->add(slots);
            }
            current->add(MStoreSlot::New(alloc(), slots, slot - templateObj->numFixedSlots(), param));
        } else {
            current->add(MStoreFixedSlot::New(alloc(), callObj, slot, param));
        }
    }

    return callObj;
}

MDefinition*
IonBuilder::createThisScripted(MDefinition* callee)
{
    
    
    
    
    
    
    
    
    
    
    
    MInstruction* getProto;
    if (!invalidatedIdempotentCache()) {
        MGetPropertyCache* getPropCache = MGetPropertyCache::New(alloc(), callee, names().prototype,
                                                                  false);
        getPropCache->setIdempotent();
        getProto = getPropCache;
    } else {
        MCallGetProperty* callGetProp = MCallGetProperty::New(alloc(), callee, names().prototype,
                                                               false);
        callGetProp->setIdempotent();
        getProto = callGetProp;
    }
    current->add(getProto);

    
    MCreateThisWithProto* createThis = MCreateThisWithProto::New(alloc(), callee, getProto);
    current->add(createThis);

    return createThis;
}

JSObject*
IonBuilder::getSingletonPrototype(JSFunction* target)
{
    TypeSet::ObjectKey* targetKey = TypeSet::ObjectKey::get(target);
    if (targetKey->unknownProperties())
        return nullptr;

    jsid protoid = NameToId(names().prototype);
    HeapTypeSetKey protoProperty = targetKey->property(protoid);

    return protoProperty.singleton(constraints());
}

MDefinition*
IonBuilder::createThisScriptedSingleton(JSFunction* target, MDefinition* callee)
{
    
    JSObject* proto = getSingletonPrototype(target);
    if (!proto)
        return nullptr;

    JSObject* templateObject = inspector->getTemplateObject(pc);
    if (!templateObject)
        return nullptr;
    if (!templateObject->is<PlainObject>() && !templateObject->is<UnboxedPlainObject>())
        return nullptr;
    if (templateObject->getProto() != proto)
        return nullptr;

    TypeSet::ObjectKey* templateObjectKey = TypeSet::ObjectKey::get(templateObject->group());
    if (templateObjectKey->hasFlags(constraints(), OBJECT_FLAG_NEW_SCRIPT_CLEARED))
        return nullptr;

    StackTypeSet* thisTypes = TypeScript::ThisTypes(target->nonLazyScript());
    if (!thisTypes || !thisTypes->hasType(TypeSet::ObjectType(templateObject)))
        return nullptr;

    
    
    MConstant* templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    MCreateThisWithTemplate* createThis =
        MCreateThisWithTemplate::New(alloc(), constraints(), templateConst,
                                     templateObject->group()->initialHeap(constraints()));
    current->add(templateConst);
    current->add(createThis);

    return createThis;
}

MDefinition*
IonBuilder::createThisScriptedBaseline(MDefinition* callee)
{
    

    JSFunction* target = inspector->getSingleCallee(pc);
    if (!target || !target->hasScript())
        return nullptr;

    JSObject* templateObject = inspector->getTemplateObject(pc);
    if (!templateObject->is<PlainObject>() && !templateObject->is<UnboxedPlainObject>())
        return nullptr;

    Shape* shape = target->lookupPure(compartment->runtime()->names().prototype);
    if (!shape || !shape->hasDefaultGetter() || !shape->hasSlot())
        return nullptr;

    Value protov = target->getSlot(shape->slot());
    if (!protov.isObject())
        return nullptr;

    JSObject* proto = &protov.toObject();
    if (proto != templateObject->getProto())
        return nullptr;

    TypeSet::ObjectKey* templateObjectKey = TypeSet::ObjectKey::get(templateObject->group());
    if (templateObjectKey->hasFlags(constraints(), OBJECT_FLAG_NEW_SCRIPT_CLEARED))
        return nullptr;

    StackTypeSet* thisTypes = TypeScript::ThisTypes(target->nonLazyScript());
    if (!thisTypes || !thisTypes->hasType(TypeSet::ObjectType(templateObject)))
        return nullptr;

    
    callee = addShapeGuard(callee, target->lastProperty(), Bailout_ShapeGuard);

    
    MOZ_ASSERT(shape->numFixedSlots() == 0, "Must be a dynamic slot");
    MSlots* slots = MSlots::New(alloc(), callee);
    current->add(slots);
    MLoadSlot* prototype = MLoadSlot::New(alloc(), slots, shape->slot());
    current->add(prototype);
    MDefinition* protoConst = constantMaybeNursery(proto);
    MGuardObjectIdentity* guard = MGuardObjectIdentity::New(alloc(), prototype, protoConst,
                                                             false);
    current->add(guard);

    
    
    MConstant* templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    MCreateThisWithTemplate* createThis =
        MCreateThisWithTemplate::New(alloc(), constraints(), templateConst,
                                     templateObject->group()->initialHeap(constraints()));
    current->add(templateConst);
    current->add(createThis);

    return createThis;
}

MDefinition*
IonBuilder::createThis(JSFunction* target, MDefinition* callee)
{
    
    if (!target) {
        if (MDefinition* createThis = createThisScriptedBaseline(callee))
            return createThis;

        MCreateThis* createThis = MCreateThis::New(alloc(), callee);
        current->add(createThis);
        return createThis;
    }

    
    if (target->isNative()) {
        if (!target->isNativeConstructor())
            return nullptr;

        MConstant* magic = MConstant::New(alloc(), MagicValue(JS_IS_CONSTRUCTING));
        current->add(magic);
        return magic;
    }

    
    if (MDefinition* createThis = createThisScriptedSingleton(target, callee))
        return createThis;

    if (MDefinition* createThis = createThisScriptedBaseline(callee))
        return createThis;

    return createThisScripted(callee);
}

bool
IonBuilder::jsop_funcall(uint32_t argc)
{
    
    
    
    
    
    

    int calleeDepth = -((int)argc + 2);
    int funcDepth = -((int)argc + 1);

    
    TemporaryTypeSet* calleeTypes = current->peek(calleeDepth)->resultTypeSet();
    JSFunction* native = getSingleCallTarget(calleeTypes);
    if (!native || !native->isNative() || native->native() != &fun_call) {
        CallInfo callInfo(alloc(), false);
        if (!callInfo.init(current, argc))
            return false;
        return makeCall(native, callInfo);
    }
    current->peek(calleeDepth)->setImplicitlyUsedUnchecked();

    
    TemporaryTypeSet* funTypes = current->peek(funcDepth)->resultTypeSet();
    JSFunction* target = getSingleCallTarget(funTypes);

    
    current->shimmySlots(funcDepth - 1);

    bool zeroArguments = (argc == 0);

    
    
    if (zeroArguments) {
        pushConstant(UndefinedValue());
    } else {
        
        argc -= 1;
    }

    CallInfo callInfo(alloc(), false);
    if (!callInfo.init(current, argc))
        return false;

    
    if (!zeroArguments) {
        InliningDecision decision = makeInliningDecision(target, callInfo);
        switch (decision) {
          case InliningDecision_Error:
            return false;
          case InliningDecision_DontInline:
          case InliningDecision_WarmUpCountTooLow:
            break;
          case InliningDecision_Inline:
            if (target->isInterpreted())
                return inlineScriptedCall(callInfo, target);
            break;
        }
    }

    
    return makeCall(target, callInfo);
}

bool
IonBuilder::jsop_funapply(uint32_t argc)
{
    int calleeDepth = -((int)argc + 2);

    TemporaryTypeSet* calleeTypes = current->peek(calleeDepth)->resultTypeSet();
    JSFunction* native = getSingleCallTarget(calleeTypes);
    if (argc != 2 || info().analysisMode() == Analysis_ArgumentsUsage) {
        CallInfo callInfo(alloc(), false);
        if (!callInfo.init(current, argc))
            return false;
        return makeCall(native, callInfo);
    }

    
    
    MDefinition* argument = current->peek(-1);
    if (script()->argumentsHasVarBinding() &&
        argument->mightBeType(MIRType_MagicOptimizedArguments) &&
        argument->type() != MIRType_MagicOptimizedArguments)
    {
        return abort("fun.apply with MaybeArguments");
    }

    
    if (argument->type() != MIRType_MagicOptimizedArguments) {
        CallInfo callInfo(alloc(), false);
        if (!callInfo.init(current, argc))
            return false;
        return makeCall(native, callInfo);
    }

    if ((!native || !native->isNative() ||
        native->native() != fun_apply) &&
        info().analysisMode() != Analysis_DefiniteProperties)
    {
        return abort("fun.apply speculation failed");
    }

    
    return jsop_funapplyarguments(argc);
}

bool
IonBuilder::jsop_funapplyarguments(uint32_t argc)
{
    
    
    
    
    

    int funcDepth = -((int)argc + 1);

    
    TemporaryTypeSet* funTypes = current->peek(funcDepth)->resultTypeSet();
    JSFunction* target = getSingleCallTarget(funTypes);

    
    
    if (inliningDepth_ == 0 && info().analysisMode() != Analysis_DefiniteProperties) {
        
        
        
        
        MDefinition* vp = current->pop();
        vp->setImplicitlyUsedUnchecked();

        MDefinition* argThis = current->pop();

        
        MDefinition* argFunc = current->pop();

        
        MDefinition* nativeFunc = current->pop();
        nativeFunc->setImplicitlyUsedUnchecked();

        MArgumentsLength* numArgs = MArgumentsLength::New(alloc());
        current->add(numArgs);

        MApplyArgs* apply = MApplyArgs::New(alloc(), target, argFunc, numArgs, argThis);
        current->add(apply);
        current->push(apply);
        if (!resumeAfter(apply))
            return false;

        TemporaryTypeSet* types = bytecodeTypes(pc);
        return pushTypeBarrier(apply, types, BarrierKind::TypeSet);
    }

    
    
    
    
    

    CallInfo callInfo(alloc(), false);

    
    MDefinition* vp = current->pop();
    vp->setImplicitlyUsedUnchecked();

    
    if (inliningDepth_) {
        if (!callInfo.setArgs(inlineCallInfo_->argv()))
            return false;
    }

    
    MDefinition* argThis = current->pop();
    callInfo.setThis(argThis);

    
    MDefinition* argFunc = current->pop();
    callInfo.setFun(argFunc);

    
    MDefinition* nativeFunc = current->pop();
    nativeFunc->setImplicitlyUsedUnchecked();

    
    InliningDecision decision = makeInliningDecision(target, callInfo);
    switch (decision) {
      case InliningDecision_Error:
        return false;
      case InliningDecision_DontInline:
      case InliningDecision_WarmUpCountTooLow:
        break;
      case InliningDecision_Inline:
        if (target->isInterpreted())
            return inlineScriptedCall(callInfo, target);
    }

    return makeCall(target, callInfo);
}

bool
IonBuilder::jsop_call(uint32_t argc, bool constructing)
{
    startTrackingOptimizations();

    
    
    TemporaryTypeSet* observed = bytecodeTypes(pc);
    if (observed->empty()) {
        if (BytecodeFlowsToBitop(pc)) {
            observed->addType(TypeSet::Int32Type(), alloc_->lifoAlloc());
        } else if (*GetNextPc(pc) == JSOP_POS) {
            
            
            
            observed->addType(TypeSet::DoubleType(), alloc_->lifoAlloc());
        }
    }

    int calleeDepth = -((int)argc + 2);

    
    ObjectVector targets(alloc());
    TemporaryTypeSet* calleeTypes = current->peek(calleeDepth)->resultTypeSet();
    if (calleeTypes && !getPolyCallTargets(calleeTypes, constructing, targets, 4))
        return false;

    CallInfo callInfo(alloc(), constructing);
    if (!callInfo.init(current, argc))
        return false;

    
    InliningStatus status = inlineCallsite(targets, callInfo);
    if (status == InliningStatus_Inlined)
        return true;
    if (status == InliningStatus_Error)
        return false;

    
    JSFunction* target = nullptr;
    if (targets.length() == 1 && targets[0]->is<JSFunction>())
        target = &targets[0]->as<JSFunction>();

    if (target && status == InliningStatus_WarmUpCountTooLow) {
        MRecompileCheck* check =
            MRecompileCheck::New(alloc(), target->nonLazyScript(),
                                 optimizationInfo().inliningRecompileThreshold(),
                                 MRecompileCheck::RecompileCheck_Inlining);
        current->add(check);
    }

    return makeCall(target, callInfo);
}

bool
IonBuilder::testShouldDOMCall(TypeSet* inTypes, JSFunction* func, JSJitInfo::OpType opType)
{
    if (IsInsideNursery(func))
        return false;

    if (!func->isNative() || !func->jitInfo())
        return false;

    
    
    
    DOMInstanceClassHasProtoAtDepth instanceChecker =
        compartment->runtime()->DOMcallbacks()->instanceClassMatchesProto;

    const JSJitInfo* jinfo = func->jitInfo();
    if (jinfo->type() != opType)
        return false;

    for (unsigned i = 0; i < inTypes->getObjectCount(); i++) {
        TypeSet::ObjectKey* key = inTypes->getObject(i);
        if (!key)
            continue;

        if (!key->hasStableClassAndProto(constraints()))
            return false;

        if (!instanceChecker(key->clasp(), jinfo->protoID, jinfo->depth))
            return false;
    }

    return true;
}

static bool
ArgumentTypesMatch(MDefinition* def, StackTypeSet* calleeTypes)
{
    if (!calleeTypes)
        return false;

    if (def->resultTypeSet()) {
        MOZ_ASSERT(def->type() == MIRType_Value || def->mightBeType(def->type()));
        return def->resultTypeSet()->isSubset(calleeTypes);
    }

    if (def->type() == MIRType_Value)
        return false;

    if (def->type() == MIRType_Object)
        return calleeTypes->unknownObject();

    return calleeTypes->mightBeMIRType(def->type());
}

bool
IonBuilder::testNeedsArgumentCheck(JSFunction* target, CallInfo& callInfo)
{
    
    
    
    if (!target->hasScript())
        return true;

    JSScript* targetScript = target->nonLazyScript();

    if (!ArgumentTypesMatch(callInfo.thisArg(), TypeScript::ThisTypes(targetScript)))
        return true;
    uint32_t expected_args = Min<uint32_t>(callInfo.argc(), target->nargs());
    for (size_t i = 0; i < expected_args; i++) {
        if (!ArgumentTypesMatch(callInfo.getArg(i), TypeScript::ArgTypes(targetScript, i)))
            return true;
    }
    for (size_t i = callInfo.argc(); i < target->nargs(); i++) {
        if (!TypeScript::ArgTypes(targetScript, i)->mightBeMIRType(MIRType_Undefined))
            return true;
    }

    return false;
}

MCall*
IonBuilder::makeCallHelper(JSFunction* target, CallInfo& callInfo)
{
    
    

    uint32_t targetArgs = callInfo.argc();

    
    
    if (target && !target->isNative())
        targetArgs = Max<uint32_t>(target->nargs(), callInfo.argc());

    bool isDOMCall = false;
    if (target && !callInfo.constructing()) {
        
        
        
        TemporaryTypeSet* thisTypes = callInfo.thisArg()->resultTypeSet();
        if (thisTypes &&
            thisTypes->getKnownMIRType() == MIRType_Object &&
            thisTypes->isDOMClass(constraints()) &&
            testShouldDOMCall(thisTypes, target, JSJitInfo::Method))
        {
            isDOMCall = true;
        }
    }

    MCall* call = MCall::New(alloc(), target, targetArgs + 1, callInfo.argc(),
                             callInfo.constructing(), isDOMCall);
    if (!call)
        return nullptr;

    
    
    for (int i = targetArgs; i > (int)callInfo.argc(); i--) {
        MOZ_ASSERT_IF(target, !target->isNative());
        MConstant* undef = constant(UndefinedValue());
        call->addArg(i, undef);
    }

    
    
    for (int32_t i = callInfo.argc() - 1; i >= 0; i--)
        call->addArg(i + 1, callInfo.getArg(i));

    
    call->computeMovable();

    
    if (callInfo.constructing()) {
        MDefinition* create = createThis(target, callInfo.fun());
        if (!create) {
            abort("Failure inlining constructor for call.");
            return nullptr;
        }

        callInfo.thisArg()->setImplicitlyUsedUnchecked();
        callInfo.setThis(create);
    }

    
    MDefinition* thisArg = callInfo.thisArg();
    call->addArg(0, thisArg);

    if (target && !testNeedsArgumentCheck(target, callInfo))
        call->disableArgCheck();

    call->initFunction(callInfo.fun());

    current->add(call);
    return call;
}

static bool
DOMCallNeedsBarrier(const JSJitInfo* jitinfo, TemporaryTypeSet* types)
{
    
    
    if (jitinfo->returnType() == JSVAL_TYPE_UNKNOWN)
        return true;

    
    
    if (jitinfo->returnType() == JSVAL_TYPE_OBJECT)
        return true;

    
    return MIRTypeFromValueType(jitinfo->returnType()) != types->getKnownMIRType();
}

bool
IonBuilder::makeCall(JSFunction* target, CallInfo& callInfo)
{
    
    
    MOZ_ASSERT_IF(callInfo.constructing() && target,
                  target->isInterpretedConstructor() || target->isNativeConstructor());

    MCall* call = makeCallHelper(target, callInfo);
    if (!call)
        return false;

    current->push(call);
    if (call->isEffectful() && !resumeAfter(call))
        return false;

    TemporaryTypeSet* types = bytecodeTypes(pc);

    if (call->isCallDOMNative())
        return pushDOMTypeBarrier(call, types, call->getSingleTarget());

    return pushTypeBarrier(call, types, BarrierKind::TypeSet);
}

bool
IonBuilder::jsop_eval(uint32_t argc)
{
    int calleeDepth = -((int)argc + 2);
    TemporaryTypeSet* calleeTypes = current->peek(calleeDepth)->resultTypeSet();

    
    
    if (calleeTypes && calleeTypes->empty())
        return jsop_call(argc,  false);

    JSFunction* singleton = getSingleCallTarget(calleeTypes);
    if (!singleton)
        return abort("No singleton callee for eval()");

    if (script()->global().valueIsEval(ObjectValue(*singleton))) {
        if (argc != 1)
            return abort("Direct eval with more than one argument");

        if (!info().funMaybeLazy())
            return abort("Direct eval in global code");

        if (info().funMaybeLazy()->isArrow())
            return abort("Direct eval from arrow function");

        
        
        
        
        MIRType type = thisTypes ? thisTypes->getKnownMIRType() : MIRType_Value;
        if (type != MIRType_Object && type != MIRType_Null && type != MIRType_Undefined)
            return abort("Direct eval from script with maybe-primitive 'this'");

        CallInfo callInfo(alloc(),  false);
        if (!callInfo.init(current, argc))
            return false;
        callInfo.setImplicitlyUsedUnchecked();

        callInfo.fun()->setImplicitlyUsedUnchecked();

        MDefinition* scopeChain = current->scopeChain();
        MDefinition* string = callInfo.getArg(0);

        
        
        if (!string->mightBeType(MIRType_String)) {
            current->push(string);
            TemporaryTypeSet* types = bytecodeTypes(pc);
            return pushTypeBarrier(string, types, BarrierKind::TypeSet);
        }

        current->pushSlot(info().thisSlot());
        MDefinition* thisValue = current->pop();

        
        
        
        if (string->isConcat() &&
            string->getOperand(1)->isConstantValue() &&
            string->getOperand(1)->constantValue().isString())
        {
            JSAtom* atom = &string->getOperand(1)->constantValue().toString()->asAtom();

            if (StringEqualsAscii(atom, "()")) {
                MDefinition* name = string->getOperand(0);
                MInstruction* dynamicName = MGetDynamicName::New(alloc(), scopeChain, name);
                current->add(dynamicName);

                current->push(dynamicName);
                current->push(thisValue);

                CallInfo evalCallInfo(alloc(),  false);
                if (!evalCallInfo.init(current,  0))
                    return false;

                return makeCall(nullptr, evalCallInfo);
            }
        }

        MInstruction* filterArguments = MFilterArgumentsOrEval::New(alloc(), string);
        current->add(filterArguments);

        MInstruction* ins = MCallDirectEval::New(alloc(), scopeChain, string, thisValue, pc);
        current->add(ins);
        current->push(ins);

        TemporaryTypeSet* types = bytecodeTypes(pc);
        return resumeAfter(ins) && pushTypeBarrier(ins, types, BarrierKind::TypeSet);
    }

    return jsop_call(argc,  false);
}

bool
IonBuilder::jsop_compare(JSOp op)
{
    MDefinition* right = current->pop();
    MDefinition* left = current->pop();

    MCompare* ins = MCompare::New(alloc(), left, right, op);
    current->add(ins);
    current->push(ins);

    ins->infer(constraints(), inspector, pc);

    if (ins->isEffectful() && !resumeAfter(ins))
        return false;
    return true;
}

bool
IonBuilder::jsop_newarray(uint32_t count)
{
    JSObject* templateObject = inspector->getTemplateObject(pc);
    if (!templateObject) {
        if (info().analysisMode() == Analysis_ArgumentsUsage) {
            MUnknownValue* unknown = MUnknownValue::New(alloc());
            current->add(unknown);
            current->push(unknown);
            return true;
        }
        return abort("No template object for NEWARRAY");
    }

    MOZ_ASSERT(templateObject->is<ArrayObject>());
    if (templateObject->group()->unknownProperties()) {
        if (info().analysisMode() == Analysis_ArgumentsUsage) {
            MUnknownValue* unknown = MUnknownValue::New(alloc());
            current->add(unknown);
            current->push(unknown);
            return true;
        }
        
        
        return abort("New array has unknown properties");
    }

    MConstant* templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    current->add(templateConst);

    MNewArray* ins = MNewArray::New(alloc(), constraints(), count, templateConst,
                                    templateObject->group()->initialHeap(constraints()),
                                    NewArray_FullyAllocating);
    current->add(ins);
    current->push(ins);
    return true;
}

bool
IonBuilder::jsop_newarray_copyonwrite()
{
    ArrayObject* templateObject = ObjectGroup::getCopyOnWriteObject(script(), pc);

    
    
    
    
    MOZ_ASSERT_IF(info().analysisMode() != Analysis_ArgumentsUsage,
                  templateObject->group()->hasAnyFlags(OBJECT_FLAG_COPY_ON_WRITE));

    MNewArrayCopyOnWrite* ins =
        MNewArrayCopyOnWrite::New(alloc(), constraints(), templateObject,
                                  templateObject->group()->initialHeap(constraints()));

    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_newobject()
{
    JSObject* templateObject = inspector->getTemplateObject(pc);
    gc::InitialHeap heap;
    MConstant* templateConst;

    if (templateObject) {
        heap = templateObject->group()->initialHeap(constraints());
        templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    } else {
        heap = gc::DefaultHeap;
        templateConst = MConstant::New(alloc(), NullValue());
    }

    current->add(templateConst);
    MNewObject* ins = MNewObject::New(alloc(), constraints(), templateConst, heap,
                                      MNewObject::ObjectLiteral);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_initelem()
{
    MDefinition* value = current->pop();
    MDefinition* id = current->pop();
    MDefinition* obj = current->peek(-1);

    MInitElem* initElem = MInitElem::New(alloc(), obj, id, value);
    current->add(initElem);

    return resumeAfter(initElem);
}

bool
IonBuilder::jsop_initelem_array()
{
    MDefinition* value = current->pop();
    MDefinition* obj = current->peek(-1);

    
    
    
    bool needStub = false;
    if (obj->isUnknownValue()) {
        needStub = true;
    } else {
        TypeSet::ObjectKey* initializer = obj->resultTypeSet()->getObject(0);
        if (value->type() == MIRType_MagicHole) {
            if (!initializer->hasFlags(constraints(), OBJECT_FLAG_NON_PACKED))
                needStub = true;
        } else if (!initializer->unknownProperties()) {
            HeapTypeSetKey elemTypes = initializer->property(JSID_VOID);
            if (!TypeSetIncludes(elemTypes.maybeTypes(), value->type(), value->resultTypeSet())) {
                elemTypes.freeze(constraints());
                needStub = true;
            }
        }
    }

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    if (needStub) {
        MCallInitElementArray* store = MCallInitElementArray::New(alloc(), obj, GET_UINT24(pc), value);
        current->add(store);
        return resumeAfter(store);
    }

    MConstant* id = MConstant::New(alloc(), Int32Value(GET_UINT24(pc)));
    current->add(id);

    
    MElements* elements = MElements::New(alloc(), obj);
    current->add(elements);

    if (obj->toNewArray()->convertDoubleElements()) {
        MInstruction* valueDouble = MToDouble::New(alloc(), value);
        current->add(valueDouble);
        value = valueDouble;
    }

    
    MStoreElement* store = MStoreElement::New(alloc(), elements, id, value,  false);
    current->add(store);

    
    
    
    MSetInitializedLength* initLength = MSetInitializedLength::New(alloc(), elements, id);
    current->add(initLength);

    if (!resumeAfter(initLength))
        return false;

   return true;
}

bool
IonBuilder::jsop_mutateproto()
{
    MDefinition* value = current->pop();
    MDefinition* obj = current->peek(-1);

    MMutateProto* mutate = MMutateProto::New(alloc(), obj, value);
    current->add(mutate);
    return resumeAfter(mutate);
}

bool
IonBuilder::jsop_initprop(PropertyName* name)
{
    bool useSlowPath = false;

    MDefinition* value = current->peek(-1);
    MDefinition* obj = current->peek(-2);
    if (obj->isLambda()) {
        useSlowPath = true;
    } else if (JSObject* templateObject = obj->toNewObject()->templateObject()) {
        if (templateObject->is<PlainObject>()) {
            if (!templateObject->as<PlainObject>().containsPure(name))
                useSlowPath = true;
        } else {
            MOZ_ASSERT(templateObject->as<UnboxedPlainObject>().layout().lookup(name));
        }
    } else {
        useSlowPath = true;
    }

    if (useSlowPath) {
        current->pop();
        MInitProp* init = MInitProp::New(alloc(), obj, name, value);
        current->add(init);
        return resumeAfter(init);
    }

    MInstruction* last = *current->rbegin();

    
    
    if (!jsop_setprop(name))
        return false;

    
    
    current->pop();
    current->push(obj);
    for (MInstructionReverseIterator riter = current->rbegin(); *riter != last; riter++) {
        if (MResumePoint* resumePoint = riter->resumePoint()) {
            MOZ_ASSERT(resumePoint->pc() == pc);
            if (resumePoint->mode() == MResumePoint::ResumeAfter) {
                size_t index = resumePoint->numOperands() - 1;
                resumePoint->replaceOperand(index, obj);
            }
            break;
        }
    }

    return true;
}

bool
IonBuilder::jsop_initprop_getter_setter(PropertyName* name)
{
    MDefinition* value = current->pop();
    MDefinition* obj = current->peek(-1);

    MInitPropGetterSetter* init = MInitPropGetterSetter::New(alloc(), obj, name, value);
    current->add(init);
    return resumeAfter(init);
}

bool
IonBuilder::jsop_initelem_getter_setter()
{
    MDefinition* value = current->pop();
    MDefinition* id = current->pop();
    MDefinition* obj = current->peek(-1);

    MInitElemGetterSetter* init = MInitElemGetterSetter::New(alloc(), obj, id, value);
    current->add(init);
    return resumeAfter(init);
}

MBasicBlock*
IonBuilder::addBlock(MBasicBlock* block, uint32_t loopDepth)
{
    if (!block)
        return nullptr;
    graph().addBlock(block);
    block->setLoopDepth(loopDepth);
    return block;
}

MBasicBlock*
IonBuilder::newBlock(MBasicBlock* predecessor, jsbytecode* pc)
{
    MBasicBlock* block = MBasicBlock::New(graph(), &analysis(), info(), predecessor,
                                          bytecodeSite(pc), MBasicBlock::NORMAL);
    return addBlock(block, loopDepth_);
}

MBasicBlock*
IonBuilder::newBlock(MBasicBlock* predecessor, jsbytecode* pc, MResumePoint* priorResumePoint)
{
    MBasicBlock* block = MBasicBlock::NewWithResumePoint(graph(), info(), predecessor,
                                                         bytecodeSite(pc), priorResumePoint);
    return addBlock(block, loopDepth_);
}

MBasicBlock*
IonBuilder::newBlockPopN(MBasicBlock* predecessor, jsbytecode* pc, uint32_t popped)
{
    MBasicBlock* block = MBasicBlock::NewPopN(graph(), info(), predecessor, bytecodeSite(pc),
                                              MBasicBlock::NORMAL, popped);
    return addBlock(block, loopDepth_);
}

MBasicBlock*
IonBuilder::newBlockAfter(MBasicBlock* at, MBasicBlock* predecessor, jsbytecode* pc)
{
    MBasicBlock* block = MBasicBlock::New(graph(), &analysis(), info(), predecessor,
                                          bytecodeSite(pc), MBasicBlock::NORMAL);
    if (!block)
        return nullptr;
    graph().insertBlockAfter(at, block);
    return block;
}

MBasicBlock*
IonBuilder::newBlock(MBasicBlock* predecessor, jsbytecode* pc, uint32_t loopDepth)
{
    MBasicBlock* block = MBasicBlock::New(graph(), &analysis(), info(), predecessor,
                                          bytecodeSite(pc), MBasicBlock::NORMAL);
    return addBlock(block, loopDepth);
}

MBasicBlock*
IonBuilder::newOsrPreheader(MBasicBlock* predecessor, jsbytecode* loopEntry)
{
    MOZ_ASSERT(LoopEntryCanIonOsr(loopEntry));
    MOZ_ASSERT(loopEntry == info().osrPc());

    
    
    
    MBasicBlock* osrBlock  = newBlockAfter(*graph().begin(), loopEntry);
    MBasicBlock* preheader = newBlock(predecessor, loopEntry);
    if (!osrBlock || !preheader)
        return nullptr;

    MOsrEntry* entry = MOsrEntry::New(alloc());
    osrBlock->add(entry);

    
    {
        uint32_t slot = info().scopeChainSlot();

        MInstruction* scopev;
        if (analysis().usesScopeChain()) {
            scopev = MOsrScopeChain::New(alloc(), entry);
        } else {
            
            
            
            scopev = MConstant::New(alloc(), UndefinedValue());
        }

        osrBlock->add(scopev);
        osrBlock->initSlot(slot, scopev);
    }
    
    {
        MInstruction* returnValue;
        if (!script()->noScriptRval())
            returnValue = MOsrReturnValue::New(alloc(), entry);
        else
            returnValue = MConstant::New(alloc(), UndefinedValue());
        osrBlock->add(returnValue);
        osrBlock->initSlot(info().returnValueSlot(), returnValue);
    }

    
    bool needsArgsObj = info().needsArgsObj();
    MInstruction* argsObj = nullptr;
    if (info().hasArguments()) {
        if (needsArgsObj)
            argsObj = MOsrArgumentsObject::New(alloc(), entry);
        else
            argsObj = MConstant::New(alloc(), UndefinedValue());
        osrBlock->add(argsObj);
        osrBlock->initSlot(info().argsObjSlot(), argsObj);
    }

    if (info().funMaybeLazy()) {
        
        MParameter* thisv = MParameter::New(alloc(), MParameter::THIS_SLOT, nullptr);
        osrBlock->add(thisv);
        osrBlock->initSlot(info().thisSlot(), thisv);

        
        for (uint32_t i = 0; i < info().nargs(); i++) {
            uint32_t slot = needsArgsObj ? info().argSlotUnchecked(i) : info().argSlot(i);

            
            
            
            
            if (needsArgsObj && info().argsObjAliasesFormals()) {
                MOZ_ASSERT(argsObj && argsObj->isOsrArgumentsObject());
                
                
                
                
                
                
                MInstruction* osrv;
                if (script()->formalIsAliased(i))
                    osrv = MConstant::New(alloc(), UndefinedValue());
                else
                    osrv = MGetArgumentsObjectArg::New(alloc(), argsObj, i);

                osrBlock->add(osrv);
                osrBlock->initSlot(slot, osrv);
            } else {
                MParameter* arg = MParameter::New(alloc(), i, nullptr);
                osrBlock->add(arg);
                osrBlock->initSlot(slot, arg);
            }
        }
    }

    
    for (uint32_t i = 0; i < info().nlocals(); i++) {
        uint32_t slot = info().localSlot(i);
        ptrdiff_t offset = BaselineFrame::reverseOffsetOfLocal(i);

        MOsrValue* osrv = MOsrValue::New(alloc(), entry, offset);
        osrBlock->add(osrv);
        osrBlock->initSlot(slot, osrv);
    }

    
    uint32_t numStackSlots = preheader->stackDepth() - info().firstStackSlot();
    for (uint32_t i = 0; i < numStackSlots; i++) {
        uint32_t slot = info().stackSlot(i);
        ptrdiff_t offset = BaselineFrame::reverseOffsetOfLocal(info().nlocals() + i);

        MOsrValue* osrv = MOsrValue::New(alloc(), entry, offset);
        osrBlock->add(osrv);
        osrBlock->initSlot(slot, osrv);
    }

    
    MStart* start = MStart::New(alloc(), MStart::StartType_Osr);
    osrBlock->add(start);

    
    
    if (!resumeAt(start, loopEntry))
        return nullptr;

    
    
    
    if (!osrBlock->linkOsrValues(start))
        return nullptr;

    
    
    
    MOZ_ASSERT(predecessor->stackDepth() == osrBlock->stackDepth());
    MOZ_ASSERT(info().scopeChainSlot() == 0);

    
    
    
    
    for (uint32_t i = info().startArgSlot(); i < osrBlock->stackDepth(); i++) {
        MDefinition* existing = current->getSlot(i);
        MDefinition* def = osrBlock->getSlot(i);
        MOZ_ASSERT_IF(!needsArgsObj || !info().isSlotAliasedAtOsr(i), def->type() == MIRType_Value);

        
        
        if (info().isSlotAliasedAtOsr(i))
            continue;

        def->setResultType(existing->type());
        def->setResultTypeSet(existing->resultTypeSet());
    }

    
    osrBlock->end(MGoto::New(alloc(), preheader));
    if (!preheader->addPredecessor(alloc(), osrBlock))
        return nullptr;
    graph().setOsrBlock(osrBlock);

    return preheader;
}

MBasicBlock*
IonBuilder::newPendingLoopHeader(MBasicBlock* predecessor, jsbytecode* pc, bool osr, bool canOsr,
                                 unsigned stackPhiCount)
{
    loopDepth_++;
    
    if (canOsr)
        stackPhiCount = predecessor->stackDepth() - info().firstStackSlot();
    MBasicBlock* block = MBasicBlock::NewPendingLoopHeader(graph(), info(), predecessor,
                                                           bytecodeSite(pc), stackPhiCount);
    if (!addBlock(block, loopDepth_))
        return nullptr;

    if (osr) {
        
        
        
        
        

        
        for (uint32_t i = info().startArgSlot(); i < block->stackDepth(); i++) {

            
            
            if (info().isSlotAliasedAtOsr(i))
                continue;

            
            
            if (i >= info().firstStackSlot())
                continue;

            MPhi* phi = block->getSlot(i)->toPhi();

            
            TypeSet::Type existingType = TypeSet::UndefinedType();
            uint32_t arg = i - info().firstArgSlot();
            uint32_t var = i - info().firstLocalSlot();
            if (info().funMaybeLazy() && i == info().thisSlot())
                existingType = baselineFrame_->thisType;
            else if (arg < info().nargs())
                existingType = baselineFrame_->argTypes[arg];
            else
                existingType = baselineFrame_->varTypes[var];

            
            LifoAlloc* lifoAlloc = alloc().lifoAlloc();
            TemporaryTypeSet* typeSet =
                lifoAlloc->new_<TemporaryTypeSet>(lifoAlloc, existingType);
            if (!typeSet)
                return nullptr;
            MIRType type = typeSet->getKnownMIRType();
            if (!phi->addBackedgeType(type, typeSet))
                return nullptr;
        }
    }

    return block;
}

MTest*
IonBuilder::newTest(MDefinition* ins, MBasicBlock* ifTrue, MBasicBlock* ifFalse)
{
    MTest* test = MTest::New(alloc(), ins, ifTrue, ifFalse);
    test->cacheOperandMightEmulateUndefined(constraints());
    return test;
}
























bool
IonBuilder::resume(MInstruction* ins, jsbytecode* pc, MResumePoint::Mode mode)
{
    MOZ_ASSERT(ins->isEffectful() || !ins->isMovable());

    MResumePoint* resumePoint = MResumePoint::New(alloc(), ins->block(), pc,
                                                  mode);
    if (!resumePoint)
        return false;
    ins->setResumePoint(resumePoint);
    return true;
}

bool
IonBuilder::resumeAt(MInstruction* ins, jsbytecode* pc)
{
    return resume(ins, pc, MResumePoint::ResumeAt);
}

bool
IonBuilder::resumeAfter(MInstruction* ins)
{
    return resume(ins, pc, MResumePoint::ResumeAfter);
}

bool
IonBuilder::maybeInsertResume()
{
    
    
    
    
    
    
    
    
    

    if (loopDepth_ == 0)
        return true;

    MNop* ins = MNop::New(alloc());
    current->add(ins);

    return resumeAfter(ins);
}


static bool
ClassHasEffectlessLookup(const Class* clasp)
{
    return (clasp == &UnboxedPlainObject::class_) ||
           IsTypedObjectClass(clasp) ||
           (clasp->isNative() && !clasp->ops.lookupProperty);
}



static bool
ObjectHasExtraOwnProperty(CompileCompartment* comp, TypeSet::ObjectKey* object, PropertyName* name)
{
    
    if (object->isGroup() && object->group()->maybeTypeDescr())
        return object->group()->typeDescr().hasProperty(comp->runtime()->names(), NameToId(name));

    const Class* clasp = object->clasp();

    
    if (clasp == &ArrayObject::class_)
        return name == comp->runtime()->names().length;

    
    if (!clasp->resolve)
        return false;

    if (clasp->resolve == str_resolve) {
        
        return false;
    }

    if (clasp->resolve == fun_resolve)
        return FunctionHasResolveHook(comp->runtime()->names(), NameToId(name));

    return true;
}

void
IonBuilder::insertRecompileCheck()
{
    
    OptimizationLevel curLevel = optimizationInfo().level();
    if (js_IonOptimizations.isLastLevel(curLevel))
        return;

    

    
    
    IonBuilder* topBuilder = outermostBuilder();

    
    
    OptimizationLevel nextLevel = js_IonOptimizations.nextLevel(curLevel);
    const OptimizationInfo* info = js_IonOptimizations.get(nextLevel);
    uint32_t warmUpThreshold = info->compilerWarmUpThreshold(topBuilder->script());
    MRecompileCheck* check = MRecompileCheck::New(alloc(), topBuilder->script(), warmUpThreshold,
                                MRecompileCheck::RecompileCheck_OptimizationLevel);
    current->add(check);
}

JSObject*
IonBuilder::testSingletonProperty(JSObject* obj, PropertyName* name)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    while (obj) {
        if (!ClassHasEffectlessLookup(obj->getClass()))
            return nullptr;

        TypeSet::ObjectKey* objKey = TypeSet::ObjectKey::get(obj);
        if (analysisContext)
            objKey->ensureTrackedProperty(analysisContext, NameToId(name));

        if (objKey->unknownProperties())
            return nullptr;

        HeapTypeSetKey property = objKey->property(NameToId(name));
        if (property.isOwnProperty(constraints())) {
            if (obj->isSingleton())
                return property.singleton(constraints());
            return nullptr;
        }

        if (ObjectHasExtraOwnProperty(compartment, objKey, name))
            return nullptr;

        obj = obj->getProto();
    }

    return nullptr;
}

bool
IonBuilder::testSingletonPropertyTypes(MDefinition* obj, JSObject* singleton, PropertyName* name,
                                       bool* testObject, bool* testString)
{
    
    
    

    *testObject = false;
    *testString = false;

    TemporaryTypeSet* types = obj->resultTypeSet();
    if (types && types->unknownObject())
        return false;

    JSObject* objectSingleton = types ? types->maybeSingleton() : nullptr;
    if (objectSingleton)
        return testSingletonProperty(objectSingleton, name) == singleton;

    JSProtoKey key;
    switch (obj->type()) {
      case MIRType_String:
        key = JSProto_String;
        break;

      case MIRType_Symbol:
        key = JSProto_Symbol;
        break;

      case MIRType_Int32:
      case MIRType_Double:
        key = JSProto_Number;
        break;

      case MIRType_Boolean:
        key = JSProto_Boolean;
        break;

      case MIRType_Object:
      case MIRType_Value: {
        if (!types)
            return false;

        if (types->hasType(TypeSet::StringType())) {
            key = JSProto_String;
            *testString = true;
            break;
        }

        if (!types->maybeObject())
            return false;

        
        
        
        for (unsigned i = 0; i < types->getObjectCount(); i++) {
            TypeSet::ObjectKey* key = types->getObject(i);
            if (!key)
                continue;
            if (analysisContext)
                key->ensureTrackedProperty(analysisContext, NameToId(name));

            const Class* clasp = key->clasp();
            if (!ClassHasEffectlessLookup(clasp) || ObjectHasExtraOwnProperty(compartment, key, name))
                return false;
            if (key->unknownProperties())
                return false;
            HeapTypeSetKey property = key->property(NameToId(name));
            if (property.isOwnProperty(constraints()))
                return false;

            if (JSObject* proto = key->proto().toObjectOrNull()) {
                
                if (testSingletonProperty(proto, name) != singleton)
                    return false;
            } else {
                
                return false;
            }
        }
        
        *testObject = (obj->type() != MIRType_Object);
        return true;
      }
      default:
        return false;
    }

    JSObject* proto = GetBuiltinPrototypePure(&script()->global(), key);
    if (proto)
        return testSingletonProperty(proto, name) == singleton;

    return false;
}

bool
IonBuilder::pushTypeBarrier(MDefinition* def, TemporaryTypeSet* observed, BarrierKind kind)
{
    MOZ_ASSERT(def == current->peek(-1));

    MDefinition* replace = addTypeBarrier(current->pop(), observed, kind);
    if (!replace)
        return false;

    current->push(replace);
    return true;
}








MDefinition*
IonBuilder::addTypeBarrier(MDefinition* def, TemporaryTypeSet* observed, BarrierKind kind,
                           MTypeBarrier** pbarrier)
{
    
    if (BytecodeIsPopped(pc))
        return def;

    
    
    
    
    
    if (kind == BarrierKind::NoBarrier) {
        MDefinition* replace = ensureDefiniteType(def, observed->getKnownMIRType());
        replace->setResultTypeSet(observed);
        return replace;
    }

    if (observed->unknown())
        return def;

    MTypeBarrier* barrier = MTypeBarrier::New(alloc(), def, observed, kind);
    current->add(barrier);

    if (pbarrier)
        *pbarrier = barrier;

    if (barrier->type() == MIRType_Undefined)
        return constant(UndefinedValue());
    if (barrier->type() == MIRType_Null)
        return constant(NullValue());

    return barrier;
}

bool
IonBuilder::pushDOMTypeBarrier(MInstruction* ins, TemporaryTypeSet* observed, JSFunction* func)
{
    MOZ_ASSERT(func && func->isNative() && func->jitInfo());

    const JSJitInfo* jitinfo = func->jitInfo();
    bool barrier = DOMCallNeedsBarrier(jitinfo, observed);
    
    
    
    
    
    
    
    MDefinition* replace = ins;
    if (jitinfo->returnType() != JSVAL_TYPE_DOUBLE ||
        observed->getKnownMIRType() != MIRType_Int32) {
        replace = ensureDefiniteType(ins, MIRTypeFromValueType(jitinfo->returnType()));
        if (replace != ins) {
            current->pop();
            current->push(replace);
        }
    } else {
        MOZ_ASSERT(barrier);
    }

    return pushTypeBarrier(replace, observed,
                           barrier ? BarrierKind::TypeSet : BarrierKind::NoBarrier);
}

MDefinition*
IonBuilder::ensureDefiniteType(MDefinition* def, MIRType definiteType)
{
    MInstruction* replace;
    switch (definiteType) {
      case MIRType_Undefined:
        def->setImplicitlyUsedUnchecked();
        replace = MConstant::New(alloc(), UndefinedValue());
        break;

      case MIRType_Null:
        def->setImplicitlyUsedUnchecked();
        replace = MConstant::New(alloc(), NullValue());
        break;

      case MIRType_Value:
        return def;

      default: {
        if (def->type() != MIRType_Value) {
            if (def->type() == MIRType_Int32 && definiteType == MIRType_Double) {
                replace = MToDouble::New(alloc(), def);
                break;
            }
            MOZ_ASSERT(def->type() == definiteType);
            return def;
        }
        replace = MUnbox::New(alloc(), def, definiteType, MUnbox::Infallible);
        break;
      }
    }

    current->add(replace);
    return replace;
}

MDefinition*
IonBuilder::ensureDefiniteTypeSet(MDefinition* def, TemporaryTypeSet* types)
{
    
    

    
    
    MDefinition* replace = ensureDefiniteType(def, types->getKnownMIRType());
    if (replace != def) {
        replace->setResultTypeSet(types);
        return replace;
    }

    
    if (def->type() != types->getKnownMIRType()) {
        MOZ_ASSERT(types->getKnownMIRType() == MIRType_Value);
        return def;
    }

    
    MFilterTypeSet* filter = MFilterTypeSet::New(alloc(), def, types);
    current->add(filter);
    return filter;
}

static size_t
NumFixedSlots(JSObject* object)
{
    
    
    
    
    gc::AllocKind kind = object->asTenured().getAllocKind();
    return gc::GetGCKindSlots(kind, object->getClass());
}

bool
IonBuilder::getStaticName(JSObject* staticObject, PropertyName* name, bool* psucceeded,
                          MDefinition* lexicalCheck)
{
    jsid id = NameToId(name);

    MOZ_ASSERT(staticObject->is<GlobalObject>() || staticObject->is<CallObject>());
    MOZ_ASSERT(staticObject->isSingleton());

    *psucceeded = true;

    if (staticObject->is<GlobalObject>()) {
        
        if (lexicalCheck)
            lexicalCheck->setNotGuardUnchecked();

        
        if (name == names().undefined)
            return pushConstant(UndefinedValue());
        if (name == names().NaN)
            return pushConstant(compartment->runtime()->NaNValue());
        if (name == names().Infinity)
            return pushConstant(compartment->runtime()->positiveInfinityValue());
    }

    
    
    
    if (lexicalCheck) {
        *psucceeded = false;
        return true;
    }

    TypeSet::ObjectKey* staticKey = TypeSet::ObjectKey::get(staticObject);
    if (analysisContext)
        staticKey->ensureTrackedProperty(analysisContext, NameToId(name));

    if (staticKey->unknownProperties()) {
        *psucceeded = false;
        return true;
    }

    HeapTypeSetKey property = staticKey->property(id);
    if (!property.maybeTypes() ||
        !property.maybeTypes()->definiteProperty() ||
        property.nonData(constraints()))
    {
        
        
        *psucceeded = false;
        return true;
    }

    TemporaryTypeSet* types = bytecodeTypes(pc);
    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(), staticKey,
                                                       name, types,  true);

    JSObject* singleton = types->maybeSingleton();

    MIRType knownType = types->getKnownMIRType();
    if (barrier == BarrierKind::NoBarrier) {
        
        if (singleton) {
            if (testSingletonProperty(staticObject, name) == singleton)
                return pushConstant(ObjectValue(*singleton));
        }

        
        Value constantValue;
        if (property.constant(constraints(), &constantValue))
            return pushConstant(constantValue);

        
        if (knownType == MIRType_Undefined)
            return pushConstant(UndefinedValue());
        if (knownType == MIRType_Null)
            return pushConstant(NullValue());
    }

    MInstruction* obj = constant(ObjectValue(*staticObject));

    MIRType rvalType = types->getKnownMIRType();
    if (barrier != BarrierKind::NoBarrier)
        rvalType = MIRType_Value;

    return loadSlot(obj, property.maybeTypes()->definiteSlot(), NumFixedSlots(staticObject),
                    rvalType, barrier, types);
}


bool
jit::NeedsPostBarrier(CompileInfo& info, MDefinition* value)
{
    if (!GetJitContext()->runtime->gcNursery().exists())
        return false;
    return value->mightBeType(MIRType_Object);
}

bool
IonBuilder::setStaticName(JSObject* staticObject, PropertyName* name)
{
    jsid id = NameToId(name);

    MOZ_ASSERT(staticObject->is<GlobalObject>() || staticObject->is<CallObject>());

    MDefinition* value = current->peek(-1);

    TypeSet::ObjectKey* staticKey = TypeSet::ObjectKey::get(staticObject);
    if (staticKey->unknownProperties())
        return jsop_setprop(name);

    HeapTypeSetKey property = staticKey->property(id);
    if (!property.maybeTypes() ||
        !property.maybeTypes()->definiteProperty() ||
        property.nonData(constraints()) ||
        property.nonWritable(constraints()))
    {
        
        
        return jsop_setprop(name);
    }

    if (!CanWriteProperty(alloc(), constraints(), property, value))
        return jsop_setprop(name);

    current->pop();

    
    MDefinition* obj = current->pop();
    MOZ_ASSERT(&obj->toConstant()->value().toObject() == staticObject);

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    
    
    MIRType slotType = MIRType_None;
    MIRType knownType = property.knownMIRType(constraints());
    if (knownType != MIRType_Value)
        slotType = knownType;

    bool needsBarrier = property.needsBarrier(constraints());
    return storeSlot(obj, property.maybeTypes()->definiteSlot(), NumFixedSlots(staticObject),
                     value, needsBarrier, slotType);
}

bool
IonBuilder::jsop_getgname(PropertyName* name)
{
    JSObject* obj = &script()->global();
    bool succeeded;
    if (!getStaticName(obj, name, &succeeded))
        return false;
    if (succeeded)
        return true;

    TemporaryTypeSet* types = bytecodeTypes(pc);
    MDefinition* globalObj = constant(ObjectValue(*obj));
    if (!getPropTryCommonGetter(&succeeded, globalObj, name, types))
        return false;
    if (succeeded)
        return true;

    return jsop_getname(name);
}

bool
IonBuilder::jsop_getname(PropertyName* name)
{
    MDefinition* object;
    if (IsGlobalOp(JSOp(*pc)) && !script()->hasPollutedGlobalScope()) {
        MInstruction* global = constant(ObjectValue(script()->global()));
        object = global;
    } else {
        current->push(current->scopeChain());
        object = current->pop();
    }

    MGetNameCache* ins;
    if (JSOp(*GetNextPc(pc)) == JSOP_TYPEOF)
        ins = MGetNameCache::New(alloc(), object, name, MGetNameCache::NAMETYPEOF);
    else
        ins = MGetNameCache::New(alloc(), object, name, MGetNameCache::NAME);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    TemporaryTypeSet* types = bytecodeTypes(pc);
    return pushTypeBarrier(ins, types, BarrierKind::TypeSet);
}

bool
IonBuilder::jsop_intrinsic(PropertyName* name)
{
    TemporaryTypeSet* types = bytecodeTypes(pc);

    
    
    if (types->empty()) {
        MCallGetIntrinsicValue* ins = MCallGetIntrinsicValue::New(alloc(), name);

        current->add(ins);
        current->push(ins);

        if (!resumeAfter(ins))
            return false;

        return pushTypeBarrier(ins, types, BarrierKind::TypeSet);
    }

    
    Value vp;
    JS_ALWAYS_TRUE(script()->global().maybeGetIntrinsicValue(name, &vp));
    MOZ_ASSERT(types->hasType(TypeSet::GetValueType(vp)));

    pushConstant(vp);
    return true;
}

bool
IonBuilder::jsop_bindname(PropertyName* name)
{
    MOZ_ASSERT(analysis().usesScopeChain());

    MDefinition* scopeChain = current->scopeChain();
    MBindNameCache* ins = MBindNameCache::New(alloc(), scopeChain, name, script(), pc);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

static MIRType
GetElemKnownType(bool needsHoleCheck, TemporaryTypeSet* types)
{
    MIRType knownType = types->getKnownMIRType();

    
    
    
    
    
    if (knownType == MIRType_Undefined || knownType == MIRType_Null)
        knownType = MIRType_Value;

    
    
    if (needsHoleCheck && !LIRGenerator::allowTypedElementHoleCheck())
        knownType = MIRType_Value;

    return knownType;
}

bool
IonBuilder::jsop_getelem()
{
    startTrackingOptimizations();

    MDefinition* index = current->pop();
    MDefinition* obj = current->pop();

    trackTypeInfo(TrackedTypeSite::Receiver, obj->type(), obj->resultTypeSet());
    trackTypeInfo(TrackedTypeSite::Index, index->type(), index->resultTypeSet());

    
    
    if (info().isAnalysis()) {
        MInstruction* ins = MCallGetElement::New(alloc(), obj, index);

        current->add(ins);
        current->push(ins);

        if (!resumeAfter(ins))
            return false;

        TemporaryTypeSet* types = bytecodeTypes(pc);
        return pushTypeBarrier(ins, types, BarrierKind::TypeSet);
    }

    bool emitted = false;

    trackOptimizationAttempt(TrackedStrategy::GetElem_TypedObject);
    if (!getElemTryTypedObject(&emitted, obj, index) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::GetElem_Dense);
    if (!getElemTryDense(&emitted, obj, index) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::GetElem_TypedStatic);
    if (!getElemTryTypedStatic(&emitted, obj, index) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::GetElem_TypedArray);
    if (!getElemTryTypedArray(&emitted, obj, index) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::GetElem_String);
    if (!getElemTryString(&emitted, obj, index) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::GetElem_Arguments);
    if (!getElemTryArguments(&emitted, obj, index) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::GetElem_ArgumentsInlined);
    if (!getElemTryArgumentsInlined(&emitted, obj, index) || emitted)
        return emitted;

    if (script()->argumentsHasVarBinding() && obj->mightBeType(MIRType_MagicOptimizedArguments))
        return abort("Type is not definitely lazy arguments.");

    trackOptimizationAttempt(TrackedStrategy::GetElem_InlineCache);
    if (!getElemTryCache(&emitted, obj, index) || emitted)
        return emitted;

    
    MInstruction* ins = MCallGetElement::New(alloc(), obj, index);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    if (*pc == JSOP_CALLELEM && IsNullOrUndefined(obj->type())) {
        
        
        
        
        return true;
    }

    TemporaryTypeSet* types = bytecodeTypes(pc);
    return pushTypeBarrier(ins, types, BarrierKind::TypeSet);
}

bool
IonBuilder::getElemTryTypedObject(bool* emitted, MDefinition* obj, MDefinition* index)
{
    MOZ_ASSERT(*emitted == false);

    
    
    trackOptimizationOutcome(TrackedOutcome::AccessNotTypedObject);

    TypedObjectPrediction objPrediction = typedObjectPrediction(obj);
    if (objPrediction.isUseless())
        return true;

    if (!objPrediction.ofArrayKind())
        return true;

    TypedObjectPrediction elemPrediction = objPrediction.arrayElementType();
    if (elemPrediction.isUseless())
        return true;

    int32_t elemSize;
    if (!elemPrediction.hasKnownSize(&elemSize))
        return true;

    switch (elemPrediction.kind()) {
      case type::Simd:
        
        trackOptimizationOutcome(TrackedOutcome::GenericFailure);
        return true;

      case type::Struct:
      case type::Array:
        return getElemTryComplexElemOfTypedObject(emitted,
                                                  obj,
                                                  index,
                                                  objPrediction,
                                                  elemPrediction,
                                                  elemSize);
      case type::Scalar:
        return getElemTryScalarElemOfTypedObject(emitted,
                                                 obj,
                                                 index,
                                                 objPrediction,
                                                 elemPrediction,
                                                 elemSize);

      case type::Reference:
        return getElemTryReferenceElemOfTypedObject(emitted,
                                                    obj,
                                                    index,
                                                    objPrediction,
                                                    elemPrediction);
    }

    MOZ_CRASH("Bad kind");
}

static MIRType
MIRTypeForTypedArrayRead(Scalar::Type arrayType, bool observedDouble);

bool
IonBuilder::checkTypedObjectIndexInBounds(int32_t elemSize,
                                          MDefinition* obj,
                                          MDefinition* index,
                                          TypedObjectPrediction objPrediction,
                                          LinearSum* indexAsByteOffset)
{
    
    MInstruction* idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);

    
    
    
    
    int32_t lenOfAll;
    MDefinition* length;
    if (objPrediction.hasKnownArrayLength(&lenOfAll)) {
        length = constantInt(lenOfAll);

        
        
        TypeSet::ObjectKey* globalKey = TypeSet::ObjectKey::get(&script()->global());
        if (globalKey->hasFlags(constraints(), OBJECT_FLAG_TYPED_OBJECT_NEUTERED)) {
            trackOptimizationOutcome(TrackedOutcome::TypedObjectNeutered);
            return false;
        }
    } else {
        trackOptimizationOutcome(TrackedOutcome::TypedObjectArrayRange);
        return false;
    }

    index = addBoundsCheck(idInt32, length);

    return indexAsByteOffset->add(index, elemSize);
}

bool
IonBuilder::getElemTryScalarElemOfTypedObject(bool* emitted,
                                              MDefinition* obj,
                                              MDefinition* index,
                                              TypedObjectPrediction objPrediction,
                                              TypedObjectPrediction elemPrediction,
                                              int32_t elemSize)
{
    MOZ_ASSERT(objPrediction.ofArrayKind());

    
    ScalarTypeDescr::Type elemType = elemPrediction.scalarType();
    MOZ_ASSERT(elemSize == ScalarTypeDescr::alignment(elemType));

    LinearSum indexAsByteOffset(alloc());
    if (!checkTypedObjectIndexInBounds(elemSize, obj, index, objPrediction, &indexAsByteOffset))
        return true;

    trackOptimizationSuccess();
    *emitted = true;

    return pushScalarLoadFromTypedObject(obj, indexAsByteOffset, elemType);
}

bool
IonBuilder::getElemTryReferenceElemOfTypedObject(bool* emitted,
                                                 MDefinition* obj,
                                                 MDefinition* index,
                                                 TypedObjectPrediction objPrediction,
                                                 TypedObjectPrediction elemPrediction)
{
    MOZ_ASSERT(objPrediction.ofArrayKind());

    ReferenceTypeDescr::Type elemType = elemPrediction.referenceType();
    size_t elemSize = ReferenceTypeDescr::size(elemType);

    LinearSum indexAsByteOffset(alloc());
    if (!checkTypedObjectIndexInBounds(elemSize, obj, index, objPrediction, &indexAsByteOffset))
        return true;

    trackOptimizationSuccess();
    *emitted = true;

    return pushReferenceLoadFromTypedObject(obj, indexAsByteOffset, elemType, nullptr);
}

bool
IonBuilder::pushScalarLoadFromTypedObject(MDefinition* obj,
                                          const LinearSum& byteOffset,
                                          ScalarTypeDescr::Type elemType)
{
    int32_t size = ScalarTypeDescr::size(elemType);
    MOZ_ASSERT(size == ScalarTypeDescr::alignment(elemType));

    
    MDefinition* elements;
    MDefinition* scaledOffset;
    int32_t adjustment;
    loadTypedObjectElements(obj, byteOffset, size, &elements, &scaledOffset, &adjustment);

    
    MLoadUnboxedScalar* load = MLoadUnboxedScalar::New(alloc(), elements, scaledOffset,
                                                       elemType,
                                                       DoesNotRequireMemoryBarrier,
                                                       adjustment);
    current->add(load);
    current->push(load);

    
    
    
    
    TemporaryTypeSet* resultTypes = bytecodeTypes(pc);
    bool allowDouble = resultTypes->hasType(TypeSet::DoubleType());

    
    
    MIRType knownType = MIRTypeForTypedArrayRead(elemType, allowDouble);

    
    
    
    
    load->setResultType(knownType);

    return true;
}

bool
IonBuilder::pushReferenceLoadFromTypedObject(MDefinition* typedObj,
                                             const LinearSum& byteOffset,
                                             ReferenceTypeDescr::Type type,
                                             PropertyName* name)
{
    
    MDefinition* elements;
    MDefinition* scaledOffset;
    int32_t adjustment;
    size_t alignment = ReferenceTypeDescr::alignment(type);
    loadTypedObjectElements(typedObj, byteOffset, alignment, &elements, &scaledOffset, &adjustment);

    TemporaryTypeSet* observedTypes = bytecodeTypes(pc);

    MInstruction* load = nullptr;  
    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(),
                                                       typedObj, name, observedTypes);

    switch (type) {
      case ReferenceTypeDescr::TYPE_ANY: {
        
        bool bailOnUndefined = barrier == BarrierKind::NoBarrier &&
                               !observedTypes->hasType(TypeSet::UndefinedType());
        if (bailOnUndefined)
            barrier = BarrierKind::TypeTagOnly;
        load = MLoadElement::New(alloc(), elements, scaledOffset, false, false, adjustment);
        break;
      }
      case ReferenceTypeDescr::TYPE_OBJECT: {
        
        
        
        
        MLoadUnboxedObjectOrNull::NullBehavior nullBehavior;
        if (barrier == BarrierKind::NoBarrier && !observedTypes->hasType(TypeSet::NullType()))
            nullBehavior = MLoadUnboxedObjectOrNull::BailOnNull;
        else
            nullBehavior = MLoadUnboxedObjectOrNull::HandleNull;
        load = MLoadUnboxedObjectOrNull::New(alloc(), elements, scaledOffset, nullBehavior,
                                             adjustment);
        break;
      }
      case ReferenceTypeDescr::TYPE_STRING: {
        load = MLoadUnboxedString::New(alloc(), elements, scaledOffset, adjustment);
        observedTypes->addType(TypeSet::StringType(), alloc().lifoAlloc());
        break;
      }
    }

    current->add(load);
    current->push(load);

    return pushTypeBarrier(load, observedTypes, barrier);
}

bool
IonBuilder::getElemTryComplexElemOfTypedObject(bool* emitted,
                                               MDefinition* obj,
                                               MDefinition* index,
                                               TypedObjectPrediction objPrediction,
                                               TypedObjectPrediction elemPrediction,
                                               int32_t elemSize)
{
    MOZ_ASSERT(objPrediction.ofArrayKind());

    MDefinition* type = loadTypedObjectType(obj);
    MDefinition* elemTypeObj = typeObjectForElementFromArrayStructType(type);

    LinearSum indexAsByteOffset(alloc());
    if (!checkTypedObjectIndexInBounds(elemSize, obj, index, objPrediction, &indexAsByteOffset))
        return true;

    return pushDerivedTypedObject(emitted, obj, indexAsByteOffset,
                                  elemPrediction, elemTypeObj);
}

bool
IonBuilder::pushDerivedTypedObject(bool* emitted,
                                   MDefinition* obj,
                                   const LinearSum& baseByteOffset,
                                   TypedObjectPrediction derivedPrediction,
                                   MDefinition* derivedTypeObj)
{
    
    MDefinition* owner;
    LinearSum ownerByteOffset(alloc());
    loadTypedObjectData(obj, &owner, &ownerByteOffset);

    if (!ownerByteOffset.add(baseByteOffset, 1))
        setForceAbort();

    MDefinition* offset = ConvertLinearSum(alloc(), current, ownerByteOffset,
                                            true);

    
    MInstruction* derivedTypedObj = MNewDerivedTypedObject::New(alloc(),
                                                                derivedPrediction,
                                                                derivedTypeObj,
                                                                owner,
                                                                offset);
    current->add(derivedTypedObj);
    current->push(derivedTypedObj);

    
    
    
    
    
    TemporaryTypeSet* objTypes = obj->resultTypeSet();
    const Class* expectedClass = nullptr;
    if (const Class* objClass = objTypes ? objTypes->getKnownClass(constraints()) : nullptr) {
        MOZ_ASSERT(IsTypedObjectClass(objClass));
        expectedClass = GetOutlineTypedObjectClass(IsOpaqueTypedObjectClass(objClass));
    }
    const TypedProto* expectedProto = derivedPrediction.getKnownPrototype();
    MOZ_ASSERT_IF(expectedClass, IsTypedObjectClass(expectedClass));

    
    
    TemporaryTypeSet* observedTypes = bytecodeTypes(pc);
    const Class* observedClass = observedTypes->getKnownClass(constraints());

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    JSObject* observedProto;
    if (observedTypes->getCommonPrototype(constraints(), &observedProto) &&
        observedClass && observedProto && observedClass == expectedClass &&
        observedProto == expectedProto)
    {
        derivedTypedObj->setResultTypeSet(observedTypes);
    } else {
        if (!pushTypeBarrier(derivedTypedObj, observedTypes, BarrierKind::TypeSet))
            return false;
    }

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryDense(bool* emitted, MDefinition* obj, MDefinition* index)
{
    MOZ_ASSERT(*emitted == false);

    if (!ElementAccessIsDenseNative(constraints(), obj, index)) {
        trackOptimizationOutcome(TrackedOutcome::AccessNotDense);
        return true;
    }

    
    
    if (ElementAccessHasExtraIndexedProperty(constraints(), obj) && failedBoundsCheck_) {
        trackOptimizationOutcome(TrackedOutcome::ProtoIndexedProps);
        return true;
    }

    
    
    if (inspector->hasSeenNegativeIndexGetElement(pc)) {
        trackOptimizationOutcome(TrackedOutcome::ArraySeenNegativeIndex);
        return true;
    }

    
    if (!jsop_getelem_dense(obj, index))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

JSObject*
IonBuilder::getStaticTypedArrayObject(MDefinition* obj, MDefinition* index)
{
    Scalar::Type arrayType;
    if (!ElementAccessIsAnyTypedArray(constraints(), obj, index, &arrayType)) {
        trackOptimizationOutcome(TrackedOutcome::AccessNotTypedArray);
        return nullptr;
    }

    if (!LIRGenerator::allowStaticTypedArrayAccesses()) {
        trackOptimizationOutcome(TrackedOutcome::Disabled);
        return nullptr;
    }

    if (ElementAccessHasExtraIndexedProperty(constraints(), obj)) {
        trackOptimizationOutcome(TrackedOutcome::ProtoIndexedProps);
        return nullptr;
    }

    if (!obj->resultTypeSet()) {
        trackOptimizationOutcome(TrackedOutcome::NoTypeInfo);
        return nullptr;
    }

    JSObject* tarrObj = obj->resultTypeSet()->maybeSingleton();
    if (!tarrObj) {
        trackOptimizationOutcome(TrackedOutcome::NotSingleton);
        return nullptr;
    }

    TypeSet::ObjectKey* tarrKey = TypeSet::ObjectKey::get(tarrObj);
    if (tarrKey->unknownProperties()) {
        trackOptimizationOutcome(TrackedOutcome::UnknownProperties);
        return nullptr;
    }

    return tarrObj;
}

bool
IonBuilder::getElemTryTypedStatic(bool* emitted, MDefinition* obj, MDefinition* index)
{
    MOZ_ASSERT(*emitted == false);

    JSObject* tarrObj = getStaticTypedArrayObject(obj, index);
    if (!tarrObj)
        return true;

    
    Scalar::Type viewType = AnyTypedArrayType(tarrObj);
    if (viewType == Scalar::Uint32) {
        trackOptimizationOutcome(TrackedOutcome::StaticTypedArrayUint32);
        return true;
    }

    MDefinition* ptr = convertShiftToMaskForStaticTypedArray(index, viewType);
    if (!ptr)
        return true;

    

    if (tarrObj->is<TypedArrayObject>()) {
        TypeSet::ObjectKey* tarrKey = TypeSet::ObjectKey::get(tarrObj);
        tarrKey->watchStateChangeForTypedArrayData(constraints());
    }

    obj->setImplicitlyUsedUnchecked();
    index->setImplicitlyUsedUnchecked();

    MLoadTypedArrayElementStatic* load = MLoadTypedArrayElementStatic::New(alloc(), tarrObj, ptr);
    current->add(load);
    current->push(load);

    
    
    
    
    
    if (viewType == Scalar::Float32 || viewType == Scalar::Float64) {
        jsbytecode* next = pc + JSOP_GETELEM_LENGTH;
        if (*next == JSOP_POS)
            load->setInfallible();
    } else {
        jsbytecode* next = pc + JSOP_GETELEM_LENGTH;
        if (*next == JSOP_ZERO && *(next + JSOP_ZERO_LENGTH) == JSOP_BITOR)
            load->setInfallible();
    }

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryTypedArray(bool* emitted, MDefinition* obj, MDefinition* index)
{
    MOZ_ASSERT(*emitted == false);

    Scalar::Type arrayType;
    if (!ElementAccessIsAnyTypedArray(constraints(), obj, index, &arrayType)) {
        trackOptimizationOutcome(TrackedOutcome::AccessNotTypedArray);
        return true;
    }

    
    if (!jsop_getelem_typed(obj, index, arrayType))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryString(bool* emitted, MDefinition* obj, MDefinition* index)
{
    MOZ_ASSERT(*emitted == false);

    if (obj->type() != MIRType_String || !IsNumberType(index->type())) {
        trackOptimizationOutcome(TrackedOutcome::AccessNotString);
        return true;
    }

    
    
    if (bytecodeTypes(pc)->hasType(TypeSet::UndefinedType())) {
        trackOptimizationOutcome(TrackedOutcome::OutOfBounds);
        return true;
    }

    
    MInstruction* idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    MStringLength* length = MStringLength::New(alloc(), obj);
    current->add(length);

    index = addBoundsCheck(index, length);

    MCharCodeAt* charCode = MCharCodeAt::New(alloc(), obj, index);
    current->add(charCode);

    MFromCharCode* result = MFromCharCode::New(alloc(), charCode);
    current->add(result);
    current->push(result);

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryArguments(bool* emitted, MDefinition* obj, MDefinition* index)
{
    MOZ_ASSERT(*emitted == false);

    if (inliningDepth_ > 0)
        return true;

    if (obj->type() != MIRType_MagicOptimizedArguments)
        return true;

    

    MOZ_ASSERT(!info().argsObjAliasesFormals());

    
    obj->setImplicitlyUsedUnchecked();

    
    MArgumentsLength* length = MArgumentsLength::New(alloc());
    current->add(length);

    
    MInstruction* idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    
    index = addBoundsCheck(index, length);

    
    MGetFrameArgument* load = MGetFrameArgument::New(alloc(), index, analysis_.hasSetArg());
    current->add(load);
    current->push(load);

    TemporaryTypeSet* types = bytecodeTypes(pc);
    if (!pushTypeBarrier(load, types, BarrierKind::TypeSet))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryArgumentsInlined(bool* emitted, MDefinition* obj, MDefinition* index)
{
    MOZ_ASSERT(*emitted == false);

    if (inliningDepth_ == 0)
        return true;

    if (obj->type() != MIRType_MagicOptimizedArguments)
        return true;

    
    obj->setImplicitlyUsedUnchecked();

    MOZ_ASSERT(!info().argsObjAliasesFormals());

    
    if (index->isConstantValue() && index->constantValue().isInt32()) {
        MOZ_ASSERT(inliningDepth_ > 0);

        int32_t id = index->constantValue().toInt32();
        index->setImplicitlyUsedUnchecked();

        if (id < (int32_t)inlineCallInfo_->argc() && id >= 0)
            current->push(inlineCallInfo_->getArg(id));
        else
            pushConstant(UndefinedValue());

        trackOptimizationSuccess();
        *emitted = true;
        return true;
    }

    
    return abort("NYI inlined not constant get argument element");
}

bool
IonBuilder::getElemTryCache(bool* emitted, MDefinition* obj, MDefinition* index)
{
    MOZ_ASSERT(*emitted == false);

    
    if (!obj->mightBeType(MIRType_Object)) {
        trackOptimizationOutcome(TrackedOutcome::NotObject);
        return true;
    }

    
    if (obj->mightBeType(MIRType_String)) {
        trackOptimizationOutcome(TrackedOutcome::GetElemStringNotCached);
        return true;
    }

    
    if (!index->mightBeType(MIRType_Int32) &&
        !index->mightBeType(MIRType_String) &&
        !index->mightBeType(MIRType_Symbol))
    {
        trackOptimizationOutcome(TrackedOutcome::IndexType);
        return true;
    }

    
    
    bool nonNativeGetElement = inspector->hasSeenNonNativeGetElement(pc);
    if (index->mightBeType(MIRType_Int32) && nonNativeGetElement) {
        trackOptimizationOutcome(TrackedOutcome::NonNativeReceiver);
        return true;
    }

    

    TemporaryTypeSet* types = bytecodeTypes(pc);
    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(), obj,
                                                       nullptr, types);

    
    
    if (index->mightBeType(MIRType_String) || index->mightBeType(MIRType_Symbol))
        barrier = BarrierKind::TypeSet;

    MInstruction* ins = MGetElementCache::New(alloc(), obj, index, barrier == BarrierKind::TypeSet);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    
    if (index->type() == MIRType_Int32 && barrier == BarrierKind::NoBarrier) {
        bool needHoleCheck = !ElementAccessIsPacked(constraints(), obj);
        MIRType knownType = GetElemKnownType(needHoleCheck, types);

        if (knownType != MIRType_Value && knownType != MIRType_Double)
            ins->setResultType(knownType);
    }

    if (!pushTypeBarrier(ins, types, barrier))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::jsop_getelem_dense(MDefinition* obj, MDefinition* index)
{
    TemporaryTypeSet* types = bytecodeTypes(pc);

    MOZ_ASSERT(index->type() == MIRType_Int32 || index->type() == MIRType_Double);
    if (JSOp(*pc) == JSOP_CALLELEM) {
        
        
        
        AddObjectsForPropertyRead(obj, nullptr, types);
    }

    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(), obj,
                                                       nullptr, types);
    bool needsHoleCheck = !ElementAccessIsPacked(constraints(), obj);

    
    
    
    bool readOutOfBounds =
        types->hasType(TypeSet::UndefinedType()) &&
        !ElementAccessHasExtraIndexedProperty(constraints(), obj);

    MIRType knownType = MIRType_Value;
    if (barrier == BarrierKind::NoBarrier)
        knownType = GetElemKnownType(needsHoleCheck, types);

    
    MInstruction* idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    
    MInstruction* elements = MElements::New(alloc(), obj);
    current->add(elements);

    
    
    
    MInitializedLength* initLength = MInitializedLength::New(alloc(), elements);
    current->add(initLength);

    
    
    TemporaryTypeSet* objTypes = obj->resultTypeSet();
    bool loadDouble =
        barrier == BarrierKind::NoBarrier &&
        loopDepth_ &&
        !readOutOfBounds &&
        !needsHoleCheck &&
        knownType == MIRType_Double &&
        objTypes &&
        objTypes->convertDoubleElements(constraints()) == TemporaryTypeSet::AlwaysConvertToDoubles;
    if (loadDouble)
        elements = addConvertElementsToDoubles(elements);

    MInstruction* load;

    if (!readOutOfBounds) {
        
        
        
        
        index = addBoundsCheck(index, initLength);

        load = MLoadElement::New(alloc(), elements, index, needsHoleCheck, loadDouble);
        current->add(load);
    } else {
        
        
        
        load = MLoadElementHole::New(alloc(), elements, index, initLength, needsHoleCheck);
        current->add(load);

        
        
        
        MOZ_ASSERT(knownType == MIRType_Value);
    }

    if (knownType != MIRType_Value)
        load->setResultType(knownType);

    current->push(load);
    return pushTypeBarrier(load, types, barrier);
}

void
IonBuilder::addTypedArrayLengthAndData(MDefinition* obj,
                                       BoundsChecking checking,
                                       MDefinition** index,
                                       MInstruction** length, MInstruction** elements)
{
    MOZ_ASSERT((index != nullptr) == (elements != nullptr));
    JSObject* tarr = nullptr;

    if (obj->isConstantValue() && obj->constantValue().isObject())
        tarr = &obj->constantValue().toObject();
    else if (obj->resultTypeSet())
        tarr = obj->resultTypeSet()->maybeSingleton();

    if (tarr) {
        void* data = AnyTypedArrayViewData(tarr);
        
        
        bool isTenured = !tarr->runtimeFromMainThread()->gc.nursery.isInside(data);
        if (isTenured && tarr->isSingleton()) {
            
            
            TypeSet::ObjectKey* tarrKey = TypeSet::ObjectKey::get(tarr);
            if (!tarrKey->unknownProperties()) {
                if (tarr->is<TypedArrayObject>())
                    tarrKey->watchStateChangeForTypedArrayData(constraints());

                obj->setImplicitlyUsedUnchecked();

                int32_t len = AssertedCast<int32_t>(AnyTypedArrayLength(tarr));
                *length = MConstant::New(alloc(), Int32Value(len));
                current->add(*length);

                if (index) {
                    if (checking == DoBoundsCheck)
                        *index = addBoundsCheck(*index, *length);

                    *elements = MConstantElements::New(alloc(), data);
                    current->add(*elements);
                }
                return;
            }
        }
    }

    *length = MTypedArrayLength::New(alloc(), obj);
    current->add(*length);

    if (index) {
        if (checking == DoBoundsCheck)
            *index = addBoundsCheck(*index, *length);

        *elements = MTypedArrayElements::New(alloc(), obj);
        current->add(*elements);
    }
}

MDefinition*
IonBuilder::convertShiftToMaskForStaticTypedArray(MDefinition* id,
                                                  Scalar::Type viewType)
{
    trackOptimizationOutcome(TrackedOutcome::StaticTypedArrayCantComputeMask);

    
    if (TypedArrayShift(viewType) == 0)
        return id;

    
    
    if (id->isConstantValue() && id->constantValue().isInt32()) {
        int32_t index = id->constantValue().toInt32();
        MConstant* offset = MConstant::New(alloc(), Int32Value(index << TypedArrayShift(viewType)));
        current->add(offset);
        return offset;
    }

    if (!id->isRsh() || id->isEffectful())
        return nullptr;
    if (!id->getOperand(1)->isConstantValue())
        return nullptr;
    const Value& value = id->getOperand(1)->constantValue();
    if (!value.isInt32() || uint32_t(value.toInt32()) != TypedArrayShift(viewType))
        return nullptr;

    
    
    MConstant* mask = MConstant::New(alloc(), Int32Value(~((1 << value.toInt32()) - 1)));
    MBitAnd* ptr = MBitAnd::New(alloc(), id->getOperand(0), mask);

    ptr->infer(nullptr, nullptr);
    MOZ_ASSERT(!ptr->isEffectful());

    current->add(mask);
    current->add(ptr);

    return ptr;
}

static MIRType
MIRTypeForTypedArrayRead(Scalar::Type arrayType, bool observedDouble)
{
    switch (arrayType) {
      case Scalar::Int8:
      case Scalar::Uint8:
      case Scalar::Uint8Clamped:
      case Scalar::Int16:
      case Scalar::Uint16:
      case Scalar::Int32:
        return MIRType_Int32;
      case Scalar::Uint32:
        return observedDouble ? MIRType_Double : MIRType_Int32;
      case Scalar::Float32:
        return MIRType_Float32;
      case Scalar::Float64:
        return MIRType_Double;
      default:
        break;
    }
    MOZ_CRASH("Unknown typed array type");
}

bool
IonBuilder::jsop_getelem_typed(MDefinition* obj, MDefinition* index,
                               Scalar::Type arrayType)
{
    TemporaryTypeSet* types = bytecodeTypes(pc);

    bool maybeUndefined = types->hasType(TypeSet::UndefinedType());

    
    
    
    bool allowDouble = types->hasType(TypeSet::DoubleType());

    
    MInstruction* idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    if (!maybeUndefined) {
        
        

        
        
        
        
        MIRType knownType = MIRTypeForTypedArrayRead(arrayType, allowDouble);

        
        MInstruction* length;
        MInstruction* elements;
        addTypedArrayLengthAndData(obj, DoBoundsCheck, &index, &length, &elements);

        
        MLoadUnboxedScalar* load = MLoadUnboxedScalar::New(alloc(), elements, index, arrayType);
        current->add(load);
        current->push(load);

        
        
        load->setResultType(knownType);
        return true;
    } else {
        
        
        
        
        BarrierKind barrier = BarrierKind::TypeSet;
        switch (arrayType) {
          case Scalar::Int8:
          case Scalar::Uint8:
          case Scalar::Uint8Clamped:
          case Scalar::Int16:
          case Scalar::Uint16:
          case Scalar::Int32:
          case Scalar::Uint32:
            if (types->hasType(TypeSet::Int32Type()))
                barrier = BarrierKind::NoBarrier;
            break;
          case Scalar::Float32:
          case Scalar::Float64:
            if (allowDouble)
                barrier = BarrierKind::NoBarrier;
            break;
          default:
            MOZ_CRASH("Unknown typed array type");
        }

        
        
        
        MLoadTypedArrayElementHole* load =
            MLoadTypedArrayElementHole::New(alloc(), obj, index, arrayType, allowDouble);
        current->add(load);
        current->push(load);

        return pushTypeBarrier(load, types, barrier);
    }
}

bool
IonBuilder::jsop_setelem()
{
    bool emitted = false;
    startTrackingOptimizations();

    MDefinition* value = current->pop();
    MDefinition* index = current->pop();
    MDefinition* object = current->pop();

    trackTypeInfo(TrackedTypeSite::Receiver, object->type(), object->resultTypeSet());
    trackTypeInfo(TrackedTypeSite::Index, index->type(), index->resultTypeSet());
    trackTypeInfo(TrackedTypeSite::Value, value->type(), value->resultTypeSet());

    trackOptimizationAttempt(TrackedStrategy::SetElem_TypedObject);
    if (!setElemTryTypedObject(&emitted, object, index, value) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::SetElem_TypedStatic);
    if (!setElemTryTypedStatic(&emitted, object, index, value) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::SetElem_TypedArray);
    if (!setElemTryTypedArray(&emitted, object, index, value) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::SetElem_Dense);
    if (!setElemTryDense(&emitted, object, index, value) || emitted)
        return emitted;

    trackOptimizationAttempt(TrackedStrategy::SetElem_Arguments);
    if (!setElemTryArguments(&emitted, object, index, value) || emitted)
        return emitted;

    if (script()->argumentsHasVarBinding() &&
        object->mightBeType(MIRType_MagicOptimizedArguments) &&
        info().analysisMode() != Analysis_ArgumentsUsage)
    {
        return abort("Type is not definitely lazy arguments.");
    }

    trackOptimizationAttempt(TrackedStrategy::SetElem_InlineCache);
    if (!setElemTryCache(&emitted, object, index, value) || emitted)
        return emitted;

    
    MInstruction* ins = MCallSetElement::New(alloc(), object, index, value, IsStrictSetPC(pc));
    current->add(ins);
    current->push(value);

    return resumeAfter(ins);
}

bool
IonBuilder::setElemTryTypedObject(bool* emitted, MDefinition* obj,
                                  MDefinition* index, MDefinition* value)
{
    MOZ_ASSERT(*emitted == false);

    
    
    trackOptimizationOutcome(TrackedOutcome::AccessNotTypedObject);

    TypedObjectPrediction objPrediction = typedObjectPrediction(obj);
    if (objPrediction.isUseless())
        return true;

    if (!objPrediction.ofArrayKind())
        return true;

    TypedObjectPrediction elemPrediction = objPrediction.arrayElementType();
    if (elemPrediction.isUseless())
        return true;

    int32_t elemSize;
    if (!elemPrediction.hasKnownSize(&elemSize))
        return true;

    switch (elemPrediction.kind()) {
      case type::Simd:
        
        trackOptimizationOutcome(TrackedOutcome::GenericFailure);
        return true;

      case type::Reference:
        return setElemTryReferenceElemOfTypedObject(emitted, obj, index,
                                                    objPrediction, value, elemPrediction);

      case type::Scalar:
        return setElemTryScalarElemOfTypedObject(emitted,
                                                 obj,
                                                 index,
                                                 objPrediction,
                                                 value,
                                                 elemPrediction,
                                                 elemSize);

      case type::Struct:
      case type::Array:
        
        trackOptimizationOutcome(TrackedOutcome::GenericFailure);
        return true;
    }

    MOZ_CRASH("Bad kind");
}

bool
IonBuilder::setElemTryReferenceElemOfTypedObject(bool* emitted,
                                                 MDefinition* obj,
                                                 MDefinition* index,
                                                 TypedObjectPrediction objPrediction,
                                                 MDefinition* value,
                                                 TypedObjectPrediction elemPrediction)
{
    ReferenceTypeDescr::Type elemType = elemPrediction.referenceType();
    size_t elemSize = ReferenceTypeDescr::size(elemType);

    LinearSum indexAsByteOffset(alloc());
    if (!checkTypedObjectIndexInBounds(elemSize, obj, index, objPrediction, &indexAsByteOffset))
        return true;

    if (!storeReferenceTypedObjectValue(obj, indexAsByteOffset, elemType, value, nullptr))
        return true;

    current->push(value);

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::setElemTryScalarElemOfTypedObject(bool* emitted,
                                              MDefinition* obj,
                                              MDefinition* index,
                                              TypedObjectPrediction objPrediction,
                                              MDefinition* value,
                                              TypedObjectPrediction elemPrediction,
                                              int32_t elemSize)
{
    
    ScalarTypeDescr::Type elemType = elemPrediction.scalarType();
    MOZ_ASSERT(elemSize == ScalarTypeDescr::alignment(elemType));

    LinearSum indexAsByteOffset(alloc());
    if (!checkTypedObjectIndexInBounds(elemSize, obj, index, objPrediction, &indexAsByteOffset))
        return true;

    
    if (!storeScalarTypedObjectValue(obj, indexAsByteOffset, elemType, value))
        return false;

    current->push(value);

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::setElemTryTypedStatic(bool* emitted, MDefinition* object,
                                  MDefinition* index, MDefinition* value)
{
    MOZ_ASSERT(*emitted == false);

    JSObject* tarrObj = getStaticTypedArrayObject(object, index);
    if (!tarrObj)
        return true;

    if (tarrObj->runtimeFromMainThread()->gc.nursery.isInside(AnyTypedArrayViewData(tarrObj)))
        return true;

    Scalar::Type viewType = AnyTypedArrayType(tarrObj);
    MDefinition* ptr = convertShiftToMaskForStaticTypedArray(index, viewType);
    if (!ptr)
        return true;

    

    if (tarrObj->is<TypedArrayObject>()) {
        TypeSet::ObjectKey* tarrKey = TypeSet::ObjectKey::get(tarrObj);
        tarrKey->watchStateChangeForTypedArrayData(constraints());
    }

    object->setImplicitlyUsedUnchecked();
    index->setImplicitlyUsedUnchecked();

    
    MDefinition* toWrite = value;
    if (viewType == Scalar::Uint8Clamped) {
        toWrite = MClampToUint8::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }

    MInstruction* store = MStoreTypedArrayElementStatic::New(alloc(), tarrObj, ptr, toWrite);
    current->add(store);
    current->push(value);

    if (!resumeAfter(store))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::setElemTryTypedArray(bool* emitted, MDefinition* object,
                                 MDefinition* index, MDefinition* value)
{
    MOZ_ASSERT(*emitted == false);

    Scalar::Type arrayType;
    if (!ElementAccessIsAnyTypedArray(constraints(), object, index, &arrayType)) {
        trackOptimizationOutcome(TrackedOutcome::AccessNotTypedArray);
        return true;
    }

    
    if (!jsop_setelem_typed(arrayType, SetElem_Normal, object, index, value))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::setElemTryDense(bool* emitted, MDefinition* object,
                            MDefinition* index, MDefinition* value)
{
    MOZ_ASSERT(*emitted == false);

    if (!ElementAccessIsDenseNative(constraints(), object, index)) {
        trackOptimizationOutcome(TrackedOutcome::AccessNotDense);
        return true;
    }

    if (PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current,
                                      &object, nullptr, &value,  true))
    {
        trackOptimizationOutcome(TrackedOutcome::NeedsTypeBarrier);
        return true;
    }

    if (!object->resultTypeSet()) {
        trackOptimizationOutcome(TrackedOutcome::NoTypeInfo);
        return true;
    }

    TemporaryTypeSet::DoubleConversion conversion =
        object->resultTypeSet()->convertDoubleElements(constraints());

    
    if (conversion == TemporaryTypeSet::AmbiguousDoubleConversion &&
        value->type() != MIRType_Int32)
    {
        trackOptimizationOutcome(TrackedOutcome::ArrayDoubleConversion);
        return true;
    }

    
    
    if (ElementAccessHasExtraIndexedProperty(constraints(), object) && failedBoundsCheck_) {
        trackOptimizationOutcome(TrackedOutcome::ProtoIndexedProps);
        return true;
    }

    
    if (!jsop_setelem_dense(conversion, SetElem_Normal, object, index, value))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::setElemTryArguments(bool* emitted, MDefinition* object,
                                MDefinition* index, MDefinition* value)
{
    MOZ_ASSERT(*emitted == false);

    if (object->type() != MIRType_MagicOptimizedArguments)
        return true;

    
    return abort("NYI arguments[]=");
}

bool
IonBuilder::setElemTryCache(bool* emitted, MDefinition* object,
                            MDefinition* index, MDefinition* value)
{
    MOZ_ASSERT(*emitted == false);

    if (!object->mightBeType(MIRType_Object)) {
        trackOptimizationOutcome(TrackedOutcome::NotObject);
        return true;
    }

    if (!index->mightBeType(MIRType_Int32) &&
        !index->mightBeType(MIRType_String) &&
        !index->mightBeType(MIRType_Symbol))
    {
        trackOptimizationOutcome(TrackedOutcome::IndexType);
        return true;
    }

    
    
    
    SetElemICInspector icInspect(inspector->setElemICInspector(pc));
    if (!icInspect.sawDenseWrite() && !icInspect.sawTypedArrayWrite()) {
        trackOptimizationOutcome(TrackedOutcome::SetElemNonDenseNonTANotCached);
        return true;
    }

    if (PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current,
                                      &object, nullptr, &value,  true))
    {
        trackOptimizationOutcome(TrackedOutcome::NeedsTypeBarrier);
        return true;
    }

    
    
    
    
    bool guardHoles = ElementAccessHasExtraIndexedProperty(constraints(), object);

    
    object = addMaybeCopyElementsForWrite(object);

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), object, value));

    
    bool strict = JSOp(*pc) == JSOP_STRICTSETELEM;
    MInstruction* ins = MSetElementCache::New(alloc(), object, index, value, strict, guardHoles);
    current->add(ins);
    current->push(value);

    if (!resumeAfter(ins))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::jsop_setelem_dense(TemporaryTypeSet::DoubleConversion conversion,
                               SetElemSafety safety,
                               MDefinition* obj, MDefinition* id, MDefinition* value)
{
    MIRType elementType = DenseNativeElementType(constraints(), obj);
    bool packed = ElementAccessIsPacked(constraints(), obj);

    
    
    bool writeOutOfBounds = !ElementAccessHasExtraIndexedProperty(constraints(), obj);

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    
    MInstruction* idInt32 = MToInt32::New(alloc(), id);
    current->add(idInt32);
    id = idInt32;

    
    obj = addMaybeCopyElementsForWrite(obj);

    
    MElements* elements = MElements::New(alloc(), obj);
    current->add(elements);

    
    MDefinition* newValue = value;
    switch (conversion) {
      case TemporaryTypeSet::AlwaysConvertToDoubles:
      case TemporaryTypeSet::MaybeConvertToDoubles: {
        MInstruction* valueDouble = MToDouble::New(alloc(), value);
        current->add(valueDouble);
        newValue = valueDouble;
        break;
      }

      case TemporaryTypeSet::AmbiguousDoubleConversion: {
        MOZ_ASSERT(value->type() == MIRType_Int32);
        MInstruction* maybeDouble = MMaybeToDoubleElement::New(alloc(), elements, value);
        current->add(maybeDouble);
        newValue = maybeDouble;
        break;
      }

      case TemporaryTypeSet::DontConvertToDoubles:
        break;

      default:
        MOZ_CRASH("Unknown double conversion");
    }

    bool writeHole = false;
    if (safety == SetElem_Normal) {
        SetElemICInspector icInspect(inspector->setElemICInspector(pc));
        writeHole = icInspect.sawOOBDenseWrite();
    }

    
    
    
    MStoreElementCommon* store;
    if (writeHole && writeOutOfBounds) {
        MOZ_ASSERT(safety == SetElem_Normal);

        MStoreElementHole* ins = MStoreElementHole::New(alloc(), obj, elements, id, newValue);
        store = ins;

        current->add(ins);
        current->push(value);

        if (!resumeAfter(ins))
            return false;
    } else {
        MInitializedLength* initLength = MInitializedLength::New(alloc(), elements);
        current->add(initLength);

        bool needsHoleCheck;
        if (safety == SetElem_Normal) {
            id = addBoundsCheck(id, initLength);
            needsHoleCheck = !packed && !writeOutOfBounds;
        } else {
            needsHoleCheck = false;
        }

        MStoreElement* ins = MStoreElement::New(alloc(), elements, id, newValue, needsHoleCheck);
        store = ins;

        current->add(ins);

        if (safety == SetElem_Normal)
            current->push(value);

        if (!resumeAfter(ins))
            return false;
    }

    
    if (obj->resultTypeSet()->propertyNeedsBarrier(constraints(), JSID_VOID))
        store->setNeedsBarrier();

    if (elementType != MIRType_None && packed)
        store->setElementType(elementType);

    return true;
}


bool
IonBuilder::jsop_setelem_typed(Scalar::Type arrayType, SetElemSafety safety,
                               MDefinition* obj, MDefinition* id, MDefinition* value)
{
    bool expectOOB;
    if (safety == SetElem_Normal) {
        SetElemICInspector icInspect(inspector->setElemICInspector(pc));
        expectOOB = icInspect.sawOOBTypedArrayWrite();
    } else {
        expectOOB = false;
    }

    if (expectOOB)
        spew("Emitting OOB TypedArray SetElem");

    
    MInstruction* idInt32 = MToInt32::New(alloc(), id);
    current->add(idInt32);
    id = idInt32;

    
    MInstruction* length;
    MInstruction* elements;
    BoundsChecking checking = (!expectOOB && safety == SetElem_Normal)
                              ? DoBoundsCheck
                              : SkipBoundsCheck;
    addTypedArrayLengthAndData(obj, checking, &id, &length, &elements);

    
    MDefinition* toWrite = value;
    if (arrayType == Scalar::Uint8Clamped) {
        toWrite = MClampToUint8::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }

    
    MInstruction* ins;
    if (expectOOB) {
        ins = MStoreTypedArrayElementHole::New(alloc(), elements, length, id, toWrite, arrayType);
    } else {
        MStoreUnboxedScalar* store =
            MStoreUnboxedScalar::New(alloc(), elements, id, toWrite, arrayType);
        ins = store;
    }

    current->add(ins);

    if (safety == SetElem_Normal)
        current->push(value);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_setelem_typed_object(Scalar::Type arrayType, SetElemSafety safety,
                                      MDefinition* object, MDefinition* index, MDefinition* value)
{
    MOZ_ASSERT(safety == SetElem_Unsafe); 

    MInstruction* intIndex = MToInt32::New(alloc(), index);
    current->add(intIndex);

    size_t elemSize = ScalarTypeDescr::alignment(arrayType);

    LinearSum byteOffset(alloc());
    if (!byteOffset.add(intIndex, elemSize))
        setForceAbort();

    return storeScalarTypedObjectValue(object, byteOffset, arrayType, value);
}

bool
IonBuilder::jsop_length()
{
    if (jsop_length_fastPath())
        return true;

    PropertyName* name = info().getAtom(pc)->asPropertyName();
    return jsop_getprop(name);
}

bool
IonBuilder::jsop_length_fastPath()
{
    TemporaryTypeSet* types = bytecodeTypes(pc);

    if (types->getKnownMIRType() != MIRType_Int32)
        return false;

    MDefinition* obj = current->peek(-1);

    if (obj->mightBeType(MIRType_String)) {
        if (obj->mightBeType(MIRType_Object))
            return false;
        current->pop();
        MStringLength* ins = MStringLength::New(alloc(), obj);
        current->add(ins);
        current->push(ins);
        return true;
    }

    if (obj->mightBeType(MIRType_Object)) {
        TemporaryTypeSet* objTypes = obj->resultTypeSet();

        
        if (objTypes &&
            objTypes->getKnownClass(constraints()) == &ArrayObject::class_ &&
            !objTypes->hasObjectFlags(constraints(), OBJECT_FLAG_LENGTH_OVERFLOW))
        {
            current->pop();
            MElements* elements = MElements::New(alloc(), obj);
            current->add(elements);

            
            MArrayLength* length = MArrayLength::New(alloc(), elements);
            current->add(length);
            current->push(length);
            return true;
        }

        
        TypedObjectPrediction prediction = typedObjectPrediction(obj);
        if (!prediction.isUseless()) {
            TypeSet::ObjectKey* globalKey = TypeSet::ObjectKey::get(&script()->global());
            if (globalKey->hasFlags(constraints(), OBJECT_FLAG_TYPED_OBJECT_NEUTERED))
                return false;

            MInstruction* length;
            int32_t sizedLength;
            if (prediction.hasKnownArrayLength(&sizedLength)) {
                obj->setImplicitlyUsedUnchecked();
                length = MConstant::New(alloc(), Int32Value(sizedLength));
            } else {
                return false;
            }

            current->pop();
            current->add(length);
            current->push(length);
            return true;
        }
    }

    return false;
}

bool
IonBuilder::jsop_arguments()
{
    if (info().needsArgsObj()) {
        current->push(current->argumentsObject());
        return true;
    }
    MOZ_ASSERT(lazyArguments_);
    current->push(lazyArguments_);
    return true;
}

bool
IonBuilder::jsop_rest()
{
    ArrayObject* templateObject = &inspector->getTemplateObject(pc)->as<ArrayObject>();

    if (inliningDepth_ == 0) {
        
        MArgumentsLength* numActuals = MArgumentsLength::New(alloc());
        current->add(numActuals);

        
        
        MRest* rest = MRest::New(alloc(), constraints(), numActuals, info().nargs() - 1,
                                 templateObject);
        current->add(rest);
        current->push(rest);
        return true;
    }

    
    unsigned numActuals = inlineCallInfo_->argv().length();
    unsigned numFormals = info().nargs() - 1;
    unsigned numRest = numActuals > numFormals ? numActuals - numFormals : 0;

    MConstant* templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    current->add(templateConst);

    MNewArray* array = MNewArray::New(alloc(), constraints(), numRest, templateConst,
                                      templateObject->group()->initialHeap(constraints()),
                                      NewArray_FullyAllocating);
    current->add(array);

    if (numRest == 0) {
        
        
        current->push(array);
        return true;
    }

    MElements* elements = MElements::New(alloc(), array);
    current->add(elements);

    
    
    MConstant* index = nullptr;
    for (unsigned i = numFormals; i < numActuals; i++) {
        index = MConstant::New(alloc(), Int32Value(i - numFormals));
        current->add(index);

        MDefinition* arg = inlineCallInfo_->argv()[i];
        MStoreElement* store = MStoreElement::New(alloc(), elements, index, arg,
                                                   false);
        current->add(store);

        if (NeedsPostBarrier(info(), arg))
            current->add(MPostWriteBarrier::New(alloc(), array, arg));
    }

    
    
    
    MSetArrayLength* length = MSetArrayLength::New(alloc(), elements, index);
    current->add(length);

    
    
    MSetInitializedLength* initLength = MSetInitializedLength::New(alloc(), elements, index);
    current->add(initLength);

    current->push(array);
    return true;
}

uint32_t
IonBuilder::getDefiniteSlot(TemporaryTypeSet* types, PropertyName* name, uint32_t* pnfixed,
                            BaselineInspector::ObjectGroupVector& convertUnboxedGroups)
{
    if (!types || types->unknownObject()) {
        trackOptimizationOutcome(TrackedOutcome::NoTypeInfo);
        return UINT32_MAX;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    for (size_t i = 0; i < types->getObjectCount(); i++) {
        TypeSet::ObjectKey* key = types->getObject(i);
        if (!key)
            continue;

        if (ObjectGroup* group = key->maybeGroup()) {
            if (group->newScript() && !group->newScript()->analyzed()) {
                addAbortedPreliminaryGroup(group);
                trackOptimizationOutcome(TrackedOutcome::NoAnalysisInfo);
                return UINT32_MAX;
            }
            if (group->maybePreliminaryObjects()) {
                addAbortedPreliminaryGroup(group);
                trackOptimizationOutcome(TrackedOutcome::NoAnalysisInfo);
                return UINT32_MAX;
            }
        }
    }

    uint32_t slot = UINT32_MAX;

    for (size_t i = 0; i < types->getObjectCount(); i++) {
        TypeSet::ObjectKey* key = types->getObject(i);
        if (!key)
            continue;

        if (key->unknownProperties()) {
            trackOptimizationOutcome(TrackedOutcome::UnknownProperties);
            return UINT32_MAX;
        }

        if (key->isSingleton()) {
            trackOptimizationOutcome(TrackedOutcome::Singleton);
            return UINT32_MAX;
        }

        
        
        
        if (key->isGroup() && key->group()->maybeUnboxedLayout()) {
            if (ObjectGroup* nativeGroup = key->group()->unboxedLayout().nativeGroup()) {
                if (!convertUnboxedGroups.append(key->group()))
                    CrashAtUnhandlableOOM("IonBuilder::getDefiniteSlot");
                key = TypeSet::ObjectKey::get(nativeGroup);
            }
        }

        HeapTypeSetKey property = key->property(NameToId(name));
        if (!property.maybeTypes() ||
            !property.maybeTypes()->definiteProperty() ||
            property.nonData(constraints()))
        {
            trackOptimizationOutcome(TrackedOutcome::NotFixedSlot);
            return UINT32_MAX;
        }

        
        
        
        size_t nfixed = NativeObject::MAX_FIXED_SLOTS;
        if (ObjectGroup* group = key->group()->maybeOriginalUnboxedGroup())
            nfixed = gc::GetGCKindSlots(group->unboxedLayout().getAllocKind());

        uint32_t propertySlot = property.maybeTypes()->definiteSlot();
        if (slot == UINT32_MAX) {
            slot = propertySlot;
            *pnfixed = nfixed;
        } else if (slot != propertySlot || nfixed != *pnfixed) {
            trackOptimizationOutcome(TrackedOutcome::InconsistentFixedSlot);
            return UINT32_MAX;
        }
    }

    return slot;
}

uint32_t
IonBuilder::getUnboxedOffset(TemporaryTypeSet* types, PropertyName* name, JSValueType* punboxedType)
{
    if (!types || types->unknownObject()) {
        trackOptimizationOutcome(TrackedOutcome::NoTypeInfo);
        return UINT32_MAX;
    }

    uint32_t offset = UINT32_MAX;

    for (size_t i = 0; i < types->getObjectCount(); i++) {
        TypeSet::ObjectKey* key = types->getObject(i);
        if (!key)
            continue;

        if (key->unknownProperties()) {
            trackOptimizationOutcome(TrackedOutcome::UnknownProperties);
            return UINT32_MAX;
        }

        if (key->isSingleton()) {
            trackOptimizationOutcome(TrackedOutcome::Singleton);
            return UINT32_MAX;
        }

        UnboxedLayout* layout = key->group()->maybeUnboxedLayout();
        if (!layout) {
            trackOptimizationOutcome(TrackedOutcome::NotUnboxed);
            return UINT32_MAX;
        }

        const UnboxedLayout::Property* property = layout->lookup(name);
        if (!property) {
            trackOptimizationOutcome(TrackedOutcome::StructNoField);
            return UINT32_MAX;
        }

        if (layout->nativeGroup()) {
            trackOptimizationOutcome(TrackedOutcome::UnboxedConvertedToNative);
            return UINT32_MAX;
        }

        key->watchStateChangeForUnboxedConvertedToNative(constraints());

        if (offset == UINT32_MAX) {
            offset = property->offset;
            *punboxedType = property->type;
        } else if (offset != property->offset) {
            trackOptimizationOutcome(TrackedOutcome::InconsistentFieldOffset);
            return UINT32_MAX;
        } else if (*punboxedType != property->type) {
            trackOptimizationOutcome(TrackedOutcome::InconsistentFieldType);
            return UINT32_MAX;
        }
    }

    return offset;
}

bool
IonBuilder::jsop_runonce()
{
    MRunOncePrologue* ins = MRunOncePrologue::New(alloc());
    current->add(ins);
    return resumeAfter(ins);
}

bool
IonBuilder::jsop_not()
{
    MDefinition* value = current->pop();

    MNot* ins = MNot::New(alloc(), value);
    current->add(ins);
    current->push(ins);
    ins->cacheOperandMightEmulateUndefined(constraints());
    return true;
}

bool
IonBuilder::objectsHaveCommonPrototype(TemporaryTypeSet* types, PropertyName* name,
                                       bool isGetter, JSObject* foundProto, bool* guardGlobal)
{
    
    
    
    

    
    if (!types || types->unknownObject())
        return false;
    *guardGlobal = false;

    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        if (types->getSingleton(i) == foundProto)
            continue;

        TypeSet::ObjectKey* key = types->getObject(i);
        if (!key)
            continue;

        while (key) {
            if (key->unknownProperties())
                return false;

            const Class* clasp = key->clasp();
            if (!ClassHasEffectlessLookup(clasp))
                return false;
            JSObject* singleton = key->isSingleton() ? key->singleton() : nullptr;
            if (ObjectHasExtraOwnProperty(compartment, key, name)) {
                if (!singleton || !singleton->is<GlobalObject>())
                    return false;
                *guardGlobal = true;
            }

            
            
            if (isGetter && clasp->ops.getProperty)
                return false;
            if (!isGetter && clasp->ops.setProperty)
                return false;

            
            
            
            HeapTypeSetKey property = key->property(NameToId(name));
            if (TypeSet* types = property.maybeTypes()) {
                if (!types->empty() || types->nonDataProperty())
                    return false;
            }
            if (singleton) {
                if (CanHaveEmptyPropertyTypesForOwnProperty(singleton)) {
                    MOZ_ASSERT(singleton->is<GlobalObject>());
                    *guardGlobal = true;
                }
            }

            JSObject* proto = key->proto().toObjectOrNull();
            if (proto == foundProto)
                break;
            if (!proto) {
                
                
                return false;
            }
            key = TypeSet::ObjectKey::get(proto);
        }
    }

    return true;
}

void
IonBuilder::freezePropertiesForCommonPrototype(TemporaryTypeSet* types, PropertyName* name,
                                               JSObject* foundProto,
                                               bool allowEmptyTypesforGlobal)
{
    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        
        
        if (types->getSingleton(i) == foundProto)
            continue;

        TypeSet::ObjectKey* key = types->getObject(i);
        if (!key)
            continue;

        while (true) {
            HeapTypeSetKey property = key->property(NameToId(name));
            JS_ALWAYS_TRUE(!property.isOwnProperty(constraints(), allowEmptyTypesforGlobal));

            
            
            
            if (key->proto() == TaggedProto(foundProto))
                break;
            key = TypeSet::ObjectKey::get(key->proto().toObjectOrNull());
        }
    }
}

bool
IonBuilder::testCommonGetterSetter(TemporaryTypeSet* types, PropertyName* name,
                                   bool isGetter, JSObject* foundProto, Shape* lastProperty,
                                   JSFunction* getterOrSetter,
                                   MDefinition** guard,
                                   Shape* globalShape,
                                   MDefinition** globalGuard)
{
    MOZ_ASSERT_IF(globalShape, globalGuard);
    bool guardGlobal;

    
    if (!objectsHaveCommonPrototype(types, name, isGetter, foundProto, &guardGlobal) ||
        (guardGlobal && !globalShape))
    {
        trackOptimizationOutcome(TrackedOutcome::MultiProtoPaths);
        return false;
    }

    
    
    
    freezePropertiesForCommonPrototype(types, name, foundProto, guardGlobal);

    
    
    
    
    
    
    
    if (guardGlobal) {
        JSObject* obj = &script()->global();
        MDefinition* globalObj = constant(ObjectValue(*obj));
        *globalGuard = addShapeGuard(globalObj, globalShape, Bailout_ShapeGuard);
    }

    if (foundProto->isNative()) {
        NativeObject& nativeProto = foundProto->as<NativeObject>();
        if (nativeProto.lastProperty() == lastProperty) {
            
            
            
            Shape* propShape = nativeProto.lookupPure(name);
            MOZ_ASSERT_IF(isGetter, propShape->getterObject() == getterOrSetter);
            MOZ_ASSERT_IF(!isGetter, propShape->setterObject() == getterOrSetter);
            if (propShape && !propShape->configurable())
                return true;
        }
    }

    MInstruction* wrapper = constantMaybeNursery(foundProto);
    *guard = addShapeGuard(wrapper, lastProperty, Bailout_ShapeGuard);
    return true;
}

void
IonBuilder::replaceMaybeFallbackFunctionGetter(MGetPropertyCache* cache)
{
    
    WrapMGetPropertyCache rai(maybeFallbackFunctionGetter_);
    maybeFallbackFunctionGetter_ = cache;
}

bool
IonBuilder::annotateGetPropertyCache(MDefinition* obj, MGetPropertyCache* getPropCache,
                                     TemporaryTypeSet* objTypes,
                                     TemporaryTypeSet* pushedTypes)
{
    PropertyName* name = getPropCache->name();

    
    if (pushedTypes->unknownObject() || pushedTypes->baseFlags() != 0)
        return true;

    for (unsigned i = 0; i < pushedTypes->getObjectCount(); i++) {
        if (pushedTypes->getGroup(i) != nullptr)
            return true;
    }

    
    if (!objTypes || objTypes->baseFlags() || objTypes->unknownObject())
        return true;

    unsigned int objCount = objTypes->getObjectCount();
    if (objCount == 0)
        return true;

    InlinePropertyTable* inlinePropTable = getPropCache->initInlinePropertyTable(alloc(), pc);
    if (!inlinePropTable)
        return false;

    
    
    for (unsigned int i = 0; i < objCount; i++) {
        ObjectGroup* group = objTypes->getGroup(i);
        if (!group)
            continue;
        TypeSet::ObjectKey* key = TypeSet::ObjectKey::get(group);
        if (key->unknownProperties() || !key->proto().isObject())
            continue;

        const Class* clasp = key->clasp();
        if (!ClassHasEffectlessLookup(clasp) || ObjectHasExtraOwnProperty(compartment, key, name))
            continue;

        HeapTypeSetKey ownTypes = key->property(NameToId(name));
        if (ownTypes.isOwnProperty(constraints()))
            continue;

        JSObject* singleton = testSingletonProperty(key->proto().toObject(), name);
        if (!singleton || !singleton->is<JSFunction>())
            continue;

        
        if (!pushedTypes->hasType(TypeSet::ObjectType(singleton)))
            continue;

        if (!inlinePropTable->addEntry(alloc(), group, &singleton->as<JSFunction>()))
            return false;
    }

    if (inlinePropTable->numEntries() == 0) {
        getPropCache->clearInlinePropertyTable();
        return true;
    }

#ifdef DEBUG
    if (inlinePropTable->numEntries() > 0)
        JitSpew(JitSpew_Inlining, "Annotated GetPropertyCache with %d/%d inline cases",
                                    (int) inlinePropTable->numEntries(), (int) objCount);
#endif

    
    
    
    
    if (inlinePropTable->numEntries() > 0) {
        
        current->push(obj);
        MResumePoint* resumePoint = MResumePoint::New(alloc(), current, pc,
                                                      MResumePoint::ResumeAt);
        if (!resumePoint)
            return false;
        inlinePropTable->setPriorResumePoint(resumePoint);
        replaceMaybeFallbackFunctionGetter(getPropCache);
        current->pop();
    }
    return true;
}



bool
IonBuilder::invalidatedIdempotentCache()
{
    IonBuilder* builder = this;
    do {
        if (builder->script()->invalidatedIdempotentCache())
            return true;
        builder = builder->callerBuilder_;
    } while (builder);

    return false;
}

bool
IonBuilder::loadSlot(MDefinition* obj, size_t slot, size_t nfixed, MIRType rvalType,
                     BarrierKind barrier, TemporaryTypeSet* types)
{
    if (slot < nfixed) {
        MLoadFixedSlot* load = MLoadFixedSlot::New(alloc(), obj, slot);
        current->add(load);
        current->push(load);

        load->setResultType(rvalType);
        return pushTypeBarrier(load, types, barrier);
    }

    MSlots* slots = MSlots::New(alloc(), obj);
    current->add(slots);

    MLoadSlot* load = MLoadSlot::New(alloc(), slots, slot - nfixed);
    current->add(load);
    current->push(load);

    load->setResultType(rvalType);
    return pushTypeBarrier(load, types, barrier);
}

bool
IonBuilder::loadSlot(MDefinition* obj, Shape* shape, MIRType rvalType,
                     BarrierKind barrier, TemporaryTypeSet* types)
{
    return loadSlot(obj, shape->slot(), shape->numFixedSlots(), rvalType, barrier, types);
}

bool
IonBuilder::storeSlot(MDefinition* obj, size_t slot, size_t nfixed,
                      MDefinition* value, bool needsBarrier,
                      MIRType slotType )
{
    if (slot < nfixed) {
        MStoreFixedSlot* store = MStoreFixedSlot::New(alloc(), obj, slot, value);
        current->add(store);
        current->push(value);
        if (needsBarrier)
            store->setNeedsBarrier();
        return resumeAfter(store);
    }

    MSlots* slots = MSlots::New(alloc(), obj);
    current->add(slots);

    MStoreSlot* store = MStoreSlot::New(alloc(), slots, slot - nfixed, value);
    current->add(store);
    current->push(value);
    if (needsBarrier)
        store->setNeedsBarrier();
    if (slotType != MIRType_None)
        store->setSlotType(slotType);
    return resumeAfter(store);
}

bool
IonBuilder::storeSlot(MDefinition* obj, Shape* shape, MDefinition* value, bool needsBarrier,
                      MIRType slotType )
{
    MOZ_ASSERT(shape->writable());
    return storeSlot(obj, shape->slot(), shape->numFixedSlots(), value, needsBarrier, slotType);
}

bool
IonBuilder::jsop_getprop(PropertyName* name)
{
    bool emitted = false;
    startTrackingOptimizations();

    MDefinition* obj = current->pop();
    TemporaryTypeSet* types = bytecodeTypes(pc);

    trackTypeInfo(TrackedTypeSite::Receiver, obj->type(), obj->resultTypeSet());

    if (!info().isAnalysis()) {
        
        
        
        trackOptimizationAttempt(TrackedStrategy::GetProp_ArgumentsLength);
        if (!getPropTryArgumentsLength(&emitted, obj) || emitted)
            return emitted;

        
        trackOptimizationAttempt(TrackedStrategy::GetProp_ArgumentsCallee);
        if (!getPropTryArgumentsCallee(&emitted, obj, name) || emitted)
            return emitted;
    }

    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(),
                                                       obj, name, types);

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_InferredConstant);
    if (barrier == BarrierKind::NoBarrier) {
        if (!getPropTryInferredConstant(&emitted, obj, name, types) || emitted)
            return emitted;
    } else {
        trackOptimizationOutcome(TrackedOutcome::NeedsTypeBarrier);
    }

    
    
    
    
    if (info().isAnalysis() || types->empty()) {
        if (types->empty()) {
            
            
            
            trackOptimizationAttempt(TrackedStrategy::GetProp_InlineCache);
            trackOptimizationOutcome(TrackedOutcome::NoTypeInfo);
        }

        MCallGetProperty* call = MCallGetProperty::New(alloc(), obj, name, *pc == JSOP_CALLPROP);
        current->add(call);

        
        
        
        
        if (info().isAnalysis()) {
            if (!getPropTryConstant(&emitted, obj, name, types) || emitted)
                return emitted;
        }

        current->push(call);
        return resumeAfter(call) && pushTypeBarrier(call, types, BarrierKind::TypeSet);
    }

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_Constant);
    if (!getPropTryConstant(&emitted, obj, name, types) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_SimdGetter);
    if (!getPropTrySimdGetter(&emitted, obj, name) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_TypedObject);
    if (!getPropTryTypedObject(&emitted, obj, name) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_DefiniteSlot);
    if (!getPropTryDefiniteSlot(&emitted, obj, name, barrier, types) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_Unboxed);
    if (!getPropTryUnboxed(&emitted, obj, name, barrier, types) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_CommonGetter);
    if (!getPropTryCommonGetter(&emitted, obj, name, types) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_InlineAccess);
    if (!getPropTryInlineAccess(&emitted, obj, name, barrier, types) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_Innerize);
    if (!getPropTryInnerize(&emitted, obj, name, types) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::GetProp_InlineCache);
    if (!getPropTryCache(&emitted, obj, name, barrier, types) || emitted)
        return emitted;

    
    MCallGetProperty* call = MCallGetProperty::New(alloc(), obj, name, *pc == JSOP_CALLPROP);
    current->add(call);
    current->push(call);
    if (!resumeAfter(call))
        return false;

    if (*pc == JSOP_CALLPROP && IsNullOrUndefined(obj->type())) {
        
        
        
        
        return true;
    }

    return pushTypeBarrier(call, types, BarrierKind::TypeSet);
}

bool
IonBuilder::improveThisTypesForCall()
{
    
    
    
    
    
    
    
    

    MOZ_ASSERT(*pc == JSOP_CALLPROP || *pc == JSOP_CALLELEM);

    
    MDefinition* thisDef = current->peek(-2);
    if (thisDef->type() != MIRType_Value ||
        !thisDef->mightBeType(MIRType_Object) ||
        !thisDef->resultTypeSet() ||
        !thisDef->resultTypeSet()->objectOrSentinel())
    {
        return true;
    }

    
    TemporaryTypeSet* types = thisDef->resultTypeSet()->cloneObjectsOnly(alloc_->lifoAlloc());
    if (!types)
        return false;

    MFilterTypeSet* filter = MFilterTypeSet::New(alloc(), thisDef, types);
    current->add(filter);
    current->rewriteAtDepth(-2, filter);

    
    
    
    filter->setDependency(current->peek(-1)->toInstruction());
    return true;
}

bool
IonBuilder::checkIsDefinitelyOptimizedArguments(MDefinition* obj, bool* isOptimizedArgs)
{
    if (obj->type() != MIRType_MagicOptimizedArguments) {
        if (script()->argumentsHasVarBinding() &&
            obj->mightBeType(MIRType_MagicOptimizedArguments))
        {
            return abort("Type is not definitely lazy arguments.");
        }

        *isOptimizedArgs = false;
        return true;
    }

    *isOptimizedArgs = true;
    return true;
}

bool
IonBuilder::getPropTryInferredConstant(bool* emitted, MDefinition* obj, PropertyName* name,
                                       TemporaryTypeSet* types)
{
    MOZ_ASSERT(*emitted == false);

    
    TemporaryTypeSet* objTypes = obj->resultTypeSet();
    if (!objTypes) {
        trackOptimizationOutcome(TrackedOutcome::NoTypeInfo);
        return true;
    }

    JSObject* singleton = objTypes->maybeSingleton();
    if (!singleton) {
        trackOptimizationOutcome(TrackedOutcome::NotSingleton);
        return true;
    }

    TypeSet::ObjectKey* key = TypeSet::ObjectKey::get(singleton);
    if (key->unknownProperties()) {
        trackOptimizationOutcome(TrackedOutcome::UnknownProperties);
        return true;
    }

    HeapTypeSetKey property = key->property(NameToId(name));

    Value constantValue = UndefinedValue();
    if (property.constant(constraints(), &constantValue)) {
        spew("Optimized constant property");
        obj->setImplicitlyUsedUnchecked();
        if (!pushConstant(constantValue))
            return false;
        types->addType(TypeSet::GetValueType(constantValue), alloc_->lifoAlloc());
        trackOptimizationSuccess();
        *emitted = true;
    }

    return true;
}

bool
IonBuilder::getPropTryArgumentsLength(bool* emitted, MDefinition* obj)
{
    MOZ_ASSERT(*emitted == false);

    bool isOptimizedArgs = false;
    if (!checkIsDefinitelyOptimizedArguments(obj, &isOptimizedArgs))
        return false;
    if (!isOptimizedArgs)
        return true;

    if (JSOp(*pc) != JSOP_LENGTH)
        return true;

    trackOptimizationSuccess();
    *emitted = true;

    obj->setImplicitlyUsedUnchecked();

    
    if (inliningDepth_ == 0) {
        MInstruction* ins = MArgumentsLength::New(alloc());
        current->add(ins);
        current->push(ins);
        return true;
    }

    
    return pushConstant(Int32Value(inlineCallInfo_->argv().length()));
}

bool
IonBuilder::getPropTryArgumentsCallee(bool* emitted, MDefinition* obj, PropertyName* name)
{
    MOZ_ASSERT(*emitted == false);

    bool isOptimizedArgs = false;
    if (!checkIsDefinitelyOptimizedArguments(obj, &isOptimizedArgs))
        return false;
    if (!isOptimizedArgs)
        return true;

    if (name != names().callee)
        return true;

    MOZ_ASSERT(!script()->strict());

    obj->setImplicitlyUsedUnchecked();
    current->push(getCallee());

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryConstant(bool* emitted, MDefinition* obj, PropertyName* name,
                               TemporaryTypeSet* types)
{
    MOZ_ASSERT(*emitted == false);

    JSObject* singleton = types ? types->maybeSingleton() : nullptr;
    if (!singleton) {
        trackOptimizationOutcome(TrackedOutcome::NotSingleton);
        return true;
    }

    bool testObject, testString;
    if (!testSingletonPropertyTypes(obj, singleton, name, &testObject, &testString))
        return true;

    
    MOZ_ASSERT(!testString || !testObject);
    if (testObject)
        current->add(MGuardObject::New(alloc(), obj));
    else if (testString)
        current->add(MGuardString::New(alloc(), obj));
    else
        obj->setImplicitlyUsedUnchecked();

    pushConstant(ObjectValue(*singleton));

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

MIRType
IonBuilder::SimdTypeDescrToMIRType(SimdTypeDescr::Type type)
{
    switch (type) {
      case SimdTypeDescr::Int32x4:   return MIRType_Int32x4;
      case SimdTypeDescr::Float32x4: return MIRType_Float32x4;
      case SimdTypeDescr::Float64x2: return MIRType_Undefined;
    }
    MOZ_CRASH("unimplemented MIR type for a SimdTypeDescr::Type");
}

bool
IonBuilder::getPropTrySimdGetter(bool* emitted, MDefinition* obj, PropertyName* name)
{
    MOZ_ASSERT(!*emitted);

    if (!JitSupportsSimd()) {
        trackOptimizationOutcome(TrackedOutcome::NoSimdJitSupport);
        return true;
    }

    TypedObjectPrediction objPrediction = typedObjectPrediction(obj);
    if (objPrediction.isUseless()) {
        trackOptimizationOutcome(TrackedOutcome::AccessNotTypedObject);
        return true;
    }

    if (objPrediction.kind() != type::Simd) {
        trackOptimizationOutcome(TrackedOutcome::AccessNotSimdObject);
        return true;
    }

    MIRType type = SimdTypeDescrToMIRType(objPrediction.simdType());
    if (type == MIRType_Undefined) {
        trackOptimizationOutcome(TrackedOutcome::SimdTypeNotOptimized);
        return true;
    }

    const JSAtomState& names = compartment->runtime()->names();

    
    if (name == names.signMask) {
        MSimdSignMask* ins = MSimdSignMask::New(alloc(), obj, type);
        current->add(ins);
        current->push(ins);
        trackOptimizationSuccess();
        *emitted = true;
        return true;
    }

    
    SimdLane lane;
    if (name == names.x) {
        lane = LaneX;
    } else if (name == names.y) {
        lane = LaneY;
    } else if (name == names.z) {
        lane = LaneZ;
    } else if (name == names.w) {
        lane = LaneW;
    } else {
        
        trackOptimizationOutcome(TrackedOutcome::UnknownSimdProperty);
        return true;
    }

    MIRType scalarType = SimdTypeToScalarType(type);
    MSimdExtractElement* ins = MSimdExtractElement::New(alloc(), obj, type, scalarType, lane);
    current->add(ins);
    current->push(ins);
    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryTypedObject(bool* emitted,
                                  MDefinition* obj,
                                  PropertyName* name)
{
    TypedObjectPrediction fieldPrediction;
    size_t fieldOffset;
    size_t fieldIndex;
    if (!typedObjectHasField(obj, name, &fieldOffset, &fieldPrediction, &fieldIndex))
        return true;

    switch (fieldPrediction.kind()) {
      case type::Simd:
        
        return true;

      case type::Struct:
      case type::Array:
        return getPropTryComplexPropOfTypedObject(emitted,
                                                  obj,
                                                  fieldOffset,
                                                  fieldPrediction,
                                                  fieldIndex);

      case type::Reference:
        return getPropTryReferencePropOfTypedObject(emitted,
                                                    obj,
                                                    fieldOffset,
                                                    fieldPrediction,
                                                    name);

      case type::Scalar:
        return getPropTryScalarPropOfTypedObject(emitted,
                                                 obj,
                                                 fieldOffset,
                                                 fieldPrediction);
    }

    MOZ_CRASH("Bad kind");
}

bool
IonBuilder::getPropTryScalarPropOfTypedObject(bool* emitted, MDefinition* typedObj,
                                              int32_t fieldOffset,
                                              TypedObjectPrediction fieldPrediction)
{
    
    Scalar::Type fieldType = fieldPrediction.scalarType();

    
    TypeSet::ObjectKey* globalKey = TypeSet::ObjectKey::get(&script()->global());
    if (globalKey->hasFlags(constraints(), OBJECT_FLAG_TYPED_OBJECT_NEUTERED))
        return true;

    trackOptimizationSuccess();
    *emitted = true;

    LinearSum byteOffset(alloc());
    if (!byteOffset.add(fieldOffset))
        setForceAbort();

    return pushScalarLoadFromTypedObject(typedObj, byteOffset, fieldType);
}

bool
IonBuilder::getPropTryReferencePropOfTypedObject(bool* emitted, MDefinition* typedObj,
                                                 int32_t fieldOffset,
                                                 TypedObjectPrediction fieldPrediction,
                                                 PropertyName* name)
{
    ReferenceTypeDescr::Type fieldType = fieldPrediction.referenceType();

    TypeSet::ObjectKey* globalKey = TypeSet::ObjectKey::get(&script()->global());
    if (globalKey->hasFlags(constraints(), OBJECT_FLAG_TYPED_OBJECT_NEUTERED))
        return true;

    trackOptimizationSuccess();
    *emitted = true;

    LinearSum byteOffset(alloc());
    if (!byteOffset.add(fieldOffset))
        setForceAbort();

    return pushReferenceLoadFromTypedObject(typedObj, byteOffset, fieldType, name);
}

bool
IonBuilder::getPropTryComplexPropOfTypedObject(bool* emitted,
                                               MDefinition* typedObj,
                                               int32_t fieldOffset,
                                               TypedObjectPrediction fieldPrediction,
                                               size_t fieldIndex)
{
    
    TypeSet::ObjectKey* globalKey = TypeSet::ObjectKey::get(&script()->global());
    if (globalKey->hasFlags(constraints(), OBJECT_FLAG_TYPED_OBJECT_NEUTERED))
        return true;

    

    
    MDefinition* type = loadTypedObjectType(typedObj);
    MDefinition* fieldTypeObj = typeObjectForFieldFromStructType(type, fieldIndex);

    LinearSum byteOffset(alloc());
    if (!byteOffset.add(fieldOffset))
        setForceAbort();

    return pushDerivedTypedObject(emitted, typedObj, byteOffset,
                                  fieldPrediction, fieldTypeObj);
}

MDefinition*
IonBuilder::convertUnboxedObjects(MDefinition* obj,
                                  const BaselineInspector::ObjectGroupVector& list)
{
    for (size_t i = 0; i < list.length(); i++) {
        obj = MConvertUnboxedObjectToNative::New(alloc(), obj, list[i]);
        current->add(obj->toInstruction());
    }
    return obj;
}

bool
IonBuilder::getPropTryDefiniteSlot(bool* emitted, MDefinition* obj, PropertyName* name,
                                   BarrierKind barrier, TemporaryTypeSet* types)
{
    MOZ_ASSERT(*emitted == false);

    BaselineInspector::ObjectGroupVector convertUnboxedGroups(alloc());

    uint32_t nfixed;
    uint32_t slot = getDefiniteSlot(obj->resultTypeSet(), name, &nfixed, convertUnboxedGroups);
    if (slot == UINT32_MAX)
        return true;

    if (obj->type() != MIRType_Object) {
        MGuardObject* guard = MGuardObject::New(alloc(), obj);
        current->add(guard);
        obj = guard;
    }

    obj = convertUnboxedObjects(obj, convertUnboxedGroups);

    MInstruction* load;
    if (slot < nfixed) {
        load = MLoadFixedSlot::New(alloc(), obj, slot);
    } else {
        MInstruction* slots = MSlots::New(alloc(), obj);
        current->add(slots);

        load = MLoadSlot::New(alloc(), slots, slot - nfixed);
    }

    if (barrier == BarrierKind::NoBarrier)
        load->setResultType(types->getKnownMIRType());

    current->add(load);
    current->push(load);

    if (!pushTypeBarrier(load, types, barrier))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

MInstruction*
IonBuilder::loadUnboxedProperty(MDefinition* obj, size_t offset, JSValueType unboxedType,
                                BarrierKind barrier, TemporaryTypeSet* types)
{
    size_t scaledOffsetConstant = offset / UnboxedTypeSize(unboxedType);
    MInstruction* scaledOffset = MConstant::New(alloc(), Int32Value(scaledOffsetConstant));
    current->add(scaledOffset);

    MInstruction* load;
    switch (unboxedType) {
      case JSVAL_TYPE_BOOLEAN:
        load = MLoadUnboxedScalar::New(alloc(), obj, scaledOffset, Scalar::Uint8,
                                       DoesNotRequireMemoryBarrier,
                                       UnboxedPlainObject::offsetOfData());
        load->setResultType(MIRType_Boolean);
        break;

      case JSVAL_TYPE_INT32:
        load = MLoadUnboxedScalar::New(alloc(), obj, scaledOffset, Scalar::Int32,
                                       DoesNotRequireMemoryBarrier,
                                       UnboxedPlainObject::offsetOfData());
        load->setResultType(MIRType_Int32);
        break;

      case JSVAL_TYPE_DOUBLE:
        load = MLoadUnboxedScalar::New(alloc(), obj, scaledOffset, Scalar::Float64,
                                       DoesNotRequireMemoryBarrier,
                                       UnboxedPlainObject::offsetOfData(),
                                        false);
        load->setResultType(MIRType_Double);
        break;

      case JSVAL_TYPE_STRING:
        load = MLoadUnboxedString::New(alloc(), obj, scaledOffset,
                                       UnboxedPlainObject::offsetOfData());
        break;

      case JSVAL_TYPE_OBJECT: {
        MLoadUnboxedObjectOrNull::NullBehavior nullBehavior;
        if (types->hasType(TypeSet::NullType()) || barrier != BarrierKind::NoBarrier)
            nullBehavior = MLoadUnboxedObjectOrNull::HandleNull;
        else
            nullBehavior = MLoadUnboxedObjectOrNull::NullNotPossible;
        load = MLoadUnboxedObjectOrNull::New(alloc(), obj, scaledOffset, nullBehavior,
                                             UnboxedPlainObject::offsetOfData());
        break;
      }

      default:
        MOZ_CRASH();
    }

    current->add(load);
    return load;
}

bool
IonBuilder::getPropTryUnboxed(bool* emitted, MDefinition* obj, PropertyName* name,
                              BarrierKind barrier, TemporaryTypeSet* types)
{
    MOZ_ASSERT(*emitted == false);

    JSValueType unboxedType;
    uint32_t offset = getUnboxedOffset(obj->resultTypeSet(), name, &unboxedType);
    if (offset == UINT32_MAX)
        return true;

    if (obj->type() != MIRType_Object) {
        MGuardObject* guard = MGuardObject::New(alloc(), obj);
        current->add(guard);
        obj = guard;
    }

    MInstruction* load = loadUnboxedProperty(obj, offset, unboxedType, barrier, types);
    current->push(load);

    if (!pushTypeBarrier(load, types, barrier))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

MDefinition*
IonBuilder::addShapeGuardsForGetterSetter(MDefinition* obj, JSObject* holder, Shape* holderShape,
                const BaselineInspector::ReceiverVector& receivers,
                const BaselineInspector::ObjectGroupVector& convertUnboxedGroups,
                bool isOwnProperty)
{
    MOZ_ASSERT(holder);
    MOZ_ASSERT(holderShape);

    obj = convertUnboxedObjects(obj, convertUnboxedGroups);

    if (isOwnProperty) {
        MOZ_ASSERT(receivers.empty());
        return addShapeGuard(obj, holderShape, Bailout_ShapeGuard);
    }

    MDefinition* holderDef = constantMaybeNursery(holder);
    addShapeGuard(holderDef, holderShape, Bailout_ShapeGuard);

    return addGuardReceiverPolymorphic(obj, receivers);
}

bool
IonBuilder::getPropTryCommonGetter(bool* emitted, MDefinition* obj, PropertyName* name,
                                   TemporaryTypeSet* types)
{
    MOZ_ASSERT(*emitted == false);

    Shape* lastProperty = nullptr;
    JSFunction* commonGetter = nullptr;
    Shape* globalShape = nullptr;
    JSObject* foundProto = nullptr;
    bool isOwnProperty = false;
    BaselineInspector::ReceiverVector receivers(alloc());
    BaselineInspector::ObjectGroupVector convertUnboxedGroups(alloc());
    if (!inspector->commonGetPropFunction(pc, &foundProto, &lastProperty, &commonGetter,
                                          &globalShape, &isOwnProperty,
                                          receivers, convertUnboxedGroups))
    {
        return true;
    }

    TemporaryTypeSet* objTypes = obj->resultTypeSet();
    MDefinition* guard = nullptr;
    MDefinition* globalGuard = nullptr;
    bool canUseTIForGetter =
        testCommonGetterSetter(objTypes, name,  true,
                               foundProto, lastProperty, commonGetter, &guard,
                               globalShape, &globalGuard);
    if (!canUseTIForGetter) {
        
        
        obj = addShapeGuardsForGetterSetter(obj, foundProto, lastProperty,
                                            receivers, convertUnboxedGroups,
                                            isOwnProperty);
        if (!obj)
            return false;
    }

    bool isDOM = objTypes && objTypes->isDOMClass(constraints());

    if (isDOM && testShouldDOMCall(objTypes, commonGetter, JSJitInfo::Getter)) {
        const JSJitInfo* jitinfo = commonGetter->jitInfo();
        MInstruction* get;
        if (jitinfo->isAlwaysInSlot) {
            
            
            
            JSObject* singleton = objTypes->maybeSingleton();
            if (singleton && jitinfo->aliasSet() == JSJitInfo::AliasNone) {
                size_t slot = jitinfo->slotIndex;
                *emitted = true;
                return pushConstant(GetReservedSlot(singleton, slot));
            }

            
            
            get = MGetDOMMember::New(alloc(), jitinfo, obj, guard, globalGuard);
        } else {
            get = MGetDOMProperty::New(alloc(), jitinfo, obj, guard, globalGuard);
        }
        if (!get) {
            return false;
        }
        current->add(get);
        current->push(get);

        if (get->isEffectful() && !resumeAfter(get))
            return false;

        if (!pushDOMTypeBarrier(get, types, commonGetter))
            return false;

        trackOptimizationOutcome(TrackedOutcome::DOM);
        *emitted = true;
        return true;
    }

    
    if (obj->type() != MIRType_Object) {
        MGuardObject* guardObj = MGuardObject::New(alloc(), obj);
        current->add(guardObj);
        obj = guardObj;
    }

    

    
    if (!current->ensureHasSlots(2))
        return false;
    current->push(constantMaybeNursery(commonGetter));

    current->push(obj);

    CallInfo callInfo(alloc(), false);
    if (!callInfo.init(current, 0))
        return false;

    if (commonGetter->isNative()) {
        InliningStatus status = inlineNativeGetter(callInfo, commonGetter);
        switch (status) {
          case InliningStatus_Error:
            return false;
          case InliningStatus_WarmUpCountTooLow:
          case InliningStatus_NotInlined:
            break;
          case InliningStatus_Inlined:
            trackOptimizationOutcome(TrackedOutcome::Inlined);
            *emitted = true;
            return true;
        }
    }

    
    if (commonGetter->isInterpreted()) {
        InliningDecision decision = makeInliningDecision(commonGetter, callInfo);
        switch (decision) {
          case InliningDecision_Error:
            return false;
          case InliningDecision_DontInline:
          case InliningDecision_WarmUpCountTooLow:
            break;
          case InliningDecision_Inline:
            if (!inlineScriptedCall(callInfo, commonGetter))
                return false;
            *emitted = true;
            return true;
        }
    }

    JSFunction* tenuredCommonGetter = IsInsideNursery(commonGetter) ? nullptr : commonGetter;
    if (!makeCall(tenuredCommonGetter, callInfo))
        return false;

    
    
    
    if (!commonGetter->isInterpreted())
        trackOptimizationSuccess();

    *emitted = true;
    return true;
}

bool
IonBuilder::canInlinePropertyOpShapes(const BaselineInspector::ReceiverVector& receivers)
{
    if (receivers.empty()) {
        trackOptimizationOutcome(TrackedOutcome::NoShapeInfo);
        return false;
    }

    for (size_t i = 0; i < receivers.length(); i++) {
        
        
        
        
        if (receivers[i].shape && receivers[i].shape->inDictionary()) {
            trackOptimizationOutcome(TrackedOutcome::InDictionaryMode);
            return false;
        }
    }

    return true;
}

static Shape*
PropertyShapesHaveSameSlot(const BaselineInspector::ReceiverVector& receivers, jsid id)
{
    Shape* firstShape = nullptr;
    for (size_t i = 0; i < receivers.length(); i++) {
        if (receivers[i].group)
            return nullptr;

        Shape* shape = receivers[i].shape->searchLinear(id);
        MOZ_ASSERT(shape);

        if (i == 0) {
            firstShape = shape;
        } else if (shape->slot() != firstShape->slot() ||
                   shape->numFixedSlots() != firstShape->numFixedSlots())
        {
            return nullptr;
        }
    }

    return firstShape;
}

bool
IonBuilder::getPropTryInlineAccess(bool* emitted, MDefinition* obj, PropertyName* name,
                                   BarrierKind barrier, TemporaryTypeSet* types)
{
    MOZ_ASSERT(*emitted == false);

    if (obj->type() != MIRType_Object) {
        trackOptimizationOutcome(TrackedOutcome::NotObject);
        return true;
    }

    BaselineInspector::ReceiverVector receivers(alloc());
    BaselineInspector::ObjectGroupVector convertUnboxedGroups(alloc());
    if (!inspector->maybeInfoForPropertyOp(pc, receivers, convertUnboxedGroups))
        return false;

    if (!canInlinePropertyOpShapes(receivers))
        return true;

    obj = convertUnboxedObjects(obj, convertUnboxedGroups);

    MIRType rvalType = types->getKnownMIRType();
    if (barrier != BarrierKind::NoBarrier || IsNullOrUndefined(rvalType))
        rvalType = MIRType_Value;

    if (receivers.length() == 1) {
        if (!receivers[0].group) {
            
            spew("Inlining monomorphic native GETPROP");

            obj = addShapeGuard(obj, receivers[0].shape, Bailout_ShapeGuard);

            Shape* shape = receivers[0].shape->searchLinear(NameToId(name));
            MOZ_ASSERT(shape);

            if (!loadSlot(obj, shape, rvalType, barrier, types))
                return false;

            trackOptimizationOutcome(TrackedOutcome::Monomorphic);
            *emitted = true;
            return true;
        }

        if (receivers[0].shape) {
            
            spew("Inlining monomorphic unboxed expando GETPROP");

            obj = addGroupGuard(obj, receivers[0].group, Bailout_ShapeGuard);
            obj = addUnboxedExpandoGuard(obj,  true, Bailout_ShapeGuard);

            MInstruction* expando = MLoadUnboxedExpando::New(alloc(), obj);
            current->add(expando);

            expando = addShapeGuard(expando, receivers[0].shape, Bailout_ShapeGuard);

            Shape* shape = receivers[0].shape->searchLinear(NameToId(name));
            MOZ_ASSERT(shape);

            if (!loadSlot(expando, shape, rvalType, barrier, types))
                return false;

            trackOptimizationOutcome(TrackedOutcome::Monomorphic);
            *emitted = true;
            return true;
        }

        
        obj = addGroupGuard(obj, receivers[0].group, Bailout_ShapeGuard);

        const UnboxedLayout::Property* property = receivers[0].group->unboxedLayout().lookup(name);
        MInstruction* load = loadUnboxedProperty(obj, property->offset, property->type, barrier, types);
        current->push(load);

        if (!pushTypeBarrier(load, types, barrier))
            return false;

        trackOptimizationOutcome(TrackedOutcome::Monomorphic);
        *emitted = true;
        return true;
    }

    MOZ_ASSERT(receivers.length() > 1);
    spew("Inlining polymorphic GETPROP");

    if (Shape* propShape = PropertyShapesHaveSameSlot(receivers, NameToId(name))) {
        obj = addGuardReceiverPolymorphic(obj, receivers);
        if (!obj)
            return false;

        if (!loadSlot(obj, propShape, rvalType, barrier, types))
            return false;

        trackOptimizationOutcome(TrackedOutcome::Polymorphic);
        *emitted = true;
        return true;
    }

    MGetPropertyPolymorphic* load = MGetPropertyPolymorphic::New(alloc(), obj, name);
    current->add(load);
    current->push(load);

    for (size_t i = 0; i < receivers.length(); i++) {
        Shape* propShape = nullptr;
        if (receivers[i].shape) {
            propShape = receivers[i].shape->searchLinear(NameToId(name));
            MOZ_ASSERT(propShape);
        }
        if (!load->addReceiver(receivers[i], propShape))
            return false;
    }

    if (failedShapeGuard_)
        load->setNotMovable();

    load->setResultType(rvalType);
    if (!pushTypeBarrier(load, types, barrier))
        return false;

    trackOptimizationOutcome(TrackedOutcome::Polymorphic);
    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryCache(bool* emitted, MDefinition* obj, PropertyName* name,
                            BarrierKind barrier, TemporaryTypeSet* types)
{
    MOZ_ASSERT(*emitted == false);

    
    
    if (obj->type() != MIRType_Object) {
        TemporaryTypeSet* types = obj->resultTypeSet();
        if (!types || !types->objectOrSentinel()) {
            trackOptimizationOutcome(TrackedOutcome::NoTypeInfo);
            return true;
        }
    }

    
    
    if (inspector->hasSeenAccessedGetter(pc))
        barrier = BarrierKind::TypeSet;

    
    
    if (barrier != BarrierKind::TypeSet) {
        BarrierKind protoBarrier =
            PropertyReadOnPrototypeNeedsTypeBarrier(constraints(), obj, name, types);
        if (protoBarrier != BarrierKind::NoBarrier) {
            MOZ_ASSERT(barrier <= protoBarrier);
            barrier = protoBarrier;
        }
    }

    MGetPropertyCache* load = MGetPropertyCache::New(alloc(), obj, name,
                                                     barrier == BarrierKind::TypeSet);

    
    if (obj->type() == MIRType_Object && !invalidatedIdempotentCache()) {
        if (PropertyReadIsIdempotent(constraints(), obj, name))
            load->setIdempotent();
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    if (JSOp(*pc) == JSOP_CALLPROP && load->idempotent()) {
        if (!annotateGetPropertyCache(obj, load, obj->resultTypeSet(), types))
            return false;
    }

    current->add(load);
    current->push(load);

    if (load->isEffectful() && !resumeAfter(load))
        return false;

    MIRType rvalType = types->getKnownMIRType();
    if (barrier != BarrierKind::NoBarrier || IsNullOrUndefined(rvalType))
        rvalType = MIRType_Value;
    load->setResultType(rvalType);

    if (!pushTypeBarrier(load, types, barrier))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

MDefinition*
IonBuilder::tryInnerizeWindow(MDefinition* obj)
{
    
    
    
    
    

    if (obj->type() != MIRType_Object)
        return obj;

    TemporaryTypeSet* types = obj->resultTypeSet();
    if (!types)
        return obj;

    JSObject* singleton = types->maybeSingleton();
    if (!singleton)
        return obj;

    JSObject* inner = GetInnerObject(singleton);
    if (inner == singleton || inner != &script()->global())
        return obj;

    
    
    
    TypeSet::ObjectKey* key = TypeSet::ObjectKey::get(singleton);
    if (key->hasFlags(constraints(), OBJECT_FLAG_UNKNOWN_PROPERTIES))
        return obj;

    obj->setImplicitlyUsedUnchecked();
    return constant(ObjectValue(script()->global()));
}

bool
IonBuilder::getPropTryInnerize(bool* emitted, MDefinition* obj, PropertyName* name,
                               TemporaryTypeSet* types)
{
    

    MOZ_ASSERT(*emitted == false);

    MDefinition* inner = tryInnerizeWindow(obj);
    if (inner == obj)
        return true;

    
    
    

    if (!getPropTryConstant(emitted, inner, name, types) || *emitted)
        return *emitted;

    if (!getStaticName(&script()->global(), name, emitted) || *emitted)
        return *emitted;

    if (!getPropTryCommonGetter(emitted, inner, name, types) || *emitted)
        return *emitted;

    
    
    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(),
                                                       inner, name, types);
    if (!getPropTryCache(emitted, inner, name, barrier, types) || *emitted)
        return *emitted;

    MOZ_ASSERT(*emitted == false);
    return true;
}

bool
IonBuilder::jsop_setprop(PropertyName* name)
{
    MDefinition* value = current->pop();
    MDefinition* obj = current->pop();

    bool emitted = false;
    startTrackingOptimizations();
    trackTypeInfo(TrackedTypeSite::Receiver, obj->type(), obj->resultTypeSet());
    trackTypeInfo(TrackedTypeSite::Value, value->type(), value->resultTypeSet());

    
    
    if (info().isAnalysis()) {
        bool strict = IsStrictSetPC(pc);
        MInstruction* ins = MCallSetProperty::New(alloc(), obj, value, name, strict);
        current->add(ins);
        current->push(value);
        return resumeAfter(ins);
    }

    
    trackOptimizationAttempt(TrackedStrategy::SetProp_CommonSetter);
    if (!setPropTryCommonSetter(&emitted, obj, name, value) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::SetProp_TypedObject);
    if (!setPropTryTypedObject(&emitted, obj, name, value) || emitted)
        return emitted;

    TemporaryTypeSet* objTypes = obj->resultTypeSet();
    bool barrier = PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current, &obj, name, &value,
                                                  true);

    
    trackOptimizationAttempt(TrackedStrategy::SetProp_Unboxed);
    if (!setPropTryUnboxed(&emitted, obj, name, value, barrier, objTypes) || emitted)
        return emitted;

    
    
    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    
    trackOptimizationAttempt(TrackedStrategy::SetProp_DefiniteSlot);
    if (!setPropTryDefiniteSlot(&emitted, obj, name, value, barrier, objTypes) || emitted)
        return emitted;

    
    trackOptimizationAttempt(TrackedStrategy::SetProp_InlineAccess);
    if (!setPropTryInlineAccess(&emitted, obj, name, value, barrier, objTypes) || emitted)
        return emitted;

    
    return setPropTryCache(&emitted, obj, name, value, barrier, objTypes);
}

bool
IonBuilder::setPropTryCommonSetter(bool* emitted, MDefinition* obj,
                                   PropertyName* name, MDefinition* value)
{
    MOZ_ASSERT(*emitted == false);

    Shape* lastProperty = nullptr;
    JSFunction* commonSetter = nullptr;
    JSObject* foundProto = nullptr;
    bool isOwnProperty;
    BaselineInspector::ReceiverVector receivers(alloc());
    BaselineInspector::ObjectGroupVector convertUnboxedGroups(alloc());
    if (!inspector->commonSetPropFunction(pc, &foundProto, &lastProperty, &commonSetter,
                                          &isOwnProperty,
                                          receivers, convertUnboxedGroups))
    {
        trackOptimizationOutcome(TrackedOutcome::NoProtoFound);
        return true;
    }

    TemporaryTypeSet* objTypes = obj->resultTypeSet();
    MDefinition* guard = nullptr;
    bool canUseTIForSetter =
        testCommonGetterSetter(objTypes, name,  false,
                               foundProto, lastProperty, commonSetter, &guard);
    if (!canUseTIForSetter) {
        
        
        obj = addShapeGuardsForGetterSetter(obj, foundProto, lastProperty,
                                            receivers, convertUnboxedGroups,
                                            isOwnProperty);
        if (!obj)
            return false;
    }

    

    
    
    

    
    if (!setPropTryCommonDOMSetter(emitted, obj, value, commonSetter, objTypes))
        return false;

    if (*emitted) {
        trackOptimizationOutcome(TrackedOutcome::DOM);
        return true;
    }

    
    if (obj->type() != MIRType_Object) {
        MGuardObject* guardObj = MGuardObject::New(alloc(), obj);
        current->add(guardObj);
        obj = guardObj;
    }

    
    
    if (!current->ensureHasSlots(3))
        return false;

    current->push(constantMaybeNursery(commonSetter));
    current->push(obj);
    current->push(value);

    
    
    CallInfo callInfo(alloc(), false);
    if (!callInfo.init(current, 1))
        return false;

    
    callInfo.markAsSetter();

    
    if (commonSetter->isInterpreted()) {
        InliningDecision decision = makeInliningDecision(commonSetter, callInfo);
        switch (decision) {
          case InliningDecision_Error:
            return false;
          case InliningDecision_DontInline:
          case InliningDecision_WarmUpCountTooLow:
            break;
          case InliningDecision_Inline:
            if (!inlineScriptedCall(callInfo, commonSetter))
                return false;
            *emitted = true;
            return true;
        }
    }

    JSFunction* tenuredCommonSetter = IsInsideNursery(commonSetter) ? nullptr : commonSetter;
    MCall* call = makeCallHelper(tenuredCommonSetter, callInfo);
    if (!call)
        return false;

    current->push(value);
    if (!resumeAfter(call))
        return false;

    
    
    
    if (!commonSetter->isInterpreted())
        trackOptimizationSuccess();

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryCommonDOMSetter(bool* emitted, MDefinition* obj,
                                      MDefinition* value, JSFunction* setter,
                                      TemporaryTypeSet* objTypes)
{
    MOZ_ASSERT(*emitted == false);

    if (!objTypes || !objTypes->isDOMClass(constraints()))
        return true;

    if (!testShouldDOMCall(objTypes, setter, JSJitInfo::Setter))
        return true;

    
    MOZ_ASSERT(setter->jitInfo()->type() == JSJitInfo::Setter);
    MSetDOMProperty* set = MSetDOMProperty::New(alloc(), setter->jitInfo()->setter, obj, value);

    current->add(set);
    current->push(value);

    if (!resumeAfter(set))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryTypedObject(bool* emitted, MDefinition* obj,
                                  PropertyName* name, MDefinition* value)
{
    TypedObjectPrediction fieldPrediction;
    size_t fieldOffset;
    size_t fieldIndex;
    if (!typedObjectHasField(obj, name, &fieldOffset, &fieldPrediction, &fieldIndex))
        return true;

    switch (fieldPrediction.kind()) {
      case type::Simd:
        
        return true;

      case type::Reference:
        return setPropTryReferencePropOfTypedObject(emitted, obj, fieldOffset,
                                                    value, fieldPrediction, name);

      case type::Scalar:
        return setPropTryScalarPropOfTypedObject(emitted, obj, fieldOffset,
                                                 value, fieldPrediction);

      case type::Struct:
      case type::Array:
        return true;
    }

    MOZ_CRASH("Unknown kind");
}

bool
IonBuilder::setPropTryReferencePropOfTypedObject(bool* emitted,
                                                 MDefinition* obj,
                                                 int32_t fieldOffset,
                                                 MDefinition* value,
                                                 TypedObjectPrediction fieldPrediction,
                                                 PropertyName* name)
{
    ReferenceTypeDescr::Type fieldType = fieldPrediction.referenceType();

    TypeSet::ObjectKey* globalKey = TypeSet::ObjectKey::get(&script()->global());
    if (globalKey->hasFlags(constraints(), OBJECT_FLAG_TYPED_OBJECT_NEUTERED))
        return true;

    LinearSum byteOffset(alloc());
    if (!byteOffset.add(fieldOffset))
        setForceAbort();

    if (!storeReferenceTypedObjectValue(obj, byteOffset, fieldType, value, name))
        return true;

    current->push(value);

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryScalarPropOfTypedObject(bool* emitted,
                                              MDefinition* obj,
                                              int32_t fieldOffset,
                                              MDefinition* value,
                                              TypedObjectPrediction fieldPrediction)
{
    
    Scalar::Type fieldType = fieldPrediction.scalarType();

    
    TypeSet::ObjectKey* globalKey = TypeSet::ObjectKey::get(&script()->global());
    if (globalKey->hasFlags(constraints(), OBJECT_FLAG_TYPED_OBJECT_NEUTERED))
        return true;

    LinearSum byteOffset(alloc());
    if (!byteOffset.add(fieldOffset))
        setForceAbort();

    if (!storeScalarTypedObjectValue(obj, byteOffset, fieldType, value))
        return false;

    current->push(value);

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryDefiniteSlot(bool* emitted, MDefinition* obj,
                                   PropertyName* name, MDefinition* value,
                                   bool barrier, TemporaryTypeSet* objTypes)
{
    MOZ_ASSERT(*emitted == false);

    if (barrier) {
        trackOptimizationOutcome(TrackedOutcome::NeedsTypeBarrier);
        return true;
    }

    BaselineInspector::ObjectGroupVector convertUnboxedGroups(alloc());

    uint32_t nfixed;
    uint32_t slot = getDefiniteSlot(obj->resultTypeSet(), name, &nfixed, convertUnboxedGroups);
    if (slot == UINT32_MAX)
        return true;

    bool writeBarrier = false;
    for (size_t i = 0; i < obj->resultTypeSet()->getObjectCount(); i++) {
        TypeSet::ObjectKey* key = obj->resultTypeSet()->getObject(i);
        if (!key)
            continue;

        HeapTypeSetKey property = key->property(NameToId(name));
        if (property.nonWritable(constraints())) {
            trackOptimizationOutcome(TrackedOutcome::NonWritableProperty);
            return true;
        }
        writeBarrier |= property.needsBarrier(constraints());
    }

    obj = convertUnboxedObjects(obj, convertUnboxedGroups);

    MInstruction* store;
    if (slot < nfixed) {
        store = MStoreFixedSlot::New(alloc(), obj, slot, value);
        if (writeBarrier)
            store->toStoreFixedSlot()->setNeedsBarrier();
    } else {
        MInstruction* slots = MSlots::New(alloc(), obj);
        current->add(slots);

        store = MStoreSlot::New(alloc(), slots, slot - nfixed, value);
        if (writeBarrier)
            store->toStoreSlot()->setNeedsBarrier();
    }

    current->add(store);
    current->push(value);

    if (!resumeAfter(store))
        return false;

    trackOptimizationSuccess();
    *emitted = true;
    return true;
}

MInstruction*
IonBuilder::storeUnboxedProperty(MDefinition* obj, size_t offset, JSValueType unboxedType,
                                 MDefinition* value)
{
    size_t scaledOffsetConstant = offset / UnboxedTypeSize(unboxedType);
    MInstruction* scaledOffset = MConstant::New(alloc(), Int32Value(scaledOffsetConstant));
    current->add(scaledOffset);

    MInstruction* store;
    switch (unboxedType) {
      case JSVAL_TYPE_BOOLEAN:
        store = MStoreUnboxedScalar::New(alloc(), obj, scaledOffset, value, Scalar::Uint8,
                                         DoesNotRequireMemoryBarrier,
                                         UnboxedPlainObject::offsetOfData());
        break;

      case JSVAL_TYPE_INT32:
        store = MStoreUnboxedScalar::New(alloc(), obj, scaledOffset, value, Scalar::Int32,
                                         DoesNotRequireMemoryBarrier,
                                         UnboxedPlainObject::offsetOfData());
        break;

      case JSVAL_TYPE_DOUBLE:
        store = MStoreUnboxedScalar::New(alloc(), obj, scaledOffset, value, Scalar::Float64,
                                         DoesNotRequireMemoryBarrier,
                                         UnboxedPlainObject::offsetOfData());
        break;

      case JSVAL_TYPE_STRING:
        store = MStoreUnboxedString::New(alloc(), obj, scaledOffset, value,
                                         UnboxedPlainObject::offsetOfData());
        break;

      case JSVAL_TYPE_OBJECT:
        store = MStoreUnboxedObjectOrNull::New(alloc(), obj, scaledOffset, value, obj,
                                               UnboxedPlainObject::offsetOfData());
        break;

      default:
        MOZ_CRASH();
    }

    current->add(store);
    return store;
}

bool
IonBuilder::setPropTryUnboxed(bool* emitted, MDefinition* obj,
                              PropertyName* name, MDefinition* value,
                              bool barrier, TemporaryTypeSet* objTypes)
{
    MOZ_ASSERT(*emitted == false);

    if (barrier) {
        trackOptimizationOutcome(TrackedOutcome::NeedsTypeBarrier);
        return true;
    }

    JSValueType unboxedType;
    uint32_t offset = getUnboxedOffset(obj->resultTypeSet(), name, &unboxedType);
    if (offset == UINT32_MAX)
        return true;

    if (obj->type() != MIRType_Object) {
        MGuardObject* guard = MGuardObject::New(alloc(), obj);
        current->add(guard);
        obj = guard;
    }

    MInstruction* store = storeUnboxedProperty(obj, offset, unboxedType, value);

    current->push(value);

    if (!resumeAfter(store))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryInlineAccess(bool* emitted, MDefinition* obj,
                                   PropertyName* name, MDefinition* value,
                                   bool barrier, TemporaryTypeSet* objTypes)
{
    MOZ_ASSERT(*emitted == false);

    if (barrier) {
        trackOptimizationOutcome(TrackedOutcome::NeedsTypeBarrier);
        return true;
    }

    BaselineInspector::ReceiverVector receivers(alloc());
    BaselineInspector::ObjectGroupVector convertUnboxedGroups(alloc());
    if (!inspector->maybeInfoForPropertyOp(pc, receivers, convertUnboxedGroups))
        return false;

    if (!canInlinePropertyOpShapes(receivers))
        return true;

    obj = convertUnboxedObjects(obj, convertUnboxedGroups);

    if (receivers.length() == 1) {
        if (!receivers[0].group) {
            
            spew("Inlining monomorphic native SETPROP");

            obj = addShapeGuard(obj, receivers[0].shape, Bailout_ShapeGuard);

            Shape* shape = receivers[0].shape->searchLinear(NameToId(name));
            MOZ_ASSERT(shape);

            bool needsBarrier = objTypes->propertyNeedsBarrier(constraints(), NameToId(name));
            if (!storeSlot(obj, shape, value, needsBarrier))
                return false;

            trackOptimizationOutcome(TrackedOutcome::Monomorphic);
            *emitted = true;
            return true;
        }

        if (receivers[0].shape) {
            
            spew("Inlining monomorphic unboxed expando SETPROP");

            obj = addGroupGuard(obj, receivers[0].group, Bailout_ShapeGuard);
            obj = addUnboxedExpandoGuard(obj,  true, Bailout_ShapeGuard);

            MInstruction* expando = MLoadUnboxedExpando::New(alloc(), obj);
            current->add(expando);

            expando = addShapeGuard(expando, receivers[0].shape, Bailout_ShapeGuard);

            Shape* shape = receivers[0].shape->searchLinear(NameToId(name));
            MOZ_ASSERT(shape);

            bool needsBarrier = objTypes->propertyNeedsBarrier(constraints(), NameToId(name));
            if (!storeSlot(expando, shape, value, needsBarrier))
                return false;

            trackOptimizationOutcome(TrackedOutcome::Monomorphic);
            *emitted = true;
            return true;
        }

        
        spew("Inlining monomorphic unboxed SETPROP");

        ObjectGroup* group = receivers[0].group;
        obj = addGroupGuard(obj, group, Bailout_ShapeGuard);

        const UnboxedLayout::Property* property = group->unboxedLayout().lookup(name);
        storeUnboxedProperty(obj, property->offset, property->type, value);

        current->push(value);

        trackOptimizationOutcome(TrackedOutcome::Monomorphic);
        *emitted = true;
        return true;
    }

    MOZ_ASSERT(receivers.length() > 1);
    spew("Inlining polymorphic SETPROP");

    if (Shape* propShape = PropertyShapesHaveSameSlot(receivers, NameToId(name))) {
        obj = addGuardReceiverPolymorphic(obj, receivers);
        if (!obj)
            return false;

        bool needsBarrier = objTypes->propertyNeedsBarrier(constraints(), NameToId(name));
        if (!storeSlot(obj, propShape, value, needsBarrier))
            return false;

        trackOptimizationOutcome(TrackedOutcome::Polymorphic);
        *emitted = true;
        return true;
    }

    MSetPropertyPolymorphic* ins = MSetPropertyPolymorphic::New(alloc(), obj, value, name);
    current->add(ins);
    current->push(value);

    for (size_t i = 0; i < receivers.length(); i++) {
        Shape* propShape = nullptr;
        if (receivers[i].shape) {
            propShape = receivers[i].shape->searchLinear(NameToId(name));
            MOZ_ASSERT(propShape);
        }
        if (!ins->addReceiver(receivers[i], propShape))
            return false;
    }

    if (objTypes->propertyNeedsBarrier(constraints(), NameToId(name)))
        ins->setNeedsBarrier();

    if (!resumeAfter(ins))
        return false;

    trackOptimizationOutcome(TrackedOutcome::Polymorphic);
    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryCache(bool* emitted, MDefinition* obj,
                            PropertyName* name, MDefinition* value,
                            bool barrier, TemporaryTypeSet* objTypes)
{
    MOZ_ASSERT(*emitted == false);

    bool strict = IsStrictSetPC(pc);
    
    MSetPropertyCache* ins = MSetPropertyCache::New(alloc(), obj, value, name, strict, barrier);

    if (!objTypes || objTypes->propertyNeedsBarrier(constraints(), NameToId(name)))
        ins->setNeedsBarrier();

    current->add(ins);
    current->push(value);

    if (!resumeAfter(ins))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::jsop_delprop(PropertyName* name)
{
    MDefinition* obj = current->pop();

    bool strict = JSOp(*pc) == JSOP_STRICTDELPROP;
    MInstruction* ins = MDeleteProperty::New(alloc(), obj, name, strict);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_delelem()
{
    MDefinition* index = current->pop();
    MDefinition* obj = current->pop();

    bool strict = JSOp(*pc) == JSOP_STRICTDELELEM;
    MDeleteElement* ins = MDeleteElement::New(alloc(), obj, index, strict);
    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_regexp(RegExpObject* reobj)
{
    
    
    
    
    
    
    
    

    bool mustClone = true;
    TypeSet::ObjectKey* globalKey = TypeSet::ObjectKey::get(&script()->global());
    if (!globalKey->hasFlags(constraints(), OBJECT_FLAG_REGEXP_FLAGS_SET)) {
#ifdef DEBUG
        
        
        if (script()->global().hasRegExpStatics()) {
            RegExpStatics* res = script()->global().getAlreadyCreatedRegExpStatics();
            MOZ_ASSERT(res);
            uint32_t origFlags = reobj->getFlags();
            uint32_t staticsFlags = res->getFlags();
            MOZ_ASSERT((origFlags & staticsFlags) == staticsFlags);
        }
#endif

        if (!reobj->global() && !reobj->sticky())
            mustClone = false;
    }

    MRegExp* regexp = MRegExp::New(alloc(), constraints(), reobj, mustClone);
    current->add(regexp);
    current->push(regexp);

    return true;
}

bool
IonBuilder::jsop_object(JSObject* obj)
{
    if (options.cloneSingletons()) {
        MCloneLiteral* clone = MCloneLiteral::New(alloc(), constant(ObjectValue(*obj)));
        current->add(clone);
        current->push(clone);
        return resumeAfter(clone);
    }

    compartment->setSingletonsAsValues();
    pushConstant(ObjectValue(*obj));
    return true;
}

bool
IonBuilder::jsop_lambda(JSFunction* fun)
{
    MOZ_ASSERT(analysis().usesScopeChain());
    MOZ_ASSERT(!fun->isArrow());

    if (fun->isNative() && IsAsmJSModuleNative(fun->native()))
        return abort("asm.js module function");

    MConstant* cst = MConstant::NewConstraintlessObject(alloc(), fun);
    current->add(cst);
    MLambda* ins = MLambda::New(alloc(), constraints(), current->scopeChain(), cst);
    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_lambda_arrow(JSFunction* fun)
{
    MOZ_ASSERT(analysis().usesScopeChain());
    MOZ_ASSERT(fun->isArrow());
    MOZ_ASSERT(!fun->isNative());

    MDefinition* thisDef = current->pop();

    MLambdaArrow* ins = MLambdaArrow::New(alloc(), constraints(), current->scopeChain(),
                                          thisDef, fun);
    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_setarg(uint32_t arg)
{
    
    
    
    
    
    MOZ_ASSERT(analysis_.hasSetArg());
    MDefinition* val = current->peek(-1);

    
    
    if (info().argsObjAliasesFormals()) {
        if (NeedsPostBarrier(info(), val))
            current->add(MPostWriteBarrier::New(alloc(), current->argumentsObject(), val));
        current->add(MSetArgumentsObjectArg::New(alloc(), current->argumentsObject(),
                                                 GET_ARGNO(pc), val));
        return true;
    }

    
    
    if (info().hasArguments())
	return abort("NYI: arguments & setarg.");

    
    
    
    
    
    if (info().argumentsAliasesFormals()) {
        
        MOZ_ASSERT(script()->uninlineable() && !isInlineBuilder());

        MSetFrameArgument* store = MSetFrameArgument::New(alloc(), arg, val);
        modifiesFrameArguments_ = true;
        current->add(store);
        current->setArg(arg);
        return true;
    }

    
    
    
    
    if (graph().numBlocks() == 1 &&
        (val->isBitOr() || val->isBitAnd() || val->isMul() ))
     {
         for (size_t i = 0; i < val->numOperands(); i++) {
            MDefinition* op = val->getOperand(i);
            if (op->isParameter() &&
                op->toParameter()->index() == (int32_t)arg &&
                op->resultTypeSet() &&
                op->resultTypeSet()->empty())
            {
                bool otherUses = false;
                for (MUseDefIterator iter(op); iter; iter++) {
                    MDefinition* def = iter.def();
                    if (def == val)
                        continue;
                    otherUses = true;
                }
                if (!otherUses) {
                    MOZ_ASSERT(op->resultTypeSet() == &argTypes[arg]);
                    argTypes[arg].addType(TypeSet::UnknownType(), alloc_->lifoAlloc());
                    if (val->isMul()) {
                        val->setResultType(MIRType_Double);
                        val->toMul()->setSpecialization(MIRType_Double);
                    } else {
                        MOZ_ASSERT(val->type() == MIRType_Int32);
                    }
                    val->setResultTypeSet(nullptr);
                }
            }
        }
    }

    current->setArg(arg);
    return true;
}

bool
IonBuilder::jsop_defvar(uint32_t index)
{
    MOZ_ASSERT(JSOp(*pc) == JSOP_DEFVAR || JSOp(*pc) == JSOP_DEFCONST);

    PropertyName* name = script()->getName(index);

    
    unsigned attrs = JSPROP_ENUMERATE;
    if (JSOp(*pc) == JSOP_DEFCONST)
        attrs |= JSPROP_READONLY;
    else
        attrs |= JSPROP_PERMANENT;
    MOZ_ASSERT(!script()->isForEval());

    
    MOZ_ASSERT(analysis().usesScopeChain());

    
    MDefVar* defvar = MDefVar::New(alloc(), name, attrs, current->scopeChain());
    current->add(defvar);

    return resumeAfter(defvar);
}

bool
IonBuilder::jsop_deffun(uint32_t index)
{
    JSFunction* fun = script()->getFunction(index);
    if (fun->isNative() && IsAsmJSModuleNative(fun->native()))
        return abort("asm.js module function");

    MOZ_ASSERT(analysis().usesScopeChain());

    MDefFun* deffun = MDefFun::New(alloc(), fun, current->scopeChain());
    current->add(deffun);

    return resumeAfter(deffun);
}

bool
IonBuilder::jsop_checklexical()
{
    uint32_t slot = info().localSlot(GET_LOCALNO(pc));
    MDefinition* lexical = addLexicalCheck(current->getSlot(slot));
    if (!lexical)
        return false;
    current->setSlot(slot, lexical);
    return true;
}

bool
IonBuilder::jsop_checkaliasedlet(ScopeCoordinate sc)
{
    MDefinition* let = addLexicalCheck(getAliasedVar(sc));
    if (!let)
        return false;

    jsbytecode* nextPc = pc + JSOP_CHECKALIASEDLEXICAL_LENGTH;
    MOZ_ASSERT(JSOp(*nextPc) == JSOP_GETALIASEDVAR || JSOp(*nextPc) == JSOP_SETALIASEDVAR);
    MOZ_ASSERT(sc == ScopeCoordinate(nextPc));

    
    
    if (JSOp(*nextPc) == JSOP_GETALIASEDVAR)
        setLexicalCheck(let);

    return true;
}

bool
IonBuilder::jsop_this()
{
    if (!info().funMaybeLazy())
        return abort("JSOP_THIS outside of a JSFunction.");

    if (info().funMaybeLazy()->isArrow()) {
        
        MLoadArrowThis* thisObj = MLoadArrowThis::New(alloc(), getCallee());
        current->add(thisObj);
        current->push(thisObj);
        return true;
    }

    if (script()->strict() || info().funMaybeLazy()->isSelfHostedBuiltin()) {
        
        current->pushSlot(info().thisSlot());
        return true;
    }

    if (thisTypes && (thisTypes->getKnownMIRType() == MIRType_Object ||
        (thisTypes->empty() && baselineFrame_ && baselineFrame_->thisType.isSomeObject())))
    {
        
        
        
        current->pushSlot(info().thisSlot());
        return true;
    }

    
    
    
    if (info().isAnalysis()) {
        current->pushSlot(info().thisSlot());
        return true;
    }

    
    MDefinition* def = current->getSlot(info().thisSlot());

    if (def->type() == MIRType_Object) {
        
        current->push(def);
        return true;
    }

    MComputeThis* thisObj = MComputeThis::New(alloc(), def);
    current->add(thisObj);
    current->push(thisObj);

    current->setSlot(info().thisSlot(), thisObj);

    return resumeAfter(thisObj);
}

bool
IonBuilder::jsop_typeof()
{
    MDefinition* input = current->pop();
    MTypeOf* ins = MTypeOf::New(alloc(), input, input->type());

    ins->cacheInputMaybeCallableOrEmulatesUndefined(constraints());

    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_toid()
{
    
    if (current->peek(-1)->type() == MIRType_Int32)
        return true;

    MDefinition* index = current->pop();
    MToId* ins = MToId::New(alloc(), current->peek(-1), index);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_iter(uint8_t flags)
{
    if (flags != JSITER_ENUMERATE)
        nonStringIteration_ = true;

    MDefinition* obj = current->pop();
    MInstruction* ins = MIteratorStart::New(alloc(), obj, flags);

    if (!iterators_.append(ins))
        return false;

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_itermore()
{
    MDefinition* iter = current->peek(-1);
    MInstruction* ins = MIteratorMore::New(alloc(), iter);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_isnoiter()
{
    MDefinition* def = current->peek(-1);
    MOZ_ASSERT(def->isIteratorMore());

    MInstruction* ins = MIsNoIter::New(alloc(), def);
    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_iterend()
{
    MDefinition* iter = current->pop();
    MInstruction* ins = MIteratorEnd::New(alloc(), iter);

    current->add(ins);

    return resumeAfter(ins);
}

MDefinition*
IonBuilder::walkScopeChain(unsigned hops)
{
    MDefinition* scope = current->getSlot(info().scopeChainSlot());

    for (unsigned i = 0; i < hops; i++) {
        MInstruction* ins = MEnclosingScope::New(alloc(), scope);
        current->add(ins);
        scope = ins;
    }

    return scope;
}

bool
IonBuilder::hasStaticScopeObject(ScopeCoordinate sc, JSObject** pcall)
{
    JSScript* outerScript = ScopeCoordinateFunctionScript(script(), pc);
    if (!outerScript || !outerScript->treatAsRunOnce())
        return false;

    TypeSet::ObjectKey* funKey =
        TypeSet::ObjectKey::get(outerScript->functionNonDelazifying());
    if (funKey->hasFlags(constraints(), OBJECT_FLAG_RUNONCE_INVALIDATED))
        return false;

    
    
    
    

    
    
    

    MDefinition* scope = current->getSlot(info().scopeChainSlot());
    scope->setImplicitlyUsedUnchecked();

    JSObject* environment = script()->functionNonDelazifying()->environment();
    while (environment && !environment->is<GlobalObject>()) {
        if (environment->is<CallObject>() &&
            !environment->as<CallObject>().isForEval() &&
            environment->as<CallObject>().callee().nonLazyScript() == outerScript)
        {
            MOZ_ASSERT(environment->isSingleton());
            *pcall = environment;
            return true;
        }
        environment = environment->enclosingScope();
    }

    
    
    
    

    if (script() == outerScript && baselineFrame_ && info().osrPc()) {
        JSObject* singletonScope = baselineFrame_->singletonScopeChain;
        if (singletonScope &&
            singletonScope->is<CallObject>() &&
            singletonScope->as<CallObject>().callee().nonLazyScript() == outerScript)
        {
            MOZ_ASSERT(singletonScope->isSingleton());
            *pcall = singletonScope;
            return true;
        }
    }

    return true;
}

MDefinition*
IonBuilder::getAliasedVar(ScopeCoordinate sc)
{
    MDefinition* obj = walkScopeChain(sc.hops());

    Shape* shape = ScopeCoordinateToStaticScopeShape(script(), pc);

    MInstruction* load;
    if (shape->numFixedSlots() <= sc.slot()) {
        MInstruction* slots = MSlots::New(alloc(), obj);
        current->add(slots);

        load = MLoadSlot::New(alloc(), slots, sc.slot() - shape->numFixedSlots());
    } else {
        load = MLoadFixedSlot::New(alloc(), obj, sc.slot());
    }

    current->add(load);
    return load;
}

bool
IonBuilder::jsop_getaliasedvar(ScopeCoordinate sc)
{
    JSObject* call = nullptr;
    if (hasStaticScopeObject(sc, &call) && call) {
        PropertyName* name = ScopeCoordinateName(scopeCoordinateNameCache, script(), pc);
        bool succeeded;
        if (!getStaticName(call, name, &succeeded, takeLexicalCheck()))
            return false;
        if (succeeded)
            return true;
    }

    
    MDefinition* load = takeLexicalCheck();
    if (!load)
        load = getAliasedVar(sc);
    current->push(load);

    TemporaryTypeSet* types = bytecodeTypes(pc);
    return pushTypeBarrier(load, types, BarrierKind::TypeSet);
}

bool
IonBuilder::jsop_setaliasedvar(ScopeCoordinate sc)
{
    JSObject* call = nullptr;
    if (hasStaticScopeObject(sc, &call)) {
        uint32_t depth = current->stackDepth() + 1;
        if (depth > current->nslots()) {
            if (!current->increaseSlots(depth - current->nslots()))
                return false;
        }
        MDefinition* value = current->pop();
        PropertyName* name = ScopeCoordinateName(scopeCoordinateNameCache, script(), pc);

        if (call) {
            
            
            pushConstant(ObjectValue(*call));
            current->push(value);
            return setStaticName(call, name);
        }

        
        
        MDefinition* obj = walkScopeChain(sc.hops());
        current->push(obj);
        current->push(value);
        return jsop_setprop(name);
    }

    MDefinition* rval = current->peek(-1);
    MDefinition* obj = walkScopeChain(sc.hops());

    Shape* shape = ScopeCoordinateToStaticScopeShape(script(), pc);

    if (NeedsPostBarrier(info(), rval))
        current->add(MPostWriteBarrier::New(alloc(), obj, rval));

    MInstruction* store;
    if (shape->numFixedSlots() <= sc.slot()) {
        MInstruction* slots = MSlots::New(alloc(), obj);
        current->add(slots);

        store = MStoreSlot::NewBarriered(alloc(), slots, sc.slot() - shape->numFixedSlots(), rval);
    } else {
        store = MStoreFixedSlot::NewBarriered(alloc(), obj, sc.slot(), rval);
    }

    current->add(store);
    return resumeAfter(store);
}

bool
IonBuilder::jsop_in()
{
    MDefinition* obj = current->peek(-1);
    MDefinition* id = current->peek(-2);

    if (ElementAccessIsDenseNative(constraints(), obj, id) &&
        !ElementAccessHasExtraIndexedProperty(constraints(), obj))
    {
        return jsop_in_dense();
    }

    current->pop();
    current->pop();
    MIn* ins = MIn::New(alloc(), id, obj);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_in_dense()
{
    MDefinition* obj = current->pop();
    MDefinition* id = current->pop();

    bool needsHoleCheck = !ElementAccessIsPacked(constraints(), obj);

    
    MInstruction* idInt32 = MToInt32::New(alloc(), id);
    current->add(idInt32);
    id = idInt32;

    
    MElements* elements = MElements::New(alloc(), obj);
    current->add(elements);

    MInitializedLength* initLength = MInitializedLength::New(alloc(), elements);
    current->add(initLength);

    
    if (!needsHoleCheck && !failedBoundsCheck_) {
        addBoundsCheck(idInt32, initLength);
        return pushConstant(BooleanValue(true));
    }

    
    MInArray* ins = MInArray::New(alloc(), elements, id, initLength, obj, needsHoleCheck);

    current->add(ins);
    current->push(ins);

    return true;
}

static bool
HasOnProtoChain(CompilerConstraintList* constraints, TypeSet::ObjectKey* key,
                JSObject* protoObject, bool* hasOnProto)
{
    MOZ_ASSERT(protoObject);

    while (true) {
        if (!key->hasStableClassAndProto(constraints) || !key->clasp()->isNative())
            return false;

        JSObject* proto = key->proto().toObjectOrNull();
        if (!proto) {
            *hasOnProto = false;
            return true;
        }

        if (proto == protoObject) {
            *hasOnProto = true;
            return true;
        }

        key = TypeSet::ObjectKey::get(proto);
    }

    MOZ_CRASH("Unreachable");
}

bool
IonBuilder::tryFoldInstanceOf(MDefinition* lhs, JSObject* protoObject)
{
    

    if (!lhs->mightBeType(MIRType_Object)) {
        
        lhs->setImplicitlyUsedUnchecked();
        pushConstant(BooleanValue(false));
        return true;
    }

    TemporaryTypeSet* lhsTypes = lhs->resultTypeSet();
    if (!lhsTypes || lhsTypes->unknownObject())
        return false;

    
    
    bool isFirst = true;
    bool knownIsInstance = false;

    for (unsigned i = 0; i < lhsTypes->getObjectCount(); i++) {
        TypeSet::ObjectKey* key = lhsTypes->getObject(i);
        if (!key)
            continue;

        bool isInstance;
        if (!HasOnProtoChain(constraints(), key, protoObject, &isInstance))
            return false;

        if (isFirst) {
            knownIsInstance = isInstance;
            isFirst = false;
        } else if (knownIsInstance != isInstance) {
            
            
            return false;
        }
    }

    if (knownIsInstance && lhsTypes->getKnownMIRType() != MIRType_Object) {
        
        
        
        MIsObject* isObject = MIsObject::New(alloc(), lhs);
        current->add(isObject);
        current->push(isObject);
        return true;
    }

    lhs->setImplicitlyUsedUnchecked();
    pushConstant(BooleanValue(knownIsInstance));
    return true;
}

bool
IonBuilder::jsop_instanceof()
{
    MDefinition* rhs = current->pop();
    MDefinition* obj = current->pop();

    
    
    do {
        TemporaryTypeSet* rhsTypes = rhs->resultTypeSet();
        JSObject* rhsObject = rhsTypes ? rhsTypes->maybeSingleton() : nullptr;
        if (!rhsObject || !rhsObject->is<JSFunction>() || rhsObject->isBoundFunction())
            break;

        TypeSet::ObjectKey* rhsKey = TypeSet::ObjectKey::get(rhsObject);
        if (rhsKey->unknownProperties())
            break;

        HeapTypeSetKey protoProperty =
            rhsKey->property(NameToId(names().prototype));
        JSObject* protoObject = protoProperty.singleton(constraints());
        if (!protoObject)
            break;

        rhs->setImplicitlyUsedUnchecked();

        if (tryFoldInstanceOf(obj, protoObject))
            return true;

        MInstanceOf* ins = MInstanceOf::New(alloc(), obj, protoObject);

        current->add(ins);
        current->push(ins);

        return resumeAfter(ins);
    } while (false);

    
    do {
        Shape* shape;
        uint32_t slot;
        JSObject* protoObject;
        if (!inspector->instanceOfData(pc, &shape, &slot, &protoObject))
            break;

        
        rhs = addShapeGuard(rhs, shape, Bailout_ShapeGuard);

        
        MOZ_ASSERT(shape->numFixedSlots() == 0, "Must be a dynamic slot");
        MSlots* slots = MSlots::New(alloc(), rhs);
        current->add(slots);
        MLoadSlot* prototype = MLoadSlot::New(alloc(), slots, slot);
        current->add(prototype);
        MConstant* protoConst = MConstant::NewConstraintlessObject(alloc(), protoObject);
        current->add(protoConst);
        MGuardObjectIdentity* guard = MGuardObjectIdentity::New(alloc(), prototype, protoConst,
                                                                 false);
        current->add(guard);

        if (tryFoldInstanceOf(obj, protoObject))
            return true;

        MInstanceOf* ins = MInstanceOf::New(alloc(), obj, protoObject);
        current->add(ins);
        current->push(ins);
        return resumeAfter(ins);
    } while (false);

    MCallInstanceOf* ins = MCallInstanceOf::New(alloc(), obj, rhs);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_debugger()
{
    MDebugger* debugger = MDebugger::New(alloc());
    current->add(debugger);

    
    
    
    return resumeAt(debugger, pc);
}

MInstruction*
IonBuilder::addConvertElementsToDoubles(MDefinition* elements)
{
    MInstruction* convert = MConvertElementsToDoubles::New(alloc(), elements);
    current->add(convert);
    return convert;
}

MDefinition*
IonBuilder::addMaybeCopyElementsForWrite(MDefinition* object)
{
    if (!ElementAccessMightBeCopyOnWrite(constraints(), object))
        return object;
    MInstruction* copy = MMaybeCopyElementsForWrite::New(alloc(), object);
    current->add(copy);
    return copy;
}

MInstruction*
IonBuilder::addBoundsCheck(MDefinition* index, MDefinition* length)
{
    MInstruction* check = MBoundsCheck::New(alloc(), index, length);
    current->add(check);

    
    if (failedBoundsCheck_)
        check->setNotMovable();

    return check;
}

MInstruction*
IonBuilder::addShapeGuard(MDefinition* obj, Shape* const shape, BailoutKind bailoutKind)
{
    MGuardShape* guard = MGuardShape::New(alloc(), obj, shape, bailoutKind);
    current->add(guard);

    
    if (failedShapeGuard_)
        guard->setNotMovable();

    return guard;
}

MInstruction*
IonBuilder::addGroupGuard(MDefinition* obj, ObjectGroup* group, BailoutKind bailoutKind)
{
    MGuardObjectGroup* guard = MGuardObjectGroup::New(alloc(), obj, group,
                                                       false, bailoutKind);
    current->add(guard);

    
    if (failedShapeGuard_)
        guard->setNotMovable();

    LifoAlloc* lifoAlloc = alloc().lifoAlloc();
    guard->setResultTypeSet(lifoAlloc->new_<TemporaryTypeSet>(lifoAlloc,
                                                            TypeSet::ObjectType(group)));

    return guard;
}

MInstruction*
IonBuilder::addUnboxedExpandoGuard(MDefinition* obj, bool hasExpando, BailoutKind bailoutKind)
{
    MGuardUnboxedExpando* guard = MGuardUnboxedExpando::New(alloc(), obj, hasExpando, bailoutKind);
    current->add(guard);

    
    if (failedShapeGuard_)
        guard->setNotMovable();

    return guard;
}

MInstruction*
IonBuilder::addGuardReceiverPolymorphic(MDefinition* obj,
                                        const BaselineInspector::ReceiverVector& receivers)
{
    if (receivers.length() == 1) {
        if (!receivers[0].group) {
            
            return addShapeGuard(obj, receivers[0].shape, Bailout_ShapeGuard);
        }

        if (!receivers[0].shape) {
            
            obj = addGroupGuard(obj, receivers[0].group, Bailout_ShapeGuard);
            return addUnboxedExpandoGuard(obj,  false, Bailout_ShapeGuard);
        }

        
        
    }

    MGuardReceiverPolymorphic* guard = MGuardReceiverPolymorphic::New(alloc(), obj);
    current->add(guard);

    if (failedShapeGuard_)
        guard->setNotMovable();

    for (size_t i = 0; i < receivers.length(); i++) {
        if (!guard->addReceiver(receivers[i]))
            return nullptr;
    }

    return guard;
}

TemporaryTypeSet*
IonBuilder::bytecodeTypes(jsbytecode* pc)
{
    return TypeScript::BytecodeTypes(script(), pc, bytecodeTypeMap, &typeArrayHint, typeArray);
}

TypedObjectPrediction
IonBuilder::typedObjectPrediction(MDefinition* typedObj)
{
    
    if (typedObj->isNewDerivedTypedObject()) {
        return typedObj->toNewDerivedTypedObject()->prediction();
    }

    TemporaryTypeSet* types = typedObj->resultTypeSet();
    return typedObjectPrediction(types);
}

TypedObjectPrediction
IonBuilder::typedObjectPrediction(TemporaryTypeSet* types)
{
    
    if (!types || types->getKnownMIRType() != MIRType_Object)
        return TypedObjectPrediction();

    
    if (types->unknownObject())
        return TypedObjectPrediction();

    TypedObjectPrediction out;
    for (uint32_t i = 0; i < types->getObjectCount(); i++) {
        ObjectGroup* group = types->getGroup(i);
        if (!group || !TypeSet::ObjectKey::get(group)->hasStableClassAndProto(constraints()))
            return TypedObjectPrediction();

        if (!IsTypedObjectClass(group->clasp()))
            return TypedObjectPrediction();

        out.addDescr(group->typeDescr());
    }

    return out;
}

MDefinition*
IonBuilder::loadTypedObjectType(MDefinition* typedObj)
{
    
    
    
    
    if (typedObj->isNewDerivedTypedObject())
        return typedObj->toNewDerivedTypedObject()->type();

    MInstruction* descr = MTypedObjectDescr::New(alloc(), typedObj);
    current->add(descr);

    return descr;
}








void
IonBuilder::loadTypedObjectData(MDefinition* typedObj,
                                MDefinition** owner,
                                LinearSum* ownerOffset)
{
    MOZ_ASSERT(typedObj->type() == MIRType_Object);

    
    
    
    
    
    if (typedObj->isNewDerivedTypedObject()) {
        MNewDerivedTypedObject* ins = typedObj->toNewDerivedTypedObject();

        SimpleLinearSum base = ExtractLinearSum(ins->offset());
        if (!ownerOffset->add(base))
            setForceAbort();

        *owner = ins->owner();
        return;
    }

    *owner = typedObj;
}






void
IonBuilder::loadTypedObjectElements(MDefinition* typedObj,
                                    const LinearSum& baseByteOffset,
                                    int32_t scale,
                                    MDefinition** ownerElements,
                                    MDefinition** ownerScaledOffset,
                                    int32_t* ownerByteAdjustment)
{
    MDefinition* owner;
    LinearSum ownerByteOffset(alloc());
    loadTypedObjectData(typedObj, &owner, &ownerByteOffset);

    if (!ownerByteOffset.add(baseByteOffset))
        setForceAbort();

    TemporaryTypeSet* ownerTypes = owner->resultTypeSet();
    const Class* clasp = ownerTypes ? ownerTypes->getKnownClass(constraints()) : nullptr;
    if (clasp && IsInlineTypedObjectClass(clasp)) {
        
        if (!ownerByteOffset.add(InlineTypedObject::offsetOfDataStart()))
            setForceAbort();
        *ownerElements = owner;
    } else {
        bool definitelyOutline = clasp && IsOutlineTypedObjectClass(clasp);
        *ownerElements = MTypedObjectElements::New(alloc(), owner, definitelyOutline);
        current->add((*ownerElements)->toInstruction());
    }

    
    *ownerByteAdjustment = ownerByteOffset.constant();
    int32_t negativeAdjustment;
    if (!SafeSub(0, *ownerByteAdjustment, &negativeAdjustment))
        setForceAbort();
    if (!ownerByteOffset.add(negativeAdjustment))
        setForceAbort();

    
    
    
    
    
    
    if (ownerByteOffset.divide(scale)) {
        *ownerScaledOffset = ConvertLinearSum(alloc(), current, ownerByteOffset);
    } else {
        MDefinition* unscaledOffset = ConvertLinearSum(alloc(), current, ownerByteOffset);
        *ownerScaledOffset = MDiv::NewAsmJS(alloc(), unscaledOffset, constantInt(scale),
                                            MIRType_Int32,  false);
        current->add((*ownerScaledOffset)->toInstruction());
    }
}





bool
IonBuilder::typedObjectHasField(MDefinition* typedObj,
                                PropertyName* name,
                                size_t* fieldOffset,
                                TypedObjectPrediction* fieldPrediction,
                                size_t* fieldIndex)
{
    TypedObjectPrediction objPrediction = typedObjectPrediction(typedObj);
    if (objPrediction.isUseless()) {
        trackOptimizationOutcome(TrackedOutcome::AccessNotTypedObject);
        return false;
    }

    
    if (objPrediction.kind() != type::Struct) {
        trackOptimizationOutcome(TrackedOutcome::NotStruct);
        return false;
    }

    
    if (!objPrediction.hasFieldNamed(NameToId(name), fieldOffset,
                                     fieldPrediction, fieldIndex))
    {
        trackOptimizationOutcome(TrackedOutcome::StructNoField);
        return false;
    }

    return true;
}

MDefinition*
IonBuilder::typeObjectForElementFromArrayStructType(MDefinition* typeObj)
{
    MInstruction* elemType = MLoadFixedSlot::New(alloc(), typeObj, JS_DESCR_SLOT_ARRAY_ELEM_TYPE);
    current->add(elemType);

    MInstruction* unboxElemType = MUnbox::New(alloc(), elemType, MIRType_Object, MUnbox::Infallible);
    current->add(unboxElemType);

    return unboxElemType;
}

MDefinition*
IonBuilder::typeObjectForFieldFromStructType(MDefinition* typeObj,
                                             size_t fieldIndex)
{
    

    MInstruction* fieldTypes = MLoadFixedSlot::New(alloc(), typeObj, JS_DESCR_SLOT_STRUCT_FIELD_TYPES);
    current->add(fieldTypes);

    MInstruction* unboxFieldTypes = MUnbox::New(alloc(), fieldTypes, MIRType_Object, MUnbox::Infallible);
    current->add(unboxFieldTypes);

    

    MInstruction* fieldTypesElements = MElements::New(alloc(), unboxFieldTypes);
    current->add(fieldTypesElements);

    MConstant* fieldIndexDef = constantInt(fieldIndex);

    MInstruction* fieldType = MLoadElement::New(alloc(), fieldTypesElements, fieldIndexDef, false, false);
    current->add(fieldType);

    MInstruction* unboxFieldType = MUnbox::New(alloc(), fieldType, MIRType_Object, MUnbox::Infallible);
    current->add(unboxFieldType);

    return unboxFieldType;
}

bool
IonBuilder::storeScalarTypedObjectValue(MDefinition* typedObj,
                                        const LinearSum& byteOffset,
                                        ScalarTypeDescr::Type type,
                                        MDefinition* value)
{
    
    MDefinition* elements;
    MDefinition* scaledOffset;
    int32_t adjustment;
    size_t alignment = ScalarTypeDescr::alignment(type);
    loadTypedObjectElements(typedObj, byteOffset, alignment, &elements, &scaledOffset, &adjustment);

    
    MDefinition* toWrite = value;
    if (type == Scalar::Uint8Clamped) {
        toWrite = MClampToUint8::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }

    MStoreUnboxedScalar* store =
        MStoreUnboxedScalar::New(alloc(), elements, scaledOffset, toWrite,
                                 type, DoesNotRequireMemoryBarrier, adjustment);
    current->add(store);

    return true;
}

bool
IonBuilder::storeReferenceTypedObjectValue(MDefinition* typedObj,
                                           const LinearSum& byteOffset,
                                           ReferenceTypeDescr::Type type,
                                           MDefinition* value,
                                           PropertyName* name)
{
    
    
    if (type != ReferenceTypeDescr::TYPE_STRING) {
        MOZ_ASSERT(type == ReferenceTypeDescr::TYPE_ANY ||
                   type == ReferenceTypeDescr::TYPE_OBJECT);
        MIRType implicitType =
            (type == ReferenceTypeDescr::TYPE_ANY) ? MIRType_Undefined : MIRType_Null;

        if (PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current, &typedObj, name, &value,
                                           true, implicitType))
        {
            trackOptimizationOutcome(TrackedOutcome::NeedsTypeBarrier);
            return false;
        }
    }

    
    MDefinition* elements;
    MDefinition* scaledOffset;
    int32_t adjustment;
    size_t alignment = ReferenceTypeDescr::alignment(type);
    loadTypedObjectElements(typedObj, byteOffset, alignment, &elements, &scaledOffset, &adjustment);

    MInstruction* store = nullptr;  
    switch (type) {
      case ReferenceTypeDescr::TYPE_ANY:
        if (NeedsPostBarrier(info(), value))
            current->add(MPostWriteBarrier::New(alloc(), typedObj, value));
        store = MStoreElement::New(alloc(), elements, scaledOffset, value, false, adjustment);
        store->toStoreElement()->setNeedsBarrier();
        break;
      case ReferenceTypeDescr::TYPE_OBJECT:
        
        
        
        
        store = MStoreUnboxedObjectOrNull::New(alloc(), elements, scaledOffset, value, typedObj, adjustment);
        break;
      case ReferenceTypeDescr::TYPE_STRING:
        
        
        store = MStoreUnboxedString::New(alloc(), elements, scaledOffset, value, adjustment);
        break;
    }

    current->add(store);
    return true;
}

MConstant*
IonBuilder::constant(const Value& v)
{
    
    
    
    MOZ_ASSERT(!v.isString() || v.toString()->isAtom(),
               "To handle non-atomized strings, you should use constantMaybeAtomize instead of constant.");
    if (v.isString() && MOZ_UNLIKELY(!v.toString()->isAtom())) {
        MConstant *cst = constantMaybeAtomize(v);
        if (!cst)
            js::CrashAtUnhandlableOOM("Use constantMaybeAtomize.");
        return cst;
    }

    MConstant* c = MConstant::New(alloc(), v, constraints());
    current->add(c);
    return c;
}

MConstant*
IonBuilder::constantInt(int32_t i)
{
    return constant(Int32Value(i));
}

MConstant*
IonBuilder::constantMaybeAtomize(const Value& v)
{
    if (!v.isString() || v.toString()->isAtom())
        return constant(v);

    JSContext* cx = GetJitContext()->cx;
    JSAtom* atom = js::AtomizeString(cx, v.toString());
    if (!atom)
        return nullptr;
    return constant(StringValue(atom));
}

MDefinition*
IonBuilder::getCallee()
{
    if (inliningDepth_ == 0) {
        MInstruction* callee = MCallee::New(alloc());
        current->add(callee);
        return callee;
    }

    return inlineCallInfo_->fun();
}

MDefinition*
IonBuilder::addLexicalCheck(MDefinition* input)
{
    MOZ_ASSERT(JSOp(*pc) == JSOP_CHECKLEXICAL || JSOp(*pc) == JSOP_CHECKALIASEDLEXICAL);

    MInstruction* lexicalCheck;

    
    if (input->type() == MIRType_MagicUninitializedLexical) {
        lexicalCheck = MThrowUninitializedLexical::New(alloc());
        current->add(lexicalCheck);
        if (!resumeAfter(lexicalCheck))
            return nullptr;
        return constant(UndefinedValue());
    }

    if (input->type() == MIRType_Value) {
        lexicalCheck = MLexicalCheck::New(alloc(), input);
        current->add(lexicalCheck);
        if (failedLexicalCheck_)
            lexicalCheck->setNotMovableUnchecked();
        return lexicalCheck;
    }

    return input;
}
