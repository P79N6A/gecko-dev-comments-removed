








































#include "jscompartment.h"
#include "assembler/assembler/MacroAssembler.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"
#include "ion/IonFrames.h"
#include "ion/Bailouts.h"

using namespace js;
using namespace js::ion;






IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    const Register reg_code = ArgReg0;
    const Register reg_argc = ArgReg1;
    const Register reg_argv = ArgReg2;
    const Register reg_vp   = ArgReg3;
#if defined(_WIN64)
    const Operand token = Operand(rbp, 8 + ShadowStackSpace);
#else
    const Register token = ArgReg4;
#endif

    MacroAssembler masm(cx);

    
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

    
    
    
    
    
    
    
    
    
    
    Label frameFixupDone;
    masm.movq(Operand(rsp, IonCommonFrameLayout::offsetOfDescriptor()), rcx);
    masm.movq(rcx, rdx);
    masm.andl(Imm32(FRAMETYPE_BITS), rdx);
    masm.cmpl(rdx, Imm32(IonFrame_JS));
    masm.j(Assembler::NotEqual, &frameFixupDone);
    {
        JS_STATIC_ASSERT(sizeof(IonJSFrameLayout) >= sizeof(IonExitFrameLayout));
        ptrdiff_t difference = sizeof(IonJSFrameLayout) - sizeof(IonExitFrameLayout);
        masm.addq(Imm32(difference << FRAMETYPE_BITS), rcx);
        masm.movq(rcx, Operand(esp, IonCommonFrameLayout::offsetOfDescriptor()));
    }
    masm.bind(&frameFixupDone);

    
    

    masm.movq(ImmWord(JS_THREAD_DATA(cx)), rdx);
    masm.movq(rsp, Operand(rdx, offsetof(ThreadData, ionTop)));

    Label exception;

    
    masm.testl(rax, rax);
    masm.j(Assembler::NonZero, &exception);

    
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
IonCompartment::generateCWrapper(JSContext *cx, const VMFunction& f)
{
    typedef MoveResolver::MoveOperand MoveOperand;

    JS_ASSERT(functionWrappers_);
    JS_ASSERT(functionWrappers_->initialized());
    VMWrapperMap::AddPtr p = functionWrappers_->lookupForAdd(&f);
    if (p)
        return p->value;

    
    MacroAssembler masm;
    Register cframe, tmp;

    
    
    const GeneralRegisterSet allocatableRegs(Registers::VolatileMask & ~Registers::ArgRegMask);
    GeneralRegisterSet regs(allocatableRegs);

    
    
    
    
    
    

    
    masm.subPtr(Imm32(offsetof(IonCFrame, returnAddress)), StackPointer);

    
    cframe = StackPointer;

    
    tmp = regs.takeAny();
    masm.mov(Operand(cframe, offsetof(IonCFrame, frameSize)), tmp);

    
    masm.lea(Operand(cframe, tmp, TimesOne, sizeof(IonCFrame) + f.explicitArgs * sizeof(void *)), tmp);
    masm.mov(tmp, Operand(cframe, offsetof(IonCFrame, topFrame)));

    
    
    
    
    
    
    

    
    
    
    
    masm.movePtr(ImmWord(this), tmp);
    masm.mov(cframe, Operand(tmp, offsetof(IonCompartment, topCFrame_)));

    
    masm.setupUnalignedABICall(f.argc(), tmp);
    
    Register outReg = InvalidReg;
    if (f.outParam == VMFunction::OutParam_Value) {
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(Value));
        masm.movl(rsp, outReg);
    }

    
    masm.movePtr(ImmWord(cx), ArgReg0);
    masm.setABIArg(0, ArgReg0);

    
    if (f.explicitArgs) {
        Register argsBase = regs.takeAny();
        masm.lea(Operand(cframe, sizeof(IonCFrame)), argsBase);

        for (uint32 i = 0; i < f.explicitArgs; i++)
            masm.setABIArg(i + 1, MoveOperand(argsBase, i * sizeof(void *)));
    }

    
    if (outReg != InvalidReg)
        masm.setABIArg(f.argc() - 1, outReg);

    masm.callWithABI(f.wrapped);

    
    if (f.outParam == VMFunction::OutParam_Value) {
        masm.loadValue(Operand(esp, 0), JSReturnOperand);
        masm.freeStack(sizeof(Value));
    }

    
    
    

    
    regs = GeneralRegisterSet(Registers::VolatileMask & ~Registers::JSCCallMask);
    tmp = regs.takeAny();

    
    masm.mov(Operand(StackPointer, offsetof(IonCFrame, returnAddress)), tmp);

    
    masm.addPtr(Imm32(sizeof(IonCFrame) + f.explicitArgs * sizeof(void *)), StackPointer);

    
    masm.push(tmp);
    

    
    
    
    switch (f.returnType)
    {
      case VMFunction::ReturnBool:
      case VMFunction::ReturnPointer:
        masm.testPtr(ReturnReg, ReturnReg);
        break;
    }

    masm.ret();

    Linker linker(masm);
    IonCode *wrapper = linker.newCode(cx);
    if (!wrapper)
        return NULL;

    if(!functionWrappers_->add(p, &f, wrapper))
        return NULL;

    return wrapper;
}
