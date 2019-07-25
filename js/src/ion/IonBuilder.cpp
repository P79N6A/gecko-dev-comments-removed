








































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

IonBuilder::IonBuilder(JSContext *cx, TempAllocator &temp, MIRGraph &graph, TypeOracle *oracle,
                       CompileInfo &info, size_t inliningDepth)
  : MIRGenerator(cx, temp, graph, info),
    script(info.script()),
    lastResumePoint_(NULL),
    callerResumePoint_(NULL),
    oracle(oracle),
    inliningDepth(inliningDepth)
{
    pc = info.startPC();
}

static inline int32
GetJumpOffset(jsbytecode *pc)
{
    JSOp op = JSOp(*pc);
    JS_ASSERT(js_CodeSpec[op].type() == JOF_JUMP ||
              js_CodeSpec[op].type() == JOF_JUMPX);
    return (js_CodeSpec[op].type() == JOF_JUMP)
           ? GET_JUMP_OFFSET(pc)
           : GET_JUMPX_OFFSET(pc);
}

static inline jsbytecode *
GetNextPc(jsbytecode *pc)
{
    return pc + js_CodeSpec[JSOp(*pc)].length;
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

bool
IonBuilder::getInliningTarget(uint32 argc, jsbytecode *pc, JSFunction **out)
{
    *out = NULL;

    types::TypeSet *calleeTypes = oracle->getCallTarget(script, argc, pc);
    if (!calleeTypes) {
        IonSpew(IonSpew_Inlining, "Cannot inline; no types for callee");
        return true;
    }

    if (calleeTypes->getKnownTypeTag(cx) != JSVAL_TYPE_OBJECT) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to non-object");
        return true;
    }

    if (calleeTypes->getObjectCount() > 1) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to multiple objects");
        return true;
    }

    JSObject *obj = calleeTypes->getSingleObject(0);
    if (!obj) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to non-single object");
        return true;
    }

    if (!obj->isFunction()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to non-function");
        return true;
    }

    JSFunction *fun = obj->getFunctionPrivate();
    if (!fun->isInterpreted()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to non-interpreted");
        return true;
    }

    if (fun->getParent() != script->global()) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to scope mismatch");
        return true;
    }

    JSScript *inlineScript = fun->script();

    bool canInline = oracle->canEnterInlinedScript(inlineScript);

    if (!canInline) {
        IonSpew(IonSpew_Inlining, "Cannot inline due to oracle veto");
        return true;
    }

    IonSpew(IonSpew_Inlining, "Inlining good to go!");
    *out = fun;
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

    initParameters();

    
    for (uint32 i = 0; i < info().nlocals(); i++) {
        MConstant *undef = MConstant::New(UndefinedValue());
        current->add(undef);
        current->initSlot(info().localSlot(i), undef);
    }

    current->makeStart(MStart::New());

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    for (uint32 i = 0; i < CountArgSlots(info().fun()); i++) {
        MInstruction *ins = current->getEntrySlot(i)->toInstruction();
        if (ins->type() == MIRType_Value)
            ins->setResumePoint(current->entryResumePoint());
    }

    return traverseBytecode();
}

