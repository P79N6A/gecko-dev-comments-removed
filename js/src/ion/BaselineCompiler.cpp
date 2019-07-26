






#include "BaselineJIT.h"
#include "BaselineIC.h"
#include "BaselineCompiler.h"
#include "FixedList.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "VMFunctions.h"
#include "IonFrames-inl.h"

using namespace js;
using namespace js::ion;

BaselineCompiler::BaselineCompiler(JSContext *cx, JSScript *script)
  : BaselineCompilerSpecific(cx, script)
{
}

bool
BaselineCompiler::init()
{
    if (!labels_.init(script->length))
        return false;

    for (size_t i = 0; i < script->length; i++)
        new (&labels_[i]) Label();

    if (!frame.init())
        return false;

    return true;
}

MethodStatus
BaselineCompiler::compile()
{
    IonSpew(IonSpew_Scripts, "Baseline compiling script %s:%d (%p)",
            script->filename, script->lineno, script);

    if (script->needsArgsObj()) {
        IonSpew(IonSpew_Abort, "Script needs arguments object");
        return Method_CantCompile;
    }

    if (!emitPrologue())
        return Method_Error;

    MethodStatus status = emitBody();
    if (status != Method_Compiled)
        return status;

    if (!emitEpilogue())
        return Method_Error;

    if (masm.oom())
        return Method_Error;

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    if (!code)
        return Method_Error;

    JS_ASSERT(!script->hasBaselineScript());

    script->baseline = BaselineScript::New(cx, caches_.length());
    if (!script->baseline)
        return Method_Error;

    IonSpew(IonSpew_Codegen, "Created BaselineScript %p (raw %p)",
            (void *) script->baseline, (void *) code->raw());

    script->baseline->setMethod(code);

    if (caches_.length())
        script->baseline->copyCacheEntries(&caches_[0], masm);

    return Method_Compiled;
}

#ifdef DEBUG
#define SPEW_OPCODE()                                                         \
    JS_BEGIN_MACRO                                                            \
        if (IsJaegerSpewChannelActive(JSpew_JSOps)) {                         \
            Sprinter sprinter(cx);                                            \
            sprinter.init();                                                  \
            RootedScript script_(cx, script);                                 \
            js_Disassemble1(cx, script_, pc, pc - script_->code,              \
                            JS_TRUE, &sprinter);                              \
            JaegerSpew(JSpew_JSOps, "    %2u %s",                             \
                       (unsigned)frame.stackDepth(), sprinter.string());      \
        }                                                                     \
    JS_END_MACRO;
#else
#define SPEW_OPCODE()
#endif 

bool
BaselineCompiler::emitPrologue()
{
    masm.push(BaselineFrameReg);
    masm.mov(BaselineStackReg, BaselineFrameReg);

    masm.subPtr(Imm32(BaselineFrame::Size()), BaselineStackReg);

    
    masm.moveValue(UndefinedValue(), R0);
    for (size_t i = 0; i < frame.nlocals(); i++)
        masm.pushValue(R0);

    return true;
}

bool
BaselineCompiler::emitEpilogue()
{
    masm.bind(&return_);

    masm.mov(BaselineFrameReg, BaselineStackReg);
    masm.pop(BaselineFrameReg);

    masm.ret();
    return true;
}

