





#include "jit/BaselineCompiler.h"

#include "jit/BaselineHelpers.h"
#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"
#include "jit/FixedList.h"
#include "jit/IonAnalysis.h"
#include "jit/IonLinker.h"
#include "jit/JitcodeMap.h"
#include "jit/JitSpewer.h"
#ifdef JS_ION_PERF
# include "jit/PerfSpewer.h"
#endif
#include "jit/VMFunctions.h"
#include "vm/TraceLogging.h"

#include "jsscriptinlines.h"

#include "vm/Interpreter-inl.h"

using namespace js;
using namespace js::jit;

BaselineCompiler::BaselineCompiler(JSContext *cx, TempAllocator &alloc, JSScript *script)
  : BaselineCompilerSpecific(cx, alloc, script),
    modifiesArguments_(false)
{
}

bool
BaselineCompiler::init()
{
    if (!analysis_.init(alloc_, cx->runtime()->gsnCache))
        return false;

    if (!labels_.init(alloc_, script->length()))
        return false;

    for (size_t i = 0; i < script->length(); i++)
        new (&labels_[i]) Label();

    if (!frame.init(alloc_))
        return false;

    return true;
}

bool
BaselineCompiler::addPCMappingEntry(bool addIndexEntry)
{
    
    size_t nentries = pcMappingEntries_.length();
    if (nentries > 0 && pcMappingEntries_[nentries - 1].pcOffset == script->pcToOffset(pc))
        return true;

    PCMappingEntry entry;
    entry.pcOffset = script->pcToOffset(pc);
    entry.nativeOffset = masm.currentOffset();
    entry.slotInfo = getStackTopSlotInfo();
    entry.addIndexEntry = addIndexEntry;

    return pcMappingEntries_.append(entry);
}

MethodStatus
BaselineCompiler::compile()
{
    JitSpew(JitSpew_BaselineScripts, "Baseline compiling script %s:%d (%p)",
            script->filename(), script->lineno(), script);

    JitSpew(JitSpew_Codegen, "# Emitting baseline code for script %s:%d",
            script->filename(), script->lineno());

    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());
    AutoTraceLog logScript(logger, TraceLogCreateTextId(logger, script));
    AutoTraceLog logCompile(logger, TraceLogger::BaselineCompilation);

    if (!script->ensureHasTypes(cx) || !script->ensureHasAnalyzedArgsUsage(cx))
        return Method_Error;

    
    types::AutoEnterAnalysis autoEnterAnalysis(cx);

    JS_ASSERT(!script->hasBaselineScript());

    if (!emitPrologue())
        return Method_Error;

    MethodStatus status = emitBody();
    if (status != Method_Compiled)
        return status;

    if (!emitEpilogue())
        return Method_Error;

#ifdef JSGC_GENERATIONAL
    if (!emitOutOfLinePostBarrierSlot())
        return Method_Error;
#endif

    if (masm.oom())
        return Method_Error;

    Linker linker(masm);
    AutoFlushICache afc("Baseline");
    JitCode *code = linker.newCode<CanGC>(cx, BASELINE_CODE);
    if (!code)
        return Method_Error;

    JSObject *templateScope = nullptr;
    if (script->functionNonDelazifying()) {
        RootedFunction fun(cx, script->functionNonDelazifying());
        if (fun->isHeavyweight()) {
            RootedScript scriptRoot(cx, script);
            templateScope = CallObject::createTemplateObject(cx, scriptRoot, gc::TenuredHeap);
            if (!templateScope)
                return Method_Error;

            if (fun->isNamedLambda()) {
                RootedObject declEnvObject(cx, DeclEnvObject::createTemplateObject(cx, fun, gc::TenuredHeap));
                if (!declEnvObject)
                    return Method_Error;
                templateScope->as<ScopeObject>().setEnclosingScope(declEnvObject);
            }
        }
    }

    
    
    Vector<PCMappingIndexEntry> pcMappingIndexEntries(cx);
    CompactBufferWriter pcEntries;
    uint32_t previousOffset = 0;

    for (size_t i = 0; i < pcMappingEntries_.length(); i++) {
        PCMappingEntry &entry = pcMappingEntries_[i];
        entry.fixupNativeOffset(masm);

        if (entry.addIndexEntry) {
            PCMappingIndexEntry indexEntry;
            indexEntry.pcOffset = entry.pcOffset;
            indexEntry.nativeOffset = entry.nativeOffset;
            indexEntry.bufferOffset = pcEntries.length();
            if (!pcMappingIndexEntries.append(indexEntry))
                return Method_Error;
            previousOffset = entry.nativeOffset;
        }

        
        
        
        JS_ASSERT((entry.slotInfo.toByte() & 0x80) == 0);

        if (entry.nativeOffset == previousOffset) {
            pcEntries.writeByte(entry.slotInfo.toByte());
        } else {
            JS_ASSERT(entry.nativeOffset > previousOffset);
            pcEntries.writeByte(0x80 | entry.slotInfo.toByte());
            pcEntries.writeUnsigned(entry.nativeOffset - previousOffset);
        }

        previousOffset = entry.nativeOffset;
    }

    if (pcEntries.oom())
        return Method_Error;

    prologueOffset_.fixup(&masm);
    epilogueOffset_.fixup(&masm);
    spsPushToggleOffset_.fixup(&masm);
    postDebugPrologueOffset_.fixup(&masm);

    
    size_t bytecodeTypeMapEntries = script->nTypeSets() + 1;

    BaselineScript *baselineScript = BaselineScript::New(script, prologueOffset_.offset(),
                                                         epilogueOffset_.offset(),
                                                         spsPushToggleOffset_.offset(),
                                                         postDebugPrologueOffset_.offset(),
                                                         icEntries_.length(),
                                                         pcMappingIndexEntries.length(),
                                                         pcEntries.length(),
                                                         bytecodeTypeMapEntries);
    if (!baselineScript)
        return Method_Error;

    baselineScript->setMethod(code);
    baselineScript->setTemplateScope(templateScope);

    JitSpew(JitSpew_BaselineScripts, "Created BaselineScript %p (raw %p) for %s:%d",
            (void *) baselineScript, (void *) code->raw(),
            script->filename(), script->lineno());

#ifdef JS_ION_PERF
    writePerfSpewerBaselineProfile(script, code);
#endif

    JS_ASSERT(pcMappingIndexEntries.length() > 0);
    baselineScript->copyPCMappingIndexEntries(&pcMappingIndexEntries[0]);

    JS_ASSERT(pcEntries.length() > 0);
    baselineScript->copyPCMappingEntries(pcEntries);

    
    if (icEntries_.length())
        baselineScript->copyICEntries(script, &icEntries_[0], masm);

    
    baselineScript->adoptFallbackStubs(&stubSpace_);

    
    for (size_t i = 0; i < icLoadLabels_.length(); i++) {
        CodeOffsetLabel label = icLoadLabels_[i].label;
        label.fixup(&masm);
        size_t icEntry = icLoadLabels_[i].icEntry;
        ICEntry *entryAddr = &(baselineScript->icEntry(icEntry));
        Assembler::PatchDataWithValueCheck(CodeLocationLabel(code, label),
                                           ImmPtr(entryAddr),
                                           ImmPtr((void*)-1));
    }

    if (modifiesArguments_)
        baselineScript->setModifiesArguments();

    
    if (cx->zone()->needsIncrementalBarrier())
        baselineScript->toggleBarriers(true);

    
    if (cx->runtime()->spsProfiler.enabled())
        baselineScript->toggleSPS(true);

    uint32_t *bytecodeMap = baselineScript->bytecodeTypeMap();
    types::FillBytecodeTypeMap(script, bytecodeMap);

    
    
    bytecodeMap[script->nTypeSets()] = 0;

    if (script->compartment()->debugMode())
        baselineScript->setDebugMode();

    
    if (cx->runtime()->jitRuntime()->isNativeToBytecodeMapEnabled(cx->runtime())) {
        JitSpew(JitSpew_Profiling, "Added JitcodeGlobalEntry for baseline script %s:%d (%p)",
                    script->filename(), script->lineno(), baselineScript);
        JitcodeGlobalEntry::BaselineEntry entry;
        entry.init(code->raw(), code->raw() + code->instructionsSize(), script);

        JitcodeGlobalTable *globalTable = cx->runtime()->jitRuntime()->getJitcodeGlobalTable();
        if (!globalTable->addEntry(entry))
            return Method_Error;

        
        code->setHasBytecodeMap();
    }

    script->setBaselineScript(cx, baselineScript);

    return Method_Compiled;
}

void
BaselineCompiler::emitInitializeLocals(size_t n, const Value &v)
{
    MOZ_ASSERT(frame.nlocals() > 0 && n <= frame.nlocals());

    
    
    
    static const size_t LOOP_UNROLL_FACTOR = 4;
    size_t toPushExtra = n % LOOP_UNROLL_FACTOR;

    masm.moveValue(v, R0);

    
    for (size_t i = 0; i < toPushExtra; i++)
        masm.pushValue(R0);

    
    if (n >= LOOP_UNROLL_FACTOR) {
        size_t toPush = n - toPushExtra;
        JS_ASSERT(toPush % LOOP_UNROLL_FACTOR == 0);
        JS_ASSERT(toPush >= LOOP_UNROLL_FACTOR);
        masm.move32(Imm32(toPush), R1.scratchReg());
        
        Label pushLoop;
        masm.bind(&pushLoop);
        for (size_t i = 0; i < LOOP_UNROLL_FACTOR; i++)
            masm.pushValue(R0);
        masm.branchSub32(Assembler::NonZero,
                         Imm32(LOOP_UNROLL_FACTOR), R1.scratchReg(), &pushLoop);
    }
}