bool
IonBuilder::buildInline(MResumePoint *callerResumePoint, MDefinition *thisDefn,
                        MDefinitionVector &args)
{
    current = newBlock(pc);
    if (!current)
        return false;

    IonSpew(IonSpew_MIR, "Inlining script %s:%d (%p)",
            script->filename, script->lineno, (void *) script);

    callerResumePoint_ = callerResumePoint;
    MBasicBlock *predecessor = callerResumePoint->block();
    predecessor->end(MGoto::New(current));
    if (!current->addPredecessorWithoutPhis(predecessor))
        return false;
    JS_ASSERT(predecessor->numSuccessors() == 1);
    JS_ASSERT(current->numPredecessors() == 1);
    current->setCallerResumePoint(callerResumePoint);

    JS_ASSERT(args.length() == info().nargs());

    current->initSlot(info().thisSlot(), thisDefn);

    IonSpew(IonSpew_Inlining, "Initializing %u arg slots", args.length());

    
    for (MDefinition **it = args.begin(), **end = args.end(); it != end; ++it) {
        MDefinition *arg = *it;
        size_t i = it - args.begin();
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

    
    JS_ASSERT(current->entryResumePoint()->numOperands() == args.length() + info().nlocals() + 1);

    return traverseBytecode();
}




void
IonBuilder::rewriteParameters()
{
    for (uint32 i = 0; i < CountArgSlots(info().fun()); i++) {
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

        
        
        
        
        
        
        
        
        
        current->rewriteSlot(i, actual);
    }
}

void
IonBuilder::initParameters()
{
    if (!info().fun())
        return;

    MParameter *param = MParameter::New(MParameter::THIS_SLOT,
                                        oracle->thisTypeSet(script));
    current->add(param);
    current->initSlot(info().thisSlot(), param);

    for (uint32 i = 0; i < info().nargs(); i++) {
        param = MParameter::New(i, oracle->parameterTypeSet(script, i));
        current->add(param);
        current->initSlot(info().argSlot(i), param);
    }
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

      case JSOP_GOTO:
      case JSOP_GOTOX:
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
            
            if (!whileLoop(op, sn))
              return ControlStatus_Error;
            return ControlStatus_Jumped;

          case SRC_FOR_IN:
            
            if (!forInLoop(op, sn))
              return ControlStatus_Error;
            return ControlStatus_Jumped;

          default:
            
            JS_NOT_REACHED("unknown goto case");
            break;
        }
        break;
      }

      case JSOP_TABLESWITCH:
        return tableSwitch(op, info().getNote(cx, pc));

      case JSOP_IFNE:
      case JSOP_IFNEX:
        
        
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
    
    if (js_CodeSpec[op].format & JOF_DECOMPOSE)
        return true;

    switch (op) {
      case JSOP_NOP:
        return true;

      case JSOP_PUSH:
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
      	return jsop_binary(op);

      case JSOP_NEG:
        return jsop_neg();

      case JSOP_AND:
      case JSOP_OR:
        return jsop_andor(op);

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

      case JSOP_FALSE:
        return pushConstant(BooleanValue(false));

      case JSOP_TRUE:
        return pushConstant(BooleanValue(true));

      case JSOP_NOTEARG:
        return jsop_notearg();

      case JSOP_CALLARG:
      {
        current->pushArg(GET_SLOTNO(pc));
        if (!pushConstant(UndefinedValue())) 
            return false;
        return jsop_notearg();
      }

      case JSOP_GETARG:
        current->pushArg(GET_SLOTNO(pc));
        return true;

      case JSOP_SETARG:
        current->setArg(GET_SLOTNO(pc));
        return true;

      case JSOP_GETLOCAL:
        current->pushLocal(GET_SLOTNO(pc));
        return true;

      case JSOP_SETLOCAL:
        current->setLocal(GET_SLOTNO(pc));
        return true;

      case JSOP_POP:
        current->pop();
        return true;

      case JSOP_IFEQX:
        return jsop_ifeq(JSOP_IFEQX);

      case JSOP_CALL:
        return jsop_call(GET_ARGC(pc));

      case JSOP_NULLBLOCKCHAIN:
        return true;

      case JSOP_INT8:
        return pushConstant(Int32Value(GET_INT8(pc)));

      case JSOP_UINT16:
        return pushConstant(Int32Value(GET_UINT16(pc)));

      case JSOP_GETGNAME:
        return jsop_getgname(info().getAtom(pc));

      case JSOP_BINDGNAME:
        return pushConstant(ObjectValue(*script->global()));

      case JSOP_SETGNAME:
        return jsop_setgname(info().getAtom(pc));

      case JSOP_UINT24:
        return pushConstant(Int32Value(GET_UINT24(pc)));

      case JSOP_INT32:
        return pushConstant(Int32Value(GET_INT32(pc)));

      case JSOP_TRACE:
        assertValidTraceOp(op);
        return true;

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

    
    
    
    current = state.loop.successor;

    
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

    
    
    if (!state.loop.entry->setBackedge(current))
        return ControlStatus_Error;
    if (successor)
        successor->inheritPhis(state.loop.entry);

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
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);

    
    
    JS_ASSERT(current);

    
    MDefinition *vins = current->pop();
    MBasicBlock *successor = newBlock(current, GetNextPc(pc));
    if (!successor)
        return ControlStatus_Error;

    
    MTest *test = MTest::New(vins, state.loop.entry, successor);
    current->end(test);
    return finishLoop(state, successor);
}

