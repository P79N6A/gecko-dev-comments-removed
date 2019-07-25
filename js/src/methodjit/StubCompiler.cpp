







































#include "StubCompiler.h"
#include "Compiler.h"
#include "assembler/assembler/LinkBuffer.h"

using namespace js;
using namespace mjit;

StubCompiler::StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame, JSScript *script)
  : cx(cx), cc(cc), frame(frame), script(script), generation(1), lastGeneration(0), hasJump(false),
    exits(SystemAllocPolicy()), joins(SystemAllocPolicy())
{
}

bool
StubCompiler::init(uint32 nargs)
{
    return true;
}




























void
StubCompiler::linkExit(Jump j)
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW MERGE CODE ---- \n");
#if 0
    if (lastGeneration == generation) {
        if (hasJump)
            lastJump.linkTo(masm.label(), &masm);
        frame.deltize(masm, snapshot);
        lastJump = masm.jump();
        hasJump = true;
    }
    frame.snapshot(snapshot);
    lastGeneration = generation;
    exits.append(CrossPatch(j, masm.label()));
#endif
    JaegerSpew(JSpew_Insns, " ---- END SLOW MERGE CODE ---- \n");
}

void
StubCompiler::leave()
{
}

void
StubCompiler::rejoin(uint32 invalidationDepth)
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW RESTORE CODE ---- \n");

#if 0
    frame.merge(masm, snapshot, invalidationDepth);

    Jump j = masm.jump();
    joins.append(CrossPatch(j, cc.getLabel()));
#endif

    JaegerSpew(JSpew_Insns, " ---- END SLOW RESTORE CODE ---- \n");
}

typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::ImmPtr ImmPtr;
typedef JSC::MacroAssembler::Imm32 Imm32;

JSC::MacroAssembler::Call
StubCompiler::stubCall(void *ptr)
{
    generation++;
    hasJump = false;
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW CALL CODE ---- \n");
    Call cl = masm.stubCall(ptr, cc.getPC(), frame.stackDepth() + script->nfixed);
    JaegerSpew(JSpew_Insns, " ---- END SLOW CALL CODE ---- \n");
    return cl;
}

void
StubCompiler::fixCrossJumps(uint8 *ncode, size_t offset, size_t total)
{
    JSC::LinkBuffer fast(ncode, total);
    JSC::LinkBuffer slow(ncode + offset, total - offset);

    for (size_t i = 0; i < exits.length(); i++)
        fast.link(exits[i].from, slow.locationOf(exits[i].to));

    for (size_t i = 0; i < joins.length(); i++)
        slow.link(joins[i].from, fast.locationOf(joins[i].to));
}

void
StubCompiler::finalize(uint8 *ncode)
{
    masm.finalize(ncode);
}

