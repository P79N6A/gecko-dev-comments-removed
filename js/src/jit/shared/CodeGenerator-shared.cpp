





#include "jit/shared/CodeGenerator-shared-inl.h"

#include "mozilla/DebugOnly.h"

#include "jit/IonCaches.h"
#include "jit/IonMacroAssembler.h"
#include "jit/IonSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/ParallelFunctions.h"

#include "jit/IonFrames-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::DebugOnly;

namespace js {
namespace jit {

MacroAssembler &
CodeGeneratorShared::ensureMasm(MacroAssembler *masmArg)
{
    if (masmArg)
        return *masmArg;
    maybeMasm_.construct();
    return maybeMasm_.ref();
}

CodeGeneratorShared::CodeGeneratorShared(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masmArg)
  : oolIns(nullptr),
    maybeMasm_(),
    masm(ensureMasm(masmArg)),
    gen(gen),
    graph(*graph),
    current(nullptr),
    deoptTable_(nullptr),
#ifdef DEBUG
    pushedArgs_(0),
#endif
    lastOsiPointOffset_(0),
    sps_(&GetIonContext()->runtime->spsProfiler, &lastPC_),
    osrEntryOffset_(0),
    skipArgCheckEntryOffset_(0),
    frameDepth_(graph->localSlotCount() * sizeof(STACK_SLOT_SIZE) +
                graph->argumentSlotCount() * sizeof(Value))
{
    if (!gen->compilingAsmJS())
        masm.setInstrumentation(&sps_);

    
    
    
    if (gen->compilingAsmJS()) {
        JS_ASSERT(graph->argumentSlotCount() == 0);
        frameDepth_ += gen->maxAsmJSStackArgBytes();

        
        
        
#ifdef JS_CPU_ARM
        bool forceAlign = true;
#else
        bool forceAlign = false;
#endif
        if (gen->performsAsmJSCall() || forceAlign) {
            unsigned alignmentAtCall = AlignmentMidPrologue + frameDepth_;
            if (unsigned rem = alignmentAtCall % StackAlignment)
                frameDepth_ += StackAlignment - rem;
        }

        
        
        frameClass_ = FrameSizeClass::None();
    } else {
        frameClass_ = FrameSizeClass::FromDepth(frameDepth_);
    }
}

bool
CodeGeneratorShared::generateOutOfLineCode()
{
    for (size_t i = 0; i < outOfLineCode_.length(); i++) {
        if (!gen->temp().ensureBallast())
            return false;
        masm.setFramePushed(outOfLineCode_[i]->framePushed());
        lastPC_ = outOfLineCode_[i]->pc();
        sps_.setPushed(outOfLineCode_[i]->script());
        outOfLineCode_[i]->bind(&masm);

        oolIns = outOfLineCode_[i];
        if (!outOfLineCode_[i]->generate(this))
            return false;
    }
    oolIns = nullptr;

    return true;
}

bool
CodeGeneratorShared::addOutOfLineCode(OutOfLineCode *code)
{
    code->setFramePushed(masm.framePushed());
    
    
    
    if (oolIns)
        code->setSource(oolIns->script(), oolIns->pc());
    else
        code->setSource(current ? current->mir()->info().script() : nullptr, lastPC_);
    return outOfLineCode_.append(code);
}


static inline int32_t
ToStackIndex(LAllocation *a)
{
    if (a->isStackSlot()) {
        JS_ASSERT(a->toStackSlot()->slot() >= 1);
        return a->toStackSlot()->slot();
    }
    JS_ASSERT(-int32_t(sizeof(IonJSFrameLayout)) <= a->toArgument()->index());
    return -int32_t(sizeof(IonJSFrameLayout) + a->toArgument()->index());
}

bool
CodeGeneratorShared::encodeSlots(LSnapshot *snapshot, MResumePoint *resumePoint,
                                 uint32_t *startIndex)
{
    IonSpew(IonSpew_Codegen, "Encoding %u of resume point %p's operands starting from %u",
            resumePoint->numOperands(), (void *) resumePoint, *startIndex);
    for (uint32_t slotno = 0, e = resumePoint->numOperands(); slotno < e; slotno++) {
        uint32_t i = slotno + *startIndex;
        MDefinition *mir = resumePoint->getOperand(slotno);

        if (mir->isPassArg())
            mir = mir->toPassArg()->getArgument();
        JS_ASSERT(!mir->isPassArg());

        if (mir->isBox())
            mir = mir->toBox()->getOperand(0);

        MIRType type = mir->isUnused()
                       ? MIRType_Undefined
                       : mir->type();

        switch (type) {
          case MIRType_Undefined:
            snapshots_.addUndefinedSlot();
            break;
          case MIRType_Null:
            snapshots_.addNullSlot();
            break;
          case MIRType_Int32:
          case MIRType_String:
          case MIRType_Object:
          case MIRType_Boolean:
          case MIRType_Double:
          case MIRType_Float32:
          {
            LAllocation *payload = snapshot->payloadOfSlot(i);
            JSValueType valueType = ValueTypeFromMIRType(type);
            if (payload->isMemory()) {
                if (type == MIRType_Float32)
                    snapshots_.addFloat32Slot(ToStackIndex(payload));
                else
                    snapshots_.addSlot(valueType, ToStackIndex(payload));
            } else if (payload->isGeneralReg()) {
                snapshots_.addSlot(valueType, ToRegister(payload));
            } else if (payload->isFloatReg()) {
                FloatRegister reg = ToFloatRegister(payload);
                if (type == MIRType_Float32)
                    snapshots_.addFloat32Slot(reg);
                else
                    snapshots_.addSlot(reg);
            } else {
                MConstant *constant = mir->toConstant();
                const Value &v = constant->value();

                
                if (v.isInt32() && v.toInt32() >= -32 && v.toInt32() <= 32) {
                    snapshots_.addInt32Slot(v.toInt32());
                } else {
                    uint32_t index;
                    if (!graph.addConstantToPool(constant->value(), &index))
                        return false;
                    snapshots_.addConstantPoolSlot(index);
                }
            }
            break;
          }
          case MIRType_Magic:
          {
            uint32_t index;
            if (!graph.addConstantToPool(MagicValue(JS_OPTIMIZED_ARGUMENTS), &index))
                return false;
            snapshots_.addConstantPoolSlot(index);
            break;
          }
          default:
          {
            JS_ASSERT(mir->type() == MIRType_Value);
            LAllocation *payload = snapshot->payloadOfSlot(i);
#ifdef JS_NUNBOX32
            LAllocation *type = snapshot->typeOfSlot(i);
            if (type->isRegister()) {
                if (payload->isRegister())
                    snapshots_.addSlot(ToRegister(type), ToRegister(payload));
                else
                    snapshots_.addSlot(ToRegister(type), ToStackIndex(payload));
            } else {
                if (payload->isRegister())
                    snapshots_.addSlot(ToStackIndex(type), ToRegister(payload));
                else
                    snapshots_.addSlot(ToStackIndex(type), ToStackIndex(payload));
            }
#elif JS_PUNBOX64
            if (payload->isRegister())
                snapshots_.addSlot(ToRegister(payload));
            else
                snapshots_.addSlot(ToStackIndex(payload));
#endif
            break;
          }
      }
    }

    *startIndex += resumePoint->numOperands();
    return true;
}

bool
CodeGeneratorShared::encode(LSnapshot *snapshot)
{
    if (snapshot->snapshotOffset() != INVALID_SNAPSHOT_OFFSET)
        return true;

    uint32_t frameCount = snapshot->mir()->frameCount();

    IonSpew(IonSpew_Snapshots, "Encoding LSnapshot %p (frameCount %u)",
            (void *)snapshot, frameCount);

    MResumePoint::Mode mode = snapshot->mir()->mode();
    JS_ASSERT(mode != MResumePoint::Outer);
    bool resumeAfter = (mode == MResumePoint::ResumeAfter);

    SnapshotOffset offset = snapshots_.startSnapshot(frameCount, snapshot->bailoutKind(),
                                                     resumeAfter);

    FlattenedMResumePointIter mirOperandIter(snapshot->mir());
    if (!mirOperandIter.init())
        return false;

    uint32_t startIndex = 0;
    for (MResumePoint **it = mirOperandIter.begin(), **end = mirOperandIter.end();
         it != end;
         ++it)
    {
        MResumePoint *mir = *it;
        MBasicBlock *block = mir->block();
        JSFunction *fun = block->info().fun();
        JSScript *script = block->info().script();
        jsbytecode *pc = mir->pc();
        uint32_t exprStack = mir->stackDepth() - block->info().ninvoke();
        snapshots_.startFrame(fun, script, pc, exprStack);

        
        
        DebugOnly<jsbytecode *> bailPC = pc;
        if (mir->mode() == MResumePoint::ResumeAfter)
          bailPC = GetNextPc(pc);

#ifdef DEBUG
        if (GetIonContext()->cx) {
            uint32_t stackDepth = js_ReconstructStackDepth(GetIonContext()->cx, script, bailPC);
            if (JSOp(*bailPC) == JSOP_FUNCALL) {
                
                
                
                JS_ASSERT(stackDepth - exprStack <= 1);
            } else if (JSOp(*bailPC) != JSOP_FUNAPPLY &&
                       !IsGetPropPC(bailPC) && !IsSetPropPC(bailPC))
            {
                
                
                
                

                
                
                
                
                JS_ASSERT(exprStack == stackDepth);
            }
        }
#endif

#ifdef TRACK_SNAPSHOTS
        LInstruction *ins = instruction();

        uint32_t pcOpcode = 0;
        uint32_t lirOpcode = 0;
        uint32_t lirId = 0;
        uint32_t mirOpcode = 0;
        uint32_t mirId = 0;

        if (ins) {
            lirOpcode = ins->op();
            lirId = ins->id();
            if (ins->mirRaw()) {
                mirOpcode = ins->mirRaw()->op();
                mirId = ins->mirRaw()->id();
                if (ins->mirRaw()->trackedPc())
                    pcOpcode = *ins->mirRaw()->trackedPc();
            }
        }
        snapshots_.trackFrame(pcOpcode, mirOpcode, mirId, lirOpcode, lirId);
#endif

        if (!encodeSlots(snapshot, mir, &startIndex))
            return false;
        snapshots_.endFrame();
    }

    snapshots_.endSnapshot();

    snapshot->setSnapshotOffset(offset);

    return !snapshots_.oom();
}

bool
CodeGeneratorShared::assignBailoutId(LSnapshot *snapshot)
{
    JS_ASSERT(snapshot->snapshotOffset() != INVALID_SNAPSHOT_OFFSET);

    
    if (!deoptTable_)
        return false;

    JS_ASSERT(frameClass_ != FrameSizeClass::None());

    if (snapshot->bailoutId() != INVALID_BAILOUT_ID)
        return true;

    
    if (bailouts_.length() >= BAILOUT_TABLE_SIZE)
        return false;

    unsigned bailoutId = bailouts_.length();
    snapshot->setBailoutId(bailoutId);
    IonSpew(IonSpew_Snapshots, "Assigned snapshot bailout id %u", bailoutId);
    return bailouts_.append(snapshot->snapshotOffset());
}

void
CodeGeneratorShared::encodeSafepoints()
{
    for (SafepointIndex *it = safepointIndices_.begin(), *end = safepointIndices_.end();
         it != end;
         ++it)
    {
        LSafepoint *safepoint = it->safepoint();

        if (!safepoint->encoded()) {
            safepoint->fixupOffset(&masm);
            safepoints_.encode(safepoint);
        }

        it->resolve();
    }
}

bool
CodeGeneratorShared::markSafepoint(LInstruction *ins)
{
    return markSafepointAt(masm.currentOffset(), ins);
}

bool
CodeGeneratorShared::markSafepointAt(uint32_t offset, LInstruction *ins)
{
    JS_ASSERT_IF(safepointIndices_.length(),
                 offset - safepointIndices_.back().displacement() >= sizeof(uint32_t));
    return safepointIndices_.append(SafepointIndex(offset, ins->safepoint()));
}

void
CodeGeneratorShared::ensureOsiSpace()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (masm.currentOffset() - lastOsiPointOffset_ < Assembler::patchWrite_NearCallSize()) {
        int32_t paddingSize = Assembler::patchWrite_NearCallSize();
        paddingSize -= masm.currentOffset() - lastOsiPointOffset_;
        for (int32_t i = 0; i < paddingSize; ++i)
            masm.nop();
    }
    JS_ASSERT(masm.currentOffset() - lastOsiPointOffset_ >= Assembler::patchWrite_NearCallSize());
    lastOsiPointOffset_ = masm.currentOffset();
}

bool
CodeGeneratorShared::markOsiPoint(LOsiPoint *ins, uint32_t *callPointOffset)
{
    if (!encode(ins->snapshot()))
        return false;

    ensureOsiSpace();

    *callPointOffset = masm.currentOffset();
    SnapshotOffset so = ins->snapshot()->snapshotOffset();
    return osiIndices_.append(OsiIndex(*callPointOffset, so));
}

#ifdef CHECK_OSIPOINT_REGISTERS
template <class Op>
static void
HandleRegisterDump(Op op, MacroAssembler &masm, RegisterSet liveRegs, Register activation,
                   Register scratch)
{
    const size_t baseOffset = JitActivation::offsetOfRegs();

    
    for (GeneralRegisterIterator iter(liveRegs.gprs()); iter.more(); iter++) {
        Register reg = *iter;
        Address dump(activation, baseOffset + RegisterDump::offsetOfRegister(reg));

        if (reg == activation) {
            
            
            masm.push(scratch);
            masm.loadPtr(Address(StackPointer, sizeof(uintptr_t)), scratch);
            op(scratch, dump);
            masm.pop(scratch);
        } else {
            op(reg, dump);
        }
    }

    
    for (FloatRegisterIterator iter(liveRegs.fpus()); iter.more(); iter++) {
        FloatRegister reg = *iter;
        Address dump(activation, baseOffset + RegisterDump::offsetOfRegister(reg));
        op(reg, dump);
    }
}

class StoreOp
{
    MacroAssembler &masm;

