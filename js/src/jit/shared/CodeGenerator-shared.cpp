





#include "jit/shared/CodeGenerator-shared-inl.h"

#include "mozilla/DebugOnly.h"

#include "jit/CompactBuffer.h"
#include "jit/IonCaches.h"
#include "jit/IonMacroAssembler.h"
#include "jit/JitcodeMap.h"
#include "jit/JitSpewer.h"
#include "jit/MIR.h"
#include "jit/MIRGenerator.h"
#include "jit/ParallelFunctions.h"
#include "vm/TraceLogging.h"

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
    maybeMasm_.emplace();
    return *maybeMasm_;
}

CodeGeneratorShared::CodeGeneratorShared(MIRGenerator *gen, LIRGraph *graph, MacroAssembler *masmArg)
  : oolIns(nullptr),
    maybeMasm_(),
    masm(ensureMasm(masmArg)),
    gen(gen),
    graph(*graph),
    current(nullptr),
    snapshots_(),
    recovers_(),
    deoptTable_(nullptr),
#ifdef DEBUG
    pushedArgs_(0),
#endif
    lastOsiPointOffset_(0),
    nativeToBytecodeMap_(nullptr),
    nativeToBytecodeMapSize_(0),
    nativeToBytecodeTableOffset_(0),
    nativeToBytecodeNumRegions_(0),
    nativeToBytecodeScriptList_(nullptr),
    nativeToBytecodeScriptListLength_(0),
    sps_(&GetIonContext()->runtime->spsProfiler(), &lastNotInlinedPC_),
    osrEntryOffset_(0),
    skipArgCheckEntryOffset_(0),
#ifdef CHECK_OSIPOINT_REGISTERS
    checkOsiPointRegisters(js_JitOptions.checkOsiPointRegisters),
#endif
    frameDepth_(graph->paddedLocalSlotsSize() + graph->argumentsSize()),
    frameInitialAdjustment_(0)
{
    if (!gen->compilingAsmJS())
        masm.setInstrumentation(&sps_);

    if (gen->compilingAsmJS()) {
        
        
        
        JS_ASSERT(graph->argumentSlotCount() == 0);
        frameDepth_ += gen->maxAsmJSStackArgBytes();

        
        
        if (gen->usesSimd()) {
            frameInitialAdjustment_ = ComputeByteAlignment(sizeof(AsmJSFrame), AsmJSStackAlignment);
            frameDepth_ += frameInitialAdjustment_;
        }

        
        
        
        if (gen->performsCall())
            frameDepth_ += ComputeByteAlignment(sizeof(AsmJSFrame) + frameDepth_, AsmJSStackAlignment);

        
        
        frameClass_ = FrameSizeClass::None();
    } else {
        frameClass_ = FrameSizeClass::FromDepth(frameDepth_);
    }
}

bool
CodeGeneratorShared::generateOutOfLineCode()
{
    JSScript *topScript = sps_.getPushed();
    for (size_t i = 0; i < outOfLineCode_.length(); i++) {
        
        
        if (!gen->compilingAsmJS()) {
            if (!addNativeToBytecodeEntry(outOfLineCode_[i]->bytecodeSite()))
                return false;
        }

        if (!gen->alloc().ensureBallast())
            return false;

        JitSpew(JitSpew_Codegen, "# Emitting out of line code");

        masm.setFramePushed(outOfLineCode_[i]->framePushed());
        lastPC_ = outOfLineCode_[i]->pc();
        if (!sps_.prepareForOOL())
            return false;
        sps_.setPushed(outOfLineCode_[i]->script());
        outOfLineCode_[i]->bind(&masm);

        oolIns = outOfLineCode_[i];
        if (!outOfLineCode_[i]->generate(this))
            return false;
        sps_.finishOOL();
    }
    sps_.setPushed(topScript);
    oolIns = nullptr;

    return true;
}

bool
CodeGeneratorShared::addOutOfLineCode(OutOfLineCode *code, const MInstruction *mir)
{
    JS_ASSERT(mir);
    return addOutOfLineCode(code, mir->trackedSite());
}

bool
CodeGeneratorShared::addOutOfLineCode(OutOfLineCode *code, const BytecodeSite &site)
{
    code->setFramePushed(masm.framePushed());
    code->setBytecodeSite(site);
    JS_ASSERT_IF(!gen->compilingAsmJS(), code->script()->containsPC(code->pc()));
    return outOfLineCode_.append(code);
}