MethodStatus
BaselineCompiler::emitBody()
{
    pc = script->code;

    while (true) {
        SPEW_OPCODE();
        JSOp op = JSOp(*pc);

        frame.assertValidState(pc);

        masm.bind(labelOf(pc));

        switch (op) {
          default:
            IonSpew(IonSpew_Abort, "Unhandled op: %s", js_CodeName[op]);
            return Method_CantCompile;

#define EMIT_OP(OP)                            \
          case OP:                             \
            if (!this->emit_##OP())            \
                return Method_Error;           \
            break;
OPCODE_LIST(EMIT_OP)
#undef EMIT_OP
        }

        if (op == JSOP_STOP)
            break;

        pc += GetBytecodeLength(pc);
    }

    JS_ASSERT(JSOp(*pc) == JSOP_STOP);
    JS_ASSERT(frame.stackDepth() == 0);
    return Method_Compiled;
}

bool
BaselineCompiler::emit_JSOP_NOP()
{
    return true;
}

bool
BaselineCompiler::emit_JSOP_POP()
{
    frame.pop();
    return true;
}

bool
BaselineCompiler::emit_JSOP_GOTO()
{
    frame.syncStack(0);

    jsbytecode *target = pc + GET_JUMP_OFFSET(pc);
    masm.jump(labelOf(target));
    return true;
}

bool
BaselineCompiler::emit_JSOP_IFNE()
{
    frame.popRegsAndSync(1);

    
    
    
    jsbytecode *target = pc + GET_JUMP_OFFSET(pc);
    masm.branchTest32(Assembler::NonZero, R0.payloadReg(), R0.payloadReg(), labelOf(target));
    return true;
}

bool
BaselineCompiler::emit_JSOP_LOOPHEAD()
{
    return true;
}

bool
BaselineCompiler::emit_JSOP_LOOPENTRY()
{
    return true;
}

bool
BaselineCompiler::emit_JSOP_ZERO()
{
    frame.push(Int32Value(0));
    return true;
}

bool
BaselineCompiler::emit_JSOP_ONE()
{
    frame.push(Int32Value(1));
    return true;
}

bool
BaselineCompiler::emit_JSOP_INT8()
{
    frame.push(Int32Value(GET_INT8(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_INT32()
{
    frame.push(Int32Value(GET_INT32(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_UINT16()
{
    frame.push(Int32Value(GET_UINT16(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_UINT24()
{
    frame.push(Int32Value(GET_UINT24(pc)));
    return true;
}

void
BaselineCompiler::storeValue(const StackValue *source, const Address &dest,
                             const ValueOperand &scratch)
{
    switch (source->kind()) {
      case StackValue::Constant:
        masm.storeValue(source->constant(), dest);
        break;
      case StackValue::Register:
        masm.storeValue(source->reg(), dest);
        break;
      case StackValue::LocalSlot:
        masm.loadValue(frame.addressOfLocal(source->localSlot()), scratch);
        masm.storeValue(scratch, dest);
        break;
      case StackValue::ArgSlot:
        masm.loadValue(frame.addressOfArg(source->argSlot()), scratch);
        masm.storeValue(scratch, dest);
        break;
      case StackValue::Stack:
        masm.loadValue(frame.addressOfStackValue(source), scratch);
        masm.storeValue(scratch, dest);
        break;
      default:
        JS_NOT_REACHED("Invalid kind");
    }
}

bool
BaselineCompiler::emit_JSOP_ADD()
{
    
    frame.popRegsAndSync(2);

    Label done;
#if 0
    
    {
        Label notInt32, overflow;
        masm.branchTestInt32(Assembler::NotEqual, R0, &notInt32);
        masm.branchTestInt32(Assembler::NotEqual, R1, &notInt32);

        masm.addl(R1.payloadReg(), R0.payloadReg());
        masm.j(Assembler::Overflow, &overflow);
        
        masm.jump(&done);

        
        masm.bind(&overflow);
        

        
        masm.bind(&notInt32);
    }
#endif

    CacheData data;
    BinaryOpCache cache(data, pc);
    IonCode *stub = cache.getCode(cx);
    data.call = masm.callWithPatch(stub);

    if (!allocateCache(cache))
        return false;

    masm.bind(&done);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_LT()
{
    
    frame.popRegsAndSync(2);

    Label done;
#if 0
    
    {
        Label notInt32;
        masm.branchTestInt32(Assembler::NotEqual, R0, &notInt32);
        masm.branchTestInt32(Assembler::NotEqual, R1, &notInt32);

        masm.cmpl(R0.payloadReg(), R1.payloadReg());

        switch (JSOp(*pc)) {
        case JSOP_LT:
            masm.setCC(Assembler::LessThan, R0.payloadReg());
            break;

        default:
            JS_NOT_REACHED("Unexpected compare op");
            break;
        }

        masm.movzxbl(R0.payloadReg(), R0.payloadReg());
        masm.movl(ImmType(JSVAL_TYPE_BOOLEAN), R0.typeReg());
        masm.jump(&done);

        masm.bind(&notInt32);
    }
#endif

    CacheData data;
    CompareCache cache(data, pc);
    IonCode *stub = cache.getCode(cx);
    data.call = masm.callWithPatch(stub);

    if (!allocateCache(cache))
        return false;

    masm.bind(&done);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_GETLOCAL()
{
    uint32_t local = GET_SLOTNO(pc);
    frame.pushLocal(local);
    return true;
}

bool
BaselineCompiler::emit_JSOP_SETLOCAL()
{
    
    
    frame.syncStack(1);

    uint32_t local = GET_SLOTNO(pc);
    storeValue(frame.peek(-1), frame.addressOfLocal(local), R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_GETARG()
{
    
    JS_ASSERT(!script->argsObjAliasesFormals());

    uint32_t arg = GET_SLOTNO(pc);
    frame.pushArg(arg);
    return true;
}

bool
BaselineCompiler::emit_JSOP_SETARG()
{
    
    JS_ASSERT(!script->argsObjAliasesFormals());

    
    frame.syncStack(1);

    uint32_t arg = GET_SLOTNO(pc);
    storeValue(frame.peek(-1), frame.addressOfArg(arg), R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_RETURN()
{
    JS_ASSERT(frame.stackDepth() == 1);

    frame.popValue(JSReturnOperand);
    masm.jump(&return_);
    return true;
}

bool
BaselineCompiler::emit_JSOP_STOP()
{
    JS_ASSERT(frame.stackDepth() == 0);

    masm.moveValue(UndefinedValue(), JSReturnOperand);

    
    return true;
}