bool
BaselineCompiler::emitPrologue()
{
    masm.push(BaselineFrameReg);
    masm.mov(BaselineStackReg, BaselineFrameReg);

    masm.subPtr(Imm32(BaselineFrame::Size()), BaselineStackReg);
    masm.checkStackAlignment();

    
    
    

    
    uint32_t flags = 0;
    if (script->isForEval())
        flags |= BaselineFrame::EVAL;
    masm.store32(Imm32(flags), frame.addressOfFlags());

    if (script->isForEval())
        masm.storePtr(ImmGCPtr(script), frame.addressOfEvalScript());

    
    
    
    
    
    if (function())
        masm.storePtr(ImmPtr(nullptr), frame.addressOfScopeChain());
    else
        masm.storePtr(R1.scratchReg(), frame.addressOfScopeChain());

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    Label earlyStackCheckFailed;
    if (needsEarlyStackCheck()) {
        if (!emitStackCheck( true))
            return false;
        masm.branchTest32(Assembler::NonZero,
                          frame.addressOfFlags(),
                          Imm32(BaselineFrame::OVER_RECURSED),
                          &earlyStackCheckFailed);
    }

    
    
    if (frame.nvars() > 0)
        emitInitializeLocals(frame.nvars(), UndefinedValue());
    if (frame.nlexicals() > 0)
        emitInitializeLocals(frame.nlexicals(), MagicValue(JS_UNINITIALIZED_LEXICAL));

    if (needsEarlyStackCheck())
        masm.bind(&earlyStackCheckFailed);

#ifdef JS_TRACE_LOGGING
    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());
    Register loggerReg = RegisterSet::Volatile().takeGeneral();
    masm.Push(loggerReg);
    masm.movePtr(ImmPtr(logger), loggerReg);
    masm.tracelogStart(loggerReg, TraceLogCreateTextId(logger, script));
    masm.tracelogStart(loggerReg, TraceLogger::Baseline);
    masm.Pop(loggerReg);
#endif

    
    
    prologueOffset_ = CodeOffsetLabel(masm.currentOffset());

    
    
    if (!initScopeChain())
        return false;

    if (!emitStackCheck())
        return false;

    if (!emitDebugPrologue())
        return false;

    if (!emitWarmUpCounterIncrement())
        return false;

    if (!emitArgumentTypeChecks())
        return false;

    if (!emitSPSPush())
        return false;

    return true;
}

bool
BaselineCompiler::emitEpilogue()
{
    
    
    epilogueOffset_ = CodeOffsetLabel(masm.currentOffset());

    masm.bind(&return_);

#ifdef JS_TRACE_LOGGING
    TraceLogger *logger = TraceLoggerForMainThread(cx->runtime());
    Register loggerReg = RegisterSet::Volatile().takeGeneral();
    masm.Push(loggerReg);
    masm.movePtr(ImmPtr(logger), loggerReg);
    masm.tracelogStop(loggerReg, TraceLogger::Baseline);
    
    
    masm.tracelogStop(loggerReg);
    masm.Pop(loggerReg);
#endif

    
    emitSPSPop();

    masm.mov(BaselineFrameReg, BaselineStackReg);
    masm.pop(BaselineFrameReg);

    masm.ret();
    return true;
}

#ifdef JSGC_GENERATIONAL






bool
BaselineCompiler::emitOutOfLinePostBarrierSlot()
{
    masm.bind(&postBarrierSlot_);

    Register objReg = R2.scratchReg();
    GeneralRegisterSet regs(GeneralRegisterSet::All());
    regs.take(R0);
    regs.take(objReg);
    regs.take(BaselineFrameReg);
    Register scratch = regs.takeAny();
#if defined(JS_CODEGEN_ARM)
    
    
    masm.push(lr);
#elif defined(JS_CODEGEN_MIPS)
    masm.push(ra);
#endif
    masm.pushValue(R0);

    masm.setupUnalignedABICall(2, scratch);
    masm.movePtr(ImmPtr(cx->runtime()), scratch);
    masm.passABIArg(scratch);
    masm.passABIArg(objReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, PostWriteBarrier));

    masm.popValue(R0);
    masm.ret();
    return true;
}
#endif 

bool
BaselineCompiler::emitIC(ICStub *stub, ICEntry::Kind kind)
{
    ICEntry *entry = allocateICEntry(stub, kind);
    if (!entry)
        return false;

    CodeOffsetLabel patchOffset;
    EmitCallIC(&patchOffset, masm);
    entry->setReturnOffset(CodeOffsetLabel(masm.currentOffset()));
    if (!addICLoadLabel(patchOffset))
        return false;

    return true;
}

typedef bool (*CheckOverRecursedWithExtraFn)(JSContext *, BaselineFrame *, uint32_t, uint32_t);
static const VMFunction CheckOverRecursedWithExtraInfo =
    FunctionInfo<CheckOverRecursedWithExtraFn>(CheckOverRecursedWithExtra);

bool
BaselineCompiler::emitStackCheck(bool earlyCheck)
{
    Label skipCall;
    uintptr_t *limitAddr = &cx->runtime()->mainThread.jitStackLimit;
    uint32_t slotsSize = script->nslots() * sizeof(Value);
    uint32_t tolerance = earlyCheck ? slotsSize : 0;

    masm.movePtr(BaselineStackReg, R1.scratchReg());

    
    
    
    if (earlyCheck)
        masm.subPtr(Imm32(tolerance), R1.scratchReg());

    
    
    
    
    
    
    Label forceCall;
    if (!earlyCheck && needsEarlyStackCheck()) {
        masm.branchTest32(Assembler::NonZero,
                          frame.addressOfFlags(),
                          Imm32(BaselineFrame::OVER_RECURSED),
                          &forceCall);
    }

    masm.branchPtr(Assembler::BelowOrEqual, AbsoluteAddress(limitAddr), R1.scratchReg(),
                   &skipCall);

    if (!earlyCheck && needsEarlyStackCheck())
        masm.bind(&forceCall);

    prepareVMCall();
    pushArg(Imm32(earlyCheck));
    pushArg(Imm32(tolerance));
    masm.loadBaselineFramePtr(BaselineFrameReg, R1.scratchReg());
    pushArg(R1.scratchReg());

    CallVMPhase phase = POST_INITIALIZE;
    if (earlyCheck)
        phase = PRE_INITIALIZE;
    else if (needsEarlyStackCheck())
        phase = CHECK_OVER_RECURSED;

    if (!callVM(CheckOverRecursedWithExtraInfo, phase))
        return false;

    masm.bind(&skipCall);
    return true;
}

typedef bool (*DebugPrologueFn)(JSContext *, BaselineFrame *, jsbytecode *, bool *);
static const VMFunction DebugPrologueInfo = FunctionInfo<DebugPrologueFn>(jit::DebugPrologue);

bool
BaselineCompiler::emitDebugPrologue()
{
    if (debugMode_) {
        
        masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

        prepareVMCall();
        pushArg(ImmPtr(pc));
        pushArg(R0.scratchReg());
        if (!callVM(DebugPrologueInfo))
            return false;

        
        icEntries_.back().setForDebugPrologue();

        
        
        Label done;
        masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &done);
        {
            masm.loadValue(frame.addressOfReturnValue(), JSReturnOperand);
            masm.jump(&return_);
        }
        masm.bind(&done);
    }

    postDebugPrologueOffset_ = CodeOffsetLabel(masm.currentOffset());

    return true;
}

typedef bool (*StrictEvalPrologueFn)(JSContext *, BaselineFrame *);
static const VMFunction StrictEvalPrologueInfo =
    FunctionInfo<StrictEvalPrologueFn>(jit::StrictEvalPrologue);

typedef bool (*HeavyweightFunPrologueFn)(JSContext *, BaselineFrame *);
static const VMFunction HeavyweightFunPrologueInfo =
    FunctionInfo<HeavyweightFunPrologueFn>(jit::HeavyweightFunPrologue);

bool
BaselineCompiler::initScopeChain()
{
    CallVMPhase phase = POST_INITIALIZE;
    if (needsEarlyStackCheck())
        phase = CHECK_OVER_RECURSED;

    RootedFunction fun(cx, function());
    if (fun) {
        
        
        
        Register callee = R0.scratchReg();
        Register scope = R1.scratchReg();
        masm.loadFunctionFromCalleeToken(frame.addressOfCalleeToken(), callee);
        masm.loadPtr(Address(callee, JSFunction::offsetOfEnvironment()), scope);
        masm.storePtr(scope, frame.addressOfScopeChain());

        if (fun->isHeavyweight()) {
            
            prepareVMCall();

            masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());
            pushArg(R0.scratchReg());

            if (!callVM(HeavyweightFunPrologueInfo, phase))
                return false;
        }
    } else {
        
        

        if (script->isForEval() && script->strict()) {
            
            prepareVMCall();

            masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());
            pushArg(R0.scratchReg());

            if (!callVM(StrictEvalPrologueInfo, phase))
                return false;
        }
    }

    return true;
}

typedef bool (*InterruptCheckFn)(JSContext *);
static const VMFunction InterruptCheckInfo = FunctionInfo<InterruptCheckFn>(InterruptCheck);

bool
BaselineCompiler::emitInterruptCheck()
{
    frame.syncStack(0);

    Label done;
    void *interrupt = (void *)&cx->runtime()->interrupt;
    masm.branch32(Assembler::Equal, AbsoluteAddress(interrupt), Imm32(0), &done);

    prepareVMCall();
    if (!callVM(InterruptCheckInfo))
        return false;

    masm.bind(&done);
    return true;
}