  public:
    StoreOp(MacroAssembler &masm)
      : masm(masm)
    {}

    void operator()(Register reg, Address dump) {
        masm.storePtr(reg, dump);
    }
    void operator()(FloatRegister reg, Address dump) {
        masm.storeDouble(reg, dump);
    }
};

static void
StoreAllLiveRegs(MacroAssembler &masm, RegisterSet liveRegs)
{
    
    
    

    
    GeneralRegisterSet allRegs(GeneralRegisterSet::All());
    Register scratch = allRegs.takeAny();
    masm.push(scratch);
    masm.loadJitActivation(scratch);

    Address checkRegs(scratch, JitActivation::offsetOfCheckRegs());
    masm.add32(Imm32(1), checkRegs);

    StoreOp op(masm);
    HandleRegisterDump<StoreOp>(op, masm, liveRegs, scratch, allRegs.getAny());

    masm.pop(scratch);
}

class VerifyOp
{
    MacroAssembler &masm;
    Label *failure_;

  public:
    VerifyOp(MacroAssembler &masm, Label *failure)
      : masm(masm), failure_(failure)
    {}

    void operator()(Register reg, Address dump) {
        masm.branchPtr(Assembler::NotEqual, dump, reg, failure_);
    }
    void operator()(FloatRegister reg, Address dump) {
        masm.loadDouble(dump, ScratchFloatReg);
        masm.branchDouble(Assembler::DoubleNotEqual, ScratchFloatReg, reg, failure_);
    }
};

static void
OsiPointRegisterCheckFailed()
{
    
    
    MOZ_ASSUME_UNREACHABLE("Modified registers between VM call and OsiPoint");
}

void
CodeGeneratorShared::verifyOsiPointRegs(LSafepoint *safepoint)
{
    
    

    
    GeneralRegisterSet allRegs(GeneralRegisterSet::All());
    Register scratch = allRegs.takeAny();
    masm.push(scratch);
    masm.loadJitActivation(scratch);

    
    
    Label failure, done;
    Address checkRegs(scratch, JitActivation::offsetOfCheckRegs());
    masm.branch32(Assembler::Equal, checkRegs, Imm32(0), &done);

    
    
    
    
    
    masm.branch32(Assembler::NotEqual, checkRegs, Imm32(1), &failure);

    
    
    
    RegisterSet liveRegs = safepoint->liveRegs();
    liveRegs = RegisterSet::Intersect(liveRegs, RegisterSet::Not(safepoint->tempRegs()));

    VerifyOp op(masm, &failure);
    HandleRegisterDump<VerifyOp>(op, masm, liveRegs, scratch, allRegs.getAny());

    masm.jump(&done);

    masm.bind(&failure);
    masm.setupUnalignedABICall(0, scratch);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, OsiPointRegisterCheckFailed));
    masm.breakpoint();

    masm.bind(&done);
    masm.pop(scratch);
}

