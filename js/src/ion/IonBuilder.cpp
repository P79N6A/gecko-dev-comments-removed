






#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "Lowering.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonAnalysis.h"
#include "IonSpewer.h"
#include "frontend/BytecodeEmitter.h"

#include "jsscriptinlines.h"
#include "jstypedarrayinlines.h"
#include "ExecutionModeInlines.h"

#ifdef JS_THREADSAFE
# include "prthread.h"
#endif

using namespace js;
using namespace js::ion;

using mozilla::DebugOnly;

IonBuilder::IonBuilder(JSContext *cx, TempAllocator *temp, MIRGraph *graph,
                       TypeOracle *oracle, CompileInfo *info, size_t inliningDepth, uint32_t loopDepth)
  : MIRGenerator(cx->compartment, temp, graph, info),
    backgroundCodegen_(NULL),
    recompileInfo(cx->compartment->types.compiledInfo),
    cx(cx),
    loopDepth_(loopDepth),
    callerResumePoint_(NULL),
    callerBuilder_(NULL),
    oracle(oracle),
    inliningDepth(inliningDepth),
    failedBoundsCheck_(info->script()->failedBoundsCheck),
    failedShapeGuard_(info->script()->failedShapeGuard),
    lazyArguments_(NULL)
{
    script_.init(info->script());
    pc = info->startPC();
}

void
IonBuilder::clearForBackEnd()
{
    cx = NULL;
    oracle = NULL;
}

bool
IonBuilder::abort(const char *message, ...)
{
    
#ifdef DEBUG
    va_list ap;
    va_start(ap, message);
    abortFmt(message, ap);
    va_end(ap);
    IonSpew(IonSpew_Abort, "aborted @ %s:%d", script_->filename, PCToLineNumber(script_, pc));
#endif
    return false;
}

void
IonBuilder::spew(const char *message)
{
    
#ifdef DEBUG
    IonSpew(IonSpew_MIR, "%s @ %s:%d", message, script_->filename, PCToLineNumber(script_, pc));
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
    state.tableswitch.breaks = NULL;
    state.tableswitch.ins = ins;
    state.tableswitch.currentBlock = 0;
    return state;
}

IonBuilder::CFGState
IonBuilder::CFGState::LookupSwitch(jsbytecode *exitpc)
{
    CFGState state;
    state.state = LOOKUP_SWITCH;
    state.stopAt = exitpc;
    state.lookupswitch.exitpc = exitpc;
    state.lookupswitch.breaks = NULL;
    state.lookupswitch.bodies =
        (FixedList<MBasicBlock *> *)GetIonContext()->temp->allocate(sizeof(FixedList<MBasicBlock *>));
    state.lookupswitch.currentBlock = 0;
    return state;
}

JSFunction *
IonBuilder::getSingleCallTarget(uint32_t argc, jsbytecode *pc)
{
    AutoAssertNoGC nogc;

    types::StackTypeSet *calleeTypes = oracle->getCallTarget(script(), argc, pc);
    if (!calleeTypes)
        return NULL;

    RawObject obj = calleeTypes->getSingleton();
    if (!obj || !obj->isFunction())
        return NULL;

    return obj->toFunction();
}

uint32_t
IonBuilder::getPolyCallTargets(uint32_t argc, jsbytecode *pc,
                               AutoObjectVector &targets, uint32_t maxTargets)
{
    types::TypeSet *calleeTypes = oracle->getCallTarget(script_, argc, pc);
    if (!calleeTypes)
        return 0;

    if (calleeTypes->baseFlags() != 0)
        return 0;

    unsigned objCount = calleeTypes->getObjectCount();

    if (objCount == 0 || objCount > maxTargets)
        return 0;

    for(unsigned i = 0; i < objCount; i++) {
        JSObject *obj = calleeTypes->getSingleObject(i);
        if (!obj || !obj->isFunction())
            return 0;
        targets.append(obj);
    }

    return (uint32_t) objCount;
}

bool
IonBuilder::canInlineTarget(JSFunction *target)
{
    AssertCanGC();

    if (!target->isInterpreted()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to non-interpreted");
        return false;
    }

    if (target->getParent() != &script_->global()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to scope mismatch");
        return false;
    }

    RootedScript inlineScript(cx, target->nonLazyScript());
    ExecutionMode executionMode = info().executionMode();
    if (!CanIonCompile(inlineScript, executionMode)) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to disable Ion compilation");
        return false;
    }

    
    IonBuilder *builder = callerBuilder_;
    while (builder) {
        if (builder->script() == inlineScript) {
            IonSpew(IonSpew_Inlining, "Not inlining recursive call");
            return false;
        }
        builder = builder->callerBuilder_;
    }

    bool canInline = oracle->canEnterInlinedFunction(target);

    if (!canInline) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to oracle veto %d", script_->lineno);
        return false;
    }

    IonSpew(IonSpew_Inlining, "Inlining good to go!");
    return true;
}

void
IonBuilder::popCfgStack()
{
    if (cfgStack_.back().isLoop())
        loops_.popBack();
    cfgStack_.popBack();
}

bool
IonBuilder::pushLoop(CFGState::State initial, jsbytecode *stopAt, MBasicBlock *entry,
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
    state.loop.entry = entry;
    state.loop.successor = NULL;
    state.loop.breaks = NULL;
    state.loop.continues = NULL;
    return cfgStack_.append(state);
}

bool
IonBuilder::build()
{
    current = newBlock(pc);
    if (!current)
        return false;

    IonSpew(IonSpew_Scripts, "Analyzing script %s:%d (%p) (usecount=%d) (maxloopcount=%d)",
            script_->filename, script_->lineno, (void *)script_, (int)script_->getUseCount(),
            (int)script_->getMaxLoopCount());

    if (!graph().addScript(script_))
        return false;

    if (!initParameters())
        return false;

    
    for (uint32_t i = 0; i < info().nlocals(); i++) {
        MConstant *undef = MConstant::New(UndefinedValue());
        current->add(undef);
        current->initSlot(info().localSlot(i), undef);
    }

    
    
    
    
    {
        MInstruction *scope = MConstant::New(UndefinedValue());
        current->add(scope);
        current->initSlot(info().scopeChainSlot(), scope);
    }

    
    current->makeStart(MStart::New(MStart::StartType_Default));
    if (instrumentedProfiling())
        current->add(MFunctionBoundary::New(script_, MFunctionBoundary::Enter));

    
    
    rewriteParameters();

    
    if (!initScopeChain())
        return false;

    
    MCheckOverRecursed *check = new MCheckOverRecursed;
    current->add(check);
    check->setResumePoint(current->entryResumePoint());

    
    if (info().fun())
        current->getSlot(info().thisSlot())->setGuard();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (uint32_t i = 0; i < CountArgSlots(info().fun()); i++) {
        MInstruction *ins = current->getEntrySlot(i)->toInstruction();
        if (ins->type() == MIRType_Value)
            ins->setResumePoint(current->entryResumePoint());
    }

    
    insertRecompileCheck();

    if (script_->argumentsHasVarBinding()) {
        lazyArguments_ = MConstant::New(MagicValue(JS_OPTIMIZED_ARGUMENTS));
        current->add(lazyArguments_);
    }

    if (!traverseBytecode())
        return false;

    if (!processIterators())
        return false;

    JS_ASSERT(loopDepth_ == 0);
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
                        MDefinition *thisDefn, MDefinitionVector &argv)
{
    IonSpew(IonSpew_Scripts, "Inlining script %s:%d (%p)",
            script_->filename, script_->lineno, (void *)script_);

    if (!graph().addScript(script_))
        return false;

    callerBuilder_ = callerBuilder;
    callerResumePoint_ = callerResumePoint;

    if (callerBuilder->failedBoundsCheck_)
        failedBoundsCheck_ = true;

    if (callerBuilder->failedShapeGuard_)
        failedShapeGuard_ = true;

    
    current = newBlock(pc);
    if (!current)
        return false;

    current->setCallerResumePoint(callerResumePoint);

    
    MBasicBlock *predecessor = callerBuilder->current;
    JS_ASSERT(predecessor == callerResumePoint->block());

    
    
    
    
    if (instrumentedProfiling())
        predecessor->add(MFunctionBoundary::New(script_,
                                                MFunctionBoundary::Inline_Enter,
                                                inliningDepth));

    predecessor->end(MGoto::New(current));
    if (!current->addPredecessorWithoutPhis(predecessor))
        return false;

    
    const size_t numActualArgs = argv.length() - 1;
    const size_t nargs = info().nargs();

    if (numActualArgs < nargs) {
        const size_t missing = nargs - numActualArgs;

        for (size_t i = 0; i < missing; i++) {
            MConstant *undef = MConstant::New(UndefinedValue());
            current->add(undef);
            if (!argv.append(undef))
                return false;
        }
    }

    
    JS_ASSERT(!script_->analysis()->usesScopeChain());
    MInstruction *scope = MConstant::New(UndefinedValue());
    current->add(scope);
    current->initSlot(info().scopeChainSlot(), scope);

    current->initSlot(info().thisSlot(), thisDefn);

    IonSpew(IonSpew_Inlining, "Initializing %u arg slots", nargs);

    
    MDefinitionVector::Range args = argv.all();
    args.popFront();
    JS_ASSERT(args.remain() >= nargs);
    for (size_t i = 0; i < nargs; ++i) {
        MDefinition *arg = args.popCopyFront();
        current->initSlot(info().argSlot(i), arg);
    }

    IonSpew(IonSpew_Inlining, "Initializing %u local slots", info().nlocals());

    
    for (uint32_t i = 0; i < info().nlocals(); i++) {
        MConstant *undef = MConstant::New(UndefinedValue());
        current->add(undef);
        current->initSlot(info().localSlot(i), undef);
    }

    IonSpew(IonSpew_Inlining, "Inline entry block MResumePoint %p, %u operands",
            (void *) current->entryResumePoint(), current->entryResumePoint()->numOperands());

    
    JS_ASSERT(current->entryResumePoint()->numOperands() == nargs + info().nlocals() + 2);

    return traverseBytecode();
}




void
IonBuilder::rewriteParameters()
{
    JS_ASSERT(info().scopeChainSlot() == 0);
    static const uint32_t START_SLOT = 1;

    for (uint32_t i = START_SLOT; i < CountArgSlots(info().fun()); i++) {
        MParameter *param = current->getSlot(i)->toParameter();

        
        
        types::StackTypeSet *types;
        if (param->index() == MParameter::THIS_SLOT)
            types = oracle->thisTypeSet(script_);
        else
            types = oracle->parameterTypeSet(script_, param->index());
        if (!types)
            continue;

        JSValueType definiteType = types->getKnownTypeTag();
        if (definiteType == JSVAL_TYPE_UNKNOWN)
            continue;

        MInstruction *actual = NULL;
        switch (definiteType) {
          case JSVAL_TYPE_UNDEFINED:
            param->setFoldedUnchecked();
            actual = MConstant::New(UndefinedValue());
            break;

          case JSVAL_TYPE_NULL:
            param->setFoldedUnchecked();
            actual = MConstant::New(NullValue());
            break;

          default:
            actual = MUnbox::New(param, MIRTypeFromValueType(definiteType), MUnbox::Infallible);
            break;
        }

        
        
        
        
        
        
        
        
        
        current->add(actual);
        current->rewriteSlot(i, actual);
    }
}

bool
IonBuilder::initParameters()
{
    if (!info().fun())
        return true;

    MParameter *param = MParameter::New(MParameter::THIS_SLOT,
                                        cloneTypeSet(oracle->thisTypeSet(script_)));
    current->add(param);
    current->initSlot(info().thisSlot(), param);

    for (uint32_t i = 0; i < info().nargs(); i++) {
        param = MParameter::New(i, cloneTypeSet(oracle->parameterTypeSet(script_, i)));
        current->add(param);
        current->initSlot(info().argSlot(i), param);
    }

    return true;
}

bool
IonBuilder::initScopeChain()
{
    MInstruction *scope = NULL;

    
    
    if (!script_->analysis()->usesScopeChain())
        return true;

    
    
    
    
    if (!script_->compileAndGo)
        return abort("non-CNG global scripts are not supported");

    if (JSFunction *fun = info().fun()) {
        MCallee *callee = MCallee::New();
        current->add(callee);

        scope = MFunctionEnvironment::New(callee);
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
        scope = MConstant::New(ObjectValue(script_->global()));
        current->add(scope);
    }

    current->setScopeChain(scope);
    return true;
}




























bool
IonBuilder::traverseBytecode()
{
    for (;;) {
        JS_ASSERT(pc < info().limitPC());

        for (;;) {
            if (!temp().ensureBallast())
                return false;

            
            
            
            if (!cfgStack_.empty() && cfgStack_.back().stopAt == pc) {
                ControlStatus status = processCfgStack();
                if (status == ControlStatus_Error)
                    return false;
                if (!current)
                    return true;
                continue;
            }

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            ControlStatus status;
            if ((status = snoopControlFlow(JSOp(*pc))) == ControlStatus_None)
                break;
            if (status == ControlStatus_Error)
                return false;
            if (!current)
                return true;
        }

        
        JSOp op = JSOp(*pc);
        if (!inspectOpcode(op))
            return false;

        pc += js_CodeSpec[op].length;
#ifdef TRACK_SNAPSHOTS
        current->updateTrackedPc(pc);
#endif
    }

    return true;
}

IonBuilder::ControlStatus
IonBuilder::snoopControlFlow(JSOp op)
{
    switch (op) {
      case JSOP_NOP:
        return maybeLoop(op, info().getNote(cx, pc));

      case JSOP_POP:
        return maybeLoop(op, info().getNote(cx, pc));

      case JSOP_RETURN:
      case JSOP_STOP:
        return processReturn(op);

      case JSOP_THROW:
        return processThrow();

      case JSOP_GOTO:
      {
        jssrcnote *sn = info().getNote(cx, pc);
        switch (sn ? SN_TYPE(sn) : SRC_NULL) {
          case SRC_BREAK:
          case SRC_BREAK2LABEL:
            return processBreak(op, sn);

          case SRC_CONTINUE:
          case SRC_CONT2LABEL:
            return processContinue(op, sn);

          case SRC_SWITCHBREAK:
            return processSwitchBreak(op, sn);

          case SRC_WHILE:
          case SRC_FOR_IN:
            
            return whileOrForInLoop(op, sn);

          default:
            
            JS_NOT_REACHED("unknown goto case");
            break;
        }
        break;
      }

      case JSOP_TABLESWITCH:
        return tableSwitch(op, info().getNote(cx, pc));

      case JSOP_LOOKUPSWITCH:
        return lookupSwitch(op, info().getNote(cx, pc));

      case JSOP_IFNE:
        
        
        JS_NOT_REACHED("we should never reach an ifne!");
        return ControlStatus_Error;

      default:
        break;
    }
    return ControlStatus_None;
}

bool
IonBuilder::inspectOpcode(JSOp op)
{
    AssertCanGC();

    
    if (js_CodeSpec[op].format & JOF_DECOMPOSE)
        return true;

    switch (op) {
      case JSOP_LOOPENTRY:
        insertRecompileCheck();
        return true;

      case JSOP_NOP:
        return true;

      case JSOP_LABEL:
        return true;

      case JSOP_UNDEFINED:
        return pushConstant(UndefinedValue());

      case JSOP_IFEQ:
        return jsop_ifeq(JSOP_IFEQ);

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
        return pushConstant(MagicValue(JS_ARRAY_HOLE));

      case JSOP_FALSE:
        return pushConstant(BooleanValue(false));

      case JSOP_TRUE:
        return pushConstant(BooleanValue(true));

      case JSOP_ARGUMENTS:
        return jsop_arguments();

      case JSOP_NOTEARG:
        return jsop_notearg();

      case JSOP_GETARG:
      case JSOP_CALLARG:
        current->pushArg(GET_SLOTNO(pc));
        return true;

      case JSOP_SETARG:
        
        
        
        
        
        if (info().hasArguments())
            return abort("NYI: arguments & setarg.");
        current->setArg(GET_SLOTNO(pc));
        return true;

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

      case JSOP_NEWINIT:
      {
        if (GET_UINT8(pc) == JSProto_Array)
            return jsop_newarray(0);
        RootedObject baseObj(cx, NULL);
        return jsop_newobject(baseObj);
      }

      case JSOP_NEWARRAY:
        return jsop_newarray(GET_UINT24(pc));

      case JSOP_NEWOBJECT:
      {
        RootedObject baseObj(cx, info().getObject(pc));
        return jsop_newobject(baseObj);
      }

      case JSOP_INITELEM:
        return jsop_initelem();

      case JSOP_INITPROP:
      {
        RootedPropertyName name(cx, info().getAtom(pc)->asPropertyName());
        return jsop_initprop(name);
      }

      case JSOP_ENDINIT:
        return true;

      case JSOP_FUNCALL:
        return jsop_funcall(GET_ARGC(pc));

      case JSOP_FUNAPPLY:
        return jsop_funapply(GET_ARGC(pc));

      case JSOP_CALL:
      case JSOP_NEW:
        return jsop_call(GET_ARGC(pc), (JSOp)*pc == JSOP_NEW);

      case JSOP_INT8:
        return pushConstant(Int32Value(GET_INT8(pc)));

      case JSOP_UINT16:
        return pushConstant(Int32Value(GET_UINT16(pc)));

      case JSOP_GETGNAME:
      case JSOP_CALLGNAME:
      {
        RootedPropertyName name(cx, info().getAtom(pc)->asPropertyName());
        return jsop_getgname(name);
      }

      case JSOP_BINDGNAME:
        return pushConstant(ObjectValue(script_->global()));

      case JSOP_SETGNAME:
      {
        RootedPropertyName name(cx, info().getAtom(pc)->asPropertyName());
        return jsop_setgname(name);
      }

      case JSOP_NAME:
      case JSOP_CALLNAME:
      {
        RootedPropertyName name(cx, info().getAtom(pc)->asPropertyName());
        return jsop_getname(name);
      }

      case JSOP_INTRINSICNAME:
      case JSOP_CALLINTRINSIC:
      {
        RootedPropertyName name(cx, info().getAtom(pc)->asPropertyName());
        return jsop_intrinsicname(name);
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
        
        JS_NOT_REACHED("JSOP_LOOPHEAD outside loop");
        return true;

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
        MCallee *callee = MCallee::New();
        current->add(callee);
        current->push(callee);
        return callee;
      }

      case JSOP_GETPROP:
      case JSOP_CALLPROP:
      {
        RootedPropertyName name(cx, info().getAtom(pc)->asPropertyName());
        return jsop_getprop(name);
      }

      case JSOP_SETPROP:
      case JSOP_SETNAME:
      {
        RootedPropertyName name(cx, info().getAtom(pc)->asPropertyName());
        return jsop_setprop(name);
      }

      case JSOP_DELPROP:
      {
        RootedPropertyName name(cx, info().getAtom(pc)->asPropertyName());
        return jsop_delprop(name);
      }

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

      case JSOP_INSTANCEOF:
        return jsop_instanceof();

      default:
#ifdef DEBUG
        return abort("Unsupported opcode: %s (line %d)", js_CodeName[op], info().lineno(cx, pc));
#else
        return abort("Unsupported opcode: %d (line %d)", op, info().lineno(cx, pc));
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

      case CFGState::LOOKUP_SWITCH:
        return processNextLookupSwitchCase(state);

      case CFGState::COND_SWITCH_CASE:
        return processCondSwitchCase(state);

      case CFGState::COND_SWITCH_BODY:
        return processCondSwitchBody(state);

      case CFGState::AND_OR:
        return processAndOrEnd(state);

      default:
        JS_NOT_REACHED("unknown cfgstate");
    }
    return ControlStatus_Error;
}

