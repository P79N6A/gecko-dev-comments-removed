





#include "jscompartment.h"

#include "jit/Bailouts.h"
#include "jit/BaselineIC.h"
#include "jit/IonCaches.h"

using namespace js;
using namespace js::jit;





JitCode *JitRuntime::generateEnterJIT(JSContext *, EnterJitType) { MOZ_CRASH(); }
JitCode *JitRuntime::generateInvalidator(JSContext *) { MOZ_CRASH(); }
JitCode *JitRuntime::generateArgumentsRectifier(JSContext *, ExecutionMode, void **) { MOZ_CRASH(); }
JitCode *JitRuntime::generateBailoutTable(JSContext *, uint32_t) { MOZ_CRASH(); }
JitCode *JitRuntime::generateBailoutHandler(JSContext *, ExecutionMode) { MOZ_CRASH(); }
JitCode *JitRuntime::generateVMWrapper(JSContext *, const VMFunction &) { MOZ_CRASH(); }
JitCode *JitRuntime::generatePreBarrier(JSContext *, MIRType) { MOZ_CRASH(); }
JitCode *JitRuntime::generateDebugTrapHandler(JSContext *) { MOZ_CRASH(); }
JitCode *JitRuntime::generateExceptionTailStub(JSContext *) { MOZ_CRASH(); }
JitCode *JitRuntime::generateBailoutTailStub(JSContext *) { MOZ_CRASH(); }
JitCode *JitRuntime::generateForkJoinGetSliceStub(JSContext *) { MOZ_CRASH(); }

FrameSizeClass FrameSizeClass::FromDepth(uint32_t) { MOZ_CRASH(); }
FrameSizeClass FrameSizeClass::ClassLimit() { MOZ_CRASH(); }
uint32_t FrameSizeClass::frameSize() const { MOZ_CRASH(); }

void DispatchIonCache::initializeAddCacheState(LInstruction *, AddCacheState *) { MOZ_CRASH(); }

void MacroAssembler::PushRegsInMask(RegisterSet) { MOZ_CRASH(); }
void MacroAssembler::clampDoubleToUint8(FloatRegister, Register) { MOZ_CRASH(); }
void MacroAssembler::PopRegsInMaskIgnore(RegisterSet, RegisterSet) { MOZ_CRASH(); }
void MacroAssembler::alignFrameForICArguments(AfterICSaveLive &) { MOZ_CRASH(); }
void MacroAssembler::restoreFrameAlignmentForICArguments(AfterICSaveLive &) { MOZ_CRASH(); }

const Register ABIArgGenerator::NonArgReturnReg0 = { 0 };
const Register ABIArgGenerator::NonArgReturnReg1 = { 0 };
const Register ABIArgGenerator::NonArg_VolatileReg = { 0 };
const Register ABIArgGenerator::NonReturn_VolatileReg0 = { 0 };
const Register ABIArgGenerator::NonReturn_VolatileReg1 = { 0 };

IonBailoutIterator::IonBailoutIterator(const JitActivationIterator &iter, BailoutStack *bailout)
  : JitFrameIterator(iter)
{
    MOZ_CRASH();
}

IonBailoutIterator::IonBailoutIterator(const JitActivationIterator &iter, InvalidationBailoutStack *bailout)
  : JitFrameIterator(iter)
{
    MOZ_CRASH();
}

bool ICCompare_Int32::Compiler::generateStubCode(MacroAssembler &) { MOZ_CRASH(); }
bool ICCompare_Double::Compiler::generateStubCode(MacroAssembler &) { MOZ_CRASH(); }
bool ICBinaryArith_Int32::Compiler::generateStubCode(MacroAssembler &) { MOZ_CRASH(); }
bool ICUnaryArith_Int32::Compiler::generateStubCode(MacroAssembler &) { MOZ_CRASH(); }
