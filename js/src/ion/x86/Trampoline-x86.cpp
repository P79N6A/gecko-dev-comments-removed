





#include "jscompartment.h"
#include "assembler/assembler/MacroAssembler.h"
#include "ion/BaselineJIT.h"
#include "ion/IonCompartment.h"
#include "ion/IonLinker.h"
#include "ion/IonFrames.h"
#include "ion/IonSpewer.h"
#include "ion/Bailouts.h"
#include "ion/VMFunctions.h"
#include "ion/x86/BaselineHelpers-x86.h"
#include "ion/ExecutionModeInlines.h"

#include "jsscriptinlines.h"

using namespace js;
using namespace js::ion;

enum EnterJitEbpArgumentOffset {
    ARG_JITCODE         = 2 * sizeof(void *),
    ARG_ARGC            = 3 * sizeof(void *),
    ARG_ARGV            = 4 * sizeof(void *),
    ARG_STACKFRAME      = 5 * sizeof(void *),
    ARG_CALLEETOKEN     = 6 * sizeof(void *),
    ARG_SCOPECHAIN      = 7 * sizeof(void *),
    ARG_STACKVALUES     = 8 * sizeof(void *),
    ARG_RESULT          = 9 * sizeof(void *)
};





IonCode *
IonRuntime::generateEnterJIT(JSContext *cx, EnterJitType type)
{
    MacroAssembler masm(cx);

    
    masm.push(ebp);
    masm.movl(esp, ebp);

    
    
    
    masm.push(ebx);
    masm.push(esi);
    masm.push(edi);
    masm.movl(esp, esi);

    
    masm.movl(Operand(ebp, ARG_ARGC), eax);
    masm.shll(Imm32(3), eax);

    
    
    
    
    
    
    
    
    masm.movl(esp, ecx);
    masm.subl(eax, ecx);
    masm.subl(Imm32(12), ecx);

    
    masm.andl(Imm32(15), ecx);
    masm.subl(ecx, esp);

    



    
    masm.movl(Operand(ebp, ARG_ARGV), ebx);

    
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


    
    
    
    masm.mov(Operand(ebp, ARG_RESULT), eax);
    masm.unboxInt32(Address(eax, 0x0), eax);
    masm.push(eax);

    
    masm.push(Operand(ebp, ARG_CALLEETOKEN));

    
    
    masm.movl(Operand(ebp, ARG_STACKFRAME), OsrFrameReg);

    


    
    masm.subl(esp, esi);
    masm.makeFrameDescriptor(esi, IonFrame_Entry);
    masm.push(esi);

    CodeLabel returnLabel;
    if (type == EnterJitBaseline) {
        
        GeneralRegisterSet regs(GeneralRegisterSet::All());
        regs.take(JSReturnOperand);
        regs.takeUnchecked(OsrFrameReg);
        regs.take(ebp);
        regs.take(ReturnReg);

        Register scratch = regs.takeAny();

        Label notOsr;
        masm.branchTestPtr(Assembler::Zero, OsrFrameReg, OsrFrameReg, &notOsr);

        Register numStackValues = regs.takeAny();
        masm.movl(Operand(ebp, ARG_STACKVALUES), numStackValues);

        Register jitcode = regs.takeAny();
        masm.movl(Operand(ebp, ARG_JITCODE), jitcode);

        
        masm.mov(returnLabel.dest(), scratch);
        masm.push(scratch);
        masm.push(ebp);

        
        Register framePtr = ebp;
        masm.subPtr(Imm32(BaselineFrame::Size()), esp);
        masm.mov(esp, framePtr);

        
        masm.mov(numStackValues, scratch);
        masm.shll(Imm32(3), scratch);
        masm.subPtr(scratch, esp);

        
        masm.addPtr(Imm32(BaselineFrame::Size() + BaselineFrame::FramePointerOffset), scratch);
        masm.makeFrameDescriptor(scratch, IonFrame_BaselineJS);
        masm.push(scratch);
        masm.push(Imm32(0)); 
        masm.enterFakeExitFrame();

        masm.push(framePtr);
        masm.push(jitcode);

        masm.setupUnalignedABICall(3, scratch);
        masm.passABIArg(framePtr); 
        masm.passABIArg(OsrFrameReg); 
        masm.passABIArg(numStackValues);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ion::InitBaselineFrameForOsr));

        masm.pop(jitcode);
        masm.pop(framePtr);

        JS_ASSERT(jitcode != ReturnReg);

        Label error;
        masm.addPtr(Imm32(IonExitFrameLayout::SizeWithFooter()), esp);
        masm.addPtr(Imm32(BaselineFrame::Size()), framePtr);
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &error);

        masm.jump(jitcode);

        
        
        masm.bind(&error);
        masm.mov(framePtr, esp);
        masm.addPtr(Imm32(2 * sizeof(uintptr_t)), esp);
        masm.moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
        masm.mov(returnLabel.dest(), scratch);
        masm.jump(scratch);

        masm.bind(&notOsr);
        masm.movl(Operand(ebp, ARG_SCOPECHAIN), R1.scratchReg());
    }

    



    masm.call(Operand(ebp, ARG_JITCODE));

    if (type == EnterJitBaseline) {
        
        masm.bind(returnLabel.src());
        if (!masm.addCodeLabel(returnLabel))
            return NULL;
    }

    
    
    masm.pop(eax);
    masm.shrl(Imm32(FRAMESIZE_SHIFT), eax); 
    masm.addl(eax, esp);

    
    
    
    
    
    
    
    
    masm.movl(Operand(esp, ARG_RESULT + 3 * sizeof(void *)), eax);
    masm.storeValue(JSReturnOperand, Operand(eax, 0));

    


    
    masm.pop(edi);
    masm.pop(esi);
    masm.pop(ebx);

    
    masm.pop(ebp);
    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

