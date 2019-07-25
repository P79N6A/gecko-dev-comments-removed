






































#if !defined jsjaeger_inl_frame_asm_h__ && defined JS_METHODJIT && defined JS_MONOIC
#define jsjaeger_inl_frame_asm_h__

#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"
#include "CodeGenIncludes.h"

namespace js {
namespace mjit {















class InlineFrameAssembler {
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::Imm32 Imm32;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;

    Assembler &masm;
    bool       isConstantThis;  
    Value      constantThis;    
    uint32     frameDepth;      
    uint32     argc;            
    RegisterID funObjReg;       
    jsbytecode *pc;             
    uint32     flags;           

  public:
    



    Registers  tempRegs;

    InlineFrameAssembler(Assembler &masm, JSContext *cx, ic::CallICInfo &ic, uint32 flags)
      : masm(masm), flags(flags)
    {
        isConstantThis = ic.isConstantThis;
        constantThis = ic.constantThis;
        frameDepth = ic.frameDepth;
        argc = ic.argc;
        funObjReg = ic.funObjReg;
        pc = cx->regs->pc;
        tempRegs.takeReg(ic.funPtrReg);
        tempRegs.takeReg(funObjReg);
    }

    InlineFrameAssembler(Assembler &masm, Compiler::CallGenInfo &gen, jsbytecode *pc, uint32 flags)
      : masm(masm), pc(pc), flags(flags)
    {
        isConstantThis = gen.isConstantThis;
        constantThis = gen.constantThis;
        frameDepth = gen.frameDepth;
        argc = gen.argc;
        funObjReg = gen.funObjReg;
        tempRegs.takeReg(funObjReg);
    }

    void assemble()
    {
        struct AdjustedFrame {
            AdjustedFrame(uint32 baseOffset)
             : baseOffset(baseOffset)
            { }

            uint32 baseOffset;

            Address addrOf(uint32 offset) {
                return Address(JSFrameReg, baseOffset + offset);
            }
        };

        RegisterID t0 = tempRegs.takeAnyReg();

        
        masm.storePtr(ImmPtr(pc), Address(JSFrameReg, offsetof(JSStackFrame, savedPC)));

        AdjustedFrame adj(sizeof(JSStackFrame) + frameDepth * sizeof(Value));
        masm.store32(Imm32(argc), adj.addrOf(offsetof(JSStackFrame, argc)));
        masm.store32(Imm32(flags), adj.addrOf(offsetof(JSStackFrame, flags)));
        masm.loadPtr(Address(funObjReg, offsetof(JSObject, parent)), t0);
        masm.storePtr(t0, adj.addrOf(JSStackFrame::offsetScopeChain()));
        masm.addPtr(Imm32(adj.baseOffset - (argc * sizeof(Value))), JSFrameReg, t0);
        masm.storePtr(t0, adj.addrOf(offsetof(JSStackFrame, argv)));

        Address targetThis = adj.addrOf(JSStackFrame::offsetThisValue());
        if (isConstantThis) {
            masm.storeValue(constantThis, targetThis);
        } else {
            Address thisvAddr = Address(t0, -int32(sizeof(Value) * 1));
#ifdef JS_NUNBOX32
            RegisterID t1 = tempRegs.takeAnyReg();
            masm.loadPayload(thisvAddr, t1);
            masm.storePayload(t1, targetThis);
            masm.loadTypeTag(thisvAddr, t1);
            masm.storeTypeTag(t1, targetThis);
            tempRegs.putReg(t1);
#elif JS_PUNBOX64
            masm.loadPtr(thisvAddr, t0);
            masm.storePtr(t0, targetThis);
#endif
        }

        masm.storePtr(JSFrameReg, adj.addrOf(offsetof(JSStackFrame, down)));

        
        masm.addPtr(Imm32(sizeof(JSStackFrame) + sizeof(Value) * frameDepth), JSFrameReg);

        tempRegs.putReg(t0);
    }
};


} 
} 

#endif 

