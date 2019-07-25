






































#include "MethodJIT.h"
#include "Compiler.h"
#include "StubCalls.h"
#include "assembler/jit/ExecutableAllocator.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;

#if defined(JS_METHODJIT_SPEW)
static const char *OpcodeNames[] = {
# define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) #name,
# include "jsopcode.tbl"
# undef OPDEF
};
#endif


#if ENABLE_ASSEMBLER && WTF_CPU_X86 && !WTF_PLATFORM_MAC
JSC::MacroAssemblerX86Common::SSE2CheckState JSC::MacroAssemblerX86Common::s_sse2CheckState =
NotCheckedSSE2; 
#endif 

mjit::Compiler::Compiler(JSContext *cx, JSScript *script, JSFunction *fun, JSObject *scopeChain)
  : cx(cx), script(script), scopeChain(scopeChain), globalObj(scopeChain->getGlobal()), fun(fun),
    analysis(cx, script), jumpMap(NULL), frame(cx, script, masm),
    branchPatches(ContextAllocPolicy(cx)), stubcc(cx, *this, frame, script)
{
}

#define CHECK_STATUS(expr)              \
    JS_BEGIN_MACRO                      \
        CompileStatus status_ = (expr); \
        if (status_ != Compile_Okay)    \
            return status_;             \
    JS_END_MACRO

CompileStatus
mjit::Compiler::Compile()
{
    JS_ASSERT(!script->ncode);

    JaegerSpew(JSpew_Scripts, "compiling script (file \"%s\") (line \"%d\") (length \"%d\")\n",
                           script->filename, script->lineno, script->length);

    
    if (!analysis.analyze()) {
        if (analysis.OOM())
            return Compile_Error;
        JaegerSpew(JSpew_Abort, "couldn't analyze bytecode; probably switchX or OOM\n");
        return Compile_Abort;
    }

    uint32 nargs = fun ? fun->nargs : 0;
    if (!frame.init(nargs) || !stubcc.init(nargs))
        return Compile_Abort;

    jumpMap = (Label *)cx->malloc(sizeof(Label) * script->length);
    if (!jumpMap)
        return Compile_Error;
#ifdef DEBUG
    for (uint32 i = 0; i < script->length; i++)
        jumpMap[i] = Label();
#endif

#ifdef JS_TRACER
    if (script->tracePoints) {
        script->trees = (TraceTreeCache*)cx->malloc(script->tracePoints * sizeof(TraceTreeCache));
        if (!script->trees)
            return Compile_Abort;
        memset(script->trees, 0, script->tracePoints * sizeof(TraceTreeCache));
    }
#endif

    Profiler prof;
    prof.start();

    CHECK_STATUS(generatePrologue());
    CHECK_STATUS(generateMethod());
    CHECK_STATUS(generateEpilogue());
    CHECK_STATUS(finishThisUp());

#ifdef JS_METHODJIT_SPEW
    prof.stop();
    JaegerSpew(JSpew_Prof, "compilation took %d us\n", prof.time_us());
#endif

    JaegerSpew(JSpew_Scripts, "successfully compiled (code \"%p\") (size \"%ld\")\n",
               (void*)script->ncode, masm.size() + stubcc.size());

    return Compile_Okay;
}

#undef CHECK_STATUS

mjit::Compiler::~Compiler()
{
    cx->free(jumpMap);
}

CompileStatus
mjit::TryCompile(JSContext *cx, JSScript *script, JSFunction *fun, JSObject *scopeChain)
{
    Compiler cc(cx, script, fun, scopeChain);

    JS_ASSERT(!script->ncode);

    CompileStatus status = cc.Compile();
    if (status != Compile_Okay)
        script->ncode = JS_UNJITTABLE_METHOD;

    return status;
}

CompileStatus
mjit::Compiler::generatePrologue()
{
#ifdef JS_CPU_ARM
    









    masm.storePtr(ARMRegisters::lr, FrameAddress(offsetof(VMFrame, scriptedReturn)));
#endif

    return Compile_Okay;
}

