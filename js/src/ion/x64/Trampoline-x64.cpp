








































#include "jscompartment.h"
#include "assembler/assembler/MacroAssembler.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"
#include "ion/IonFrames.h"
#include "ion/Bailouts.h"
#include "ion/VMFunctions.h"
#include "ion/IonSpewer.h"

using namespace js;
using namespace js::ion;





IonCode *
IonCompartment::generateOsrPrologue(JSContext *cx)
{
    MacroAssembler masm(cx);
    
#if defined(_WIN64)
    const Operand fp = Operand(rbp, 16 + ShadowStackSpace);
    masm.movq(fp, OsrFrameReg);
#else
    JS_ASSERT(OsrFrameReg == ArgReg5); 
#endif

    
    
    JS_ASSERT(enterJIT_);
    masm.jmp(enterJIT_);

    Linker linker(masm);
    return linker.newCode(cx);
}






IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    MacroAssembler masm(cx);

    const Register reg_code = ArgReg0;
    const Register reg_argc = ArgReg1;
    const Register reg_argv = ArgReg2;
    const Register reg_vp   = ArgReg3;
#if defined(_WIN64)
    const Operand token = Operand(rbp, 8 + ShadowStackSpace);
    
#else
    const Register token = ArgReg4;
    
#endif

    
    masm.push(rbp);
    masm.mov(rsp, rbp);

    
    
    
    masm.push(rbx);
    masm.push(r12);
    masm.push(r13);
    masm.push(r14);
    masm.push(r15);
#if defined(_WIN64)
    masm.push(rdi);
    masm.push(rsi);
#endif

    
    masm.push(reg_vp);

    
    masm.mov(rsp, r14);

    
    masm.mov(reg_argc, r13);
    masm.shll(Imm32(3), r13);

    
    
    
    
    masm.mov(rsp, r12);
    masm.subq(r13, r12);
    masm.subq(Imm32(8), r12);
    masm.andl(Imm32(0xf), r12);
    masm.subq(r12, rsp);

    



    
    masm.addq(reg_argv, r13); 

    
    {
        Label header, footer;
        masm.bind(&header);

        masm.cmpq(r13, reg_argv);
        masm.j(AssemblerX86Shared::BelowOrEqual, &footer);

        masm.subq(Imm32(8), r13);
        masm.push(Operand(r13, 0));
        masm.jmp(&header);

        masm.bind(&footer);
    }

    
    
    masm.push(token);

    


    masm.subq(rsp, r14);
    masm.shlq(Imm32(FRAMETYPE_BITS), r14);
    masm.orl(Imm32(IonFrame_Entry), r14);
    masm.push(r14);

    
    masm.call(reg_code);

    
    masm.pop(r14);              
    masm.shrq(Imm32(FRAMETYPE_BITS), r14);
    masm.addq(r14, rsp);        

    


    masm.pop(r12); 
    masm.storeValue(JSReturnOperand, Operand(r12, 0));

    
#if defined(_WIN64)
    masm.pop(rsi);
    masm.pop(rdi);
#endif
    masm.pop(r15);
    masm.pop(r14);
    masm.pop(r13);
    masm.pop(r12);
    masm.pop(rbx);

    
    masm.pop(rbp);
    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx);
}

IonCode *
IonCompartment::generateReturnError(JSContext *cx)
{
    MacroAssembler masm(cx);

    masm.pop(r14);              
    masm.xorl(Imm32(0x1), r14); 
    masm.addq(r14, rsp);        
    masm.pop(r11);              

    Linker linker(masm);
    return linker.newCode(cx);
}

static void
GenerateBailoutTail(MacroAssembler &masm)
{
    masm.linkExitFrame();

    Label reflow;
    Label interpret;
    Label exception;

    
    
    
    
    
    

    masm.cmpl(rax, Imm32(BAILOUT_RETURN_FATAL_ERROR));
    masm.j(Assembler::LessThan, &interpret);
    masm.j(Assembler::Equal, &exception);

    masm.cmpl(rax, Imm32(BAILOUT_RETURN_RECOMPILE_CHECK));
    masm.j(Assembler::LessThan, &reflow);

    
    masm.setupUnalignedABICall(0, rdx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, RecompileForInlining));

    masm.testl(rax, rax);
    masm.j(Assembler::Zero, &exception);

    masm.jmp(&interpret);

    
    masm.bind(&reflow);
    masm.setupUnalignedABICall(1, rdx);
    masm.setABIArg(0, rax);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ReflowTypeInfo));

    masm.testl(rax, rax);
    masm.j(Assembler::Zero, &exception);

    masm.bind(&interpret);
    
    masm.subq(Imm32(sizeof(Value)), rsp);
    masm.movq(rsp, rcx);

    
    masm.setupUnalignedABICall(1, rdx);
    masm.setABIArg(0, rcx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ThunkToInterpreter));

    
    masm.popValue(JSReturnOperand);

    
    masm.testl(rax, rax);
    masm.j(Assembler::Zero, &exception);

    
    masm.ret();

    masm.bind(&exception);
    masm.handleException();
}

