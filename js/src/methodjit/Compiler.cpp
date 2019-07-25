








































#include "MethodJIT.h"
#include "jsnum.h"
#include "jsbool.h"
#include "jsemit.h"
#include "jsiter.h"
#include "Compiler.h"
#include "StubCalls.h"
#include "MonoIC.h"
#include "PolyIC.h"
#include "ICChecker.h"
#include "Retcon.h"
#include "assembler/jit/ExecutableAllocator.h"
#include "assembler/assembler/LinkBuffer.h"
#include "FrameState-inl.h"
#include "jsobjinlines.h"
#include "jsscriptinlines.h"
#include "InlineFrameAssembler.h"
#include "jscompartment.h"
#include "jsobjinlines.h"
#include "jsopcodeinlines.h"
#include "jshotloop.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;
#if defined(JS_POLYIC) || defined(JS_MONOIC)
using namespace js::mjit::ic;
#endif

#define RETURN_IF_OOM(retval)                                   \
    JS_BEGIN_MACRO                                              \
        if (oomInVector || masm.oom() || stubcc.masm.oom())     \
            return retval;                                      \
    JS_END_MACRO

#if defined(JS_METHODJIT_SPEW)
static const char *OpcodeNames[] = {
# define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) #name,
# include "jsopcode.tbl"
# undef OPDEF
};
#endif





static const size_t CALLS_BACKEDGES_BEFORE_INLINING = 10000;

mjit::Compiler::Compiler(JSContext *cx, JSScript *outerScript, bool isConstructing,
                         const Vector<PatchableFrame> *patchFrames)
  : BaseCompiler(cx),
    outerScript(outerScript),
    isConstructing(isConstructing),
    globalObj(outerScript->global),
    globalSlots((globalObj && globalObj->isGlobal()) ? globalObj->getRawSlots() : NULL),
    patchFrames(patchFrames),
    savedTraps(NULL),
    frame(cx, *thisFromCtor(), masm, stubcc),
    a(NULL), outer(NULL), script(NULL), PC(NULL), loop(NULL),
    inlineFrames(CompilerAllocPolicy(cx, *thisFromCtor())),
    branchPatches(CompilerAllocPolicy(cx, *thisFromCtor())),
#if defined JS_MONOIC
    getGlobalNames(CompilerAllocPolicy(cx, *thisFromCtor())),
    setGlobalNames(CompilerAllocPolicy(cx, *thisFromCtor())),
    callICs(CompilerAllocPolicy(cx, *thisFromCtor())),
    equalityICs(CompilerAllocPolicy(cx, *thisFromCtor())),
    traceICs(CompilerAllocPolicy(cx, *thisFromCtor())),
#endif
#if defined JS_POLYIC
    pics(CompilerAllocPolicy(cx, *thisFromCtor())), 
    getElemICs(CompilerAllocPolicy(cx, *thisFromCtor())),
    setElemICs(CompilerAllocPolicy(cx, *thisFromCtor())),
#endif
    callPatches(CompilerAllocPolicy(cx, *thisFromCtor())),
    callSites(CompilerAllocPolicy(cx, *thisFromCtor())),
    rejoinSites(CompilerAllocPolicy(cx, *thisFromCtor())),
    doubleList(CompilerAllocPolicy(cx, *thisFromCtor())),
    fixedDoubleEntries(CompilerAllocPolicy(cx, *thisFromCtor())),
    jumpTables(CompilerAllocPolicy(cx, *thisFromCtor())),
    jumpTableOffsets(CompilerAllocPolicy(cx, *thisFromCtor())),
    loopEntries(CompilerAllocPolicy(cx, *thisFromCtor())),
    stubcc(cx, *thisFromCtor(), frame),
    debugMode_(cx->compartment->debugMode),
#if defined JS_TRACER
    addTraceHints(cx->traceJitEnabled),
#else
    addTraceHints(false),
#endif
    inlining(false),
    hasGlobalReallocation(false),
    oomInVector(false),
    applyTricks(NoApplyTricks)
{
    JS_ASSERT(!outerScript->isUncachedEval);

    
    if (cx->typeInferenceEnabled())
        addTraceHints = false;

    





    if (outerScript->callCount() >= CALLS_BACKEDGES_BEFORE_INLINING ||
        cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS)) {
        inlining = true;
    }
}

CompileStatus
mjit::Compiler::compile()
{
    JS_ASSERT_IF(isConstructing, !outerScript->jitCtor);
    JS_ASSERT_IF(!isConstructing, !outerScript->jitNormal);

    JITScript **jit = isConstructing ? &outerScript->jitCtor : &outerScript->jitNormal;
    void **checkAddr = isConstructing
                       ? &outerScript->jitArityCheckCtor
                       : &outerScript->jitArityCheckNormal;

    CompileStatus status = performCompilation(jit);
    if (status == Compile_Okay) {
        
        
        
        
        *checkAddr = (*jit)->arityCheckEntry
                     ? (*jit)->arityCheckEntry
                     : (*jit)->invokeEntry;
    } else {
        *checkAddr = JS_UNJITTABLE_SCRIPT;
        if (outerScript->fun && !cx->markTypeFunctionUninlineable(outerScript->fun->getType()))
            return Compile_Error;
    }

    return status;
}

CompileStatus
mjit::Compiler::pushActiveFrame(JSScript *script, uint32 argc)
{
    ActiveFrame *newa = cx->new_<ActiveFrame>(cx);
    if (!newa)
        return Compile_Error;

    newa->parent = a;
    if (a)
        newa->parentPC = PC;
    newa->script = script;

    if (outer) {
        newa->inlineIndex = uint32(inlineFrames.length());
        inlineFrames.append(newa);
    } else {
        newa->inlineIndex = uint32(-1);
        outer = newa;
    }

    analyze::ScriptAnalysis *newAnalysis = script->analysis(cx);
    if (!newAnalysis)
        return Compile_Error;
    if (!newAnalysis->failed() && !newAnalysis->ranBytecode())
        newAnalysis->analyzeBytecode(cx);

    if (newAnalysis->OOM())
        return Compile_Error;
    if (newAnalysis->failed()) {
        JaegerSpew(JSpew_Abort, "couldn't analyze bytecode; probably switchX or OOM\n");
        return Compile_Abort;
    }

    if (cx->typeInferenceEnabled()) {
        if (!newAnalysis->ranSSA())
            newAnalysis->analyzeSSA(cx);
        if (!newAnalysis->failed() && !newAnalysis->ranLifetimes())
            newAnalysis->analyzeLifetimes(cx);
        if (newAnalysis->failed()) {
            js_ReportOutOfMemory(cx);
            return Compile_Error;
        }
    }

#ifdef JS_METHODJIT_SPEW
    if (cx->typeInferenceEnabled() && IsJaegerSpewChannelActive(JSpew_Regalloc)) {
        unsigned nargs = script->fun ? script->fun->nargs : 0;
        for (unsigned i = 0; i < nargs; i++) {
            uint32 slot = analyze::ArgSlot(i);
            if (!newAnalysis->slotEscapes(slot)) {
                JaegerSpew(JSpew_Regalloc, "Argument %u:", i);
                newAnalysis->liveness(slot).print();
            }
        }
        for (unsigned i = 0; i < script->nfixed; i++) {
            uint32 slot = analyze::LocalSlot(script, i);
            if (!newAnalysis->slotEscapes(slot)) {
                JaegerSpew(JSpew_Regalloc, "Local %u:", i);
                newAnalysis->liveness(slot).print();
            }
        }
    }
#endif

    if (a)
        frame.getUnsyncedEntries(&newa->depth, &newa->unsyncedEntries);

    if (!frame.pushActiveFrame(script, argc)) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }

    newa->jumpMap = (Label *)cx->malloc_(sizeof(Label) * script->length);
    if (!newa->jumpMap) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }
#ifdef DEBUG
    for (uint32 i = 0; i < script->length; i++)
        newa->jumpMap[i] = Label();
#endif

    if (cx->typeInferenceEnabled()) {
        CompileStatus status = prepareInferenceTypes(script, newa);
        if (status != Compile_Okay)
            return status;
    }

    this->script = script;
    this->analysis = newAnalysis;
    this->PC = script->code;
    this->a = newa;

    variadicRejoin = false;

    return Compile_Okay;
}

void
mjit::Compiler::popActiveFrame()
{
    JS_ASSERT(a->parent);
    this->PC = a->parentPC;
    this->a = a->parent;
    this->script = a->script;
    this->analysis = this->script->analysis(cx);

    frame.popActiveFrame();
}

#define CHECK_STATUS(expr)                                           \
    JS_BEGIN_MACRO                                                   \
        CompileStatus status_ = (expr);                              \
        if (status_ != Compile_Okay) {                               \
            if (oomInVector || masm.oom() || stubcc.masm.oom())      \
                js_ReportOutOfMemory(cx);                            \
            return status_;                                          \
        }                                                            \
    JS_END_MACRO

CompileStatus
mjit::Compiler::performCompilation(JITScript **jitp)
{
    JaegerSpew(JSpew_Scripts, "compiling script (file \"%s\") (line \"%d\") (length \"%d\")\n",
               outerScript->filename, outerScript->lineno, outerScript->length);

    if (inlining) {
        JaegerSpew(JSpew_Inlining, "inlining calls in script (file \"%s\") (line \"%d\")\n",
                   outerScript->filename, outerScript->lineno);
    }

#ifdef JS_METHODJIT_SPEW
    Profiler prof;
    prof.start();
#endif

#ifdef JS_METHODJIT
    outerScript->debugMode = debugMode();
#endif

    JS_ASSERT(cx->compartment->activeInference);

    {
        types::AutoEnterCompilation enter(cx, outerScript);

        CHECK_STATUS(pushActiveFrame(outerScript, 0));
        CHECK_STATUS(generatePrologue());
        CHECK_STATUS(generateMethod());
        CHECK_STATUS(generateEpilogue());
        CHECK_STATUS(finishThisUp(jitp));
    }

#ifdef JS_METHODJIT_SPEW
    prof.stop();
    JaegerSpew(JSpew_Prof, "compilation took %d us\n", prof.time_us());
#endif

    JaegerSpew(JSpew_Scripts, "successfully compiled (code \"%p\") (size \"%ld\")\n",
               (*jitp)->code.m_code.executableAddress(), (*jitp)->code.m_size);

    if (!*jitp)
        return Compile_Abort;

    



    bool expanded = false;
    for (unsigned i = 0; i < (*jitp)->nInlineFrames; i++) {
        JSScript *script = (*jitp)->inlineFrames()[i].fun->script();

        script->inlineParents = true;

        
        JS_ASSERT(script->jitArityCheckNormal != JS_UNJITTABLE_SCRIPT);

        if (script->jitNormal && !script->jitNormal->rejoinPoints) {
            if (!expanded) {
                ExpandInlineFrames(cx, true);
                expanded = true;
            }
            mjit::Recompiler recompiler(cx, script);
            if (!recompiler.recompile()) {
                ReleaseScriptCode(cx, outerScript, true);
                return Compile_Error;
            }
        }

        if (!script->jitNormal) {
            CompileStatus status = Compile_Retry;
            while (status == Compile_Retry) {
                mjit::Compiler cc(cx, script, isConstructing, NULL);
                status = cc.compile();
            }
            if (status != Compile_Okay) {
                ReleaseScriptCode(cx, outerScript, true);
                return status;
            }
        }
    }

    return Compile_Okay;
}

#undef CHECK_STATUS

mjit::Compiler::ActiveFrame::ActiveFrame(JSContext *cx)
    : parent(NULL), parentPC(NULL), script(NULL), inlineIndex(uint32(-1)),
      jumpMap(NULL), unsyncedEntries(cx),
      needReturnValue(false), syncReturnValue(false),
      returnValueDouble(false), returnSet(false), returnParentRegs(0),
      temporaryParentRegs(0), returnJumps(NULL)
{}

mjit::Compiler::ActiveFrame::~ActiveFrame()
{
    js::Foreground::free_(jumpMap);
}

mjit::Compiler::~Compiler()
{
    if (outer)
        cx->delete_(outer);
    for (unsigned i = 0; i < inlineFrames.length(); i++)
        cx->delete_(inlineFrames[i]);

    cx->free_(savedTraps);
}

CompileStatus
mjit::Compiler::prepareInferenceTypes(JSScript *script, ActiveFrame *a)
{
    
    analyze::ScriptAnalysis *analysis = script->analysis(cx);
    if (!analysis->ranInference()) {
        analysis->analyzeTypes(cx);
        if (!analysis->ranInference())
            return Compile_Error;
    }

    






















    a->varTypes = (VarType *)
        cx->calloc_(analyze::TotalSlots(script) * sizeof(VarType));
    if (!a->varTypes)
        return Compile_Error;

    for (uint32 slot = analyze::ArgSlot(0); slot < analyze::TotalSlots(script); slot++) {
        VarType &vt = a->varTypes[slot];
        vt.types = script->slotTypes(slot);
        vt.type = vt.types->getKnownTypeTag(cx);
    }

    return Compile_Okay;
}

CompileStatus JS_NEVER_INLINE
mjit::TryCompile(JSContext *cx, StackFrame *fp)
{
    JS_ASSERT(cx->fp() == fp);

#if JS_HAS_SHARP_VARS
    if (fp->script()->hasSharps)
        return Compile_Abort;
#endif

    
    if (fp->script()->isUncachedEval)
        return Compile_Abort;

    
    if (fp->isConstructing() && !fp->script()->nslots)
        fp->script()->nslots++;

    types::AutoEnterTypeInference enter(cx, true);

    
    
    
    CompileStatus status = Compile_Retry;
    for (unsigned i = 0; status == Compile_Retry && i < 5; i++) {
        Compiler cc(cx, fp->script(), fp->isConstructing(), NULL);
        status = cc.compile();
    }

    return status;
}

bool
mjit::Compiler::loadOldTraps(const Vector<CallSite> &sites)
{
    savedTraps = (bool *)cx->calloc_(sizeof(bool) * outerScript->length);
    if (!savedTraps)
        return false;
    
    for (size_t i = 0; i < sites.length(); i++) {
        const CallSite &site = sites[i];
        if (site.isTrap()) {
            JS_ASSERT(site.inlineIndex == uint32(-1) && site.pcOffset < outerScript->length);
            savedTraps[site.pcOffset] = true;
        }
    }

    return true;
}

CompileStatus
mjit::Compiler::generatePrologue()
{
    invokeLabel = masm.label();

    



    if (script->fun) {
        Jump j = masm.jump();

        



        invokeLabel = masm.label();

        Label fastPath = masm.label();

        
        masm.storePtr(ImmPtr(script->fun), Address(JSFrameReg, StackFrame::offsetOfExec()));

        {
            REJOIN_SITE(stubs::CheckArgumentTypes);

            





            arityLabel = stubcc.masm.label();

            Jump argMatch = stubcc.masm.branch32(Assembler::Equal, JSParamReg_Argc,
                                                 Imm32(script->fun->nargs));

            if (JSParamReg_Argc != Registers::ArgReg1)
                stubcc.masm.move(JSParamReg_Argc, Registers::ArgReg1);

            
            stubcc.masm.storePtr(ImmPtr(script->fun),
                                 Address(JSFrameReg, StackFrame::offsetOfExec()));
            OOL_STUBCALL_NO_REJOIN(stubs::FixupArity);
            stubcc.masm.move(Registers::ReturnReg, JSFrameReg);
            argMatch.linkTo(stubcc.masm.label(), &stubcc.masm);

            
            if (cx->typeInferenceEnabled()) {
#ifdef JS_MONOIC
                this->argsCheckJump = stubcc.masm.jump();
                this->argsCheckStub = stubcc.masm.label();
                this->argsCheckJump.linkTo(this->argsCheckStub, &stubcc.masm);
#endif
                stubcc.masm.storePtr(ImmPtr(script->fun), Address(JSFrameReg, StackFrame::offsetOfExec()));
                OOL_STUBCALL(stubs::CheckArgumentTypes);
#ifdef JS_MONOIC
                this->argsCheckFallthrough = stubcc.masm.label();
#endif
            }

            stubcc.crossJump(stubcc.masm.jump(), fastPath);
        }

        




        JS_STATIC_ASSERT(StackSpace::STACK_EXTRA >= VALUES_PER_STACK_FRAME);
        uint32 nvals = script->nslots + VALUES_PER_STACK_FRAME + StackSpace::STACK_EXTRA;
        masm.addPtr(Imm32(nvals * sizeof(Value)), JSFrameReg, Registers::ReturnReg);
        Jump stackCheck = masm.branchPtr(Assembler::AboveOrEqual, Registers::ReturnReg,
                                         FrameAddress(offsetof(VMFrame, stackLimit)));

        
        {
            stubcc.linkExitDirect(stackCheck, stubcc.masm.label());
            OOL_STUBCALL_NO_REJOIN(stubs::HitStackQuota);
            stubcc.crossJump(stubcc.masm.jump(), masm.label());
        }

        




        for (uint32 i = 0; i < script->nfixed; i++) {
            if (analysis->localHasUseBeforeDef(i) || addTraceHints) {
                Address local(JSFrameReg, sizeof(StackFrame) + i * sizeof(Value));
                masm.storeValue(UndefinedValue(), local);
            }
        }

        
        if (script->fun->isHeavyweight()) {
            prepareStubCall(Uses(0));
            INLINE_STUBCALL_NO_REJOIN(stubs::CreateFunCallObject);
        }

        j.linkTo(masm.label(), &masm);

        if (analysis->usesScopeChain() && !script->fun->isHeavyweight()) {
            




            RegisterID t0 = Registers::ReturnReg;
            Jump hasScope = masm.branchTest32(Assembler::NonZero,
                                              FrameFlagsAddress(), Imm32(StackFrame::HAS_SCOPECHAIN));
            masm.loadPayload(Address(JSFrameReg, StackFrame::offsetOfCallee(script->fun)), t0);
            masm.loadPtr(Address(t0, offsetof(JSObject, parent)), t0);
            masm.storePtr(t0, Address(JSFrameReg, StackFrame::offsetOfScopeChain()));
            hasScope.linkTo(masm.label(), &masm);
        }
    }

    if (isConstructing)
        constructThis();

    if (debugMode() || Probes::callTrackingActive(cx)) {
        REJOIN_SITE(stubs::ScriptDebugPrologue);
        INLINE_STUBCALL(stubs::ScriptDebugPrologue);
    }

    if (cx->typeInferenceEnabled()) {
        
        for (uint32 i = 0; script->fun && i < script->fun->nargs; i++) {
            uint32 slot = analyze::ArgSlot(i);
            if (a->varTypes[slot].type == JSVAL_TYPE_DOUBLE && analysis->trackSlot(slot))
                frame.ensureDouble(frame.getArg(i));
        }
    }

    recompileCheckHelper();

    return Compile_Okay;
}

