






































#if !defined jsjaeger_inl_frame_asm_h__ && defined JS_METHODJIT && defined JS_MONOIC
#define jsjaeger_inl_frame_asm_h__

#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"
#include "CodeGenIncludes.h"

namespace js {
namespace mjit {

struct AdjustedFrame {
    AdjustedFrame(uint32 baseOffset)
     : baseOffset(baseOffset)
    { }

    uint32 baseOffset;

    JSC::MacroAssembler::Address addrOf(uint32 offset) {
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
    jsbytecode *pc;             
    uint32     flags;           

  public:
    



    Registers  tempRegs;

    InlineFrameAssembler(Assembler &masm, ic::CallICInfo &ic, uint32 flags)
      : masm(masm), pc(ic.pc), flags(flags)
    {
        frameSize = ic.frameSize;
        funObjReg = ic.funObjReg;
        tempRegs.takeReg(ic.funPtrReg);
        tempRegs.takeReg(funObjReg);
    }

    InlineFrameAssembler(Assembler &masm, Compiler::CallGenInfo &gen, uint32 flags)
      : masm(masm), pc(gen.pc), flags(flags)
    {
        frameSize = gen.frameSize;
        funObjReg = gen.funObjReg;
        tempRegs.takeReg(funObjReg);
    }

    DataLabelPtr assemble(void *ncode)
    {
        JS_ASSERT((flags & ~JSFRAME_CONSTRUCTING) == 0);

        

        DataLabelPtr ncodePatch;
        if (frameSize.isStatic()) {
            uint32 frameDepth = frameSize.staticLocalSlots();
            AdjustedFrame newfp(sizeof(JSStackFrame) + frameDepth * sizeof(Value));

            Address flagsAddr = newfp.addrOf(JSStackFrame::offsetOfFlags());
            masm.store32(Imm32(JSFRAME_FUNCTION | flags), flagsAddr);
            Address prevAddr = newfp.addrOf(JSStackFrame::offsetOfPrev());
            masm.storePtr(JSFrameReg, prevAddr);
            Address ncodeAddr = newfp.addrOf(JSStackFrame::offsetOfncode());
            ncodePatch = masm.storePtrWithPatch(ImmPtr(ncode), ncodeAddr);

            masm.addPtr(Imm32(sizeof(JSStackFrame) + frameDepth * sizeof(Value)), JSFrameReg);
        } else {
            







            RegisterID newfp = tempRegs.takeAnyReg();
            masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.sp)), newfp);

            Address flagsAddr(newfp, JSStackFrame::offsetOfFlags());
            masm.store32(Imm32(JSFRAME_FUNCTION | flags), flagsAddr);
            Address prevAddr(newfp, JSStackFrame::offsetOfPrev());
            masm.storePtr(JSFrameReg, prevAddr);
            Address ncodeAddr(newfp, JSStackFrame::offsetOfncode());
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