IonCode *
IonCompartment::generateInvalidator(JSContext *cx)
{
    AutoIonContextAlloc aica(cx);
    MacroAssembler masm(cx);

    

    
    masm.reserveStack(Registers::Total * sizeof(void *));
    for (uint32 i = 0; i < Registers::Total; i++)
        masm.movq(Register::FromCode(i), Operand(rsp, i * sizeof(void *)));

    
    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32 i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(rsp, i * sizeof(double)));

    masm.movq(rsp, rbx); 

    
    masm.reserveStack(sizeof(size_t));
    masm.movq(rsp, rcx);

    masm.setupUnalignedABICall(2, rdx);
    masm.setABIArg(0, rbx);
    masm.setABIArg(1, rcx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, InvalidationBailout));

    masm.pop(rbx); 

    
    const uint32 BailoutDataSize = sizeof(double) * FloatRegisters::Total +
                                   sizeof(void *) * Registers::Total;
    masm.lea(Operand(rsp, rbx, TimesOne, BailoutDataSize), rsp);

    GenerateBailoutTail(masm);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx);
    IonSpew(IonSpew_Invalidate, "   invalidation thunk created at %p", (void *) code->raw());
    return code;
}

IonCode *
IonCompartment::generateArgumentsRectifier(JSContext *cx)
{
    MacroAssembler masm(cx);

    
    
    JS_ASSERT(ArgumentsRectifierReg == r8);

    
    masm.movq(Operand(rsp, IonJSFrameLayout::offsetOfCalleeToken()), rax);
    masm.load16(Operand(rax, offsetof(JSFunction, nargs)), rcx);
    masm.subq(r8, rcx);

    masm.moveValue(UndefinedValue(), r10);

    masm.movq(rsp, rbp); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(r10);
        masm.subl(Imm32(1), rcx);

        masm.testl(rcx, rcx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    masm.movq(r8, r9);
    masm.shlq(Imm32(3), r9); 

    masm.movq(rbp, rcx);
    masm.addq(Imm32(sizeof(IonRectifierFrameLayout)), rcx);
    masm.addq(r9, rcx);

    
    {
        Label copyLoopTop, initialSkip;

        masm.jump(&initialSkip);

        masm.bind(&copyLoopTop);
        masm.subq(Imm32(sizeof(Value)), rcx);
        masm.subl(Imm32(1), r8);
        masm.bind(&initialSkip);

        masm.mov(Operand(rcx, 0x0), rdx);
        masm.push(rdx);

        masm.testl(r8, r8);
        masm.j(Assembler::NonZero, &copyLoopTop);
    }

    
    masm.subq(rsp, rbp);
    masm.shlq(Imm32(FRAMETYPE_BITS), rbp);
    masm.orq(Imm32(IonFrame_Rectifier), rbp);

    
    masm.push(rax); 
    masm.push(rbp); 

    
    
    masm.movq(Operand(rax, offsetof(JSFunction, u.i.script_)), rax);
    masm.movq(Operand(rax, offsetof(JSScript, ion)), rax);
    masm.movq(Operand(rax, offsetof(IonScript, method_)), rax);
    masm.movq(Operand(rax, IonCode::OffsetOfCode()), rax);
    masm.call(rax);

    
    masm.pop(rbp);            
    masm.shrq(Imm32(FRAMETYPE_BITS), rbp);
    masm.pop(r11);            
    masm.addq(rbp, rsp);      

    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx);
}

static void
GenerateBailoutThunk(JSContext *cx, MacroAssembler &masm, uint32 frameClass)
{
    
    masm.reserveStack(Registers::Total * sizeof(void *));
    for (uint32 i = 0; i < Registers::Total; i++)
        masm.movq(Register::FromCode(i), Operand(rsp, i * sizeof(void *)));

    
    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32 i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(rsp, i * sizeof(double)));

    
    masm.movq(rsp, r8);

    
    masm.setupUnalignedABICall(1, rax);
    masm.setABIArg(0, r8);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));

    
    
    
    
    
    
    
    static const uint32 BailoutDataSize = sizeof(void *) * Registers::Total +
                                          sizeof(double) * FloatRegisters::Total;
    masm.addq(Imm32(BailoutDataSize), rsp);
    masm.pop(rcx);
    masm.lea(Operand(rsp, rcx, TimesOne, sizeof(void *)), rsp);

    GenerateBailoutTail(masm);
}

