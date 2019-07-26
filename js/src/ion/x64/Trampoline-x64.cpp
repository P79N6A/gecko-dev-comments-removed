






#include "jscompartment.h"
#include "assembler/assembler/MacroAssembler.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"
#include "ion/IonFrames.h"
#include "ion/Bailouts.h"
#include "ion/VMFunctions.h"
#include "ion/IonSpewer.h"
#include "ion/ExecutionModeInlines.h"

#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;






IonCode *
IonRuntime::generateEnterJIT(JSContext *cx)
{
    MacroAssembler masm(cx);

    const Register reg_code  = IntArgReg0;
    const Register reg_argc  = IntArgReg1;
    const Register reg_argv  = IntArgReg2;
    JS_ASSERT(OsrFrameReg == IntArgReg3);

#if defined(_WIN64)
    const Operand token  = Operand(rbp, 16 + ShadowStackSpace);
    const Operand result = Operand(rbp, 24 + ShadowStackSpace);
#else
    const Register token  = IntArgReg4;
    const Register result = IntArgReg5;
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

    
    masm.subq(Imm32(16 * 10 + 8), rsp);

    masm.movdqa(xmm6, Operand(rsp, 16 * 0));
    masm.movdqa(xmm7, Operand(rsp, 16 * 1));
    masm.movdqa(xmm8, Operand(rsp, 16 * 2));
    masm.movdqa(xmm9, Operand(rsp, 16 * 3));
    masm.movdqa(xmm10, Operand(rsp, 16 * 4));
    masm.movdqa(xmm11, Operand(rsp, 16 * 5));
    masm.movdqa(xmm12, Operand(rsp, 16 * 6));
    masm.movdqa(xmm13, Operand(rsp, 16 * 7));
    masm.movdqa(xmm14, Operand(rsp, 16 * 8));
    masm.movdqa(xmm15, Operand(rsp, 16 * 9));
#endif

    
    masm.push(result);

    
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

    
    
    
    masm.movq(result, reg_argc);
    masm.unboxInt32(Operand(reg_argc, 0), reg_argc);
    masm.push(reg_argc);

    
    masm.push(token);

    


    masm.subq(rsp, r14);

    
    masm.makeFrameDescriptor(r14, IonFrame_Entry);
    masm.push(r14);

    
    masm.call(reg_code);

    
    masm.pop(r14);              
    masm.shrq(Imm32(FRAMESIZE_SHIFT), r14);
    masm.addq(r14, rsp);        

    


    masm.pop(r12); 
    masm.storeValue(JSReturnOperand, Operand(r12, 0));

    
#if defined(_WIN64)
    masm.movdqa(Operand(rsp, 16 * 0), xmm6);
    masm.movdqa(Operand(rsp, 16 * 1), xmm7);
    masm.movdqa(Operand(rsp, 16 * 2), xmm8);
    masm.movdqa(Operand(rsp, 16 * 3), xmm9);
    masm.movdqa(Operand(rsp, 16 * 4), xmm10);
    masm.movdqa(Operand(rsp, 16 * 5), xmm11);
    masm.movdqa(Operand(rsp, 16 * 6), xmm12);
    masm.movdqa(Operand(rsp, 16 * 7), xmm13);
    masm.movdqa(Operand(rsp, 16 * 8), xmm14);
    masm.movdqa(Operand(rsp, 16 * 9), xmm15);

    masm.addq(Imm32(16 * 10 + 8), rsp);

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
    return linker.newCode(cx, JSC::OTHER_CODE);
}

IonCode *
IonRuntime::generateInvalidator(JSContext *cx)
{
    AutoIonContextAlloc aica(cx);
    MacroAssembler masm(cx);

    

    masm.addq(Imm32(sizeof(uintptr_t)), rsp);

    
    masm.reserveStack(Registers::Total * sizeof(void *));
    for (uint32_t i = 0; i < Registers::Total; i++)
        masm.movq(Register::FromCode(i), Operand(rsp, i * sizeof(void *)));

    
    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32_t i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(rsp, i * sizeof(double)));

    masm.movq(rsp, rbx); 

    
    masm.reserveStack(sizeof(size_t));
    masm.movq(rsp, rcx);

    masm.setupUnalignedABICall(2, rdx);
    masm.passABIArg(rbx);
    masm.passABIArg(rcx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, InvalidationBailout));

    masm.pop(rbx); 

    
    masm.lea(Operand(rsp, rbx, TimesOne, sizeof(InvalidationBailoutStack)), rsp);

    masm.generateBailoutTail(rdx);

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

