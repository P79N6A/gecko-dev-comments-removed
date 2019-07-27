





#include "jit/IonBuilder.h"

#include "mozilla/DebugOnly.h"

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

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsopcodeinlines.h"
#include "jsscriptinlines.h"

#include "jit/CompileInfo-inl.h"
#include "jit/ExecutionMode-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::AssertedCast;
using mozilla::DebugOnly;
using mozilla::Maybe;

class jit::BaselineFrameInspector
{
  public:
    types::Type thisType;
    JSObject *singletonScopeChain;

    Vector<types::Type, 4, IonAllocPolicy> argTypes;
    Vector<types::Type, 4, IonAllocPolicy> varTypes;

    explicit BaselineFrameInspector(TempAllocator *temp)
      : thisType(types::Type::UndefinedType()),
        singletonScopeChain(nullptr),
        argTypes(*temp),
        varTypes(*temp)
    {}
};

BaselineFrameInspector *
jit::NewBaselineFrameInspector(TempAllocator *temp, BaselineFrame *frame, CompileInfo *info)
{
    JS_ASSERT(frame);

    BaselineFrameInspector *inspector = temp->lifoAlloc()->new_<BaselineFrameInspector>(temp);
    if (!inspector)
        return nullptr;

    
    
    

    inspector->thisType = types::GetMaybeOptimizedOutValueType(frame->thisValue());

    if (frame->scopeChain()->hasSingletonType())
        inspector->singletonScopeChain = frame->scopeChain();

    JSScript *script = frame->script();

    if (script->functionNonDelazifying()) {
        if (!inspector->argTypes.reserve(frame->numFormalArgs()))
            return nullptr;
        for (size_t i = 0; i < frame->numFormalArgs(); i++) {
            if (script->formalIsAliased(i)) {
                inspector->argTypes.infallibleAppend(types::Type::UndefinedType());
            } else if (!script->argsObjAliasesFormals()) {
                types::Type type = types::GetMaybeOptimizedOutValueType(frame->unaliasedFormal(i));
                inspector->argTypes.infallibleAppend(type);
            } else if (frame->hasArgsObj()) {
                types::Type type = types::GetMaybeOptimizedOutValueType(frame->argsObj().arg(i));
                inspector->argTypes.infallibleAppend(type);
            } else {
                inspector->argTypes.infallibleAppend(types::Type::UndefinedType());
            }
        }
    }

    if (!inspector->varTypes.reserve(frame->script()->nfixed()))
        return nullptr;
    for (size_t i = 0; i < frame->script()->nfixed(); i++) {
        if (info->isSlotAliasedAtOsr(i + info->firstLocalSlot())) {
            inspector->varTypes.infallibleAppend(types::Type::UndefinedType());
        } else {
            types::Type type = types::GetMaybeOptimizedOutValueType(frame->unaliasedLocal(i));
            inspector->varTypes.infallibleAppend(type);
        }
    }

    return inspector;
}

IonBuilder::IonBuilder(JSContext *analysisContext, CompileCompartment *comp,
                       const JitCompileOptions &options, TempAllocator *temp,
                       MIRGraph *graph, types::CompilerConstraintList *constraints,
                       BaselineInspector *inspector, CompileInfo *info,
                       const OptimizationInfo *optimizationInfo,
                       BaselineFrameInspector *baselineFrame, size_t inliningDepth,
                       uint32_t loopDepth)
  : MIRGenerator(comp, options, temp, graph, info, optimizationInfo),
    backgroundCodegen_(nullptr),
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
    numLoopRestarts_(0),
    failedBoundsCheck_(info->script()->failedBoundsCheck()),
    failedShapeGuard_(info->script()->failedShapeGuard()),
    nonStringIteration_(false),
    lazyArguments_(nullptr),
    inlineCallInfo_(nullptr)
{
    script_ = info->script();
    pc = info->startPC();
    abortReason_ = AbortReason_Disable;

    JS_ASSERT(script()->hasBaselineScript() == (info->executionMode() != ArgumentsUsageAnalysis));
    JS_ASSERT(!!analysisContext == (info->executionMode() == DefinitePropertiesAnalysis));

    if (!info->executionModeIsAnalysis())
        script()->baselineScript()->setIonCompiledOrInlined();
}

void
IonBuilder::clearForBackEnd()
{
    JS_ASSERT(!analysisContext);
    baselineFrame_ = nullptr;

    
    
    
    
    gsn.purge();
    scopeCoordinateNameCache.purge();
}

bool
IonBuilder::abort(const char *message, ...)
{
    
#ifdef DEBUG
    va_list ap;
    va_start(ap, message);
    abortFmt(message, ap);
    va_end(ap);
    JitSpew(JitSpew_Abort, "aborted @ %s:%d", script()->filename(), PCToLineNumber(script(), pc));
#endif
    return false;
}

void
IonBuilder::spew(const char *message)
{
    
#ifdef DEBUG
    JitSpew(JitSpew_MIR, "%s @ %s:%d", message, script()->filename(), PCToLineNumber(script(), pc));
#endif
}

static inline int32_t
GetJumpOffset(jsbytecode *pc)
{
    JS_ASSERT(js_CodeSpec[JSOp(*pc)].type() == JOF_JUMP);
    return GET_JUMP_OFFSET(pc);
}

