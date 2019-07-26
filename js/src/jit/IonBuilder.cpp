





#include "jit/IonBuilder.h"

#include "mozilla/DebugOnly.h"

#include "jsautooplen.h"

#include "builtin/Eval.h"
#include "builtin/TypedObject.h"
#include "builtin/TypeRepresentation.h"
#include "frontend/SourceNotes.h"
#include "jit/BaselineFrame.h"
#include "jit/BaselineInspector.h"
#include "jit/ExecutionModeInlines.h"
#include "jit/Ion.h"
#include "jit/IonSpewer.h"
#include "jit/Lowering.h"
#include "jit/MIRGraph.h"

#include "vm/ArgumentsObject.h"
#include "vm/RegExpStatics.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"

#include "jit/CompileInfo-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;
using mozilla::Maybe;

IonBuilder::IonBuilder(JSContext *analysisContext, CompileCompartment *comp, TempAllocator *temp, MIRGraph *graph,
                       types::CompilerConstraintList *constraints,
                       BaselineInspector *inspector, CompileInfo *info, BaselineFrame *baselineFrame,
                       size_t inliningDepth, uint32_t loopDepth)
  : MIRGenerator(comp, temp, graph, info),
    backgroundCodegen_(nullptr),
    analysisContext(analysisContext),
    baselineFrame_(baselineFrame),
    abortReason_(AbortReason_Disable),
    reprSetHash_(nullptr),
    constraints_(constraints),
    analysis_(*temp, info->script()),
    thisTypes(nullptr),
    argTypes(nullptr),
    typeArray(nullptr),
    typeArrayHint(0),
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
    failedBoundsCheck_(info->script()->failedBoundsCheck),
    failedShapeGuard_(info->script()->failedShapeGuard),
    nonStringIteration_(false),
    lazyArguments_(nullptr),
    inlineCallInfo_(nullptr)
{
    script_.init(info->script());
    pc = info->startPC();

    JS_ASSERT(script()->hasBaselineScript());
    JS_ASSERT(!!analysisContext == (info->executionMode() == DefinitePropertiesAnalysis));
}

void
IonBuilder::clearForBackEnd()
{
    JS_ASSERT(!analysisContext);
    baselineFrame_ = nullptr;

    
    
    
    
    gsn.purge();
}

bool
IonBuilder::abort(const char *message, ...)
{
    
#ifdef DEBUG
    va_list ap;
    va_start(ap, message);
    abortFmt(message, ap);
    va_end(ap);
    IonSpew(IonSpew_Abort, "aborted @ %s:%d", script()->filename(), PCToLineNumber(script(), pc));
#endif
    return false;
}

void
IonBuilder::spew(const char *message)
{
    
#ifdef DEBUG
    IonSpew(IonSpew_MIR, "%s @ %s:%d", message, script()->filename(), PCToLineNumber(script(), pc));
#endif
}

static inline int32_t
GetJumpOffset(jsbytecode *pc)
{
    JS_ASSERT(js_CodeSpec[JSOp(*pc)].type() == JOF_JUMP);
    return GET_JUMP_OFFSET(pc);
}

