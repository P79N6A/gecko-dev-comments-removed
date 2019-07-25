






































#if !defined jsjaeger_baseassembler_h__ && defined JS_METHODJIT
#define jsjaeger_baseassembler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/RepatchBuffer.h"
#include "assembler/moco/MocoStubs.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/MachineRegs.h"

namespace js {
namespace mjit {



struct FrameAddress : JSC::MacroAssembler::Address
{
    FrameAddress(int32 offset)
      : Address(JSC::MacroAssembler::stackPointerRegister, offset)
    { }
};

struct ImmIntPtr : public JSC::MacroAssembler::ImmPtr
{
    ImmIntPtr(intptr_t val)
      : ImmPtr(reinterpret_cast<void*>(val))
    { }
};

class BaseAssembler : public JSC::MacroAssembler
{
    struct CallPatch {
        CallPatch(ptrdiff_t distance, void *fun)
          : distance(distance), fun(fun)
        { }

        ptrdiff_t distance;
        JSC::FunctionPtr fun;
    };

    
#if defined(JS_CPU_X86) || defined(JS_CPU_ARM)
    static const RegisterID ClobberInCall = JSC::X86Registers::ecx;
#elif defined(JS_CPU_ARM)
    static const RegisterID ClobberInCall = JSC::ARMRegisters::r2;
#endif

    
    Label startLabel;
    Vector<CallPatch, 64, SystemAllocPolicy> callPatches;

  public:
    BaseAssembler()
      : callPatches(SystemAllocPolicy())
    {
        startLabel = label();
    }

    
    static const uint32 TotalFPRegisters = FPRegisters::TotalFPRegisters;

    


#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const RegisterID JSFrameReg = JSC::X86Registers::ebx;
#elif defined(JS_CPU_ARM)
    static const RegisterID JSFrameReg = JSC::X86Registers::r11;
#endif

    size_t distanceOf(Label l) {
        return differenceBetween(startLabel, l);
    }

    void load32FromImm(void *ptr, RegisterID reg) {
        load32(ptr, reg);
    }

    void loadShape(RegisterID obj, RegisterID shape) {
        loadPtr(Address(obj, offsetof(JSObject, map)), shape);
        load32(Address(shape, offsetof(JSObjectMap, shape)), shape);
    }

    


    Address objSlotRef(JSObject *obj, RegisterID reg, uint32 slot) {
        if (slot < JS_INITIAL_NSLOTS) {
            void *vp = &obj->getSlotRef(slot);
            move(ImmPtr(vp), reg);
            return Address(reg, 0);
        }
        move(ImmPtr(&obj->dslots), reg);
        loadPtr(reg, reg);
        return Address(reg, (slot - JS_INITIAL_NSLOTS) * sizeof(Value));
    }

    


    void * getCallTarget(void *fun) {
#ifdef JS_CPU_ARM
        










        void *pfun = JS_FUNC_TO_DATA_PTR(void *, JaegerStubVeneer);

        




        move(Imm32(intptr_t(fun)), ARMRegisters::ip);
#else
        




        void *pfun = fun;
#endif
        return pfun;
    }


#define STUB_CALL_TYPE(type)                                    \
    Call stubCall(type stub, jsbytecode *pc, uint32 fd) {       \
        return stubCall(JS_FUNC_TO_DATA_PTR(void *, stub),      \
                        pc, fd);                                \
    }

    STUB_CALL_TYPE(JSObjStub);
    STUB_CALL_TYPE(VoidPtrStubUInt32);
    STUB_CALL_TYPE(VoidStubUInt32);

#undef STUB_CALL_TYPE

    Call stubCall(void *ptr, jsbytecode *pc, uint32 frameDepth) {
        JS_STATIC_ASSERT(ClobberInCall != Registers::ArgReg1);

        void *pfun = getCallTarget(ptr);

        
        storePtr(ImmPtr(pc),
                 FrameAddress(offsetof(VMFrame, regs) + offsetof(JSFrameRegs, pc)));

        
        fixScriptStack(frameDepth);

        
        setupVMFrame();

#ifdef JS_METHODJIT_PROFILE_STUBS
        push(Registers::ArgReg0);
        push(Registers::ArgReg1);
        call(JS_FUNC_TO_DATA_PTR(void *, mjit::ProfileStubCall));
        pop(Registers::ArgReg1);
        pop(Registers::ArgReg0);
#endif

        return call(pfun);
    }

    void fixScriptStack(uint32 frameDepth) {
        
        addPtr(Imm32(sizeof(JSStackFrame) + frameDepth * sizeof(jsval)),
               JSFrameReg,
               ClobberInCall);

        
        storePtr(ClobberInCall,
                 FrameAddress(offsetof(VMFrame, regs) + offsetof(JSFrameRegs, sp)));
    }

    void setupVMFrame() {
        move(MacroAssembler::stackPointerRegister, Registers::ArgReg0);
    }

    Call call(void *fun) {
#if defined(_MSC_VER) && defined(_M_X64)
        subPtr(JSC::MacroAssembler::Imm32(32),
               JSC::MacroAssembler::stackPointerRegister);
#endif

        Call cl = JSC::MacroAssembler::call();

#if defined(_MSC_VER) && defined(_M_X64)
        addPtr(JSC::MacroAssembler::Imm32(32),
               JSC::MacroAssembler::stackPointerRegister);
#endif

        callPatches.append(CallPatch(differenceBetween(startLabel, cl), fun));
        return cl;
    }

    Call call(RegisterID reg) {
        return MacroAssembler::call(reg);
    }

    void finalize(uint8 *ncode) {
        JSC::JITCode jc(ncode, size());
        JSC::CodeBlock cb(jc);
        JSC::RepatchBuffer repatchBuffer(&cb);

        for (size_t i = 0; i < callPatches.length(); i++) {
            JSC::MacroAssemblerCodePtr cp(ncode + callPatches[i].distance);
            repatchBuffer.relink(JSC::CodeLocationCall(cp), callPatches[i].fun);
        }
    }
};


static const JSC::MacroAssembler::RegisterID JSFrameReg = BaseAssembler::JSFrameReg;

} 
} 

#endif

