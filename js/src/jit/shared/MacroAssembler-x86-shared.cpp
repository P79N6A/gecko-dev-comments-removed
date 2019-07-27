





#include "jit/shared/MacroAssembler-x86-shared.h"

#include "jit/IonFrames.h"
#include "jit/IonMacroAssembler.h"

using namespace js;
using namespace js::jit;

void
MacroAssembler::PushRegsInMask(RegisterSet set, FloatRegisterSet simdSet)
{
    FloatRegisterSet doubleSet(FloatRegisterSet::Subtract(set.fpus(), simdSet));
    JS_ASSERT_IF(simdSet.empty(), doubleSet == set.fpus());
    unsigned numSimd = simdSet.size();
    unsigned numDouble = doubleSet.size();
    int32_t diffF = numDouble * sizeof(double) + numSimd * Simd128DataSize;
    int32_t diffG = set.gprs().size() * sizeof(intptr_t);

    
    
    for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++) {
        diffG -= sizeof(intptr_t);
        Push(*iter);
    }
    JS_ASSERT(diffG == 0);

    reserveStack(diffF);
    for (FloatRegisterBackwardIterator iter(doubleSet); iter.more(); iter++) {
        diffF -= sizeof(double);
        numDouble -= 1;
        storeDouble(*iter, Address(StackPointer, diffF));
    }
    JS_ASSERT(numDouble == 0);
    for (FloatRegisterBackwardIterator iter(simdSet); iter.more(); iter++) {
        diffF -= Simd128DataSize;
        numSimd -= 1;
        
        storeUnalignedInt32x4(*iter, Address(StackPointer, diffF));
    }
    JS_ASSERT(numSimd == 0);
    JS_ASSERT(diffF == 0);
}

void
MacroAssembler::PopRegsInMaskIgnore(RegisterSet set, RegisterSet ignore, FloatRegisterSet simdSet)
{
    FloatRegisterSet doubleSet(FloatRegisterSet::Subtract(set.fpus(), simdSet));
    JS_ASSERT_IF(simdSet.empty(), doubleSet == set.fpus());
    unsigned numSimd = simdSet.size();
    unsigned numDouble = doubleSet.size();
    int32_t diffG = set.gprs().size() * sizeof(intptr_t);
    int32_t diffF = numDouble * sizeof(double) + numSimd * Simd128DataSize;
    const int32_t reservedG = diffG;
    const int32_t reservedF = diffF;

    for (FloatRegisterBackwardIterator iter(simdSet); iter.more(); iter++) {
        diffF -= Simd128DataSize;
        numSimd -= 1;
        if (!ignore.has(*iter))
            
            loadUnalignedInt32x4(Address(StackPointer, diffF), *iter);
    }
    JS_ASSERT(numSimd == 0);
    for (FloatRegisterBackwardIterator iter(doubleSet); iter.more(); iter++) {
        diffF -= sizeof(double);
        numDouble -= 1;
        if (!ignore.has(*iter))
            loadDouble(Address(StackPointer, diffF), *iter);
    }
    freeStack(reservedF);
    JS_ASSERT(numDouble == 0);
    JS_ASSERT(diffF == 0);

    
    
    
    if (ignore.empty(false)) {
        for (GeneralRegisterForwardIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= sizeof(intptr_t);
            Pop(*iter);
        }
    } else {
        for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++) {
            diffG -= sizeof(intptr_t);
            if (!ignore.has(*iter))
                loadPtr(Address(StackPointer, diffG), *iter);
        }
        freeStack(reservedG);
    }
    JS_ASSERT(diffG == 0);
}


void
MacroAssembler::clampDoubleToUint8(FloatRegister input, Register output)
{
    JS_ASSERT(input != ScratchDoubleReg);
    Label positive, done;

    
    zeroDouble(ScratchDoubleReg);
    branchDouble(DoubleGreaterThan, input, ScratchDoubleReg, &positive);
    {
        move32(Imm32(0), output);
        jump(&done);
    }

    bind(&positive);

    
    loadConstantDouble(0.5, ScratchDoubleReg);
    addDouble(ScratchDoubleReg, input);

    Label outOfRange;

    
    
    
    cvttsd2si(input, output);
    branch32(Assembler::Above, output, Imm32(255), &outOfRange);
    {
        
        convertInt32ToDouble(output, ScratchDoubleReg);
        branchDouble(DoubleNotEqual, input, ScratchDoubleReg, &done);

        
        
        and32(Imm32(~1), output);
        jump(&done);
    }

    
    bind(&outOfRange);
    {
        move32(Imm32(255), output);
    }

    bind(&done);
}



bool
MacroAssemblerX86Shared::buildFakeExitFrame(Register scratch, uint32_t *offset)
{
    mozilla::DebugOnly<uint32_t> initialDepth = framePushed();

    CodeLabel cl;
    mov(cl.dest(), scratch);

    uint32_t descriptor = MakeFrameDescriptor(framePushed(), JitFrame_IonJS);
    Push(Imm32(descriptor));
    Push(scratch);

    bind(cl.src());
    *offset = currentOffset();

    JS_ASSERT(framePushed() == initialDepth + IonExitFrameLayout::Size());
    return addCodeLabel(cl);
}

void
MacroAssemblerX86Shared::callWithExitFrame(JitCode *target)
{
    uint32_t descriptor = MakeFrameDescriptor(framePushed(), JitFrame_IonJS);
    Push(Imm32(descriptor));
    call(target);
}

void
MacroAssembler::alignFrameForICArguments(AfterICSaveLive &aic)
{
    
}

void
MacroAssembler::restoreFrameAlignmentForICArguments(AfterICSaveLive &aic)
{
    
}

bool
MacroAssemblerX86Shared::buildOOLFakeExitFrame(void *fakeReturnAddr)
{
    uint32_t descriptor = MakeFrameDescriptor(framePushed(), JitFrame_IonJS);
    Push(Imm32(descriptor));
    Push(ImmPtr(fakeReturnAddr));
    return true;
}

void
MacroAssemblerX86Shared::branchNegativeZero(FloatRegister reg,
                                            Register scratch,
                                            Label *label)
{
    
    

#if defined(JS_CODEGEN_X86)
    Label nonZero;

    
    xorpd(ScratchDoubleReg, ScratchDoubleReg);

    
    branchDouble(DoubleNotEqual, reg, ScratchDoubleReg, &nonZero);

    
    movmskpd(reg, scratch);

    
    
    branchTest32(NonZero, scratch, Imm32(1), label);

    bind(&nonZero);
#elif defined(JS_CODEGEN_X64)
    movq(reg, scratch);
    cmpq(scratch, Imm32(1));
    j(Overflow, label);
#endif
}

void
MacroAssemblerX86Shared::branchNegativeZeroFloat32(FloatRegister reg,
                                                   Register scratch,
                                                   Label *label)
{
    movd(reg, scratch);
    cmpl(scratch, Imm32(1));
    j(Overflow, label);
}
