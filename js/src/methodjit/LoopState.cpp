





































#include "methodjit/Compiler.h"
#include "methodjit/LoopState.h"
#include "methodjit/FrameState-inl.h"
#include "methodjit/StubCalls.h"

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
      invariantEntries(CompilerAllocPolicy(cx, *cc)),
      outer(NULL), PC(NULL),
      testLHS(UNASSIGNED), testRHS(UNASSIGNED),
      testConstant(0), testLessEqual(false), testLength(false),
      increments(CompilerAllocPolicy(cx, *cc)), unknownModset(false),
      growArrays(CompilerAllocPolicy(cx, *cc)),
      modifiedProperties(CompilerAllocPolicy(cx, *cc))
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

    if (!stack.analyze(liveness->pool, script, lifetime->head,
                       lifetime->backedge - lifetime->head + 1, analysis)) {
        return false;
    }

    analyzeLoopTest();
    analyzeLoopIncrements();
    analyzeModset();

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

    



    for (unsigned i = 0; i < invariantEntries.length(); i++) {
        InvariantEntry &entry = invariantEntries[i];
        if (entry.kind == InvariantEntry::BOUNDS_CHECK &&
            entry.u.check.arraySlot == arraySlot &&
            entry.u.check.valueSlot1 == valueSlot1 &&
            entry.u.check.valueSlot2 == valueSlot2) {
            if (entry.u.check.constant < constant)
                entry.u.check.constant = constant;
            return true;
        }
    }

    





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

        InvariantEntry entry;
        entry.kind = InvariantEntry::INVARIANT_SLOTS;
        entry.u.array.arraySlot = arraySlot;
        entry.u.array.temporary = which;
        invariantEntries.append(entry);
    }

    InvariantEntry entry;
    entry.kind = InvariantEntry::BOUNDS_CHECK;
    entry.u.check.arraySlot = arraySlot;
    entry.u.check.valueSlot1 = valueSlot1;
    entry.u.check.valueSlot2 = valueSlot2;
    entry.u.check.constant = constant;
    invariantEntries.append(entry);

    return true;
}

void
LoopState::addNegativeCheck(uint32 valueSlot, int32 constant)
{
    JaegerSpew(JSpew_Analysis, "Nonnegative check %s + %d >= 0\n",
               frame.entryName(valueSlot), constant);

    



    for (unsigned i = 0; i < invariantEntries.length(); i++) {
        InvariantEntry &entry = invariantEntries[i];
        if (entry.kind == InvariantEntry::NEGATIVE_CHECK &&
            entry.u.check.valueSlot1 == valueSlot) {
            if (entry.u.check.constant > constant)
                entry.u.check.constant = constant;
            return;
        }
    }

    InvariantEntry entry;
    entry.kind = InvariantEntry::NEGATIVE_CHECK;
    entry.u.check.valueSlot1 = valueSlot;
    entry.u.check.constant = constant;
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
        




        RegisterAllocation *entry = liveness->getCode(lifetime->entry).allocation;
        JS_ASSERT(entry && !entry->assigned(reg));
        entry->set(reg, slot, true);
    }
}

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