bool
CodeGeneratorShared::addNativeToBytecodeEntry(const BytecodeSite &site)
{
    
    if (!isNativeToBytecodeMapEnabled())
        return true;

    JS_ASSERT(site.tree());
    JS_ASSERT(site.pc());

    InlineScriptTree *tree = site.tree();
    jsbytecode *pc = site.pc();
    uint32_t nativeOffset = masm.currentOffset();

    JS_ASSERT_IF(nativeToBytecodeList_.empty(), nativeOffset == 0);

    if (!nativeToBytecodeList_.empty()) {
        size_t lastIdx = nativeToBytecodeList_.length() - 1;
        NativeToBytecode &lastEntry = nativeToBytecodeList_[lastIdx];

        JS_ASSERT(nativeOffset >= lastEntry.nativeOffset.offset());

        
        
        
        if (lastEntry.tree == tree && lastEntry.pc == pc) {
            JitSpew(JitSpew_Profiling, " => In-place update [%u-%u]",
                    lastEntry.nativeOffset.offset(), nativeOffset);
            return true;
        }

        
        
        
        if (lastEntry.nativeOffset.offset() == nativeOffset) {
            lastEntry.tree = tree;
            lastEntry.pc = pc;
            JitSpew(JitSpew_Profiling, " => Overwriting zero-length native region.");

            
            
            if (lastIdx > 0) {
                NativeToBytecode &nextToLastEntry = nativeToBytecodeList_[lastIdx - 1];
                if (nextToLastEntry.tree == lastEntry.tree && nextToLastEntry.pc == lastEntry.pc) {
                    JitSpew(JitSpew_Profiling, " => Merging with previous region");
                    nativeToBytecodeList_.erase(&lastEntry);
                }
            }

            dumpNativeToBytecodeEntry(nativeToBytecodeList_.length() - 1);
            return true;
        }
    }

    
    
    NativeToBytecode entry;
    entry.nativeOffset = CodeOffsetLabel(nativeOffset);
    entry.tree = tree;
    entry.pc = pc;
    if (!nativeToBytecodeList_.append(entry))
        return false;

    JitSpew(JitSpew_Profiling, " => Push new entry.");
    dumpNativeToBytecodeEntry(nativeToBytecodeList_.length() - 1);
    return true;
}

void
CodeGeneratorShared::dumpNativeToBytecodeEntries()
{
#ifdef DEBUG
    InlineScriptTree *topTree = gen->info().inlineScriptTree();
    JitSpewStart(JitSpew_Profiling, "Native To Bytecode Entries for %s:%d\n",
                 topTree->script()->filename(), topTree->script()->lineno());
    for (unsigned i = 0; i < nativeToBytecodeList_.length(); i++)
        dumpNativeToBytecodeEntry(i);
#endif
}

