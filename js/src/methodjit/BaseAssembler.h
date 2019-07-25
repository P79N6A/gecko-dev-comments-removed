







































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

class MaybeJump {
    typedef JSC::MacroAssembler::Jump Jump;
  public:
    MaybeJump()
      : set(false)
    { }

    inline Jump getJump() const { JS_ASSERT(set); return jump; }
    inline Jump get() const { JS_ASSERT(set); return jump; }
    inline void setJump(const Jump &j) { jump = j; set = true; }
    inline bool isSet() const { return set; }

    inline MaybeJump &operator=(Jump j) { setJump(j); return *this; }

  private:
    Jump jump;
    bool set;
};



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

    
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const RegisterID ClobberInCall = JSC::X86Registers::ecx;
#elif defined(JS_CPU_ARM)
    static const RegisterID ClobberInCall = JSC::ARMRegisters::r2;
#endif

    
    Label startLabel;
    Vector<CallPatch, 64, SystemAllocPolicy> callPatches;

  public:
#if defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)
    
    
    Label callLabel;
#endif
    BaseAssembler()
      : callPatches(SystemAllocPolicy())
    {
        startLabel = label();
    }

    
    static const uint32 TotalFPRegisters = FPRegisters::TotalFPRegisters;

    


#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const RegisterID JSFrameReg = JSC::X86Registers::ebx;
#elif defined(JS_CPU_ARM)
    static const RegisterID JSFrameReg = JSC::ARMRegisters::r11;
#endif

    
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
static const JSC::MacroAssembler::RegisterID JSReturnReg_Type = JSC::X86Registers::ecx;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data = JSC::X86Registers::edx;
#elif defined(JS_CPU_ARM)
static const JSC::MacroAssembler::RegisterID JSReturnReg_Type = JSC::ARMRegisters::r2;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data = JSC::ARMRegisters::r1;
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

    Jump testFunction(Condition cond, RegisterID fun) {
        return branchPtr(cond, Address(fun, offsetof(JSObject, clasp)),
                         ImmPtr(&js_FunctionClass));
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

#ifdef JS_CPU_X86
    void idiv(RegisterID reg) {
        m_assembler.cdq();
        m_assembler.idivl_r(reg);
    }

    void fastLoadDouble(RegisterID lo, RegisterID hi, FPRegisterID fpReg) {
        if (MacroAssemblerX86Common::getSSEState() >= HasSSE4_1) {
            m_assembler.movd_rr(lo, fpReg);
            m_assembler.pinsrd_rr(hi, fpReg);
        } else {
            m_assembler.movd_rr(lo, fpReg);
            m_assembler.movd_rr(hi, FPRegisters::Temp0);
            m_assembler.unpcklps_rr(FPRegisters::Temp0, fpReg);
        }
    }
#endif

    


    void * getCallTarget(void *fun) {
#ifdef JS_CPU_ARM
        










        void *pfun = JS_FUNC_TO_DATA_PTR(void *, JaegerStubVeneer);

        




        move(Imm32(intptr_t(fun)), JSC::ARMRegisters::ip);
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
    STUB_CALL_TYPE(VoidStub);

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
#if defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)
        push(Registers::ArgReg1);
        push(Registers::ArgReg0);
#endif
        Call cl = call(pfun);
#if defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)
        callLabel = label();
        addPtr(JSC::MacroAssembler::Imm32(8),
               JSC::MacroAssembler::stackPointerRegister);
#endif
        return cl;
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

#if defined(JS_CPU_ARM)
    void ret() {
        




        MacroAssembler::pop(JSC::ARMRegisters::pc);
    }
    
#endif

    void finalize(uint8 *ncode) {
        JSC::JITCode jc(ncode, size());
        JSC::CodeBlock cb(jc);
        JSC::RepatchBuffer repatchBuffer(&cb);

        for (size_t i = 0; i < callPatches.length(); i++) {
            JSC::MacroAssemblerCodePtr cp(ncode + callPatches[i].distance);
            repatchBuffer.relink(JSC::CodeLocationCall(cp), callPatches[i].fun);
        }
    }

    




#ifdef JS_CPU_X86
    static void insertJump(uint8 *source, const uint8 *target) {
        source[0] = 0xE9; 
        *reinterpret_cast<int*>(source + 1) = (int) target - (int) source - 5;
    }
#endif
};


static const JSC::MacroAssembler::RegisterID JSFrameReg = BaseAssembler::JSFrameReg;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Type = BaseAssembler::JSReturnReg_Type;
static const JSC::MacroAssembler::RegisterID JSReturnReg_Data = BaseAssembler::JSReturnReg_Data;

} 
} 

#endif