CompileStatus
mjit::Compiler::generateEpilogue()
{
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::finishThisUp()
{
    for (size_t i = 0; i < branchPatches.length(); i++) {
        Label label = labelOf(branchPatches[i].pc);
        branchPatches[i].jump.linkTo(label, &masm);
    }

    JSC::ExecutablePool *execPool = getExecPool(masm.size() + stubcc.size());
    if (!execPool)
        return Compile_Abort;

    uint8 *result = (uint8 *)execPool->alloc(masm.size() + stubcc.size());
    JSC::ExecutableAllocator::makeWritable(result, masm.size() + stubcc.size());
    memcpy(result, masm.buffer(), masm.size());
    memcpy(result + masm.size(), stubcc.buffer(), stubcc.size());

    
    stubcc.fixCrossJumps(result, masm.size(), masm.size() + stubcc.size());

    
    masm.finalize(result);
    stubcc.finalize(result + masm.size());

    JSC::ExecutableAllocator::makeExecutable(result, masm.size() + stubcc.size());
    JSC::ExecutableAllocator::cacheFlush(result, masm.size() + stubcc.size());

    script->ncode = result;
#ifdef DEBUG
    script->jitLength = masm.size() + stubcc.size();
#endif
    script->execPool = execPool;

    return Compile_Okay;
}

#define BEGIN_CASE(name)        case name:
#define END_CASE(name)                      \
    JS_BEGIN_MACRO                          \
        PC += name##_LENGTH;                \
    JS_END_MACRO;                           \
    break;

CompileStatus
mjit::Compiler::generateMethod()
{
    PC = script->code;

    for (;;) {
        JSOp op = JSOp(*PC);

        OpcodeStatus &opinfo = analysis[PC];
        if (opinfo.nincoming) {
            opinfo.safePoint = true;
            frame.forgetEverything(opinfo.stackDepth);
            jumpMap[uint32(PC - script->code)] = masm.label();
        }

        if (!opinfo.visited) {
            if (op == JSOP_STOP)
                break;
            if (js_CodeSpec[op].length != -1)
                PC += js_CodeSpec[op].length;
            else
                PC += js_GetVariableBytecodeLength(PC);
            continue;
        }

#ifdef DEBUG
        if (IsJaegerSpewChannelActive(JSpew_JSOps)) {
            JaegerSpew(JSpew_JSOps, "    %2d ", frame.stackDepth());
            js_Disassemble1(cx, script, PC, PC - script->code,
                            JS_TRUE, stdout);
        }
#endif

        JS_ASSERT(frame.stackDepth() == opinfo.stackDepth);

    

 

        switch (op) {
          BEGIN_CASE(JSOP_GOTO)
          {
            
            frame.forgetEverything();
            Jump j = masm.jump();
            jumpInScript(j, PC + GET_JUMP_OFFSET(PC));
          }
          END_CASE(JSOP_GOTO)

          BEGIN_CASE(JSOP_BITAND)
            jsop_bitop(op);
          END_CASE(JSOP_BITAND)

          BEGIN_CASE(JSOP_NAME)
            prepareStubCall();
            masm.move(Imm32(fullAtomIndex(PC)), Registers::ArgReg1);
            stubCall(stubs::Name, Uses(0), Defs(1));
            frame.pushSynced();
          END_CASE(JSOP_NAME)

          BEGIN_CASE(JSOP_DOUBLE)
          {
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);
            double d = *ATOM_TO_DOUBLE(atom);
            frame.push(Value(DoubleTag(d)));
          }
          END_CASE(JSOP_DOUBLE)

          BEGIN_CASE(JSOP_ZERO)
            frame.push(Valueify(JSVAL_ZERO));
          END_CASE(JSOP_ZERO)

          BEGIN_CASE(JSOP_ONE)
            frame.push(Valueify(JSVAL_ONE));
          END_CASE(JSOP_ONE)

          BEGIN_CASE(JSOP_POP)
            frame.pop();
          END_CASE(JSOP_POP)

          BEGIN_CASE(JSOP_UINT16)
            frame.push(Value(Int32Tag((int32_t) GET_UINT16(PC))));
          END_CASE(JSOP_UINT16)

          BEGIN_CASE(JSOP_BINDNAME)
            jsop_bindname(fullAtomIndex(PC));
          END_CASE(JSOP_BINDNAME)

          BEGIN_CASE(JSOP_SETNAME)
            prepareStubCall();
            masm.move(Imm32(fullAtomIndex(PC)), Registers::ArgReg1);
            stubCall(stubs::SetName, Uses(2), Defs(1));
            frame.pop();
          END_CASE(JSOP_SETNAME)

          BEGIN_CASE(JSOP_UINT24)
            frame.push(Value(Int32Tag((int32_t) GET_UINT24(PC))));
          END_CASE(JSOP_UINT24)

          BEGIN_CASE(JSOP_STOP)
            
            masm.storeValue(Value(UndefinedTag()),
                            Address(Assembler::FpReg, offsetof(JSStackFrame, rval)));
            emitReturn();
            goto done;
          END_CASE(JSOP_STOP)

          BEGIN_CASE(JSOP_INT8)
            frame.push(Value(Int32Tag(GET_INT8(PC))));
          END_CASE(JSOP_INT8)

          BEGIN_CASE(JSOP_INT32)
            frame.push(Value(Int32Tag(GET_INT32(PC))));
          END_CASE(JSOP_INT32)

          BEGIN_CASE(JSOP_TRACE)
          END_CASE(JSOP_TRACE)

          BEGIN_CASE(JSOP_GETGLOBAL)
            jsop_getglobal(GET_SLOTNO(PC));
          END_CASE(JSOP_GETGLOBAL)

          BEGIN_CASE(JSOP_SETGLOBAL)
            jsop_setglobal(GET_SLOTNO(PC));
          END_CASE(JSOP_SETGLOBAL)

          default:
           
#ifdef JS_METHODJIT_SPEW
            JaegerSpew(JSpew_Abort, "opcode %s not handled yet\n", OpcodeNames[op]);
#endif
            return Compile_Abort;
        }

    

 

        frame.assertValidRegisterState();
    }

  done:
    return Compile_Okay;
}

