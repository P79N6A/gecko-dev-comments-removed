






































#if !defined jsjaeger_compilerbase_h__ && defined JS_METHODJIT
#define jsjaeger_compilerbase_h__

#include "jscntxt.h"
#include "jstl.h"
#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/LinkBuffer.h"

namespace js {
namespace mjit {

class BaseCompiler
{
  protected:
    JSContext *cx;

  public:
    BaseCompiler() : cx(NULL)
    { }

    BaseCompiler(JSContext *cx) : cx(cx)
    { }

  protected:
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
    typedef JSC::CodeBlock CodeBlock;
    typedef JSC::CodeLocationLabel CodeLocationLabel;
    typedef JSC::JITCode JITCode;
    typedef JSC::ReturnAddressPtr ReturnAddressPtr;
    typedef JSC::MacroAssemblerCodePtr MacroAssemblerCodePtr;

    JSC::ExecutablePool *
    getExecPool(size_t size) {
        return BaseCompiler::GetExecPool(cx, size);
    }

  public:
    static JSC::ExecutablePool *
    GetExecPool(JSContext *cx, size_t size) {
        ThreadData *jaegerData = &JS_METHODJIT_DATA(cx);
        JSC::ExecutablePool *pool = jaegerData->execAlloc->poolForSize(size);
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
};

} 
} 

#endif
