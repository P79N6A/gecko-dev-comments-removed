









































#if !defined jsjaeger_icrepatcher_h__ && defined JS_METHODJIT
#define jsjaeger_icrepatcher_h__

#include "assembler/assembler/RepatchBuffer.h"
#include "assembler/moco/MocoStubs.h"
#include "methodjit/ICChecker.h"

namespace js {
namespace mjit {
namespace ic {

class Repatcher : public JSC::RepatchBuffer
{
    typedef JSC::CodeLocationLabel  CodeLocationLabel;
    typedef JSC::CodeLocationCall   CodeLocationCall;
    typedef JSC::FunctionPtr        FunctionPtr;

    CodeLocationLabel label;

  public:
    explicit Repatcher(JITScript *js)
      : JSC::RepatchBuffer(js->code), label(js->code.m_code.executableAddress())
    { }

    explicit Repatcher(const JSC::JITCode &code)
      : JSC::RepatchBuffer(code), label(code.start())
    { }

    using JSC::RepatchBuffer::relink;

    
    void relink(CodeLocationCall call, FunctionPtr stub) {
#if defined JS_CPU_X64 || defined JS_CPU_X86
        JSC::RepatchBuffer::relink(call, stub);
#elif defined JS_CPU_ARM
        










        CheckIsStubCall(call.labelAtOffset(0));
        JSC::RepatchBuffer::relink(call.callAtOffset(-4), stub);
#else
# error
#endif
    }

    
    void patchAddressOffsetForValueLoad(CodeLocationLabel label, uint32 offset) {
#if defined JS_CPU_X64 || defined JS_CPU_ARM
        repatch(label.dataLabel32AtOffset(0), offset);
#elif defined JS_CPU_X86
        static const unsigned LOAD_TYPE_OFFSET = 6;
        static const unsigned LOAD_DATA_OFFSET = 12;

        





        repatch(label.dataLabel32AtOffset(LOAD_DATA_OFFSET), offset);
        repatch(label.dataLabel32AtOffset(LOAD_TYPE_OFFSET), offset + 4);
#else
# error
#endif
    }

    void patchAddressOffsetForValueStore(CodeLocationLabel label, uint32 offset, bool typeConst) {
#if defined JS_CPU_ARM || defined JS_CPU_X64
        (void) typeConst;
        repatch(label.dataLabel32AtOffset(0), offset);
#elif defined JS_CPU_X86
        static const unsigned STORE_TYPE_OFFSET = 6;
        static const unsigned STORE_DATA_CONST_TYPE_OFFSET = 16;
        static const unsigned STORE_DATA_TYPE_OFFSET = 12;

        























        repatch(label.dataLabel32AtOffset(STORE_TYPE_OFFSET), offset + 4);

        unsigned payloadOffset = typeConst ? STORE_DATA_CONST_TYPE_OFFSET : STORE_DATA_TYPE_OFFSET;
        repatch(label.dataLabel32AtOffset(payloadOffset), offset);
#else
# error
#endif
    }
};

} 
} 
} 

#endif
