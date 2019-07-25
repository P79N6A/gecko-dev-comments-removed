






































#if !defined jsjaeger_codegen_h__ && defined JS_METHODJIT
#define jsjaeger_codegen_h__

#include "jscntxt.h"
#include "jstl.h"
#include "methodjit/MethodJIT.h"
#include "methodjit/nunbox/FrameState.h"
#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace mjit {

class CodeGenerator
{
    typedef JSC::MacroAssembler MacroAssembler;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::Imm32 Imm32;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::RegisterID RegisterID;

    MacroAssembler &masm;
    FrameState     &frame;

  public:
    CodeGenerator(MacroAssembler &masm, FrameState &frame);

    void storeValue(FrameEntry *vi, Address address, bool popped);
    void storeJsval(const Value &v, Address address);
    void pushValueOntoFrame(Address address);
};

} 
} 

#endif
