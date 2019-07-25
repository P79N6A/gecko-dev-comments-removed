






































#if !defined jsjaeger_baseassembler_h__ && defined JS_METHODJIT
#define jsjaeger_baseassembler_h__

#include "jscntxt.h"
#include "jstl.h"
#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace mjit {

class BaseAssembler : public JSC::MacroAssembler
{
    struct CallPatch {
        CallPatch(ptrdiff_t distance, void *fun)
          : distance(distance), fun(fun)
        { }

        ptrdiff_t distance;
        JSC::FunctionPtr fun;
    };

    
    Label startLabel;
    Vector<CallPatch, 64, SystemAllocPolicy> callPatches;

  public:
    BaseAssembler()
      : callPatches(SystemAllocPolicy())
    {
        startLabel = label();
    }

    Call call(void *fun) {
#if defined(_MSC_VER) && defined(_M_X64)
        masm.subPtr(JSC::MacroAssembler::Imm32(32),
                    JSC::MacroAssembler::stackPointerRegister);
#endif

        Call cl = JSC::MacroAssembler::call();

#if defined(_MSC_VER) && defined(_M_X64)
        masm.addPtr(JSC::MacroAssembler::Imm32(32),
                    JSC::MacroAssembler::stackPointerRegister);
#endif

        callPatches.append(CallPatch(differenceBetween(startLabel, cl), fun));
        return cl;
    }
};

} 
} 

#endif