CompileStatus
mjit::Compiler::generateEpilogue()
{
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::finishThisUp(JITScript **jitp)
{
    RETURN_IF_OOM(Compile_Error);

    



    if (globalSlots && globalObj->getRawSlots() != globalSlots)
        return Compile_Retry;

    for (size_t i = 0; i < branchPatches.length(); i++) {
        Label label = labelOf(branchPatches[i].pc, branchPatches[i].inlineIndex);
        branchPatches[i].jump.linkTo(label, &masm);
    }

#ifdef JS_CPU_ARM
    masm.forceFlushConstantPool();
    stubcc.masm.forceFlushConstantPool();
#endif
    JaegerSpew(JSpew_Insns, "## Fast code (masm) size = %u, Slow code (stubcc) size = %u.\n", masm.size(), stubcc.size());

    size_t totalSize = masm.size() +
                       stubcc.size() +
                       (masm.numDoubles() * sizeof(double)) +
                       (stubcc.masm.numDoubles() * sizeof(double)) +
                       jumpTableOffsets.length() * sizeof(void *);

    JSC::ExecutablePool *execPool;
    uint8 *result =
        (uint8 *)script->compartment->jaegerCompartment->execAlloc()->alloc(totalSize, &execPool);
    if (!result) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }
    JS_ASSERT(execPool);
    JSC::ExecutableAllocator::makeWritable(result, totalSize);
    masm.executableCopy(result);
    stubcc.masm.executableCopy(result + masm.size());
    
    JSC::LinkBuffer fullCode(result, totalSize);
    JSC::LinkBuffer stubCode(result + masm.size(), stubcc.size());

    size_t nNmapLive = loopEntries.length();
    for (size_t i = 0; i < script->length; i++) {
        analyze::Bytecode *opinfo = analysis->maybeCode(i);
        if (opinfo && opinfo->safePoint) {
            
            if (!cx->typeInferenceEnabled() || !opinfo->loopHead)
                nNmapLive++;
        }
    }

    size_t nUnsyncedEntries = 0;
    for (size_t i = 0; i < inlineFrames.length(); i++)
        nUnsyncedEntries += inlineFrames[i]->unsyncedEntries.length();

    
    size_t totalBytes = sizeof(JITScript) +
                        sizeof(NativeMapEntry) * nNmapLive +
                        sizeof(InlineFrame) * inlineFrames.length() +
                        sizeof(CallSite) * callSites.length() +
                        sizeof(RejoinSite) * rejoinSites.length() +
#if defined JS_MONOIC
                        sizeof(ic::GetGlobalNameIC) * getGlobalNames.length() +
                        sizeof(ic::SetGlobalNameIC) * setGlobalNames.length() +
                        sizeof(ic::CallICInfo) * callICs.length() +
                        sizeof(ic::EqualityICInfo) * equalityICs.length() +
                        sizeof(ic::TraceICInfo) * traceICs.length() +
#endif
#if defined JS_POLYIC
                        sizeof(ic::PICInfo) * pics.length() +
                        sizeof(ic::GetElementIC) * getElemICs.length() +
                        sizeof(ic::SetElementIC) * setElemICs.length() +
#endif
                        sizeof(UnsyncedEntry) * nUnsyncedEntries;

    uint8 *cursor = (uint8 *)cx->calloc_(totalBytes);
    if (!cursor) {
        execPool->release();
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }

    JITScript *jit = new(cursor) JITScript;
    cursor += sizeof(JITScript);

    JS_ASSERT(outerScript == script);

    jit->script = script;
    jit->code = JSC::MacroAssemblerCodeRef(result, execPool, masm.size() + stubcc.size());
    jit->invokeEntry = result;
    jit->singleStepMode = script->singleStepMode;
    jit->rejoinPoints = script->inlineParents;
    if (script->fun) {
        jit->arityCheckEntry = stubCode.locationOf(arityLabel).executableAddress();
        jit->fastEntry = fullCode.locationOf(invokeLabel).executableAddress();
    }

    




    
    Label *jumpMap = a->jumpMap;

    
    NativeMapEntry *jitNmap = (NativeMapEntry *)cursor;
    jit->nNmapPairs = nNmapLive;
    cursor += sizeof(NativeMapEntry) * jit->nNmapPairs;
    size_t ix = 0;
    if (jit->nNmapPairs > 0) {
        for (size_t i = 0; i < script->length; i++) {
            analyze::Bytecode *opinfo = analysis->maybeCode(i);
            if (opinfo && opinfo->safePoint) {
                Label L = jumpMap[i];
                JS_ASSERT(L.isValid());
                jitNmap[ix].bcOff = i;
                jitNmap[ix].ncode = (uint8 *)(result + masm.distanceOf(L));
                ix++;
            }
        }
        for (size_t i = 0; i < loopEntries.length(); i++) {
            
            const LoopEntry &entry = loopEntries[i];
            size_t j;
            for (j = 0; j < ix; j++) {
                if (jitNmap[j].bcOff > entry.pcOffset) {
                    memmove(jitNmap + j + 1, jitNmap + j, (ix - j) * sizeof(NativeMapEntry));
                    break;
                }
            }
            jitNmap[j].bcOff = entry.pcOffset;
            jitNmap[j].ncode = (uint8 *) stubCode.locationOf(entry.label).executableAddress();
            ix++;
        }
    }
    JS_ASSERT(ix == jit->nNmapPairs);

    
    InlineFrame *jitInlineFrames = (InlineFrame *)cursor;
    jit->nInlineFrames = inlineFrames.length();
    cursor += sizeof(InlineFrame) * jit->nInlineFrames;
    for (size_t i = 0; i < jit->nInlineFrames; i++) {
        InlineFrame &to = jitInlineFrames[i];
        ActiveFrame *from = inlineFrames[i];
        if (from->parent != outer)
            to.parent = &jitInlineFrames[from->parent->inlineIndex];
        else
            to.parent = NULL;
        to.parentpc = from->parentPC;
        to.fun = from->script->fun;
        to.depth = from->depth;
    }

    
    CallSite *jitCallSites = (CallSite *)cursor;
    jit->nCallSites = callSites.length();
    cursor += sizeof(CallSite) * jit->nCallSites;
    for (size_t i = 0; i < jit->nCallSites; i++) {
        CallSite &to = jitCallSites[i];
        InternalCallSite &from = callSites[i];

        





        JS_ASSERT_IF(cx->typeInferenceEnabled(), !from.needsRejoin);

        
        if (cx->typeInferenceEnabled() &&
            from.id != CallSite::NCODE_RETURN_ID &&
            from.id != CallSite::MAGIC_TRAP_ID &&
            from.inlineIndex != uint32(-1)) {
            if (from.ool)
                stubCode.patch(from.inlinePatch, &to);
            else
                fullCode.patch(from.inlinePatch, &to);
        }

        JSScript *script =
            (from.inlineIndex == uint32(-1)) ? outerScript : inlineFrames[from.inlineIndex]->script;
        uint32 codeOffset = from.ool
                            ? masm.size() + from.returnOffset
                            : from.returnOffset;
        to.initialize(codeOffset, from.inlineIndex, from.inlinepc - script->code, from.id);

        




        if (from.loopPatch.hasPatch)
            stubCode.patch(from.loopPatch.codePatch, result + codeOffset);
    }

    
    RejoinSite *jitRejoinSites = (RejoinSite *)cursor;
    jit->nRejoinSites = rejoinSites.length();
    cursor += sizeof(RejoinSite) * jit->nRejoinSites;
    for (size_t i = 0; i < jit->nRejoinSites; i++) {
        RejoinSite &to = jitRejoinSites[i];
        InternalRejoinSite &from = rejoinSites[i];

        uint32 codeOffset = (uint8 *) stubCode.locationOf(from.label).executableAddress() - result;
        to.initialize(codeOffset, from.pc - outerScript->code, from.id);

        




        if (from.loopPatch.hasPatch)
            stubCode.patch(from.loopPatch.codePatch, result + codeOffset);
    }

#if defined JS_MONOIC
    JS_INIT_CLIST(&jit->callers);

    if (script->fun && cx->typeInferenceEnabled()) {
        jit->argsCheckStub = stubCode.locationOf(argsCheckStub);
        jit->argsCheckFallthrough = stubCode.locationOf(argsCheckFallthrough);
        jit->argsCheckJump = stubCode.locationOf(argsCheckJump);
        jit->argsCheckPool = NULL;
    }

    ic::GetGlobalNameIC *getGlobalNames_ = (ic::GetGlobalNameIC *)cursor;
    jit->nGetGlobalNames = getGlobalNames.length();
    cursor += sizeof(ic::GetGlobalNameIC) * jit->nGetGlobalNames;
    for (size_t i = 0; i < jit->nGetGlobalNames; i++) {
        ic::GetGlobalNameIC &to = getGlobalNames_[i];
        GetGlobalNameICInfo &from = getGlobalNames[i];
        from.copyTo(to, fullCode, stubCode);

        int offset = fullCode.locationOf(from.load) - to.fastPathStart;
        to.loadStoreOffset = offset;
        JS_ASSERT(to.loadStoreOffset == offset);

        stubCode.patch(from.addrLabel, &to);
    }

    ic::SetGlobalNameIC *setGlobalNames_ = (ic::SetGlobalNameIC *)cursor;
    jit->nSetGlobalNames = setGlobalNames.length();
    cursor += sizeof(ic::SetGlobalNameIC) * jit->nSetGlobalNames;
    for (size_t i = 0; i < jit->nSetGlobalNames; i++) {
        ic::SetGlobalNameIC &to = setGlobalNames_[i];
        SetGlobalNameICInfo &from = setGlobalNames[i];
        from.copyTo(to, fullCode, stubCode);
        to.slowPathStart = stubCode.locationOf(from.slowPathStart);

        int offset = fullCode.locationOf(from.store).labelAtOffset(0) -
                     to.fastPathStart;
        to.loadStoreOffset = offset;
        JS_ASSERT(to.loadStoreOffset == offset);

        to.hasExtraStub = 0;
        to.objConst = from.objConst;
        to.shapeReg = from.shapeReg;
        to.objReg = from.objReg;
        to.vr = from.vr;

        offset = fullCode.locationOf(from.shapeGuardJump) -
                 to.fastPathStart;
        to.inlineShapeJump = offset;
        JS_ASSERT(to.inlineShapeJump == offset);

        offset = fullCode.locationOf(from.fastPathRejoin) -
                 to.fastPathStart;
        to.fastRejoinOffset = offset;
        JS_ASSERT(to.fastRejoinOffset == offset);

        stubCode.patch(from.addrLabel, &to);
    }

    ic::CallICInfo *jitCallICs = (ic::CallICInfo *)cursor;
    jit->nCallICs = callICs.length();
    cursor += sizeof(ic::CallICInfo) * jit->nCallICs;
    for (size_t i = 0; i < jit->nCallICs; i++) {
        jitCallICs[i].reset();
        jitCallICs[i].funGuard = fullCode.locationOf(callICs[i].funGuard);
        jitCallICs[i].funJump = fullCode.locationOf(callICs[i].funJump);
        jitCallICs[i].slowPathStart = stubCode.locationOf(callICs[i].slowPathStart);
        jitCallICs[i].typeMonitored = callICs[i].typeMonitored;
        jitCallICs[i].argTypes = callICs[i].argTypes;

        
        uint32 offset = fullCode.locationOf(callICs[i].hotJump) -
                        fullCode.locationOf(callICs[i].funGuard);
        jitCallICs[i].hotJumpOffset = offset;
        JS_ASSERT(jitCallICs[i].hotJumpOffset == offset);

        
        offset = fullCode.locationOf(callICs[i].joinPoint) -
                 fullCode.locationOf(callICs[i].funGuard);
        jitCallICs[i].joinPointOffset = offset;
        JS_ASSERT(jitCallICs[i].joinPointOffset == offset);
                                        
        
        offset = stubCode.locationOf(callICs[i].oolCall) -
                 stubCode.locationOf(callICs[i].slowPathStart);
        jitCallICs[i].oolCallOffset = offset;
        JS_ASSERT(jitCallICs[i].oolCallOffset == offset);

        
        offset = stubCode.locationOf(callICs[i].oolJump) -
                 stubCode.locationOf(callICs[i].slowPathStart);
        jitCallICs[i].oolJumpOffset = offset;
        JS_ASSERT(jitCallICs[i].oolJumpOffset == offset);

        
        offset = stubCode.locationOf(callICs[i].icCall) -
                 stubCode.locationOf(callICs[i].slowPathStart);
        jitCallICs[i].icCallOffset = offset;
        JS_ASSERT(jitCallICs[i].icCallOffset == offset);

        
        offset = stubCode.locationOf(callICs[i].slowJoinPoint) -
                 stubCode.locationOf(callICs[i].slowPathStart);
        jitCallICs[i].slowJoinOffset = offset;
        JS_ASSERT(jitCallICs[i].slowJoinOffset == offset);

        
        offset = stubCode.locationOf(callICs[i].hotPathLabel) -
                 stubCode.locationOf(callICs[i].funGuard);
        jitCallICs[i].hotPathOffset = offset;
        JS_ASSERT(jitCallICs[i].hotPathOffset == offset);

        jitCallICs[i].call = &jitCallSites[callICs[i].callIndex];
        jitCallICs[i].frameSize = callICs[i].frameSize;
        jitCallICs[i].funObjReg = callICs[i].funObjReg;
        jitCallICs[i].funPtrReg = callICs[i].funPtrReg;
        stubCode.patch(callICs[i].addrLabel1, &jitCallICs[i]);
        stubCode.patch(callICs[i].addrLabel2, &jitCallICs[i]);
    }

    ic::EqualityICInfo *jitEqualityICs = (ic::EqualityICInfo *)cursor;
    jit->nEqualityICs = equalityICs.length();
    cursor += sizeof(ic::EqualityICInfo) * jit->nEqualityICs;
    for (size_t i = 0; i < jit->nEqualityICs; i++) {
        if (equalityICs[i].trampoline) {
            jitEqualityICs[i].target = stubCode.locationOf(equalityICs[i].trampolineStart);
        } else {
            uint32 offs = uint32(equalityICs[i].jumpTarget - script->code);
            JS_ASSERT(jumpMap[offs].isValid());
            jitEqualityICs[i].target = fullCode.locationOf(jumpMap[offs]);
        }
        jitEqualityICs[i].stubEntry = stubCode.locationOf(equalityICs[i].stubEntry);
        jitEqualityICs[i].stubCall = stubCode.locationOf(equalityICs[i].stubCall);
        jitEqualityICs[i].stub = equalityICs[i].stub;
        jitEqualityICs[i].lvr = equalityICs[i].lvr;
        jitEqualityICs[i].rvr = equalityICs[i].rvr;
        jitEqualityICs[i].tempReg = equalityICs[i].tempReg;
        jitEqualityICs[i].cond = equalityICs[i].cond;
        if (equalityICs[i].jumpToStub.isSet())
            jitEqualityICs[i].jumpToStub = fullCode.locationOf(equalityICs[i].jumpToStub.get());
        jitEqualityICs[i].fallThrough = fullCode.locationOf(equalityICs[i].fallThrough);
        
        stubCode.patch(equalityICs[i].addrLabel, &jitEqualityICs[i]);
    }

    ic::TraceICInfo *jitTraceICs = (ic::TraceICInfo *)cursor;
    jit->nTraceICs = traceICs.length();
    cursor += sizeof(ic::TraceICInfo) * jit->nTraceICs;
    for (size_t i = 0; i < jit->nTraceICs; i++) {
        jitTraceICs[i].initialized = traceICs[i].initialized;
        if (!traceICs[i].initialized)
            continue;

        if (traceICs[i].fastTrampoline) {
            jitTraceICs[i].fastTarget = stubCode.locationOf(traceICs[i].trampolineStart);
        } else {
            uint32 offs = uint32(traceICs[i].jumpTarget - script->code);
            JS_ASSERT(jumpMap[offs].isValid());
            jitTraceICs[i].fastTarget = fullCode.locationOf(jumpMap[offs]);
        }
        jitTraceICs[i].slowTarget = stubCode.locationOf(traceICs[i].trampolineStart);

        jitTraceICs[i].traceHint = fullCode.locationOf(traceICs[i].traceHint);
        jitTraceICs[i].stubEntry = stubCode.locationOf(traceICs[i].stubEntry);
        jitTraceICs[i].traceData = NULL;
#ifdef DEBUG
        jitTraceICs[i].jumpTargetPC = traceICs[i].jumpTarget;
#endif

        jitTraceICs[i].hasSlowTraceHint = traceICs[i].slowTraceHint.isSet();
        if (traceICs[i].slowTraceHint.isSet())
            jitTraceICs[i].slowTraceHint = stubCode.locationOf(traceICs[i].slowTraceHint.get());
#ifdef JS_TRACER
        uint32 hotloop = GetHotloop(cx);
        uint32 prevCount = cx->compartment->backEdgeCount(traceICs[i].jumpTarget);
        jitTraceICs[i].loopCounterStart = hotloop;
        jitTraceICs[i].loopCounter = hotloop < prevCount ? 1 : hotloop - prevCount;
#endif
        
        stubCode.patch(traceICs[i].addrLabel, &jitTraceICs[i]);
    }
#endif 

    for (size_t i = 0; i < callPatches.length(); i++) {
        CallPatchInfo &patch = callPatches[i];

        CodeLocationLabel joinPoint = patch.joinSlow
            ? stubCode.locationOf(patch.joinPoint)
            : fullCode.locationOf(patch.joinPoint);

        if (patch.hasFastNcode)
            fullCode.patch(patch.fastNcodePatch, joinPoint);
        if (patch.hasSlowNcode)
            stubCode.patch(patch.slowNcodePatch, joinPoint);
    }

#ifdef JS_POLYIC
    ic::GetElementIC *jitGetElems = (ic::GetElementIC *)cursor;
    jit->nGetElems = getElemICs.length();
    cursor += sizeof(ic::GetElementIC) * jit->nGetElems;
    for (size_t i = 0; i < jit->nGetElems; i++) {
        ic::GetElementIC &to = jitGetElems[i];
        GetElementICInfo &from = getElemICs[i];

        new (&to) ic::GetElementIC();
        from.copyTo(to, fullCode, stubCode);

        to.typeReg = from.typeReg;
        to.objReg = from.objReg;
        to.idRemat = from.id;

        if (from.typeGuard.isSet()) {
            int inlineTypeGuard = fullCode.locationOf(from.typeGuard.get()) -
                                  fullCode.locationOf(from.fastPathStart);
            to.inlineTypeGuard = inlineTypeGuard;
            JS_ASSERT(to.inlineTypeGuard == inlineTypeGuard);
        }
        int inlineClaspGuard = fullCode.locationOf(from.claspGuard) -
                               fullCode.locationOf(from.fastPathStart);
        to.inlineClaspGuard = inlineClaspGuard;
        JS_ASSERT(to.inlineClaspGuard == inlineClaspGuard);

        stubCode.patch(from.paramAddr, &to);
    }

    ic::SetElementIC *jitSetElems = (ic::SetElementIC *)cursor;
    jit->nSetElems = setElemICs.length();
    cursor += sizeof(ic::SetElementIC) * jit->nSetElems;
    for (size_t i = 0; i < jit->nSetElems; i++) {
        ic::SetElementIC &to = jitSetElems[i];
        SetElementICInfo &from = setElemICs[i];

        new (&to) ic::SetElementIC();
        from.copyTo(to, fullCode, stubCode);

        to.strictMode = script->strictModeCode;
        to.vr = from.vr;
        to.objReg = from.objReg;
        to.objRemat = from.objRemat.toInt32();
        JS_ASSERT(to.objRemat == from.objRemat.toInt32());

        to.hasConstantKey = from.key.isConstant();
        if (from.key.isConstant())
            to.keyValue = from.key.index();
        else
            to.keyReg = from.key.reg();

        int inlineClaspGuard = fullCode.locationOf(from.claspGuard) -
                               fullCode.locationOf(from.fastPathStart);
        to.inlineClaspGuard = inlineClaspGuard;
        JS_ASSERT(to.inlineClaspGuard == inlineClaspGuard);

        int inlineHoleGuard = fullCode.locationOf(from.holeGuard) -
                               fullCode.locationOf(from.fastPathStart);
        to.inlineHoleGuard = inlineHoleGuard;
        JS_ASSERT(to.inlineHoleGuard == inlineHoleGuard);

        CheckIsStubCall(to.slowPathCall.labelAtOffset(0));

        to.volatileMask = from.volatileMask;
        JS_ASSERT(to.volatileMask == from.volatileMask);

        stubCode.patch(from.paramAddr, &to);
    }

    ic::PICInfo *jitPics = (ic::PICInfo *)cursor;
    jit->nPICs = pics.length();
    cursor += sizeof(ic::PICInfo) * jit->nPICs;
    for (size_t i = 0; i < jit->nPICs; i++) {
        new (&jitPics[i]) ic::PICInfo();
        pics[i].copyTo(jitPics[i], fullCode, stubCode);
        pics[i].copySimpleMembersTo(jitPics[i]);

        jitPics[i].shapeGuard = masm.distanceOf(pics[i].shapeGuard) -
                                masm.distanceOf(pics[i].fastPathStart);
        JS_ASSERT(jitPics[i].shapeGuard == masm.distanceOf(pics[i].shapeGuard) -
                                           masm.distanceOf(pics[i].fastPathStart));
        jitPics[i].shapeRegHasBaseShape = true;
        jitPics[i].pc = pics[i].pc;

        if (pics[i].kind == ic::PICInfo::SET ||
            pics[i].kind == ic::PICInfo::SETMETHOD) {
            jitPics[i].u.vr = pics[i].vr;
        } else if (pics[i].kind != ic::PICInfo::NAME) {
            if (pics[i].hasTypeCheck) {
                int32 distance = stubcc.masm.distanceOf(pics[i].typeCheck) -
                                 stubcc.masm.distanceOf(pics[i].slowPathStart);
                JS_ASSERT(distance <= 0);
                jitPics[i].u.get.typeCheckOffset = distance;
            }
        }
        stubCode.patch(pics[i].paramAddr, &jitPics[i]);
    }
#endif

    for (size_t i = 0; i < jit->nInlineFrames; i++) {
        InlineFrame &to = jitInlineFrames[i];
        ActiveFrame *from = inlineFrames[i];
        to.nUnsyncedEntries = from->unsyncedEntries.length();
        to.unsyncedEntries = (UnsyncedEntry *) cursor;
        cursor += sizeof(UnsyncedEntry) * to.nUnsyncedEntries;
        for (size_t j = 0; j < to.nUnsyncedEntries; j++)
            to.unsyncedEntries[j] = from->unsyncedEntries[j];
    }

    JS_ASSERT(size_t(cursor - (uint8*)jit) == totalBytes);

    
    stubcc.fixCrossJumps(result, masm.size(), masm.size() + stubcc.size());

    size_t doubleOffset = masm.size() + stubcc.size();
    double *inlineDoubles = (double *) (result + doubleOffset);
    double *oolDoubles = (double*) (result + doubleOffset +
                                    masm.numDoubles() * sizeof(double));

    
    void **jumpVec = (void **)(oolDoubles + stubcc.masm.numDoubles());

    for (size_t i = 0; i < jumpTableOffsets.length(); i++) {
        uint32 offset = jumpTableOffsets[i];
        JS_ASSERT(jumpMap[offset].isValid());
        jumpVec[i] = (void *)(result + masm.distanceOf(jumpMap[offset]));
    }

    
    for (size_t i = 0; i < jumpTables.length(); i++) {
        JumpTable &jumpTable = jumpTables[i];
        fullCode.patch(jumpTable.label, &jumpVec[jumpTable.offsetIndex]);
    }

    
    masm.finalize(fullCode, inlineDoubles);
    stubcc.masm.finalize(stubCode, oolDoubles);

    JSC::ExecutableAllocator::makeExecutable(result, masm.size() + stubcc.size());
    JSC::ExecutableAllocator::cacheFlush(result, masm.size() + stubcc.size());

    *jitp = jit;

    
    cx->runtime->mjitMemoryUsed += totalSize + totalBytes;

    return Compile_Okay;
}

class SrcNoteLineScanner {
    ptrdiff_t offset;
    jssrcnote *sn;

public:
    SrcNoteLineScanner(jssrcnote *sn) : offset(SN_DELTA(sn)), sn(sn) {}

    bool firstOpInLine(ptrdiff_t relpc) {
        while ((offset < relpc) && !SN_IS_TERMINATOR(sn)) {
            sn = SN_NEXT(sn);
            offset += SN_DELTA(sn);
        }

        while ((offset == relpc) && !SN_IS_TERMINATOR(sn)) {
            JSSrcNoteType type = (JSSrcNoteType) SN_TYPE(sn);
            if (type == SRC_SETLINE || type == SRC_NEWLINE)
                return true;

            sn = SN_NEXT(sn);
            offset += SN_DELTA(sn);
        }

        return false;
    }
};

#ifdef DEBUG
#define SPEW_OPCODE()                                                         \
    JS_BEGIN_MACRO                                                            \
        if (IsJaegerSpewChannelActive(JSpew_JSOps)) {                         \
            JaegerSpew(JSpew_JSOps, "    %2d ", frame.stackDepth());          \
            void *mark = JS_ARENA_MARK(&cx->tempPool);                        \
            Sprinter sprinter;                                                \
            INIT_SPRINTER(cx, &sprinter, &cx->tempPool, 0);                   \
            js_Disassemble1(cx, script, PC, PC - script->code,                \
                            JS_TRUE, &sprinter);                              \
            fprintf(stdout, "%s", sprinter.base);                             \
            JS_ARENA_RELEASE(&cx->tempPool, mark);                            \
        }                                                                     \
    JS_END_MACRO;
#else
#define SPEW_OPCODE()
#endif 

#define BEGIN_CASE(name)        case name:
#define END_CASE(name)                      \
    JS_BEGIN_MACRO                          \
        PC += name##_LENGTH;                \
    JS_END_MACRO;                           \
    break;

static inline void
FixDouble(Value &val)
{
    if (val.isInt32())
        val.setDouble((double)val.toInt32());
}

