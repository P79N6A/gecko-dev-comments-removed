





































#include "methodjit/Compiler.h"
#include "methodjit/LoopState.h"
#include "methodjit/FrameState-inl.h"
#include "methodjit/StubCalls.h"

using namespace js;
using namespace js::mjit;
using namespace js::analyze;

inline bool
SafeAdd(int32 one, int32 two, int32 *res)
{
    *res = one + two;
    int64 ores = (int64)one + (int64)two;
    if (ores == (int64)*res)
        return true;
    JaegerSpew(JSpew_Analysis, "Overflow computing %d + %d\n", one, two);
    return false;
}

inline bool
SafeSub(int32 one, int32 two, int32 *res)
{
    *res = one - two;
    int64 ores = (int64)one - (int64)two;
    if (ores == (int64)*res)
        return true;
    JaegerSpew(JSpew_Analysis, "Overflow computing %d - %d\n", one, two);
    return false;
}

inline bool
SafeMul(int32 one, int32 two, int32 *res)
{
    *res = one * two;
    int64 ores = (int64)one * (int64)two;
    if (ores == (int64)*res)
        return true;
    JaegerSpew(JSpew_Analysis, "Overflow computing %d * %d\n", one, two);
    return false;
}

LoopState::LoopState(JSContext *cx, JSScript *script,
                     mjit::Compiler *cc, FrameState *frame)
    : cx(cx), script(script), analysis(script->analysis(cx)), cc(*cc), frame(*frame),
      lifetime(NULL), alloc(NULL), loopRegs(0), skipAnalysis(false),
      loopJoins(CompilerAllocPolicy(cx, *cc)),
      loopPatches(CompilerAllocPolicy(cx, *cc)),
      restoreInvariantCalls(CompilerAllocPolicy(cx, *cc)),
      invariantEntries(CompilerAllocPolicy(cx, *cc)),
      outer(NULL), PC(NULL),
      testLHS(UNASSIGNED), testRHS(UNASSIGNED),
      testConstant(0), testLessEqual(false), testLength(false),
      increments(CompilerAllocPolicy(cx, *cc)), unknownModset(false),
      growArrays(CompilerAllocPolicy(cx, *cc)),
      modifiedProperties(CompilerAllocPolicy(cx, *cc)),
      constrainedLoop(true)
{
    JS_ASSERT(cx->typeInferenceEnabled());
}