#undef END_CASE
#undef BEGIN_CASE

const JSC::MacroAssembler::Label &
mjit::Compiler::labelOf(jsbytecode *pc)
{
    uint32 offs = uint32(pc - script->code);
    JS_ASSERT(jumpMap[offs].isValid());
    return jumpMap[offs];
}

JSC::ExecutablePool *
mjit::Compiler::getExecPool(size_t size)
{
    ThreadData *jaegerData = &JS_METHODJIT_DATA(cx);
    return jaegerData->execPool->poolForSize(size);
}

uint32
mjit::Compiler::fullAtomIndex(jsbytecode *pc)
{
    return GET_SLOTNO(pc);

    
#if 0
    return GET_SLOTNO(pc) + (atoms - script->atomMap.vector);
#endif
}

void
mjit::Compiler::jumpInScript(Jump j, jsbytecode *pc)
{
    JS_ASSERT(pc >= script->code && uint32(pc - script->code) < script->length);

    

    if (pc < PC)
        j.linkTo(jumpMap[uint32(pc - script->code)], &masm);
    else
        branchPatches.append(BranchPatch(j, pc));
}

void
mjit::Compiler::jsop_setglobal(uint32 index)
{
    JS_ASSERT(globalObj);
    uint32 slot = script->getGlobalSlot(index);

    FrameEntry *fe = frame.peek(-1);
    bool popped = PC[JSOP_SETGLOBAL_LENGTH] == JSOP_POP;

    RegisterID reg = frame.allocReg();
    if (slot < JS_INITIAL_NSLOTS) {
        void *vp = &globalObj->getSlotRef(slot);
        masm.move(ImmPtr(vp), reg);
        frame.storeTo(fe, Address(reg, 0), popped);
    } else {
        masm.move(ImmPtr(&globalObj->dslots), reg);
        masm.loadPtr(reg, reg);
        frame.storeTo(fe, Address(reg, (slot - JS_INITIAL_NSLOTS) * sizeof(Value)), popped);
    }
    frame.freeReg(reg);
}

void
mjit::Compiler::jsop_getglobal(uint32 index)
{
    JS_ASSERT(globalObj);
    uint32 slot = script->getGlobalSlot(index);

    RegisterID reg = frame.allocReg();
    if (slot < JS_INITIAL_NSLOTS) {
        void *vp = &globalObj->getSlotRef(slot);
        masm.move(ImmPtr(vp), reg);
        frame.push(Address(reg, 0));
    } else {
        masm.move(ImmPtr(&globalObj->dslots), reg);
        masm.loadPtr(reg, reg);
        frame.push(Address(reg, (slot - JS_INITIAL_NSLOTS) * sizeof(Value)));
    }
    frame.freeReg(reg);
}

void
mjit::Compiler::emitReturn()
{
#if defined(JS_CPU_ARM)
    masm.loadPtr(FrameAddress(offsetof(VMFrame, scriptedReturn)), ARMRegisters::lr);
#endif
    masm.ret();
}

void
mjit::Compiler::prepareStubCall()
{
    JaegerSpew(JSpew_Insns, " ---- STUB CALL, SYNCING FRAME ---- \n");
    frame.syncAndKill(Registers::TempRegs);
    JaegerSpew(JSpew_Insns, " ---- FRAME SYNCING DONE ---- \n");
}

JSC::MacroAssembler::Call
mjit::Compiler::stubCall(void *ptr, Uses uses, Defs defs)
{
    
    JaegerSpew(JSpew_Insns, " ---- CALLING STUB ---- \n");
    Call cl = masm.stubCall(ptr, PC, frame.stackDepth() + script->nfixed);
    JaegerSpew(JSpew_Insns, " ---- END STUB CALL ---- \n");
    return cl;
}