IonCode *
IonRuntime::generateInvalidator(JSContext *cx)
{
    AutoIonContextAlloc aica(cx);
    MacroAssembler masm(cx);

    
    
    
    
    
    
    
    
    

    masm.addl(Imm32(sizeof(uintptr_t)), esp);

    masm.reserveStack(Registers::Total * sizeof(void *));
    for (uint32_t i = 0; i < Registers::Total; i++)
        masm.movl(Register::FromCode(i), Operand(esp, i * sizeof(void *)));

    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32_t i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(esp, i * sizeof(double)));

    masm.movl(esp, eax); 

    
    masm.reserveStack(sizeof(size_t));
    masm.movl(esp, ebx);

    
    masm.reserveStack(sizeof(void *));
    masm.movl(esp, ecx);

    masm.setupUnalignedABICall(3, edx);
    masm.passABIArg(eax);
    masm.passABIArg(ebx);
    masm.passABIArg(ecx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, InvalidationBailout));

    masm.pop(ecx); 
    masm.pop(ebx); 

    
    masm.lea(Operand(esp, ebx, TimesOne, sizeof(InvalidationBailoutStack)), esp);

    masm.generateBailoutTail(edx, ecx);

    Linker linker(masm);
    IonCode *code = linker.newCode(cx, JSC::OTHER_CODE);
    IonSpew(IonSpew_Invalidate, "   invalidation thunk created at %p", (void *) code->raw());
    return code;
}

IonCode *
IonRuntime::generateArgumentsRectifier(JSContext *cx, ExecutionMode mode, void **returnAddrOut)
{
    MacroAssembler masm(cx);

    
    
    JS_ASSERT(ArgumentsRectifierReg == esi);

    
    masm.movl(Operand(esp, IonRectifierFrameLayout::offsetOfCalleeToken()), eax);
    masm.clearCalleeTag(eax, mode);
    masm.movzwl(Operand(eax, offsetof(JSFunction, nargs)), ecx);
    masm.subl(esi, ecx);

    
    masm.movl(Operand(esp, IonRectifierFrameLayout::offsetOfNumActualArgs()), edx);

    masm.moveValue(UndefinedValue(), ebx, edi);

    
    
    
    
    masm.push(FramePointer);
    masm.movl(esp, FramePointer); 

    
    {
        Label undefLoopTop;
        masm.bind(&undefLoopTop);

        masm.push(ebx); 
        masm.push(edi); 
        masm.subl(Imm32(1), ecx);

        masm.testl(ecx, ecx);
        masm.j(Assembler::NonZero, &undefLoopTop);
    }

    
    
    BaseIndex b = BaseIndex(FramePointer, esi, TimesEight,
                            sizeof(IonRectifierFrameLayout) + sizeof(void*));
    masm.lea(Operand(b), ecx);

    
    {
        Label copyLoopTop, initialSkip;

        masm.jump(&initialSkip);

        masm.bind(&copyLoopTop);
        masm.subl(Imm32(sizeof(Value)), ecx);
        masm.subl(Imm32(1), esi);
        masm.bind(&initialSkip);

        masm.push(Operand(ecx, sizeof(Value)/2));
        masm.push(Operand(ecx, 0x0));

        masm.testl(esi, esi);
        masm.j(Assembler::NonZero, &copyLoopTop);
    }

    
    masm.lea(Operand(FramePointer, sizeof(void*)), ebx);
    masm.subl(esp, ebx);
    masm.makeFrameDescriptor(ebx, IonFrame_Rectifier);

    
    masm.push(edx); 
    masm.push(eax); 
    masm.push(ebx); 

    
    
    masm.movl(Operand(eax, JSFunction::offsetOfNativeOrScript()), eax);
    masm.loadBaselineOrIonRaw(eax, eax, mode, NULL);
    masm.call(eax);
    uint32_t returnOffset = masm.currentOffset();

    
    masm.pop(ebx);            
    masm.shrl(Imm32(FRAMESIZE_SHIFT), ebx); 
    masm.pop(edi);            
    masm.pop(edi);            

    
    BaseIndex unwind = BaseIndex(esp, ebx, TimesOne, -int32_t(sizeof(void*)));
    masm.lea(Operand(unwind), esp);

    masm.pop(FramePointer);
    masm.ret();

    Linker linker(masm);
    IonCode *code = linker.newCode(cx, JSC::OTHER_CODE);

    CodeOffsetLabel returnLabel(returnOffset);
    returnLabel.fixup(&masm);
    if (returnAddrOut)
        *returnAddrOut = (void *) (code->raw() + returnLabel.offset());
    return code;
}

