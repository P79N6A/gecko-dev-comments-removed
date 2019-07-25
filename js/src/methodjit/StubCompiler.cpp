







































#include "StubCompiler.h"
#include "Compiler.h"

using namespace js;
using namespace mjit;

StubCompiler::StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame, JSScript *script)
  : cx(cx), cc(cc), frame(frame), script(script), exits(SystemAllocPolicy())
{
}

void
StubCompiler::linkExit(Jump j)
{
    
    exits.append(ExitPatch(j, masm.label()));
}

void
StubCompiler::leave()
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN STUB SPILL CODE ---- \n");
    frame.sync(masm, snapshot);
    JaegerSpew(JSpew_Insns, " ---- END STUB SPILL CODE ---- \n");
    JaegerSpew(JSpew_Insns, " ---- BEGIN STUB CALL CODE ---- \n");
}

void
StubCompiler::rejoin(uint32 invalidationDepth)
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN STUB RESTORE CODE ---- \n");
    frame.merge(masm, snapshot, invalidationDepth);
    JaegerSpew(JSpew_Insns, " ---- END STUB RESTORE CODE ---- \n");
}

typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::ImmPtr ImmPtr;
typedef JSC::MacroAssembler::Imm32 Imm32;

JSC::MacroAssembler::Call
StubCompiler::stubCall(void *ptr)
{
    Call cl = masm.stubCall(ptr, cc.getPC(), frame.stackDepth() + script->nfixed);
    JaegerSpew(JSpew_Insns, " ---- END STUB CALL CODE ---- \n");
    return cl;
}

void
StubCompiler::finalize(uint8* ncode)
{
    masm.finalize(ncode);
}