CompileStatus
mjit::Compiler::generateMethod()
{
    mjit::AutoScriptRetrapper trapper(cx, script);
    SrcNoteLineScanner scanner(script->notes());

    
    bool fallthrough = true;

    for (;;) {
        JSOp op = JSOp(*PC);
        int trap = stubs::JSTRAP_NONE;
        if (op == JSOP_TRAP) {
            if (!trapper.untrap(PC))
                return Compile_Error;
            op = JSOp(*PC);
            trap |= stubs::JSTRAP_TRAP;
        }
        if (script->singleStepMode && scanner.firstOpInLine(PC - script->code))
            trap |= stubs::JSTRAP_SINGLESTEP;
        variadicRejoin = false;

        analyze::Bytecode *opinfo = analysis->maybeCode(PC);

        if (!opinfo) {
            if (op == JSOP_STOP)
                break;
            if (js_CodeSpec[op].length != -1)
                PC += js_CodeSpec[op].length;
            else
                PC += js_GetVariableBytecodeLength(PC);
            continue;
        }

        if (loop)
            loop->PC = PC;

        frame.setPC(PC);
        frame.setInTryBlock(opinfo->inTryBlock);

        if (fallthrough) {
            








            for (unsigned i = 0; i < fixedDoubleEntries.length(); i++) {
                FrameEntry *fe = frame.getOrTrack(fixedDoubleEntries[i]);
                frame.ensureInteger(fe);
            }
        }
        fixedDoubleEntries.clear();

#ifdef DEBUG
        if (fallthrough && cx->typeInferenceEnabled()) {
            for (uint32 slot = analyze::ArgSlot(0); slot < analyze::TotalSlots(script); slot++) {
                if (a->varTypes[slot].type == JSVAL_TYPE_INT32) {
                    FrameEntry *fe = frame.getOrTrack(slot);
                    JS_ASSERT(!fe->isType(JSVAL_TYPE_DOUBLE));
                }
            }
        }
#endif

        if (opinfo->jumpTarget || trap) {
            if (fallthrough) {
                fixDoubleTypes(PC);
                fixedDoubleEntries.clear();

                





                if (cx->typeInferenceEnabled() && analysis->getCode(PC).loopHead) {
                    frame.syncAndForgetEverything();
                    Jump j = masm.jump();
                    if (!startLoop(PC, j, PC))
                        return Compile_Error;
                } else {
                    if (!frame.syncForBranch(PC, Uses(0)))
                        return Compile_Error;
                    JS_ASSERT(frame.consistentRegisters(PC));
                }
            }

            if (!frame.discardForJoin(PC, opinfo->stackDepth))
                return Compile_Error;
            restoreAnalysisTypes();
            fallthrough = true;

            if (!cx->typeInferenceEnabled()) {
                
                opinfo->safePoint = true;
            }
        }

        a->jumpMap[uint32(PC - script->code)] = masm.label();

        SPEW_OPCODE();
        JS_ASSERT(frame.stackDepth() == opinfo->stackDepth);

        if (trap) {
            REJOIN_SITE(CallSite::MAGIC_TRAP_ID);
            prepareStubCall(Uses(0));
            masm.move(Imm32(trap), Registers::ArgReg1);
            Call cl = emitStubCall(JS_FUNC_TO_DATA_PTR(void *, stubs::Trap), NULL);
            InternalCallSite site(masm.callReturnOffset(cl), a->inlineIndex, PC,
                                  CallSite::MAGIC_TRAP_ID, false, true);
            addCallSite(site);
        } else if (!a->parent && savedTraps && savedTraps[PC - script->code]) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            REJOIN_SITE(CallSite::MAGIC_TRAP_ID);
            uint32 offset = stubcc.masm.distanceOf(stubcc.masm.label());
            if (Assembler::ReturnStackAdjustment) {
                stubcc.masm.addPtr(Imm32(Assembler::ReturnStackAdjustment),
                                   Assembler::stackPointerRegister);
            }
            stubcc.crossJump(stubcc.masm.jump(), masm.label());

            InternalCallSite site(offset, a->inlineIndex, PC,
                                  CallSite::MAGIC_TRAP_ID, true, true);
            addCallSite(site);
        }

    

 

        jsbytecode *oldPC = PC;

        switch (op) {
          BEGIN_CASE(JSOP_NOP)
          END_CASE(JSOP_NOP)

          BEGIN_CASE(JSOP_PUSH)
            frame.push(UndefinedValue());
          END_CASE(JSOP_PUSH)

          BEGIN_CASE(JSOP_POPV)
          BEGIN_CASE(JSOP_SETRVAL)
          {
            RegisterID reg = frame.allocReg();
            masm.load32(FrameFlagsAddress(), reg);
            masm.or32(Imm32(StackFrame::HAS_RVAL), reg);
            masm.store32(reg, FrameFlagsAddress());
            frame.freeReg(reg);

            
            JS_ASSERT(a == outer);

            FrameEntry *fe = frame.peek(-1);
            frame.storeTo(fe, Address(JSFrameReg, StackFrame::offsetOfReturnValue()), true);
            frame.pop();
          }
          END_CASE(JSOP_POPV)

          BEGIN_CASE(JSOP_RETURN)
            emitReturn(frame.peek(-1));
            fallthrough = false;
          END_CASE(JSOP_RETURN)

          BEGIN_CASE(JSOP_GOTO)
          BEGIN_CASE(JSOP_DEFAULT)
          {
            unsigned targetOffset = analyze::FollowBranch(script, PC - script->code);
            jsbytecode *target = script->code + targetOffset;

            fixDoubleTypes(target);

            




            jsbytecode *next = PC + JSOP_GOTO_LENGTH;
            if (cx->typeInferenceEnabled() && analysis->maybeCode(next) &&
                analysis->getCode(next).loopHead) {
                frame.syncAndForgetEverything();
                Jump j = masm.jump();
                if (!startLoop(next, j, target))
                    return Compile_Error;
            } else {
                if (!frame.syncForBranch(target, Uses(0)))
                    return Compile_Error;
                Jump j = masm.jump();
                if (!jumpAndTrace(j, target))
                    return Compile_Error;
            }
            fallthrough = false;
          }
          END_CASE(JSOP_GOTO)

          BEGIN_CASE(JSOP_IFEQ)
          BEGIN_CASE(JSOP_IFNE)
            if (!jsop_ifneq(op, PC + GET_JUMP_OFFSET(PC)))
                return Compile_Error;
          END_CASE(JSOP_IFNE)

          BEGIN_CASE(JSOP_ARGUMENTS)
            






            if (canUseApplyTricks())
                applyTricks = LazyArgsObj;
            else
                jsop_arguments();
            pushSyncedEntry(0);
          END_CASE(JSOP_ARGUMENTS)

          BEGIN_CASE(JSOP_FORARG)
          {
            updateVarType();
            uint32 arg = GET_SLOTNO(PC);
            iterNext();
            frame.storeArg(arg, true);
            frame.pop();
          }
          END_CASE(JSOP_FORARG)

          BEGIN_CASE(JSOP_FORLOCAL)
          {
            updateVarType();
            uint32 slot = GET_SLOTNO(PC);
            iterNext();
            frame.storeLocal(slot, true, true);
            frame.pop();
          }
          END_CASE(JSOP_FORLOCAL)

          BEGIN_CASE(JSOP_DUP)
            frame.dup();
          END_CASE(JSOP_DUP)

          BEGIN_CASE(JSOP_DUP2)
            frame.dup2();
          END_CASE(JSOP_DUP2)

          BEGIN_CASE(JSOP_BITOR)
          BEGIN_CASE(JSOP_BITXOR)
          BEGIN_CASE(JSOP_BITAND)
            jsop_bitop(op);
          END_CASE(JSOP_BITAND)

          BEGIN_CASE(JSOP_LT)
          BEGIN_CASE(JSOP_LE)
          BEGIN_CASE(JSOP_GT)
          BEGIN_CASE(JSOP_GE)
          BEGIN_CASE(JSOP_EQ)
          BEGIN_CASE(JSOP_NE)
          {
            
            jsbytecode *next = &PC[JSOP_GE_LENGTH];
            JSOp fused = JSOp(*next);
            if ((fused != JSOP_IFEQ && fused != JSOP_IFNE) || analysis->jumpTarget(next))
                fused = JSOP_NOP;

            
            jsbytecode *target = NULL;
            if (fused != JSOP_NOP) {
                target = next + GET_JUMP_OFFSET(next);
                fixDoubleTypes(target);
            }

            BoolStub stub = NULL;
            switch (op) {
              case JSOP_LT:
                stub = stubs::LessThan;
                break;
              case JSOP_LE:
                stub = stubs::LessEqual;
                break;
              case JSOP_GT:
                stub = stubs::GreaterThan;
                break;
              case JSOP_GE:
                stub = stubs::GreaterEqual;
                break;
              case JSOP_EQ:
                stub = stubs::Equal;
                break;
              case JSOP_NE:
                stub = stubs::NotEqual;
                break;
              default:
                JS_NOT_REACHED("WAT");
                break;
            }

            





            FrameEntry *rhs = frame.peek(-1);
            FrameEntry *lhs = frame.peek(-2);

            
            if (lhs->isConstant() && rhs->isConstant()) {
                
                const Value &lv = lhs->getValue();
                const Value &rv = rhs->getValue();

                AutoRejoinSite autoRejoin(this, (void *) RejoinSite::VARIADIC_ID);

                if (lv.isPrimitive() && rv.isPrimitive()) {
                    bool result = compareTwoValues(cx, op, lv, rv);

                    frame.pop();
                    frame.pop();

                    if (!target) {
                        frame.push(Value(BooleanValue(result)));
                    } else {
                        if (fused == JSOP_IFEQ)
                            result = !result;

                        if (result) {
                            fixDoubleTypes(target);
                            if (!frame.syncForBranch(target, Uses(0)))
                                return Compile_Error;
                            if (needRejoins(PC)) {
                                autoRejoin.oolRejoin(stubcc.masm.label());
                                stubcc.rejoin(Changes(0));
                            }
                            Jump j = masm.jump();
                            if (!jumpAndTrace(j, target))
                                return Compile_Error;
                        } else {
                            



                            if (target < PC && !finishLoop(target))
                                return Compile_Error;
                        }
                    }
                } else {
                    if (!emitStubCmpOp(stub, autoRejoin, target, fused))
                        return Compile_Error;
                }
            } else {
                
                AutoRejoinSite autoRejoin(this, (void *) RejoinSite::VARIADIC_ID);
                if (!jsop_relational(op, stub, autoRejoin, target, fused))
                    return Compile_Error;
            }

            
            JS_STATIC_ASSERT(JSOP_LT_LENGTH == JSOP_GE_LENGTH);
            JS_STATIC_ASSERT(JSOP_LE_LENGTH == JSOP_GE_LENGTH);
            JS_STATIC_ASSERT(JSOP_GT_LENGTH == JSOP_GE_LENGTH);
            JS_STATIC_ASSERT(JSOP_EQ_LENGTH == JSOP_GE_LENGTH);
            JS_STATIC_ASSERT(JSOP_NE_LENGTH == JSOP_GE_LENGTH);

            PC += JSOP_GE_LENGTH;
            if (fused != JSOP_NOP) {
                SPEW_OPCODE();
                PC += JSOP_IFNE_LENGTH;
            }
            break;
          }
          END_CASE(JSOP_GE)

          BEGIN_CASE(JSOP_LSH)
            jsop_bitop(op);
          END_CASE(JSOP_LSH)

          BEGIN_CASE(JSOP_RSH)
            jsop_bitop(op);
          END_CASE(JSOP_RSH)

          BEGIN_CASE(JSOP_URSH)
            jsop_bitop(op);
          END_CASE(JSOP_URSH)

          BEGIN_CASE(JSOP_ADD)
            if (!jsop_binary(op, stubs::Add, knownPushedType(0), pushedTypeSet(0)))
                return Compile_Retry;
          END_CASE(JSOP_ADD)

          BEGIN_CASE(JSOP_SUB)
            if (!jsop_binary(op, stubs::Sub, knownPushedType(0), pushedTypeSet(0)))
                return Compile_Retry;
          END_CASE(JSOP_SUB)

          BEGIN_CASE(JSOP_MUL)
            if (!jsop_binary(op, stubs::Mul, knownPushedType(0), pushedTypeSet(0)))
                return Compile_Retry;
          END_CASE(JSOP_MUL)

          BEGIN_CASE(JSOP_DIV)
            if (!jsop_binary(op, stubs::Div, knownPushedType(0), pushedTypeSet(0)))
                return Compile_Retry;
          END_CASE(JSOP_DIV)

          BEGIN_CASE(JSOP_MOD)
            if (!jsop_mod())
                return Compile_Retry;
          END_CASE(JSOP_MOD)

          BEGIN_CASE(JSOP_NOT)
            jsop_not();
          END_CASE(JSOP_NOT)

          BEGIN_CASE(JSOP_BITNOT)
          {
            FrameEntry *top = frame.peek(-1);
            if (top->isConstant() && top->getValue().isPrimitive()) {
                int32_t i;
                ValueToECMAInt32(cx, top->getValue(), &i);
                i = ~i;
                frame.pop();
                frame.push(Int32Value(i));
            } else {
                jsop_bitnot();
            }
          }
          END_CASE(JSOP_BITNOT)

          BEGIN_CASE(JSOP_NEG)
          {
            REJOIN_SITE(stubs::Neg);
            FrameEntry *top = frame.peek(-1);
            if (top->isConstant() && top->getValue().isPrimitive()) {
                double d;
                ValueToNumber(cx, top->getValue(), &d);
                d = -d;
                Value v = NumberValue(d);

                
                types::TypeSet *pushed = pushedTypeSet(0);
                if (!v.isInt32() && pushed && !pushed->hasType(types::TYPE_DOUBLE)) {
                    script->typeMonitorResult(cx, PC, types::TYPE_DOUBLE);
                    return Compile_Retry;
                }

                frame.pop();
                frame.push(v);
            } else {
                jsop_neg();
            }
          }
          END_CASE(JSOP_NEG)

          BEGIN_CASE(JSOP_POS)
            jsop_pos();
          END_CASE(JSOP_POS)

          BEGIN_CASE(JSOP_DELNAME)
          {
            REJOIN_SITE_ANY();
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DelName);
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_DELNAME)

          BEGIN_CASE(JSOP_DELPROP)
          {
            REJOIN_SITE_ANY();
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(1));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(STRICT_VARIANT(stubs::DelProp));
            frame.pop();
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_DELPROP) 

          BEGIN_CASE(JSOP_DELELEM)
          {
            REJOIN_SITE_ANY();
            prepareStubCall(Uses(2));
            INLINE_STUBCALL(STRICT_VARIANT(stubs::DelElem));
            frame.popn(2);
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_DELELEM)

          BEGIN_CASE(JSOP_TYPEOF)
          BEGIN_CASE(JSOP_TYPEOFEXPR)
            jsop_typeof();
          END_CASE(JSOP_TYPEOF)

          BEGIN_CASE(JSOP_VOID)
            frame.pop();
            frame.push(UndefinedValue());
          END_CASE(JSOP_VOID)

          BEGIN_CASE(JSOP_INCNAME)
          {
            CompileStatus status = jsop_nameinc(op, STRICT_VARIANT(stubs::IncName), fullAtomIndex(PC));
            if (status != Compile_Okay)
                return status;
            break;
          }
          END_CASE(JSOP_INCNAME)

          BEGIN_CASE(JSOP_INCGNAME)
            if (!jsop_gnameinc(op, STRICT_VARIANT(stubs::IncGlobalName), fullAtomIndex(PC)))
                return Compile_Retry;
            break;
          END_CASE(JSOP_INCGNAME)

          BEGIN_CASE(JSOP_INCPROP)
          {
            CompileStatus status = jsop_propinc(op, STRICT_VARIANT(stubs::IncProp), fullAtomIndex(PC));
            if (status != Compile_Okay)
                return status;
            break;
          }
          END_CASE(JSOP_INCPROP)

          BEGIN_CASE(JSOP_INCELEM)
            jsop_eleminc(op, STRICT_VARIANT(stubs::IncElem));
          END_CASE(JSOP_INCELEM)

          BEGIN_CASE(JSOP_DECNAME)
          {
            CompileStatus status = jsop_nameinc(op, STRICT_VARIANT(stubs::DecName), fullAtomIndex(PC));
            if (status != Compile_Okay)
                return status;
            break;
          }
          END_CASE(JSOP_DECNAME)

          BEGIN_CASE(JSOP_DECGNAME)
            if (!jsop_gnameinc(op, STRICT_VARIANT(stubs::DecGlobalName), fullAtomIndex(PC)))
                return Compile_Retry;
            break;
          END_CASE(JSOP_DECGNAME)

          BEGIN_CASE(JSOP_DECPROP)
          {
            CompileStatus status = jsop_propinc(op, STRICT_VARIANT(stubs::DecProp), fullAtomIndex(PC));
            if (status != Compile_Okay)
                return status;
            break;
          }
          END_CASE(JSOP_DECPROP)

          BEGIN_CASE(JSOP_DECELEM)
            jsop_eleminc(op, STRICT_VARIANT(stubs::DecElem));
          END_CASE(JSOP_DECELEM)

          BEGIN_CASE(JSOP_NAMEINC)
          {
            CompileStatus status = jsop_nameinc(op, STRICT_VARIANT(stubs::NameInc), fullAtomIndex(PC));
            if (status != Compile_Okay)
                return status;
            break;
          }
          END_CASE(JSOP_NAMEINC)

          BEGIN_CASE(JSOP_GNAMEINC)
            if (!jsop_gnameinc(op, STRICT_VARIANT(stubs::GlobalNameInc), fullAtomIndex(PC)))
                return Compile_Retry;
            break;
          END_CASE(JSOP_GNAMEINC)

          BEGIN_CASE(JSOP_PROPINC)
          {
            CompileStatus status = jsop_propinc(op, STRICT_VARIANT(stubs::PropInc), fullAtomIndex(PC));
            if (status != Compile_Okay)
                return status;
            break;
          }
          END_CASE(JSOP_PROPINC)

          BEGIN_CASE(JSOP_ELEMINC)
            jsop_eleminc(op, STRICT_VARIANT(stubs::ElemInc));
          END_CASE(JSOP_ELEMINC)

          BEGIN_CASE(JSOP_NAMEDEC)
          {
            CompileStatus status = jsop_nameinc(op, STRICT_VARIANT(stubs::NameDec), fullAtomIndex(PC));
            if (status != Compile_Okay)
                return status;
            break;
          }
          END_CASE(JSOP_NAMEDEC)

          BEGIN_CASE(JSOP_GNAMEDEC)
            if (!jsop_gnameinc(op, STRICT_VARIANT(stubs::GlobalNameDec), fullAtomIndex(PC)))
                return Compile_Retry;
            break;
          END_CASE(JSOP_GNAMEDEC)

          BEGIN_CASE(JSOP_PROPDEC)
          {
            CompileStatus status = jsop_propinc(op, STRICT_VARIANT(stubs::PropDec), fullAtomIndex(PC));
            if (status != Compile_Okay)
                return status;
            break;
          }
          END_CASE(JSOP_PROPDEC)

          BEGIN_CASE(JSOP_ELEMDEC)
            jsop_eleminc(op, STRICT_VARIANT(stubs::ElemDec));
          END_CASE(JSOP_ELEMDEC)

          BEGIN_CASE(JSOP_GETPROP)
          BEGIN_CASE(JSOP_LENGTH)
            if (!jsop_getprop(script->getAtom(fullAtomIndex(PC)), knownPushedType(0)))
                return Compile_Error;
          END_CASE(JSOP_GETPROP)

          BEGIN_CASE(JSOP_GETELEM)
            if (!jsop_getelem(false))
                return Compile_Error;
          END_CASE(JSOP_GETELEM)

          BEGIN_CASE(JSOP_SETELEM)
          BEGIN_CASE(JSOP_SETHOLE)
          {
            jsbytecode *next = &PC[JSOP_SETELEM_LENGTH];
            bool pop = (JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next));
            if (!jsop_setelem(pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETELEM);

          BEGIN_CASE(JSOP_CALLNAME)
          {
            REJOIN_SITE_ANY();
            uint32 index = fullAtomIndex(PC);
            prepareStubCall(Uses(0));
            masm.move(Imm32(index), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::CallName);
            pushSyncedEntry(0);
            pushSyncedEntry(1);
            frame.extra(frame.peek(-2)).name = script->getAtom(index);
          }
          END_CASE(JSOP_CALLNAME)

          BEGIN_CASE(JSOP_EVAL)
          {
            REJOIN_SITE_ANY();
            JaegerSpew(JSpew_Insns, " --- EVAL --- \n");
            emitEval(GET_ARGC(PC));
            JaegerSpew(JSpew_Insns, " --- END EVAL --- \n");
          }
          END_CASE(JSOP_EVAL)

          BEGIN_CASE(JSOP_CALL)
          BEGIN_CASE(JSOP_NEW)
          BEGIN_CASE(JSOP_FUNAPPLY)
          BEGIN_CASE(JSOP_FUNCALL)
          {
            REJOIN_SITE_ANY();
          {
            bool callingNew = (op == JSOP_NEW);

            AutoRejoinSite autoRejoinCall(this,
                JS_FUNC_TO_DATA_PTR(void *, callingNew ? ic::New : ic::Call),
                JS_FUNC_TO_DATA_PTR(void *, callingNew ? stubs::UncachedNew : stubs::UncachedCall));
            AutoRejoinSite autoRejoinNcode(this, (void *) CallSite::NCODE_RETURN_ID);

            bool done = false;
            bool inlined = false;
            if (op == JSOP_CALL) {
                CompileStatus status = inlineNativeFunction(GET_ARGC(PC), callingNew);
                if (status == Compile_Okay)
                    done = true;
                else if (status != Compile_InlineAbort)
                    return status;
            }
            if (!done && inlining) {
                CompileStatus status = inlineScriptedFunction(GET_ARGC(PC), callingNew);
                if (status == Compile_Okay) {
                    done = true;
                    inlined = true;
                }
                else if (status != Compile_InlineAbort)
                    return status;
            }

            FrameSize frameSize;
            frameSize.initStatic(frame.totalDepth(), GET_ARGC(PC));

            if (!done) {
                JaegerSpew(JSpew_Insns, " --- SCRIPTED CALL --- \n");
                inlineCallHelper(GET_ARGC(PC), callingNew, frameSize);
                JaegerSpew(JSpew_Insns, " --- END SCRIPTED CALL --- \n");
            }

            










            if (needRejoins(PC) || inlined) {
                if (inlined)
                    autoRejoinNcode.forceGeneration();

                autoRejoinCall.oolRejoin(stubcc.masm.label());
                Jump fallthrough = stubcc.masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                                             Registers::ReturnReg);
                if (frameSize.isStatic())
                    stubcc.masm.move(Imm32(frameSize.staticArgc()), JSParamReg_Argc);
                else
                    stubcc.masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), JSParamReg_Argc);
                stubcc.masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.sp)), JSFrameReg);

                CallPatchInfo callPatch;
                callPatch.hasSlowNcode = true;
                callPatch.slowNcodePatch =
                    stubcc.masm.storePtrWithPatch(ImmPtr(NULL),
                                                  Address(JSFrameReg, StackFrame::offsetOfNcode()));
                stubcc.masm.jump(Registers::ReturnReg);

                callPatch.joinPoint = stubcc.masm.label();
                callPatch.joinSlow = true;

                addReturnSite(true );

                autoRejoinNcode.oolRejoin(stubcc.masm.label());

                




                bool loweredCallApply =
                    (frameSize.isDynamic() || frameSize.staticArgc() != GET_ARGC(PC));
                Address address = frame.addressOf(frame.peek(-1));
                if (loweredCallApply) {
                    frame.pushSynced(JSVAL_TYPE_UNKNOWN);
                    address = frame.addressOf(frame.peek(-1));
                    frame.pop();
                }

                stubcc.masm.storeValueFromComponents(JSReturnReg_Type, JSReturnReg_Data, address);
                fallthrough.linkTo(stubcc.masm.label(), &stubcc.masm);

                




                if (loweredCallApply) {
                    frame.reloadEntry(stubcc.masm, address, frame.peek(-1));
                    stubcc.crossJump(stubcc.masm.jump(), masm.label());
                } else {
                    stubcc.rejoin(Changes(1));
                }

                callPatches.append(callPatch);
            }
          } }
          END_CASE(JSOP_CALL)

          BEGIN_CASE(JSOP_NAME)
          {
            JSAtom *atom = script->getAtom(fullAtomIndex(PC));
            jsop_name(atom, knownPushedType(0));
            frame.extra(frame.peek(-1)).name = atom;
          }
          END_CASE(JSOP_NAME)

          BEGIN_CASE(JSOP_DOUBLE)
          {
            uint32 index = fullAtomIndex(PC);
            double d = script->getConst(index).toDouble();
            frame.push(Value(DoubleValue(d)));
          }
          END_CASE(JSOP_DOUBLE)

          BEGIN_CASE(JSOP_STRING)
            frame.push(StringValue(script->getAtom(fullAtomIndex(PC))));
          END_CASE(JSOP_STRING)

          BEGIN_CASE(JSOP_ZERO)
            frame.push(Valueify(JSVAL_ZERO));
          END_CASE(JSOP_ZERO)

          BEGIN_CASE(JSOP_ONE)
            frame.push(Valueify(JSVAL_ONE));
          END_CASE(JSOP_ONE)

          BEGIN_CASE(JSOP_NULL)
            frame.push(NullValue());
          END_CASE(JSOP_NULL)

          BEGIN_CASE(JSOP_THIS)
            jsop_this();
          END_CASE(JSOP_THIS)

          BEGIN_CASE(JSOP_FALSE)
            frame.push(Value(BooleanValue(false)));
          END_CASE(JSOP_FALSE)

          BEGIN_CASE(JSOP_TRUE)
            frame.push(Value(BooleanValue(true)));
          END_CASE(JSOP_TRUE)

          BEGIN_CASE(JSOP_OR)
          BEGIN_CASE(JSOP_AND)
            if (!jsop_andor(op, PC + GET_JUMP_OFFSET(PC)))
                return Compile_Error;
          END_CASE(JSOP_AND)

          BEGIN_CASE(JSOP_TABLESWITCH)
            






#if defined JS_CPU_ARM 
            frame.syncAndKillEverything();
            masm.move(ImmPtr(PC), Registers::ArgReg1);

            
            INLINE_STUBCALL_NO_REJOIN(stubs::TableSwitch);
            frame.pop();

            masm.jump(Registers::ReturnReg);
#else
            if (!jsop_tableswitch(PC))
                return Compile_Error;
#endif
            PC += js_GetVariableBytecodeLength(PC);
            break;
          END_CASE(JSOP_TABLESWITCH)

          BEGIN_CASE(JSOP_LOOKUPSWITCH)
            frame.syncAndForgetEverything();
            masm.move(ImmPtr(PC), Registers::ArgReg1);

            
            INLINE_STUBCALL_NO_REJOIN(stubs::LookupSwitch);
            frame.pop();

            masm.jump(Registers::ReturnReg);
            PC += js_GetVariableBytecodeLength(PC);
            break;
          END_CASE(JSOP_LOOKUPSWITCH)

          BEGIN_CASE(JSOP_CASE)
            

            frame.dupAt(-2);
            

            jsop_stricteq(JSOP_STRICTEQ);
            

            if (!jsop_ifneq(JSOP_IFNE, PC + GET_JUMP_OFFSET(PC)))
                return Compile_Error;
          END_CASE(JSOP_CASE)

          BEGIN_CASE(JSOP_STRICTEQ)
            jsop_stricteq(op);
          END_CASE(JSOP_STRICTEQ)

          BEGIN_CASE(JSOP_STRICTNE)
            jsop_stricteq(op);
          END_CASE(JSOP_STRICTNE)

          BEGIN_CASE(JSOP_ITER)
            if (!iter(PC[1]))
                return Compile_Error;
          END_CASE(JSOP_ITER)

          BEGIN_CASE(JSOP_MOREITER)
          {
            
            if (!iterMore())
                return Compile_Error;
            JSOp next = JSOp(PC[JSOP_MOREITER_LENGTH]);
            PC += JSOP_MOREITER_LENGTH;
            PC += js_CodeSpec[next].length;
            break;
          }
          END_CASE(JSOP_MOREITER)

          BEGIN_CASE(JSOP_ENDITER)
            iterEnd();
          END_CASE(JSOP_ENDITER)

          BEGIN_CASE(JSOP_POP)
            frame.pop();
          END_CASE(JSOP_POP)

          BEGIN_CASE(JSOP_GETARG)
          {
            uint32 arg = GET_SLOTNO(PC);
            frame.pushArg(arg);
          }
          END_CASE(JSOP_GETARG)

          BEGIN_CASE(JSOP_CALLARG)
          {
            uint32 arg = GET_SLOTNO(PC);
            if (JSObject *singleton = pushedSingleton(0))
                frame.push(ObjectValue(*singleton));
            else
                frame.pushArg(arg);
            frame.push(UndefinedValue());
          }
          END_CASE(JSOP_GETARG)

          BEGIN_CASE(JSOP_BINDGNAME)
            jsop_bindgname();
          END_CASE(JSOP_BINDGNAME)

          BEGIN_CASE(JSOP_SETARG)
          {
            updateVarType();
            jsbytecode *next = &PC[JSOP_SETLOCAL_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            frame.storeArg(GET_SLOTNO(PC), pop);

            




            if (cx->typeInferenceEnabled()) {
                uint32 slot = analyze::ArgSlot(GET_SLOTNO(PC));
                if (a->varTypes[slot].type == JSVAL_TYPE_DOUBLE && fixDoubleSlot(slot))
                    frame.ensureDouble(frame.getArg(GET_SLOTNO(PC)));
            }

            if (pop) {
                frame.pop();
                PC += JSOP_SETARG_LENGTH + JSOP_POP_LENGTH;
                break;
            }
          }
          END_CASE(JSOP_SETARG)

          BEGIN_CASE(JSOP_GETLOCAL)
          {
            uint32 slot = GET_SLOTNO(PC);
            frame.pushLocal(slot);
          }
          END_CASE(JSOP_GETLOCAL)

          BEGIN_CASE(JSOP_SETLOCAL)
          {
            updateVarType();
            jsbytecode *next = &PC[JSOP_SETLOCAL_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            frame.storeLocal(GET_SLOTNO(PC), pop, true);

            if (cx->typeInferenceEnabled()) {
                uint32 slot = analyze::LocalSlot(script, GET_SLOTNO(PC));
                if (a->varTypes[slot].type == JSVAL_TYPE_DOUBLE && fixDoubleSlot(slot))
                    frame.ensureDouble(frame.getLocal(GET_SLOTNO(PC)));
            }

            if (pop) {
                frame.pop();
                PC += JSOP_SETLOCAL_LENGTH + JSOP_POP_LENGTH;
                break;
            }
          }
          END_CASE(JSOP_SETLOCAL)

          BEGIN_CASE(JSOP_SETLOCALPOP)
          {
            updateVarType();
            uint32 slot = GET_SLOTNO(PC);
            frame.storeLocal(slot, true, true);
            frame.pop();
          }
          END_CASE(JSOP_SETLOCALPOP)

          BEGIN_CASE(JSOP_UINT16)
            frame.push(Value(Int32Value((int32_t) GET_UINT16(PC))));
          END_CASE(JSOP_UINT16)

          BEGIN_CASE(JSOP_NEWINIT)
            if (!jsop_newinit())
                return Compile_Error;
          END_CASE(JSOP_NEWINIT)

          BEGIN_CASE(JSOP_NEWARRAY)
            if (!jsop_newinit())
                return Compile_Error;
          END_CASE(JSOP_NEWARRAY)

          BEGIN_CASE(JSOP_NEWOBJECT)
            if (!jsop_newinit())
                return Compile_Error;
          END_CASE(JSOP_NEWOBJECT)

          BEGIN_CASE(JSOP_ENDINIT)
          END_CASE(JSOP_ENDINIT)

          BEGIN_CASE(JSOP_INITMETHOD)
            jsop_initmethod();
            frame.pop();
          END_CASE(JSOP_INITMETHOD)

          BEGIN_CASE(JSOP_INITPROP)
            jsop_initprop();
            frame.pop();
          END_CASE(JSOP_INITPROP)

          BEGIN_CASE(JSOP_INITELEM)
            jsop_initelem();
            frame.popn(2);
          END_CASE(JSOP_INITELEM)

          BEGIN_CASE(JSOP_INCARG)
          BEGIN_CASE(JSOP_DECARG)
          BEGIN_CASE(JSOP_ARGINC)
          BEGIN_CASE(JSOP_ARGDEC)
          {
            jsbytecode *next = &PC[JSOP_ARGINC_LENGTH];
            bool popped = false;
            if (JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next))
                popped = true;
            if (!jsop_arginc(op, GET_SLOTNO(PC), popped))
                return Compile_Retry;
            PC += JSOP_ARGINC_LENGTH;
            if (popped)
                PC += JSOP_POP_LENGTH;
            break;
          }
          END_CASE(JSOP_ARGDEC)

          BEGIN_CASE(JSOP_INCLOCAL)
          BEGIN_CASE(JSOP_DECLOCAL)
          BEGIN_CASE(JSOP_LOCALINC)
          BEGIN_CASE(JSOP_LOCALDEC)
          {
            jsbytecode *next = &PC[JSOP_LOCALINC_LENGTH];
            bool popped = false;
            if (JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next))
                popped = true;
            
            if (!jsop_localinc(op, GET_SLOTNO(PC), popped))
                return Compile_Retry;
            PC += JSOP_LOCALINC_LENGTH;
            if (popped)
                PC += JSOP_POP_LENGTH;
            break;
          }
          END_CASE(JSOP_LOCALDEC)

          BEGIN_CASE(JSOP_FORNAME)
            jsop_forname(script->getAtom(fullAtomIndex(PC)));
          END_CASE(JSOP_FORNAME)

          BEGIN_CASE(JSOP_FORGNAME)
            jsop_forgname(script->getAtom(fullAtomIndex(PC)));
          END_CASE(JSOP_FORGNAME)

          BEGIN_CASE(JSOP_FORPROP)
            jsop_forprop(script->getAtom(fullAtomIndex(PC)));
          END_CASE(JSOP_FORPROP)

          BEGIN_CASE(JSOP_FORELEM)
            
            
            iterNext();
          END_CASE(JSOP_FORELEM)

          BEGIN_CASE(JSOP_BINDNAME)
            jsop_bindname(script->getAtom(fullAtomIndex(PC)), true);
          END_CASE(JSOP_BINDNAME)

          BEGIN_CASE(JSOP_SETPROP)
          {
            jsbytecode *next = &PC[JSOP_SETLOCAL_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            if (!jsop_setprop(script->getAtom(fullAtomIndex(PC)), true, pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETPROP)

          BEGIN_CASE(JSOP_SETNAME)
          BEGIN_CASE(JSOP_SETMETHOD)
          {
            jsbytecode *next = &PC[JSOP_SETLOCAL_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            if (!jsop_setprop(script->getAtom(fullAtomIndex(PC)), true, pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETNAME)

          BEGIN_CASE(JSOP_THROW)
            prepareStubCall(Uses(1));
            INLINE_STUBCALL_NO_REJOIN(stubs::Throw);
            frame.pop();
          END_CASE(JSOP_THROW)

          BEGIN_CASE(JSOP_IN)
          {
            REJOIN_SITE_ANY();
            prepareStubCall(Uses(2));
            INLINE_STUBCALL(stubs::In);
            frame.popn(2);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
          }
          END_CASE(JSOP_IN)

          BEGIN_CASE(JSOP_INSTANCEOF)
            if (!jsop_instanceof())
                return Compile_Error;
          END_CASE(JSOP_INSTANCEOF)

          BEGIN_CASE(JSOP_EXCEPTION)
          {
            REJOIN_SITE_ANY();
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::Exception);
            frame.pushSynced(JSVAL_TYPE_UNKNOWN);
          }
          END_CASE(JSOP_EXCEPTION)

          BEGIN_CASE(JSOP_LINENO)
          END_CASE(JSOP_LINENO)

          BEGIN_CASE(JSOP_ENUMELEM)
            
            
            
            
            
            
            
            

            
            
            frame.dupAt(-3);

            
            
            if (!jsop_setelem(true))
                return Compile_Error;

            
            
            frame.popn(2);
          END_CASE(JSOP_ENUMELEM)

          BEGIN_CASE(JSOP_BLOCKCHAIN)
          END_CASE(JSOP_BLOCKCHAIN)

          BEGIN_CASE(JSOP_NULLBLOCKCHAIN)
          END_CASE(JSOP_NULLBLOCKCHAIN)

          BEGIN_CASE(JSOP_CONDSWITCH)
            
          END_CASE(JSOP_CONDSWITCH)

          BEGIN_CASE(JSOP_DEFFUN)
          {
            REJOIN_SITE_ANY();
            uint32 index = fullAtomIndex(PC);
            JSFunction *innerFun = script->getFunction(index);

            if (script->fun && script->bindings.hasBinding(cx, innerFun->atom))
                frame.syncAndForgetEverything();

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(innerFun), Registers::ArgReg1);
            INLINE_STUBCALL(STRICT_VARIANT(stubs::DefFun));
          }
          END_CASE(JSOP_DEFFUN)

          BEGIN_CASE(JSOP_DEFVAR)
          BEGIN_CASE(JSOP_DEFCONST)
          {
            REJOIN_SITE_ANY();
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DefVarOrConst);
          }
          END_CASE(JSOP_DEFVAR)

          BEGIN_CASE(JSOP_SETCONST)
          {
            REJOIN_SITE_ANY();
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            if (script->fun && script->bindings.hasBinding(cx, atom))
                frame.syncAndForgetEverything();

            prepareStubCall(Uses(1));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::SetConst);
          }
          END_CASE(JSOP_SETCONST)

          BEGIN_CASE(JSOP_DEFLOCALFUN_FC)
          {
            REJOIN_SITE_ANY();
            updateVarType();
            uint32 slot = GET_SLOTNO(PC);
            JSFunction *fun = script->getFunction(fullAtomIndex(&PC[SLOTNO_LEN]));
            prepareStubCall(Uses(frame.frameSlots()));
            masm.move(ImmPtr(fun), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DefLocalFun_FC);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
            frame.storeLocal(slot, JSVAL_TYPE_OBJECT, true);
            frame.pop();
          }
          END_CASE(JSOP_DEFLOCALFUN_FC)

          BEGIN_CASE(JSOP_LAMBDA)
          {
            JSFunction *fun = script->getFunction(fullAtomIndex(PC));

            JSObjStubFun stub = stubs::Lambda;
            uint32 uses = 0;

            jsbytecode *pc2 = AdvanceOverBlockchainOp(PC + JSOP_LAMBDA_LENGTH);
            JSOp next = JSOp(*pc2);
            
            if (next == JSOP_INITMETHOD) {
                stub = stubs::LambdaForInit;
            } else if (next == JSOP_SETMETHOD) {
                stub = stubs::LambdaForSet;
                uses = 1;
            } else if (fun->joinable()) {
                if (next == JSOP_CALL) {
                    stub = stubs::LambdaJoinableForCall;
                    uses = frame.frameSlots();
                } else if (next == JSOP_NULL) {
                    stub = stubs::LambdaJoinableForNull;
                }
            }

            prepareStubCall(Uses(uses));
            masm.move(ImmPtr(fun), Registers::ArgReg1);

            if (stub == stubs::Lambda) {
                REJOIN_SITE(stub);
                INLINE_STUBCALL(stub);
            } else {
                REJOIN_SITE(stub);
                jsbytecode *savedPC = PC;
                PC = pc2;
                INLINE_STUBCALL(stub);
                PC = savedPC;
            }

            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
          }
          END_CASE(JSOP_LAMBDA)

          BEGIN_CASE(JSOP_TRY)
            frame.syncAndForgetEverything();
          END_CASE(JSOP_TRY)

          BEGIN_CASE(JSOP_GETFCSLOT)
          BEGIN_CASE(JSOP_CALLFCSLOT)
          {
            uintN index = GET_UINT16(PC);

            
            frame.pushCallee();
            RegisterID reg = frame.copyDataIntoReg(frame.peek(-1));
            frame.pop();

            
            Address upvarAddress(reg, JSObject::getFlatClosureUpvarsOffset());
            masm.loadPrivate(upvarAddress, reg);
            
            frame.freeReg(reg);
            frame.push(Address(reg, index * sizeof(Value)), knownPushedType(0));
            if (op == JSOP_CALLFCSLOT)
                frame.push(UndefinedValue());
          }
          END_CASE(JSOP_CALLFCSLOT)

          BEGIN_CASE(JSOP_ARGSUB)
          {
            REJOIN_SITE_ANY();
            prepareStubCall(Uses(0));
            masm.move(Imm32(GET_ARGNO(PC)), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::ArgSub);
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_ARGSUB)

          BEGIN_CASE(JSOP_ARGCNT)
          {
            REJOIN_SITE_ANY();
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::ArgCnt);
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_ARGCNT)

          BEGIN_CASE(JSOP_DEFLOCALFUN)
          {
            REJOIN_SITE_ANY();
            updateVarType();
            uint32 slot = GET_SLOTNO(PC);
            JSFunction *fun = script->getFunction(fullAtomIndex(&PC[SLOTNO_LEN]));
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(fun), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DefLocalFun);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
            frame.storeLocal(slot, JSVAL_TYPE_OBJECT, true);
            frame.pop();
          }
          END_CASE(JSOP_DEFLOCALFUN)

          BEGIN_CASE(JSOP_RETRVAL)
            emitReturn(NULL);
          END_CASE(JSOP_RETRVAL)

          BEGIN_CASE(JSOP_GETGNAME)
          BEGIN_CASE(JSOP_CALLGNAME)
          {
            uint32 index = fullAtomIndex(PC);
            jsop_getgname(index);
            frame.extra(frame.peek(-1)).name = script->getAtom(index);
            if (op == JSOP_CALLGNAME)
                jsop_callgname_epilogue();
          }
          END_CASE(JSOP_GETGNAME)

          BEGIN_CASE(JSOP_SETGNAME)
          {
            jsbytecode *next = &PC[JSOP_SETLOCAL_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            jsop_setgname(script->getAtom(fullAtomIndex(PC)), true, pop);
          }
          END_CASE(JSOP_SETGNAME)

          BEGIN_CASE(JSOP_REGEXP)
          {
            JSObject *regex = script->getRegExp(fullAtomIndex(PC));
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(regex), Registers::ArgReg1);
            INLINE_STUBCALL_NO_REJOIN(stubs::RegExp);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
          }
          END_CASE(JSOP_REGEXP)

          BEGIN_CASE(JSOP_OBJECT)
          {
            JSObject *object = script->getObject(fullAtomIndex(PC));
            RegisterID reg = frame.allocReg();
            masm.move(ImmPtr(object), reg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, reg);
          }
          END_CASE(JSOP_OBJECT)

          BEGIN_CASE(JSOP_CALLPROP)
          {
              




              AutoRejoinSite rejoinGetProp(this, JS_FUNC_TO_DATA_PTR(void *, stubs::GetProp),
                                           JS_FUNC_TO_DATA_PTR(void *, ic::GetProp));

              if (!jsop_callprop(script->getAtom(fullAtomIndex(PC))))
                  return Compile_Error;

              if (needRejoins(PC)) {
                  rejoinGetProp.oolRejoin(stubcc.masm.label());
                  stubcc.masm.infallibleVMCall(JS_FUNC_TO_DATA_PTR(void *, stubs::CallPropSwap),
                                               frame.totalDepth());
                  stubcc.rejoin(Changes(2));
              }
          }
          END_CASE(JSOP_CALLPROP)

          BEGIN_CASE(JSOP_UINT24)
            frame.push(Value(Int32Value((int32_t) GET_UINT24(PC))));
          END_CASE(JSOP_UINT24)

          BEGIN_CASE(JSOP_CALLELEM)
            jsop_getelem(true);
          END_CASE(JSOP_CALLELEM)

          BEGIN_CASE(JSOP_STOP)
            emitReturn(NULL);
            goto done;
          END_CASE(JSOP_STOP)

          BEGIN_CASE(JSOP_GETXPROP)
            if (!jsop_xname(script->getAtom(fullAtomIndex(PC))))
                return Compile_Error;
          END_CASE(JSOP_GETXPROP)

          BEGIN_CASE(JSOP_ENTERBLOCK)
            enterBlock(script->getObject(fullAtomIndex(PC)));
          END_CASE(JSOP_ENTERBLOCK);

          BEGIN_CASE(JSOP_LEAVEBLOCK)
            leaveBlock();
          END_CASE(JSOP_LEAVEBLOCK)

          BEGIN_CASE(JSOP_CALLLOCAL)
          {
            uint32 slot = GET_SLOTNO(PC);
            if (JSObject *singleton = pushedSingleton(0))
                frame.push(ObjectValue(*singleton));
            else
                frame.pushLocal(slot);
            frame.push(UndefinedValue());
          }
          END_CASE(JSOP_CALLLOCAL)

          BEGIN_CASE(JSOP_INT8)
            frame.push(Value(Int32Value(GET_INT8(PC))));
          END_CASE(JSOP_INT8)

          BEGIN_CASE(JSOP_INT32)
            frame.push(Value(Int32Value(GET_INT32(PC))));
          END_CASE(JSOP_INT32)

          BEGIN_CASE(JSOP_HOLE)
            frame.push(MagicValue(JS_ARRAY_HOLE));
          END_CASE(JSOP_HOLE)

          BEGIN_CASE(JSOP_LAMBDA_FC)
          {
            JSFunction *fun = script->getFunction(fullAtomIndex(PC));
            prepareStubCall(Uses(frame.frameSlots()));
            masm.move(ImmPtr(fun), Registers::ArgReg1);
            {
                REJOIN_SITE(stubs::FlatLambda);
                INLINE_STUBCALL(stubs::FlatLambda);
            }
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
          }
          END_CASE(JSOP_LAMBDA_FC)

          BEGIN_CASE(JSOP_TRACE)
          BEGIN_CASE(JSOP_NOTRACE)
          {
            if (analysis->jumpTarget(PC)) {
                interruptCheckHelper();
                recompileCheckHelper();
            }
          }
          END_CASE(JSOP_TRACE)

          BEGIN_CASE(JSOP_DEBUGGER)
          {
            REJOIN_SITE_ANY();
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(PC), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::Debugger);
          }
          END_CASE(JSOP_DEBUGGER)

          BEGIN_CASE(JSOP_UNBRAND)
            jsop_unbrand();
          END_CASE(JSOP_UNBRAND)

          BEGIN_CASE(JSOP_UNBRANDTHIS)
            jsop_this();
            jsop_unbrand();
            frame.pop();
          END_CASE(JSOP_UNBRANDTHIS)

          BEGIN_CASE(JSOP_GETGLOBAL)
          BEGIN_CASE(JSOP_CALLGLOBAL)
            jsop_getglobal(GET_SLOTNO(PC));
            if (op == JSOP_CALLGLOBAL)
                frame.push(UndefinedValue());
          END_CASE(JSOP_GETGLOBAL)

          default:
           
