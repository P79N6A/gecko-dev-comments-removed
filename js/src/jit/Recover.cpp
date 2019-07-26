





#include "jit/Recover.h"

#include "jit/IonSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGraph.h"

using namespace js;
using namespace js::jit;

bool
MResumePoint::writeRecoverData(CompactBufferWriter &writer) const
{
    writer.writeUnsigned(uint32_t(Recover_ResumePoint));

    MBasicBlock *bb = block();
    JSFunction *fun = bb->info().funMaybeLazy();
    JSScript *script = bb->info().script();
    uint32_t exprStack = stackDepth() - bb->info().ninvoke();

#ifdef DEBUG
    
    
    if (GetIonContext()->cx) {
        uint32_t stackDepth;
        bool reachablePC;
        jsbytecode *bailPC = pc();

        if (mode() == MResumePoint::ResumeAfter)
            bailPC = GetNextPc(pc());

        if (!ReconstructStackDepth(GetIonContext()->cx, script,
                                   bailPC, &stackDepth, &reachablePC))
        {
            return false;
        }

        if (reachablePC) {
            if (JSOp(*bailPC) == JSOP_FUNCALL) {
                
                
                
                MOZ_ASSERT(stackDepth - exprStack <= 1);
            } else if (JSOp(*bailPC) != JSOP_FUNAPPLY &&
                       !IsGetPropPC(bailPC) && !IsSetPropPC(bailPC))
            {
                
                
                
                

                
                
                
                
                
                MOZ_ASSERT(exprStack == stackDepth);
            }
        }
    }
#endif

    
    
    
    
    MOZ_ASSERT(CountArgSlots(script, fun) < SNAPSHOT_MAX_NARGS + 4);

    uint32_t implicit = StartArgSlot(script);
    uint32_t formalArgs = CountArgSlots(script, fun);
    uint32_t nallocs = formalArgs + script->nfixed() + exprStack;

    IonSpew(IonSpew_Snapshots, "Starting frame; implicit %u, formals %u, fixed %u, exprs %u",
            implicit, formalArgs - implicit, script->nfixed(), exprStack);

    uint32_t pcoff = script->pcToOffset(pc());
    IonSpew(IonSpew_Snapshots, "Writing pc offset %u, nslots %u", pcoff, nallocs);
    writer.writeUnsigned(pcoff);
    writer.writeUnsigned(nallocs);
    return true;
}

RResumePoint::RResumePoint(CompactBufferReader &reader)
{
    static_assert(sizeof(*this) <= sizeof(RInstructionStorage),
                  "Storage space is too small to decode this recover instruction.");
    pcOffset_ = reader.readUnsigned();
    numOperands_ = reader.readUnsigned();
    IonSpew(IonSpew_Snapshots, "Read RResumePoint (pc offset %u, nslots %u)",
            pcOffset_, numOperands_);
}

void
RResumePoint::readRecoverData(CompactBufferReader &reader, RInstructionStorage *raw)
{
    mozilla::DebugOnly<uint32_t> op = reader.readUnsigned();
    MOZ_ASSERT(op == uint32_t(Recover_ResumePoint));
    new (raw->addr()) RResumePoint(reader);
}