IonCode *
IonCompartment::generateBailoutTable(JSContext *cx, uint32 frameClass)
{
    JS_NOT_REACHED("x64 does not use bailout tables");
    return NULL;
}

IonCode *
IonCompartment::generateBailoutHandler(JSContext *cx)
{
    MacroAssembler masm;

    GenerateBailoutThunk(cx, masm, NO_FRAME_SIZE_CLASS_ID);

    Linker linker(masm);
    return linker.newCode(cx);
}

IonCode *
IonCompartment::generateVMWrapper(JSContext *cx, const VMFunction &f)
{
    typedef MoveResolver::MoveOperand MoveOperand;

    JS_ASSERT(functionWrappers_);
    JS_ASSERT(functionWrappers_->initialized());
    VMWrapperMap::AddPtr p = functionWrappers_->lookupForAdd(&f);
    if (p)
        return p->value;

    
    MacroAssembler masm;

    
    
    GeneralRegisterSet regs =
        GeneralRegisterSet::Not(GeneralRegisterSet(Register::Codes::VolatileMask));

    
    
    
    
    
    
    
    
    masm.linkExitFrame();

    
    masm.push(Operand(rsp, 0));

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = regs.takeAny();
        masm.lea(Operand(rsp, sizeof(IonExitFrameLayout) + sizeof(uintptr_t)), argsBase);
    }

    
    Register outReg = InvalidReg;
    if (f.outParam == Type_Value) {
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(Value));
        masm.movq(rsp, outReg);
    }

    Register temp = regs.getAny();
    masm.setupUnalignedABICall(f.argc(), temp);

    
    Register cxreg = ArgReg0;
    masm.movePtr(ImmWord(JS_THREAD_DATA(cx)), cxreg);
    masm.loadPtr(Address(cxreg, offsetof(ThreadData, ionJSContext)), cxreg);
    masm.setABIArg(0, cxreg);

    size_t argDisp = 0;
    size_t argc = 1;

    
    if (f.explicitArgs) {
        for (uint32 explicitArg = 0; explicitArg < f.explicitArgs; explicitArg++) {
            MoveOperand from;
            switch (f.argProperties(explicitArg)) {
              case VMFunction::WordByValue:
                masm.setABIArg(argc++, MoveOperand(argsBase, argDisp));
                argDisp += sizeof(void *);
                break;
              case VMFunction::DoubleByValue:
                masm.setABIArg(argc++, MoveOperand(argsBase, argDisp));
                argDisp += sizeof(void *);
                masm.setABIArg(argc++, MoveOperand(argsBase, argDisp));
                argDisp += sizeof(void *);
                break;
              case VMFunction::WordByRef:
                masm.setABIArg(argc++, MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE));
                argDisp += sizeof(void *);
                break;
              case VMFunction::DoubleByRef:
                masm.setABIArg(argc++, MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE));
                argDisp += 2 * sizeof(void *);
                break;
            }
        }
    }

    
    if (outReg != InvalidReg)
        masm.setABIArg(argc++, outReg);
    JS_ASSERT(f.argc() == argc);

    masm.callWithABI(f.wrapped);

    
    Label exception;
    switch (f.failType()) {
      case Type_Object:
        masm.testq(rax, rax);
        masm.j(Assembler::Zero, &exception);
        break;
      case Type_Bool:
        masm.testl(eax, eax);
        masm.j(Assembler::Zero, &exception);
        break;
      default:
        JS_NOT_REACHED("unknown failure kind");
        break;
    }

    
    if (f.outParam == Type_Value) {
        masm.loadValue(Address(esp, 0), JSReturnOperand);
        masm.freeStack(sizeof(Value));
    }

    
    
    Label invalidated;
    masm.pop(ScratchReg);
    masm.cmpq(ScratchReg, Operand(rsp, 0));
    masm.j(Assembler::NotEqual, &invalidated);

    masm.retn(Imm32(sizeof(IonExitFrameLayout) + f.explicitStackSlots() * sizeof(void *)));

    masm.bind(&exception);
    masm.handleException();

    masm.bind(&invalidated);
    masm.cmpq(Operand(rsp, 0), Imm32(0));
    masm.j(Assembler::Equal, &exception);
    masm.ret();

    Linker linker(masm);
    IonCode *wrapper = linker.newCode(cx);
    if (!wrapper || !functionWrappers_->add(p, &f, wrapper))
        return NULL;

    return wrapper;
}