bool
LoopState::hoistArrayLengthCheck(const FrameEntry *obj, unsigned indexPopped)
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

    





    types::TypeSet *objTypes = cc.getTypeSet(obj);
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
    if (!getEntryValue(PC - script->code, indexPopped, &index, &indexConstant)) {
        JaegerSpew(JSpew_Analysis, "Could not compute index in terms of loop entry state\n");
        return false;
    }

    if (index == UNASSIGNED) {
        
        return addHoistedCheck(frame.indexOfFe(obj), UNASSIGNED, UNASSIGNED, indexConstant);
    }

    if (loopInvariantEntry(index)) {
        
        return addHoistedCheck(frame.indexOfFe(obj), index, UNASSIGNED, indexConstant);
    }

    




    if (!liveness->nonDecreasing(index, lifetime)) {
        JaegerSpew(JSpew_Analysis, "Index may decrease in future iterations\n");
        return false;
    }

    








    if (index == testLHS && testLessEqual) {
        uint32 rhs = testRHS;

        if (rhs != UNASSIGNED) {
            types::TypeSet *types = cc.getTypeSet(rhs);
            if (!types) {
                JaegerSpew(JSpew_Analysis, "Unknown type of branch test\n");
                return false;
            }
            if (testLength) {
                FrameEntry *rhsFE = frame.getOrTrack(rhs);
                FrameEntry *lengthEntry = invariantLength(rhsFE);
                if (!lengthEntry) {
                    JaegerSpew(JSpew_Analysis, "Could not get invariant entry for length\n");
                    return false;
                }
                rhs = frame.indexOfFe(lengthEntry);
            }
        }

        int32 constant;
        if (!SafeSub(testConstant, indexConstant, &constant))
            return false;

        






        addNegativeCheck(index, indexConstant);

        return addHoistedCheck(frame.indexOfFe(obj), rhs, UNASSIGNED, constant);
    }

    











    if (testLHS == UNASSIGNED || testRHS != UNASSIGNED) {
        JaegerSpew(JSpew_Analysis, "Branch test is not comparison with constant\n");
        return false;
    }

    uint32 incrementOffset = getIncrement(index);
    if (incrementOffset == uint32(-1)) {
        




        JaegerSpew(JSpew_Analysis, "No increment found for index variable\n");
        return false;
    }

    uint32 decrementOffset = getIncrement(testLHS);
    JSOp op = (decrementOffset == uint32(-1)) ? JSOP_NOP : JSOp(script->code[decrementOffset]);
    switch (op) {
      case JSOP_DECLOCAL:
      case JSOP_LOCALDEC:
      case JSOP_DECARG:
      case JSOP_ARGDEC:
        break;
      default:
        JaegerSpew(JSpew_Analysis, "No decrement on loop condition variable\n");
        return false;
    }

    int32 constant;
    if (!SafeSub(indexConstant, testConstant, &constant))
        return false;

    addNegativeCheck(index, indexConstant);

    return addHoistedCheck(frame.indexOfFe(obj), index, testLHS, constant);
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
LoopState::invariantLength(const FrameEntry *obj)
{
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

    if (!loopInvariantEntry(frame.indexOfFe(obj)))
        return NULL;

    
    types::TypeSet *types = cc.getTypeSet(slot);
    types::ObjectKind kind = types ? types->getKnownObjectKind(cx) : types::OBJECT_UNKNOWN;
    if (kind != types::OBJECT_DENSE_ARRAY && kind != types::OBJECT_PACKED_ARRAY)
        return NULL;

    




    for (unsigned i = 0; i < types->getObjectCount(); i++) {
        types::TypeObject *object = types->getObject(i);
        if (!object)
            continue;
        if (object->unknownProperties() || hasModifiedProperty(object, JSID_VOID))
            return NULL;
    }
    types->addFreeze(cx);

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
            if (cc.getTypeSet(array)->getKnownTypeTag(cx) != JSVAL_TYPE_OBJECT) {
                Jump notObject = masm.testObject(Assembler::NotEqual, frame.addressOf(array));
                jumps->append(notObject);
            }
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




static inline uint32 localSlot(JSScript *script, uint32 local) {
    return 2 + (script->fun ? script->fun->nargs : 0) + local;
}
static inline uint32 argSlot(uint32 arg) {
    return 2 + arg;
}
static inline uint32 thisSlot() {
    return 1;
}


bool
LoopState::loopVariableAccess(jsbytecode *pc)
{
    switch (JSOp(*pc)) {
      case JSOP_GETLOCAL:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC:
        if (analysis->localEscapes(GET_SLOTNO(pc)))
            return false;
        return liveness->firstWrite(localSlot(script, GET_SLOTNO(pc)), lifetime) != uint32(-1);
      case JSOP_GETARG:
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
        if (analysis->argEscapes(GET_SLOTNO(pc)))
            return false;
        return liveness->firstWrite(argSlot(GET_SLOTNO(pc)), lifetime) != uint32(-1);
      default:
        return false;
    }
}

static inline int32
GetBytecodeInteger(jsbytecode *pc)
{
    switch (JSOp(*pc)) {

      case JSOP_ZERO:
        return 0;

      case JSOP_ONE:
        return 1;

      case JSOP_UINT16:
        return (int32_t) GET_UINT16(pc);

      case JSOP_UINT24:
        return (int32_t) GET_UINT24(pc);

      case JSOP_INT8:
        return GET_INT8(pc);

      case JSOP_INT32:
        return GET_INT32(pc);

      default:
        JS_NOT_REACHED("Bad op");
        return 0;
    }
}





bool
LoopState::getLoopTestAccess(jsbytecode *pc, uint32 *pslot, int32 *pconstant)
{
    *pslot = UNASSIGNED;
    *pconstant = 0;

    








    JSOp op = JSOp(*pc);
    switch (op) {

      case JSOP_GETLOCAL:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC: {
        uint32 local = GET_SLOTNO(pc);
        if (analysis->localEscapes(local))
            return false;
        *pslot = localSlot(script, local);
        if (op == JSOP_LOCALINC)
            *pconstant = -1;
        else if (op == JSOP_LOCALDEC)
            *pconstant = 1;
        return true;
      }

      case JSOP_GETARG:
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC: {
        uint32 arg = GET_SLOTNO(pc);
        if (analysis->argEscapes(arg))
            return false;
        *pslot = argSlot(arg);
        if (op == JSOP_ARGINC)
            *pconstant = -1;
        else if (op == JSOP_ARGDEC)
            *pconstant = 1;
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
    StackAnalysis::PoppedValue test = stack.popped(backedge, 0);
    if (test.offset == StackAnalysis::UNKNOWN_PUSHED)
        return;
    JSOp cmpop = JSOp(script->code[test.offset]);
    switch (cmpop) {
      case JSOP_GT:
      case JSOP_GE:
      case JSOP_LT:
      case JSOP_LE:
        break;
      default:
        return;
    }

    StackAnalysis::PoppedValue poppedOne = stack.popped(test.offset, 1);
    StackAnalysis::PoppedValue poppedTwo = stack.popped(test.offset, 0);

    if (poppedOne.offset == StackAnalysis::UNKNOWN_PUSHED ||
        poppedTwo.offset == StackAnalysis::UNKNOWN_PUSHED) {
        return;
    }

    jsbytecode *one = script->code + poppedOne.offset;
    jsbytecode *two = script->code + poppedTwo.offset;

    
    if (loopVariableAccess(two)) {
        jsbytecode *tmp = one;
        one = two;
        two = tmp;
        cmpop = ReverseCompareOp(cmpop);
    }

    
    if (loopVariableAccess(two))
        return;

    uint32 lhs;
    int32 lhsConstant;
    if (!getLoopTestAccess(one, &lhs, &lhsConstant))
        return;

    uint32 rhs = UNASSIGNED;
    int32 rhsConstant = 0;
    bool rhsLength = false;

    if (JSOp(*two) == JSOP_LENGTH) {
        
        StackAnalysis::PoppedValue array = stack.popped(two, 0);
        if (array.offset == StackAnalysis::UNKNOWN_PUSHED)
            return;
        jsbytecode *arraypc = script->code + array.offset;
        if (loopVariableAccess(arraypc))
            return;
        switch (JSOp(*arraypc)) {
          case JSOP_GETLOCAL: {
            uint32 local = GET_SLOTNO(arraypc);
            if (analysis->localEscapes(local))
                return;
            rhs = localSlot(script, local);
            break;
          }
          case JSOP_GETARG: {
            uint32 arg = GET_SLOTNO(arraypc);
            if (analysis->argEscapes(arg))
                return;
            rhs = argSlot(arg);
            break;
          }
          case JSOP_THIS:
            rhs = thisSlot();
            break;
          default:
            return;
        }
        rhsLength = true;
    } else {
        if (!getLoopTestAccess(two, &rhs, &rhsConstant))
            return;
    }

    if (lhs == UNASSIGNED)
        return;

    
    types::TypeSet *lhsTypes = cc.getTypeSet(lhs);
    if (!lhsTypes || lhsTypes->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
        return;
    if (rhs != UNASSIGNED) {
        types::TypeSet *rhsTypes = cc.getTypeSet(rhs);
        if (!rhsTypes || rhsTypes->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
            return;
    }

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
    





    unsigned nargs = script->fun ? script->fun->nargs : 0;
    for (unsigned i = 0; i < nargs; i++) {
        if (analysis->argEscapes(i))
            continue;

        uint32 offset = liveness->onlyWrite(argSlot(i), lifetime);
        if (offset == uint32(-1) || offset < lifetime->lastBlock)
            continue;

        JSOp op = JSOp(script->code[offset]);
        if (op == JSOP_SETARG)
            continue;

        types::TypeSet *types = cc.getTypeSet(argSlot(i));
        if (!types || types->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
            continue;

        Increment inc;
        inc.slot = argSlot(i);
        inc.offset = offset;
        increments.append(inc);
    }

    for (unsigned i = 0; i < script->nfixed; i++) {
        if (analysis->localEscapes(i))
            continue;

        uint32 offset = liveness->onlyWrite(localSlot(script, i), lifetime);
        if (offset == uint32(-1) || offset < lifetime->lastBlock)
            continue;

        JSOp op = JSOp(script->code[offset]);
        if (op == JSOP_SETLOCAL || op == JSOP_SETLOCALPOP)
            continue;

        types::TypeSet *types = cc.getTypeSet(localSlot(script, i));
        if (!types || types->getKnownTypeTag(cx) != JSVAL_TYPE_INT32)
            continue;

        Increment inc;
        inc.slot = localSlot(script, i);
        inc.offset = offset;
        increments.append(inc);
    }
}

void
LoopState::analyzeModset()
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
            types::TypeSet *objTypes = poppedTypes(pc, 2);
            types::TypeSet *elemTypes = poppedTypes(pc, 1);

            



            if (!objTypes || objTypes->unknown() || !elemTypes ||
                elemTypes->getKnownTypeTag(cx) != JSVAL_TYPE_INT32) {
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
            break;
          }

          default:
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

inline types::TypeSet *
LoopState::poppedTypes(jsbytecode *pc, unsigned which)
{
    StackAnalysis::PoppedValue value = stack.popped(pc, which);
    if (value.offset == StackAnalysis::UNKNOWN_PUSHED)
        return NULL;
    return script->types->pushed(value.offset, value.which);
}

bool
LoopState::getEntryValue(uint32 offset, uint32 popped, uint32 *pslot, int32 *pconstant)
{
    




    StackAnalysis::PoppedValue value = stack.popped(offset, popped);
    if (value.offset == StackAnalysis::UNKNOWN_PUSHED)
        return false;

    jsbytecode *pc = script->code + value.offset;
    JSOp op = (JSOp)*pc;

    switch (op) {

      case JSOP_GETLOCAL:
      case JSOP_LOCALINC:
      case JSOP_INCLOCAL: {
        uint32 local = GET_SLOTNO(pc);
        if (analysis->localEscapes(local))
            return false;
        uint32 write = liveness->firstWrite(localSlot(script, local), lifetime);
        if (write != uint32(-1) && write < value.offset) {
            
            return false;
        }
        *pslot = localSlot(script, local);
        *pconstant = (op == JSOP_INCLOCAL) ? 1 : 0;
        return true;
      }

      case JSOP_GETARG:
      case JSOP_ARGINC:
      case JSOP_INCARG: {
        uint32 arg = GET_SLOTNO(pc);
        if (analysis->argEscapes(arg))
            return false;
        uint32 write = liveness->firstWrite(argSlot(arg), lifetime);
        if (write != uint32(-1) && write < value.offset)
            return false;
        *pslot = argSlot(arg);
        *pconstant = (op == JSOP_INCARG) ? 1 : 0;
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