IonCode *
IonRuntime::generateArgumentsRectifier(JSContext *cx, ExecutionMode mode)
{
    

    MacroAssembler masm(cx);

    
    
    JS_ASSERT(ArgumentsRectifierReg == r8);

    
    masm.movq(Operand(rsp, IonRectifierFrameLayout::offsetOfCalleeToken()), rax);
    masm.movzwl(Operand(rax, offsetof(JSFunction, nargs)), rcx);
    masm.subq(r8, rcx);

    
    masm.movq(Operand(rsp, IonRectifierFrameLayout::offsetOfNumActualArgs()), rdx);

    masm.moveValue(UndefinedValue(), r10);

    masm.movq(rsp, r9); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(r10);
        masm.subl(Imm32(1), rcx);

        masm.testl(rcx, rcx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    BaseIndex b = BaseIndex(r9, r8, TimesEight, sizeof(IonRectifierFrameLayout));
    masm.lea(Operand(b), rcx);

    
    {
        Label copyLoopTop, initialSkip;

        masm.jump(&initialSkip);

        masm.bind(&copyLoopTop);
        masm.subq(Imm32(sizeof(Value)), rcx);
        masm.subl(Imm32(1), r8);
        masm.bind(&initialSkip);

        masm.push(Operand(rcx, 0x0));

        masm.testl(r8, r8);
        masm.j(Assembler::NonZero, &copyLoopTop);
    }

    
    masm.subq(rsp, r9);
    masm.makeFrameDescriptor(r9, IonFrame_Rectifier);

    
    masm.push(rdx); 
    masm.push(rax); 
    masm.push(r9); 

    
    
    masm.movq(Operand(rax, offsetof(JSFunction, u.i.script_)), rax);
    masm.movq(Operand(rax, OffsetOfIonInJSScript(mode)), rax);
    masm.movq(Operand(rax, IonScript::offsetOfMethod()), rax);
    masm.movq(Operand(rax, IonCode::offsetOfCode()), rax);
    masm.call(rax);

    
    masm.pop(r9);             
    masm.shrq(Imm32(FRAMESIZE_SHIFT), r9);
    masm.pop(r11);            
    masm.pop(r11);            
    masm.addq(r9, rsp);       

    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

static void
GenerateBailoutThunk(JSContext *cx, MacroAssembler &masm, uint32_t frameClass)
{
    
    masm.reserveStack(Registers::Total * sizeof(void *));
    for (uint32_t i = 0; i < Registers::Total; i++)
        masm.movq(Register::FromCode(i), Operand(rsp, i * sizeof(void *)));

    
    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32_t i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(rsp, i * sizeof(double)));

    
    masm.movq(rsp, r8);

    
    masm.setupUnalignedABICall(1, rax);
    masm.passABIArg(r8);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));

    
    
    
    
    
    
    
    static const uint32_t BailoutDataSize = sizeof(void *) * Registers::Total +
                                          sizeof(double) * FloatRegisters::Total;
    masm.addq(Imm32(BailoutDataSize), rsp);
    masm.pop(rcx);
    masm.lea(Operand(rsp, rcx, TimesOne, sizeof(void *)), rsp);

    masm.generateBailoutTail(rdx);
}

IonCode *
IonRuntime::generateBailoutTable(JSContext *cx, uint32_t frameClass)
{
    JS_NOT_REACHED("x64 does not use bailout tables");
    return NULL;
}

