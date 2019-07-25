







































#include "StubCalls.h"
#include "StubCompiler.h"
#include "Compiler.h"
#include "assembler/assembler/LinkBuffer.h"
#include "FrameState-inl.h"

using namespace js;
using namespace mjit;

StubCompiler::StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame, JSScript *script)
: cx(cx),
  cc(cc),
  frame(frame),
  script(script),
  generation(1),
  lastGeneration(0),
  exits(CompilerAllocPolicy(cx, cc)),
  joins(CompilerAllocPolicy(cx, cc)),
  scriptJoins(CompilerAllocPolicy(cx, cc)),
  jumpList(SystemAllocPolicy())
{
#ifdef DEBUG
    masm.setSpewPath(true);
#endif
}

void
StubCompiler::linkExitDirect(Jump j, Label L)
{
    exits.append(CrossPatch(j, L));
}

JSC::MacroAssembler::Label
StubCompiler::syncExit(Uses uses)
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW MERGE CODE ---- \n");

    if (lastGeneration == generation) {
        Jump j2 = masm.jump();
        jumpList.append(j2);
    }

    Label l = masm.label();
    frame.sync(masm, uses);
    lastGeneration = generation;

    JaegerSpew(JSpew_Insns, " ---- END SLOW MERGE CODE ---- \n");
    
    return l;
}

JSC::MacroAssembler::Label
StubCompiler::syncExitAndJump(Uses uses)
{
    Label l = syncExit(uses);
    Jump j2 = masm.jump();
    jumpList.append(j2);
    
    generation++;
    return l;
}
















JSC::MacroAssembler::Label
StubCompiler::linkExit(Jump j, Uses uses)
{
    Label l = syncExit(uses);
    linkExitDirect(j, l);
    return l;
}





void
StubCompiler::linkExitForBranch(Jump j)
{
    Label l = syncExit(Uses(frame.frameSlots()));
    linkExitDirect(j, l);
}

void
StubCompiler::leave()
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW LEAVE CODE ---- \n");
    for (size_t i = 0; i < jumpList.length(); i++)
        jumpList[i].linkTo(masm.label(), &masm);
    jumpList.clear();
    generation++;
    JaegerSpew(JSpew_Insns, " ---- END SLOW LEAVE CODE ---- \n");
}
 
void
StubCompiler::rejoin(Changes changes)
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW RESTORE CODE ---- \n");

    frame.merge(masm, changes);

    Jump j = masm.jump();
    crossJump(j, cc.getLabel());

    JaegerSpew(JSpew_Insns, " ---- END SLOW RESTORE CODE ---- \n");
}

void
StubCompiler::linkRejoin(Jump j)
{
    crossJump(j, cc.getLabel());
}

typedef JSC::MacroAssembler::RegisterID RegisterID;
typedef JSC::MacroAssembler::ImmPtr ImmPtr;
typedef JSC::MacroAssembler::Imm32 Imm32;

JSC::MacroAssembler::Call
StubCompiler::emitStubCall(void *ptr, uint32 id)
{
    return emitStubCall(ptr, frame.stackDepth() + script->nfixed, id);
}

JSC::MacroAssembler::Call
StubCompiler::emitStubCall(void *ptr, int32 slots, uint32 id)
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW CALL CODE ---- \n");
    Call cl = masm.fallibleVMCall(ptr, cc.getPC(), slots);
    JaegerSpew(JSpew_Insns, " ---- END SLOW CALL CODE ---- \n");
    if (cc.debugMode()) {
        Compiler::InternalCallSite site(masm.callReturnOffset(cl), cc.getPC(), id, true, true);
        cc.addCallSite(site);
    }
    return cl;
}

void
StubCompiler::fixCrossJumps(uint8 *ncode, size_t offset, size_t total)
{
    JSC::LinkBuffer fast(ncode, total);
    JSC::LinkBuffer slow(ncode + offset, total - offset);

    for (size_t i = 0; i < exits.length(); i++)
        fast.link(exits[i].from, slow.locationOf(exits[i].to));

    for (size_t i = 0; i < scriptJoins.length(); i++) {
        const CrossJumpInScript &cj = scriptJoins[i];
        slow.link(cj.from, fast.locationOf(cc.labelOf(cj.pc)));
    }

    for (size_t i = 0; i < joins.length(); i++)
        slow.link(joins[i].from, fast.locationOf(joins[i].to));
}

void
StubCompiler::crossJump(Jump j, Label L)
{
    joins.append(CrossPatch(j, L));
}

bool
StubCompiler::jumpInScript(Jump j, jsbytecode *target)
{
    if (cc.knownJump(target)) {
        crossJump(j, cc.labelOf(target));
        return true;
    } else {
        return scriptJoins.append(CrossJumpInScript(j, target));
    }
}