#ifdef JS_METHODJIT_SPEW
            JaegerSpew(JSpew_Abort, "opcode %s not handled yet (%s line %d)\n", OpcodeNames[op],
                       script->filename, js_PCToLineNumber(cx, script, PC));
#endif
            return Compile_Abort;
        }

    

 

        if (cx->typeInferenceEnabled() && PC == oldPC + analyze::GetBytecodeLength(oldPC)) {
            




            unsigned nuses = analyze::GetUseCount(script, oldPC - script->code);
            unsigned ndefs = analyze::GetDefCount(script, oldPC - script->code);
            for (unsigned i = 0; i < ndefs; i++) {
                FrameEntry *fe = frame.getStack(opinfo->stackDepth - nuses + i);
                if (fe) {
                    
                    frame.extra(fe).types = analysis->pushedTypes(oldPC - script->code, i);
                }
            }
        }

#ifdef DEBUG
        frame.assertValidRegisterState();
#endif
    }

  done:
    return Compile_Okay;
}

#undef END_CASE
#undef BEGIN_CASE

JSC::MacroAssembler::Label
mjit::Compiler::labelOf(jsbytecode *pc, uint32 inlineIndex)
{
    ActiveFrame *a = (inlineIndex == uint32(-1)) ? outer : inlineFrames[inlineIndex];
    JS_ASSERT(uint32(pc - a->script->code) < a->script->length);

    uint32 offs = uint32(pc - a->script->code);
    JS_ASSERT(a->jumpMap[offs].isValid());
    return a->jumpMap[offs];
}

uint32
mjit::Compiler::fullAtomIndex(jsbytecode *pc)
{
    return GET_SLOTNO(pc);

    
#if 0
    return GET_SLOTNO(pc) + (atoms - script->atomMap.vector);
#endif
}

bool
mjit::Compiler::knownJump(jsbytecode *pc)
{
    return pc < PC;
}

bool
mjit::Compiler::jumpInScript(Jump j, jsbytecode *pc)
{
    JS_ASSERT(pc >= script->code && uint32(pc - script->code) < script->length);

    if (pc < PC) {
        j.linkTo(a->jumpMap[uint32(pc - script->code)], &masm);
        return true;
    }
    return branchPatches.append(BranchPatch(j, pc, a->inlineIndex));
}

void
mjit::Compiler::jsop_getglobal(uint32 index)
{
    REJOIN_SITE_ANY();

    JS_ASSERT(globalObj);
    uint32 slot = script->getGlobalSlot(index);

    JSObject *singleton = pushedSingleton(0);
    if (singleton && !globalObj->getSlot(slot).isUndefined()) {
        frame.push(ObjectValue(*singleton));
        return;
    }

    if (cx->typeInferenceEnabled() && globalObj->isGlobal() &&
        !globalObj->getType()->unknownProperties()) {
        Value *value = &globalObj->getSlotRef(slot);
        if (!value->isUndefined()) {
            watchGlobalReallocation();
            RegisterID reg = frame.allocReg();
            masm.move(ImmPtr(value), reg);
            frame.push(Address(reg), knownPushedType(0), true);
            return;
        }
    }

    RegisterID reg = frame.allocReg();
    Address address = masm.objSlotRef(globalObj, reg, slot);
    frame.push(address, knownPushedType(0));
    frame.freeReg(reg);

    



    if (cx->typeInferenceEnabled() &&
        globalObj->getSlot(slot).isUndefined() &&
        (JSOp(*PC) == JSOP_CALLGLOBAL || PC[JSOP_GETGLOBAL_LENGTH] != JSOP_POP)) {
        Jump jump = masm.testUndefined(Assembler::Equal, address);
        stubcc.linkExit(jump, Uses(0));
        stubcc.leave();
        OOL_STUBCALL(stubs::UndefinedHelper);
        stubcc.rejoin(Changes(1));
    }
}

void
mjit::Compiler::emitFinalReturn(Assembler &masm)
{
    masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfNcode()), Registers::ReturnReg);
    masm.jump(Registers::ReturnReg);
}













void
mjit::Compiler::loadReturnValue(Assembler *masm, FrameEntry *fe)
{
    RegisterID typeReg = JSReturnReg_Type;
    RegisterID dataReg = JSReturnReg_Data;

    if (fe) {
        
        
        if (masm != &this->masm) {
            if (fe->isConstant()) {
                stubcc.masm.loadValueAsComponents(fe->getValue(), typeReg, dataReg);
            } else {
                Address rval(frame.addressOf(fe));
                if (fe->isTypeKnown() && !fe->isType(JSVAL_TYPE_DOUBLE)) {
                    stubcc.masm.loadPayload(rval, dataReg);
                    stubcc.masm.move(ImmType(fe->getKnownType()), typeReg);
                } else {
                    stubcc.masm.loadValueAsComponents(rval, typeReg, dataReg);
                }
            }
        } else {
            frame.loadForReturn(fe, typeReg, dataReg, Registers::ReturnReg);
        }
    } else {
         
         
        masm->loadValueAsComponents(UndefinedValue(), typeReg, dataReg);
        if (analysis->usesReturnValue()) {
            Jump rvalClear = masm->branchTest32(Assembler::Zero,
                                               FrameFlagsAddress(),
                                               Imm32(StackFrame::HAS_RVAL));
            Address rvalAddress(JSFrameReg, StackFrame::offsetOfReturnValue());
            masm->loadValueAsComponents(rvalAddress, typeReg, dataReg);
            rvalClear.linkTo(masm->label(), masm);
        }
    }
}





void
mjit::Compiler::fixPrimitiveReturn(Assembler *masm, FrameEntry *fe)
{
    JS_ASSERT(isConstructing);

    bool ool = (masm != &this->masm);
    Address thisv(JSFrameReg, StackFrame::offsetOfThis(script->fun));

    
    
    
    if ((!fe && !analysis->usesReturnValue()) ||
        (fe && fe->isTypeKnown() && fe->getKnownType() != JSVAL_TYPE_OBJECT))
    {
        if (ool)
            masm->loadValueAsComponents(thisv, JSReturnReg_Type, JSReturnReg_Data);
        else
            frame.loadThisForReturn(JSReturnReg_Type, JSReturnReg_Data, Registers::ReturnReg);
        return;
    }

    
    if (fe && fe->isTypeKnown() && fe->getKnownType() == JSVAL_TYPE_OBJECT) {
        loadReturnValue(masm, fe);
        return;
    }

    
    
    loadReturnValue(masm, fe);
    Jump j = masm->testObject(Assembler::Equal, JSReturnReg_Type);
    masm->loadValueAsComponents(thisv, JSReturnReg_Type, JSReturnReg_Data);
    j.linkTo(masm->label(), masm);
}




void
mjit::Compiler::emitReturnValue(Assembler *masm, FrameEntry *fe)
{
    if (isConstructing)
        fixPrimitiveReturn(masm, fe);
    else
        loadReturnValue(masm, fe);
}

void
mjit::Compiler::emitInlineReturnValue(FrameEntry *fe)
{
    JS_ASSERT(!isConstructing && a->needReturnValue);

    if (a->syncReturnValue) {
        
        Address address = frame.addressForInlineReturn();
        if (fe)
            frame.storeTo(fe, address);
        else
            masm.storeValue(UndefinedValue(), address);
        return;
    }

    if (a->returnValueDouble) {
        JS_ASSERT(fe);
        frame.ensureDouble(fe);
        Registers mask(a->returnSet
                       ? Registers::maskReg(a->returnRegister)
                       : Registers::AvailFPRegs);
        FPRegisterID fpreg;
        if (!fe->isConstant()) {
            fpreg = frame.tempRegInMaskForData(fe, mask.freeMask).fpreg();
        } else {
            fpreg = frame.allocReg(mask.freeMask).fpreg();
            masm.slowLoadConstantDouble(fe->getValue().toDouble(), fpreg);
        }
        JS_ASSERT_IF(a->returnSet, fpreg == a->returnRegister.fpreg());
        a->returnRegister = fpreg;
    } else {
        Registers mask(a->returnSet
                       ? Registers::maskReg(a->returnRegister)
                       : Registers::AvailRegs);
        RegisterID reg;
        if (fe && !fe->isConstant()) {
            reg = frame.tempRegInMaskForData(fe, mask.freeMask).reg();
        } else {
            reg = frame.allocReg(mask.freeMask).reg();
            Value val = fe ? fe->getValue() : UndefinedValue();
            masm.loadValuePayload(val, reg);
        }
        JS_ASSERT_IF(a->returnSet, reg == a->returnRegister.reg());
        a->returnRegister = reg;
    }
}

void
mjit::Compiler::emitReturn(FrameEntry *fe)
{
    JS_ASSERT_IF(!script->fun, JSOp(*PC) == JSOP_STOP);

    
    JS_ASSERT_IF(fe, fe == frame.peek(-1));

    if (debugMode() || Probes::callTrackingActive(cx)) {
        REJOIN_SITE(stubs::ScriptDebugEpilogue);
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::ScriptDebugEpilogue);
    }

    if (a != outer) {
        





        if (a->needReturnValue)
            emitInlineReturnValue(fe);

        
        if (!a->returnSet) {
            a->returnParentRegs = frame.getParentRegs().freeMask & ~a->temporaryParentRegs.freeMask;
            if (a->needReturnValue && !a->syncReturnValue &&
                a->returnParentRegs.hasReg(a->returnRegister)) {
                a->returnParentRegs.takeReg(a->returnRegister);
            }
        }

        frame.discardLocalRegisters();
        frame.syncParentRegistersInMask(masm,
            frame.getParentRegs().freeMask & ~a->returnParentRegs.freeMask &
            ~a->temporaryParentRegs.freeMask, true);
        frame.restoreParentRegistersInMask(masm,
            a->returnParentRegs.freeMask & ~frame.getParentRegs().freeMask, true);

        a->returnSet = true;

        



        bool endOfScript =
            (JSOp(*PC) == JSOP_STOP) ||
            (JSOp(*PC) == JSOP_RETURN &&
             (JSOp(*(PC + JSOP_RETURN_LENGTH)) == JSOP_STOP &&
              !analysis->maybeCode(PC + JSOP_RETURN_LENGTH)));
        if (!endOfScript)
            a->returnJumps->append(masm.jump());

        frame.discardFrame();
        return;
    }

    









    if (script->fun && script->fun->isHeavyweight()) {
        
        prepareStubCall(Uses(fe ? 1 : 0));
        INLINE_STUBCALL_NO_REJOIN(stubs::PutActivationObjects);
    } else {
        
        Jump putObjs = masm.branchTest32(Assembler::NonZero,
                                         Address(JSFrameReg, StackFrame::offsetOfFlags()),
                                         Imm32(StackFrame::HAS_CALL_OBJ | StackFrame::HAS_ARGS_OBJ));
        stubcc.linkExit(putObjs, Uses(frame.frameSlots()));

        stubcc.leave();
        OOL_STUBCALL_NO_REJOIN(stubs::PutActivationObjects);

        emitReturnValue(&stubcc.masm, fe);
        emitFinalReturn(stubcc.masm);
    }

    emitReturnValue(&masm, fe);
    emitFinalReturn(masm);

    





    frame.discardFrame();
}

void
mjit::Compiler::prepareStubCall(Uses uses)
{
    JaegerSpew(JSpew_Insns, " ---- STUB CALL, SYNCING FRAME ---- \n");
    frame.syncAndKill(Registers(Registers::TempAnyRegs), uses);
    JaegerSpew(JSpew_Insns, " ---- FRAME SYNCING DONE ---- \n");
}

JSC::MacroAssembler::Call
mjit::Compiler::emitStubCall(void *ptr, DataLabelPtr *pinline)
{
    JaegerSpew(JSpew_Insns, " ---- CALLING STUB ---- \n");
    Call cl = masm.fallibleVMCall(cx->typeInferenceEnabled(),
                                  ptr, outerPC(), pinline, frame.totalDepth());
    JaegerSpew(JSpew_Insns, " ---- END STUB CALL ---- \n");
    return cl;
}

void
mjit::Compiler::interruptCheckHelper()
{
    REJOIN_SITE(stubs::Interrupt);

    






#ifdef JS_THREADSAFE
    void *interrupt = (void*) &cx->runtime->interruptCounter;
#else
    void *interrupt = (void*) &JS_THREAD_DATA(cx)->interruptFlags;
#endif

#if defined(JS_CPU_X86) || defined(JS_CPU_ARM)
    Jump jump = masm.branch32(Assembler::NotEqual, AbsoluteAddress(interrupt), Imm32(0));
#else
    
    RegisterID reg = frame.allocReg();
    masm.move(ImmPtr(interrupt), reg);
    Jump jump = masm.branchTest32(Assembler::NonZero, Address(reg, 0));
    frame.freeReg(reg);
#endif

    stubcc.linkExitDirect(jump, stubcc.masm.label());

    frame.sync(stubcc.masm, Uses(0));
    stubcc.masm.move(ImmPtr(PC), Registers::ArgReg1);
    OOL_STUBCALL(stubs::Interrupt);
    stubcc.rejoin(Changes(0));
}

void
mjit::Compiler::recompileCheckHelper()
{
    REJOIN_SITE(stubs::RecompileForInline);

    if (!analysis->hasFunctionCalls() || !cx->typeInferenceEnabled() ||
        script->callCount() >= CALLS_BACKEDGES_BEFORE_INLINING) {
        return;
    }

    size_t *addr = script->addressOfCallCount();
    masm.add32(Imm32(1), AbsoluteAddress(addr));
#if defined(JS_CPU_X86) || defined(JS_CPU_ARM)
    Jump jump = masm.branch32(Assembler::GreaterThanOrEqual, AbsoluteAddress(addr),
                              Imm32(CALLS_BACKEDGES_BEFORE_INLINING));
#else
    
    RegisterID reg = frame.allocReg();
    masm.move(ImmPtr(addr), reg);
    Jump jump = masm.branch32(Assembler::GreaterThanOrEqual, Address(reg, 0),
                              Imm32(CALLS_BACKEDGES_BEFORE_INLINING));
    frame.freeReg(reg);
#endif
    stubcc.linkExit(jump, Uses(0));
    stubcc.leave();

    OOL_STUBCALL(stubs::RecompileForInline);
    stubcc.rejoin(Changes(0));
}

void
mjit::Compiler::addReturnSite(bool ool)
{
    Assembler &masm = ool ? stubcc.masm : this->masm;
    InternalCallSite site(masm.distanceOf(masm.label()), a->inlineIndex, PC,
                          CallSite::NCODE_RETURN_ID, ool, true);
    addCallSite(site);
    masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfPrev()), JSFrameReg);
}

void
mjit::Compiler::emitUncachedCall(uint32 argc, bool callingNew)
{
    CallPatchInfo callPatch;

    RegisterID r0 = Registers::ReturnReg;
    VoidPtrStubUInt32 stub = callingNew ? stubs::UncachedNew : stubs::UncachedCall;

    frame.syncAndKill(Uses(argc + 2));
    prepareStubCall(Uses(argc + 2));
    masm.move(Imm32(argc), Registers::ArgReg1);
    INLINE_STUBCALL(stub);

    Jump notCompiled = masm.branchTestPtr(Assembler::Zero, r0, r0);

    masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.sp)), JSFrameReg);
    callPatch.hasFastNcode = true;
    callPatch.fastNcodePatch =
        masm.storePtrWithPatch(ImmPtr(NULL),
                               Address(JSFrameReg, StackFrame::offsetOfNcode()));

    masm.jump(r0);
    callPatch.joinPoint = masm.label();
    addReturnSite(false );

    frame.popn(argc + 2);

    frame.takeReg(JSReturnReg_Type);
    frame.takeReg(JSReturnReg_Data);
    frame.pushRegs(JSReturnReg_Type, JSReturnReg_Data, knownPushedType(0));

    stubcc.linkExitDirect(notCompiled, stubcc.masm.label());
    stubcc.rejoin(Changes(1));
    callPatches.append(callPatch);
}

static bool
IsLowerableFunCallOrApply(jsbytecode *pc)
{
#ifdef JS_MONOIC
    return (*pc == JSOP_FUNCALL && GET_ARGC(pc) >= 1) ||
           (*pc == JSOP_FUNAPPLY && GET_ARGC(pc) == 2);
#else
    return false;
#endif
}

void
mjit::Compiler::checkCallApplySpeculation(uint32 callImmArgc, uint32 speculatedArgc,
                                          FrameEntry *origCallee, FrameEntry *origThis,
                                          MaybeRegisterID origCalleeType, RegisterID origCalleeData,
                                          MaybeRegisterID origThisType, RegisterID origThisData,
                                          Jump *uncachedCallSlowRejoin, CallPatchInfo *uncachedCallPatch)
{
    JS_ASSERT(IsLowerableFunCallOrApply(PC));

    




    MaybeJump isObj;
    if (origCalleeType.isSet())
        isObj = masm.testObject(Assembler::NotEqual, origCalleeType.reg());
    Jump isFun = masm.testFunction(Assembler::NotEqual, origCalleeData);
    masm.loadObjPrivate(origCalleeData, origCalleeData);
    Native native = *PC == JSOP_FUNCALL ? js_fun_call : js_fun_apply;
    Jump isNative = masm.branchPtr(Assembler::NotEqual,
                                   Address(origCalleeData, JSFunction::offsetOfNativeOrScript()),
                                   ImmPtr(JS_FUNC_TO_DATA_PTR(void *, native)));

    



    {
        if (isObj.isSet())
            stubcc.linkExitDirect(isObj.getJump(), stubcc.masm.label());
        stubcc.linkExitDirect(isFun, stubcc.masm.label());
        stubcc.linkExitDirect(isNative, stubcc.masm.label());

        int32 frameDepthAdjust;
        if (applyTricks == LazyArgsObj) {
            OOL_STUBCALL_NO_REJOIN(stubs::Arguments);
            frameDepthAdjust = +1;
        } else {
            frameDepthAdjust = 0;
        }

        stubcc.masm.move(Imm32(callImmArgc), Registers::ArgReg1);
        JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW CALL CODE ---- \n");
        OOL_STUBCALL_LOCAL_SLOTS(JS_FUNC_TO_DATA_PTR(void *, stubs::SlowCall),
                                 frame.totalDepth() + frameDepthAdjust);
        JaegerSpew(JSpew_Insns, " ---- END SLOW CALL CODE ---- \n");

        




        JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW RESTORE CODE ---- \n");
        Address rval = frame.addressOf(origCallee);  
        if (knownPushedType(0) == JSVAL_TYPE_DOUBLE)
            stubcc.masm.ensureInMemoryDouble(rval);
        stubcc.masm.loadValueAsComponents(rval, JSReturnReg_Type, JSReturnReg_Data);
        *uncachedCallSlowRejoin = stubcc.masm.jump();
        JaegerSpew(JSpew_Insns, " ---- END SLOW RESTORE CODE ---- \n");
    }

    




    if (*PC == JSOP_FUNAPPLY) {
        masm.store32(Imm32(applyTricks == LazyArgsObj),
                     FrameAddress(offsetof(VMFrame, u.call.lazyArgsObj)));
    }
}


