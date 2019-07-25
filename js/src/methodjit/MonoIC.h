







































#if !defined jsjaeger_mono_ic_h__ && defined JS_METHODJIT && defined JS_MONOIC
#define jsjaeger_mono_ic_h__

#include "assembler/assembler/MacroAssembler.h"
#include "assembler/assembler/CodeLocation.h"
#include "methodjit/MethodJIT.h"
#include "CodeGenIncludes.h"

namespace js {
namespace mjit {
namespace ic {

struct MICInfo {
#ifdef JS_CPU_X86
    static const uint32 GET_DATA_OFFSET = 6;
    static const uint32 GET_TYPE_OFFSET = 12;

    static const uint32 SET_TYPE_OFFSET = 6;
    static const uint32 SET_DATA_CONST_TYPE_OFFSET = 16;
    static const uint32 SET_DATA_TYPE_OFFSET = 12;
#elif JS_CPU_X64 || JS_CPU_ARM
    
    
#endif

    enum Kind
#ifdef _MSC_VER
    : uint8_t
#endif
    {
        GET,
        SET,
        CALL,
        EMPTYCALL,  
        TRACER
    };

    
    JSC::CodeLocationLabel entry;
    JSC::CodeLocationLabel stubEntry;

    

    
    JSC::CodeLocationLabel load;
    JSC::CodeLocationDataLabel32 shape;
    JSC::CodeLocationCall stubCall;
#if defined JS_PUNBOX64
    uint32 patchValueOffset;
#endif

    
    uint32 argc;
    uint32 frameDepth;
    JSC::CodeLocationLabel knownObject;
    JSC::CodeLocationLabel callEnd;
    JSC::MacroAssembler::RegisterID dataReg;

    
    JSC::CodeLocationJump traceHint;
    JSC::CodeLocationJump slowTraceHint;

    
    Kind kind : 4;
    union {
        
        struct {
            bool touched : 1;
            bool typeConst : 1;
            bool dataConst : 1;
            bool dataWrite : 1;
        } name;
        
        bool generated;
        
        bool hasSlowTraceHint;
    } u;
};

void JS_FASTCALL GetGlobalName(VMFrame &f, uint32 index);
void JS_FASTCALL SetGlobalName(VMFrame &f, uint32 index);

#ifdef JS_CPU_X86


class NativeCallCompiler
{
    typedef JSC::MacroAssembler::Jump Jump;

    struct Patch {
        Patch(Jump from, uint8 *to)
          : from(from), to(to)
        { }

        Jump from;
        uint8 *to;
    };

  public:
    Assembler masm;

  private:
    
    Vector<Patch, 8, SystemAllocPolicy> jumps;

  public:
    NativeCallCompiler();

    size_t size() { return masm.size(); }
    uint8 *buffer() { return masm.buffer(); }

    
    void addLink(Jump j, uint8 *target) { jumps.append(Patch(j, target)); }

    



    void finish(JSScript *script, uint8 *start, uint8 *fallthrough);
};

void CallFastNative(JSContext *cx, JSScript *script, MICInfo &mic, JSFunction *fun, bool isNew);

#endif 

void PurgeMICs(JSContext *cx, JSScript *script);

} 
} 
} 

#endif 