bool
BaselineCompiler::emitWarmUpCounterIncrement(bool allowOsr)
{
    
    

    if (!ionCompileable_ && !ionOSRCompileable_)
        return true;

    Register scriptReg = R2.scratchReg();
    Register countReg = R0.scratchReg();
    Address warmUpCounterAddr(scriptReg, JSScript::offsetOfWarmUpCounter());

    masm.movePtr(ImmGCPtr(script), scriptReg);
    masm.load32(warmUpCounterAddr, countReg);
    masm.add32(Imm32(1), countReg);
    masm.store32(countReg, warmUpCounterAddr);

    
    
    if (analysis_.info(pc).loopEntryInCatchOrFinally) {
        JS_ASSERT(JSOp(*pc) == JSOP_LOOPENTRY);
        return true;
    }

    
    if (!allowOsr) {
        JS_ASSERT(JSOp(*pc) == JSOP_LOOPENTRY);
        return true;
    }

    Label skipCall;

    const OptimizationInfo *info = js_IonOptimizations.get(js_IonOptimizations.firstLevel());
    uint32_t warmUpThreshold = info->compilerWarmUpThreshold(script, pc);
    masm.branch32(Assembler::LessThan, countReg, Imm32(warmUpThreshold), &skipCall);

    masm.branchPtr(Assembler::Equal,
                   Address(scriptReg, JSScript::offsetOfIonScript()),
                   ImmPtr(ION_COMPILING_SCRIPT), &skipCall);

    
    ICWarmUpCounter_Fallback::Compiler stubCompiler(cx);
    if (!emitNonOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    masm.bind(&skipCall);

    return true;
}

bool
BaselineCompiler::emitArgumentTypeChecks()
{
    if (!function())
        return true;

    frame.pushThis();
    frame.popRegsAndSync(1);

    ICTypeMonitor_Fallback::Compiler compiler(cx, (uint32_t) 0);
    if (!emitNonOpIC(compiler.getStub(&stubSpace_)))
        return false;

    for (size_t i = 0; i < function()->nargs(); i++) {
        frame.pushArg(i);
        frame.popRegsAndSync(1);

        ICTypeMonitor_Fallback::Compiler compiler(cx, i + 1);
        if (!emitNonOpIC(compiler.getStub(&stubSpace_)))
            return false;
    }

    return true;
}

bool
BaselineCompiler::emitDebugTrap()
{
    JS_ASSERT(debugMode_);
    JS_ASSERT(frame.numUnsyncedSlots() == 0);

    bool enabled = script->stepModeEnabled() || script->hasBreakpointsAt(pc);

    
    JitCode *handler = cx->runtime()->jitRuntime()->debugTrapHandler(cx);
    mozilla::DebugOnly<CodeOffsetLabel> offset = masm.toggledCall(handler, enabled);

#ifdef DEBUG
    
    PCMappingEntry &entry = pcMappingEntries_.back();
    JS_ASSERT((&offset)->offset() == entry.nativeOffset);
#endif

    
    ICEntry icEntry(script->pcToOffset(pc), ICEntry::Kind_DebugTrap);
    icEntry.setReturnOffset(CodeOffsetLabel(masm.currentOffset()));
    if (!icEntries_.append(icEntry))
        return false;

    return true;
}

bool
BaselineCompiler::emitSPSPush()
{
    
    Label noPush;
    CodeOffsetLabel toggleOffset = masm.toggledJump(&noPush);
    JS_ASSERT(frame.numUnsyncedSlots() == 0);
    ICProfiler_Fallback::Compiler compiler(cx);
    if (!emitNonOpIC(compiler.getStub(&stubSpace_)))
        return false;
    masm.bind(&noPush);

    
    JS_ASSERT(spsPushToggleOffset_.offset() == 0);
    spsPushToggleOffset_ = toggleOffset;
    return true;
}

void
BaselineCompiler::emitSPSPop()
{
    
    Label noPop;
    masm.branchTest32(Assembler::Zero, frame.addressOfFlags(),
                      Imm32(BaselineFrame::HAS_PUSHED_SPS_FRAME), &noPop);
    masm.spsPopFrameSafe(&cx->runtime()->spsProfiler, R1.scratchReg());
    masm.bind(&noPop);
}

MethodStatus
BaselineCompiler::emitBody()
{
    JS_ASSERT(pc == script->code());

    bool lastOpUnreachable = false;
    uint32_t emittedOps = 0;
    mozilla::DebugOnly<jsbytecode *> prevpc = pc;

    while (true) {
        JSOp op = JSOp(*pc);
        JitSpew(JitSpew_BaselineOp, "Compiling op @ %d: %s",
                int(script->pcToOffset(pc)), js_CodeName[op]);

        BytecodeInfo *info = analysis_.maybeInfo(pc);

        
        if (!info) {
            
            pc += GetBytecodeLength(pc);
            if (pc >= script->codeEnd())
                break;

            lastOpUnreachable = true;
            prevpc = pc;
            continue;
        }

        
        if (info->jumpTarget) {
            frame.syncStack(0);
            frame.setStackDepth(info->stackDepth);
        }

        
        if (debugMode_)
            frame.syncStack(0);

        
        if (frame.stackDepth() > 2)
            frame.syncStack(2);

        frame.assertValidState(*info);

        masm.bind(labelOf(pc));

        
        
        
        
        bool addIndexEntry = (pc == script->code() || lastOpUnreachable || emittedOps > 100);
        if (addIndexEntry)
            emittedOps = 0;
        if (!addPCMappingEntry(addIndexEntry))
            return Method_Error;

        
        if (debugMode_ && !emitDebugTrap())
            return Method_Error;

        switch (op) {
          default:
            JitSpew(JitSpew_BaselineAbort, "Unhandled op: %s", js_CodeName[op]);
            return Method_CantCompile;

#define EMIT_OP(OP)                            \
          case OP:                             \
            if (!this->emit_##OP())            \
                return Method_Error;           \
            break;
OPCODE_LIST(EMIT_OP)
#undef EMIT_OP
        }

        
        pc += GetBytecodeLength(pc);
        if (pc >= script->codeEnd())
            break;

        emittedOps++;
        lastOpUnreachable = false;
#ifdef DEBUG
        prevpc = pc;
#endif
    }

    JS_ASSERT(JSOp(*prevpc) == JSOP_RETRVAL);
    return Method_Compiled;
}

bool
BaselineCompiler::emit_JSOP_NOP()
{
    return true;
}

bool
BaselineCompiler::emit_JSOP_LABEL()
{
    return true;
}

bool
BaselineCompiler::emit_JSOP_POP()
{
    frame.pop();
    return true;
}

bool
BaselineCompiler::emit_JSOP_POPN()
{
    frame.popn(GET_UINT16(pc));
    return true;
}

bool
BaselineCompiler::emit_JSOP_DUPAT()
{
    frame.syncStack(0);

    
    
    

    int depth = -(GET_UINT24(pc) + 1);
    masm.loadValue(frame.addressOfStackValue(frame.peek(depth)), R0);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_DUP()
{
    
    
    
    frame.popRegsAndSync(1);
    masm.moveValue(R0, R1);

    
    frame.push(R1);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_DUP2()
{
    frame.syncStack(0);

    masm.loadValue(frame.addressOfStackValue(frame.peek(-2)), R0);
    masm.loadValue(frame.addressOfStackValue(frame.peek(-1)), R1);

    frame.push(R0);
    frame.push(R1);
    return true;
}

bool
BaselineCompiler::emit_JSOP_SWAP()
{
    
    frame.popRegsAndSync(2);

    frame.push(R1);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_PICK()
{
    frame.syncStack(0);

    
    
    
    

    
    int depth = -(GET_INT8(pc) + 1);
    masm.loadValue(frame.addressOfStackValue(frame.peek(depth)), R0);

    
    depth++;
    for (; depth < 0; depth++) {
        Address source = frame.addressOfStackValue(frame.peek(depth));
        Address dest = frame.addressOfStackValue(frame.peek(depth - 1));
        masm.loadValue(source, R1);
        masm.storeValue(R1, dest);
    }

    
    frame.pop();
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_GOTO()
{
    frame.syncStack(0);

    jsbytecode *target = pc + GET_JUMP_OFFSET(pc);
    masm.jump(labelOf(target));
    return true;
}

bool
BaselineCompiler::emitToBoolean()
{
    Label skipIC;
    masm.branchTestBoolean(Assembler::Equal, R0, &skipIC);

    
    ICToBool_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    masm.bind(&skipIC);
    return true;
}

bool
BaselineCompiler::emitTest(bool branchIfTrue)
{
    bool knownBoolean = frame.peek(-1)->isKnownBoolean();

    
    frame.popRegsAndSync(1);

    if (!knownBoolean && !emitToBoolean())
        return false;

    
    masm.branchTestBooleanTruthy(branchIfTrue, R0, labelOf(pc + GET_JUMP_OFFSET(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_IFEQ()
{
    return emitTest(false);
}

bool
BaselineCompiler::emit_JSOP_IFNE()
{
    return emitTest(true);
}

bool
BaselineCompiler::emitAndOr(bool branchIfTrue)
{
    bool knownBoolean = frame.peek(-1)->isKnownBoolean();

    
    frame.syncStack(0);

    masm.loadValue(frame.addressOfStackValue(frame.peek(-1)), R0);
    if (!knownBoolean && !emitToBoolean())
        return false;

    masm.branchTestBooleanTruthy(branchIfTrue, R0, labelOf(pc + GET_JUMP_OFFSET(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_AND()
{
    return emitAndOr(false);
}

bool
BaselineCompiler::emit_JSOP_OR()
{
    return emitAndOr(true);
}

bool
BaselineCompiler::emit_JSOP_NOT()
{
    bool knownBoolean = frame.peek(-1)->isKnownBoolean();

    
    frame.popRegsAndSync(1);

    if (!knownBoolean && !emitToBoolean())
        return false;

    masm.notBoolean(R0);

    frame.push(R0, JSVAL_TYPE_BOOLEAN);
    return true;
}

bool
BaselineCompiler::emit_JSOP_POS()
{
    
    frame.popRegsAndSync(1);

    
    Label done;
    masm.branchTestNumber(Assembler::Equal, R0, &done);

    
    ICToNumber_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    masm.bind(&done);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_LOOPHEAD()
{
    return emitInterruptCheck();
}

bool
BaselineCompiler::emit_JSOP_LOOPENTRY()
{
    frame.syncStack(0);
    return emitWarmUpCounterIncrement(LoopEntryCanIonOsr(pc));
}

bool
BaselineCompiler::emit_JSOP_VOID()
{
    frame.pop();
    frame.push(UndefinedValue());
    return true;
}

bool
BaselineCompiler::emit_JSOP_UNDEFINED()
{
    frame.push(UndefinedValue());
    return true;
}

bool
BaselineCompiler::emit_JSOP_HOLE()
{
    frame.push(MagicValue(JS_ELEMENTS_HOLE));
    return true;
}

bool
BaselineCompiler::emit_JSOP_NULL()
{
    frame.push(NullValue());
    return true;
}

bool
BaselineCompiler::emit_JSOP_THIS()
{
    if (function() && function()->isArrow()) {
        
        
        frame.syncStack(0);
        Register scratch = R0.scratchReg();
        masm.loadFunctionFromCalleeToken(frame.addressOfCalleeToken(), scratch);
        masm.loadValue(Address(scratch, FunctionExtended::offsetOfArrowThisSlot()), R0);
        frame.push(R0);
        return true;
    }

    
    frame.pushThis();

    
    if (script->strict() || (function() && function()->isSelfHostedBuiltin()))
        return true;

    Label skipIC;
    
    frame.popRegsAndSync(1);
    
    masm.branchTestObject(Assembler::Equal, R0, &skipIC);

    
    ICThis_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    masm.storeValue(R0, frame.addressOfThis());

    
    masm.bind(&skipIC);
    frame.push(R0);

    return true;
}

bool
BaselineCompiler::emit_JSOP_TRUE()
{
    frame.push(BooleanValue(true));
    return true;
}

bool
BaselineCompiler::emit_JSOP_FALSE()
{
    frame.push(BooleanValue(false));
    return true;
}

bool
BaselineCompiler::emit_JSOP_ZERO()
{
    frame.push(Int32Value(0));
    return true;
}

bool
BaselineCompiler::emit_JSOP_ONE()
{
    frame.push(Int32Value(1));
    return true;
}

bool
BaselineCompiler::emit_JSOP_INT8()
{
    frame.push(Int32Value(GET_INT8(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_INT32()
{
    frame.push(Int32Value(GET_INT32(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_UINT16()
{
    frame.push(Int32Value(GET_UINT16(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_UINT24()
{
    frame.push(Int32Value(GET_UINT24(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_DOUBLE()
{
    frame.push(script->getConst(GET_UINT32_INDEX(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_STRING()
{
    frame.push(StringValue(script->getAtom(pc)));
    return true;
}

typedef JSObject *(*DeepCloneObjectLiteralFn)(JSContext *, HandleObject, NewObjectKind);
static const VMFunction DeepCloneObjectLiteralInfo =
    FunctionInfo<DeepCloneObjectLiteralFn>(DeepCloneObjectLiteral);

bool
BaselineCompiler::emit_JSOP_OBJECT()
{
    if (JS::CompartmentOptionsRef(cx).cloneSingletons()) {
        RootedObject obj(cx, script->getObject(GET_UINT32_INDEX(pc)));
        if (!obj)
            return false;

        prepareVMCall();

        pushArg(ImmWord(js::MaybeSingletonObject));
        pushArg(ImmGCPtr(obj));

        if (!callVM(DeepCloneObjectLiteralInfo))
            return false;

        
        masm.tagValue(JSVAL_TYPE_OBJECT, ReturnReg, R0);
        frame.push(R0);
        return true;
    }

    JS::CompartmentOptionsRef(cx).setSingletonsAsValues();
    frame.push(ObjectValue(*script->getObject(pc)));
    return true;
}

bool
BaselineCompiler::emit_JSOP_CALLSITEOBJ()
{
    RootedObject cso(cx, script->getObject(pc));
    RootedObject raw(cx, script->getObject(GET_UINT32_INDEX(pc) + 1));
    if (!cso || !raw)
        return false;
    RootedValue rawValue(cx);
    rawValue.setObject(*raw);

    if (!ProcessCallSiteObjOperation(cx, cso, raw, rawValue))
        return false;

    frame.push(ObjectValue(*cso));
    return true;
}

typedef JSObject *(*CloneRegExpObjectFn)(JSContext *, JSObject *);
static const VMFunction CloneRegExpObjectInfo =
    FunctionInfo<CloneRegExpObjectFn>(CloneRegExpObject);

bool
BaselineCompiler::emit_JSOP_REGEXP()
{
    RootedObject reObj(cx, script->getRegExp(pc));

    prepareVMCall();
    pushArg(ImmGCPtr(reObj));
    if (!callVM(CloneRegExpObjectInfo))
        return false;

    
    masm.tagValue(JSVAL_TYPE_OBJECT, ReturnReg, R0);
    frame.push(R0);
    return true;
}

typedef JSObject *(*LambdaFn)(JSContext *, HandleFunction, HandleObject);
static const VMFunction LambdaInfo = FunctionInfo<LambdaFn>(js::Lambda);

bool
BaselineCompiler::emit_JSOP_LAMBDA()
{
    RootedFunction fun(cx, script->getFunction(GET_UINT32_INDEX(pc)));

    prepareVMCall();
    masm.loadPtr(frame.addressOfScopeChain(), R0.scratchReg());

    pushArg(R0.scratchReg());
    pushArg(ImmGCPtr(fun));

    if (!callVM(LambdaInfo))
        return false;

    
    masm.tagValue(JSVAL_TYPE_OBJECT, ReturnReg, R0);
    frame.push(R0);
    return true;
}

typedef JSObject *(*LambdaArrowFn)(JSContext *, HandleFunction, HandleObject, HandleValue);
static const VMFunction LambdaArrowInfo = FunctionInfo<LambdaArrowFn>(js::LambdaArrow);

bool
BaselineCompiler::emit_JSOP_LAMBDA_ARROW()
{
    
    frame.popRegsAndSync(1);

    RootedFunction fun(cx, script->getFunction(GET_UINT32_INDEX(pc)));

    prepareVMCall();
    masm.loadPtr(frame.addressOfScopeChain(), R1.scratchReg());

    pushArg(R0);
    pushArg(R1.scratchReg());
    pushArg(ImmGCPtr(fun));

    if (!callVM(LambdaArrowInfo))
        return false;

    
    masm.tagValue(JSVAL_TYPE_OBJECT, ReturnReg, R0);
    frame.push(R0);
    return true;
}

void
BaselineCompiler::storeValue(const StackValue *source, const Address &dest,
                             const ValueOperand &scratch)
{
    switch (source->kind()) {
      case StackValue::Constant:
        masm.storeValue(source->constant(), dest);
        break;
      case StackValue::Register:
        masm.storeValue(source->reg(), dest);
        break;
      case StackValue::LocalSlot:
        masm.loadValue(frame.addressOfLocal(source->localSlot()), scratch);
        masm.storeValue(scratch, dest);
        break;
      case StackValue::ArgSlot:
        masm.loadValue(frame.addressOfArg(source->argSlot()), scratch);
        masm.storeValue(scratch, dest);
        break;
      case StackValue::ThisSlot:
        masm.loadValue(frame.addressOfThis(), scratch);
        masm.storeValue(scratch, dest);
        break;
      case StackValue::Stack:
        masm.loadValue(frame.addressOfStackValue(source), scratch);
        masm.storeValue(scratch, dest);
        break;
      default:
        MOZ_CRASH("Invalid kind");
    }
}

bool
BaselineCompiler::emit_JSOP_BITOR()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_BITXOR()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_BITAND()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_LSH()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_RSH()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_URSH()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_ADD()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_SUB()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_MUL()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_DIV()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emit_JSOP_MOD()
{
    return emitBinaryArith();
}

bool
BaselineCompiler::emitBinaryArith()
{
    
    frame.popRegsAndSync(2);

    
    ICBinaryArith_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emitUnaryArith()
{
    
    frame.popRegsAndSync(1);

    
    ICUnaryArith_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_BITNOT()
{
    return emitUnaryArith();
}

bool
BaselineCompiler::emit_JSOP_NEG()
{
    return emitUnaryArith();
}

bool
BaselineCompiler::emit_JSOP_LT()
{
    return emitCompare();
}

bool
BaselineCompiler::emit_JSOP_LE()
{
    return emitCompare();
}

bool
BaselineCompiler::emit_JSOP_GT()
{
    return emitCompare();
}

bool
BaselineCompiler::emit_JSOP_GE()
{
    return emitCompare();
}

bool
BaselineCompiler::emit_JSOP_EQ()
{
    return emitCompare();
}

bool
BaselineCompiler::emit_JSOP_NE()
{
    return emitCompare();
}

bool
BaselineCompiler::emitCompare()
{
    

    
    frame.popRegsAndSync(2);

    
    ICCompare_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0, JSVAL_TYPE_BOOLEAN);
    return true;
}

bool
BaselineCompiler::emit_JSOP_STRICTEQ()
{
    return emitCompare();
}

bool
BaselineCompiler::emit_JSOP_STRICTNE()
{
    return emitCompare();
}

bool
BaselineCompiler::emit_JSOP_CONDSWITCH()
{
    return true;
}

bool
BaselineCompiler::emit_JSOP_CASE()
{
    frame.popRegsAndSync(2);
    frame.push(R0);
    frame.syncStack(0);

    
    ICCompare_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    Register payload = masm.extractInt32(R0, R0.scratchReg());
    jsbytecode *target = pc + GET_JUMP_OFFSET(pc);

    Label done;
    masm.branch32(Assembler::Equal, payload, Imm32(0), &done);
    {
        
        masm.addPtr(Imm32(sizeof(Value)), StackPointer);
        masm.jump(labelOf(target));
    }
    masm.bind(&done);
    return true;
}

bool
BaselineCompiler::emit_JSOP_DEFAULT()
{
    frame.pop();
    return emit_JSOP_GOTO();
}

bool
BaselineCompiler::emit_JSOP_LINENO()
{
    return true;
}

bool
BaselineCompiler::emit_JSOP_NEWARRAY()
{
    frame.syncStack(0);

    uint32_t length = GET_UINT24(pc);
    RootedTypeObject type(cx);
    if (!types::UseNewTypeForInitializer(script, pc, JSProto_Array)) {
        type = types::TypeScript::InitObject(cx, script, pc, JSProto_Array);
        if (!type)
            return false;
    }

    
    masm.move32(Imm32(length), R0.scratchReg());
    masm.movePtr(ImmGCPtr(type), R1.scratchReg());

    JSObject *templateObject = NewDenseUnallocatedArray(cx, length, nullptr, TenuredObject);
    if (!templateObject)
        return false;
    templateObject->setType(type);

    ICNewArray_Fallback::Compiler stubCompiler(cx, templateObject);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

typedef JSObject *(*NewArrayCopyOnWriteFn)(JSContext *, HandleObject, gc::InitialHeap);
const VMFunction jit::NewArrayCopyOnWriteInfo =
    FunctionInfo<NewArrayCopyOnWriteFn>(js::NewDenseCopyOnWriteArray);

bool
BaselineCompiler::emit_JSOP_NEWARRAY_COPYONWRITE()
{
    RootedScript scriptRoot(cx, script);
    JSObject *obj = types::GetOrFixupCopyOnWriteObject(cx, scriptRoot, pc);
    if (!obj)
        return false;

    prepareVMCall();

    pushArg(Imm32(gc::DefaultHeap));
    pushArg(ImmGCPtr(obj));

    if (!callVM(NewArrayCopyOnWriteInfo))
        return false;

    
    masm.tagValue(JSVAL_TYPE_OBJECT, ReturnReg, R0);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_INITELEM_ARRAY()
{
    
    frame.syncStack(0);

    
    masm.loadValue(frame.addressOfStackValue(frame.peek(-2)), R0);
    masm.moveValue(Int32Value(GET_UINT24(pc)), R1);

    
    ICSetElem_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.pop();
    return true;
}

bool
BaselineCompiler::emit_JSOP_NEWOBJECT()
{
    frame.syncStack(0);

    RootedTypeObject type(cx);
    if (!types::UseNewTypeForInitializer(script, pc, JSProto_Object)) {
        type = types::TypeScript::InitObject(cx, script, pc, JSProto_Object);
        if (!type)
            return false;
    }

    RootedObject baseObject(cx, script->getObject(pc));
    RootedObject templateObject(cx, CopyInitializerObject(cx, baseObject, TenuredObject));
    if (!templateObject)
        return false;

    if (type) {
        templateObject->setType(type);
    } else {
        if (!JSObject::setSingletonType(cx, templateObject))
            return false;
    }

    ICNewObject_Fallback::Compiler stubCompiler(cx, templateObject);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_NEWINIT()
{
    frame.syncStack(0);
    JSProtoKey key = JSProtoKey(GET_UINT8(pc));

    RootedTypeObject type(cx);
    if (!types::UseNewTypeForInitializer(script, pc, key)) {
        type = types::TypeScript::InitObject(cx, script, pc, key);
        if (!type)
            return false;
    }

    if (key == JSProto_Array) {
        
        masm.move32(Imm32(0), R0.scratchReg());
        masm.movePtr(ImmGCPtr(type), R1.scratchReg());

        JSObject *templateObject = NewDenseUnallocatedArray(cx, 0, nullptr, TenuredObject);
        if (!templateObject)
            return false;
        templateObject->setType(type);

        ICNewArray_Fallback::Compiler stubCompiler(cx, templateObject);
        if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
            return false;
    } else {
        JS_ASSERT(key == JSProto_Object);

        RootedObject templateObject(cx);
        templateObject = NewBuiltinClassInstance(cx, &JSObject::class_, TenuredObject);
        if (!templateObject)
            return false;

        if (type) {
            templateObject->setType(type);
        } else {
            if (!JSObject::setSingletonType(cx, templateObject))
                return false;
        }

        ICNewObject_Fallback::Compiler stubCompiler(cx, templateObject);
        if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
            return false;
    }

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_INITELEM()
{
    
    storeValue(frame.peek(-1), frame.addressOfScratchValue(), R2);
    frame.pop();

    
    frame.popRegsAndSync(2);

    
    frame.push(R0);
    frame.syncStack(0);

    
    frame.pushScratchValue();

    
    ICSetElem_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.pop();
    return true;
}

typedef bool (*MutateProtoFn)(JSContext *cx, HandleObject obj, HandleValue newProto);
static const VMFunction MutateProtoInfo = FunctionInfo<MutateProtoFn>(MutatePrototype);

bool
BaselineCompiler::emit_JSOP_MUTATEPROTO()
{
    
    frame.syncStack(0);

    masm.extractObject(frame.addressOfStackValue(frame.peek(-2)), R0.scratchReg());
    masm.loadValue(frame.addressOfStackValue(frame.peek(-1)), R1);

    prepareVMCall();

    pushArg(R1);
    pushArg(R0.scratchReg());

    if (!callVM(MutateProtoInfo))
        return false;

    frame.pop();
    return true;
}

bool
BaselineCompiler::emit_JSOP_INITPROP()
{
    
    frame.popRegsAndSync(2);

    
    frame.push(R0);
    frame.syncStack(0);

    
    ICSetProp_Fallback::Compiler compiler(cx);
    return emitOpIC(compiler.getStub(&stubSpace_));
}

bool
BaselineCompiler::emit_JSOP_ENDINIT()
{
    return true;
}

typedef bool (*NewbornArrayPushFn)(JSContext *, HandleObject, const Value &);
static const VMFunction NewbornArrayPushInfo = FunctionInfo<NewbornArrayPushFn>(NewbornArrayPush);

bool
BaselineCompiler::emit_JSOP_ARRAYPUSH()
{
    
    frame.popRegsAndSync(2);
    masm.unboxObject(R1, R1.scratchReg());

    prepareVMCall();

    pushArg(R0);
    pushArg(R1.scratchReg());

    return callVM(NewbornArrayPushInfo);
}

bool
BaselineCompiler::emit_JSOP_GETELEM()
{
    
    frame.popRegsAndSync(2);

    
    ICGetElem_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_CALLELEM()
{
    return emit_JSOP_GETELEM();
}

bool
BaselineCompiler::emit_JSOP_SETELEM()
{
    
    storeValue(frame.peek(-1), frame.addressOfScratchValue(), R2);
    frame.pop();

    
    frame.popRegsAndSync(2);

    
    frame.pushScratchValue();

    
    ICSetElem_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    return true;
}

typedef bool (*DeleteElementFn)(JSContext *, HandleValue, HandleValue, bool *);
static const VMFunction DeleteElementStrictInfo = FunctionInfo<DeleteElementFn>(DeleteElement<true>);
static const VMFunction DeleteElementNonStrictInfo = FunctionInfo<DeleteElementFn>(DeleteElement<false>);

bool
BaselineCompiler::emit_JSOP_DELELEM()
{
    
    frame.syncStack(0);
    masm.loadValue(frame.addressOfStackValue(frame.peek(-2)), R0);
    masm.loadValue(frame.addressOfStackValue(frame.peek(-1)), R1);

    prepareVMCall();

    pushArg(R1);
    pushArg(R0);

    if (!callVM(script->strict() ? DeleteElementStrictInfo : DeleteElementNonStrictInfo))
        return false;

    masm.boxNonDouble(JSVAL_TYPE_BOOLEAN, ReturnReg, R1);
    frame.popn(2);
    frame.push(R1);
    return true;
}

bool
BaselineCompiler::emit_JSOP_IN()
{
    frame.popRegsAndSync(2);

    ICIn_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_GETGNAME()
{
    RootedPropertyName name(cx, script->getName(pc));

    if (name == cx->names().undefined) {
        frame.push(UndefinedValue());
        return true;
    }
    if (name == cx->names().NaN) {
        frame.push(cx->runtime()->NaNValue);
        return true;
    }
    if (name == cx->names().Infinity) {
        frame.push(cx->runtime()->positiveInfinityValue);
        return true;
    }

    frame.syncStack(0);

    masm.movePtr(ImmGCPtr(&script->global()), R0.scratchReg());

    
    ICGetName_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_BINDGNAME()
{
    frame.push(ObjectValue(script->global()));
    return true;
}

bool
BaselineCompiler::emit_JSOP_SETPROP()
{
    
    frame.popRegsAndSync(2);

    
    ICSetProp_Fallback::Compiler compiler(cx);
    if (!emitOpIC(compiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_SETNAME()
{
    return emit_JSOP_SETPROP();
}

bool
BaselineCompiler::emit_JSOP_SETGNAME()
{
    return emit_JSOP_SETPROP();
}

bool
BaselineCompiler::emit_JSOP_GETPROP()
{
    
    frame.popRegsAndSync(1);

    
    ICGetProp_Fallback::Compiler compiler(cx);
    if (!emitOpIC(compiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_CALLPROP()
{
    return emit_JSOP_GETPROP();
}

bool
BaselineCompiler::emit_JSOP_LENGTH()
{
    return emit_JSOP_GETPROP();
}

bool
BaselineCompiler::emit_JSOP_GETXPROP()
{
    return emit_JSOP_GETPROP();
}

typedef bool (*DeletePropertyFn)(JSContext *, HandleValue, HandlePropertyName, bool *);
static const VMFunction DeletePropertyStrictInfo = FunctionInfo<DeletePropertyFn>(DeleteProperty<true>);
static const VMFunction DeletePropertyNonStrictInfo = FunctionInfo<DeletePropertyFn>(DeleteProperty<false>);

bool
BaselineCompiler::emit_JSOP_DELPROP()
{
    
    frame.syncStack(0);
    masm.loadValue(frame.addressOfStackValue(frame.peek(-1)), R0);

    prepareVMCall();

    pushArg(ImmGCPtr(script->getName(pc)));
    pushArg(R0);

    if (!callVM(script->strict() ? DeletePropertyStrictInfo : DeletePropertyNonStrictInfo))
        return false;

    masm.boxNonDouble(JSVAL_TYPE_BOOLEAN, ReturnReg, R1);
    frame.pop();
    frame.push(R1);
    return true;
}

void
BaselineCompiler::getScopeCoordinateObject(Register reg)
{
    ScopeCoordinate sc(pc);

    masm.loadPtr(frame.addressOfScopeChain(), reg);
    for (unsigned i = sc.hops(); i; i--)
        masm.extractObject(Address(reg, ScopeObject::offsetOfEnclosingScope()), reg);
}

Address
BaselineCompiler::getScopeCoordinateAddressFromObject(Register objReg, Register reg)
{
    ScopeCoordinate sc(pc);
    Shape *shape = ScopeCoordinateToStaticScopeShape(script, pc);

    Address addr;
    if (shape->numFixedSlots() <= sc.slot()) {
        masm.loadPtr(Address(objReg, JSObject::offsetOfSlots()), reg);
        return Address(reg, (sc.slot() - shape->numFixedSlots()) * sizeof(Value));
    }

    return Address(objReg, JSObject::getFixedSlotOffset(sc.slot()));
}

Address
BaselineCompiler::getScopeCoordinateAddress(Register reg)
{
    getScopeCoordinateObject(reg);
    return getScopeCoordinateAddressFromObject(reg, reg);
}

bool
BaselineCompiler::emit_JSOP_GETALIASEDVAR()
{
    frame.syncStack(0);

    Address address = getScopeCoordinateAddress(R0.scratchReg());
    masm.loadValue(address, R0);

    ICTypeMonitor_Fallback::Compiler compiler(cx, (ICMonitoredFallbackStub *) nullptr);
    if (!emitOpIC(compiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_SETALIASEDVAR()
{
    JSScript *outerScript = ScopeCoordinateFunctionScript(script, pc);
    if (outerScript && outerScript->treatAsRunOnce()) {
        
        

        
        frame.syncStack(1);
        frame.popValue(R1);

        
        getScopeCoordinateObject(R2.scratchReg());
        masm.tagValue(JSVAL_TYPE_OBJECT, R2.scratchReg(), R0);

        
        ICSetProp_Fallback::Compiler compiler(cx);
        if (!emitOpIC(compiler.getStub(&stubSpace_)))
            return false;

        
        frame.push(R0);
        return true;
    }

    
    frame.popRegsAndSync(1);
    Register objReg = R2.scratchReg();

    getScopeCoordinateObject(objReg);
    Address address = getScopeCoordinateAddressFromObject(objReg, R1.scratchReg());
    masm.patchableCallPreBarrier(address, MIRType_Value);
    masm.storeValue(R0, address);
    frame.push(R0);

#ifdef JSGC_GENERATIONAL
    
    
    Register temp = R1.scratchReg();

    Label skipBarrier;
    masm.branchPtrInNurseryRange(Assembler::Equal, objReg, temp, &skipBarrier);
    masm.branchValueIsNurseryObject(Assembler::NotEqual, R0, temp, &skipBarrier);

    masm.call(&postBarrierSlot_); 

    masm.bind(&skipBarrier);
#endif

    return true;
}

bool
BaselineCompiler::emit_JSOP_NAME()
{
    frame.syncStack(0);

    masm.loadPtr(frame.addressOfScopeChain(), R0.scratchReg());

    
    ICGetName_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_BINDNAME()
{
    frame.syncStack(0);

    masm.loadPtr(frame.addressOfScopeChain(), R0.scratchReg());

    
    ICBindName_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0);
    return true;
}

typedef bool (*DeleteNameFn)(JSContext *, HandlePropertyName, HandleObject,
                             MutableHandleValue);
static const VMFunction DeleteNameInfo = FunctionInfo<DeleteNameFn>(DeleteNameOperation);

bool
BaselineCompiler::emit_JSOP_DELNAME()
{
    frame.syncStack(0);
    masm.loadPtr(frame.addressOfScopeChain(), R0.scratchReg());

    prepareVMCall();

    pushArg(R0.scratchReg());
    pushArg(ImmGCPtr(script->getName(pc)));

    if (!callVM(DeleteNameInfo))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_GETINTRINSIC()
{
    frame.syncStack(0);

    ICGetIntrinsic_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

typedef bool (*DefVarOrConstFn)(JSContext *, HandlePropertyName, unsigned, HandleObject);
static const VMFunction DefVarOrConstInfo = FunctionInfo<DefVarOrConstFn>(DefVarOrConst);

bool
BaselineCompiler::emit_JSOP_DEFVAR()
{
    frame.syncStack(0);

    unsigned attrs = JSPROP_ENUMERATE;
    if (JSOp(*pc) == JSOP_DEFCONST)
        attrs |= JSPROP_READONLY;
    else if (!script->isForEval())
        attrs |= JSPROP_PERMANENT;
    JS_ASSERT(attrs <= UINT32_MAX);

    masm.loadPtr(frame.addressOfScopeChain(), R0.scratchReg());

    prepareVMCall();

    pushArg(R0.scratchReg());
    pushArg(Imm32(attrs));
    pushArg(ImmGCPtr(script->getName(pc)));

    return callVM(DefVarOrConstInfo);
}

bool
BaselineCompiler::emit_JSOP_DEFCONST()
{
    return emit_JSOP_DEFVAR();
}

typedef bool (*SetConstFn)(JSContext *, HandlePropertyName, HandleObject, HandleValue);
static const VMFunction SetConstInfo = FunctionInfo<SetConstFn>(SetConst);

bool
BaselineCompiler::emit_JSOP_SETCONST()
{
    frame.popRegsAndSync(1);
    frame.push(R0);
    frame.syncStack(0);

    masm.loadPtr(frame.addressOfScopeChain(), R1.scratchReg());

    prepareVMCall();

    pushArg(R0);
    pushArg(R1.scratchReg());
    pushArg(ImmGCPtr(script->getName(pc)));

    return callVM(SetConstInfo);
}

typedef bool (*DefFunOperationFn)(JSContext *, HandleScript, HandleObject, HandleFunction);
static const VMFunction DefFunOperationInfo = FunctionInfo<DefFunOperationFn>(DefFunOperation);

bool
BaselineCompiler::emit_JSOP_DEFFUN()
{
    RootedFunction fun(cx, script->getFunction(GET_UINT32_INDEX(pc)));

    frame.syncStack(0);
    masm.loadPtr(frame.addressOfScopeChain(), R0.scratchReg());

    prepareVMCall();

    pushArg(ImmGCPtr(fun));
    pushArg(R0.scratchReg());
    pushArg(ImmGCPtr(script));

    return callVM(DefFunOperationInfo);
}

typedef bool (*InitPropGetterSetterFn)(JSContext *, jsbytecode *, HandleObject, HandlePropertyName,
                                       HandleObject);
static const VMFunction InitPropGetterSetterInfo =
    FunctionInfo<InitPropGetterSetterFn>(InitGetterSetterOperation);

bool
BaselineCompiler::emitInitPropGetterSetter()
{
    JS_ASSERT(JSOp(*pc) == JSOP_INITPROP_GETTER ||
              JSOp(*pc) == JSOP_INITPROP_SETTER);

    
    frame.syncStack(0);

    prepareVMCall();

    masm.extractObject(frame.addressOfStackValue(frame.peek(-1)), R0.scratchReg());
    masm.extractObject(frame.addressOfStackValue(frame.peek(-2)), R1.scratchReg());

    pushArg(R0.scratchReg());
    pushArg(ImmGCPtr(script->getName(pc)));
    pushArg(R1.scratchReg());
    pushArg(ImmPtr(pc));

    if (!callVM(InitPropGetterSetterInfo))
        return false;

    frame.pop();
    return true;
}

bool
BaselineCompiler::emit_JSOP_INITPROP_GETTER()
{
    return emitInitPropGetterSetter();
}

bool
BaselineCompiler::emit_JSOP_INITPROP_SETTER()
{
    return emitInitPropGetterSetter();
}

typedef bool (*InitElemGetterSetterFn)(JSContext *, jsbytecode *, HandleObject, HandleValue,
                                       HandleObject);
static const VMFunction InitElemGetterSetterInfo =
    FunctionInfo<InitElemGetterSetterFn>(InitGetterSetterOperation);

bool
BaselineCompiler::emitInitElemGetterSetter()
{
    JS_ASSERT(JSOp(*pc) == JSOP_INITELEM_GETTER ||
              JSOp(*pc) == JSOP_INITELEM_SETTER);

    
    
    frame.syncStack(0);
    masm.loadValue(frame.addressOfStackValue(frame.peek(-2)), R0);
    masm.extractObject(frame.addressOfStackValue(frame.peek(-1)), R1.scratchReg());

    prepareVMCall();

    pushArg(R1.scratchReg());
    pushArg(R0);
    masm.extractObject(frame.addressOfStackValue(frame.peek(-3)), R0.scratchReg());
    pushArg(R0.scratchReg());
    pushArg(ImmPtr(pc));

    if (!callVM(InitElemGetterSetterInfo))
        return false;

    frame.popn(2);
    return true;
}

bool
BaselineCompiler::emit_JSOP_INITELEM_GETTER()
{
    return emitInitElemGetterSetter();
}

bool
BaselineCompiler::emit_JSOP_INITELEM_SETTER()
{
    return emitInitElemGetterSetter();
}

bool
BaselineCompiler::emit_JSOP_INITELEM_INC()
{
    
    frame.syncStack(0);

    
    masm.loadValue(frame.addressOfStackValue(frame.peek(-3)), R0);
    masm.loadValue(frame.addressOfStackValue(frame.peek(-2)), R1);

    
    ICSetElem_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.pop();

    
    Address indexAddr = frame.addressOfStackValue(frame.peek(-1));
    masm.incrementInt32Value(indexAddr);
    return true;
}

bool
BaselineCompiler::emit_JSOP_GETLOCAL()
{
    frame.pushLocal(GET_LOCALNO(pc));
    return true;
}

bool
BaselineCompiler::emit_JSOP_SETLOCAL()
{
    
    
    frame.syncStack(1);

    uint32_t local = GET_LOCALNO(pc);
    storeValue(frame.peek(-1), frame.addressOfLocal(local), R0);
    return true;
}

bool
BaselineCompiler::emitFormalArgAccess(uint32_t arg, bool get)
{
    
    
    if (!script->argumentsHasVarBinding() || script->strict()) {
        if (get) {
            frame.pushArg(arg);
        } else {
            
            frame.syncStack(1);
            storeValue(frame.peek(-1), frame.addressOfArg(arg), R0);
        }

        return true;
    }

    
    frame.syncStack(0);

    
    
    
    Label done;
    if (!script->needsArgsObj()) {
        Label hasArgsObj;
        masm.branchTest32(Assembler::NonZero, frame.addressOfFlags(),
                          Imm32(BaselineFrame::HAS_ARGS_OBJ), &hasArgsObj);
        if (get)
            masm.loadValue(frame.addressOfArg(arg), R0);
        else
            storeValue(frame.peek(-1), frame.addressOfArg(arg), R0);
        masm.jump(&done);
        masm.bind(&hasArgsObj);
    }

    
    Register reg = R2.scratchReg();
    masm.loadPtr(Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfArgsObj()), reg);
    masm.loadPrivate(Address(reg, ArgumentsObject::getDataSlotOffset()), reg);

    
    Address argAddr(reg, ArgumentsData::offsetOfArgs() + arg * sizeof(Value));
    if (get) {
        masm.loadValue(argAddr, R0);
        frame.push(R0);
    } else {
        masm.patchableCallPreBarrier(argAddr, MIRType_Value);
        masm.loadValue(frame.addressOfStackValue(frame.peek(-1)), R0);
        masm.storeValue(R0, argAddr);

#ifdef JSGC_GENERATIONAL
        MOZ_ASSERT(frame.numUnsyncedSlots() == 0);

        Register temp = R1.scratchReg();

        
        Register reg = R2.scratchReg();
        masm.loadPtr(Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfArgsObj()), reg);

        Label skipBarrier;

        masm.branchPtrInNurseryRange(Assembler::Equal, reg, temp, &skipBarrier);
        masm.branchValueIsNurseryObject(Assembler::NotEqual, R0, temp, &skipBarrier);

        masm.call(&postBarrierSlot_);

        masm.bind(&skipBarrier);
#endif
    }

    masm.bind(&done);
    return true;
}

bool
BaselineCompiler::emit_JSOP_GETARG()
{
    uint32_t arg = GET_ARGNO(pc);
    return emitFormalArgAccess(arg,  true);
}

bool
BaselineCompiler::emit_JSOP_SETARG()
{
    
    if (!script->argsObjAliasesFormals() && script->argumentsAliasesFormals())
        script->setUninlineable();

    modifiesArguments_ = true;

    uint32_t arg = GET_ARGNO(pc);
    return emitFormalArgAccess(arg,  false);
}

typedef bool (*ThrowUninitializedLexicalFn)(JSContext *cx);
static const VMFunction ThrowUninitializedLexicalInfo =
    FunctionInfo<ThrowUninitializedLexicalFn>(jit::ThrowUninitializedLexical);

bool
BaselineCompiler::emitUninitializedLexicalCheck(const ValueOperand &val)
{
    Label done;
    masm.branchTestMagicValue(Assembler::NotEqual, val, JS_UNINITIALIZED_LEXICAL, &done);

    prepareVMCall();
    if (!callVM(ThrowUninitializedLexicalInfo))
        return false;

    masm.bind(&done);
    return true;
}


bool
BaselineCompiler::emit_JSOP_CHECKLEXICAL()
{
    frame.syncStack(0);
    masm.loadValue(frame.addressOfLocal(GET_LOCALNO(pc)), R0);
    return emitUninitializedLexicalCheck(R0);
}

bool
BaselineCompiler::emit_JSOP_INITLEXICAL()
{
    return emit_JSOP_SETLOCAL();
}

bool
BaselineCompiler::emit_JSOP_CHECKALIASEDLEXICAL()
{
    frame.syncStack(0);
    masm.loadValue(getScopeCoordinateAddress(R0.scratchReg()), R0);
    return emitUninitializedLexicalCheck(R0);
}

bool
BaselineCompiler::emit_JSOP_INITALIASEDLEXICAL()
{
    return emit_JSOP_SETALIASEDVAR();
}

bool
BaselineCompiler::emit_JSOP_UNINITIALIZED()
{
    frame.push(MagicValue(JS_UNINITIALIZED_LEXICAL));
    return true;
}

bool
BaselineCompiler::emitCall()
{
    JS_ASSERT(IsCallPC(pc));

    uint32_t argc = GET_ARGC(pc);

    frame.syncStack(0);
    masm.move32(Imm32(argc), R0.scratchReg());

    
    ICCall_Fallback::Compiler stubCompiler(cx,  JSOp(*pc) == JSOP_NEW,
                                            false);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.popn(argc + 2);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emitSpreadCall()
{
    JS_ASSERT(IsCallPC(pc));

    frame.syncStack(0);
    masm.move32(Imm32(1), R0.scratchReg());

    
    ICCall_Fallback::Compiler stubCompiler(cx,  JSOp(*pc) == JSOP_SPREADNEW,
                                            true);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    
    frame.popn(3);
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_CALL()
{
    return emitCall();
}

bool
BaselineCompiler::emit_JSOP_NEW()
{
    return emitCall();
}

bool
BaselineCompiler::emit_JSOP_FUNCALL()
{
    return emitCall();
}

bool
BaselineCompiler::emit_JSOP_FUNAPPLY()
{
    return emitCall();
}

bool
BaselineCompiler::emit_JSOP_EVAL()
{
    return emitCall();
}

bool
BaselineCompiler::emit_JSOP_SPREADCALL()
{
    return emitSpreadCall();
}

bool
BaselineCompiler::emit_JSOP_SPREADNEW()
{
    return emitSpreadCall();
}

bool
BaselineCompiler::emit_JSOP_SPREADEVAL()
{
    return emitSpreadCall();
}

typedef bool (*ImplicitThisFn)(JSContext *, HandleObject, HandlePropertyName,
                               MutableHandleValue);
static const VMFunction ImplicitThisInfo = FunctionInfo<ImplicitThisFn>(ImplicitThisOperation);

bool
BaselineCompiler::emit_JSOP_IMPLICITTHIS()
{
    frame.syncStack(0);
    masm.loadPtr(frame.addressOfScopeChain(), R0.scratchReg());

    prepareVMCall();

    pushArg(ImmGCPtr(script->getName(pc)));
    pushArg(R0.scratchReg());

    if (!callVM(ImplicitThisInfo))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_INSTANCEOF()
{
    frame.popRegsAndSync(2);

    ICInstanceOf_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_TYPEOF()
{
    frame.popRegsAndSync(1);

    ICTypeOf_Fallback::Compiler stubCompiler(cx);
    if (!emitOpIC(stubCompiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_TYPEOFEXPR()
{
    return emit_JSOP_TYPEOF();
}

typedef bool (*SetCallFn)(JSContext *);
static const VMFunction SetCallInfo = FunctionInfo<SetCallFn>(js::SetCallOperation);

bool
BaselineCompiler::emit_JSOP_SETCALL()
{
    prepareVMCall();
    return callVM(SetCallInfo);
}

typedef bool (*ThrowFn)(JSContext *, HandleValue);
static const VMFunction ThrowInfo = FunctionInfo<ThrowFn>(js::Throw);

bool
BaselineCompiler::emit_JSOP_THROW()
{
    
    frame.popRegsAndSync(1);

    prepareVMCall();
    pushArg(R0);

    return callVM(ThrowInfo);
}

bool
BaselineCompiler::emit_JSOP_TRY()
{
    
    script->setUninlineable();
    return true;
}

bool
BaselineCompiler::emit_JSOP_FINALLY()
{
    
    
    frame.setStackDepth(frame.stackDepth() + 2);

    
    
    return emitInterruptCheck();
}

bool
BaselineCompiler::emit_JSOP_GOSUB()
{
    
    
    
    frame.push(BooleanValue(false));

    int32_t nextOffset = script->pcToOffset(GetNextPc(pc));
    frame.push(Int32Value(nextOffset));

    
    frame.syncStack(0);
    jsbytecode *target = pc + GET_JUMP_OFFSET(pc);
    masm.jump(labelOf(target));
    return true;
}

bool
BaselineCompiler::emit_JSOP_RETSUB()
{
    frame.popRegsAndSync(2);

    ICRetSub_Fallback::Compiler stubCompiler(cx);
    return emitOpIC(stubCompiler.getStub(&stubSpace_));
}

typedef bool (*PushBlockScopeFn)(JSContext *, BaselineFrame *, Handle<StaticBlockObject *>);
static const VMFunction PushBlockScopeInfo = FunctionInfo<PushBlockScopeFn>(jit::PushBlockScope);

bool
BaselineCompiler::emit_JSOP_PUSHBLOCKSCOPE()
{
    StaticBlockObject &blockObj = script->getObject(pc)->as<StaticBlockObject>();

    
    prepareVMCall();
    masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    pushArg(ImmGCPtr(&blockObj));
    pushArg(R0.scratchReg());

    return callVM(PushBlockScopeInfo);
}

typedef bool (*PopBlockScopeFn)(JSContext *, BaselineFrame *);
static const VMFunction PopBlockScopeInfo = FunctionInfo<PopBlockScopeFn>(jit::PopBlockScope);

bool
BaselineCompiler::emit_JSOP_POPBLOCKSCOPE()
{
    
    prepareVMCall();

    masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());
    pushArg(R0.scratchReg());

    return callVM(PopBlockScopeInfo);
}

typedef bool (*DebugLeaveBlockFn)(JSContext *, BaselineFrame *, jsbytecode *);
static const VMFunction DebugLeaveBlockInfo = FunctionInfo<DebugLeaveBlockFn>(jit::DebugLeaveBlock);

bool
BaselineCompiler::emit_JSOP_DEBUGLEAVEBLOCK()
{
    if (!debugMode_)
        return true;

    prepareVMCall();
    masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());
    pushArg(ImmPtr(pc));
    pushArg(R0.scratchReg());

    return callVM(DebugLeaveBlockInfo);
}

typedef bool (*EnterWithFn)(JSContext *, BaselineFrame *, HandleValue, Handle<StaticWithObject *>);
static const VMFunction EnterWithInfo = FunctionInfo<EnterWithFn>(jit::EnterWith);

bool
BaselineCompiler::emit_JSOP_ENTERWITH()
{
    StaticWithObject &withObj = script->getObject(pc)->as<StaticWithObject>();

    
    frame.popRegsAndSync(1);

    
    prepareVMCall();
    masm.loadBaselineFramePtr(BaselineFrameReg, R1.scratchReg());

    pushArg(ImmGCPtr(&withObj));
    pushArg(R0);
    pushArg(R1.scratchReg());

    return callVM(EnterWithInfo);
}

typedef bool (*LeaveWithFn)(JSContext *, BaselineFrame *);
static const VMFunction LeaveWithInfo = FunctionInfo<LeaveWithFn>(jit::LeaveWith);

bool
BaselineCompiler::emit_JSOP_LEAVEWITH()
{
    
    prepareVMCall();

    masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());
    pushArg(R0.scratchReg());

    return callVM(LeaveWithInfo);
}

typedef bool (*GetAndClearExceptionFn)(JSContext *, MutableHandleValue);
static const VMFunction GetAndClearExceptionInfo =
    FunctionInfo<GetAndClearExceptionFn>(GetAndClearException);

bool
BaselineCompiler::emit_JSOP_EXCEPTION()
{
    prepareVMCall();

    if (!callVM(GetAndClearExceptionInfo))
        return false;

    frame.push(R0);
    return true;
}

typedef bool (*OnDebuggerStatementFn)(JSContext *, BaselineFrame *, jsbytecode *pc, bool *);
static const VMFunction OnDebuggerStatementInfo =
    FunctionInfo<OnDebuggerStatementFn>(jit::OnDebuggerStatement);

bool
BaselineCompiler::emit_JSOP_DEBUGGER()
{
    if (!debugMode_)
        return true;

    prepareVMCall();
    pushArg(ImmPtr(pc));

    masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());
    pushArg(R0.scratchReg());

    if (!callVM(OnDebuggerStatementInfo))
        return false;

    
    Label done;
    masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &done);
    {
        masm.loadValue(frame.addressOfReturnValue(), JSReturnOperand);
        masm.jump(&return_);
    }
    masm.bind(&done);
    return true;
}

typedef bool (*DebugEpilogueFn)(JSContext *, BaselineFrame *, jsbytecode *);
static const VMFunction DebugEpilogueInfo =
    FunctionInfo<DebugEpilogueFn>(jit::DebugEpilogueOnBaselineReturn);

bool
BaselineCompiler::emitReturn()
{
    if (debugMode_) {
        
        masm.storeValue(JSReturnOperand, frame.addressOfReturnValue());
        masm.or32(Imm32(BaselineFrame::HAS_RVAL), frame.addressOfFlags());

        
        frame.syncStack(0);
        masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

        prepareVMCall();
        pushArg(ImmPtr(pc));
        pushArg(R0.scratchReg());
        if (!callVM(DebugEpilogueInfo))
            return false;

        
        icEntries_.back().setForDebugEpilogue();

        masm.loadValue(frame.addressOfReturnValue(), JSReturnOperand);
    }

    
    
    
    if (pc + GetBytecodeLength(pc) < script->codeEnd())
        masm.jump(&return_);

    return true;
}

bool
BaselineCompiler::emit_JSOP_RETURN()
{
    JS_ASSERT(frame.stackDepth() == 1);

    frame.popValue(JSReturnOperand);
    return emitReturn();
}

bool
BaselineCompiler::emit_JSOP_RETRVAL()
{
    JS_ASSERT(frame.stackDepth() == 0);

    masm.moveValue(UndefinedValue(), JSReturnOperand);

    if (!script->noScriptRval()) {
        
        Label done;
        Address flags = frame.addressOfFlags();
        masm.branchTest32(Assembler::Zero, flags, Imm32(BaselineFrame::HAS_RVAL), &done);
        masm.loadValue(frame.addressOfReturnValue(), JSReturnOperand);
        masm.bind(&done);
    }

    return emitReturn();
}

typedef bool (*ToIdFn)(JSContext *, HandleScript, jsbytecode *, HandleValue, HandleValue,
                       MutableHandleValue);
static const VMFunction ToIdInfo = FunctionInfo<ToIdFn>(js::ToIdOperation);

bool
BaselineCompiler::emit_JSOP_TOID()
{
    
    frame.syncStack(0);
    masm.loadValue(frame.addressOfStackValue(frame.peek(-1)), R0);

    
    Label done;
    masm.branchTestInt32(Assembler::Equal, R0, &done);

    prepareVMCall();

    masm.loadValue(frame.addressOfStackValue(frame.peek(-2)), R1);

    pushArg(R0);
    pushArg(R1);
    pushArg(ImmPtr(pc));
    pushArg(ImmGCPtr(script));

    if (!callVM(ToIdInfo))
        return false;

    masm.bind(&done);
    frame.pop(); 
    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_TABLESWITCH()
{
    frame.popRegsAndSync(1);

    
    ICTableSwitch::Compiler compiler(cx, pc);
    return emitOpIC(compiler.getStub(&stubSpace_));
}

bool
BaselineCompiler::emit_JSOP_ITER()
{
    frame.popRegsAndSync(1);

    ICIteratorNew_Fallback::Compiler compiler(cx);
    if (!emitOpIC(compiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_MOREITER()
{
    frame.syncStack(0);
    masm.loadValue(frame.addressOfStackValue(frame.peek(-1)), R0);

    ICIteratorMore_Fallback::Compiler compiler(cx);
    if (!emitOpIC(compiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_ITERNEXT()
{
    frame.syncStack(0);
    masm.loadValue(frame.addressOfStackValue(frame.peek(-1)), R0);

    ICIteratorNext_Fallback::Compiler compiler(cx);
    if (!emitOpIC(compiler.getStub(&stubSpace_)))
        return false;

    frame.push(R0);
    return true;
}

bool
BaselineCompiler::emit_JSOP_ENDITER()
{
    frame.popRegsAndSync(1);

    ICIteratorClose_Fallback::Compiler compiler(cx);
    return emitOpIC(compiler.getStub(&stubSpace_));
}

bool
BaselineCompiler::emit_JSOP_SETRVAL()
{
    
    storeValue(frame.peek(-1), frame.addressOfReturnValue(), R2);
    masm.or32(Imm32(BaselineFrame::HAS_RVAL), frame.addressOfFlags());
    frame.pop();
    return true;
}

bool
BaselineCompiler::emit_JSOP_CALLEE()
{
    JS_ASSERT(function());
    frame.syncStack(0);
    masm.loadFunctionFromCalleeToken(frame.addressOfCalleeToken(), R0.scratchReg());
    masm.tagValue(JSVAL_TYPE_OBJECT, R0.scratchReg(), R0);
    frame.push(R0);
    return true;
}

typedef bool (*NewArgumentsObjectFn)(JSContext *, BaselineFrame *, MutableHandleValue);
static const VMFunction NewArgumentsObjectInfo =
    FunctionInfo<NewArgumentsObjectFn>(jit::NewArgumentsObject);

bool
BaselineCompiler::emit_JSOP_ARGUMENTS()
{
    frame.syncStack(0);

    Label done;
    if (!script->argumentsHasVarBinding() || !script->needsArgsObj()) {
        
        
        
        
        masm.moveValue(MagicValue(JS_OPTIMIZED_ARGUMENTS), R0);

        
        Register scratch = R1.scratchReg();
        masm.movePtr(ImmGCPtr(script), scratch);
        masm.loadPtr(Address(scratch, JSScript::offsetOfBaselineScript()), scratch);

        
        masm.branchTest32(Assembler::Zero, Address(scratch, BaselineScript::offsetOfFlags()),
                          Imm32(BaselineScript::NEEDS_ARGS_OBJ), &done);
    }

    prepareVMCall();

    masm.loadBaselineFramePtr(BaselineFrameReg, R0.scratchReg());
    pushArg(R0.scratchReg());

    if (!callVM(NewArgumentsObjectInfo))
        return false;

    masm.bind(&done);
    frame.push(R0);
    return true;
}

typedef bool (*RunOnceScriptPrologueFn)(JSContext *, HandleScript);
static const VMFunction RunOnceScriptPrologueInfo =
    FunctionInfo<RunOnceScriptPrologueFn>(js::RunOnceScriptPrologue);

bool
BaselineCompiler::emit_JSOP_RUNONCE()
{
    frame.syncStack(0);

    prepareVMCall();

    masm.movePtr(ImmGCPtr(script), R0.scratchReg());
    pushArg(R0.scratchReg());

    return callVM(RunOnceScriptPrologueInfo);
}

bool
BaselineCompiler::emit_JSOP_REST()
{
    frame.syncStack(0);

    JSObject *templateObject = NewDenseUnallocatedArray(cx, 0, nullptr, TenuredObject);
    if (!templateObject)
        return false;
    types::FixRestArgumentsType(cx, templateObject);

    
    ICRest_Fallback::Compiler compiler(cx, templateObject);
    if (!emitOpIC(compiler.getStub(&stubSpace_)))
        return false;

    
    frame.push(R0);
    return true;
}