void
CodeGeneratorShared::dumpNativeToBytecodeEntry(uint32_t idx)
{
#ifdef DEBUG
    NativeToBytecode &ref = nativeToBytecodeList_[idx];
    InlineScriptTree *tree = ref.tree;
    JSScript *script = tree->script();
    uint32_t nativeOffset = ref.nativeOffset.offset();
    unsigned nativeDelta = 0;
    unsigned pcDelta = 0;
    if (idx + 1 < nativeToBytecodeList_.length()) {
        NativeToBytecode *nextRef = &ref + 1;
        nativeDelta = nextRef->nativeOffset.offset() - nativeOffset;
        if (nextRef->tree == ref.tree)
            pcDelta = nextRef->pc - ref.pc;
    }
    JitSpewStart(JitSpew_Profiling, "    %08x [+%-6d] => %-6d [%-4d] {%-10s} (%s:%d",
                 ref.nativeOffset.offset(),
                 nativeDelta,
                 ref.pc - script->code(),
                 pcDelta,
                 js_CodeName[JSOp(*ref.pc)],
                 script->filename(), script->lineno());

    for (tree = tree->caller(); tree; tree = tree->caller()) {
        JitSpewCont(JitSpew_Profiling, " <= %s:%d", tree->script()->filename(),
                                                    tree->script()->lineno());
    }
    JitSpewCont(JitSpew_Profiling, ")");
    JitSpewFin(JitSpew_Profiling);
#endif
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
CodeGeneratorShared::encodeAllocation(LSnapshot *snapshot, MDefinition *mir,
                                      uint32_t *allocIndex)
{
    if (mir->isBox())
        mir = mir->toBox()->getOperand(0);

    MIRType type =
        mir->isRecoveredOnBailout() ? MIRType_None :
        mir->isUnused() ? MIRType_MagicOptimizedOut :
        mir->type();

    RValueAllocation alloc;

    switch (type) {
      case MIRType_None:
      {
        MOZ_ASSERT(mir->isRecoveredOnBailout());
        uint32_t index = 0;
        LRecoverInfo *recoverInfo = snapshot->recoverInfo();
        MNode **it = recoverInfo->begin(), **end = recoverInfo->end();
        while (it != end && mir != *it) {
            ++it;
            ++index;
        }

        
        
        MOZ_ASSERT(it != end && mir == *it);
        alloc = RValueAllocation::RecoverInstruction(index);
        break;
      }
      case MIRType_Undefined:
        alloc = RValueAllocation::Undefined();
        break;
      case MIRType_Null:
        alloc = RValueAllocation::Null();
        break;
      case MIRType_Int32:
      case MIRType_String:
      case MIRType_Symbol:
      case MIRType_Object:
      case MIRType_Boolean:
      case MIRType_Double:
      case MIRType_Float32:
      {
        LAllocation *payload = snapshot->payloadOfSlot(*allocIndex);
        JSValueType valueType = ValueTypeFromMIRType(type);
        if (payload->isMemory()) {
            if (type == MIRType_Float32)
                alloc = RValueAllocation::Float32(ToStackIndex(payload));
            else
                alloc = RValueAllocation::Typed(valueType, ToStackIndex(payload));
        } else if (payload->isGeneralReg()) {
            alloc = RValueAllocation::Typed(valueType, ToRegister(payload));
        } else if (payload->isFloatReg()) {
            FloatRegister reg = ToFloatRegister(payload);
            if (type == MIRType_Float32)
                alloc = RValueAllocation::Float32(reg);
            else
                alloc = RValueAllocation::Double(reg);
        } else {
            MConstant *constant = mir->toConstant();
            uint32_t index;
            if (!graph.addConstantToPool(constant->value(), &index))
                return false;
            alloc = RValueAllocation::ConstantPool(index);
        }
        break;
      }
      case MIRType_MagicOptimizedArguments:
      case MIRType_MagicOptimizedOut:
      {
        uint32_t index;
        JSWhyMagic why = (type == MIRType_MagicOptimizedArguments
                          ? JS_OPTIMIZED_ARGUMENTS
                          : JS_OPTIMIZED_OUT);
        Value v = MagicValue(why);
        if (!graph.addConstantToPool(v, &index))
            return false;
        alloc = RValueAllocation::ConstantPool(index);
        break;
      }
      default:
      {
        JS_ASSERT(mir->type() == MIRType_Value);
        LAllocation *payload = snapshot->payloadOfSlot(*allocIndex);
#ifdef JS_NUNBOX32
        LAllocation *type = snapshot->typeOfSlot(*allocIndex);
        if (type->isRegister()) {
            if (payload->isRegister())
                alloc = RValueAllocation::Untyped(ToRegister(type), ToRegister(payload));
            else
                alloc = RValueAllocation::Untyped(ToRegister(type), ToStackIndex(payload));
        } else {
            if (payload->isRegister())
                alloc = RValueAllocation::Untyped(ToStackIndex(type), ToRegister(payload));
            else
                alloc = RValueAllocation::Untyped(ToStackIndex(type), ToStackIndex(payload));
        }
#elif JS_PUNBOX64
        if (payload->isRegister())
            alloc = RValueAllocation::Untyped(ToRegister(payload));
        else
            alloc = RValueAllocation::Untyped(ToStackIndex(payload));
#endif
        break;
      }
    }

    snapshots_.add(alloc);
    *allocIndex += mir->isRecoveredOnBailout() ? 0 : 1;
    return true;
}

bool
CodeGeneratorShared::encode(LRecoverInfo *recover)
{
    if (recover->recoverOffset() != INVALID_RECOVER_OFFSET)
        return true;

    uint32_t numInstructions = recover->numInstructions();
    JitSpew(JitSpew_IonSnapshots, "Encoding LRecoverInfo %p (frameCount %u, instructions %u)",
            (void *)recover, recover->mir()->frameCount(), numInstructions);

    MResumePoint::Mode mode = recover->mir()->mode();
    JS_ASSERT(mode != MResumePoint::Outer);
    bool resumeAfter = (mode == MResumePoint::ResumeAfter);

    RecoverOffset offset = recovers_.startRecover(numInstructions, resumeAfter);

    for (MNode **it = recover->begin(), **end = recover->end(); it != end; ++it) {
        if (!recovers_.writeInstruction(*it))
            return false;
    }

    recovers_.endRecover();
    recover->setRecoverOffset(offset);
    return !recovers_.oom();
}

bool
CodeGeneratorShared::encode(LSnapshot *snapshot)
{
    if (snapshot->snapshotOffset() != INVALID_SNAPSHOT_OFFSET)
        return true;

    LRecoverInfo *recoverInfo = snapshot->recoverInfo();
    if (!encode(recoverInfo))
        return false;

    RecoverOffset recoverOffset = recoverInfo->recoverOffset();
    MOZ_ASSERT(recoverOffset != INVALID_RECOVER_OFFSET);

    JitSpew(JitSpew_IonSnapshots, "Encoding LSnapshot %p (LRecover %p)",
            (void *)snapshot, (void*) recoverInfo);

    SnapshotOffset offset = snapshots_.startSnapshot(recoverOffset, snapshot->bailoutKind());

#ifdef TRACK_SNAPSHOTS
    uint32_t pcOpcode = 0;
    uint32_t lirOpcode = 0;
    uint32_t lirId = 0;
    uint32_t mirOpcode = 0;
    uint32_t mirId = 0;

    if (LInstruction *ins = instruction()) {
        lirOpcode = ins->op();
        lirId = ins->id();
        if (ins->mirRaw()) {
            mirOpcode = ins->mirRaw()->op();
            mirId = ins->mirRaw()->id();
            if (ins->mirRaw()->trackedPc())
                pcOpcode = *ins->mirRaw()->trackedPc();
        }
    }
    snapshots_.trackSnapshot(pcOpcode, mirOpcode, mirId, lirOpcode, lirId);
#endif

    uint32_t allocIndex = 0;
    for (LRecoverInfo::OperandIter it(recoverInfo); !it; ++it) {
        DebugOnly<uint32_t> allocWritten = snapshots_.allocWritten();
        if (!encodeAllocation(snapshot, *it, &allocIndex))
            return false;
        MOZ_ASSERT(allocWritten + 1 == snapshots_.allocWritten());
    }

    MOZ_ASSERT(allocIndex == snapshot->numSlots());
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

    
    switch (gen->info().executionMode()) {
      case SequentialExecution: break;
      case ParallelExecution: return false;
      default: MOZ_CRASH("No such execution mode");
    }

    JS_ASSERT(frameClass_ != FrameSizeClass::None());

    if (snapshot->bailoutId() != INVALID_BAILOUT_ID)
        return true;

    
    if (bailouts_.length() >= BAILOUT_TABLE_SIZE)
        return false;

    unsigned bailoutId = bailouts_.length();
    snapshot->setBailoutId(bailoutId);
    JitSpew(JitSpew_IonSnapshots, "Assigned snapshot bailout id %u", bailoutId);
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
CodeGeneratorShared::createNativeToBytecodeScriptList(JSContext *cx)
{
    js::Vector<JSScript *, 0, SystemAllocPolicy> scriptList;
    InlineScriptTree *tree = gen->info().inlineScriptTree();
    for (;;) {
        
        bool found = false;
        for (uint32_t i = 0; i < scriptList.length(); i++) {
            if (scriptList[i] == tree->script()) {
                found = true;
                break;
            }
        }
        if (!found) {
            if (!scriptList.append(tree->script()))
                return false;
        }

        

        
        if (tree->hasChildren()) {
            tree = tree->firstChild();
            continue;
        }

        
        
        while (!tree->hasNextCallee() && tree->hasCaller())
            tree = tree->caller();

        
        if (tree->hasNextCallee()) {
            tree = tree->nextCallee();
            continue;
        }

        
        JS_ASSERT(tree->isOutermostCaller());
        break;
    }

    
    JSScript **data = cx->runtime()->pod_malloc<JSScript *>(scriptList.length());
    if (!data)
        return false;

    for (uint32_t i = 0; i < scriptList.length(); i++)
        data[i] = scriptList[i];

    
    nativeToBytecodeScriptListLength_ = scriptList.length();
    nativeToBytecodeScriptList_ = data;
    return true;
}

bool
CodeGeneratorShared::generateCompactNativeToBytecodeMap(JSContext *cx, JitCode *code)
{
    JS_ASSERT(nativeToBytecodeScriptListLength_ == 0);
    JS_ASSERT(nativeToBytecodeScriptList_ == nullptr);
    JS_ASSERT(nativeToBytecodeMap_ == nullptr);
    JS_ASSERT(nativeToBytecodeMapSize_ == 0);
    JS_ASSERT(nativeToBytecodeTableOffset_ == 0);
    JS_ASSERT(nativeToBytecodeNumRegions_ == 0);

    
    for (unsigned i = 0; i < nativeToBytecodeList_.length(); i++) {
        NativeToBytecode &entry = nativeToBytecodeList_[i];

        
        entry.nativeOffset = CodeOffsetLabel(masm.actualOffset(entry.nativeOffset.offset()));
    }

    if (!createNativeToBytecodeScriptList(cx))
        return false;

    JS_ASSERT(nativeToBytecodeScriptListLength_ > 0);
    JS_ASSERT(nativeToBytecodeScriptList_ != nullptr);

    CompactBufferWriter writer;
    uint32_t tableOffset = 0;
    uint32_t numRegions = 0;

    if (!JitcodeIonTable::WriteIonTable(
            writer, nativeToBytecodeScriptList_, nativeToBytecodeScriptListLength_,
            &nativeToBytecodeList_[0],
            &nativeToBytecodeList_[0] + nativeToBytecodeList_.length(),
            &tableOffset, &numRegions))
    {
        return false;
    }

    JS_ASSERT(tableOffset > 0);
    JS_ASSERT(numRegions > 0);

    
    uint8_t *data = cx->runtime()->pod_malloc<uint8_t>(writer.length());
    if (!data)
        return false;

    memcpy(data, writer.buffer(), writer.length());
    nativeToBytecodeMap_ = data;
    nativeToBytecodeMapSize_ = writer.length();
    nativeToBytecodeTableOffset_ = tableOffset;
    nativeToBytecodeNumRegions_ = numRegions;

    verifyCompactNativeToBytecodeMap(code);

    JitSpew(JitSpew_Profiling, "Compact Native To Bytecode Map [%p-%p]",
            data, data + nativeToBytecodeMapSize_);

    return true;
}

void
CodeGeneratorShared::verifyCompactNativeToBytecodeMap(JitCode *code)
{
#ifdef DEBUG
    JS_ASSERT(nativeToBytecodeScriptListLength_ > 0);
    JS_ASSERT(nativeToBytecodeScriptList_ != nullptr);
    JS_ASSERT(nativeToBytecodeMap_ != nullptr);
    JS_ASSERT(nativeToBytecodeMapSize_ > 0);
    JS_ASSERT(nativeToBytecodeTableOffset_ > 0);
    JS_ASSERT(nativeToBytecodeNumRegions_ > 0);

    
    const uint8_t *tablePtr = nativeToBytecodeMap_ + nativeToBytecodeTableOffset_;
    JS_ASSERT(uintptr_t(tablePtr) % sizeof(uint32_t) == 0);

    
    const JitcodeIonTable *ionTable = reinterpret_cast<const JitcodeIonTable *>(tablePtr);
    JS_ASSERT(ionTable->numRegions() == nativeToBytecodeNumRegions_);

    
    
    
    
    JS_ASSERT(ionTable->regionOffset(0) == nativeToBytecodeTableOffset_);

    
    for (uint32_t i = 0; i < ionTable->numRegions(); i++) {
        
        JS_ASSERT(ionTable->regionOffset(i) <= nativeToBytecodeTableOffset_);

        
        
        JS_ASSERT_IF(i > 0, ionTable->regionOffset(i) < ionTable->regionOffset(i - 1));

        JitcodeRegionEntry entry = ionTable->regionEntry(i);

        
        JS_ASSERT(entry.nativeOffset() <= code->instructionsSize());

        
        JitcodeRegionEntry::ScriptPcIterator scriptPcIter = entry.scriptPcIterator();
        while (scriptPcIter.hasMore()) {
            uint32_t scriptIdx = 0, pcOffset = 0;
            scriptPcIter.readNext(&scriptIdx, &pcOffset);

            
            JS_ASSERT(scriptIdx < nativeToBytecodeScriptListLength_);
            JSScript *script = nativeToBytecodeScriptList_[scriptIdx];

            
            JS_ASSERT(pcOffset < script->length());
        }

        
        uint32_t curNativeOffset = entry.nativeOffset();
        JSScript *script = nullptr;
        uint32_t curPcOffset = 0;
        {
            uint32_t scriptIdx = 0;
            scriptPcIter.reset();
            scriptPcIter.readNext(&scriptIdx, &curPcOffset);
            script = nativeToBytecodeScriptList_[scriptIdx];
        }

        
        JitcodeRegionEntry::DeltaIterator deltaIter = entry.deltaIterator();
        while (deltaIter.hasMore()) {
            uint32_t nativeDelta = 0;
            int32_t pcDelta = 0;
            deltaIter.readNext(&nativeDelta, &pcDelta);

            curNativeOffset += nativeDelta;
            curPcOffset = uint32_t(int32_t(curPcOffset) + pcDelta);

            
            JS_ASSERT(curNativeOffset <= code->instructionsSize());

            
            JS_ASSERT(curPcOffset < script->length());
        }
    }
#endif 
}

bool
CodeGeneratorShared::markSafepoint(LInstruction *ins)
{
    return markSafepointAt(masm.currentOffset(), ins);
}

bool
CodeGeneratorShared::markSafepointAt(uint32_t offset, LInstruction *ins)
{
    JS_ASSERT_IF(!safepointIndices_.empty(),
                 offset - safepointIndices_.back().displacement() >= sizeof(uint32_t));
    return safepointIndices_.append(SafepointIndex(offset, ins->safepoint()));
}

void
CodeGeneratorShared::ensureOsiSpace()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (masm.currentOffset() - lastOsiPointOffset_ < Assembler::PatchWrite_NearCallSize()) {
        int32_t paddingSize = Assembler::PatchWrite_NearCallSize();
        paddingSize -= masm.currentOffset() - lastOsiPointOffset_;
        for (int32_t i = 0; i < paddingSize; ++i)
            masm.nop();
    }
    JS_ASSERT(masm.currentOffset() - lastOsiPointOffset_ >= Assembler::PatchWrite_NearCallSize());
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
    explicit StoreOp(MacroAssembler &masm)
      : masm(masm)
    {}

    void operator()(Register reg, Address dump) {
        masm.storePtr(reg, dump);
    }
    void operator()(FloatRegister reg, Address dump) {
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
        if (reg.isDouble()) {
            masm.storeDouble(reg, dump);
        } else {
            masm.storeFloat32(reg, dump);
        }
#else
        masm.storeDouble(reg, dump);
#endif
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
        FloatRegister scratch;
#if defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
        if (reg.isDouble()) {
            scratch = ScratchDoubleReg;
            masm.loadDouble(dump, scratch);
            masm.branchDouble(Assembler::DoubleNotEqual, scratch, reg, failure_);
        } else {
            scratch = ScratchFloat32Reg;
            masm.loadFloat32(dump, scratch);
            masm.branchFloat(Assembler::DoubleNotEqual, scratch, reg, failure_);
        }
#else
        scratch = ScratchFloat32Reg;
        masm.loadDouble(dump, scratch);
        masm.branchDouble(Assembler::DoubleNotEqual, scratch, reg, failure_);
#endif
    }
};

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
    liveRegs = RegisterSet::Intersect(liveRegs, RegisterSet::Not(safepoint->clobberedRegs()));

    VerifyOp op(masm, &failure);
    HandleRegisterDump<VerifyOp>(op, masm, liveRegs, scratch, allRegs.getAny());

    masm.jump(&done);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    masm.bind(&failure);
    masm.assumeUnreachable("Modified registers between VM call and OsiPoint");

    masm.bind(&done);
    masm.pop(scratch);
}

