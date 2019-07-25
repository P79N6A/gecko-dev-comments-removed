








































#include "BytecodeAnalyzer.h"
#include "MIRGraph.h"
#include "Ion.h"
#include "IonSpew.h"
#include "jsemit.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

bool
ion::Go(JSContext *cx, JSScript *script, StackFrame *fp)
{
    TempAllocator temp(&cx->tempPool);

    JSFunction *fun = fp->isFunctionFrame() ? fp->fun() : NULL;

    MIRGraph graph(cx);
    C1Spewer spew(graph, script);
    BytecodeAnalyzer analyzer(cx, script, fun, temp, graph);
    spew.enable("/tmp/ion.cfg");

    if (!analyzer.analyze())
        return false;

    spew.spew("Building SSA");

    return false;
}

BytecodeAnalyzer::BytecodeAnalyzer(JSContext *cx, JSScript *script, JSFunction *fun, TempAllocator &temp,
                                   MIRGraph &graph)
  : MIRGenerator(cx, temp, script, fun, graph),
    cfgStack_(TempAllocPolicy(cx)),
    loops_(TempAllocPolicy(cx))
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
BytecodeAnalyzer::readIndex(jsbytecode *pc)
{
    return (atoms - script->atomMap.vector) + GET_INDEX(pc);
}

BytecodeAnalyzer::CFGState
BytecodeAnalyzer::CFGState::If(jsbytecode *join, MBasicBlock *ifFalse)
{
    CFGState state;
    state.state = IF_TRUE;
    state.stopAt = join;
    state.branch.ifFalse = ifFalse;
    return state;
}

BytecodeAnalyzer::CFGState
BytecodeAnalyzer::CFGState::IfElse(jsbytecode *trueEnd, jsbytecode *falseEnd, MBasicBlock *ifFalse) 
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
BytecodeAnalyzer::popCfgStack()
{
    if (cfgStack_.back().isLoop())
        loops_.popBack();
    cfgStack_.popBack();
}

