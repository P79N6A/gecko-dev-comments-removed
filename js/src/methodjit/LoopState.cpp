





































#include "methodjit/Compiler.h"
#include "methodjit/LoopState.h"
#include "methodjit/FrameState-inl.h"

using namespace js;
using namespace js::mjit;
using namespace js::analyze;

LoopState::LoopState(JSContext *cx, JSScript *script,
                     mjit::Compiler *cc, FrameState *frame,
                     Script *analysis, LifetimeScript *liveness)
    : cx(cx), script(script), cc(*cc), frame(*frame), analysis(analysis), liveness(liveness),
      lifetime(NULL), alloc(NULL), loopRegs(0), skipAnalysis(false),
      loopJoins(CompilerAllocPolicy(cx, *cc)),
      loopPatches(CompilerAllocPolicy(cx, *cc)),
      restoreInvariantCalls(CompilerAllocPolicy(cx, *cc)),
      hoistedBoundsChecks(CompilerAllocPolicy(cx, *cc)),
      invariantArraySlots(CompilerAllocPolicy(cx, *cc)),
      outer(NULL), PC(NULL)
{
    JS_ASSERT(cx->typeInferenceEnabled());
}

bool
LoopState::init(jsbytecode *head, Jump entry, jsbytecode *entryTarget)
{
    this->lifetime = liveness->getCode(head).loop;
    JS_ASSERT(lifetime &&
              lifetime->head == uint32(head - script->code) &&
              lifetime->entry == uint32(entryTarget - script->code));

    this->entry = entry;

    liveness->analyzeLoopTest(lifetime);
    if (!liveness->analyzeLoopIncrements(cx, lifetime))
        return false;
    if (!liveness->analyzeLoopModset(cx, lifetime))
        return false;

    if (lifetime->testLHS != LifetimeLoop::UNASSIGNED) {
        JaegerSpew(JSpew_Analysis, "loop test at %u: %s %s %s + %d\n", lifetime->head,
                   frame.entryName(lifetime->testLHS),
                   lifetime->testLessEqual ? "<=" : ">=",
                   (lifetime->testRHS == LifetimeLoop::UNASSIGNED)
                       ? ""
                       : frame.entryName(lifetime->testRHS),
                   lifetime->testConstant);
    }

    for (unsigned i = 0; i < lifetime->nIncrements; i++) {
        JaegerSpew(JSpew_Analysis, "loop increment at %u for %s: %u\n", lifetime->head,
                   frame.entryName(lifetime->increments[i].slot),
                   lifetime->increments[i].offset);
    }

    for (unsigned i = 0; i < lifetime->nGrowArrays; i++) {
        JaegerSpew(JSpew_Analysis, "loop grow array at %u: %s\n", lifetime->head,
                   lifetime->growArrays[i]->name());
    }

    RegisterAllocation *&alloc = liveness->getCode(head).allocation;
    JS_ASSERT(!alloc);

    alloc = ArenaNew<RegisterAllocation>(liveness->pool, true);
    if (!alloc)
        return false;

    this->alloc = alloc;
    this->loopRegs = Registers::AvailAnyRegs;
    this->PC = head;

    



    if (script->fun) {
        types::ObjectKind kind = types::TypeSet::GetObjectKind(cx, script->fun->getType());
        if (kind != types::OBJECT_INLINEABLE_FUNCTION && kind != types::OBJECT_SCRIPTED_FUNCTION)
            this->skipAnalysis = true;
    }

    





    if (lifetime->hasSafePoints)
        this->skipAnalysis = true;

    



    if (lifetime->hasCallsLoops)
        this->skipAnalysis = true;

    return true;
}

void
LoopState::addJoin(unsigned index, bool script)
{
    StubJoin r;
    r.index = index;
    r.script = script;
    loopJoins.append(r);
}

void
LoopState::addInvariantCall(Jump jump, Label label, bool ool)
{
    RestoreInvariantCall call;
    call.jump = jump;
    call.label = label;
    call.ool = ool;
    restoreInvariantCalls.append(call);
}

void
LoopState::flushLoop(StubCompiler &stubcc)
{
    clearLoopRegisters();

    



    for (unsigned i = 0; i < loopPatches.length(); i++) {
        const StubJoinPatch &p = loopPatches[i];
        stubcc.patchJoin(p.join.index, p.join.script, p.address, p.reg);
    }
    loopJoins.clear();
    loopPatches.clear();

    if (hasInvariants()) {
        for (unsigned i = 0; i < restoreInvariantCalls.length(); i++) {
            RestoreInvariantCall &call = restoreInvariantCalls[i];
            Assembler &masm = cc.getAssembler(true);
            if (call.ool) {
                call.jump.linkTo(masm.label(), &masm);
                restoreInvariants(masm);
                masm.jump().linkTo(call.label, &masm);
            } else {
                stubcc.linkExitDirect(call.jump, masm.label());
                restoreInvariants(masm);
                stubcc.crossJump(masm.jump(), call.label);
            }
        }
    } else {
        for (unsigned i = 0; i < restoreInvariantCalls.length(); i++) {
            RestoreInvariantCall &call = restoreInvariantCalls[i];
            Assembler &masm = cc.getAssembler(call.ool);
            call.jump.linkTo(call.label, &masm);
        }
    }
    restoreInvariantCalls.clear();
}

