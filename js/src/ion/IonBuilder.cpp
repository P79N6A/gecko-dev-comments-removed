








































#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonAnalysis.h"
#include "IonSpewer.h"
#include "frontend/BytecodeEmitter.h"
#include "jsscriptinlines.h"

#ifdef JS_THREADSAFE
# include "prthread.h"
#endif

using namespace js;
using namespace js::ion;

IonBuilder::IonBuilder(JSContext *cx, HandleObject scopeChain, TempAllocator &temp, MIRGraph &graph,
                       TypeOracle *oracle, CompileInfo &info, size_t inliningDepth, uint32 loopDepth)
  : MIRGenerator(cx, temp, graph, info),
    script(info.script()),
    initialScopeChain_(scopeChain),
    loopDepth_(loopDepth),
    callerResumePoint_(NULL),
    callerBuilder_(NULL),
    oracle(oracle),
    inliningDepth(inliningDepth)
{
    pc = info.startPC();
}

bool
IonBuilder::abort(const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    abortFmt(message, ap);
    va_end(ap);
    IonSpew(IonSpew_Abort, "aborted @ %s:%d", script->filename, PCToLineNumber(script, pc));
    return false;
}

static inline int32
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
IonBuilder::getSingleCallTarget(uint32 argc, jsbytecode *pc)
{
    types::TypeSet *calleeTypes = oracle->getCallTarget(script, argc, pc);
    if (!calleeTypes)
        return NULL;

    JSObject *obj = calleeTypes->getSingleton(cx, false);
    if (!obj || !obj->isFunction())
        return NULL;

    return obj->toFunction();
}

unsigned
IonBuilder::getPolyCallTargets(uint32 argc, jsbytecode *pc,
                               AutoObjectVector &targets, uint32_t maxTargets)
{
    types::TypeSet *calleeTypes = oracle->getCallTarget(script, argc, pc);
    if (!calleeTypes)
        return 0;

    unsigned objCount = calleeTypes->getObjectCount();
    if (calleeTypes->baseFlags() != 0)
        return 0;

    if (objCount == 0 || objCount > maxTargets)
        return 0;

    for(unsigned i = 0; i < objCount; i++) {
        JSObject *obj = calleeTypes->getSingleObject(i);
        if (!obj || !obj->isFunction())
            return 0;
        targets.append(obj);
    }

    








    return objCount;
}