bool
BytecodeAnalyzer::pushLoop(CFGState::State initial, jsbytecode *stopAt, MBasicBlock *entry,
                           jsbytecode *bodyStart, jsbytecode *bodyEnd, jsbytecode *exitpc)
{
    LoopInfo loop(cfgStack_.length());
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
BytecodeAnalyzer::analyze()
{
    current = newBlock(pc);
    if (!current)
        return false;

    
    if (fun()) {
        MParameter *param = MParameter::New(this, MParameter::CALLEE_SLOT);
        if (!current->add(param))
            return false;
        current->initSlot(calleeSlot(), param);

        param = MParameter::New(this, MParameter::THIS_SLOT);
        if (!current->add(param))
            return false;
        current->initSlot(thisSlot(), param);

        for (uint32 i = 0; i < nargs(); i++) {
            param = MParameter::New(this, int(i) - 2);
            if (!current->add(param))
                return false;
            current->initSlot(argSlot(i), param);
        }
    }

    
    for (uint32 i = 0; i < nlocals(); i++) {
        MConstant *undef = MConstant::New(this, UndefinedValue());
        if (!current->add(undef))
            return false;
        current->initSlot(localSlot(i), undef);
    }
    if (!current->initHeader())
        return false;

    if (!traverseBytecode())
        return false;

    return true;
}




























bool
BytecodeAnalyzer::traverseBytecode()
{
    for (;;) {
        JS_ASSERT(pc < script->code + script->length);

        for (;;) {
            
            
            
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

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::snoopControlFlow(JSOp op)
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
BytecodeAnalyzer::inspectOpcode(JSOp op)
{
    switch (op) {
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















BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processControlEnd()
{
    JS_ASSERT(!current);

    if (cfgStack_.empty()) {
        
        
        return ControlStatus_Ended;
    }

    return processCfgStack();
}







BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processCfgStack()
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

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processCfgEntry(CFGState &state)
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

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processIfEnd(CFGState &state)
{
    if (!current)
        return ControlStatus_Ended;

    
    
    
    MGoto *ins = MGoto::New(this, state.branch.ifFalse);
    if (!current->end(ins))
        return ControlStatus_Error;

    if (!current->addPredecessor(state.branch.ifFalse))
        return ControlStatus_Error;

    current = state.branch.ifFalse;
    return ControlStatus_Joined;
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processIfElseTrueEnd(CFGState &state)
{
    
    
    state.state = CFGState::IF_ELSE_FALSE;
    state.branch.ifTrue = current;
    state.stopAt = state.branch.falseEnd;
    pc = state.branch.ifFalse->pc();
    current = state.branch.ifFalse;
    return ControlStatus_Jumped;
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processIfElseFalseEnd(CFGState &state)
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
  
    
    MGoto *edge = MGoto::New(this, join);
    if (!pred->end(edge))
        return ControlStatus_Error;
    if (other) {
        edge = MGoto::New(this, join);
        if (!other->end(edge))
            return ControlStatus_Error;
        if (!join->addPredecessor(other))
            return ControlStatus_Error;
    }
  
    
    current = join;
    return ControlStatus_Joined;
}

bool
BytecodeAnalyzer::finalizeLoop(CFGState &state, MInstruction *last)
{
    JS_ASSERT(!state.loop.continues);

    
    MBasicBlock *breaks = NULL;
    if (state.loop.breaks) {
        breaks = newBlock(state.loop.exitpc);

        DeferredEdge *edge = state.loop.breaks;
        while (edge) {
            MGoto *ins = MGoto::New(this, breaks);
            if (!edge->block->end(ins))
                return false;
            if (!breaks->addPredecessor(edge->block))
                return false;
            edge = edge->next;
        }
    }

    
    
    MBasicBlock *successor = state.loop.successor
                             ? state.loop.successor
                             : breaks;

    
    
    
    if (current) {
        JS_ASSERT_IF(last, state.loop.successor);

        MControlInstruction *ins;
        if (last)
            ins = MTest::New(this, last, state.loop.entry, state.loop.successor);
        else
            ins = MGoto::New(this, state.loop.entry);
        if (!current->end(ins))
            return false;

        if (!state.loop.entry->setBackedge(current, successor))
            return false;
    }

    
    
    if (successor && breaks && (successor != breaks)) {
        MGoto *ins = MGoto::New(this, successor);
        if (!breaks->end(ins))
            return false;
        if (!successor->addPredecessor(breaks))
            return false;
    }

    state.loop.successor = successor;
    return true;
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processDoWhileEnd(CFGState &state)
{
    if (current || state.loop.breaks) {
        state.loop.successor = newBlock(current, state.loop.exitpc);
        if (!state.loop.successor)
            return ControlStatus_Error;
    }

    if (!processDeferredContinues(state))
        return ControlStatus_Error;
    if (!finalizeLoop(state, current->pop()))
        return ControlStatus_Error;

    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);
    pc += js_CodeSpec[JSOp(*pc)].length;
    current = state.loop.successor;
    return current ? ControlStatus_Joined : ControlStatus_Ended;
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processWhileCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);

    
    MInstruction *ins = current->pop();

    
    MBasicBlock *body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc);
    MTest *test = MTest::New(this, ins, body, state.loop.successor);
    if (!current->end(test))
        return ControlStatus_Error;

    state.state = CFGState::WHILE_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    current = body;
    return ControlStatus_Jumped;
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processWhileBodyEnd(CFGState &state)
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

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processForCondEnd(CFGState &state)
{
    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);

    
    MInstruction *ins = current->pop();

    
    MBasicBlock *body = newBlock(current, state.loop.bodyStart);
    state.loop.successor = newBlock(current, state.loop.exitpc);
    MTest *test = MTest::New(this, ins, body, state.loop.successor);
    if (!current->end(test))
        return ControlStatus_Error;

    state.state = CFGState::FOR_LOOP_BODY;
    state.stopAt = state.loop.bodyEnd;
    pc = state.loop.bodyStart;
    current = body;
    return ControlStatus_Jumped;
}

bool
BytecodeAnalyzer::processDeferredContinues(CFGState &state)
{
    
    
    if (state.loop.continues) {
        MBasicBlock *update = newBlock(pc);
        if (current) {
            MGoto *ins = MGoto::New(this, update);
            if (!current->end(ins))
                return ControlStatus_Error;
            if (!update->addPredecessor(current))
                return ControlStatus_Error;
        }

        DeferredEdge *edge = state.loop.continues;
        while (edge) {
            MGoto *ins = MGoto::New(this, update);
            if (!edge->block->end(ins))
                return ControlStatus_Error;
            if (!update->addPredecessor(edge->block))
                return ControlStatus_Error;
            edge = edge->next;
        }
        state.loop.continues = NULL;

        current = update;
    }

    return true;
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processForBodyEnd(CFGState &state)
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

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processForUpdateEnd(CFGState &state)
{
    if (!finalizeLoop(state, NULL))
        return ControlStatus_Error;

    current = state.loop.successor;
    if (!current)
        return ControlStatus_Ended;

    pc = current->pc();
    return ControlStatus_Joined;
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processBreak(JSOp op, jssrcnote *sn)
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

    DeferredEdge *edge = new (temp()) DeferredEdge(current, state.loop.breaks);
    if (!edge)
        return ControlStatus_Error;
    state.loop.breaks = edge;

    current = NULL;
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processContinue(JSOp op, jssrcnote *sn)
{
    JS_ASSERT(op == JSOP_GOTO || op == JSOP_GOTOX);

    
    CFGState *found = NULL;
    jsbytecode *target = pc + GetJumpOffset(pc);
    for (size_t i = loops_.length() - 1; i < loops_.length(); i--) {
        CFGState &cfg = cfgStack_[loops_[i].cfgEntry];
        if (cfg.loop.entry->pc() == target) {
            found = &cfg;
            break;
        }
    }

    
    
    JS_ASSERT(found);
    CFGState &state = *found;

    DeferredEdge *edge = new (temp()) DeferredEdge(current, state.loop.continues);
    if (!edge)
        return ControlStatus_Error;
    state.loop.continues = edge;

    current = NULL;
    pc += js_CodeSpec[op].length;
    return processControlEnd();
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::maybeLoop(JSOp op, jssrcnote *sn)
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
BytecodeAnalyzer::assertValidTraceOp(JSOp op)
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

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::doWhileLoop(JSOp op, jssrcnote *sn)
{
    int offset = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *ifne = pc + offset;
    JS_ASSERT(ifne > pc);

    
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_TRACE);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    
    
    
    
    
    MBasicBlock *header = newLoopHeader(current, pc);
    MGoto *ins = MGoto::New(this, header);
    if (!current->end(ins))
        return ControlStatus_Error;

    current = header;
    jsbytecode *bodyStart = GetNextPc(GetNextPc(pc));
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!pushLoop(CFGState::DO_WHILE_LOOP, ifne, header, bodyStart, ifne, exitpc))
        return ControlStatus_Error;

    return ControlStatus_Jumped;
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::whileLoop(JSOp op, jssrcnote *sn)
{
    
    
    
    
    
    
    
    int ifneOffset = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *ifne = pc + ifneOffset;
    JS_ASSERT(ifne > pc);

    
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_TRACE);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    MBasicBlock *header = newLoopHeader(current, pc);
    MGoto *ins = MGoto::New(this, header);
    if (!current->end(ins))
        return ControlStatus_Error;

    
    jsbytecode *bodyStart = GetNextPc(GetNextPc(pc));
    jsbytecode *bodyEnd = pc + GetJumpOffset(pc);
    jsbytecode *exitpc = GetNextPc(ifne);
    if (!pushLoop(CFGState::WHILE_LOOP_COND, ifne, header, bodyStart, bodyEnd, exitpc))
        return ControlStatus_Error;

    
    pc = bodyEnd;
    current = header;
    return ControlStatus_Jumped;
}

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::forLoop(JSOp op, jssrcnote *sn)
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
    MGoto *ins = MGoto::New(this, header);
    if (!current->end(ins))
        return ControlStatus_Error;

    
    
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

    if (!pushLoop(initial, ifne, header, bodyStart, bodyEnd, exitpc))
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
BytecodeAnalyzer::jsop_ifeq(JSOp op)
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
    MTest *test = MTest::New(this, ins, ifTrue, ifFalse);
    if (!current->end(test))
        return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
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

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processReturn(JSOp op)
{
    MInstruction *ins;
    switch (op) {
      case JSOP_RETURN:
        ins = current->pop();
        break;

      case JSOP_STOP:
        ins = MConstant::New(this, UndefinedValue());
        if (!current->add(ins))
            return ControlStatus_Error;
        break;

      default:
        ins = NULL;
        JS_NOT_REACHED("unknown return op");
        break;
    }

    MReturn *ret = MReturn::New(this, ins);
    if (!current->end(ret))
        return ControlStatus_Error;

    
    current = NULL;
    return processControlEnd();
}

bool
BytecodeAnalyzer::pushConstant(const Value &v)
{
    MConstant *ins = MConstant::New(this, v);
    if (!current->add(ins))
        return false;
    current->push(ins);
    return true;
}

bool
BytecodeAnalyzer::jsop_binary(JSOp op)
{
    MInstruction *right = current->pop();
    MInstruction *left = current->pop();

    MInstruction *ins;
    switch (op) {
      case JSOP_BITAND:
        ins = MBitAnd::New(this, left, right);
        break;

      default:
        JS_NOT_REACHED("unexpected binary opcode");
        return false;
    }

    if (!current->add(ins))
        return false;
    current->push(ins);

    return true;
}

MBasicBlock *
BytecodeAnalyzer::newBlock(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::New(this, predecessor, pc);
    if (!graph().addBlock(block))
        return NULL;
    return block;
}

MBasicBlock *
BytecodeAnalyzer::newLoopHeader(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::NewLoopHeader(this, predecessor, pc);
    if (!graph().addBlock(block))
        return NULL;
    return block;
}

