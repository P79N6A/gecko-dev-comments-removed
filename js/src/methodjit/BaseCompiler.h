






































#if !defined jsjaeger_compilerbase_h__ && defined JS_METHODJIT
#define jsjaeger_compilerbase_h__

#include "jscntxt.h"
#include "jstl.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/LinkBuffer.h"
#include "assembler/assembler/RepatchBuffer.h"
#include "assembler/jit/ExecutableAllocator.h"

namespace js {
namespace mjit {

struct MacroAssemblerTypedefs {
    typedef JSC::MacroAssembler::Label Label;
    typedef JSC::MacroAssembler::Imm32 Imm32;
    typedef JSC::MacroAssembler::ImmPtr ImmPtr;
    typedef JSC::MacroAssembler::RegisterID RegisterID;
    typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;
    typedef JSC::MacroAssembler::Address Address;
    typedef JSC::MacroAssembler::BaseIndex BaseIndex;
    typedef JSC::MacroAssembler::AbsoluteAddress AbsoluteAddress;
    typedef JSC::MacroAssembler MacroAssembler;
    typedef JSC::MacroAssembler::Jump Jump;
    typedef JSC::MacroAssembler::JumpList JumpList;
    typedef JSC::MacroAssembler::Call Call;
    typedef JSC::MacroAssembler::DataLabelPtr DataLabelPtr;
    typedef JSC::MacroAssembler::DataLabel32 DataLabel32;
    typedef JSC::FunctionPtr FunctionPtr;
    typedef JSC::RepatchBuffer RepatchBuffer;
    typedef JSC::CodeLocationLabel CodeLocationLabel;
    typedef JSC::CodeLocationCall CodeLocationCall;
    typedef JSC::ReturnAddressPtr ReturnAddressPtr;
    typedef JSC::MacroAssemblerCodePtr MacroAssemblerCodePtr;
    typedef JSC::JITCode JITCode;
};

class BaseCompiler : public MacroAssemblerTypedefs
{
  protected:
    JSContext *cx;

  public:
    BaseCompiler() : cx(NULL)
    { }

    BaseCompiler(JSContext *cx) : cx(cx)
    { }

  protected:

    JSC::ExecutablePool *
    getExecPool(size_t size) {
        return BaseCompiler::GetExecPool(cx, size);
    }

  public:
    static JSC::ExecutablePool *
    GetExecPool(JSContext *cx, size_t size) {
        JSC::ExecutablePool *pool = cx->jaegerCompartment()->poolForSize(size);
        if (!pool)
            js_ReportOutOfMemory(cx);
        return pool;
    }
};




class LinkerHelper : public JSC::LinkBuffer
{
  protected:
    JSContext *cx;

  public:
    LinkerHelper(JSContext *cx) : cx(cx)
    { }

    JSC::ExecutablePool *init(Assembler &masm) {
        
        
        JSC::ExecutablePool *ep = BaseCompiler::GetExecPool(cx, masm.size());
        if (!ep)
            return ep;

        m_size = masm.size();
        m_code = executableCopy(masm, ep);
        if (!m_code) {
            ep->release();
            js_ReportOutOfMemory(cx);
            return NULL;
        }
        return ep;
    }

    void maybeLink(MaybeJump jump, JSC::CodeLocationLabel label) {
        if (!jump.isSet())
            return;
        link(jump.get(), label);
    }

    size_t size() const {
        return m_size;
    }
};

class Repatcher : public JSC::RepatchBuffer
{
  public:
    Repatcher(JITScript *jit) : JSC::RepatchBuffer(jit->code)
    { }

    Repatcher(const JSC::JITCode &code) : JSC::RepatchBuffer(code)
    { }
};

} 
} 

#endif
