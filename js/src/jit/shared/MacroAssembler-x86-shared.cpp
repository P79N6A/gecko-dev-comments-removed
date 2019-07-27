





#include "jit/shared/MacroAssembler-x86-shared.h"

#include "jit/JitFrames.h"
#include "jit/MacroAssembler.h"

using namespace js;
using namespace js::jit;

void
MacroAssembler::PushRegsInMask(RegisterSet set, FloatRegisterSet simdSet)
{
    FloatRegisterSet doubleSet(FloatRegisterSet::Subtract(set.fpus(), simdSet));
    MOZ_ASSERT_IF(simdSet.empty(), doubleSet == set.fpus());
    doubleSet = doubleSet.reduceSetForPush();
    unsigned numSimd = simdSet.size();
    unsigned numDouble = doubleSet.size();
    int32_t diffF = doubleSet.getPushSizeInBytes() + numSimd * Simd128DataSize;
    int32_t diffG = set.gprs().size() * sizeof(intptr_t);

    
    
    for (GeneralRegisterBackwardIterator iter(set.gprs()); iter.more(); iter++) {
        diffG -= sizeof(intptr_t);
        Push(*iter);
    }
    MOZ_ASSERT(diffG == 0);

    reserveStack(diffF);
    for (FloatRegisterBackwardIterator iter(doubleSet); iter.more(); iter++) {
        FloatRegister reg = *iter;
        diffF -= reg.size();
        numDouble -= 1;
        Address spillAddress(StackPointer, diffF);
        if (reg.isDouble())
            storeDouble(reg, spillAddress);
        else if (reg.isSingle())
            storeFloat32(reg, spillAddress);
        else if (reg.isInt32x4())
            storeUnalignedInt32x4(reg, spillAddress);
        else if (reg.isFloat32x4())
            storeUnalignedFloat32x4(reg, spillAddress);
        else
            MOZ_CRASH("Unknown register type.");
    }
    MOZ_ASSERT(numDouble == 0);
    for (FloatRegisterBackwardIterator iter(simdSet); iter.more(); iter++) {
        diffF -= Simd128DataSize;
        numSimd -= 1;
        
        storeUnalignedInt32x4(*iter, Address(StackPointer, diffF));
    }
    MOZ_ASSERT(numSimd == 0);
    MOZ_ASSERT(diffF == 0);
}

void
MacroAssembler::PopRegsInMaskIgnore(RegisterSet set, RegisterSet ignore, FloatRegisterSet simdSet)
{
    FloatRegisterSet doubleSet(FloatRegisterSet::Subtract(set.fpus(), simdSet));
    MOZ_ASSERT_IF(simdSet.empty(), doubleSet == set.fpus());
    doubleSet = doubleSet.reduceSetForPush();
    unsigned numSimd = simdSet.size();
    unsigned numDouble = doubleSet.size();
    int32_t diffG = set.gprs().size() * sizeof(intptr_t);
    int32_t diffF = doubleSet.getPushSizeInBytes() + numSimd * Simd128DataSize;
    const int32_t reservedG = diffG;
    const int32_t reservedF = diffF;

    for (FloatRegisterBackwardIterator iter(simdSet); iter.more(); iter++) {
        diffF -= Simd128DataSize;
        numSimd -= 1;
        if (!ignore.has(*iter))
            
            loadUnalignedInt32x4(Address(StackPointer, diffF), *iter);
    }
    MOZ_ASSERT(numSimd == 0);
    for (FloatRegisterBackwardIterator iter(doubleSet); iter.more(); iter++) {
        FloatRegister reg = *iter;
        diffF -= reg.size();
        numDouble -= 1;
        if (ignore.has(reg))
            continue;

        Address spillAddress(StackPointer, diffF);
        if (reg.isDouble())
            loadDouble(spillAddress, reg);
        else if (reg.isSingle())
            loadFloat32(spillAddress, reg);
        else if (reg.isInt32x4())
            loadUnalignedInt32x4(spillAddress, reg);
        else if (reg.isFloat32x4())
            loadUnalignedFloat32x4(spillAddress, reg);
        else
            MOZ_CRASH("Unknown register type.");
    }
    freeStack(reservedF);
    MOZ_ASSERT(numDouble == 0);
    MOZ_ASSERT(diffF == 0);

    
    
    
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
    MOZ_ASSERT(diffG == 0);
}


void
MacroAssembler::clampDoubleToUint8(FloatRegister input, Register output)
{
    MOZ_ASSERT(input != ScratchDoubleReg);
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

    
    
    
    vcvttsd2si(input, output);
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



void
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

    MOZ_ASSERT(framePushed() == initialDepth + ExitFrameLayout::Size());
    addCodeLabel(cl);
}

void
MacroAssemblerX86Shared::callWithExitFrame(Label *target)
{
    uint32_t descriptor = MakeFrameDescriptor(framePushed(), JitFrame_IonJS);
    Push(Imm32(descriptor));
    call(target);
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
                                            Label *label,
                                            bool maybeNonZero)
{
    
    

#if defined(JS_CODEGEN_X86)
    Label nonZero;

    
    if (maybeNonZero) {
        
        zeroDouble(ScratchDoubleReg);

        
        branchDouble(DoubleNotEqual, reg, ScratchDoubleReg, &nonZero);
    }
    
    vmovmskpd(reg, scratch);

    
    
    branchTest32(NonZero, scratch, Imm32(1), label);

    bind(&nonZero);
#elif defined(JS_CODEGEN_X64)
    vmovq(reg, scratch);
    cmpq(Imm32(1), scratch);
    j(Overflow, label);
#endif
}

void
MacroAssemblerX86Shared::branchNegativeZeroFloat32(FloatRegister reg,
                                                   Register scratch,
                                                   Label *label)
{
    vmovd(reg, scratch);
    cmp32(scratch, Imm32(1));
    j(Overflow, label);
}