bool
CodeGeneratorShared::shouldVerifyOsiPointRegs(LSafepoint *safepoint)
{
    if (!checkOsiPointRegisters)
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

    
    
    JS_ASSERT_IF(fun.outParam == Type_Double, GetIonContext()->runtime->jitSupportsFloatingPoint());

#ifdef DEBUG
    if (ins->mirRaw()) {
        JS_ASSERT(ins->mirRaw()->isInstruction());
        MInstruction *mir = ins->mirRaw()->toInstruction();
        JS_ASSERT_IF(mir->needsResumePoint(), mir->resumePoint());
    }
#endif

#ifdef JS_TRACE_LOGGING
    if (!emitTracelogStartEvent(TraceLogger::VM))
        return false;
#endif

    
    
    
#ifdef DEBUG
    JS_ASSERT(pushedArgs_ == fun.explicitArgs);
    pushedArgs_ = 0;
#endif

    
    JitCode *wrapper = gen->jitRuntime()->getVMWrapper(fun);
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
    
    

#ifdef JS_TRACE_LOGGING
    if (!emitTracelogStopEvent(TraceLogger::VM))
        return false;
#endif

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
CodeGeneratorShared::oolTruncateDouble(FloatRegister src, Register dest, MInstruction *mir)
{
    OutOfLineTruncateSlow *ool = new(alloc()) OutOfLineTruncateSlow(src, dest);
    if (!addOutOfLineCode(ool, mir))
        return nullptr;
    return ool;
}

bool
CodeGeneratorShared::emitTruncateDouble(FloatRegister src, Register dest, MInstruction *mir)
{
    OutOfLineCode *ool = oolTruncateDouble(src, dest, mir);
    if (!ool)
        return false;

    masm.branchTruncateDouble(src, dest, ool->entry());
    masm.bind(ool->rejoin());
    return true;
}

bool
CodeGeneratorShared::emitTruncateFloat32(FloatRegister src, Register dest, MInstruction *mir)
{
    OutOfLineTruncateSlow *ool = new(alloc()) OutOfLineTruncateSlow(src, dest, true);
    if (!addOutOfLineCode(ool, mir))
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
#ifdef JS_CODEGEN_ARM
    if (ool->needFloat32Conversion()) {
        masm.convertFloat32ToDouble(src, ScratchDoubleReg);
        src = ScratchDoubleReg;
    }

#else
    if (ool->needFloat32Conversion()) {
        masm.push(src);
        masm.convertFloat32ToDouble(src, src);
    }
#endif
    masm.setupUnalignedABICall(1, dest);
    masm.passABIArg(src, MoveOp::DOUBLE);
    if (gen->compilingAsmJS())
        masm.callWithABI(AsmJSImm_ToInt32);
    else
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::ToInt32));
    masm.storeCallResult(dest);

