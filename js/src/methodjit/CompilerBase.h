






































#if !defined jsjaeger_compilerbase_h__ && defined JS_METHODJIT
#define jsjaeger_compilerbase_h__

#include "jscntxt.h"
#include "jstl.h"
#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace mjit {

class CompilerBase
{
  protected:
    typedef JSC::MacroAssembler::Label Label;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler MacroAssembler;

  private:
    JSContext *cx;

  public:
    CompilerBase(JSContext *cx)
      : cx(cx)
    { }

  protected:
    JSC::ExecutablePool *getExecPool(size_t size) {
        ThreadData *jaegerData = &JS_METHODJIT_DATA(cx);
        return jaegerData->execPool->poolForSize(size);
    }
};

} 
} 

#endif
