






































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
    uint32     frameDepth;      
    uint32     argc;            
    RegisterID funObjReg;       
    jsbytecode *pc;             
    uint32     flags;           

  public:
    



    Registers  tempRegs;

    InlineFrameAssembler(Assembler &masm, ic::CallICInfo &ic, uint32 flags)
      : masm(masm), pc(ic.pc), flags(flags)
    {
        frameDepth = ic.frameDepth;
        argc = ic.argc;
        funObjReg = ic.funObjReg;
        tempRegs.takeReg(ic.funPtrReg);
        tempRegs.takeReg(funObjReg);
    }

    InlineFrameAssembler(Assembler &masm, Compiler::CallGenInfo &gen, uint32 flags)
      : masm(masm), pc(gen.pc), flags(flags)
    {
        frameDepth = gen.frameDepth;
        argc = gen.argc;
        funObjReg = gen.funObjReg;
        tempRegs.takeReg(funObjReg);
    }

    DataLabelPtr assemble(void *ncode)
    {
        JS_ASSERT((flags & ~JSFRAME_CONSTRUCTING) == 0);

        RegisterID t0 = tempRegs.takeAnyReg();

        AdjustedFrame adj(sizeof(JSStackFrame) + frameDepth * sizeof(Value));
        masm.store32(Imm32(JSFRAME_FUNCTION | flags), adj.addrOf(JSStackFrame::offsetOfFlags()));
        masm.loadPtr(Address(funObjReg, offsetof(JSObject, parent)), t0);
        masm.storePtr(t0, adj.addrOf(JSStackFrame::offsetOfScopeChain()));
        masm.storePtr(JSFrameReg, adj.addrOf(JSStackFrame::offsetOfPrev()));

        DataLabelPtr ncodePatch =
            masm.storePtrWithPatch(ImmPtr(ncode), adj.addrOf(JSStackFrame::offsetOfncode()));

        
        masm.addPtr(Imm32(sizeof(JSStackFrame) + sizeof(Value) * frameDepth), JSFrameReg);

        tempRegs.putReg(t0);

        return ncodePatch;
    }
};


} 
} 

#endif 

