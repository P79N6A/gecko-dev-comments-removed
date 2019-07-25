








































#include "IonAnalysis.h"
#include "IonBuilder.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonSpew.h"
#include "IonAnalysis.h"
#include "IonLowering.h"
#include "jsemit.h"
#include "jsscriptinlines.h"

#ifdef JS_THREADSAFE
# include "prthread.h"
#endif

using namespace js;
using namespace js::ion;

bool
ion::Go(JSContext *cx, JSScript *script, StackFrame *fp)
{
    TempAllocator temp(&cx->tempPool);
    IonContext ictx(cx, &temp);

    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;

    MIRGraph graph;

    DummyOracle oracle;
    C1Spewer spew(graph, script);
    IonBuilder analyzer(cx, script, fun, temp, graph, &oracle);
    spew.enable("/tmp/ion.cfg");

    if (!analyzer.analyze())
        return false;
    spew.spew("Build SSA");

    if (!ApplyTypeInformation(graph))
        return false;
    if (!Lower(graph))
        return false;
    spew.spew("Lower");

    if (!RenumberInstructions(graph))
        return false;
    spew.spew("Renumber Instructions");

    return false;
}

IonBuilder::IonBuilder(JSContext *cx, JSScript *script, JSFunction *fun, TempAllocator &temp,
                       MIRGraph &graph, TypeOracle *oracle)
  : MIRGenerator(temp, script, fun, graph),
    cx(cx),
    oracle(oracle)
{
    pc = script->code;
    atoms = script->atomMap.vector;
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

uint32
IonBuilder::readIndex(jsbytecode *pc)
{
    return (atoms - script->atomMap.vector) + GET_INDEX(pc);
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

    LoopInfo loop(cfgStack_.length(), continuepc);
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
IonBuilder::analyze()
{
    current = newBlock(pc);
    if (!current)
        return false;

    
    if (fun()) {
        MParameter *param = MParameter::New(MParameter::CALLEE_SLOT);
        current->add(param);
        current->initSlot(calleeSlot(), param);

        param = MParameter::New(MParameter::THIS_SLOT);
        current->add(param);
        current->initSlot(thisSlot(), param);

        for (uint32 i = 0; i < nargs(); i++) {
            param = MParameter::New(int(i));
            current->add(param);
            current->initSlot(argSlot(i), param);
        }
    }

    
    for (uint32 i = 0; i < nlocals(); i++) {
        MConstant *undef = MConstant::New(UndefinedValue());
        current->add(undef);
        current->initSlot(localSlot(i), undef);
    }

    if (!traverseBytecode())
        return false;

    return true;
}




























bool
IonBuilder::traverseBytecode()
{
    for (;;) {
        JS_ASSERT(pc < script->code + script->length);

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
        return maybeLoop(op, js_GetSrcNote(script, pc));

      case JSOP_POP:
        return maybeLoop(op, js_GetSrcNote(script, pc));

      case JSOP_RETURN:
      case JSOP_STOP:
        return processReturn(op);

      case JSOP_GOTO:
      case JSOP_GOTOX:
      {
        jssrcnote *sn = js_GetSrcNote(script, pc);
        switch (sn ? SN_TYPE(sn) : SRC_NULL) {
          case SRC_BREAK:
          case SRC_BREAK2LABEL:
            return processBreak(op, sn);

          case SRC_CONTINUE:
          case SRC_CONT2LABEL:
            return processContinue(op, sn);

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
    switch (op) {
      case JSOP_NOP:
        return true;

      case JSOP_PUSH:
        return pushConstant(UndefinedValue());

      case JSOP_IFEQ:
        return jsop_ifeq(JSOP_IFEQ);

      case JSOP_BITAND:
        return jsop_binary(op);

      case JSOP_DOUBLE:
        return pushConstant(script->getConst(readIndex(pc)));

      case JSOP_STRING:
        return pushConstant(StringValue(atoms[GET_INDEX(pc)]));

      case JSOP_ZERO:
        return pushConstant(Int32Value(0));

      case JSOP_ONE:
        return pushConstant(Int32Value(1));

      case JSOP_NULL:
        return pushConstant(NullValue());

      case JSOP_FALSE:
        return pushConstant(BooleanValue(false));

      case JSOP_TRUE:
        return pushConstant(BooleanValue(true));

      case JSOP_GETARG:
        current->pushArg(GET_SLOTNO(pc));
        return true;

      case JSOP_GETLOCAL:
        current->pushLocal(GET_SLOTNO(pc));
        return true;

      case JSOP_SETLOCAL:
        return current->setLocal(GET_SLOTNO(pc));

      case JSOP_POP:
        current->pop();
        return true;

      case JSOP_IFEQX:
        return jsop_ifeq(JSOP_IFEQX);

      case JSOP_NULLBLOCKCHAIN:
        return true;

      case JSOP_INT8:
        return pushConstant(Int32Value(GET_INT8(pc)));

      case JSOP_TRACE:
        assertValidTraceOp(op);
        return true;

      default:
        fprintf(stdout, "%d\n", op);
        JS_NOT_REACHED("what");
        return false;
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

      case CFGState::DO_WHILE_LOOP:
        return processDoWhileEnd(state);

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

        if (!current->addPredecessor(state.branch.ifFalse))
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
  
    
    MBasicBlock *join = newBlock(pred, pc);
    if (!join)
        return ControlStatus_Error;
  
    
    pred->end(MGoto::New(join));

    if (other) {
        other->end(MGoto::New(join));
        if (!join->addPredecessor(other))
            return ControlStatus_Error;
    }
  
    
    current = join;
    return ControlStatus_Joined;
}

bool
IonBuilder::finalizeLoop(CFGState &state, MInstruction *last)
{
    JS_ASSERT(!state.loop.continues);

    
    MBasicBlock *breaks = NULL;
    if (state.loop.breaks) {
        breaks = newBlock(state.loop.exitpc);
        if (!breaks)
            return false;

        DeferredEdge *edge = state.loop.breaks;
        while (edge) {
            edge->block->end(MGoto::New(breaks));
            if (!breaks->addPredecessor(edge->block))
                return false;
            edge = edge->next;
        }
    }

    
    
    MBasicBlock *successor = state.loop.successor
                             ? state.loop.successor
                             : breaks;

    
    
    
    if (current) {
        JS_ASSERT_IF(last, successor);

        MControlInstruction *ins;
        if (last)
            ins = MTest::New(last, state.loop.entry, successor);
        else
            ins = MGoto::New(state.loop.entry);
        current->end(ins);

        if (!state.loop.entry->setBackedge(current, successor))
            return false;
    }

    
    
    if (successor && breaks && (successor != breaks)) {
        breaks->end(MGoto::New(successor));
        if (!successor->addPredecessor(breaks))
            return false;
    }

    state.loop.successor = successor;
    return true;
}

IonBuilder::ControlStatus
IonBuilder::processDoWhileEnd(CFGState &state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    MInstruction *last = NULL;
    if (current) {
        last = current->pop();
        state.loop.successor = newBlock(current, GetNextPc(pc));
        if (!state.loop.successor)
            return ControlStatus_Error;
    }

    if (!finalizeLoop(state, last))
        return ControlStatus_Error;

    current = state.loop.successor;
    if (!current)
        return ControlStatus_Ended;

    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processWhileCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);

    
    MInstruction *ins = current->pop();

    
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
    if (!finalizeLoop(state, NULL))
        return ControlStatus_Error;

    current = state.loop.successor;
    if (!current)
        return ControlStatus_Ended;

    pc = current->pc();
    return ControlStatus_Joined;
}

IonBuilder::ControlStatus
IonBuilder::processForCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);

    
    MInstruction *ins = current->pop();

    
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

bool
IonBuilder::processDeferredContinues(CFGState &state)
{
    
    
    if (state.loop.continues) {
        MBasicBlock *update = newBlock(pc);
        if (!update)
            return false;

        if (current) {
            current->end(MGoto::New(update));
            if (!update->addPredecessor(current))
                return ControlStatus_Error;
        }

        DeferredEdge *edge = state.loop.continues;
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

IonBuilder::ControlStatus
IonBuilder::processForBodyEnd(CFGState &state)
{
    if (!processDeferredContinues(state))
        return ControlStatus_Error;

    if (!state.loop.updatepc)
        return processForUpdateEnd(state);

    pc = state.loop.updatepc;

    state.state = CFGState::FOR_LOOP_UPDATE;
    state.stopAt = state.loop.updateEnd;
    return ControlStatus_Jumped;
}

IonBuilder::ControlStatus
IonBuilder::processForUpdateEnd(CFGState &state)
{
    if (!finalizeLoop(state, NULL))
        return ControlStatus_Error;

    current = state.loop.successor;
    if (!current)
        return ControlStatus_Ended;

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

    
    
    JS_ASSERT(found);
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
    jssrcnote *sn = js_GetSrcNote(script, pc);
    jsbytecode *ifne = pc + js_GetSrcNoteOffset(sn, 0);
    CFGState &state = cfgStack_.back();

    
    JS_ASSERT(GetNextPc(state.loop.entry->pc()) == pc);

    jsbytecode *expected_ifne;
    switch (state.state) {
      case CFGState::DO_WHILE_LOOP:
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
    int offset = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *ifne = pc + offset;
    JS_ASSERT(ifne > pc);

    
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_TRACE);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    
    
    
    
    
    MBasicBlock *header = newLoopHeader(current, pc);
    if (!header)
        return ControlStatus_Error;
    current->end(MGoto::New(header));

    current = header;
    jsbytecode *bodyStart = GetNextPc(GetNextPc(pc));
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!pushLoop(CFGState::DO_WHILE_LOOP, ifne, header, bodyStart, ifne, exitpc))
        return ControlStatus_Error;

    pc = GetNextPc(GetNextPc(pc));
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

    MBasicBlock *header = newLoopHeader(current, pc);
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

    MBasicBlock *header = newLoopHeader(current, pc);
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

    if (!pushLoop(initial, ifne, header, bodyStart, bodyEnd, exitpc, updatepc))
        return ControlStatus_Error;

    CFGState &state = cfgStack_.back();
    state.loop.condpc = (condpc != ifne) ? condpc : NULL;
    state.loop.updatepc = (updatepc != condpc) ? updatepc : NULL;
    if (state.loop.updatepc)
        state.loop.updateEnd = condpc;

    current = header;
    return ControlStatus_Jumped;
}

bool
IonBuilder::jsop_ifeq(JSOp op)
{
    
    jsbytecode *trueStart = pc + js_CodeSpec[op].length;
    jsbytecode *falseStart = pc + GetJumpOffset(pc);
    JS_ASSERT(falseStart > pc);

    
    jssrcnote *sn = js_GetSrcNote(script, pc);
    if (!sn) {
        
        return false;
    }

    MInstruction *ins = current->pop();

    
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
        JS_ASSERT(!js_GetSrcNote(script, trueEnd));

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
    MInstruction *ins;
    switch (op) {
      case JSOP_RETURN:
        ins = current->pop();
        break;

      case JSOP_STOP:
        ins = MConstant::New(UndefinedValue());
        current->add(ins);
        break;

      default:
        ins = NULL;
        JS_NOT_REACHED("unknown return op");
        break;
    }

    MReturn *ret = MReturn::New(ins);
    current->end(ret);

    
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
IonBuilder::jsop_binary(JSOp op)
{
    
    MSnapshot *snapshot = takeSnapshot();

    
    MInstruction *right = current->pop();
    MInstruction *left = current->pop();

    MBinaryInstruction *ins;
    switch (op) {
      case JSOP_BITAND:
        ins = MBitAnd::New(left, right);
        break;

      default:
        JS_NOT_REACHED("unexpected binary opcode");
        return false;
    }

    current->add(ins);
    ins->infer(oracle->binaryOp(script, pc));
    ins->assignSnapshot(snapshot);

    current->push(ins);
    return true;
}

MBasicBlock *
IonBuilder::newBlock(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::New(this, predecessor, pc);
    if (!graph().addBlock(block))
        return NULL;
    return block;
}

MBasicBlock *
IonBuilder::newLoopHeader(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::NewLoopHeader(this, predecessor, pc);
    if (!graph().addBlock(block))
        return NULL;
    return block;
}

MSnapshot *
IonBuilder::takeSnapshot()
{
    MSnapshot *snapshot = MSnapshot::New(current, pc);
    if (!snapshot)
        return NULL;
    current->add(snapshot);
    return snapshot;
}