bool
mjit::Compiler::canUseApplyTricks()
{
    JS_ASSERT(*PC == JSOP_ARGUMENTS);
    jsbytecode *nextpc = PC + JSOP_ARGUMENTS_LENGTH;
    return *nextpc == JSOP_FUNAPPLY &&
           IsLowerableFunCallOrApply(nextpc) &&
           !analysis->jumpTarget(nextpc) &&
           !debugMode() && !a->parent;
}


bool
mjit::Compiler::inlineCallHelper(uint32 callImmArgc, bool callingNew, FrameSize &callFrameSize)
{
    
    interruptCheckHelper();

    int32 speculatedArgc;
    if (applyTricks == LazyArgsObj) {
        frame.pop();
        speculatedArgc = 1;
    } else {
        speculatedArgc = callImmArgc;
    }

    FrameEntry *origCallee = frame.peek(-(speculatedArgc + 2));
    FrameEntry *origThis = frame.peek(-(speculatedArgc + 1));

    




    if (callingNew)
        frame.discardFe(origThis);

    if (!cx->typeInferenceEnabled()) {
        CompileStatus status = callArrayBuiltin(callImmArgc, callingNew);
        if (status != Compile_InlineAbort)
            return status;
    }

    











    bool lowerFunCallOrApply = IsLowerableFunCallOrApply(PC);

    bool newType = callingNew && cx->typeInferenceEnabled() && types::UseNewType(cx, script, PC);

#ifdef JS_MONOIC
    if (debugMode() || newType) {
#endif
        if (applyTricks == LazyArgsObj) {
            
            jsop_arguments();
            frame.pushSynced(JSVAL_TYPE_UNKNOWN);
        }
        emitUncachedCall(callImmArgc, callingNew);
        applyTricks = NoApplyTricks;
        return true;
#ifdef JS_MONOIC
    }

    frame.forgetMismatchedObject(origCallee);
    if (lowerFunCallOrApply)
        frame.forgetMismatchedObject(origThis);

    
    CallGenInfo     callIC;
    CallPatchInfo   callPatch;
    MaybeRegisterID icCalleeType; 
    RegisterID      icCalleeData; 
    Address         icRvalAddr;   

    














    
    Jump            uncachedCallSlowRejoin;
    CallPatchInfo   uncachedCallPatch;

    {
        MaybeRegisterID origCalleeType, maybeOrigCalleeData;
        RegisterID origCalleeData;

        
        frame.ensureFullRegs(origCallee, &origCalleeType, &maybeOrigCalleeData);
        origCalleeData = maybeOrigCalleeData.reg();
        PinRegAcrossSyncAndKill p1(frame, origCalleeData), p2(frame, origCalleeType);

        if (lowerFunCallOrApply) {
            MaybeRegisterID origThisType, maybeOrigThisData;
            RegisterID origThisData;
            {
                
                frame.ensureFullRegs(origThis, &origThisType, &maybeOrigThisData);
                origThisData = maybeOrigThisData.reg();
                PinRegAcrossSyncAndKill p3(frame, origThisData), p4(frame, origThisType);

                
                frame.syncAndKill(Uses(speculatedArgc + 2));
            }

            checkCallApplySpeculation(callImmArgc, speculatedArgc,
                                      origCallee, origThis,
                                      origCalleeType, origCalleeData,
                                      origThisType, origThisData,
                                      &uncachedCallSlowRejoin, &uncachedCallPatch);

            icCalleeType = origThisType;
            icCalleeData = origThisData;
            icRvalAddr = frame.addressOf(origThis);

            





            if (*PC == JSOP_FUNCALL)
                callIC.frameSize.initStatic(frame.totalDepth(), speculatedArgc - 1);
            else
                callIC.frameSize.initDynamic();
        } else {
            
            frame.syncAndKill(Uses(speculatedArgc + 2));

            icCalleeType = origCalleeType;
            icCalleeData = origCalleeData;
            icRvalAddr = frame.addressOf(origCallee);
            callIC.frameSize.initStatic(frame.totalDepth(), speculatedArgc);
        }
    }

    callFrameSize = callIC.frameSize;

    callIC.argTypes = NULL;
    callIC.typeMonitored = monitored(PC);
    if (callIC.typeMonitored && callIC.frameSize.isStatic()) {
        unsigned argc = callIC.frameSize.staticArgc();
        callIC.argTypes = (types::ClonedTypeSet *)
            js_calloc((1 + argc) * sizeof(types::ClonedTypeSet));
        if (!callIC.argTypes) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        types::TypeSet *types = frame.extra(frame.peek(-((int)argc + 1))).types;
        types::TypeSet::Clone(cx, types, &callIC.argTypes[0]);
        for (unsigned i = 0; i < argc; i++) {
            types::TypeSet *types = frame.extra(frame.peek(-((int)argc - i))).types;
            types::TypeSet::Clone(cx, types, &callIC.argTypes[i + 1]);
        }
    }

    
    MaybeJump notObjectJump;
    if (icCalleeType.isSet())
        notObjectJump = masm.testObject(Assembler::NotEqual, icCalleeType.reg());

    



    Registers tempRegs(Registers::AvailRegs);
    if (callIC.frameSize.isDynamic() && !Registers::isSaved(icCalleeData)) {
        RegisterID x = tempRegs.takeAnyReg(Registers::SavedRegs).reg();
        masm.move(icCalleeData, x);
        icCalleeData = x;
    } else {
        tempRegs.takeReg(icCalleeData);
    }
    RegisterID funPtrReg = tempRegs.takeAnyReg(Registers::SavedRegs).reg();

    
    RESERVE_IC_SPACE(masm);

    




    Jump j = masm.branchPtrWithPatch(Assembler::NotEqual, icCalleeData, callIC.funGuard);
    callIC.funJump = j;

    
    RESERVE_OOL_SPACE(stubcc.masm);

    Jump rejoin1, rejoin2;
    {
        



        AutoRejoinSite autoRejoinSplat(this,
            JS_FUNC_TO_DATA_PTR(void *, ic::SplatApplyArgs));

        RESERVE_OOL_SPACE(stubcc.masm);
        stubcc.linkExitDirect(j, stubcc.masm.label());
        callIC.slowPathStart = stubcc.masm.label();

        



        Jump notFunction = stubcc.masm.testFunction(Assembler::NotEqual, icCalleeData);

        
        RegisterID tmp = tempRegs.takeAnyReg().reg();
        stubcc.masm.loadObjPrivate(icCalleeData, funPtrReg);
        stubcc.masm.load16(Address(funPtrReg, offsetof(JSFunction, flags)), tmp);
        stubcc.masm.and32(Imm32(JSFUN_KINDMASK), tmp);
        Jump isNative = stubcc.masm.branch32(Assembler::Below, tmp, Imm32(JSFUN_INTERPRETED));
        tempRegs.putReg(tmp);

        




        if (callIC.frameSize.isDynamic()) {
            OOL_STUBCALL(ic::SplatApplyArgs);
            autoRejoinSplat.oolRejoin(stubcc.masm.label());
        }

        



        Jump toPatch = stubcc.masm.jump();
        toPatch.linkTo(stubcc.masm.label(), &stubcc.masm);
        callIC.oolJump = toPatch;
        callIC.icCall = stubcc.masm.label();

        




        callIC.addrLabel1 = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
        void *icFunPtr = JS_FUNC_TO_DATA_PTR(void *, callingNew ? ic::New : ic::Call);
        if (callIC.frameSize.isStatic())
            callIC.oolCall = OOL_STUBCALL_LOCAL_SLOTS(icFunPtr, frame.totalDepth());
        else
            callIC.oolCall = OOL_STUBCALL_LOCAL_SLOTS(icFunPtr, -1);

        callIC.funObjReg = icCalleeData;
        callIC.funPtrReg = funPtrReg;

        



        rejoin1 = stubcc.masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                            Registers::ReturnReg);
        if (callIC.frameSize.isStatic())
            stubcc.masm.move(Imm32(callIC.frameSize.staticArgc()), JSParamReg_Argc);
        else
            stubcc.masm.load32(FrameAddress(offsetof(VMFrame, u.call.dynamicArgc)), JSParamReg_Argc);
        stubcc.masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.sp)), JSFrameReg);
        callPatch.hasSlowNcode = true;
        callPatch.slowNcodePatch =
            stubcc.masm.storePtrWithPatch(ImmPtr(NULL),
                                          Address(JSFrameReg, StackFrame::offsetOfNcode()));
        stubcc.masm.jump(Registers::ReturnReg);



        






        if (notObjectJump.isSet())
            stubcc.linkExitDirect(notObjectJump.get(), stubcc.masm.label());
        notFunction.linkTo(stubcc.masm.label(), &stubcc.masm);
        isNative.linkTo(stubcc.masm.label(), &stubcc.masm);

        callIC.addrLabel2 = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
        OOL_STUBCALL(callingNew ? ic::NativeNew : ic::NativeCall);

        rejoin2 = stubcc.masm.jump();
    }

    



    callIC.hotPathLabel = masm.label();

    uint32 flags = 0;
    if (callingNew)
        flags |= StackFrame::CONSTRUCTING;

    InlineFrameAssembler inlFrame(masm, callIC, flags);
    callPatch.hasFastNcode = true;
    callPatch.fastNcodePatch = inlFrame.assemble(NULL);

    callIC.hotJump = masm.jump();
    callIC.joinPoint = callPatch.joinPoint = masm.label();
    callIC.callIndex = callSites.length();
    addReturnSite(false );
    if (lowerFunCallOrApply)
        uncachedCallPatch.joinPoint = callIC.joinPoint;

    



    CHECK_IC_SPACE();

    JSValueType type = knownPushedType(0);

    frame.popn(speculatedArgc + 2);
    frame.takeReg(JSReturnReg_Type);
    frame.takeReg(JSReturnReg_Data);
    frame.pushRegs(JSReturnReg_Type, JSReturnReg_Data, type);

    





    callIC.slowJoinPoint = stubcc.masm.label();
    rejoin1.linkTo(callIC.slowJoinPoint, &stubcc.masm);
    rejoin2.linkTo(callIC.slowJoinPoint, &stubcc.masm);
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW RESTORE CODE ---- \n");
    frame.reloadEntry(stubcc.masm, icRvalAddr, frame.peek(-1));
    stubcc.crossJump(stubcc.masm.jump(), masm.label());
    JaegerSpew(JSpew_Insns, " ---- END SLOW RESTORE CODE ---- \n");

    CHECK_OOL_SPACE();

    if (lowerFunCallOrApply)
        stubcc.crossJump(uncachedCallSlowRejoin, masm.label());

    callICs.append(callIC);
    callPatches.append(callPatch);
    if (lowerFunCallOrApply)
        callPatches.append(uncachedCallPatch);

    applyTricks = NoApplyTricks;
    return true;
#endif
}

CompileStatus
mjit::Compiler::callArrayBuiltin(uint32 argc, bool callingNew)
{
    if (!script->compileAndGo)
        return Compile_InlineAbort;

    if (applyTricks == LazyArgsObj)
        return Compile_InlineAbort;

    FrameEntry *origCallee = frame.peek(-((int)argc + 2));
    if (origCallee->isNotType(JSVAL_TYPE_OBJECT))
        return Compile_InlineAbort;

    if (frame.extra(origCallee).name != cx->runtime->atomState.classAtoms[JSProto_Array])
        return Compile_InlineAbort;

    JSObject *arrayObj;
    if (!js_GetClassObject(cx, globalObj, JSProto_Array, &arrayObj))
        return Compile_Error;

    JSObject *arrayProto;
    if (!js_GetClassPrototype(cx, globalObj, JSProto_Array, &arrayProto))
        return Compile_Error;

    if (argc > 1)
        return Compile_InlineAbort;
    FrameEntry *origArg = (argc == 1) ? frame.peek(-1) : NULL;
    if (origArg) {
        if (origArg->isNotType(JSVAL_TYPE_INT32))
            return Compile_InlineAbort;
        if (origArg->isConstant() && origArg->getValue().toInt32() < 0)
            return Compile_InlineAbort;
    }

    if (!origCallee->isTypeKnown()) {
        Jump notObject = frame.testObject(Assembler::NotEqual, origCallee);
        stubcc.linkExit(notObject, Uses(argc + 2));
    }

    RegisterID reg = frame.tempRegForData(origCallee);
    Jump notArray = masm.branchPtr(Assembler::NotEqual, reg, ImmPtr(arrayObj));
    stubcc.linkExit(notArray, Uses(argc + 2));

    int32 knownSize = 0;
    MaybeRegisterID sizeReg;
    if (origArg) {
        if (origArg->isConstant()) {
            knownSize = origArg->getValue().toInt32();
        } else {
            if (!origArg->isTypeKnown()) {
                Jump notInt = frame.testInt32(Assembler::NotEqual, origArg);
                stubcc.linkExit(notInt, Uses(argc + 2));
            }
            sizeReg = frame.tempRegForData(origArg);
            Jump belowZero = masm.branch32(Assembler::LessThan, sizeReg.reg(), Imm32(0));
            stubcc.linkExit(belowZero, Uses(argc + 2));
        }
    } else {
        knownSize = 0;
    }

    stubcc.leave();
    stubcc.masm.move(Imm32(argc), Registers::ArgReg1);
    OOL_STUBCALL(callingNew ? stubs::SlowNew : stubs::SlowCall);

    {
        PinRegAcrossSyncAndKill p1(frame, sizeReg);
        frame.popn(argc + 2);
        frame.syncAndKill(Uses(0));
    }

    prepareStubCall(Uses(0));
    masm.storePtr(ImmPtr(arrayProto), FrameAddress(offsetof(VMFrame, scratch)));
    if (sizeReg.isSet())
        masm.move(sizeReg.reg(), Registers::ArgReg1);
    else
        masm.move(Imm32(knownSize), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::NewDenseUnallocatedArray);

    frame.takeReg(Registers::ReturnReg);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
    frame.forgetType(frame.peek(-1));

    stubcc.rejoin(Changes(1));

    return Compile_Okay;
}


static const uint32 INLINE_SITE_LIMIT = 5;

CompileStatus
mjit::Compiler::inlineScriptedFunction(uint32 argc, bool callingNew)
{
    JS_ASSERT(inlining);

    if (!cx->typeInferenceEnabled())
        return Compile_InlineAbort;

    
    if (isConstructing || callingNew)
        return Compile_InlineAbort;

    if (applyTricks == LazyArgsObj)
        return Compile_InlineAbort;

    
    if (!outerScript->compileAndGo ||
        (outerScript->fun && outerScript->fun->getParent() != globalObj) ||
        (outerScript->fun && outerScript->fun->isHeavyweight()) ||
        outerScript->isActiveEval) {
        return Compile_InlineAbort;
    }

    FrameEntry *origCallee = frame.peek(-((int)argc + 2));
    FrameEntry *origThis = frame.peek(-((int)argc + 1));

    types::TypeSet *types = frame.extra(origCallee).types;
    if (!types || types->getKnownTypeTag(cx) != JSVAL_TYPE_OBJECT)
        return Compile_InlineAbort;

    



    types::ObjectKind kind = types->getKnownObjectKind(cx);
    if (kind != types::OBJECT_INLINEABLE_FUNCTION)
        return Compile_InlineAbort;

    if (types->getObjectCount() >= INLINE_SITE_LIMIT)
        return Compile_InlineAbort;

    




    uint32 stackLimit = outerScript->nslots + StackSpace::STACK_EXTRA - VALUES_PER_STACK_FRAME;

    



    unsigned count = types->getObjectCount();
    for (unsigned i = 0; i < count; i++) {
        types::TypeObject *object = types->getObject(i);
        if (!object)
            continue;

        if (!object->singleton || !object->singleton->isFunction())
            return Compile_InlineAbort;

        JSFunction *fun = object->singleton->getFunctionPrivate();
        if (!fun->isInterpreted())
            return Compile_InlineAbort;
        JSScript *script = fun->script();

        




        if (!script->compileAndGo ||
            fun->getParent() != globalObj ||
            outerScript->strictModeCode != script->strictModeCode) {
            return Compile_InlineAbort;
        }

        
        ActiveFrame *checka = a;
        while (checka) {
            if (checka->script == script)
                return Compile_InlineAbort;
            checka = checka->parent;
        }

        
        if (frame.totalDepth() + VALUES_PER_STACK_FRAME + fun->script()->nslots >= stackLimit)
            return Compile_InlineAbort;

        analyze::ScriptAnalysis *analysis = script->analysis(cx);
        if (analysis && !analysis->failed() && !analysis->ranBytecode())
            analysis->analyzeBytecode(cx);
        if (!analysis || analysis->OOM())
            return Compile_Error;
        if (analysis->failed())
            return Compile_Abort;

        if (!analysis->inlineable(argc))
            return Compile_InlineAbort;

        if (analysis->usesThisValue() && origThis->isNotType(JSVAL_TYPE_OBJECT))
            return Compile_InlineAbort;
    }

    types->addFreeze(cx);

    





    frame.tryCopyRegister(origThis, origCallee);
    for (unsigned i = 0; i < argc; i++)
        frame.tryCopyRegister(frame.peek(-((int)i + 1)), origCallee);

    




    MaybeRegisterID calleeReg;
    if (count > 1) {
        frame.forgetMismatchedObject(origCallee);
        calleeReg = frame.tempRegForData(origCallee);
    }
    MaybeJump calleePrevious;

    



    Registers temporaryParentRegs = frame.getTemporaryCallRegisters(origCallee);

    JSValueType returnType = knownPushedType(0);

    bool needReturnValue = JSOP_POP != (JSOp)*(PC + JSOP_CALL_LENGTH);
    bool syncReturnValue = needReturnValue && returnType == JSVAL_TYPE_UNKNOWN;

    
    bool returnSet = false;
    AnyRegisterID returnRegister;
    Registers returnParentRegs = 0;

    Vector<Jump, 4, CompilerAllocPolicy> returnJumps(CompilerAllocPolicy(cx, *this));

    for (unsigned i = 0; i < count; i++) {
        types::TypeObject *object = types->getObject(i);
        if (!object)
            continue;

        JSFunction *fun = object->singleton->getFunctionPrivate();

        CompileStatus status;

        status = pushActiveFrame(fun->script(), argc);
        if (status != Compile_Okay)
            return status;

        JaegerSpew(JSpew_Inlining, "inlining call to script (file \"%s\") (line \"%d\")\n",
                   script->filename, script->lineno);

        if (calleePrevious.isSet()) {
            calleePrevious.get().linkTo(masm.label(), &masm);
            calleePrevious = MaybeJump();
        }

        if (i + 1 != count) {
            
            JS_ASSERT(calleeReg.isSet());
            calleePrevious = masm.branchPtr(Assembler::NotEqual, calleeReg.reg(), ImmPtr(fun));
        }

        a->returnJumps = &returnJumps;
        a->needReturnValue = needReturnValue;
        a->syncReturnValue = syncReturnValue;
        a->returnValueDouble = returnType == JSVAL_TYPE_DOUBLE;
        if (returnSet) {
            a->returnSet = true;
            a->returnRegister = returnRegister;
            a->returnParentRegs = returnParentRegs;
        }
        a->temporaryParentRegs = temporaryParentRegs;

        status = generateMethod();
        if (status != Compile_Okay) {
            popActiveFrame();
            if (status == Compile_Abort) {
                
                if (!cx->markTypeFunctionUninlineable(fun->getType()))
                    return Compile_Error;
                return Compile_Retry;
            }
            return status;
        }

        if (!returnSet) {
            JS_ASSERT(a->returnSet);
            returnSet = true;
            returnRegister = a->returnRegister;
            returnParentRegs = a->returnParentRegs;
        }

        popActiveFrame();

        if (i + 1 != count)
            returnJumps.append(masm.jump());
    }

    for (unsigned i = 0; i < returnJumps.length(); i++)
        returnJumps[i].linkTo(masm.label(), &masm);

    Registers evictedRegisters = Registers(Registers::AvailAnyRegs & ~returnParentRegs.freeMask);
    frame.evictInlineModifiedRegisters(evictedRegisters);

    frame.popn(argc + 2);
    if (needReturnValue && !syncReturnValue) {
        frame.takeReg(returnRegister);
        if (returnRegister.isReg())
            frame.pushTypedPayload(returnType, returnRegister.reg());
        else
            frame.pushDouble(returnRegister.fpreg());
    } else {
        frame.pushSynced(JSVAL_TYPE_UNKNOWN);
    }

    JaegerSpew(JSpew_Inlining, "finished inlining call to script (file \"%s\") (line \"%d\")\n",
               script->filename, script->lineno);

    return Compile_Okay;
}






void
mjit::Compiler::addCallSite(const InternalCallSite &site)
{
    callSites.append(site);
}

void
mjit::Compiler::inlineStubCall(void *stub, bool needsRejoin)
{
    DataLabelPtr inlinePatch;
    Call cl = emitStubCall(stub, &inlinePatch);
    InternalCallSite site(masm.callReturnOffset(cl), a->inlineIndex, PC,
                          (size_t)stub, false, needsRejoin);
    site.inlinePatch = inlinePatch;
    if (loop && loop->generatingInvariants()) {
        Jump j = masm.jump();
        Label l = masm.label();
        loop->addInvariantCall(j, l, false, callSites.length(), true);
    }
    addCallSite(site);
}

#ifdef DEBUG
void
mjit::Compiler::checkRejoinSite(uint32 nCallSites, uint32 nRejoinSites, void *stub)
{
    JS_ASSERT(!variadicRejoin);
    size_t id = (size_t) stub;

    if (id == RejoinSite::VARIADIC_ID) {
        for (unsigned i = nCallSites; i < callSites.length(); i++)
            callSites[i].needsRejoin = false;
        variadicRejoin = true;
        return;
    }

    for (unsigned i = nCallSites; i < callSites.length(); i++) {
        if (callSites[i].id == id)
            callSites[i].needsRejoin = false;
    }
}
#endif

void
mjit::Compiler::addRejoinSite(void *stub, bool ool, Label oolLabel)
{
    JS_ASSERT(a == outer);

    InternalRejoinSite rejoin(stubcc.masm.label(), PC, (size_t) stub);
    rejoinSites.append(rejoin);

    




    if (stub == (void *) CallSite::NCODE_RETURN_ID)
        stubcc.masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfPrev()), JSFrameReg);
    else
        stubcc.masm.loadPtr(FrameAddress(VMFrame::offsetOfFp), JSFrameReg);

    







    frame.ensureInMemoryDoubles(stubcc.masm);

    
    if (loop && loop->generatingInvariants()) {
        Jump j = stubcc.masm.jump();
        Label l = stubcc.masm.label();
        loop->addInvariantCall(j, l, true, rejoinSites.length() - 1, false);
    }

    if (ool) {
        
        stubcc.masm.jump().linkTo(oolLabel, &stubcc.masm);
    } else {
        
        stubcc.rejoin(Changes(0));
    }
}

bool
mjit::Compiler::compareTwoValues(JSContext *cx, JSOp op, const Value &lhs, const Value &rhs)
{
    JS_ASSERT(lhs.isPrimitive());
    JS_ASSERT(rhs.isPrimitive());

    if (lhs.isString() && rhs.isString()) {
        int32 cmp;
        CompareStrings(cx, lhs.toString(), rhs.toString(), &cmp);
        switch (op) {
          case JSOP_LT:
            return cmp < 0;
          case JSOP_LE:
            return cmp <= 0;
          case JSOP_GT:
            return cmp > 0;
          case JSOP_GE:
            return cmp >= 0;
          case JSOP_EQ:
            return cmp == 0;
          case JSOP_NE:
            return cmp != 0;
          default:
            JS_NOT_REACHED("NYI");
        }
    } else {
        double ld, rd;
        
        
        ValueToNumber(cx, lhs, &ld);
        ValueToNumber(cx, rhs, &rd);
        switch(op) {
          case JSOP_LT:
            return ld < rd;
          case JSOP_LE:
            return ld <= rd;
          case JSOP_GT:
            return ld > rd;
          case JSOP_GE:
            return ld >= rd;
          case JSOP_EQ: 
          case JSOP_NE:
            
            if (lhs.isNullOrUndefined()) {
                if (rhs.isNullOrUndefined())
                    return op == JSOP_EQ;
                return op == JSOP_NE;
            }
            if (rhs.isNullOrUndefined())
                return op == JSOP_NE;

            
            return (op == JSOP_EQ) ? (ld == rd) : (ld != rd);
          default:
            JS_NOT_REACHED("NYI");
        }
    }

    JS_NOT_REACHED("NYI");
    return false;
}

bool
mjit::Compiler::emitStubCmpOp(BoolStub stub, AutoRejoinSite &autoRejoin, jsbytecode *target, JSOp fused)
{
    if (target) {
        fixDoubleTypes(target);
        frame.syncAndKillEverything();
    } else {
        frame.syncAndKill(Uses(2));
    }

    prepareStubCall(Uses(2));
    INLINE_STUBCALL(stub);
    frame.pop();
    frame.pop();

    if (!target) {
        frame.takeReg(Registers::ReturnReg);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
        return true;
    }

    if (needRejoins(PC)) {
        autoRejoin.oolRejoin(stubcc.masm.label());
        stubcc.rejoin(Changes(0));
    }

    JS_ASSERT(fused == JSOP_IFEQ || fused == JSOP_IFNE);
    Assembler::Condition cond = (fused == JSOP_IFEQ)
                                ? Assembler::Zero
                                : Assembler::NonZero;
    Jump j = masm.branchTest32(cond, Registers::ReturnReg,
                               Registers::ReturnReg);
    return jumpAndTrace(j, target);
}

void
mjit::Compiler::jsop_setprop_slow(JSAtom *atom, bool usePropCache)
{
    prepareStubCall(Uses(2));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    if (usePropCache)
        INLINE_STUBCALL(STRICT_VARIANT(stubs::SetName));
    else
        INLINE_STUBCALL(STRICT_VARIANT(stubs::SetPropNoCache));
    JS_STATIC_ASSERT(JSOP_SETNAME_LENGTH == JSOP_SETPROP_LENGTH);
    frame.shimmy(1);
}

void
mjit::Compiler::jsop_getprop_slow(JSAtom *atom, bool usePropCache)
{
    prepareStubCall(Uses(1));
    if (usePropCache) {
        INLINE_STUBCALL(stubs::GetProp);
    } else {
        masm.move(ImmPtr(atom), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::GetPropNoCache);
    }
    frame.pop();
    frame.pushSynced(JSVAL_TYPE_UNKNOWN);
}

bool
mjit::Compiler::jsop_callprop_slow(JSAtom *atom)
{
    prepareStubCall(Uses(1));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::CallProp);
    frame.pop();
    pushSyncedEntry(0);
    pushSyncedEntry(1);
    return true;
}

#ifdef JS_MONOIC
void
mjit::Compiler::passMICAddress(GlobalNameICInfo &ic)
{
    ic.addrLabel = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
}
#endif