bool
CodeGeneratorShared::shouldVerifyOsiPointRegs(LSafepoint *safepoint)
{
    if (!js_IonOptions.checkOsiPointRegisters)
        return false;

    if (gen->info().executionMode() != SequentialExecution)
        return false;

    if (safepoint->liveRegs().empty(true) && safepoint->liveRegs().empty(false))
        return false; 

    return true;
}

void
CodeGeneratorShared::resetOsiPointRegs(LSafepoint *safepoint)
{
    if (!shouldVerifyOsiPointRegs(safepoint))
        return;

    
    
    GeneralRegisterSet allRegs(GeneralRegisterSet::All());
    Register scratch = allRegs.takeAny();
    masm.push(scratch);
    masm.loadJitActivation(scratch);
    Address checkRegs(scratch, JitActivation::offsetOfCheckRegs());
    masm.store32(Imm32(0), checkRegs);
    masm.pop(scratch);
}
#endif



bool
CodeGeneratorShared::callVM(const VMFunction &fun, LInstruction *ins, const Register *dynStack)
{
    
    JS_ASSERT(fun.executionMode == gen->info().executionMode());

    
    
    JS_ASSERT_IF(fun.outParam == Type_Double, GetIonContext()->runtime->jitSupportsFloatingPoint);

#ifdef DEBUG
    if (ins->mirRaw()) {
        JS_ASSERT(ins->mirRaw()->isInstruction());
        MInstruction *mir = ins->mirRaw()->toInstruction();
        JS_ASSERT_IF(mir->isEffectful(), mir->resumePoint());
    }
#endif

    
    
    
#ifdef DEBUG
    JS_ASSERT(pushedArgs_ == fun.explicitArgs);
    pushedArgs_ = 0;
#endif

    
    IonCode *wrapper = gen->ionRuntime()->getVMWrapper(fun);
    if (!wrapper)
        return false;

#ifdef CHECK_OSIPOINT_REGISTERS
    if (shouldVerifyOsiPointRegs(ins->safepoint()))
        StoreAllLiveRegs(masm, ins->safepoint()->liveRegs());
#endif

    
    
    
    
    uint32_t callOffset;
    if (dynStack)
        callOffset = masm.callWithExitFrame(wrapper, *dynStack);
    else
        callOffset = masm.callWithExitFrame(wrapper);

    if (!markSafepointAt(callOffset, ins))
        return false;

    
    
    int framePop = sizeof(IonExitFrameLayout) - sizeof(void*);

    
    masm.implicitPop(fun.explicitStackSlots() * sizeof(void *) + framePop);
    
    
    return true;
}

