





#include "jit/shared/BaselineCompiler-shared.h"

#include "jit/BaselineIC.h"
#include "jit/VMFunctions.h"

using namespace js;
using namespace js::jit;

BaselineCompilerShared::BaselineCompilerShared(JSContext *cx, TempAllocator &alloc, JSScript *script)
  : cx(cx),
    script(script),
    pc(script->code()),
    ionCompileable_(jit::IsIonEnabled(cx) && CanIonCompileScript(cx, script, false)),
    ionOSRCompileable_(jit::IsIonEnabled(cx) && CanIonCompileScript(cx, script, true)),
    debugMode_(cx->compartment()->debugMode()),
    alloc_(alloc),
    analysis_(alloc, script),
    frame(script, masm),
    stubSpace_(),
    icEntries_(),
    pcMappingEntries_(),
    icLoadLabels_(),
    pushedBeforeCall_(0),
    inCall_(false),
    spsPushToggleOffset_()
{ }

bool
BaselineCompilerShared::callVM(const VMFunction &fun, CallVMPhase phase)
{
    JitCode *code = cx->runtime()->jitRuntime()->getVMWrapper(fun);
    if (!code)
        return false;

#ifdef DEBUG
    
    JS_ASSERT(inCall_);
    inCall_ = false;
#endif

    
    
    uint32_t argSize = fun.explicitStackSlots() * sizeof(void *) + sizeof(void *);

    
    JS_ASSERT(masm.framePushed() - pushedBeforeCall_ == argSize);

    Address frameSizeAddress(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize());
    uint32_t frameVals = frame.nlocals() + frame.stackDepth();
    uint32_t frameBaseSize = BaselineFrame::FramePointerOffset + BaselineFrame::Size();
    uint32_t frameFullSize = frameBaseSize + (frameVals * sizeof(Value));
    if (phase == POST_INITIALIZE) {
        masm.store32(Imm32(frameFullSize), frameSizeAddress);
        uint32_t descriptor = MakeFrameDescriptor(frameFullSize + argSize, JitFrame_BaselineJS);
        masm.push(Imm32(descriptor));

    } else if (phase == PRE_INITIALIZE) {
        masm.store32(Imm32(frameBaseSize), frameSizeAddress);
        uint32_t descriptor = MakeFrameDescriptor(frameBaseSize + argSize, JitFrame_BaselineJS);
        masm.push(Imm32(descriptor));

    } else {
        JS_ASSERT(phase == CHECK_OVER_RECURSED);
        Label afterWrite;
        Label writePostInitialize;

        
        masm.branchTest32(Assembler::Zero,
                          frame.addressOfFlags(),
                          Imm32(BaselineFrame::OVER_RECURSED),
                          &writePostInitialize);

        masm.move32(Imm32(frameBaseSize), BaselineTailCallReg);
        masm.jump(&afterWrite);

        masm.bind(&writePostInitialize);
        masm.move32(Imm32(frameFullSize), BaselineTailCallReg);

        masm.bind(&afterWrite);
        masm.store32(BaselineTailCallReg, frameSizeAddress);
        masm.add32(Imm32(argSize), BaselineTailCallReg);
        masm.makeFrameDescriptor(BaselineTailCallReg, JitFrame_BaselineJS);
        masm.push(BaselineTailCallReg);
    }

    
    masm.call(code);
    uint32_t callOffset = masm.currentOffset();
    masm.pop(BaselineFrameReg);

    
    
    ICEntry entry(script->pcToOffset(pc), ICEntry::Kind_CallVM);
    entry.setReturnOffset(CodeOffsetLabel(callOffset));

    return icEntries_.append(entry);
}
