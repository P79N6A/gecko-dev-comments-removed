






































#if !defined jsjaeger_framestate_h__ && defined JS_METHODJIT
#define jsjaeger_framestate_h__

#include "jsapi.h"
#include "MachineRegs.h"
#include "assembler/assembler/MacroAssembler.h"
#include "FrameEntry.h"

namespace js {
namespace mjit {

enum TypeInfo {
    Type_Unknown
};

class FrameState
{
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler MacroAssembler;

    struct RegState {
        enum ValuePart {
            Part_Type,
            Part_Data
        };

        uint32     index;
        ValuePart  part;
        bool       spillable;
        bool       tracked;
    };

  public:
    FrameState(JSContext *cx, JSScript *script, MacroAssembler &masm)
      : cx(cx), script(script), masm(masm), base(NULL)
    { }
    ~FrameState();

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const RegisterID FpReg = JSC::X86Registers::ebx;
#elif defined(JS_CPU_ARM)
    static const RegisterID FpReg = JSC::X86Registers::r11;
#endif

  public:
    bool init(uint32 nargs);

    RegisterID allocReg() {
        if (!regalloc.anyRegsFree())
            evictSomething();
        RegisterID reg = regalloc.allocReg();
        regstate[reg].tracked = false;
        return reg;
    }

    void freeReg(RegisterID reg) {
        JS_ASSERT(!regstate[reg].tracked);
        regalloc.freeReg(reg);
    }

    


    void pushUnknownType(RegisterID reg) {
        sp[0].type.setRegister(reg, false);
        sp[0].data.setMemory();
        sp[0].copies = 0;
        regstate[reg].tracked = true;
        regstate[reg].index = uint32(sp - base);
        regstate[reg].part = RegState::Part_Type;
        sp++;
    }

    void push(const Value &v) {
        push(Jsvalify(v));
    }

    void push(const jsval &v) {
        sp[0].setConstant(v);
        sp[0].copies = 0;
        sp++;
        JS_ASSERT(sp - locals <= script->nslots);
    }

    FrameEntry *peek(int32 depth) {
        JS_ASSERT(depth < 0);
        JS_ASSERT(sp + depth >= locals + script->nfixed);
        return &sp[depth];
    }

    void pop() {
        FrameEntry *vi = peek(-1);
        if (!vi->isConstant()) {
            if (vi->type.inRegister())
                regalloc.freeReg(vi->type.reg());
            if (vi->data.inRegister())
                regalloc.freeReg(vi->data.reg());
        }
        sp--;
    }

    Address topOfStack() {
        return Address(FpReg, sizeof(JSStackFrame) +
                              (script->nfixed + stackDepth()) * sizeof(Value));
    }

    uint32 stackDepth() {
        return sp - (locals + script->nfixed);
    }

    RegisterID tempRegForType(FrameEntry *fe) {
        JS_ASSERT(!fe->type.isConstant());
        if (fe->type.inRegister())
            return fe->type.reg();
        JS_NOT_REACHED("wat");
    }

    Address addressOf(FrameEntry *fe) {
        JS_ASSERT(fe >= locals);
        if (fe >= locals) {
            return Address(FpReg, sizeof(JSStackFrame) +
                                  (fe - locals) * script->nfixed);
        }
        return Address(FpReg, 0);
    }

    void assertValidRegisterState();

  private:
    void evictSomething();
    RegisterID getDataReg(FrameEntry *vi, FrameEntry *backing);

  private:
    JSContext *cx;
    JSScript *script;
    MacroAssembler &masm;
    FrameEntry *base;
    FrameEntry *locals;
    FrameEntry *args;
    FrameEntry *sp;
    Registers regalloc;
    RegState  regstate[MacroAssembler::TotalRegisters];
};

} 
} 

#endif 