static void
GenerateBailoutThunk(JSContext *cx, MacroAssembler &masm, uint32_t frameClass)
{
    
    masm.reserveStack(Registers::Total * sizeof(void *));
    for (uint32_t i = 0; i < Registers::Total; i++)
        masm.movl(Register::FromCode(i), Operand(esp, i * sizeof(void *)));

    
    masm.reserveStack(FloatRegisters::Total * sizeof(double));
    for (uint32_t i = 0; i < FloatRegisters::Total; i++)
        masm.movsd(FloatRegister::FromCode(i), Operand(esp, i * sizeof(double)));

    
    masm.push(Imm32(frameClass));

    
    masm.movl(esp, eax);

    
    masm.reserveStack(sizeof(void *));
    masm.movl(esp, ebx);

    
    masm.setupUnalignedABICall(2, ecx);
    masm.passABIArg(eax);
    masm.passABIArg(ebx);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, Bailout));

    masm.pop(ebx); 

    
    const uint32_t BailoutDataSize = sizeof(void *) + 
                                   sizeof(double) * FloatRegisters::Total +
                                   sizeof(void *) * Registers::Total;

    
    if (frameClass == NO_FRAME_SIZE_CLASS_ID) {
        
        
        
        
        
        masm.addl(Imm32(BailoutDataSize), esp);
        masm.pop(ecx);
        masm.addl(Imm32(sizeof(uint32_t)), esp);
        masm.addl(ecx, esp);
    } else {
        
        
        
        
        uint32_t frameSize = FrameSizeClass::FromClass(frameClass).frameSize();
        masm.addl(Imm32(BailoutDataSize + sizeof(void *) + frameSize), esp);
    }

    masm.generateBailoutTail(edx, ebx);
}

