






#include "BaselineJIT.h"
#include "BaselineIC.h"
#include "BaselineHelpers.h"
#include "BaselineCompiler.h"
#include "FixedList.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "VMFunctions.h"
#include "IonFrames-inl.h"

using namespace js;
using namespace js::ion;

BaselineCompiler::BaselineCompiler(JSContext *cx, JSScript *script)
  : BaselineCompilerSpecific(cx, script),
    return_(new HeapLabel())
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

    BaselineScript *baselineScript = BaselineScript::New(cx, icEntries_.length());
    if (!baselineScript)
        return Method_Error;
    script->baseline = baselineScript;

    IonSpew(IonSpew_Codegen, "Created BaselineScript %p (raw %p)",
            (void *) script->baseline, (void *) code->raw());

    script->baseline->setMethod(code);

    
    if (icEntries_.length())
        baselineScript->copyICEntries(&icEntries_[0], masm);

    
    for (size_t i = 0; i < icLoadLabels_.length(); i++) {
        CodeOffsetLabel label = icLoadLabels_[i];
        label.fixup(&masm);
        ICEntry *entryAddr = &(baselineScript->icEntry(i));
        Assembler::patchDataWithValueCheck(CodeLocationLabel(code, label),
                                           ImmWord(uintptr_t(entryAddr)),
                                           ImmWord(uintptr_t(-1)));
    }

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
    masm.bind(return_);

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
        IonSpew(IonSpew_Scripts, "Compiling op: %s", js_CodeName[op]);
        frame.assertValidState(pc);

        masm.bind(labelOf(pc));

        switch (op) {
          default:
            
            if (js_CodeSpec[op].format & JOF_DECOMPOSE)
                break;
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
BaselineCompiler::emit_JSOP_LABEL()
{
    return true;
}

bool
BaselineCompiler::emit_JSOP_NOTEARG()
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
BaselineCompiler::emit_JSOP_DUP()
{
    
    
    
    frame.popRegsAndSync(1);
    masm.moveValue(R0, R1);

    
    frame.push(R1);
    frame.push(R0);
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
    
    ICToBool_Fallback::Compiler stubCompiler(cx);
    ICEntry *entry = allocateICEntry(stubCompiler.getStub());
    if (!entry)
        return false;

    
    
    
    frame.popRegsAndSync(1);

    
    CodeOffsetLabel patchOffset;
    EmitCallIC(&patchOffset, masm);
    entry->setReturnOffset(masm.currentOffset());
    if (!addICLoadLabel(patchOffset))
        return false;

    
    masm.branchTestBooleanTruthy(true, R0, labelOf(pc + GET_JUMP_OFFSET(pc)));

    return true;
}

bool
BaselineCompiler::emit_JSOP_POS()
{
    
    ICToNumber_Fallback::Compiler stubCompiler(cx);
    ICEntry *entry = allocateICEntry(stubCompiler.getStub());
    if (!entry)
        return false;

    
    frame.popRegsAndSync(1);

    
    Label done;
    masm.branchTestNumber(Assembler::Equal, R0, &done);

    
    CodeOffsetLabel patchOffset;
    EmitCallIC(&patchOffset, masm);
    entry->setReturnOffset(masm.currentOffset());
    if (!addICLoadLabel(patchOffset))
        return false;

    masm.bind(&done);
    frame.push(R0);
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
BaselineCompiler::emit_JSOP_VOID()
{
    frame.pop();
    frame.push(UndefinedValue());
    return true;
}

bool
BaselineCompiler::emit_JSOP_UNDEFINED()
{
    frame.push(UndefinedValue());
    return true;
}

bool
BaselineCompiler::emit_JSOP_HOLE()
{
    frame.push(MagicValue(JS_ARRAY_HOLE));
    return true;
}

bool
BaselineCompiler::emit_JSOP_NULL()
{
    frame.push(NullValue());
    return true;
}

bool
BaselineCompiler::emit_JSOP_TRUE()
{
    frame.push(BooleanValue(true));
    return true;
}

bool
BaselineCompiler::emit_JSOP_FALSE()
{
    frame.push(BooleanValue(false));
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

bool
BaselineCompiler::emit_JSOP_DOUBLE()
{
    frame.push(script->getConst(GET_UINT32_INDEX(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_STRING()
{
    frame.push(StringValue(script->getAtom(GET_UINT32_INDEX(pc))));
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
    
    ICBinaryArith_Fallback::Compiler stubCompiler(cx);
    ICEntry *entry = allocateICEntry(stubCompiler.getStub());
    if (!entry)
        return false;

    
    
    
    frame.popRegsAndSync(2);

    
    CodeOffsetLabel patchOffset;
    EmitCallIC(&patchOffset, masm);
    entry->setReturnOffset(masm.currentOffset());
    if (!addICLoadLabel(patchOffset))
        return false;

    
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_LT()
{
    return emitCompare();
}

bool
BaselineCompiler::emit_JSOP_GT()
{
    return emitCompare();
}

bool
BaselineCompiler::emitCompare()
{
    
    ICCompare_Fallback::Compiler stubCompiler(cx);
    ICEntry *entry = allocateICEntry(stubCompiler.getStub());
    if (!entry)
        return false;

    

    
    frame.popRegsAndSync(2);

    
    CodeOffsetLabel patchOffset;
    EmitCallIC(&patchOffset, masm);
    entry->setReturnOffset(masm.currentOffset());
    if (!addICLoadLabel(patchOffset))
        return false;

    
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_GETLOCAL()
{
    uint32_t local = GET_SLOTNO(pc);

    if (local >= frame.nlocals()) {
        
        frame.syncStack(0);
        masm.loadValue(Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfLocal(local)), R0);
        frame.push(R0);
        return true;
    }

    frame.pushLocal(local);
    return true;
}

bool
BaselineCompiler::emit_JSOP_CALLLOCAL()
{
    return emit_JSOP_GETLOCAL();
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
BaselineCompiler::emit_JSOP_CALLARG()
{
    return emit_JSOP_GETARG();
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
BaselineCompiler::emitCall()
{
    JS_ASSERT(js_CodeSpec[*pc].format & JOF_INVOKE);

    uint32_t argc = GET_ARGC(pc);

    frame.syncStack(0);
    masm.mov(Imm32(argc), R0.scratchReg());

    
    ICCall_Fallback::Compiler stubCompiler(cx);
    ICEntry *entry = allocateICEntry(stubCompiler.getStub());
    if (!entry)
        return false;

    
    CodeOffsetLabel patchOffset;
    EmitCallIC(&patchOffset, masm);
    entry->setReturnOffset(masm.currentOffset());
    if (!addICLoadLabel(patchOffset))
        return false;

    
    
    masm.addPtr(Imm32((argc + 2) * sizeof(Value) * 2), BaselineStackReg);

    
    frame.popn(argc + 2, DontAdjustStack);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_CALL()
{
    return emitCall();
}

bool
BaselineCompiler::emit_JSOP_NEW()
{
    return emitCall();
}

bool
BaselineCompiler::emit_JSOP_FUNCALL()
{
    return emitCall();
}

bool
BaselineCompiler::emit_JSOP_FUNAPPLY()
{
    return emitCall();
}

bool
BaselineCompiler::emit_JSOP_RETURN()
{
    JS_ASSERT(frame.stackDepth() == 1);

    frame.popValue(JSReturnOperand);
    masm.jump(return_);
    return true;
}

bool
BaselineCompiler::emit_JSOP_STOP()
{
    JS_ASSERT(frame.stackDepth() == 0);

    masm.moveValue(UndefinedValue(), JSReturnOperand);

    
    return true;
}