#if defined JS_POLYIC
void
mjit::Compiler::passICAddress(BaseICInfo *ic)
{
    ic->paramAddr = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
}

bool
mjit::Compiler::jsop_getprop(JSAtom *atom, JSValueType knownType,
                             bool doTypeCheck, bool usePropCache)
{
    REJOIN_SITE_3(ic::GetProp, ic::GetPropNoCache, stubs::GetProp);
    FrameEntry *top = frame.peek(-1);

    
    if (atom == cx->runtime->atomState.lengthAtom &&
        top->isTypeKnown() && top->getKnownType() == JSVAL_TYPE_STRING) {
        if (top->isConstant()) {
            JSString *str = top->getValue().toString();
            Value v;
            v.setNumber(uint32(str->length()));
            frame.pop();
            frame.push(v);
        } else {
            RegisterID str = frame.ownRegForData(top);
            masm.loadPtr(Address(str, JSString::offsetOfLengthAndFlags()), str);
            masm.urshift32(Imm32(JSString::LENGTH_SHIFT), str);
            frame.pop();
            frame.pushTypedPayload(JSVAL_TYPE_INT32, str);
        }
        return true;
    }

    
    if (top->isNotType(JSVAL_TYPE_OBJECT)) {
        jsop_getprop_slow(atom, usePropCache);
        return true;
    }

    frame.forgetMismatchedObject(top);

    if (JSOp(*PC) == JSOP_LENGTH) {
        



        if (loop && loop->generatingInvariants()) {
            FrameEntry *fe = loop->invariantLength(top, frame.extra(top).types);
            if (fe) {
                frame.pop();
                frame.pushTemporary(fe);
                return true;
            }
        }

        




        types::TypeSet *types = frame.extra(top).types;
        types::ObjectKind kind = types ? types->getKnownObjectKind(cx) : types::OBJECT_UNKNOWN;
        if (kind == types::OBJECT_DENSE_ARRAY || kind == types::OBJECT_PACKED_ARRAY) {
            bool isObject = top->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, top);
                stubcc.linkExit(notObject, Uses(1));
                stubcc.leave();
                OOL_STUBCALL(stubs::GetProp);
            }
            RegisterID reg = frame.tempRegForData(top);
            frame.pop();
            frame.push(Address(reg, offsetof(JSObject, privateData)), JSVAL_TYPE_INT32);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            return true;
        }
    }

    




    JSOp op = JSOp(*PC);
    types::TypeSet *types = frame.extra(top).types;
    if (op == JSOP_GETPROP && types && !types->unknown() && types->getObjectCount() == 1 &&
        !types->getObject(0)->unknownProperties()) {
        JS_ASSERT(usePropCache);
        types::TypeObject *object = types->getObject(0);
        types::TypeSet *propertyTypes = object->getProperty(cx, ATOM_TO_JSID(atom), false);
        if (!propertyTypes)
            return false;
        if (propertyTypes->isDefiniteProperty() && !propertyTypes->isOwnProperty(cx, true)) {
            types->addFreeze(cx);
            uint32 slot = propertyTypes->definiteSlot();
            bool isObject = top->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, top);
                stubcc.linkExit(notObject, Uses(1));
                stubcc.leave();
                OOL_STUBCALL(stubs::GetProp);
            }
            RegisterID reg = frame.tempRegForData(top);
            frame.pop();
            frame.push(Address(reg, JSObject::getFixedSlotOffset(slot)), knownType);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            return true;
        }
    }

    




    RegisterID objReg = Registers::ReturnReg;
    RegisterID shapeReg = Registers::ReturnReg;
    if (atom == cx->runtime->atomState.lengthAtom) {
        objReg = frame.copyDataIntoReg(top);
        shapeReg = frame.allocReg();
    }

    RESERVE_IC_SPACE(masm);

    PICGenInfo pic(ic::PICInfo::GET, JSOp(*PC), usePropCache);

    
    Label typeCheck;
    if (doTypeCheck && !top->isTypeKnown()) {
        RegisterID reg = frame.tempRegForType(top);
        pic.typeReg = reg;

        
        pic.fastPathStart = masm.label();
        Jump j = masm.testObject(Assembler::NotEqual, reg);
        typeCheck = masm.label();
        RETURN_IF_OOM(false);

        pic.typeCheck = stubcc.linkExit(j, Uses(1));
        pic.hasTypeCheck = true;
    } else {
        pic.fastPathStart = masm.label();
        pic.hasTypeCheck = false;
        pic.typeReg = Registers::ReturnReg;
    }

    if (atom != cx->runtime->atomState.lengthAtom) {
        objReg = frame.copyDataIntoReg(top);
        shapeReg = frame.allocReg();
    }

    pic.shapeReg = shapeReg;
    pic.atom = atom;

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();

    DataLabel32 inlineShapeLabel;
    Jump j = masm.branch32WithPatch(Assembler::NotEqual, shapeReg,
                                    Imm32(int32(INVALID_SHAPE)),
                                    inlineShapeLabel);
    Label inlineShapeJump = masm.label();

    RESERVE_OOL_SPACE(stubcc.masm);
    pic.slowPathStart = stubcc.linkExit(j, Uses(1));

    stubcc.leave();
    passICAddress(&pic);
    pic.slowPathCall = OOL_STUBCALL(usePropCache ? ic::GetProp : ic::GetPropNoCache);
    CHECK_OOL_SPACE();

    
    Label dslotsLoadLabel = masm.loadPtrWithPatchToLEA(Address(objReg, offsetof(JSObject, slots)),
                                                               objReg);

    
    Address slot(objReg, 1 << 24);
    frame.pop();

    Label fastValueLoad = masm.loadValueWithAddressOffsetPatch(slot, shapeReg, objReg);
    pic.fastPathRejoin = masm.label();

    RETURN_IF_OOM(false);

    
    GetPropLabels &labels = pic.getPropLabels();
    labels.setDslotsLoad(masm, pic.fastPathRejoin, dslotsLoadLabel);
    labels.setInlineShapeData(masm, pic.shapeGuard, inlineShapeLabel);

    labels.setValueLoad(masm, pic.fastPathRejoin, fastValueLoad);
    if (pic.hasTypeCheck)
        labels.setInlineTypeJump(masm, pic.fastPathStart, typeCheck);
#ifdef JS_CPU_X64
    labels.setInlineShapeJump(masm, inlineShapeLabel, inlineShapeJump);
#else
    labels.setInlineShapeJump(masm, pic.shapeGuard, inlineShapeJump);
#endif

    pic.objReg = objReg;
    frame.pushRegs(shapeReg, objReg, knownType);

    stubcc.rejoin(Changes(1));
    pics.append(pic);
    return true;
}

bool
mjit::Compiler::jsop_callprop_generic(JSAtom *atom)
{
    FrameEntry *top = frame.peek(-1);

    




    RegisterID objReg = frame.copyDataIntoReg(top);
    RegisterID shapeReg = frame.allocReg();

    PICGenInfo pic(ic::PICInfo::CALL, JSOp(*PC), true);

    pic.pc = PC;

    
    pic.typeReg = frame.copyTypeIntoReg(top);

    RESERVE_IC_SPACE(masm);

    
    pic.fastPathStart = masm.label();

    





    Jump typeCheckJump = masm.testObject(Assembler::NotEqual, pic.typeReg);
    Label typeCheck = masm.label();
    RETURN_IF_OOM(false);

    pic.typeCheck = stubcc.linkExit(typeCheckJump, Uses(1));
    pic.hasTypeCheck = true;
    pic.objReg = objReg;
    pic.shapeReg = shapeReg;
    pic.atom = atom;

    



    uint32 thisvSlot = frame.totalDepth();
    Address thisv = Address(JSFrameReg, sizeof(StackFrame) + thisvSlot * sizeof(Value));

#if defined JS_NUNBOX32
    masm.storeValueFromComponents(pic.typeReg, pic.objReg, thisv);
#elif defined JS_PUNBOX64
    masm.orPtr(pic.objReg, pic.typeReg);
    masm.storePtr(pic.typeReg, thisv);
#endif

    frame.freeReg(pic.typeReg);

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();

    DataLabel32 inlineShapeLabel;
    Jump j = masm.branch32WithPatch(Assembler::NotEqual, shapeReg,
                           Imm32(int32(INVALID_SHAPE)),
                           inlineShapeLabel);
    Label inlineShapeJump = masm.label();

    
    RESERVE_OOL_SPACE(stubcc.masm);
    pic.slowPathStart = stubcc.linkExit(j, Uses(1));
    stubcc.leave();
    passICAddress(&pic);
    pic.slowPathCall = OOL_STUBCALL(ic::CallProp);
    CHECK_OOL_SPACE();

    
    frame.pop();
    frame.pushRegs(shapeReg, objReg, knownPushedType(0));
    pushSyncedEntry(1);

    
    Label dslotsLoadLabel = masm.loadPtrWithPatchToLEA(Address(objReg, offsetof(JSObject, slots)),
                                                               objReg);

    
    Address slot(objReg, 1 << 24);

    Label fastValueLoad = masm.loadValueWithAddressOffsetPatch(slot, shapeReg, objReg);
    pic.fastPathRejoin = masm.label();

    RETURN_IF_OOM(false);

    



    GetPropLabels &labels = pic.getPropLabels();
    labels.setDslotsLoadOffset(masm.differenceBetween(pic.fastPathRejoin, dslotsLoadLabel));
    labels.setInlineShapeOffset(masm.differenceBetween(pic.shapeGuard, inlineShapeLabel));
    labels.setValueLoad(masm, pic.fastPathRejoin, fastValueLoad);
    labels.setInlineTypeJump(masm, pic.fastPathStart, typeCheck);
#ifdef JS_CPU_X64
    labels.setInlineShapeJump(masm, inlineShapeLabel, inlineShapeJump);
#else
    labels.setInlineShapeJump(masm, pic.shapeGuard, inlineShapeJump);
#endif

    stubcc.rejoin(Changes(2));
    pics.append(pic);
    return true;
}

bool
mjit::Compiler::jsop_callprop_str(JSAtom *atom)
{
    if (!script->compileAndGo) {
        jsop_callprop_slow(atom);
        return true; 
    }

    





    JSObject *obj;
    if (!js_GetClassPrototype(cx, globalObj, JSProto_String, &obj))
        return false;

    
    RegisterID reg = frame.allocReg();

    masm.move(ImmPtr(obj), reg);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, reg);

    
    if (!jsop_getprop(atom, knownPushedType(0)))
        return false;

    
    frame.dup2();
    frame.shift(-3);
    frame.shift(-1);

    




    RegisterID strReg;
    FrameEntry *strFe = frame.peek(-1);
    if (strFe->isConstant()) {
        strReg = frame.allocReg();
        masm.move(ImmPtr(strFe->getValue().toString()), strReg);
    } else {
        strReg = frame.ownRegForData(strFe);
    }
    frame.pop();
    frame.pushTypedPayload(JSVAL_TYPE_STRING, strReg);
    frame.forgetType(frame.peek(-1));

    return true;
}

bool
mjit::Compiler::jsop_callprop_obj(JSAtom *atom)
{
    FrameEntry *top = frame.peek(-1);

    PICGenInfo pic(ic::PICInfo::CALL, JSOp(*PC), true);

    JS_ASSERT(top->isTypeKnown());
    JS_ASSERT(top->getKnownType() == JSVAL_TYPE_OBJECT);
    
    RESERVE_IC_SPACE(masm);

    pic.pc = PC;
    pic.fastPathStart = masm.label();
    pic.hasTypeCheck = false;
    pic.typeReg = Registers::ReturnReg;

    RegisterID shapeReg = frame.allocReg();
    pic.shapeReg = shapeReg;
    pic.atom = atom;

    RegisterID objReg;
    if (top->isConstant()) {
        objReg = frame.allocReg();
        masm.move(ImmPtr(&top->getValue().toObject()), objReg);
    } else {
        objReg = frame.copyDataIntoReg(top);
    }

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();

    DataLabel32 inlineShapeLabel;
    Jump j = masm.branch32WithPatch(Assembler::NotEqual, shapeReg,
                           Imm32(int32(INVALID_SHAPE)),
                           inlineShapeLabel);
    Label inlineShapeJump = masm.label();

    
    RESERVE_OOL_SPACE(stubcc.masm);
    pic.slowPathStart = stubcc.linkExit(j, Uses(1));
    stubcc.leave();
    passICAddress(&pic);
    pic.slowPathCall = OOL_STUBCALL(ic::CallProp);
    CHECK_OOL_SPACE();

    
    Label dslotsLoadLabel = masm.loadPtrWithPatchToLEA(Address(objReg, offsetof(JSObject, slots)),
                                                               objReg);

    
    Address slot(objReg, 1 << 24);

    Label fastValueLoad = masm.loadValueWithAddressOffsetPatch(slot, shapeReg, objReg);

    pic.fastPathRejoin = masm.label();
    pic.objReg = objReg;

    








    frame.dup();
    frame.pushRegs(shapeReg, objReg, knownPushedType(0));
    frame.shift(-2);

    



    RETURN_IF_OOM(false);

    GetPropLabels &labels = pic.getPropLabels();
    labels.setDslotsLoadOffset(masm.differenceBetween(pic.fastPathRejoin, dslotsLoadLabel));
    labels.setInlineShapeOffset(masm.differenceBetween(pic.shapeGuard, inlineShapeLabel));
    labels.setValueLoad(masm, pic.fastPathRejoin, fastValueLoad);
#ifdef JS_CPU_X64
    labels.setInlineShapeJump(masm, inlineShapeLabel, inlineShapeJump);
#else
    labels.setInlineShapeJump(masm, pic.shapeGuard, inlineShapeJump);
#endif

    stubcc.rejoin(Changes(2));
    pics.append(pic);

    return true;
}

bool
mjit::Compiler::testSingletonProperty(JSObject *obj, jsid id)
{
    












    if (!obj->isNative())
        return false;
    if (obj->getClass()->ops.lookupProperty)
        return false;

    JSObject *holder;
    JSProperty *prop = NULL;
    if (!obj->lookupProperty(cx, id, &holder, &prop))
        return false;
    if (!prop)
        return false;

    Shape *shape = (Shape *) prop;
    if (shape->hasDefaultGetter()) {
        if (!shape->hasSlot())
            return false;
        if (holder->getSlot(shape->slot).isUndefined())
            return false;
    } else if (!shape->isMethod()) {
        return false;
    }

    return true;
}

bool
mjit::Compiler::testSingletonPropertyTypes(FrameEntry *top, jsid id, bool *testObject)
{
    *testObject = false;

    types::TypeSet *types = frame.extra(top).types;
    if (!types)
        return false;

    JSObject *singleton = types->getSingleton(cx);
    if (singleton)
        return testSingletonProperty(singleton, id);

    if (!script->compileAndGo)
        return false;

    JSProtoKey key;
    JSValueType type = types->getKnownTypeTag(cx);
    switch (type) {
      case JSVAL_TYPE_STRING:
        key = JSProto_String;
        break;

      case JSVAL_TYPE_INT32:
      case JSVAL_TYPE_DOUBLE:
        key = JSProto_Number;
        break;

      case JSVAL_TYPE_BOOLEAN:
        key = JSProto_Boolean;
        break;

      case JSVAL_TYPE_OBJECT:
      case JSVAL_TYPE_UNKNOWN:
        if (types->getObjectCount() == 1 && !top->isNotType(JSVAL_TYPE_OBJECT)) {
            JS_ASSERT_IF(top->isTypeKnown(), top->isType(JSVAL_TYPE_OBJECT));
            types::TypeObject *object = types->getObject(0);
            if (object->proto) {
                if (!testSingletonProperty(object->proto, id))
                    return false;

                
                *testObject = (type != JSVAL_TYPE_OBJECT) && !top->isTypeKnown();
                return true;
            }
        }
        return false;

      default:
        return false;
    }

    JSObject *proto;
    if (!js_GetClassPrototype(cx, globalObj, key, &proto, NULL))
        return NULL;

    return testSingletonProperty(proto, id);
}

bool
mjit::Compiler::jsop_callprop(JSAtom *atom)
{
    REJOIN_SITE_2(stubs::CallProp, ic::CallProp);

    FrameEntry *top = frame.peek(-1);

    bool testObject;
    JSObject *singleton = pushedSingleton(0);
    if (singleton && singleton->isFunction() &&
        testSingletonPropertyTypes(top, ATOM_TO_JSID(atom), &testObject)) {
        if (testObject) {
            Jump notObject = frame.testObject(Assembler::NotEqual, top);
            stubcc.linkExit(notObject, Uses(1));
            stubcc.leave();
            stubcc.masm.move(ImmPtr(atom), Registers::ArgReg1);
            OOL_STUBCALL(stubs::CallProp);
        }

        

        frame.dup();
        

        frame.push(ObjectValue(*singleton));
        

        frame.shift(-2);
        

        if (testObject)
            stubcc.rejoin(Changes(2));

        return true;
    }

    
    if (top->isTypeKnown() && top->getKnownType() != JSVAL_TYPE_OBJECT) {
        if (top->getKnownType() == JSVAL_TYPE_STRING)
            return jsop_callprop_str(atom);
        return jsop_callprop_slow(atom);
    }

    if (top->isTypeKnown())
        return jsop_callprop_obj(atom);
    return jsop_callprop_generic(atom);
}

bool
mjit::Compiler::jsop_setprop(JSAtom *atom, bool usePropCache, bool popGuaranteed)
{
    REJOIN_SITE_2(usePropCache
                  ? STRICT_VARIANT(stubs::SetName)
                  : STRICT_VARIANT(stubs::SetPropNoCache),
                  ic::SetProp);

    FrameEntry *lhs = frame.peek(-2);
    FrameEntry *rhs = frame.peek(-1);

    
    if (lhs->isTypeKnown() && lhs->getKnownType() != JSVAL_TYPE_OBJECT) {
        jsop_setprop_slow(atom, usePropCache);
        return true;
    }

    



    types::TypeSet *types = frame.extra(lhs).types;
    if (JSOp(*PC) == JSOP_SETPROP &&
        types && !types->unknown() && types->getObjectCount() == 1 &&
        !types->getObject(0)->unknownProperties()) {
        JS_ASSERT(usePropCache);
        types::TypeObject *object = types->getObject(0);
        types::TypeSet *propertyTypes = object->getProperty(cx, ATOM_TO_JSID(atom), false);
        if (!propertyTypes)
            return false;
        if (propertyTypes->isDefiniteProperty() && !propertyTypes->isOwnProperty(cx, true)) {
            types->addFreeze(cx);
            uint32 slot = propertyTypes->definiteSlot();
            bool isObject = lhs->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, lhs);
                stubcc.linkExit(notObject, Uses(2));
                stubcc.leave();
                stubcc.masm.move(ImmPtr(atom), Registers::ArgReg1);
                OOL_STUBCALL(STRICT_VARIANT(stubs::SetName));
            }
            RegisterID reg = frame.tempRegForData(lhs);
            frame.storeTo(rhs, Address(reg, JSObject::getFixedSlotOffset(slot)), popGuaranteed);
            frame.shimmy(1);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            return true;
        }
    }

    JSOp op = JSOp(*PC);

    ic::PICInfo::Kind kind = (op == JSOP_SETMETHOD)
                             ? ic::PICInfo::SETMETHOD
                             : ic::PICInfo::SET;
    PICGenInfo pic(kind, op, usePropCache);
    pic.atom = atom;

    if (monitored(PC)) {
        types::TypeSet *types = frame.extra(rhs).types;
        pic.typeMonitored = true;
        pic.rhsTypes = (types::ClonedTypeSet *) ::js_calloc(sizeof(types::ClonedTypeSet));
        if (!pic.rhsTypes) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        types::TypeSet::Clone(cx, types, pic.rhsTypes);
    } else {
        pic.typeMonitored = false;
        pic.rhsTypes = NULL;
    }

    RESERVE_IC_SPACE(masm);
    RESERVE_OOL_SPACE(stubcc.masm);

    
    Jump typeCheck;
    if (!lhs->isTypeKnown()) {
        RegisterID reg = frame.tempRegForType(lhs);
        pic.typeReg = reg;

        
        pic.fastPathStart = masm.label();
        Jump j = masm.testObject(Assembler::NotEqual, reg);

        pic.typeCheck = stubcc.linkExit(j, Uses(2));
        stubcc.leave();

        stubcc.masm.move(ImmPtr(atom), Registers::ArgReg1);
        if (usePropCache)
            OOL_STUBCALL(STRICT_VARIANT(stubs::SetName));
        else
            OOL_STUBCALL(STRICT_VARIANT(stubs::SetPropNoCache));
        typeCheck = stubcc.masm.jump();
        pic.hasTypeCheck = true;
    } else {
        pic.fastPathStart = masm.label();
        pic.hasTypeCheck = false;
        pic.typeReg = Registers::ReturnReg;
    }

    frame.forgetMismatchedObject(lhs);

    
    RegisterID objReg = frame.copyDataIntoReg(lhs);
    pic.objReg = objReg;

    
    ValueRemat vr;
    frame.pinEntry(rhs, vr);
    pic.vr = vr;

    RegisterID shapeReg = frame.allocReg();
    pic.shapeReg = shapeReg;

    frame.unpinEntry(vr);

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();
    DataLabel32 inlineShapeData;
    Jump j = masm.branch32WithPatch(Assembler::NotEqual, shapeReg,
                                    Imm32(int32(INVALID_SHAPE)),
                                    inlineShapeData);
    Label afterInlineShapeJump = masm.label();

    
    {
        pic.slowPathStart = stubcc.linkExit(j, Uses(2));

        stubcc.leave();
        passICAddress(&pic);
        pic.slowPathCall = OOL_STUBCALL(ic::SetProp);
        CHECK_OOL_SPACE();
    }

    
    Label dslotsLoadLabel = masm.loadPtrWithPatchToLEA(Address(objReg, offsetof(JSObject, slots)),
                                                       objReg);

    
    Address slot(objReg, 1 << 24);
    DataLabel32 inlineValueStore = masm.storeValueWithAddressOffsetPatch(vr, slot);
    pic.fastPathRejoin = masm.label();

    frame.freeReg(objReg);
    frame.freeReg(shapeReg);

    
    frame.shimmy(1);

    
    {
        if (pic.hasTypeCheck)
            typeCheck.linkTo(stubcc.masm.label(), &stubcc.masm);
        stubcc.rejoin(Changes(1));
    }

    RETURN_IF_OOM(false);

    SetPropLabels &labels = pic.setPropLabels();
    labels.setInlineShapeData(masm, pic.shapeGuard, inlineShapeData);
    labels.setDslotsLoad(masm, pic.fastPathRejoin, dslotsLoadLabel, vr);
    labels.setInlineValueStore(masm, pic.fastPathRejoin, inlineValueStore, vr);
    labels.setInlineShapeJump(masm, pic.shapeGuard, afterInlineShapeJump);

    pics.append(pic);
    return true;
}

void
mjit::Compiler::jsop_name(JSAtom *atom, JSValueType type)
{
    REJOIN_SITE_2(ic::Name, stubs::UndefinedHelper);

    PICGenInfo pic(ic::PICInfo::NAME, JSOp(*PC), true);

    RESERVE_IC_SPACE(masm);

    pic.shapeReg = frame.allocReg();
    pic.objReg = frame.allocReg();
    pic.typeReg = Registers::ReturnReg;
    pic.atom = atom;
    pic.hasTypeCheck = false;
    pic.fastPathStart = masm.label();

    
    pic.shapeGuard = masm.label();
    Jump inlineJump = masm.jump();
    {
        RESERVE_OOL_SPACE(stubcc.masm);
        pic.slowPathStart = stubcc.linkExit(inlineJump, Uses(0));
        stubcc.leave();
        passICAddress(&pic);
        pic.slowPathCall = OOL_STUBCALL(ic::Name);
        CHECK_OOL_SPACE();
    }
    pic.fastPathRejoin = masm.label();

    
    ScopeNameLabels &labels = pic.scopeNameLabels();
    labels.setInlineJump(masm, pic.fastPathStart, inlineJump);

    MaybeJump undefinedGuard;
    if (cx->typeInferenceEnabled()) {
        
        undefinedGuard = masm.testUndefined(Assembler::Equal, pic.shapeReg);
    }

    




    JSObject *singleton = pushedSingleton(0);
    if (singleton) {
        frame.push(ObjectValue(*singleton));
        frame.freeReg(pic.shapeReg);
        frame.freeReg(pic.objReg);
    } else {
        frame.pushRegs(pic.shapeReg, pic.objReg, type);
    }

    stubcc.rejoin(Changes(1));

    if (cx->typeInferenceEnabled()) {
        stubcc.linkExit(undefinedGuard.get(), Uses(0));
        stubcc.leave();
        OOL_STUBCALL(stubs::UndefinedHelper);
        stubcc.rejoin(Changes(1));
    }

    pics.append(pic);
}

bool
mjit::Compiler::jsop_xname(JSAtom *atom)
{
    REJOIN_SITE_ANY();
    PICGenInfo pic(ic::PICInfo::XNAME, JSOp(*PC), true);

    FrameEntry *fe = frame.peek(-1);
    if (fe->isNotType(JSVAL_TYPE_OBJECT)) {
        return jsop_getprop(atom, knownPushedType(0));
    }

    if (!fe->isTypeKnown()) {
        Jump notObject = frame.testObject(Assembler::NotEqual, fe);
        stubcc.linkExit(notObject, Uses(1));
    }

    frame.forgetMismatchedObject(fe);

    RESERVE_IC_SPACE(masm);

    pic.shapeReg = frame.allocReg();
    pic.objReg = frame.copyDataIntoReg(fe);
    pic.typeReg = Registers::ReturnReg;
    pic.atom = atom;
    pic.hasTypeCheck = false;
    pic.fastPathStart = masm.label();

    
    pic.shapeGuard = masm.label();
    Jump inlineJump = masm.jump();
    {
        RESERVE_OOL_SPACE(stubcc.masm);
        pic.slowPathStart = stubcc.linkExit(inlineJump, Uses(1));
        stubcc.leave();
        passICAddress(&pic);
        pic.slowPathCall = OOL_STUBCALL(ic::XName);
        CHECK_OOL_SPACE();
    }

    pic.fastPathRejoin = masm.label();

    RETURN_IF_OOM(false);

    
    ScopeNameLabels &labels = pic.scopeNameLabels();
    labels.setInlineJumpOffset(masm.differenceBetween(pic.fastPathStart, inlineJump));

    frame.pop();
    frame.pushRegs(pic.shapeReg, pic.objReg, knownPushedType(0));

    MaybeJump undefinedGuard;
    if (cx->typeInferenceEnabled()) {
        
        undefinedGuard = masm.testUndefined(Assembler::Equal, pic.shapeReg);
    }

    stubcc.rejoin(Changes(1));

    if (cx->typeInferenceEnabled()) {
        stubcc.linkExit(undefinedGuard.get(), Uses(0));
        stubcc.leave();
        OOL_STUBCALL(stubs::UndefinedHelper);
        stubcc.rejoin(Changes(1));
    }

    pics.append(pic);
    return true;
}