void
LoopState::clearLoopRegisters()
{
    alloc->clearLoops();
    loopRegs = 0;
}

bool
LoopState::loopInvariantEntry(const FrameEntry *fe)
{
    uint32 slot = frame.indexOfFe(fe);
    unsigned nargs = script->fun ? script->fun->nargs : 0;

    if (slot >= 2 + nargs + script->nfixed)
        return false;

    if (liveness->firstWrite(slot, lifetime) != uint32(-1))
        return false;

    if (slot == 0) 
        return false;
    if (slot == 1) 
        return true;
    slot -= 2;

    if (slot < nargs && !analysis->argEscapes(slot))
        return true;
    if (script->fun)
        slot -= script->fun->nargs;

    return !analysis->localEscapes(slot);
}

bool
LoopState::addHoistedCheck(uint32 arraySlot, uint32 valueSlot, int32 constant)
{
    



    for (unsigned i = 0; i < hoistedBoundsChecks.length(); i++) {
        HoistedBoundsCheck &check = hoistedBoundsChecks[i];
        if (check.arraySlot == arraySlot && check.valueSlot == valueSlot) {
            if (check.constant < constant)
                check.constant = constant;
            return true;
        }
    }

    





    bool hasInvariantSlots = false;
    for (unsigned i = 0; !hasInvariantSlots && i < invariantArraySlots.length(); i++) {
        if (invariantArraySlots[i].arraySlot == arraySlot)
            hasInvariantSlots = true;
    }
    if (!hasInvariantSlots) {
        uint32 which = frame.allocTemporary();
        if (which == uint32(-1))
            return false;
        FrameEntry *fe = frame.getTemporary(which);

        JaegerSpew(JSpew_Analysis, "Using %s for loop invariant slots of %s\n",
                   frame.entryName(fe), frame.entryName(arraySlot));

        InvariantArraySlots slots;
        slots.arraySlot = arraySlot;
        slots.temporary = which;
        invariantArraySlots.append(slots);
    }

    HoistedBoundsCheck check;
    check.arraySlot = arraySlot;
    check.valueSlot = valueSlot;
    check.constant = constant;
    hoistedBoundsChecks.append(check);

    return true;
}

void
LoopState::setLoopReg(AnyRegisterID reg, FrameEntry *fe)
{
    JS_ASSERT(alloc->loop(reg));
    loopRegs.takeReg(reg);

    uint32 slot = frame.indexOfFe(fe);
    JaegerSpew(JSpew_Regalloc, "allocating loop register %s for %s\n",
               reg.name(), frame.entryName(fe));

    alloc->set(reg, slot, true);

    



    for (unsigned i = 0; i < loopJoins.length(); i++) {
        StubJoinPatch p;
        p.join = loopJoins[i];
        p.address = frame.addressOf(fe);
        p.reg = reg;
        loopPatches.append(p);
    }

    if (lifetime->entry != lifetime->head && PC >= script->code + lifetime->entry) {
        




        RegisterAllocation *entry = liveness->getCode(lifetime->entry).allocation;
        JS_ASSERT(entry && !entry->assigned(reg));
        entry->set(reg, slot, true);
    }
}