class OutOfLineTruncateSlow : public OutOfLineCodeBase<CodeGeneratorShared>
{
    FloatRegister src_;
    Register dest_;
    bool needFloat32Conversion_;

  public:
    OutOfLineTruncateSlow(FloatRegister src, Register dest, bool needFloat32Conversion = false)
      : src_(src), dest_(dest), needFloat32Conversion_(needFloat32Conversion)
    { }

    bool accept(CodeGeneratorShared *codegen) {
        return codegen->visitOutOfLineTruncateSlow(this);
    }
    FloatRegister src() const {
        return src_;
    }
    Register dest() const {
        return dest_;
    }
    bool needFloat32Conversion() const {
        return needFloat32Conversion_;
    }

};

OutOfLineCode *
CodeGeneratorShared::oolTruncateDouble(const FloatRegister &src, const Register &dest)
{
    OutOfLineTruncateSlow *ool = new OutOfLineTruncateSlow(src, dest);
    if (!addOutOfLineCode(ool))
        return nullptr;
    return ool;
}

bool
CodeGeneratorShared::emitTruncateDouble(const FloatRegister &src, const Register &dest)
{
    OutOfLineCode *ool = oolTruncateDouble(src, dest);
    if (!ool)
        return false;

    masm.branchTruncateDouble(src, dest, ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGeneratorShared::emitTruncateFloat32(const FloatRegister &src, const Register &dest)
{
    OutOfLineTruncateSlow *ool = new OutOfLineTruncateSlow(src, dest, true);
    if (!addOutOfLineCode(ool))
        return false;

    masm.branchTruncateFloat32(src, dest, ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGeneratorShared::visitOutOfLineTruncateSlow(OutOfLineTruncateSlow *ool)
{
    FloatRegister src = ool->src();
    Register dest = ool->dest();

    saveVolatile(dest);

    if (ool->needFloat32Conversion()) {
        masm.push(src);
        masm.convertFloatToDouble(src, src);
    }

    masm.setupUnalignedABICall(1, dest);
    masm.passABIArg(src);
    if (gen->compilingAsmJS())
        masm.callWithABI(AsmJSImm_ToInt32);
    else
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::ToInt32));
    masm.storeCallResult(dest);

    restoreVolatile(dest);

    if (ool->needFloat32Conversion())
        masm.pop(src);

    masm.jump(ool->rejoin());
    return true;
}

void
CodeGeneratorShared::emitPreBarrier(Register base, const LAllocation *index, MIRType type)
{
    if (index->isConstant()) {
        Address address(base, ToInt32(index) * sizeof(Value));
        masm.patchableCallPreBarrier(address, type);
    } else {
        BaseIndex address(base, ToRegister(index), TimesEight);
        masm.patchableCallPreBarrier(address, type);
    }
}

void
CodeGeneratorShared::emitPreBarrier(Address address, MIRType type)
{
    masm.patchableCallPreBarrier(address, type);
}

void
CodeGeneratorShared::dropArguments(unsigned argc)
{
    for (unsigned i = 0; i < argc; i++)
        pushedArgumentSlots_.popBack();
}

bool
CodeGeneratorShared::markArgumentSlots(LSafepoint *safepoint)
{
    for (size_t i = 0; i < pushedArgumentSlots_.length(); i++) {
        if (!safepoint->addValueSlot(pushedArgumentSlots_[i]))
            return false;
    }
    return true;
}

OutOfLineAbortPar *
CodeGeneratorShared::oolAbortPar(ParallelBailoutCause cause, MBasicBlock *basicBlock,
                                 jsbytecode *bytecode)
{
    OutOfLineAbortPar *ool = new OutOfLineAbortPar(cause, basicBlock, bytecode);
    if (!ool || !addOutOfLineCode(ool))
        return nullptr;
    return ool;
}

OutOfLineAbortPar *
CodeGeneratorShared::oolAbortPar(ParallelBailoutCause cause, LInstruction *lir)
{
    MDefinition *mir = lir->mirRaw();
    MBasicBlock *block = mir->block();
    jsbytecode *pc = mir->trackedPc();
    if (!pc) {
        if (lir->snapshot())
            pc = lir->snapshot()->mir()->pc();
        else
            pc = block->pc();
    }
    return oolAbortPar(cause, block, pc);
}

OutOfLinePropagateAbortPar *
CodeGeneratorShared::oolPropagateAbortPar(LInstruction *lir)
{
    OutOfLinePropagateAbortPar *ool = new OutOfLinePropagateAbortPar(lir);
    if (!ool || !addOutOfLineCode(ool))
        return nullptr;
    return ool;
}

bool
OutOfLineAbortPar::generate(CodeGeneratorShared *codegen)
{
    codegen->callTraceLIR(0xDEADBEEF, nullptr, "AbortPar");
    return codegen->visitOutOfLineAbortPar(this);
}

bool
OutOfLinePropagateAbortPar::generate(CodeGeneratorShared *codegen)
{
    codegen->callTraceLIR(0xDEADBEEF, nullptr, "AbortPar");
    return codegen->visitOutOfLinePropagateAbortPar(this);
}

bool
CodeGeneratorShared::callTraceLIR(uint32_t blockIndex, LInstruction *lir,
                                  const char *bailoutName)
{
    JS_ASSERT_IF(!lir, bailoutName);

    if (!IonSpewEnabled(IonSpew_Trace))
        return true;

    uint32_t execMode = (uint32_t) gen->info().executionMode();
    uint32_t lirIndex;
    const char *lirOpName;
    const char *mirOpName;
    JSScript *script;
    jsbytecode *pc;

    masm.PushRegsInMask(RegisterSet::Volatile());
    masm.reserveStack(sizeof(IonLIRTraceData));

    
    
    
    masm.move32(Imm32(0xDEADBEEF), CallTempReg0);

    if (lir) {
        lirIndex = lir->id();
        lirOpName = lir->opName();
        if (MDefinition *mir = lir->mirRaw()) {
            mirOpName = mir->opName();
            script = mir->block()->info().script();
            pc = mir->trackedPc();
        } else {
            mirOpName = nullptr;
            script = nullptr;
            pc = nullptr;
        }
    } else {
        blockIndex = lirIndex = 0xDEADBEEF;
        lirOpName = mirOpName = bailoutName;
        script = nullptr;
        pc = nullptr;
    }

    masm.store32(Imm32(blockIndex),
                 Address(StackPointer, offsetof(IonLIRTraceData, blockIndex)));
    masm.store32(Imm32(lirIndex),
                 Address(StackPointer, offsetof(IonLIRTraceData, lirIndex)));
    masm.store32(Imm32(execMode),
                 Address(StackPointer, offsetof(IonLIRTraceData, execModeInt)));
    masm.storePtr(ImmPtr(lirOpName),
                  Address(StackPointer, offsetof(IonLIRTraceData, lirOpName)));
    masm.storePtr(ImmPtr(mirOpName),
                  Address(StackPointer, offsetof(IonLIRTraceData, mirOpName)));
    masm.storePtr(ImmGCPtr(script),
                  Address(StackPointer, offsetof(IonLIRTraceData, script)));
    masm.storePtr(ImmPtr(pc),
                  Address(StackPointer, offsetof(IonLIRTraceData, pc)));

    masm.movePtr(StackPointer, CallTempReg0);
    masm.setupUnalignedABICall(1, CallTempReg1);
    masm.passABIArg(CallTempReg0);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, TraceLIR));

    masm.freeStack(sizeof(IonLIRTraceData));
    masm.PopRegsInMask(RegisterSet::Volatile());

    return true;
}