IonBuilder::CFGState
IonBuilder::CFGState::If(jsbytecode *join, MTest *test)
{
    CFGState state;
    state.state = IF_TRUE;
    state.stopAt = join;
    state.branch.ifFalse = test->ifFalse();
    state.branch.test = test;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::IfElse(jsbytecode *trueEnd, jsbytecode *falseEnd, MTest *test)
{
    MBasicBlock *ifFalse = test->ifFalse();

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
IonBuilder::CFGState::AndOr(jsbytecode *join, MBasicBlock *joinStart)
{
    CFGState state;
    state.state = AND_OR;
    state.stopAt = join;
    state.branch.ifFalse = joinStart;
    state.branch.test = nullptr;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::TableSwitch(jsbytecode *exitpc, MTableSwitch *ins)
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

JSFunction *
IonBuilder::getSingleCallTarget(types::TemporaryTypeSet *calleeTypes)
{
    if (!calleeTypes)
        return nullptr;

    JSObject *obj = calleeTypes->getSingleton();
    if (!obj || !obj->is<JSFunction>())
        return nullptr;

    return &obj->as<JSFunction>();
}

bool
IonBuilder::getPolyCallTargets(types::TemporaryTypeSet *calleeTypes, bool constructing,
                               ObjectVector &targets, uint32_t maxTargets, bool *gotLambda)
{
    JS_ASSERT(targets.empty());
    JS_ASSERT(gotLambda);
    *gotLambda = false;

    if (!calleeTypes)
        return true;

    if (calleeTypes->baseFlags() != 0)
        return true;

    unsigned objCount = calleeTypes->getObjectCount();

    if (objCount == 0 || objCount > maxTargets)
        return true;

    if (!targets.reserve(objCount))
        return false;
    for(unsigned i = 0; i < objCount; i++) {
        JSObject *obj = calleeTypes->getSingleObject(i);
        JSFunction *fun;
        if (obj) {
            if (!obj->is<JSFunction>()) {
                targets.clear();
                return true;
            }
            fun = &obj->as<JSFunction>();
        } else {
            types::TypeObject *typeObj = calleeTypes->getTypeObject(i);
            JS_ASSERT(typeObj);
            if (!typeObj->interpretedFunction) {
                targets.clear();
                return true;
            }

            fun = typeObj->interpretedFunction;
            *gotLambda = true;
        }

        
        
        
        if (constructing && !fun->isInterpretedConstructor() && !fun->isNativeConstructor()) {
            targets.clear();
            return true;
        }

        DebugOnly<bool> appendOk = targets.append(fun);
        JS_ASSERT(appendOk);
    }

    
    if (*gotLambda && targets.length() > 1)
        targets.clear();

    return true;
}

IonBuilder::InliningDecision
IonBuilder::DontInline(JSScript *targetScript, const char *reason)
{
    if (targetScript) {
        JitSpew(JitSpew_Inlining, "Cannot inline %s:%u: %s",
                targetScript->filename(), targetScript->lineno(), reason);
    } else {
        JitSpew(JitSpew_Inlining, "Cannot inline: %s", reason);
    }

    return InliningDecision_DontInline;
}

IonBuilder::InliningDecision
IonBuilder::canInlineTarget(JSFunction *target, CallInfo &callInfo)
{
    if (!optimizationInfo().inlineInterpreted())
        return InliningDecision_DontInline;

    if (!target->isInterpreted())
        return DontInline(nullptr, "Non-interpreted target");

    
    
    if (target->isInterpreted() && info().executionMode() == DefinitePropertiesAnalysis) {
        RootedScript script(analysisContext, target->getOrCreateScript(analysisContext));
        if (!script)
            return InliningDecision_Error;

        if (!script->hasBaselineScript() && script->canBaselineCompile()) {
            MethodStatus status = BaselineCompile(analysisContext, script);
            if (status == Method_Error)
                return InliningDecision_Error;
            if (status != Method_Compiled)
                return InliningDecision_DontInline;
        }
    }

    if (!target->hasScript())
        return DontInline(nullptr, "Lazy script");

    JSScript *inlineScript = target->nonLazyScript();
    if (callInfo.constructing() && !target->isInterpretedConstructor())
        return DontInline(inlineScript, "Callee is not a constructor");

    ExecutionMode executionMode = info().executionMode();
    if (!CanIonCompile(inlineScript, executionMode))
        return DontInline(inlineScript, "Disabled Ion compilation");

    
    if (!inlineScript->hasBaselineScript())
        return DontInline(inlineScript, "No baseline jitcode");

    if (TooManyFormalArguments(target->nargs()))
        return DontInline(inlineScript, "Too many args");

    
    
    
    if (TooManyFormalArguments(callInfo.argc()))
        return DontInline(inlineScript, "Too many actual args");

    
    IonBuilder *builder = callerBuilder_;
    while (builder) {
        if (builder->script() == inlineScript)
            return DontInline(inlineScript, "Recursive call");
        builder = builder->callerBuilder_;
    }

    if (target->isHeavyweight())
        return DontInline(inlineScript, "Heavyweight function");

    if (inlineScript->uninlineable())
        return DontInline(inlineScript, "Uninlineable script");

    if (inlineScript->needsArgsObj())
        return DontInline(inlineScript, "Script that needs an arguments object");

    types::TypeObjectKey *targetType = types::TypeObjectKey::get(target);
    if (targetType->unknownProperties())
        return DontInline(inlineScript, "Target type has unknown properties");

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
IonBuilder::analyzeNewLoopTypes(MBasicBlock *entry, jsbytecode *start, jsbytecode *end)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    for (size_t i = 0; i < loopHeaders_.length(); i++) {
        if (loopHeaders_[i].pc == start) {
            MBasicBlock *oldEntry = loopHeaders_[i].header;

            
            
            if (!oldEntry->isDead()) {
                MResumePoint *oldEntryRp = oldEntry->entryResumePoint();
                size_t stackDepth = oldEntryRp->numOperands();
                for (size_t slot = 0; slot < stackDepth; slot++) {
                    MDefinition *oldDef = oldEntryRp->getOperand(slot);
                    if (!oldDef->isPhi()) {
                        MOZ_ASSERT(oldDef->block()->id() < oldEntry->id());
                        MOZ_ASSERT(oldDef == entry->getSlot(slot));
                        continue;
                    }
                    MPhi *oldPhi = oldDef->toPhi();
                    MPhi *newPhi = entry->getSlot(slot)->toPhi();
                    if (!newPhi->addBackedgeType(oldPhi->type(), oldPhi->resultTypeSet()))
                        return false;
                }
            }

            
            
            
            loopHeaders_[i].header = entry;
            return true;
        }
    }
    loopHeaders_.append(LoopHeader(start, entry));

    jsbytecode *last = nullptr, *earlier = nullptr;
    for (jsbytecode *pc = start; pc != end; earlier = last, last = pc, pc += GetBytecodeLength(pc)) {
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

        MPhi *phi = entry->getSlot(slot)->toPhi();

        if (*last == JSOP_POS)
            last = earlier;

        if (js_CodeSpec[*last].format & JOF_TYPESET) {
            types::TemporaryTypeSet *typeSet = bytecodeTypes(last);
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
                MPhi *otherPhi = entry->getSlot(slot)->toPhi();
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
              case JSOP_ITERNEXT:
                type = MIRType_String;
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
IonBuilder::pushLoop(CFGState::State initial, jsbytecode *stopAt, MBasicBlock *entry, bool osr,
                     jsbytecode *loopHead, jsbytecode *initialPc,
                     jsbytecode *bodyStart, jsbytecode *bodyEnd, jsbytecode *exitpc,
                     jsbytecode *continuepc)
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
    if (!types::TypeScript::FreezeTypeSets(constraints(), script(),
                                           &thisTypes, &argTypes, &typeArray))
    {
        return false;
    }

    if (!analysis().init(alloc(), gsn))
        return false;

    
    
    if (script()->hasBaselineScript()) {
        bytecodeTypeMap = script()->baselineScript()->bytecodeTypeMap();
    } else {
        bytecodeTypeMap = alloc_->lifoAlloc()->newArrayUninitialized<uint32_t>(script()->nTypeSets());
        if (!bytecodeTypeMap)
            return false;
        types::FillBytecodeTypeMap(script(), bytecodeTypeMap);
    }

    return true;
}

bool
IonBuilder::build()
{
    if (!init())
        return false;

    if (!setCurrentAndSpecializePhis(newBlock(pc)))
        return false;
    if (!current)
        return false;

#ifdef DEBUG
    if (info().executionModeIsAnalysis()) {
        JitSpew(JitSpew_Scripts, "Analyzing script %s:%d (%p) %s",
                script()->filename(), script()->lineno(), (void *)script(),
                ExecutionModeString(info().executionMode()));
    } else if (info().executionMode() == SequentialExecution && script()->hasIonScript()) {
        JitSpew(JitSpew_Scripts, "Recompiling script %s:%d (%p) (warmup-counter=%d, level=%s)",
                script()->filename(), script()->lineno(), (void *)script(),
                (int)script()->getWarmUpCounter(), OptimizationLevelString(optimizationInfo().level()));
    } else {
        JitSpew(JitSpew_Scripts, "Compiling script %s:%d (%p) (warmup-counter=%d, level=%s)",
                script()->filename(), script()->lineno(), (void *)script(),
                (int)script()->getWarmUpCounter(), OptimizationLevelString(optimizationInfo().level()));
    }
#endif

    initParameters();

    
    for (uint32_t i = 0; i < info().nlocals(); i++) {
        MConstant *undef = MConstant::New(alloc(), UndefinedValue());
        current->add(undef);
        current->initSlot(info().localSlot(i), undef);
    }

    
    
    
    
    MInstruction *scope = MConstant::New(alloc(), UndefinedValue());
    current->add(scope);
    current->initSlot(info().scopeChainSlot(), scope);

    
    MInstruction *returnValue = MConstant::New(alloc(), UndefinedValue());
    current->add(returnValue);
    current->initSlot(info().returnValueSlot(), returnValue);

    
    if (info().hasArguments()) {
        MInstruction *argsObj = MConstant::New(alloc(), UndefinedValue());
        current->add(argsObj);
        current->initSlot(info().argsObjSlot(), argsObj);
    }

    
    current->add(MStart::New(alloc(), MStart::StartType_Default));
    if (instrumentedProfiling())
        current->add(MProfilerStackOp::New(alloc(), script(), MProfilerStackOp::Enter));

    
    
    
    
    MCheckOverRecursed *check = MCheckOverRecursed::New(alloc());
    current->add(check);
    check->setResumePoint(MResumePoint::Copy(alloc(), current->entryResumePoint()));

    
    
    rewriteParameters();

    
    if (!initScopeChain())
        return false;

    if (info().needsArgsObj() && !initArgumentsObject())
        return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (uint32_t i = 0; i < info().endArgSlot(); i++) {
        MInstruction *ins = current->getEntrySlot(i)->toInstruction();
        if (ins->type() == MIRType_Value)
            ins->setResumePoint(MResumePoint::Copy(alloc(), current->entryResumePoint()));
    }

    
    if (info().hasArguments() && !info().argsObjAliasesFormals()) {
        lazyArguments_ = MConstant::New(alloc(), MagicValue(JS_OPTIMIZED_ARGUMENTS));
        current->add(lazyArguments_);
    }

    insertRecompileCheck();

    if (!traverseBytecode())
        return false;

    if (!maybeAddOsrTypeBarriers())
        return false;

    if (!processIterators())
        return false;

    if (!abortedNewScriptPropertiesTypes().empty()) {
        JS_ASSERT(!info().executionModeIsAnalysis());
        abortReason_ = AbortReason_NewScriptProperties;
        return false;
    }

    JS_ASSERT(loopDepth_ == 0);
    abortReason_ = AbortReason_NoAbort;
    return true;
}

bool
IonBuilder::processIterators()
{
    
    Vector<MPhi *, 0, SystemAllocPolicy> worklist;
    for (size_t i = 0; i < iterators_.length(); i++) {
        MInstruction *ins = iterators_[i];
        for (MUseDefIterator iter(ins); iter; iter++) {
            if (iter.def()->isPhi()) {
                if (!worklist.append(iter.def()->toPhi()))
                    return false;
            }
        }
    }

    
    
    while (!worklist.empty()) {
        MPhi *phi = worklist.popCopy();
        phi->setIterator();
        phi->setImplicitlyUsedUnchecked();

        for (MUseDefIterator iter(phi); iter; iter++) {
            if (iter.def()->isPhi()) {
                MPhi *other = iter.def()->toPhi();
                if (!other->isIterator() && !worklist.append(other))
                    return false;
            }
        }
    }

    return true;
}

bool
IonBuilder::buildInline(IonBuilder *callerBuilder, MResumePoint *callerResumePoint,
                        CallInfo &callInfo)
{
    if (!init())
        return false;

    inlineCallInfo_ = &callInfo;

    JitSpew(JitSpew_Scripts, "Inlining script %s:%d (%p)",
            script()->filename(), script()->lineno(), (void *)script());

    callerBuilder_ = callerBuilder;
    callerResumePoint_ = callerResumePoint;

    if (callerBuilder->failedBoundsCheck_)
        failedBoundsCheck_ = true;

    if (callerBuilder->failedShapeGuard_)
        failedShapeGuard_ = true;

    
    if (!setCurrentAndSpecializePhis(newBlock(pc)))
        return false;
    if (!current)
        return false;

    current->setCallerResumePoint(callerResumePoint);

    
    MBasicBlock *predecessor = callerBuilder->current;
    JS_ASSERT(predecessor == callerResumePoint->block());

    predecessor->end(MGoto::New(alloc(), current));
    if (!current->addPredecessorWithoutPhis(predecessor))
        return false;

    
    MInstruction *scope = MConstant::New(alloc(), UndefinedValue());
    current->add(scope);
    current->initSlot(info().scopeChainSlot(), scope);

    
    MInstruction *returnValue = MConstant::New(alloc(), UndefinedValue());
    current->add(returnValue);
    current->initSlot(info().returnValueSlot(), returnValue);

    
    if (info().hasArguments()) {
        MInstruction *argsObj = MConstant::New(alloc(), UndefinedValue());
        current->add(argsObj);
        current->initSlot(info().argsObjSlot(), argsObj);
    }

    
    current->initSlot(info().thisSlot(), callInfo.thisArg());

    JitSpew(JitSpew_Inlining, "Initializing %u arg slots", info().nargs());

    
    
    JS_ASSERT(!info().needsArgsObj());

    
    uint32_t existing_args = Min<uint32_t>(callInfo.argc(), info().nargs());
    for (size_t i = 0; i < existing_args; ++i) {
        MDefinition *arg = callInfo.getArg(i);
        current->initSlot(info().argSlot(i), arg);
    }

    
    for (size_t i = callInfo.argc(); i < info().nargs(); ++i) {
        MConstant *arg = MConstant::New(alloc(), UndefinedValue());
        current->add(arg);
        current->initSlot(info().argSlot(i), arg);
    }

    
    if (!initScopeChain(callInfo.fun()))
        return false;

    JitSpew(JitSpew_Inlining, "Initializing %u local slots", info().nlocals());

    
    for (uint32_t i = 0; i < info().nlocals(); i++) {
        MConstant *undef = MConstant::New(alloc(), UndefinedValue());
        current->add(undef);
        current->initSlot(info().localSlot(i), undef);
    }

    JitSpew(JitSpew_Inlining, "Inline entry block MResumePoint %p, %u operands",
            (void *) current->entryResumePoint(), current->entryResumePoint()->numOperands());

    
    JS_ASSERT(current->entryResumePoint()->numOperands() == info().totalSlots());

    if (script_->argumentsHasVarBinding()) {
        lazyArguments_ = MConstant::New(alloc(), MagicValue(JS_OPTIMIZED_ARGUMENTS));
        current->add(lazyArguments_);
    }

    insertRecompileCheck();

    if (!traverseBytecode())
        return false;

    return true;
}

void
IonBuilder::rewriteParameter(uint32_t slotIdx, MDefinition *param, int32_t argIndex)
{
    JS_ASSERT(param->isParameter() || param->isGetArgumentsObjectArg());

    types::TemporaryTypeSet *types = param->resultTypeSet();
    MDefinition *actual = ensureDefiniteType(param, types->getKnownMIRType());
    if (actual == param)
        return;

    
    
    
    
    
    
    
    
    
    current->rewriteSlot(slotIdx, actual);
}




void
IonBuilder::rewriteParameters()
{
    JS_ASSERT(info().scopeChainSlot() == 0);

    if (!info().funMaybeLazy())
        return;

    for (uint32_t i = info().startArgSlot(); i < info().endArgSlot(); i++) {
        MDefinition *param = current->getSlot(i);
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

    MParameter *param = MParameter::New(alloc(), MParameter::THIS_SLOT, thisTypes);
    current->add(param);
    current->initSlot(info().thisSlot(), param);

    for (uint32_t i = 0; i < info().nargs(); i++) {
        types::TemporaryTypeSet *types = &argTypes[i];
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

bool
IonBuilder::initScopeChain(MDefinition *callee)
{
    MInstruction *scope = nullptr;

    
    
    
    
    if (!info().needsArgsObj() && !analysis().usesScopeChain())
        return true;

    
    
    
    

    if (JSFunction *fun = info().funMaybeLazy()) {
        if (!callee) {
            MCallee *calleeIns = MCallee::New(alloc());
            current->add(calleeIns);
            callee = calleeIns;
        }
        scope = MFunctionEnvironment::New(alloc(), callee);
        current->add(scope);

        
        
        
        if (fun->isHeavyweight() && !info().executionModeIsAnalysis()) {
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
        
        MOZ_ASSERT(script()->compileAndGo());
        scope = constant(ObjectValue(script()->global()));
    }

    current->setScopeChain(scope);
    return true;
}

bool
IonBuilder::initArgumentsObject()
{
    JitSpew(JitSpew_MIR, "%s:%d - Emitting code to initialize arguments object! block=%p",
                              script()->filename(), script()->lineno(), current);
    JS_ASSERT(info().needsArgsObj());
    MCreateArgumentsObject *argsObj = MCreateArgumentsObject::New(alloc(), current->scopeChain());
    current->add(argsObj);
    current->setArgumentsObject(argsObj);
    return true;
}

bool
IonBuilder::addOsrValueTypeBarrier(uint32_t slot, MInstruction **def_,
                                   MIRType type, types::TemporaryTypeSet *typeSet)
{
    MInstruction *&def = *def_;
    MBasicBlock *osrBlock = def->block();

    
    def->setResultType(MIRType_Value);
    def->setResultTypeSet(nullptr);

    if (typeSet && !typeSet->unknown()) {
        MInstruction *barrier = MTypeBarrier::New(alloc(), def, typeSet);
        osrBlock->insertBefore(osrBlock->lastIns(), barrier);
        osrBlock->rewriteSlot(slot, barrier);
        def = barrier;
    } else if (type == MIRType_Null ||
               type == MIRType_Undefined ||
               type == MIRType_MagicOptimizedArguments)
    {
        
        
        types::Type ntype = types::Type::PrimitiveType(ValueTypeFromMIRType(type));
        LifoAlloc *lifoAlloc = alloc().lifoAlloc();
        typeSet = lifoAlloc->new_<types::TemporaryTypeSet>(lifoAlloc, ntype);
        if (!typeSet)
            return false;
        MInstruction *barrier = MTypeBarrier::New(alloc(), def, typeSet);
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
            MUnbox *unbox = MUnbox::New(alloc(), def, type, MUnbox::Fallible);
            osrBlock->insertBefore(osrBlock->lastIns(), unbox);
            osrBlock->rewriteSlot(slot, unbox);
            def = unbox;
        }
        break;

      case MIRType_Null:
      {
        MConstant *c = MConstant::New(alloc(), NullValue());
        osrBlock->insertBefore(osrBlock->lastIns(), c);
        osrBlock->rewriteSlot(slot, c);
        def = c;
        break;
      }

      case MIRType_Undefined:
      {
        MConstant *c = MConstant::New(alloc(), UndefinedValue());
        osrBlock->insertBefore(osrBlock->lastIns(), c);
        osrBlock->rewriteSlot(slot, c);
        def = c;
        break;
      }

      case MIRType_MagicOptimizedArguments:
        JS_ASSERT(lazyArguments_);
        osrBlock->rewriteSlot(slot, lazyArguments_);
        def = lazyArguments_;
        break;

      default:
        break;
    }

    JS_ASSERT(def == osrBlock->getSlot(slot));
    return true;
}

bool
IonBuilder::maybeAddOsrTypeBarriers()
{
    if (!info().osrPc())
        return true;

    
    
    
    

    MBasicBlock *osrBlock = graph().osrBlock();
    if (!osrBlock) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        JS_ASSERT(graph().hasTryBlock());
        return abort("OSR block only reachable through catch block");
    }

    MBasicBlock *preheader = osrBlock->getSuccessor(0);
    MBasicBlock *header = preheader->getSuccessor(0);
    static const size_t OSR_PHI_POSITION = 1;
    JS_ASSERT(preheader->getPredecessor(OSR_PHI_POSITION) == osrBlock);

    MResumePoint *headerRp = header->entryResumePoint();
    size_t stackDepth = headerRp->numOperands();
    MOZ_ASSERT(stackDepth == osrBlock->stackDepth());
    for (uint32_t slot = info().startArgSlot(); slot < stackDepth; slot++) {
        
        
        
        if (info().isSlotAliasedAtOsr(slot))
            continue;

        MInstruction *def = osrBlock->getSlot(slot)->toInstruction();
        MPhi *preheaderPhi = preheader->getSlot(slot)->toPhi();
        MPhi *headerPhi = headerRp->getOperand(slot)->toPhi();

        MIRType type = headerPhi->type();
        types::TemporaryTypeSet *typeSet = headerPhi->resultTypeSet();

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
        JS_ASSERT(pc < info().limitPC());

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
        
        
        
        
        
        
        
        
        
        Vector<MDefinition *, 4, IonAllocPolicy> popped(alloc());
        Vector<size_t, 4, IonAllocPolicy> poppedUses(alloc());
        unsigned nuses = GetWarmUpCounter(script_, script_->pcToOffset(pc));

        for (unsigned i = 0; i < nuses; i++) {
            MDefinition *def = current->peek(-int32_t(i + 1));
            if (!popped.append(def) || !poppedUses.append(def->defWarmUpCounter()))
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
              case JSOP_SETRVAL:
              case JSOP_VOID:
                
                break;

              case JSOP_POS:
              case JSOP_TOID:
                
                
                
                
                JS_ASSERT(i == 0);
                if (current->peek(-1) == popped[0])
                    break;
                

              default:
                JS_ASSERT(popped[i]->isImplicitlyUsed() ||

                          
                          
                          
                          
                          popped[i]->isNewDerivedTypedObject() ||

                          popped[i]->defWarmUpCounter() > poppedUses[i]);
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
        jssrcnote *sn = info().getNote(gsn, pc);
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
            MGetArgumentsObjectArg *getArg = MGetArgumentsObjectArg::New(alloc(),
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
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        return jsop_initprop(name);
      }

      case JSOP_MUTATEPROTO:
      {
        return jsop_mutateproto();
      }

      case JSOP_INITPROP_GETTER:
      case JSOP_INITPROP_SETTER: {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        return jsop_initprop_getter_setter(name);
      }

      case JSOP_INITELEM_GETTER:
      case JSOP_INITELEM_SETTER:
        return jsop_initelem_getter_setter();

      case JSOP_ENDINIT:
        return true;

      case JSOP_FUNCALL:
        return jsop_funcall(GET_ARGC(pc));

      case JSOP_FUNAPPLY:
        return jsop_funapply(GET_ARGC(pc));

      case JSOP_CALL:
      case JSOP_NEW:
        return jsop_call(GET_ARGC(pc), (JSOp)*pc == JSOP_NEW);

      case JSOP_EVAL:
        return jsop_eval(GET_ARGC(pc));

      case JSOP_INT8:
        return pushConstant(Int32Value(GET_INT8(pc)));

      case JSOP_UINT16:
        return pushConstant(Int32Value(GET_UINT16(pc)));

      case JSOP_GETGNAME:
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        return jsop_getgname(name);
      }

      case JSOP_BINDGNAME:
        return pushConstant(ObjectValue(script()->global()));

      case JSOP_SETGNAME:
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        JSObject *obj = &script()->global();
        return setStaticName(obj, name);
      }

      case JSOP_NAME:
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        return jsop_getname(name);
      }

      case JSOP_GETINTRINSIC:
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        return jsop_intrinsic(name);
      }

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
        return jsop_getelem();

      case JSOP_SETELEM:
        return jsop_setelem();

      case JSOP_LENGTH:
        return jsop_length();

      case JSOP_NOT:
        return jsop_not();

      case JSOP_THIS:
        return jsop_this();

      case JSOP_CALLEE: {
         MDefinition *callee = getCallee();
         current->push(callee);
         return true;
      }

      case JSOP_GETPROP:
      case JSOP_CALLPROP:
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        return jsop_getprop(name);
      }

      case JSOP_SETPROP:
      case JSOP_SETNAME:
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        return jsop_setprop(name);
      }

      case JSOP_DELPROP:
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        return jsop_delprop(name);
      }

      case JSOP_DELELEM:
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

      case JSOP_ITERNEXT:
        return jsop_iternext();

      case JSOP_MOREITER:
        return jsop_itermore();

      case JSOP_ENDITER:
        return jsop_iterend();

      case JSOP_IN:
        return jsop_in();

      case JSOP_SETRVAL:
        JS_ASSERT(!script()->noScriptRval());
        current->setSlot(info().returnValueSlot(), current->pop());
        return true;

      case JSOP_INSTANCEOF:
        return jsop_instanceof();

      case JSOP_DEBUGLEAVEBLOCK:
        return true;

      default:
#ifdef DEBUG
        return abort("Unsupported opcode: %s (line %d)", js_CodeName[op], info().lineno(pc));
#else
        return abort("Unsupported opcode: %d (line %d)", op, info().lineno(pc));
#endif
    }
}














IonBuilder::ControlStatus
IonBuilder::processControlEnd()
{
    JS_ASSERT(!current);

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
IonBuilder::processCfgEntry(CFGState &state)
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
IonBuilder::processIfEnd(CFGState &state)
{
    if (current) {
        
        
        
        current->end(MGoto::New(alloc(), state.branch.ifFalse));

        if (!state.branch.ifFalse->addPredecessor(alloc(), current))
            return ControlStatus_Error;
    }

    if (!setCurrentAndSpecializePhis(state.branch.ifFalse))
        return ControlStatus_Error;
    graph().moveBlockToEnd(current);
    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processIfElseTrueEnd(CFGState &state)
{
    
    
    state.state = CFGState::IF_ELSE_FALSE;
    state.branch.ifTrue = current;
    state.stopAt = state.branch.falseEnd;
    pc = state.branch.ifFalse->pc();
    if (!setCurrentAndSpecializePhis(state.branch.ifFalse))
        return ControlStatus_Error;
    graph().moveBlockToEnd(current);

    if (state.branch.test) {
        MTest *test = state.branch.test;
        if (!improveTypesAtTest(test->getOperand(0), test->ifTrue() == current, test))
            return ControlStatus_Error;
    }

    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processIfElseFalseEnd(CFGState &state)
{
    
    state.branch.ifFalse = current;

    
    
    MBasicBlock *pred = state.branch.ifTrue
                        ? state.branch.ifTrue
                        : state.branch.ifFalse;
    MBasicBlock *other = (pred == state.branch.ifTrue) ? state.branch.ifFalse : state.branch.ifTrue;

    if (!pred)
        return ControlStatus_Ended;

    
    MBasicBlock *join = newBlock(pred, state.branch.falseEnd);
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
IonBuilder::processBrokenLoop(CFGState &state)
{
    JS_ASSERT(!current);

    JS_ASSERT(loopDepth_);
    loopDepth_--;

    
    
    for (MBasicBlockIterator i(graph().begin(state.loop.entry)); i != graph().end(); i++) {
        if (i->loopDepth() > loopDepth_)
            i->setLoopDepth(i->loopDepth() - 1);
    }

    
    
    
    if (!setCurrentAndSpecializePhis(state.loop.successor))
        return ControlStatus_Error;
    if (current) {
        JS_ASSERT(current->loopDepth() == loopDepth_);
        graph().moveBlockToEnd(current);
    }

    
    if (state.loop.breaks) {
        MBasicBlock *block = createBreakCatchBlock(state.loop.breaks, state.loop.exitpc);
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
IonBuilder::finishLoop(CFGState &state, MBasicBlock *successor)
{
    JS_ASSERT(current);

    JS_ASSERT(loopDepth_);
    loopDepth_--;
    JS_ASSERT_IF(successor, successor->loopDepth() == loopDepth_);

    
    
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
        
        DeferredEdge *edge = state.loop.breaks;
        while (edge) {
            edge->block->inheritPhis(state.loop.entry);
            edge = edge->next;
        }

        
        MBasicBlock *block = createBreakCatchBlock(state.loop.breaks, state.loop.exitpc);
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

    MBasicBlock *header = state.loop.entry;

    
    
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

    CFGState &nstate = cfgStack_.back();

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
IonBuilder::processDoWhileBodyEnd(CFGState &state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    
    
    if (!current)
        return processBrokenLoop(state);

    MBasicBlock *header = newBlock(current, state.loop.updatepc);
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
IonBuilder::processDoWhileCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE);

    
    
    JS_ASSERT(current);

    
    MDefinition *vins = current->pop();
    MBasicBlock *successor = newBlock(current, GetNextPc(pc), loopDepth_ - 1);
    if (!successor)
        return ControlStatus_Error;

    
    if (vins->isConstant()) {
        MConstant *cte = vins->toConstant();
        if (cte->value().isBoolean() && !cte->value().toBoolean()) {
            current->end(MGoto::New(alloc(), successor));
            current = nullptr;

            state.loop.successor = successor;
            return processBrokenLoop(state);
        }
    }

    
    MTest *test = newTest(vins, state.loop.entry, successor);
    current->end(test);
    return finishLoop(state, successor);
}

IonBuilder::ControlStatus
IonBuilder::processWhileCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFEQ);

    
    MDefinition *ins = current->pop();

    
    MBasicBlock *body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc, loopDepth_ - 1);
    if (!body || !state.loop.successor)
        return ControlStatus_Error;

    MTest *test;
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
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processWhileBodyEnd(CFGState &state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    if (!current)
        return processBrokenLoop(state);

    current->end(MGoto::New(alloc(), state.loop.entry));
    return finishLoop(state, state.loop.successor);
}

IonBuilder::ControlStatus
IonBuilder::processForCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE);

    
    MDefinition *ins = current->pop();

    
    MBasicBlock *body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc, loopDepth_ - 1);
    if (!body || !state.loop.successor)
        return ControlStatus_Error;

    MTest *test = newTest(ins, body, state.loop.successor);
    current->end(test);

    state.state = CFGState::FOR_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    if (!setCurrentAndSpecializePhis(body))
        return ControlStatus_Error;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processForBodyEnd(CFGState &state)
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
IonBuilder::processForUpdateEnd(CFGState &state)
{
    
    
    if (!current)
        return processBrokenLoop(state);

    current->end(MGoto::New(alloc(), state.loop.entry));
    return finishLoop(state, state.loop.successor);
}

IonBuilder::DeferredEdge *
IonBuilder::filterDeadDeferredEdges(DeferredEdge *edge)
{
    DeferredEdge *head = edge, *prev = nullptr;

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

    
    
    
    JS_ASSERT(head);

    return head;
}

bool
IonBuilder::processDeferredContinues(CFGState &state)
{
    
    
    if (state.loop.continues) {
        DeferredEdge *edge = filterDeadDeferredEdges(state.loop.continues);

        MBasicBlock *update = newBlock(edge->block, loops_.back().continuepc);
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

MBasicBlock *
IonBuilder::createBreakCatchBlock(DeferredEdge *edge, jsbytecode *pc)
{
    edge = filterDeadDeferredEdges(edge);

    
    MBasicBlock *successor = newBlock(edge->block, pc);
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
IonBuilder::processNextTableSwitchCase(CFGState &state)
{
    JS_ASSERT(state.state == CFGState::TABLE_SWITCH);

    state.tableswitch.currentBlock++;

    
    if (state.tableswitch.currentBlock >= state.tableswitch.ins->numBlocks())
        return processSwitchEnd(state.tableswitch.breaks, state.tableswitch.exitpc);

    
    MBasicBlock *successor = state.tableswitch.ins->getBlock(state.tableswitch.currentBlock);

    
    
    
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
IonBuilder::processAndOrEnd(CFGState &state)
{
    
    
    current->end(MGoto::New(alloc(), state.branch.ifFalse));

    if (!state.branch.ifFalse->addPredecessor(alloc(), current))
        return ControlStatus_Error;

    if (!setCurrentAndSpecializePhis(state.branch.ifFalse))
        return ControlStatus_Error;
    graph().moveBlockToEnd(current);
    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processLabelEnd(CFGState &state)
{
    JS_ASSERT(state.state == CFGState::LABEL);

    
    if (!state.label.breaks && !current)
        return ControlStatus_Ended;

    
    if (!state.label.breaks)
        return ControlStatus_Joined;

    MBasicBlock *successor = createBreakCatchBlock(state.label.breaks, state.stopAt);
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
IonBuilder::processTryEnd(CFGState &state)
{
    JS_ASSERT(state.state == CFGState::TRY);

    if (!state.try_.successor) {
        JS_ASSERT(!current);
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
IonBuilder::processBreak(JSOp op, jssrcnote *sn)
{
    JS_ASSERT(op == JSOP_GOTO);

    JS_ASSERT(SN_TYPE(sn) == SRC_BREAK ||
              SN_TYPE(sn) == SRC_BREAK2LABEL);

    
    jsbytecode *target = pc + GetJumpOffset(pc);
    DebugOnly<bool> found = false;

    if (SN_TYPE(sn) == SRC_BREAK2LABEL) {
        for (size_t i = labels_.length() - 1; i < labels_.length(); i--) {
            CFGState &cfg = cfgStack_[labels_[i].cfgEntry];
            JS_ASSERT(cfg.state == CFGState::LABEL);
            if (cfg.stopAt == target) {
                cfg.label.breaks = new(alloc()) DeferredEdge(current, cfg.label.breaks);
                found = true;
                break;
            }
        }
    } else {
        for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
            CFGState &cfg = cfgStack_[loops_[i].cfgEntry];
            JS_ASSERT(cfg.isLoop());
            if (cfg.loop.exitpc == target) {
                cfg.loop.breaks = new(alloc()) DeferredEdge(current, cfg.loop.breaks);
                found = true;
                break;
            }
        }
    }

    JS_ASSERT(found);

    setCurrent(nullptr);
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

static inline jsbytecode *
EffectiveContinue(jsbytecode *pc)
{
    if (JSOp(*pc) == JSOP_GOTO)
        return pc + GetJumpOffset(pc);
    return pc;
}

IonBuilder::ControlStatus
IonBuilder::processContinue(JSOp op)
{
    JS_ASSERT(op == JSOP_GOTO);

    
    CFGState *found = nullptr;
    jsbytecode *target = pc + GetJumpOffset(pc);
    for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
        if (loops_[i].continuepc == target ||
            EffectiveContinue(loops_[i].continuepc) == target)
        {
            found = &cfgStack_[loops_[i].cfgEntry];
            break;
        }
    }

    
    
    JS_ASSERT(found);
    CFGState &state = *found;

    state.loop.continues = new(alloc()) DeferredEdge(current, state.loop.continues);

    setCurrent(nullptr);
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processSwitchBreak(JSOp op)
{
    JS_ASSERT(op == JSOP_GOTO);

    
    CFGState *found = nullptr;
    jsbytecode *target = pc + GetJumpOffset(pc);
    for (size_t i = switches_.length() - 1; i < switches_.length(); i--) {
        if (switches_[i].continuepc == target) {
            found = &cfgStack_[switches_[i].cfgEntry];
            break;
        }
    }

    
    
    JS_ASSERT(found);
    CFGState &state = *found;

    DeferredEdge **breaks = nullptr;
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
IonBuilder::processSwitchEnd(DeferredEdge *breaks, jsbytecode *exitpc)
{
    
    
    
    if (!breaks && !current)
        return ControlStatus_Ended;

    
    
    
    MBasicBlock *successor = nullptr;
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
IonBuilder::maybeLoop(JSOp op, jssrcnote *sn)
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
IonBuilder::assertValidLoopHeadOp(jsbytecode *pc)
{
#ifdef DEBUG
    JS_ASSERT(JSOp(*pc) == JSOP_LOOPHEAD);

    
    
    CFGState &state = cfgStack_.back();
    JS_ASSERT_IF((JSOp)*(state.loop.entry->pc()) == JSOP_GOTO,
        GetNextPc(state.loop.entry->pc()) == pc);

    
    jssrcnote *sn = info().getNote(gsn, pc);
    if (sn) {
        jsbytecode *ifne = pc + js_GetSrcNoteOffset(sn, 0);

        jsbytecode *expected_ifne;
        switch (state.state) {
          case CFGState::DO_WHILE_LOOP_BODY:
            expected_ifne = state.loop.updateEnd;
            break;

          default:
            MOZ_CRASH("JSOP_LOOPHEAD unexpected source note");
        }

        
        
        JS_ASSERT(ifne == expected_ifne);
    } else {
        JS_ASSERT(state.state != CFGState::DO_WHILE_LOOP_BODY);
    }
#endif
}

IonBuilder::ControlStatus
IonBuilder::doWhileLoop(JSOp op, jssrcnote *sn)
{
    
    
    
    
    
    
    
    
    
    int condition_offset = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *conditionpc = pc + condition_offset;

    jssrcnote *sn2 = info().getNote(gsn, pc+1);
    int offset = js_GetSrcNoteOffset(sn2, 0);
    jsbytecode *ifne = pc + offset + 1;
    JS_ASSERT(ifne > pc);

    
    jsbytecode *loopHead = GetNextPc(pc);
    JS_ASSERT(JSOp(*loopHead) == JSOP_LOOPHEAD);
    JS_ASSERT(loopHead == ifne + GetJumpOffset(ifne));

    jsbytecode *loopEntry = GetNextPc(loopHead);
    bool canOsr = LoopEntryCanIonOsr(loopEntry);
    bool osr = info().hasOsrAt(loopEntry);

    if (osr) {
        MBasicBlock *preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(alloc(), preheader));
        if (!setCurrentAndSpecializePhis(preheader))
            return ControlStatus_Error;
    }

    unsigned stackPhiCount = 0;
    MBasicBlock *header = newPendingLoopHeader(current, pc, osr, canOsr, stackPhiCount);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(alloc(), header));

    jsbytecode *loophead = GetNextPc(pc);
    jsbytecode *bodyStart = GetNextPc(loophead);
    jsbytecode *bodyEnd = conditionpc;
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!analyzeNewLoopTypes(header, bodyStart, exitpc))
        return ControlStatus_Error;
    if (!pushLoop(CFGState::DO_WHILE_LOOP_BODY, conditionpc, header, osr,
                  loopHead, bodyStart, bodyStart, bodyEnd, exitpc, conditionpc))
    {
        return ControlStatus_Error;
    }

    CFGState &state = cfgStack_.back();
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
IonBuilder::whileOrForInLoop(jssrcnote *sn)
{
    
    
    
    
    
    
    
    
    
    JS_ASSERT(SN_TYPE(sn) == SRC_FOR_OF || SN_TYPE(sn) == SRC_FOR_IN || SN_TYPE(sn) == SRC_WHILE);
    int ifneOffset = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *ifne = pc + ifneOffset;
    JS_ASSERT(ifne > pc);

    
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_LOOPHEAD);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    jsbytecode *loopEntry = pc + GetJumpOffset(pc);
    bool canOsr = LoopEntryCanIonOsr(loopEntry);
    bool osr = info().hasOsrAt(loopEntry);

    if (osr) {
        MBasicBlock *preheader = newOsrPreheader(current, loopEntry);
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

    MBasicBlock *header = newPendingLoopHeader(current, pc, osr, canOsr, stackPhiCount);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(alloc(), header));

    
    jsbytecode *loopHead = GetNextPc(pc);
    jsbytecode *bodyStart = GetNextPc(loopHead);
    jsbytecode *bodyEnd = pc + GetJumpOffset(pc);
    jsbytecode *exitpc = GetNextPc(ifne);
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
IonBuilder::forLoop(JSOp op, jssrcnote *sn)
{
    
    JS_ASSERT(op == JSOP_POP || op == JSOP_NOP);
    pc = GetNextPc(pc);

    jsbytecode *condpc = pc + js_GetSrcNoteOffset(sn, 0);
    jsbytecode *updatepc = pc + js_GetSrcNoteOffset(sn, 1);
    jsbytecode *ifne = pc + js_GetSrcNoteOffset(sn, 2);
    jsbytecode *exitpc = GetNextPc(ifne);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    jsbytecode *bodyStart = pc;
    jsbytecode *bodyEnd = updatepc;
    jsbytecode *loopEntry = condpc;
    if (condpc != ifne) {
        JS_ASSERT(JSOp(*bodyStart) == JSOP_GOTO);
        JS_ASSERT(bodyStart + GetJumpOffset(bodyStart) == condpc);
        bodyStart = GetNextPc(bodyStart);
    } else {
        
        if (op != JSOP_NOP) {
            
            JS_ASSERT(JSOp(*bodyStart) == JSOP_NOP);
            bodyStart = GetNextPc(bodyStart);
        }
        loopEntry = GetNextPc(bodyStart);
    }
    jsbytecode *loopHead = bodyStart;
    JS_ASSERT(JSOp(*bodyStart) == JSOP_LOOPHEAD);
    JS_ASSERT(ifne + GetJumpOffset(ifne) == bodyStart);
    bodyStart = GetNextPc(bodyStart);

    bool osr = info().hasOsrAt(loopEntry);
    bool canOsr = LoopEntryCanIonOsr(loopEntry);

    if (osr) {
        MBasicBlock *preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(alloc(), preheader));
        if (!setCurrentAndSpecializePhis(preheader))
            return ControlStatus_Error;
    }

    unsigned stackPhiCount = 0;
    MBasicBlock *header = newPendingLoopHeader(current, pc, osr, canOsr, stackPhiCount);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(alloc(), header));

    
    
    jsbytecode *stopAt;
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

    CFGState &state = cfgStack_.back();
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
IonBuilder::CmpSuccessors(const void *a, const void *b)
{
    const MBasicBlock *a0 = * (MBasicBlock * const *)a;
    const MBasicBlock *b0 = * (MBasicBlock * const *)b;
    if (a0->pc() == b0->pc())
        return 0;

    return (a0->pc() > b0->pc()) ? 1 : -1;
}

IonBuilder::ControlStatus
IonBuilder::tableSwitch(JSOp op, jssrcnote *sn)
{
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT(op == JSOP_TABLESWITCH);
    JS_ASSERT(SN_TYPE(sn) == SRC_TABLESWITCH);

    
    MDefinition *ins = current->pop();

    
    jsbytecode *exitpc = pc + js_GetSrcNoteOffset(sn, 0);
    jsbytecode *defaultpc = pc + GET_JUMP_OFFSET(pc);

    JS_ASSERT(defaultpc > pc && defaultpc <= exitpc);

    
    jsbytecode *pc2 = pc;
    pc2 += JUMP_OFFSET_LEN;
    int low = GET_JUMP_OFFSET(pc2);
    pc2 += JUMP_OFFSET_LEN;
    int high = GET_JUMP_OFFSET(pc2);
    pc2 += JUMP_OFFSET_LEN;

    
    MTableSwitch *tableswitch = MTableSwitch::New(alloc(), ins, low, high);

    
    MBasicBlock *defaultcase = newBlock(current, defaultpc);
    if (!defaultcase)
        return ControlStatus_Error;
    tableswitch->addDefault(defaultcase);
    tableswitch->addBlock(defaultcase);

    
    jsbytecode *casepc = nullptr;
    for (int i = 0; i < high-low+1; i++) {
        casepc = pc + GET_JUMP_OFFSET(pc2);

        JS_ASSERT(casepc >= pc && casepc <= exitpc);

        MBasicBlock *caseblock = newBlock(current, casepc);
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

    JS_ASSERT(tableswitch->numCases() == (uint32_t)(high - low + 1));
    JS_ASSERT(tableswitch->numSuccessors() > 0);

    
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
IonBuilder::replaceTypeSet(MDefinition *subject, types::TemporaryTypeSet *type, MTest *test)
{
    if (type->unknown())
        return true;

    MFilterTypeSet *replace = nullptr;
    MDefinition *ins;

    for (uint32_t i = 0; i < current->stackDepth(); i++) {
        ins = current->getSlot(i);

        
        if (ins->isFilterTypeSet() && ins->getOperand(0) == subject &&
            ins->dependency() == test)
        {
            types::TemporaryTypeSet *intersect =
                types::TypeSet::intersectSets(ins->resultTypeSet(), type, alloc_->lifoAlloc());
            if (!intersect)
                return false;

            ins->toFilterTypeSet()->setResultType(intersect->getKnownMIRType());
            ins->toFilterTypeSet()->setResultTypeSet(intersect);
            continue;
        }

        if (ins == subject) {
            if (!replace) {
                replace = MFilterTypeSet::New(alloc(), subject, type);

                if (!replace)
                    return false;
                if (replace == subject)
                    break;

                current->add(replace);

                if (replace != subject) {
                    
                    
                    
                    
                    replace->setDependency(test);
                }
            }
            current->setSlot(i, replace);
        }
    }
    return true;
}

bool
IonBuilder::detectAndOrStructure(MPhi *ins, bool *branchIsAnd)
{
    
    return false;

    if (current->numPredecessors() != 1)
        return false;

    MTest *initialTest;
    MBasicBlock *initialBlock;
    MBasicBlock *branchBlock;
    MBasicBlock *testBlock = ins->block();

    
    *branchIsAnd = true;
    
    if (testBlock->numPredecessors() != 2)
        return false;

    if (testBlock->getPredecessor(0)->lastIns()->isTest())
        initialBlock = testBlock->getPredecessor(0);
    else if (testBlock->getPredecessor(1)->lastIns()->isTest())
        initialBlock = testBlock->getPredecessor(1);
    else
        return false;

    initialTest = initialBlock->lastIns()->toTest();

    if (initialTest->ifTrue() == testBlock) {
        branchBlock = initialTest->ifFalse();
        *branchIsAnd = false;
    } else {
        branchBlock = initialTest->ifTrue();
    }

    if (branchBlock->numSuccessors() != 1 || branchBlock->getSuccessor(0) != testBlock)
        return false;

    if (branchBlock->numPredecessors() != 1)
        return false;

    MPhi *phi = ins->toPhi();

    MDefinition *branchResult = phi->getOperand(testBlock->indexForPredecessor(branchBlock));
    MDefinition *initialResult = phi->getOperand(testBlock->indexForPredecessor(initialBlock));

    if (branchBlock->stackDepth() != initialBlock->stackDepth())
        return false;
    if (branchBlock->stackDepth() != testBlock->stackDepth() + 1)
        return false;
    if (branchResult != branchBlock->peek(-1) || initialResult != initialBlock->peek(-1))
        return false;

    return true;
}

bool
IonBuilder::improveTypesAtCompare(MCompare *ins, bool trueBranch, MTest *test)
{
    
    if (ins->compareType() != MCompare::Compare_Undefined &&
        ins->compareType() != MCompare::Compare_Null)
    {
        return true;
    }

    MOZ_ASSERT(ins->jsop() == JSOP_STRICTNE || ins->jsop() == JSOP_NE ||
               ins->jsop() == JSOP_STRICTEQ || ins->jsop() == JSOP_EQ);

    
    if (!trueBranch && (ins->jsop() == JSOP_STRICTNE || ins->jsop() == JSOP_NE))
        return true;

    
    if (trueBranch && (ins->jsop() == JSOP_STRICTEQ || ins->jsop() == JSOP_EQ))
        return true;

    bool filtersUndefined = false;
    bool filtersNull = false;
    if (ins->jsop() == JSOP_STRICTEQ || ins->jsop() == JSOP_STRICTNE) {
        filtersUndefined = (ins->compareType() == MCompare::Compare_Undefined);
        filtersNull = (ins->compareType() == MCompare::Compare_Null);
    } else {
        filtersUndefined = filtersNull = true;
    }

    MOZ_ASSERT(IsNullOrUndefined(ins->rhs()->type()));

    MDefinition *subject = ins->lhs();
    if (!subject->resultTypeSet() || subject->resultTypeSet()->unknown())
        return true;

    
    if ((!filtersUndefined || !subject->mightBeType(MIRType_Undefined)) &&
        (!filtersNull || !subject->mightBeType(MIRType_Null)))
    {
        return true;
    }

    types::TemporaryTypeSet *type =
        type = subject->resultTypeSet()->filter(alloc_->lifoAlloc(), filtersUndefined,
                                                                     filtersNull);
    if (!type)
        return false;

    return replaceTypeSet(subject, type, test);
}

bool
IonBuilder::improveTypesAtTest(MDefinition *ins, bool trueBranch, MTest *test)
{
    
    
    if (!ins)
        return true;

    switch(ins->op()) {
      case MDefinition::Op_Not:
        return improveTypesAtTest(ins->toNot()->getOperand(0), !trueBranch, test);
      case MDefinition::Op_IsObject: {
        types::TemporaryTypeSet *oldType = ins->getOperand(0)->resultTypeSet();
        if (!oldType)
            return true;
        if (oldType->unknown() || !oldType->mightBeMIRType(MIRType_Object))
            return true;

        types::TemporaryTypeSet *type = nullptr;
        if (trueBranch)
            type = oldType->cloneObjectsOnly(alloc_->lifoAlloc());
        else
            type = oldType->cloneWithoutObjects(alloc_->lifoAlloc());

        if (!type)
            return false;

        return replaceTypeSet(ins->getOperand(0), type, test);
      }
      case MDefinition::Op_Phi: {
        bool branchIsAnd = true;
        if (!detectAndOrStructure(ins->toPhi(), &branchIsAnd))
            return true;

        
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

    return true;
}

bool
IonBuilder::jsop_label()
{
    JS_ASSERT(JSOp(*pc) == JSOP_LABEL);

    jsbytecode *endpc = pc + GET_JUMP_OFFSET(pc);
    JS_ASSERT(endpc > pc);

    ControlFlowInfo label(cfgStack_.length(), endpc);
    if (!labels_.append(label))
        return false;

    return cfgStack_.append(CFGState::Label(endpc));
}

bool
IonBuilder::jsop_condswitch()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT(JSOp(*pc) == JSOP_CONDSWITCH);
    jssrcnote *sn = info().getNote(gsn, pc);
    JS_ASSERT(SN_TYPE(sn) == SRC_CONDSWITCH);

    
    jsbytecode *exitpc = pc + js_GetSrcNoteOffset(sn, 0);
    jsbytecode *firstCase = pc + js_GetSrcNoteOffset(sn, 1);

    
    
    
    
    jsbytecode *curCase = firstCase;
    jsbytecode *lastTarget = GetJumpOffset(curCase) + curCase;
    size_t nbBodies = 2; 

    JS_ASSERT(pc < curCase && curCase <= exitpc);
    while (JSOp(*curCase) == JSOP_CASE) {
        
        jssrcnote *caseSn = info().getNote(gsn, curCase);
        JS_ASSERT(caseSn && SN_TYPE(caseSn) == SRC_NEXTCASE);
        ptrdiff_t off = js_GetSrcNoteOffset(caseSn, 0);
        curCase = off ? curCase + off : GetNextPc(curCase);
        JS_ASSERT(pc < curCase && curCase <= exitpc);

        
        jsbytecode *curTarget = GetJumpOffset(curCase) + curCase;
        if (lastTarget < curTarget)
            nbBodies++;
        lastTarget = curTarget;
    }

    
    
    JS_ASSERT(JSOp(*curCase) == JSOP_DEFAULT);
    jsbytecode *defaultTarget = GetJumpOffset(curCase) + curCase;
    JS_ASSERT(curCase < defaultTarget && defaultTarget <= exitpc);

    
    CFGState state = CFGState::CondSwitch(this, exitpc, defaultTarget);
    if (!state.condswitch.bodies || !state.condswitch.bodies->init(alloc(), nbBodies))
        return ControlStatus_Error;

    
    JS_ASSERT(JSOp(*firstCase) == JSOP_CASE);
    state.stopAt = firstCase;
    state.state = CFGState::COND_SWITCH_CASE;

    return cfgStack_.append(state);
}

IonBuilder::CFGState
IonBuilder::CFGState::CondSwitch(IonBuilder *builder, jsbytecode *exitpc, jsbytecode *defaultTarget)
{
    CFGState state;
    state.state = COND_SWITCH_CASE;
    state.stopAt = nullptr;
    state.condswitch.bodies = (FixedList<MBasicBlock *> *)builder->alloc_->allocate(
        sizeof(FixedList<MBasicBlock *>));
    state.condswitch.currentIdx = 0;
    state.condswitch.defaultTarget = defaultTarget;
    state.condswitch.defaultIdx = uint32_t(-1);
    state.condswitch.exitpc = exitpc;
    state.condswitch.breaks = nullptr;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::Label(jsbytecode *exitpc)
{
    CFGState state;
    state.state = LABEL;
    state.stopAt = exitpc;
    state.label.breaks = nullptr;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::Try(jsbytecode *exitpc, MBasicBlock *successor)
{
    CFGState state;
    state.state = TRY;
    state.stopAt = exitpc;
    state.try_.successor = successor;
    return state;
}

IonBuilder::ControlStatus
IonBuilder::processCondSwitchCase(CFGState &state)
{
    JS_ASSERT(state.state == CFGState::COND_SWITCH_CASE);
    JS_ASSERT(!state.condswitch.breaks);
    JS_ASSERT(current);
    JS_ASSERT(JSOp(*pc) == JSOP_CASE);
    FixedList<MBasicBlock *> &bodies = *state.condswitch.bodies;
    jsbytecode *defaultTarget = state.condswitch.defaultTarget;
    uint32_t &currentIdx = state.condswitch.currentIdx;
    jsbytecode *lastTarget = currentIdx ? bodies[currentIdx - 1]->pc() : nullptr;

    
    jssrcnote *sn = info().getNote(gsn, pc);
    ptrdiff_t off = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *casePc = off ? pc + off : GetNextPc(pc);
    bool caseIsDefault = JSOp(*casePc) == JSOP_DEFAULT;
    JS_ASSERT(JSOp(*casePc) == JSOP_CASE || caseIsDefault);

    
    bool bodyIsNew = false;
    MBasicBlock *bodyBlock = nullptr;
    jsbytecode *bodyTarget = pc + GetJumpOffset(pc);
    if (lastTarget < bodyTarget) {
        
        if (lastTarget < defaultTarget && defaultTarget <= bodyTarget) {
            JS_ASSERT(state.condswitch.defaultIdx == uint32_t(-1));
            state.condswitch.defaultIdx = currentIdx;
            bodies[currentIdx] = nullptr;
            
            
            if (defaultTarget < bodyTarget)
                currentIdx++;
        }

        bodyIsNew = true;
        
        bodyBlock = newBlockPopN(current, bodyTarget, 2);
        bodies[currentIdx++] = bodyBlock;
    } else {
        
        JS_ASSERT(lastTarget == bodyTarget);
        JS_ASSERT(currentIdx > 0);
        bodyBlock = bodies[currentIdx - 1];
    }

    if (!bodyBlock)
        return ControlStatus_Error;

    lastTarget = bodyTarget;

    
    
    bool caseIsNew = false;
    MBasicBlock *caseBlock = nullptr;
    if (!caseIsDefault) {
        caseIsNew = true;
        
        caseBlock = newBlockPopN(current, GetNextPc(pc), 1);
    } else {
        
        
        

        if (state.condswitch.defaultIdx == uint32_t(-1)) {
            
            JS_ASSERT(lastTarget < defaultTarget);
            state.condswitch.defaultIdx = currentIdx++;
            caseIsNew = true;
        } else if (bodies[state.condswitch.defaultIdx] == nullptr) {
            
            
            JS_ASSERT(defaultTarget < lastTarget);
            caseIsNew = true;
        } else {
            
            JS_ASSERT(defaultTarget <= lastTarget);
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
        MDefinition *caseOperand = current->pop();
        MDefinition *switchOperand = current->peek(-1);
        MCompare *cmpResult = MCompare::New(alloc(), switchOperand, caseOperand, JSOP_STRICTEQ);
        cmpResult->infer(inspector, pc);
        JS_ASSERT(!cmpResult->isEffectful());
        current->add(cmpResult);
        current->end(newTest(cmpResult, bodyBlock, caseBlock));

        
        
        if (!bodyIsNew && !bodyBlock->addPredecessorPopN(alloc(), current, 1))
            return ControlStatus_Error;

        
        
        
        
        JS_ASSERT_IF(!caseIsNew, caseIsDefault);
        if (!caseIsNew && !caseBlock->addPredecessorPopN(alloc(), current, 1))
            return ControlStatus_Error;
    } else {
        
        JS_ASSERT(caseIsDefault);
        current->pop(); 
        current->pop(); 
        current->end(MGoto::New(alloc(), bodyBlock));
        if (!bodyIsNew && !bodyBlock->addPredecessor(alloc(), current))
            return ControlStatus_Error;
    }

    if (caseIsDefault) {
        
        
        
        
        JS_ASSERT(currentIdx == bodies.length() || currentIdx + 1 == bodies.length());
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
IonBuilder::processCondSwitchBody(CFGState &state)
{
    JS_ASSERT(state.state == CFGState::COND_SWITCH_BODY);
    JS_ASSERT(pc <= state.condswitch.exitpc);
    FixedList<MBasicBlock *> &bodies = *state.condswitch.bodies;
    uint32_t &currentIdx = state.condswitch.currentIdx;

    JS_ASSERT(currentIdx <= bodies.length());
    if (currentIdx == bodies.length()) {
        JS_ASSERT_IF(current, pc == state.condswitch.exitpc);
        return processSwitchEnd(state.condswitch.breaks, state.condswitch.exitpc);
    }

    
    MBasicBlock *nextBody = bodies[currentIdx++];
    JS_ASSERT_IF(current, pc == nextBody->pc());

    
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
    JS_ASSERT(op == JSOP_AND || op == JSOP_OR);

    jsbytecode *rhsStart = pc + js_CodeSpec[op].length;
    jsbytecode *joinStart = pc + GetJumpOffset(pc);
    JS_ASSERT(joinStart > pc);

    
    MDefinition *lhs = current->peek(-1);

    MBasicBlock *evalRhs = newBlock(current, rhsStart);
    MBasicBlock *join = newBlock(current, joinStart);
    if (!evalRhs || !join)
        return false;

    MTest *test = (op == JSOP_AND)
                  ? newTest(lhs, evalRhs, join)
                  : newTest(lhs, join, evalRhs);
    current->end(test);

    if (!cfgStack_.append(CFGState::AndOr(joinStart, join)))
        return false;

    return setCurrentAndSpecializePhis(evalRhs);
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
IonBuilder::jsop_loophead(jsbytecode *pc)
{
    assertValidLoopHeadOp(pc);

    current->add(MInterruptCheck::New(alloc()));
    insertRecompileCheck();

    return true;
}

bool
IonBuilder::jsop_ifeq(JSOp op)
{
    
    jsbytecode *trueStart = pc + js_CodeSpec[op].length;
    jsbytecode *falseStart = pc + GetJumpOffset(pc);
    JS_ASSERT(falseStart > pc);

    
    jssrcnote *sn = info().getNote(gsn, pc);
    if (!sn)
        return abort("expected sourcenote");

    MDefinition *ins = current->pop();

    
    MBasicBlock *ifTrue = newBlock(current, trueStart);
    MBasicBlock *ifFalse = newBlock(current, falseStart);
    if (!ifTrue || !ifFalse)
        return false;

    MTest *test = newTest(ins, ifTrue, ifFalse);
    current->end(test);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    switch (SN_TYPE(sn)) {
      case SRC_IF:
        if (!cfgStack_.append(CFGState::If(falseStart, test)))
            return false;
        break;

      case SRC_IF_ELSE:
      case SRC_COND:
      {
        
        
        jsbytecode *trueEnd = pc + js_GetSrcNoteOffset(sn, 0);
        JS_ASSERT(trueEnd > pc);
        JS_ASSERT(trueEnd < falseStart);
        JS_ASSERT(JSOp(*trueEnd) == JSOP_GOTO);
        JS_ASSERT(!info().getNote(gsn, trueEnd));

        jsbytecode *falseEnd = trueEnd + GetJumpOffset(trueEnd);
        JS_ASSERT(falseEnd > trueEnd);
        JS_ASSERT(falseEnd >= falseStart);

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
    JS_ASSERT(JSOp(*pc) == JSOP_TRY);

    if (!js_JitOptions.compileTryCatch)
        return abort("Try-catch support disabled");

    
    if (analysis().hasTryFinally())
        return abort("Has try-finally");

    
    JS_ASSERT(!isInlineBuilder());

    
    
    if (info().executionMode() == ArgumentsUsageAnalysis)
        return abort("Try-catch during arguments usage analysis");

    graph().setHasTryBlock();

    jssrcnote *sn = info().getNote(gsn, pc);
    JS_ASSERT(SN_TYPE(sn) == SRC_TRY);

    
    
    jsbytecode *endpc = pc + js_GetSrcNoteOffset(sn, 0);
    JS_ASSERT(JSOp(*endpc) == JSOP_GOTO);
    JS_ASSERT(GetJumpOffset(endpc) > 0);

    jsbytecode *afterTry = endpc + GetJumpOffset(endpc);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    MBasicBlock *tryBlock = newBlock(current, GetNextPc(pc));
    if (!tryBlock)
        return false;

    MBasicBlock *successor;
    if (analysis().maybeInfo(afterTry)) {
        successor = newBlock(current, afterTry);
        if (!successor)
            return false;

        
        MConstant *true_ = MConstant::New(alloc(), BooleanValue(true));
        current->add(true_);
        current->end(newTest(true_, tryBlock, successor));
    } else {
        successor = nullptr;
        current->end(MGoto::New(alloc(), tryBlock));
    }

    if (!cfgStack_.append(CFGState::Try(endpc, successor)))
        return false;

    
    
    JS_ASSERT(info().osrPc() < endpc || info().osrPc() >= afterTry);

    
    return setCurrentAndSpecializePhis(tryBlock);
}

IonBuilder::ControlStatus
IonBuilder::processReturn(JSOp op)
{
    MDefinition *def;
    switch (op) {
      case JSOP_RETURN:
        
        def = current->pop();
        break;

      case JSOP_RETRVAL:
        
        if (script()->noScriptRval()) {
            MInstruction *ins = MConstant::New(alloc(), UndefinedValue());
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

    if (instrumentedProfiling() && inliningDepth_ == 0) {
        current->add(MProfilerStackOp::New(alloc(), script(), MProfilerStackOp::Exit));
    }
    MReturn *ret = MReturn::New(alloc(), def);
    current->end(ret);

    if (!graph().addReturn(current))
        return ControlStatus_Error;

    
    setCurrent(nullptr);
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processThrow()
{
    MDefinition *def = current->pop();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    MNop *nop = MNop::New(alloc());
    current->add(nop);

    if (!resumeAfter(nop))
        return ControlStatus_Error;

    MThrow *ins = MThrow::New(alloc(), def);
    current->end(ins);

    
    setCurrent(nullptr);
    return processControlEnd();
}

bool
IonBuilder::pushConstant(const Value &v)
{
    current->push(constant(v));
    return true;
}

bool
IonBuilder::jsop_bitnot()
{
    MDefinition *input = current->pop();
    MBitNot *ins = MBitNot::New(alloc(), input);

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
    
    MDefinition *right = current->pop();
    MDefinition *left = current->pop();

    MBinaryBitwiseInstruction *ins;
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
IonBuilder::jsop_binary(JSOp op, MDefinition *left, MDefinition *right)
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
        MConcat *ins = MConcat::New(alloc(), left, right);
        current->add(ins);
        current->push(ins);
        return maybeInsertResume();
    }

    MBinaryArithInstruction *ins;
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
    MDefinition *right = current->pop();
    MDefinition *left = current->pop();

    return jsop_binary(op, left, right);
}

bool
IonBuilder::jsop_pos()
{
    if (IsNumberType(current->peek(-1)->type())) {
        
        
        
        current->peek(-1)->setImplicitlyUsedUnchecked();
        return true;
    }

    
    MDefinition *value = current->pop();
    MConstant *one = MConstant::New(alloc(), Int32Value(1));
    current->add(one);

    return jsop_binary(JSOP_MUL, value, one);
}

bool
IonBuilder::jsop_neg()
{
    
    
    MConstant *negator = MConstant::New(alloc(), Int32Value(-1));
    current->add(negator);

    MDefinition *right = current->pop();

    if (!jsop_binary(JSOP_MUL, negator, right))
        return false;
    return true;
}

class AutoAccumulateReturns
{
    MIRGraph &graph_;
    MIRGraphReturns *prev_;

  public:
    AutoAccumulateReturns(MIRGraph &graph, MIRGraphReturns &returns)
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
IonBuilder::inlineScriptedCall(CallInfo &callInfo, JSFunction *target)
{
    JS_ASSERT(target->hasScript());
    JS_ASSERT(IsIonInlinablePC(pc));

    callInfo.setImplicitlyUsedUnchecked();

    
    uint32_t depth = current->stackDepth() + callInfo.numFormals();
    if (depth > current->nslots()) {
        if (!current->increaseSlots(depth - current->nslots()))
            return false;
    }

    
    if (callInfo.constructing()) {
        MDefinition *thisDefn = createThis(target, callInfo.fun());
        if (!thisDefn)
            return false;
        callInfo.setThis(thisDefn);
    }

    
    callInfo.pushFormals(current);

    MResumePoint *outerResumePoint =
        MResumePoint::New(alloc(), current, pc, callerResumePoint_, MResumePoint::Outer);
    if (!outerResumePoint)
        return false;
    current->setOuterResumePoint(outerResumePoint);

    
    callInfo.popFormals(current);
    current->push(callInfo.fun());

    JSScript *calleeScript = target->nonLazyScript();
    BaselineInspector inspector(calleeScript);

    
    if (callInfo.constructing() &&
        !callInfo.thisArg()->resultTypeSet() &&
        calleeScript->types)
    {
        types::StackTypeSet *types = types::TypeScript::ThisTypes(calleeScript);
        if (!types->unknown()) {
            types::TemporaryTypeSet *clonedTypes = types->clone(alloc_->lifoAlloc());
            if (!clonedTypes)
                return oom();
            MTypeBarrier *barrier = MTypeBarrier::New(alloc(), callInfo.thisArg(), clonedTypes);
            current->add(barrier);
            callInfo.setThis(barrier);
        }
    }

    
    LifoAlloc *lifoAlloc = alloc_->lifoAlloc();
    InlineScriptTree *inlineScriptTree =
        info().inlineScriptTree()->addCallee(alloc_, pc, calleeScript);
    if (!inlineScriptTree)
        return false;
    CompileInfo *info = lifoAlloc->new_<CompileInfo>(calleeScript, target,
                                                     (jsbytecode *)nullptr, callInfo.constructing(),
                                                     this->info().executionMode(),
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
            JitSpew(JitSpew_Abort, "Inline builder raised exception.");
            abortReason_ = AbortReason_Error;
            return false;
        }

        
        
        if (inlineBuilder.abortReason_ == AbortReason_Disable) {
            calleeScript->setUninlineable();
            abortReason_ = AbortReason_Inlining;
        } else if (inlineBuilder.abortReason_ == AbortReason_Inlining) {
            abortReason_ = AbortReason_Inlining;
        } else if (inlineBuilder.abortReason_ == AbortReason_NewScriptProperties) {
            const TypeObjectVector &types = inlineBuilder.abortedNewScriptPropertiesTypes();
            JS_ASSERT(!types.empty());
            for (size_t i = 0; i < types.length(); i++)
                addAbortedNewScriptPropertiesType(types[i]);
            abortReason_ = AbortReason_NewScriptProperties;
        }

        return false;
    }

    
    jsbytecode *postCall = GetNextPc(pc);
    MBasicBlock *returnBlock = newBlock(nullptr, postCall);
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
    MDefinition *retvalDefn = patchInlinedReturns(callInfo, returns, returnBlock);
    if (!retvalDefn)
        return false;
    returnBlock->push(retvalDefn);

    
    if (!returnBlock->initEntrySlots(alloc()))
        return false;

    return setCurrentAndSpecializePhis(returnBlock);
}

MDefinition *
IonBuilder::patchInlinedReturn(CallInfo &callInfo, MBasicBlock *exit, MBasicBlock *bottom)
{
    
    MDefinition *rdef = exit->lastIns()->toReturn()->input();
    exit->discardLastIns();

    
    if (callInfo.constructing()) {
        if (rdef->type() == MIRType_Value) {
            
            MReturnFromCtor *filter = MReturnFromCtor::New(alloc(), rdef, callInfo.thisArg());
            exit->add(filter);
            rdef = filter;
        } else if (rdef->type() != MIRType_Object) {
            
            rdef = callInfo.thisArg();
        }
    } else if (callInfo.isSetter()) {
        
        rdef = callInfo.getArg(0);
    }

    MGoto *replacement = MGoto::New(alloc(), bottom);
    exit->end(replacement);
    if (!bottom->addPredecessorWithoutPhis(exit))
        return nullptr;

    return rdef;
}

MDefinition *
IonBuilder::patchInlinedReturns(CallInfo &callInfo, MIRGraphReturns &returns, MBasicBlock *bottom)
{
    
    
    JS_ASSERT(returns.length() > 0);

    if (returns.length() == 1)
        return patchInlinedReturn(callInfo, returns[0], bottom);

    
    MPhi *phi = MPhi::New(alloc());
    if (!phi->reserveLength(returns.length()))
        return nullptr;

    for (size_t i = 0; i < returns.length(); i++) {
        MDefinition *rdef = patchInlinedReturn(callInfo, returns[i], bottom);
        if (!rdef)
            return nullptr;
        phi->addInput(rdef);
    }

    bottom->addPhi(phi);
    return phi;
}

IonBuilder::InliningDecision
IonBuilder::makeInliningDecision(JSFunction *target, CallInfo &callInfo)
{
    
    if (target == nullptr)
        return InliningDecision_DontInline;

    
    if (info().executionMode() == ArgumentsUsageAnalysis)
        return InliningDecision_DontInline;

    
    if (target->isNative())
        return InliningDecision_Inline;

    
    InliningDecision decision = canInlineTarget(target, callInfo);
    if (decision != InliningDecision_Inline)
        return decision;

    
    JSScript *targetScript = target->nonLazyScript();

    
    if (!targetScript->shouldInline()) {
        
        if (js_JitOptions.isSmallFunction(targetScript)) {
            if (inliningDepth_ >= optimizationInfo().smallFunctionMaxInlineDepth())
                return DontInline(targetScript, "Vetoed: exceeding allowed inline depth");
        } else {
            if (inliningDepth_ >= optimizationInfo().maxInlineDepth())
                return DontInline(targetScript, "Vetoed: exceeding allowed inline depth");

            if (targetScript->hasLoops())
                return DontInline(targetScript, "Vetoed: big function that contains a loop");

            
            if (script()->length() >= optimizationInfo().inliningMaxCallerBytecodeLength())
                return DontInline(targetScript, "Vetoed: caller excessively large");
        }

        
        
        if (targetScript->length() > optimizationInfo().inlineMaxTotalBytecodeLength())
            return DontInline(targetScript, "Vetoed: callee excessively large");

        
        
        
        if (targetScript->getWarmUpCounter() < optimizationInfo().usesBeforeInlining() &&
            !targetScript->baselineScript()->ionCompiledOrInlined() &&
            info().executionMode() != DefinitePropertiesAnalysis)
        {
            return DontInline(targetScript, "Vetoed: callee is insufficiently hot.");
        }
    }

    
    types::TypeObjectKey *targetType = types::TypeObjectKey::get(target);
    targetType->watchStateChangeForInlinedCall(constraints());

    return InliningDecision_Inline;
}

bool
IonBuilder::selectInliningTargets(ObjectVector &targets, CallInfo &callInfo, BoolVector &choiceSet,
                                  uint32_t *numInlineable)
{
    *numInlineable = 0;
    uint32_t totalSize = 0;

    
    if (!choiceSet.reserve(targets.length()))
        return false;

    
    
    if (info().executionMode() == DefinitePropertiesAnalysis && targets.length() > 1)
        return true;

    for (size_t i = 0; i < targets.length(); i++) {
        JSFunction *target = &targets[i]->as<JSFunction>();
        bool inlineable;
        InliningDecision decision = makeInliningDecision(target, callInfo);
        switch (decision) {
          case InliningDecision_Error:
            return false;
          case InliningDecision_DontInline:
            inlineable = false;
            break;
          case InliningDecision_Inline:
            inlineable = true;
            break;
          default:
            MOZ_CRASH("Unhandled InliningDecision value!");
        }

        
        if (inlineable && target->isInterpreted()) {
            totalSize += target->nonLazyScript()->length();
            if (totalSize > optimizationInfo().inlineMaxTotalBytecodeLength())
                inlineable = false;
        }

        choiceSet.append(inlineable);
        if (inlineable)
            *numInlineable += 1;
    }

    JS_ASSERT(choiceSet.length() == targets.length());
    return true;
}

static bool
CanInlineGetPropertyCache(MGetPropertyCache *cache, MDefinition *thisDef)
{
    JS_ASSERT(cache->object()->type() == MIRType_Object);
    if (cache->object() != thisDef)
        return false;

    InlinePropertyTable *table = cache->propTable();
    if (!table)
        return false;
    if (table->numEntries() == 0)
        return false;
    return true;
}

class WrapMGetPropertyCache
{
    MGetPropertyCache *cache_;

  private:
    void discardPriorResumePoint() {
        if (!cache_)
            return;

        InlinePropertyTable *propTable = cache_->propTable();
        if (!propTable)
            return;
        MResumePoint *rp = propTable->takePriorResumePoint();
        if (!rp)
            return;
        cache_->block()->discardPreAllocatedResumePoint(rp);
    }

  public:
    explicit WrapMGetPropertyCache(MGetPropertyCache *cache)
      : cache_(cache)
    { }

    ~WrapMGetPropertyCache() {
        discardPriorResumePoint();
    }

    MGetPropertyCache *get() {
        return cache_;
    }
    MGetPropertyCache *operator->() {
        return get();
    }

    
    
    MGetPropertyCache *moveableCache(bool hasTypeBarrier, MDefinition *thisDef) {
        
        
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

        MGetPropertyCache *ret = cache_;
        cache_ = nullptr;
        return ret;
    }
};

MGetPropertyCache *
IonBuilder::getInlineableGetPropertyCache(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return nullptr;

    MDefinition *thisDef = callInfo.thisArg();
    if (thisDef->type() != MIRType_Object)
        return nullptr;

    MDefinition *funcDef = callInfo.fun();
    if (funcDef->type() != MIRType_Object)
        return nullptr;

    
    if (funcDef->isGetPropertyCache()) {
        WrapMGetPropertyCache cache(funcDef->toGetPropertyCache());
        return cache.moveableCache( false, thisDef);
    }

    
    
    if (funcDef->isTypeBarrier()) {
        MTypeBarrier *barrier = funcDef->toTypeBarrier();
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
IonBuilder::inlineSingleCall(CallInfo &callInfo, JSFunction *target)
{
    
    if (target->isNative())
        return inlineNativeCall(callInfo, target);

    if (!inlineScriptedCall(callInfo, target))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineCallsite(ObjectVector &targets, ObjectVector &originals,
                           bool lambda, CallInfo &callInfo)
{
    if (targets.empty())
        return InliningStatus_NotInlined;

    
    
    
    WrapMGetPropertyCache propCache(getInlineableGetPropertyCache(callInfo));

    
    
    if (!propCache.get() && targets.length() == 1) {
        JSFunction *target = &targets[0]->as<JSFunction>();
        InliningDecision decision = makeInliningDecision(target, callInfo);
        switch (decision) {
          case InliningDecision_Error:
            return InliningStatus_Error;
          case InliningDecision_DontInline:
            return InliningStatus_NotInlined;
          case InliningDecision_Inline:
            break;
        }

        
        
        
        callInfo.fun()->setImplicitlyUsedUnchecked();

        
        
        
        if (!lambda) {
            
            MConstant *constFun = constant(ObjectValue(*target));
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

    
    if (!inlineCalls(callInfo, targets, originals, choiceSet, propCache.get()))
        return InliningStatus_Error;

    return InliningStatus_Inlined;
}

bool
IonBuilder::inlineGenericFallback(JSFunction *target, CallInfo &callInfo, MBasicBlock *dispatchBlock,
                                  bool clonedAtCallsite)
{
    
    MBasicBlock *fallbackBlock = newBlock(dispatchBlock, pc);
    if (!fallbackBlock)
        return false;

    
    CallInfo fallbackInfo(alloc(), callInfo.constructing());
    if (!fallbackInfo.init(callInfo))
        return false;
    fallbackInfo.popFormals(fallbackBlock);

    
    if (!setCurrentAndSpecializePhis(fallbackBlock))
        return false;
    if (!makeCall(target, fallbackInfo, clonedAtCallsite))
        return false;

    
    return true;
}

bool
IonBuilder::inlineTypeObjectFallback(CallInfo &callInfo, MBasicBlock *dispatchBlock,
                                     MTypeObjectDispatch *dispatch, MGetPropertyCache *cache,
                                     MBasicBlock **fallbackTarget)
{
    
    
    
    JS_ASSERT(callInfo.fun()->isGetPropertyCache() || callInfo.fun()->isTypeBarrier());

    
    JS_ASSERT(dispatch->numCases() > 0);

    
    
    JS_ASSERT_IF(callInfo.fun()->isGetPropertyCache(), !cache->hasUses());
    JS_ASSERT_IF(callInfo.fun()->isTypeBarrier(), cache->hasOneUse());

    
    
    
    MOZ_ASSERT(cache->idempotent());

    
    CallInfo fallbackInfo(alloc(), callInfo.constructing());
    if (!fallbackInfo.init(callInfo))
        return false;

    
    MResumePoint *preCallResumePoint =
        MResumePoint::New(alloc(), dispatchBlock, pc, callerResumePoint_, MResumePoint::ResumeAt);
    if (!preCallResumePoint)
        return false;

    DebugOnly<size_t> preCallFuncIndex = preCallResumePoint->numOperands() - callInfo.numFormals();
    JS_ASSERT(preCallResumePoint->getOperand(preCallFuncIndex) == fallbackInfo.fun());

    
    MConstant *undefined = MConstant::New(alloc(), UndefinedValue());
    dispatchBlock->add(undefined);
    dispatchBlock->rewriteAtDepth(-int(callInfo.numFormals()), undefined);

    
    
    MBasicBlock *prepBlock = newBlock(dispatchBlock, pc);
    if (!prepBlock)
        return false;
    fallbackInfo.popFormals(prepBlock);

    
    
    InlinePropertyTable *propTable = cache->propTable();
    MResumePoint *priorResumePoint = propTable->takePriorResumePoint();
    JS_ASSERT(propTable->pc() != nullptr);
    JS_ASSERT(priorResumePoint != nullptr);
    MBasicBlock *getPropBlock = newBlock(prepBlock, propTable->pc(), priorResumePoint);
    if (!getPropBlock)
        return false;

    prepBlock->end(MGoto::New(alloc(), getPropBlock));

    
    
    DebugOnly<MDefinition *> checkObject = getPropBlock->pop();
    JS_ASSERT(checkObject == cache->object());

    
    if (fallbackInfo.fun()->isGetPropertyCache()) {
        JS_ASSERT(fallbackInfo.fun()->toGetPropertyCache() == cache);
        getPropBlock->addFromElsewhere(cache);
        getPropBlock->push(cache);
    } else {
        MTypeBarrier *barrier = callInfo.fun()->toTypeBarrier();
        JS_ASSERT(barrier->type() == MIRType_Object);
        JS_ASSERT(barrier->input()->isGetPropertyCache());
        JS_ASSERT(barrier->input()->toGetPropertyCache() == cache);

        getPropBlock->addFromElsewhere(cache);
        getPropBlock->addFromElsewhere(barrier);
        getPropBlock->push(barrier);
    }

    
    MBasicBlock *preCallBlock = newBlock(getPropBlock, pc, preCallResumePoint);
    if (!preCallBlock)
        return false;
    getPropBlock->end(MGoto::New(alloc(), preCallBlock));

    
    if (!inlineGenericFallback(nullptr, fallbackInfo, preCallBlock, false))
        return false;

    
    preCallBlock->end(MGoto::New(alloc(), current));
    *fallbackTarget = prepBlock;
    return true;
}

bool
IonBuilder::inlineCalls(CallInfo &callInfo, ObjectVector &targets,
                        ObjectVector &originals, BoolVector &choiceSet,
                        MGetPropertyCache *maybeCache)
{
    
    JS_ASSERT(IsIonInlinablePC(pc));
    JS_ASSERT(choiceSet.length() == targets.length());
    JS_ASSERT_IF(!maybeCache, targets.length() >= 2);
    JS_ASSERT_IF(maybeCache, targets.length() >= 1);

    MBasicBlock *dispatchBlock = current;
    callInfo.setImplicitlyUsedUnchecked();
    callInfo.pushFormals(dispatchBlock);

    
    
    
    
    
    
    
    
    if (maybeCache) {
        InlinePropertyTable *propTable = maybeCache->propTable();
        propTable->trimToTargets(originals);
        if (propTable->numEntries() == 0)
            maybeCache = nullptr;
    }

    
    MDispatchInstruction *dispatch;
    if (maybeCache) {
        dispatch = MTypeObjectDispatch::New(alloc(), maybeCache->object(), maybeCache->propTable());
        callInfo.fun()->setImplicitlyUsedUnchecked();
    } else {
        dispatch = MFunctionDispatch::New(alloc(), callInfo.fun());
    }

    
    jsbytecode *postCall = GetNextPc(pc);
    MBasicBlock *returnBlock = newBlock(nullptr, postCall);
    if (!returnBlock)
        return false;
    returnBlock->setCallerResumePoint(callerResumePoint_);

    
    returnBlock->inheritSlots(dispatchBlock);
    callInfo.popFormals(returnBlock);

    MPhi *retPhi = MPhi::New(alloc());
    returnBlock->addPhi(retPhi);
    returnBlock->push(retPhi);

    
    returnBlock->initEntrySlots(alloc());

    
    
    uint32_t count = 1; 
    for (uint32_t i = 0; i < targets.length(); i++) {
        if (choiceSet[i])
            count++;
    }
    retPhi->reserveLength(count);

    
    
    
    types::TemporaryTypeSet *cacheObjectTypeSet =
        maybeCache ? maybeCache->object()->resultTypeSet() : nullptr;

    
    JS_ASSERT(targets.length() == originals.length());
    for (uint32_t i = 0; i < targets.length(); i++) {
        
        
        
        JSFunction *original = &originals[i]->as<JSFunction>();
        JSFunction *target = &targets[i]->as<JSFunction>();

        
        if (!choiceSet[i])
            continue;

        
        if (maybeCache && !maybeCache->propTable()->hasFunction(original)) {
            choiceSet[i] = false;
            continue;
        }

        MBasicBlock *inlineBlock = newBlock(dispatchBlock, pc);
        if (!inlineBlock)
            return false;

        
        MConstant *funcDef = MConstant::New(alloc(), ObjectValue(*target), constraints());
        funcDef->setImplicitlyUsedUnchecked();
        dispatchBlock->add(funcDef);

        
        int funIndex = inlineBlock->entryResumePoint()->numOperands() - callInfo.numFormals();
        inlineBlock->entryResumePoint()->replaceOperand(funIndex, funcDef);
        inlineBlock->rewriteSlot(funIndex, funcDef);

        
        CallInfo inlineInfo(alloc(), callInfo.constructing());
        if (!inlineInfo.init(callInfo))
            return false;
        inlineInfo.popFormals(inlineBlock);
        inlineInfo.setFun(funcDef);

        if (maybeCache) {
            JS_ASSERT(callInfo.thisArg() == maybeCache->object());
            types::TemporaryTypeSet *targetThisTypes =
                maybeCache->propTable()->buildTypeSetForFunction(original);
            if (!targetThisTypes)
                return false;
            maybeCache->object()->setResultTypeSet(targetThisTypes);
        }

        
        if (!setCurrentAndSpecializePhis(inlineBlock))
            return false;
        InliningStatus status = inlineSingleCall(inlineInfo, target);
        if (status == InliningStatus_Error)
            return false;

        
        if (status == InliningStatus_NotInlined) {
            JS_ASSERT(target->isNative());
            JS_ASSERT(current == inlineBlock);
            graph().removeBlock(inlineBlock);
            choiceSet[i] = false;
            continue;
        }

        
        MBasicBlock *inlineReturnBlock = current;
        setCurrent(dispatchBlock);

        
        
        
        
        dispatch->addCase(original, inlineBlock);

        MDefinition *retVal = inlineReturnBlock->peek(-1);
        retPhi->addInput(retVal);
        inlineReturnBlock->end(MGoto::New(alloc(), returnBlock));
        if (!returnBlock->addPredecessorWithoutPhis(inlineReturnBlock))
            return false;
    }

    
    
    
    if (maybeCache) {
        maybeCache->object()->setResultTypeSet(cacheObjectTypeSet);

        InlinePropertyTable *propTable = maybeCache->propTable();
        propTable->trimTo(originals, choiceSet);

        
        if (propTable->numEntries() == 0) {
            JS_ASSERT(dispatch->numCases() == 0);
            maybeCache = nullptr;
        }
    }

    
    
    if (maybeCache || dispatch->numCases() < targets.length()) {
        
        if (maybeCache) {
            MBasicBlock *fallbackTarget;
            if (!inlineTypeObjectFallback(callInfo, dispatchBlock, (MTypeObjectDispatch *)dispatch,
                                          maybeCache, &fallbackTarget))
            {
                return false;
            }
            dispatch->addFallback(fallbackTarget);
        } else {
            JSFunction *remaining = nullptr;
            bool clonedAtCallsite = false;

            
            
            if (dispatch->numCases() + 1 == originals.length()) {
                for (uint32_t i = 0; i < originals.length(); i++) {
                    if (choiceSet[i])
                        continue;

                    remaining = &targets[i]->as<JSFunction>();
                    clonedAtCallsite = targets[i] != originals[i];
                    break;
                }
            }

            if (!inlineGenericFallback(remaining, callInfo, dispatchBlock, clonedAtCallsite))
                return false;
            dispatch->addFallback(current);
        }

        MBasicBlock *fallbackReturnBlock = current;

        
        MDefinition *retVal = fallbackReturnBlock->peek(-1);
        retPhi->addInput(retVal);
        fallbackReturnBlock->end(MGoto::New(alloc(), returnBlock));
        if (!returnBlock->addPredecessorWithoutPhis(fallbackReturnBlock))
            return false;
    }

    
    
    dispatchBlock->end(dispatch);

    
    JS_ASSERT(returnBlock->stackDepth() == dispatchBlock->stackDepth() - callInfo.numFormals() + 1);

    graph().moveBlockToEnd(returnBlock);
    return setCurrentAndSpecializePhis(returnBlock);
}

MInstruction *
IonBuilder::createDeclEnvObject(MDefinition *callee, MDefinition *scope)
{
    
    
    DeclEnvObject *templateObj = inspector->templateDeclEnvObject();

    
    
    JS_ASSERT(!templateObj->hasDynamicSlots());

    
    
    
    MInstruction *declEnvObj = MNewDeclEnvObject::New(alloc(), templateObj);
    current->add(declEnvObj);

    
    
    
    
    current->add(MStoreFixedSlot::New(alloc(), declEnvObj, DeclEnvObject::enclosingScopeSlot(), scope));
    current->add(MStoreFixedSlot::New(alloc(), declEnvObj, DeclEnvObject::lambdaSlot(), callee));

    return declEnvObj;
}

MInstruction *
IonBuilder::createCallObject(MDefinition *callee, MDefinition *scope)
{
    
    
    CallObject *templateObj = inspector->templateCallObject();

    
    
    MNullaryInstruction *callObj;
    if (script()->treatAsRunOnce())
        callObj = MNewRunOnceCallObject::New(alloc(), templateObj);
    else
        callObj = MNewCallObject::New(alloc(), templateObj);
    current->add(callObj);

    
    
    current->add(MStoreFixedSlot::New(alloc(), callObj, CallObject::enclosingScopeSlot(), scope));
    current->add(MStoreFixedSlot::New(alloc(), callObj, CallObject::calleeSlot(), callee));

    
    MSlots *slots = nullptr;
    for (AliasedFormalIter i(script()); i; i++) {
        unsigned slot = i.scopeSlot();
        unsigned formal = i.frameIndex();
        MDefinition *param = current->getSlot(info().argSlotUnchecked(formal));
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

MDefinition *
IonBuilder::createThisScripted(MDefinition *callee)
{
    
    
    
    
    
    
    
    
    
    
    
    MInstruction *getProto;
    if (!invalidatedIdempotentCache()) {
        MGetPropertyCache *getPropCache = MGetPropertyCache::New(alloc(), callee, names().prototype,
                                                                  false);
        getPropCache->setIdempotent();
        getProto = getPropCache;
    } else {
        MCallGetProperty *callGetProp = MCallGetProperty::New(alloc(), callee, names().prototype,
                                                               false);
        callGetProp->setIdempotent();
        getProto = callGetProp;
    }
    current->add(getProto);

    
    MCreateThisWithProto *createThis = MCreateThisWithProto::New(alloc(), callee, getProto);
    current->add(createThis);

    return createThis;
}

JSObject *
IonBuilder::getSingletonPrototype(JSFunction *target)
{
    if (!target || !target->hasSingletonType())
        return nullptr;
    types::TypeObjectKey *targetType = types::TypeObjectKey::get(target);
    if (targetType->unknownProperties())
        return nullptr;

    jsid protoid = NameToId(names().prototype);
    types::HeapTypeSetKey protoProperty = targetType->property(protoid);

    return protoProperty.singleton(constraints());
}

MDefinition *
IonBuilder::createThisScriptedSingleton(JSFunction *target, MDefinition *callee)
{
    
    JSObject *proto = getSingletonPrototype(target);
    if (!proto)
        return nullptr;

    JSObject *templateObject = inspector->getTemplateObject(pc);
    if (!templateObject || !templateObject->is<JSObject>())
        return nullptr;
    if (!templateObject->hasTenuredProto() || templateObject->getProto() != proto)
        return nullptr;

    if (!target->nonLazyScript()->types)
        return nullptr;
    if (!types::TypeScript::ThisTypes(target->nonLazyScript())->hasType(types::Type::ObjectType(templateObject)))
        return nullptr;

    
    
    MCreateThisWithTemplate *createThis =
        MCreateThisWithTemplate::New(alloc(), constraints(), templateObject,
                                     templateObject->type()->initialHeap(constraints()));
    current->add(createThis);

    return createThis;
}

MDefinition *
IonBuilder::createThis(JSFunction *target, MDefinition *callee)
{
    
    if (!target) {
        MCreateThis *createThis = MCreateThis::New(alloc(), callee);
        current->add(createThis);
        return createThis;
    }

    
    if (target->isNative()) {
        if (!target->isNativeConstructor())
            return nullptr;

        MConstant *magic = MConstant::New(alloc(), MagicValue(JS_IS_CONSTRUCTING));
        current->add(magic);
        return magic;
    }

    
    MDefinition *createThis = createThisScriptedSingleton(target, callee);
    if (createThis)
        return createThis;

    return createThisScripted(callee);
}

bool
IonBuilder::jsop_funcall(uint32_t argc)
{
    
    
    
    
    
    

    int calleeDepth = -((int)argc + 2);
    int funcDepth = -((int)argc + 1);

    
    types::TemporaryTypeSet *calleeTypes = current->peek(calleeDepth)->resultTypeSet();
    JSFunction *native = getSingleCallTarget(calleeTypes);
    if (!native || !native->isNative() || native->native() != &js_fun_call) {
        CallInfo callInfo(alloc(), false);
        if (!callInfo.init(current, argc))
            return false;
        return makeCall(native, callInfo, false);
    }
    current->peek(calleeDepth)->setImplicitlyUsedUnchecked();

    
    types::TemporaryTypeSet *funTypes = current->peek(funcDepth)->resultTypeSet();
    JSFunction *target = getSingleCallTarget(funTypes);

    
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
            break;
          case InliningDecision_Inline:
            if (target->isInterpreted())
                return inlineScriptedCall(callInfo, target);
            break;
        }
    }

    
    return makeCall(target, callInfo, false);
}

bool
IonBuilder::jsop_funapply(uint32_t argc)
{
    int calleeDepth = -((int)argc + 2);

    types::TemporaryTypeSet *calleeTypes = current->peek(calleeDepth)->resultTypeSet();
    JSFunction *native = getSingleCallTarget(calleeTypes);
    if (argc != 2 || info().executionMode() == ArgumentsUsageAnalysis) {
        CallInfo callInfo(alloc(), false);
        if (!callInfo.init(current, argc))
            return false;
        return makeCall(native, callInfo, false);
    }

    
    
    MDefinition *argument = current->peek(-1);
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
        return makeCall(native, callInfo, false);
    }

    if ((!native || !native->isNative() ||
        native->native() != js_fun_apply) &&
        info().executionMode() != DefinitePropertiesAnalysis)
    {
        return abort("fun.apply speculation failed");
    }

    
    return jsop_funapplyarguments(argc);
}

bool
IonBuilder::jsop_funapplyarguments(uint32_t argc)
{
    
    
    
    
    

    int funcDepth = -((int)argc + 1);

    
    types::TemporaryTypeSet *funTypes = current->peek(funcDepth)->resultTypeSet();
    JSFunction *target = getSingleCallTarget(funTypes);

    
    
    if (inliningDepth_ == 0 && info().executionMode() != DefinitePropertiesAnalysis) {
        
        
        
        
        MDefinition *vp = current->pop();
        vp->setImplicitlyUsedUnchecked();

        MDefinition *argThis = current->pop();

        
        MDefinition *argFunc = current->pop();

        
        MDefinition *nativeFunc = current->pop();
        nativeFunc->setImplicitlyUsedUnchecked();

        MArgumentsLength *numArgs = MArgumentsLength::New(alloc());
        current->add(numArgs);

        MApplyArgs *apply = MApplyArgs::New(alloc(), target, argFunc, numArgs, argThis);
        current->add(apply);
        current->push(apply);
        if (!resumeAfter(apply))
            return false;

        types::TemporaryTypeSet *types = bytecodeTypes(pc);
        return pushTypeBarrier(apply, types, BarrierKind::TypeSet);
    }

    
    
    
    
    

    CallInfo callInfo(alloc(), false);

    
    MDefinition *vp = current->pop();
    vp->setImplicitlyUsedUnchecked();

    
    MDefinitionVector args(alloc());
    if (inliningDepth_) {
        if (!args.appendAll(inlineCallInfo_->argv()))
            return false;
    }
    callInfo.setArgs(&args);

    
    MDefinition *argThis = current->pop();
    callInfo.setThis(argThis);

    
    MDefinition *argFunc = current->pop();
    callInfo.setFun(argFunc);

    
    MDefinition *nativeFunc = current->pop();
    nativeFunc->setImplicitlyUsedUnchecked();

    
    InliningDecision decision = makeInliningDecision(target, callInfo);
    switch (decision) {
      case InliningDecision_Error:
        return false;
      case InliningDecision_DontInline:
        break;
      case InliningDecision_Inline:
        if (target->isInterpreted())
            return inlineScriptedCall(callInfo, target);
    }

    return makeCall(target, callInfo, false);
}

bool
IonBuilder::jsop_call(uint32_t argc, bool constructing)
{
    
    
    types::TemporaryTypeSet *observed = bytecodeTypes(pc);
    if (observed->empty()) {
        if (BytecodeFlowsToBitop(pc)) {
            observed->addType(types::Type::Int32Type(), alloc_->lifoAlloc());
        } else if (*GetNextPc(pc) == JSOP_POS) {
            
            
            
            observed->addType(types::Type::DoubleType(), alloc_->lifoAlloc());
        }
    }

    int calleeDepth = -((int)argc + 2);

    
    ObjectVector originals(alloc());
    bool gotLambda = false;
    types::TemporaryTypeSet *calleeTypes = current->peek(calleeDepth)->resultTypeSet();
    if (calleeTypes) {
        if (!getPolyCallTargets(calleeTypes, constructing, originals, 4, &gotLambda))
            return false;
    }
    JS_ASSERT_IF(gotLambda, originals.length() <= 1);

    
    
    bool hasClones = false;
    ObjectVector targets(alloc());
    for (uint32_t i = 0; i < originals.length(); i++) {
        JSFunction *fun = &originals[i]->as<JSFunction>();
        if (fun->hasScript() && fun->nonLazyScript()->shouldCloneAtCallsite()) {
            if (JSFunction *clone = ExistingCloneFunctionAtCallsite(compartment->callsiteClones(), fun, script(), pc)) {
                fun = clone;
                hasClones = true;
            }
        }
        if (!targets.append(fun))
            return false;
    }

    CallInfo callInfo(alloc(), constructing);
    if (!callInfo.init(current, argc))
        return false;

    
    InliningStatus status = inlineCallsite(targets, originals, gotLambda, callInfo);
    if (status == InliningStatus_Inlined)
        return true;
    if (status == InliningStatus_Error)
        return false;

    
    JSFunction *target = nullptr;
    if (targets.length() == 1)
        target = &targets[0]->as<JSFunction>();

    return makeCall(target, callInfo, hasClones);
}

MDefinition *
IonBuilder::makeCallsiteClone(JSFunction *target, MDefinition *fun)
{
    
    
    
    
    if (target) {
        fun->setImplicitlyUsedUnchecked();
        return constant(ObjectValue(*target));
    }

    
    
    
    MCallsiteCloneCache *clone = MCallsiteCloneCache::New(alloc(), fun, pc);
    current->add(clone);
    return clone;
}

bool
IonBuilder::testShouldDOMCall(types::TypeSet *inTypes,
                              JSFunction *func, JSJitInfo::OpType opType)
{
    if (!func->isNative() || !func->jitInfo())
        return false;

    
    
    
    DOMInstanceClassHasProtoAtDepth instanceChecker =
        compartment->runtime()->DOMcallbacks()->instanceClassMatchesProto;

    const JSJitInfo *jinfo = func->jitInfo();
    if (jinfo->type() != opType)
        return false;

    for (unsigned i = 0; i < inTypes->getObjectCount(); i++) {
        types::TypeObjectKey *curType = inTypes->getObject(i);
        if (!curType)
            continue;

        if (!instanceChecker(curType->clasp(), jinfo->protoID, jinfo->depth))
            return false;
    }

    return true;
}

static bool
ArgumentTypesMatch(MDefinition *def, types::StackTypeSet *calleeTypes)
{
    if (def->resultTypeSet()) {
        JS_ASSERT(def->type() == MIRType_Value || def->mightBeType(def->type()));
        return def->resultTypeSet()->isSubset(calleeTypes);
    }

    if (def->type() == MIRType_Value)
        return false;

    if (def->type() == MIRType_Object)
        return calleeTypes->unknownObject();

    return calleeTypes->mightBeMIRType(def->type());
}

bool
IonBuilder::testNeedsArgumentCheck(JSFunction *target, CallInfo &callInfo)
{
    
    
    
    if (!target->hasScript())
        return true;

    JSScript *targetScript = target->nonLazyScript();

    if (!targetScript->types)
        return true;

    if (!ArgumentTypesMatch(callInfo.thisArg(), types::TypeScript::ThisTypes(targetScript)))
        return true;
    uint32_t expected_args = Min<uint32_t>(callInfo.argc(), target->nargs());
    for (size_t i = 0; i < expected_args; i++) {
        if (!ArgumentTypesMatch(callInfo.getArg(i), types::TypeScript::ArgTypes(targetScript, i)))
            return true;
    }
    for (size_t i = callInfo.argc(); i < target->nargs(); i++) {
        if (!types::TypeScript::ArgTypes(targetScript, i)->mightBeMIRType(MIRType_Undefined))
            return true;
    }

    return false;
}

MCall *
IonBuilder::makeCallHelper(JSFunction *target, CallInfo &callInfo, bool cloneAtCallsite)
{
    
    

    uint32_t targetArgs = callInfo.argc();

    
    
    if (target && !target->isNative())
        targetArgs = Max<uint32_t>(target->nargs(), callInfo.argc());

    bool isDOMCall = false;
    if (target && !callInfo.constructing()) {
        
        
        
        types::TemporaryTypeSet *thisTypes = callInfo.thisArg()->resultTypeSet();
        if (thisTypes &&
            thisTypes->getKnownMIRType() == MIRType_Object &&
            thisTypes->isDOMClass() &&
            testShouldDOMCall(thisTypes, target, JSJitInfo::Method))
        {
            isDOMCall = true;
        }
    }

    MCall *call = MCall::New(alloc(), target, targetArgs + 1, callInfo.argc(),
                             callInfo.constructing(), isDOMCall);
    if (!call)
        return nullptr;

    
    
    for (int i = targetArgs; i > (int)callInfo.argc(); i--) {
        JS_ASSERT_IF(target, !target->isNative());
        MConstant *undef = constant(UndefinedValue());
        call->addArg(i, undef);
    }

    
    
    for (int32_t i = callInfo.argc() - 1; i >= 0; i--)
        call->addArg(i + 1, callInfo.getArg(i));

    
    call->computeMovable();

    
    if (callInfo.constructing()) {
        MDefinition *create = createThis(target, callInfo.fun());
        if (!create) {
            abort("Failure inlining constructor for call.");
            return nullptr;
        }

        callInfo.thisArg()->setImplicitlyUsedUnchecked();
        callInfo.setThis(create);
    }

    
    MDefinition *thisArg = callInfo.thisArg();
    call->addArg(0, thisArg);

    
    
    if (cloneAtCallsite) {
        MDefinition *fun = makeCallsiteClone(target, callInfo.fun());
        callInfo.setFun(fun);
    }

    if (target && !testNeedsArgumentCheck(target, callInfo))
        call->disableArgCheck();

    call->initFunction(callInfo.fun());

    current->add(call);
    return call;
}

static bool
DOMCallNeedsBarrier(const JSJitInfo* jitinfo, types::TemporaryTypeSet *types)
{
    
    
    if (jitinfo->returnType() == JSVAL_TYPE_UNKNOWN)
        return true;

    
    
    if (jitinfo->returnType() == JSVAL_TYPE_OBJECT)
        return true;

    
    return MIRTypeFromValueType(jitinfo->returnType()) != types->getKnownMIRType();
}

bool
IonBuilder::makeCall(JSFunction *target, CallInfo &callInfo, bool cloneAtCallsite)
{
    
    
    JS_ASSERT_IF(callInfo.constructing() && target,
                 target->isInterpretedConstructor() || target->isNativeConstructor());

    MCall *call = makeCallHelper(target, callInfo, cloneAtCallsite);
    if (!call)
        return false;

    current->push(call);
    if (call->isEffectful() && !resumeAfter(call))
        return false;

    types::TemporaryTypeSet *types = bytecodeTypes(pc);

    if (call->isCallDOMNative())
        return pushDOMTypeBarrier(call, types, call->getSingleTarget());

    return pushTypeBarrier(call, types, BarrierKind::TypeSet);
}

bool
IonBuilder::jsop_eval(uint32_t argc)
{
    int calleeDepth = -((int)argc + 2);
    types::TemporaryTypeSet *calleeTypes = current->peek(calleeDepth)->resultTypeSet();

    
    
    if (calleeTypes && calleeTypes->empty())
        return jsop_call(argc,  false);

    JSFunction *singleton = getSingleCallTarget(calleeTypes);
    if (!singleton)
        return abort("No singleton callee for eval()");

    if (script()->global().valueIsEval(ObjectValue(*singleton))) {
        if (argc != 1)
            return abort("Direct eval with more than one argument");

        if (!info().funMaybeLazy())
            return abort("Direct eval in global code");

        
        
        
        
        MIRType type = thisTypes->getKnownMIRType();
        if (type != MIRType_Object && type != MIRType_Null && type != MIRType_Undefined)
            return abort("Direct eval from script with maybe-primitive 'this'");

        CallInfo callInfo(alloc(),  false);
        if (!callInfo.init(current, argc))
            return false;
        callInfo.setImplicitlyUsedUnchecked();

        callInfo.fun()->setImplicitlyUsedUnchecked();

        MDefinition *scopeChain = current->scopeChain();
        MDefinition *string = callInfo.getArg(0);

        
        
        if (!string->mightBeType(MIRType_String)) {
            current->push(string);
            types::TemporaryTypeSet *types = bytecodeTypes(pc);
            return pushTypeBarrier(string, types, BarrierKind::TypeSet);
        }

        current->pushSlot(info().thisSlot());
        MDefinition *thisValue = current->pop();

        
        
        
        if (string->isConcat() &&
            string->getOperand(1)->isConstant() &&
            string->getOperand(1)->toConstant()->value().isString())
        {
            JSAtom *atom = &string->getOperand(1)->toConstant()->value().toString()->asAtom();

            if (StringEqualsAscii(atom, "()")) {
                MDefinition *name = string->getOperand(0);
                MInstruction *dynamicName = MGetDynamicName::New(alloc(), scopeChain, name);
                current->add(dynamicName);

                current->push(dynamicName);
                current->push(thisValue);

                CallInfo evalCallInfo(alloc(),  false);
                if (!evalCallInfo.init(current,  0))
                    return false;

                return makeCall(nullptr, evalCallInfo, false);
            }
        }

        MInstruction *filterArguments = MFilterArgumentsOrEval::New(alloc(), string);
        current->add(filterArguments);

        MInstruction *ins = MCallDirectEval::New(alloc(), scopeChain, string, thisValue, pc);
        current->add(ins);
        current->push(ins);

        types::TemporaryTypeSet *types = bytecodeTypes(pc);
        return resumeAfter(ins) && pushTypeBarrier(ins, types, BarrierKind::TypeSet);
    }

    return jsop_call(argc,  false);
}

bool
IonBuilder::jsop_compare(JSOp op)
{
    MDefinition *right = current->pop();
    MDefinition *left = current->pop();

    MCompare *ins = MCompare::New(alloc(), left, right, op);
    current->add(ins);
    current->push(ins);

    ins->infer(inspector, pc);

    if (ins->isEffectful() && !resumeAfter(ins))
        return false;
    return true;
}

bool
IonBuilder::jsop_newarray(uint32_t count)
{
    JSObject *templateObject = inspector->getTemplateObject(pc);
    if (!templateObject)
        return abort("No template object for NEWARRAY");

    JS_ASSERT(templateObject->is<ArrayObject>());
    if (templateObject->type()->unknownProperties()) {
        
        
        return abort("New array has unknown properties");
    }

    MConstant *templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    current->add(templateConst);

    MNewArray *ins = MNewArray::New(alloc(), constraints(), count, templateConst,
                                    templateObject->type()->initialHeap(constraints()),
                                    NewArray_FullyAllocating);
    current->add(ins);
    current->push(ins);

    types::TemporaryTypeSet::DoubleConversion conversion =
        ins->resultTypeSet()->convertDoubleElements(constraints());

    if (conversion == types::TemporaryTypeSet::AlwaysConvertToDoubles)
        templateObject->setShouldConvertDoubleElements();
    else
        templateObject->clearShouldConvertDoubleElements();
    return true;
}

bool
IonBuilder::jsop_newarray_copyonwrite()
{
    JSObject *templateObject = types::GetCopyOnWriteObject(script(), pc);

    
    
    
    
    JS_ASSERT_IF(info().executionMode() != ArgumentsUsageAnalysis,
                 templateObject->type()->hasAnyFlags(types::OBJECT_FLAG_COPY_ON_WRITE));

    MNewArrayCopyOnWrite *ins =
        MNewArrayCopyOnWrite::New(alloc(), constraints(), templateObject,
                                  templateObject->type()->initialHeap(constraints()));

    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_newobject()
{
    JSObject *templateObject = inspector->getTemplateObject(pc);
    if (!templateObject)
        return abort("No template object for NEWOBJECT");

    JS_ASSERT(templateObject->is<JSObject>());
    MConstant *templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    current->add(templateConst);
    MNewObject *ins = MNewObject::New(alloc(), constraints(), templateConst,
                                      templateObject->hasSingletonType()
                                      ? gc::TenuredHeap
                                      : templateObject->type()->initialHeap(constraints()),
                                       false);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_initelem()
{
    MDefinition *value = current->pop();
    MDefinition *id = current->pop();
    MDefinition *obj = current->peek(-1);

    MInitElem *initElem = MInitElem::New(alloc(), obj, id, value);
    current->add(initElem);

    return resumeAfter(initElem);
}

bool
IonBuilder::jsop_initelem_array()
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->peek(-1);

    
    
    
    bool needStub = false;
    types::TypeObjectKey *initializer = obj->resultTypeSet()->getObject(0);
    if (value->type() == MIRType_MagicHole) {
        if (!initializer->hasFlags(constraints(), types::OBJECT_FLAG_NON_PACKED))
            needStub = true;
    } else if (!initializer->unknownProperties()) {
        types::HeapTypeSetKey elemTypes = initializer->property(JSID_VOID);
        if (!TypeSetIncludes(elemTypes.maybeTypes(), value->type(), value->resultTypeSet())) {
            elemTypes.freeze(constraints());
            needStub = true;
        }
    }

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    if (needStub) {
        MCallInitElementArray *store = MCallInitElementArray::New(alloc(), obj, GET_UINT24(pc), value);
        current->add(store);
        return resumeAfter(store);
    }

    MConstant *id = MConstant::New(alloc(), Int32Value(GET_UINT24(pc)));
    current->add(id);

    
    MElements *elements = MElements::New(alloc(), obj);
    current->add(elements);

    JSObject *templateObject = obj->toNewArray()->templateObject();

    if (templateObject->shouldConvertDoubleElements()) {
        MInstruction *valueDouble = MToDouble::New(alloc(), value);
        current->add(valueDouble);
        value = valueDouble;
    }

    
    MStoreElement *store = MStoreElement::New(alloc(), elements, id, value,  false);
    current->add(store);

    
    
    
    MSetInitializedLength *initLength = MSetInitializedLength::New(alloc(), elements, id);
    current->add(initLength);

    if (!resumeAfter(initLength))
        return false;

   return true;
}

bool
IonBuilder::jsop_mutateproto()
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->peek(-1);

    MMutateProto *mutate = MMutateProto::New(alloc(), obj, value);
    current->add(mutate);
    return resumeAfter(mutate);
}

bool
IonBuilder::jsop_initprop(PropertyName *name)
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->peek(-1);

    JSObject *templateObject = obj->toNewObject()->templateObject();

    Shape *shape = templateObject->lastProperty()->searchLinear(NameToId(name));

    if (!shape) {
        
        MInitProp *init = MInitProp::New(alloc(), obj, name, value);
        current->add(init);
        return resumeAfter(init);
    }

    if (PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current,
                                      &obj, name, &value,  true))
    {
        
        MInitProp *init = MInitProp::New(alloc(), obj, name, value);
        current->add(init);
        return resumeAfter(init);
    }

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    bool needsBarrier = true;
    if (obj->resultTypeSet() &&
        !obj->resultTypeSet()->propertyNeedsBarrier(constraints(), NameToId(name)))
    {
        needsBarrier = false;
    }

    
    
    if (info().executionMode() == ParallelExecution)
        needsBarrier = false;

    if (templateObject->isFixedSlot(shape->slot())) {
        MStoreFixedSlot *store = MStoreFixedSlot::New(alloc(), obj, shape->slot(), value);
        if (needsBarrier)
            store->setNeedsBarrier();

        current->add(store);
        return resumeAfter(store);
    }

    MSlots *slots = MSlots::New(alloc(), obj);
    current->add(slots);

    uint32_t slot = templateObject->dynamicSlotIndex(shape->slot());
    MStoreSlot *store = MStoreSlot::New(alloc(), slots, slot, value);
    if (needsBarrier)
        store->setNeedsBarrier();

    current->add(store);
    return resumeAfter(store);
}

bool
IonBuilder::jsop_initprop_getter_setter(PropertyName *name)
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->peek(-1);

    MInitPropGetterSetter *init = MInitPropGetterSetter::New(alloc(), obj, name, value);
    current->add(init);
    return resumeAfter(init);
}

bool
IonBuilder::jsop_initelem_getter_setter()
{
    MDefinition *value = current->pop();
    MDefinition *id = current->pop();
    MDefinition *obj = current->peek(-1);

    MInitElemGetterSetter *init = MInitElemGetterSetter::New(alloc(), obj, id, value);
    current->add(init);
    return resumeAfter(init);
}

MBasicBlock *
IonBuilder::addBlock(MBasicBlock *block, uint32_t loopDepth)
{
    if (!block)
        return nullptr;
    graph().addBlock(block);
    block->setLoopDepth(loopDepth);
    return block;
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::New(graph(), &analysis(), info(), predecessor,
                                          bytecodeSite(pc), MBasicBlock::NORMAL);
    return addBlock(block, loopDepth_);
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc, MResumePoint *priorResumePoint)
{
    MBasicBlock *block = MBasicBlock::NewWithResumePoint(graph(), info(), predecessor,
                                                         bytecodeSite(pc), priorResumePoint);
    return addBlock(block, loopDepth_);
}

MBasicBlock *
IonBuilder::newBlockPopN(MBasicBlock *predecessor, jsbytecode *pc, uint32_t popped)
{
    MBasicBlock *block = MBasicBlock::NewPopN(graph(), info(), predecessor, bytecodeSite(pc),
                                              MBasicBlock::NORMAL, popped);
    return addBlock(block, loopDepth_);
}

MBasicBlock *
IonBuilder::newBlockAfter(MBasicBlock *at, MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::New(graph(), &analysis(), info(), predecessor,
                                          bytecodeSite(pc), MBasicBlock::NORMAL);
    if (!block)
        return nullptr;
    graph().insertBlockAfter(at, block);
    return block;
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc, uint32_t loopDepth)
{
    MBasicBlock *block = MBasicBlock::New(graph(), &analysis(), info(), predecessor,
                                          bytecodeSite(pc), MBasicBlock::NORMAL);
    return addBlock(block, loopDepth);
}

MBasicBlock *
IonBuilder::newOsrPreheader(MBasicBlock *predecessor, jsbytecode *loopEntry)
{
    JS_ASSERT(LoopEntryCanIonOsr(loopEntry));
    JS_ASSERT(loopEntry == info().osrPc());

    
    
    
    MBasicBlock *osrBlock  = newBlockAfter(*graph().begin(), loopEntry);
    MBasicBlock *preheader = newBlock(predecessor, loopEntry);
    if (!osrBlock || !preheader)
        return nullptr;

    MOsrEntry *entry = MOsrEntry::New(alloc());
    osrBlock->add(entry);

    
    {
        uint32_t slot = info().scopeChainSlot();

        MInstruction *scopev;
        if (analysis().usesScopeChain()) {
            scopev = MOsrScopeChain::New(alloc(), entry);
        } else {
            
            
            
            scopev = MConstant::New(alloc(), UndefinedValue());
        }

        osrBlock->add(scopev);
        osrBlock->initSlot(slot, scopev);
    }
    
    {
        MInstruction *returnValue;
        if (!script()->noScriptRval())
            returnValue = MOsrReturnValue::New(alloc(), entry);
        else
            returnValue = MConstant::New(alloc(), UndefinedValue());
        osrBlock->add(returnValue);
        osrBlock->initSlot(info().returnValueSlot(), returnValue);
    }

    
    bool needsArgsObj = info().needsArgsObj();
    MInstruction *argsObj = nullptr;
    if (info().hasArguments()) {
        if (needsArgsObj)
            argsObj = MOsrArgumentsObject::New(alloc(), entry);
        else
            argsObj = MConstant::New(alloc(), UndefinedValue());
        osrBlock->add(argsObj);
        osrBlock->initSlot(info().argsObjSlot(), argsObj);
    }

    if (info().funMaybeLazy()) {
        
        MParameter *thisv = MParameter::New(alloc(), MParameter::THIS_SLOT, nullptr);
        osrBlock->add(thisv);
        osrBlock->initSlot(info().thisSlot(), thisv);

        
        for (uint32_t i = 0; i < info().nargs(); i++) {
            uint32_t slot = needsArgsObj ? info().argSlotUnchecked(i) : info().argSlot(i);

            
            
            
            
            if (needsArgsObj && info().argsObjAliasesFormals()) {
                JS_ASSERT(argsObj && argsObj->isOsrArgumentsObject());
                
                
                
                
                
                
                MInstruction *osrv;
                if (script()->formalIsAliased(i))
                    osrv = MConstant::New(alloc(), UndefinedValue());
                else
                    osrv = MGetArgumentsObjectArg::New(alloc(), argsObj, i);

                osrBlock->add(osrv);
                osrBlock->initSlot(slot, osrv);
            } else {
                MParameter *arg = MParameter::New(alloc(), i, nullptr);
                osrBlock->add(arg);
                osrBlock->initSlot(slot, arg);
            }
        }
    }

    
    for (uint32_t i = 0; i < info().nlocals(); i++) {
        uint32_t slot = info().localSlot(i);
        ptrdiff_t offset = BaselineFrame::reverseOffsetOfLocal(i);

        MOsrValue *osrv = MOsrValue::New(alloc(), entry, offset);
        osrBlock->add(osrv);
        osrBlock->initSlot(slot, osrv);
    }

    
    uint32_t numStackSlots = preheader->stackDepth() - info().firstStackSlot();
    for (uint32_t i = 0; i < numStackSlots; i++) {
        uint32_t slot = info().stackSlot(i);
        ptrdiff_t offset = BaselineFrame::reverseOffsetOfLocal(info().nlocals() + i);

        MOsrValue *osrv = MOsrValue::New(alloc(), entry, offset);
        osrBlock->add(osrv);
        osrBlock->initSlot(slot, osrv);
    }

    
    MStart *start = MStart::New(alloc(), MStart::StartType_Osr);
    osrBlock->add(start);

    
    
    if (!resumeAt(start, loopEntry))
        return nullptr;

    
    
    
    osrBlock->linkOsrValues(start);

    
    
    
    JS_ASSERT(predecessor->stackDepth() == osrBlock->stackDepth());
    JS_ASSERT(info().scopeChainSlot() == 0);

    
    
    
    
    for (uint32_t i = info().startArgSlot(); i < osrBlock->stackDepth(); i++) {
        MDefinition *existing = current->getSlot(i);
        MDefinition *def = osrBlock->getSlot(i);
        JS_ASSERT_IF(!needsArgsObj || !info().isSlotAliasedAtOsr(i), def->type() == MIRType_Value);

        
        
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

MBasicBlock *
IonBuilder::newPendingLoopHeader(MBasicBlock *predecessor, jsbytecode *pc, bool osr, bool canOsr,
                                 unsigned stackPhiCount)
{
    loopDepth_++;
    
    if (canOsr)
        stackPhiCount = predecessor->stackDepth() - info().firstStackSlot();
    MBasicBlock *block = MBasicBlock::NewPendingLoopHeader(graph(), info(), predecessor,
                                                           bytecodeSite(pc), stackPhiCount);
    if (!addBlock(block, loopDepth_))
        return nullptr;

    if (osr) {
        
        
        
        
        

        
        for (uint32_t i = info().startArgSlot(); i < block->stackDepth(); i++) {

            
            
            if (info().isSlotAliasedAtOsr(i))
                continue;

            
            
            if (i >= info().firstStackSlot())
                continue;

            MPhi *phi = block->getSlot(i)->toPhi();

            
            types::Type existingType = types::Type::UndefinedType();
            uint32_t arg = i - info().firstArgSlot();
            uint32_t var = i - info().firstLocalSlot();
            if (info().funMaybeLazy() && i == info().thisSlot())
                existingType = baselineFrame_->thisType;
            else if (arg < info().nargs())
                existingType = baselineFrame_->argTypes[arg];
            else
                existingType = baselineFrame_->varTypes[var];

            
            LifoAlloc *lifoAlloc = alloc().lifoAlloc();
            types::TemporaryTypeSet *typeSet =
                lifoAlloc->new_<types::TemporaryTypeSet>(lifoAlloc, existingType);
            if (!typeSet)
                return nullptr;
            MIRType type = typeSet->getKnownMIRType();
            if (!phi->addBackedgeType(type, typeSet))
                return nullptr;
        }
    }

    return block;
}

MTest *
IonBuilder::newTest(MDefinition *ins, MBasicBlock *ifTrue, MBasicBlock *ifFalse)
{
    MTest *test = MTest::New(alloc(), ins, ifTrue, ifFalse);
    test->cacheOperandMightEmulateUndefined();
    return test;
}
























bool
IonBuilder::resume(MInstruction *ins, jsbytecode *pc, MResumePoint::Mode mode)
{
    JS_ASSERT(ins->isEffectful() || !ins->isMovable());

    MResumePoint *resumePoint = MResumePoint::New(alloc(), ins->block(), pc, callerResumePoint_,
                                                  mode);
    if (!resumePoint)
        return false;
    ins->setResumePoint(resumePoint);
    return true;
}

bool
IonBuilder::resumeAt(MInstruction *ins, jsbytecode *pc)
{
    return resume(ins, pc, MResumePoint::ResumeAt);
}

bool
IonBuilder::resumeAfter(MInstruction *ins)
{
    return resume(ins, pc, MResumePoint::ResumeAfter);
}

bool
IonBuilder::maybeInsertResume()
{
    
    
    
    
    
    
    
    
    

    if (loopDepth_ == 0)
        return true;

    MNop *ins = MNop::New(alloc());
    current->add(ins);

    return resumeAfter(ins);
}

static bool
ClassHasEffectlessLookup(const Class *clasp, PropertyName *name)
{
    return clasp->isNative() && !clasp->ops.lookupGeneric;
}

static bool
ClassHasResolveHook(CompileCompartment *comp, const Class *clasp, PropertyName *name)
{
    
    
    
    if (clasp == &ArrayObject::class_)
        return name == comp->runtime()->names().length;

    if (clasp->resolve == JS_ResolveStub)
        return false;

    if (clasp->resolve == (JSResolveOp)str_resolve) {
        
        return false;
    }

    if (clasp->resolve == (JSResolveOp)fun_resolve)
        return FunctionHasResolveHook(comp->runtime()->names(), name);

    return true;
}

void
IonBuilder::insertRecompileCheck()
{
    
    if (info().executionMode() != SequentialExecution)
        return;

    
    OptimizationLevel curLevel = optimizationInfo().level();
    if (js_IonOptimizations.isLastLevel(curLevel))
        return;

    

    
    
    IonBuilder *topBuilder = this;
    while (topBuilder->callerBuilder_)
        topBuilder = topBuilder->callerBuilder_;

    
    
    OptimizationLevel nextLevel = js_IonOptimizations.nextLevel(curLevel);
    const OptimizationInfo *info = js_IonOptimizations.get(nextLevel);
    uint32_t warmUpCounter = info->usesBeforeCompile(topBuilder->script());
    current->add(MRecompileCheck::New(alloc(), topBuilder->script(), warmUpCounter));
}

JSObject *
IonBuilder::testSingletonProperty(JSObject *obj, PropertyName *name)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    while (obj) {
        if (!ClassHasEffectlessLookup(obj->getClass(), name))
            return nullptr;

        types::TypeObjectKey *objType = types::TypeObjectKey::get(obj);
        if (analysisContext)
            objType->ensureTrackedProperty(analysisContext, NameToId(name));

        if (objType->unknownProperties())
            return nullptr;

        types::HeapTypeSetKey property = objType->property(NameToId(name));
        if (property.isOwnProperty(constraints())) {
            if (obj->hasSingletonType())
                return property.singleton(constraints());
            return nullptr;
        }

        if (ClassHasResolveHook(compartment, obj->getClass(), name))
            return nullptr;

        if (!obj->hasTenuredProto())
            return nullptr;
        obj = obj->getProto();
    }

    return nullptr;
}

bool
IonBuilder::testSingletonPropertyTypes(MDefinition *obj, JSObject *singleton, PropertyName *name,
                                       bool *testObject, bool *testString)
{
    
    
    

    *testObject = false;
    *testString = false;

    types::TemporaryTypeSet *types = obj->resultTypeSet();
    if (types && types->unknownObject())
        return false;

    JSObject *objectSingleton = types ? types->getSingleton() : nullptr;
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

        if (types->hasType(types::Type::StringType())) {
            key = JSProto_String;
            *testString = true;
            break;
        }

        if (!types->maybeObject())
            return false;

        
        
        
        for (unsigned i = 0; i < types->getObjectCount(); i++) {
            types::TypeObjectKey *object = types->getObject(i);
            if (!object)
                continue;
            if (analysisContext)
                object->ensureTrackedProperty(analysisContext, NameToId(name));

            const Class *clasp = object->clasp();
            if (!ClassHasEffectlessLookup(clasp, name) || ClassHasResolveHook(compartment, clasp, name))
                return false;
            if (object->unknownProperties())
                return false;
            types::HeapTypeSetKey property = object->property(NameToId(name));
            if (property.isOwnProperty(constraints()))
                return false;

            if (!object->hasTenuredProto())
                return false;
            if (JSObject *proto = object->proto().toObjectOrNull()) {
                
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

    JSObject *proto = GetBuiltinPrototypePure(&script()->global(), key);
    if (proto)
        return testSingletonProperty(proto, name) == singleton;

    return false;
}












bool
IonBuilder::pushTypeBarrier(MDefinition *def, types::TemporaryTypeSet *observed, BarrierKind kind)
{
    
    if (BytecodeIsPopped(pc))
        return true;

    
    
    
    
    

    if (kind == BarrierKind::NoBarrier) {
        MDefinition *replace = ensureDefiniteType(def, observed->getKnownMIRType());
        if (replace != def) {
            current->pop();
            current->push(replace);
        }
        replace->setResultTypeSet(observed);
        return true;
    }

    if (observed->unknown())
        return true;

    current->pop();

    MInstruction *barrier = MTypeBarrier::New(alloc(), def, observed, kind);
    current->add(barrier);

    if (barrier->type() == MIRType_Undefined)
        return pushConstant(UndefinedValue());
    if (barrier->type() == MIRType_Null)
        return pushConstant(NullValue());

    current->push(barrier);
    return true;
}

bool
IonBuilder::pushDOMTypeBarrier(MInstruction *ins, types::TemporaryTypeSet *observed, JSFunction* func)
{
    JS_ASSERT(func && func->isNative() && func->jitInfo());

    const JSJitInfo *jitinfo = func->jitInfo();
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
        JS_ASSERT(barrier);
    }

    return pushTypeBarrier(replace, observed,
                           barrier ? BarrierKind::TypeSet : BarrierKind::NoBarrier);
}

MDefinition *
IonBuilder::ensureDefiniteType(MDefinition *def, MIRType definiteType)
{
    MInstruction *replace;
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
            JS_ASSERT(def->type() == definiteType);
            return def;
        }
        replace = MUnbox::New(alloc(), def, definiteType, MUnbox::Infallible);
        break;
      }
    }

    current->add(replace);
    return replace;
}

MDefinition *
IonBuilder::ensureDefiniteTypeSet(MDefinition *def, types::TemporaryTypeSet *types)
{
    
    

    
    
    MDefinition *replace = ensureDefiniteType(def, types->getKnownMIRType());
    if (replace != def) {
        replace->setResultTypeSet(types);
        return replace;
    }

    
    if (def->type() != types->getKnownMIRType()) {
        MOZ_ASSERT(types->getKnownMIRType() == MIRType_Value);
        return def;
    }

    
    MFilterTypeSet *filter = MFilterTypeSet::New(alloc(), def, types);
    current->add(filter);
    return filter;
}

static size_t
NumFixedSlots(JSObject *object)
{
    
    
    
    
    gc::AllocKind kind = object->tenuredGetAllocKind();
    return gc::GetGCKindSlots(kind, object->getClass());
}

bool
IonBuilder::getStaticName(JSObject *staticObject, PropertyName *name, bool *psucceeded)
{
    jsid id = NameToId(name);

    JS_ASSERT(staticObject->is<GlobalObject>() || staticObject->is<CallObject>());
    JS_ASSERT(staticObject->hasSingletonType());

    *psucceeded = true;

    if (staticObject->is<GlobalObject>()) {
        
        if (name == names().undefined)
            return pushConstant(UndefinedValue());
        if (name == names().NaN)
            return pushConstant(compartment->runtime()->NaNValue());
        if (name == names().Infinity)
            return pushConstant(compartment->runtime()->positiveInfinityValue());
    }

    types::TypeObjectKey *staticType = types::TypeObjectKey::get(staticObject);
    if (analysisContext)
        staticType->ensureTrackedProperty(analysisContext, NameToId(name));

    if (staticType->unknownProperties()) {
        *psucceeded = false;
        return true;
    }

    types::HeapTypeSetKey property = staticType->property(id);
    if (!property.maybeTypes() ||
        !property.maybeTypes()->definiteProperty() ||
        property.nonData(constraints()))
    {
        
        
        *psucceeded = false;
        return true;
    }

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(), staticType,
                                                       name, types,  true);

    JSObject *singleton = types->getSingleton();

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

    MInstruction *obj = constant(ObjectValue(*staticObject));

    MIRType rvalType = types->getKnownMIRType();
    if (barrier != BarrierKind::NoBarrier)
        rvalType = MIRType_Value;

    return loadSlot(obj, property.maybeTypes()->definiteSlot(), NumFixedSlots(staticObject),
                    rvalType, barrier, types);
}


bool
jit::TypeSetIncludes(types::TypeSet *types, MIRType input, types::TypeSet *inputTypes)
{
    if (!types)
        return inputTypes && inputTypes->empty();

    switch (input) {
      case MIRType_Undefined:
      case MIRType_Null:
      case MIRType_Boolean:
      case MIRType_Int32:
      case MIRType_Double:
      case MIRType_Float32:
      case MIRType_String:
      case MIRType_Symbol:
      case MIRType_MagicOptimizedArguments:
        return types->hasType(types::Type::PrimitiveType(ValueTypeFromMIRType(input)));

      case MIRType_Object:
        return types->unknownObject() || (inputTypes && inputTypes->isSubset(types));

      case MIRType_Value:
        return types->unknown() || (inputTypes && inputTypes->isSubset(types));

      default:
        MOZ_CRASH("Bad input type");
    }
}


bool
jit::NeedsPostBarrier(CompileInfo &info, MDefinition *value)
{
#ifdef JSGC_GENERATIONAL
    if (!GetIonContext()->runtime->gcNursery().exists())
        return false;
#endif
    return info.executionMode() != ParallelExecution && value->mightBeType(MIRType_Object);
}

bool
IonBuilder::setStaticName(JSObject *staticObject, PropertyName *name)
{
    jsid id = NameToId(name);

    JS_ASSERT(staticObject->is<GlobalObject>() || staticObject->is<CallObject>());

    MDefinition *value = current->peek(-1);

    types::TypeObjectKey *staticType = types::TypeObjectKey::get(staticObject);
    if (staticType->unknownProperties())
        return jsop_setprop(name);

    types::HeapTypeSetKey property = staticType->property(id);
    if (!property.maybeTypes() ||
        !property.maybeTypes()->definiteProperty() ||
        property.nonData(constraints()) ||
        property.nonWritable(constraints()))
    {
        
        
        return jsop_setprop(name);
    }

    if (!CanWriteProperty(constraints(), property, value))
        return jsop_setprop(name);

    current->pop();

    
    MDefinition *obj = current->pop();
    JS_ASSERT(&obj->toConstant()->value().toObject() == staticObject);

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
IonBuilder::jsop_getgname(PropertyName *name)
{
    JSObject *obj = &script()->global();
    bool succeeded;
    if (!getStaticName(obj, name, &succeeded))
        return false;
    if (succeeded)
        return true;

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    MDefinition *globalObj = constant(ObjectValue(*obj));
    if (!getPropTryCommonGetter(&succeeded, globalObj, name, types))
        return false;
    if (succeeded)
        return true;

    return jsop_getname(name);
}

bool
IonBuilder::jsop_getname(PropertyName *name)
{
    MDefinition *object;
    if (js_CodeSpec[*pc].format & JOF_GNAME) {
        MInstruction *global = constant(ObjectValue(script()->global()));
        object = global;
    } else {
        current->push(current->scopeChain());
        object = current->pop();
    }

    MGetNameCache *ins;
    if (JSOp(*GetNextPc(pc)) == JSOP_TYPEOF)
        ins = MGetNameCache::New(alloc(), object, name, MGetNameCache::NAMETYPEOF);
    else
        ins = MGetNameCache::New(alloc(), object, name, MGetNameCache::NAME);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    return pushTypeBarrier(ins, types, BarrierKind::TypeSet);
}

bool
IonBuilder::jsop_intrinsic(PropertyName *name)
{
    types::TemporaryTypeSet *types = bytecodeTypes(pc);

    
    
    if (types->empty()) {
        MCallGetIntrinsicValue *ins = MCallGetIntrinsicValue::New(alloc(), name);

        current->add(ins);
        current->push(ins);

        if (!resumeAfter(ins))
            return false;

        return pushTypeBarrier(ins, types, BarrierKind::TypeSet);
    }

    
    Value vp;
    JS_ALWAYS_TRUE(script()->global().maybeGetIntrinsicValue(name, &vp));
    JS_ASSERT(types->hasType(types::GetValueType(vp)));

    pushConstant(vp);
    return true;
}

bool
IonBuilder::jsop_bindname(PropertyName *name)
{
    JS_ASSERT(analysis().usesScopeChain());

    MDefinition *scopeChain = current->scopeChain();
    MBindNameCache *ins = MBindNameCache::New(alloc(), scopeChain, name, script(), pc);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

static MIRType
GetElemKnownType(bool needsHoleCheck, types::TemporaryTypeSet *types)
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
    MDefinition *index = current->pop();
    MDefinition *obj = current->pop();

    
    
    if (info().executionModeIsAnalysis()) {
        MInstruction *ins = MCallGetElement::New(alloc(), obj, index);

        current->add(ins);
        current->push(ins);

        if (!resumeAfter(ins))
            return false;

        types::TemporaryTypeSet *types = bytecodeTypes(pc);
        return pushTypeBarrier(ins, types, BarrierKind::TypeSet);
    }

    bool emitted = false;

    if (!getElemTryTypedObject(&emitted, obj, index) || emitted)
        return emitted;

    if (!getElemTryDense(&emitted, obj, index) || emitted)
        return emitted;

    if (!getElemTryTypedStatic(&emitted, obj, index) || emitted)
        return emitted;

    if (!getElemTryTypedArray(&emitted, obj, index) || emitted)
        return emitted;

    if (!getElemTryString(&emitted, obj, index) || emitted)
        return emitted;

    if (!getElemTryArguments(&emitted, obj, index) || emitted)
        return emitted;

    if (!getElemTryArgumentsInlined(&emitted, obj, index) || emitted)
        return emitted;

    if (script()->argumentsHasVarBinding() && obj->mightBeType(MIRType_MagicOptimizedArguments))
        return abort("Type is not definitely lazy arguments.");

    if (!getElemTryCache(&emitted, obj, index) || emitted)
        return emitted;

    
    MInstruction *ins = MCallGetElement::New(alloc(), obj, index);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    return pushTypeBarrier(ins, types, BarrierKind::TypeSet);
}

bool
IonBuilder::getElemTryTypedObject(bool *emitted, MDefinition *obj, MDefinition *index)
{
    JS_ASSERT(*emitted == false);

    TypedObjectPrediction objPrediction = typedObjectPrediction(obj);
    if (objPrediction.isUseless())
        return true;

    if (!objPrediction.ofArrayKind())
        return true;

    TypedObjectPrediction elemPrediction = objPrediction.arrayElementType();
    if (elemPrediction.isUseless())
        return true;

    JS_ASSERT(TypeDescr::isSized(elemPrediction.kind()));

    int32_t elemSize;
    if (!elemPrediction.hasKnownSize(&elemSize))
        return true;

    switch (elemPrediction.kind()) {
      case type::X4:
        
        return true;

      case type::Struct:
      case type::SizedArray:
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
        return true;

      case type::UnsizedArray:
        MOZ_CRASH("Unsized arrays cannot be element types");
    }

    MOZ_CRASH("Bad kind");
}

static MIRType
MIRTypeForTypedArrayRead(Scalar::Type arrayType, bool observedDouble);

bool
IonBuilder::checkTypedObjectIndexInBounds(int32_t elemSize,
                                          MDefinition *obj,
                                          MDefinition *index,
                                          TypedObjectPrediction objPrediction,
                                          MDefinition **indexAsByteOffset,
                                          bool *canBeNeutered)
{
    
    MInstruction *idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);

    
    
    
    
    int32_t lenOfAll;
    MDefinition *length;
    if (objPrediction.hasKnownArrayLength(&lenOfAll)) {
        length = constantInt(lenOfAll);

        
        
        *canBeNeutered = true;
    } else {
        MInstruction *lengthValue = MLoadFixedSlot::New(alloc(), obj, JS_BUFVIEW_SLOT_LENGTH);
        current->add(lengthValue);

        MInstruction *length32 = MTruncateToInt32::New(alloc(), lengthValue);
        current->add(length32);

        length = length32;

        
        
        
        *canBeNeutered = false;
    }

    index = addBoundsCheck(idInt32, length);

    
    
    MMul *mul = MMul::New(alloc(), index, constantInt(elemSize),
                          MIRType_Int32, MMul::Integer);
    current->add(mul);

    *indexAsByteOffset = mul;
    return true;
}

bool
IonBuilder::getElemTryScalarElemOfTypedObject(bool *emitted,
                                              MDefinition *obj,
                                              MDefinition *index,
                                              TypedObjectPrediction objPrediction,
                                              TypedObjectPrediction elemPrediction,
                                              int32_t elemSize)
{
    JS_ASSERT(objPrediction.ofArrayKind());

    
    ScalarTypeDescr::Type elemType = elemPrediction.scalarType();
    JS_ASSERT(elemSize == ScalarTypeDescr::alignment(elemType));

    bool canBeNeutered;
    MDefinition *indexAsByteOffset;
    if (!checkTypedObjectIndexInBounds(elemSize, obj, index, objPrediction,
                                       &indexAsByteOffset, &canBeNeutered))
    {
        return false;
    }

    return pushScalarLoadFromTypedObject(emitted, obj, indexAsByteOffset, elemType, canBeNeutered);
}

bool
IonBuilder::pushScalarLoadFromTypedObject(bool *emitted,
                                          MDefinition *obj,
                                          MDefinition *offset,
                                          ScalarTypeDescr::Type elemType,
                                          bool canBeNeutered)
{
    int32_t size = ScalarTypeDescr::size(elemType);
    JS_ASSERT(size == ScalarTypeDescr::alignment(elemType));

    
    MDefinition *elements, *scaledOffset;
    loadTypedObjectElements(obj, offset, size, canBeNeutered,
                            &elements, &scaledOffset);

    
    MLoadTypedArrayElement *load = MLoadTypedArrayElement::New(alloc(), elements, scaledOffset, elemType);
    current->add(load);
    current->push(load);

    
    
    
    
    types::TemporaryTypeSet *resultTypes = bytecodeTypes(pc);
    bool allowDouble = resultTypes->hasType(types::Type::DoubleType());

    
    
    MIRType knownType = MIRTypeForTypedArrayRead(elemType, allowDouble);

    
    
    
    
    load->setResultType(knownType);

    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryComplexElemOfTypedObject(bool *emitted,
                                               MDefinition *obj,
                                               MDefinition *index,
                                               TypedObjectPrediction objPrediction,
                                               TypedObjectPrediction elemPrediction,
                                               int32_t elemSize)
{
    JS_ASSERT(objPrediction.ofArrayKind());

    MDefinition *type = loadTypedObjectType(obj);
    MDefinition *elemTypeObj = typeObjectForElementFromArrayStructType(type);

    bool canBeNeutered;
    MDefinition *indexAsByteOffset;
    if (!checkTypedObjectIndexInBounds(elemSize, obj, index, objPrediction,
                                       &indexAsByteOffset, &canBeNeutered))
    {
        return false;
    }

    return pushDerivedTypedObject(emitted, obj, indexAsByteOffset,
                                  elemPrediction, elemTypeObj, canBeNeutered);
}

bool
IonBuilder::pushDerivedTypedObject(bool *emitted,
                                   MDefinition *obj,
                                   MDefinition *offset,
                                   TypedObjectPrediction derivedPrediction,
                                   MDefinition *derivedTypeObj,
                                   bool canBeNeutered)
{
    
    MDefinition *owner, *ownerOffset;
    loadTypedObjectData(obj, offset, canBeNeutered, &owner, &ownerOffset);

    
    MInstruction *derivedTypedObj = MNewDerivedTypedObject::New(alloc(),
                                                                derivedPrediction,
                                                                derivedTypeObj,
                                                                owner,
                                                                ownerOffset);
    current->add(derivedTypedObj);
    current->push(derivedTypedObj);

    
    
    
    
    
    types::TemporaryTypeSet *objTypes = obj->resultTypeSet();
    const Class *expectedClass = objTypes ? objTypes->getKnownClass() : nullptr;
    const TypedProto *expectedProto = derivedPrediction.getKnownPrototype();
    JS_ASSERT_IF(expectedClass, IsTypedObjectClass(expectedClass));

    
    
    types::TemporaryTypeSet *observedTypes = bytecodeTypes(pc);
    const Class *observedClass = observedTypes->getKnownClass();
    JSObject *observedProto = observedTypes->getCommonPrototype();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (observedClass && observedProto && observedClass == expectedClass &&
        observedProto == expectedProto)
    {
        derivedTypedObj->setResultTypeSet(observedTypes);
    } else {
        if (!pushTypeBarrier(derivedTypedObj, observedTypes, BarrierKind::TypeSet))
            return false;
    }

    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryDense(bool *emitted, MDefinition *obj, MDefinition *index)
{
    JS_ASSERT(*emitted == false);

    if (!ElementAccessIsDenseNative(obj, index))
        return true;

    
    
    if (ElementAccessHasExtraIndexedProperty(constraints(), obj) && failedBoundsCheck_)
        return true;

    
    
    if (inspector->hasSeenNegativeIndexGetElement(pc))
        return true;

    
    if (!jsop_getelem_dense(obj, index))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryTypedStatic(bool *emitted, MDefinition *obj, MDefinition *index)
{
    JS_ASSERT(*emitted == false);

    Scalar::Type arrayType;
    if (!ElementAccessIsTypedArray(obj, index, &arrayType))
        return true;

    if (!LIRGenerator::allowStaticTypedArrayAccesses())
        return true;

    if (ElementAccessHasExtraIndexedProperty(constraints(), obj))
        return true;

    if (!obj->resultTypeSet())
        return true;

    JSObject *tarrObj = obj->resultTypeSet()->getSingleton();
    if (!tarrObj)
        return true;

    TypedArrayObject *tarr = &tarrObj->as<TypedArrayObject>();

    types::TypeObjectKey *tarrType = types::TypeObjectKey::get(tarr);
    if (tarrType->unknownProperties())
        return true;

    
    Scalar::Type viewType = tarr->type();
    if (viewType == Scalar::Uint32)
        return true;

    MDefinition *ptr = convertShiftToMaskForStaticTypedArray(index, viewType);
    if (!ptr)
        return true;

    
    tarrType->watchStateChangeForTypedArrayData(constraints());

    obj->setImplicitlyUsedUnchecked();
    index->setImplicitlyUsedUnchecked();

    MLoadTypedArrayElementStatic *load = MLoadTypedArrayElementStatic::New(alloc(), tarr, ptr);
    current->add(load);
    current->push(load);

    
    
    
    
    
    if (viewType == Scalar::Float32 || viewType == Scalar::Float64) {
        jsbytecode *next = pc + JSOP_GETELEM_LENGTH;
        if (*next == JSOP_POS)
            load->setInfallible();
    } else {
        jsbytecode *next = pc + JSOP_GETELEM_LENGTH;
        if (*next == JSOP_ZERO && *(next + JSOP_ZERO_LENGTH) == JSOP_BITOR)
            load->setInfallible();
    }

    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryTypedArray(bool *emitted, MDefinition *obj, MDefinition *index)
{
    JS_ASSERT(*emitted == false);

    Scalar::Type arrayType;
    if (!ElementAccessIsTypedArray(obj, index, &arrayType))
        return true;

    
    if (!jsop_getelem_typed(obj, index, arrayType))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryString(bool *emitted, MDefinition *obj, MDefinition *index)
{
    JS_ASSERT(*emitted == false);

    if (obj->type() != MIRType_String || !IsNumberType(index->type()))
        return true;

    
    
    if (bytecodeTypes(pc)->hasType(types::Type::UndefinedType()))
        return true;

    
    MInstruction *idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    MStringLength *length = MStringLength::New(alloc(), obj);
    current->add(length);

    index = addBoundsCheck(index, length);

    MCharCodeAt *charCode = MCharCodeAt::New(alloc(), obj, index);
    current->add(charCode);

    MFromCharCode *result = MFromCharCode::New(alloc(), charCode);
    current->add(result);
    current->push(result);

    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryArguments(bool *emitted, MDefinition *obj, MDefinition *index)
{
    JS_ASSERT(*emitted == false);

    if (inliningDepth_ > 0)
        return true;

    if (obj->type() != MIRType_MagicOptimizedArguments)
        return true;

    

    JS_ASSERT(!info().argsObjAliasesFormals());

    
    obj->setImplicitlyUsedUnchecked();

    
    MArgumentsLength *length = MArgumentsLength::New(alloc());
    current->add(length);

    
    MInstruction *idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    
    index = addBoundsCheck(index, length);

    
    MGetFrameArgument *load = MGetFrameArgument::New(alloc(), index, analysis_.hasSetArg());
    current->add(load);
    current->push(load);

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    if (!pushTypeBarrier(load, types, BarrierKind::TypeSet))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::getElemTryArgumentsInlined(bool *emitted, MDefinition *obj, MDefinition *index)
{
    JS_ASSERT(*emitted == false);

    if (inliningDepth_ == 0)
        return true;

    if (obj->type() != MIRType_MagicOptimizedArguments)
        return true;

    
    obj->setImplicitlyUsedUnchecked();

    JS_ASSERT(!info().argsObjAliasesFormals());

    
    if (index->isConstant() && index->toConstant()->value().isInt32()) {
        JS_ASSERT(inliningDepth_ > 0);

        int32_t id = index->toConstant()->value().toInt32();
        index->setImplicitlyUsedUnchecked();

        if (id < (int32_t)inlineCallInfo_->argc() && id >= 0)
            current->push(inlineCallInfo_->getArg(id));
        else
            pushConstant(UndefinedValue());

        *emitted = true;
        return true;
    }

    
    return abort("NYI inlined not constant get argument element");
}

bool
IonBuilder::getElemTryCache(bool *emitted, MDefinition *obj, MDefinition *index)
{
    JS_ASSERT(*emitted == false);

    
    if (!obj->mightBeType(MIRType_Object))
        return true;

    
    if (obj->mightBeType(MIRType_String))
        return true;

    
    if (!index->mightBeType(MIRType_Int32) &&
        !index->mightBeType(MIRType_String) &&
        !index->mightBeType(MIRType_Symbol))
    {
        return true;
    }

    
    
    bool nonNativeGetElement = inspector->hasSeenNonNativeGetElement(pc);
    if (index->mightBeType(MIRType_Int32) && nonNativeGetElement)
        return true;

    

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(), obj,
                                                       nullptr, types);

    
    
    if (index->mightBeType(MIRType_String) || index->mightBeType(MIRType_Symbol))
        barrier = BarrierKind::TypeSet;

    
    if (needsToMonitorMissingProperties(types))
        barrier = BarrierKind::TypeSet;

    MInstruction *ins = MGetElementCache::New(alloc(), obj, index, barrier != BarrierKind::NoBarrier);

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

    *emitted = true;
    return true;
}

bool
IonBuilder::jsop_getelem_dense(MDefinition *obj, MDefinition *index)
{
    types::TemporaryTypeSet *types = bytecodeTypes(pc);

    MOZ_ASSERT(index->type() == MIRType_Int32 || index->type() == MIRType_Double);
    if (JSOp(*pc) == JSOP_CALLELEM) {
        
        
        
        AddObjectsForPropertyRead(obj, nullptr, types);
    }

    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(), obj,
                                                       nullptr, types);
    bool needsHoleCheck = !ElementAccessIsPacked(constraints(), obj);

    
    
    
    bool readOutOfBounds =
        types->hasType(types::Type::UndefinedType()) &&
        !ElementAccessHasExtraIndexedProperty(constraints(), obj);

    MIRType knownType = MIRType_Value;
    if (barrier == BarrierKind::NoBarrier)
        knownType = GetElemKnownType(needsHoleCheck, types);

    
    MInstruction *idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    
    MInstruction *elements = MElements::New(alloc(), obj);
    current->add(elements);

    
    
    
    MInitializedLength *initLength = MInitializedLength::New(alloc(), elements);
    current->add(initLength);

    
    
    
    
    
    
    types::TemporaryTypeSet *objTypes = obj->resultTypeSet();
    ExecutionMode executionMode = info().executionMode();
    bool loadDouble =
        executionMode == SequentialExecution &&
        barrier == BarrierKind::NoBarrier &&
        loopDepth_ &&
        !readOutOfBounds &&
        !needsHoleCheck &&
        knownType == MIRType_Double &&
        objTypes &&
        objTypes->convertDoubleElements(constraints()) == types::TemporaryTypeSet::AlwaysConvertToDoubles;
    if (loadDouble)
        elements = addConvertElementsToDoubles(elements);

    MInstruction *load;

    if (!readOutOfBounds) {
        
        
        
        
        index = addBoundsCheck(index, initLength);

        load = MLoadElement::New(alloc(), elements, index, needsHoleCheck, loadDouble);
        current->add(load);
    } else {
        
        
        
        load = MLoadElementHole::New(alloc(), elements, index, initLength, needsHoleCheck);
        current->add(load);

        
        
        
        JS_ASSERT(knownType == MIRType_Value);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (executionMode == ParallelExecution &&
        barrier != BarrierKind::NoBarrier &&
        types->getKnownMIRType() == MIRType_Int32 &&
        objTypes &&
        objTypes->convertDoubleElements(constraints()) == types::TemporaryTypeSet::AlwaysConvertToDoubles)
    {
        
        LifoAlloc *lifoAlloc = alloc().lifoAlloc();
        types = lifoAlloc->new_<types::TemporaryTypeSet>(lifoAlloc, types::Type::DoubleType());
        if (!types)
            return false;

        barrier = BarrierKind::NoBarrier; 
    }

    if (knownType != MIRType_Value)
        load->setResultType(knownType);

    current->push(load);
    return pushTypeBarrier(load, types, barrier);
}

void
IonBuilder::addTypedArrayLengthAndData(MDefinition *obj,
                                       BoundsChecking checking,
                                       MDefinition **index,
                                       MInstruction **length, MInstruction **elements)
{
    MOZ_ASSERT((index != nullptr) == (elements != nullptr));

    if (obj->isConstant() && obj->toConstant()->value().isObject()) {
        TypedArrayObject *tarr = &obj->toConstant()->value().toObject().as<TypedArrayObject>();
        void *data = tarr->viewData();
        
        
#ifdef JSGC_GENERATIONAL
        bool isTenured = !tarr->runtimeFromMainThread()->gc.nursery.isInside(data);
#else
        bool isTenured = true;
#endif
        if (isTenured && tarr->hasSingletonType()) {
            
            
            types::TypeObjectKey *tarrType = types::TypeObjectKey::get(tarr);
            if (!tarrType->unknownProperties()) {
                tarrType->watchStateChangeForTypedArrayData(constraints());

                obj->setImplicitlyUsedUnchecked();

                int32_t len = AssertedCast<int32_t>(tarr->length());
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

MDefinition *
IonBuilder::convertShiftToMaskForStaticTypedArray(MDefinition *id,
                                                  Scalar::Type viewType)
{
    
    if (TypedArrayShift(viewType) == 0)
        return id;

    
    
    if (id->isConstant() && id->toConstant()->value().isInt32()) {
        int32_t index = id->toConstant()->value().toInt32();
        MConstant *offset = MConstant::New(alloc(), Int32Value(index << TypedArrayShift(viewType)));
        current->add(offset);
        return offset;
    }

    if (!id->isRsh() || id->isEffectful())
        return nullptr;
    if (!id->getOperand(1)->isConstant())
        return nullptr;
    const Value &value = id->getOperand(1)->toConstant()->value();
    if (!value.isInt32() || uint32_t(value.toInt32()) != TypedArrayShift(viewType))
        return nullptr;

    
    
    MConstant *mask = MConstant::New(alloc(), Int32Value(~((1 << value.toInt32()) - 1)));
    MBitAnd *ptr = MBitAnd::New(alloc(), id->getOperand(0), mask);

    ptr->infer(nullptr, nullptr);
    JS_ASSERT(!ptr->isEffectful());

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
IonBuilder::jsop_getelem_typed(MDefinition *obj, MDefinition *index,
                               Scalar::Type arrayType)
{
    types::TemporaryTypeSet *types = bytecodeTypes(pc);

    bool maybeUndefined = types->hasType(types::Type::UndefinedType());

    
    
    
    bool allowDouble = types->hasType(types::Type::DoubleType());

    
    MInstruction *idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    if (!maybeUndefined) {
        
        

        
        
        
        
        MIRType knownType = MIRTypeForTypedArrayRead(arrayType, allowDouble);

        
        MInstruction *length;
        MInstruction *elements;
        addTypedArrayLengthAndData(obj, DoBoundsCheck, &index, &length, &elements);

        
        MLoadTypedArrayElement *load = MLoadTypedArrayElement::New(alloc(), elements, index, arrayType);
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
            if (types->hasType(types::Type::Int32Type()))
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

        
        
        
        MLoadTypedArrayElementHole *load =
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

    MDefinition *value = current->pop();
    MDefinition *index = current->pop();
    MDefinition *object = current->pop();

    if (!setElemTryTypedObject(&emitted, object, index, value) || emitted)
        return emitted;

    if (!setElemTryTypedStatic(&emitted, object, index, value) || emitted)
        return emitted;

    if (!setElemTryTypedArray(&emitted, object, index, value) || emitted)
        return emitted;

    if (!setElemTryDense(&emitted, object, index, value) || emitted)
        return emitted;

    if (!setElemTryArguments(&emitted, object, index, value) || emitted)
        return emitted;

    if (script()->argumentsHasVarBinding() && object->mightBeType(MIRType_MagicOptimizedArguments))
        return abort("Type is not definitely lazy arguments.");

    if (!setElemTryCache(&emitted, object, index, value) || emitted)
        return emitted;

    
    MInstruction *ins = MCallSetElement::New(alloc(), object, index, value);
    current->add(ins);
    current->push(value);

    return resumeAfter(ins);
}

bool
IonBuilder::setElemTryTypedObject(bool *emitted, MDefinition *obj,
                                  MDefinition *index, MDefinition *value)
{
    JS_ASSERT(*emitted == false);

    TypedObjectPrediction objPrediction = typedObjectPrediction(obj);
    if (objPrediction.isUseless())
        return true;

    if (!objPrediction.ofArrayKind())
        return true;

    TypedObjectPrediction elemPrediction = objPrediction.arrayElementType();
    if (elemPrediction.isUseless())
        return true;

    JS_ASSERT(TypeDescr::isSized(elemPrediction.kind()));

    int32_t elemSize;
    if (!elemPrediction.hasKnownSize(&elemSize))
        return true;

    switch (elemPrediction.kind()) {
      case type::X4:
        
        return true;

      case type::Reference:
      case type::Struct:
      case type::SizedArray:
      case type::UnsizedArray:
        
        return true;

      case type::Scalar:
        return setElemTryScalarElemOfTypedObject(emitted,
                                                 obj,
                                                 index,
                                                 objPrediction,
                                                 value,
                                                 elemPrediction,
                                                 elemSize);
    }

    MOZ_CRASH("Bad kind");
}

bool
IonBuilder::setElemTryScalarElemOfTypedObject(bool *emitted,
                                              MDefinition *obj,
                                              MDefinition *index,
                                              TypedObjectPrediction objPrediction,
                                              MDefinition *value,
                                              TypedObjectPrediction elemPrediction,
                                              int32_t elemSize)
{
    
    ScalarTypeDescr::Type elemType = elemPrediction.scalarType();
    JS_ASSERT(elemSize == ScalarTypeDescr::alignment(elemType));

    bool canBeNeutered;
    MDefinition *indexAsByteOffset;
    if (!checkTypedObjectIndexInBounds(elemSize, obj, index, objPrediction,
                                       &indexAsByteOffset, &canBeNeutered))
    {
        return false;
    }

    
    if (!storeScalarTypedObjectValue(obj, indexAsByteOffset, elemType, canBeNeutered, false, value))
        return false;

    current->push(value);

    *emitted = true;
    return true;
}

bool
IonBuilder::setElemTryTypedStatic(bool *emitted, MDefinition *object,
                                  MDefinition *index, MDefinition *value)
{
    JS_ASSERT(*emitted == false);

    Scalar::Type arrayType;
    if (!ElementAccessIsTypedArray(object, index, &arrayType))
        return true;

    if (!LIRGenerator::allowStaticTypedArrayAccesses())
        return true;

    if (ElementAccessHasExtraIndexedProperty(constraints(), object))
        return true;

    if (!object->resultTypeSet())
        return true;
    JSObject *tarrObj = object->resultTypeSet()->getSingleton();
    if (!tarrObj)
        return true;

    TypedArrayObject *tarr = &tarrObj->as<TypedArrayObject>();

#ifdef JSGC_GENERATIONAL
    if (tarr->runtimeFromMainThread()->gc.nursery.isInside(tarr->viewData()))
        return true;
#endif

    types::TypeObjectKey *tarrType = types::TypeObjectKey::get(tarr);
    if (tarrType->unknownProperties())
        return true;

    Scalar::Type viewType = tarr->type();
    MDefinition *ptr = convertShiftToMaskForStaticTypedArray(index, viewType);
    if (!ptr)
        return true;

    
    tarrType->watchStateChangeForTypedArrayData(constraints());

    object->setImplicitlyUsedUnchecked();
    index->setImplicitlyUsedUnchecked();

    
    MDefinition *toWrite = value;
    if (viewType == Scalar::Uint8Clamped) {
        toWrite = MClampToUint8::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }

    MInstruction *store = MStoreTypedArrayElementStatic::New(alloc(), tarr, ptr, toWrite);
    current->add(store);
    current->push(value);

    if (!resumeAfter(store))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setElemTryTypedArray(bool *emitted, MDefinition *object,
                                 MDefinition *index, MDefinition *value)
{
    JS_ASSERT(*emitted == false);

    Scalar::Type arrayType;
    if (!ElementAccessIsTypedArray(object, index, &arrayType))
        return true;

    
    if (!jsop_setelem_typed(arrayType, SetElem_Normal, object, index, value))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setElemTryDense(bool *emitted, MDefinition *object,
                            MDefinition *index, MDefinition *value)
{
    JS_ASSERT(*emitted == false);

    if (!ElementAccessIsDenseNative(object, index))
        return true;
    if (PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current,
                                      &object, nullptr, &value,  true))
    {
        return true;
    }
    if (!object->resultTypeSet())
        return true;

    types::TemporaryTypeSet::DoubleConversion conversion =
        object->resultTypeSet()->convertDoubleElements(constraints());

    
    if (conversion == types::TemporaryTypeSet::AmbiguousDoubleConversion &&
        value->type() != MIRType_Int32)
    {
        return true;
    }

    
    
    if (ElementAccessHasExtraIndexedProperty(constraints(), object) && failedBoundsCheck_)
        return true;

    
    if (!jsop_setelem_dense(conversion, SetElem_Normal, object, index, value))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setElemTryArguments(bool *emitted, MDefinition *object,
                                MDefinition *index, MDefinition *value)
{
    JS_ASSERT(*emitted == false);

    if (object->type() != MIRType_MagicOptimizedArguments)
        return true;

    
    return abort("NYI arguments[]=");
}

bool
IonBuilder::setElemTryCache(bool *emitted, MDefinition *object,
                            MDefinition *index, MDefinition *value)
{
    JS_ASSERT(*emitted == false);

    if (!object->mightBeType(MIRType_Object))
        return true;

    if (!index->mightBeType(MIRType_Int32) &&
        !index->mightBeType(MIRType_String) &&
        !index->mightBeType(MIRType_Symbol))
    {
        return true;
    }

    
    
    
    SetElemICInspector icInspect(inspector->setElemICInspector(pc));
    if (!icInspect.sawDenseWrite() && !icInspect.sawTypedArrayWrite())
        return true;

    if (PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current,
                                      &object, nullptr, &value,  true))
    {
        return true;
    }

    
    
    
    
    bool guardHoles = ElementAccessHasExtraIndexedProperty(constraints(), object);

    
    object = addMaybeCopyElementsForWrite(object);

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), object, value));

    
    MInstruction *ins = MSetElementCache::New(alloc(), object, index, value, script()->strict(), guardHoles);
    current->add(ins);
    current->push(value);

    if (!resumeAfter(ins))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::jsop_setelem_dense(types::TemporaryTypeSet::DoubleConversion conversion,
                               SetElemSafety safety,
                               MDefinition *obj, MDefinition *id, MDefinition *value)
{
    MIRType elementType = DenseNativeElementType(constraints(), obj);
    bool packed = ElementAccessIsPacked(constraints(), obj);

    
    
    bool writeOutOfBounds = !ElementAccessHasExtraIndexedProperty(constraints(), obj);

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    
    MInstruction *idInt32 = MToInt32::New(alloc(), id);
    current->add(idInt32);
    id = idInt32;

    
    obj = addMaybeCopyElementsForWrite(obj);

    
    MElements *elements = MElements::New(alloc(), obj);
    current->add(elements);

    
    MDefinition *newValue = value;
    switch (conversion) {
      case types::TemporaryTypeSet::AlwaysConvertToDoubles:
      case types::TemporaryTypeSet::MaybeConvertToDoubles: {
        MInstruction *valueDouble = MToDouble::New(alloc(), value);
        current->add(valueDouble);
        newValue = valueDouble;
        break;
      }

      case types::TemporaryTypeSet::AmbiguousDoubleConversion: {
        JS_ASSERT(value->type() == MIRType_Int32);
        MInstruction *maybeDouble = MMaybeToDoubleElement::New(alloc(), elements, value);
        current->add(maybeDouble);
        newValue = maybeDouble;
        break;
      }

      case types::TemporaryTypeSet::DontConvertToDoubles:
        break;

      default:
        MOZ_CRASH("Unknown double conversion");
    }

    bool writeHole = false;
    if (safety == SetElem_Normal) {
        SetElemICInspector icInspect(inspector->setElemICInspector(pc));
        writeHole = icInspect.sawOOBDenseWrite();
    }

    
    
    
    MStoreElementCommon *store;
    if (writeHole && writeOutOfBounds) {
        JS_ASSERT(safety == SetElem_Normal);

        MStoreElementHole *ins = MStoreElementHole::New(alloc(), obj, elements, id, newValue);
        store = ins;

        current->add(ins);
        current->push(value);

        if (!resumeAfter(ins))
            return false;
    } else {
        MInitializedLength *initLength = MInitializedLength::New(alloc(), elements);
        current->add(initLength);

        bool needsHoleCheck;
        if (safety == SetElem_Normal) {
            id = addBoundsCheck(id, initLength);
            needsHoleCheck = !packed && !writeOutOfBounds;
        } else {
            needsHoleCheck = false;
        }

        MStoreElement *ins = MStoreElement::New(alloc(), elements, id, newValue, needsHoleCheck);
        store = ins;

        if (safety == SetElem_Unsafe)
            ins->setRacy();

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
                               MDefinition *obj, MDefinition *id, MDefinition *value)
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

    
    MInstruction *idInt32 = MToInt32::New(alloc(), id);
    current->add(idInt32);
    id = idInt32;

    
    MInstruction *length;
    MInstruction *elements;
    BoundsChecking checking = (!expectOOB && safety == SetElem_Normal)
                              ? DoBoundsCheck
                              : SkipBoundsCheck;
    addTypedArrayLengthAndData(obj, checking, &id, &length, &elements);

    
    MDefinition *toWrite = value;
    if (arrayType == Scalar::Uint8Clamped) {
        toWrite = MClampToUint8::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }

    
    MInstruction *ins;
    if (expectOOB) {
        ins = MStoreTypedArrayElementHole::New(alloc(), elements, length, id, toWrite, arrayType);
    } else {
        MStoreTypedArrayElement *store =
            MStoreTypedArrayElement::New(alloc(), elements, id, toWrite, arrayType);
        if (safety == SetElem_Unsafe)
            store->setRacy();
        ins = store;
    }

    current->add(ins);

    if (safety == SetElem_Normal)
        current->push(value);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_setelem_typed_object(Scalar::Type arrayType, SetElemSafety safety, bool racy,
                                      MDefinition *object, MDefinition *index, MDefinition *value)
{
    JS_ASSERT(safety == SetElem_Unsafe); 

    MInstruction *int_index = MToInt32::New(alloc(), index);
    current->add(int_index);

    size_t elemSize = ScalarTypeDescr::alignment(arrayType);
    MMul *byteOffset = MMul::New(alloc(), int_index, constantInt(elemSize),
                                        MIRType_Int32, MMul::Integer);
    current->add(byteOffset);

    if (!storeScalarTypedObjectValue(object, byteOffset, arrayType, false, racy, value))
        return false;

    return true;
}

bool
IonBuilder::jsop_length()
{
    if (jsop_length_fastPath())
        return true;

    PropertyName *name = info().getAtom(pc)->asPropertyName();
    return jsop_getprop(name);
}

bool
IonBuilder::jsop_length_fastPath()
{
    types::TemporaryTypeSet *types = bytecodeTypes(pc);

    if (types->getKnownMIRType() != MIRType_Int32)
        return false;

    MDefinition *obj = current->peek(-1);

    if (obj->mightBeType(MIRType_String)) {
        if (obj->mightBeType(MIRType_Object))
            return false;
        current->pop();
        MStringLength *ins = MStringLength::New(alloc(), obj);
        current->add(ins);
        current->push(ins);
        return true;
    }

    if (obj->mightBeType(MIRType_Object)) {
        types::TemporaryTypeSet *objTypes = obj->resultTypeSet();

        if (objTypes &&
            objTypes->getKnownClass() == &ArrayObject::class_ &&
            !objTypes->hasObjectFlags(constraints(), types::OBJECT_FLAG_LENGTH_OVERFLOW))
        {
            current->pop();
            MElements *elements = MElements::New(alloc(), obj);
            current->add(elements);

            
            MArrayLength *length = MArrayLength::New(alloc(), elements);
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
    JS_ASSERT(lazyArguments_);
    current->push(lazyArguments_);
    return true;
}

bool
IonBuilder::jsop_rest()
{
    JSObject *templateObject = inspector->getTemplateObject(pc);
    JS_ASSERT(templateObject->is<ArrayObject>());

    if (inliningDepth_ == 0) {
        
        MArgumentsLength *numActuals = MArgumentsLength::New(alloc());
        current->add(numActuals);

        
        
        MRest *rest = MRest::New(alloc(), constraints(), numActuals, info().nargs() - 1,
                                 templateObject);
        current->add(rest);
        current->push(rest);
        return true;
    }

    
    unsigned numActuals = inlineCallInfo_->argv().length();
    unsigned numFormals = info().nargs() - 1;
    unsigned numRest = numActuals > numFormals ? numActuals - numFormals : 0;

    MConstant *templateConst = MConstant::NewConstraintlessObject(alloc(), templateObject);
    current->add(templateConst);

    MNewArray *array = MNewArray::New(alloc(), constraints(), numRest, templateConst,
                                      templateObject->type()->initialHeap(constraints()),
                                      NewArray_FullyAllocating);
    current->add(array);

    if (numRest == 0) {
        
        
        current->push(array);
        return true;
    }

    MElements *elements = MElements::New(alloc(), array);
    current->add(elements);

    
    
    MConstant *index = nullptr;
    for (unsigned i = numFormals; i < numActuals; i++) {
        index = MConstant::New(alloc(), Int32Value(i - numFormals));
        current->add(index);

        MDefinition *arg = inlineCallInfo_->argv()[i];
        MStoreElement *store = MStoreElement::New(alloc(), elements, index, arg,
                                                   false);
        current->add(store);

        if (NeedsPostBarrier(info(), arg))
            current->add(MPostWriteBarrier::New(alloc(), array, arg));
    }

    
    
    
    MSetArrayLength *length = MSetArrayLength::New(alloc(), elements, index);
    current->add(length);

    
    
    MSetInitializedLength *initLength = MSetInitializedLength::New(alloc(), elements, index);
    current->add(initLength);

    current->push(array);
    return true;
}

uint32_t
IonBuilder::getDefiniteSlot(types::TemporaryTypeSet *types, PropertyName *name)
{
    if (!types || types->unknownObject())
        return UINT32_MAX;

    
    
    
    
    
    
    
    
    
    
    
    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObjectKey *type = types->getObject(i);
        if (!type)
            continue;

        if (types::TypeObject *typeObject = type->maybeType()) {
            if (typeObject->newScript() && !typeObject->newScript()->analyzed()) {
                addAbortedNewScriptPropertiesType(typeObject);
                return UINT32_MAX;
            }
        }
    }

    uint32_t slot = UINT32_MAX;

    for (size_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObjectKey *type = types->getObject(i);
        if (!type)
            continue;

        if (type->unknownProperties() || type->singleton())
            return UINT32_MAX;

        types::HeapTypeSetKey property = type->property(NameToId(name));
        if (!property.maybeTypes() ||
            !property.maybeTypes()->definiteProperty() ||
            property.nonData(constraints()))
        {
            return UINT32_MAX;
        }

        uint32_t propertySlot = property.maybeTypes()->definiteSlot();
        if (slot == UINT32_MAX)
            slot = propertySlot;
        else if (slot != propertySlot)
            return UINT32_MAX;
    }

    return slot;
}

bool
IonBuilder::jsop_runonce()
{
    MRunOncePrologue *ins = MRunOncePrologue::New(alloc());
    current->add(ins);
    return resumeAfter(ins);
}

bool
IonBuilder::jsop_not()
{
    MDefinition *value = current->pop();

    MNot *ins = MNot::New(alloc(), value);
    current->add(ins);
    current->push(ins);
    ins->cacheOperandMightEmulateUndefined();
    return true;
}

bool
IonBuilder::objectsHaveCommonPrototype(types::TemporaryTypeSet *types, PropertyName *name,
                                       bool isGetter, JSObject *foundProto)
{
    
    
    
    

    
    if (!types || types->unknownObject())
        return false;

    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        if (types->getSingleObject(i) == foundProto)
            continue;

        types::TypeObjectKey *type = types->getObject(i);
        if (!type)
            continue;

        while (type) {
            if (type->unknownProperties())
                return false;

            const Class *clasp = type->clasp();
            if (!ClassHasEffectlessLookup(clasp, name) || ClassHasResolveHook(compartment, clasp, name))
                return false;

            
            
            
            if (isGetter && clasp->ops.getGeneric && !IsTypedArrayClass(clasp))
                return false;
            if (!isGetter && clasp->ops.setGeneric)
                return false;

            
            
            
            types::HeapTypeSetKey property = type->property(NameToId(name));
            if (types::TypeSet *types = property.maybeTypes()) {
                if (!types->empty() || types->nonDataProperty())
                    return false;
            }
            if (JSObject *obj = type->singleton()) {
                if (types::CanHaveEmptyPropertyTypesForOwnProperty(obj))
                    return false;
            }

            if (!type->hasTenuredProto())
                return false;
            JSObject *proto = type->proto().toObjectOrNull();
            if (proto == foundProto)
                break;
            if (!proto) {
                
                
                return false;
            }
            type = types::TypeObjectKey::get(type->proto().toObjectOrNull());
        }
    }

    return true;
}

void
IonBuilder::freezePropertiesForCommonPrototype(types::TemporaryTypeSet *types, PropertyName *name,
                                               JSObject *foundProto)
{
    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        
        
        if (types->getSingleObject(i) == foundProto)
            continue;

        types::TypeObjectKey *type = types->getObject(i);
        if (!type)
            continue;

        while (true) {
            types::HeapTypeSetKey property = type->property(NameToId(name));
            JS_ALWAYS_TRUE(!property.isOwnProperty(constraints()));

            
            
            
            if (type->proto() == TaggedProto(foundProto))
                break;
            type = types::TypeObjectKey::get(type->proto().toObjectOrNull());
        }
    }
}

inline MDefinition *
IonBuilder::testCommonGetterSetter(types::TemporaryTypeSet *types, PropertyName *name,
                                   bool isGetter, JSObject *foundProto, Shape *lastProperty)
{
    
    if (!objectsHaveCommonPrototype(types, name, isGetter, foundProto))
        return nullptr;

    
    
    
    freezePropertiesForCommonPrototype(types, name, foundProto);

    
    
    
    
    MInstruction *wrapper = constant(ObjectValue(*foundProto));
    return addShapeGuard(wrapper, lastProperty, Bailout_ShapeGuard);
}

bool
IonBuilder::annotateGetPropertyCache(MDefinition *obj, MGetPropertyCache *getPropCache,
                                     types::TemporaryTypeSet *objTypes,
                                     types::TemporaryTypeSet *pushedTypes)
{
    PropertyName *name = getPropCache->name();

    
    if (pushedTypes->unknownObject() || pushedTypes->baseFlags() != 0)
        return true;

    for (unsigned i = 0; i < pushedTypes->getObjectCount(); i++) {
        if (pushedTypes->getTypeObject(i) != nullptr)
            return true;
    }

    
    if (!objTypes || objTypes->baseFlags() || objTypes->unknownObject())
        return true;

    unsigned int objCount = objTypes->getObjectCount();
    if (objCount == 0)
        return true;

    InlinePropertyTable *inlinePropTable = getPropCache->initInlinePropertyTable(alloc(), pc);
    if (!inlinePropTable)
        return false;

    
    
    for (unsigned int i = 0; i < objCount; i++) {
        types::TypeObject *baseTypeObj = objTypes->getTypeObject(i);
        if (!baseTypeObj)
            continue;
        types::TypeObjectKey *typeObj = types::TypeObjectKey::get(baseTypeObj);
        if (typeObj->unknownProperties() || !typeObj->hasTenuredProto() || !typeObj->proto().isObject())
            continue;

        const Class *clasp = typeObj->clasp();
        if (!ClassHasEffectlessLookup(clasp, name) || ClassHasResolveHook(compartment, clasp, name))
            continue;

        types::HeapTypeSetKey ownTypes = typeObj->property(NameToId(name));
        if (ownTypes.isOwnProperty(constraints()))
            continue;

        JSObject *singleton = testSingletonProperty(typeObj->proto().toObject(), name);
        if (!singleton || !singleton->is<JSFunction>())
            continue;

        
        if (!pushedTypes->hasType(types::Type::ObjectType(singleton)))
            continue;

        if (!inlinePropTable->addEntry(alloc(), baseTypeObj, &singleton->as<JSFunction>()))
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
        MResumePoint *resumePoint = MResumePoint::New(alloc(), current, pc, callerResumePoint_,
                                                      MResumePoint::ResumeAt);
        if (!resumePoint)
            return false;
        inlinePropTable->setPriorResumePoint(resumePoint);
        current->pop();
    }
    return true;
}



bool
IonBuilder::invalidatedIdempotentCache()
{
    IonBuilder *builder = this;
    do {
        if (builder->script()->invalidatedIdempotentCache())
            return true;
        builder = builder->callerBuilder_;
    } while (builder);

    return false;
}

bool
IonBuilder::loadSlot(MDefinition *obj, size_t slot, size_t nfixed, MIRType rvalType,
                     BarrierKind barrier, types::TemporaryTypeSet *types)
{
    if (slot < nfixed) {
        MLoadFixedSlot *load = MLoadFixedSlot::New(alloc(), obj, slot);
        current->add(load);
        current->push(load);

        load->setResultType(rvalType);
        return pushTypeBarrier(load, types, barrier);
    }

    MSlots *slots = MSlots::New(alloc(), obj);
    current->add(slots);

    MLoadSlot *load = MLoadSlot::New(alloc(), slots, slot - nfixed);
    current->add(load);
    current->push(load);

    load->setResultType(rvalType);
    return pushTypeBarrier(load, types, barrier);
}

bool
IonBuilder::loadSlot(MDefinition *obj, Shape *shape, MIRType rvalType,
                     BarrierKind barrier, types::TemporaryTypeSet *types)
{
    return loadSlot(obj, shape->slot(), shape->numFixedSlots(), rvalType, barrier, types);
}

bool
IonBuilder::storeSlot(MDefinition *obj, size_t slot, size_t nfixed,
                      MDefinition *value, bool needsBarrier,
                      MIRType slotType )
{
    if (slot < nfixed) {
        MStoreFixedSlot *store = MStoreFixedSlot::New(alloc(), obj, slot, value);
        current->add(store);
        current->push(value);
        if (needsBarrier)
            store->setNeedsBarrier();
        return resumeAfter(store);
    }

    MSlots *slots = MSlots::New(alloc(), obj);
    current->add(slots);

    MStoreSlot *store = MStoreSlot::New(alloc(), slots, slot - nfixed, value);
    current->add(store);
    current->push(value);
    if (needsBarrier)
        store->setNeedsBarrier();
    if (slotType != MIRType_None)
        store->setSlotType(slotType);
    return resumeAfter(store);
}

bool
IonBuilder::storeSlot(MDefinition *obj, Shape *shape, MDefinition *value, bool needsBarrier,
                      MIRType slotType )
{
    JS_ASSERT(shape->writable());
    return storeSlot(obj, shape->slot(), shape->numFixedSlots(), value, needsBarrier, slotType);
}

bool
IonBuilder::jsop_getprop(PropertyName *name)
{
    bool emitted = false;

    MDefinition *obj = current->pop();
    types::TemporaryTypeSet *types = bytecodeTypes(pc);

    
    if (!getPropTryInferredConstant(&emitted, obj, name, types) || emitted)
        return emitted;

    
    if (!getPropTryArgumentsLength(&emitted, obj) || emitted)
        return emitted;

    
    if (!getPropTryArgumentsCallee(&emitted, obj, name) || emitted)
        return emitted;

    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(),
                                                       obj, name, types);

    
    
    
    
    if (info().executionModeIsAnalysis() || types->empty()) {
        MCallGetProperty *call = MCallGetProperty::New(alloc(), obj, name, *pc == JSOP_CALLPROP);
        current->add(call);

        
        
        
        
        if (info().executionModeIsAnalysis()) {
            if (!getPropTryConstant(&emitted, obj, name, types) || emitted)
                return emitted;
        }

        current->push(call);
        return resumeAfter(call) && pushTypeBarrier(call, types, BarrierKind::TypeSet);
    }

    
    if (!getPropTryConstant(&emitted, obj, name, types) || emitted)
        return emitted;

    
    if (!getPropTryTypedObject(&emitted, obj, name, types) || emitted)
        return emitted;

    
    if (!getPropTryDefiniteSlot(&emitted, obj, name, barrier, types) || emitted)
        return emitted;

    
    if (!getPropTryCommonGetter(&emitted, obj, name, types) || emitted)
        return emitted;

    
    if (!getPropTryInlineAccess(&emitted, obj, name, barrier, types) || emitted)
        return emitted;

    
    if (!getPropTryInnerize(&emitted, obj, name, types) || emitted)
        return emitted;

    
    if (!getPropTryCache(&emitted, obj, name, barrier, types) || emitted)
        return emitted;

    
    MCallGetProperty *call = MCallGetProperty::New(alloc(), obj, name, *pc == JSOP_CALLPROP);
    current->add(call);
    current->push(call);
    if (!resumeAfter(call))
        return false;

    return pushTypeBarrier(call, types, BarrierKind::TypeSet);
}

bool
IonBuilder::checkIsDefinitelyOptimizedArguments(MDefinition *obj, bool *isOptimizedArgs)
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
IonBuilder::getPropTryInferredConstant(bool *emitted, MDefinition *obj, PropertyName *name,
                                       types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);

    
    types::TemporaryTypeSet *objTypes = obj->resultTypeSet();
    if (!objTypes)
        return true;

    JSObject *singleton = objTypes->getSingleton();
    if (!singleton)
        return true;

    types::TypeObjectKey *type = types::TypeObjectKey::get(singleton);
    if (type->unknownProperties())
        return true;

    types::HeapTypeSetKey property = type->property(NameToId(name));

    Value constantValue = UndefinedValue();
    if (property.constant(constraints(), &constantValue)) {
        spew("Optimized constant property");
        obj->setImplicitlyUsedUnchecked();
        if (!pushConstant(constantValue))
            return false;
        types->addType(types::GetValueType(constantValue), alloc_->lifoAlloc());
        *emitted = true;
    }

    return true;
}

bool
IonBuilder::getPropTryArgumentsLength(bool *emitted, MDefinition *obj)
{
    JS_ASSERT(*emitted == false);

    bool isOptimizedArgs = false;
    if (!checkIsDefinitelyOptimizedArguments(obj, &isOptimizedArgs))
        return false;
    if (!isOptimizedArgs)
        return true;

    if (JSOp(*pc) != JSOP_LENGTH)
        return true;

    *emitted = true;

    obj->setImplicitlyUsedUnchecked();

    
    if (inliningDepth_ == 0) {
        MInstruction *ins = MArgumentsLength::New(alloc());
        current->add(ins);
        current->push(ins);
        return true;
    }

    
    return pushConstant(Int32Value(inlineCallInfo_->argv().length()));
}

bool
IonBuilder::getPropTryArgumentsCallee(bool *emitted, MDefinition *obj, PropertyName *name)
{
    JS_ASSERT(*emitted == false);

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

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryConstant(bool *emitted, MDefinition *obj, PropertyName *name,
                               types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);
    JSObject *singleton = types ? types->getSingleton() : nullptr;
    if (!singleton)
        return true;

    bool testObject, testString;
    if (!testSingletonPropertyTypes(obj, singleton, name, &testObject, &testString))
        return true;

    
    JS_ASSERT(!testString || !testObject);
    if (testObject)
        current->add(MGuardObject::New(alloc(), obj));
    else if (testString)
        current->add(MGuardString::New(alloc(), obj));
    else
        obj->setImplicitlyUsedUnchecked();

    pushConstant(ObjectValue(*singleton));

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryTypedObject(bool *emitted,
                                  MDefinition *obj,
                                  PropertyName *name,
                                  types::TemporaryTypeSet *resultTypes)
{
    TypedObjectPrediction fieldPrediction;
    size_t fieldOffset;
    size_t fieldIndex;
    if (!typedObjectHasField(obj, name, &fieldOffset, &fieldPrediction, &fieldIndex))
        return true;

    switch (fieldPrediction.kind()) {
      case type::Reference:
        return true;

      case type::X4:
        
        return true;

      case type::Struct:
      case type::SizedArray:
        return getPropTryComplexPropOfTypedObject(emitted,
                                                  obj,
                                                  fieldOffset,
                                                  fieldPrediction,
                                                  fieldIndex,
                                                  resultTypes);

      case type::Scalar:
        return getPropTryScalarPropOfTypedObject(emitted,
                                                 obj,
                                                 fieldOffset,
                                                 fieldPrediction,
                                                 resultTypes);

      case type::UnsizedArray:
        MOZ_CRASH("Field of unsized array type");
    }

    MOZ_CRASH("Bad kind");
}

bool
IonBuilder::getPropTryScalarPropOfTypedObject(bool *emitted, MDefinition *typedObj,
                                              int32_t fieldOffset,
                                              TypedObjectPrediction fieldPrediction,
                                              types::TemporaryTypeSet *resultTypes)
{
    
    Scalar::Type fieldType = fieldPrediction.scalarType();

    
    return pushScalarLoadFromTypedObject(emitted, typedObj, constantInt(fieldOffset),
                                         fieldType, true);
}

bool
IonBuilder::getPropTryComplexPropOfTypedObject(bool *emitted,
                                               MDefinition *typedObj,
                                               int32_t fieldOffset,
                                               TypedObjectPrediction fieldPrediction,
                                               size_t fieldIndex,
                                               types::TemporaryTypeSet *resultTypes)
{
    

    
    MDefinition *type = loadTypedObjectType(typedObj);
    MDefinition *fieldTypeObj = typeObjectForFieldFromStructType(type, fieldIndex);

    return pushDerivedTypedObject(emitted, typedObj, constantInt(fieldOffset),
                                  fieldPrediction, fieldTypeObj, true);
}

bool
IonBuilder::getPropTryDefiniteSlot(bool *emitted, MDefinition *obj, PropertyName *name,
                                   BarrierKind barrier, types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);
    uint32_t slot = getDefiniteSlot(obj->resultTypeSet(), name);
    if (slot == UINT32_MAX)
        return true;

    if (obj->type() != MIRType_Object) {
        MGuardObject *guard = MGuardObject::New(alloc(), obj);
        current->add(guard);
        obj = guard;
    }

    MInstruction *load;
    if (slot < JSObject::MAX_FIXED_SLOTS) {
        load = MLoadFixedSlot::New(alloc(), obj, slot);
    } else {
        MInstruction *slots = MSlots::New(alloc(), obj);
        current->add(slots);

        load = MLoadSlot::New(alloc(), slots, slot - JSObject::MAX_FIXED_SLOTS);
    }

    if (barrier == BarrierKind::NoBarrier)
        load->setResultType(types->getKnownMIRType());

    current->add(load);
    current->push(load);

    if (!pushTypeBarrier(load, types, barrier))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryCommonGetter(bool *emitted, MDefinition *obj, PropertyName *name,
                                   types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);

    Shape *lastProperty = nullptr;
    JSFunction *commonGetter = nullptr;
    JSObject *foundProto = inspector->commonGetPropFunction(pc, &lastProperty, &commonGetter);
    if (!foundProto)
        return true;

    types::TemporaryTypeSet *objTypes = obj->resultTypeSet();
    MDefinition *guard = testCommonGetterSetter(objTypes, name,  true,
                                                foundProto, lastProperty);
    if (!guard)
        return true;

    bool isDOM = objTypes->isDOMClass();

    if (isDOM && testShouldDOMCall(objTypes, commonGetter, JSJitInfo::Getter)) {
        const JSJitInfo *jitinfo = commonGetter->jitInfo();
        MInstruction *get;
        if (jitinfo->isAlwaysInSlot) {
            
            
            get = MGetDOMMember::New(alloc(), jitinfo, obj, guard);
        } else {
            get = MGetDOMProperty::New(alloc(), jitinfo, obj, guard);
        }
        current->add(get);
        current->push(get);

        if (get->isEffectful() && !resumeAfter(get))
            return false;

        if (!pushDOMTypeBarrier(get, types, commonGetter))
            return false;

        *emitted = true;
        return true;
    }

    
    if (objTypes->getKnownMIRType() != MIRType_Object) {
        MGuardObject *guardObj = MGuardObject::New(alloc(), obj);
        current->add(guardObj);
        obj = guardObj;
    }

    

    
    if (!current->ensureHasSlots(2))
        return false;
    pushConstant(ObjectValue(*commonGetter));

    current->push(obj);

    CallInfo callInfo(alloc(), false);
    if (!callInfo.init(current, 0))
        return false;

    if (commonGetter->isNative()) {
        InliningStatus status = inlineNativeGetter(callInfo, commonGetter);
        switch (status) {
          case InliningStatus_Error:
            return false;
          case InliningStatus_NotInlined:
            break;
          case InliningStatus_Inlined:
            *emitted = true;
            return true;
        }
    }

    
    bool inlineable = false;
    if (commonGetter->isInterpreted()) {
        InliningDecision decision = makeInliningDecision(commonGetter, callInfo);
        switch (decision) {
          case InliningDecision_Error:
            return false;
          case InliningDecision_DontInline:
            break;
          case InliningDecision_Inline:
            inlineable = true;
            break;
        }
    }

    if (inlineable) {
        if (!inlineScriptedCall(callInfo, commonGetter))
            return false;
    } else {
        if (!makeCall(commonGetter, callInfo, false))
            return false;
    }

    *emitted = true;
    return true;
}

static bool
CanInlinePropertyOpShapes(const BaselineInspector::ShapeVector &shapes)
{
    for (size_t i = 0; i < shapes.length(); i++) {
        
        
        
        
        if (shapes[i]->inDictionary())
            return false;
    }

    return true;
}

static bool
GetPropertyShapes(jsid id, const BaselineInspector::ShapeVector &shapes,
                  BaselineInspector::ShapeVector &propShapes, bool *sameSlot)
{
    MOZ_ASSERT(shapes.length() > 1);
    MOZ_ASSERT(propShapes.empty());

    if (!propShapes.reserve(shapes.length()))
        return false;

    *sameSlot = true;
    for (size_t i = 0; i < shapes.length(); i++) {
        Shape *objShape = shapes[i];
        Shape *shape = objShape->searchLinear(id);
        MOZ_ASSERT(shape);
        propShapes.infallibleAppend(shape);

        if (i > 0) {
            if (shape->slot() != propShapes[0]->slot() ||
                shape->numFixedSlots() != propShapes[0]->numFixedSlots())
            {
                *sameSlot = false;
            }
        }
    }

    return true;
}

bool
IonBuilder::getPropTryInlineAccess(bool *emitted, MDefinition *obj, PropertyName *name,
                                   BarrierKind barrier, types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);
    if (obj->type() != MIRType_Object)
        return true;

    BaselineInspector::ShapeVector shapes(alloc());
    if (!inspector->maybeShapesForPropertyOp(pc, shapes))
        return false;

    if (shapes.empty() || !CanInlinePropertyOpShapes(shapes))
        return true;

    MIRType rvalType = types->getKnownMIRType();
    if (barrier != BarrierKind::NoBarrier || IsNullOrUndefined(rvalType))
        rvalType = MIRType_Value;

    if (shapes.length() == 1) {
        
        
        spew("Inlining monomorphic GETPROP");

        Shape *objShape = shapes[0];
        obj = addShapeGuard(obj, objShape, Bailout_ShapeGuard);

        Shape *shape = objShape->searchLinear(NameToId(name));
        JS_ASSERT(shape);

        if (!loadSlot(obj, shape, rvalType, barrier, types))
            return false;

        *emitted = true;
        return true;
    }

    MOZ_ASSERT(shapes.length() > 1);
    spew("Inlining polymorphic GETPROP");

    BaselineInspector::ShapeVector propShapes(alloc());
    bool sameSlot;
    if (!GetPropertyShapes(NameToId(name), shapes, propShapes, &sameSlot))
        return false;

    if (sameSlot) {
        MGuardShapePolymorphic *guard = MGuardShapePolymorphic::New(alloc(), obj);
        current->add(guard);
        obj = guard;

        if (failedShapeGuard_)
            guard->setNotMovable();

        for (size_t i = 0; i < shapes.length(); i++) {
            if (!guard->addShape(shapes[i]))
                return false;
        }

        if (!loadSlot(obj, propShapes[0], rvalType, barrier, types))
            return false;

        *emitted = true;
        return true;
    }

    MGetPropertyPolymorphic *load = MGetPropertyPolymorphic::New(alloc(), obj, name);
    current->add(load);
    current->push(load);

    for (size_t i = 0; i < shapes.length(); i++) {
        if (!load->addShape(shapes[i], propShapes[i]))
            return false;
    }

    if (failedShapeGuard_)
        load->setNotMovable();

    load->setResultType(rvalType);
    if (!pushTypeBarrier(load, types, barrier))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryCache(bool *emitted, MDefinition *obj, PropertyName *name,
                            BarrierKind barrier, types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);

    
    
    if (obj->type() != MIRType_Object) {
        types::TemporaryTypeSet *types = obj->resultTypeSet();
        if (!types || !types->objectOrSentinel())
            return true;
    }

    
    
    if (inspector->hasSeenAccessedGetter(pc))
        barrier = BarrierKind::TypeSet;

    if (needsToMonitorMissingProperties(types))
        barrier = BarrierKind::TypeSet;

    
    
    if (barrier == BarrierKind::NoBarrier)
        barrier = PropertyReadOnPrototypeNeedsTypeBarrier(constraints(), obj, name, types);

    MGetPropertyCache *load = MGetPropertyCache::New(alloc(), obj, name,
                                                     barrier != BarrierKind::NoBarrier);

    
    
    
    
    
    
    if (obj->type() == MIRType_Object && !invalidatedIdempotentCache() &&
        info().executionMode() != ParallelExecution)
    {
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

    *emitted = true;
    return true;
}

MDefinition *
IonBuilder::tryInnerizeWindow(MDefinition *obj)
{
    
    
    
    
    

    if (obj->type() != MIRType_Object)
        return obj;

    types::TemporaryTypeSet *types = obj->resultTypeSet();
    if (!types)
        return obj;

    JSObject *singleton = types->getSingleton();
    if (!singleton)
        return obj;

    JSObject *inner = GetInnerObject(singleton);
    if (inner == singleton || inner != &script()->global())
        return obj;

    
    
    
    types::TypeObjectKey *objType = types::TypeObjectKey::get(singleton);
    if (objType->hasFlags(constraints(), types::OBJECT_FLAG_UNKNOWN_PROPERTIES))
        return obj;

    obj->setImplicitlyUsedUnchecked();
    return constant(ObjectValue(script()->global()));
}

bool
IonBuilder::getPropTryInnerize(bool *emitted, MDefinition *obj, PropertyName *name,
                               types::TemporaryTypeSet *types)
{
    

    MOZ_ASSERT(*emitted == false);

    MDefinition *inner = tryInnerizeWindow(obj);
    if (inner == obj)
        return true;

    
    
    
    
    
    
    
    

    if (!getPropTryConstant(emitted, inner, name, types) || *emitted)
        return *emitted;

    if (!getStaticName(&script()->global(), name, emitted) || *emitted)
        return *emitted;

    
    
    BarrierKind barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(),
                                                       inner, name, types);
    if (!getPropTryCache(emitted, inner, name, barrier, types) || *emitted)
        return *emitted;

    MOZ_ASSERT(*emitted == false);
    return true;
}

bool
IonBuilder::needsToMonitorMissingProperties(types::TemporaryTypeSet *types)
{
    
    
    
    
    return info().executionMode() == ParallelExecution &&
           !types->hasType(types::Type::UndefinedType());
}

bool
IonBuilder::jsop_setprop(PropertyName *name)
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->pop();

    bool emitted = false;

    
    
    if (info().executionModeIsAnalysis()) {
        MInstruction *ins = MCallSetProperty::New(alloc(), obj, value, name, script()->strict());
        current->add(ins);
        current->push(value);
        return resumeAfter(ins);
    }

    
    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    
    if (!setPropTryCommonSetter(&emitted, obj, name, value) || emitted)
        return emitted;

    types::TemporaryTypeSet *objTypes = obj->resultTypeSet();
    bool barrier = PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current, &obj, name, &value,
                                                  true);

    
    if (!setPropTryTypedObject(&emitted, obj, name, value) || emitted)
        return emitted;

    if (!barrier) {
        
        if (!setPropTryDefiniteSlot(&emitted, obj, name, value, objTypes) || emitted)
            return emitted;

        
        if (!setPropTryInlineAccess(&emitted, obj, name, value, objTypes) || emitted)
            return emitted;
    }

    
    return setPropTryCache(&emitted, obj, name, value, barrier, objTypes);
}

bool
IonBuilder::setPropTryCommonSetter(bool *emitted, MDefinition *obj,
                                   PropertyName *name, MDefinition *value)
{
    JS_ASSERT(*emitted == false);

    Shape *lastProperty = nullptr;
    JSFunction *commonSetter = nullptr;
    JSObject *foundProto = inspector->commonSetPropFunction(pc, &lastProperty, &commonSetter);
    if (!foundProto)
        return true;

    types::TemporaryTypeSet *objTypes = obj->resultTypeSet();
    MDefinition *guard = testCommonGetterSetter(objTypes, name,  false,
                                                foundProto, lastProperty);
    if (!guard)
        return true;

    bool isDOM = objTypes->isDOMClass();

    

    
    
    

    
    if (!setPropTryCommonDOMSetter(emitted, obj, value, commonSetter, isDOM))
        return false;

    if (*emitted)
        return true;

    
    if (objTypes->getKnownMIRType() != MIRType_Object) {
        MGuardObject *guardObj = MGuardObject::New(alloc(), obj);
        current->add(guardObj);
        obj = guardObj;
    }

    
    
    if (!current->ensureHasSlots(3))
        return false;

    pushConstant(ObjectValue(*commonSetter));

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
            break;
          case InliningDecision_Inline:
            if (!inlineScriptedCall(callInfo, commonSetter))
                return false;
            *emitted = true;
            return true;
        }
    }

    MCall *call = makeCallHelper(commonSetter, callInfo, false);
    if (!call)
        return false;

    current->push(value);
    if (!resumeAfter(call))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryCommonDOMSetter(bool *emitted, MDefinition *obj,
                                      MDefinition *value, JSFunction *setter,
                                      bool isDOM)
{
    JS_ASSERT(*emitted == false);

    if (!isDOM)
        return true;

    types::TemporaryTypeSet *objTypes = obj->resultTypeSet();
    if (!testShouldDOMCall(objTypes, setter, JSJitInfo::Setter))
        return true;

    
    JS_ASSERT(setter->jitInfo()->type() == JSJitInfo::Setter);
    MSetDOMProperty *set = MSetDOMProperty::New(alloc(), setter->jitInfo()->setter, obj, value);

    current->add(set);
    current->push(value);

    if (!resumeAfter(set))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryTypedObject(bool *emitted, MDefinition *obj,
                                  PropertyName *name, MDefinition *value)
{
    TypedObjectPrediction fieldPrediction;
    size_t fieldOffset;
    size_t fieldIndex;
    if (!typedObjectHasField(obj, name, &fieldOffset, &fieldPrediction, &fieldIndex))
        return true;

    switch (fieldPrediction.kind()) {
      case type::X4:
        
        return true;

      case type::Reference:
      case type::Struct:
      case type::SizedArray:
      case type::UnsizedArray:
        
        return true;

      case type::Scalar:
        return setPropTryScalarPropOfTypedObject(emitted, obj, fieldOffset,
                                                 value, fieldPrediction);
    }

    MOZ_CRASH("Unknown kind");
}

bool
IonBuilder::setPropTryScalarPropOfTypedObject(bool *emitted,
                                              MDefinition *obj,
                                              int32_t fieldOffset,
                                              MDefinition *value,
                                              TypedObjectPrediction fieldPrediction)
{
    
    Scalar::Type fieldType = fieldPrediction.scalarType();

    

    if (!storeScalarTypedObjectValue(obj, constantInt(fieldOffset), fieldType, true, false, value))
        return false;

    current->push(value);

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryDefiniteSlot(bool *emitted, MDefinition *obj,
                                   PropertyName *name, MDefinition *value,
                                   types::TemporaryTypeSet *objTypes)
{
    JS_ASSERT(*emitted == false);

    uint32_t slot = getDefiniteSlot(obj->resultTypeSet(), name);
    if (slot == UINT32_MAX)
        return true;

    bool writeBarrier = false;
    for (size_t i = 0; i < obj->resultTypeSet()->getObjectCount(); i++) {
        types::TypeObjectKey *type = obj->resultTypeSet()->getObject(i);
        if (!type)
            continue;

        types::HeapTypeSetKey property = type->property(NameToId(name));
        if (property.nonWritable(constraints()))
            return true;
        writeBarrier |= property.needsBarrier(constraints());
    }

    MInstruction *store;
    if (slot < JSObject::MAX_FIXED_SLOTS) {
        store = MStoreFixedSlot::New(alloc(), obj, slot, value);
        if (writeBarrier)
            store->toStoreFixedSlot()->setNeedsBarrier();
    } else {
        MInstruction *slots = MSlots::New(alloc(), obj);
        current->add(slots);

        store = MStoreSlot::New(alloc(), slots, slot - JSObject::MAX_FIXED_SLOTS, value);
        if (writeBarrier)
            store->toStoreSlot()->setNeedsBarrier();
    }

    current->add(store);
    current->push(value);

    if (!resumeAfter(store))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryInlineAccess(bool *emitted, MDefinition *obj,
                                   PropertyName *name, MDefinition *value,
                                   types::TemporaryTypeSet *objTypes)
{
    JS_ASSERT(*emitted == false);

    BaselineInspector::ShapeVector shapes(alloc());
    if (!inspector->maybeShapesForPropertyOp(pc, shapes))
        return false;

    if (shapes.empty())
        return true;

    if (!CanInlinePropertyOpShapes(shapes))
        return true;

    if (shapes.length() == 1) {
        spew("Inlining monomorphic SETPROP");

        
        
        
        
        Shape *objShape = shapes[0];
        obj = addShapeGuard(obj, objShape, Bailout_ShapeGuard);

        Shape *shape = objShape->searchLinear(NameToId(name));
        JS_ASSERT(shape);

        bool needsBarrier = objTypes->propertyNeedsBarrier(constraints(), NameToId(name));
        if (!storeSlot(obj, shape, value, needsBarrier))
            return false;

        *emitted = true;
        return true;
    }

    MOZ_ASSERT(shapes.length() > 1);
    spew("Inlining polymorphic SETPROP");

    BaselineInspector::ShapeVector propShapes(alloc());
    bool sameSlot;
    if (!GetPropertyShapes(NameToId(name), shapes, propShapes, &sameSlot))
        return false;

    if (sameSlot) {
        MGuardShapePolymorphic *guard = MGuardShapePolymorphic::New(alloc(), obj);
        current->add(guard);
        obj = guard;

        if (failedShapeGuard_)
            guard->setNotMovable();

        for (size_t i = 0; i < shapes.length(); i++) {
            if (!guard->addShape(shapes[i]))
                return false;
        }

        bool needsBarrier = objTypes->propertyNeedsBarrier(constraints(), NameToId(name));
        if (!storeSlot(obj, propShapes[0], value, needsBarrier))
            return false;

        *emitted = true;
        return true;
    }

    MSetPropertyPolymorphic *ins = MSetPropertyPolymorphic::New(alloc(), obj, value);
    current->add(ins);
    current->push(value);

    for (size_t i = 0; i < shapes.length(); i++) {
        Shape *objShape = shapes[i];
        Shape *shape =  objShape->searchLinear(NameToId(name));
        JS_ASSERT(shape);
        if (!ins->addShape(objShape, shape))
            return false;
    }

    if (objTypes->propertyNeedsBarrier(constraints(), NameToId(name)))
        ins->setNeedsBarrier();

    if (!resumeAfter(ins))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryCache(bool *emitted, MDefinition *obj,
                            PropertyName *name, MDefinition *value,
                            bool barrier, types::TemporaryTypeSet *objTypes)
{
    JS_ASSERT(*emitted == false);

    
    MSetPropertyCache *ins = MSetPropertyCache::New(alloc(), obj, value, name, script()->strict(), barrier);

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
IonBuilder::jsop_delprop(PropertyName *name)
{
    MDefinition *obj = current->pop();

    MInstruction *ins = MDeleteProperty::New(alloc(), obj, name);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_delelem()
{
    MDefinition *index = current->pop();
    MDefinition *obj = current->pop();

    MDeleteElement *ins = MDeleteElement::New(alloc(), obj, index);
    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_regexp(RegExpObject *reobj)
{
    
    
    
    
    
    
    
    

    bool mustClone = true;
    types::TypeObjectKey *typeObj = types::TypeObjectKey::get(&script()->global());
    if (!typeObj->hasFlags(constraints(), types::OBJECT_FLAG_REGEXP_FLAGS_SET)) {
#ifdef DEBUG
        
        
        if (script()->global().hasRegExpStatics()) {
            RegExpStatics *res = script()->global().getAlreadyCreatedRegExpStatics();
            MOZ_ASSERT(res);
            uint32_t origFlags = reobj->getFlags();
            uint32_t staticsFlags = res->getFlags();
            JS_ASSERT((origFlags & staticsFlags) == staticsFlags);
        }
#endif

        if (!reobj->global() && !reobj->sticky())
            mustClone = false;
    }

    MRegExp *regexp = MRegExp::New(alloc(), constraints(), reobj, mustClone);
    current->add(regexp);
    current->push(regexp);

    return true;
}

bool
IonBuilder::jsop_object(JSObject *obj)
{
    if (options.cloneSingletons()) {
        MCloneLiteral *clone = MCloneLiteral::New(alloc(), constant(ObjectValue(*obj)));
        current->add(clone);
        current->push(clone);
        return resumeAfter(clone);
    }

    compartment->setSingletonsAsValues();
    pushConstant(ObjectValue(*obj));
    return true;
}

bool
IonBuilder::jsop_lambda(JSFunction *fun)
{
    MOZ_ASSERT(analysis().usesScopeChain());
    MOZ_ASSERT(!fun->isArrow());

    if (fun->isNative() && IsAsmJSModuleNative(fun->native()))
        return abort("asm.js module function");

    MLambda *ins = MLambda::New(alloc(), constraints(), current->scopeChain(), fun);
    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_lambda_arrow(JSFunction *fun)
{
    MOZ_ASSERT(analysis().usesScopeChain());
    MOZ_ASSERT(fun->isArrow());
    MOZ_ASSERT(!fun->isNative());

    MDefinition *thisDef = current->pop();

    MLambdaArrow *ins = MLambdaArrow::New(alloc(), constraints(), current->scopeChain(),
                                          thisDef, fun);
    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_setarg(uint32_t arg)
{
    
    
    
    
    
    JS_ASSERT(analysis_.hasSetArg());
    MDefinition *val = current->peek(-1);

    
    
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
        
        JS_ASSERT(script()->uninlineable() && !isInlineBuilder());

        MSetFrameArgument *store = MSetFrameArgument::New(alloc(), arg, val);
        modifiesFrameArguments_ = true;
        current->add(store);
        current->setArg(arg);
        return true;
    }

    
    
    
    
    if (graph().numBlocks() == 1 &&
        (val->isBitOr() || val->isBitAnd() || val->isMul() ))
     {
         for (size_t i = 0; i < val->numOperands(); i++) {
            MDefinition *op = val->getOperand(i);
            if (op->isParameter() &&
                op->toParameter()->index() == (int32_t)arg &&
                op->resultTypeSet() &&
                op->resultTypeSet()->empty())
            {
                bool otherUses = false;
                for (MUseDefIterator iter(op); iter; iter++) {
                    MDefinition *def = iter.def();
                    if (def == val)
                        continue;
                    otherUses = true;
                }
                if (!otherUses) {
                    JS_ASSERT(op->resultTypeSet() == &argTypes[arg]);
                    argTypes[arg].addType(types::Type::UnknownType(), alloc_->lifoAlloc());
                    if (val->isMul()) {
                        val->setResultType(MIRType_Double);
                        val->toMul()->setSpecialization(MIRType_Double);
                    } else {
                        JS_ASSERT(val->type() == MIRType_Int32);
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
    JS_ASSERT(JSOp(*pc) == JSOP_DEFVAR || JSOp(*pc) == JSOP_DEFCONST);

    PropertyName *name = script()->getName(index);

    
    unsigned attrs = JSPROP_ENUMERATE;
    if (JSOp(*pc) == JSOP_DEFCONST)
        attrs |= JSPROP_READONLY;
    else
        attrs |= JSPROP_PERMANENT;
    JS_ASSERT(!script()->isForEval());

    
    JS_ASSERT(analysis().usesScopeChain());

    
    MDefVar *defvar = MDefVar::New(alloc(), name, attrs, current->scopeChain());
    current->add(defvar);

    return resumeAfter(defvar);
}

bool
IonBuilder::jsop_deffun(uint32_t index)
{
    JSFunction *fun = script()->getFunction(index);
    if (fun->isNative() && IsAsmJSModuleNative(fun->native()))
        return abort("asm.js module function");

    JS_ASSERT(analysis().usesScopeChain());

    MDefFun *deffun = MDefFun::New(alloc(), fun, current->scopeChain());
    current->add(deffun);

    return resumeAfter(deffun);
}

bool
IonBuilder::jsop_this()
{
    if (!info().funMaybeLazy())
        return abort("JSOP_THIS outside of a JSFunction.");

    if (info().funMaybeLazy()->isArrow()) {
        
        MLoadArrowThis *thisObj = MLoadArrowThis::New(alloc(), getCallee());
        current->add(thisObj);
        current->push(thisObj);
        return true;
    }

    if (script()->strict() || info().funMaybeLazy()->isSelfHostedBuiltin()) {
        
        current->pushSlot(info().thisSlot());
        return true;
    }

    if (thisTypes->getKnownMIRType() == MIRType_Object ||
        (thisTypes->empty() && baselineFrame_ && baselineFrame_->thisType.isSomeObject()))
    {
        
        
        
        current->pushSlot(info().thisSlot());
        return true;
    }

    
    
    
    if (info().executionModeIsAnalysis()) {
        current->pushSlot(info().thisSlot());
        return true;
    }

    
    MDefinition *def = current->getSlot(info().thisSlot());

    if (def->type() == MIRType_Object) {
        
        current->push(def);
        return true;
    }

    MComputeThis *thisObj = MComputeThis::New(alloc(), def);
    current->add(thisObj);
    current->push(thisObj);

    current->setSlot(info().thisSlot(), thisObj);

    return resumeAfter(thisObj);
}

bool
IonBuilder::jsop_typeof()
{
    MDefinition *input = current->pop();
    MTypeOf *ins = MTypeOf::New(alloc(), input, input->type());

    ins->cacheInputMaybeCallableOrEmulatesUndefined();

    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_toid()
{
    
    if (current->peek(-1)->type() == MIRType_Int32)
        return true;

    MDefinition *index = current->pop();
    MToId *ins = MToId::New(alloc(), current->peek(-1), index);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_iter(uint8_t flags)
{
    if (flags != JSITER_ENUMERATE)
        nonStringIteration_ = true;

    MDefinition *obj = current->pop();
    MInstruction *ins = MIteratorStart::New(alloc(), obj, flags);

    if (!iterators_.append(ins))
        return false;

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_iternext()
{
    MDefinition *iter = current->peek(-1);
    MInstruction *ins = MIteratorNext::New(alloc(), iter);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    if (!nonStringIteration_ && !inspector->hasSeenNonStringIterNext(pc)) {
        ins = MUnbox::New(alloc(), ins, MIRType_String, MUnbox::Fallible,
                          Bailout_NonStringInputInvalidate);
        current->add(ins);
        current->rewriteAtDepth(-1, ins);
    }

    return true;
}

bool
IonBuilder::jsop_itermore()
{
    MDefinition *iter = current->peek(-1);
    MInstruction *ins = MIteratorMore::New(alloc(), iter);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_iterend()
{
    MDefinition *iter = current->pop();
    MInstruction *ins = MIteratorEnd::New(alloc(), iter);

    current->add(ins);

    return resumeAfter(ins);
}

MDefinition *
IonBuilder::walkScopeChain(unsigned hops)
{
    MDefinition *scope = current->getSlot(info().scopeChainSlot());

    for (unsigned i = 0; i < hops; i++) {
        MInstruction *ins = MEnclosingScope::New(alloc(), scope);
        current->add(ins);
        scope = ins;
    }

    return scope;
}

bool
IonBuilder::hasStaticScopeObject(ScopeCoordinate sc, JSObject **pcall)
{
    JSScript *outerScript = ScopeCoordinateFunctionScript(script(), pc);
    if (!outerScript || !outerScript->treatAsRunOnce())
        return false;

    types::TypeObjectKey *funType =
            types::TypeObjectKey::get(outerScript->functionNonDelazifying());
    if (funType->hasFlags(constraints(), types::OBJECT_FLAG_RUNONCE_INVALIDATED))
        return false;

    
    
    
    

    
    
    

    MDefinition *scope = current->getSlot(info().scopeChainSlot());
    scope->setImplicitlyUsedUnchecked();

    JSObject *environment = script()->functionNonDelazifying()->environment();
    while (environment && !environment->is<GlobalObject>()) {
        if (environment->is<CallObject>() &&
            !environment->as<CallObject>().isForEval() &&
            environment->as<CallObject>().callee().nonLazyScript() == outerScript)
        {
            JS_ASSERT(environment->hasSingletonType());
            *pcall = environment;
            return true;
        }
        environment = environment->enclosingScope();
    }

    
    
    
    

    if (script() == outerScript && baselineFrame_ && info().osrPc()) {
        JSObject *singletonScope = baselineFrame_->singletonScopeChain;
        if (singletonScope &&
            singletonScope->is<CallObject>() &&
            singletonScope->as<CallObject>().callee().nonLazyScript() == outerScript)
        {
            JS_ASSERT(singletonScope->hasSingletonType());
            *pcall = singletonScope;
            return true;
        }
    }

    return true;
}

bool
IonBuilder::jsop_getaliasedvar(ScopeCoordinate sc)
{
    JSObject *call = nullptr;
    if (hasStaticScopeObject(sc, &call) && call) {
        PropertyName *name = ScopeCoordinateName(scopeCoordinateNameCache, script(), pc);
        bool succeeded;
        if (!getStaticName(call, name, &succeeded))
            return false;
        if (succeeded)
            return true;
    }

    MDefinition *obj = walkScopeChain(sc.hops());

    Shape *shape = ScopeCoordinateToStaticScopeShape(script(), pc);

    MInstruction *load;
    if (shape->numFixedSlots() <= sc.slot()) {
        MInstruction *slots = MSlots::New(alloc(), obj);
        current->add(slots);

        load = MLoadSlot::New(alloc(), slots, sc.slot() - shape->numFixedSlots());
    } else {
        load = MLoadFixedSlot::New(alloc(), obj, sc.slot());
    }

    current->add(load);
    current->push(load);

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    return pushTypeBarrier(load, types, BarrierKind::TypeSet);
}

bool
IonBuilder::jsop_setaliasedvar(ScopeCoordinate sc)
{
    JSObject *call = nullptr;
    if (hasStaticScopeObject(sc, &call)) {
        uint32_t depth = current->stackDepth() + 1;
        if (depth > current->nslots()) {
            if (!current->increaseSlots(depth - current->nslots()))
                return false;
        }
        MDefinition *value = current->pop();
        PropertyName *name = ScopeCoordinateName(scopeCoordinateNameCache, script(), pc);

        if (call) {
            
            
            pushConstant(ObjectValue(*call));
            current->push(value);
            return setStaticName(call, name);
        }

        
        
        MDefinition *obj = walkScopeChain(sc.hops());
        current->push(obj);
        current->push(value);
        return jsop_setprop(name);
    }

    MDefinition *rval = current->peek(-1);
    MDefinition *obj = walkScopeChain(sc.hops());

    Shape *shape = ScopeCoordinateToStaticScopeShape(script(), pc);

    if (NeedsPostBarrier(info(), rval))
        current->add(MPostWriteBarrier::New(alloc(), obj, rval));

    MInstruction *store;
    if (shape->numFixedSlots() <= sc.slot()) {
        MInstruction *slots = MSlots::New(alloc(), obj);
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
    MDefinition *obj = current->peek(-1);
    MDefinition *id = current->peek(-2);

    if (ElementAccessIsDenseNative(obj, id) &&
        !ElementAccessHasExtraIndexedProperty(constraints(), obj))
    {
        return jsop_in_dense();
    }

    current->pop();
    current->pop();
    MIn *ins = MIn::New(alloc(), id, obj);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_in_dense()
{
    MDefinition *obj = current->pop();
    MDefinition *id = current->pop();

    bool needsHoleCheck = !ElementAccessIsPacked(constraints(), obj);

    
    MInstruction *idInt32 = MToInt32::New(alloc(), id);
    current->add(idInt32);
    id = idInt32;

    
    MElements *elements = MElements::New(alloc(), obj);
    current->add(elements);

    MInitializedLength *initLength = MInitializedLength::New(alloc(), elements);
    current->add(initLength);

    
    if (!needsHoleCheck && !failedBoundsCheck_) {
        addBoundsCheck(idInt32, initLength);
        return pushConstant(BooleanValue(true));
    }

    
    MInArray *ins = MInArray::New(alloc(), elements, id, initLength, obj, needsHoleCheck);

    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_instanceof()
{
    MDefinition *rhs = current->pop();
    MDefinition *obj = current->pop();

    
    
    do {
        types::TemporaryTypeSet *rhsTypes = rhs->resultTypeSet();
        JSObject *rhsObject = rhsTypes ? rhsTypes->getSingleton() : nullptr;
        if (!rhsObject || !rhsObject->is<JSFunction>() || rhsObject->isBoundFunction())
            break;

        types::TypeObjectKey *rhsType = types::TypeObjectKey::get(rhsObject);
        if (rhsType->unknownProperties())
            break;

        types::HeapTypeSetKey protoProperty =
            rhsType->property(NameToId(names().prototype));
        JSObject *protoObject = protoProperty.singleton(constraints());
        if (!protoObject)
            break;

        rhs->setImplicitlyUsedUnchecked();

        MInstanceOf *ins = MInstanceOf::New(alloc(), obj, protoObject);

        current->add(ins);
        current->push(ins);

        return resumeAfter(ins);
    } while (false);

    MCallInstanceOf *ins = MCallInstanceOf::New(alloc(), obj, rhs);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

MInstruction *
IonBuilder::addConvertElementsToDoubles(MDefinition *elements)
{
    MInstruction *convert = MConvertElementsToDoubles::New(alloc(), elements);
    current->add(convert);
    return convert;
}

MDefinition *
IonBuilder::addMaybeCopyElementsForWrite(MDefinition *object)
{
    if (!ElementAccessMightBeCopyOnWrite(constraints(), object))
        return object;
    MInstruction *copy = MMaybeCopyElementsForWrite::New(alloc(), object);
    current->add(copy);
    return copy;
}

MInstruction *
IonBuilder::addBoundsCheck(MDefinition *index, MDefinition *length)
{
    MInstruction *check = MBoundsCheck::New(alloc(), index, length);
    current->add(check);

    
    if (failedBoundsCheck_)
        check->setNotMovable();

    return check;
}

MInstruction *
IonBuilder::addShapeGuard(MDefinition *obj, Shape *const shape, BailoutKind bailoutKind)
{
    MGuardShape *guard = MGuardShape::New(alloc(), obj, shape, bailoutKind);
    current->add(guard);

    
    if (failedShapeGuard_)
        guard->setNotMovable();

    return guard;
}

types::TemporaryTypeSet *
IonBuilder::bytecodeTypes(jsbytecode *pc)
{
    return types::TypeScript::BytecodeTypes(script(), pc, bytecodeTypeMap, &typeArrayHint, typeArray);
}

TypedObjectPrediction
IonBuilder::typedObjectPrediction(MDefinition *typedObj)
{
    
    if (typedObj->isNewDerivedTypedObject()) {
        return typedObj->toNewDerivedTypedObject()->prediction();
    }

    types::TemporaryTypeSet *types = typedObj->resultTypeSet();
    return typedObjectPrediction(types);
}

TypedObjectPrediction
IonBuilder::typedObjectPrediction(types::TemporaryTypeSet *types)
{
    
    if (!types || types->getKnownMIRType() != MIRType_Object)
        return TypedObjectPrediction();

    
    if (types->unknownObject())
        return TypedObjectPrediction();

    TypedObjectPrediction out;
    for (uint32_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObject *type = types->getTypeObject(i);
        if (!type || type->unknownProperties())
            return TypedObjectPrediction();

        if (!IsTypedObjectClass(type->clasp()))
            return TypedObjectPrediction();

        TaggedProto proto = type->proto();

        
        
        JS_ASSERT(proto.isObject() && proto.toObject()->is<TypedProto>());

        TypedProto &typedProto = proto.toObject()->as<TypedProto>();
        out.addProto(typedProto);
    }

    return out;
}

MDefinition *
IonBuilder::loadTypedObjectType(MDefinition *typedObj)
{
    
    
    
    
    if (typedObj->isNewDerivedTypedObject())
        return typedObj->toNewDerivedTypedObject()->type();

    MInstruction *proto = MTypedObjectProto::New(alloc(), typedObj);
    current->add(proto);

    MInstruction *load = MLoadFixedSlot::New(alloc(), proto, JS_TYPROTO_SLOT_DESCR);
    current->add(load);

    MInstruction *unbox = MUnbox::New(alloc(), load, MIRType_Object, MUnbox::Infallible);
    current->add(unbox);

    return unbox;
}








void
IonBuilder::loadTypedObjectData(MDefinition *typedObj,
                                MDefinition *offset,
                                bool canBeNeutered,
                                MDefinition **owner,
                                MDefinition **ownerOffset)
{
    JS_ASSERT(typedObj->type() == MIRType_Object);
    JS_ASSERT(offset->type() == MIRType_Int32);

    
    
    
    
    
    if (typedObj->isNewDerivedTypedObject()) {
        MNewDerivedTypedObject *ins = typedObj->toNewDerivedTypedObject();

        
        
        

        MAdd *offsetAdd = MAdd::NewAsmJS(alloc(), ins->offset(), offset, MIRType_Int32);
        current->add(offsetAdd);

        *owner = ins->owner();
        *ownerOffset = offsetAdd;
        return;
    }

    if (canBeNeutered) {
        MNeuterCheck *chk = MNeuterCheck::New(alloc(), typedObj);
        current->add(chk);
        typedObj = chk;
    }

    *owner = typedObj;
    *ownerOffset = offset;
}






void
IonBuilder::loadTypedObjectElements(MDefinition *typedObj,
                                    MDefinition *offset,
                                    int32_t unit,
                                    bool canBeNeutered,
                                    MDefinition **ownerElements,
                                    MDefinition **ownerScaledOffset)
{
    MDefinition *owner, *ownerOffset;
    loadTypedObjectData(typedObj, offset, canBeNeutered, &owner, &ownerOffset);

    
    MTypedObjectElements *elements = MTypedObjectElements::New(alloc(), owner);
    current->add(elements);

    
    if (unit != 1) {
        MDiv *scaledOffset = MDiv::NewAsmJS(alloc(), ownerOffset, constantInt(unit), MIRType_Int32,
                                             false);
        current->add(scaledOffset);
        *ownerScaledOffset = scaledOffset;
    } else {
        *ownerScaledOffset = ownerOffset;
    }

    *ownerElements = elements;
}





bool
IonBuilder::typedObjectHasField(MDefinition *typedObj,
                                PropertyName *name,
                                size_t *fieldOffset,
                                TypedObjectPrediction *fieldPrediction,
                                size_t *fieldIndex)
{
    TypedObjectPrediction objPrediction = typedObjectPrediction(typedObj);
    if (objPrediction.isUseless())
        return false;

    
    if (objPrediction.kind() != type::Struct)
        return false;

    
    return objPrediction.hasFieldNamed(NameToId(name), fieldOffset,
                                       fieldPrediction, fieldIndex);
}

MDefinition *
IonBuilder::typeObjectForElementFromArrayStructType(MDefinition *typeObj)
{
    MInstruction *elemType = MLoadFixedSlot::New(alloc(), typeObj, JS_DESCR_SLOT_ARRAY_ELEM_TYPE);
    current->add(elemType);

    MInstruction *unboxElemType = MUnbox::New(alloc(), elemType, MIRType_Object, MUnbox::Infallible);
    current->add(unboxElemType);

    return unboxElemType;
}

MDefinition *
IonBuilder::typeObjectForFieldFromStructType(MDefinition *typeObj,
                                             size_t fieldIndex)
{
    

    MInstruction *fieldTypes = MLoadFixedSlot::New(alloc(), typeObj, JS_DESCR_SLOT_STRUCT_FIELD_TYPES);
    current->add(fieldTypes);

    MInstruction *unboxFieldTypes = MUnbox::New(alloc(), fieldTypes, MIRType_Object, MUnbox::Infallible);
    current->add(unboxFieldTypes);

    

    MInstruction *fieldTypesElements = MElements::New(alloc(), unboxFieldTypes);
    current->add(fieldTypesElements);

    MConstant *fieldIndexDef = constantInt(fieldIndex);

    MInstruction *fieldType = MLoadElement::New(alloc(), fieldTypesElements, fieldIndexDef, false, false);
    current->add(fieldType);

    MInstruction *unboxFieldType = MUnbox::New(alloc(), fieldType, MIRType_Object, MUnbox::Infallible);
    current->add(unboxFieldType);

    return unboxFieldType;
}

bool
IonBuilder::storeScalarTypedObjectValue(MDefinition *typedObj,
                                        MDefinition *byteOffset,
                                        ScalarTypeDescr::Type type,
                                        bool canBeNeutered,
                                        bool racy,
                                        MDefinition *value)
{
    
    MDefinition *elements, *scaledOffset;
    size_t alignment = ScalarTypeDescr::alignment(type);
    loadTypedObjectElements(typedObj, byteOffset, alignment, canBeNeutered,
                            &elements, &scaledOffset);

    
    MDefinition *toWrite = value;
    if (type == Scalar::Uint8Clamped) {
        toWrite = MClampToUint8::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }

    MStoreTypedArrayElement *store =
        MStoreTypedArrayElement::New(alloc(), elements, scaledOffset, toWrite,
                                     type);
    if (racy)
        store->setRacy();
    current->add(store);

    return true;
}

MConstant *
IonBuilder::constant(const Value &v)
{
    MConstant *c = MConstant::New(alloc(), v, constraints());
    current->add(c);
    return c;
}

MConstant *
IonBuilder::constantInt(int32_t i)
{
    return constant(Int32Value(i));
}

MDefinition *
IonBuilder::getCallee()
{
    if (inliningDepth_ == 0) {
        MInstruction *callee = MCallee::New(alloc());
        current->add(callee);
        return callee;
    }

    return inlineCallInfo_->fun();
}