bool
LoopState::hoistArrayLengthCheck(const FrameEntry *obj, const FrameEntry *index)
{
    if (skipAnalysis || script->failedBoundsCheck)
        return false;

    





    obj = obj->backing();
    index = index->backing();

    JaegerSpew(JSpew_Analysis, "Trying to hoist bounds check array %s index %s\n",
               frame.entryName(obj), frame.entryName(index));

    if (!loopInvariantEntry(obj)) {
        JaegerSpew(JSpew_Analysis, "Object is not loop invariant\n");
        return false;
    }

    types::TypeSet *objTypes = cc.getTypeSet(obj);
    JS_ASSERT(objTypes && !objTypes->unknown());

    
    if (objTypes->getKnownTypeTag(cx) != JSVAL_TYPE_OBJECT) {
        JaegerSpew(JSpew_Analysis, "Object might be a primitive\n");
        return false;
    }

    





    if (lifetime->nGrowArrays) {
        types::TypeObject **growArrays = lifetime->growArrays;
        unsigned count = objTypes->getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            types::TypeObject *object = objTypes->getObject(i);
            if (object) {
                for (unsigned j = 0; j < lifetime->nGrowArrays; j++) {
                    if (object == growArrays[j]) {
                        JaegerSpew(JSpew_Analysis, "Object might grow inside loop\n");
                        return false;
                    }
                }
            }
        }
    }

    if (index->isConstant()) {
        
        int32 value = index->getValue().toInt32();
        JaegerSpew(JSpew_Analysis, "Hoisted as initlen > %d\n", value);

        return addHoistedCheck(frame.indexOfFe(obj), uint32(-1), value);
    }

    if (loopInvariantEntry(index)) {
        
        JaegerSpew(JSpew_Analysis, "Hoisted as initlen > %s\n", frame.entryName(index));

        return addHoistedCheck(frame.indexOfFe(obj), frame.indexOfFe(index), 0);
    }

    if (frame.indexOfFe(index) == lifetime->testLHS && lifetime->testLessEqual) {
        




        uint32 rhs = lifetime->testRHS;
        int32 constant = lifetime->testConstant;

        



        if (!liveness->nonDecreasing(lifetime->testLHS, lifetime)) {
            JaegerSpew(JSpew_Analysis, "Index may decrease in future iterations\n");
            return false;
        }

        uint32 write = liveness->firstWrite(lifetime->testLHS, lifetime);
        JS_ASSERT(write != LifetimeLoop::UNASSIGNED);
        if (write < uint32(PC - script->code)) {
            JaegerSpew(JSpew_Analysis, "Index previously modified in loop\n");
            return false;
        }

        if (rhs != LifetimeLoop::UNASSIGNED) {
            




            types::TypeSet *types = cc.getTypeSet(rhs);
            if (types->getKnownTypeTag(cx) != JSVAL_TYPE_INT32) {
                JaegerSpew(JSpew_Analysis, "Branch test may not be on integer\n");
                return false;
            }
        }

        JaegerSpew(JSpew_Analysis, "Hoisted as initlen > %s + %d\n",
                   (rhs == LifetimeLoop::UNASSIGNED) ? "" : frame.entryName(rhs),
                   constant);

        return addHoistedCheck(frame.indexOfFe(obj), rhs, constant);
    }

    JaegerSpew(JSpew_Analysis, "No match found\n");
    return false;
}

bool
LoopState::checkHoistedBounds(jsbytecode *PC, Assembler &masm, Vector<Jump> *jumps)
{
    restoreInvariants(masm);

    






    for (unsigned i = 0; i < hoistedBoundsChecks.length(); i++) {
        
        const HoistedBoundsCheck &check = hoistedBoundsChecks[i];

        RegisterID initlen = Registers::ArgReg0;
        masm.loadPayload(frame.addressOf(check.arraySlot), initlen);
        masm.load32(Address(initlen, offsetof(JSObject, initializedLength)), initlen);

        if (check.valueSlot != uint32(-1)) {
            RegisterID value = Registers::ArgReg1;
            masm.loadPayload(frame.addressOf(check.valueSlot), value);
            if (check.constant != 0) {
                Jump overflow = masm.branchAdd32(Assembler::Overflow,
                                                 Imm32(check.constant), value);
                if (!jumps->append(overflow))
                    return false;
            }
            Jump j = masm.branch32(Assembler::BelowOrEqual, initlen, value);
            if (!jumps->append(j))
                return false;
        } else {
            Jump j = masm.branch32(Assembler::BelowOrEqual, initlen, Imm32(check.constant));
            if (!jumps->append(j))
                return false;
        }
    }

    return true;
}

FrameEntry *
LoopState::invariantSlots(const FrameEntry *obj)
{
    obj = obj->backing();
    uint32 slot = frame.indexOfFe(obj);

    for (unsigned i = 0; i < invariantArraySlots.length(); i++) {
        if (invariantArraySlots[i].arraySlot == slot)
            return frame.getTemporary(invariantArraySlots[i].temporary);
    }

    
    JS_NOT_REACHED("Missing invariant slots");
    return NULL;
}

void
LoopState::restoreInvariants(Assembler &masm)
{
    





    Registers regs(Registers::AvailRegs);
    regs.takeReg(Registers::ReturnReg);

    for (unsigned i = 0; i < invariantArraySlots.length(); i++) {
        const InvariantArraySlots &entry = invariantArraySlots[i];
        FrameEntry *fe = frame.getTemporary(entry.temporary);

        Address array = frame.addressOf(entry.arraySlot);
        Address address = frame.addressOf(fe);

        RegisterID reg = regs.takeAnyReg().reg();
        masm.loadPayload(array, reg);
        masm.loadPtr(Address(reg, JSObject::offsetOfSlots()), reg);
        masm.storePtr(reg, address);
        regs.putReg(reg);
    }
}