typedef bool (*InterruptCheckFn)(JSContext *);
const VMFunction InterruptCheckInfo = FunctionInfo<InterruptCheckFn>(InterruptCheck);

Label *
CodeGeneratorShared::labelForBackedgeWithImplicitCheck(MBasicBlock *mir)
{
    
    
    
    
    
    if (!gen->compilingAsmJS() && mir->isLoopHeader() && mir->id() <= current->mir()->id()) {
        for (LInstructionIterator iter = mir->lir()->begin(); iter != mir->lir()->end(); iter++) {
            if (iter->isLabel() || iter->isMoveGroup()) {
                
            } else if (iter->isInterruptCheckImplicit()) {
                return iter->toInterruptCheckImplicit()->oolEntry();
            } else {
                
                
                JS_ASSERT(iter->isInterruptCheck() || iter->isCheckInterruptPar());
                return nullptr;
            }
        }
    }

    return nullptr;
}

void
CodeGeneratorShared::jumpToBlock(MBasicBlock *mir)
{
    
    if (isNextBlock(mir->lir()))
        return;

    if (Label *oolEntry = labelForBackedgeWithImplicitCheck(mir)) {
        
        
        RepatchLabel rejoin;
        CodeOffsetJump backedge = masm.jumpWithPatch(&rejoin);
        masm.bind(&rejoin);

        if (!patchableBackedges_.append(PatchableBackedgeInfo(backedge, mir->lir()->label(), oolEntry)))
            MOZ_CRASH();
    } else {
        masm.jump(mir->lir()->label());
    }
}

void
CodeGeneratorShared::jumpToBlock(MBasicBlock *mir, Assembler::Condition cond)
{
    if (Label *oolEntry = labelForBackedgeWithImplicitCheck(mir)) {
        
        
        RepatchLabel rejoin;
        CodeOffsetJump backedge = masm.jumpWithPatch(&rejoin, cond);
        masm.bind(&rejoin);

        if (!patchableBackedges_.append(PatchableBackedgeInfo(backedge, mir->lir()->label(), oolEntry)))
            MOZ_CRASH();
    } else {
        masm.j(cond, mir->lir()->label());
    }
}

size_t
CodeGeneratorShared::addCacheLocations(const CacheLocationList &locs, size_t *numLocs)
{
    size_t firstIndex = runtimeData_.length();
    size_t numLocations = 0;
    for (CacheLocationList::iterator iter = locs.begin(); iter != locs.end(); iter++) {
        
        
        size_t curIndex = allocateData(sizeof(CacheLocation));
        new (&runtimeData_[curIndex]) CacheLocation(iter->pc, iter->script);
        numLocations++;
    }
    JS_ASSERT(numLocations != 0);
    *numLocs = numLocations;
    return firstIndex;
}

} 
} 