IonCode *
IonRuntime::generateBailoutHandler(JSContext *cx)
{
    MacroAssembler masm;

    GenerateBailoutThunk(cx, masm, NO_FRAME_SIZE_CLASS_ID);

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

IonCode *
IonRuntime::generateVMWrapper(JSContext *cx, const VMFunction &f)
{
    typedef MoveResolver::MoveOperand MoveOperand;

    JS_ASSERT(!StackKeptAligned);
    JS_ASSERT(functionWrappers_);
    JS_ASSERT(functionWrappers_->initialized());
    VMWrapperMap::AddPtr p = functionWrappers_->lookupForAdd(&f);
    if (p)
        return p->value;

    
    MacroAssembler masm;

    
    
    GeneralRegisterSet regs = GeneralRegisterSet(Register::Codes::WrapperMask);

    
    JS_STATIC_ASSERT((Register::Codes::VolatileMask & ~Register::Codes::WrapperMask) == 0);

    
    
    
    
    
    
    
    masm.enterExitFrame(&f);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = r10;
        regs.take(argsBase);
        masm.lea(Operand(rsp,IonExitFrameLayout::SizeWithFooter()), argsBase);
    }

    
    Register outReg = InvalidReg;
    switch (f.outParam) {
      case Type_Value:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(Value));
        masm.movq(esp, outReg);
        break;

      case Type_Handle:
        outReg = regs.takeAny();
        masm.Push(UndefinedValue());
        masm.movq(esp, outReg);
        break;

      case Type_Int32:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(int32_t));
        masm.movq(esp, outReg);
        break;

      default:
        JS_ASSERT(f.outParam == Type_Void);
        break;
    }

    Register temp = regs.getAny();
    masm.setupUnalignedABICall(f.argc(), temp);

    
    Register cxreg = IntArgReg0;
    masm.loadJSContext(cxreg);
    masm.passABIArg(cxreg);

    size_t argDisp = 0;

    
    if (f.explicitArgs) {
        for (uint32_t explicitArg = 0; explicitArg < f.explicitArgs; explicitArg++) {
            MoveOperand from;
            switch (f.argProperties(explicitArg)) {
              case VMFunction::WordByValue:
                masm.passABIArg(MoveOperand(argsBase, argDisp));
                argDisp += sizeof(void *);
                break;
              case VMFunction::WordByRef:
                masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE));
                argDisp += sizeof(void *);
                break;
              case VMFunction::DoubleByValue:
              case VMFunction::DoubleByRef:
                JS_NOT_REACHED("NYI: x64 callVM should not be used with 128bits values.");
                break;
            }
        }
    }

    
    if (outReg != InvalidReg)
        masm.passABIArg(outReg);

    masm.callWithABI(f.wrapped);

    
    Label exception;
    switch (f.failType()) {
      case Type_Object:
        masm.testq(rax, rax);
        masm.j(Assembler::Zero, &exception);
        break;
      case Type_Bool:
        masm.testb(rax, rax);
        masm.j(Assembler::Zero, &exception);
        break;
      default:
        JS_NOT_REACHED("unknown failure kind");
        break;
    }

    
    switch (f.outParam) {
      case Type_Handle:
      case Type_Value:
        masm.loadValue(Address(esp, 0), JSReturnOperand);
        masm.freeStack(sizeof(Value));
        break;

      case Type_Int32:
        masm.load32(Address(esp, 0), ReturnReg);
        masm.freeStack(sizeof(int32_t));
        break;

      default:
        JS_ASSERT(f.outParam == Type_Void);
        break;
    }
    masm.leaveExitFrame();
    masm.retn(Imm32(sizeof(IonExitFrameLayout) + f.explicitStackSlots() * sizeof(void *)));

    masm.bind(&exception);
    masm.handleException();

    Linker linker(masm);
    IonCode *wrapper = linker.newCode(cx, JSC::OTHER_CODE);
    if (!wrapper)
        return NULL;

    
    
    if (!functionWrappers_->relookupOrAdd(p, &f, wrapper))
        return NULL;

    return wrapper;
}

IonCode *
IonRuntime::generatePreBarrier(JSContext *cx, MIRType type)
{
    MacroAssembler masm;

    RegisterSet regs = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                                   FloatRegisterSet(FloatRegisters::VolatileMask));
    masm.PushRegsInMask(regs);

    JS_ASSERT(PreBarrierReg == rdx);
    masm.movq(ImmWord(cx->runtime), rcx);

    masm.setupUnalignedABICall(2, rax);
    masm.passABIArg(rcx);
    masm.passABIArg(rdx);
    if (type == MIRType_Value) {
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, MarkValueFromIon));
    } else {
        JS_ASSERT(type == MIRType_Shape);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, MarkShapeFromIon));
    }

    masm.PopRegsInMask(regs);
    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