IonCode *
IonRuntime::generateBailoutTable(JSContext *cx, uint32_t frameClass)
{
    MacroAssembler masm;

    Label bailout;
    for (size_t i = 0; i < BAILOUT_TABLE_SIZE; i++)
        masm.call(&bailout);
    masm.bind(&bailout);

    GenerateBailoutThunk(cx, masm, frameClass);

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
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

    
    Register cxreg = regs.takeAny();

    
    
    
    
    
    
    
    masm.enterExitFrameAndLoadContext(&f, cxreg, regs.getAny(), f.executionMode);

    
    Register argsBase = InvalidReg;
    if (f.explicitArgs) {
        argsBase = regs.takeAny();
        masm.lea(Operand(esp, IonExitFrameLayout::SizeWithFooter()), argsBase);
    }

    
    Register outReg = InvalidReg;
    switch (f.outParam) {
      case Type_Value:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(Value));
        masm.movl(esp, outReg);
        break;

      case Type_Handle:
        outReg = regs.takeAny();
        masm.PushEmptyRooted(f.outParamRootType);
        masm.movl(esp, outReg);
        break;

      case Type_Int32:
      case Type_Pointer:
        outReg = regs.takeAny();
        masm.reserveStack(sizeof(int32_t));
        masm.movl(esp, outReg);
        break;

      default:
        JS_ASSERT(f.outParam == Type_Void);
        break;
    }

    masm.setupUnalignedABICall(f.argc(), regs.getAny());
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
              case VMFunction::DoubleByValue:
                
                
                masm.passABIArg(MoveOperand(argsBase, argDisp));
                argDisp += sizeof(void *);
                masm.passABIArg(MoveOperand(argsBase, argDisp));
                argDisp += sizeof(void *);
                break;
              case VMFunction::WordByRef:
                masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE));
                argDisp += sizeof(void *);
                break;
              case VMFunction::DoubleByRef:
                masm.passABIArg(MoveOperand(argsBase, argDisp, MoveOperand::EFFECTIVE));
                argDisp += 2 * sizeof(void *);
                break;
            }
        }
    }

    
    if (outReg != InvalidReg)
        masm.passABIArg(outReg);

    masm.callWithABI(f.wrapped);

    
    Label failure;
    switch (f.failType()) {
      case Type_Object:
        masm.branchTestPtr(Assembler::Zero, eax, eax, &failure);
        break;
      case Type_Bool:
        masm.testb(eax, eax);
        masm.j(Assembler::Zero, &failure);
        break;
      case Type_ParallelResult:
        masm.branchPtr(Assembler::NotEqual, eax, Imm32(TP_SUCCESS), &failure);
        break;
      default:
        JS_NOT_REACHED("unknown failure kind");
        break;
    }

    
    switch (f.outParam) {
      case Type_Handle:
        masm.popRooted(f.outParamRootType, ReturnReg, JSReturnOperand);
        break;

      case Type_Value:
        masm.loadValue(Address(esp, 0), JSReturnOperand);
        masm.freeStack(sizeof(Value));
        break;

      case Type_Int32:
      case Type_Pointer:
        masm.load32(Address(esp, 0), ReturnReg);
        masm.freeStack(sizeof(int32_t));
        break;

      default:
        JS_ASSERT(f.outParam == Type_Void);
        break;
    }
    masm.leaveExitFrame();
    masm.retn(Imm32(sizeof(IonExitFrameLayout) +
                    f.explicitStackSlots() * sizeof(void *) +
                    f.extraValuesToPop * sizeof(Value)));

    masm.bind(&failure);
    masm.handleFailure(f.executionMode);

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

    RegisterSet save;
    if (cx->runtime()->jitSupportsFloatingPoint) {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet(FloatRegisters::VolatileMask));
    } else {
        save = RegisterSet(GeneralRegisterSet(Registers::VolatileMask),
                           FloatRegisterSet());
    }
    masm.PushRegsInMask(save);

    JS_ASSERT(PreBarrierReg == edx);
    masm.movl(ImmWord(cx->runtime()), ecx);

    masm.setupUnalignedABICall(2, eax);
    masm.passABIArg(ecx);
    masm.passABIArg(edx);

    if (type == MIRType_Value) {
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, MarkValueFromIon));
    } else {
        JS_ASSERT(type == MIRType_Shape);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, MarkShapeFromIon));
    }

    masm.PopRegsInMask(save);
    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}

typedef bool (*HandleDebugTrapFn)(JSContext *, BaselineFrame *, uint8_t *, JSBool *);
static const VMFunction HandleDebugTrapInfo = FunctionInfo<HandleDebugTrapFn>(HandleDebugTrap);

IonCode *
IonRuntime::generateDebugTrapHandler(JSContext *cx)
{
    MacroAssembler masm;

    Register scratch1 = eax;
    Register scratch2 = ecx;
    Register scratch3 = edx;

    
    masm.loadPtr(Address(esp, 0), scratch1);

    
    masm.mov(ebp, scratch2);
    masm.subPtr(Imm32(BaselineFrame::Size()), scratch2);

    
    
    
    masm.movePtr(ImmWord((void *)NULL), BaselineStubReg);
    EmitEnterStubFrame(masm, scratch3);

    IonCompartment *ion = cx->compartment()->ionCompartment();
    IonCode *code = ion->getVMWrapper(HandleDebugTrapInfo);
    if (!code)
        return NULL;

    masm.push(scratch1);
    masm.push(scratch2);
    EmitCallVM(code, masm);

    EmitLeaveStubFrame(masm);

    
    
    
    Label forcedReturn;
    masm.branchTest32(Assembler::NonZero, ReturnReg, ReturnReg, &forcedReturn);
    masm.ret();

    masm.bind(&forcedReturn);
    masm.loadValue(Address(ebp, BaselineFrame::reverseOffsetOfReturnValue()),
                   JSReturnOperand);
    masm.mov(ebp, esp);
    masm.pop(ebp);
    masm.ret();

    Linker linker(masm);
    return linker.newCode(cx, JSC::OTHER_CODE);
}
