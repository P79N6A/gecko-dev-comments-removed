





#include "BaselineCompiler-shared.h"
#include "ion/BaselineIC.h"
#include "ion/VMFunctions.h"

using namespace js;
using namespace js::ion;

BaselineCompilerShared::BaselineCompilerShared(JSContext *cx, HandleScript script)
  : cx(cx),
    script(cx, script),
    pc(script->code),
    ionCompileable_(ion::IsEnabled(cx) && CanIonCompileScript(cx, script)),
    debugMode_(cx->compartment->debugMode()),
    frame(cx, script, masm),
    stubSpace_(),
    icEntries_(),
    pcMappingEntries_(),
    icLoadLabels_(),
    pushedBeforeCall_(0),
    inCall_(false),
    spsPushToggleOffset_()
{ }

bool
BaselineCompilerShared::callVM(const VMFunction &fun)
{
    IonCompartment *ion = cx->compartment->ionCompartment();
    IonCode *code = ion->getVMWrapper(fun);
    if (!code)
        return false;

#ifdef DEBUG
    
    JS_ASSERT(inCall_);
    inCall_ = false;
#endif

    
    
    uint32_t argSize = fun.explicitStackSlots() * sizeof(void *) + sizeof(void *);

    
    JS_ASSERT(masm.framePushed() - pushedBeforeCall_ == argSize);

    uint32_t frameSize = BaselineFrame::FramePointerOffset + BaselineFrame::Size() +
        (frame.nlocals() + frame.stackDepth()) * sizeof(Value);

    masm.store32(Imm32(frameSize), Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFrameSize()));

    uint32_t descriptor = MakeFrameDescriptor(frameSize + argSize, IonFrame_BaselineJS);
    masm.push(Imm32(descriptor));

    
    masm.call(code);
    uint32_t callOffset = masm.currentOffset();
    masm.pop(BaselineFrameReg);

    
    
    ICEntry entry(pc - script->code, false);
    entry.setReturnOffset(callOffset);

    return icEntries_.append(entry);
}