void
mjit::Compiler::jsop_bindname(JSAtom *atom, bool usePropCache)
{
    REJOIN_SITE(ic::BindName);
    PICGenInfo pic(ic::PICInfo::BIND, JSOp(*PC), usePropCache);

    
    
    
    
    JS_ASSERT(analysis->usesScopeChain());

    pic.shapeReg = frame.allocReg();
    pic.objReg = frame.allocReg();
    pic.typeReg = Registers::ReturnReg;
    pic.atom = atom;
    pic.hasTypeCheck = false;

    RESERVE_IC_SPACE(masm);
    pic.fastPathStart = masm.label();

    Address parent(pic.objReg, offsetof(JSObject, parent));
    masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);

    pic.shapeGuard = masm.label();
    Jump inlineJump = masm.branchPtr(Assembler::NotEqual, parent, ImmPtr(0));
    {
        RESERVE_OOL_SPACE(stubcc.masm);
        pic.slowPathStart = stubcc.linkExit(inlineJump, Uses(0));
        stubcc.leave();
        passICAddress(&pic);
        pic.slowPathCall = OOL_STUBCALL(ic::BindName);
        CHECK_OOL_SPACE();
    }

    pic.fastPathRejoin = masm.label();

    
    BindNameLabels &labels = pic.bindNameLabels();
    labels.setInlineJump(masm, pic.shapeGuard, inlineJump);

    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, pic.objReg);
    frame.freeReg(pic.shapeReg);

    stubcc.rejoin(Changes(1));

    pics.append(pic);
}

#else 

void
mjit::Compiler::jsop_name(JSAtom *atom, JSValueType type, types::TypeSet *typeSet)
{
    REJOIN_SITE(stubs::Name);
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(stubs::Name);
    frame.pushSynced(type, typeSet);
}

bool
mjit::Compiler::jsop_xname(JSAtom *atom)
{
    return jsop_getprop(atom, knownPushedType(0), pushedTypeSet(0));
}

bool
mjit::Compiler::jsop_getprop(JSAtom *atom, JSValueType knownType, types::TypeSet *typeSet,
                             bool typecheck, bool usePropCache)
{
    REJOIN_SITE_2(ic::GetProp, ic::GetPropNoCache);
    jsop_getprop_slow(atom, usePropCache);
    return true;
}

bool
mjit::Compiler::jsop_callprop(JSAtom *atom)
{
    REJOIN_SITE_2(stubs::CallProp);
    return jsop_callprop_slow(atom);
}

bool
mjit::Compiler::jsop_setprop(JSAtom *atom, bool usePropCache)
{
    REJOIN_SITE(usePropCache
                ? STRICT_VARIANT(stubs::SetName)
                : STRICT_VARIANT(stubs::SetPropNoCache));

    jsop_setprop_slow(atom, usePropCache);
    return true;
}

void
mjit::Compiler::jsop_bindname(JSAtom *atom, bool usePropCache)
{
    REJOIN_SITE_2(stubs::BindName, stubs::BindNameNoCache);
    RegisterID reg = frame.allocReg();
    Address scopeChain(JSFrameReg, StackFrame::offsetOfScopeChain());
    masm.loadPtr(scopeChain, reg);

    Address address(reg, offsetof(JSObject, parent));

    Jump j = masm.branchPtr(Assembler::NotEqual, address, ImmPtr(0));

    stubcc.linkExit(j, Uses(0));
    stubcc.leave();
    if (usePropCache) {
        OOL_STUBCALL(stubs::BindName);
    } else {
        stubcc.masm.move(ImmPtr(atom), Registers::ArgReg1);
        OOL_STUBCALL(stubs::BindNameNoCache);
    }

    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, reg);

    stubcc.rejoin(Changes(1));
}
#endif

void
mjit::Compiler::jsop_this()
{
    REJOIN_SITE(stubs::This);

    frame.pushThis();

    




    if (script->fun && !script->strictModeCode) {
        FrameEntry *thisFe = frame.peek(-1);
        if (!thisFe->isTypeKnown()) {
            JSValueType type = cx->typeInferenceEnabled()
                ? script->thisTypes()->getKnownTypeTag(cx)
                : JSVAL_TYPE_UNKNOWN;
            if (type != JSVAL_TYPE_OBJECT) {
                Jump notObj = frame.testObject(Assembler::NotEqual, thisFe);
                stubcc.linkExit(notObj, Uses(1));
                stubcc.leave();
                OOL_STUBCALL(stubs::This);
                stubcc.rejoin(Changes(1));
            }

            
            frame.pop();
            frame.learnThisIsObject(type != JSVAL_TYPE_OBJECT);
            frame.pushThis();
        }

        JS_ASSERT(thisFe->isType(JSVAL_TYPE_OBJECT));
    }
}

bool
mjit::Compiler::jsop_gnameinc(JSOp op, VoidStubAtom stub, uint32 index)
{
    JSAtom *atom = script->getAtom(index);

#if defined JS_MONOIC
    jsbytecode *next = &PC[JSOP_GNAMEINC_LENGTH];
    bool pop = (JSOp(*next) == JSOP_POP) && !analysis->jumpTarget(next);
    int amt = (op == JSOP_GNAMEINC || op == JSOP_INCGNAME) ? -1 : 1;

    if (pop || (op == JSOP_INCGNAME || op == JSOP_DECGNAME)) {
        

        jsop_getgname(index);
        

        frame.push(Int32Value(amt));
        

        
        if (!jsop_binary(JSOP_SUB, stubs::Sub, knownPushedType(0), pushedTypeSet(0)))
            return false;
        

        jsop_bindgname();
        

        frame.dup2();
        

        frame.shift(-3);
        

        frame.shift(-1);
        

        jsop_setgname(atom, false, pop);
        

        if (pop)
            frame.pop();
    } else {
        

        jsop_getgname(index);
        

        jsop_pos();
        

        frame.dup();
        

        frame.push(Int32Value(-amt));
        

        if (!jsop_binary(JSOP_ADD, stubs::Add, knownPushedType(0), pushedTypeSet(0)))
            return false;
        

        jsop_bindgname();
        

        frame.dup2();
        

        frame.shift(-3);
        

        frame.shift(-1);
        

        jsop_setgname(atom, false, true);
        

        frame.pop();
        
    }

    if (pop)
        PC += JSOP_POP_LENGTH;
#else
    prepareStubCall(Uses(0));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    INLINE_STUBCALL(stub);
    frame.pushSynced(knownPushedType(0));
#endif

    PC += JSOP_GNAMEINC_LENGTH;
    return true;
}

CompileStatus
mjit::Compiler::jsop_nameinc(JSOp op, VoidStubAtom stub, uint32 index)
{
    JSAtom *atom = script->getAtom(index);
#if defined JS_POLYIC
    jsbytecode *next = &PC[JSOP_NAMEINC_LENGTH];
    bool pop = (JSOp(*next) == JSOP_POP) && !analysis->jumpTarget(next);
    int amt = (op == JSOP_NAMEINC || op == JSOP_INCNAME) ? -1 : 1;

    if (pop || (op == JSOP_INCNAME || op == JSOP_DECNAME)) {
        

        jsop_bindname(atom, false);
        

        jsop_name(atom, JSVAL_TYPE_UNKNOWN);
        

        frame.push(Int32Value(amt));
        

        
        frame.syncAt(-3);
        if (!jsop_binary(JSOP_SUB, stubs::Sub, JSVAL_TYPE_UNKNOWN, pushedTypeSet(0)))
            return Compile_Retry;
        

        if (!jsop_setprop(atom, false, pop))
            return Compile_Error;
        

        if (pop)
            frame.pop();
    } else {
        

        jsop_name(atom, JSVAL_TYPE_UNKNOWN);
        

        jsop_pos();
        

        jsop_bindname(atom, false);
        

        frame.dupAt(-2);
        

        frame.push(Int32Value(-amt));
        

        frame.syncAt(-3);
        if (!jsop_binary(JSOP_ADD, stubs::Add, JSVAL_TYPE_UNKNOWN, pushedTypeSet(0)))
            return Compile_Retry;
        

        if (!jsop_setprop(atom, false, true))
            return Compile_Error;
        

        frame.pop();
        
    }

    if (pop)
        PC += JSOP_POP_LENGTH;
#else
    prepareStubCall(Uses(0));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    INLINE_STUBCALL(stub);
    frame.pushSynced(knownPushedType(0));
#endif

    PC += JSOP_NAMEINC_LENGTH;
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::jsop_propinc(JSOp op, VoidStubAtom stub, uint32 index)
{
    JSAtom *atom = script->getAtom(index);
#if defined JS_POLYIC
    jsbytecode *next = &PC[JSOP_PROPINC_LENGTH];
    bool pop = (JSOp(*next) == JSOP_POP) && !analysis->jumpTarget(next);
    int amt = (op == JSOP_PROPINC || op == JSOP_INCPROP) ? -1 : 1;

    if (pop || (op == JSOP_INCPROP || op == JSOP_DECPROP)) {
        






        frame.dup();
        

        frame.dup();
        

        if (!jsop_getprop(atom, JSVAL_TYPE_UNKNOWN))
            return Compile_Error;
        

        frame.push(Int32Value(amt));
        

        
        frame.syncAt(-4);
        if (!jsop_binary(JSOP_SUB, stubs::Sub, JSVAL_TYPE_UNKNOWN, pushedTypeSet(0)))
            return Compile_Retry;
        

        frame.shimmy(1);
        

        if (!jsop_setprop(atom, false, pop))
            return Compile_Error;
        

        if (pop)
            frame.pop();
    } else {
        

        frame.dup();
        

        if (!jsop_getprop(atom, JSVAL_TYPE_UNKNOWN))
            return Compile_Error;
        

        jsop_pos();
        

        frame.dup();
        

        frame.push(Int32Value(-amt));
        

        frame.syncAt(-4);
        if (!jsop_binary(JSOP_ADD, stubs::Add, JSVAL_TYPE_UNKNOWN, pushedTypeSet(0)))
            return Compile_Retry;
        

        frame.dupAt(-3);
        

        frame.dupAt(-2);
        

        if (!jsop_setprop(atom, false, true))
            return Compile_Error;
        

        frame.popn(2);
        

        frame.shimmy(1);
        
    }
    if (pop)
        PC += JSOP_POP_LENGTH;
#else
    prepareStubCall(Uses(1));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    INLINE_STUBCALL(stub);
    frame.pop();
    pushSyncedEntry(0);
#endif

    PC += JSOP_PROPINC_LENGTH;
    return Compile_Okay;
}

bool
mjit::Compiler::iter(uintN flags)
{
    REJOIN_SITE_ANY();
    FrameEntry *fe = frame.peek(-1);

    



    if ((flags != JSITER_ENUMERATE) || fe->isNotType(JSVAL_TYPE_OBJECT)) {
        prepareStubCall(Uses(1));
        masm.move(Imm32(flags), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::Iter);
        frame.pop();
        frame.pushSynced(JSVAL_TYPE_UNKNOWN);
        return true;
    }

    if (!fe->isTypeKnown()) {
        Jump notObject = frame.testObject(Assembler::NotEqual, fe);
        stubcc.linkExit(notObject, Uses(1));
    }

    frame.forgetMismatchedObject(fe);

    RegisterID reg = frame.tempRegForData(fe);

    frame.pinReg(reg);
    RegisterID ioreg = frame.allocReg();  
    RegisterID nireg = frame.allocReg();  
    RegisterID T1 = frame.allocReg();
    RegisterID T2 = frame.allocReg();
    frame.unpinReg(reg);

    
    masm.loadPtr(&script->compartment->nativeIterCache.last, ioreg);

    
    Jump nullIterator = masm.branchTest32(Assembler::Zero, ioreg, ioreg);
    stubcc.linkExit(nullIterator, Uses(1));

    
    masm.loadObjPrivate(ioreg, nireg);

    
    Address flagsAddr(nireg, offsetof(NativeIterator, flags));
    masm.load32(flagsAddr, T1);
    Jump activeIterator = masm.branchTest32(Assembler::NonZero, T1,
                                            Imm32(JSITER_ACTIVE|JSITER_UNREUSABLE));
    stubcc.linkExit(activeIterator, Uses(1));

    
    masm.loadShape(reg, T1);
    masm.loadPtr(Address(nireg, offsetof(NativeIterator, shapes_array)), T2);
    masm.load32(Address(T2, 0), T2);
    Jump mismatchedObject = masm.branch32(Assembler::NotEqual, T1, T2);
    stubcc.linkExit(mismatchedObject, Uses(1));

    
    masm.loadPtr(Address(reg, offsetof(JSObject, type)), T1);
    masm.loadPtr(Address(T1, offsetof(types::TypeObject, proto)), T1);
    masm.loadShape(T1, T1);
    masm.loadPtr(Address(nireg, offsetof(NativeIterator, shapes_array)), T2);
    masm.load32(Address(T2, sizeof(uint32)), T2);
    Jump mismatchedProto = masm.branch32(Assembler::NotEqual, T1, T2);
    stubcc.linkExit(mismatchedProto, Uses(1));

    





    masm.loadPtr(Address(reg, offsetof(JSObject, type)), T1);
    masm.loadPtr(Address(T1, offsetof(types::TypeObject, proto)), T1);
    masm.loadPtr(Address(T1, offsetof(JSObject, type)), T1);
    masm.loadPtr(Address(T1, offsetof(types::TypeObject, proto)), T1);
    Jump overlongChain = masm.branchPtr(Assembler::NonZero, T1, T1);
    stubcc.linkExit(overlongChain, Uses(1));

    

    
    masm.storePtr(reg, Address(nireg, offsetof(NativeIterator, obj)));
    masm.load32(flagsAddr, T1);
    masm.or32(Imm32(JSITER_ACTIVE), T1);
    masm.store32(T1, flagsAddr);

    
    masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), T1);
    masm.loadPtr(Address(T1, offsetof(JSContext, enumerators)), T2);
    masm.storePtr(T2, Address(nireg, offsetof(NativeIterator, next)));
    masm.storePtr(ioreg, Address(T1, offsetof(JSContext, enumerators)));

    frame.freeReg(nireg);
    frame.freeReg(T1);
    frame.freeReg(T2);

    stubcc.leave();
    stubcc.masm.move(Imm32(flags), Registers::ArgReg1);
    OOL_STUBCALL(stubs::Iter);

    
    frame.pop();
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, ioreg);

    stubcc.rejoin(Changes(1));

    return true;
}





void
mjit::Compiler::iterNext()
{
    REJOIN_SITE(stubs::IterNext);

    FrameEntry *fe = frame.peek(-1);
    RegisterID reg = frame.tempRegForData(fe);

    
    frame.pinReg(reg);
    RegisterID T1 = frame.allocReg();
    frame.unpinReg(reg);

    
    Jump notFast = masm.testObjClass(Assembler::NotEqual, reg, &js_IteratorClass);
    stubcc.linkExit(notFast, Uses(1));

    
    masm.loadObjPrivate(reg, T1);

    RegisterID T3 = frame.allocReg();
    RegisterID T4 = frame.allocReg();

    
    masm.load32(Address(T1, offsetof(NativeIterator, flags)), T3);
    notFast = masm.branchTest32(Assembler::NonZero, T3, Imm32(JSITER_FOREACH));
    stubcc.linkExit(notFast, Uses(1));

    RegisterID T2 = frame.allocReg();

    
    masm.loadPtr(Address(T1, offsetof(NativeIterator, props_cursor)), T2);

    
    masm.loadPtr(T2, T3);
    masm.move(T3, T4);
    masm.andPtr(Imm32(JSID_TYPE_MASK), T4);
    notFast = masm.branchTestPtr(Assembler::NonZero, T4, T4);
    stubcc.linkExit(notFast, Uses(1));

    
    masm.addPtr(Imm32(sizeof(jsid)), T2, T4);
    masm.storePtr(T4, Address(T1, offsetof(NativeIterator, props_cursor)));

    frame.freeReg(T4);
    frame.freeReg(T1);
    frame.freeReg(T2);

    stubcc.leave();
    OOL_STUBCALL(stubs::IterNext);

    frame.pushUntypedPayload(JSVAL_TYPE_STRING, T3);

    
    stubcc.rejoin(Changes(1));
}

bool
mjit::Compiler::iterMore()
{
    AutoRejoinSite autoRejoin(this, JS_FUNC_TO_DATA_PTR(void *, stubs::IterMore));

    jsbytecode *target = &PC[JSOP_MOREITER_LENGTH];
    JSOp next = JSOp(*target);
    JS_ASSERT(next == JSOP_IFNE || next == JSOP_IFNEX);

    target += (next == JSOP_IFNE)
              ? GET_JUMP_OFFSET(target)
              : GET_JUMPX_OFFSET(target);

    fixDoubleTypes(target);
    if (!frame.syncForBranch(target, Uses(1)))
        return false;

    FrameEntry *fe = frame.peek(-1);
    RegisterID reg = frame.tempRegForData(fe);
    RegisterID tempreg = frame.allocReg();

    
    Jump notFast = masm.testObjClass(Assembler::NotEqual, reg, &js_IteratorClass);
    stubcc.linkExitForBranch(notFast);

    
    masm.loadObjPrivate(reg, reg);

    
    notFast = masm.branchTest32(Assembler::NonZero, Address(reg, offsetof(NativeIterator, flags)),
                                Imm32(JSITER_FOREACH));
    stubcc.linkExitForBranch(notFast);

    
    masm.loadPtr(Address(reg, offsetof(NativeIterator, props_cursor)), tempreg);
    masm.loadPtr(Address(reg, offsetof(NativeIterator, props_end)), reg);

    Jump jFast = masm.branchPtr(Assembler::LessThan, tempreg, reg);

    stubcc.leave();
    OOL_STUBCALL(stubs::IterMore);
    autoRejoin.oolRejoin(stubcc.masm.label());
    Jump j = stubcc.masm.branchTest32(Assembler::NonZero, Registers::ReturnReg,
                                      Registers::ReturnReg);

    stubcc.rejoin(Changes(1));
    frame.freeReg(tempreg);

    return jumpAndTrace(jFast, target, &j);
}

void
mjit::Compiler::iterEnd()
{
    REJOIN_SITE_ANY();
    FrameEntry *fe= frame.peek(-1);
    RegisterID reg = frame.tempRegForData(fe);

    frame.pinReg(reg);
    RegisterID T1 = frame.allocReg();
    frame.unpinReg(reg);

    
    Jump notIterator = masm.testObjClass(Assembler::NotEqual, reg, &js_IteratorClass);
    stubcc.linkExit(notIterator, Uses(1));

    
    masm.loadObjPrivate(reg, T1);

    RegisterID T2 = frame.allocReg();

    
    Address flagAddr(T1, offsetof(NativeIterator, flags));
    masm.loadPtr(flagAddr, T2);

    
    Jump notEnumerate = masm.branchTest32(Assembler::Zero, T2, Imm32(JSITER_ENUMERATE));
    stubcc.linkExit(notEnumerate, Uses(1));

    
    masm.and32(Imm32(~JSITER_ACTIVE), T2);
    masm.storePtr(T2, flagAddr);

    
    masm.loadPtr(Address(T1, offsetof(NativeIterator, props_array)), T2);
    masm.storePtr(T2, Address(T1, offsetof(NativeIterator, props_cursor)));

    
    masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), T2);
    masm.loadPtr(Address(T1, offsetof(NativeIterator, next)), T1);
    masm.storePtr(T1, Address(T2, offsetof(JSContext, enumerators)));

    frame.freeReg(T1);
    frame.freeReg(T2);

    stubcc.leave();
    OOL_STUBCALL(stubs::EndIter);

    frame.pop();

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_eleminc(JSOp op, VoidStub stub)
{
    REJOIN_SITE_ANY();
    prepareStubCall(Uses(2));
    INLINE_STUBCALL(stub);
    frame.popn(2);
    pushSyncedEntry(0);
}

void
mjit::Compiler::jsop_getgname_slow(uint32 index)
{
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(stubs::GetGlobalName);
    frame.pushSynced(JSVAL_TYPE_UNKNOWN);
}

void
mjit::Compiler::jsop_bindgname()
{
    REJOIN_SITE(stubs::BindGlobalName);

    if (script->compileAndGo && globalObj) {
        frame.push(ObjectValue(*globalObj));
        return;
    }

    
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(stubs::BindGlobalName);
    frame.takeReg(Registers::ReturnReg);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
}

void
mjit::Compiler::jsop_getgname(uint32 index)
{
    REJOIN_SITE_2(ic::GetGlobalName, stubs::GetGlobalName);

    
    JSAtom *atom = script->getAtom(index);
    if (atom == cx->runtime->atomState.typeAtoms[JSTYPE_VOID]) {
        frame.push(UndefinedValue());
        return;
    }
    if (atom == cx->runtime->atomState.NaNAtom) {
        frame.push(cx->runtime->NaNValue);
        return;
    }
    if (atom == cx->runtime->atomState.InfinityAtom) {
        frame.push(cx->runtime->positiveInfinityValue);
        return;
    }

    
    JSObject *obj = pushedSingleton(0);
    if (obj && testSingletonProperty(globalObj, ATOM_TO_JSID(atom))) {
        frame.push(ObjectValue(*obj));
        return;
    }

    



    JSValueType type = JSVAL_TYPE_UNKNOWN;
    if (cx->typeInferenceEnabled() && globalObj->isGlobal() &&
        !globalObj->getType()->unknownProperties()) {
        types::TypeSet *types = globalObj->getType()->getProperty(cx, ATOM_TO_JSID(atom), false);
        if (!types)
            return;
        type = types->getKnownTypeTag(cx);

        const js::Shape *shape = globalObj->nativeLookup(ATOM_TO_JSID(atom));
        if (shape && shape->hasDefaultGetterOrIsMethod() && shape->hasSlot()) {
            Value *value = &globalObj->getSlotRef(shape->slot);
            if (!value->isUndefined() && !types->isOwnProperty(cx, true)) {
                watchGlobalReallocation();
                RegisterID reg = frame.allocReg();
                masm.move(ImmPtr(value), reg);
                frame.push(Address(reg), type, true);
                return;
            }
        }
        if (knownPushedType(0) != type)
            type = JSVAL_TYPE_UNKNOWN;
    }

#if defined JS_MONOIC
    jsop_bindgname();

    FrameEntry *fe = frame.peek(-1);
    JS_ASSERT(fe->isTypeKnown() && fe->getKnownType() == JSVAL_TYPE_OBJECT);

    GetGlobalNameICInfo ic;
    RESERVE_IC_SPACE(masm);
    RegisterID objReg;
    Jump shapeGuard;

    ic.usePropertyCache = true;

    ic.fastPathStart = masm.label();
    if (fe->isConstant()) {
        JSObject *obj = &fe->getValue().toObject();
        frame.pop();
        JS_ASSERT(obj->isNative());

        objReg = frame.allocReg();

        masm.load32FromImm(&obj->objShape, objReg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, objReg,
                                            Imm32(int32(INVALID_SHAPE)), ic.shape);
        masm.move(ImmPtr(obj), objReg);
    } else {
        objReg = frame.ownRegForData(fe);
        frame.pop();
        RegisterID reg = frame.allocReg();

        masm.loadShape(objReg, reg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, reg,
                                            Imm32(int32(INVALID_SHAPE)), ic.shape);
        frame.freeReg(reg);
    }
    stubcc.linkExit(shapeGuard, Uses(0));

    stubcc.leave();
    passMICAddress(ic);
    ic.slowPathCall = OOL_STUBCALL(ic::GetGlobalName);

    
    uint32 slot = 1 << 24;

    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);
    Address address(objReg, slot);
    
    
    RegisterID treg = frame.allocReg();
    
    RegisterID dreg = objReg;

    ic.load = masm.loadValueWithAddressOffsetPatch(address, treg, dreg);

    frame.pushRegs(treg, dreg, type);

    stubcc.rejoin(Changes(1));

    getGlobalNames.append(ic);

#else
    jsop_getgname_slow(index);
#endif

    




}





