







































#include "StubCalls.h"
#include "StubCompiler.h"
#include "Compiler.h"
#include "assembler/assembler/LinkBuffer.h"
#include "FrameState-inl.h"

using namespace js;
using namespace mjit;

StubCompiler::StubCompiler(JSContext *cx, mjit::Compiler &cc, FrameState &frame, JSScript *script)
  : cx(cx), cc(cc), frame(frame), script(script), generation(1), lastGeneration(0),
    exits(SystemAllocPolicy()), joins(SystemAllocPolicy()), jumpList(SystemAllocPolicy())
{
}

bool
StubCompiler::init(uint32 nargs)
{
    return true;
}

void
StubCompiler::linkExitDirect(Jump j, Label L)
{
    exits.append(CrossPatch(j, L));
}

JSC::MacroAssembler::Label
StubCompiler::syncExit()
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW MERGE CODE ---- \n");

    if (lastGeneration == generation) {
        Jump j2 = masm.jump();
        jumpList.append(j2);
    }

    Label l = masm.label();
    frame.sync(masm);
    lastGeneration = generation;

    JaegerSpew(JSpew_Insns, " ---- END SLOW MERGE CODE ---- \n");
    
    return l;
}

JSC::MacroAssembler::Label
StubCompiler::syncExitAndJump()
{
    Label l = syncExit();
    Jump j2 = masm.jump();
    jumpList.append(j2);
    
    generation++;
    return l;
}








void
StubCompiler::linkExit(Jump j)
{
    Label l = syncExit();
    linkExitDirect(j, l);
}

void
StubCompiler::leave()
{
    for (size_t i = 0; i < jumpList.length(); i++)
        jumpList[i].linkTo(masm.label(), &masm);
    jumpList.clear();
    generation++;
}
 
void
StubCompiler::rejoin(uint32 invalidationDepth)
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW RESTORE CODE ---- \n");

    frame.merge(masm, invalidationDepth);

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
StubCompiler::stubCall(void *ptr)
{
    return stubCall(ptr, frame.stackDepth() + script->nfixed);
}

JSC::MacroAssembler::Call
StubCompiler::stubCall(void *ptr, uint32 slots)
{
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW CALL CODE ---- \n");
    Call cl = masm.stubCall(ptr, cc.getPC(), slots);
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

    for (size_t i = 0; i < scriptJoins.length(); i++) {
        const CrossJumpInScript &cj = scriptJoins[i];
        slow.link(cj.from, fast.locationOf(cc.labelOf(cj.pc)));
    }

    for (size_t i = 0; i < joins.length(); i++)
        slow.link(joins[i].from, fast.locationOf(joins[i].to));
}

void
StubCompiler::finalize(uint8 *ncode)
{
    masm.finalize(ncode);
}

JSC::MacroAssembler::Call
StubCompiler::vpInc(JSOp op, uint32 depth)
{
    uint32 slots = depth + script->nfixed;

    VoidVpStub stub = NULL;
    switch (op) {
      case JSOP_GLOBALINC:
      case JSOP_ARGINC:
      case JSOP_LOCALINC:
        stub = stubs::VpInc;
        break;

      case JSOP_GLOBALDEC:
      case JSOP_ARGDEC:
      case JSOP_LOCALDEC:
        stub = stubs::VpDec;
        break;

      case JSOP_INCGLOBAL:
      case JSOP_INCARG:
      case JSOP_INCLOCAL:
        stub = stubs::IncVp;
        break;

      case JSOP_DECGLOBAL:
      case JSOP_DECARG:
      case JSOP_DECLOCAL:
        stub = stubs::DecVp;
        break;

      default:
        JS_NOT_REACHED("unknown incdec op");
        break;
    }

    return stubCall(JS_FUNC_TO_DATA_PTR(void *, stub), slots);
}

void
StubCompiler::crossJump(Jump j, Label L)
{
    joins.append(CrossPatch(j, L));
}

void
StubCompiler::jumpInScript(Jump j, jsbytecode *target)
{
    if (cc.knownJump(target))
        crossJump(j, cc.labelOf(target));
    else
        scriptJoins.append(CrossJumpInScript(j, target));
}