IonBuilder::CFGState
IonBuilder::CFGState::If(jsbytecode *join, MBasicBlock *ifFalse)
{
    CFGState state;
    state.state = IF_TRUE;
    state.stopAt = join;
    state.branch.ifFalse = ifFalse;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::IfElse(jsbytecode *trueEnd, jsbytecode *falseEnd, MBasicBlock *ifFalse)
{
    CFGState state;
    
    
    
    
    
    state.state = (falseEnd == ifFalse->pc())
                  ? IF_TRUE_EMPTY_ELSE
                  : IF_ELSE_TRUE;
    state.stopAt = trueEnd;
    state.branch.falseEnd = falseEnd;
    state.branch.ifFalse = ifFalse;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::AndOr(jsbytecode *join, MBasicBlock *joinStart)
{
    CFGState state;
    state.state = AND_OR;
    state.stopAt = join;
    state.branch.ifFalse = joinStart;
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
    JS_ASSERT(targets.length() == 0);
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

bool
IonBuilder::canEnterInlinedFunction(JSFunction *target)
{
    if (target->isHeavyweight())
        return false;

    JSScript *targetScript = target->nonLazyScript();

    if (targetScript->uninlineable)
        return false;

    if (!targetScript->analyzedArgsUsage())
        return false;

    if (targetScript->needsArgsObj())
        return false;

    if (!targetScript->compileAndGo)
        return false;

    types::TypeObjectKey *targetType = types::TypeObjectKey::get(target);
    if (targetType->unknownProperties())
        return false;

    return true;
}

bool
IonBuilder::canInlineTarget(JSFunction *target, CallInfo &callInfo)
{
    if (!target->isInterpreted()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to non-interpreted");
        return false;
    }

    
    
    if (target->isInterpreted() && info().executionMode() == DefinitePropertiesAnalysis) {
        if (!target->getOrCreateScript(analysisContext))
            return false;

        RootedScript script(analysisContext, target->nonLazyScript());
        if (!script->hasBaselineScript() && script->canBaselineCompile()) {
            MethodStatus status = BaselineCompile(analysisContext, script);
            if (status != Method_Compiled)
                return false;
        }
    }

    if (!target->hasScript()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to lack of Non-Lazy script");
        return false;
    }

    if (callInfo.constructing() && !target->isInterpretedConstructor()) {
        IonSpew(IonSpew_Inlining, "Cannot inline because callee is not a constructor");
        return false;
    }

    JSScript *inlineScript = target->nonLazyScript();
    ExecutionMode executionMode = info().executionMode();
    if (!CanIonCompile(inlineScript, executionMode)) {
        IonSpew(IonSpew_Inlining, "%s:%d Cannot inline due to disable Ion compilation",
                                  inlineScript->filename(), inlineScript->lineno);
        return false;
    }

    
    if (!inlineScript->hasBaselineScript()) {
        IonSpew(IonSpew_Inlining, "%s:%d Cannot inline target with no baseline jitcode",
                                  inlineScript->filename(), inlineScript->lineno);
        return false;
    }

    if (TooManyArguments(target->nargs)) {
        IonSpew(IonSpew_Inlining, "%s:%d Cannot inline too many args",
                                  inlineScript->filename(), inlineScript->lineno);
        return false;
    }

    if (TooManyArguments(callInfo.argc())) {
        IonSpew(IonSpew_Inlining, "%s:%d Cannot inline too many args",
                                  inlineScript->filename(), inlineScript->lineno);
        return false;
    }

    
    IonBuilder *builder = callerBuilder_;
    while (builder) {
        if (builder->script() == inlineScript) {
            IonSpew(IonSpew_Inlining, "%s:%d Not inlining recursive call",
                                       inlineScript->filename(), inlineScript->lineno);
            return false;
        }
        builder = builder->callerBuilder_;
    }

    if (!canEnterInlinedFunction(target)) {
        IonSpew(IonSpew_Inlining, "%s:%d Cannot inline due to oracle veto %d",
                                  inlineScript->filename(), inlineScript->lineno,
                                  script()->lineno);
        return false;
    }

    return true;
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

void
IonBuilder::analyzeNewLoopTypes(MBasicBlock *entry, jsbytecode *start, jsbytecode *end)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    
    
    for (size_t i = 0; i < loopHeaders_.length(); i++) {
        if (loopHeaders_[i].pc == start) {
            MBasicBlock *oldEntry = loopHeaders_[i].header;
            for (MPhiIterator oldPhi = oldEntry->phisBegin();
                 oldPhi != oldEntry->phisEnd();
                 oldPhi++)
            {
                MPhi *newPhi = entry->getSlot(oldPhi->slot())->toPhi();
                newPhi->addBackedgeType(oldPhi->type(), oldPhi->resultTypeSet());
            }
            
            
            
            loopHeaders_[i].header = entry;
            return;
        }
    }
    loopHeaders_.append(LoopHeader(start, entry));

    jsbytecode *last = nullptr, *earlier = nullptr;
    for (jsbytecode *pc = start; pc != end; earlier = last, last = pc, pc += GetBytecodeLength(pc)) {
        uint32_t slot;
        if (*pc == JSOP_SETLOCAL)
            slot = info().localSlot(GET_SLOTNO(pc));
        else if (*pc == JSOP_SETARG)
            slot = info().argSlotUnchecked(GET_SLOTNO(pc));
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
                MIRType type = MIRTypeFromValueType(typeSet->getKnownTypeTag());
                phi->addBackedgeType(type, typeSet);
            }
        } else if (*last == JSOP_GETLOCAL || *last == JSOP_GETARG) {
            uint32_t slot = (*last == JSOP_GETLOCAL)
                            ? info().localSlot(GET_SLOTNO(last))
                            : info().argSlotUnchecked(GET_SLOTNO(last));
            if (slot < info().firstStackSlot()) {
                MPhi *otherPhi = entry->getSlot(slot)->toPhi();
                if (otherPhi->hasBackedgeType())
                    phi->addBackedgeType(otherPhi->type(), otherPhi->resultTypeSet());
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
            if (type != MIRType_None)
                phi->addBackedgeType(type, nullptr);
        }
    }
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

    return true;
}

bool
IonBuilder::build()
{
    if (!init())
        return false;

    setCurrentAndSpecializePhis(newBlock(pc));
    if (!current)
        return false;

    IonSpew(IonSpew_Scripts, "Analyzing script %s:%d (%p) (usecount=%d)",
            script()->filename(), script()->lineno, (void *)script(), (int)script()->getUseCount());

    if (!initParameters())
        return false;

    
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

    
    current->makeStart(MStart::New(alloc(), MStart::StartType_Default));
    if (instrumentedProfiling())
        current->add(MFunctionBoundary::New(alloc(), script(), MFunctionBoundary::Enter));

    
    
    
    
    MCheckOverRecursed *check = new MCheckOverRecursed;
    current->add(check);
    check->setResumePoint(current->entryResumePoint());

    
    
    rewriteParameters();

    
    if (!initScopeChain())
        return false;

    if (info().needsArgsObj() && !initArgumentsObject())
        return false;

    
    if (info().fun())
        current->getSlot(info().thisSlot())->setGuard();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (uint32_t i = 0; i < info().endArgSlot(); i++) {
        MInstruction *ins = current->getEntrySlot(i)->toInstruction();
        if (ins->type() == MIRType_Value)
            ins->setResumePoint(current->entryResumePoint());
    }

    
    if (info().hasArguments() && !info().argsObjAliasesFormals()) {
        lazyArguments_ = MConstant::New(alloc(), MagicValue(JS_OPTIMIZED_ARGUMENTS));
        current->add(lazyArguments_);
    }

    if (!traverseBytecode())
        return false;

    if (!processIterators())
        return false;

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
        phi->setFoldedUnchecked();

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

    IonSpew(IonSpew_Scripts, "Inlining script %s:%d (%p)",
            script()->filename(), script()->lineno, (void *)script());

    callerBuilder_ = callerBuilder;
    callerResumePoint_ = callerResumePoint;

    if (callerBuilder->failedBoundsCheck_)
        failedBoundsCheck_ = true;

    if (callerBuilder->failedShapeGuard_)
        failedShapeGuard_ = true;

    
    setCurrentAndSpecializePhis(newBlock(pc));
    if (!current)
        return false;

    current->setCallerResumePoint(callerResumePoint);

    
    MBasicBlock *predecessor = callerBuilder->current;
    JS_ASSERT(predecessor == callerResumePoint->block());

    
    
    
    
    if (instrumentedProfiling())
        predecessor->add(MFunctionBoundary::New(alloc(), script(),
                                                MFunctionBoundary::Inline_Enter,
                                                inliningDepth_));

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

    IonSpew(IonSpew_Inlining, "Initializing %u arg slots", info().nargs());

    
    
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

    IonSpew(IonSpew_Inlining, "Initializing %u local slots", info().nlocals());

    
    for (uint32_t i = 0; i < info().nlocals(); i++) {
        MConstant *undef = MConstant::New(alloc(), UndefinedValue());
        current->add(undef);
        current->initSlot(info().localSlot(i), undef);
    }

    IonSpew(IonSpew_Inlining, "Inline entry block MResumePoint %p, %u operands",
            (void *) current->entryResumePoint(), current->entryResumePoint()->numOperands());

    
    JS_ASSERT(current->entryResumePoint()->numOperands() == info().totalSlots());

    if (script_->argumentsHasVarBinding()) {
        lazyArguments_ = MConstant::New(alloc(), MagicValue(JS_OPTIMIZED_ARGUMENTS));
        current->add(lazyArguments_);
    }

    if (!traverseBytecode())
        return false;

    return true;
}

void
IonBuilder::rewriteParameter(uint32_t slotIdx, MDefinition *param, int32_t argIndex)
{
    JS_ASSERT(param->isParameter() || param->isGetArgumentsObjectArg());

    types::TemporaryTypeSet *types = param->resultTypeSet();
    JSValueType definiteType = types->getKnownTypeTag();

    MDefinition *actual = ensureDefiniteType(param, definiteType);
    if (actual == param)
        return;

    
    
    
    
    
    
    
    
    
    current->rewriteSlot(slotIdx, actual);
}




void
IonBuilder::rewriteParameters()
{
    JS_ASSERT(info().scopeChainSlot() == 0);

    if (!info().fun())
        return;

    for (uint32_t i = info().startArgSlot(); i < info().endArgSlot(); i++) {
        MDefinition *param = current->getSlot(i);
        rewriteParameter(i, param, param->toParameter()->index());
    }
}

bool
IonBuilder::initParameters()
{
    if (!info().fun())
        return true;

    
    
    

    if (thisTypes->empty() && baselineFrame_) {
        if (!thisTypes->addType(types::GetValueType(baselineFrame_->thisValue()), alloc_->lifoAlloc()))
            return false;
    }

    MParameter *param = MParameter::New(alloc(), MParameter::THIS_SLOT, thisTypes);
    current->add(param);
    current->initSlot(info().thisSlot(), param);

    for (uint32_t i = 0; i < info().nargs(); i++) {
        types::TemporaryTypeSet *types = &argTypes[i];
        if (types->empty() && baselineFrame_ &&
            !script_->baselineScript()->modifiesArguments())
        {
            if (!types->addType(types::GetValueType(baselineFrame_->argv()[i]), alloc_->lifoAlloc()))
                return false;
        }

        param = MParameter::New(alloc(), i, types);
        current->add(param);
        current->initSlot(info().argSlotUnchecked(i), param);
    }

    return true;
}

bool
IonBuilder::initScopeChain(MDefinition *callee)
{
    MInstruction *scope = nullptr;

    
    
    
    
    if (!info().needsArgsObj() && !analysis().usesScopeChain())
        return true;

    
    
    
    
    if (!script()->compileAndGo)
        return abort("non-CNG global scripts are not supported");

    if (JSFunction *fun = info().fun()) {
        if (!callee) {
            MCallee *calleeIns = MCallee::New(alloc());
            current->add(calleeIns);
            callee = calleeIns;
        }
        scope = MFunctionEnvironment::New(alloc(), callee);
        current->add(scope);

        
        if (fun->isHeavyweight()) {
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
        scope = MConstant::New(alloc(), ObjectValue(script()->global()));
        current->add(scope);
    }

    current->setScopeChain(scope);
    return true;
}

bool
IonBuilder::initArgumentsObject()
{
    IonSpew(IonSpew_MIR, "%s:%d - Emitting code to initialize arguments object! block=%p",
                              script()->filename(), script()->lineno, current);
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
               type == MIRType_Magic)
    {
        
        
        types::Type ntype = types::Type::PrimitiveType(ValueTypeFromMIRType(type));
        typeSet = alloc_->lifoAlloc()->new_<types::TemporaryTypeSet>(ntype);
        if (!typeSet)
            return false;
        MInstruction *barrier = MTypeBarrier::New(alloc(), def, typeSet);
        osrBlock->insertBefore(osrBlock->lastIns(), barrier);
        osrBlock->rewriteSlot(slot, barrier);
        def = barrier;
    }

    if (type != def->type()) {
        switch (type) {
          case MIRType_Boolean:
          case MIRType_Int32:
          case MIRType_Double:
          case MIRType_String:
          case MIRType_Object:
          {
            MUnbox *unbox = MUnbox::New(alloc(), def, type, MUnbox::Fallible);
            osrBlock->insertBefore(osrBlock->lastIns(), unbox);
            osrBlock->rewriteSlot(slot, unbox);
            def = unbox;
            break;
          }

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

          case MIRType_Magic:
            JS_ASSERT(lazyArguments_);
            osrBlock->rewriteSlot(slot, lazyArguments_);
            def = lazyArguments_;
            break;

          default:
            break;
        }
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

    MPhiIterator headerPhi = header->phisBegin();
    while (headerPhi != header->phisEnd() && headerPhi->slot() < info().startArgSlot())
        headerPhi++;

    for (uint32_t i = info().startArgSlot(); i < osrBlock->stackDepth(); i++, headerPhi++) {

        
        
        
        if (info().isSlotAliased(i))
            continue;

        MInstruction *def = osrBlock->getSlot(i)->toInstruction();

        JS_ASSERT(headerPhi->slot() == i);
        MPhi *preheaderPhi = preheader->getSlot(i)->toPhi();

        MIRType type = headerPhi->type();
        types::TemporaryTypeSet *typeSet = headerPhi->resultTypeSet();

        if (!addOsrValueTypeBarrier(i, &def, type, typeSet))
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
                    return maybeAddOsrTypeBarriers();
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
                return maybeAddOsrTypeBarriers();
        }

#ifdef DEBUG
        
        
        
        
        
        
        
        
        
        Vector<MDefinition *, 4, IonAllocPolicy> popped(alloc());
        Vector<size_t, 4, IonAllocPolicy> poppedUses(alloc());
        unsigned nuses = GetUseCount(script_, pc - script_->code);

        for (unsigned i = 0; i < nuses; i++) {
            MDefinition *def = current->peek(-int32_t(i + 1));
            if (!popped.append(def) || !poppedUses.append(def->defUseCount()))
                return false;
        }
#endif

        
        JSOp op = JSOp(*pc);
        if (!inspectOpcode(op))
            return false;

#ifdef DEBUG
        for (size_t i = 0; i < popped.length(); i++) {
            
            if (popped[i]->isPassArg() && !popped[i]->hasUses())
                continue;

            switch (op) {
              case JSOP_POP:
              case JSOP_POPN:
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
                JS_ASSERT(popped[i]->isFolded() ||

                          
                          
                          
                          
                          popped[i]->isNewDerivedTypedObject() ||

                          popped[i]->defUseCount() > poppedUses[i]);
                break;
            }
        }
#endif

        pc += js_CodeSpec[op].length;
        current->updateTrackedPc(pc);
    }

    return maybeAddOsrTypeBarriers();
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
            
            MOZ_ASSUME_UNREACHABLE("unknown goto case");
        }
        break;
      }

      case JSOP_TABLESWITCH:
        return tableSwitch(op, info().getNote(gsn, pc));

      case JSOP_IFNE:
        
        
        MOZ_ASSUME_UNREACHABLE("we should never reach an ifne!");

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

      case JSOP_NOTEARG:
        return jsop_notearg();

      case JSOP_GETARG:
      case JSOP_CALLARG:
        if (info().argsObjAliasesFormals()) {
            MGetArgumentsObjectArg *getArg = MGetArgumentsObjectArg::New(alloc(),
                                                                         current->argumentsObject(),
                                                                         GET_SLOTNO(pc));
            current->add(getArg);
            current->push(getArg);
        } else {
            current->pushArg(GET_SLOTNO(pc));
        }
        return true;

      case JSOP_SETARG:
        return jsop_setarg(GET_SLOTNO(pc));

      case JSOP_GETLOCAL:
      case JSOP_CALLLOCAL:
        current->pushLocal(GET_SLOTNO(pc));
        return true;

      case JSOP_SETLOCAL:
        current->setLocal(GET_SLOTNO(pc));
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

      case JSOP_NEWINIT:
        if (GET_UINT8(pc) == JSProto_Array)
            return jsop_newarray(0);
        return jsop_newobject();

      case JSOP_NEWARRAY:
        return jsop_newarray(GET_UINT24(pc));

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
      case JSOP_CALLGNAME:
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        JSObject *obj = &script()->global();
        bool succeeded;
        if (!getStaticName(obj, name, &succeeded))
            return false;
        return succeeded || jsop_getname(name);
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
      case JSOP_CALLNAME:
      {
        PropertyName *name = info().getAtom(pc)->asPropertyName();
        return jsop_getname(name);
      }

      case JSOP_GETINTRINSIC:
      case JSOP_CALLINTRINSIC:
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
      case JSOP_CALLALIASEDVAR:
        return jsop_getaliasedvar(ScopeCoordinate(pc));

      case JSOP_SETALIASEDVAR:
        return jsop_setaliasedvar(ScopeCoordinate(pc));

      case JSOP_UINT24:
        return pushConstant(Int32Value(GET_UINT24(pc)));

      case JSOP_INT32:
        return pushConstant(Int32Value(GET_INT32(pc)));

      case JSOP_LOOPHEAD:
        
        MOZ_ASSUME_UNREACHABLE("JSOP_LOOPHEAD outside loop");

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

      case JSOP_CALLEE:
      {
        MDefinition *callee;
        if (inliningDepth_ == 0) {
            MInstruction *calleeIns = MCallee::New(alloc());
            current->add(calleeIns);
            callee = calleeIns;
        } else {
            callee = inlineCallInfo_->fun();
        }
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

      case JSOP_OBJECT:
        return jsop_object(info().getObject(pc));

      case JSOP_TYPEOF:
      case JSOP_TYPEOFEXPR:
        return jsop_typeof();

      case JSOP_TOID:
        return jsop_toid();

      case JSOP_LAMBDA:
        return jsop_lambda(info().getFunction(pc));

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
        JS_ASSERT(!script()->noScriptRval);
        current->setSlot(info().returnValueSlot(), current->pop());
        return true;

      case JSOP_INSTANCEOF:
        return jsop_instanceof();

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
        MOZ_ASSUME_UNREACHABLE("unknown cfgstate");
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

    setCurrentAndSpecializePhis(state.branch.ifFalse);
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
    setCurrentAndSpecializePhis(state.branch.ifFalse);
    graph().moveBlockToEnd(current);
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

    
    setCurrentAndSpecializePhis(join);
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

    
    
    
    setCurrentAndSpecializePhis(state.loop.successor);
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

        setCurrentAndSpecializePhis(block);
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

    setCurrentAndSpecializePhis(successor);

    
    if (!current)
        return ControlStatus_Ended;

    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::restartLoop(CFGState state)
{
    spew("New types at loop header, restarting loop body");

    if (js_IonOptions.limitScriptSize) {
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
    setCurrentAndSpecializePhis(header);
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

    
    MTest *test = MTest::New(alloc(), vins, state.loop.entry, successor);
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
        test = MTest::New(alloc(), ins, body, state.loop.successor);
    else
        test = MTest::New(alloc(), ins, state.loop.successor, body);
    current->end(test);

    state.state = CFGState::WHILE_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    setCurrentAndSpecializePhis(body);
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

    MTest *test = MTest::New(alloc(), ins, body, state.loop.successor);
    current->end(test);

    state.state = CFGState::FOR_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    setCurrentAndSpecializePhis(body);
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
                return ControlStatus_Error;
        }

        
        
        edge->block->end(MGoto::New(alloc(), update));
        edge = edge->next;

        
        while (edge) {
            edge->block->end(MGoto::New(alloc(), update));
            if (!update->addPredecessor(alloc(), edge->block))
                return ControlStatus_Error;
            edge = edge->next;
        }
        state.loop.continues = nullptr;

        setCurrentAndSpecializePhis(update);
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
        successor->addPredecessor(alloc(), current);
    }

    
    graph().moveBlockToEnd(successor);

    
    
    if (state.tableswitch.currentBlock+1 < state.tableswitch.ins->numBlocks())
        state.stopAt = state.tableswitch.ins->getBlock(state.tableswitch.currentBlock+1)->pc();
    else
        state.stopAt = state.tableswitch.exitpc;

    setCurrentAndSpecializePhis(successor);
    pc = current->pc();
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processAndOrEnd(CFGState &state)
{
    
    
    current->end(MGoto::New(alloc(), state.branch.ifFalse));

    if (!state.branch.ifFalse->addPredecessor(alloc(), current))
        return ControlStatus_Error;

    setCurrentAndSpecializePhis(state.branch.ifFalse);
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
        successor->addPredecessor(alloc(), current);
    }

    pc = state.stopAt;
    setCurrentAndSpecializePhis(successor);
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

    
    setCurrentAndSpecializePhis(state.try_.successor);
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
                cfg.label.breaks = new DeferredEdge(current, cfg.label.breaks);
                found = true;
                break;
            }
        }
    } else {
        for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
            CFGState &cfg = cfgStack_[loops_[i].cfgEntry];
            JS_ASSERT(cfg.isLoop());
            if (cfg.loop.exitpc == target) {
                cfg.loop.breaks = new DeferredEdge(current, cfg.loop.breaks);
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

    state.loop.continues = new DeferredEdge(current, state.loop.continues);

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
        MOZ_ASSUME_UNREACHABLE("Unexpected switch state.");
    }

    *breaks = new DeferredEdge(current, *breaks);

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
        if (breaks)
            successor->addPredecessor(alloc(), current);
    }

    pc = exitpc;
    setCurrentAndSpecializePhis(successor);
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
        MOZ_ASSUME_UNREACHABLE("unexpected opcode");
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
            MOZ_ASSUME_UNREACHABLE("JSOP_LOOPHEAD unexpected source note");
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
    bool osr = info().hasOsrAt(loopEntry);

    if (osr) {
        MBasicBlock *preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(alloc(), preheader));
        setCurrentAndSpecializePhis(preheader);
    }

    MBasicBlock *header = newPendingLoopHeader(current, pc, osr);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(alloc(), header));

    jsbytecode *loophead = GetNextPc(pc);
    jsbytecode *bodyStart = GetNextPc(loophead);
    jsbytecode *bodyEnd = conditionpc;
    jsbytecode *exitpc = GetNextPc(ifne);
    analyzeNewLoopTypes(header, bodyStart, exitpc);
    if (!pushLoop(CFGState::DO_WHILE_LOOP_BODY, conditionpc, header, osr,
                  loopHead, bodyStart, bodyStart, bodyEnd, exitpc, conditionpc))
    {
        return ControlStatus_Error;
    }

    CFGState &state = cfgStack_.back();
    state.loop.updatepc = conditionpc;
    state.loop.updateEnd = ifne;

    setCurrentAndSpecializePhis(header);
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
    bool osr = info().hasOsrAt(loopEntry);

    if (osr) {
        MBasicBlock *preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(alloc(), preheader));
        setCurrentAndSpecializePhis(preheader);
    }

    MBasicBlock *header = newPendingLoopHeader(current, pc, osr);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(alloc(), header));

    
    jsbytecode *loopHead = GetNextPc(pc);
    jsbytecode *bodyStart = GetNextPc(loopHead);
    jsbytecode *bodyEnd = pc + GetJumpOffset(pc);
    jsbytecode *exitpc = GetNextPc(ifne);
    analyzeNewLoopTypes(header, bodyStart, exitpc);
    if (!pushLoop(CFGState::WHILE_LOOP_COND, ifne, header, osr,
                  loopHead, bodyEnd, bodyStart, bodyEnd, exitpc))
    {
        return ControlStatus_Error;
    }

    
    setCurrentAndSpecializePhis(header);
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

    if (osr) {
        MBasicBlock *preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(alloc(), preheader));
        setCurrentAndSpecializePhis(preheader);
    }

    MBasicBlock *header = newPendingLoopHeader(current, pc, osr);
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

    analyzeNewLoopTypes(header, bodyStart, exitpc);
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

    setCurrentAndSpecializePhis(header);
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
            defaultcase->addPredecessor(alloc(), caseblock);
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
    setCurrentAndSpecializePhis(tableswitch->getBlock(0));

    if (!cfgStack_.append(state))
        return ControlStatus_Error;

    pc = current->pc();
    return ControlStatus_Jumped;
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
    if (!state.condswitch.bodies || !state.condswitch.bodies->init(nbBodies))
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
        current->end(MTest::New(alloc(), cmpResult, bodyBlock, caseBlock));

        
        
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

    
    setCurrentAndSpecializePhis(caseBlock);
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
        nextBody->addPredecessor(alloc(), current);
    }

    
    setCurrentAndSpecializePhis(nextBody);
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
                  ? MTest::New(alloc(), lhs, evalRhs, join)
                  : MTest::New(alloc(), lhs, join, evalRhs);
    test->infer();
    current->end(test);

    if (!cfgStack_.append(CFGState::AndOr(joinStart, join)))
        return false;

    setCurrentAndSpecializePhis(evalRhs);
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
IonBuilder::jsop_loophead(jsbytecode *pc)
{
    assertValidLoopHeadOp(pc);

    current->add(MInterruptCheck::New(alloc()));

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

    MTest *test = MTest::New(alloc(), ins, ifTrue, ifFalse);
    current->end(test);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    switch (SN_TYPE(sn)) {
      case SRC_IF:
        if (!cfgStack_.append(CFGState::If(falseStart, ifFalse)))
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

        if (!cfgStack_.append(CFGState::IfElse(trueEnd, falseEnd, ifFalse)))
            return false;
        break;
      }

      default:
        MOZ_ASSUME_UNREACHABLE("unexpected source note type");
    }

    
    
    setCurrentAndSpecializePhis(ifTrue);

    return true;
}