bool
LoopState::init(jsbytecode *head, Jump entry, jsbytecode *entryTarget)
{
    this->lifetime = analysis->getLoop(head);
    JS_ASSERT(lifetime &&
              lifetime->head == uint32(head - script->code) &&
              lifetime->entry == uint32(entryTarget - script->code));

    this->entry = entry;

    analyzeLoopTest();
    analyzeLoopIncrements();
    analyzeLoopBody();

    if (testLHS != UNASSIGNED) {
        JaegerSpew(JSpew_Analysis, "loop test at %u: %s %s%s %s + %d\n", lifetime->head,
                   frame.entryName(testLHS),
                   testLessEqual ? "<=" : ">=",
                   testLength ? " length" : "",
                   (testRHS == UNASSIGNED) ? "" : frame.entryName(testRHS),
                   testConstant);
    }

    for (unsigned i = 0; i < increments.length(); i++) {
        JaegerSpew(JSpew_Analysis, "loop increment at %u for %s: %u\n", lifetime->head,
                   frame.entryName(increments[i].slot),
                   increments[i].offset);
    }

    for (unsigned i = 0; i < growArrays.length(); i++) {
        JaegerSpew(JSpew_Analysis, "loop grow array at %u: %s\n", lifetime->head,
                   growArrays[i]->name());
    }

    for (unsigned i = 0; i < modifiedProperties.length(); i++) {
        JaegerSpew(JSpew_Analysis, "loop modified property at %u: %s %s\n", lifetime->head,
                   modifiedProperties[i].object->name(),
                   types::TypeIdString(modifiedProperties[i].id));
    }

    RegisterAllocation *&alloc = analysis->getAllocation(head);
    JS_ASSERT(!alloc);

    alloc = ArenaNew<RegisterAllocation>(cx->compartment->pool, true);
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
LoopState::addInvariantCall(Jump jump, Label label, bool ool, unsigned patchIndex, bool patchCall)
{
    RestoreInvariantCall call;
    call.jump = jump;
    call.label = label;
    call.ool = ool;
    call.patchIndex = patchIndex;
    call.patchCall = patchCall;
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
            Vector<Jump> failureJumps(cx);

            jsbytecode *pc = cc.getInvariantPC(call.patchIndex, call.patchCall);

            if (call.ool) {
                call.jump.linkTo(masm.label(), &masm);
                restoreInvariants(pc, masm, &failureJumps);
                masm.jump().linkTo(call.label, &masm);
            } else {
                stubcc.linkExitDirect(call.jump, masm.label());
                restoreInvariants(pc, masm, &failureJumps);
                stubcc.crossJump(masm.jump(), call.label);
            }

            if (!failureJumps.empty()) {
                for (unsigned i = 0; i < failureJumps.length(); i++)
                    failureJumps[i].linkTo(masm.label(), &masm);

                



                InvariantCodePatch *patch = cc.getInvariantPatch(call.patchIndex, call.patchCall);
                patch->hasPatch = true;
                patch->codePatch = masm.storePtrWithPatch(ImmPtr(NULL),
                                                          FrameAddress(offsetof(VMFrame, scratch)));
                JS_STATIC_ASSERT(Registers::ReturnReg != Registers::ArgReg1);
                masm.move(Registers::ReturnReg, Registers::ArgReg1);
                masm.fallibleVMCall(true, JS_FUNC_TO_DATA_PTR(void *, stubs::InvariantFailure),
                                    pc, NULL, 0);
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
LoopState::loopInvariantEntry(uint32 slot)
{
    if (slot == analyze::CalleeSlot() || analysis->slotEscapes(slot))
        return false;
    return analysis->liveness(slot).firstWrite(lifetime) == uint32(-1);
}

inline bool
LoopState::entryRedundant(const InvariantEntry &e0, const InvariantEntry &e1)
{
    JS_ASSERT(e0.isCheck() && e1.isCheck());

    uint32 array0 = e0.u.check.arraySlot;
    uint32 array1 = e1.u.check.arraySlot;

    uint32 value01 = e0.u.check.valueSlot1;
    uint32 value02 = e0.u.check.valueSlot2;

    uint32 value11 = e1.u.check.valueSlot1;
    uint32 value12 = e1.u.check.valueSlot2;

    int32 c0 = e0.u.check.constant;
    int32 c1 = e1.u.check.constant;

    








    if (e0.kind == InvariantEntry::RANGE_CHECK && e1.kind == InvariantEntry::BOUNDS_CHECK &&
        value01 == value11 && value02 == value12) {
        int32 constant;
        if (c1 >= 0)
            constant = c0;
        else if (!SafeAdd(c0, c1, &constant))
            return false;
        return constant >= JSObject::NSLOTS_LIMIT;
    }

    
    if (e0.kind == e1.kind && array0 == array1 && value01 == value11 && value02 == value12) {
        if (e0.kind == InvariantEntry::BOUNDS_CHECK) {
            
            return (c0 <= c1);
        } else {
            
            return (c0 >= c1);
        }
    }

    return false;
}

bool
LoopState::checkRedundantEntry(const InvariantEntry &entry)
{
    



    JS_ASSERT(entry.isCheck());
    for (unsigned i = 0; i < invariantEntries.length(); i++) {
        InvariantEntry &baseEntry = invariantEntries[i];
        if (!baseEntry.isCheck())
            continue;
        if (entryRedundant(entry, baseEntry))
            return true;
        if (entryRedundant(baseEntry, entry)) {
            invariantEntries[i--] = invariantEntries.back();
            invariantEntries.popBack();
        }
    }
    return false;
}

bool
LoopState::addHoistedCheck(uint32 arraySlot, uint32 valueSlot1, uint32 valueSlot2, int32 constant)
{
#ifdef DEBUG
    JS_ASSERT_IF(valueSlot1 == UNASSIGNED, valueSlot2 == UNASSIGNED);
    if (valueSlot1 == UNASSIGNED) {
        JaegerSpew(JSpew_Analysis, "Hoist initlen > %d\n", constant);
    } else if (valueSlot2 == UNASSIGNED) {
        JaegerSpew(JSpew_Analysis, "Hoisted as initlen > %s + %d\n",
                   frame.entryName(valueSlot1), constant);
    } else {
        JaegerSpew(JSpew_Analysis, "Hoisted as initlen > %s + %s + %d\n",
                   frame.entryName(valueSlot1), frame.entryName(valueSlot2), constant);
    }
#endif

    InvariantEntry entry;
    entry.kind = InvariantEntry::BOUNDS_CHECK;
    entry.u.check.arraySlot = arraySlot;
    entry.u.check.valueSlot1 = valueSlot1;
    entry.u.check.valueSlot2 = valueSlot2;
    entry.u.check.constant = constant;

    if (checkRedundantEntry(entry))
        return true;

    





    bool hasInvariantSlots = false;
    for (unsigned i = 0; !hasInvariantSlots && i < invariantEntries.length(); i++) {
        InvariantEntry &entry = invariantEntries[i];
        if (entry.kind == InvariantEntry::INVARIANT_SLOTS &&
            entry.u.array.arraySlot == arraySlot) {
            hasInvariantSlots = true;
        }
    }
    if (!hasInvariantSlots) {
        uint32 which = frame.allocTemporary();
        if (which == uint32(-1))
            return false;
        FrameEntry *fe = frame.getTemporary(which);

        JaegerSpew(JSpew_Analysis, "Using %s for loop invariant slots of %s\n",
                   frame.entryName(fe), frame.entryName(arraySlot));

        InvariantEntry slotsEntry;
        slotsEntry.kind = InvariantEntry::INVARIANT_SLOTS;
        slotsEntry.u.array.arraySlot = arraySlot;
        slotsEntry.u.array.temporary = which;
        invariantEntries.append(slotsEntry);
    }

    invariantEntries.append(entry);
    return true;
}

void
LoopState::addNegativeCheck(uint32 valueSlot, int32 constant)
{
    JaegerSpew(JSpew_Analysis, "Nonnegative check %s + %d >= 0\n",
               frame.entryName(valueSlot), constant);

    InvariantEntry entry;
    entry.kind = InvariantEntry::NEGATIVE_CHECK;
    entry.u.check.valueSlot1 = valueSlot;
    entry.u.check.constant = constant;

    if (!checkRedundantEntry(entry))
        invariantEntries.append(entry);
}

void
LoopState::addRangeCheck(uint32 valueSlot1, uint32 valueSlot2, int32 constant)
{
    JaegerSpew(JSpew_Analysis, "Range check %d >= %s + %s\n",
               constant, frame.entryName(valueSlot1),
               valueSlot2 == uint32(-1) ? "" : frame.entryName(valueSlot2));

    InvariantEntry entry;
    entry.kind = InvariantEntry::RANGE_CHECK;
    entry.u.check.valueSlot1 = valueSlot1;
    entry.u.check.valueSlot2 = valueSlot2;
    entry.u.check.constant = constant;

    if (!checkRedundantEntry(entry))
        invariantEntries.append(entry);
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
        




        RegisterAllocation *entry = analysis->getAllocation(lifetime->entry);
        JS_ASSERT(entry && !entry->assigned(reg));
        entry->set(reg, slot, true);
    }
}

bool
LoopState::hoistArrayLengthCheck(const FrameEntry *obj, types::TypeSet *objTypes,
                                 unsigned indexPopped)
{
    if (skipAnalysis || script->failedBoundsCheck)
        return false;

    obj = obj->backing();

    JaegerSpew(JSpew_Analysis, "Trying to hoist bounds check on %s\n",
               frame.entryName(obj));

    if (!loopInvariantEntry(frame.indexOfFe(obj))) {
        JaegerSpew(JSpew_Analysis, "Object is not loop invariant\n");
        return false;
    }

    





    JS_ASSERT(objTypes && !objTypes->unknown());
    if (!growArrays.empty()) {
        unsigned count = objTypes->getObjectCount();
        for (unsigned i = 0; i < count; i++) {
            types::TypeObject *object = objTypes->getObject(i);
            if (object) {
                for (unsigned j = 0; j < growArrays.length(); j++) {
                    if (object == growArrays[j]) {
                        JaegerSpew(JSpew_Analysis, "Object might grow inside loop\n");
                        return false;
                    }
                }
            }
        }
    }

    



    uint32 index;
    int32 indexConstant;
    if (!getEntryValue(analysis->poppedValue(PC - script->code, indexPopped), &index, &indexConstant)) {
        JaegerSpew(JSpew_Analysis, "Could not compute index in terms of loop entry state\n");
        return false;
    }

    if (index == UNASSIGNED) {
        
        return addHoistedCheck(frame.indexOfFe(obj), UNASSIGNED, UNASSIGNED, indexConstant);
    }

    if (loopInvariantEntry(index)) {
        
        return addHoistedCheck(frame.indexOfFe(obj), index, UNASSIGNED, indexConstant);
    }

    




    if (!analysis->liveness(index).nonDecreasing(script, lifetime)) {
        JaegerSpew(JSpew_Analysis, "Index may decrease in future iterations\n");
        return false;
    }

    








    if (index == testLHS && testLessEqual) {
        uint32 rhs = testRHS;

        if (testLength) {
            FrameEntry *rhsFE = frame.getOrTrack(rhs);
            FrameEntry *lengthEntry = invariantLength(rhsFE, NULL);

            



            JS_ASSERT(lengthEntry);

            rhs = frame.indexOfFe(lengthEntry);
        }

        int32 constant;
        if (!SafeAdd(testConstant, indexConstant, &constant))
            return false;

        






        addNegativeCheck(index, indexConstant);

        return addHoistedCheck(frame.indexOfFe(obj), rhs, UNASSIGNED, constant);
    }

    










    if (hasTestLinearRelationship(index)) {
        int32 constant;
        if (!SafeSub(indexConstant, testConstant, &constant))
            return false;

        addNegativeCheck(index, indexConstant);
        return addHoistedCheck(frame.indexOfFe(obj), index, testLHS, constant);
    }

    JaegerSpew(JSpew_Analysis, "No match found\n");
    return false;
}

bool
LoopState::hasTestLinearRelationship(uint32 slot)
{
    





    if (testLHS == UNASSIGNED || testRHS != UNASSIGNED || testLessEqual || testLength)
        return false;

    uint32 incrementOffset = getIncrement(slot);
    if (incrementOffset == uint32(-1)) {
        




        return false;
    }

    uint32 decrementOffset = getIncrement(testLHS);
    if (decrementOffset == uint32(-1))
        return false;

    JSOp op = JSOp(script->code[decrementOffset]);
    switch (op) {
      case JSOP_DECLOCAL:
      case JSOP_LOCALDEC:
      case JSOP_DECARG:
      case JSOP_ARGDEC:
        return true;
      default:
        return false;
    }
}

FrameEntry *
LoopState::invariantSlots(const FrameEntry *obj)
{
    obj = obj->backing();
    uint32 slot = frame.indexOfFe(obj);

    for (unsigned i = 0; i < invariantEntries.length(); i++) {
        InvariantEntry &entry = invariantEntries[i];
        if (entry.kind == InvariantEntry::INVARIANT_SLOTS &&
            entry.u.array.arraySlot == slot) {
            return frame.getTemporary(entry.u.array.temporary);
        }
    }

    
    JS_NOT_REACHED("Missing invariant slots");
    return NULL;
}

FrameEntry *
LoopState::invariantLength(const FrameEntry *obj, types::TypeSet *objTypes)
{
    if (skipAnalysis || script->failedBoundsCheck)
        return NULL;

    obj = obj->backing();
    uint32 slot = frame.indexOfFe(obj);

    for (unsigned i = 0; i < invariantEntries.length(); i++) {
        InvariantEntry &entry = invariantEntries[i];
        if (entry.kind == InvariantEntry::INVARIANT_LENGTH &&
            entry.u.array.arraySlot == slot) {
            FrameEntry *fe = frame.getTemporary(entry.u.array.temporary);
            frame.learnType(fe, JSVAL_TYPE_INT32, false);
            return fe;
        }
    }

    if (!objTypes)
        return NULL;

    if (!loopInvariantEntry(frame.indexOfFe(obj)))
        return NULL;

    types::ObjectKind kind = objTypes->getKnownObjectKind(cx);
    if (kind != types::OBJECT_DENSE_ARRAY && kind != types::OBJECT_PACKED_ARRAY)
        return NULL;

    




    for (unsigned i = 0; i < objTypes->getObjectCount(); i++) {
        types::TypeObject *object = objTypes->getObject(i);
        if (!object)
            continue;
        if (object->unknownProperties() || hasModifiedProperty(object, JSID_VOID))
            return NULL;
    }
    objTypes->addFreeze(cx);

    uint32 which = frame.allocTemporary();
    if (which == uint32(-1))
        return NULL;
    FrameEntry *fe = frame.getTemporary(which);
    frame.learnType(fe, JSVAL_TYPE_INT32, false);

    JaegerSpew(JSpew_Analysis, "Using %s for loop invariant length of %s\n",
               frame.entryName(fe), frame.entryName(slot));

    InvariantEntry entry;
    entry.kind = InvariantEntry::INVARIANT_LENGTH;
    entry.u.array.arraySlot = slot;
    entry.u.array.temporary = which;
    invariantEntries.append(entry);

    return fe;
}

bool
LoopState::cannotIntegerOverflow()
{
    if (skipAnalysis || script->failedBoundsCheck)
        return false;

    
    SSAValue pushed;
    pushed.initPushed(PC - script->code, 0);

    int32 min, max;
    if (computeInterval(pushed, &min, &max)) {
        JaegerSpew(JSpew_Analysis, "Integer operation fits in range [%d, %d]\n", min, max);
        return true;
    }

    




    uint32 baseSlot = UNASSIGNED;
    int32 baseConstant = 0;
    JSOp op = JSOp(*PC);
    switch (op) {

      case JSOP_INCLOCAL:
      case JSOP_LOCALINC:
      case JSOP_INCARG:
      case JSOP_ARGINC:
        if (!getEntryValue(analysis->poppedValue(PC - script->code, 0), &baseSlot, &baseConstant))
            return false;
        if (!SafeAdd(baseConstant, 1, &baseConstant))
            return false;
        break;

      case JSOP_DECLOCAL:
      case JSOP_LOCALDEC:
      case JSOP_DECARG:
      case JSOP_ARGDEC:
          if (!getEntryValue(analysis->poppedValue(PC - script->code, 0), &baseSlot, &baseConstant))
            return false;
        if (!SafeSub(baseConstant, 1, &baseConstant))
            return false;
        break;

      case JSOP_ADD:
      case JSOP_SUB: {
        uint32 lhs = UNASSIGNED, rhs = UNASSIGNED;
        int32 lhsconstant = 0, rhsconstant = 0;
        if (!getEntryValue(analysis->poppedValue(PC - script->code, 1), &lhs, &lhsconstant))
            return false;
        if (!getEntryValue(analysis->poppedValue(PC - script->code, 0), &rhs, &rhsconstant))
            return false;
        if (op == JSOP_ADD) {
            if (!SafeAdd(lhsconstant, rhsconstant, &baseConstant))
                return false;
            if (lhs != UNASSIGNED && rhs != UNASSIGNED)
                return false;
            baseSlot = (lhs == UNASSIGNED) ? rhs : lhs;
        } else {
            if (!SafeSub(lhsconstant, rhsconstant, &baseConstant))
                return false;
            if (rhs != UNASSIGNED)
                return false;
            baseSlot = lhs;
        }
        break;
      }

      default:
        return false;
    }

    if (baseSlot == UNASSIGNED)
        return false;

    JaegerSpew(JSpew_Analysis, "Trying to hoist integer overflow check on %s + %d\n",
               frame.entryName(baseSlot), baseConstant);

    if (baseConstant == 0) {
        JaegerSpew(JSpew_Analysis, "Vacuously holds\n");
        return true;
    }

    if (baseConstant < 0) {
        






        if (baseSlot == testLHS && !testLessEqual && !testLength && testRHS == UNASSIGNED) {
            int32 constant;
            if (!SafeAdd(testConstant, baseConstant, &constant))
                return false;

            JaegerSpew(JSpew_Analysis, "Loop test comparison must hold\n");
            return true;
        }

        JaegerSpew(JSpew_Analysis, "No match found\n");
        return false;
    }

    








    if (baseSlot == testLHS && testLessEqual && !testLength) {
        int32 constant;
        if (!SafeAdd(testConstant, baseConstant, &constant))
            return false;

        if (testRHS == UNASSIGNED || constant <= 0) {
            



            JaegerSpew(JSpew_Analysis, "Loop test comparison must hold\n");
            return true;
        }

        constant = JSVAL_INT_MAX - constant;

        addRangeCheck(testRHS, UNASSIGNED, constant);
        return true;
    }

    









    if (hasTestLinearRelationship(baseSlot)) {
        int32 constant;
        if (!SafeSub(testConstant, baseConstant, &constant))
            return false;

        if (constant >= 0)
            constant = 0;
        constant = JSVAL_INT_MAX + constant;

        addRangeCheck(baseSlot, testLHS, constant);
        return true;
    }

    JaegerSpew(JSpew_Analysis, "No match found\n");
    return false;
}

bool
LoopState::ignoreIntegerOverflow()
{
    if (skipAnalysis || script->failedBoundsCheck || !constrainedLoop)
        return false;

    























    JSOp op = JSOp(*PC);
    if (op != JSOP_MUL && op != JSOP_ADD)
        return false;

    SSAValue v;
    v.initPushed(PC - script->code, 0);
    if (valueFlowsToBitops(v)) {
        JaegerSpew(JSpew_Analysis, "Integer result flows to bitops\n");
        return true;
    }

    if (op == JSOP_MUL) {
        




        if (!analysis->trackUseChain(v))
            return false;

        SSAUseChain *use = analysis->useChain(v);
        if (!use || use->next || !use->popped || script->code[use->offset] != JSOP_ADD)
            return false;

        if (use->u.which == 1) {
            





            return false;
        }

        types::TypeSet *lhsTypes = analysis->poppedTypes(use->offset, 1);
        if (lhsTypes->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
            return false;

        JaegerSpew(JSpew_Analysis, "Integer result is RHS in integer addition\n");
        return true;
    }

    return false;
}

bool
LoopState::valueFlowsToBitops(const analyze::SSAValue &v)
{
    




    if (!analysis->trackUseChain(v))
        return false;

    SSAUseChain *use = analysis->useChain(v);
    while (use) {
        if (!use->popped) {
            




            if (v.kind() == SSAValue::VAR) {
                analyze::Lifetime *lifetime = analysis->liveness(v.varSlot()).live(use->offset);
                if (!lifetime) {
                    use = use->next;
                    continue;
                }
            }
            return false;
        }

        if (use->offset > lifetime->backedge)
            return false;

        jsbytecode *pc = script->code + use->offset;
        JSOp op = JSOp(*pc);
        switch (op) {
          case JSOP_ADD:
          case JSOP_GETLOCAL: {
            SSAValue pushv;
            pushv.initPushed(use->offset, 0);
            if (!valueFlowsToBitops(pushv))
                return false;
            break;
          }

          case JSOP_SETLOCAL: {
            uint32 slot = GetBytecodeSlot(script, pc);
            if (!analysis->trackSlot(slot))
                return false;
            SSAValue writev;
            writev.initWritten(slot, use->offset);
            if (!valueFlowsToBitops(writev))
                return false;
            break;
          }

          case JSOP_BITAND:
          case JSOP_BITOR:
          case JSOP_BITXOR:
          case JSOP_RSH:
          case JSOP_LSH:
          case JSOP_URSH:
          case JSOP_BITNOT:
            break;

          default:
            return false;
        }

        use = use->next;
    }

    return true;
}

void
LoopState::restoreInvariants(jsbytecode *pc, Assembler &masm, Vector<Jump> *jumps)
{
    






    Registers regs(Registers::TempRegs);
    regs.takeReg(Registers::ReturnReg);

    RegisterID T0 = regs.takeAnyReg().reg();
    RegisterID T1 = regs.takeAnyReg().reg();

    for (unsigned i = 0; i < invariantEntries.length(); i++) {
        const InvariantEntry &entry = invariantEntries[i];
        switch (entry.kind) {

          case InvariantEntry::BOUNDS_CHECK: {
            



            masm.loadPayload(frame.addressOf(entry.u.check.arraySlot), T0);
            masm.load32(Address(T0, offsetof(JSObject, initializedLength)), T0);

            int32 constant = entry.u.check.constant;

            if (entry.u.check.valueSlot1 != uint32(-1)) {
                constant += adjustConstantForIncrement(pc, entry.u.check.valueSlot1);
                masm.loadPayload(frame.addressOf(entry.u.check.valueSlot1), T1);
                if (entry.u.check.valueSlot2 != uint32(-1)) {
                    constant += adjustConstantForIncrement(pc, entry.u.check.valueSlot2);
                    Jump overflow = masm.branchAdd32(Assembler::Overflow,
                                                     frame.addressOf(entry.u.check.valueSlot2), T1);
                    jumps->append(overflow);
                }
                if (constant != 0) {
                    Jump overflow = masm.branchAdd32(Assembler::Overflow,
                                                     Imm32(constant), T1);
                    jumps->append(overflow);
                }
                Jump j = masm.branch32(Assembler::BelowOrEqual, T0, T1);
                jumps->append(j);
            } else {
                Jump j = masm.branch32(Assembler::BelowOrEqual, T0,
                                       Imm32(constant));
                jumps->append(j);
            }
            break;
          }

          case InvariantEntry::RANGE_CHECK: {
            int32 constant = 0;

            constant += adjustConstantForIncrement(pc, entry.u.check.valueSlot1);
            masm.loadPayload(frame.addressOf(entry.u.check.valueSlot1), T0);
            if (entry.u.check.valueSlot2 != uint32(-1)) {
                constant += adjustConstantForIncrement(pc, entry.u.check.valueSlot2);
                Jump overflow = masm.branchAdd32(Assembler::Overflow,
                                                 frame.addressOf(entry.u.check.valueSlot2), T0);
                jumps->append(overflow);
            }
            if (constant != 0) {
                Jump overflow = masm.branchAdd32(Assembler::Overflow, Imm32(constant), T0);
                jumps->append(overflow);
            }
            Jump j = masm.branch32(Assembler::GreaterThan, T0, Imm32(entry.u.check.constant));
            jumps->append(j);
            break;
          }

          case InvariantEntry::NEGATIVE_CHECK: {
            masm.loadPayload(frame.addressOf(entry.u.check.valueSlot1), T0);
            if (entry.u.check.constant != 0) {
                Jump overflow = masm.branchAdd32(Assembler::Overflow,
                                                 Imm32(entry.u.check.constant), T0);
                jumps->append(overflow);
            }
            Jump j = masm.branch32(Assembler::LessThan, T0, Imm32(0));
            jumps->append(j);
            break;
          }

          case InvariantEntry::INVARIANT_SLOTS:
          case InvariantEntry::INVARIANT_LENGTH: {
            uint32 array = entry.u.array.arraySlot;
            Jump notObject = masm.testObject(Assembler::NotEqual, frame.addressOf(array));
            jumps->append(notObject);
            masm.loadPayload(frame.addressOf(array), T0);

            uint32 offset = (entry.kind == InvariantEntry::INVARIANT_SLOTS)
                ? JSObject::offsetOfSlots()
                : offsetof(JSObject, privateData);

            masm.loadPtr(Address(T0, offset), T0);
            masm.storePtr(T0, frame.addressOf(frame.getTemporary(entry.u.array.temporary)));
            break;
          }

          default:
            JS_NOT_REACHED("Bad invariant kind");
        }
    }
}







bool
LoopState::getLoopTestAccess(const SSAValue &v, uint32 *pslot, int32 *pconstant)
{
    *pslot = UNASSIGNED;
    *pconstant = 0;

    if (v.kind() == SSAValue::PHI || v.kind() == SSAValue::VAR) {
        



        uint32 slot;
        uint32 offset;
        if (v.kind() == SSAValue::PHI) {
            slot = v.phiSlot();
            offset = v.phiOffset();
        } else {
            slot = v.varSlot();
            offset = v.varInitial() ? 0 : v.varOffset();
        }
        if (analysis->slotEscapes(slot))
            return false;
        if (analysis->liveness(slot).firstWrite(offset + 1, lifetime->backedge) != uint32(-1))
            return false;
        *pslot = slot;
        *pconstant = 0;
        return true;
    }

    jsbytecode *pc = script->code + v.pushedOffset();

    JSOp op = JSOp(*pc);
    const JSCodeSpec *cs = &js_CodeSpec[op];

    








    switch (op) {

      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC:
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC: {
        uint32 slot = GetBytecodeSlot(script, pc);
        if (analysis->slotEscapes(slot))
            return false;

        
        types::TypeSet *types = analysis->pushedTypes(pc, 0);
        if (types->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
            return false;

        *pslot = slot;
        if (cs->format & JOF_POST) {
            if (cs->format & JOF_INC)
                *pconstant = -1;
            else
                *pconstant = 1;
        }
        return true;
      }

      case JSOP_ZERO:
      case JSOP_ONE:
      case JSOP_UINT16:
      case JSOP_UINT24:
      case JSOP_INT8:
      case JSOP_INT32:
        *pconstant = GetBytecodeInteger(pc);
        return true;

      default:
        return false;
    }
}

void
LoopState::analyzeLoopTest()
{
    
    if (lifetime->entry == lifetime->head)
        return;

    
    if (lifetime->entry < lifetime->lastBlock)
        return;

    
    jsbytecode *backedge = script->code + lifetime->backedge;
    if (JSOp(*backedge) != JSOP_IFNE)
        return;
    const SSAValue &test = analysis->poppedValue(backedge, 0);
    if (test.kind() != SSAValue::PUSHED)
        return;
    JSOp cmpop = JSOp(script->code[test.pushedOffset()]);
    switch (cmpop) {
      case JSOP_GT:
      case JSOP_GE:
      case JSOP_LT:
      case JSOP_LE:
        break;
      default:
        return;
    }

    SSAValue one = analysis->poppedValue(test.pushedOffset(), 1);
    SSAValue two = analysis->poppedValue(test.pushedOffset(), 0);

    
    uint32 swapRHS;
    int32 swapConstant;
    if (getLoopTestAccess(two, &swapRHS, &swapConstant)) {
        if (swapRHS != UNASSIGNED && analysis->liveness(swapRHS).firstWrite(lifetime) != uint32(-1)) {
            SSAValue tmp = one;
            one = two;
            two = tmp;
            cmpop = ReverseCompareOp(cmpop);
        }
    }

    uint32 lhs;
    int32 lhsConstant;
    if (!getLoopTestAccess(one, &lhs, &lhsConstant))
        return;

    uint32 rhs = UNASSIGNED;
    int32 rhsConstant = 0;
    bool rhsLength = false;

    if (two.kind() == SSAValue::PUSHED &&
        JSOp(script->code[two.pushedOffset()] == JSOP_LENGTH)) {
        
        const SSAValue &array = analysis->poppedValue(two.pushedOffset(), 0);
        if (!getLoopTestAccess(array, &rhs, &rhsConstant))
            return;
        if (rhsConstant != 0 || analysis->liveness(rhs).firstWrite(lifetime) != uint32(-1)) {
            return;
        }
        if (!invariantLength(frame.getOrTrack(rhs), analysis->getValueTypes(array)))
            return;
        rhsLength = true;
    } else {
        if (!getLoopTestAccess(two, &rhs, &rhsConstant))
            return;

        
        if (rhs != UNASSIGNED && analysis->liveness(rhs).firstWrite(lifetime) != uint32(-1))
            return;
    }

    if (lhs == UNASSIGNED)
        return;

    int32 constant;
    if (!SafeSub(rhsConstant, lhsConstant, &constant))
        return;

    
    if (cmpop == JSOP_GT && !SafeAdd(constant, 1, &constant))
        return;

    
    if (cmpop == JSOP_LT && !SafeSub(constant, 1, &constant))
        return;

    

    this->testLHS = lhs;
    this->testRHS = rhs;
    this->testConstant = constant;
    this->testLength = rhsLength;
    this->testLessEqual = (cmpop == JSOP_LT || cmpop == JSOP_LE);
}

void
LoopState::analyzeLoopIncrements()
{
    





    for (uint32 slot = ArgSlot(0); slot < LocalSlot(script, script->nfixed); slot++) {
        if (analysis->slotEscapes(slot))
            continue;

        uint32 offset = analysis->liveness(slot).onlyWrite(lifetime);
        if (offset == uint32(-1) || offset < lifetime->lastBlock)
            continue;

        JSOp op = JSOp(script->code[offset]);
        const JSCodeSpec *cs = &js_CodeSpec[op];
        if (cs->format & (JOF_INC | JOF_DEC)) {
            types::TypeSet *types = analysis->pushedTypes(offset);
            if (types->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
                continue;

            Increment inc;
            inc.slot = slot;
            inc.offset = offset;
            increments.append(inc);
        }
    }
}

bool
LoopState::definiteArrayAccess(const SSAValue &obj, const SSAValue &index)
{
    












    types::TypeSet *objTypes = analysis->getValueTypes(obj);
    types::TypeSet *elemTypes = analysis->getValueTypes(index);

    if (objTypes->getKnownTypeTag(cx) != JSVAL_TYPE_OBJECT ||
        elemTypes->getKnownTypeTag(cx) != JSVAL_TYPE_INT32) {
        return false;
    }

    types::ObjectKind kind = objTypes->getKnownObjectKind(cx);
    if (kind != types::OBJECT_DENSE_ARRAY && kind != types::OBJECT_PACKED_ARRAY)
        return false;

    if (cc.arrayPrototypeHasIndexedProperty())
        return false;

    uint32 objSlot;
    int32 objConstant;
    if (!getEntryValue(obj, &objSlot, &objConstant) || objConstant != 0)
        return false;
    if (!loopInvariantEntry(objSlot))
        return false;

    
    if (index.kind() == SSAValue::PUSHED) {
        JSOp op = JSOp(script->code[index.pushedOffset()]);
        switch (op) {
          case JSOP_BITAND:
          case JSOP_BITOR:
          case JSOP_BITXOR:
          case JSOP_BITNOT:
          case JSOP_RSH:
          case JSOP_LSH:
          case JSOP_URSH:
            return true;
          default:;
        }
    }

    uint32 indexSlot;
    int32 indexConstant;
    if (!getEntryValue(index, &indexSlot, &indexConstant))
        return false;

    




    return true;
}

void
LoopState::analyzeLoopBody()
{
    

    unsigned offset = lifetime->head;
    while (offset < lifetime->backedge) {
        jsbytecode *pc = script->code + offset;
        uint32 successorOffset = offset + GetBytecodeLength(pc);

        analyze::Bytecode *opinfo = analysis->maybeCode(offset);
        if (!opinfo) {
            offset = successorOffset;
            continue;
        }

        JSOp op = JSOp(*pc);
        switch (op) {

          case JSOP_SETHOLE:
          case JSOP_SETELEM: {
            SSAValue objValue = analysis->poppedValue(pc, 2);
            SSAValue elemValue = analysis->poppedValue(pc, 1);

            types::TypeSet *objTypes = analysis->getValueTypes(objValue);
            types::TypeSet *elemTypes = analysis->getValueTypes(elemValue);

            



            if (objTypes->unknown() || elemTypes->getKnownTypeTag(cx) != JSVAL_TYPE_INT32) {
                unknownModset = true;
                return;
            }

            objTypes->addFreeze(cx);
            for (unsigned i = 0; i < objTypes->getObjectCount(); i++) {
                types::TypeObject *object = objTypes->getObject(i);
                if (!object)
                    continue;
                if (!addModifiedProperty(object, JSID_VOID))
                    return;
                if (op == JSOP_SETHOLE && !addGrowArray(object))
                    return;
            }

            if (constrainedLoop && !definiteArrayAccess(objValue, elemValue))
                constrainedLoop = false;
            break;
          }

          case JSOP_GETELEM: {
            SSAValue objValue = analysis->poppedValue(pc, 1);
            SSAValue elemValue = analysis->poppedValue(pc, 0);

            if (constrainedLoop && !definiteArrayAccess(objValue, elemValue))
                constrainedLoop = false;
            break;
          }

          case JSOP_TRACE:
          case JSOP_NOTRACE:
          case JSOP_POP:
          case JSOP_ZERO:
          case JSOP_ONE:
          case JSOP_INT8:
          case JSOP_INT32:
          case JSOP_UINT16:
          case JSOP_UINT24:
          case JSOP_FALSE:
          case JSOP_TRUE:
          case JSOP_GETARG:
          case JSOP_SETARG:
          case JSOP_INCARG:
          case JSOP_DECARG:
          case JSOP_ARGINC:
          case JSOP_ARGDEC:
          case JSOP_THIS:
          case JSOP_GETLOCAL:
          case JSOP_SETLOCAL:
          case JSOP_SETLOCALPOP:
          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC:
          case JSOP_IFEQ:
          case JSOP_IFEQX:
          case JSOP_IFNE:
          case JSOP_IFNEX:
          case JSOP_AND:
          case JSOP_ANDX:
          case JSOP_OR:
          case JSOP_ORX:
          case JSOP_GOTO:
          case JSOP_GOTOX:
            break;

          case JSOP_ADD:
          case JSOP_SUB:
          case JSOP_MUL:
          case JSOP_MOD:
          case JSOP_DIV:
          case JSOP_BITAND:
          case JSOP_BITOR:
          case JSOP_BITXOR:
          case JSOP_RSH:
          case JSOP_LSH:
          case JSOP_URSH:
          case JSOP_EQ:
          case JSOP_NE:
          case JSOP_LT:
          case JSOP_LE:
          case JSOP_GT:
          case JSOP_GE:
          case JSOP_STRICTEQ:
          case JSOP_STRICTNE: {
            JSValueType type = analysis->poppedTypes(pc, 1)->getKnownTypeTag(cx);
            if (type != JSVAL_TYPE_INT32 && type != JSVAL_TYPE_DOUBLE)
                constrainedLoop = false;
          }
          

          case JSOP_POS:
          case JSOP_NEG:
          case JSOP_BITNOT: {
            JSValueType type = analysis->poppedTypes(pc, 0)->getKnownTypeTag(cx);
            if (type != JSVAL_TYPE_INT32 && type != JSVAL_TYPE_DOUBLE)
                constrainedLoop = false;
            break;
          }

          default:
            constrainedLoop = false;
            break;
        }

        offset = successorOffset;
    }
}

bool
LoopState::addGrowArray(types::TypeObject *object)
{
    static const uint32 MAX_SIZE = 10;
    for (unsigned i = 0; i < growArrays.length(); i++) {
        if (growArrays[i] == object)
            return true;
    }
    if (growArrays.length() >= MAX_SIZE) {
        unknownModset = true;
        return false;
    }
    growArrays.append(object);

    return true;
}

bool
LoopState::addModifiedProperty(types::TypeObject *object, jsid id)
{
    static const uint32 MAX_SIZE = 20;
    for (unsigned i = 0; i < modifiedProperties.length(); i++) {
        if (modifiedProperties[i].object == object && modifiedProperties[i].id == id)
            return true;
    }
    if (modifiedProperties.length() >= MAX_SIZE) {
        unknownModset = true;
        return false;
    }

    ModifiedProperty property;
    property.object = object;
    property.id = id;
    modifiedProperties.append(property);

    return true;
}

bool
LoopState::hasGrowArray(types::TypeObject *object)
{
    if (unknownModset)
        return true;
    for (unsigned i = 0; i < growArrays.length(); i++) {
        if (growArrays[i] == object)
            return true;
    }
    return false;
}

bool
LoopState::hasModifiedProperty(types::TypeObject *object, jsid id)
{
    if (unknownModset)
        return true;
    for (unsigned i = 0; i < modifiedProperties.length(); i++) {
        if (modifiedProperties[i].object == object && modifiedProperties[i].id == id)
            return true;
    }
    return false;
}

uint32
LoopState::getIncrement(uint32 slot)
{
    for (unsigned i = 0; i < increments.length(); i++) {
        if (increments[i].slot == slot)
            return increments[i].offset;
    }
    return uint32(-1);
}

int32
LoopState::adjustConstantForIncrement(jsbytecode *pc, uint32 slot)
{
    







    uint32 offset = getIncrement(slot);

    







    if (offset == uint32(-1) || offset < uint32(pc - script->code))
        return 0;

    switch (JSOp(script->code[offset])) {
      case JSOP_INCLOCAL:
      case JSOP_LOCALINC:
      case JSOP_INCARG:
      case JSOP_ARGINC:
        return 1;
      case JSOP_DECLOCAL:
      case JSOP_LOCALDEC:
      case JSOP_DECARG:
      case JSOP_ARGDEC:
        return -1;
      default:
        JS_NOT_REACHED("Bad op");
        return 0;
    }
}

bool
LoopState::getEntryValue(const SSAValue &v, uint32 *pslot, int32 *pconstant)
{
    





    if (v.kind() == SSAValue::PHI && v.phiSlot() < TotalSlots(script)) {
        if (v.phiOffset() > lifetime->head &&
            analysis->liveness(v.phiSlot()).firstWrite(lifetime) < v.phiOffset()) {
            return false;
        }
        *pslot = v.phiSlot();
        *pconstant = 0;
        return true;
    }

    if (v.kind() == SSAValue::VAR) {
        if (v.varInitial() || v.varOffset() < lifetime->head) {
            *pslot = v.varSlot();
            *pconstant = 0;
            return true;
        }
    }

    if (v.kind() != SSAValue::PUSHED)
        return false;

    jsbytecode *pc = script->code + v.pushedOffset();
    JSOp op = (JSOp)*pc;

    switch (op) {

      case JSOP_GETLOCAL:
      case JSOP_LOCALINC:
      case JSOP_INCLOCAL:
      case JSOP_GETARG:
      case JSOP_ARGINC:
      case JSOP_INCARG: {
        uint32 slot = GetBytecodeSlot(script, pc);
        if (analysis->slotEscapes(slot))
            return false;
        uint32 write = analysis->liveness(slot).firstWrite(lifetime);
        if (write != uint32(-1) && write < v.pushedOffset()) {
            
            return false;
        }
        *pslot = slot;
        *pconstant = (op == JSOP_INCLOCAL || op == JSOP_INCARG) ? 1 : 0;
        return true;
      }

      case JSOP_ZERO:
      case JSOP_ONE:
      case JSOP_UINT16:
      case JSOP_UINT24:
      case JSOP_INT8:
      case JSOP_INT32:
        *pslot = UNASSIGNED;
        *pconstant = GetBytecodeInteger(pc);
        return true;

      default:
        return false;
    }
}

bool
LoopState::computeInterval(const analyze::SSAValue &v, int32 *pmin, int32 *pmax)
{
    if (v.kind() == SSAValue::VAR && !v.varInitial()) {
        jsbytecode *pc = script->code + v.varOffset();
        switch (JSOp(*pc)) {
          case JSOP_SETLOCAL:
          case JSOP_SETARG:
            return computeInterval(analysis->poppedValue(pc, 0), pmin, pmax);

          default:
            return false;
        }
    }

    if (v.kind() != SSAValue::PUSHED)
        return false;

    jsbytecode *pc = script->code + v.pushedOffset();
    JSOp op = (JSOp)*pc;

    
    switch (op) {

      case JSOP_ZERO:
      case JSOP_ONE:
      case JSOP_UINT16:
      case JSOP_UINT24:
      case JSOP_INT8:
      case JSOP_INT32: {
        int32 constant = GetBytecodeInteger(pc);
        *pmin = constant;
        *pmax = constant;
        return true;
      }

      case JSOP_BITAND: {
        int32 lhsmin, lhsmax, rhsmin, rhsmax;
        bool haslhs = computeInterval(analysis->poppedValue(pc, 1), &lhsmin, &lhsmax);
        bool hasrhs = computeInterval(analysis->poppedValue(pc, 0), &rhsmin, &rhsmax);

        
        haslhs = haslhs && lhsmin == lhsmax && lhsmin >= 0;
        hasrhs = hasrhs && rhsmin == rhsmax && rhsmin >= 0;

        if (haslhs && hasrhs) {
            *pmin = 0;
            *pmax = Min(lhsmax, rhsmax);
        } else if (haslhs) {
            *pmin = 0;
            *pmax = lhsmax;
        } else if (hasrhs) {
            *pmin = 0;
            *pmax = rhsmax;
        } else {
            return false;
        }
        return true;
      }

      case JSOP_RSH: {
        int32 rhsmin, rhsmax;
        if (!computeInterval(analysis->poppedValue(pc, 0), &rhsmin, &rhsmax) || rhsmin != rhsmax)
            return false;

        
        int32 shift = rhsmin & 0x1f;
        *pmin = -(1 << (31 - shift));
        *pmax = (1 << (31 - shift)) - 1;
        return true;
      }

      case JSOP_URSH: {
        int32 rhsmin, rhsmax;
        if (!computeInterval(analysis->poppedValue(pc, 0), &rhsmin, &rhsmax) || rhsmin != rhsmax)
            return false;

        
        int32 shift = rhsmin & 0x1f;
        if (shift == 0)
            return false;
        *pmin = 0;
        *pmax = (1 << (31 - shift)) - 1;
        return true;
      }

      case JSOP_MOD: {
        int32 rhsmin, rhsmax;
        if (!computeInterval(analysis->poppedValue(pc, 0), &rhsmin, &rhsmax) || rhsmin != rhsmax)
            return false;

        int32 rhs = abs(rhsmax);
        *pmin = -(rhs - 1);
        *pmax = rhs - 1;
        return true;
      }

      case JSOP_ADD: {
        int32 lhsmin, lhsmax, rhsmin, rhsmax;
        if (!computeInterval(analysis->poppedValue(pc, 1), &lhsmin, &lhsmax) ||
            !computeInterval(analysis->poppedValue(pc, 0), &rhsmin, &rhsmax)) {
            return false;
        }
        return SafeAdd(lhsmin, rhsmin, pmin) && SafeAdd(lhsmax, rhsmax, pmax);
      }

      case JSOP_SUB: {
        int32 lhsmin, lhsmax, rhsmin, rhsmax;
        if (!computeInterval(analysis->poppedValue(pc, 1), &lhsmin, &lhsmax) ||
            !computeInterval(analysis->poppedValue(pc, 0), &rhsmin, &rhsmax)) {
            return false;
        }
        return SafeSub(lhsmin, rhsmax, pmin) && SafeSub(lhsmax, rhsmin, pmax);
      }

      case JSOP_MUL: {
        int32 lhsmin, lhsmax, rhsmin, rhsmax;
        if (!computeInterval(analysis->poppedValue(pc, 1), &lhsmin, &lhsmax) ||
            !computeInterval(analysis->poppedValue(pc, 0), &rhsmin, &rhsmax)) {
            return false;
        }
        int32 nlhs = Max(abs(lhsmin), abs(lhsmax));
        int32 nrhs = Max(abs(rhsmin), abs(rhsmax));

        if (!SafeMul(nlhs, nrhs, pmax))
            return false;

        if (lhsmin < 0 || rhsmin < 0) {
            
            *pmin = -*pmax;
        } else {
            *pmin = 0;
        }

        return true;
      }

      default:
        return false;
    }
}
