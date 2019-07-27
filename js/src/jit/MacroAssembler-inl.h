





#ifndef jit_MacroAssembler_inl_h
#define jit_MacroAssembler_inl_h

#include "jit/MacroAssembler.h"

namespace js {
namespace jit {




uint32_t
MacroAssembler::framePushed() const
{
    return framePushed_;
}

void
MacroAssembler::setFramePushed(uint32_t framePushed)
{
    framePushed_ = framePushed;
}

void
MacroAssembler::adjustFrame(int32_t value)
{
    MOZ_ASSERT_IF(value < 0, framePushed_ >= uint32_t(-value));
    setFramePushed(framePushed_ + value);
}

void
MacroAssembler::implicitPop(uint32_t bytes)
{
    MOZ_ASSERT(bytes % sizeof(intptr_t) == 0);
    MOZ_ASSERT(bytes <= INT32_MAX);
    adjustFrame(-int32_t(bytes));
}




CodeOffsetLabel
MacroAssembler::PushWithPatch(ImmWord word)
{
    framePushed_ += sizeof(word.value);
    return pushWithPatch(word);
}

CodeOffsetLabel
MacroAssembler::PushWithPatch(ImmPtr imm)
{
    return PushWithPatch(ImmWord(uintptr_t(imm.value)));
}




void
MacroAssembler::call(const CallSiteDesc& desc, const Register reg)
{
    call(reg);
    append(desc, currentOffset(), framePushed());
}

void
MacroAssembler::call(const CallSiteDesc& desc, Label* label)
{
    call(label);
    append(desc, currentOffset(), framePushed());
}



void
MacroAssembler::PushStubCode()
{
    exitCodePatch_ = PushWithPatch(ImmWord(-1));
}

void
MacroAssembler::enterExitFrame(const VMFunction* f)
{
    linkExitFrame();
    
    PushStubCode();
    
    Push(ImmPtr(f));
}

void
MacroAssembler::enterFakeExitFrame(JitCode* codeVal)
{
    linkExitFrame();
    Push(ImmPtr(codeVal));
    Push(ImmPtr(nullptr));
}

void
MacroAssembler::leaveExitFrame(size_t extraFrame)
{
    freeStack(ExitFooterFrame::Size() + extraFrame);
}

bool
MacroAssembler::hasEnteredExitFrame() const
{
    return exitCodePatch_.offset() != 0;
}

} 
} 

#endif 