#ifndef JS_CODEGEN_ARM
    if (ool->needFloat32Conversion())
        masm.pop(src);
#endif
    restoreVolatile(dest);

    masm.jump(ool->rejoin());
    return true;
}

bool
CodeGeneratorShared::omitOverRecursedCheck() const
{
    
    
    
    
    
    return frameSize() < 64 && !gen->performsCall();
}

void
CodeGeneratorShared::emitPreBarrier(Register base, const LAllocation *index)
{
    if (index->isConstant()) {
        Address address(base, ToInt32(index) * sizeof(Value));
        masm.patchableCallPreBarrier(address, MIRType_Value);
    } else {
        BaseIndex address(base, ToRegister(index), TimesEight);
        masm.patchableCallPreBarrier(address, MIRType_Value);
    }
}

void
CodeGeneratorShared::emitPreBarrier(Address address)
{
    masm.patchableCallPreBarrier(address, MIRType_Value);
}

void
CodeGeneratorShared::dropArguments(unsigned argc)
{
    pushedArgumentSlots_.shrinkBy(argc);
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

Label *
CodeGeneratorShared::labelForBackedgeWithImplicitCheck(MBasicBlock *mir)
{
    
    
    
    
    
    if (!gen->compilingAsmJS() && mir->isLoopHeader() && mir->id() <= current->mir()->id()) {
        for (LInstructionIterator iter = mir->lir()->begin(); iter != mir->lir()->end(); iter++) {
            if (iter->isLabel() || iter->isMoveGroup()) {
                
            } else if (iter->isInterruptCheckImplicit()) {
                return iter->toInterruptCheckImplicit()->oolEntry();
            } else {
                
                
                JS_ASSERT(iter->isInterruptCheck() || iter->isInterruptCheckPar());
                return nullptr;
            }
        }
    }

    return nullptr;
}

void
CodeGeneratorShared::jumpToBlock(MBasicBlock *mir)
{
    
    mir = skipTrivialBlocks(mir);

    
    if (isNextBlock(mir->lir()))
        return;

    if (Label *oolEntry = labelForBackedgeWithImplicitCheck(mir)) {
        
        
        RepatchLabel rejoin;
        CodeOffsetJump backedge = masm.backedgeJump(&rejoin);
        masm.bind(&rejoin);

        masm.propagateOOM(patchableBackedges_.append(PatchableBackedgeInfo(backedge, mir->lir()->label(), oolEntry)));
    } else {
        masm.jump(mir->lir()->label());
    }
}


#ifndef JS_CODEGEN_MIPS
void
CodeGeneratorShared::jumpToBlock(MBasicBlock *mir, Assembler::Condition cond)
{
    
    mir = skipTrivialBlocks(mir);

    if (Label *oolEntry = labelForBackedgeWithImplicitCheck(mir)) {
        
        
        RepatchLabel rejoin;
        CodeOffsetJump backedge = masm.jumpWithPatch(&rejoin, cond);
        masm.bind(&rejoin);

        masm.propagateOOM(patchableBackedges_.append(PatchableBackedgeInfo(backedge, mir->lir()->label(), oolEntry)));
    } else {
        masm.j(cond, mir->lir()->label());
    }
}
#endif

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

ReciprocalMulConstants
CodeGeneratorShared::computeDivisionConstants(int d) {
    
    JS_ASSERT(d > 0 && (d & (d - 1)) != 0);

    
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    

    int32_t shift = 0;
    while ((int64_t(1) << (shift+1)) + (int64_t(1) << (shift+32)) % d < d)
        shift++;

    
    
    
    ReciprocalMulConstants rmc;
    rmc.multiplier = int32_t((int64_t(1) << (shift+32))/d + 1);
    rmc.shiftAmount = shift;

    return rmc;
}


#ifdef JS_TRACE_LOGGING

bool
CodeGeneratorShared::emitTracelogScript(bool isStart)
{
    Label done;

    RegisterSet regs = RegisterSet::Volatile();
    Register logger = regs.takeGeneral();
    Register script = regs.takeGeneral();

    masm.Push(logger);

    CodeOffsetLabel patchLogger = masm.movWithPatch(ImmPtr(nullptr), logger);
    if (!patchableTraceLoggers_.append(patchLogger))
        return false;

    Address enabledAddress(logger, TraceLogger::offsetOfEnabled());
    masm.branch32(Assembler::Equal, enabledAddress, Imm32(0), &done);

    masm.Push(script);

    CodeOffsetLabel patchScript = masm.movWithPatch(ImmWord(0), script);
    if (!patchableTLScripts_.append(patchScript))
        return false;

    if (isStart)
        masm.tracelogStart(logger, script);
    else
        masm.tracelogStop(logger, script);

    masm.Pop(script);

    masm.bind(&done);

    masm.Pop(logger);
    return true;
}

bool
CodeGeneratorShared::emitTracelogTree(bool isStart, uint32_t textId)
{
    if (!TraceLogTextIdEnabled(textId))
        return true;

    Label done;
    RegisterSet regs = RegisterSet::Volatile();
    Register logger = regs.takeGeneral();

    masm.Push(logger);

    CodeOffsetLabel patchLocation = masm.movWithPatch(ImmPtr(nullptr), logger);
    if (!patchableTraceLoggers_.append(patchLocation))
        return false;

    Address enabledAddress(logger, TraceLogger::offsetOfEnabled());
    masm.branch32(Assembler::Equal, enabledAddress, Imm32(0), &done);

    if (isStart) {
        masm.tracelogStart(logger, textId);
    } else {
#ifdef DEBUG
        masm.tracelogStop(logger, textId);
#else
        masm.tracelogStop(logger);
#endif
    }

    masm.bind(&done);

    masm.Pop(logger);
    return true;
}
#endif

} 
} 
