





#include "jscompartment.h"

#include "jit/Bailouts.h"
#include "jit/BaselineIC.h"
#include "jit/IonCaches.h"

using namespace js;
using namespace js::jit;





JitCode* JitRuntime::generateEnterJIT(JSContext*, EnterJitType) { MOZ_CRASH(); }
JitCode* JitRuntime::generateInvalidator(JSContext*) { MOZ_CRASH(); }
JitCode* JitRuntime::generateArgumentsRectifier(JSContext*, void**) { MOZ_CRASH(); }
JitCode* JitRuntime::generateBailoutTable(JSContext*, uint32_t) { MOZ_CRASH(); }
JitCode* JitRuntime::generateBailoutHandler(JSContext*) { MOZ_CRASH(); }
JitCode* JitRuntime::generateVMWrapper(JSContext*, const VMFunction&) { MOZ_CRASH(); }
JitCode* JitRuntime::generatePreBarrier(JSContext*, MIRType) { MOZ_CRASH(); }
JitCode* JitRuntime::generateDebugTrapHandler(JSContext*) { MOZ_CRASH(); }
JitCode* JitRuntime::generateExceptionTailStub(JSContext*, void*) { MOZ_CRASH(); }
JitCode* JitRuntime::generateBailoutTailStub(JSContext*) { MOZ_CRASH(); }

FrameSizeClass FrameSizeClass::FromDepth(uint32_t) { MOZ_CRASH(); }
FrameSizeClass FrameSizeClass::ClassLimit() { MOZ_CRASH(); }
uint32_t FrameSizeClass::frameSize() const { MOZ_CRASH(); }

const Register ABIArgGenerator::NonArgReturnReg0 = { Registers::invalid_reg };
const Register ABIArgGenerator::NonArgReturnReg1 = {  Registers::invalid_reg };
const Register ABIArgGenerator::NonArg_VolatileReg = {  Registers::invalid_reg };
const Register ABIArgGenerator::NonReturn_VolatileReg0 = {  Registers::invalid_reg };
const Register ABIArgGenerator::NonReturn_VolatileReg1 = {  Registers::invalid_reg };

BailoutFrameInfo::BailoutFrameInfo(const JitActivationIterator& iter, BailoutStack* bailout)
{
    MOZ_CRASH();
}

BailoutFrameInfo::BailoutFrameInfo(const JitActivationIterator& iter, InvalidationBailoutStack* bailout)
{
    MOZ_CRASH();
}

bool ICCompare_Int32::Compiler::generateStubCode(MacroAssembler&) { MOZ_CRASH(); }
bool ICCompare_Double::Compiler::generateStubCode(MacroAssembler&) { MOZ_CRASH(); }
bool ICBinaryArith_Int32::Compiler::generateStubCode(MacroAssembler&) { MOZ_CRASH(); }
bool ICUnaryArith_Int32::Compiler::generateStubCode(MacroAssembler&) { MOZ_CRASH(); }
JitCode* JitRuntime::generateProfilerExitFrameTailStub(JSContext*) { MOZ_CRASH(); }




void MacroAssembler::PushRegsInMask(LiveRegisterSet) { MOZ_CRASH(); }
void MacroAssembler::PopRegsInMaskIgnore(LiveRegisterSet, LiveRegisterSet) { MOZ_CRASH(); }

void MacroAssembler::Push(Register reg) { MOZ_CRASH(); }
void MacroAssembler::Push(const Imm32 imm) { MOZ_CRASH(); }
void MacroAssembler::Push(const ImmWord imm) { MOZ_CRASH(); }
void MacroAssembler::Push(const ImmPtr imm) { MOZ_CRASH(); }
void MacroAssembler::Push(const ImmGCPtr ptr) { MOZ_CRASH(); }
void MacroAssembler::Push(FloatRegister reg) { MOZ_CRASH(); }

void MacroAssembler::Pop(Register reg) { MOZ_CRASH(); }
void MacroAssembler::Pop(const ValueOperand& val) { MOZ_CRASH(); }
