








































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
    cfgStack_(TempAllocPolicy(cx))
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
        if (!undef)
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

        
        if (!cfgStack_.empty() && cfgStack_.back().stopAt == pc) {
            ControlStatus status = processCfgStack();
            if (status == ControlStatus_Error)
                return false;
            if (!current)
                return true;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        ControlStatus status = snoopControlFlow(JSOp(*pc));
        if (status == ControlStatus_Error)
            return false;
        if (!current)
            return true;

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
            JS_NOT_REACHED("break NYI");
            return ControlStatus_Error;

          case SRC_CONTINUE:
          case SRC_CONT2LABEL:
            JS_NOT_REACHED("continue NYI");
            return ControlStatus_Error;

          case SRC_WHILE:
            
            if (!whileLoop(op, sn))
              return ControlStatus_Error;
            return ControlStatus_Jumped;

          case SRC_FOR:
            
            if (!forLoop(op, sn))
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

BytecodeAnalyzer::CFGState
BytecodeAnalyzer::CFGState::DoWhile(jsbytecode *ifne, MBasicBlock *entry)
{
    CFGState state;
    state.state = DO_WHILE_LOOP;
    state.stopAt = ifne;
    state.loop.entry = entry;
    return state;
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
        cfgStack_.popBack();
        if (cfgStack_.empty())
            return status;
        status = processCfgEntry(cfgStack_.back());
    }

    
    if (status == ControlStatus_Joined)
        cfgStack_.popBack();

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

BytecodeAnalyzer::ControlStatus
BytecodeAnalyzer::processDoWhileEnd(CFGState &state)
{
    if (!current)
        return ControlStatus_Ended;

    
    MInstruction *ins = current->pop();

    
    MBasicBlock *successor = newBlock(current, pc);
    MTest *test = MTest::New(this, ins, state.loop.entry, successor);
    if (!current->end(test))
        return ControlStatus_Error;

    
    
    if (!state.loop.entry->addBackedge(current, successor))
        return ControlStatus_Error;

    JS_ASSERT(JSOp(*pc) == JSOP_IFNE || JSOp(*pc) == JSOP_IFNEX);
    pc += js_CodeSpec[JSOp(*pc)].length;

    current = successor;
    return ControlStatus_Joined;
}

uint32
BytecodeAnalyzer::readIndex(jsbytecode *pc)
{
    return (atoms - script->atomMap.vector) + GET_INDEX(pc);
}

bool
BytecodeAnalyzer::inspectOpcode(JSOp op)
{
    switch (op) {
      case JSOP_NOP:
        return maybeLoop(op, js_GetSrcNote(script, pc));

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

      case JSOP_POP:
        current->pop();
        return maybeLoop(op, js_GetSrcNote(script, pc));

      case JSOP_GETARG:
        current->pushArg(GET_SLOTNO(pc));
        return true;

      case JSOP_GETLOCAL:
        current->pushLocal(GET_SLOTNO(pc));
        return true;

      case JSOP_SETLOCAL:
        return current->setLocal(GET_SLOTNO(pc));

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
        return false;
    }
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
BytecodeAnalyzer::maybeLoop(JSOp op, jssrcnote *sn)
{
    
    
    
    
    
    switch (op) {
      case JSOP_POP:
        
        if (sn && SN_TYPE(sn) == SRC_FOR)
            return forLoop(op, sn);
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
        return false;
    }

    return true;
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

bool
BytecodeAnalyzer::doWhileLoop(JSOp op, jssrcnote *sn)
{
    int offset = js_GetSrcNoteOffset(sn, 0);
    jsbytecode *ifne = pc + offset;
    JS_ASSERT(ifne > pc);

    
    JS_ASSERT(JSOp(*GetNextPc(pc)) == JSOP_TRACE);
    JS_ASSERT(GetNextPc(pc) == ifne + GetJumpOffset(ifne));

    
    
    
    
    
    MBasicBlock *header = newLoopHeader(current, pc);
    if (!header)
        return false;
    MGoto *ins = MGoto::New(this, header);
    if (!current->end(ins))
        return false;

    current = header;
    if (!cfgStack_.append(CFGState::DoWhile(ifne, header)))
        return false;

    return true;
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
    if (!graph.addBlock(block))
        return NULL;
    return block;
}

MBasicBlock *
BytecodeAnalyzer::newLoopHeader(MBasicBlock *predecessor, jsbytecode *pc)
{
    MBasicBlock *block = MBasicBlock::NewLoopHeader(this, predecessor, pc);
    if (!graph.addBlock(block))
        return NULL;
    return block;
}