IonBuilder::ControlStatus
IonBuilder::processIfEnd(CFGState &state)
{
    if (current) {
        
        
        
        current->end(MGoto::New(state.branch.ifFalse));

        if (!state.branch.ifFalse->addPredecessor(current))
            return ControlStatus_Error;
    }

    current = state.branch.ifFalse;
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
    current = state.branch.ifFalse;
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

    
    pred->end(MGoto::New(join));

    if (other) {
        other->end(MGoto::New(join));
        if (!join->addPredecessor(other))
            return ControlStatus_Error;
    }

    
    current = join;
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

    
    
    
    current = state.loop.successor;
    if (current) {
        JS_ASSERT(current->loopDepth() == loopDepth_);
        graph().moveBlockToEnd(current);
    }

    
    if (state.loop.breaks) {
        MBasicBlock *block = createBreakCatchBlock(state.loop.breaks, state.loop.exitpc);
        if (!block)
            return ControlStatus_Error;

        if (current) {
            current->end(MGoto::New(block));
            if (!block->addPredecessor(current))
                return ControlStatus_Error;
        }

        current = block;
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

    
    
    if (!state.loop.entry->setBackedge(current))
        return ControlStatus_Error;
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
            
            
            successor->end(MGoto::New(block));
            if (!block->addPredecessor(successor))
                return ControlStatus_Error;
        }
        successor = block;
    }

    current = successor;

    
    if (!current)
        return ControlStatus_Ended;

    pc = current->pc();
    return ControlStatus_Joined;
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
    current->end(MGoto::New(header));

    state.state = CFGState::DO_WHILE_LOOP_COND;
    state.stopAt = state.loop.updateEnd;
    pc = state.loop.updatepc;
    current = header;
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

    
    MTest *test = MTest::New(vins, state.loop.entry, successor);
    current->end(test);
    return finishLoop(state, successor);
}

IonBuilder::ControlStatus
IonBuilder::processWhileCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE);

    
    MDefinition *ins = current->pop();

    
    MBasicBlock *body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc, loopDepth_ - 1);
    if (!body || !state.loop.successor)
        return ControlStatus_Error;

    MTest *test = MTest::New(ins, body, state.loop.successor);
    current->end(test);

    state.state = CFGState::WHILE_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    current = body;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processWhileBodyEnd(CFGState &state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    if (!current)
        return processBrokenLoop(state);

    current->end(MGoto::New(state.loop.entry));
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

    MTest *test = MTest::New(ins, body, state.loop.successor);
    current->end(test);

    state.state = CFGState::FOR_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    current = body;
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

    current->end(MGoto::New(state.loop.entry));
    return finishLoop(state, state.loop.successor);
}

bool
IonBuilder::processDeferredContinues(CFGState &state)
{
    
    
    if (state.loop.continues) {
        DeferredEdge *edge = state.loop.continues;

        MBasicBlock *update = newBlock(edge->block, loops_.back().continuepc);
        if (!update)
            return false;

        if (current) {
            current->end(MGoto::New(update));
            if (!update->addPredecessor(current))
                return ControlStatus_Error;
        }

        
        
        edge->block->end(MGoto::New(update));
        edge = edge->next;

        
        while (edge) {
            edge->block->end(MGoto::New(update));
            if (!update->addPredecessor(edge->block))
                return ControlStatus_Error;
            edge = edge->next;
        }
        state.loop.continues = NULL;

        current = update;
    }

    return true;
}