IonBuilder::ControlStatus
IonBuilder::processWhileCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);

    
    MDefinition *ins = current->pop();

    
    MBasicBlock *body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc);
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
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);

    
    MDefinition *ins = current->pop();

    
    MBasicBlock *body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc);
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

        MBasicBlock *update = newBlock(edge->block, pc);
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

    state.tableswitch.currentSuccessor++;

    
    if (state.tableswitch.currentSuccessor >= state.tableswitch.ins->numSuccessors())
        return processTableSwitchEnd(state);

    
    MBasicBlock *successor = state.tableswitch.ins->getSuccessor(state.tableswitch.currentSuccessor);

    
    
    
    if (current) {
        current->end(MGoto::New(successor));
        successor->addPredecessor(current);
    }

    
    
    if (state.tableswitch.currentSuccessor+1 < state.tableswitch.ins->numSuccessors())
        state.stopAt = state.tableswitch.ins->getSuccessor(state.tableswitch.currentSuccessor+1)->pc();
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
IonBuilder::processAndOrEnd(CFGState &state)
{
    
    
    current->end(MGoto::New(state.branch.ifFalse));

    if (!state.branch.ifFalse->addPredecessor(current))
        return ControlStatus_Error;

    current = state.branch.ifFalse;
    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processBreak(JSOp op, jssrcnote *sn)
{
    JS_ASSERT(op == JSOP_GOTO || op == JSOP_GOTOX);

    
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

IonBuilder::ControlStatus
IonBuilder::processContinue(JSOp op, jssrcnote *sn)
{
    JS_ASSERT(op == JSOP_GOTO || op == JSOP_GOTOX);

    
    CFGState *found = NULL;
    jsbytecode *target = pc + GetJumpOffset(pc);
    for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
        if (loops_[i].continuepc == target) {
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
    JS_ASSERT(op == JSOP_GOTO || op == JSOP_GOTOX);

    
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

    state.tableswitch.breaks = new DeferredEdge(current, state.tableswitch.breaks);

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
IonBuilder::assertValidTraceOp(JSOp op)
{
#ifdef DEBUG
    jssrcnote *sn = info().getNote(cx, pc);
    jsbytecode *ifne = pc + js_GetSrcNoteOffset(sn, 0);
    CFGState &state = cfgStack_.back();

    
    JS_ASSERT(GetNextPc(state.loop.entry->pc()) == pc);

    jsbytecode *expected_ifne;
    switch (state.state) {
      case CFGState::DO_WHILE_LOOP_BODY:
        expected_ifne = state.stopAt;
        break;

      default:
        JS_NOT_REACHED("JSOP_TRACE appeared in unknown control flow construct");
        return;
    }

    
    
    JS_ASSERT(ifne == expected_ifne);
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

    
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_TRACE);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    MBasicBlock *header = newPendingLoopHeader(current, pc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(header));

    current = header;
    jsbytecode *bodyStart = GetNextPc(GetNextPc(pc));
    jsbytecode *bodyEnd = conditionpc;
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!pushLoop(CFGState::DO_WHILE_LOOP_BODY, conditionpc, header, bodyStart, bodyEnd, exitpc, conditionpc))
        return ControlStatus_Error;

    CFGState &state = cfgStack_.back();
    state.loop.updatepc = conditionpc;
    state.loop.updateEnd = ifne;

    pc = bodyStart;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::whileLoop(JSOp op, jssrcnote *sn)
{
    
    
    
    
    
    
    
    int ifneOffset = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *ifne = pc + ifneOffset;
    JS_ASSERT(ifne > pc);

    
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_TRACE);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    MBasicBlock *header = newPendingLoopHeader(current, pc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(header));

    
    jsbytecode *bodyStart = GetNextPc(GetNextPc(pc));
    jsbytecode *bodyEnd = pc + GetJumpOffset(pc);
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!pushLoop(CFGState::WHILE_LOOP_COND, ifne, header, bodyStart, bodyEnd, exitpc))
        return ControlStatus_Error;

    
    pc = bodyEnd;
    current = header;
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
    if (condpc != ifne) {
        JS_ASSERT(JSOp(*bodyStart) == JSOP_GOTO || JSOp(*bodyStart) == JSOP_GOTOX);
        JS_ASSERT(bodyStart + GetJumpOffset(bodyStart) == condpc);
        bodyStart = GetNextPc(bodyStart);
    }
    JS_ASSERT(JSOp(*bodyStart) == JSOP_TRACE);
    JS_ASSERT(ifne + GetJumpOffset(ifne) == bodyStart);
    bodyStart = GetNextPc(bodyStart);

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
    jsint low = GET_JUMP_OFFSET(pc2);
    pc2 += JUMP_OFFSET_LEN;
    jsint high = GET_JUMP_OFFSET(pc2);
    pc2 += JUMP_OFFSET_LEN;

    
    MTableSwitch *tableswitch = MTableSwitch::New(ins, low, high);

    
    MBasicBlock *defaultcase = newBlock(current, defaultpc);
    if (!defaultcase)
        return ControlStatus_Error;
    tableswitch->addDefault(defaultcase);

    
    jsbytecode *casepc = NULL;
    for (jsint i = 0; i < high-low+1; i++) {
        casepc = pc + GET_JUMP_OFFSET(pc2);
        
        JS_ASSERT(casepc >= pc && casepc <= exitpc);

        
        
        
        if (casepc == pc) {
            tableswitch->addCase(defaultcase, true);
        } else {
            MBasicBlock *caseblock = newBlock(current, casepc);
            if (!caseblock)
                return ControlStatus_Error;
            tableswitch->addCase(caseblock);
        }

        pc2 += JUMP_OFFSET_LEN;
    }

    JS_ASSERT(tableswitch->numCases() == (uint32)(high - low + 1));
    JS_ASSERT(tableswitch->numSuccessors() > 0);

    
    qsort(tableswitch->successors(), tableswitch->numSuccessors(),
          sizeof(MBasicBlock*), CmpSuccessors);

    
    ControlFlowInfo switchinfo(cfgStack_.length(), exitpc);
    if (!switches_.append(switchinfo))
        return ControlStatus_Error;

    
    CFGState state;
    state.state = CFGState::TABLE_SWITCH;
    state.tableswitch.exitpc = exitpc;
    state.tableswitch.breaks = NULL;
    state.tableswitch.ins = tableswitch;
    state.tableswitch.currentSuccessor = 0;

    
    current->end(tableswitch);

    
    
    if (tableswitch->numSuccessors() == 1)
        state.stopAt = state.tableswitch.exitpc;
    else
        state.stopAt = tableswitch->getSuccessor(1)->pc();
    current = tableswitch->getSuccessor(0);

    if (!cfgStack_.append(state))
        return ControlStatus_Error;

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
IonBuilder::jsop_ifeq(JSOp op)
{
    
    jsbytecode *trueStart = pc + js_CodeSpec[op].length;
    jsbytecode *falseStart = pc + GetJumpOffset(pc);
    JS_ASSERT(falseStart > pc);

    
    jssrcnote *sn = info().getNote(cx, pc);
    if (!sn) {
        
        return false;
    }

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
        JS_ASSERT(JSOp(*trueEnd) == JSOP_GOTO || JSOp(*trueEnd) == JSOP_GOTOX);
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
    return true;
}

bool
IonBuilder::jsop_binary(JSOp op, MDefinition *left, MDefinition *right)
{
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

      default:
        JS_NOT_REACHED("unexpected binary opcode");
        return false;
    }

    current->add(ins);
    ins->infer(oracle->binaryOp(script, pc));

    current->push(ins);
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

bool
IonBuilder::jsop_call_inline(uint32 argc, IonBuilder &inlineBuilder, InliningData *data)
{
#ifdef DEBUG
    uint32 origStackDepth = current->stackDepth();
#endif

    
    MBasicBlock *top = current;

    MResumePoint *inlineResumePoint = MResumePoint::NewUnwrapArgs(top, argc, pc, callerResumePoint_);
    if (!inlineResumePoint)
        return false;

    
    
    
    MDefinitionVector args;
    if (!args.reserve(argc + 2))
        return false;
    for (uintN i = 0; i < argc; ++i) {
        MPassArg *passArg = top->pop()->toPassArg();
        JS_ASSERT(passArg->useCount() == 0);
        passArg->block()->discard(passArg);
        MDefinition *wrapped = passArg->getArgument();
        args.infallibleAppend(wrapped);
    }

    MPassArg *thisArg = top->pop()->toPassArg();
    MDefinition *thisDefn = thisArg->getArgument();
    thisArg->block()->discard(thisArg);

    
    if (!inlineBuilder.buildInline(inlineResumePoint, thisDefn, args))
        return false;

    MIRGraphExits &exits = inlineBuilder.graph().getExitAccumulator();

    
    JS_ASSERT(*pc == JSOP_CALL);
    jsbytecode *postCall = GetNextPc(pc);
    MBasicBlock *bottom = newBlock(NULL, postCall);

    
    Vector<MDefinition *, 8, IonAllocPolicy> retvalDefns;
    for (MBasicBlock **it = exits.begin(), **end = exits.end(); it != end; ++it) {
        MBasicBlock *exitBlock = *it;
        JS_ASSERT(exitBlock->lastIns()->op() == MDefinition::Op_Return);

        if (!retvalDefns.append(exitBlock->lastIns()->toReturn()->getOperand(0)))
            return false;

        exitBlock->discardLastIns();
        MGoto *replacement = MGoto::New(bottom);
        exitBlock->end(replacement);
        if (!bottom->addPredecessorWithoutPhis(exitBlock))
            return false;
    }
    JS_ASSERT(!retvalDefns.empty());

    if (!bottom->inheritNonPredecessor(top))
        return false;

    
    (void) bottom->pop();

    MDefinition *retvalDefn;
    if (retvalDefns.length() > 1) {
        
        MPhi *phi = MPhi::New(bottom->stackDepth());
        for (MDefinition **it = retvalDefns.begin(), **end = retvalDefns.end(); it != end; ++it) {
            if (!phi->addInput(*it))
                return false;
        }
        retvalDefn = phi;
    } else {
        retvalDefn = retvalDefns.back();
    }

    bottom->push(retvalDefn);

    
    
    
    
    JS_ASSERT(bottom->stackDepth() == origStackDepth - argc - 1);

    current = bottom;
    return true;
}

class AutoAccumulateExits
{
    MIRGraph &graph;

  public:
    AutoAccumulateExits(MIRGraph &graph, MIRGraphExits &exits) : graph(graph) {
        graph.setExitAccumulator(&exits);
    }
    ~AutoAccumulateExits() {
        graph.setExitAccumulator(NULL);
    }
};

bool
IonBuilder::makeInliningDecision(uint32 argc, InliningData *data)
{
    JS_ASSERT(data->shouldInline == false);

    JSFunction *inlineFunc = NULL;
    if (!getInliningTarget(argc, pc, &inlineFunc))
        return false;

    if (!inlineFunc) {
        IonSpew(IonSpew_Inlining, "Decided not to inline");
        return true;
    }

    data->shouldInline = true;
    data->callee = inlineFunc;
    return true;
}

IonBuilder::InliningStatus
IonBuilder::maybeInline(uint32 argc)
{
    InliningData data;
    if (!makeInliningDecision(argc, &data))
        return InliningStatus_Error;

    if (!data.shouldInline || inliningDepth >= 1)
        return InliningStatus_NotInlined;

    IonSpew(IonSpew_Inlining, "Recursively building");
    
    
    CompileInfo *info = cx->tempLifoAlloc().new_<CompileInfo>(data.callee->script().get(),
                                                              data.callee);
    if (!info)
        return InliningStatus_Error;

    MIRGraphExits exits;
    AutoAccumulateExits aae(graph(), exits);

    if (cx->typeInferenceEnabled()) {
        TypeInferenceOracle oracle;
        if (!oracle.init(cx, data.callee->script()))
            return InliningStatus_Error;
        IonBuilder inlineBuilder(cx, temp(), graph(), &oracle, *info, inliningDepth + 1);
        return jsop_call_inline(argc, inlineBuilder, &data)
             ? InliningStatus_Inlined
             : InliningStatus_Error;
    }

    DummyOracle oracle;
    IonBuilder inlineBuilder(cx, temp(), graph(), &oracle, *info, inliningDepth + 1);
    return jsop_call_inline(argc, inlineBuilder, &data)
         ? InliningStatus_Inlined
         : InliningStatus_Error;
}

bool
IonBuilder::jsop_call(uint32 argc)
{
    if (inliningEnabled()) {
        InliningStatus status = maybeInline(argc);
        switch (status) {
          case InliningStatus_Error:
            return false;
          case InliningStatus_Inlined:
            return true;
          case InliningStatus_NotInlined:
            IonSpew(IonSpew_Inlining, "Building out-of-line call");
            break;
        }

    }

    MCall *ins = MCall::New(argc + 1); 
    if (!ins)
        return false;

    
    for (int32 i = argc; i >= 0; i--)
        ins->addArg(i, current->pop()->toPassArg());
    ins->initFunction(current->pop());

    
    MPrepareCall *start = new MPrepareCall;
    MPassArg *arg = ins->getArg(0)->toPassArg();
    current->insertBefore(arg, start);

    ins->initPrepareCall(start);

    current->add(ins);
    current->push(ins);
    if (!resumeAfter(ins))
        return false;

    types::TypeSet *barrier;
    types::TypeSet *types = oracle->returnTypeSet(script, pc, &barrier);
    return pushTypeBarrier(ins, types, barrier);
}

bool
IonBuilder::jsop_incslot(JSOp op, uint32 slot)
{
    int32 amt = (js_CodeSpec[op].format & JOF_INC) ? 1 : -1;
    bool post = !!(js_CodeSpec[op].format & JOF_POST);
    TypeOracle::Binary types = oracle->binaryOp(script, pc);

    
    
    
    
    current->pushSlot(slot);
    MDefinition *value = current->pop();
    MInstruction *lhs;
    if (types.lhs == MIRType_Int32)
        lhs = MToInt32::New(value);
    else
        lhs = MToDouble::New(value);
    current->add(lhs);

    
    if (post)
        current->push(lhs);

    MConstant *rhs = MConstant::New(Int32Value(amt));
    current->add(rhs);

    MAdd *result = MAdd::New(lhs, rhs);
    current->add(result);
    result->infer(types);
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

    ins->infer(oracle->binaryOp(script, pc));

    
    current->push(ins);
    return true;
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::New(graph(), info(), predecessor, pc, MBasicBlock::NORMAL);
    graph().addBlock(block);
    return block;
}

MBasicBlock *
IonBuilder::newPendingLoopHeader(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::NewPendingLoopHeader(graph(), info(), predecessor, pc);
    graph().addBlock(block);
    return block;
}


























bool
IonBuilder::resumeAfter(MInstruction *ins)
{
    return resumeAt(ins, GetNextPc(pc));
}

bool
IonBuilder::resumeAt(MInstruction *ins, jsbytecode *pc)
{
    JS_ASSERT(!ins->isIdempotent());

    MResumePoint *resumePoint = MResumePoint::New(current, pc, callerResumePoint_);
    if (!resumePoint)
        return false;
    lastResumePoint_ = resumePoint;
    ins->setResumePoint(resumePoint);
    return true;
}

static inline bool
TestSingletonProperty(JSContext *cx, JSObject *obj, jsid id, bool *isKnownConstant)
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
    if (shape->hasDefaultGetter()) {
        if (!shape->hasSlot())
            return true;
        if (holder->getSlot(shape->slot).isUndefined())
            return true;
    } else if (!shape->isMethod()) {
        return true;
    }

    *isKnownConstant = true;
    return true;
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

    
    if (type == JSVAL_TYPE_OBJECT && !observed->hasType(types::Type::AnyObjectType()))
        type = JSVAL_TYPE_UNKNOWN;

    if (type == JSVAL_TYPE_UNKNOWN) {
        barrier = MTypeBarrier::New(ins, observed);
        current->add(barrier);

        if (type == JSVAL_TYPE_UNDEFINED)
            return pushConstant(UndefinedValue());
        if (type == JSVAL_TYPE_NULL)
            return pushConstant(NullValue());
    } else {
        MUnbox::Mode mode = ins->isIdempotent() ? MUnbox::Fallible : MUnbox::TypeBarrier;
        barrier = MUnbox::New(ins, MIRTypeFromValueType(type), mode);
        current->add(barrier);
    }
    current->push(barrier);
    return true;
}

bool
IonBuilder::jsop_getgname(JSAtom *atom)
{
    
    if (atom == cx->runtime->atomState.typeAtoms[JSTYPE_VOID])
        return pushConstant(UndefinedValue());
    if (atom == cx->runtime->atomState.NaNAtom)
        return pushConstant(cx->runtime->NaNValue);
    if (atom == cx->runtime->atomState.InfinityAtom)
        return pushConstant(cx->runtime->positiveInfinityValue);

    jsid id = ATOM_TO_JSID(atom);
    JSObject *globalObj = script->global();
    JS_ASSERT(globalObj->isNative());

    
    
    const js::Shape *shape = globalObj->nativeLookup(cx, id);
    if (!shape)
        return abort("GETGNAME property not found on global");
    if (!shape->hasDefaultGetterOrIsMethod())
        return abort("GETGNAME found a getter");
    if (!shape->hasSlot())
        return abort("GETGNAME property has no slot");

    
    bool needShapeGuard = shape->configurable();
    JSValueType knownType = JSVAL_TYPE_UNKNOWN;

    types::TypeSet *barrier;
    types::TypeSet *types = oracle->propertyRead(script, pc, &barrier);
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

    if (needShapeGuard) {
        MGuardShape *guard = MGuardShape::New(global, shape);
        current->add(guard);
    }

    JS_ASSERT(shape->slot >= globalObj->numFixedSlots());

    MSlots *slots = MSlots::New(global);
    current->add(slots);
    MLoadSlot *load = MLoadSlot::New(slots, shape->slot - globalObj->numFixedSlots());
    current->add(load);

    
    if (knownType != JSVAL_TYPE_UNKNOWN && !barrier)
        load->setResultType(MIRTypeFromValueType(knownType));

    current->push(load);
    return pushTypeBarrier(load, types, barrier);
}

bool
IonBuilder::jsop_setgname(JSAtom *atom)
{
    jsid id = ATOM_TO_JSID(atom);

    bool canSpecialize;
    types::TypeSet *propertyTypes = oracle->globalPropertyWrite(script, pc, id, &canSpecialize);

    
    if (!canSpecialize)
        return abort("SETGNAME unable to specialize property access");

    JSObject *globalObj = script->global();
    JS_ASSERT(globalObj->isNative());

    
    
    const js::Shape *shape = globalObj->nativeLookup(cx, id);
    if (!shape)
        return abort("SETGNAME property not found on global");
    if (shape->isMethod())
        return abort("SETGNAME found a method");
    if (!shape->hasDefaultSetter())
        return abort("SETGNAME found a setter");
    if (!shape->writable())
        return abort("SETGNAME non-writable property");
    if (!shape->hasSlot())
        return abort("SETGNAME property has no slot");

    if (propertyTypes && propertyTypes->isOwnProperty(cx, globalObj->getType(cx), true))
        return abort("SETGNAME property is non-configurable, non-enumerable or non-writable");

    MInstruction *global = MConstant::New(ObjectValue(*globalObj));
    current->add(global);

    
    
    
    if (!propertyTypes) {
        MGuardShape *guard = MGuardShape::New(global, shape);
        current->add(guard);
    }

    JS_ASSERT(shape->slot >= globalObj->numFixedSlots());

    MSlots *slots = MSlots::New(global);
    current->add(slots);

    MDefinition *value = current->pop();
    MStoreSlot *store = MStoreSlot::New(slots, shape->slot - globalObj->numFixedSlots(), value);
    current->add(store);

#ifdef JSGC_INCREMENTAL
    
    if (cx->compartment->needsBarrier() &&
        (!propertyTypes || propertyTypes->needsBarrier(cx)))
    {
        store->setNeedsBarrier(true);
    }
#endif

    if (!resumeAfter(store))
        return false;

    
    DebugOnly<MDefinition *> pushedGlobal = current->pop();
    JS_ASSERT(&pushedGlobal->toConstant()->value().toObject() == globalObj);

    
    
    
    
    if (propertyTypes && !globalObj->getSlot(shape->slot).isUndefined()) {
        JSValueType knownType = propertyTypes->getKnownTypeTag(cx);
        if (knownType != JSVAL_TYPE_UNKNOWN)
            store->setSlotType(MIRTypeFromValueType(knownType));
    }

    current->push(value);
    return true;
}