bool
IonBuilder::jsop_try()
{
    JS_ASSERT(JSOp(*pc) == JSOP_TRY);

    if (!js_IonOptions.compileTryCatch)
        return abort("Try-catch support disabled");

    
    if (analysis().hasTryFinally())
        return abort("Has try-finally");

    
    if (isInlineBuilder())
        return abort("try-catch within inline frame");

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
        current->end(MTest::New(alloc(), true_, tryBlock, successor));
    } else {
        successor = nullptr;
        current->end(MGoto::New(alloc(), tryBlock));
    }

    if (!cfgStack_.append(CFGState::Try(endpc, successor)))
        return false;

    
    
    JS_ASSERT(info().osrPc() < endpc || info().osrPc() >= afterTry);

    
    setCurrentAndSpecializePhis(tryBlock);
    return true;
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
        
        if (script()->noScriptRval) {
            MInstruction *ins = MConstant::New(alloc(), UndefinedValue());
            current->add(ins);
            def = ins;
            break;
        }

        def = current->getSlot(info().returnValueSlot());
        break;

      default:
        def = nullptr;
        MOZ_ASSUME_UNREACHABLE("unknown return op");
    }

    if (instrumentedProfiling())
        current->add(MFunctionBoundary::New(alloc(), script(), MFunctionBoundary::Exit));
    MReturn *ret = MReturn::New(alloc(), def);
    current->end(ret);

    if (!graph().addExit(current))
        return ControlStatus_Error;

    
    setCurrent(nullptr);
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processThrow()
{
    
    if (isInlineBuilder())
        return ControlStatus_Abort;

    MDefinition *def = current->pop();

    if (graph().hasTryBlock()) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        MNop *ins = MNop::New(alloc());
        current->add(ins);

        if (!resumeAfter(ins))
            return ControlStatus_Error;
    }

    MThrow *ins = MThrow::New(alloc(), def);
    current->end(ins);

    if (!graph().addExit(current))
        return ControlStatus_Error;

    
    setCurrent(nullptr);
    return processControlEnd();
}

bool
IonBuilder::pushConstant(const Value &v)
{
    MConstant *ins = MConstant::New(alloc(), v);
    current->add(ins);
    current->push(ins);
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
        MOZ_ASSUME_UNREACHABLE("unexpected bitop");
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
        MOZ_ASSUME_UNREACHABLE("unexpected binary opcode");
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

bool
IonBuilder::jsop_notearg()
{
    
    
    MDefinition *def = current->pop();
    MPassArg *arg = MPassArg::New(alloc(), def);

    current->add(arg);
    current->push(arg);
    return true;
}

class AutoAccumulateExits
{
    MIRGraph &graph_;
    MIRGraphExits *prev_;

  public:
    AutoAccumulateExits(MIRGraph &graph, MIRGraphExits &exits)
      : graph_(graph)
    {
        prev_ = graph_.exitAccumulator();
        graph_.setExitAccumulator(&exits);
    }
    ~AutoAccumulateExits() {
        graph_.setExitAccumulator(prev_);
    }
};

bool
IonBuilder::inlineScriptedCall(CallInfo &callInfo, JSFunction *target)
{
    JS_ASSERT(target->isInterpreted());
    JS_ASSERT(IsIonInlinablePC(pc));

    
    if (callInfo.isWrapped())
        callInfo.unwrapArgs();

    
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
            MTypeBarrier *barrier =
                MTypeBarrier::New(alloc(), callInfo.thisArg(), types->clone(alloc_->lifoAlloc()));
            current->add(barrier);
            callInfo.setThis(barrier);
        }
    }

    
    LifoAlloc *lifoAlloc = alloc_->lifoAlloc();
    CompileInfo *info = lifoAlloc->new_<CompileInfo>(calleeScript, target,
                                                     (jsbytecode *)nullptr, callInfo.constructing(),
                                                     this->info().executionMode());
    if (!info)
        return false;

    MIRGraphExits saveExits(alloc());
    AutoAccumulateExits aae(graph(), saveExits);

    
    JS_ASSERT_IF(analysisContext, !analysisContext->isExceptionPending());
    IonBuilder inlineBuilder(analysisContext, compartment,
                             &alloc(), &graph(), constraints(), &inspector, info, nullptr,
                             inliningDepth_ + 1, loopDepth_);
    if (!inlineBuilder.buildInline(this, outerResumePoint, callInfo)) {
        if (analysisContext && analysisContext->isExceptionPending()) {
            IonSpew(IonSpew_Abort, "Inline builder raised exception.");
            abortReason_ = AbortReason_Error;
            return false;
        }

        
        
        if (inlineBuilder.abortReason_ == AbortReason_Disable) {
            calleeScript->uninlineable = true;
            abortReason_ = AbortReason_Inlining;
        } else if (inlineBuilder.abortReason_ == AbortReason_Inlining) {
            abortReason_ = AbortReason_Inlining;
        }

        return false;
    }

    
    jsbytecode *postCall = GetNextPc(pc);
    MBasicBlock *returnBlock = newBlock(nullptr, postCall);
    if (!returnBlock)
        return false;
    returnBlock->setCallerResumePoint(callerResumePoint_);

    
    if (instrumentedProfiling())
        returnBlock->add(MFunctionBoundary::New(alloc(), nullptr, MFunctionBoundary::Inline_Exit));

    
    returnBlock->inheritSlots(current);
    returnBlock->pop();

    
    MIRGraphExits &exits = *inlineBuilder.graph().exitAccumulator();
    if (exits.length() == 0) {
        
        calleeScript->uninlineable = true;
        abortReason_ = AbortReason_Inlining;
        return false;
    }
    MDefinition *retvalDefn = patchInlinedReturns(callInfo, exits, returnBlock);
    if (!retvalDefn)
        return false;
    returnBlock->push(retvalDefn);

    
    if (!returnBlock->initEntrySlots(alloc()))
        return false;

    setCurrentAndSpecializePhis(returnBlock);
    return true;
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
IonBuilder::patchInlinedReturns(CallInfo &callInfo, MIRGraphExits &exits, MBasicBlock *bottom)
{
    
    
    JS_ASSERT(exits.length() > 0);

    if (exits.length() == 1)
        return patchInlinedReturn(callInfo, exits[0], bottom);

    
    MPhi *phi = MPhi::New(alloc(), bottom->stackDepth());
    if (!phi->reserveLength(exits.length()))
        return nullptr;

    for (size_t i = 0; i < exits.length(); i++) {
        MDefinition *rdef = patchInlinedReturn(callInfo, exits[i], bottom);
        if (!rdef)
            return nullptr;
        phi->addInput(rdef);
    }

    bottom->addPhi(phi);
    return phi;
}

static bool
IsSmallFunction(JSScript *script)
{
    return script->length <= js_IonOptions.smallFunctionMaxBytecodeLength;
}

bool
IonBuilder::makeInliningDecision(JSFunction *target, CallInfo &callInfo)
{
    
    if (!inliningEnabled())
        return false;

    
    if (target == nullptr)
        return false;

    
    if (target->isNative())
        return true;

    
    if (!canInlineTarget(target, callInfo))
        return false;

    
    JSScript *targetScript = target->nonLazyScript();

    
    if (!targetScript->shouldInline) {
        
        if (IsSmallFunction(targetScript)) {
            if (inliningDepth_ >= js_IonOptions.smallFunctionMaxInlineDepth) {
                IonSpew(IonSpew_Inlining, "%s:%d - Vetoed: exceeding allowed inline depth",
                        targetScript->filename(), targetScript->lineno);
                return false;
            }
        } else {
            if (inliningDepth_ >= js_IonOptions.maxInlineDepth) {
                IonSpew(IonSpew_Inlining, "%s:%d - Vetoed: exceeding allowed inline depth",
                        targetScript->filename(), targetScript->lineno);
                return false;
            }

            if (targetScript->hasLoops()) {
                IonSpew(IonSpew_Inlining, "%s:%d - Vetoed: big function that contains a loop",
                        targetScript->filename(), targetScript->lineno);
                return false;
            }
        }

        
        
        if (targetScript->length > js_IonOptions.inlineMaxTotalBytecodeLength) {
            IonSpew(IonSpew_Inlining, "%s:%d - Vetoed: callee excessively large.",
                    targetScript->filename(), targetScript->lineno);
            return false;
        }

        
        
        uint32_t callerUses = script()->getUseCount();
        if (callerUses < js_IonOptions.usesBeforeInlining() &&
            info().executionMode() != DefinitePropertiesAnalysis)
        {
            IonSpew(IonSpew_Inlining, "%s:%d - Vetoed: caller is insufficiently hot.",
                    targetScript->filename(), targetScript->lineno);
            return false;
        }

        
        if (targetScript->getUseCount() * js_IonOptions.inlineUseCountRatio < callerUses &&
            info().executionMode() != DefinitePropertiesAnalysis)
        {
            IonSpew(IonSpew_Inlining, "%s:%d - Vetoed: callee is not hot.",
                    targetScript->filename(), targetScript->lineno);
            return false;
        }
    }

    
    types::TypeObjectKey *targetType = types::TypeObjectKey::get(target);
    targetType->watchStateChangeForInlinedCall(constraints());

    return true;
}

