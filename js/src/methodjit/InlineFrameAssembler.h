






































#if !defined jsjaeger_inl_frame_asm_h__ && defined JS_METHODJIT && defined JS_MONOIC
#define jsjaeger_inl_frame_asm_h__

#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"
#include "CodeGenIncludes.h"

namespace js {
namespace mjit {

struct AdjustedFrame {
    AdjustedFrame(uint32_t baseOffset)
     : baseOffset(baseOffset)
    { }

    uint32_t baseOffset;

    JSC::MacroAssembler::Address addrOf(uint32_t offset) {
        return JSC::MacroAssembler::Address(JSFrameReg, baseOffset + offset);
    }
};







class InlineFrameAssembler {
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::Imm32 Imm32;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::DataLabelPtr DataLabelPtr;

    Assembler &masm;
    FrameSize  frameSize;       
    RegisterID funObjReg;       
    uint32_t   flags;           

  public:
    



    Registers  tempRegs;

    InlineFrameAssembler(Assembler &masm, ic::CallICInfo &ic, uint32_t flags)
      : masm(masm), flags(flags), tempRegs(Registers::AvailRegs)
    {
        frameSize = ic.frameSize;
        funObjReg = ic.funObjReg;
        tempRegs.takeReg(funObjReg);
    }

    InlineFrameAssembler(Assembler &masm, Compiler::CallGenInfo &gen, uint32_t flags)
      : masm(masm), flags(flags), tempRegs(Registers::AvailRegs)
    {
        frameSize = gen.frameSize;
        funObjReg = gen.funObjReg;
        tempRegs.takeReg(funObjReg);
    }

    DataLabelPtr assemble(void *ncode, jsbytecode *pc)
    {
        JS_ASSERT((flags & ~StackFrame::CONSTRUCTING) == 0);

        

        
        JS_ASSERT(!(flags & ~StackFrame::CONSTRUCTING));
        uint32_t flags = this->flags | StackFrame::FUNCTION;
        if (frameSize.lowered(pc))
            flags |= StackFrame::LOWERED_CALL_APPLY;

        DataLabelPtr ncodePatch;
        if (frameSize.isStatic()) {
            uint32_t frameDepth = frameSize.staticLocalSlots();
            AdjustedFrame newfp(sizeof(StackFrame) + frameDepth * sizeof(Value));

            Address flagsAddr = newfp.addrOf(StackFrame::offsetOfFlags());
            masm.store32(Imm32(flags), flagsAddr);
            Address prevAddr = newfp.addrOf(StackFrame::offsetOfPrev());
            masm.storePtr(JSFrameReg, prevAddr);
            Address ncodeAddr = newfp.addrOf(StackFrame::offsetOfNcode());
            ncodePatch = masm.storePtrWithPatch(ImmPtr(ncode), ncodeAddr);

            masm.addPtr(Imm32(sizeof(StackFrame) + frameDepth * sizeof(Value)), JSFrameReg);
        } else {
            







            RegisterID newfp = tempRegs.takeAnyReg().reg();
            masm.loadPtr(FrameAddress(VMFrame::offsetOfRegsSp()), newfp);

            Address flagsAddr(newfp, StackFrame::offsetOfFlags());
            masm.store32(Imm32(flags), flagsAddr);
            Address prevAddr(newfp, StackFrame::offsetOfPrev());
            masm.storePtr(JSFrameReg, prevAddr);
            Address ncodeAddr(newfp, StackFrame::offsetOfNcode());
            ncodePatch = masm.storePtrWithPatch(ImmPtr(ncode), ncodeAddr);

            masm.move(newfp, JSFrameReg);
            tempRegs.putReg(newfp);
        }

        return ncodePatch;
    }
};


} 
} 

#endif 

