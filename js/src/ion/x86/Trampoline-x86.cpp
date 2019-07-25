








































#include "jscompartment.h"
#include "assembler/assembler/MacroAssembler.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"
#include "ion/IonFrames.h"
#include "ion/IonSpewer.h"
#include "ion/Bailouts.h"
#include "ion/VMFunctions.h"

using namespace js;
using namespace js::ion;





IonCode *
IonCompartment::generateOsrPrologue(JSContext *cx)
{
    MacroAssembler masm(cx);

    
    masm.movl(Operand(esp, 6 * sizeof(void *)), OsrFrameReg);

    
    
    JS_ASSERT(enterJIT_);
    masm.jmp(enterJIT_);

    Linker linker(masm);
    return linker.newCode(cx);
}







IonCode *
IonCompartment::generateEnterJIT(JSContext *cx)
{
    MacroAssembler masm(cx);
    

    
    masm.push(ebp);
    masm.movl(esp, ebp);

    
    
    
    masm.push(ebx);
    masm.push(esi);
    masm.push(edi);

    
    
    masm.movl(Operand(ebp, 12), eax);
    masm.shll(Imm32(3), eax);

    
    
    
    
    
    
    
    
    masm.movl(esp, ecx);
    masm.subl(eax, ecx);
    masm.subl(Imm32(12), ecx);

    
    masm.andl(Imm32(15), ecx);
    masm.subl(ecx, esp);

    



    
    masm.movl(Operand(ebp, 16), ebx);

    
    masm.addl(ebx, eax);

    
    {
        Label header, footer;
        masm.bind(&header);

        masm.cmpl(eax, ebx);
        masm.j(Assembler::BelowOrEqual, &footer);

        
        masm.subl(Imm32(8), eax);

        
        masm.push(Operand(eax, 4));
        masm.push(Operand(eax, 0));

        masm.jmp(&header);
        masm.bind(&footer);
    }

    
    masm.push(Operand(ebp, 24));

    
    
    masm.movl(Operand(ebp, 12), eax);
    masm.shll(Imm32(3), eax);
    masm.addl(eax, ecx);
    masm.addl(Imm32(4), ecx);

    
    masm.makeFrameDescriptor(ecx, IonFrame_Entry);
    masm.push(ecx);

    



    
    masm.call(Operand(ebp, 8));

    
    
    masm.pop(eax);
    masm.shrl(Imm32(FRAMETYPE_BITS), eax); 
    masm.addl(eax, esp);

    
    
    
    
    
    
    
    
    
    
    
    
    masm.movl(Operand(esp, 32), eax);
    masm.storeValue(JSReturnOperand, Operand(eax, 0));

    


    
    masm.pop(edi);
    masm.pop(esi);
    masm.pop(ebx);

    
    masm.pop(ebp);
    masm.ret();

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

    
    
    
    
    
    

    masm.cmpl(eax, Imm32(BAILOUT_RETURN_FATAL_ERROR));
    masm.j(Assembler::LessThan, &interpret);
    masm.j(Assembler::Equal, &exception);

    masm.cmpl(eax, Imm32(BAILOUT_RETURN_RECOMPILE_CHECK));
    masm.j(Assembler::LessThan, &reflow);

    
    masm.setupUnalignedABICall(0, edx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, RecompileForInlining));

    masm.testl(eax, eax);
    masm.j(Assembler::Zero, &exception);

    masm.jmp(&interpret);

    
    masm.bind(&reflow);
    masm.setupUnalignedABICall(1, edx);
    masm.setABIArg(0, eax);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ReflowTypeInfo));

    masm.testl(eax, eax);
    masm.j(Assembler::Zero, &exception);

    masm.bind(&interpret);
    
    masm.subl(Imm32(sizeof(Value)), esp);
    masm.movl(esp, ecx);

    
    masm.setupUnalignedABICall(1, edx);
    masm.setABIArg(0, ecx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ThunkToInterpreter));

    
    masm.popValue(JSReturnOperand);

    
    masm.testl(eax, eax);
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
        masm.movl(Register::FromCode(i), Operand(esp, i * sizeof(void *)));

    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32 i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(esp, i * sizeof(double)));

    masm.movl(esp, ebx); 

    
    masm.reserveStack(sizeof(size_t));
    masm.movl(esp, ecx);

    masm.setupUnalignedABICall(3, edx);
    masm.setABIArg(0, ebx);
    masm.setABIArg(1, ecx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, InvalidationBailout));

    masm.pop(ebx); 

    
    const uint32 BailoutDataSize = sizeof(double) * FloatRegisters::Total +
                                   sizeof(void *) * Registers::Total;
    masm.lea(Operand(esp, ebx, TimesOne, BailoutDataSize), esp);

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

    
    
    JS_ASSERT(ArgumentsRectifierReg == esi);

    
    masm.movl(Operand(esp, IonJSFrameLayout::offsetOfCalleeToken()), eax);
    masm.load16(Operand(eax, offsetof(JSFunction, nargs)), ecx);
    masm.subl(esi, ecx);

    masm.moveValue(UndefinedValue(), ebx, edi);

    masm.movl(esp, ebp); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(ebx); 
        masm.push(edi); 
        masm.subl(Imm32(1), ecx);

        masm.testl(ecx, ecx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    masm.movl(esi, edi);
    masm.shll(Imm32(3), edi); 

    masm.movl(ebp, ecx);
    masm.addl(Imm32(sizeof(IonRectifierFrameLayout)), ecx);
    masm.addl(edi, ecx);

    
    {
        Label copyLoopTop, initialSkip;

        masm.jump(&initialSkip);

        masm.bind(&copyLoopTop);
        masm.subl(Imm32(sizeof(Value)), ecx);
        masm.subl(Imm32(1), esi);
        masm.bind(&initialSkip);

        masm.mov(Operand(ecx, sizeof(Value)/2), edx);
        masm.push(edx);
        masm.mov(Operand(ecx, 0x0), edx);
        masm.push(edx);

        masm.testl(esi, esi);
        masm.j(Assembler::NonZero, &copyLoopTop);
    }

    
    masm.subl(esp, ebp);
    masm.makeFrameDescriptor(ebp, IonFrame_Rectifier);

    
    masm.push(eax); 
    masm.push(ebp); 

    
    
    masm.movl(Operand(eax, offsetof(JSFunction, u.i.script_)), eax);
    masm.movl(Operand(eax, offsetof(JSScript, ion)), eax);
    masm.movl(Operand(eax, offsetof(IonScript, method_)), eax);
    masm.movl(Operand(eax, IonCode::OffsetOfCode()), eax);
    masm.call(eax);

    
    masm.pop(ebp);            
    masm.shrl(Imm32(FRAMETYPE_BITS), ebp); 
    masm.pop(edi);            
    masm.addl(ebp, esp);      

    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx);
}