uint32_t
IonBuilder::selectInliningTargets(ObjectVector &targets, CallInfo &callInfo, BoolVector &choiceSet)
{
    uint32_t totalSize = 0;
    uint32_t numInlineable = 0;

    
    if (!choiceSet.reserve(targets.length()))
        return false;
    for (size_t i = 0; i < targets.length(); i++) {
        JSFunction *target = &targets[i]->as<JSFunction>();
        bool inlineable = makeInliningDecision(target, callInfo);

        
        if (inlineable && target->isInterpreted()) {
            totalSize += target->nonLazyScript()->length;
            if (totalSize > js_IonOptions.inlineMaxTotalBytecodeLength)
                inlineable = false;
        }

        choiceSet.append(inlineable);
        if (inlineable)
            numInlineable++;
    }

    JS_ASSERT(choiceSet.length() == targets.length());
    return numInlineable;
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

MGetPropertyCache *
IonBuilder::getInlineableGetPropertyCache(CallInfo &callInfo)
{
    if (callInfo.constructing())
        return nullptr;

    MDefinition *thisDef = callInfo.thisArg();
    if (thisDef->type() != MIRType_Object)
        return nullptr;

    
    if (thisDef->isPassArg())
        thisDef = thisDef->toPassArg()->getArgument();

    MDefinition *funcDef = callInfo.fun();
    if (funcDef->type() != MIRType_Object)
        return nullptr;

    
    if (funcDef->isGetPropertyCache()) {
        MGetPropertyCache *cache = funcDef->toGetPropertyCache();
        if (cache->hasUses())
            return nullptr;
        if (!CanInlineGetPropertyCache(cache, thisDef))
            return nullptr;
        return cache;
    }

    
    
    if (funcDef->isTypeBarrier()) {
        MTypeBarrier *barrier = funcDef->toTypeBarrier();
        if (barrier->hasUses())
            return nullptr;
        if (barrier->type() != MIRType_Object)
            return nullptr;
        if (!barrier->input()->isGetPropertyCache())
            return nullptr;

        MGetPropertyCache *cache = barrier->input()->toGetPropertyCache();
        if (cache->hasUses() && !cache->hasOneUse())
            return nullptr;
        if (!CanInlineGetPropertyCache(cache, thisDef))
            return nullptr;
        return cache;
    }

    return nullptr;
}

IonBuilder::InliningStatus
IonBuilder::inlineSingleCall(CallInfo &callInfo, JSFunction *target)
{
    
    if (target->isNative())
        return inlineNativeCall(callInfo, target->native());

    if (!inlineScriptedCall(callInfo, target))
        return InliningStatus_Error;
    return InliningStatus_Inlined;
}

IonBuilder::InliningStatus
IonBuilder::inlineCallsite(ObjectVector &targets, ObjectVector &originals,
                           bool lambda, CallInfo &callInfo)
{
    if (!inliningEnabled())
        return InliningStatus_NotInlined;

    if (targets.length() == 0)
        return InliningStatus_NotInlined;

    
    
    
    MGetPropertyCache *propCache = getInlineableGetPropertyCache(callInfo);

    
    
    if (!propCache && targets.length() == 1) {
        JSFunction *target = &targets[0]->as<JSFunction>();
        if (!makeInliningDecision(target, callInfo))
            return InliningStatus_NotInlined;

        
        
        
        callInfo.fun()->setFoldedUnchecked();

        
        
        
        if (!lambda) {
            
            MConstant *constFun = MConstant::New(alloc(), ObjectValue(*target));
            current->add(constFun);
            callInfo.setFun(constFun);
        }

        return inlineSingleCall(callInfo, target);
    }

    
    BoolVector choiceSet(alloc());
    uint32_t numInlined = selectInliningTargets(targets, callInfo, choiceSet);
    if (numInlined == 0)
        return InliningStatus_NotInlined;

    
    if (!inlineCalls(callInfo, targets, originals, choiceSet, propCache))
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
    fallbackInfo.wrapArgs(alloc(), fallbackBlock);

    
    setCurrentAndSpecializePhis(fallbackBlock);
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
    JS_ASSERT(propTable->pc() != nullptr);
    JS_ASSERT(propTable->priorResumePoint() != nullptr);
    MBasicBlock *getPropBlock = newBlock(prepBlock, propTable->pc(), propTable->priorResumePoint());
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

    
    JS_ASSERT(callInfo.isWrapped());
    callInfo.unwrapArgs();
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
        callInfo.fun()->setFoldedUnchecked();
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

    MPhi *retPhi = MPhi::New(alloc(), returnBlock->stackDepth());
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

        
        MConstant *funcDef = MConstant::New(alloc(), ObjectValue(*target));
        funcDef->setFoldedUnchecked();
        dispatchBlock->add(funcDef);

        
        int funIndex = inlineBlock->entryResumePoint()->numOperands() - callInfo.numFormals();
        inlineBlock->entryResumePoint()->replaceOperand(funIndex, funcDef);
        inlineBlock->rewriteSlot(funIndex, funcDef);

        
        CallInfo inlineInfo(alloc(), callInfo.constructing());
        if (!inlineInfo.init(callInfo))
            return false;
        inlineInfo.popFormals(inlineBlock);
        inlineInfo.setFun(funcDef);
        inlineInfo.wrapArgs(alloc(), inlineBlock);

        if (maybeCache) {
            JS_ASSERT(callInfo.thisArg() == maybeCache->object());
            types::TemporaryTypeSet *targetThisTypes =
                maybeCache->propTable()->buildTypeSetForFunction(original);
            if (!targetThisTypes)
                return false;
            maybeCache->object()->setResultTypeSet(targetThisTypes);
        }

        
        setCurrentAndSpecializePhis(inlineBlock);
        InliningStatus status = inlineSingleCall(inlineInfo, target);
        if (status == InliningStatus_Error)
            return false;

        
        if (status == InliningStatus_NotInlined) {
            JS_ASSERT(target->isNative());
            JS_ASSERT(current == inlineBlock);
            inlineBlock->discardAllResumePoints();
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
    setCurrentAndSpecializePhis(returnBlock);
    return true;
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

    
    MInstruction *slots;
    if (templateObj->hasDynamicSlots()) {
        size_t nslots = JSObject::dynamicSlotsCount(templateObj->numFixedSlotsForCompilation(),
                                                    templateObj->lastProperty()->slotSpan(templateObj->getClass()));
        slots = MNewSlots::New(alloc(), nslots);
    } else {
        slots = MConstant::New(alloc(), NullValue());
    }
    current->add(slots);

    
    
    
    
    MInstruction *callObj = MNewCallObject::New(alloc(), templateObj, script()->treatAsRunOnce, slots);
    current->add(callObj);

    
    
    if (templateObj->type()->isLongLivedForJITAlloc())
        current->add(MPostWriteBarrier::New(alloc(), callObj));

    
    
    current->add(MStoreFixedSlot::New(alloc(), callObj, CallObject::enclosingScopeSlot(), scope));
    current->add(MStoreFixedSlot::New(alloc(), callObj, CallObject::calleeSlot(), callee));

    
    for (AliasedFormalIter i(script()); i; i++) {
        unsigned slot = i.scopeSlot();
        unsigned formal = i.frameIndex();
        MDefinition *param = current->getSlot(info().argSlotUnchecked(formal));
        if (slot >= templateObj->numFixedSlotsForCompilation())
            current->add(MStoreSlot::New(alloc(), slots, slot - templateObj->numFixedSlotsForCompilation(), param));
        else
            current->add(MStoreFixedSlot::New(alloc(), callObj, slot, param));
    }

    return callObj;
}

MDefinition *
IonBuilder::createThisScripted(MDefinition *callee)
{
    
    
    
    
    
    
    
    
    
    
    
    MInstruction *getProto;
    if (!invalidatedIdempotentCache()) {
        MGetPropertyCache *getPropCache = MGetPropertyCache::New(alloc(), callee, names().prototype);
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
    if (templateObject->getProto() != proto)
        return nullptr;

    if (!target->nonLazyScript()->types)
        return nullptr;
    if (!types::TypeScript::ThisTypes(target->nonLazyScript())->hasType(types::Type::ObjectType(templateObject)))
        return nullptr;

    
    
    
    
    types::TypeObject *templateType = templateObject->type();
    if (templateType->hasNewScript()) {
        templateObject = templateType->newScript()->templateObject;
        JS_ASSERT(templateObject->type() == templateType);

        
        types::TypeObjectKey::get(templateType)->watchStateChangeForNewScriptTemplate(constraints());
    }

    
    
    MCreateThisWithTemplate *createThis = MCreateThisWithTemplate::New(alloc(), templateObject);
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
    current->peek(calleeDepth)->setFoldedUnchecked();

    
    types::TemporaryTypeSet *funTypes = current->peek(funcDepth)->resultTypeSet();
    JSFunction *target = getSingleCallTarget(funTypes);

    
    MPassArg *passFunc = current->peek(funcDepth)->toPassArg();
    current->rewriteAtDepth(funcDepth, passFunc->getArgument());

    
    passFunc->replaceAllUsesWith(passFunc->getArgument());
    passFunc->block()->discard(passFunc);

    
    current->shimmySlots(funcDepth - 1);

    
    
    if (argc == 0) {
        MConstant *undef = MConstant::New(alloc(), UndefinedValue());
        current->add(undef);
        MPassArg *pass = MPassArg::New(alloc(), undef);
        current->add(pass);
        current->push(pass);
    } else {
        
        argc -= 1;
    }

    CallInfo callInfo(alloc(), false);
    if (!callInfo.init(current, argc))
        return false;

    
    if (argc > 0 && makeInliningDecision(target, callInfo) && target->isInterpreted())
        return inlineScriptedCall(callInfo, target);

    
    return makeCall(target, callInfo, false);
}

bool
IonBuilder::jsop_funapply(uint32_t argc)
{
    int calleeDepth = -((int)argc + 2);

    types::TemporaryTypeSet *calleeTypes = current->peek(calleeDepth)->resultTypeSet();
    JSFunction *native = getSingleCallTarget(calleeTypes);
    if (argc != 2) {
        CallInfo callInfo(alloc(), false);
        if (!callInfo.init(current, argc))
            return false;
        return makeCall(native, callInfo, false);
    }

    
    
    MDefinition *argument = current->peek(-1);
    if (script()->argumentsHasVarBinding() &&
        argument->mightBeType(MIRType_Magic) &&
        argument->type() != MIRType_Magic)
    {
        return abort("fun.apply with MaybeArguments");
    }

    
    if (argument->type() != MIRType_Magic) {
        CallInfo callInfo(alloc(), false);
        if (!callInfo.init(current, argc))
            return false;
        return makeCall(native, callInfo, false);
    }

    if (!native ||
        !native->isNative() ||
        native->native() != js_fun_apply)
    {
        return abort("fun.apply speculation failed");
    }

    current->peek(calleeDepth)->setFoldedUnchecked();

    
    return jsop_funapplyarguments(argc);
}

bool
IonBuilder::jsop_funapplyarguments(uint32_t argc)
{
    
    
    
    
    

    int funcDepth = -((int)argc + 1);

    
    types::TemporaryTypeSet *funTypes = current->peek(funcDepth)->resultTypeSet();
    JSFunction *target = getSingleCallTarget(funTypes);

    
    
    if (inliningDepth_ == 0 && info().executionMode() != DefinitePropertiesAnalysis) {

        
        MPassArg *passVp = current->pop()->toPassArg();
        passVp->getArgument()->setFoldedUnchecked();
        passVp->replaceAllUsesWith(passVp->getArgument());
        passVp->block()->discard(passVp);

        
        MPassArg *passThis = current->pop()->toPassArg();
        MDefinition *argThis = passThis->getArgument();
        passThis->replaceAllUsesWith(argThis);
        passThis->block()->discard(passThis);

        
        MPassArg *passFunc = current->pop()->toPassArg();
        MDefinition *argFunc = passFunc->getArgument();
        passFunc->replaceAllUsesWith(argFunc);
        passFunc->block()->discard(passFunc);

        
        current->pop();

        MArgumentsLength *numArgs = MArgumentsLength::New(alloc());
        current->add(numArgs);

        MApplyArgs *apply = MApplyArgs::New(alloc(), target, argFunc, numArgs, argThis);
        current->add(apply);
        current->push(apply);
        if (!resumeAfter(apply))
            return false;

        types::TemporaryTypeSet *types = bytecodeTypes(pc);
        return pushTypeBarrier(apply, types, true);
    }

    
    
    
    
    

    CallInfo callInfo(alloc(), false);

    
    MPassArg *passVp = current->pop()->toPassArg();
    passVp->getArgument()->setFoldedUnchecked();
    passVp->replaceAllUsesWith(passVp->getArgument());
    passVp->block()->discard(passVp);

    
    MDefinitionVector args(alloc());
    if (inliningDepth_) {
        if (!args.append(inlineCallInfo_->argv().begin(), inlineCallInfo_->argv().end()))
            return false;
    }
    callInfo.setArgs(&args);

    
    MPassArg *passThis = current->pop()->toPassArg();
    MDefinition *argThis = passThis->getArgument();
    passThis->replaceAllUsesWith(argThis);
    passThis->block()->discard(passThis);
    callInfo.setThis(argThis);

    
    MPassArg *passFunc = current->pop()->toPassArg();
    MDefinition *argFunc = passFunc->getArgument();
    passFunc->replaceAllUsesWith(argFunc);
    passFunc->block()->discard(passFunc);

    callInfo.setFun(argFunc);

    
    current->pop();

    
    if (makeInliningDecision(target, callInfo) && target->isInterpreted())
        return inlineScriptedCall(callInfo, target);

    callInfo.wrapArgs(alloc(), current);
    return makeCall(target, callInfo, false);
}

bool
IonBuilder::jsop_call(uint32_t argc, bool constructing)
{
    
    
    types::TemporaryTypeSet *observed = bytecodeTypes(pc);
    if (observed->empty()) {
        if (BytecodeFlowsToBitop(pc)) {
            if (!observed->addType(types::Type::Int32Type(), alloc_->lifoAlloc()))
                return false;
        } else if (*GetNextPc(pc) == JSOP_POS) {
            
            
            
            if (!observed->addType(types::Type::DoubleType(), alloc_->lifoAlloc()))
                return false;
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
        if (fun->hasScript() && fun->nonLazyScript()->shouldCloneAtCallsite) {
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
        MConstant *constant = MConstant::New(alloc(), ObjectValue(*target));
        current->add(constant);
        fun->setFoldedUnchecked();
        return constant;
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

    
    
    
    DOMInstanceClassMatchesProto instanceChecker =
        compartment->runtime()->DOMcallbacks()->instanceClassMatchesProto;

    const JSJitInfo *jinfo = func->jitInfo();
    if (jinfo->type != opType)
        return false;

    for (unsigned i = 0; i < inTypes->getObjectCount(); i++) {
        types::TypeObjectKey *curType = inTypes->getObject(i);
        if (!curType)
            continue;

        JSObject *proto = curType->proto().toObjectOrNull();
        if (!instanceChecker(proto, jinfo->protoID, jinfo->depth))
            return false;
    }

    return true;
}

static bool
TestAreKnownDOMTypes(types::TypeSet *inTypes)
{
    if (inTypes->unknownObject())
        return false;

    
    
    for (unsigned i = 0; i < inTypes->getObjectCount(); i++) {
        types::TypeObjectKey *curType = inTypes->getObject(i);
        if (!curType)
            continue;

        if (curType->unknownProperties())
            return false;
        if (!(curType->clasp()->flags & JSCLASS_IS_DOMJSCLASS))
            return false;
    }

    
    if (inTypes->getObjectCount() > 0)
        return true;

    return false;
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

    return calleeTypes->mightBeType(ValueTypeFromMIRType(def->type()));
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
    uint32_t expected_args = Min<uint32_t>(callInfo.argc(), target->nargs);
    for (size_t i = 0; i < expected_args; i++) {
        if (!ArgumentTypesMatch(callInfo.getArg(i), types::TypeScript::ArgTypes(targetScript, i)))
            return true;
    }
    for (size_t i = callInfo.argc(); i < target->nargs; i++) {
        if (!types::TypeScript::ArgTypes(targetScript, i)->mightBeType(JSVAL_TYPE_UNDEFINED))
            return true;
    }

    return false;
}

MCall *
IonBuilder::makeCallHelper(JSFunction *target, CallInfo &callInfo, bool cloneAtCallsite)
{
    JS_ASSERT(callInfo.isWrapped());

    
    

    uint32_t targetArgs = callInfo.argc();

    
    
    if (target && !target->isNative())
        targetArgs = Max<uint32_t>(target->nargs, callInfo.argc());

    MCall *call =
        MCall::New(alloc(), target, targetArgs + 1, callInfo.argc(), callInfo.constructing());
    if (!call)
        return nullptr;

    
    
    for (int i = targetArgs; i > (int)callInfo.argc(); i--) {
        JS_ASSERT_IF(target, !target->isNative());
        MConstant *undef = MConstant::New(alloc(), UndefinedValue());
        current->add(undef);
        MPassArg *pass = MPassArg::New(alloc(), undef);
        current->add(pass);
        call->addArg(i, pass);
    }

    
    
    for (int32_t i = callInfo.argc() - 1; i >= 0; i--) {
        JS_ASSERT(callInfo.getArg(i)->isPassArg());
        call->addArg(i + 1, callInfo.getArg(i)->toPassArg());
    }

    
    
    JS_ASSERT(callInfo.thisArg()->isPassArg());
    MPassArg *thisArg = callInfo.thisArg()->toPassArg();
    MPrepareCall *start = new MPrepareCall;
    thisArg->block()->insertBefore(thisArg, start);
    call->initPrepareCall(start);

    
    if (callInfo.constructing()) {
        MDefinition *create = createThis(target, callInfo.fun());
        if (!create) {
            abort("Failure inlining constructor for call.");
            return nullptr;
        }

        
        thisArg->replaceAllUsesWith(thisArg->getArgument());
        thisArg->block()->discard(thisArg);

        MPassArg *newThis = MPassArg::New(alloc(), create);
        current->add(newThis);

        thisArg = newThis;
        callInfo.setThis(newThis);
    }

    
    call->addArg(0, thisArg);

    
    
    if (cloneAtCallsite) {
        MDefinition *fun = makeCallsiteClone(target, callInfo.fun());
        callInfo.setFun(fun);
    }

    if (target && JSOp(*pc) == JSOP_CALL) {
        
        
        
        types::TemporaryTypeSet *thisTypes = thisArg->resultTypeSet();
        if (thisTypes &&
            TestAreKnownDOMTypes(thisTypes) &&
            testShouldDOMCall(thisTypes, target, JSJitInfo::Method))
        {
            call->setDOMFunction();
        }
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
    
    
    if (jitinfo->returnType == JSVAL_TYPE_UNKNOWN)
        return true;

    
    
    if (jitinfo->returnType == JSVAL_TYPE_OBJECT)
        return true;

    
    return jitinfo->returnType != types->getKnownTypeTag();
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
    if (!resumeAfter(call))
        return false;

    types::TemporaryTypeSet *types = bytecodeTypes(pc);

    if (call->isDOMFunction())
        return pushDOMTypeBarrier(call, types, call->getSingleTarget());

    return pushTypeBarrier(call, types, true);
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

    if (IsBuiltinEvalForScope(&script()->global(), ObjectValue(*singleton))) {
        if (argc != 1)
            return abort("Direct eval with more than one argument");

        if (!info().fun())
            return abort("Direct eval in global code");

        
        
        
        
        JSValueType type = thisTypes->getKnownTypeTag();
        if (type != JSVAL_TYPE_OBJECT && type != JSVAL_TYPE_NULL && type != JSVAL_TYPE_UNDEFINED)
            return abort("Direct eval from script with maybe-primitive 'this'");

        CallInfo callInfo(alloc(),  false);
        if (!callInfo.init(current, argc))
            return false;
        callInfo.unwrapArgs();

        callInfo.fun()->setFoldedUnchecked();

        MDefinition *scopeChain = current->scopeChain();
        MDefinition *string = callInfo.getArg(0);

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

                MInstruction *thisv = MPassArg::New(alloc(), thisValue);
                current->add(thisv);

                current->push(dynamicName);
                current->push(thisv);

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
        return resumeAfter(ins) && pushTypeBarrier(ins, types, true);
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
    JS_ASSERT(script()->compileAndGo);

    JSObject *templateObject = inspector->getTemplateObject(pc);
    if (!templateObject)
        return abort("No template object for NEWARRAY");

    JS_ASSERT(templateObject->is<ArrayObject>());
    if (templateObject->type()->unknownProperties()) {
        
        
        return abort("New array has unknown properties");
    }

    types::TemporaryTypeSet::DoubleConversion conversion =
        bytecodeTypes(pc)->convertDoubleElements(constraints());
    if (conversion == types::TemporaryTypeSet::AlwaysConvertToDoubles)
        templateObject->setShouldConvertDoubleElements();

    MNewArray *ins = MNewArray::New(alloc(), count, templateObject, MNewArray::NewArray_Allocating);

    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_newobject()
{
    
    JS_ASSERT(script()->compileAndGo);

    JSObject *templateObject = inspector->getTemplateObject(pc);
    if (!templateObject)
        return abort("No template object for NEWOBJECT");

    JS_ASSERT(templateObject->is<JSObject>());
    MNewObject *ins = MNewObject::New(alloc(), templateObject,
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
    if (value->isConstant() && value->toConstant()->value().isMagic(JS_ELEMENTS_HOLE)) {
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

    if (obj->toNewArray()->templateObject()->shouldConvertDoubleElements()) {
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

    
    
    switch (info().executionMode()) {
      case SequentialExecution:
      case DefinitePropertiesAnalysis:
        break;
      case ParallelExecution:
        needsBarrier = false;
        break;
    }

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
    MBasicBlock *block = MBasicBlock::New(graph(), &analysis(), info(),
                                          predecessor, pc, MBasicBlock::NORMAL);
    return addBlock(block, loopDepth_);
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc, MResumePoint *priorResumePoint)
{
    MBasicBlock *block = MBasicBlock::NewWithResumePoint(graph(), info(), predecessor, pc,
                                                         priorResumePoint);
    return addBlock(block, loopDepth_);
}

MBasicBlock *
IonBuilder::newBlockPopN(MBasicBlock *predecessor, jsbytecode *pc, uint32_t popped)
{
    MBasicBlock *block = MBasicBlock::NewPopN(graph(), info(), predecessor, pc, MBasicBlock::NORMAL, popped);
    return addBlock(block, loopDepth_);
}

MBasicBlock *
IonBuilder::newBlockAfter(MBasicBlock *at, MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::New(graph(), &analysis(), info(),
                                          predecessor, pc, MBasicBlock::NORMAL);
    if (!block)
        return nullptr;
    graph().insertBlockAfter(at, block);
    return block;
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc, uint32_t loopDepth)
{
    MBasicBlock *block = MBasicBlock::New(graph(), &analysis(), info(),
                                          predecessor, pc, MBasicBlock::NORMAL);
    return addBlock(block, loopDepth);
}

MBasicBlock *
IonBuilder::newOsrPreheader(MBasicBlock *predecessor, jsbytecode *loopEntry)
{
    JS_ASSERT((JSOp)*loopEntry == JSOP_LOOPENTRY);
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
        if (!script()->noScriptRval)
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

    if (info().fun()) {
        
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
    graph().setOsrStart(start);

    
    
    if (!resumeAt(start, loopEntry))
        return nullptr;

    
    
    
    osrBlock->linkOsrValues(start);

    
    
    
    JS_ASSERT(predecessor->stackDepth() == osrBlock->stackDepth());
    JS_ASSERT(info().scopeChainSlot() == 0);

    
    
    
    
    for (uint32_t i = info().startArgSlot(); i < osrBlock->stackDepth(); i++) {
        MDefinition *existing = current->getSlot(i);
        MDefinition *def = osrBlock->getSlot(i);
        JS_ASSERT_IF(!needsArgsObj || !info().isSlotAliased(i), def->type() == MIRType_Value);

        
        
        if (info().isSlotAliased(i))
            continue;

        def->setResultType(existing->type());
        def->setResultTypeSet(existing->resultTypeSet());
    }

    
    osrBlock->end(MGoto::New(alloc(), preheader));
    preheader->addPredecessor(alloc(), osrBlock);
    graph().setOsrBlock(osrBlock);

    
    
    if (info().fun())
        preheader->getSlot(info().thisSlot())->setGuard();

    return preheader;
}

MBasicBlock *
IonBuilder::newPendingLoopHeader(MBasicBlock *predecessor, jsbytecode *pc, bool osr)
{
    loopDepth_++;
    MBasicBlock *block = MBasicBlock::NewPendingLoopHeader(graph(), info(), predecessor, pc);
    if (!addBlock(block, loopDepth_))
        return nullptr;

    if (osr) {
        
        
        
        
        

        
        for (uint32_t i = info().startArgSlot(); i < block->stackDepth(); i++) {

            
            
            if (info().isSlotAliased(i))
                continue;

            
            
            if (i >= info().firstStackSlot())
                continue;

            MPhi *phi = block->getSlot(i)->toPhi();

            
            Value existingValue;
            uint32_t arg = i - info().firstArgSlot();
            uint32_t var = i - info().firstLocalSlot();
            if (info().fun() && i == info().thisSlot()) {
                existingValue = baselineFrame_->thisValue();
            } else if (arg < info().nargs()) {
                if (info().needsArgsObj())
                    existingValue = baselineFrame_->argsObj().arg(arg);
                else
                    existingValue = baselineFrame_->unaliasedFormal(arg);
            } else {
                existingValue = baselineFrame_->unaliasedVar(var);
            }

            
            MIRType type = existingValue.isDouble()
                         ? MIRType_Double
                         : MIRTypeFromValueType(existingValue.extractNonDoubleType());
            types::Type ntype = types::GetValueType(existingValue);
            types::TemporaryTypeSet *typeSet =
                alloc_->lifoAlloc()->new_<types::TemporaryTypeSet>(ntype);
            if (!typeSet)
                return nullptr;
            phi->addBackedgeType(type, typeSet);
        }
    }

    return block;
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
    resumePoint->setInstruction(ins);
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
    if (IsTypedArrayClass(clasp)) {
        
        
        JS_ASSERT(name);
        return true;
    }
    return clasp->isNative() && !clasp->ops.lookupGeneric;
}

static bool
ClassHasResolveHook(CompileCompartment *comp, const Class *clasp, PropertyName *name)
{
    if (clasp->resolve == JS_ResolveStub)
        return false;

    if (clasp->resolve == (JSResolveOp)str_resolve) {
        
        return false;
    }

    if (clasp->resolve == (JSResolveOp)fun_resolve)
        return FunctionHasResolveHook(comp->runtime()->names(), name);

    return true;
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

    if (!types && obj->type() != MIRType_String)
        return false;

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

      case MIRType_Int32:
      case MIRType_Double:
        key = JSProto_Number;
        break;

      case MIRType_Boolean:
        key = JSProto_Boolean;
        break;

      case MIRType_Object:
      case MIRType_Value: {
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

            if (object->unknownProperties())
                return false;
            types::HeapTypeSetKey property = object->property(NameToId(name));
            if (property.isOwnProperty(constraints()))
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

    JSObject *proto = GetClassPrototypePure(&script()->global(), key);
    if (proto)
        return testSingletonProperty(proto, name) == singleton;

    return false;
}












bool
IonBuilder::pushTypeBarrier(MDefinition *def, types::TemporaryTypeSet *observed, bool needsBarrier)
{
    
    if (BytecodeIsPopped(pc))
        return true;

    
    
    
    
    

    if (!needsBarrier) {
        JSValueType type = observed->getKnownTypeTag();
        MDefinition *replace = ensureDefiniteType(def, type);
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

    MInstruction *barrier = MTypeBarrier::New(alloc(), def, observed);
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
    if (jitinfo->returnType != JSVAL_TYPE_DOUBLE ||
        observed->getKnownTypeTag() != JSVAL_TYPE_INT32) {
        JS_ASSERT(jitinfo->returnType == JSVAL_TYPE_UNKNOWN ||
                  observed->getKnownTypeTag() == JSVAL_TYPE_UNKNOWN ||
                  jitinfo->returnType == observed->getKnownTypeTag());
        replace = ensureDefiniteType(ins, jitinfo->returnType);
        if (replace != ins) {
            current->pop();
            current->push(replace);
        }
    } else {
        JS_ASSERT(barrier);
    }

    return pushTypeBarrier(replace, observed, barrier);
}

MDefinition *
IonBuilder::ensureDefiniteType(MDefinition *def, JSValueType definiteType)
{
    MInstruction *replace;
    switch (definiteType) {
      case JSVAL_TYPE_UNDEFINED:
        def->setFoldedUnchecked();
        replace = MConstant::New(alloc(), UndefinedValue());
        break;

      case JSVAL_TYPE_NULL:
        def->setFoldedUnchecked();
        replace = MConstant::New(alloc(), NullValue());
        break;

      case JSVAL_TYPE_UNKNOWN:
        return def;

      default: {
        MIRType replaceType = MIRTypeFromValueType(definiteType);
        if (def->type() != MIRType_Value) {
            JS_ASSERT(def->type() == replaceType);
            return def;
        }
        replace = MUnbox::New(alloc(), def, replaceType, MUnbox::Infallible);
        break;
      }
    }

    current->add(replace);
    return replace;
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
        property.configured(constraints(), staticType))
    {
        
        
        *psucceeded = false;
        return true;
    }

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    bool barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(), staticType,
                                                name, types,  true);

    JSObject *singleton = types->getSingleton();

    JSValueType knownType = types->getKnownTypeTag();
    if (!barrier) {
        if (singleton) {
            
            if (testSingletonProperty(staticObject, name) == singleton)
                return pushConstant(ObjectValue(*singleton));
        }
        if (knownType == JSVAL_TYPE_UNDEFINED)
            return pushConstant(UndefinedValue());
        if (knownType == JSVAL_TYPE_NULL)
            return pushConstant(NullValue());
    }

    MInstruction *obj = MConstant::New(alloc(), ObjectValue(*staticObject));
    current->add(obj);

    MIRType rvalType = MIRTypeFromValueType(types->getKnownTypeTag());
    if (barrier)
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
      case MIRType_Magic:
        return types->hasType(types::Type::PrimitiveType(ValueTypeFromMIRType(input)));

      case MIRType_Object:
        return types->unknownObject() || (inputTypes && inputTypes->isSubset(types));

      case MIRType_Value:
        return types->unknown() || (inputTypes && inputTypes->isSubset(types));

      default:
        MOZ_ASSUME_UNREACHABLE("Bad input type");
    }
}


bool
jit::NeedsPostBarrier(CompileInfo &info, MDefinition *value)
{
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
        property.configured(constraints(), staticType))
    {
        
        
        return jsop_setprop(name);
    }

    if (!TypeSetIncludes(property.maybeTypes(), value->type(), value->resultTypeSet()))
        return jsop_setprop(name);

    current->pop();

    
    MDefinition *obj = current->pop();
    JS_ASSERT(&obj->toConstant()->value().toObject() == staticObject);

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), obj, value));

    
    
    MIRType slotType = MIRType_None;
    JSValueType knownType = property.knownTypeTag(constraints());
    if (knownType != JSVAL_TYPE_UNKNOWN)
        slotType = MIRTypeFromValueType(knownType);

    bool needsBarrier = property.needsBarrier(constraints());
    return storeSlot(obj, property.maybeTypes()->definiteSlot(), NumFixedSlots(staticObject),
                     value, needsBarrier, slotType);
}

bool
IonBuilder::jsop_getname(PropertyName *name)
{
    MDefinition *object;
    if (js_CodeSpec[*pc].format & JOF_GNAME) {
        MInstruction *global = MConstant::New(alloc(), ObjectValue(script()->global()));
        current->add(global);
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
    return pushTypeBarrier(ins, types, true);
}

bool
IonBuilder::jsop_intrinsic(PropertyName *name)
{
    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    JSValueType type = types->getKnownTypeTag();

    
    
    if (type == JSVAL_TYPE_UNKNOWN) {
        MCallGetIntrinsicValue *ins = MCallGetIntrinsicValue::New(alloc(), name);

        current->add(ins);
        current->push(ins);

        if (!resumeAfter(ins))
            return false;

        return pushTypeBarrier(ins, types, true);
    }

    
    Value vp;
    JS_ALWAYS_TRUE(script()->global().maybeGetIntrinsicValue(name, &vp));
    JS_ASSERT(types->hasType(types::GetValueType(vp)));

    MConstant *ins = MConstant::New(alloc(), vp);
    current->add(ins);
    current->push(ins);

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

static JSValueType
GetElemKnownType(bool needsHoleCheck, types::TemporaryTypeSet *types)
{
    JSValueType knownType = types->getKnownTypeTag();

    
    
    
    
    
    if (knownType == JSVAL_TYPE_UNDEFINED || knownType == JSVAL_TYPE_NULL)
        knownType = JSVAL_TYPE_UNKNOWN;

    
    
    if (needsHoleCheck && !LIRGenerator::allowTypedElementHoleCheck())
        knownType = JSVAL_TYPE_UNKNOWN;

    return knownType;
}

bool
IonBuilder::jsop_getelem()
{
    MDefinition *index = current->pop();
    MDefinition *obj = current->pop();

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

    if (script()->argumentsHasVarBinding() && obj->mightBeType(MIRType_Magic))
        return abort("Type is not definitely lazy arguments.");

    if (!getElemTryCache(&emitted, obj, index) || emitted)
        return emitted;

    
    MInstruction *ins = MCallGetElement::New(alloc(), obj, index);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    return pushTypeBarrier(ins, types, true);
}

bool
IonBuilder::getElemTryTypedObject(bool *emitted, MDefinition *obj, MDefinition *index)
{
    JS_ASSERT(*emitted == false);

    TypeRepresentationSet objTypeReprs;
    if (!lookupTypeRepresentationSet(obj, &objTypeReprs))
        return false;

    if (!objTypeReprs.allOfArrayKind())
        return true;

    TypeRepresentationSet elemTypeReprs;
    if (!objTypeReprs.arrayElementType(*this, &elemTypeReprs))
        return false;

    size_t elemSize;
    if (!elemTypeReprs.allHaveSameSize(&elemSize))
        return true;

    switch (elemTypeReprs.kind()) {
      case TypeRepresentation::X4:
        
        return true;

      case TypeRepresentation::Struct:
      case TypeRepresentation::Array:
        return getElemTryComplexElemOfTypedObject(emitted,
                                                  obj,
                                                  index,
                                                  objTypeReprs,
                                                  elemTypeReprs,
                                                  elemSize);
      case TypeRepresentation::Scalar:
        return getElemTryScalarElemOfTypedObject(emitted,
                                                 obj,
                                                 index,
                                                 objTypeReprs,
                                                 elemTypeReprs,
                                                 elemSize);

      case TypeRepresentation::Reference:
        return true;
    }

    MOZ_ASSUME_UNREACHABLE("Bad kind");
}

static MIRType
MIRTypeForTypedArrayRead(ScalarTypeRepresentation::Type arrayType,
                         bool observedDouble);

bool
IonBuilder::getElemTryScalarElemOfTypedObject(bool *emitted,
                                              MDefinition *obj,
                                              MDefinition *index,
                                              TypeRepresentationSet objTypeReprs,
                                              TypeRepresentationSet elemTypeReprs,
                                              size_t elemSize)
{
    JS_ASSERT(objTypeReprs.allOfArrayKind());

    
    if (!elemTypeReprs.singleton())
        return true;
    ScalarTypeRepresentation *elemTypeRepr = elemTypeReprs.getTypeRepresentation()->asScalar();

    
    size_t lenOfAll = objTypeReprs.arrayLength();
    if (lenOfAll >= size_t(INT_MAX)) 
        return true;
    MInstruction *length = MConstant::New(alloc(), Int32Value(int32_t(lenOfAll)));

    *emitted = true;
    current->add(length);

    
    MInstruction *idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    
    index = addBoundsCheck(index, length);

    
    
    const int32_t alignment = elemTypeRepr->alignment();
    MMul *indexAsByteOffset = MMul::New(alloc(), index, constantInt(alignment),
                                        MIRType_Int32, MMul::Integer);
    current->add(indexAsByteOffset);

    
    MDefinition *elements, *scaledOffset;
    loadTypedObjectElements(obj, indexAsByteOffset, alignment, &elements, &scaledOffset);

    
    MLoadTypedArrayElement *load = MLoadTypedArrayElement::New(alloc(), elements, scaledOffset, elemTypeRepr->type());
    current->add(load);
    current->push(load);

    
    
    
    
    types::TemporaryTypeSet *resultTypes = bytecodeTypes(pc);
    bool allowDouble = resultTypes->hasType(types::Type::DoubleType());
    MIRType knownType = MIRTypeForTypedArrayRead(elemTypeRepr->type(), allowDouble);
    
    
    load->setResultType(knownType);
    load->setResultTypeSet(resultTypes);

    return true;
}

bool
IonBuilder::getElemTryComplexElemOfTypedObject(bool *emitted,
                                               MDefinition *obj,
                                               MDefinition *index,
                                               TypeRepresentationSet objTypeReprs,
                                               TypeRepresentationSet elemTypeReprs,
                                               size_t elemSize)
{
    JS_ASSERT(objTypeReprs.allOfArrayKind());

    MDefinition *type = loadTypedObjectType(obj);
    MDefinition *elemType = typeObjectForElementFromArrayStructType(type);

    
    size_t lenOfAll = objTypeReprs.arrayLength();
    if (lenOfAll >= size_t(INT_MAX)) 
        return true;
    MInstruction *length = MConstant::New(alloc(), Int32Value(int32_t(lenOfAll)));

    *emitted = true;
    current->add(length);

    
    MInstruction *idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    
    index = addBoundsCheck(index, length);

    
    MConstant *alignment = MConstant::New(alloc(), Int32Value(elemSize));
    current->add(alignment);

    
    
    MMul *indexAsByteOffset = MMul::New(alloc(), index, alignment, MIRType_Int32,
                                        MMul::Integer);
    current->add(indexAsByteOffset);

    
    MDefinition *owner, *ownerOffset;
    loadTypedObjectData(obj, indexAsByteOffset, &owner, &ownerOffset);

    
    MInstruction *derived = new MNewDerivedTypedObject(elemTypeReprs,
                                                       elemType,
                                                       owner,
                                                       ownerOffset);

    types::TemporaryTypeSet *resultTypes = bytecodeTypes(pc);
    derived->setResultTypeSet(resultTypes);
    current->add(derived);
    current->push(derived);

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

    ScalarTypeRepresentation::Type arrayType;
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
    ArrayBufferView::ViewType viewType = (ArrayBufferView::ViewType) tarr->type();

    
    if (viewType == ArrayBufferView::TYPE_UINT32)
        return true;

    MDefinition *ptr = convertShiftToMaskForStaticTypedArray(index, viewType);
    if (!ptr)
        return true;

    

    obj->setFoldedUnchecked();
    index->setFoldedUnchecked();

    MLoadTypedArrayElementStatic *load = MLoadTypedArrayElementStatic::New(alloc(), tarr, ptr);
    current->add(load);
    current->push(load);

    
    
    
    
    
    if (viewType == ArrayBufferView::TYPE_FLOAT32 || viewType == ArrayBufferView::TYPE_FLOAT64) {
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

    ScalarTypeRepresentation::Type arrayType;
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

    if (obj->type() != MIRType_Magic)
        return true;

    

    JS_ASSERT(!info().argsObjAliasesFormals());

    
    obj->setFoldedUnchecked();

    
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
    if (!pushTypeBarrier(load, types, true))
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

    if (obj->type() != MIRType_Magic)
        return true;

    
    obj->setFoldedUnchecked();

    JS_ASSERT(!info().argsObjAliasesFormals());

    
    if (index->isConstant() && index->toConstant()->value().isInt32()) {
        JS_ASSERT(inliningDepth_ > 0);

        int32_t id = index->toConstant()->value().toInt32();
        index->setFoldedUnchecked();

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

    
    if (!index->mightBeType(MIRType_Int32) && !index->mightBeType(MIRType_String))
        return true;

    
    
    bool nonNativeGetElement = inspector->hasSeenNonNativeGetElement(pc);
    if (index->mightBeType(MIRType_Int32) && nonNativeGetElement)
        return true;

    

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    bool barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(), obj, nullptr, types);

    
    
    if (index->mightBeType(MIRType_String))
        barrier = true;

    
    if (needsToMonitorMissingProperties(types))
        barrier = true;

    MInstruction *ins = MGetElementCache::New(alloc(), obj, index, barrier);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    
    if (index->type() == MIRType_Int32 && !barrier) {
        bool needHoleCheck = !ElementAccessIsPacked(constraints(), obj);
        JSValueType knownType = GetElemKnownType(needHoleCheck, types);

        if (knownType != JSVAL_TYPE_UNKNOWN && knownType != JSVAL_TYPE_DOUBLE)
            ins->setResultType(MIRTypeFromValueType(knownType));
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

    if (JSOp(*pc) == JSOP_CALLELEM && !index->mightBeType(MIRType_String)) {
        
        
        
        if (!AddObjectsForPropertyRead(obj, nullptr, types))
            return false;
    }

    bool barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(), obj, nullptr, types);
    bool needsHoleCheck = !ElementAccessIsPacked(constraints(), obj);

    
    
    
    bool readOutOfBounds =
        types->hasType(types::Type::UndefinedType()) &&
        !ElementAccessHasExtraIndexedProperty(constraints(), obj);

    JSValueType knownType = JSVAL_TYPE_UNKNOWN;
    if (!barrier)
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
        !barrier &&
        loopDepth_ &&
        !readOutOfBounds &&
        !needsHoleCheck &&
        knownType == JSVAL_TYPE_DOUBLE &&
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

        
        
        
        JS_ASSERT(knownType == JSVAL_TYPE_UNKNOWN);
    }

    if (knownType != JSVAL_TYPE_UNKNOWN)
        load->setResultType(MIRTypeFromValueType(knownType));

    current->push(load);
    return pushTypeBarrier(load, types, barrier);
}

MInstruction *
IonBuilder::getTypedArrayLength(MDefinition *obj)
{
    if (obj->isConstant() && obj->toConstant()->value().isObject()) {
        TypedArrayObject *tarr = &obj->toConstant()->value().toObject().as<TypedArrayObject>();
        int32_t length = (int32_t) tarr->length();
        obj->setFoldedUnchecked();
        return MConstant::New(alloc(), Int32Value(length));
    }
    return MTypedArrayLength::New(alloc(), obj);
}

MInstruction *
IonBuilder::getTypedArrayElements(MDefinition *obj)
{
    if (obj->isConstant() && obj->toConstant()->value().isObject()) {
        TypedArrayObject *tarr = &obj->toConstant()->value().toObject().as<TypedArrayObject>();
        void *data = tarr->viewData();

        
        
        types::TypeObjectKey *tarrType = types::TypeObjectKey::get(tarr);
        tarrType->watchStateChangeForTypedArrayBuffer(constraints());

        obj->setFoldedUnchecked();
        return MConstantElements::New(alloc(), data);
    }
    return MTypedArrayElements::New(alloc(), obj);
}

MDefinition *
IonBuilder::convertShiftToMaskForStaticTypedArray(MDefinition *id,
                                                  ArrayBufferView::ViewType viewType)
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
MIRTypeForTypedArrayRead(ScalarTypeRepresentation::Type arrayType,
                         bool observedDouble)
{
    switch (arrayType) {
      case ScalarTypeRepresentation::TYPE_INT8:
      case ScalarTypeRepresentation::TYPE_UINT8:
      case ScalarTypeRepresentation::TYPE_UINT8_CLAMPED:
      case ScalarTypeRepresentation::TYPE_INT16:
      case ScalarTypeRepresentation::TYPE_UINT16:
      case ScalarTypeRepresentation::TYPE_INT32:
        return MIRType_Int32;
      case ScalarTypeRepresentation::TYPE_UINT32:
        return observedDouble ? MIRType_Double : MIRType_Int32;
      case ScalarTypeRepresentation::TYPE_FLOAT32:
        return (LIRGenerator::allowFloat32Optimizations()) ? MIRType_Float32 : MIRType_Double;
      case ScalarTypeRepresentation::TYPE_FLOAT64:
        return MIRType_Double;
    }
    MOZ_ASSUME_UNREACHABLE("Unknown typed array type");
}

bool
IonBuilder::jsop_getelem_typed(MDefinition *obj, MDefinition *index,
                               ScalarTypeRepresentation::Type arrayType)
{
    types::TemporaryTypeSet *types = bytecodeTypes(pc);

    bool maybeUndefined = types->hasType(types::Type::UndefinedType());

    
    
    
    bool allowDouble = types->hasType(types::Type::DoubleType());

    
    MInstruction *idInt32 = MToInt32::New(alloc(), index);
    current->add(idInt32);
    index = idInt32;

    if (!maybeUndefined) {
        
        

        
        
        
        
        MIRType knownType = MIRTypeForTypedArrayRead(arrayType, allowDouble);

        
        MInstruction *length = getTypedArrayLength(obj);
        current->add(length);

        
        index = addBoundsCheck(index, length);

        
        MInstruction *elements = getTypedArrayElements(obj);
        current->add(elements);

        
        MLoadTypedArrayElement *load = MLoadTypedArrayElement::New(alloc(), elements, index, arrayType);
        current->add(load);
        current->push(load);

        
        
        load->setResultType(knownType);
        return true;
    } else {
        
        
        
        
        bool needsBarrier = true;
        switch (arrayType) {
          case ScalarTypeRepresentation::TYPE_INT8:
          case ScalarTypeRepresentation::TYPE_UINT8:
          case ScalarTypeRepresentation::TYPE_UINT8_CLAMPED:
          case ScalarTypeRepresentation::TYPE_INT16:
          case ScalarTypeRepresentation::TYPE_UINT16:
          case ScalarTypeRepresentation::TYPE_INT32:
          case ScalarTypeRepresentation::TYPE_UINT32:
            if (types->hasType(types::Type::Int32Type()))
                needsBarrier = false;
            break;
          case ScalarTypeRepresentation::TYPE_FLOAT32:
          case ScalarTypeRepresentation::TYPE_FLOAT64:
            if (allowDouble)
                needsBarrier = false;
            break;
          default:
            MOZ_ASSUME_UNREACHABLE("Unknown typed array type");
        }

        
        
        
        MLoadTypedArrayElementHole *load =
            MLoadTypedArrayElementHole::New(alloc(), obj, index, arrayType, allowDouble);
        current->add(load);
        current->push(load);

        return pushTypeBarrier(load, types, needsBarrier);
    }
}

bool
IonBuilder::jsop_setelem()
{
    bool emitted = false;

    MDefinition *value = current->pop();
    MDefinition *index = current->pop();
    MDefinition *object = current->pop();

    if (!setElemTryTypedStatic(&emitted, object, index, value) || emitted)
        return emitted;

    if (!setElemTryTyped(&emitted, object, index, value) || emitted)
        return emitted;

    if (!setElemTryDense(&emitted, object, index, value) || emitted)
        return emitted;

    if (!setElemTryArguments(&emitted, object, index, value) || emitted)
        return emitted;

    if (script()->argumentsHasVarBinding() && object->mightBeType(MIRType_Magic))
        return abort("Type is not definitely lazy arguments.");

    if (!setElemTryCache(&emitted, object, index, value) || emitted)
        return emitted;

    
    MInstruction *ins = MCallSetElement::New(alloc(), object, index, value);
    current->add(ins);
    current->push(value);

    return resumeAfter(ins);
}

bool
IonBuilder::setElemTryTypedStatic(bool *emitted, MDefinition *object,
                                  MDefinition *index, MDefinition *value)
{
    JS_ASSERT(*emitted == false);

    ScalarTypeRepresentation::Type arrayType;
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
    ArrayBufferView::ViewType viewType = (ArrayBufferView::ViewType) tarr->type();

    MDefinition *ptr = convertShiftToMaskForStaticTypedArray(index, viewType);
    if (!ptr)
        return true;

    
    object->setFoldedUnchecked();
    index->setFoldedUnchecked();

    
    MDefinition *toWrite = value;
    if (viewType == ArrayBufferView::TYPE_UINT8_CLAMPED) {
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
IonBuilder::setElemTryTyped(bool *emitted, MDefinition *object,
                            MDefinition *index, MDefinition *value)
{
    JS_ASSERT(*emitted == false);

    ScalarTypeRepresentation::Type arrayType;
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

    if (object->type() != MIRType_Magic)
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

    if (!index->mightBeType(MIRType_Int32) && !index->mightBeType(MIRType_String))
        return true;

    
    
    
    SetElemICInspector icInspect(inspector->setElemICInspector(pc));
    if (!icInspect.sawDenseWrite() && !icInspect.sawTypedArrayWrite())
        return true;

    if (PropertyWriteNeedsTypeBarrier(alloc(), constraints(), current,
                                      &object, nullptr, &value,  true))
    {
        return true;
    }

    
    
    
    
    bool guardHoles = ElementAccessHasExtraIndexedProperty(constraints(), object);

    if (NeedsPostBarrier(info(), value))
        current->add(MPostWriteBarrier::New(alloc(), object, value));

    
    MInstruction *ins = MSetElementCache::New(alloc(), object, index, value, script()->strict, guardHoles);
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
        MOZ_ASSUME_UNREACHABLE("Unknown double conversion");
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
IonBuilder::jsop_setelem_typed(ScalarTypeRepresentation::Type arrayType,
                               SetElemSafety safety,
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

    
    MInstruction *length = getTypedArrayLength(obj);
    current->add(length);

    if (!expectOOB && safety == SetElem_Normal) {
        
        id = addBoundsCheck(id, length);
    }

    
    MInstruction *elements = getTypedArrayElements(obj);
    current->add(elements);

    
    MDefinition *toWrite = value;
    if (arrayType == ScalarTypeRepresentation::TYPE_UINT8_CLAMPED) {
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

    if (types->getKnownTypeTag() != JSVAL_TYPE_INT32)
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

            
            MArrayLength *length = new MArrayLength(elements);
            current->add(length);
            current->push(length);
            return true;
        }

        if (objTypes && objTypes->getTypedArrayType() != ScalarTypeRepresentation::TYPE_MAX) {
            current->pop();
            MInstruction *length = getTypedArrayLength(obj);
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
IonBuilder::jsop_arguments_length()
{
    
    MDefinition *args = current->pop();
    args->setFoldedUnchecked();

    
    if (inliningDepth_ == 0) {
        MInstruction *ins = MArgumentsLength::New(alloc());
        current->add(ins);
        current->push(ins);
        return true;
    }

    
    return pushConstant(Int32Value(inlineCallInfo_->argv().length()));
}

bool
IonBuilder::jsop_rest()
{
    JSObject *templateObject = inspector->getTemplateObject(pc);
    JS_ASSERT(templateObject->is<ArrayObject>());

    if (inliningDepth_ == 0) {
        
        MArgumentsLength *numActuals = MArgumentsLength::New(alloc());
        current->add(numActuals);

        
        
        MRest *rest = MRest::New(alloc(), numActuals, info().nargs() - 1, templateObject);
        current->add(rest);
        current->push(rest);
        return true;
    }

    
    unsigned numActuals = inlineCallInfo_->argv().length();
    unsigned numFormals = info().nargs() - 1;
    unsigned numRest = numActuals > numFormals ? numActuals - numFormals : 0;

    MNewArray *array = MNewArray::New(alloc(), numRest, templateObject, MNewArray::NewArray_Allocating);
    current->add(array);

    if (numActuals <= numFormals) {
        current->push(array);
        return true;
    }

    MElements *elements = MElements::New(alloc(), array);
    current->add(elements);

    
    
    MConstant *index;
    for (unsigned i = numFormals; i < numActuals; i++) {
        index = MConstant::New(alloc(), Int32Value(i - numFormals));
        current->add(index);

        MDefinition *arg = inlineCallInfo_->argv()[i];
        MStoreElement *store = MStoreElement::New(alloc(), elements, index, arg,
                                                   false);
        current->add(store);
    }

    MSetInitializedLength *initLength = MSetInitializedLength::New(alloc(), elements, index);
    current->add(initLength);
    current->push(array);

    return true;
}

bool
IonBuilder::getDefiniteSlot(types::TemporaryTypeSet *types, PropertyName *name,
                            types::HeapTypeSetKey *property)
{
    if (!types || types->unknownObject() || types->getObjectCount() != 1)
        return false;

    types::TypeObjectKey *type = types->getObject(0);
    if (type->unknownProperties() || type->singleton())
        return false;

    jsid id = NameToId(name);

    *property = type->property(id);
    return property->maybeTypes() &&
           property->maybeTypes()->definiteProperty() &&
           !property->configured(constraints(), type);
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

    MNot *ins = new MNot(value);
    current->add(ins);
    current->push(ins);
    ins->infer();
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
                if (!types->empty() || types->configuredProperty())
                    return false;
            }
            if (JSObject *obj = type->singleton()) {
                if (types::CanHaveEmptyPropertyTypesForOwnProperty(obj))
                    return false;
            }

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

            
            
            
            if (type->proto() == foundProto)
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

    
    
    
    
    MInstruction *wrapper = MConstant::New(alloc(), ObjectValue(*foundProto));
    current->add(wrapper);

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
        if (typeObj->unknownProperties() || !typeObj->proto().isObject())
            continue;

        types::HeapTypeSetKey ownTypes = typeObj->property(NameToId(name));
        if (ownTypes.isOwnProperty(constraints()))
            continue;

        JSObject *singleton = testSingletonProperty(typeObj->proto().toObject(), name);
        if (!singleton)
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
        IonSpew(IonSpew_Inlining, "Annotated GetPropertyCache with %d/%d inline cases",
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
        if (builder->script()->invalidatedIdempotentCache)
            return true;
        builder = builder->callerBuilder_;
    } while (builder);

    return false;
}

bool
IonBuilder::loadSlot(MDefinition *obj, size_t slot, size_t nfixed, MIRType rvalType,
                     bool barrier, types::TemporaryTypeSet *types)
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
                     bool barrier, types::TemporaryTypeSet *types)
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

    
    if (!getPropTryArgumentsLength(&emitted) || emitted)
        return emitted;

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    bool barrier = PropertyReadNeedsTypeBarrier(analysisContext, constraints(),
                                                current->peek(-1), name, types);

    
    
    
    
    if (info().executionMode() == DefinitePropertiesAnalysis || types->empty()) {
        MDefinition *obj = current->peek(-1);
        MCallGetProperty *call = MCallGetProperty::New(alloc(), obj, name, *pc == JSOP_CALLPROP);
        current->add(call);

        
        
        
        
        if (info().executionMode() == DefinitePropertiesAnalysis) {
            if (!getPropTryConstant(&emitted, name, types) || emitted)
                return emitted;
        }

        current->pop();
        current->push(call);
        return resumeAfter(call) && pushTypeBarrier(call, types, true);
    }

    
    if (!getPropTryConstant(&emitted, name, types) || emitted)
        return emitted;

    
    if (!getPropTryTypedObject(&emitted, name, types) || emitted)
        return emitted;

    
    if (!getPropTryDefiniteSlot(&emitted, name, barrier, types) || emitted)
        return emitted;

    
    if (!getPropTryCommonGetter(&emitted, name, types) || emitted)
        return emitted;

    
    if (!getPropTryInlineAccess(&emitted, name, barrier, types) || emitted)
        return emitted;

    
    if (!getPropTryCache(&emitted, name, barrier, types) || emitted)
        return emitted;

    
    MDefinition *obj = current->pop();
    MCallGetProperty *call = MCallGetProperty::New(alloc(), obj, name, *pc == JSOP_CALLPROP);
    current->add(call);
    current->push(call);
    if (!resumeAfter(call))
        return false;

    return pushTypeBarrier(call, types, true);
}

bool
IonBuilder::getPropTryArgumentsLength(bool *emitted)
{
    JS_ASSERT(*emitted == false);
    if (current->peek(-1)->type() != MIRType_Magic) {
        if (script()->argumentsHasVarBinding() && current->peek(-1)->mightBeType(MIRType_Magic))
            return abort("Type is not definitely lazy arguments.");
        return true;
    }
    if (JSOp(*pc) != JSOP_LENGTH)
        return true;

    *emitted = true;
    return jsop_arguments_length();
}

bool
IonBuilder::getPropTryConstant(bool *emitted, PropertyName *name,
                               types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);
    JSObject *singleton = types ? types->getSingleton() : nullptr;
    if (!singleton)
        return true;

    bool testObject, testString;
    if (!testSingletonPropertyTypes(current->peek(-1), singleton, name, &testObject, &testString))
        return true;

    MDefinition *obj = current->pop();

    
    JS_ASSERT(!testString || !testObject);
    if (testObject)
        current->add(MGuardObject::New(alloc(), obj));
    else if (testString)
        current->add(MGuardString::New(alloc(), obj));
    else
        obj->setFoldedUnchecked();

    MConstant *known = MConstant::New(alloc(), ObjectValue(*singleton));

    current->add(known);
    current->push(known);

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryTypedObject(bool *emitted, PropertyName *name,
                                  types::TemporaryTypeSet *resultTypes)
{
    TypeRepresentationSet fieldTypeReprs;
    int32_t fieldOffset;
    size_t fieldIndex;
    if (!lookupTypedObjectField(current->peek(-1), name, &fieldOffset,
                                &fieldTypeReprs, &fieldIndex))
        return false;
    if (fieldTypeReprs.empty())
        return true;

    switch (fieldTypeReprs.kind()) {
      case TypeRepresentation::Reference:
        return true;

      case TypeRepresentation::X4:
        
        return true;

      case TypeRepresentation::Struct:
      case TypeRepresentation::Array:
        return getPropTryComplexPropOfTypedObject(emitted,
                                                  fieldOffset,
                                                  fieldTypeReprs,
                                                  fieldIndex,
                                                  resultTypes);

      case TypeRepresentation::Scalar:
        return getPropTryScalarPropOfTypedObject(emitted,
                                                 fieldOffset,
                                                 fieldTypeReprs,
                                                 resultTypes);
    }

    MOZ_ASSUME_UNREACHABLE("Bad kind");
}

bool
IonBuilder::getPropTryScalarPropOfTypedObject(bool *emitted,
                                              int32_t fieldOffset,
                                              TypeRepresentationSet fieldTypeReprs,
                                              types::TemporaryTypeSet *resultTypes)
{
    
    if (!fieldTypeReprs.singleton())
        return true;
    ScalarTypeRepresentation *fieldTypeRepr = fieldTypeReprs.getTypeRepresentation()->asScalar();

    

    MDefinition *typedObj = current->pop();

    
    MDefinition *elements, *scaledOffset;
    loadTypedObjectElements(typedObj, constantInt(fieldOffset),
                            fieldTypeRepr->alignment(),
                            &elements, &scaledOffset);

    
    
    
    bool allowDouble = resultTypes->hasType(types::Type::DoubleType());
    MIRType knownType = MIRTypeForTypedArrayRead(fieldTypeRepr->type(), allowDouble);

    MLoadTypedArrayElement *load =
        MLoadTypedArrayElement::New(alloc(), elements, scaledOffset,
                                    fieldTypeRepr->type());
    load->setResultType(knownType);
    load->setResultTypeSet(resultTypes);
    current->add(load);
    current->push(load);
    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryComplexPropOfTypedObject(bool *emitted,
                                               int32_t fieldOffset,
                                               TypeRepresentationSet fieldTypeReprs,
                                               size_t fieldIndex,
                                               types::TemporaryTypeSet *resultTypes)
{
    
    
    if (fieldIndex == SIZE_MAX)
        return true;

    

    MDefinition *typedObj = current->pop();

    
    MDefinition *type = loadTypedObjectType(typedObj);
    MDefinition *fieldType = typeObjectForFieldFromStructType(type, fieldIndex);

    
    MDefinition *owner, *ownerOffset;
    loadTypedObjectData(typedObj, constantInt(fieldOffset),
                        &owner, &ownerOffset);

    
    MInstruction *derived = new MNewDerivedTypedObject(fieldTypeReprs,
                                                       fieldType,
                                                       owner,
                                                       ownerOffset);
    derived->setResultTypeSet(resultTypes);
    current->add(derived);
    current->push(derived);
    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryDefiniteSlot(bool *emitted, PropertyName *name,
                                   bool barrier, types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);
    types::HeapTypeSetKey property;
    if (!getDefiniteSlot(current->peek(-1)->resultTypeSet(), name, &property))
        return true;

    MDefinition *obj = current->pop();
    MDefinition *useObj = obj;
    if (obj->type() != MIRType_Object) {
        MGuardObject *guard = MGuardObject::New(alloc(), obj);
        current->add(guard);
        useObj = guard;
    }

    MLoadFixedSlot *fixed = MLoadFixedSlot::New(alloc(), useObj, property.maybeTypes()->definiteSlot());
    if (!barrier)
        fixed->setResultType(MIRTypeFromValueType(types->getKnownTypeTag()));

    current->add(fixed);
    current->push(fixed);

    if (!pushTypeBarrier(fixed, types, barrier))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryCommonGetter(bool *emitted, PropertyName *name,
                                   types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);

    Shape *lastProperty = nullptr;
    JSFunction *commonGetter = nullptr;
    JSObject *foundProto = inspector->commonGetPropFunction(pc, &lastProperty, &commonGetter);
    if (!foundProto)
        return true;

    types::TemporaryTypeSet *objTypes = current->peek(-1)->resultTypeSet();
    MDefinition *guard = testCommonGetterSetter(objTypes, name,  true,
                                                foundProto, lastProperty);
    if (!guard)
        return true;

    bool isDOM = objTypes->isDOMClass();

    MDefinition *obj = current->pop();

    if (isDOM && testShouldDOMCall(objTypes, commonGetter, JSJitInfo::Getter)) {
        const JSJitInfo *jitinfo = commonGetter->jitInfo();
        MGetDOMProperty *get = MGetDOMProperty::New(alloc(), jitinfo, obj, guard);
        current->add(get);
        current->push(get);

        if (get->isEffectful() && !resumeAfter(get))
            return false;

        if (!pushDOMTypeBarrier(get, types, commonGetter))
            return false;

        *emitted = true;
        return true;
    }

    
    if (objTypes->getKnownTypeTag() != JSVAL_TYPE_OBJECT) {
        MGuardObject *guardObj = MGuardObject::New(alloc(), obj);
        current->add(guardObj);
        obj = guardObj;
    }

    
    pushConstant(ObjectValue(*commonGetter));

    MPassArg *wrapper = MPassArg::New(alloc(), obj);
    current->add(wrapper);
    current->push(wrapper);

    CallInfo callInfo(alloc(), false);
    if (!callInfo.init(current, 0))
        return false;

    
    if (makeInliningDecision(commonGetter, callInfo) && commonGetter->isInterpreted()) {
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

bool
IonBuilder::getPropTryInlineAccess(bool *emitted, PropertyName *name,
                                   bool barrier, types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);
    if (current->peek(-1)->type() != MIRType_Object)
        return true;

    BaselineInspector::ShapeVector shapes(alloc());
    if (!inspector->maybeShapesForPropertyOp(pc, shapes))
        return false;

    if (shapes.empty() || !CanInlinePropertyOpShapes(shapes))
        return true;

    MIRType rvalType = MIRTypeFromValueType(types->getKnownTypeTag());
    if (barrier || IsNullOrUndefined(rvalType))
        rvalType = MIRType_Value;

    MDefinition *obj = current->pop();
    if (shapes.length() == 1) {
        
        
        spew("Inlining monomorphic GETPROP");

        Shape *objShape = shapes[0];
        obj = addShapeGuard(obj, objShape, Bailout_ShapeGuard);

        Shape *shape = objShape->searchLinear(NameToId(name));
        JS_ASSERT(shape);

        if (!loadSlot(obj, shape, rvalType, barrier, types))
            return false;
    } else {
        JS_ASSERT(shapes.length() > 1);
        spew("Inlining polymorphic GETPROP");

        MGetPropertyPolymorphic *load = MGetPropertyPolymorphic::New(alloc(), obj, name);
        current->add(load);
        current->push(load);

        for (size_t i = 0; i < shapes.length(); i++) {
            Shape *objShape = shapes[i];
            Shape *shape =  objShape->searchLinear(NameToId(name));
            JS_ASSERT(shape);
            if (!load->addShape(objShape, shape))
                return false;
        }

        if (failedShapeGuard_)
            load->setNotMovable();

        load->setResultType(rvalType);
        if (!pushTypeBarrier(load, types, barrier))
            return false;
    }

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryCache(bool *emitted, PropertyName *name,
                            bool barrier, types::TemporaryTypeSet *types)
{
    JS_ASSERT(*emitted == false);
    bool accessGetter = inspector->hasSeenAccessedGetter(pc);

    MDefinition *obj = current->peek(-1);

    
    
    if (obj->type() != MIRType_Object) {
        types::TemporaryTypeSet *types = obj->resultTypeSet();
        if (!types || !types->objectOrSentinel())
            return true;
    }

    current->pop();
    MGetPropertyCache *load = MGetPropertyCache::New(alloc(), obj, name);

    
    
    
    
    
    
    if (obj->type() == MIRType_Object && !invalidatedIdempotentCache() &&
        info().executionMode() != ParallelExecution)
    {
        if (PropertyReadIsIdempotent(constraints(), obj, name))
            load->setIdempotent();
    }

    if (JSOp(*pc) == JSOP_CALLPROP) {
        if (!annotateGetPropertyCache(obj, load, obj->resultTypeSet(), types))
            return false;
    }

    
    if (accessGetter)
        load->setAllowGetters();

    current->add(load);
    current->push(load);

    if (load->isEffectful() && !resumeAfter(load))
        return false;

    if (accessGetter)
        barrier = true;

    if (needsToMonitorMissingProperties(types))
        barrier = true;

    
    
    if (!barrier)
        barrier = PropertyReadOnPrototypeNeedsTypeBarrier(constraints(), obj, name, types);

    MIRType rvalType = MIRTypeFromValueType(types->getKnownTypeTag());
    if (barrier || IsNullOrUndefined(rvalType))
        rvalType = MIRType_Value;
    load->setResultType(rvalType);

    if (!pushTypeBarrier(load, types, barrier))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::needsToMonitorMissingProperties(types::TemporaryTypeSet *types)
{
    
    
    
    
    return (info().executionMode() == ParallelExecution &&
            !types->hasType(types::Type::UndefinedType()));
}

bool
IonBuilder::jsop_setprop(PropertyName *name)
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->pop();

    bool emitted = false;

    
    
    if (info().executionMode() == DefinitePropertiesAnalysis) {
        MInstruction *ins = MCallSetProperty::New(alloc(), obj, value, name, script()->strict);
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

    
    if (!setPropTryDefiniteSlot(&emitted, obj, name, value, barrier, objTypes) || emitted)
        return emitted;

    
    if (!setPropTryInlineAccess(&emitted, obj, name, value, barrier, objTypes) || emitted)
        return emitted;

    
    if (!setPropTryCache(&emitted, obj, name, value, barrier, objTypes) || emitted)
        return emitted;

    
    MInstruction *ins = MCallSetProperty::New(alloc(), obj, value, name, script()->strict);
    current->add(ins);
    current->push(value);
    return resumeAfter(ins);
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

    
    if (objTypes->getKnownTypeTag() != JSVAL_TYPE_OBJECT) {
        MGuardObject *guardObj = MGuardObject::New(alloc(), obj);
        current->add(guardObj);
        obj = guardObj;
    }

    
    
    uint32_t depth = current->stackDepth() + 3;
    if (depth > current->nslots()) {
        if (!current->increaseSlots(depth - current->nslots()))
            return false;
    }

    pushConstant(ObjectValue(*commonSetter));

    MPassArg *wrapper = MPassArg::New(alloc(), obj);
    current->push(wrapper);
    current->add(wrapper);

    MPassArg *arg = MPassArg::New(alloc(), value);
    current->push(arg);
    current->add(arg);

    
    
    CallInfo callInfo(alloc(), false);
    if (!callInfo.init(current, 1))
        return false;

    
    callInfo.markAsSetter();

    
    if (makeInliningDecision(commonSetter, callInfo) && commonSetter->isInterpreted()) {
        if (!inlineScriptedCall(callInfo, commonSetter))
            return false;

        *emitted = true;
        return true;
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

    
    JS_ASSERT(setter->jitInfo()->type == JSJitInfo::Setter);
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
    TypeRepresentationSet fieldTypeReprs;
    int32_t fieldOffset;
    size_t fieldIndex;
    if (!lookupTypedObjectField(obj, name, &fieldOffset, &fieldTypeReprs,
                                &fieldIndex))
        return false;
    if (fieldTypeReprs.empty())
        return true;

    switch (fieldTypeReprs.kind()) {
      case TypeRepresentation::X4:
        
        return true;

      case TypeRepresentation::Reference:
      case TypeRepresentation::Struct:
      case TypeRepresentation::Array:
        
        return true;

      case TypeRepresentation::Scalar:
        return setPropTryScalarPropOfTypedObject(emitted, obj, fieldOffset,
                                                 value, fieldTypeReprs);
    }

    MOZ_ASSUME_UNREACHABLE("Unknown kind");
}

bool
IonBuilder::setPropTryScalarPropOfTypedObject(bool *emitted,
                                              MDefinition *obj,
                                              int32_t fieldOffset,
                                              MDefinition *value,
                                              TypeRepresentationSet fieldTypeReprs)
{
    
    if (!fieldTypeReprs.singleton())
        return true;
    ScalarTypeRepresentation *fieldTypeRepr =
        fieldTypeReprs.getTypeRepresentation()->asScalar();

    

    if (!storeScalarTypedObjectValue(obj, constantInt(fieldOffset), fieldTypeRepr, value))
        return false;

    current->push(value);

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryDefiniteSlot(bool *emitted, MDefinition *obj,
                                   PropertyName *name, MDefinition *value,
                                   bool barrier, types::TemporaryTypeSet *objTypes)
{
    JS_ASSERT(*emitted == false);

    if (barrier)
        return true;

    types::HeapTypeSetKey property;
    if (!getDefiniteSlot(obj->resultTypeSet(), name, &property))
        return true;

    MStoreFixedSlot *fixed = MStoreFixedSlot::New(alloc(), obj, property.maybeTypes()->definiteSlot(), value);
    current->add(fixed);
    current->push(value);

    if (property.needsBarrier(constraints()))
        fixed->setNeedsBarrier();

    if (!resumeAfter(fixed))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryInlineAccess(bool *emitted, MDefinition *obj,
                                   PropertyName *name,
                                   MDefinition *value, bool barrier,
                                   types::TemporaryTypeSet *objTypes)
{
    JS_ASSERT(*emitted == false);

    if (barrier)
        return true;

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
    } else {
        JS_ASSERT(shapes.length() > 1);
        spew("Inlining polymorphic SETPROP");

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
    }

    *emitted = true;
    return true;
}

bool
IonBuilder::setPropTryCache(bool *emitted, MDefinition *obj,
                            PropertyName *name, MDefinition *value,
                            bool barrier, types::TemporaryTypeSet *objTypes)
{
    JS_ASSERT(*emitted == false);

    
    MSetPropertyCache *ins = MSetPropertyCache::New(alloc(), obj, value, name, script()->strict, barrier);

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
        RegExpStatics *res = script()->global().getRegExpStatics();

        DebugOnly<uint32_t> origFlags = reobj->getFlags();
        DebugOnly<uint32_t> staticsFlags = res->getFlags();
        JS_ASSERT((origFlags & staticsFlags) == staticsFlags);

        if (!reobj->global() && !reobj->sticky())
            mustClone = false;
    }

    JSObject *prototype = reobj->getProto();

    MRegExp *regexp = MRegExp::New(alloc(), reobj, prototype, mustClone);
    current->add(regexp);
    current->push(regexp);

    regexp->setMovable();

    
    
    
    if (reobj->sticky() || reobj->global()) {
        JS_ASSERT(mustClone);
        MConstant *zero = MConstant::New(alloc(), Int32Value(0));
        current->add(zero);

        MStoreFixedSlot *lastIndex =
            MStoreFixedSlot::New(alloc(), regexp, RegExpObject::lastIndexSlot(), zero);
        current->add(lastIndex);
    }

    return true;
}

bool
IonBuilder::jsop_object(JSObject *obj)
{
    MConstant *ins = MConstant::New(alloc(), ObjectValue(*obj));
    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_lambda(JSFunction *fun)
{
    JS_ASSERT(analysis().usesScopeChain());
    if (fun->isArrow())
        return abort("bound arrow function");
    if (fun->isNative() && IsAsmJSModuleNative(fun->native()))
        return abort("asm.js module function");

    MLambda *ins = MLambda::New(alloc(), current->scopeChain(), fun);
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
        current->add(MSetArgumentsObjectArg::New(alloc(), current->argumentsObject(), GET_SLOTNO(pc), val));
        return true;
    }

    
    
    
    
    
    if (info().argumentsAliasesFormals()) {
        
        if (isInlineBuilder())
            return abort("JSOP_SETARG with magic arguments in inlined function.");

        MSetFrameArgument *store = MSetFrameArgument::New(alloc(), arg, val);
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
                    if (!argTypes[arg].addType(types::Type::UnknownType(), alloc_->lifoAlloc()))
                        return false;
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

    
    unsigned attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT;
    if (JSOp(*pc) == JSOP_DEFCONST)
        attrs |= JSPROP_READONLY;

    
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
    if (!info().fun())
        return abort("JSOP_THIS outside of a JSFunction.");

    if (script()->strict || info().fun()->isSelfHostedBuiltin()) {
        
        current->pushSlot(info().thisSlot());
        return true;
    }

    if (thisTypes->getKnownTypeTag() == JSVAL_TYPE_OBJECT ||
        (thisTypes->empty() && baselineFrame_ && baselineFrame_->thisValue().isObject()))
    {
        
        
        
        current->pushSlot(info().thisSlot());
        return true;
    }

    
    
    
    
    if (info().executionMode() == DefinitePropertiesAnalysis) {
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

    ins->infer();

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
        ins = MUnbox::New(alloc(), ins, MIRType_String, MUnbox::Fallible, Bailout_BaselineInfo);
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
    if (!outerScript || !outerScript->treatAsRunOnce)
        return false;

    types::TypeObjectKey *funType = types::TypeObjectKey::get(outerScript->function());
    if (funType->hasFlags(constraints(), types::OBJECT_FLAG_RUNONCE_INVALIDATED))
        return false;

    
    
    
    

    
    
    

    MDefinition *scope = current->getSlot(info().scopeChainSlot());
    scope->setFoldedUnchecked();

    JSObject *environment = script()->function()->environment();
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
        JSObject *scope = baselineFrame_->scopeChain();
        if (scope->is<CallObject>() &&
            scope->as<CallObject>().callee().nonLazyScript() == outerScript)
        {
            JS_ASSERT(scope->hasSingletonType());
            *pcall = scope;
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
        PropertyName *name = ScopeCoordinateName(script(), pc);
        bool succeeded;
        if (!getStaticName(call, name, &succeeded))
            return false;
        if (succeeded)
            return true;
    }

    MDefinition *obj = walkScopeChain(sc.hops);

    Shape *shape = ScopeCoordinateToStaticScopeShape(script(), pc);

    MInstruction *load;
    if (shape->numFixedSlots() <= sc.slot) {
        MInstruction *slots = MSlots::New(alloc(), obj);
        current->add(slots);

        load = MLoadSlot::New(alloc(), slots, sc.slot - shape->numFixedSlots());
    } else {
        load = MLoadFixedSlot::New(alloc(), obj, sc.slot);
    }

    current->add(load);
    current->push(load);

    types::TemporaryTypeSet *types = bytecodeTypes(pc);
    return pushTypeBarrier(load, types, true);
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
        PropertyName *name = ScopeCoordinateName(script(), pc);

        if (call) {
            
            
            MInstruction *constant = MConstant::New(alloc(), ObjectValue(*call));
            current->add(constant);
            current->push(constant);
            current->push(value);
            return setStaticName(call, name);
        }

        
        
        MDefinition *obj = walkScopeChain(sc.hops);
        current->push(obj);
        current->push(value);
        return jsop_setprop(name);
    }

    MDefinition *rval = current->peek(-1);
    MDefinition *obj = walkScopeChain(sc.hops);

    Shape *shape = ScopeCoordinateToStaticScopeShape(script(), pc);

    if (NeedsPostBarrier(info(), rval))
        current->add(MPostWriteBarrier::New(alloc(), obj, rval));

    MInstruction *store;
    if (shape->numFixedSlots() <= sc.slot) {
        MInstruction *slots = MSlots::New(alloc(), obj);
        current->add(slots);

        store = MStoreSlot::NewBarriered(alloc(), slots, sc.slot - shape->numFixedSlots(), rval);
    } else {
        store = MStoreFixedSlot::NewBarriered(alloc(), obj, sc.slot, rval);
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
    MIn *ins = new MIn(id, obj);

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

        rhs->setFoldedUnchecked();

        MInstanceOf *ins = new MInstanceOf(obj, protoObject);

        current->add(ins);
        current->push(ins);

        return resumeAfter(ins);
    } while (false);

    MCallInstanceOf *ins = new MCallInstanceOf(obj, rhs);

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
    return types::TypeScript::BytecodeTypes(script(), pc, &typeArrayHint, typeArray);
}

TypeRepresentationSetHash *
IonBuilder::getOrCreateReprSetHash()
{
    if (!reprSetHash_) {
        TypeRepresentationSetHash *hash =
            alloc_->lifoAlloc()->new_<TypeRepresentationSetHash>(alloc());
        if (!hash || !hash->init())
            return nullptr;

        reprSetHash_ = hash;
    }
    return reprSetHash_;
}

bool
IonBuilder::lookupTypeRepresentationSet(MDefinition *typedObj,
                                        TypeRepresentationSet *out)
{
    *out = TypeRepresentationSet(); 

    
    if (typedObj->isNewDerivedTypedObject()) {
        *out = typedObj->toNewDerivedTypedObject()->set();
        return true;
    }

    
    types::TemporaryTypeSet *types = typedObj->resultTypeSet();
    if (!types || types->getKnownTypeTag() != JSVAL_TYPE_OBJECT)
        return true;

    
    if (types->unknownObject())
        return true;

    TypeRepresentationSetBuilder set;
    for (uint32_t i = 0; i < types->getObjectCount(); i++) {
        types::TypeObject *type = types->getTypeObject(i);
        if (!type || type->unknownProperties())
            return true;

        if (!type->hasTypedObject())
            return true;

        TypeRepresentation *typeRepr = type->typedObject()->typeRepr;
        if (!set.insert(typeRepr))
            return false;
    }

    return set.build(*this, out);
}

MDefinition *
IonBuilder::loadTypedObjectType(MDefinition *typedObj)
{
    
    
    
    
    if (typedObj->isNewDerivedTypedObject())
        return typedObj->toNewDerivedTypedObject()->type();

    MInstruction *load = MLoadFixedSlot::New(alloc(), typedObj,
                                             JS_DATUM_SLOT_TYPE_OBJ);
    current->add(load);
    return load;
}








void
IonBuilder::loadTypedObjectData(MDefinition *typedObj,
                                MDefinition *offset,
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

    *owner = typedObj;
    *ownerOffset = offset;
}






void
IonBuilder::loadTypedObjectElements(MDefinition *typedObj,
                                    MDefinition *offset,
                                    int32_t unit,
                                    MDefinition **ownerElements,
                                    MDefinition **ownerScaledOffset)
{
    MDefinition *owner, *ownerOffset;
    loadTypedObjectData(typedObj, offset, &owner, &ownerOffset);

    
    MTypedObjectElements *elements = MTypedObjectElements::New(alloc(), owner);
    current->add(elements);

    
    if (unit != 1) {
        MDiv *scaledOffset = MDiv::NewAsmJS(alloc(), ownerOffset, constantInt(unit), MIRType_Int32);
        current->add(scaledOffset);
        *ownerScaledOffset = scaledOffset;
    } else {
        *ownerScaledOffset = ownerOffset;
    }

    *ownerElements = elements;
}





bool
IonBuilder::lookupTypedObjectField(MDefinition *typedObj,
                                   PropertyName *name,
                                   int32_t *fieldOffset,
                                   TypeRepresentationSet *fieldTypeReprs,
                                   size_t *fieldIndex)
{
    TypeRepresentationSet objTypeReprs;
    if (!lookupTypeRepresentationSet(typedObj, &objTypeReprs))
        return false;

    
    if (!objTypeReprs.allOfKind(TypeRepresentation::Struct))
        return true;

    
    size_t offset;
    if (!objTypeReprs.fieldNamed(*this, NameToId(name), &offset,
                                 fieldTypeReprs, fieldIndex))
        return false;
    if (fieldTypeReprs->empty())
        return true;

    
    if (offset >= size_t(INT_MAX)) {
        *fieldTypeReprs = TypeRepresentationSet();
        return true;
    }

    *fieldOffset = int32_t(offset);
    JS_ASSERT(*fieldOffset >= 0);

    return true;
}

MDefinition *
IonBuilder::typeObjectForElementFromArrayStructType(MDefinition *typeObj)
{
    MInstruction *elemType = MLoadFixedSlot::New(alloc(), typeObj, JS_TYPEOBJ_SLOT_ARRAY_ELEM_TYPE);
    current->add(elemType);

    MInstruction *unboxElemType = MUnbox::New(alloc(), elemType, MIRType_Object, MUnbox::Infallible);
    current->add(unboxElemType);

    return unboxElemType;
}

MDefinition *
IonBuilder::typeObjectForFieldFromStructType(MDefinition *typeObj,
                                             size_t fieldIndex)
{
    

    MInstruction *fieldTypes = MLoadFixedSlot::New(alloc(), typeObj, JS_TYPEOBJ_SLOT_STRUCT_FIELD_TYPES);
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
                                        MDefinition *offset,
                                        ScalarTypeRepresentation *typeRepr,
                                        MDefinition *value)
{
    
    MDefinition *elements, *scaledOffset;
    loadTypedObjectElements(typedObj, offset, typeRepr->alignment(), &elements, &scaledOffset);

    
    MDefinition *toWrite = value;
    if (typeRepr->type() == ScalarTypeRepresentation::TYPE_UINT8_CLAMPED) {
        toWrite = MClampToUint8::New(alloc(), value);
        current->add(toWrite->toInstruction());
    }

    MStoreTypedArrayElement *store =
        MStoreTypedArrayElement::New(alloc(), elements, scaledOffset, toWrite,
                                     typeRepr->type());
    current->add(store);

    return true;
}

MConstant *
IonBuilder::constant(const Value &v)
{
    MConstant *c = MConstant::New(alloc(), v);
    current->add(c);
    return c;
}

MConstant *
IonBuilder::constantInt(int32_t i)
{
    return constant(Int32Value(i));
}