MBasicBlock *
IonBuilder::createBreakCatchBlock(DeferredEdge *edge, jsbytecode *pc)
{
    
    MBasicBlock *successor = newBlock(edge->block, pc);
    if (!successor)
        return NULL;

    
    
    edge->block->end(MGoto::New(successor));
    edge = edge->next;

    
    while (edge) {
        edge->block->end(MGoto::New(successor));
        if (!successor->addPredecessor(edge->block))
            return NULL;
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
        current->end(MGoto::New(successor));
        successor->addPredecessor(current);

        
        graph().moveBlockToEnd(successor);
    }

    
    
    if (state.tableswitch.currentBlock+1 < state.tableswitch.ins->numBlocks())
        state.stopAt = state.tableswitch.ins->getBlock(state.tableswitch.currentBlock+1)->pc();
    else
        state.stopAt = state.tableswitch.exitpc;

    current = successor;
    pc = current->pc();
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processNextLookupSwitchCase(CFGState &state)
{
    JS_ASSERT(state.state == CFGState::LOOKUP_SWITCH);

    size_t curBlock = state.lookupswitch.currentBlock;
    IonSpew(IonSpew_MIR, "processNextLookupSwitchCase curBlock=%d", curBlock);

    state.lookupswitch.currentBlock = ++curBlock;

    
    if (curBlock >= state.lookupswitch.bodies->length())
        return processSwitchEnd(state.lookupswitch.breaks, state.lookupswitch.exitpc);

    
    MBasicBlock *successor = (*state.lookupswitch.bodies)[curBlock];

    
    
    
    if (current) {
        current->end(MGoto::New(successor));
        successor->addPredecessor(current);
    }

    
    graph().moveBlockToEnd(successor);

    
    
    if (curBlock + 1 < state.lookupswitch.bodies->length())
        state.stopAt = (*state.lookupswitch.bodies)[curBlock + 1]->pc();
    else
        state.stopAt = state.lookupswitch.exitpc;

    current = successor;
    pc = current->pc();
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processAndOrEnd(CFGState &state)
{
    
    
    current->end(MGoto::New(state.branch.ifFalse));

    if (!state.branch.ifFalse->addPredecessor(current))
        return ControlStatus_Error;

    current = state.branch.ifFalse;
    graph().moveBlockToEnd(current);
    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processBreak(JSOp op, jssrcnote *sn)
{
    JS_ASSERT(op == JSOP_GOTO);

    
    CFGState *found = NULL;
    jsbytecode *target = pc + GetJumpOffset(pc);
    for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
        CFGState &cfg = cfgStack_[loops_[i].cfgEntry];
        if (cfg.loop.exitpc == target) {
            found = &cfg;
            break;
        }
    }

    if (!found) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        abort("could not find the target of a break");
        return ControlStatus_Error;
    }

    
    
    CFGState &state = *found;

    state.loop.breaks = new DeferredEdge(current, state.loop.breaks);

    current = NULL;
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
IonBuilder::processContinue(JSOp op, jssrcnote *sn)
{
    JS_ASSERT(op == JSOP_GOTO);

    
    CFGState *found = NULL;
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

    current = NULL;
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processSwitchBreak(JSOp op, jssrcnote *sn)
{
    JS_ASSERT(op == JSOP_GOTO);

    
    CFGState *found = NULL;
    jsbytecode *target = pc + GetJumpOffset(pc);
    for (size_t i = switches_.length() - 1; i < switches_.length(); i--) {
        if (switches_[i].continuepc == target) {
            found = &cfgStack_[switches_[i].cfgEntry];
            break;
        }
    }

    
    
    JS_ASSERT(found);
    CFGState &state = *found;

    DeferredEdge **breaks = NULL;
    switch (state.state) {
      case CFGState::TABLE_SWITCH:
        breaks = &state.tableswitch.breaks;
        break;
      case CFGState::LOOKUP_SWITCH:
        breaks = &state.lookupswitch.breaks;
        break;
      case CFGState::COND_SWITCH_BODY:
        breaks = &state.condswitch.breaks;
        break;
      default:
        JS_NOT_REACHED("Unexpected switch state.");
        return ControlStatus_Error;
    }

    *breaks = new DeferredEdge(current, *breaks);

    current = NULL;
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processSwitchEnd(DeferredEdge *breaks, jsbytecode *exitpc)
{
    
    
    
    if (!breaks && !current)
        return ControlStatus_Ended;

    
    
    
    MBasicBlock *successor = NULL;
    if (breaks)
        successor = createBreakCatchBlock(breaks, exitpc);
    else
        successor = newBlock(current, exitpc);

    if (!successor)
        return ControlStatus_Ended;

    
    
    if (current) {
        current->end(MGoto::New(successor));
        if (breaks)
            successor->addPredecessor(current);
    }

    pc = exitpc;
    current = successor;
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
        JS_NOT_REACHED("unexpected opcode");
        return ControlStatus_Error;
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

    
    jssrcnote *sn = info().getNote(cx, pc);
    if (sn) {
        jsbytecode *ifne = pc + js_GetSrcNoteOffset(sn, 0);

        jsbytecode *expected_ifne;
        switch (state.state) {
          case CFGState::DO_WHILE_LOOP_BODY:
            expected_ifne = state.loop.updateEnd;
            break;

          default:
            JS_NOT_REACHED("JSOP_LOOPHEAD unexpected source note");
            return;
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

    jssrcnote *sn2 = info().getNote(cx, pc+1);
    int offset = js_GetSrcNoteOffset(sn2, 0);
    jsbytecode *ifne = pc + offset + 1;
    JS_ASSERT(ifne > pc);

    
    jsbytecode *loopHead = GetNextPc(pc);
    JS_ASSERT(JSOp(*loopHead) == JSOP_LOOPHEAD);
    JS_ASSERT(loopHead == ifne + GetJumpOffset(ifne));

    jsbytecode *loopEntry = GetNextPc(loopHead);
    if (info().hasOsrAt(loopEntry)) {
        MBasicBlock *preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(preheader));
        current = preheader;
    }

    MBasicBlock *header = newPendingLoopHeader(current, pc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(header));

    jsbytecode *bodyStart = GetNextPc(GetNextPc(pc));
    jsbytecode *bodyEnd = conditionpc;
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!pushLoop(CFGState::DO_WHILE_LOOP_BODY, conditionpc, header,
                  bodyStart, bodyEnd, exitpc, conditionpc))
    {
        return ControlStatus_Error;
    }

    CFGState &state = cfgStack_.back();
    state.loop.updatepc = conditionpc;
    state.loop.updateEnd = ifne;

    current = header;
    if (!jsop_loophead(GetNextPc(pc)))
        return ControlStatus_Error;

    pc = bodyStart;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::whileOrForInLoop(JSOp op, jssrcnote *sn)
{
    
    
    
    
    
    
    
    
    
    size_t which = (SN_TYPE(sn) == SRC_FOR_IN) ? 1 : 0;
    int ifneOffset = js_GetSrcNoteOffset(sn, which);
    jsbytecode *ifne = pc + ifneOffset;
    JS_ASSERT(ifne > pc);

    
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_LOOPHEAD);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    jsbytecode *loopEntry = pc + GetJumpOffset(pc);
    if (info().hasOsrAt(loopEntry)) {
        MBasicBlock *preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(preheader));
        current = preheader;
    }

    MBasicBlock *header = newPendingLoopHeader(current, pc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(header));

    
    jsbytecode *bodyStart = GetNextPc(GetNextPc(pc));
    jsbytecode *bodyEnd = pc + GetJumpOffset(pc);
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!pushLoop(CFGState::WHILE_LOOP_COND, ifne, header, bodyStart, bodyEnd, exitpc))
        return ControlStatus_Error;

    
    current = header;
    if (!jsop_loophead(GetNextPc(pc)))
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

    if (info().hasOsrAt(loopEntry)) {
        MBasicBlock *preheader = newOsrPreheader(current, loopEntry);
        if (!preheader)
            return ControlStatus_Error;
        current->end(MGoto::New(preheader));
        current = preheader;
    }

    MBasicBlock *header = newPendingLoopHeader(current, pc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(header));

    
    
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

    if (!pushLoop(initial, stopAt, header, bodyStart, bodyEnd, exitpc, updatepc))
        return ControlStatus_Error;

    CFGState &state = cfgStack_.back();
    state.loop.condpc = (condpc != ifne) ? condpc : NULL;
    state.loop.updatepc = (updatepc != condpc) ? updatepc : NULL;
    if (state.loop.updatepc)
        state.loop.updateEnd = condpc;

    current = header;
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

    
    MTableSwitch *tableswitch = MTableSwitch::New(ins, low, high);

    
    MBasicBlock *defaultcase = newBlock(current, defaultpc);
    if (!defaultcase)
        return ControlStatus_Error;
    tableswitch->addDefault(defaultcase);
    tableswitch->addBlock(defaultcase);

    
    jsbytecode *casepc = NULL;
    for (int i = 0; i < high-low+1; i++) {
        casepc = pc + GET_JUMP_OFFSET(pc2);

        JS_ASSERT(casepc >= pc && casepc <= exitpc);

        MBasicBlock *caseblock = newBlock(current, casepc);
        if (!caseblock)
            return ControlStatus_Error;

        
        
        
        
        if (casepc == pc) {
            caseblock->end(MGoto::New(defaultcase));
            defaultcase->addPredecessor(caseblock);
        }

        tableswitch->addCase(caseblock);

        
        
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
    current = tableswitch->getBlock(0);

    if (!cfgStack_.append(state))
        return ControlStatus_Error;

    pc = current->pc();
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::lookupSwitch(JSOp op, jssrcnote *sn)
{
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT(op == JSOP_LOOKUPSWITCH);

    
    MDefinition *ins = current->pop();

    
    jsbytecode *exitpc = pc + js_GetSrcNoteOffset(sn, 0);
    jsbytecode *defaultpc = pc + GET_JUMP_OFFSET(pc);

    JS_ASSERT(defaultpc > pc && defaultpc <= exitpc);

    
    
    jsbytecode *pc2 = pc;
    pc2 += JUMP_OFFSET_LEN;
    unsigned int ncases = GET_UINT16(pc2);
    pc2 += UINT16_LEN;
    JS_ASSERT(ncases >= 1);

    
    Vector<MBasicBlock*, 0, IonAllocPolicy> bodyBlocks;

    MBasicBlock *defaultBody = NULL;
    unsigned int defaultIdx = UINT_MAX;
    bool defaultShared = false;

    MBasicBlock *prevCond = NULL;
    MCompare *prevCmpIns = NULL;
    MBasicBlock *prevBody = NULL;
    bool prevShared = false;
    jsbytecode *prevpc = NULL;
    for (unsigned int i = 0; i < ncases; i++) {
        Value rval = script_->getConst(GET_UINT32_INDEX(pc2));
        pc2 += UINT32_INDEX_LEN;
        jsbytecode *casepc = pc + GET_JUMP_OFFSET(pc2);
        pc2 += JUMP_OFFSET_LEN;
        JS_ASSERT(casepc > pc && casepc <= exitpc);
        JS_ASSERT_IF(i > 0, prevpc <= casepc);

        
        MBasicBlock *cond = newBlock(((i == 0) ? current : prevCond), casepc);
        if (!cond)
            return ControlStatus_Error;

        MConstant *rvalIns = MConstant::New(rval);
        cond->add(rvalIns);

        MCompare *cmpIns = MCompare::New(ins, rvalIns, JSOP_STRICTEQ);
        cond->add(cmpIns);
        if (cmpIns->isEffectful() && !resumeAfter(cmpIns))
            return ControlStatus_Error;

        
        MBasicBlock *body;
        if (prevpc == casepc) {
            body = prevBody;
        } else {
            body = newBlock(cond, casepc);
            if (!body)
                return ControlStatus_Error;
            bodyBlocks.append(body);
        }

        
        if (defaultpc <= casepc && defaultIdx == UINT_MAX) {
            defaultIdx = bodyBlocks.length() - 1;
            if (defaultpc == casepc) {
                defaultBody = body;
                defaultShared = true;
            }
        }

        
        
        if (i == 0) {
            
            current->end(MGoto::New(cond));
        } else {
            
            prevCond->end(MTest::New(prevCmpIns, prevBody, cond));

            
            
            
            if (prevShared)
                prevBody->addPredecessor(prevCond);
        }

        
        prevCond = cond;
        prevCmpIns = cmpIns;
        prevBody = body;
        prevShared = (prevpc == casepc);
        prevpc = casepc;
    }

    
    if (!defaultBody) {
        JS_ASSERT(!defaultShared);
        defaultBody = newBlock(prevCond, defaultpc);
        if (!defaultBody)
            return ControlStatus_Error;

        if (defaultIdx >= bodyBlocks.length())
            bodyBlocks.append(defaultBody);
        else
            bodyBlocks.insert(&bodyBlocks[defaultIdx], defaultBody);
    }

    
    if (defaultBody == prevBody) {
        
        
        prevCond->end(MGoto::New(defaultBody));
    } else {
        
        
        prevCond->end(MTest::New(prevCmpIns, prevBody, defaultBody));

        
        
        
        
        if (defaultShared)
            defaultBody->addPredecessor(prevCond);
    }

    
    
    if (prevShared)
        prevBody->addPredecessor(prevCond);

    
    CFGState state = CFGState::LookupSwitch(exitpc);
    if (!state.lookupswitch.bodies || !state.lookupswitch.bodies->init(bodyBlocks.length()))
        return ControlStatus_Error;

    
    
    for (size_t i = 0; i < bodyBlocks.length(); i++) {
        (*state.lookupswitch.bodies)[i] = bodyBlocks[i];
    }
    graph().moveBlockToEnd(bodyBlocks[0]);

    
    ControlFlowInfo switchinfo(cfgStack_.length(), exitpc);
    if (!switches_.append(switchinfo))
        return ControlStatus_Error;

    
    if (state.lookupswitch.bodies->length() > 1)
        state.stopAt = (*state.lookupswitch.bodies)[1]->pc();
    if (!cfgStack_.append(state))
        return ControlStatus_Error;

    current = (*state.lookupswitch.bodies)[0];
    pc = current->pc();
    return ControlStatus_Jumped;
}

bool
IonBuilder::jsop_condswitch()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JS_ASSERT(JSOp(*pc) == JSOP_CONDSWITCH);
    jssrcnote *sn = info().getNote(cx, pc);

    
    jsbytecode *exitpc = pc + js_GetSrcNoteOffset(sn, 0);
    jsbytecode *firstCase = pc + js_GetSrcNoteOffset(sn, 1);

    
    
    
    
    jsbytecode *curCase = firstCase;
    jsbytecode *lastTarget = GetJumpOffset(curCase) + curCase;
    size_t nbBodies = 2; 

    JS_ASSERT(pc < curCase && curCase <= exitpc);
    while (JSOp(*curCase) == JSOP_CASE) {
        
        jssrcnote *caseSn = info().getNote(cx, curCase);
        JS_ASSERT(caseSn && SN_TYPE(caseSn) == SRC_PCDELTA);
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

    
    CFGState state = CFGState::CondSwitch(exitpc, defaultTarget);
    if (!state.condswitch.bodies || !state.condswitch.bodies->init(nbBodies))
        return ControlStatus_Error;

    
    JS_ASSERT(JSOp(*firstCase) == JSOP_CASE);
    state.stopAt = firstCase;
    state.state = CFGState::COND_SWITCH_CASE;

    return cfgStack_.append(state);
}

IonBuilder::CFGState
IonBuilder::CFGState::CondSwitch(jsbytecode *exitpc, jsbytecode *defaultTarget)
{
    CFGState state;
    state.state = COND_SWITCH_CASE;
    state.stopAt = NULL;
    state.condswitch.bodies = (FixedList<MBasicBlock *> *)GetIonContext()->temp->allocate(
        sizeof(FixedList<MBasicBlock *>));
    state.condswitch.currentIdx = 0;
    state.condswitch.defaultTarget = defaultTarget;
    state.condswitch.defaultIdx = uint32_t(-1);
    state.condswitch.exitpc = exitpc;
    state.condswitch.breaks = NULL;
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
    jsbytecode *lastTarget = currentIdx ? bodies[currentIdx - 1]->pc() : NULL;

    
    jssrcnote *sn = info().getNote(cx, pc);
    ptrdiff_t off = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *casePc = off ? pc + off : GetNextPc(pc);
    bool caseIsDefault = JSOp(*casePc) == JSOP_DEFAULT;
    JS_ASSERT(JSOp(*casePc) == JSOP_CASE || caseIsDefault);

    
    bool bodyIsNew = false;
    MBasicBlock *bodyBlock = NULL;
    jsbytecode *bodyTarget = pc + GetJumpOffset(pc);
    if (lastTarget < bodyTarget) {
        
        if (lastTarget < defaultTarget && defaultTarget <= bodyTarget) {
            JS_ASSERT(state.condswitch.defaultIdx == uint32_t(-1));
            state.condswitch.defaultIdx = currentIdx;
            bodies[currentIdx] = NULL;
            
            
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
    MBasicBlock *caseBlock = NULL;
    if (!caseIsDefault) {
        caseIsNew = true;
        
        caseBlock = newBlockPopN(current, GetNextPc(pc), 1);
    } else {
        
        
        

        if (state.condswitch.defaultIdx == uint32_t(-1)) {
            
            JS_ASSERT(lastTarget < defaultTarget);
            state.condswitch.defaultIdx = currentIdx++;
            caseIsNew = true;
        } else if (bodies[state.condswitch.defaultIdx] == NULL) {
            
            
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
        MCompare *cmpResult = MCompare::New(switchOperand, caseOperand, JSOP_STRICTEQ);
        JS_ASSERT(!cmpResult->isEffectful());
        current->add(cmpResult);
        current->end(MTest::New(cmpResult, bodyBlock, caseBlock));

        
        
        if (!bodyIsNew && !bodyBlock->addPredecessorPopN(current, 1))
            return ControlStatus_Error;

        
        
        
        
        JS_ASSERT_IF(!caseIsNew, caseIsDefault);
        if (!caseIsNew && !caseBlock->addPredecessorPopN(current, 1))
            return ControlStatus_Error;
    } else {
        
        JS_ASSERT(caseIsDefault);
        current->pop(); 
        current->pop(); 
        current->end(MGoto::New(bodyBlock));
        if (!bodyIsNew && !bodyBlock->addPredecessor(current))
            return ControlStatus_Error;
    }

    if (caseIsDefault) {
        
        
        
        
        JS_ASSERT(currentIdx == bodies.length() || currentIdx + 1 == bodies.length());
        bodies.shrink(bodies.length() - currentIdx);

        
        
        ControlFlowInfo breakInfo(cfgStack_.length() - 1, state.condswitch.exitpc);
        if (!switches_.append(breakInfo))
            return ControlStatus_Error;

        
        currentIdx = 0;
        current = NULL;
        state.state = CFGState::COND_SWITCH_BODY;
        return processCondSwitchBody(state);
    }

    
    current = caseBlock;
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
        current->end(MGoto::New(nextBody));
        nextBody->addPredecessor(current);
    }

    
    current = nextBody;
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
    jsbytecode *rhsStart = pc + js_CodeSpec[op].length;
    jsbytecode *joinStart = pc + GetJumpOffset(pc);
    JS_ASSERT(joinStart > pc);

    
    MDefinition *lhs = current->peek(-1);

    MBasicBlock *evalRhs = newBlock(current, rhsStart);
    MBasicBlock *join = newBlock(current, joinStart);
    if (!evalRhs || !join)
        return false;

    if (op == JSOP_AND) {
        current->end(MTest::New(lhs, evalRhs, join));
    } else {
        JS_ASSERT(op == JSOP_OR);
        current->end(MTest::New(lhs, join, evalRhs));
    }

    if (!cfgStack_.append(CFGState::AndOr(joinStart, join)))
        return false;

    current = evalRhs;
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

    current->add(MInterruptCheck::New());

    return true;
}

bool
IonBuilder::jsop_ifeq(JSOp op)
{
    
    jsbytecode *trueStart = pc + js_CodeSpec[op].length;
    jsbytecode *falseStart = pc + GetJumpOffset(pc);
    JS_ASSERT(falseStart > pc);

    
    jssrcnote *sn = info().getNote(cx, pc);
    if (!sn)
        return abort("expected sourcenote");

    MDefinition *ins = current->pop();

    
    MBasicBlock *ifTrue = newBlock(current, trueStart);
    MBasicBlock *ifFalse = newBlock(current, falseStart);
    if (!ifTrue || !ifFalse)
        return false;

    current->end(MTest::New(ins, ifTrue, ifFalse));

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
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
        JS_ASSERT(!info().getNote(cx, trueEnd));

        jsbytecode *falseEnd = trueEnd + GetJumpOffset(trueEnd);
        JS_ASSERT(falseEnd > trueEnd);
        JS_ASSERT(falseEnd >= falseStart);

        if (!cfgStack_.append(CFGState::IfElse(trueEnd, falseEnd, ifFalse)))
            return false;
        break;
      }

      default:
        JS_NOT_REACHED("unexpected source note type");
        break;
    }

    
    
    current = ifTrue;

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

      case JSOP_STOP:
      {
        MInstruction *ins = MConstant::New(UndefinedValue());
        current->add(ins);
        def = ins;
        break;
      }

      default:
        def = NULL;
        JS_NOT_REACHED("unknown return op");
        break;
    }

    if (instrumentedProfiling())
        current->add(MFunctionBoundary::New(script_, MFunctionBoundary::Exit));
    MReturn *ret = MReturn::New(def);
    current->end(ret);

    if (!graph().addExit(current))
        return ControlStatus_Error;

    
    current = NULL;
    return processControlEnd();
}

IonBuilder::ControlStatus
IonBuilder::processThrow()
{
    MDefinition *def = current->pop();

    MThrow *ins = MThrow::New(def);
    current->end(ins);

    if (!graph().addExit(current))
        return ControlStatus_Error;

    
    current = NULL;
    return processControlEnd();
}

bool
IonBuilder::pushConstant(const Value &v)
{
    MConstant *ins = MConstant::New(v);
    current->add(ins);
    current->push(ins);
    return true;
}

bool
IonBuilder::jsop_bitnot()
{
    MDefinition *input = current->pop();
    MBitNot *ins = MBitNot::New(input);

    current->add(ins);
    ins->infer(oracle->unaryTypes(script_, pc));

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
        ins = MBitAnd::New(left, right);
        break;

      case JSOP_BITOR:
        ins = MBitOr::New(left, right);
        break;

      case JSOP_BITXOR:
        ins = MBitXor::New(left, right);
        break;

      case JSOP_LSH:
        ins = MLsh::New(left, right);
        break;

      case JSOP_RSH:
        ins = MRsh::New(left, right);
        break;

      case JSOP_URSH:
        ins = MUrsh::New(left, right);
        break;

      default:
        JS_NOT_REACHED("unexpected bitop");
        return false;
    }

    current->add(ins);
    ins->infer(oracle->binaryTypes(script_, pc));

    current->push(ins);
    if (ins->isEffectful() && !resumeAfter(ins))
        return false;

    return true;
}

bool
IonBuilder::jsop_binary(JSOp op, MDefinition *left, MDefinition *right)
{
    TypeOracle::Binary b = oracle->binaryOp(script_, pc);

    if (op == JSOP_ADD && b.rval == MIRType_String &&
        (b.lhs == MIRType_String || b.lhs == MIRType_Int32) &&
        (b.rhs == MIRType_String || b.rhs == MIRType_Int32))
    {
        MConcat *ins = MConcat::New(left, right);
        current->add(ins);
        current->push(ins);
        return maybeInsertResume();
    }

    MBinaryArithInstruction *ins;
    switch (op) {
      case JSOP_ADD:
        ins = MAdd::New(left, right);
        break;

      case JSOP_SUB:
        ins = MSub::New(left, right);
        break;

      case JSOP_MUL:
        ins = MMul::New(left, right);
        break;

      case JSOP_DIV:
        ins = MDiv::New(left, right);
        break;

      case JSOP_MOD:
        ins = MMod::New(left, right);
        break;

      default:
        JS_NOT_REACHED("unexpected binary opcode");
        return false;
    }

    TypeOracle::BinaryTypes types = oracle->binaryTypes(script_, pc);
    current->add(ins);
    ins->infer(cx, types);
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
    TypeOracle::Unary types = oracle->unaryOp(script_, pc);
    if (IsNumberType(types.ival)) {
        
        JS_ASSERT(IsNumberType(types.rval));
        return true;
    }

    
    MDefinition *value = current->pop();
    MConstant *one = MConstant::New(Int32Value(1));
    current->add(one);

    return jsop_binary(JSOP_MUL, value, one);
}

bool
IonBuilder::jsop_neg()
{
    
    
    MConstant *negator = MConstant::New(Int32Value(-1));
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
    MPassArg *arg = MPassArg::New(def);

    current->add(arg);
    current->push(arg);
    return true;
}

class AutoAccumulateExits
{
    MIRGraph &graph_;
    MIRGraphExits *prev_;

  public:
    AutoAccumulateExits(MIRGraph &graph, MIRGraphExits &exits) : graph_(graph) {
        prev_ = graph_.exitAccumulator();
        graph_.setExitAccumulator(&exits);
    }
    ~AutoAccumulateExits() {
        graph_.setExitAccumulator(prev_);
    }
};


bool
IonBuilder::jsop_call_inline(HandleFunction callee, uint32_t argc, bool constructing,
                             MConstant *constFun, MBasicBlock *bottom,
                             Vector<MDefinition *, 8, IonAllocPolicy> &retvalDefns)
{
    AssertCanGC();

    
    
    current->rewriteAtDepth(-((int) argc + 2), constFun);

    
    
    MResumePoint *inlineResumePoint =
        MResumePoint::New(current, pc, callerResumePoint_, MResumePoint::Outer);
    if (!inlineResumePoint)
        return false;

    
    JS_ASSERT(argc == GET_ARGC(inlineResumePoint->pc()));

    
    
    
    MDefinitionVector argv;
    if (!argv.resizeUninitialized(argc + 1))
        return false;
    for (int32_t i = argc; i >= 0; i--)
        argv[i] = current->pop();

    
    
    RootedScript calleeScript(cx, callee->nonLazyScript());
    CompileInfo *info = cx->tempLifoAlloc().new_<CompileInfo>(calleeScript.get(), callee,
                                                              (jsbytecode *)NULL, constructing,
                                                              SequentialExecution);
    if (!info)
        return false;

    MIRGraphExits saveExits;
    AutoAccumulateExits aae(graph(), saveExits);

    TypeInferenceOracle oracle;
    if (!oracle.init(cx, calleeScript))
        return false;

    IonBuilder inlineBuilder(cx, &temp(), &graph(), &oracle,
                             info, inliningDepth + 1, loopDepth_);

    
    MDefinition *thisDefn = NULL;
    if (constructing) {
        thisDefn = createThis(callee, constFun);
        if (!thisDefn)
            return false;
    } else {
        thisDefn = argv[0];
    }

    
    if (!inlineBuilder.buildInline(this, inlineResumePoint, thisDefn, argv))
        return false;

    MIRGraphExits &exits = *inlineBuilder.graph().exitAccumulator();

    
    
    for (MBasicBlock **it = exits.begin(), **end = exits.end(); it != end; ++it) {
        MBasicBlock *exitBlock = *it;

        MDefinition *rval = exitBlock->lastIns()->toReturn()->getOperand(0);
        exitBlock->discardLastIns();

        
        if (constructing) {
            if (rval->type() == MIRType_Value) {
                MReturnFromCtor *filter = MReturnFromCtor::New(rval, thisDefn);
                exitBlock->add(filter);
                rval = filter;
            } else if (rval->type() != MIRType_Object) {
                rval = thisDefn;
            }
        }

        if (!retvalDefns.append(rval))
            return false;

        MGoto *replacement = MGoto::New(bottom);
        exitBlock->end(replacement);
        if (!bottom->addPredecessorWithoutPhis(exitBlock))
            return false;
    }
    JS_ASSERT(!retvalDefns.empty());
    return true;
}

bool
IonBuilder::makeInliningDecision(AutoObjectVector &targets, uint32_t argc)
{
    AssertCanGC();

    if (inliningDepth >= js_IonOptions.maxInlineDepth)
        return false;

    
    
    
    
    
    
    
    
    
    

    uint32_t callerUses = script_->getUseCount();

    uint32_t totalSize = 0;
    uint32_t checkUses = js_IonOptions.usesBeforeInlining;
    bool allFunctionsAreSmall = true;
    RootedFunction target(cx);
    RootedScript script(cx);
    for (size_t i = 0; i < targets.length(); i++) {
        target = targets[i]->toFunction();
        if (!target->isInterpreted())
            return false;

        script = target->nonLazyScript();
        uint32_t calleeUses = script->getUseCount();

        if (target->nargs < argc) {
            IonSpew(IonSpew_Inlining, "Not inlining, overflow of arguments.");
            return false;
        }

        totalSize += script->length;
        if (totalSize > js_IonOptions.inlineMaxTotalBytecodeLength)
            return false;

        if (script->length > js_IonOptions.smallFunctionMaxBytecodeLength)
            allFunctionsAreSmall = false;

        if (calleeUses * js_IonOptions.inlineUseCountRatio < callerUses) {
            IonSpew(IonSpew_Inlining, "Not inlining, callee is not hot");
            return false;
        }
    }
    if (allFunctionsAreSmall)
        checkUses = js_IonOptions.smallFunctionUsesBeforeInlining;

    if (script_->getUseCount() < checkUses) {
        IonSpew(IonSpew_Inlining, "Not inlining, caller is not hot");
        return false;
    }

    if (!oracle->canInlineCall(script_, pc)) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to uninlineable call site");
        return false;
    }

    for (size_t i = 0; i < targets.length(); i++) {
        if (!canInlineTarget(targets[i]->toFunction())) {
            IonSpew(IonSpew_Inlining, "Decided not to inline");
            return false;
        }
    }

    return true;
}

static bool
ValidateInlineableGetPropertyCache(MGetPropertyCache *getPropCache, MDefinition *thisDefn,
                                   size_t maxUseCount)
{
    JS_ASSERT(getPropCache->object()->type() == MIRType_Object);

    if (getPropCache->useCount() > maxUseCount)
        return false;

    
    if (getPropCache->object() != thisDefn)
        return false;

    InlinePropertyTable *propTable = getPropCache->inlinePropertyTable();
    if (!propTable || propTable->numEntries() == 0)
        return false;

    return true;
}

MGetPropertyCache *
IonBuilder::checkInlineableGetPropertyCache(uint32_t argc)
{
    
    
    

    JS_ASSERT(current->stackDepth() >= argc + 2);

    
    int thisDefnDepth = -((int) argc + 1);
    MDefinition *thisDefn = current->peek(thisDefnDepth);
    if (thisDefn->type() != MIRType_Object)
        return NULL;

    
    
    int funcDefnDepth = -((int) argc + 2);
    MDefinition *funcDefn = current->peek(funcDefnDepth);
    if (funcDefn->type() != MIRType_Object)
        return NULL;

    
    
    if (funcDefn->isConstant())
        return NULL;

    
    
    

    
    
    if (funcDefn->isGetPropertyCache()) {
        MGetPropertyCache *getPropCache = funcDefn->toGetPropertyCache();
        if (!ValidateInlineableGetPropertyCache(getPropCache, thisDefn, 0))
            return NULL;

        return getPropCache;
    }

    
    if (!funcDefn->isUnbox() || funcDefn->toUnbox()->useCount() > 0)
        return NULL;

    MUnbox *unbox = current->peek(funcDefnDepth)->toUnbox();
    if (unbox->mode() != MUnbox::Infallible || !unbox->input()->isTypeBarrier())
        return NULL;

    MTypeBarrier *typeBarrier = unbox->input()->toTypeBarrier();
    if (typeBarrier->useCount() != 1 || !typeBarrier->input()->isGetPropertyCache())
        return NULL;

    MGetPropertyCache *getPropCache = typeBarrier->input()->toGetPropertyCache();
    JS_ASSERT(getPropCache->object()->type() == MIRType_Object);

    if (!ValidateInlineableGetPropertyCache(getPropCache, thisDefn, 1))
        return NULL;

    return getPropCache;
}

MPolyInlineDispatch *
IonBuilder::makePolyInlineDispatch(JSContext *cx, AutoObjectVector &targets, int argc,
                                   MGetPropertyCache *getPropCache,
                                   types::StackTypeSet *types, types::StackTypeSet *barrier,
                                   MBasicBlock *bottom,
                                   Vector<MDefinition *, 8, IonAllocPolicy> &retvalDefns)
{
    int funcDefnDepth = -((int) argc + 2);
    MDefinition *funcDefn = current->peek(funcDefnDepth);

    
    if (!getPropCache)
        return MPolyInlineDispatch::New(funcDefn);

    InlinePropertyTable *inlinePropTable = getPropCache->inlinePropertyTable();

    
    
    MResumePoint *preCallResumePoint = MResumePoint::New(current, pc, callerResumePoint_,
                                                         MResumePoint::ResumeAt);
    if (!preCallResumePoint)
        return NULL;
    DebugOnly<size_t> preCallFuncDefnIdx = preCallResumePoint->numOperands() - (((size_t) argc) + 2);
    JS_ASSERT(preCallResumePoint->getOperand(preCallFuncDefnIdx) == funcDefn);

    MDefinition *targetObject = getPropCache->object();

    
    
    
    
    
    
    
    
    

    
    
    MConstant *undef = MConstant::New(UndefinedValue());
    current->add(undef);
    current->rewriteAtDepth(funcDefnDepth, undef);

    
    
    MBasicBlock *fallbackPrepBlock = newBlock(current, pc);
    if (!fallbackPrepBlock)
        return NULL;

    for (int i = argc + 1; i >= 0; i--)
        (void) fallbackPrepBlock->pop();

    
    
    JS_ASSERT(inlinePropTable->pc() != NULL);
    JS_ASSERT(inlinePropTable->priorResumePoint() != NULL);
    MBasicBlock *fallbackBlock = newBlock(fallbackPrepBlock, inlinePropTable->pc(),
                                          inlinePropTable->priorResumePoint());
    if (!fallbackBlock)
        return NULL;

    fallbackPrepBlock->end(MGoto::New(fallbackBlock));

    
    
    DebugOnly<MDefinition *> checkTargetObject = fallbackBlock->pop();
    JS_ASSERT(checkTargetObject == targetObject);

    
    
    if (funcDefn->isGetPropertyCache()) {
        JS_ASSERT(funcDefn->toGetPropertyCache() == getPropCache);
        fallbackBlock->addFromElsewhere(getPropCache);
        fallbackBlock->push(getPropCache);
    } else {
        JS_ASSERT(funcDefn->isUnbox());
        MUnbox *unbox = funcDefn->toUnbox();
        JS_ASSERT(unbox->input()->isTypeBarrier());
        JS_ASSERT(unbox->type() == MIRType_Object);
        JS_ASSERT(unbox->mode() == MUnbox::Infallible);

        MTypeBarrier *typeBarrier = unbox->input()->toTypeBarrier();
        JS_ASSERT(typeBarrier->input()->isGetPropertyCache());
        JS_ASSERT(typeBarrier->input()->toGetPropertyCache() == getPropCache);

        fallbackBlock->addFromElsewhere(getPropCache);
        fallbackBlock->addFromElsewhere(typeBarrier);
        fallbackBlock->addFromElsewhere(unbox);
        fallbackBlock->push(unbox);
    }

    
    
    MBasicBlock *fallbackEndBlock = newBlock(fallbackBlock, pc, preCallResumePoint);
    if (!fallbackEndBlock)
        return NULL;
    fallbackBlock->end(MGoto::New(fallbackEndBlock));

    
    MCall *call = MCall::New(NULL, argc + 1, argc, false);
    if (!call)
        return NULL;

    
    MPrepareCall *prepCall = new MPrepareCall;
    fallbackEndBlock->add(prepCall);

    
    for (int32_t i = 0; i <= argc; i++) {
        int32_t argno = argc - i;
        MDefinition *argDefn = fallbackEndBlock->pop();
        JS_ASSERT(!argDefn->isPassArg());
        MPassArg *passArg = MPassArg::New(argDefn);
        fallbackEndBlock->add(passArg);
        call->addArg(argno, passArg);
    }

    
    call->initPrepareCall(prepCall);

    
    call->initFunction(fallbackEndBlock->pop());

    fallbackEndBlock->add(call);
    fallbackEndBlock->push(call);
    if (!resumeAfter(call))
        return NULL;

    MBasicBlock *top = current;
    current = fallbackEndBlock;
    if (!pushTypeBarrier(call, types, barrier))
        return NULL;
    current = top;

    
    return MPolyInlineDispatch::New(targetObject, inlinePropTable,
                                    fallbackPrepBlock, fallbackBlock,
                                    fallbackEndBlock);
}

bool
IonBuilder::inlineScriptedCall(AutoObjectVector &targets, uint32_t argc, bool constructing,
                               types::StackTypeSet *types, types::StackTypeSet *barrier)
{
#ifdef DEBUG
    uint32_t origStackDepth = current->stackDepth();
#endif

    IonSpew(IonSpew_Inlining, "Inlining %d targets", (int) targets.length());
    JS_ASSERT(targets.length() > 0);

    
    MBasicBlock *top = current;

    
    
    for (int32_t i = argc; i >= 0; i--) {
        
        int argSlotDepth = -((int) i + 1);
        MPassArg *passArg = top->peek(argSlotDepth)->toPassArg();
        MBasicBlock *block = passArg->block();
        MDefinition *wrapped = passArg->getArgument();
        passArg->replaceAllUsesWith(wrapped);
        top->rewriteAtDepth(argSlotDepth, wrapped);
        block->discard(passArg);
    }

    
    
    MGetPropertyCache *getPropCache = NULL;
    if (!constructing) {
        getPropCache = checkInlineableGetPropertyCache(argc);
        if(getPropCache) {
            InlinePropertyTable *inlinePropTable = getPropCache->inlinePropertyTable();
            
            JS_ASSERT(inlinePropTable != NULL);

            int numCases = inlinePropTable->numEntries();
            IonSpew(IonSpew_Inlining, "Got inlineable property cache with %d cases", numCases);

            inlinePropTable->trimToTargets(targets);

            
            IonSpew(IonSpew_Inlining, "%d inlineable cases left after trimming to %d targets",
                                        (int) inlinePropTable->numEntries(),
                                        (int) targets.length());

            if (inlinePropTable->numEntries() == 0)
                getPropCache = NULL;
        }
    }

    
    JS_ASSERT(types::IsInlinableCall(pc));
    jsbytecode *postCall = GetNextPc(pc);
    MBasicBlock *bottom = newBlock(NULL, postCall);
    if (!bottom)
        return false;
    bottom->setCallerResumePoint(callerResumePoint_);

    Vector<MDefinition *, 8, IonAllocPolicy> retvalDefns;

    
    
    
    
    
    
    
    
    
    if (getPropCache == NULL && targets.length() == 1) {
        JSFunction *func = targets[0]->toFunction();
        MConstant *constFun = MConstant::New(ObjectValue(*func));
        current->add(constFun);

        
        RootedFunction target(cx, func);
        if (!jsop_call_inline(target, argc, constructing, constFun, bottom, retvalDefns))
            return false;

        
        
        if (instrumentedProfiling())
            bottom->add(MFunctionBoundary::New(NULL, MFunctionBoundary::Inline_Exit));

    } else {
        

        
        MPolyInlineDispatch *disp = makePolyInlineDispatch(cx, targets, argc, getPropCache,
                                                           types, barrier, bottom, retvalDefns);
        if (!disp)
            return false;
        for (size_t i = 0; i < targets.length(); i++) {
            
            JSFunction *func = targets[i]->toFunction();
            RootedFunction target(cx, func);
            MConstant *constFun = MConstant::New(ObjectValue(*func));

            
            MBasicBlock *entryBlock = newBlock(current, pc);
            if (!entryBlock)
                return false;

            
            entryBlock->add(constFun);
            disp->addCallee(constFun, entryBlock);
        }
        top->end(disp);

        
        
        
        
        
        
        
        MBasicBlock *inlineBottom = bottom;
        if (instrumentedProfiling() && disp->inlinePropertyTable()) {
            inlineBottom = newBlock(NULL, pc);
            if (inlineBottom == NULL)
                return false;
        }

        for (size_t i = 0; i < disp->numCallees(); i++) {
            
            MConstant *constFun = disp->getFunctionConstant(i);
            RootedFunction target(cx, constFun->value().toObject().toFunction());
            MBasicBlock *block = disp->getSuccessor(i);
            graph().moveBlockToEnd(block);
            current = block;

            if (!jsop_call_inline(target, argc, constructing, constFun, inlineBottom, retvalDefns))
                return false;
        }

        
        
        
        if (instrumentedProfiling())
            inlineBottom->add(MFunctionBoundary::New(NULL, MFunctionBoundary::Inline_Exit));

        
        
        
        
        if (inlineBottom != bottom) {
            graph().moveBlockToEnd(inlineBottom);
            inlineBottom->inheritSlots(top);
            if (!inlineBottom->initEntrySlots())
                return false;

            
            if (retvalDefns.length() > 1) {
                
                
                MPhi *phi = MPhi::New(inlineBottom->stackDepth() - argc - 2);
                inlineBottom->addPhi(phi);

                MDefinition **it = retvalDefns.begin(), **end = retvalDefns.end();
                for (; it != end; ++it) {
                    if (!phi->addInput(*it))
                        return false;
                }
                
                retvalDefns.clear();
                if (!retvalDefns.append(phi))
                    return false;
            }

            inlineBottom->end(MGoto::New(bottom));
            if (!bottom->addPredecessorWithoutPhis(inlineBottom))
                return false;
        }

        
        
        
        if (disp->inlinePropertyTable()) {
            graph().moveBlockToEnd(disp->fallbackPrepBlock());
            graph().moveBlockToEnd(disp->fallbackMidBlock());
            graph().moveBlockToEnd(disp->fallbackEndBlock());

            
            MBasicBlock *fallbackEndBlock = disp->fallbackEndBlock();
            MDefinition *fallbackResult = fallbackEndBlock->pop();
            if(!retvalDefns.append(fallbackResult))
                return false;
            fallbackEndBlock->end(MGoto::New(bottom));
            if (!bottom->addPredecessorWithoutPhis(fallbackEndBlock))
                return false;
        }
    }

    graph().moveBlockToEnd(bottom);

    bottom->inheritSlots(top);

    
    
    
    if (getPropCache || targets.length() > 1) {
        for (uint32_t i = 0; i < argc + 1; i++)
            bottom->pop();
    }

    
    bottom->pop();

    MDefinition *retvalDefn;
    if (retvalDefns.length() > 1) {
        
        MPhi *phi = MPhi::New(bottom->stackDepth());
        bottom->addPhi(phi);

        for (MDefinition **it = retvalDefns.begin(), **end = retvalDefns.end(); it != end; ++it) {
            if (!phi->addInput(*it))
                return false;
        }
        retvalDefn = phi;
    } else {
        retvalDefn = retvalDefns.back();
    }

    bottom->push(retvalDefn);

    
    if (!bottom->initEntrySlots())
        return false;

    
    
    
    
    

    if (getPropCache || targets.length() > 1) {
        MBasicBlock *bottom2 = newBlock(bottom, postCall);
        if (!bottom2)
            return false;

        bottom->end(MGoto::New(bottom2));
        current = bottom2;
    } else {
        current = bottom;
    }

    
    
    
    
    JS_ASSERT(current->stackDepth() == origStackDepth - argc - 1);

    return true;
}

MInstruction *
IonBuilder::createDeclEnvObject(MDefinition *callee, MDefinition *scope)
{
    
    

    RootedScript script(cx, script_);
    RootedFunction fun(cx, info().fun());
    RootedObject templateObj(cx, DeclEnvObject::createTemplateObject(cx, fun));
    if (!templateObj)
        return NULL;

    
    
    templateObj->setFixedSlot(DeclEnvObject::enclosingScopeSlot(), MagicValue(JS_GENERIC_MAGIC));
    templateObj->setFixedSlot(DeclEnvObject::lambdaSlot(), MagicValue(JS_GENERIC_MAGIC));

    
    
    JS_ASSERT(!templateObj->hasDynamicSlots());

    
    
    
    MInstruction *declEnvObj = MNewDeclEnvObject::New(templateObj);
    current->add(declEnvObj);

    
    current->add(MStoreFixedSlot::New(declEnvObj, DeclEnvObject::enclosingScopeSlot(), scope));
    current->add(MStoreFixedSlot::New(declEnvObj, DeclEnvObject::lambdaSlot(), callee));

    return declEnvObj;
}

MInstruction *
IonBuilder::createCallObject(MDefinition *callee, MDefinition *scope)
{
    
    

    RootedObject templateObj(cx, CallObject::createTemplateObject(cx, script_));
    if (!templateObj)
        return NULL;

    
    MInstruction *slots;
    if (templateObj->hasDynamicSlots()) {
        size_t nslots = JSObject::dynamicSlotsCount(templateObj->numFixedSlots(),
                                                    templateObj->slotSpan());
        slots = MNewSlots::New(nslots);
    } else {
        slots = MConstant::New(NullValue());
    }
    current->add(slots);

    
    
    
    MInstruction *callObj = MNewCallObject::New(templateObj, slots);
    current->add(callObj);

    
    current->add(MStoreFixedSlot::New(callObj, CallObject::enclosingScopeSlot(), scope));
    current->add(MStoreFixedSlot::New(callObj, CallObject::calleeSlot(), callee));

    
    for (AliasedFormalIter i(script_); i; i++) {
        unsigned slot = i.scopeSlot();
        unsigned formal = i.frameIndex();
        MDefinition *param = current->getSlot(info().argSlot(formal));
        if (slot >= templateObj->numFixedSlots())
            current->add(MStoreSlot::New(slots, slot - templateObj->numFixedSlots(), param));
        else
            current->add(MStoreFixedSlot::New(callObj, slot, param));
    }

    return callObj;
}

MDefinition *
IonBuilder::createThisNative()
{
    
    MConstant *magic = MConstant::New(MagicValue(JS_IS_CONSTRUCTING));
    current->add(magic);
    return magic;
}

MDefinition *
IonBuilder::createThisScripted(MDefinition *callee)
{
    
    
    
    
    
    
    
    
    
    
    
    MInstruction *getProto;
    if (!invalidatedIdempotentCache()) {
        MGetPropertyCache *getPropCache = MGetPropertyCache::New(callee, cx->names().classPrototype);
        getPropCache->setIdempotent();
        getProto = getPropCache;
    } else {
        MCallGetProperty *callGetProp = MCallGetProperty::New(callee, cx->names().classPrototype);
        callGetProp->setIdempotent();
        getProto = callGetProp;
    }
    current->add(getProto);

    
    MCreateThis *createThis = MCreateThis::New(callee, getProto);
    current->add(createThis);

    return createThis;
}

JSObject *
IonBuilder::getSingletonPrototype(JSFunction *target)
{
    if (!target->hasSingletonType())
        return NULL;
    if (target->getType(cx)->unknownProperties())
        return NULL;

    jsid protoid = NameToId(cx->names().classPrototype);
    types::HeapTypeSet *protoTypes = target->getType(cx)->getProperty(cx, protoid, false);
    if (!protoTypes)
        return NULL;

    return protoTypes->getSingleton(cx);
}

MDefinition *
IonBuilder::createThisScriptedSingleton(HandleFunction target, HandleObject proto, MDefinition *callee)
{
    
    
    types::TypeObject *type = proto->getNewType(cx, target);
    if (!type)
        return NULL;
    if (!types::TypeScript::ThisTypes(target->nonLazyScript())->hasType(types::Type::ObjectType(type)))
        return NULL;

    RootedObject templateObject(cx, js_CreateThisForFunctionWithProto(cx, target, proto));
    if (!templateObject)
        return NULL;

    
    if (templateObject->type()->newScript)
        types::HeapTypeSet::WatchObjectStateChange(cx, templateObject->type());

    MCreateThisWithTemplate *createThis = MCreateThisWithTemplate::New(templateObject);
    current->add(createThis);

    return createThis;
}

MDefinition *
IonBuilder::createThis(HandleFunction target, MDefinition *callee)
{
    
    if (!target)
        return createThisScripted(callee);

    
    if (target->isNative()) {
        if (!target->isNativeConstructor())
            return NULL;
        return createThisNative();
    }

    
    RootedObject proto(cx, getSingletonPrototype(target));

    
    if (proto) {
        MDefinition *createThis = createThisScriptedSingleton(target, proto, callee);
        if (createThis)
            return createThis;
    }

    MDefinition *createThis = createThisScripted(callee);
    if (!createThis)
        return NULL;

    
    
    JS_ASSERT(createThis->isCreateThis());
    createThis->toCreateThis()->removeNativeCheck();

    return createThis;
}

bool
IonBuilder::jsop_funcall(uint32_t argc)
{
    
    
    
    
    
    

    
    RootedFunction native(cx, getSingleCallTarget(argc, pc));
    if (!native || !native->isNative() || native->native() != &js_fun_call)
        return makeCall(native, argc, false);

    
    types::StackTypeSet *funTypes = oracle->getCallArg(script_, argc, 0, pc);
    RootedObject funobj(cx, (funTypes) ? funTypes->getSingleton() : NULL);
    RootedFunction target(cx, (funobj && funobj->isFunction()) ? funobj->toFunction() : NULL);

    
    int funcDepth = -((int)argc + 1);
    MPassArg *passFunc = current->peek(funcDepth)->toPassArg();
    current->rewriteAtDepth(funcDepth, passFunc->getArgument());

    
    passFunc->replaceAllUsesWith(passFunc->getArgument());
    passFunc->block()->discard(passFunc);

    
    current->shimmySlots(funcDepth - 1);

    
    
    if (argc == 0) {
        MConstant *undef = MConstant::New(UndefinedValue());
        current->add(undef);
        MPassArg *pass = MPassArg::New(undef);
        current->add(pass);
        current->push(pass);
    } else {
        
        argc -= 1; 
    }

    
    return makeCall(target, argc, false);
}

bool
IonBuilder::jsop_funapply(uint32_t argc)
{
    RootedFunction native(cx, getSingleCallTarget(argc, pc));
    if (argc != 2)
        return makeCall(native, argc, false);

    
    
    types::StackTypeSet *argObjTypes = oracle->getCallArg(script_, argc, 2, pc);
    LazyArgumentsType isArgObj = oracle->isArgumentObject(argObjTypes);
    if (isArgObj == MaybeArguments)
        return abort("fun.apply with MaybeArguments");

    
    if (isArgObj != DefinitelyArguments)
        return makeCall(native, argc, false);

    if (!native ||
        !native->isNative() ||
        native->native() != js_fun_apply)
    {
        return abort("fun.apply speculation failed");
    }

    
    
    
    
    

    
    types::StackTypeSet *funTypes = oracle->getCallArg(script_, argc, 0, pc);
    RootedObject funobj(cx, (funTypes) ? funTypes->getSingleton() : NULL);
    RootedFunction target(cx, (funobj && funobj->isFunction()) ? funobj->toFunction() : NULL);

    
    MPassArg *passVp = current->pop()->toPassArg();
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

    MArgumentsLength *numArgs = MArgumentsLength::New();
    current->add(numArgs);

    MApplyArgs *apply = MApplyArgs::New(target, argFunc, numArgs, argThis);
    current->add(apply);
    current->push(apply);
    if (!resumeAfter(apply))
        return false;

    types::StackTypeSet *barrier;
    types::StackTypeSet *types = oracle->returnTypeSet(script_, pc, &barrier);
    return pushTypeBarrier(apply, types, barrier);
}

bool
IonBuilder::jsop_call(uint32_t argc, bool constructing)
{
    AssertCanGC();

    
    AutoObjectVector targets(cx);
    uint32_t numTargets = getPolyCallTargets(argc, pc, targets, 4);
    types::StackTypeSet *barrier;
    types::StackTypeSet *types = oracle->returnTypeSet(script_, pc, &barrier);

    
    if (inliningEnabled()) {
        
        if (numTargets == 1 && targets[0]->toFunction()->isNative()) {
            RootedFunction target(cx, targets[0]->toFunction());
            switch (inlineNativeCall(target->native(), argc, constructing)) {
              case InliningStatus_Inlined:
                return true;
              case InliningStatus_Error:
                return false;
              case InliningStatus_NotInlined:
                break;
            }
        }

        if (numTargets > 0 && makeInliningDecision(targets, argc))
            return inlineScriptedCall(targets, argc, constructing, types, barrier);
    }

    RootedFunction target(cx, NULL);
    if (numTargets == 1)
        target = targets[0]->toFunction();

    return makeCallBarrier(target, argc, constructing, types, barrier);
}

MCall *
IonBuilder::makeCallHelper(HandleFunction target, uint32_t argc, bool constructing)
{
    
    

    uint32_t targetArgs = argc;

    
    
    if (target && !target->isNative())
        targetArgs = Max<uint32_t>(target->nargs, argc);

    MCall *call = MCall::New(target, targetArgs + 1, argc, constructing);
    if (!call)
        return NULL;

    
    
    for (int i = targetArgs; i > (int)argc; i--) {
        JS_ASSERT_IF(target, !target->isNative());
        MConstant *undef = MConstant::New(UndefinedValue());
        current->add(undef);
        MPassArg *pass = MPassArg::New(undef);
        current->add(pass);
        call->addArg(i, pass);
    }

    
    
    for (int32_t i = argc; i > 0; i--)
        call->addArg(i, current->pop()->toPassArg());

    
    
    MPrepareCall *start = new MPrepareCall;
    MPassArg *firstArg = current->peek(-1)->toPassArg();
    firstArg->block()->insertBefore(firstArg, start);
    call->initPrepareCall(start);

    MPassArg *thisArg = current->pop()->toPassArg();

    
    if (constructing) {
        MDefinition *callee = current->peek(-1);
        MDefinition *create = createThis(target, callee);
        if (!create) {
            abort("Failure inlining constructor for call.");
            return NULL;
        }

        MPassArg *newThis = MPassArg::New(create);

        thisArg->block()->discard(thisArg);
        current->add(newThis);
        thisArg = newThis;
    }

    
    call->addArg(0, thisArg);

    MDefinition *fun = current->pop();
    if (fun->isDOMFunction())
        call->setDOMFunction();
    call->initFunction(fun);

    current->add(call);
    return call;
}

bool
IonBuilder::makeCallBarrier(HandleFunction target, uint32_t argc,
                            bool constructing,
                            types::StackTypeSet *types,
                            types::StackTypeSet *barrier)
{
    MCall *call = makeCallHelper(target, argc, constructing);
    if (!call)
        return false;

    current->push(call);
    if (!resumeAfter(call))
        return false;

    return pushTypeBarrier(call, types, barrier);
}

bool
IonBuilder::makeCall(HandleFunction target, uint32_t argc, bool constructing)
{
    types::StackTypeSet *barrier;
    types::StackTypeSet *types = oracle->returnTypeSet(script_, pc, &barrier);
    return makeCallBarrier(target, argc, constructing, types, barrier);
}

bool
IonBuilder::jsop_compare(JSOp op)
{
    MDefinition *right = current->pop();
    MDefinition *left = current->pop();

    MCompare *ins = MCompare::New(left, right, op);
    current->add(ins);
    current->push(ins);

    ins->infer(cx, oracle->binaryTypes(script_, pc));

    if (ins->isEffectful() && !resumeAfter(ins))
        return false;
    return true;
}

JSObject *
IonBuilder::getNewArrayTemplateObject(uint32_t count)
{
    RootedObject templateObject(cx, NewDenseUnallocatedArray(cx, count));
    if (!templateObject)
        return NULL;

    RootedScript script(cx, script_);
    if (types::UseNewTypeForInitializer(cx, script, pc, JSProto_Array)) {
        if (!JSObject::setSingletonType(cx, templateObject))
            return NULL;
    } else {
        types::TypeObject *type = types::TypeScript::InitObject(cx, script, pc, JSProto_Array);
        if (!type)
            return NULL;
        templateObject->setType(type);
    }

    return templateObject;
}

bool
IonBuilder::jsop_newarray(uint32_t count)
{
    JS_ASSERT(script_->compileAndGo);

    JSObject *templateObject = getNewArrayTemplateObject(count);
    if (!templateObject)
        return false;

    MNewArray *ins = new MNewArray(count, templateObject, MNewArray::NewArray_Allocating);

    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_newobject(HandleObject baseObj)
{
    
    JS_ASSERT(script_->compileAndGo);

    RootedObject templateObject(cx);

    if (baseObj) {
        templateObject = CopyInitializerObject(cx, baseObj);
    } else {
        gc::AllocKind kind = GuessObjectGCKind(0);
        templateObject = NewBuiltinClassInstance(cx, &ObjectClass, kind);
    }

    if (!templateObject)
        return false;

    RootedScript script(cx, script_);
    if (types::UseNewTypeForInitializer(cx, script, pc, JSProto_Object)) {
        if (!JSObject::setSingletonType(cx, templateObject))
            return false;
    } else {
        types::TypeObject *type = types::TypeScript::InitObject(cx, script, pc, JSProto_Object);
        if (!type)
            return false;
        templateObject->setType(type);
    }

    MNewObject *ins = MNewObject::New(templateObject);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_initelem()
{
    if (oracle->propertyWriteCanSpecialize(script_, pc)) {
        if (oracle->elementWriteIsDenseArray(script_, pc))
            return jsop_initelem_dense();
    }

    return abort("NYI: JSOP_INITELEM supports for non dense objects/arrays.");
}

bool
IonBuilder::jsop_initelem_dense()
{
    MDefinition *value = current->pop();
    MDefinition *id = current->pop();
    MDefinition *obj = current->peek(-1);

    
    MElements *elements = MElements::New(obj);
    current->add(elements);

    
    MStoreElement *store = MStoreElement::New(elements, id, value);
    current->add(store);

    
    MSetInitializedLength *initLength = MSetInitializedLength::New(elements, id);
    current->add(initLength);

    if (!resumeAfter(initLength))
        return false;

   return true;
}

static bool
CanEffectlesslyCallLookupGenericOnObject(JSObject *obj)
{
    while (obj) {
        if (!obj->isNative())
            return false;
        if (obj->getClass()->ops.lookupProperty)
            return false;
        obj = obj->getProto();
    }
    return true;
}

bool
IonBuilder::jsop_initprop(HandlePropertyName name)
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->peek(-1);

    RootedObject templateObject(cx, obj->toNewObject()->templateObject());

    if (!oracle->propertyWriteCanSpecialize(script_, pc)) {
        
        return abort("INITPROP Monitored initprop");
    }

    if (!CanEffectlesslyCallLookupGenericOnObject(templateObject))
        return abort("INITPROP template object is special");

    RootedObject holder(cx);
    RootedShape shape(cx);
    RootedId id(cx, NameToId(name));
    bool res = LookupPropertyWithFlags(cx, templateObject, id,
                                       JSRESOLVE_QUALIFIED, &holder, &shape);
    if (!res)
        return false;

    if (!shape || holder != templateObject) {
        
        MInitProp *init = MInitProp::New(obj, name, value);
        current->add(init);
        return resumeAfter(init);
    }

    bool needsBarrier = true;
    TypeOracle::BinaryTypes b = oracle->binaryTypes(script_, pc);
    if (b.lhsTypes &&
        ((jsid)id == types::MakeTypeId(cx, id)) &&
        !b.lhsTypes->propertyNeedsBarrier(cx, id))
    {
        needsBarrier = false;
    }

    if (templateObject->isFixedSlot(shape->slot())) {
        MStoreFixedSlot *store = MStoreFixedSlot::New(obj, shape->slot(), value);
        if (needsBarrier)
            store->setNeedsBarrier();

        current->add(store);
        return resumeAfter(store);
    }

    MSlots *slots = MSlots::New(obj);
    current->add(slots);

    uint32_t slot = templateObject->dynamicSlotIndex(shape->slot());
    MStoreSlot *store = MStoreSlot::New(slots, slot, value);
    if (needsBarrier)
        store->setNeedsBarrier();

    current->add(store);
    return resumeAfter(store);
}

MBasicBlock *
IonBuilder::addBlock(MBasicBlock *block, uint32_t loopDepth)
{
    if (!block)
        return NULL;
    graph().addBlock(block);
    block->setLoopDepth(loopDepth);
    return block;
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::New(graph(), info(), predecessor, pc, MBasicBlock::NORMAL);
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
    MBasicBlock *block = MBasicBlock::New(graph(), info(), predecessor, pc, MBasicBlock::NORMAL);
    if (!block)
        return NULL;
    graph().insertBlockAfter(at, block);
    return block;
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc, uint32_t loopDepth)
{
    MBasicBlock *block = MBasicBlock::New(graph(), info(), predecessor, pc, MBasicBlock::NORMAL);
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
        return NULL;

    MOsrEntry *entry = MOsrEntry::New();
    osrBlock->add(entry);

    
    {
        uint32_t slot = info().scopeChainSlot();

        MOsrScopeChain *scopev = MOsrScopeChain::New(entry);
        osrBlock->add(scopev);
        osrBlock->initSlot(slot, scopev);
    }

    if (info().fun()) {
        
        uint32_t slot = info().thisSlot();
        ptrdiff_t offset = StackFrame::offsetOfThis(info().fun());

        MOsrValue *thisv = MOsrValue::New(entry, offset);
        osrBlock->add(thisv);
        osrBlock->initSlot(slot, thisv);

        
        for (uint32_t i = 0; i < info().nargs(); i++) {
            uint32_t slot = info().argSlot(i);
            ptrdiff_t offset = StackFrame::offsetOfFormalArg(info().fun(), i);

            MOsrValue *osrv = MOsrValue::New(entry, offset);
            osrBlock->add(osrv);
            osrBlock->initSlot(slot, osrv);
        }
    }

    
    for (uint32_t i = 0; i < info().nlocals(); i++) {
        uint32_t slot = info().localSlot(i);
        ptrdiff_t offset = StackFrame::offsetOfFixed(i);

        MOsrValue *osrv = MOsrValue::New(entry, offset);
        osrBlock->add(osrv);
        osrBlock->initSlot(slot, osrv);
    }

    
    uint32_t numSlots = preheader->stackDepth() - CountArgSlots(info().fun()) - info().nlocals();
    for (uint32_t i = 0; i < numSlots; i++) {
        uint32_t slot = info().stackSlot(i);
        ptrdiff_t offset = StackFrame::offsetOfFixed(info().nlocals() + i);

        MOsrValue *osrv = MOsrValue::New(entry, offset);
        osrBlock->add(osrv);
        osrBlock->initSlot(slot, osrv);
    }

    
    MStart *start = MStart::New(MStart::StartType_Osr);
    osrBlock->add(start);
    graph().setOsrStart(start);

    
    
    if (!resumeAt(start, loopEntry))
        return NULL;

    
    
    
    osrBlock->linkOsrValues(start);

    
    
    
    JS_ASSERT(predecessor->stackDepth() == osrBlock->stackDepth());
    JS_ASSERT(info().scopeChainSlot() == 0);
    JS_ASSERT(osrBlock->scopeChain()->type() == MIRType_Object);

    Vector<MIRType> slotTypes(cx);
    if (!slotTypes.growByUninitialized(osrBlock->stackDepth()))
        return NULL;

    
    for (uint32_t i = 0; i < osrBlock->stackDepth(); i++)
        slotTypes[i] = MIRType_Value;

    
    if (!oracle->getOsrTypes(loopEntry, slotTypes))
        return NULL;

    for (uint32_t i = 1; i < osrBlock->stackDepth(); i++) {
        
        switch (slotTypes[i]) {
          case MIRType_Boolean:
          case MIRType_Int32:
          case MIRType_Double:
          case MIRType_String:
          case MIRType_Object:
          {
            MDefinition *def = osrBlock->getSlot(i);
            JS_ASSERT(def->type() == MIRType_Value);

            MInstruction *actual = MUnbox::New(def, slotTypes[i], MUnbox::Infallible);
            osrBlock->add(actual);
            osrBlock->rewriteSlot(i, actual);
            break;
          }

          case MIRType_Null:
          {
            MConstant *c = MConstant::New(NullValue());
            osrBlock->add(c);
            osrBlock->rewriteSlot(i, c);
            break;
          }

          case MIRType_Undefined:
          {
            MConstant *c = MConstant::New(UndefinedValue());
            osrBlock->add(c);
            osrBlock->rewriteSlot(i, c);
            break;
          }

          case MIRType_Magic:
            JS_ASSERT(lazyArguments_);
            osrBlock->rewriteSlot(i, lazyArguments_);
            break;

          default:
            break;
        }
    }

    
    osrBlock->end(MGoto::New(preheader));
    preheader->addPredecessor(osrBlock);
    graph().setOsrBlock(osrBlock);

    
    
    if (info().fun())
        preheader->getSlot(info().thisSlot())->setGuard();

    return preheader;
}

MBasicBlock *
IonBuilder::newPendingLoopHeader(MBasicBlock *predecessor, jsbytecode *pc)
{
    loopDepth_++;
    MBasicBlock *block = MBasicBlock::NewPendingLoopHeader(graph(), info(), predecessor, pc);
    return addBlock(block, loopDepth_);
}
























bool
IonBuilder::resume(MInstruction *ins, jsbytecode *pc, MResumePoint::Mode mode)
{
    JS_ASSERT(ins->isEffectful() || !ins->isMovable());

    MResumePoint *resumePoint = MResumePoint::New(ins->block(), pc, callerResumePoint_, mode);
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

    MNop *ins = MNop::New();
    current->add(ins);

    return resumeAfter(ins);
}

void
IonBuilder::insertRecompileCheck()
{
    if (!inliningEnabled())
        return;

    if (inliningDepth > 0)
        return;

    
    if (script_->getUseCount() >= js_IonOptions.usesBeforeInlining)
        return;

    
    
    if (!oracle->canInlineCalls())
        return;

    uint32_t minUses = UsesBeforeIonRecompile(script_, pc);
    MRecompileCheck *check = MRecompileCheck::New(minUses);
    current->add(check);
}

static inline bool
TestSingletonProperty(JSContext *cx, HandleObject obj, HandleId id, bool *isKnownConstant)
{
    
    
    
    
    
    
    
    
    
    
    
    

    *isKnownConstant = false;

    if (!CanEffectlesslyCallLookupGenericOnObject(obj))
        return true;

    RootedObject holder(cx);
    RootedShape shape(cx);
    if (!JSObject::lookupGeneric(cx, obj, id, &holder, &shape))
        return false;
    if (!shape)
        return true;

    if (!shape->hasDefaultGetter())
        return true;
    if (!shape->hasSlot())
        return true;
    if (holder->getSlot(shape->slot()).isUndefined())
        return true;

    *isKnownConstant = true;
    return true;
}

static inline bool
TestSingletonPropertyTypes(JSContext *cx, types::StackTypeSet *types,
                           HandleObject globalObj, HandleId id,
                           bool *isKnownConstant, bool *testObject,
                           bool *testString)
{
    
    
    

    *isKnownConstant = false;
    *testObject = false;
    *testString = false;

    if (!types || types->unknownObject())
        return true;

    RootedObject singleton(cx, types->getSingleton());
    if (singleton)
        return TestSingletonProperty(cx, singleton, id, isKnownConstant);

    if (!globalObj)
        return true;

    JSProtoKey key;
    JSValueType type = types->getKnownTypeTag();
    switch (type) {
      case JSVAL_TYPE_STRING:
        key = JSProto_String;
        break;

      case JSVAL_TYPE_INT32:
      case JSVAL_TYPE_DOUBLE:
        key = JSProto_Number;
        break;

      case JSVAL_TYPE_BOOLEAN:
        key = JSProto_Boolean;
        break;

      case JSVAL_TYPE_OBJECT:
      case JSVAL_TYPE_UNKNOWN: {
        if (types->hasType(types::Type::StringType())) {
            
            if (types->maybeObject())
                return true;
            key = JSProto_String;
            *testString = true;
            break;
        }

        
        
        
        bool thoughtConstant = true;
        for (unsigned i = 0; i < types->getObjectCount(); i++) {
            types::TypeObject *object = types->getTypeObject(i);
            if (!object) {
                
                JSObject *curObj = types->getSingleObject(i);
                
                
                if (!curObj)
                    continue;
                object = curObj->getType(cx);
            }

            if (object->proto) {
                
                RootedObject proto(cx, object->proto);
                if (!TestSingletonProperty(cx, proto, id, &thoughtConstant))
                    return false;
                
                if (!thoughtConstant)
                    break;
            } else {
                
                thoughtConstant = false;
                break;
            }
        }
        if (thoughtConstant) {
            
            *testObject = (type != JSVAL_TYPE_OBJECT);
        }
        *isKnownConstant = thoughtConstant;
        return true;
      }
      default:
        return true;
    }

    RootedObject proto(cx);
    if (!js_GetClassPrototype(cx, key, &proto, NULL))
        return false;

    return TestSingletonProperty(cx, proto, id, isKnownConstant);
}












bool
IonBuilder::pushTypeBarrier(MInstruction *ins, types::StackTypeSet *actual,
                            types::StackTypeSet *observed)
{
    
    
    
    
    

    if (!actual) {
        JS_ASSERT(!observed);
        return true;
    }

    if (!observed) {
        JSValueType type = actual->getKnownTypeTag();
        MInstruction *replace = NULL;
        switch (type) {
          case JSVAL_TYPE_UNDEFINED:
            ins->setFoldedUnchecked();
            replace = MConstant::New(UndefinedValue());
            break;
          case JSVAL_TYPE_NULL:
            ins->setFoldedUnchecked();
            replace = MConstant::New(NullValue());
            break;
          case JSVAL_TYPE_UNKNOWN:
            break;
          default: {
            MIRType replaceType = MIRTypeFromValueType(type);
            if (ins->type() == MIRType_Value)
                replace = MUnbox::New(ins, replaceType, MUnbox::Infallible);
            else
                JS_ASSERT(ins->type() == replaceType);
            break;
          }
        }
        if (replace) {
            current->pop();
            current->add(replace);
            current->push(replace);
        }
        return true;
    }

    if (observed->unknown())
        return true;

    current->pop();

    MInstruction *barrier;
    JSValueType type = observed->getKnownTypeTag();

    
    
    bool isObject = false;
    if (type == JSVAL_TYPE_OBJECT && !observed->hasType(types::Type::AnyObjectType())) {
        type = JSVAL_TYPE_UNKNOWN;
        isObject = true;
    }

    switch (type) {
      case JSVAL_TYPE_UNKNOWN:
      case JSVAL_TYPE_UNDEFINED:
      case JSVAL_TYPE_NULL:
        barrier = MTypeBarrier::New(ins, cloneTypeSet(observed));
        current->add(barrier);

        if (type == JSVAL_TYPE_UNDEFINED)
            return pushConstant(UndefinedValue());
        if (type == JSVAL_TYPE_NULL)
            return pushConstant(NullValue());
        if (isObject) {
            barrier = MUnbox::New(barrier, MIRType_Object, MUnbox::Infallible);
            current->add(barrier);
        }
        break;
      default:
        MUnbox::Mode mode = ins->isEffectful() ? MUnbox::TypeBarrier : MUnbox::TypeGuard;
        barrier = MUnbox::New(ins, MIRTypeFromValueType(type), mode);
        current->add(barrier);
    }
    current->push(barrier);
    return true;
}



void
IonBuilder::monitorResult(MInstruction *ins, types::TypeSet *barrier, types::TypeSet *types)
{
    
    if (barrier)
        return;

    if (!types || types->unknown())
        return;

    MInstruction *monitor = MMonitorTypes::New(ins, cloneTypeSet(types));
    current->add(monitor);
}

bool
IonBuilder::jsop_getgname(HandlePropertyName name)
{
    
    if (name == cx->names().undefined)
        return pushConstant(UndefinedValue());
    if (name == cx->names().NaN)
        return pushConstant(cx->runtime->NaNValue);
    if (name == cx->names().Infinity)
        return pushConstant(cx->runtime->positiveInfinityValue);

    RootedObject globalObj(cx, &script_->global());
    JS_ASSERT(globalObj->isNative());

    RootedId id(cx, NameToId(name));

    
    
    const js::Shape *shape = globalObj->nativeLookup(cx, id);
    if (!shape || !shape->hasDefaultGetter() || !shape->hasSlot())
        return jsop_getname(name);

    types::HeapTypeSet *propertyTypes = oracle->globalPropertyTypeSet(script_, pc, id);
    if (propertyTypes && propertyTypes->isOwnProperty(cx, globalObj->getType(cx), true)) {
        
        
        return jsop_getname(name);
    }

    
    JSValueType knownType = JSVAL_TYPE_UNKNOWN;

    types::StackTypeSet *barrier = oracle->propertyReadBarrier(script_, pc);
    types::StackTypeSet *types = oracle->propertyRead(script_, pc);
    if (types) {
        JSObject *singleton = types->getSingleton();

        knownType = types->getKnownTypeTag();
        if (!barrier) {
            if (singleton) {
                
                bool isKnownConstant;
                if (!TestSingletonProperty(cx, globalObj, id, &isKnownConstant))
                    return false;
                if (isKnownConstant)
                    return pushConstant(ObjectValue(*singleton));
            }
            if (knownType == JSVAL_TYPE_UNDEFINED)
                return pushConstant(UndefinedValue());
            if (knownType == JSVAL_TYPE_NULL)
                return pushConstant(NullValue());
        }
    }

    MInstruction *global = MConstant::New(ObjectValue(*globalObj));
    current->add(global);

    
    
    if (!propertyTypes && shape->configurable())
        global = addShapeGuard(global, globalObj->lastProperty(), Bailout_ShapeGuard);

    JS_ASSERT(shape->slot() >= globalObj->numFixedSlots());

    MSlots *slots = MSlots::New(global);
    current->add(slots);
    MLoadSlot *load = MLoadSlot::New(slots, shape->slot() - globalObj->numFixedSlots());
    current->add(load);

    
    if (knownType != JSVAL_TYPE_UNKNOWN && !barrier)
        load->setResultType(MIRTypeFromValueType(knownType));

    current->push(load);
    return pushTypeBarrier(load, types, barrier);
}

bool
IonBuilder::jsop_setgname(HandlePropertyName name)
{
    RootedObject globalObj(cx, &script_->global());
    RootedId id(cx, NameToId(name));

    JS_ASSERT(globalObj->isNative());

    bool canSpecialize;
    types::HeapTypeSet *propertyTypes = oracle->globalPropertyWrite(script_, pc, id, &canSpecialize);

    
    if (!canSpecialize || globalObj->watched())
        return jsop_setprop(name);

    
    
    const js::Shape *shape = globalObj->nativeLookup(cx, id);
    if (!shape || !shape->hasDefaultSetter() || !shape->writable() || !shape->hasSlot())
        return jsop_setprop(name);

    if (propertyTypes && propertyTypes->isOwnProperty(cx, globalObj->getType(cx), true)) {
        
        
        return jsop_setprop(name);
    }

    MInstruction *global = MConstant::New(ObjectValue(*globalObj));
    current->add(global);

    
    
    
    if (!propertyTypes)
        global = addShapeGuard(global, globalObj->lastProperty(), Bailout_ShapeGuard);

    JS_ASSERT(shape->slot() >= globalObj->numFixedSlots());

    MSlots *slots = MSlots::New(global);
    current->add(slots);

    MDefinition *value = current->pop();
    MStoreSlot *store = MStoreSlot::New(slots, shape->slot() - globalObj->numFixedSlots(), value);
    current->add(store);

    
    if (!propertyTypes || propertyTypes->needsBarrier(cx))
        store->setNeedsBarrier();

    
    DebugOnly<MDefinition *> pushedGlobal = current->pop();
    JS_ASSERT(&pushedGlobal->toConstant()->value().toObject() == globalObj);

    
    
    
    
    if (propertyTypes && !globalObj->getSlot(shape->slot()).isUndefined()) {
        JSValueType knownType = propertyTypes->getKnownTypeTag(cx);
        if (knownType != JSVAL_TYPE_UNKNOWN)
            store->setSlotType(MIRTypeFromValueType(knownType));
    }

    JS_ASSERT_IF(store->needsBarrier(), store->slotType() != MIRType_None);

    current->push(value);
    return resumeAfter(store);
}

bool
IonBuilder::jsop_getname(HandlePropertyName name)
{
    MDefinition *object;
    if (js_CodeSpec[*pc].format & JOF_GNAME) {
        MInstruction *global = MConstant::New(ObjectValue(script_->global()));
        current->add(global);
        object = global;
    } else {
        current->push(current->scopeChain());
        object = current->pop();
    }

    MGetNameCache *ins;
    if (JSOp(*GetNextPc(pc)) == JSOP_TYPEOF)
        ins = MGetNameCache::New(object, name, MGetNameCache::NAMETYPEOF);
    else
        ins = MGetNameCache::New(object, name, MGetNameCache::NAME);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    types::StackTypeSet *barrier = oracle->propertyReadBarrier(script_, pc);
    types::StackTypeSet *types = oracle->propertyRead(script_, pc);

    monitorResult(ins, barrier, types);
    return pushTypeBarrier(ins, types, barrier);
}

bool
IonBuilder::jsop_intrinsicname(HandlePropertyName name)
{
    types::StackTypeSet *types = oracle->propertyRead(script_, pc);
    JSValueType type = types->getKnownTypeTag();

    
    
    if (type == JSVAL_TYPE_UNKNOWN) {
        MCallGetIntrinsicValue *ins = MCallGetIntrinsicValue::New(name);

        current->add(ins);
        current->push(ins);

        if (!resumeAfter(ins))
            return false;

        types::StackTypeSet *barrier = oracle->propertyReadBarrier(script_, pc);
        monitorResult(ins, barrier, types);
        return pushTypeBarrier(ins, types, barrier);
    }

    
    RootedValue vp(cx, UndefinedValue());
    if (!cx->global()->getIntrinsicValue(cx, name, &vp))
        return false;

    JS_ASSERT(types->hasType(types::GetValueType(cx, vp)));

    MConstant *ins = MConstant::New(vp);
    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_bindname(PropertyName *name)
{
    JS_ASSERT(script_->analysis()->usesScopeChain());

    MDefinition *scopeChain = current->scopeChain();
    MBindNameCache *ins = MBindNameCache::New(scopeChain, name, script_, pc);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_getelem()
{
    if (oracle->elementReadIsDenseArray(script_, pc))
        return jsop_getelem_dense();

    int arrayType = TypedArray::TYPE_MAX;
    if (oracle->elementReadIsTypedArray(script_, pc, &arrayType))
        return jsop_getelem_typed(arrayType);

    if (oracle->elementReadIsString(script_, pc))
        return jsop_getelem_string();

    LazyArgumentsType isArguments = oracle->elementReadMagicArguments(script_, pc);
    if (isArguments == MaybeArguments)
        return abort("Type is not definitely lazy arguments.");
    if (isArguments == DefinitelyArguments)
        return jsop_arguments_getelem();

    MDefinition *rhs = current->pop();
    MDefinition *lhs = current->pop();

    MInstruction *ins;

    
    
    
    
    bool mustMonitorResult = false;
    bool cacheable = false;

    oracle->elementReadGeneric(script_, pc, &cacheable, &mustMonitorResult);

    if (cacheable)
        ins = MGetElementCache::New(lhs, rhs, mustMonitorResult);
    else
        ins = MCallGetElement::New(lhs, rhs);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    types::StackTypeSet *barrier = oracle->propertyReadBarrier(script_, pc);
    types::StackTypeSet *types = oracle->propertyRead(script_, pc);

    if (mustMonitorResult)
        monitorResult(ins, barrier, types);
    return pushTypeBarrier(ins, types, barrier);
}

bool
IonBuilder::jsop_getelem_dense()
{
    if (oracle->arrayPrototypeHasIndexedProperty())
        return abort("GETELEM Array proto has indexed properties");

    types::StackTypeSet *barrier = oracle->propertyReadBarrier(script_, pc);
    types::StackTypeSet *types = oracle->propertyRead(script_, pc);
    bool needsHoleCheck = !oracle->elementReadIsPacked(script_, pc);
    bool maybeUndefined = types->hasType(types::Type::UndefinedType());

    MDefinition *id = current->pop();
    MDefinition *obj = current->pop();

    JSValueType knownType = JSVAL_TYPE_UNKNOWN;
    if (!barrier) {
        knownType = types->getKnownTypeTag();

        
        
        
        
        
        if (knownType == JSVAL_TYPE_UNDEFINED || knownType == JSVAL_TYPE_NULL)
            knownType = JSVAL_TYPE_UNKNOWN;

        
        
        if (needsHoleCheck && !LIRGenerator::allowTypedElementHoleCheck())
            knownType = JSVAL_TYPE_UNKNOWN;
    }

    
    MInstruction *idInt32 = MToInt32::New(id);
    current->add(idInt32);
    id = idInt32;

    
    MElements *elements = MElements::New(obj);
    current->add(elements);

    MInitializedLength *initLength = MInitializedLength::New(elements);
    current->add(initLength);

    MInstruction *load;

    if (!maybeUndefined) {
        
        
        
        
        id = addBoundsCheck(id, initLength);

        load = MLoadElement::New(elements, id, needsHoleCheck);
        current->add(load);
    } else {
        
        
        
        load = MLoadElementHole::New(elements, id, initLength, needsHoleCheck);
        current->add(load);

        
        
        
        JS_ASSERT(knownType == JSVAL_TYPE_UNKNOWN);
    }

    if (knownType != JSVAL_TYPE_UNKNOWN)
        load->setResultType(MIRTypeFromValueType(knownType));

    current->push(load);
    return pushTypeBarrier(load, types, barrier);
}

static MInstruction *
GetTypedArrayLength(MDefinition *obj)
{
    if (obj->isConstant()) {
        JSObject *array = &obj->toConstant()->value().toObject();
        int32_t length = (int32_t) TypedArray::length(array);
        obj->setFoldedUnchecked();
        return MConstant::New(Int32Value(length));
    }
    return MTypedArrayLength::New(obj);
}

static MInstruction *
GetTypedArrayElements(MDefinition *obj)
{
    if (obj->isConstant()) {
        JSObject *array = &obj->toConstant()->value().toObject();
        void *data = TypedArray::viewData(array);
        obj->setFoldedUnchecked();
        return MConstantElements::New(data);
    }
    return MTypedArrayElements::New(obj);
}

bool
IonBuilder::jsop_getelem_typed(int arrayType)
{
    types::StackTypeSet *barrier = oracle->propertyReadBarrier(script_, pc);
    types::StackTypeSet *types = oracle->propertyRead(script_, pc);

    MDefinition *id = current->pop();
    MDefinition *obj = current->pop();

    bool maybeUndefined = types->hasType(types::Type::UndefinedType());

    
    
    
    bool allowDouble = types->hasType(types::Type::DoubleType());

    
    MInstruction *idInt32 = MToInt32::New(id);
    current->add(idInt32);
    id = idInt32;

    if (!maybeUndefined) {
        
        

        
        
        
        MIRType knownType;
        switch (arrayType) {
          case TypedArray::TYPE_INT8:
          case TypedArray::TYPE_UINT8:
          case TypedArray::TYPE_UINT8_CLAMPED:
          case TypedArray::TYPE_INT16:
          case TypedArray::TYPE_UINT16:
          case TypedArray::TYPE_INT32:
            knownType = MIRType_Int32;
            break;
          case TypedArray::TYPE_UINT32:
            knownType = allowDouble ? MIRType_Double : MIRType_Int32;
            break;
          case TypedArray::TYPE_FLOAT32:
          case TypedArray::TYPE_FLOAT64:
            knownType = MIRType_Double;
            break;
          default:
            JS_NOT_REACHED("Unknown typed array type");
            return false;
        }

        
        MInstruction *length = GetTypedArrayLength(obj);
        current->add(length);

        
        id = addBoundsCheck(id, length);

        
        MInstruction *elements = GetTypedArrayElements(obj);
        current->add(elements);

        
        MLoadTypedArrayElement *load = MLoadTypedArrayElement::New(elements, id, arrayType);
        current->add(load);
        current->push(load);

        load->setResultType(knownType);

        
        
        JS_ASSERT_IF(knownType == MIRType_Int32, types->hasType(types::Type::Int32Type()));
        JS_ASSERT_IF(knownType == MIRType_Double, types->hasType(types::Type::DoubleType()));
        return true;
    } else {
        
        
        
        MLoadTypedArrayElementHole *load =
            MLoadTypedArrayElementHole::New(obj, id, arrayType, allowDouble);
        current->add(load);
        current->push(load);

        return resumeAfter(load) && pushTypeBarrier(load, types, barrier);
    }
}

bool
IonBuilder::jsop_getelem_string()
{
    MDefinition *id = current->pop();
    MDefinition *str = current->pop();

    MInstruction *idInt32 = MToInt32::New(id);
    current->add(idInt32);
    id = idInt32;

    MStringLength *length = MStringLength::New(str);
    current->add(length);

    
    
    JS_ASSERT(oracle->propertyRead(script_, pc)->getKnownTypeTag() == JSVAL_TYPE_STRING);
    id = addBoundsCheck(id, length);

    MCharCodeAt *charCode = MCharCodeAt::New(str, id);
    current->add(charCode);

    MFromCharCode *result = MFromCharCode::New(charCode);
    current->add(result);
    current->push(result);
    return true;
}

bool
IonBuilder::jsop_setelem()
{
    if (oracle->propertyWriteCanSpecialize(script_, pc)) {
        if (oracle->elementWriteIsDenseArray(script_, pc))
            return jsop_setelem_dense();

        int arrayType = TypedArray::TYPE_MAX;
        if (oracle->elementWriteIsTypedArray(script_, pc, &arrayType))
            return jsop_setelem_typed(arrayType);
    }

    LazyArgumentsType isArguments = oracle->elementWriteMagicArguments(script_, pc);
    if (isArguments == MaybeArguments)
        return abort("Type is not definitely lazy arguments.");
    if (isArguments == DefinitelyArguments)
        return jsop_arguments_setelem();

    MDefinition *value = current->pop();
    MDefinition *index = current->pop();
    MDefinition *object = current->pop();

    MInstruction *ins = MCallSetElement::New(object, index, value);
    current->add(ins);
    current->push(value);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_setelem_dense()
{
    if (oracle->arrayPrototypeHasIndexedProperty())
        return abort("SETELEM Array proto has indexed properties");

    MIRType elementType = oracle->elementWrite(script_, pc);
    bool packed = oracle->elementWriteIsPacked(script_, pc);

    MDefinition *value = current->pop();
    MDefinition *id = current->pop();
    MDefinition *obj = current->pop();

    
    MInstruction *idInt32 = MToInt32::New(id);
    current->add(idInt32);
    id = idInt32;

    
    MElements *elements = MElements::New(obj);
    current->add(elements);

    
    
    
    MStoreElementCommon *store;
    if (oracle->setElementHasWrittenHoles(script_, pc)) {
        MStoreElementHole *ins = MStoreElementHole::New(obj, elements, id, value);
        store = ins;

        current->add(ins);
        current->push(value);

        if (!resumeAfter(ins))
            return false;
    } else {
        MInitializedLength *initLength = MInitializedLength::New(elements);
        current->add(initLength);

        id = addBoundsCheck(id, initLength);

        MStoreElement *ins = MStoreElement::New(elements, id, value);
        store = ins;

        current->add(ins);
        current->push(value);

        if (!resumeAfter(ins))
            return false;
    }

    
    if (oracle->elementWriteNeedsBarrier(script_, pc))
        store->setNeedsBarrier();

    if (elementType != MIRType_None && packed)
        store->setElementType(elementType);

    return true;
}

bool
IonBuilder::jsop_setelem_typed(int arrayType)
{
    MDefinition *value = current->pop();
    MDefinition *id = current->pop();
    MDefinition *obj = current->pop();

    
    MInstruction *idInt32 = MToInt32::New(id);
    current->add(idInt32);
    id = idInt32;

    
    MInstruction *length = GetTypedArrayLength(obj);
    current->add(length);

    
    id = addBoundsCheck(id, length);

    
    MInstruction *elements = GetTypedArrayElements(obj);
    current->add(elements);

    
    MDefinition *unclampedValue = value;
    if (arrayType == TypedArray::TYPE_UINT8_CLAMPED) {
        value = MClampToUint8::New(value);
        current->add(value->toInstruction());
    }

    
    MStoreTypedArrayElement *store = MStoreTypedArrayElement::New(elements, id, value, arrayType);
    current->add(store);

    current->push(unclampedValue);
    return resumeAfter(store);
}

bool
IonBuilder::jsop_length()
{
    if (jsop_length_fastPath())
        return true;

    RootedPropertyName name(cx, info().getAtom(pc)->asPropertyName());
    return jsop_getprop(name);
}

bool
IonBuilder::jsop_length_fastPath()
{
    TypeOracle::UnaryTypes sig = oracle->unaryTypes(script_, pc);
    if (!sig.inTypes || !sig.outTypes)
        return false;

    if (sig.outTypes->getKnownTypeTag() != JSVAL_TYPE_INT32)
        return false;

    switch (sig.inTypes->getKnownTypeTag()) {
      case JSVAL_TYPE_STRING: {
        MDefinition *obj = current->pop();
        MStringLength *ins = MStringLength::New(obj);
        current->add(ins);
        current->push(ins);
        return true;
      }

      case JSVAL_TYPE_OBJECT: {
        if (!sig.inTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY)) {
            MDefinition *obj = current->pop();
            MElements *elements = MElements::New(obj);
            current->add(elements);

            
            MArrayLength *length = new MArrayLength(elements);
            current->add(length);
            current->push(length);
            return true;
        }

        if (!sig.inTypes->hasObjectFlags(cx, types::OBJECT_FLAG_NON_TYPED_ARRAY)) {
            MDefinition *obj = current->pop();
            MInstruction *length = GetTypedArrayLength(obj);
            current->add(length);
            current->push(length);
            return true;
        }

        return false;
      }

      default:
        break;
    }

    return false;
}

bool
IonBuilder::jsop_arguments()
{
    JS_ASSERT(lazyArguments_);
    current->push(lazyArguments_);
    return true;
}

bool
IonBuilder::jsop_arguments_length()
{
    
    MDefinition *args = current->pop();
    args->setFoldedUnchecked();

    MInstruction *ins = MArgumentsLength::New();
    current->add(ins);
    current->push(ins);
    return true;
}

bool
IonBuilder::jsop_arguments_getelem()
{
    types::StackTypeSet *barrier = oracle->propertyReadBarrier(script_, pc);
    types::StackTypeSet *types = oracle->propertyRead(script_, pc);

    MDefinition *idx = current->pop();

    
    MDefinition *args = current->pop();
    args->setFoldedUnchecked();

    
    MArgumentsLength *length = MArgumentsLength::New();
    current->add(length);

    
    MInstruction *index = MToInt32::New(idx);
    current->add(index);

    
    index = addBoundsCheck(index, length);

    
    MGetArgument *load = MGetArgument::New(index);
    current->add(load);
    current->push(load);

    return pushTypeBarrier(load, types, barrier);
}

bool
IonBuilder::jsop_arguments_setelem()
{
    return abort("NYI arguments[]=");
}

inline types::HeapTypeSet *
GetDefiniteSlot(JSContext *cx, types::StackTypeSet *types, JSAtom *atom)
{
    if (!types || types->unknownObject() || types->getObjectCount() != 1)
        return NULL;

    types::TypeObject *type = types->getTypeObject(0);
    if (!type || type->unknownProperties())
        return NULL;

    jsid id = AtomToId(atom);
    if (id != types::MakeTypeId(cx, id))
        return NULL;

    types::HeapTypeSet *propertyTypes = type->getProperty(cx, id, false);
    if (!propertyTypes ||
        !propertyTypes->definiteProperty() ||
        propertyTypes->isOwnProperty(cx, type, true))
    {
        return NULL;
    }

    return propertyTypes;
}

bool
IonBuilder::jsop_not()
{
    MDefinition *value = current->pop();

    MNot *ins = new MNot(value);
    current->add(ins);
    current->push(ins);
    return true;
}


inline bool
IonBuilder::TestCommonPropFunc(JSContext *cx, types::StackTypeSet *types, HandleId id,
                               JSFunction **funcp, bool isGetter, bool *isDOM,
                               MDefinition **guardOut)
{
    JSObject *found = NULL;
    JSObject *foundProto = NULL;

    *funcp = NULL;
    *isDOM = false;

    bool thinkDOM = true;

    
    if (!types || types->unknownObject())
        return true;

    
    
    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        RootedObject curObj(cx, types->getSingleObject(i));

        
        if (!curObj) {
            types::TypeObject *typeObj = types->getTypeObject(i);

            if (!typeObj)
                continue;

            if (typeObj->unknownProperties())
                return true;

            
            
            jsid typeId = types::MakeTypeId(cx, id);
            types::HeapTypeSet *propSet = typeObj->getProperty(cx, typeId, false);
            if (!propSet)
                return false;
            if (propSet->ownProperty(false))
                return true;

            
            thinkDOM = thinkDOM && !typeObj->hasAnyFlags(types::OBJECT_FLAG_NON_DOM);

            
            curObj = typeObj->proto;
        } else {
            
            
            
            if (!isGetter && curObj->watched())
                return true;

            
            types::TypeObject *objType = curObj->getType(cx);
            thinkDOM = thinkDOM && !objType->hasAnyFlags(types::OBJECT_FLAG_NON_DOM);
        }

        
        
        if (!CanEffectlesslyCallLookupGenericOnObject(curObj))
            return true;

        RootedObject proto(cx);
        RootedShape shape(cx);
        if (!JSObject::lookupGeneric(cx, curObj, id, &proto, &shape))
            return false;

        if (!shape)
            return true;

        
        
        if (isGetter) {
            if (shape->hasDefaultGetter() || !shape->hasGetterValue())
                return true;
        } else {
            if (shape->hasDefaultSetter() || !shape->hasSetterValue())
                return true;
        }

        JSObject * curFound = isGetter ? shape->getterObject():
                                         shape->setterObject();

        
        if (!found) {
            if (!curFound->isFunction())
                return true;
            found = curFound;
        } else if (found != curFound) {
            return true;
        }

        
        
        
        if (!foundProto)
            foundProto = proto;
        else if (foundProto != proto)
            return true;

        
        
        
        
        
        
        while (curObj != foundProto) {
            types::TypeObject *typeObj = curObj->getType(cx);

            if (typeObj->unknownProperties())
                return true;

            
            
            
            
            

            
            
            
            jsid typeId = types::MakeTypeId(cx, id);
            types::HeapTypeSet *propSet = typeObj->getProperty(cx, typeId, false);
            if (!propSet)
                return false;
            if (propSet->ownProperty(false))
                return true;

            curObj = curObj->getProto();
        }
    }

    
    if (!found)
        return true;

    JS_ASSERT(foundProto);

    
    
    
    
    MInstruction *wrapper = MConstant::New(ObjectValue(*foundProto));
    current->add(wrapper);
    wrapper = addShapeGuard(wrapper, foundProto->lastProperty(), Bailout_ShapeGuard);

    
    if (isGetter) {
        JS_ASSERT(wrapper->isGuardShape());
        *guardOut = wrapper;
    }

    
    
    types::TypeObject *curType;
    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        curType = types->getTypeObject(i);
        JSObject *obj = NULL;
        if (!curType) {
            obj = types->getSingleObject(i);
            if (!obj)
                continue;

            curType = obj->getType(cx);
        }

        
        if (thinkDOM) {
            
            DebugOnly<bool> wasntDOM =
                types::HeapTypeSet::HasObjectFlags(cx, curType, types::OBJECT_FLAG_NON_DOM);
            JS_ASSERT(!wasntDOM);
        }

        
        
        if (obj != foundProto) {
            
            
            jsid typeId = types::MakeTypeId(cx, id);
            while (true) {
                types::HeapTypeSet *propSet = curType->getProperty(cx, typeId, false);
                
                
                JS_ASSERT(propSet);
                
                DebugOnly<bool> isOwn = propSet->isOwnProperty(cx, curType, false);
                JS_ASSERT(!isOwn);
                
                
                
                if (curType->proto == foundProto)
                    break;
                curType = curType->proto->getType(cx);
            }
        }
    }

    *funcp = found->toFunction();
    *isDOM = thinkDOM;

    return true;
}

static bool
TestShouldDOMCall(JSContext *cx, types::TypeSet *inTypes, HandleFunction func)
{
    if (!func->isNative() || !func->jitInfo())
        return false;
    
    
    
    DOMInstanceClassMatchesProto instanceChecker =
        GetDOMCallbacks(cx->runtime)->instanceClassMatchesProto;

    const JSJitInfo *jinfo = func->jitInfo();

    for (unsigned i = 0; i < inTypes->getObjectCount(); i++) {
        types::TypeObject *curType = inTypes->getTypeObject(i);

        if (!curType) {
            JSObject *curObj = inTypes->getSingleObject(i);

            if (!curObj)
                continue;

            curType = curObj->getType(cx);
        }

        JSObject *typeProto = curType->proto;
        RootedObject proto(cx, typeProto);
        if (!instanceChecker(proto, jinfo->protoID, jinfo->depth))
            return false;
    }

    return true;
}

static bool
TestAreKnownDOMTypes(JSContext *cx, types::TypeSet *inTypes)
{
    if (inTypes->unknown())
        return false;

    
    
    for (unsigned i = 0; i < inTypes->getObjectCount(); i++) {
        types::TypeObject *curType = inTypes->getTypeObject(i);

        if (!curType) {
            JSObject *curObj = inTypes->getSingleObject(i);

            
            if (!curObj)
                continue;

            curType = curObj->getType(cx);
        }

        if (curType->unknownProperties())
            return false;

        
        
        if (curType->hasAnyFlags(types::OBJECT_FLAG_NON_DOM))
            return false;
    }

    
    if (inTypes->getObjectCount() > 0)
        return true;

    return false;
}

static void
FreezeDOMTypes(JSContext *cx, types::StackTypeSet *inTypes)
{
    for (unsigned i = 0; i < inTypes->getObjectCount(); i++) {
        types::TypeObject *curType = inTypes->getTypeObject(i);

        if (!curType) {
            JSObject *curObj = inTypes->getSingleObject(i);

            
            if (!curObj)
                continue;

            curType = curObj->getType(cx);
        }

        
        DebugOnly<bool> wasntDOM =
            types::HeapTypeSet::HasObjectFlags(cx, curType, types::OBJECT_FLAG_NON_DOM);
        JS_ASSERT(!wasntDOM);
    }
}

bool
IonBuilder::annotateGetPropertyCache(JSContext *cx, MDefinition *obj, MGetPropertyCache *getPropCache,
                                    types::StackTypeSet *objTypes, types::StackTypeSet *pushedTypes)
{
    RootedId id(cx, NameToId(getPropCache->name()));
    if ((jsid)id != types::MakeTypeId(cx, id))
        return true;

    
    if (pushedTypes->unknownObject() || pushedTypes->baseFlags() != 0)
        return true;

    for (unsigned i = 0; i < pushedTypes->getObjectCount(); i++) {
        if (pushedTypes->getTypeObject(i) != NULL)
            return true;
    }

    
    if (objTypes->baseFlags() || objTypes->unknownObject())
        return true;

    unsigned int objCount = objTypes->getObjectCount();
    if (objCount == 0)
        return true;

    InlinePropertyTable *inlinePropTable = getPropCache->initInlinePropertyTable(pc);
    if (!inlinePropTable)
        return false;

    
    
    for (unsigned int i = 0; i < objCount; i++) {
        types::TypeObject *typeObj = objTypes->getTypeObject(i);
        if (!typeObj || typeObj->unknownProperties() || !typeObj->proto)
            continue;

        types::HeapTypeSet *ownTypes = typeObj->getProperty(cx, id, false);
        if (!ownTypes)
            continue;

        if (ownTypes->isOwnProperty(cx, typeObj, false))
            continue;

        bool knownConstant = false;
        Rooted<JSObject*> proto(cx, typeObj->proto);
        if (!TestSingletonProperty(cx, proto, id, &knownConstant))
            return false;

        if (!knownConstant || proto->getType(cx)->unknownProperties())
            continue;

        types::HeapTypeSet *protoTypes = proto->getType(cx)->getProperty(cx, id, false);
        if (!protoTypes)
            continue;

        JSObject *obj = protoTypes->getSingleton(cx);
        if (!obj || !obj->isFunction())
            continue;

        
        if (!pushedTypes->hasType(types::Type::ObjectType(obj)))
            continue;

        if (!inlinePropTable->addEntry(typeObj, obj->toFunction()))
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
        MResumePoint *resumePoint = MResumePoint::New(current, pc, callerResumePoint_,
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
IonBuilder::loadSlot(MDefinition *obj, Shape *shape, MIRType rvalType)
{
    JS_ASSERT(shape->hasDefaultGetter());
    JS_ASSERT(shape->hasSlot());

    types::StackTypeSet *barrier = oracle->propertyReadBarrier(script_, pc);
    types::StackTypeSet *types = oracle->propertyRead(script_, pc);

    if (shape->slot() < shape->numFixedSlots()) {
        MLoadFixedSlot *load = MLoadFixedSlot::New(obj, shape->slot());
        current->add(load);
        current->push(load);

        load->setResultType(rvalType);
        return pushTypeBarrier(load, types, barrier);
    }

    MSlots *slots = MSlots::New(obj);
    current->add(slots);

    MLoadSlot *load = MLoadSlot::New(slots, shape->slot() - shape->numFixedSlots());
    current->add(load);
    current->push(load);

    load->setResultType(rvalType);
    return pushTypeBarrier(load, types, barrier);
}

bool
IonBuilder::storeSlot(MDefinition *obj, Shape *shape, MDefinition *value, bool needsBarrier)
{
    JS_ASSERT(shape->hasDefaultSetter());
    JS_ASSERT(shape->writable());
    JS_ASSERT(shape->hasSlot());

    if (shape->slot() < shape->numFixedSlots()) {
        MStoreFixedSlot *store = MStoreFixedSlot::New(obj, shape->slot(), value);
        current->add(store);
        current->push(value);
        if (needsBarrier)
            store->setNeedsBarrier();
        return resumeAfter(store);
    }

    MSlots *slots = MSlots::New(obj);
    current->add(slots);

    MStoreSlot *store = MStoreSlot::New(slots, shape->slot() - shape->numFixedSlots(), value);
    current->add(store);
    current->push(value);
    if (needsBarrier)
        store->setNeedsBarrier();
    return resumeAfter(store);
}

bool
IonBuilder::jsop_getprop(HandlePropertyName name)
{
    RootedId id(cx, NameToId(name));

    types::StackTypeSet *barrier = oracle->propertyReadBarrier(script_, pc);
    types::StackTypeSet *types = oracle->propertyRead(script_, pc);
    TypeOracle::Unary unary = oracle->unaryOp(script_, pc);
    TypeOracle::UnaryTypes uTypes = oracle->unaryTypes(script_, pc);

    bool emitted = false;

    
    if (!getPropTryArgumentsLength(&emitted) || emitted)
        return emitted;

    
    if (!getPropTryConstant(&emitted, id, barrier, types, uTypes) || emitted)
        return emitted;

    
    if (!getPropTryDefiniteSlot(&emitted, name, barrier, types, unary, uTypes) || emitted)
        return emitted;

    
    if (!getPropTryCommonGetter(&emitted, id, barrier, types, uTypes) || emitted)
        return emitted;

    
    if (!getPropTryMonomorphic(&emitted, id, barrier, unary, uTypes) || emitted)
        return emitted;

    
    if (!getPropTryPolymorphic(&emitted, name, id, barrier, types, unary, uTypes) || emitted)
        return emitted;

    
    MDefinition *obj = current->pop();
    MCallGetProperty *call = MCallGetProperty::New(obj, name);
    current->add(call);
    current->push(call);
    if (!resumeAfter(call))
        return false;

    monitorResult(call, barrier, types);
    return pushTypeBarrier(call, types, barrier);
}

bool
IonBuilder::getPropTryArgumentsLength(bool *emitted)
{
    JS_ASSERT(*emitted == false);
    LazyArgumentsType isArguments = oracle->propertyReadMagicArguments(script_, pc);

    if (isArguments == MaybeArguments)
        return abort("Type is not definitely lazy arguments.");
    if (isArguments != DefinitelyArguments)
        return true;
    if (JSOp(*pc) != JSOP_LENGTH)
        return true;

    *emitted = true;
    return jsop_arguments_length();
}

bool
IonBuilder::getPropTryConstant(bool *emitted, HandleId id, types::StackTypeSet *barrier,
                               types::StackTypeSet *types, TypeOracle::UnaryTypes unaryTypes)
{
    JS_ASSERT(*emitted == false);
    JSObject *singleton = types ? types->getSingleton() : NULL;
    if (!singleton || barrier)
        return true;

    RootedObject global(cx, &script_->global());

    bool isConstant, testObject, testString;
    if (!TestSingletonPropertyTypes(cx, unaryTypes.inTypes, global, id,
                                    &isConstant, &testObject, &testString))
        return false;

    if (!isConstant)
        return true;

    MDefinition *obj = current->pop();

    
    JS_ASSERT(!testString || !testObject);
    if (testObject)
        current->add(MGuardObject::New(obj));
    else if (testString)
        current->add(MGuardString::New(obj));
    else
        obj->setFoldedUnchecked();

    MConstant *known = MConstant::New(ObjectValue(*singleton));
    if (singleton->isFunction()) {
        RootedFunction singletonFunc(cx, singleton->toFunction());
        if (TestAreKnownDOMTypes(cx, unaryTypes.inTypes) &&
            TestShouldDOMCall(cx, unaryTypes.inTypes, singletonFunc))
        {
            FreezeDOMTypes(cx, unaryTypes.inTypes);
            known->setDOMFunction();
        }
    }

    current->add(known);
    current->push(known);

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryDefiniteSlot(bool *emitted, HandlePropertyName name,
                                   types::StackTypeSet *barrier, types::StackTypeSet *types,
                                   TypeOracle::Unary unary, TypeOracle::UnaryTypes unaryTypes)
{
    JS_ASSERT(*emitted == false);
    types::TypeSet *propTypes = GetDefiniteSlot(cx, unaryTypes.inTypes, name);
    if (!propTypes)
        return true;

    MDefinition *obj = current->pop();
    MDefinition *useObj = obj;
    if (unaryTypes.inTypes && unaryTypes.inTypes->baseFlags()) {
        MGuardObject *guard = MGuardObject::New(obj);
        current->add(guard);
        useObj = guard;
    }

    MLoadFixedSlot *fixed = MLoadFixedSlot::New(useObj, propTypes->definiteSlot());
    if (!barrier)
        fixed->setResultType(unary.rval);

    current->add(fixed);
    current->push(fixed);

    if (!pushTypeBarrier(fixed, types, barrier))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryCommonGetter(bool *emitted, HandleId id, types::StackTypeSet *barrier,
                                   types::StackTypeSet *types, TypeOracle::UnaryTypes unaryTypes)
{
    JS_ASSERT(*emitted == false);
    JSFunction *commonGetter;
    bool isDOM;
    MDefinition *guard;

    if (!TestCommonPropFunc(cx, unaryTypes.inTypes, id, &commonGetter, true,
                            &isDOM, &guard))
    {
        return false;
    }
    if (!commonGetter)
        return true;

    MDefinition *obj = current->pop();
    RootedFunction getter(cx, commonGetter);

    if (isDOM && TestShouldDOMCall(cx, unaryTypes.inTypes, getter)) {
        const JSJitInfo *jitinfo = getter->jitInfo();
        MGetDOMProperty *get = MGetDOMProperty::New(jitinfo, obj, guard);
        current->add(get);
        current->push(get);

        if (get->isEffectful() && !resumeAfter(get))
            return false;
        if (!pushTypeBarrier(get, types, barrier))
            return false;

        *emitted = true;
        return true;
    }

    
    pushConstant(ObjectValue(*commonGetter));

    MPassArg *wrapper = MPassArg::New(obj);
    current->add(wrapper);
    current->push(wrapper);

    if (!makeCallBarrier(getter, 0, false, types, barrier))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryMonomorphic(bool *emitted, HandleId id, types::StackTypeSet *barrier,
                                  TypeOracle::Unary unary, TypeOracle::UnaryTypes unaryTypes)
{
    JS_ASSERT(*emitted == false);
    bool accessGetter = oracle->propertyReadAccessGetter(script_, pc);

    if (unary.ival != MIRType_Object)
        return true;

    Shape *objShape = mjit::GetPICSingleShape(cx, script_, pc, info().constructing());
    if (!objShape || objShape->inDictionary()) {
        spew("GETPROP not monomorphic");
        return true;
    }

    MDefinition *obj = current->pop();

    
    
    
    
    obj = addShapeGuard(obj, objShape, Bailout_CachedShapeGuard);

    spew("Inlining monomorphic GETPROP");
    Shape *shape = objShape->search(cx, id);
    JS_ASSERT(shape);

    MIRType rvalType = unary.rval;
    if (barrier || IsNullOrUndefined(unary.rval) || accessGetter)
        rvalType = MIRType_Value;

    if (!loadSlot(obj, shape, rvalType))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::getPropTryPolymorphic(bool *emitted, HandlePropertyName name, HandleId id,
                                  types::StackTypeSet *barrier, types::StackTypeSet *types,
                                  TypeOracle::Unary unary, TypeOracle::UnaryTypes unaryTypes)
{
    JS_ASSERT(*emitted == false);
    bool accessGetter = oracle->propertyReadAccessGetter(script_, pc);

    
    
    if (unary.ival != MIRType_Object && !unaryTypes.inTypes->objectOrSentinel())
        return true;

    MIRType rvalType = unary.rval;
    if (barrier || IsNullOrUndefined(unary.rval) || accessGetter)
        rvalType = MIRType_Value;

    MDefinition *obj = current->pop();
    MGetPropertyCache *load = MGetPropertyCache::New(obj, name);
    load->setResultType(rvalType);

    
    
    
    if (unary.ival == MIRType_Object &&
        (cx->methodJitEnabled || js_IonOptions.eagerCompilation) &&
        !invalidatedIdempotentCache())
    {
        if (oracle->propertyReadIdempotent(script_, pc, id))
            load->setIdempotent();
    }

    if (JSOp(*pc) == JSOP_CALLPROP) {
        if (!annotateGetPropertyCache(cx, obj, load, unaryTypes.inTypes, types))
            return false;
    }

    
    if (accessGetter)
        load->setAllowGetters();

    current->add(load);
    current->push(load);

    if (load->isEffectful() && !resumeAfter(load))
        return false;

    if (accessGetter)
        monitorResult(load, barrier, types);

    if (!pushTypeBarrier(load, types, barrier))
        return false;

    *emitted = true;
    return true;
}

bool
IonBuilder::jsop_setprop(HandlePropertyName name)
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->pop();

    bool monitored = !oracle->propertyWriteCanSpecialize(script_, pc);

    TypeOracle::BinaryTypes binaryTypes = oracle->binaryTypes(script_, pc);

    if (!monitored) {
        if (types::HeapTypeSet *propTypes = GetDefiniteSlot(cx, binaryTypes.lhsTypes, name)) {
            MStoreFixedSlot *fixed = MStoreFixedSlot::New(obj, propTypes->definiteSlot(), value);
            current->add(fixed);
            current->push(value);
            if (propTypes->needsBarrier(cx))
                fixed->setNeedsBarrier();
            return resumeAfter(fixed);
        }
    }

    RootedId id(cx, NameToId(name));
    types::StackTypeSet *types = binaryTypes.lhsTypes;

    JSFunction *commonSetter;
    bool isDOM;
    if (!TestCommonPropFunc(cx, types, id, &commonSetter, false, &isDOM, NULL))
        return false;
    if (!monitored && commonSetter) {
        RootedFunction setter(cx, commonSetter);
        if (isDOM && TestShouldDOMCall(cx, types, setter)) {
            MSetDOMProperty *set = MSetDOMProperty::New(setter->jitInfo()->op, obj, value);
            if (!set)
                return false;

            current->add(set);
            current->push(value);

            return resumeAfter(set);
        }

        
        pushConstant(ObjectValue(*setter));

        MPassArg *wrapper = MPassArg::New(obj);
        current->push(wrapper);
        current->add(wrapper);

        MPassArg *arg = MPassArg::New(value);
        current->push(arg);
        current->add(arg);

        
        
        MCall *call = makeCallHelper(setter, 1, false);
        if (!call)
            return false;

        current->push(value);
        return resumeAfter(call);
    }

    oracle->binaryOp(script_, pc);

    MSetPropertyInstruction *ins;
    if (monitored) {
        ins = MCallSetProperty::New(obj, value, name, script_->strictModeCode);
    } else {
        Shape *objShape;
        if ((objShape = mjit::GetPICSingleShape(cx, script_, pc, info().constructing())) &&
            !objShape->inDictionary())
        {
            
            
            
            
            obj = addShapeGuard(obj, objShape, Bailout_CachedShapeGuard);

            Shape *shape = objShape->search(cx, NameToId(name));
            JS_ASSERT(shape);

            spew("Inlining monomorphic SETPROP");

            jsid typeId = types::MakeTypeId(cx, id);
            bool needsBarrier = oracle->propertyWriteNeedsBarrier(script_, pc, typeId);

            return storeSlot(obj, shape, value, needsBarrier);
        }

        spew("SETPROP not monomorphic");

        ins = MSetPropertyCache::New(obj, value, name, script_->strictModeCode);

        if (!binaryTypes.lhsTypes || binaryTypes.lhsTypes->propertyNeedsBarrier(cx, id))
            ins->setNeedsBarrier();
    }

    current->add(ins);
    current->push(value);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_delprop(HandlePropertyName name)
{
    MDefinition *obj = current->pop();

    MInstruction *ins = MDeleteProperty::New(obj, name);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_regexp(RegExpObject *reobj)
{
    JSObject *prototype = script_->global().getOrCreateRegExpPrototype(cx);
    if (!prototype)
        return false;

    MRegExp *ins = MRegExp::New(reobj, prototype, MRegExp::MustClone);
    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_object(JSObject *obj)
{
    MConstant *ins = MConstant::New(ObjectValue(*obj));
    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_lambda(JSFunction *fun)
{
    JS_ASSERT(script_->analysis()->usesScopeChain());
    MLambda *ins = MLambda::New(current->scopeChain(), fun);
    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_deflocalfun(uint32_t local, JSFunction *fun)
{
    JS_ASSERT(script_->analysis()->usesScopeChain());

    MLambda *ins = MLambda::New(current->scopeChain(), fun);
    current->add(ins);
    current->push(ins);

    current->setLocal(local);
    current->pop();

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_defvar(uint32_t index)
{
    JS_ASSERT(JSOp(*pc) == JSOP_DEFVAR || JSOp(*pc) == JSOP_DEFCONST);

    PropertyName *name = script_->getName(index);

    
    unsigned attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT;
    if (JSOp(*pc) == JSOP_DEFCONST)
        attrs |= JSPROP_READONLY;

    
    JS_ASSERT(script_->analysis()->usesScopeChain());

    
    MDefVar *defvar = MDefVar::New(name, attrs, current->scopeChain());
    current->add(defvar);

    return resumeAfter(defvar);
}

bool
IonBuilder::jsop_this()
{
    if (!info().fun())
        return abort("JSOP_THIS outside of a JSFunction.");

    if (script_->strictModeCode) {
        current->pushSlot(info().thisSlot());
        return true;
    }

    types::StackTypeSet *types = oracle->thisTypeSet(script_);
    if (types && types->getKnownTypeTag() == JSVAL_TYPE_OBJECT) {
        
        
        
        current->pushSlot(info().thisSlot());
        return true;
    }

    return abort("JSOP_THIS hard case not yet handled");
}

bool
IonBuilder::jsop_typeof()
{
    TypeOracle::Unary unary = oracle->unaryOp(script_, pc);

    MDefinition *input = current->pop();
    MTypeOf *ins = MTypeOf::New(input, unary.ival);

    current->add(ins);
    current->push(ins);

    if (ins->isEffectful() && !resumeAfter(ins))
        return false;
    return true;
}

bool
IonBuilder::jsop_toid()
{
    
    TypeOracle::Unary unary = oracle->unaryOp(script_, pc);
    if (unary.ival == MIRType_Int32)
        return true;

    MDefinition *index = current->pop();
    MToId *ins = MToId::New(current->peek(-1), index);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_iter(uint8_t flags)
{
    MDefinition *obj = current->pop();
    MInstruction *ins = MIteratorStart::New(obj, flags);

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
    MInstruction *ins = MIteratorNext::New(iter);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_itermore()
{
    MDefinition *iter = current->peek(-1);
    MInstruction *ins = MIteratorMore::New(iter);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_iterend()
{
    MDefinition *iter = current->pop();
    MInstruction *ins = MIteratorEnd::New(iter);

    current->add(ins);

    return resumeAfter(ins);
}

MDefinition *
IonBuilder::walkScopeChain(unsigned hops)
{
    MDefinition *scope = current->getSlot(info().scopeChainSlot());

    for (unsigned i = 0; i < hops; i++) {
        MInstruction *ins = MEnclosingScope::New(scope);
        current->add(ins);
        scope = ins;
    }

    return scope;
}

bool
IonBuilder::jsop_getaliasedvar(ScopeCoordinate sc)
{
    types::StackTypeSet *barrier;
    types::StackTypeSet *actual = oracle->aliasedVarBarrier(script_, pc, &barrier);

    MDefinition *obj = walkScopeChain(sc.hops);

    RootedShape shape(cx, ScopeCoordinateToStaticScope(script_, pc).scopeShape());

    MInstruction *load;
    if (shape->numFixedSlots() <= sc.slot) {
        MInstruction *slots = MSlots::New(obj);
        current->add(slots);

        load = MLoadSlot::New(slots, sc.slot - shape->numFixedSlots());
    } else {
        load = MLoadFixedSlot::New(obj, sc.slot);
    }

    if (!barrier) {
        JSValueType type = actual->getKnownTypeTag();
        if (type != JSVAL_TYPE_UNKNOWN &&
            type != JSVAL_TYPE_UNDEFINED &&
            type != JSVAL_TYPE_NULL)
        {
            load->setResultType(MIRTypeFromValueType(type));
        }
    }

    current->add(load);
    current->push(load);

    return pushTypeBarrier(load, actual, barrier);
}

bool
IonBuilder::jsop_setaliasedvar(ScopeCoordinate sc)
{
    MDefinition *rval = current->peek(-1);
    MDefinition *obj = walkScopeChain(sc.hops);

    RootedShape shape(cx, ScopeCoordinateToStaticScope(script_, pc).scopeShape());

    MInstruction *store;
    if (shape->numFixedSlots() <= sc.slot) {
        MInstruction *slots = MSlots::New(obj);
        current->add(slots);

        store = MStoreSlot::NewBarriered(slots, sc.slot - shape->numFixedSlots(), rval);
    } else {
        store = MStoreFixedSlot::NewBarriered(obj, sc.slot, rval);
    }

    current->add(store);
    return resumeAfter(store);
}

bool
IonBuilder::jsop_in()
{
    if (oracle->inObjectIsDenseArray(script_, pc))
        return jsop_in_dense();

    MDefinition *obj = current->pop();
    MDefinition *id = current->pop();
    MIn *ins = new MIn(id, obj);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_in_dense()
{
    if (oracle->arrayPrototypeHasIndexedProperty())
        return abort("JSOP_IN Array proto has indexed properties");

    bool needsHoleCheck = !oracle->inArrayIsPacked(script_, pc);

    MDefinition *obj = current->pop();
    MDefinition *id = current->pop();

    
    MInstruction *idInt32 = MToInt32::New(id);
    current->add(idInt32);
    id = idInt32;

    
    MElements *elements = MElements::New(obj);
    current->add(elements);

    MInitializedLength *initLength = MInitializedLength::New(elements);
    current->add(initLength);

    
    MInArray *ins = MInArray::New(elements, id, initLength, needsHoleCheck);

    current->add(ins);
    current->push(ins);

    return true;
}

bool
IonBuilder::jsop_instanceof()
{
    MDefinition *rhs = current->pop();
    MDefinition *obj = current->pop();

    TypeOracle::BinaryTypes types = oracle->binaryTypes(script_, pc);

    
    
    do {
        RawObject rhsObject = types.rhsTypes ? types.rhsTypes->getSingleton() : NULL;
        if (!rhsObject || !rhsObject->isFunction() || rhsObject->isBoundFunction())
            break;

        types::TypeObject *rhsType = rhsObject->getType(cx);
        if (!rhsType || rhsType->unknownProperties())
            break;

        types::HeapTypeSet *protoTypes =
            rhsType->getProperty(cx, NameToId(cx->names().classPrototype), false);
        RawObject protoObject = protoTypes ? protoTypes->getSingleton(cx) : NULL;
        if (!protoObject)
            break;

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
IonBuilder::addBoundsCheck(MDefinition *index, MDefinition *length)
{
    MInstruction *check = MBoundsCheck::New(index, length);
    current->add(check);

    
    if (failedBoundsCheck_)
        check->setNotMovable();

    return check;
}

MInstruction *
IonBuilder::addShapeGuard(MDefinition *obj, const Shape *shape, BailoutKind bailoutKind)
{
    MGuardShape *guard = MGuardShape::New(obj, shape, bailoutKind);
    current->add(guard);

    
    if (failedShapeGuard_)
        guard->setNotMovable();

    return guard;
}

const types::TypeSet *
IonBuilder::cloneTypeSet(const types::TypeSet *types)
{
    if (!js_IonOptions.parallelCompilation)
        return types;

    
    
    
    
    return types->clone(GetIonContext()->temp->lifoAlloc());
}