static void
GenerateBailoutThunk(JSContext *cx, MacroAssembler &masm, uint32 frameClass)
{
    
    masm.reserveStack(Registers::Total * sizeof(void *));
    for (uint32 i = 0; i < Registers::Total; i++)
        masm.movl(Register::FromCode(i), Operand(esp, i * sizeof(void *)));

    
    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32 i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(esp, i * sizeof(double)));

    
    masm.push(Imm32(frameClass));

    
    masm.movl(esp, eax);

    
    masm.setupUnalignedABICall(1, ecx);
    masm.setABIArg(0, eax);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));

    
    const uint32 BailoutDataSize = sizeof(void *) + 
                                   sizeof(double) * FloatRegisters::Total +
                                   sizeof(void *) * Registers::Total;

    
    if (frameClass == NO_FRAME_SIZE_CLASS_ID) {
        
        
        
        
        
        masm.addl(Imm32(BailoutDataSize), esp);
        masm.pop(ecx);
        masm.addl(Imm32(sizeof(uint32)), esp);
        masm.addl(ecx, esp);
    } else {
        
        
        
        
        uint32 frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        masm.addl(Imm32(BailoutDataSize + sizeof(void *) + frameSize), esp);
    }

    GenerateBailoutTail(masm);
}

IonCode *
IonCompartment::generateBailoutTable(JSContext *cx, uint32 frameClass)
{
    MacroAssembler masm;

    Label bailout;
    for (size_t i = 0; i < BAILOUT_TABLE_SIZE; i++)
        masm.call(&bailout);
    masm.bind(&bailout);

    GenerateBailoutThunk(cx, masm, frameClass);

    Linker linker(masm);
    return linker.newCode(cx);
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

    
    
    GeneralRegisterSet regs = GeneralRegisterSet::VolatileNot(GeneralRegisterSet());

    
    
    
    
    
    
    
    
    masm.linkExitFrame();

    
    masm.push(Operand(esp, 0));

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = regs.takeAny();
        masm.lea(Operand(esp, sizeof(IonExitFrameLayout) + sizeof(uintptr_t)), argsBase);
    }

    
    Register outReg = InvalidReg;
    if (f.outParam == Type_Value) {
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(Value));
        masm.movl(esp, outReg);
    }

    Register temp = regs.getAny();
    masm.setupUnalignedABICall(f.argc(), temp);

    
    Register cxreg = regs.takeAny();
    masm.movePtr(ImmWord(JS_THREAD_DATA(cx)), cxreg);
    masm.loadPtr(Address(cxreg, offsetof(ThreadData, ionJSContext)), cxreg);
    masm.setABIArg(0, cxreg);

    
    if (f.explicitArgs) {
        for (uint32 i = 0; i < f.explicitArgs; i++)
            masm.setABIArg(i + 1, MoveOperand(argsBase, i * sizeof(void *)));
    }

    
    if (outReg != InvalidReg)
        masm.setABIArg(f.argc() - 1, outReg);

    masm.callWithABI(f.wrapped);

    
    Label exception;
    masm.testl(eax, eax);
    masm.j(Assembler::Zero, &exception);

    
    if (f.outParam == Type_Value) {
        masm.loadValue(Address(esp, 0), JSReturnOperand);
        masm.freeStack(sizeof(Value));
    }

    
    
    Register scratch = (f.outParam == Type_Value) ? ReturnReg : JSReturnReg_Type;
    Label invalidated;
    masm.pop(scratch);
    masm.cmpl(scratch, Operand(esp, 0));
    masm.j(Assembler::NotEqual, &invalidated);

    masm.retn(Imm32(sizeof(IonExitFrameLayout) + f.explicitArgs * sizeof(void *)));

    masm.bind(&exception);
    masm.handleException();

    masm.bind(&invalidated);
    masm.cmpl(Operand(esp, 0), Imm32(0));
    masm.j(Assembler::Equal, &exception);
    masm.ret();

    Linker linker(masm);
    IonCode *wrapper = linker.newCode(cx);
    if (!wrapper || !functionWrappers_->add(p, &f, wrapper))
        return NULL;

    return wrapper;
}