bool
IonBuilder::canInlineTarget(JSFunction *target)
{
    if (!target->isInterpreted()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to non-interpreted");
        return false;
    }

    if (target->getParent() != script->global()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to scope mismatch");
        return false;
    }

    JSScript *inlineScript = target->script();

    if (!inlineScript->canIonCompile()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to disable Ion compilation");
        return false;
    }

    
    IonBuilder *builder = callerBuilder_;
    while (builder) {
        if (builder->script == inlineScript) {
            IonSpew(IonSpew_Inlining, "Not inlining recursive call");
            return false;
        }
        builder = builder->callerBuilder_;
    }

    bool canInline = oracle->canEnterInlinedFunction(target);

    if (!canInline) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to oracle veto");
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

    IonSpew(IonSpew_MIR, "Analyzing script %s:%d (%p)",
            script->filename, script->lineno, (void *) script);

    if (!initParameters())
        return false;

    
    for (uint32 i = 0; i < info().nlocals(); i++) {
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

    
    
    rewriteParameters();

    
    if (!initScopeChain())
        return false;

    
    MCheckOverRecursed *check = new MCheckOverRecursed;
    current->add(check);
    check->setResumePoint(current->entryResumePoint());

    
    if (info().fun())
        current->getSlot(info().thisSlot())->setGuard();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (uint32 i = 0; i < CountArgSlots(info().fun()); i++) {
        MInstruction *ins = current->getEntrySlot(i)->toInstruction();
        if (ins->type() == MIRType_Value)
            ins->setResumePoint(current->entryResumePoint());
    }

    
    insertRecompileCheck();

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
        phi->setHasBytecodeUses();

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
                        MDefinition *thisDefn, MDefinitionVector &argv, int polymorphic)
{
    IonSpew(IonSpew_MIR, "Inlining script %s:%d (%p)",
            script->filename, script->lineno, (void *)script);

    callerBuilder_ = callerBuilder;
    callerResumePoint_ = callerResumePoint;

    
    current = newBlock(pc);
    if (!current)
        return false;

    current->setCallerResumePoint(callerResumePoint);

    
    MBasicBlock *predecessor = callerBuilder->current;
    if (polymorphic == 0) {
        JS_ASSERT(predecessor == callerResumePoint->block());
        predecessor->end(MGoto::New(current));
    } else if (polymorphic == 1) {
        predecessor->end(MInlineFunctionGuard::New(NULL, NULL, current, NULL));
    } else {
        JS_ASSERT(polymorphic == 2);
        
        JS_ASSERT(predecessor->lastIns() && predecessor->lastIns()->isInlineFunctionGuard());
        MInlineFunctionGuard *guardIns = predecessor->lastIns()->toInlineFunctionGuard();
        guardIns->setFallbackBlock(current);
    }
    if (!current->addPredecessorWithoutPhis(predecessor))
        return false;

#ifdef DEBUG
    if(polymorphic == 0) {
        JS_ASSERT(predecessor->numSuccessors() == 1);
    } else {
        JS_ASSERT(predecessor->numSuccessors() == 2);
    }
#endif

    JS_ASSERT(current->numPredecessors() == 1);

    
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

    
    JS_ASSERT(!script->analysis()->usesScopeChain());
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

    
    for (uint32 i = 0; i < info().nlocals(); i++) {
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
    static const uint32 START_SLOT = 1;

    for (uint32 i = START_SLOT; i < CountArgSlots(info().fun()); i++) {
        MParameter *param = current->getSlot(i)->toParameter();
        types::TypeSet *types = param->typeSet();
        if (!types)
            continue;

        JSValueType definiteType = types->getKnownTypeTag(cx);
        if (definiteType == JSVAL_TYPE_UNKNOWN)
            continue;

        MInstruction *actual = NULL;
        switch (definiteType) {
          case JSVAL_TYPE_UNDEFINED:
            actual = MConstant::New(UndefinedValue());
            break;

          case JSVAL_TYPE_NULL:
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
                                        oracle->thisTypeSet(script));
    current->add(param);
    current->initSlot(info().thisSlot(), param);

    for (uint32 i = 0; i < info().nargs(); i++) {
        param = MParameter::New(i, oracle->parameterTypeSet(script, i));
        current->add(param);
        current->initSlot(info().argSlot(i), param);
    }

    return true;
}

bool
IonBuilder::initScopeChain()
{
    MInstruction *scope = NULL;

    
    
    if (!script->analysis()->usesScopeChain())
        return true;

    
    
    
    
    if (!script->compileAndGo)
        return abort("non-CNG global scripts are not supported");

    if (JSFunction *fun = info().fun()) {
        MCallee *callee = MCallee::New();
        current->add(callee);

        scope = MFunctionEnvironment::New(callee);
        current->add(scope);

        if (fun->isHeavyweight()) {
            
            if (js_IsNamedLambda(fun))
                return abort("DeclEnv scope objects are not yet supported");

            scope = createCallObject(callee, scope);
            if (!scope)
                return false;
        }
    } else {
        scope = MConstant::New(ObjectValue(*initialScopeChain_));
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
        markPhiBytecodeUses(pc);
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

void
IonBuilder::markPhiBytecodeUses(jsbytecode *pc)
{
    unsigned nuses = analyze::GetUseCount(script, pc - script->code);
    for (unsigned i = 0; i < nuses; i++) {
        MDefinition *def = current->peek(-(i + 1));
        if (def->isPassArg())
            def = def->toPassArg()->getArgument();
        if (def->isPhi())
            def->toPhi()->setHasBytecodeUses();
    }
}

bool
IonBuilder::inspectOpcode(JSOp op)
{
    
    if (js_CodeSpec[op].format & JOF_DECOMPOSE)
        return true;

    switch (op) {
      case JSOP_LOOPENTRY:
        return true;

      case JSOP_NOP:
        return true;

      case JSOP_LABEL:
        return true;

      case JSOP_UNDEFINED:
        return pushConstant(UndefinedValue());

      case JSOP_IFEQ:
        return jsop_ifeq(JSOP_IFEQ);

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

      case JSOP_LOCALINC:
      case JSOP_INCLOCAL:
      case JSOP_LOCALDEC:
      case JSOP_DECLOCAL:
        return jsop_localinc(op);

      case JSOP_EQ:
      case JSOP_NE:
      case JSOP_STRICTEQ:
      case JSOP_STRICTNE:
      case JSOP_LT:
      case JSOP_LE:
      case JSOP_GT:
      case JSOP_GE:
        return jsop_compare(op);

      case JSOP_ARGINC:
      case JSOP_INCARG:
      case JSOP_ARGDEC:
      case JSOP_DECARG:
        return jsop_arginc(op);

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
        return true;

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

      case JSOP_CALL:
      case JSOP_NEW:
      case JSOP_FUNAPPLY:
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
        return pushConstant(ObjectValue(*script->global()));

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
        return jsop_delprop(info().getAtom(pc));

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
        return jsop_iternext(GET_INT8(pc));

      case JSOP_MOREITER:
        return jsop_itermore();

      case JSOP_ENDITER:
        return jsop_iterend();

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
    graph_.moveBlockToEnd(current);
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
    graph_.moveBlockToEnd(current);
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

    
    
    for (MBasicBlockIterator i(graph_.begin(state.loop.entry)); i != graph_.end(); i++) {
        if (i->loopDepth() > loopDepth_)
            i->setLoopDepth(i->loopDepth() - 1);
    }

    
    
    
    current = state.loop.successor;
    if (current) {
        JS_ASSERT(current->loopDepth() == loopDepth_);
        graph_.moveBlockToEnd(current);
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
        graph_.moveBlockToEnd(successor);
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
        return processTableSwitchEnd(state);

    
    MBasicBlock *successor = state.tableswitch.ins->getBlock(state.tableswitch.currentBlock);

    
    
    
    if (current) {
        current->end(MGoto::New(successor));
        successor->addPredecessor(current);

        
        graph_.moveBlockToEnd(successor);
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
IonBuilder::processTableSwitchEnd(CFGState &state)
{
    
    
    
    if (!state.tableswitch.breaks && !current)
        return ControlStatus_Ended;

    
    
    
    MBasicBlock *successor = NULL;
    if (state.tableswitch.breaks)
        successor = createBreakCatchBlock(state.tableswitch.breaks, state.tableswitch.exitpc);
    else
        successor = newBlock(current, state.tableswitch.exitpc);

    if (!successor)
        return ControlStatus_Ended;

    
    
    if (current) {
        current->end(MGoto::New(successor));
        if (state.tableswitch.breaks)
            successor->addPredecessor(current);
    }

    pc = state.tableswitch.exitpc;
    current = successor;
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processNextLookupSwitchCase(CFGState &state)
{
    JS_ASSERT(state.state == CFGState::LOOKUP_SWITCH);

    size_t curBlock = state.lookupswitch.currentBlock;
    IonSpew(IonSpew_MIR, "processNextLookupSwitchCase curBlock=%d", curBlock);
    
    state.lookupswitch.currentBlock = ++curBlock;

    
    if (curBlock >= state.lookupswitch.bodies->length())
        return processLookupSwitchEnd(state);

    
    MBasicBlock *successor = (*state.lookupswitch.bodies)[curBlock];

    
    
    
    if (current) {
        current->end(MGoto::New(successor));
        successor->addPredecessor(current);
    }

    
    graph_.moveBlockToEnd(successor);

    
    
    if (curBlock + 1 < state.lookupswitch.bodies->length())
        state.stopAt = (*state.lookupswitch.bodies)[curBlock + 1]->pc();
    else
        state.stopAt = state.lookupswitch.exitpc;

    current = successor;
    pc = current->pc();
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processLookupSwitchEnd(CFGState &state)
{
    
    
    
    if (!state.lookupswitch.breaks && !current)
        return ControlStatus_Ended;

    
    
    
    MBasicBlock *successor = NULL;
    if (state.lookupswitch.breaks)
        successor = createBreakCatchBlock(state.lookupswitch.breaks, state.lookupswitch.exitpc);
    else
        successor = newBlock(current, state.lookupswitch.exitpc);

    if (!successor)
        return ControlStatus_Ended;

    
    
    if (current) {
        current->end(MGoto::New(successor));
        if (state.lookupswitch.breaks)
            successor->addPredecessor(current);
    }

    pc = state.lookupswitch.exitpc;
    current = successor;
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processAndOrEnd(CFGState &state)
{
    
    
    current->end(MGoto::New(state.branch.ifFalse));

    if (!state.branch.ifFalse->addPredecessor(current))
        return ControlStatus_Error;

    current = state.branch.ifFalse;
    graph_.moveBlockToEnd(current);
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

    JS_ASSERT(state.state == CFGState::TABLE_SWITCH || state.state == CFGState::LOOKUP_SWITCH);

    if (state.state == CFGState::TABLE_SWITCH)
    state.tableswitch.breaks = new DeferredEdge(current, state.tableswitch.breaks);
    else
        state.lookupswitch.breaks = new DeferredEdge(current, state.lookupswitch.breaks);

    current = NULL;
    pc += js_CodeSpec[op].length;
    return processControlEnd();
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
    if (!pushLoop(CFGState::DO_WHILE_LOOP_BODY, conditionpc, header, bodyStart, bodyEnd, exitpc, conditionpc))
        return ControlStatus_Error;

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

    
    graph_.moveBlockToEnd(defaultcase);

    JS_ASSERT(tableswitch->numCases() == (uint32)(high - low + 1));
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
        Value rval = script->getConst(GET_UINT32_INDEX(pc2));
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
    if (!state.lookupswitch.bodies->init(bodyBlocks.length()))
        return ControlStatus_Error;

    
    
    for (size_t i = 0; i < bodyBlocks.length(); i++) {
        (*state.lookupswitch.bodies)[i] = bodyBlocks[i];
    }
    graph_.moveBlockToEnd(bodyBlocks[0]);

    
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
    uint32 lhsSlot = current->stackDepth() - 2;
    uint32 rhsSlot = current->stackDepth() - 1;
    current->pushSlot(lhsSlot);
    current->pushSlot(rhsSlot);
    return true;
}

bool
IonBuilder::jsop_loophead(jsbytecode *pc)
{
    assertValidLoopHeadOp(pc);
    insertRecompileCheck();

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
    ins->infer(oracle->unaryOp(script, pc));

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
    ins->infer(oracle->binaryOp(script, pc));

    current->push(ins);
    if (ins->isEffectful() && !resumeAfter(ins))
        return false;

    return true;
}

bool
IonBuilder::jsop_binary(JSOp op, MDefinition *left, MDefinition *right)
{
    TypeOracle::Binary b = oracle->binaryOp(script, pc);

    if (op == JSOP_ADD && b.rval == MIRType_String &&
        (b.lhs == MIRType_String || b.lhs == MIRType_Int32) &&
        (b.rhs == MIRType_String || b.rhs == MIRType_Int32))
    {
        MConcat *ins = MConcat::New(left, right);
        current->add(ins);
        current->push(ins);
        return true;
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

    TypeOracle::BinaryTypes types = oracle->binaryTypes(script, pc);
    current->add(ins);
    ins->infer(cx, types);
    current->push(ins);

    if (ins->isEffectful())
        return resumeAfter(ins);
    return true;
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
    TypeOracle::Unary types = oracle->unaryOp(script, pc);
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

    
    if (def->type() == MIRType_ArgObj)
        return abort("NYI: escaping of the argument object.");

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
IonBuilder::jsop_call_inline(HandleFunction callee, uint32 argc, bool constructing,
                             MConstant *constFun, MResumePoint *inlineResumePoint,
                             MDefinitionVector &argv, MBasicBlock *bottom,
                             Vector<MDefinition *, 8, IonAllocPolicy> &retvalDefns,
                             int polymorphic)
{
    
    
    

    
    
    CompileInfo *info = cx->tempLifoAlloc().new_<CompileInfo>(
                            callee->script(), callee, (jsbytecode *)NULL);
    if (!info)
        return false;

    MIRGraphExits saveExits;
    AutoAccumulateExits aae(graph(), saveExits);

    TypeInferenceOracle oracle;
    if (!oracle.init(cx, callee->script()))
        return false;

    RootedObject scopeChain(NULL);

    IonBuilder inlineBuilder(cx, scopeChain, temp(), graph(), &oracle,
                             *info, inliningDepth + 1, loopDepth_);

    
    MDefinition *thisDefn = NULL;
    if (constructing) {
        thisDefn = createThis(callee, constFun);
        if (!thisDefn)
            return false;
    } else {
        thisDefn = argv[0];
    }

    
    if (!inlineBuilder.buildInline(this, inlineResumePoint, thisDefn, argv, polymorphic))
        return false;

    MIRGraphExits &exits = *inlineBuilder.graph().exitAccumulator();

    
    
    for (MBasicBlock **it = exits.begin(), **end = exits.end(); it != end; ++it) {
        MBasicBlock *exitBlock = *it;

        MDefinition *rval = exitBlock->lastIns()->toReturn()->getOperand(0);
        exitBlock->discardLastIns();

        
        if (constructing) {
            if (rval->type() == MIRType_Value) {
                MReturnFromCtor *filter = MReturnFromCtor::New(rval, thisDefn);
                rval->block()->add(filter);
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
IonBuilder::makeInliningDecision(AutoObjectVector &targets)
{
    if (inliningDepth >= js_IonOptions.maxInlineDepth)
        return false;

    
    
    
    
    
    
    
    

    uint32_t totalSize = 0;
    uint32_t checkUses = js_IonOptions.usesBeforeInlining;
    bool allFunctionsAreSmall = true;
    for (size_t i = 0; i < targets.length(); i++) {
        JSFunction *target = targets[i]->toFunction();
        if (!target->isInterpreted())
            return false;

        JSScript *script = target->script();
        totalSize += script->length;
        if (totalSize > js_IonOptions.inlineMaxTotalBytecodeLength)
            return false;

        if (script->length > js_IonOptions.smallFunctionMaxBytecodeLength)
            allFunctionsAreSmall = false;
    }
    if (allFunctionsAreSmall)
        checkUses = js_IonOptions.smallFunctionUsesBeforeInlining;

    if (script->getUseCount() < checkUses) {
        IonSpew(IonSpew_Inlining, "Not inlining, caller is not hot");
        return false;
    }

    if (!oracle->canInlineCall(script, pc)) {
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

bool
IonBuilder::inlineScriptedCall(AutoObjectVector &targets, uint32 argc, bool constructing)
{
#ifdef DEBUG
    uint32 origStackDepth = current->stackDepth();
#endif

    IonSpew(IonSpew_Inlining, "Inlinig %d targets", (int) targets.length());
    JS_ASSERT(targets.length() > 0);

    
    MBasicBlock *top = current;

    
    MConstant *constFun = NULL;
    if (targets.length() == 1) {
        JSFunction *singleTarget = targets[0]->toFunction();
        constFun = MConstant::New(ObjectValue(*singleTarget));
        current->add(constFun);
    }

    
    
    MResumePoint *inlineResumePoint =
        MResumePoint::New(top, pc, callerResumePoint_, MResumePoint::Outer);
    if (!inlineResumePoint)
        return false;

    
    JS_ASSERT(argc == GET_ARGC(inlineResumePoint->pc()));

    
    
    
    MDefinitionVector argv;
    if (!discardCallArgs(argc, argv, top))
        return false;

    
    
    MDefinition *funcDefn = NULL;
    if (targets.length() == 1) {
        JS_ASSERT(constFun != NULL);
        inlineResumePoint->replaceOperand(
            inlineResumePoint->numOperands() - (argc + 2), constFun);
        current->pop();
        current->push(constFun);
        funcDefn = constFun;
    } else {
        funcDefn = current->pop();
        current->push(funcDefn);
    }

    
    JS_ASSERT(types::IsInlinableCall(pc));
    jsbytecode *postCall = GetNextPc(pc);
    MBasicBlock *bottom = newBlock(NULL, postCall);
    bottom->setCallerResumePoint(callerResumePoint_);

    Vector<MDefinition *, 8, IonAllocPolicy> retvalDefns;

    
    if (targets.length() == 1) {
        
        RootedFunction target(cx, targets[0]->toFunction());
        if(!jsop_call_inline(target, argc, constructing, constFun, inlineResumePoint,
                             argv, bottom, retvalDefns, 0))
            return false;
    } else {
        
        
        MBasicBlock *entryBlock = top;
        for (size_t i = 0; i < targets.length(); i++) {
            
            current = entryBlock;
            RootedFunction target(cx, targets[i]->toFunction());
            if (!jsop_call_inline(target, argc, constructing, constFun,
                                  inlineResumePoint, argv, bottom, retvalDefns,
                                  (i == targets.length() - 1) ? 2 : 1))
                return false;

            JS_ASSERT(entryBlock->lastIns());
            JS_ASSERT(entryBlock->lastIns()->isInlineFunctionGuard());

            
            if (i < targets.length() - 1) {
                MInlineFunctionGuard *guardIns =
                    entryBlock->lastIns()->toInlineFunctionGuard();
                guardIns->setFunction(target);
                guardIns->setInput(funcDefn);
                JS_ASSERT(guardIns->functionBlock() != NULL);

                if (i < targets.length() - 2) {
                    MBasicBlock *fallbackBlock = newBlock(entryBlock, pc);
                    guardIns->setFallbackBlock(fallbackBlock);
                    entryBlock = fallbackBlock;
                }
            }
#ifdef DEBUG
            if (i == targets.length() - 1) {
                MInlineFunctionGuard *guardIns = entryBlock->lastIns()->toInlineFunctionGuard();
                JS_ASSERT(guardIns->function() != NULL);
                JS_ASSERT(guardIns->input() != NULL);
                JS_ASSERT(guardIns->fallbackBlock() != NULL);
                JS_ASSERT(guardIns->functionBlock() != NULL);
            }
#endif
        }
    }

    graph_.moveBlockToEnd(bottom);

    if (!bottom->inheritNonPredecessor(top))
        return false;

    
    (void) bottom->pop();

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

    
    uint32 retvalSlot = bottom->stackDepth() - 1;
    bottom->entryResumePoint()->replaceOperand(retvalSlot, retvalDefn);

    
    
    
    
    JS_ASSERT(bottom->stackDepth() == origStackDepth - argc - 1);

    current = bottom;
    return true;
}

void
IonBuilder::copyFormalIntoCallObj(MDefinition *callObj, MDefinition *slots, unsigned formal)
{
    
    MDefinition *param = current->getSlot(info().argSlot(formal));
    if (slots->type() == MIRType_Slots)
        current->add(MStoreSlot::New(slots, formal, param));
    else
        current->add(MStoreFixedSlot::New(callObj, CallObject::RESERVED_SLOTS + formal, param));
}

MInstruction *
IonBuilder::createCallObject(MDefinition *callee, MDefinition *scope)
{
    
    
    RootedObject templateObj(cx);
    {
        RootedShape shape(cx, script->bindings.callObjectShape(cx));
        if (!shape)
            return NULL;

        RootedTypeObject type(cx, cx->compartment->getEmptyType(cx));
        if (!type)
            return NULL;
        gc::AllocKind kind = gc::GetGCObjectKind(shape->numFixedSlots());

        HeapSlot *slots;
        if (!PreallocateObjectDynamicSlots(cx, shape, &slots))
            return NULL;

        templateObj = JSObject::create(cx, kind, shape, type, slots);
        if (!templateObj) {
            cx->free_(slots);
            return NULL;
        }
    }

    
    MInstruction *slots;
    if (templateObj->hasDynamicSlots()) {
        size_t nslots = JSObject::dynamicSlotsCount(templateObj->lastProperty()->numFixedSlots(),
                                                    templateObj->lastProperty()->slotSpan());
        slots = MNewSlots::New(nslots);
    } else {
        slots = MConstant::New(NullValue());
    }
    current->add(slots);

    
    
    
    MInstruction *callObj = MNewCallObject::New(templateObj, slots);
    current->add(callObj);

    
    current->add(MStoreFixedSlot::New(callObj, CallObject::calleeSlot(), callee));
    current->add(MStoreFixedSlot::New(callObj, CallObject::enclosingScopeSlot(), scope));

    
    if (script->bindingsAccessedDynamically) {
        for (unsigned slot = 0; slot < info().fun()->nargs; slot++)
            copyFormalIntoCallObj(callObj, slots, slot);
    } else if (unsigned n = script->numClosedArgs()) {
        for (unsigned i = 0; i < n; i++)
            copyFormalIntoCallObj(callObj, slots, script->getClosedArg(i));
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
    
    
    
    
    RootedPropertyName name(cx, cx->runtime->atomState.classPrototypeAtom);
    MCallGetProperty *getProto = MCallGetProperty::New(callee, name);

    
    getProto->markUneffectful();
    current->add(getProto);

    MCreateThis *createThis = MCreateThis::New(callee, getProto, NULL);
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

    jsid protoid = AtomToId(cx->runtime->atomState.classPrototypeAtom);
    types::TypeSet *protoTypes = target->getType(cx)->getProperty(cx, protoid, false);
    if (!protoTypes)
        return NULL;

    return protoTypes->getSingleton(cx, true); 
}

MDefinition *
IonBuilder::createThisScriptedSingleton(HandleFunction target, HandleObject proto, MDefinition *callee)
{
    
    
    types::TypeObject *type = proto->getNewType(cx, target);
    if (!type)
        return NULL;
    if (!types::TypeScript::ThisTypes(target->script())->hasType(types::Type::ObjectType(type)))
        return NULL;

    RootedObject templateObject(cx, js_CreateThisForFunctionWithProto(cx, target, proto));
    if (!templateObject)
        return NULL;

    
    if (templateObject->type()->newScript)
        types::TypeSet::WatchObjectStateChange(cx, templateObject->type());

    MConstant *protoDef = MConstant::New(ObjectValue(*proto));
    current->add(protoDef);

    MCreateThis *createThis = MCreateThis::New(callee, protoDef, templateObject);
    current->add(createThis);

    return createThis;
}

MDefinition *
IonBuilder::createThis(HandleFunction target, MDefinition *callee)
{
    if (target->isNative()) {
        if (!target->isNativeConstructor())
            return NULL;
        return createThisNative();
    }

    MDefinition *createThis = NULL;
    RootedObject proto(cx, getSingletonPrototype(target));

    
    if (proto)
        createThis = createThisScriptedSingleton(target, proto, callee);

    
    if (!createThis)
        createThis = createThisScripted(callee);

    return createThis;
}

bool
IonBuilder::jsop_funcall(uint32 argc)
{
    
    
    
    
    
    

    
    RootedFunction native(cx, getSingleCallTarget(argc, pc));
    if (!native || !native->isNative() || native->native() != &js_fun_call)
        return makeCall(native, argc, false);

    
    types::TypeSet *funTypes = oracle->getCallArg(script, argc, 0, pc);
    RootedObject funobj(cx, (funTypes) ? funTypes->getSingleton(cx, false) : NULL);
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
IonBuilder::jsop_call_fun_barrier(AutoObjectVector &targets, int numTargets,
                                  uint32 argc, 
                                  bool constructing,
                                  types::TypeSet *types,
                                  types::TypeSet *barrier)
{
    
    if (inliningEnabled()) {
        
        if(numTargets == 1 && targets[0]->toFunction()->isNative()) {
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

        if (numTargets > 0) {
            if (makeInliningDecision(targets))
                return inlineScriptedCall(targets, argc, constructing);
        }
    }

    RootedFunction target(cx, numTargets == 1 ? targets[0]->toFunction() : NULL);
    return makeCallBarrier(target, argc, constructing, types, barrier);
}

bool
IonBuilder::jsop_call(uint32 argc, bool constructing)
{
    
    AutoObjectVector targets(cx);
    unsigned numTargets = getPolyCallTargets(argc, pc, targets, 4);
    types::TypeSet *barrier;
    types::TypeSet *types = oracle->returnTypeSet(script, pc, &barrier);
    return jsop_call_fun_barrier(targets, numTargets, argc, constructing, types, barrier);
}

bool
IonBuilder::makeCallBarrier(HandleFunction target, uint32 argc,
                            bool constructing,
                            types::TypeSet *types,
                            types::TypeSet *barrier)
{
    
    

    uint32 targetArgs = argc;

    
    
    if (target && !target->isNative())
        targetArgs = Max<uint32>(target->nargs, argc);

    MCall *call = MCall::New(target, targetArgs + 1, argc, constructing);
    if (!call)
        return false;

    
    
    for (int i = targetArgs; i > (int)argc; i--) {
        JS_ASSERT_IF(target, !target->isNative());
        MConstant *undef = MConstant::New(UndefinedValue());
        current->add(undef);
        MPassArg *pass = MPassArg::New(undef);
        current->add(pass);
        call->addArg(i, pass);
    }

    
    
    for (int32 i = argc; i > 0; i--)
        call->addArg(i, current->pop()->toPassArg());

    
    
    MPrepareCall *start = new MPrepareCall;
    MPassArg *firstArg = current->peek(-1)->toPassArg();
    firstArg->block()->insertBefore(firstArg, start);
    call->initPrepareCall(start);

    MPassArg *thisArg = current->pop()->toPassArg();

    
    if (constructing && target) {
        MDefinition *callee = current->peek(-1);
        MDefinition *create = createThis(target, callee);
        if (!create)
            return abort("Failure inlining constructor for call.");

        MPassArg *newThis = MPassArg::New(create);

        thisArg->block()->discard(thisArg);
        current->add(newThis);
        thisArg = newThis;
    }

    
    call->addArg(0, thisArg);
    call->initFunction(current->pop());

    current->add(call);
    current->push(call);
    if (!resumeAfter(call))
        return false;

    return pushTypeBarrier(call, types, barrier);
}

bool
IonBuilder::makeCall(HandleFunction target, uint32 argc, bool constructing)
{
    types::TypeSet *barrier;
    types::TypeSet *types = oracle->returnTypeSet(script, pc, &barrier);
    return makeCallBarrier(target, argc, constructing, types, barrier);
}

bool
IonBuilder::jsop_incslot(JSOp op, uint32 slot)
{
    int32 amt = (js_CodeSpec[op].format & JOF_INC) ? 1 : -1;
    bool post = !!(js_CodeSpec[op].format & JOF_POST);
    TypeOracle::Binary b = oracle->binaryOp(script, pc);

    
    
    
    
    current->pushSlot(slot);
    MDefinition *value = current->pop();
    MInstruction *lhs;
    if (b.lhs == MIRType_Int32) {
        lhs = MToInt32::New(value);
    } else if (b.lhs == MIRType_Double) {
        lhs = MToDouble::New(value);
    } else {
        
        return abort("INCSLOT non-int/double lhs");
    }
    current->add(lhs);

    
    if (post)
        current->push(lhs);

    MConstant *rhs = MConstant::New(Int32Value(amt));
    current->add(rhs);

    TypeOracle::BinaryTypes types = oracle->binaryTypes(script, pc);
    MAdd *result = MAdd::New(lhs, rhs);
    current->add(result);
    result->infer(cx, types);
    current->push(result);
    current->setSlot(slot);

    if (post)
        current->pop();
    return true;
}

bool
IonBuilder::jsop_localinc(JSOp op)
{
    return jsop_incslot(op, info().localSlot(GET_SLOTNO(pc)));
}

bool
IonBuilder::jsop_arginc(JSOp op)
{
    return jsop_incslot(op, info().argSlot(GET_SLOTNO(pc)));
}

bool
IonBuilder::jsop_compare(JSOp op)
{
    MDefinition *right = current->pop();
    MDefinition *left = current->pop();

    MCompare *ins = MCompare::New(left, right, op);
    current->add(ins);
    current->push(ins);

    ins->infer(cx, oracle->binaryTypes(script, pc));

    if (ins->isEffectful() && !resumeAfter(ins))
        return false;
    return true;
}

bool
IonBuilder::jsop_newarray(uint32 count)
{
    JS_ASSERT(script->hasGlobal());

    types::TypeObject *type = NULL;
    if (!types::UseNewTypeForInitializer(cx, script, pc, JSProto_Array)) {
        type = types::TypeScript::InitObject(cx, script, pc, JSProto_Array);
        if (!type)
            return false;
    }

    MNewArray *ins = new MNewArray(count, type, MNewArray::NewArray_Allocating);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_newobject(HandleObject baseObj)
{
    
    JS_ASSERT(script->hasGlobal());

    types::TypeObject *type = NULL;
    if (!types::UseNewTypeForInitializer(cx, script, pc, JSProto_Object)) {
        type = types::TypeScript::InitObject(cx, script, pc, JSProto_Object);
        if (!type)
            return false;
    }

    MNewObject *ins = MNewObject::New(baseObj, type);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_initelem()
{
    if (oracle->propertyWriteCanSpecialize(script, pc)) {
        if (oracle->elementWriteIsDenseArray(script, pc))
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

bool
IonBuilder::jsop_initprop(HandlePropertyName name)
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->peek(-1);

    RootedObject baseObj(cx, obj->toNewObject()->baseObj());

    if (!oracle->propertyWriteCanSpecialize(script, pc)) {
        
        return abort("INITPROP Monitored initprop");
    }

    
    if (!baseObj) {
        MInitProp *init = MInitProp::New(obj, name, value);
        current->add(init);
        return resumeAfter(init);
    }

    JSObject *holder;
    JSProperty *prop = NULL;
    RootedId id(cx, NameToId(name));
    DebugOnly<bool> res = LookupPropertyWithFlags(cx, baseObj, id,
                                                  JSRESOLVE_QUALIFIED, &holder, &prop);
    JS_ASSERT(res && prop && holder == baseObj);

    Shape *shape = (Shape *)prop;

    if (baseObj->isFixedSlot(shape->slot())) {
        MStoreFixedSlot *store = MStoreFixedSlot::New(obj, shape->slot(), value);
        current->add(store);
        return resumeAfter(store);
    }

    MSlots *slots = MSlots::New(obj);
    current->add(slots);

    MStoreSlot *store = MStoreSlot::New(slots, baseObj->dynamicSlotIndex(shape->slot()), value);
    current->add(store);
    return resumeAfter(store);
}

MBasicBlock *
IonBuilder::addBlock(MBasicBlock *block, uint32 loopDepth)
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
IonBuilder::newBlockAfter(MBasicBlock *at, MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::New(graph(), info(), predecessor, pc, MBasicBlock::NORMAL);
    if (!block)
        return NULL;
    graph_.insertBlockAfter(at, block);
    return block;
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc, uint32 loopDepth)
{
    MBasicBlock *block = MBasicBlock::New(graph(), info(), predecessor, pc, MBasicBlock::NORMAL);
    return addBlock(block, loopDepth);
}

MBasicBlock *
IonBuilder::newOsrPreheader(MBasicBlock *predecessor, jsbytecode *loopEntry)
{
    JS_ASSERT((JSOp)*loopEntry == JSOP_LOOPENTRY);
    JS_ASSERT(loopEntry == info().osrPc());

    
    
    
    MBasicBlock *osrBlock  = newBlockAfter(*graph_.begin(), loopEntry);
    MBasicBlock *preheader = newBlock(predecessor, loopEntry);
    if (!osrBlock || !preheader)
        return NULL;

    MOsrEntry *entry = MOsrEntry::New();
    osrBlock->add(entry);

    
    {
        uint32 slot = info().scopeChainSlot();

        MOsrScopeChain *scopev = MOsrScopeChain::New(entry);
        osrBlock->add(scopev);
        osrBlock->initSlot(slot, scopev);
    }

    if (info().fun()) {
        
        uint32 slot = info().thisSlot();
        ptrdiff_t offset = StackFrame::offsetOfThis(info().fun());

        MOsrValue *thisv = MOsrValue::New(entry, offset);
        osrBlock->add(thisv);
        osrBlock->initSlot(slot, thisv);

        
        for (uint32 i = 0; i < info().nargs(); i++) {
            uint32 slot = info().argSlot(i);
            ptrdiff_t offset = StackFrame::offsetOfFormalArg(info().fun(), i);

            MOsrValue *osrv = MOsrValue::New(entry, offset);
            osrBlock->add(osrv);
            osrBlock->initSlot(slot, osrv);
        }
    }

    
    for (uint32 i = 0; i < info().nlocals(); i++) {
        uint32 slot = info().localSlot(i);
        ptrdiff_t offset = StackFrame::offsetOfFixed(i);

        MOsrValue *osrv = MOsrValue::New(entry, offset);
        osrBlock->add(osrv);
        osrBlock->initSlot(slot, osrv);
    }

    
    uint32 numSlots = preheader->stackDepth() - CountArgSlots(info().fun()) - info().nlocals();
    for (uint32 i = 0; i < numSlots; i++) {
        uint32 slot = info().stackSlot(i);
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

    
    for (uint32 i = 0; i < osrBlock->stackDepth(); i++)
        slotTypes[i] = MIRType_Value;

    
    if (!oracle->getOsrTypes(loopEntry, slotTypes))
        return NULL;

    for (uint32 i = 1; i < osrBlock->stackDepth(); i++) {
        MIRType type = slotTypes[i];
        
        if (type != MIRType_Value && type != MIRType_Undefined && type != MIRType_Null) {
            MDefinition *def = osrBlock->getSlot(i);
            JS_ASSERT(def->type() == MIRType_Value);
            MInstruction *actual = MUnbox::New(def, slotTypes[i], MUnbox::Infallible);
            osrBlock->add(actual);
            osrBlock->rewriteSlot(i, actual);
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
    JS_ASSERT(ins->isEffectful());

    MResumePoint *resumePoint = MResumePoint::New(ins->block(), pc, callerResumePoint_, mode);
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

void
IonBuilder::insertRecompileCheck()
{
    if (!inliningEnabled())
        return;

    if (inliningDepth > 0)
        return;

    
    if (script->getUseCount() >= js_IonOptions.usesBeforeInlining)
        return;

    
    
    if (!oracle->canInlineCalls())
        return;

    MRecompileCheck *check = MRecompileCheck::New();
    current->add(check);
}

static inline bool
TestSingletonProperty(JSContext *cx, JSObject *obj, HandleId id, bool *isKnownConstant)
{
    
    
    
    
    
    
    
    
    
    
    
    

    *isKnownConstant = false;

    JSObject *pobj = obj;
    while (pobj) {
        if (!pobj->isNative())
            return true;
        if (pobj->getClass()->ops.lookupProperty)
            return true;
        pobj = pobj->getProto();
    }

    JSObject *holder;
    JSProperty *prop = NULL;
    if (!obj->lookupGeneric(cx, id, &holder, &prop))
        return false;
    if (!prop)
        return true;

    Shape *shape = (Shape *)prop;
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
TestSingletonPropertyTypes(JSContext *cx, types::TypeSet *types,
                           HandleObject globalObj, HandleId id,
                           bool *isKnownConstant, bool *testObject)
{
    
    
    

    *isKnownConstant = false;
    *testObject = false;

    if (!types || types->unknownObject())
        return true;

    JSObject *singleton = types->getSingleton(cx);
    if (singleton)
        return TestSingletonProperty(cx, singleton, id, isKnownConstant);

    if (!globalObj)
        return true;

    JSProtoKey key;
    JSValueType type = types->getKnownTypeTag(cx);
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
      case JSVAL_TYPE_UNKNOWN:
        
        
        
        if (types->getObjectCount() == 1) {
            types::TypeObject *object = types->getTypeObject(0);
            if (!object)
                return true;
            if (object && object->proto) {
                if (!TestSingletonProperty(cx, object->proto, id, isKnownConstant))
                    return false;
                if (*isKnownConstant) {
                    types->addFreeze(cx);

                    
                    *testObject = (type != JSVAL_TYPE_OBJECT);
                }
                return true;
            }
        }
        return true;

      default:
        return true;
    }

    JSObject *proto;
    if (!js_GetClassPrototype(cx, globalObj, key, &proto, NULL))
        return false;

    return TestSingletonProperty(cx, proto, id, isKnownConstant);
}












bool
IonBuilder::pushTypeBarrier(MInstruction *ins, types::TypeSet *actual, types::TypeSet *observed)
{
    
    
    
    
    

    if (!actual) {
        JS_ASSERT(!observed);
        return true;
    }

    if (!observed) {
        JSValueType type = actual->getKnownTypeTag(cx);
        MInstruction *replace = NULL;
        switch (type) {
          case JSVAL_TYPE_UNDEFINED:
            replace = MConstant::New(UndefinedValue());
            break;
          case JSVAL_TYPE_NULL:
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
    observed->addFreeze(cx);

    MInstruction *barrier;
    JSValueType type = observed->getKnownTypeTag(cx);

    
    
    bool isObject = false;
    if (type == JSVAL_TYPE_OBJECT && !observed->hasType(types::Type::AnyObjectType())) {
        type = JSVAL_TYPE_UNKNOWN;
        isObject = true;
    }

    switch (type) {
      case JSVAL_TYPE_UNKNOWN:
      case JSVAL_TYPE_UNDEFINED:
      case JSVAL_TYPE_NULL:
        barrier = MTypeBarrier::New(ins, observed);
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
IonBuilder::monitorResult(MInstruction *ins, types::TypeSet *types)
{
    if (!types || types->unknown())
        return;

    MInstruction *monitor = MMonitorTypes::New(ins, types);
    current->add(monitor);
}

bool
IonBuilder::jsop_getgname(HandlePropertyName name)
{
    
    if (name == cx->runtime->atomState.typeAtoms[JSTYPE_VOID])
        return pushConstant(UndefinedValue());
    if (name == cx->runtime->atomState.NaNAtom)
        return pushConstant(cx->runtime->NaNValue);
    if (name == cx->runtime->atomState.InfinityAtom)
        return pushConstant(cx->runtime->positiveInfinityValue);

    JSObject *globalObj = script->global();
    JS_ASSERT(globalObj->isNative());

    RootedId id(cx, NameToId(name));

    
    
    const js::Shape *shape = globalObj->nativeLookup(cx, id);
    if (!shape || !shape->hasDefaultGetter() || !shape->hasSlot())
        return jsop_getname(name);

    types::TypeSet *propertyTypes = oracle->globalPropertyTypeSet(script, pc, id);
    if (propertyTypes && propertyTypes->isOwnProperty(cx, globalObj->getType(cx), true)) {
        
        
        return jsop_getname(name);
    }

    
    JSValueType knownType = JSVAL_TYPE_UNKNOWN;

    types::TypeSet *barrier = oracle->propertyReadBarrier(script, pc);
    types::TypeSet *types = oracle->propertyRead(script, pc);
    if (types) {
        JSObject *singleton = types->getSingleton(cx);

        knownType = types->getKnownTypeTag(cx);
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

    
    
    if (!propertyTypes && shape->configurable()) {
        MGuardShape *guard = MGuardShape::New(global, globalObj->lastProperty());
        current->add(guard);
    }

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
    RootedObject globalObj(cx, script->global());
    RootedId id(cx, NameToId(name));

    JS_ASSERT(globalObj->isNative());

    bool canSpecialize;
    types::TypeSet *propertyTypes = oracle->globalPropertyWrite(script, pc, id, &canSpecialize);

    
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

    
    
    
    if (!propertyTypes) {
        MGuardShape *guard = MGuardShape::New(global, globalObj->lastProperty());
        current->add(guard);
    }

    JS_ASSERT(shape->slot() >= globalObj->numFixedSlots());

    MSlots *slots = MSlots::New(global);
    current->add(slots);

    MDefinition *value = current->pop();
    MStoreSlot *store = MStoreSlot::New(slots, shape->slot() - globalObj->numFixedSlots(), value);
    current->add(store);

    
    if (!propertyTypes || propertyTypes->needsBarrier(cx))
        store->setNeedsBarrier(true);

    
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
        MInstruction *global = MConstant::New(ObjectValue(*script->global()));
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

    types::TypeSet *barrier = oracle->propertyReadBarrier(script, pc);
    types::TypeSet *types = oracle->propertyRead(script, pc);

    monitorResult(ins, types);
    return pushTypeBarrier(ins, types, barrier);
}

bool
IonBuilder::jsop_bindname(PropertyName *name)
{
    JS_ASSERT(script->analysis()->usesScopeChain());

    MDefinition *scopeChain = current->scopeChain();
    MBindNameCache *ins = MBindNameCache::New(scopeChain, name, script, pc);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_getelem()
{
    if (oracle->elementReadIsDenseArray(script, pc))
        return jsop_getelem_dense();

    int arrayType = TypedArray::TYPE_MAX;
    if (oracle->elementReadIsTypedArray(script, pc, &arrayType))
        return jsop_getelem_typed(arrayType);

    if (oracle->elementReadIsString(script, pc))
        return jsop_getelem_string();

    LazyArgumentsType isArguments = oracle->elementReadMagicArguments(script, pc);
    if (isArguments == MaybeArguments)
        return abort("Type is not definitely lazy arguments.");
    if (isArguments == DefinitelyArguments)
        return jsop_arguments_getelem();

    MDefinition *rhs = current->pop();
    MDefinition *lhs = current->pop();

    MInstruction *ins;

    
    
    
    
    bool mustMonitorResult = false;
    bool cacheable = false;

    oracle->elementReadGeneric(script, pc, &cacheable, &mustMonitorResult);

    if (cacheable)
        ins = MGetElementCache::New(lhs, rhs, mustMonitorResult);
    else
        ins = MCallGetElement::New(lhs, rhs);

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    types::TypeSet *barrier = oracle->propertyReadBarrier(script, pc);
    types::TypeSet *types = oracle->propertyRead(script, pc);

    if (mustMonitorResult)
        monitorResult(ins, types);
    return pushTypeBarrier(ins, types, barrier);
}

bool
IonBuilder::jsop_getelem_dense()
{
    if (oracle->arrayPrototypeHasIndexedProperty())
        return abort("GETELEM Array proto has indexed properties");

    types::TypeSet *barrier = oracle->propertyReadBarrier(script, pc);
    types::TypeSet *types = oracle->propertyRead(script, pc);
    bool needsHoleCheck = !oracle->elementReadIsPacked(script, pc);
    bool maybeUndefined = types->hasType(types::Type::UndefinedType());

    MDefinition *id = current->pop();
    MDefinition *obj = current->pop();

    JSValueType knownType = JSVAL_TYPE_UNKNOWN;
    if (!needsHoleCheck && !barrier) {
        knownType = types->getKnownTypeTag(cx);

        
        
        
        
        
        if (knownType == JSVAL_TYPE_UNDEFINED || knownType == JSVAL_TYPE_NULL)
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
        
        
        
        
        MBoundsCheck *check = MBoundsCheck::New(id, initLength);
        current->add(check);

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

bool
IonBuilder::jsop_getelem_typed(int arrayType)
{
    types::TypeSet *barrier = oracle->propertyReadBarrier(script, pc);
    types::TypeSet *types = oracle->propertyRead(script, pc);

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

        
        MTypedArrayLength *length = MTypedArrayLength::New(obj);
        current->add(length);

        
        MBoundsCheck *check = MBoundsCheck::New(id, length);
        current->add(check);

        
        MTypedArrayElements *elements = MTypedArrayElements::New(obj);
        current->add(elements);

        
        MLoadTypedArrayElement *load = MLoadTypedArrayElement::New(elements, id, arrayType);
        current->add(load);
        current->push(load);

        load->setResultType(knownType);

        
        
        JS_ASSERT_IF(knownType == MIRType_Int32, types->hasType(types::Type::Int32Type()));
        JS_ASSERT_IF(knownType == MIRType_Double, types->hasType(types::Type::DoubleType()));
        return true;
    } else {
        
        
        
        MLoadTypedArrayElementHole *load = MLoadTypedArrayElementHole::New(obj, id, arrayType, allowDouble);
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

    MBoundsCheck *boundsCheck = MBoundsCheck::New(id, length);
    current->add(boundsCheck);

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
    if (oracle->propertyWriteCanSpecialize(script, pc)) {
        if (oracle->elementWriteIsDenseArray(script, pc))
            return jsop_setelem_dense();

        int arrayType = TypedArray::TYPE_MAX;
        if (oracle->elementWriteIsTypedArray(script, pc, &arrayType))
            return jsop_setelem_typed(arrayType);
    }

    LazyArgumentsType isArguments = oracle->elementWriteMagicArguments(script, pc);
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

    MIRType elementType = oracle->elementWrite(script, pc);
    bool packed = oracle->elementWriteIsPacked(script, pc);

    MDefinition *value = current->pop();
    MDefinition *id = current->pop();
    MDefinition *obj = current->pop();

    
    MInstruction *idInt32 = MToInt32::New(id);
    current->add(idInt32);
    id = idInt32;

    
    MElements *elements = MElements::New(obj);
    current->add(elements);

    
    
    
    MStoreElementCommon *store;
    if (oracle->setElementHasWrittenHoles(script, pc)) {
        MStoreElementHole *ins = MStoreElementHole::New(obj, elements, id, value);
        store = ins;

        current->add(ins);
        current->push(value);

        if (!resumeAfter(ins))
            return false;
    } else {
        MInitializedLength *initLength = MInitializedLength::New(elements);
        current->add(initLength);

        MBoundsCheck *check = MBoundsCheck::New(id, initLength);
        current->add(check);

        MStoreElement *ins = MStoreElement::New(elements, id, value);
        store = ins;

        current->add(ins);
        current->push(value);

        if (!resumeAfter(ins))
            return false;
    }

    
    if (oracle->elementWriteNeedsBarrier(script, pc))
        store->setNeedsBarrier(true);

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

    
    MTypedArrayLength *length = MTypedArrayLength::New(obj);
    current->add(length);

    
    MBoundsCheck *check = MBoundsCheck::New(id, length);
    current->add(check);

    
    MTypedArrayElements *elements = MTypedArrayElements::New(obj);
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
    TypeOracle::UnaryTypes sig = oracle->unaryTypes(script, pc);
    if (!sig.inTypes || !sig.outTypes)
        return false;

    if (sig.outTypes->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
        return false;

    switch (sig.inTypes->getKnownTypeTag(cx)) {
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
            MTypedArrayLength *length = MTypedArrayLength::New(obj);
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
    MInstruction *ins = MLazyArguments::New();
    current->add(ins);
    current->push(ins);
    return true;
}

bool
IonBuilder::jsop_arguments_length()
{
    MDefinition *args = current->pop();
    MInstruction *ins = MArgumentsLength::New(args);
    current->add(ins);
    current->push(ins);
    return true;
}

bool
IonBuilder::jsop_arguments_getelem()
{
    types::TypeSet *barrier = oracle->propertyReadBarrier(script, pc);
    types::TypeSet *types = oracle->propertyRead(script, pc);

    MDefinition *idx = current->pop();
    MDefinition *args = current->pop();

    
    MArgumentsLength *length = MArgumentsLength::New(args);
    current->add(length);

    
    MToInt32 *index = MToInt32::New(idx);
    current->add(index);

    
    MBoundsCheck *check = MBoundsCheck::New(index, length);
    current->add(check);

    
    MGetArgument *load = MGetArgument::New(index);
    current->add(load);
    current->push(load);

    return pushTypeBarrier(load, types, barrier);
}

bool
IonBuilder::jsop_arguments_setelem()
{
    MDefinition *val = current->pop();
    MDefinition *idx = current->pop();
    MDefinition *obj = current->pop();
    return abort("NYI arguments[]=");
}

inline types::TypeSet *
GetDefiniteSlot(JSContext *cx, types::TypeSet *types, JSAtom *atom)
{
    if (!types || types->unknownObject() || types->getObjectCount() != 1)
        return NULL;

    types::TypeObject *type = types->getTypeObject(0);
    if (!type || type->unknownProperties())
        return NULL;

    jsid id = AtomToId(atom);
    if (id != types::MakeTypeId(cx, id))
        return NULL;

    types::TypeSet *propertyTypes = type->getProperty(cx, id, false);
    if (!propertyTypes ||
        !propertyTypes->isDefiniteProperty() ||
        propertyTypes->isOwnProperty(cx, type, true))
    {
        return NULL;
    }

    types->addFreeze(cx);
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
IonBuilder::TestCommonPropFunc(JSContext *cx, types::TypeSet *types, HandleId id,
                   JSFunction **funcp, bool isGetter)
{
    JSObject *found = NULL;
    JSObject *foundProto = NULL;

    *funcp = NULL;

    
    if (!types || types->unknownObject())
        return true;

    
    
    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        JSObject *curObj = types->getSingleObject(i);

        
        if (!curObj) {
            types::TypeObject *typeObj = types->getTypeObject(i);

            if (!typeObj)
                continue;

            if (typeObj->unknownProperties())
                return true;

            
            
            jsid typeId = types::MakeTypeId(cx, id);
            types::TypeSet *propSet = typeObj->getProperty(cx, typeId, false);
            if (!propSet)
                return false;
            if (propSet->isOwnProperty(false))
                return true;

            
            curObj = typeObj->proto;
        }

        
        
        JSObject *walker = curObj;
        while (walker) {
            if (walker->getClass()->ops.lookupProperty)
                return true;
            walker = walker->getProto();
        }

        JSObject *proto;
        JSProperty *prop;

        if (!curObj->lookupGeneric(cx, id, &proto, &prop))
            return false;
        if (!prop)
            return true;

        Shape *shape = (Shape *)prop;

        
        
        if (isGetter) {
            if (shape->hasDefaultGetter() || !shape->hasGetterValue())
                return true;
        } else {
            if (shape->hasDefaultSetter() || !shape->hasSetterValue())
                return true;
        }

        JSObject * curFound = isGetter ? shape->getterObject():
                                         shape->setterObject();

        
        if (!found)
            found = curFound;
        else if (found != curFound)
            return true;

        
        
        
        if (!foundProto)
            foundProto = proto;
        else if (foundProto != proto)
            return true;
    }

    
    if (!found)
        return true;

    JS_ASSERT(foundProto);

    types->addFreeze(cx);

    MInstruction *wrapper = MConstant::New(ObjectValue(*foundProto));
    current->add(wrapper);
    MGuardShape *guard = MGuardShape::New(wrapper, foundProto->lastProperty());
    current->add(guard);

    
    
    types::TypeObject *curType;
    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        curType = types->getTypeObject(i);

        if (!curType) {
            JSObject *obj = types->getSingleObject(i);
            if (!obj)
                continue;

            curType = obj->getType(cx);
        }

        
        
        jsid typeId = types::MakeTypeId(cx, id);
        types::TypeSet *propSet = curType->getProperty(cx, typeId, false);
        JS_ASSERT(propSet);
        while (!propSet->isOwnProperty(cx, curType, false)) {
            curType = curType->proto->getType(cx);
            propSet = curType->getProperty(cx, id, false);
            JS_ASSERT(propSet);
        }
    }

    *funcp = (JSFunction *)found;

    return true;
}

bool
IonBuilder::jsop_getprop(HandlePropertyName name)
{

    LazyArgumentsType isArguments = oracle->propertyReadMagicArguments(script, pc);
    if (isArguments == MaybeArguments)
        return abort("Type is not definitely lazy arguments.");
    if (isArguments == DefinitelyArguments) {
        if (JSOp(*pc) == JSOP_LENGTH)
            return jsop_arguments_length();
        
    }

    MDefinition *obj = current->pop();
    MInstruction *ins;

    types::TypeSet *barrier = oracle->propertyReadBarrier(script, pc);
    types::TypeSet *types = oracle->propertyRead(script, pc);

    TypeOracle::Unary unary = oracle->unaryOp(script, pc);
    TypeOracle::UnaryTypes unaryTypes = oracle->unaryTypes(script, pc);

    RootedId id(cx, NameToId(name));

    JSObject *singleton = types ? types->getSingleton(cx) : NULL;
    if (singleton && !barrier) {
        bool isKnownConstant, testObject;
        RootedObject global(cx, script->global());
        if (!TestSingletonPropertyTypes(cx, unaryTypes.inTypes,
                                        global, id,
                                        &isKnownConstant, &testObject))
        {
            return false;
        }

        if (isKnownConstant) {
            if (testObject) {
                MGuardObject *guard = MGuardObject::New(obj);
                current->add(guard);
            }
            return pushConstant(ObjectValue(*singleton));
        }
    }

    if (types::TypeSet *propTypes = GetDefiniteSlot(cx, unaryTypes.inTypes, name)) {
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

        return pushTypeBarrier(fixed, types, barrier);
    }

    
    JSFunction *commonGetter;
    if (!TestCommonPropFunc(cx, unaryTypes.inTypes, id, &commonGetter, true))
        return false;
    if (commonGetter) {
        
        pushConstant(ObjectValue(*commonGetter));

        MPassArg *wrapper = MPassArg::New(obj);
        current->push(wrapper);
        current->add(wrapper);

        RootedFunction getter(cx, commonGetter);

        return makeCallBarrier(getter, 0, false, types, barrier);
    }

    if (unary.ival == MIRType_Object) {
        MGetPropertyCache *load = MGetPropertyCache::New(obj, name);
        if (!barrier) {
            
            
            if (unary.rval != MIRType_Undefined && unary.rval != MIRType_Null)
                load->setResultType(unary.rval);
        }
        ins = load;
    } else {
        ins = MCallGetProperty::New(obj, name);
    }

    current->add(ins);
    current->push(ins);

    if (!resumeAfter(ins))
        return false;

    if (ins->isCallGetProperty())
        monitorResult(ins, types);
    return pushTypeBarrier(ins, types, barrier);
}

bool
IonBuilder::jsop_setprop(HandlePropertyName name)
{
    MDefinition *value = current->pop();
    MDefinition *obj = current->pop();

    bool monitored = !oracle->propertyWriteCanSpecialize(script, pc);

    TypeOracle::BinaryTypes binaryTypes = oracle->binaryTypes(script, pc);

    if (!monitored) {
        if (types::TypeSet *propTypes = GetDefiniteSlot(cx, binaryTypes.lhsTypes, name)) {
            MStoreFixedSlot *fixed = MStoreFixedSlot::New(obj, propTypes->definiteSlot(), value);
            current->add(fixed);
            current->push(value);
            if (propTypes->needsBarrier(cx))
                fixed->setNeedsBarrier();
            return resumeAfter(fixed);
        }
    }

    RootedId id(cx, NameToId(name));

    JSFunction *commonSetter;
    if (!TestCommonPropFunc(cx, binaryTypes.lhsTypes, id, &commonSetter, false))
        return false;
    if (!monitored && commonSetter) {
        
        pushConstant(ObjectValue(*commonSetter));

        MPassArg *wrapper = MPassArg::New(obj);
        current->push(wrapper);
        current->add(wrapper);

        MPassArg *arg = MPassArg::New(value);
        current->push(arg);
        current->add(arg);

        RootedFunction setter(cx, commonSetter);

        return makeCallBarrier(setter, 1, false, NULL, NULL);
    }

    oracle->binaryOp(script, pc);

    MSetPropertyInstruction *ins;
    if (monitored) {
        ins = MCallSetProperty::New(obj, value, name, script->strictModeCode);
    } else {
        ins = MSetPropertyCache::New(obj, value, name, script->strictModeCode);

        if (!binaryTypes.lhsTypes || binaryTypes.lhsTypes->propertyNeedsBarrier(cx, id))
            ins->setNeedsBarrier();
    }

    current->add(ins);
    current->push(value);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_delprop(JSAtom *atom)
{
    MDefinition *obj = current->pop();

    MInstruction *ins = MDeleteProperty::New(obj, atom);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_regexp(RegExpObject *reobj)
{
    MRegExp *ins = MRegExp::New(reobj, MRegExp::MustClone);
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
    JS_ASSERT(script->analysis()->usesScopeChain());
    MLambda *ins = MLambda::New(current->scopeChain(), fun);
    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_deflocalfun(uint32 local, JSFunction *fun)
{
    JS_ASSERT(script->analysis()->usesScopeChain());

    MLambda *ins = MLambda::New(current->scopeChain(), fun);
    current->add(ins);
    current->push(ins);

    current->setLocal(local);
    current->pop();

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_defvar(uint32 index)
{
    JS_ASSERT(JSOp(*pc) == JSOP_DEFVAR || JSOp(*pc) == JSOP_DEFCONST);

    PropertyName *name = script->getName(index);

    
    unsigned attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT;
    if (JSOp(*pc) == JSOP_DEFCONST)
        attrs |= JSPROP_READONLY;

    
    JS_ASSERT(script->analysis()->usesScopeChain());

    
    MDefVar *defvar = MDefVar::New(name, attrs, current->scopeChain());
    current->add(defvar);

    return resumeAfter(defvar);
}

bool
IonBuilder::jsop_this()
{
    if (!info().fun())
        return abort("JSOP_THIS outside of a JSFunction.");

    if (script->strictModeCode) {
        current->pushSlot(info().thisSlot());
        return true;
    }

    types::TypeSet *types = oracle->thisTypeSet(script);
    if (types && types->getKnownTypeTag(cx) == JSVAL_TYPE_OBJECT) {
        
        
        
        current->pushSlot(info().thisSlot());
        return true;
    }

    return abort("JSOP_THIS hard case not yet handled");
}

bool
IonBuilder::jsop_typeof()
{
    TypeOracle::Unary unary = oracle->unaryOp(script, pc);

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
    
    TypeOracle::Unary unary = oracle->unaryOp(script, pc);
    if (unary.ival == MIRType_Int32)
        return true;

    MDefinition *index = current->pop();
    MToId *ins = MToId::New(current->peek(-1), index);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

bool
IonBuilder::jsop_iter(uint8 flags)
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
IonBuilder::jsop_iternext(uint8 depth)
{
    MDefinition *iter = current->peek(-depth);
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

static inline bool
InDynamicScopeSlots(JSScript *script, jsbytecode *pc, unsigned *slot)
{
    if (ScopeCoordinateBlockChain(script, pc)) {
         
         
         
        uint32 nfixed = gc::GetGCKindSlots(BlockObject::FINALIZE_KIND);
        if (nfixed <= *slot) {
            *slot -= nfixed;
            return true;
        }
    } else {
         
         
         
         if (script->bindings.lastShape()->numFixedSlots() <= *slot) {
             *slot -= ScopeObject::CALL_BLOCK_RESERVED_SLOTS;
             return true;
         }
    }
    return false;
}

bool
IonBuilder::jsop_getaliasedvar(ScopeCoordinate sc)
{
    MIRType type = oracle->aliasedVarType(script, pc);

    if (type == MIRType_Null || type == MIRType_Undefined) {
        if (type == MIRType_Null)
            return pushConstant(NullValue());
        return pushConstant(UndefinedValue());
    }

    MDefinition *obj = walkScopeChain(sc.hops);
    unsigned slot = ScopeObject::CALL_BLOCK_RESERVED_SLOTS + sc.slot;

    MInstruction *load;
    if (InDynamicScopeSlots(script, pc, &slot)) {
        MInstruction *slots = MSlots::New(obj);
        current->add(slots);

        load = MLoadSlot::New(slots, slot);
    } else {
        load = MLoadFixedSlot::New(obj, slot);
    }
    load->setResultType(type);
    current->add(load);
    current->push(load);

    return true;
}

bool
IonBuilder::jsop_setaliasedvar(ScopeCoordinate sc)
{
    MDefinition *rval = current->peek(-1);
    MDefinition *obj = walkScopeChain(sc.hops);
    unsigned slot = ScopeObject::CALL_BLOCK_RESERVED_SLOTS + sc.slot;

    MInstruction *store;
    if (InDynamicScopeSlots(script, pc, &slot)) {
        MInstruction *slots = MSlots::New(obj);
        current->add(slots);
        store = MStoreSlot::NewBarriered(slots, slot, rval);
    } else {
        store = MStoreFixedSlot::NewBarriered(obj, slot, rval);
    }

    current->add(store);
    return resumeAfter(store);
}


bool
IonBuilder::jsop_instanceof()
{
    MDefinition *proto = current->pop();
    MDefinition *obj = current->pop();
    MInstanceOf *ins = new MInstanceOf(obj, proto);

    current->add(ins);
    current->push(ins);

    return resumeAfter(ins);
}