void
mjit::Compiler::jsop_callgname_epilogue()
{
    


    if (!script->compileAndGo) {
        prepareStubCall(Uses(1));
        INLINE_STUBCALL_NO_REJOIN(stubs::PushImplicitThisForGlobal);
        frame.pushSynced(JSVAL_TYPE_UNKNOWN);
        return;
    }

    
    FrameEntry *fval = frame.peek(-1);
    if (fval->isNotType(JSVAL_TYPE_OBJECT)) {
        frame.push(UndefinedValue());
        return;
    }

    
    if (fval->isConstant()) {
        JSObject *obj = &fval->getValue().toObject();
        if (obj->getParent() == globalObj) {
            frame.push(UndefinedValue());
        } else {
            prepareStubCall(Uses(1));
            INLINE_STUBCALL_NO_REJOIN(stubs::PushImplicitThisForGlobal);
            frame.pushSynced(JSVAL_TYPE_UNKNOWN);
        }
        return;
    }

    









    
    MaybeRegisterID typeReg = frame.maybePinType(fval);
    RegisterID objReg = frame.copyDataIntoReg(fval);

    MaybeJump isNotObj;
    if (!fval->isType(JSVAL_TYPE_OBJECT)) {
        isNotObj = frame.testObject(Assembler::NotEqual, fval);
        frame.maybeUnpinReg(typeReg);
    }

    


    Jump notFunction = masm.testFunction(Assembler::NotEqual, objReg);
    stubcc.linkExit(notFunction, Uses(1));

    



    masm.loadPtr(Address(objReg, offsetof(JSObject, parent)), objReg);
    Jump globalMismatch = masm.branchPtr(Assembler::NotEqual, objReg, ImmPtr(globalObj));
    stubcc.linkExit(globalMismatch, Uses(1));
    frame.freeReg(objReg);

    
    stubcc.leave();
    OOL_STUBCALL_NO_REJOIN(stubs::PushImplicitThisForGlobal);

    
    if (isNotObj.isSet())
        isNotObj.getJump().linkTo(masm.label(), &masm);
    frame.pushUntypedValue(UndefinedValue());

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_setgname_slow(JSAtom *atom, bool usePropertyCache)
{
    prepareStubCall(Uses(2));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    if (usePropertyCache)
        INLINE_STUBCALL(STRICT_VARIANT(stubs::SetGlobalName));
    else
        INLINE_STUBCALL(STRICT_VARIANT(stubs::SetGlobalNameNoCache));
    frame.popn(2);
    pushSyncedEntry(0);
}

void
mjit::Compiler::jsop_setgname(JSAtom *atom, bool usePropertyCache, bool popGuaranteed)
{
    REJOIN_SITE_2(ic::SetGlobalName,
                  usePropertyCache
                  ? STRICT_VARIANT(stubs::SetGlobalName)
                  : STRICT_VARIANT(stubs::SetGlobalNameNoCache));

    if (monitored(PC)) {
        
        jsop_setgname_slow(atom, usePropertyCache);
        return;
    }

    if (cx->typeInferenceEnabled() && globalObj->isGlobal() &&
        !globalObj->getType()->unknownProperties()) {
        





        types::TypeSet *types = globalObj->getType()->getProperty(cx, ATOM_TO_JSID(atom), false);
        if (!types)
            return;
        const js::Shape *shape = globalObj->nativeLookup(ATOM_TO_JSID(atom));
        if (shape && !shape->isMethod() && shape->hasDefaultSetter() &&
            shape->writable() && shape->hasSlot() && !types->isOwnProperty(cx, true)) {
            watchGlobalReallocation();
            Value *value = &globalObj->getSlotRef(shape->slot);
            RegisterID reg = frame.allocReg();
            masm.move(ImmPtr(value), reg);
            frame.storeTo(frame.peek(-1), Address(reg), popGuaranteed);
            frame.shimmy(1);
            frame.freeReg(reg);
            return;
        }
    }

#if defined JS_MONOIC
    FrameEntry *objFe = frame.peek(-2);
    FrameEntry *fe = frame.peek(-1);
    JS_ASSERT_IF(objFe->isTypeKnown(), objFe->getKnownType() == JSVAL_TYPE_OBJECT);

    if (!fe->isConstant() && fe->isType(JSVAL_TYPE_DOUBLE))
        frame.forgetKnownDouble(fe);

    SetGlobalNameICInfo ic;

    frame.pinEntry(fe, ic.vr);
    Jump shapeGuard;

    RESERVE_IC_SPACE(masm);
    ic.fastPathStart = masm.label();
    if (objFe->isConstant()) {
        JSObject *obj = &objFe->getValue().toObject();
        JS_ASSERT(obj->isNative());

        ic.objReg = frame.allocReg();
        ic.shapeReg = ic.objReg;
        ic.objConst = true;

        masm.load32FromImm(&obj->objShape, ic.shapeReg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, ic.shapeReg,
                                            Imm32(int32(INVALID_SHAPE)),
                                            ic.shape);
        masm.move(ImmPtr(obj), ic.objReg);
    } else {
        ic.objReg = frame.copyDataIntoReg(objFe);
        ic.shapeReg = frame.allocReg();
        ic.objConst = false;

        masm.loadShape(ic.objReg, ic.shapeReg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, ic.shapeReg,
                                            Imm32(int32(INVALID_SHAPE)),
                                            ic.shape);
        frame.freeReg(ic.shapeReg);
    }
    ic.shapeGuardJump = shapeGuard;
    ic.slowPathStart = stubcc.linkExit(shapeGuard, Uses(2));

    stubcc.leave();
    passMICAddress(ic);
    ic.slowPathCall = OOL_STUBCALL(ic::SetGlobalName);

    
    uint32 slot = 1 << 24;

    ic.usePropertyCache = usePropertyCache;

    masm.loadPtr(Address(ic.objReg, offsetof(JSObject, slots)), ic.objReg);
    Address address(ic.objReg, slot);

    if (ic.vr.isConstant()) {
        ic.store = masm.storeValueWithAddressOffsetPatch(ic.vr.value(), address);
    } else if (ic.vr.isTypeKnown()) {
        ic.store = masm.storeValueWithAddressOffsetPatch(ImmType(ic.vr.knownType()),
                                                          ic.vr.dataReg(), address);
    } else {
        ic.store = masm.storeValueWithAddressOffsetPatch(ic.vr.typeReg(), ic.vr.dataReg(), address);
    }

    frame.freeReg(ic.objReg);
    frame.unpinEntry(ic.vr);
    frame.shimmy(1);

    stubcc.rejoin(Changes(1));

    ic.fastPathRejoin = masm.label();
    setGlobalNames.append(ic);
#else
    jsop_setgname_slow(atom, usePropertyCache);
#endif
}

void
mjit::Compiler::jsop_setelem_slow()
{
    prepareStubCall(Uses(3));
    INLINE_STUBCALL(STRICT_VARIANT(stubs::SetElem));
    frame.popn(3);
    frame.pushSynced(JSVAL_TYPE_UNKNOWN);
}

void
mjit::Compiler::jsop_getelem_slow()
{
    prepareStubCall(Uses(2));
    INLINE_STUBCALL(stubs::GetElem);
    frame.popn(2);
    pushSyncedEntry(0);
}

void
mjit::Compiler::jsop_unbrand()
{
    REJOIN_SITE(stubs::Unbrand);
    prepareStubCall(Uses(1));
    INLINE_STUBCALL(stubs::Unbrand);
}

bool
mjit::Compiler::jsop_instanceof()
{
    REJOIN_SITE_ANY();

    FrameEntry *lhs = frame.peek(-2);
    FrameEntry *rhs = frame.peek(-1);

    
    if (rhs->isNotType(JSVAL_TYPE_OBJECT) || lhs->isNotType(JSVAL_TYPE_OBJECT)) {
        stubcc.linkExit(masm.jump(), Uses(2));
        frame.discardFe(lhs);
        frame.discardFe(rhs);
    }

    MaybeJump firstSlow;
    if (!rhs->isTypeKnown()) {
        Jump j = frame.testObject(Assembler::NotEqual, rhs);
        stubcc.linkExit(j, Uses(2));
    }

    frame.forgetMismatchedObject(lhs);
    frame.forgetMismatchedObject(rhs);

    RegisterID obj = frame.tempRegForData(rhs);
    Jump notFunction = masm.testFunction(Assembler::NotEqual, obj);
    stubcc.linkExit(notFunction, Uses(2));

    
    Jump isBound = masm.branchTest32(Assembler::NonZero, Address(obj, offsetof(JSObject, flags)),
                                     Imm32(JSObject::BOUND_FUNCTION));
    {
        stubcc.linkExit(isBound, Uses(2));
        stubcc.leave();
        OOL_STUBCALL(stubs::InstanceOf);
        firstSlow = stubcc.masm.jump();
    }
    

    
    frame.dup();

    if (!jsop_getprop(cx->runtime->atomState.classPrototypeAtom, JSVAL_TYPE_UNKNOWN, false))
        return false;

    
    rhs = frame.peek(-1);
    Jump j = frame.testPrimitive(Assembler::Equal, rhs);
    stubcc.linkExit(j, Uses(3));

    
    obj = frame.copyDataIntoReg(lhs);
    RegisterID proto = frame.copyDataIntoReg(rhs);
    RegisterID temp = frame.allocReg();

    MaybeJump isFalse;
    if (!lhs->isTypeKnown())
        isFalse = frame.testPrimitive(Assembler::Equal, lhs);

    Label loop = masm.label();

    
    masm.loadPtr(Address(obj, offsetof(JSObject, type)), obj);
    masm.loadPtr(Address(obj, offsetof(types::TypeObject, proto)), obj);
    Jump isFalse2 = masm.branchTestPtr(Assembler::Zero, obj, obj);
    Jump isTrue = masm.branchPtr(Assembler::NotEqual, obj, proto);
    isTrue.linkTo(loop, &masm);
    masm.move(Imm32(1), temp);
    isTrue = masm.jump();

    if (isFalse.isSet())
        isFalse.getJump().linkTo(masm.label(), &masm);
    isFalse2.linkTo(masm.label(), &masm);
    masm.move(Imm32(0), temp);
    isTrue.linkTo(masm.label(), &masm);

    frame.freeReg(proto);
    frame.freeReg(obj);

    stubcc.leave();
    OOL_STUBCALL(stubs::FastInstanceOf);

    frame.popn(3);
    frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, temp);

    if (firstSlow.isSet())
        firstSlow.getJump().linkTo(stubcc.masm.label(), &stubcc.masm);
    stubcc.rejoin(Changes(1));
    return true;
}

void
mjit::Compiler::emitEval(uint32 argc)
{
    
    interruptCheckHelper();

    frame.syncAndKill(Uses(argc + 2));
    prepareStubCall(Uses(argc + 2));
    masm.move(Imm32(argc), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::Eval);
    frame.popn(argc + 2);
    pushSyncedEntry(0);
}

void
mjit::Compiler::jsop_arguments()
{
    REJOIN_SITE(stubs::Arguments);
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(stubs::Arguments);
}

bool
mjit::Compiler::jsop_newinit()
{
    bool isArray;
    unsigned count = 0;
    JSObject *baseobj = NULL;
    switch (*PC) {
      case JSOP_NEWINIT:
        isArray = (PC[1] == JSProto_Array);
        break;
      case JSOP_NEWARRAY:
        isArray = true;
        count = GET_UINT24(PC);
        break;
      case JSOP_NEWOBJECT:
        isArray = false;
        baseobj = script->getObject(fullAtomIndex(PC));
        break;
      default:
        JS_NOT_REACHED("Bad op");
        return false;
    }

    prepareStubCall(Uses(0));

    
    types::TypeObject *type = NULL;
    if (script->compileAndGo) {
        type = script->getTypeInitObject(cx, PC, isArray);
        if (!type)
            return false;
    }
    masm.storePtr(ImmPtr(type), FrameAddress(offsetof(VMFrame, scratch)));

    if (isArray) {
        masm.move(Imm32(count), Registers::ArgReg1);
        INLINE_STUBCALL_NO_REJOIN(stubs::NewInitArray);
    } else {
        masm.move(ImmPtr(baseobj), Registers::ArgReg1);
        INLINE_STUBCALL_NO_REJOIN(stubs::NewInitObject);
    }
    frame.takeReg(Registers::ReturnReg);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);

    frame.extra(frame.peek(-1)).initArray = (*PC == JSOP_NEWARRAY);
    frame.extra(frame.peek(-1)).initObject = baseobj;

    return true;
}

bool
mjit::Compiler::startLoop(jsbytecode *head, Jump entry, jsbytecode *entryTarget)
{
    JS_ASSERT(cx->typeInferenceEnabled() && script == outerScript);

    if (loop) {
        





        loop->clearLoopRegisters();
    }

    LoopState *nloop = cx->new_<LoopState>(cx, script, this, &frame);
    if (!nloop || !nloop->init(head, entry, entryTarget))
        return false;

    nloop->outer = loop;
    loop = nloop;
    frame.setLoop(loop);

    return true;
}

bool
mjit::Compiler::finishLoop(jsbytecode *head)
{
    if (!cx->typeInferenceEnabled())
        return true;

    




    JS_ASSERT(loop && loop->headOffset() == uint32(head - script->code));

    jsbytecode *entryTarget = script->code + loop->entryOffset();

    




    Jump fallthrough = masm.jump();

#ifdef DEBUG
    if (IsJaegerSpewChannelActive(JSpew_Regalloc)) {
        RegisterAllocation *alloc = analysis->getAllocation(head);
        JaegerSpew(JSpew_Regalloc, "loop allocation at %u:", head - script->code);
        frame.dumpAllocation(alloc);
    }
#endif

    loop->entryJump().linkTo(masm.label(), &masm);

    jsbytecode *oldPC = PC;

    PC = entryTarget;
    {
        REJOIN_SITE(stubs::MissedBoundsCheckEntry);
        OOL_STUBCALL(stubs::MissedBoundsCheckEntry);

        if (loop->generatingInvariants()) {
            



            if (oomInVector)
                return false;
            Label label = callSites[callSites.length() - 1].loopJumpLabel;
            stubcc.linkExitDirect(masm.jump(), label);
        }
        stubcc.crossJump(stubcc.masm.jump(), masm.label());
    }
    PC = oldPC;

    frame.prepareForJump(entryTarget, masm, true);

    if (!jumpInScript(masm.jump(), entryTarget))
        return false;

    PC = head;
    if (!analysis->getCode(head).safePoint) {
        



        LoopEntry entry;
        entry.pcOffset = head - script->code;

        AutoRejoinSite autoRejoinHead(this, JS_FUNC_TO_DATA_PTR(void *, stubs::MissedBoundsCheckHead));
        OOL_STUBCALL(stubs::MissedBoundsCheckHead);

        if (loop->generatingInvariants()) {
            if (oomInVector)
                return false;
            entry.label = callSites[callSites.length() - 1].loopJumpLabel;
        } else {
            entry.label = stubcc.masm.label();
        }

        






        for (uint32 slot = analyze::ArgSlot(0); slot < analyze::TotalSlots(script); slot++) {
            if (a->varTypes[slot].type == JSVAL_TYPE_DOUBLE) {
                FrameEntry *fe = frame.getOrTrack(slot);
                stubcc.masm.ensureInMemoryDouble(frame.addressOf(fe));
            }
        }

        autoRejoinHead.oolRejoin(stubcc.masm.label());
        frame.prepareForJump(head, stubcc.masm, true);
        if (!stubcc.jumpInScript(stubcc.masm.jump(), head))
            return false;

        loopEntries.append(entry);
    }
    PC = oldPC;

    
    loop->flushLoop(stubcc);

    LoopState *nloop = loop->outer;
    cx->delete_(loop);
    loop = nloop;
    frame.setLoop(loop);

    fallthrough.linkTo(masm.label(), &masm);

    



    frame.clearTemporaries();

    return true;
}
















bool
mjit::Compiler::jumpAndTrace(Jump j, jsbytecode *target, Jump *slow, bool *trampoline)
{
    if (trampoline)
        *trampoline = false;

    



    RegisterAllocation *lvtarget = NULL;
    bool consistent = true;
    if (cx->typeInferenceEnabled()) {
        RegisterAllocation *&alloc = analysis->getAllocation(target);
        if (!alloc) {
            alloc = ArenaNew<RegisterAllocation>(cx->compartment->pool, false);
            if (!alloc)
                return false;
        }
        lvtarget = alloc;
        consistent = frame.consistentRegisters(target);
    }

    if (!addTraceHints || target >= PC ||
        (JSOp(*target) != JSOP_TRACE && JSOp(*target) != JSOP_NOTRACE)
#ifdef JS_MONOIC
        || GET_UINT16(target) == BAD_TRACEIC_INDEX
#endif
        )
    {
        if (!lvtarget || lvtarget->synced()) {
            JS_ASSERT(consistent);
            if (!jumpInScript(j, target))
                return false;
            if (slow && !stubcc.jumpInScript(*slow, target))
                return false;
        } else {
            if (consistent) {
                if (!jumpInScript(j, target))
                    return false;
            } else {
                



                stubcc.linkExitDirect(j, stubcc.masm.label());
                frame.prepareForJump(target, stubcc.masm, false);
                if (!stubcc.jumpInScript(stubcc.masm.jump(), target))
                    return false;
                if (trampoline)
                    *trampoline = true;
            }

            if (slow) {
                slow->linkTo(stubcc.masm.label(), &stubcc.masm);
                frame.prepareForJump(target, stubcc.masm, true);
                if (!stubcc.jumpInScript(stubcc.masm.jump(), target))
                    return false;
            }
        }

        if (target < PC)
            return finishLoop(target);
        return true;
    }

    
    JS_ASSERT(!trampoline);

#ifndef JS_TRACER
    JS_NOT_REACHED("Bad addTraceHints");
    return false;
#else

# if JS_MONOIC
    TraceGenInfo ic;

    ic.initialized = true;
    ic.stubEntry = stubcc.masm.label();
    ic.traceHint = j;
    if (slow)
        ic.slowTraceHint = *slow;

    uint16 index = GET_UINT16(target);
    if (traceICs.length() <= index)
        if (!traceICs.resize(index+1))
            return false;
# endif

    Label traceStart = stubcc.masm.label();

    stubcc.linkExitDirect(j, traceStart);
    if (slow)
        slow->linkTo(traceStart, &stubcc.masm);

# if JS_MONOIC
    ic.addrLabel = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);

    Jump nonzero = stubcc.masm.branchSub32(Assembler::NonZero, Imm32(1),
                                           Address(Registers::ArgReg1,
                                                   offsetof(TraceICInfo, loopCounter)));
# endif

    
    {
        jsbytecode* pc = PC;
        PC = target;

        OOL_STUBCALL(stubs::InvokeTracer);

        PC = pc;
    }

    Jump no = stubcc.masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                        Registers::ReturnReg);
    if (!cx->typeInferenceEnabled())
        stubcc.masm.loadPtr(FrameAddress(VMFrame::offsetOfFp), JSFrameReg);
    stubcc.masm.jump(Registers::ReturnReg);
    no.linkTo(stubcc.masm.label(), &stubcc.masm);

#ifdef JS_MONOIC
    nonzero.linkTo(stubcc.masm.label(), &stubcc.masm);

    ic.jumpTarget = target;
    ic.fastTrampoline = !consistent;
    ic.trampolineStart = stubcc.masm.label();

    traceICs[index] = ic;
#endif

    



    if (JSOp(*target) == JSOP_NOTRACE) {
        if (consistent) {
            if (!jumpInScript(j, target))
                return false;
        } else {
            stubcc.linkExitDirect(j, stubcc.masm.label());
        }
        if (slow)
            slow->linkTo(stubcc.masm.label(), &stubcc.masm);
    }

    




    frame.prepareForJump(target, stubcc.masm, true);

    if (!stubcc.jumpInScript(stubcc.masm.jump(), target))
        return false;
#endif

    return finishLoop(target);
}

void
mjit::Compiler::enterBlock(JSObject *obj)
{
    
    
    
    
    
    
    if (analysis->getCode(PC).exceptionEntry) {
        masm.loadPtr(FrameAddress(VMFrame::offsetOfFp), JSFrameReg);
        interruptCheckHelper();
    }

    
    frame.syncAndForgetEverything();
    masm.move(ImmPtr(obj), Registers::ArgReg1);
    uint32 n = js_GetEnterBlockStackDefs(cx, script, PC);
    INLINE_STUBCALL_NO_REJOIN(stubs::EnterBlock);
    frame.enterBlock(n);
}

void
mjit::Compiler::leaveBlock()
{
    



    uint32 n = js_GetVariableStackUses(JSOP_LEAVEBLOCK, PC);
    JSObject *obj = script->getObject(fullAtomIndex(PC + UINT16_LEN));
    prepareStubCall(Uses(n));
    masm.move(ImmPtr(obj), Registers::ArgReg1);
    INLINE_STUBCALL_NO_REJOIN(stubs::LeaveBlock);
    frame.leaveBlock(n);
}









bool
mjit::Compiler::constructThis()
{
    JS_ASSERT(isConstructing);
    REJOIN_SITE(stubs::CreateThis);

    
    frame.pushCallee();

    
    if (!jsop_getprop(cx->runtime->atomState.classPrototypeAtom, JSVAL_TYPE_UNKNOWN, false, false))
        return false;

    
    FrameEntry *protoFe = frame.peek(-1);
    RegisterID protoReg = frame.ownRegForData(protoFe);

    
    JS_ASSERT_IF(protoFe->isTypeKnown(), protoFe->isType(JSVAL_TYPE_OBJECT));
    if (!protoFe->isType(JSVAL_TYPE_OBJECT)) {
        Jump isNotObject = frame.testObject(Assembler::NotEqual, protoFe);
        stubcc.linkExitDirect(isNotObject, stubcc.masm.label());
        stubcc.masm.move(ImmPtr(NULL), protoReg);
        stubcc.crossJump(stubcc.masm.jump(), masm.label());
    }

    
    frame.pop();

    prepareStubCall(Uses(0));
    if (protoReg != Registers::ArgReg1)
        masm.move(protoReg, Registers::ArgReg1);
    INLINE_STUBCALL(stubs::CreateThis);
    frame.freeReg(protoReg);
    return true;
}

bool
mjit::Compiler::jsop_tableswitch(jsbytecode *pc)
{
#if defined JS_CPU_ARM
    JS_NOT_REACHED("Implement jump(BaseIndex) for ARM");
    return true;
#else
    jsbytecode *originalPC = pc;

    uint32 defaultTarget = GET_JUMP_OFFSET(pc);
    pc += JUMP_OFFSET_LEN;

    jsint low = GET_JUMP_OFFSET(pc);
    pc += JUMP_OFFSET_LEN;
    jsint high = GET_JUMP_OFFSET(pc);
    pc += JUMP_OFFSET_LEN;
    int numJumps = high + 1 - low;
    JS_ASSERT(numJumps >= 0);

    



    if (numJumps == 0) {
        frame.pop();
        return true;
    }

    FrameEntry *fe = frame.peek(-1);
    if (fe->isNotType(JSVAL_TYPE_INT32) || numJumps > 256) {
        frame.syncAndForgetEverything();
        masm.move(ImmPtr(originalPC), Registers::ArgReg1);

        
        INLINE_STUBCALL_NO_REJOIN(stubs::TableSwitch);
        frame.pop();
        masm.jump(Registers::ReturnReg);
        return true;
    }

    RegisterID dataReg;
    if (fe->isConstant()) {
        JS_ASSERT(fe->isType(JSVAL_TYPE_INT32));
        dataReg = frame.allocReg();
        masm.move(Imm32(fe->getValue().toInt32()), dataReg);
    } else {
        dataReg = frame.copyDataIntoReg(fe);
    }

    RegisterID reg = frame.allocReg();
    frame.syncAndForgetEverything();

    MaybeJump notInt;
    if (!fe->isType(JSVAL_TYPE_INT32))
        notInt = masm.testInt32(Assembler::NotEqual, frame.addressOf(fe));

    JumpTable jt;
    jt.offsetIndex = jumpTableOffsets.length();
    jt.label = masm.moveWithPatch(ImmPtr(NULL), reg);
    jumpTables.append(jt);

    for (int i = 0; i < numJumps; i++) {
        uint32 target = GET_JUMP_OFFSET(pc);
        if (!target)
            target = defaultTarget;
        uint32 offset = (originalPC + target) - script->code;
        jumpTableOffsets.append(offset);
        pc += JUMP_OFFSET_LEN;
    }
    if (low != 0)
        masm.sub32(Imm32(low), dataReg);
    Jump defaultCase = masm.branch32(Assembler::AboveOrEqual, dataReg, Imm32(numJumps));
    BaseIndex jumpTarget(reg, dataReg, Assembler::ScalePtr);
    masm.jump(jumpTarget);

    if (notInt.isSet()) {
        stubcc.linkExitDirect(notInt.get(), stubcc.masm.label());
        stubcc.leave();
        stubcc.masm.move(ImmPtr(originalPC), Registers::ArgReg1);
        OOL_STUBCALL_NO_REJOIN(stubs::TableSwitch);
        stubcc.masm.jump(Registers::ReturnReg);
    }
    frame.pop();
    return jumpAndTrace(defaultCase, originalPC + defaultTarget);
#endif
}

void
mjit::Compiler::jsop_callelem_slow()
{
    prepareStubCall(Uses(2));
    INLINE_STUBCALL(stubs::CallElem);
    frame.popn(2);
    pushSyncedEntry(0);
    pushSyncedEntry(1);
}

void
mjit::Compiler::jsop_forprop(JSAtom *atom)
{
    
    
    frame.dupAt(-2);

    
    
    iterNext();

    
    
    frame.shimmy(1);

    
    
    jsop_setprop(atom, false, true);

    
    
    frame.pop();
}

void
mjit::Compiler::jsop_forname(JSAtom *atom)
{
    
    
    jsop_bindname(atom, false);
    jsop_forprop(atom);
}

void
mjit::Compiler::jsop_forgname(JSAtom *atom)
{
    
    
    jsop_bindgname();

    
    
    frame.dupAt(-2);

    
    
    iterNext();

    
    
    frame.shimmy(1);

    
    
    jsop_setgname(atom, false, true);

    
    
    frame.pop();
}












inline bool
mjit::Compiler::fixDoubleSlot(uint32 slot)
{
    if (!analysis->trackSlot(slot))
        return false;

    



    if (slot < analyze::LocalSlot(script, 0) && a->parent)
        return false;

    return true;
}

void
mjit::Compiler::fixDoubleTypes(jsbytecode *target)
{
    if (!cx->typeInferenceEnabled())
        return;

    





    const analyze::SlotValue *newv = analysis->newValues(target);
    if (newv) {
        while (newv->slot) {
            if (newv->value.kind() != analyze::SSAValue::PHI ||
                newv->value.phiOffset() != uint32(target - script->code)) {
                newv++;
                continue;
            }
            if (newv->slot < analyze::TotalSlots(script)) {
                VarType &vt = a->varTypes[newv->slot];
                if (vt.type == JSVAL_TYPE_INT32) {
                    types::TypeSet *targetTypes = analysis->getValueTypes(newv->value);
                    if (targetTypes->getKnownTypeTag(cx) == JSVAL_TYPE_DOUBLE &&
                        fixDoubleSlot(newv->slot)) {
                        fixedDoubleEntries.append(newv->slot);
                        FrameEntry *fe = frame.getOrTrack(newv->slot);
                        frame.ensureDouble(fe);
                    }
                }
            }
            newv++;
        }
    }
}

void
mjit::Compiler::restoreAnalysisTypes()
{
    if (!cx->typeInferenceEnabled())
        return;

    
    const analyze::SlotValue *newv = analysis->newValues(PC);
    if (newv) {
        while (newv->slot) {
            if (newv->slot < analyze::TotalSlots(script)) {
                VarType &vt = a->varTypes[newv->slot];
                vt.types = analysis->getValueTypes(newv->value);
                vt.type = vt.types->getKnownTypeTag(cx);
            }
            newv++;
        }
    }

    
    for (uint32 slot = analyze::ArgSlot(0); slot < analyze::TotalSlots(script); slot++) {
        JSValueType type = a->varTypes[slot].type;
        if (type != JSVAL_TYPE_UNKNOWN && (type != JSVAL_TYPE_DOUBLE || fixDoubleSlot(slot))) {
            FrameEntry *fe = frame.getOrTrack(slot);
            JS_ASSERT_IF(fe->isTypeKnown(), fe->isType(type));
            if (!fe->isTypeKnown())
                frame.learnType(fe, type, false);
        }
    }
}

void
mjit::Compiler::watchGlobalReallocation()
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (hasGlobalReallocation)
        return;
    types::TypeSet::WatchObjectReallocation(cx, globalObj);
    hasGlobalReallocation = true;
}

void
mjit::Compiler::updateVarType()
{
    if (!cx->typeInferenceEnabled())
        return;

    






    types::TypeSet *types = NULL;
    switch (JSOp(*PC)) {
      case JSOP_SETARG:
      case JSOP_SETLOCAL:
      case JSOP_SETLOCALPOP:
      case JSOP_DEFLOCALFUN:
      case JSOP_DEFLOCALFUN_FC:
      case JSOP_INCARG:
      case JSOP_DECARG:
      case JSOP_ARGINC:
      case JSOP_ARGDEC:
      case JSOP_INCLOCAL:
      case JSOP_DECLOCAL:
      case JSOP_LOCALINC:
      case JSOP_LOCALDEC:
        types = pushedTypeSet(0);
        break;
      case JSOP_FORARG:
      case JSOP_FORLOCAL:
        types = pushedTypeSet(1);
        break;
      default:
        JS_NOT_REACHED("Bad op");
    }

    uint32 slot = analyze::GetBytecodeSlot(script, PC);

    if (analysis->trackSlot(slot)) {
        VarType &vt = a->varTypes[slot];
        vt.types = types;
        vt.type = types->getKnownTypeTag(cx);
    }
}

JSValueType
mjit::Compiler::knownPushedType(uint32 pushed)
{
    if (!cx->typeInferenceEnabled())
        return JSVAL_TYPE_UNKNOWN;
    types::TypeSet *types = analysis->pushedTypes(PC, pushed);
    return types->getKnownTypeTag(cx);
}

bool
mjit::Compiler::mayPushUndefined(uint32 pushed)
{
    JS_ASSERT(cx->typeInferenceEnabled());

    





    types::TypeSet *types = analysis->pushedTypes(PC, pushed);
    return types->hasType(types::TYPE_UNDEFINED);
}

types::TypeSet *
mjit::Compiler::pushedTypeSet(uint32 pushed)
{
    if (!cx->typeInferenceEnabled())
        return NULL;
    return analysis->pushedTypes(PC, pushed);
}

bool
mjit::Compiler::monitored(jsbytecode *pc)
{
    return cx->typeInferenceEnabled() && analysis->monitoredTypes(pc - script->code);
}

void
mjit::Compiler::pushSyncedEntry(uint32 pushed)
{
    frame.pushSynced(knownPushedType(pushed));
}

JSObject *
mjit::Compiler::pushedSingleton(unsigned pushed)
{
    if (!cx->typeInferenceEnabled())
        return NULL;

    types::TypeSet *types = analysis->pushedTypes(PC, pushed);
    return types->getSingleton(cx);
}

bool
mjit::Compiler::arrayPrototypeHasIndexedProperty()
{
    if (!cx->typeInferenceEnabled())
        return true;

    




    JSObject *proto;
    if (!js_GetClassPrototype(cx, NULL, JSProto_Array, &proto, NULL))
        return false;
    types::TypeSet *arrayTypes = proto->getType()->getProperty(cx, JSID_VOID, false);
    types::TypeSet *objectTypes = proto->getProto()->getType()->getProperty(cx, JSID_VOID, false);
    return arrayTypes->knownNonEmpty(cx) || objectTypes->knownNonEmpty(cx);
}
