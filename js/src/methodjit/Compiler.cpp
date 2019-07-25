








































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

mjit::Compiler::Compiler(JSContext *cx, JSStackFrame *fp)
  : BaseCompiler(cx),
    fp(fp),
    script(fp->script()),
    scopeChain(&fp->scopeChain()),
    globalObj(scopeChain->getGlobal()),
    fun(fp->isFunctionFrame() && !fp->isEvalFrame()
        ? fp->fun()
        : NULL),
    isConstructing(fp->isConstructing()),
    analysis(NULL), jumpMap(NULL), savedTraps(NULL),
    frame(cx, script, fun, *this, masm, stubcc, liveness),
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
    doubleList(CompilerAllocPolicy(cx, *thisFromCtor())),
    jumpTables(CompilerAllocPolicy(cx, *thisFromCtor())),
    jumpTableOffsets(CompilerAllocPolicy(cx, *thisFromCtor())),
    loopEntries(CompilerAllocPolicy(cx, *thisFromCtor())),
    stubcc(cx, *thisFromCtor(), frame, script),
    debugMode_(cx->compartment->debugMode),
#if defined JS_TRACER
    addTraceHints(cx->traceJitEnabled),
#else
    addTraceHints(false),
#endif
    recompiling(false),
    hasThisType(false),
    thisType(JSVAL_TYPE_UNKNOWN),
    argumentTypes(ContextAllocPolicy(cx)),
    localTypes(ContextAllocPolicy(cx)),
    oomInVector(false),
    applyTricks(NoApplyTricks)
{
    
    if (cx->typeInferenceEnabled())
        addTraceHints = false;
}

CompileStatus
mjit::Compiler::compile(const Vector<JSStackFrame*> *frames)
{
    JS_ASSERT_IF(isConstructing, !script->jitCtor);
    JS_ASSERT_IF(!isConstructing, !script->jitNormal);

    JITScript **jit = isConstructing ? &script->jitCtor : &script->jitNormal;
    void **checkAddr = isConstructing
                       ? &script->jitArityCheckCtor
                       : &script->jitArityCheckNormal;

    CompileStatus status = performCompilation(jit, frames);
    if (status == Compile_Okay) {
        
        
        
        
        *checkAddr = (*jit)->arityCheckEntry
                     ? (*jit)->arityCheckEntry
                     : (*jit)->invokeEntry;
    } else {
        *checkAddr = JS_UNJITTABLE_SCRIPT;
    }

    return status;
}

#define CHECK_STATUS(expr)                                           \
    JS_BEGIN_MACRO                                                   \
        CompileStatus status_ = (expr);                              \
        if (status_ != Compile_Okay) {                               \
            if (oomInVector || masm.oom() || stubcc.masm.oom())      \
                js_ReportOutOfMemory(cx);                            \
            if (!cx->compartment->types.checkPendingRecompiles(cx))  \
                return Compile_Error;                                \
            return status_;                                          \
        }                                                            \
    JS_END_MACRO

CompileStatus
mjit::Compiler::performCompilation(JITScript **jitp, const Vector<JSStackFrame*> *frames)
{
    JaegerSpew(JSpew_Scripts, "compiling script (file \"%s\") (line \"%d\") (length \"%d\")\n",
               script->filename, script->lineno, script->length);

    analyze::Script analysis_;
    analysis_.analyze(cx, script);

    this->analysis = &analysis_;

    if (analysis->OOM())
        return Compile_Error;
    if (analysis->failed()) {
        JaegerSpew(JSpew_Abort, "couldn't analyze bytecode; probably switchX or OOM\n");
        return Compile_Abort;
    }
    frame.setAnalysis(analysis);

    if (!liveness.analyze(cx, analysis, script, fun)) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }

#ifdef JS_METHODJIT_SPEW
    if (IsJaegerSpewChannelActive(JSpew_Regalloc)) {
        for (unsigned i = 0; i < script->nfixed; i++) {
            if (!analysis->localEscapes(i)) {
                JaegerSpew(JSpew_Regalloc, "Local %u:", i);
                liveness.dumpLocal(i);
            }
        }
        for (unsigned i = 0; fun && i < fun->nargs; i++) {
            if (!analysis->argEscapes(i)) {
                JaegerSpew(JSpew_Regalloc, "Argument %u:", i);
                liveness.dumpArg(i);
            }
        }
    }
#endif

    if (!frame.init()) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }

    jumpMap = (Label *)cx->malloc(sizeof(Label) * script->length);
    if (!jumpMap) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }
#ifdef DEBUG
    for (uint32 i = 0; i < script->length; i++)
        jumpMap[i] = Label();
#endif

#ifdef JS_METHODJIT_SPEW
    Profiler prof;
    prof.start();
#endif

    
    PC = script->code;

#ifdef JS_METHODJIT
    script->debugMode = debugMode();
#endif

    for (uint32 i = 0; i < script->nClosedVars; i++)
        frame.setClosedVar(script->getClosedVar(i));
    for (uint32 i = 0; i < script->nClosedArgs; i++)
        frame.setClosedArg(script->getClosedArg(i));

    types::AutoEnterTypeInference enter(cx, true);

    if (cx->typeInferenceEnabled()) {
        CompileStatus status = prepareInferenceTypes(frames);
        if (status != Compile_Okay) {
            if (!cx->compartment->types.checkPendingRecompiles(cx))
                return Compile_Error;
            return status;
        }
    }

    CHECK_STATUS(generatePrologue());
    CHECK_STATUS(generateMethod());
    CHECK_STATUS(generateEpilogue());
    CHECK_STATUS(finishThisUp(jitp));

#ifdef JS_METHODJIT_SPEW
    prof.stop();
    JaegerSpew(JSpew_Prof, "compilation took %d us\n", prof.time_us());
#endif

    JaegerSpew(JSpew_Scripts, "successfully compiled (code \"%p\") (size \"%ld\")\n",
               (*jitp)->code.m_code.executableAddress(), (*jitp)->code.m_size);

    if (!cx->compartment->types.checkPendingRecompiles(cx))
        return Compile_Error;

    if (!*jitp)
        return Compile_Abort;

    return Compile_Okay;
}

#undef CHECK_STATUS

mjit::Compiler::~Compiler()
{
    cx->free(jumpMap);
    cx->free(savedTraps);
}

CompileStatus
mjit::Compiler::prepareInferenceTypes(const Vector<JSStackFrame*> *frames)
{
    
    if (!script->types) {
        
        if (script->isUncachedEval)
            return Compile_Abort;
        types::AnalyzeScriptTypes(cx, script);
        if (!script->types)
            return Compile_Error;
    }

    










    uint32 nargs = fun ? fun->nargs : 0;
    if (!argumentTypes.reserve(nargs))
        return Compile_Error;
    for (unsigned i = 0; i < nargs; i++) {
        JSValueType type = JSVAL_TYPE_UNKNOWN;
        if (!analysis->argEscapes(i))
            type = script->argTypes(i)->getKnownTypeTag(cx, script);
        argumentTypes.append(type);
        if (type == JSVAL_TYPE_DOUBLE && frames) {
            for (unsigned j = 0; j < frames->length(); j++) {
                Value *vp = (*frames)[j]->formalArgs() + i;
                if (vp->isInt32())
                    vp->setDouble((double)vp->toInt32());
            }
        }
    }

    if (!localTypes.reserve(script->nfixed))
        return Compile_Error;
    for (unsigned i = 0; i < script->nfixed; i++) {
        JSValueType type = JSVAL_TYPE_UNKNOWN;
        if (!analysis->localHasUseBeforeDef(i))
            type = script->localTypes(i)->getKnownTypeTag(cx, script);
        localTypes.append(type);
        if (type == JSVAL_TYPE_DOUBLE && frames) {
            for (unsigned j = 0; j < frames->length(); j++) {
                Value *vp = (*frames)[j]->slots() + i;
                if (vp->isInt32())
                    vp->setDouble((double)vp->toInt32());
            }
        }
    }

    








    for (unsigned i = 0; frames && i < frames->length(); i++) {
        uint32 offset = (*frames)[i]->pc(cx) - script->code;
        uint32 stackDepth = analysis->getCode(offset).stackDepth;
        if (!stackDepth)
            continue;
        JS_ASSERT(offset);
        while (offset--) {
            analyze::Bytecode *code = analysis->maybeCode(offset);
            if (!code)
                continue;

            uint32 startDepth = code->stackDepth - analyze::GetUseCount(script, offset);
            if (startDepth < stackDepth) {
                for (unsigned j = startDepth; j < stackDepth; j++) {
                    types::TypeSet *types = script->types->pushed(offset, j - startDepth);
                    JSValueType type = types->getKnownTypeTag(cx, NULL);
                    if (type == JSVAL_TYPE_DOUBLE) {
                        Value *vp = (*frames)[i]->slots() + script->nfixed + j;
                        if (vp->isInt32())
                            vp->setDouble((double)vp->toInt32());
                    }
                }
                stackDepth = startDepth;
            }

            if (code->jumpTarget)
                break;
        }
    }

    return Compile_Okay;
}

CompileStatus JS_NEVER_INLINE
mjit::TryCompile(JSContext *cx, JSStackFrame *fp)
{
    JS_ASSERT(cx->fp() == fp);

#if JS_HAS_SHARP_VARS
    if (fp->script()->hasSharps)
        return Compile_Abort;
#endif

    
    if (fp->isConstructing() && !fp->script()->nslots)
        fp->script()->nslots++;

    
    
    CompileStatus status = Compile_Overflow;
    for (unsigned i = 0; status == Compile_Overflow && i < 5; i++) {
        Compiler cc(cx, fp);
        status = cc.compile(NULL);
    }

    return status;
}

bool
mjit::Compiler::loadOldTraps(const Vector<CallSite> &sites)
{
    recompiling = true;

    savedTraps = (bool *)cx->calloc(sizeof(bool) * script->length);
    if (!savedTraps)
        return false;
    
    for (size_t i = 0; i < sites.length(); i++) {
        const CallSite &site = sites[i];
        if (site.isTrap())
            savedTraps[site.pcOffset] = true;
    }

    return true;
}

CompileStatus
mjit::Compiler::generatePrologue()
{
    invokeLabel = masm.label();

    



    if (fun) {
        Jump j = masm.jump();

        



        invokeLabel = masm.label();

        Label fastPath = masm.label();

        
        masm.storePtr(ImmPtr(fun), Address(JSFrameReg, JSStackFrame::offsetOfExec()));

        {
            





            arityLabel = stubcc.masm.label();

            Jump argMatch = stubcc.masm.branch32(Assembler::Equal, JSParamReg_Argc,
                                                 Imm32(fun->nargs));

            if (JSParamReg_Argc != Registers::ArgReg1)
                stubcc.masm.move(JSParamReg_Argc, Registers::ArgReg1);

            
            stubcc.masm.storePtr(ImmPtr(fun), Address(JSFrameReg, JSStackFrame::offsetOfExec()));
            stubcc.masm.storePtr(JSFrameReg, FrameAddress(offsetof(VMFrame, regs.fp)));
            OOL_STUBCALL(stubs::FixupArity);
            stubcc.masm.move(Registers::ReturnReg, JSFrameReg);
            argMatch.linkTo(stubcc.masm.label(), &stubcc.masm);

            
            if (cx->typeInferenceEnabled()) {
#ifdef JS_MONOIC
                this->argsCheckJump = stubcc.masm.jump();
                this->argsCheckStub = stubcc.masm.label();
                this->argsCheckJump.linkTo(this->argsCheckStub, &stubcc.masm);
#endif
                stubcc.masm.storePtr(ImmPtr(fun), Address(JSFrameReg, JSStackFrame::offsetOfExec()));
                OOL_STUBCALL(stubs::CheckArgumentTypes);
#ifdef JS_MONOIC
                this->argsCheckFallthrough = stubcc.masm.label();
#endif
            }

            stubcc.crossJump(stubcc.masm.jump(), fastPath);
        }

        



        masm.addPtr(Imm32((script->nslots + VALUES_PER_STACK_FRAME * 2) * sizeof(Value)),
                    JSFrameReg,
                    Registers::ReturnReg);
        Jump stackCheck = masm.branchPtr(Assembler::AboveOrEqual, Registers::ReturnReg,
                                         FrameAddress(offsetof(VMFrame, stackLimit)));

        
        {
            stubcc.linkExitDirect(stackCheck, stubcc.masm.label());
            OOL_STUBCALL(stubs::HitStackQuota);
            stubcc.crossJump(stubcc.masm.jump(), masm.label());
        }

        




        for (uint32 i = 0; i < script->nfixed; i++) {
            if (analysis->localHasUseBeforeDef(i) || addTraceHints) {
                Address local(JSFrameReg, sizeof(JSStackFrame) + i * sizeof(Value));
                masm.storeValue(UndefinedValue(), local);
            }
        }

        
        if (fun->isHeavyweight()) {
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::GetCallObject);
        }

        j.linkTo(masm.label(), &masm);

        if (analysis->usesScopeChain() && !fun->isHeavyweight()) {
            




            RegisterID t0 = Registers::ReturnReg;
            Jump hasScope = masm.branchTest32(Assembler::NonZero,
                                              FrameFlagsAddress(), Imm32(JSFRAME_HAS_SCOPECHAIN));
            masm.loadPayload(Address(JSFrameReg, JSStackFrame::offsetOfCallee(fun)), t0);
            masm.loadPtr(Address(t0, offsetof(JSObject, parent)), t0);
            masm.storePtr(t0, Address(JSFrameReg, JSStackFrame::offsetOfScopeChain()));
            hasScope.linkTo(masm.label(), &masm);
        }
    }

    if (isConstructing)
        constructThis();

    if (debugMode() || Probes::callTrackingActive(cx))
        INLINE_STUBCALL(stubs::EnterScript);

    



    for (uint32 i = 0; i < script->nfixed; i++) {
        JSValueType type = knownLocalType(i);
        if (type != JSVAL_TYPE_UNKNOWN) {
            JS_ASSERT(!analysis->localHasUseBeforeDef(i));
            
            if (type != JSVAL_TYPE_DOUBLE) {
                Address local(JSFrameReg, sizeof(JSStackFrame) + i * sizeof(Value));
                masm.storeTypeTag(ImmType(type), local);
            }
            frame.learnType(frame.getLocal(i), type, false);
        }
    }

    



    for (uint32 i = 0; fun && i < fun->nargs; i++) {
        JSValueType type = knownArgumentType(i);
        if (type != JSVAL_TYPE_UNKNOWN) {
            if (type == JSVAL_TYPE_DOUBLE) {
                if (!analysis->argEscapes(i))
                    frame.ensureDouble(frame.getArg(i));
            } else {
                frame.learnType(frame.getArg(i), type, false);
            }
        }
    }

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

    for (size_t i = 0; i < branchPatches.length(); i++) {
        Label label = labelOf(branchPatches[i].pc);
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
        if (opinfo && opinfo->safePoint && !liveness.getCode(i).loopBackedge)
            nNmapLive++;
    }

    
    size_t totalBytes = sizeof(JITScript) +
                        sizeof(NativeMapEntry) * nNmapLive +
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
                        sizeof(CallSite) * callSites.length();

    uint8 *cursor = (uint8 *)cx->calloc(totalBytes);
    if (!cursor) {
        execPool->release();
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }

    JITScript *jit = new(cursor) JITScript;
    cursor += sizeof(JITScript);

    jit->code = JSC::MacroAssemblerCodeRef(result, execPool, masm.size() + stubcc.size());
    jit->invokeEntry = result;
    jit->singleStepMode = script->singleStepMode;
    if (fun) {
        jit->arityCheckEntry = stubCode.locationOf(arityLabel).executableAddress();
        jit->fastEntry = fullCode.locationOf(invokeLabel).executableAddress();
    }

    




    
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

#if defined JS_MONOIC
    JS_INIT_CLIST(&jit->callers);

    if (fun && cx->typeInferenceEnabled()) {
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

        jitCallICs[i].pc = callICs[i].pc;
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

        if (patch.hasFastNcode)
            fullCode.patch(patch.fastNcodePatch, fullCode.locationOf(patch.joinPoint));
        if (patch.hasSlowNcode)
            stubCode.patch(patch.slowNcodePatch, fullCode.locationOf(patch.joinPoint));
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

    
    CallSite *jitCallSites = (CallSite *)cursor;
    jit->nCallSites = callSites.length();
    cursor += sizeof(CallSite) * jit->nCallSites;
    for (size_t i = 0; i < jit->nCallSites; i++) {
        CallSite &to = jitCallSites[i];
        InternalCallSite &from = callSites[i];
        uint32 codeOffset = from.ool
                            ? masm.size() + from.returnOffset
                            : from.returnOffset;
        to.initialize(codeOffset, from.pc - script->code, from.id);
    }

    JS_ASSERT(size_t(cursor - (uint8*)jit) == totalBytes);

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
            js_Disassemble1(cx, script, PC, PC - script->code,                \
                            JS_TRUE, stdout);                                 \
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

        frame.setPC(PC);
        frame.setInTryBlock(opinfo->inTryBlock);
        if (opinfo->jumpTarget || trap) {
            if (fallthrough) {
                fixDoubleTypes(Uses(0));

                





                if (liveness.getCode(PC).loopBackedge) {
                    frame.syncAndForgetEverything();
                    Jump j = masm.jump();
                    if (!frame.pushLoop(PC, j, PC))
                        return Compile_Error;
                } else {
                    if (!frame.syncForBranch(PC, Uses(0)))
                        return Compile_Error;
                    JS_ASSERT(frame.consistentRegisters(PC));
                }
            }

            if (!frame.discardForJoin(PC, opinfo->stackDepth))
                return Compile_Error;
            restoreAnalysisTypes(opinfo->stackDepth);
            fallthrough = true;
        }

        jumpMap[uint32(PC - script->code)] = masm.label();

        SPEW_OPCODE();
        JS_ASSERT(frame.stackDepth() == opinfo->stackDepth);

        if (trap) {
            prepareStubCall(Uses(0));
            masm.move(Imm32(trap), Registers::ArgReg1);
            Call cl = emitStubCall(JS_FUNC_TO_DATA_PTR(void *, stubs::Trap));
            InternalCallSite site(masm.callReturnOffset(cl), PC,
                                  CallSite::MAGIC_TRAP_ID, true, false);
            addCallSite(site);
        } else if (savedTraps && savedTraps[PC - script->code]) {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            uint32 offset = stubcc.masm.distanceOf(stubcc.masm.label());
            if (Assembler::ReturnStackAdjustment) {
                stubcc.masm.addPtr(Imm32(Assembler::ReturnStackAdjustment),
                                   Assembler::stackPointerRegister);
            }
            stubcc.crossJump(stubcc.masm.jump(), masm.label());

            InternalCallSite site(offset, PC, CallSite::MAGIC_TRAP_ID, false, true);
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
            masm.or32(Imm32(JSFRAME_HAS_RVAL), reg);
            masm.store32(reg, FrameFlagsAddress());
            frame.freeReg(reg);

            FrameEntry *fe = frame.peek(-1);
            frame.storeTo(fe, Address(JSFrameReg, JSStackFrame::offsetOfReturnValue()), true);
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
            jsbytecode *target = PC + GET_JUMP_OFFSET(PC);
            fixDoubleTypes(Uses(0));

            





            if (target < PC) {
                if (JSOp(*target) == JSOP_GOTO) {
                    target = target + GET_JUMP_OFFSET(target);
                    JS_ASSERT(target >= PC);
                } else if (JSOp(*target) == JSOP_GOTOX) {
                    return Compile_Abort;
                }
            }

            



            jsbytecode *next = PC + JSOP_GOTO_LENGTH;
            if (analysis->maybeCode(next) && liveness.getCode(next).loopBackedge) {
                frame.syncAndForgetEverything();
                Jump j = masm.jump();
                if (!frame.pushLoop(next, j, target))
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
            fixDoubleTypes(Uses(1));
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
            uint32 arg = GET_SLOTNO(PC);
            iterNext();
            frame.storeArg(arg, knownArgumentType(arg), true);
            frame.pop();
          }
          END_CASE(JSOP_FORARG)

          BEGIN_CASE(JSOP_FORLOCAL)
          {
            uint32 slot = GET_SLOTNO(PC);
            iterNext();
            frame.storeLocal(slot, knownLocalType(slot), true, true);
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
                fixDoubleTypes(Uses(2));
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
                            fixDoubleTypes(Uses(0));
                            if (!frame.syncForBranch(target, Uses(0)))
                                return Compile_Error;
                            Jump j = masm.jump();
                            if (!jumpAndTrace(j, target))
                                return Compile_Error;
                        } else {
                            



                            if (target < PC && !finishLoop(target))
                                return Compile_Error;
                        }
                    }
                } else {
                    if (!emitStubCmpOp(stub, target, fused))
                        return Compile_Error;
                }
            } else {
                
                if (!jsop_relational(op, stub, target, fused))
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
            jsop_rsh();
          END_CASE(JSOP_RSH)

          BEGIN_CASE(JSOP_URSH)
            jsop_bitop(op);
          END_CASE(JSOP_URSH)

          BEGIN_CASE(JSOP_ADD)
            if (!jsop_binary(op, stubs::Add, knownPushedType(0))) {
                markPushedOverflow();
                return Compile_Overflow;
            }
          END_CASE(JSOP_ADD)

          BEGIN_CASE(JSOP_SUB)
            if (!jsop_binary(op, stubs::Sub, knownPushedType(0))) {
                markPushedOverflow();
                return Compile_Overflow;
            }
          END_CASE(JSOP_SUB)

          BEGIN_CASE(JSOP_MUL)
            if (!jsop_binary(op, stubs::Mul, knownPushedType(0))) {
                markPushedOverflow();
                return Compile_Overflow;
            }
          END_CASE(JSOP_MUL)

          BEGIN_CASE(JSOP_DIV)
            if (!jsop_binary(op, stubs::Div, knownPushedType(0))) {
                markPushedOverflow();
                return Compile_Overflow;
            }
          END_CASE(JSOP_DIV)

          BEGIN_CASE(JSOP_MOD)
            if (!jsop_mod())
                return Compile_Overflow;
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
            FrameEntry *top = frame.peek(-1);
            if (top->isConstant() && top->getValue().isPrimitive()) {
                double d;
                ValueToNumber(cx, top->getValue(), &d);
                d = -d;
                Value v = NumberValue(d);

                
                if (!v.isInt32() && knownPushedType(0) == JSVAL_TYPE_INT32) {
                    JaegerSpew(JSpew_Abort, "overflow in negation (%u)\n", PC - script->code);
                    markPushedOverflow();
                    return Compile_Overflow;
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
            prepareStubCall(Uses(2));
            INLINE_STUBCALL(STRICT_VARIANT(stubs::DelElem));
            frame.popn(2);
            pushSyncedEntry(0);
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
            if (!jsop_nameinc(op, STRICT_VARIANT(stubs::IncName), fullAtomIndex(PC)))
                return Compile_Error;
            break;
          END_CASE(JSOP_INCNAME)

          BEGIN_CASE(JSOP_INCGNAME)
            jsop_gnameinc(op, STRICT_VARIANT(stubs::IncGlobalName), fullAtomIndex(PC));
            break;
          END_CASE(JSOP_INCGNAME)

          BEGIN_CASE(JSOP_INCPROP)
            if (!jsop_propinc(op, STRICT_VARIANT(stubs::IncProp), fullAtomIndex(PC)))
                return Compile_Error;
            break;
          END_CASE(JSOP_INCPROP)

          BEGIN_CASE(JSOP_INCELEM)
            jsop_eleminc(op, STRICT_VARIANT(stubs::IncElem));
          END_CASE(JSOP_INCELEM)

          BEGIN_CASE(JSOP_DECNAME)
            if (!jsop_nameinc(op, STRICT_VARIANT(stubs::DecName), fullAtomIndex(PC)))
                return Compile_Error;
            break;
          END_CASE(JSOP_DECNAME)

          BEGIN_CASE(JSOP_DECGNAME)
            jsop_gnameinc(op, STRICT_VARIANT(stubs::DecGlobalName), fullAtomIndex(PC));
            break;
          END_CASE(JSOP_DECGNAME)

          BEGIN_CASE(JSOP_DECPROP)
            if (!jsop_propinc(op, STRICT_VARIANT(stubs::DecProp), fullAtomIndex(PC)))
                return Compile_Error;
            break;
          END_CASE(JSOP_DECPROP)

          BEGIN_CASE(JSOP_DECELEM)
            jsop_eleminc(op, STRICT_VARIANT(stubs::DecElem));
          END_CASE(JSOP_DECELEM)

          BEGIN_CASE(JSOP_NAMEINC)
            if (!jsop_nameinc(op, STRICT_VARIANT(stubs::NameInc), fullAtomIndex(PC)))
                return Compile_Error;
            break;
          END_CASE(JSOP_NAMEINC)

          BEGIN_CASE(JSOP_GNAMEINC)
            jsop_gnameinc(op, STRICT_VARIANT(stubs::GlobalNameInc), fullAtomIndex(PC));
            break;
          END_CASE(JSOP_GNAMEINC)

          BEGIN_CASE(JSOP_PROPINC)
            if (!jsop_propinc(op, STRICT_VARIANT(stubs::PropInc), fullAtomIndex(PC)))
                return Compile_Error;
            break;
          END_CASE(JSOP_PROPINC)

          BEGIN_CASE(JSOP_ELEMINC)
            jsop_eleminc(op, STRICT_VARIANT(stubs::ElemInc));
          END_CASE(JSOP_ELEMINC)

          BEGIN_CASE(JSOP_NAMEDEC)
            if (!jsop_nameinc(op, STRICT_VARIANT(stubs::NameDec), fullAtomIndex(PC)))
                return Compile_Error;
            break;
          END_CASE(JSOP_NAMEDEC)

          BEGIN_CASE(JSOP_GNAMEDEC)
            jsop_gnameinc(op, STRICT_VARIANT(stubs::GlobalNameDec), fullAtomIndex(PC));
            break;
          END_CASE(JSOP_GNAMEDEC)

          BEGIN_CASE(JSOP_PROPDEC)
            if (!jsop_propinc(op, STRICT_VARIANT(stubs::PropDec), fullAtomIndex(PC)))
                return Compile_Error;
            break;
          END_CASE(JSOP_PROPDEC)

          BEGIN_CASE(JSOP_ELEMDEC)
            jsop_eleminc(op, STRICT_VARIANT(stubs::ElemDec));
          END_CASE(JSOP_ELEMDEC)

          BEGIN_CASE(JSOP_GETTHISPROP)
            
            jsop_this();
            if (!jsop_getprop(script->getAtom(fullAtomIndex(PC)), knownPushedType(0)))
                return Compile_Error;
          END_CASE(JSOP_GETTHISPROP);

          BEGIN_CASE(JSOP_GETARGPROP)
          {
            
            uint32 arg = GET_SLOTNO(PC);
            frame.pushArg(arg, knownArgumentType(arg));
            if (!jsop_getprop(script->getAtom(fullAtomIndex(&PC[ARGNO_LEN])), knownPushedType(0)))
                return Compile_Error;
          }
          END_CASE(JSOP_GETARGPROP)

          BEGIN_CASE(JSOP_GETLOCALPROP)
          {
            uint32 local = GET_SLOTNO(PC);
            frame.pushLocal(local, knownLocalType(local));
            if (!jsop_getprop(script->getAtom(fullAtomIndex(&PC[SLOTNO_LEN])), knownPushedType(0)))
                return Compile_Error;
          }
          END_CASE(JSOP_GETLOCALPROP)

          BEGIN_CASE(JSOP_GETPROP)
            if (!jsop_getprop(script->getAtom(fullAtomIndex(PC)), knownPushedType(0)))
                return Compile_Error;
          END_CASE(JSOP_GETPROP)

          BEGIN_CASE(JSOP_LENGTH)
            if (!jsop_length())
                return Compile_Error;
          END_CASE(JSOP_LENGTH)

          BEGIN_CASE(JSOP_GETELEM)
            if (!jsop_getelem(false))
                return Compile_Error;
          END_CASE(JSOP_GETELEM)

          BEGIN_CASE(JSOP_SETELEM)
          {
            jsbytecode *next = &PC[JSOP_SETELEM_LENGTH];
            bool pop = (JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next));
            if (!jsop_setelem(pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETELEM);

          BEGIN_CASE(JSOP_CALLNAME)
            prepareStubCall(Uses(0));
            masm.move(Imm32(fullAtomIndex(PC)), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::CallName);
            pushSyncedEntry(0);
            pushSyncedEntry(1);
          END_CASE(JSOP_CALLNAME)

          BEGIN_CASE(JSOP_EVAL)
          {
            JaegerSpew(JSpew_Insns, " --- EVAL --- \n");
            emitEval(GET_ARGC(PC));
            JaegerSpew(JSpew_Insns, " --- END EVAL --- \n");
          }
          END_CASE(JSOP_EVAL)

          BEGIN_CASE(JSOP_CALL)
          BEGIN_CASE(JSOP_FUNAPPLY)
          BEGIN_CASE(JSOP_FUNCALL)
          {
            JaegerSpew(JSpew_Insns, " --- SCRIPTED CALL --- \n");
            inlineCallHelper(GET_ARGC(PC), false);
            JaegerSpew(JSpew_Insns, " --- END SCRIPTED CALL --- \n");
          }
          END_CASE(JSOP_CALL)

          BEGIN_CASE(JSOP_NAME)
            jsop_name(script->getAtom(fullAtomIndex(PC)), knownPushedType(0));
          END_CASE(JSOP_NAME)

          BEGIN_CASE(JSOP_DOUBLE)
          {
            uint32 index = fullAtomIndex(PC);
            double d = script->getConst(index).toDouble();
            frame.push(Value(DoubleValue(d)));
          }
          END_CASE(JSOP_DOUBLE)

          BEGIN_CASE(JSOP_STRING)
          {
            JSAtom *atom = script->getAtom(fullAtomIndex(PC));
            JSString *str = ATOM_TO_STRING(atom);
            frame.push(Value(StringValue(str)));
          }
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
            fixDoubleTypes(Uses(0));
            if (!jsop_andor(op, PC + GET_JUMP_OFFSET(PC)))
                return Compile_Error;
          END_CASE(JSOP_AND)

          BEGIN_CASE(JSOP_TABLESWITCH)
            




            fixDoubleTypes(Uses(1));
#if defined JS_CPU_ARM 
            frame.syncAndForgetEverything();
            masm.move(ImmPtr(PC), Registers::ArgReg1);

            
            INLINE_STUBCALL(stubs::TableSwitch);
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
            fixDoubleTypes(Uses(1));
            frame.syncAndForgetEverything();
            masm.move(ImmPtr(PC), Registers::ArgReg1);

            
            INLINE_STUBCALL(stubs::LookupSwitch);
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
            
            if (!iterMore())
                return Compile_Error;
            break;
          END_CASE(JSOP_MOREITER)

          BEGIN_CASE(JSOP_ENDITER)
            iterEnd();
          END_CASE(JSOP_ENDITER)

          BEGIN_CASE(JSOP_POP)
            frame.pop();
          END_CASE(JSOP_POP)

          BEGIN_CASE(JSOP_NEW)
          {
            JaegerSpew(JSpew_Insns, " --- NEW OPERATOR --- \n");
            inlineCallHelper(GET_ARGC(PC), true);
            JaegerSpew(JSpew_Insns, " --- END NEW OPERATOR --- \n");
          }
          END_CASE(JSOP_NEW)

          BEGIN_CASE(JSOP_GETARG)
          BEGIN_CASE(JSOP_CALLARG)
          {
            uint32 arg = GET_SLOTNO(PC);
            frame.pushArg(arg, knownArgumentType(arg));
            if (op == JSOP_CALLARG)
                frame.push(UndefinedValue());
          }
          END_CASE(JSOP_GETARG)

          BEGIN_CASE(JSOP_BINDGNAME)
            jsop_bindgname();
          END_CASE(JSOP_BINDGNAME)

          BEGIN_CASE(JSOP_SETARG)
          {
            uint32 arg = GET_SLOTNO(PC);
            jsbytecode *next = &PC[JSOP_SETLOCAL_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            frame.storeArg(arg, knownArgumentType(arg), pop);
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
            frame.pushLocal(slot, knownPushedType(0));
          }
          END_CASE(JSOP_GETLOCAL)

          BEGIN_CASE(JSOP_SETLOCAL)
          {
            uint32 slot = GET_SLOTNO(PC);
            jsbytecode *next = &PC[JSOP_SETLOCAL_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            frame.storeLocal(slot, knownLocalType(slot), pop, true);
            if (pop) {
                frame.pop();
                PC += JSOP_SETLOCAL_LENGTH + JSOP_POP_LENGTH;
                break;
            }
          }
          END_CASE(JSOP_SETLOCAL)

          BEGIN_CASE(JSOP_SETLOCALPOP)
          {
            uint32 slot = GET_SLOTNO(PC);
            frame.storeLocal(slot, knownLocalType(slot), true, true);
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
                return Compile_Overflow;
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
                return Compile_Overflow;
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
            if (!jsop_setprop(script->getAtom(fullAtomIndex(PC)), true))
                return Compile_Error;
          END_CASE(JSOP_SETPROP)

          BEGIN_CASE(JSOP_SETNAME)
          BEGIN_CASE(JSOP_SETMETHOD)
            if (!jsop_setprop(script->getAtom(fullAtomIndex(PC)), true))
                return Compile_Error;
          END_CASE(JSOP_SETNAME)

          BEGIN_CASE(JSOP_THROW)
            prepareStubCall(Uses(1));
            INLINE_STUBCALL(stubs::Throw);
            frame.pop();
          END_CASE(JSOP_THROW)

          BEGIN_CASE(JSOP_IN)
            prepareStubCall(Uses(2));
            INLINE_STUBCALL(stubs::In);
            frame.popn(2);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
          END_CASE(JSOP_IN)

          BEGIN_CASE(JSOP_INSTANCEOF)
            if (!jsop_instanceof())
                return Compile_Error;
          END_CASE(JSOP_INSTANCEOF)

          BEGIN_CASE(JSOP_EXCEPTION)
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::Exception);
            frame.pushSynced(JSVAL_TYPE_UNKNOWN);
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
            uint32 index = fullAtomIndex(PC);
            JSFunction *innerFun = script->getFunction(index);

            if (fun && script->bindings.hasBinding(cx, innerFun->atom))
                frame.syncAndForgetEverything();

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(innerFun), Registers::ArgReg1);
            INLINE_STUBCALL(STRICT_VARIANT(stubs::DefFun));
          }
          END_CASE(JSOP_DEFFUN)

          BEGIN_CASE(JSOP_DEFVAR)
          BEGIN_CASE(JSOP_DEFCONST)
          {
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DefVarOrConst);
          }
          END_CASE(JSOP_DEFVAR)

          BEGIN_CASE(JSOP_SETCONST)
          {
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            if (fun && script->bindings.hasBinding(cx, atom))
                frame.syncAndForgetEverything();

            prepareStubCall(Uses(1));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::SetConst);
          }
          END_CASE(JSOP_SETCONST)

          BEGIN_CASE(JSOP_DEFLOCALFUN_FC)
          {
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
                INLINE_STUBCALL(stub);
            } else {
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

            
            masm.loadPtr(Address(reg, offsetof(JSObject, slots)), reg);
            Address upvarAddress(reg, JSObject::JSSLOT_FLAT_CLOSURE_UPVARS * sizeof(Value));
            masm.loadPrivate(upvarAddress, reg);
            
            frame.freeReg(reg);
            frame.push(Address(reg, index * sizeof(Value)), knownPushedType(0));
            if (op == JSOP_CALLFCSLOT)
                frame.push(UndefinedValue());
          }
          END_CASE(JSOP_CALLFCSLOT)

          BEGIN_CASE(JSOP_ARGSUB)
            prepareStubCall(Uses(0));
            masm.move(Imm32(GET_ARGNO(PC)), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::ArgSub);
            pushSyncedEntry(0);
          END_CASE(JSOP_ARGSUB)

          BEGIN_CASE(JSOP_ARGCNT)
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::ArgCnt);
            pushSyncedEntry(0);
          END_CASE(JSOP_ARGCNT)

          BEGIN_CASE(JSOP_DEFLOCALFUN)
          {
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
            jsop_getgname(fullAtomIndex(PC), knownPushedType(0));
            if (op == JSOP_CALLGNAME)
                jsop_callgname_epilogue();
          END_CASE(JSOP_GETGNAME)

          BEGIN_CASE(JSOP_SETGNAME)
            jsop_setgname(script->getAtom(fullAtomIndex(PC)), true);
          END_CASE(JSOP_SETGNAME)

          BEGIN_CASE(JSOP_REGEXP)
          {
            JSObject *regex = script->getRegExp(fullAtomIndex(PC));
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(regex), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::RegExp);
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
            if (!jsop_callprop(script->getAtom(fullAtomIndex(PC))))
                return Compile_Error;
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
            frame.pushLocal(slot, knownPushedType(0));
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
            INLINE_STUBCALL(stubs::FlatLambda);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
          }
          END_CASE(JSOP_LAMBDA_FC)

          BEGIN_CASE(JSOP_TRACE)
          BEGIN_CASE(JSOP_NOTRACE)
          {
            if (analysis->jumpTarget(PC))
                interruptCheckHelper();
          }
          END_CASE(JSOP_TRACE)

          BEGIN_CASE(JSOP_DEBUGGER)
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(PC), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::Debugger);
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

    

 

        if (cx->typeInferenceEnabled()) {
            
            unsigned nuses = analyze::GetUseCount(script, oldPC - script->code);
            unsigned ndefs = analyze::GetDefCount(script, oldPC - script->code);
            for (unsigned i = 0; i < ndefs; i++) {
                frame.learnTypeSet(opinfo->stackDepth - nuses + i,
                                   script->types->pushed(oldPC - script->code, i));
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
mjit::Compiler::labelOf(jsbytecode *pc)
{
    uint32 offs = uint32(pc - script->code);
    JS_ASSERT(jumpMap[offs].isValid());
    return jumpMap[offs];
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

void *
mjit::Compiler::findCallSite(const CallSite &callSite)
{
    JS_ASSERT(callSite.pcOffset < script->length);

    JITScript *jit = script->getJIT(fp->isConstructing());
    uint8* ilPath = (uint8 *)jit->code.m_code.executableAddress();
    uint8* oolPath = ilPath + masm.size();

    for (uint32 i = 0; i < callSites.length(); i++) {
        InternalCallSite &cs = callSites[i];
        if (cs.pc == script->code + callSite.pcOffset && cs.id == callSite.id) {
#ifdef DEBUG
            for (i++; i < callSites.length(); i++)
                JS_ASSERT(cs.pc != callSites[i].pc || cs.id != callSites[i].id);
#endif
            if (cs.ool)
                return oolPath + cs.returnOffset;
            return ilPath + cs.returnOffset;
        }
    }

    
    JS_NOT_REACHED("Call site vanished.");
    return NULL;
}

bool
mjit::Compiler::jumpInScript(Jump j, jsbytecode *pc)
{
    JS_ASSERT(pc >= script->code && uint32(pc - script->code) < script->length);

    if (pc < PC) {
        j.linkTo(jumpMap[uint32(pc - script->code)], &masm);
        return true;
    }
    return branchPatches.append(BranchPatch(j, pc));
}

void
mjit::Compiler::jsop_getglobal(uint32 index)
{
    JS_ASSERT(globalObj);
    uint32 slot = script->getGlobalSlot(index);

    RegisterID reg = frame.allocReg();
    Address address = masm.objSlotRef(globalObj, reg, slot);
    frame.push(address, knownPushedType(0));
    frame.freeReg(reg);

    



    if (globalObj->getSlot(slot).isUndefined() &&
        (JSOp(*PC) == JSOP_CALLGLOBAL || PC[JSOP_GETGLOBAL_LENGTH] != JSOP_POP)) {
        Jump jump = masm.testUndefined(Assembler::Equal, address);
        stubcc.linkExit(jump, Uses(0));
        stubcc.leave();
        OOL_STUBCALL(stubs::UndefinedHelper);
        stubcc.rejoin(Changes(0));
    }
}

void
mjit::Compiler::emitFinalReturn(Assembler &masm)
{
    masm.loadPtr(Address(JSFrameReg, JSStackFrame::offsetOfncode()), Registers::ReturnReg);
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
                                               Imm32(JSFRAME_HAS_RVAL));
            Address rvalAddress(JSFrameReg, JSStackFrame::offsetOfReturnValue());
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
    Address thisv(JSFrameReg, JSStackFrame::offsetOfThis(fun));

    
    
    
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
mjit::Compiler::emitReturn(FrameEntry *fe)
{
    JS_ASSERT_IF(!fun, JSOp(*PC) == JSOP_STOP);

    
    JS_ASSERT_IF(fe, fe == frame.peek(-1));

    if (debugMode() || Probes::callTrackingActive(cx)) {
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::LeaveScript);
    }

    








    if (fun) {
        if (fun->isHeavyweight()) {
            
            prepareStubCall(Uses(fe ? 1 : 0));
            INLINE_STUBCALL(stubs::PutActivationObjects);
        } else {
            
            Jump putObjs = masm.branchTest32(Assembler::NonZero,
                                             Address(JSFrameReg, JSStackFrame::offsetOfFlags()),
                                             Imm32(JSFRAME_HAS_CALL_OBJ | JSFRAME_HAS_ARGS_OBJ));
            stubcc.linkExit(putObjs, Uses(frame.frameSlots()));

            stubcc.leave();
            OOL_STUBCALL(stubs::PutActivationObjects);

            emitReturnValue(&stubcc.masm, fe);
            emitFinalReturn(stubcc.masm);
        }
    } else {
        if (fp->isEvalFrame() && script->strictModeCode) {
            
            prepareStubCall(Uses(fe ? 1 : 0));
            INLINE_STUBCALL(stubs::PutStrictEvalCallObject);
        }
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
mjit::Compiler::emitStubCall(void *ptr)
{
    JaegerSpew(JSpew_Insns, " ---- CALLING STUB ---- \n");
    Call cl = masm.fallibleVMCall(ptr, PC, frame.stackDepth() + script->nfixed);
    JaegerSpew(JSpew_Insns, " ---- END STUB CALL ---- \n");
    return cl;
}

void
mjit::Compiler::interruptCheckHelper()
{
    






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
mjit::Compiler::addReturnSite(Label joinPoint)
{
    InternalCallSite site(masm.distanceOf(joinPoint), PC, CallSite::NCODE_RETURN_ID, false, false);
    addCallSite(site);
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

    if (recompiling) {
        
        OOL_STUBCALL(JS_FUNC_TO_DATA_PTR(void *, callingNew ? ic::New : ic::Call));
        stubcc.crossJump(stubcc.masm.jump(), masm.label());
    }

    Jump notCompiled = masm.branchTestPtr(Assembler::Zero, r0, r0);

    masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.fp)), JSFrameReg);
    callPatch.hasFastNcode = true;
    callPatch.fastNcodePatch =
        masm.storePtrWithPatch(ImmPtr(NULL),
                               Address(JSFrameReg, JSStackFrame::offsetOfncode()));

    masm.jump(r0);
    callPatch.joinPoint = masm.label();
    addReturnSite(callPatch.joinPoint);
    masm.loadPtr(Address(JSFrameReg, JSStackFrame::offsetOfPrev()), JSFrameReg);

    frame.popn(argc + 2);

    frame.takeReg(JSReturnReg_Type);
    frame.takeReg(JSReturnReg_Data);
    frame.pushRegs(JSReturnReg_Type, JSReturnReg_Data, knownPushedType(0));

    if (recompiling) {
        
        OOL_STUBCALL(JS_FUNC_TO_DATA_PTR(void *, callingNew ? ic::NativeNew : ic::NativeCall));
    }

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
            OOL_STUBCALL(stubs::Arguments);
            frameDepthAdjust = +1;
        } else {
            frameDepthAdjust = 0;
        }

        stubcc.masm.move(Imm32(callImmArgc), Registers::ArgReg1);
        JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW CALL CODE ---- \n");
        OOL_STUBCALL_LOCAL_SLOTS(JS_FUNC_TO_DATA_PTR(void *, stubs::UncachedCall),
                           frame.localSlots() + frameDepthAdjust);
        JaegerSpew(JSpew_Insns, " ---- END SLOW CALL CODE ---- \n");

        RegisterID r0 = Registers::ReturnReg;
        Jump notCompiled = stubcc.masm.branchTestPtr(Assembler::Zero, r0, r0);

        stubcc.masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.fp)), JSFrameReg);
        Address ncodeAddr(JSFrameReg, JSStackFrame::offsetOfncode());
        uncachedCallPatch->hasSlowNcode = true;
        uncachedCallPatch->slowNcodePatch = stubcc.masm.storePtrWithPatch(ImmPtr(NULL), ncodeAddr);

        stubcc.masm.jump(r0);
        notCompiled.linkTo(stubcc.masm.label(), &stubcc.masm);

        




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
           !debugMode();
}


bool
mjit::Compiler::inlineCallHelper(uint32 callImmArgc, bool callingNew)
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

    






    bool lowerFunCallOrApply = IsLowerableFunCallOrApply(PC);

    bool newType = callingNew && cx->typeInferenceEnabled() && types::UseNewType(cx, script, PC);

    



#ifdef JS_MONOIC
    if (debugMode() || newType ||
        origCallee->isConstant() || origCallee->isNotType(JSVAL_TYPE_OBJECT) ||
        (lowerFunCallOrApply &&
         (origThis->isConstant() || origThis->isNotType(JSVAL_TYPE_OBJECT)))) {
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

    
    CallGenInfo     callIC(PC);
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
                callIC.frameSize.initStatic(frame.localSlots(), speculatedArgc - 1);
            else
                callIC.frameSize.initDynamic();
        } else {
            
            frame.syncAndKill(Uses(speculatedArgc + 2));

            icCalleeType = origCalleeType;
            icCalleeData = origCalleeData;
            icRvalAddr = frame.addressOf(origCallee);
            callIC.frameSize.initStatic(frame.localSlots(), speculatedArgc);
        }
    }

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
        types::TypeSet *types = frame.getTypeSet(frame.peek(-(argc + 1)));
        types::TypeSet::Clone(cx, script, types, &callIC.argTypes[0]);
        for (unsigned i = 0; i < argc; i++) {
            types::TypeSet *types = frame.getTypeSet(frame.peek(-(argc - i)));
            types::TypeSet::Clone(cx, script, types, &callIC.argTypes[i + 1]);
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

        




        if (callIC.frameSize.isDynamic())
            OOL_STUBCALL(ic::SplatApplyArgs);

        



        Jump toPatch = stubcc.masm.jump();
        toPatch.linkTo(stubcc.masm.label(), &stubcc.masm);
        callIC.oolJump = toPatch;
        callIC.icCall = stubcc.masm.label();

        




        callIC.addrLabel1 = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
        void *icFunPtr = JS_FUNC_TO_DATA_PTR(void *, callingNew ? ic::New : ic::Call);
        if (callIC.frameSize.isStatic())
            callIC.oolCall = OOL_STUBCALL_LOCAL_SLOTS(icFunPtr, frame.localSlots());
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
        stubcc.masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.fp)), JSFrameReg);
        callPatch.hasSlowNcode = true;
        callPatch.slowNcodePatch =
            stubcc.masm.storePtrWithPatch(ImmPtr(NULL),
                                          Address(JSFrameReg, JSStackFrame::offsetOfncode()));
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
        flags |= JSFRAME_CONSTRUCTING;

    InlineFrameAssembler inlFrame(masm, callIC, flags);
    callPatch.hasFastNcode = true;
    callPatch.fastNcodePatch = inlFrame.assemble(NULL);

    callIC.hotJump = masm.jump();
    callIC.joinPoint = callPatch.joinPoint = masm.label();
    addReturnSite(callPatch.joinPoint);
    if (lowerFunCallOrApply)
        uncachedCallPatch.joinPoint = callIC.joinPoint;
    masm.loadPtr(Address(JSFrameReg, JSStackFrame::offsetOfPrev()), JSFrameReg);

    



    CHECK_IC_SPACE();

    JSValueType type = knownPushedType(0);

    frame.popn(speculatedArgc + 2);
    frame.takeReg(JSReturnReg_Type);
    frame.takeReg(JSReturnReg_Data);
    FPRegisterID fpreg = frame.pushRegs(JSReturnReg_Type, JSReturnReg_Data, type);

    





    callIC.slowJoinPoint = stubcc.masm.label();
    rejoin1.linkTo(callIC.slowJoinPoint, &stubcc.masm);
    rejoin2.linkTo(callIC.slowJoinPoint, &stubcc.masm);
    JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW RESTORE CODE ---- \n");
    if (type == JSVAL_TYPE_DOUBLE)
        stubcc.masm.moveInt32OrDouble(icRvalAddr, fpreg);
    else
        stubcc.masm.loadValueAsComponents(icRvalAddr, JSReturnReg_Type, JSReturnReg_Data);
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






void
mjit::Compiler::addCallSite(const InternalCallSite &site)
{
#if 1 
    for (unsigned i = 0; i < callSites.length(); i++)
        JS_ASSERT(site.pc != callSites[i].pc || site.id != callSites[i].id);
#endif
    callSites.append(site);
}

void
mjit::Compiler::restoreFrameRegs(Assembler &masm)
{
    masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.fp)), JSFrameReg);
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
mjit::Compiler::emitStubCmpOp(BoolStub stub, jsbytecode *target, JSOp fused)
{
    fixDoubleTypes(Uses(2));
    if (target)
        frame.syncAndForgetEverything();
    else
        frame.syncAndKill(Uses(2));

    prepareStubCall(Uses(2));
    INLINE_STUBCALL(stub);
    frame.pop();
    frame.pop();

    if (!target) {
        frame.takeReg(Registers::ReturnReg);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
        return true;
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

bool
mjit::Compiler::jsop_length()
{
    FrameEntry *top = frame.peek(-1);

    if (top->isTypeKnown() && top->getKnownType() == JSVAL_TYPE_STRING) {
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

#if defined JS_POLYIC
    return jsop_getprop(cx->runtime->atomState.lengthAtom, knownPushedType(0));
#else
    prepareStubCall(Uses(1));
    INLINE_STUBCALL(stubs::Length);
    frame.pop();
    pushSyncedEntry(0);
    return true;
#endif
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
    FrameEntry *top = frame.peek(-1);

    
    if (top->isTypeKnown() && top->getKnownType() != JSVAL_TYPE_OBJECT) {
        JS_ASSERT_IF(atom == cx->runtime->atomState.lengthAtom,
                     top->getKnownType() != JSVAL_TYPE_STRING);
        jsop_getprop_slow(atom, usePropCache);
        return true;
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
                                    Imm32(int32(JSObjectMap::INVALID_SHAPE)),
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

    if (recompiling) {
        if (usePropCache)
            OOL_STUBCALL(stubs::GetProp);
        else
            OOL_STUBCALL(stubs::GetPropNoCache);
        stubcc.rejoin(Changes(1));
    }

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

    



    uint32 thisvSlot = frame.localSlots();
    Address thisv = Address(JSFrameReg, sizeof(JSStackFrame) + thisvSlot * sizeof(Value));

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
                           Imm32(int32(JSObjectMap::INVALID_SHAPE)),
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

    if (recompiling) {
        OOL_STUBCALL(stubs::CallProp);
        stubcc.rejoin(Changes(2));
    }

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
    if (!js_GetClassPrototype(cx, &fp->scopeChain(), JSProto_String, &obj))
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

    RegisterID objReg = frame.copyDataIntoReg(top);
    RegisterID shapeReg = frame.allocReg();

    pic.shapeReg = shapeReg;
    pic.atom = atom;

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();

    DataLabel32 inlineShapeLabel;
    Jump j = masm.branch32WithPatch(Assembler::NotEqual, shapeReg,
                           Imm32(int32(JSObjectMap::INVALID_SHAPE)),
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
mjit::Compiler::jsop_callprop(JSAtom *atom)
{
    FrameEntry *top = frame.peek(-1);

    
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
mjit::Compiler::jsop_setprop(JSAtom *atom, bool usePropCache)
{
    FrameEntry *lhs = frame.peek(-2);
    FrameEntry *rhs = frame.peek(-1);

    
    if (lhs->isTypeKnown() && lhs->getKnownType() != JSVAL_TYPE_OBJECT) {
        jsop_setprop_slow(atom, usePropCache);
        return true;
    }

    JSOp op = JSOp(*PC);

    ic::PICInfo::Kind kind = (op == JSOP_SETMETHOD)
                             ? ic::PICInfo::SETMETHOD
                             : ic::PICInfo::SET;
    PICGenInfo pic(kind, op, usePropCache);
    pic.atom = atom;

    if (monitored(PC)) {
        types::TypeSet *types = frame.getTypeSet(rhs);
        pic.typeMonitored = true;
        pic.rhsTypes = (types::ClonedTypeSet *) ::js_calloc(sizeof(types::ClonedTypeSet));
        if (!pic.rhsTypes) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        types::TypeSet::Clone(cx, script, types, pic.rhsTypes);
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
                                    Imm32(int32(JSObjectMap::INVALID_SHAPE)),
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

    
    Jump undefinedGuard = masm.testUndefined(Assembler::Equal, pic.shapeReg);

    frame.pushRegs(pic.shapeReg, pic.objReg, type);

    stubcc.rejoin(Changes(1));

    stubcc.linkExit(undefinedGuard, Uses(0));
    stubcc.leave();
    OOL_STUBCALL(stubs::UndefinedHelper);
    stubcc.rejoin(Changes(0));

    pics.append(pic);
}

bool
mjit::Compiler::jsop_xname(JSAtom *atom)
{
    PICGenInfo pic(ic::PICInfo::XNAME, JSOp(*PC), true);

    FrameEntry *fe = frame.peek(-1);
    if (fe->isNotType(JSVAL_TYPE_OBJECT)) {
        return jsop_getprop(atom, knownPushedType(0));
    }

    if (!fe->isTypeKnown()) {
        Jump notObject = frame.testObject(Assembler::NotEqual, fe);
        stubcc.linkExit(notObject, Uses(1));
    }

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

    
    Jump undefinedGuard = masm.testUndefined(Assembler::Equal, pic.shapeReg);

    stubcc.rejoin(Changes(1));

    stubcc.linkExit(undefinedGuard, Uses(0));
    stubcc.leave();
    OOL_STUBCALL(stubs::UndefinedHelper);
    stubcc.rejoin(Changes(0));

    pics.append(pic);
    return true;
}

void
mjit::Compiler::jsop_bindname(JSAtom *atom, bool usePropCache)
{
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
    masm.loadPtr(Address(JSFrameReg, JSStackFrame::offsetOfScopeChain()), pic.objReg);

    pic.shapeGuard = masm.label();
    Jump inlineJump = masm.branchPtr(Assembler::NotEqual, masm.payloadOf(parent), ImmPtr(0));
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
    jsop_getprop_slow(atom, usePropCache);
    return true;
}

bool
mjit::Compiler::jsop_callprop(JSAtom *atom)
{
    return jsop_callprop_slow(atom);
}

bool
mjit::Compiler::jsop_setprop(JSAtom *atom, bool usePropCache)
{
    jsop_setprop_slow(atom, usePropCache);
    return true;
}

void
mjit::Compiler::jsop_bindname(JSAtom *atom, bool usePropCache)
{
    RegisterID reg = frame.allocReg();
    Address scopeChain(JSFrameReg, JSStackFrame::offsetOfScopeChain());
    masm.loadPtr(scopeChain, reg);

    Address address(reg, offsetof(JSObject, parent));

    Jump j = masm.branchPtr(Assembler::NotEqual, masm.payloadOf(address), ImmPtr(0));

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
    frame.pushThis();

    




    if (fun && !script->strictModeCode) {
        FrameEntry *thisFe = frame.peek(-1);
        if (!thisFe->isTypeKnown()) {
            JSValueType type = knownThisType();
            if (type != JSVAL_TYPE_OBJECT) {
                Jump notObj = frame.testObject(Assembler::NotEqual, thisFe);
                stubcc.linkExit(notObj, Uses(1));
                stubcc.leave();
                OOL_STUBCALL(stubs::This);
                stubcc.rejoin(Changes(1));
            }

            
            frame.pop();
            frame.learnThisIsObject();
            frame.pushThis();
        }

        JS_ASSERT(thisFe->isType(JSVAL_TYPE_OBJECT));
    }
}

void
mjit::Compiler::jsop_gnameinc(JSOp op, VoidStubAtom stub, uint32 index)
{
    JSAtom *atom = script->getAtom(index);

#if defined JS_MONOIC
    jsbytecode *next = &PC[JSOP_GNAMEINC_LENGTH];
    bool pop = (JSOp(*next) == JSOP_POP) && !analysis->jumpTarget(next);
    int amt = (op == JSOP_GNAMEINC || op == JSOP_INCGNAME) ? -1 : 1;

    if (pop || (op == JSOP_INCGNAME || op == JSOP_DECGNAME)) {
        

        jsop_getgname(index, JSVAL_TYPE_UNKNOWN);
        

        frame.push(Int32Value(amt));
        

        
        jsop_binary(JSOP_SUB, stubs::Sub, JSVAL_TYPE_UNKNOWN);
        

        jsop_bindgname();
        

        frame.dup2();
        

        frame.shift(-3);
        

        frame.shift(-1);
        

        jsop_setgname(atom, false);
        

        if (pop)
            frame.pop();
    } else {
        

        jsop_getgname(index, JSVAL_TYPE_UNKNOWN);
        

        jsop_pos();
        

        frame.dup();
        

        frame.push(Int32Value(-amt));
        

        jsop_binary(JSOP_ADD, stubs::Add, JSVAL_TYPE_UNKNOWN);
        

        jsop_bindgname();
        

        frame.dup2();
        

        frame.shift(-3);
        

        frame.shift(-1);
        

        jsop_setgname(atom, false);
        

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
}

bool
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
        jsop_binary(JSOP_SUB, stubs::Sub, JSVAL_TYPE_UNKNOWN);
        

        if (!jsop_setprop(atom, false))
            return false;
        

        if (pop)
            frame.pop();
    } else {
        

        jsop_name(atom, JSVAL_TYPE_UNKNOWN);
        

        jsop_pos();
        

        jsop_bindname(atom, false);
        

        frame.dupAt(-2);
        

        frame.push(Int32Value(-amt));
        

        frame.syncAt(-3);
        jsop_binary(JSOP_ADD, stubs::Add, JSVAL_TYPE_UNKNOWN);
        

        if (!jsop_setprop(atom, false))
            return false;
        

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
    return true;
}

bool
mjit::Compiler::jsop_propinc(JSOp op, VoidStubAtom stub, uint32 index)
{
    JSAtom *atom = script->getAtom(index);
#if defined JS_POLYIC
    FrameEntry *objFe = frame.peek(-1);
    if (!objFe->isTypeKnown() || objFe->getKnownType() == JSVAL_TYPE_OBJECT) {
        jsbytecode *next = &PC[JSOP_PROPINC_LENGTH];
        bool pop = (JSOp(*next) == JSOP_POP) && !analysis->jumpTarget(next);
        int amt = (op == JSOP_PROPINC || op == JSOP_INCPROP) ? -1 : 1;

        if (pop || (op == JSOP_INCPROP || op == JSOP_DECPROP)) {
            






            frame.dup();
            

            frame.dup();
            

            if (!jsop_getprop(atom, JSVAL_TYPE_UNKNOWN))
                return false;
            

            frame.push(Int32Value(amt));
            

            
            frame.syncAt(-4);
            jsop_binary(JSOP_SUB, stubs::Sub, JSVAL_TYPE_UNKNOWN);
            

            frame.shimmy(1);
            

            if (!jsop_setprop(atom, false))
                return false;
            

            if (pop)
                frame.pop();
        } else {
            

            frame.dup();
            

            if (!jsop_getprop(atom, JSVAL_TYPE_UNKNOWN))
                return false;
            

            jsop_pos();
            

            frame.dup();
            

            frame.push(Int32Value(-amt));
            

            frame.syncAt(-4);
            jsop_binary(JSOP_ADD, stubs::Add, JSVAL_TYPE_UNKNOWN);
            

            frame.dupAt(-3);
            

            frame.dupAt(-2);
            

            if (!jsop_setprop(atom, false))
                return false;
            

            frame.popn(2);
            

            frame.shimmy(1);
            
        }
        if (pop)
            PC += JSOP_POP_LENGTH;
    } else
#endif
    {
        prepareStubCall(Uses(1));
        masm.move(ImmPtr(atom), Registers::ArgReg1);
        INLINE_STUBCALL(stub);
        frame.pop();
        pushSyncedEntry(0);
    }

    PC += JSOP_PROPINC_LENGTH;
    return true;
}

bool
mjit::Compiler::iter(uintN flags)
{
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
    jsbytecode *target = &PC[JSOP_MOREITER_LENGTH];
    JSOp next = JSOp(*target);
    JS_ASSERT(next == JSOP_IFNE || next == JSOP_IFNEX);

    target += (next == JSOP_IFNE)
              ? GET_JUMP_OFFSET(target)
              : GET_JUMPX_OFFSET(target);

    fixDoubleTypes(Uses(0));
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
    Jump j = stubcc.masm.branchTest32(Assembler::NonZero, Registers::ReturnReg,
                                      Registers::ReturnReg);

    PC += JSOP_MOREITER_LENGTH;
    PC += js_CodeSpec[next].length;

    stubcc.rejoin(Changes(1));
    frame.freeReg(tempreg);

    return jumpAndTrace(jFast, target, &j);
}

void
mjit::Compiler::iterEnd()
{
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
mjit::Compiler::jsop_getgname(uint32 index, JSValueType type)
{
    
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
                                            Imm32(int32(JSObjectMap::INVALID_SHAPE)), ic.shape);
        masm.move(ImmPtr(obj), objReg);
    } else {
        objReg = frame.ownRegForData(fe);
        frame.pop();
        RegisterID reg = frame.allocReg();

        masm.loadShape(objReg, reg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, reg,
                                            Imm32(int32(JSObjectMap::INVALID_SHAPE)), ic.shape);
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
        INLINE_STUBCALL(stubs::PushImplicitThisForGlobal);
        frame.pushSynced(JSVAL_TYPE_UNKNOWN);
        return;
    }

    
    FrameEntry *fval = frame.peek(-1);
    if (fval->isNotType(JSVAL_TYPE_OBJECT)) {
        frame.push(UndefinedValue());
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
    OOL_STUBCALL(stubs::PushImplicitThisForGlobal);

    
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
mjit::Compiler::jsop_setgname(JSAtom *atom, bool usePropertyCache)
{
    if (monitored(PC)) {
        
        jsop_setgname_slow(atom, usePropertyCache);
        return;
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
                                            Imm32(int32(JSObjectMap::INVALID_SHAPE)),
                                            ic.shape);
        masm.move(ImmPtr(obj), ic.objReg);
    } else {
        ic.objReg = frame.copyDataIntoReg(objFe);
        ic.shapeReg = frame.allocReg();
        ic.objConst = false;

        masm.loadShape(ic.objReg, ic.shapeReg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, ic.shapeReg,
                                            Imm32(int32(JSObjectMap::INVALID_SHAPE)),
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

    if (recompiling) {
        OOL_STUBCALL(STRICT_VARIANT(ic::SetElement));
        stubcc.rejoin(Changes(2));
    }
}

void
mjit::Compiler::jsop_getelem_slow()
{
    prepareStubCall(Uses(2));
    INLINE_STUBCALL(stubs::GetElem);
    frame.popn(2);
    pushSyncedEntry(0);

    if (recompiling) {
        OOL_STUBCALL(ic::GetElement);
        stubcc.rejoin(Changes(1));
    }
}

void
mjit::Compiler::jsop_unbrand()
{
    prepareStubCall(Uses(1));
    INLINE_STUBCALL(stubs::Unbrand);
}

bool
mjit::Compiler::jsop_instanceof()
{
    FrameEntry *lhs = frame.peek(-2);
    FrameEntry *rhs = frame.peek(-1);

    
    if (rhs->isNotType(JSVAL_TYPE_OBJECT) || lhs->isNotType(JSVAL_TYPE_OBJECT)) {
        prepareStubCall(Uses(2));
        INLINE_STUBCALL(stubs::InstanceOf);
        frame.popn(2);
        frame.takeReg(Registers::ReturnReg);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
        return true;
    }

    MaybeJump firstSlow;
    if (!rhs->isTypeKnown()) {
        Jump j = frame.testObject(Assembler::NotEqual, rhs);
        stubcc.linkExit(j, Uses(2));
    }

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
        INLINE_STUBCALL(stubs::NewInitArray);
    } else {
        masm.move(ImmPtr(baseobj), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::NewInitObject);
    }
    frame.takeReg(Registers::ReturnReg);
    frame.pushInitializerObject(Registers::ReturnReg, *PC == JSOP_NEWARRAY, baseobj);

    return true;
}

bool
mjit::Compiler::finishLoop(jsbytecode *head)
{
    





    




    Jump fallthrough = masm.jump();

    Jump entry;
    jsbytecode *entryTarget;
    frame.popLoop(head, &entry, &entryTarget);

    if (!jumpInScript(entry, entryTarget))
        return false;

    fallthrough.linkTo(masm.label(), &masm);

    if (!analysis->getCode(head).safePoint) {
        



        LoopEntry entry;
        entry.pcOffset = head - script->code;
        entry.label = stubcc.masm.label();
        loopEntries.append(entry);

        frame.prepareForJump(head, stubcc.masm, true);
        if (!stubcc.jumpInScript(stubcc.masm.jump(), head))
            return false;
    }

    return true;
}
















bool
mjit::Compiler::jumpAndTrace(Jump j, jsbytecode *target, Jump *slow, bool *trampoline)
{
    if (trampoline)
        *trampoline = false;

    



    RegisterAllocation *&lvtarget = liveness.getCode(target).allocation;
    if (!lvtarget) {
        lvtarget = ArenaNew<RegisterAllocation>(liveness.pool, false);
        if (!lvtarget)
            return false;
    }

    bool consistent = frame.consistentRegisters(target);

    if (!addTraceHints || target >= PC ||
        (JSOp(*target) != JSOP_TRACE && JSOp(*target) != JSOP_NOTRACE)
#ifdef JS_MONOIC
        || GET_UINT16(target) == BAD_TRACEIC_INDEX
#endif
        )
    {
        if (lvtarget->synced()) {
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
# endif

    
    {
        jsbytecode* pc = PC;
        PC = target;

        OOL_STUBCALL(stubs::InvokeTracer);

        PC = pc;
    }

    Jump no = stubcc.masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                        Registers::ReturnReg);
    restoreFrameRegs(stubcc.masm);
    stubcc.masm.jump(Registers::ReturnReg);
    no.linkTo(stubcc.masm.label(), &stubcc.masm);

#ifdef JS_MONOIC
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
        restoreFrameRegs(masm);
        interruptCheckHelper();
    }

    uint32 oldFrameDepth = frame.localSlots();

    
    frame.syncAndForgetEverything();
    masm.move(ImmPtr(obj), Registers::ArgReg1);
    uint32 n = js_GetEnterBlockStackDefs(cx, script, PC);
    INLINE_STUBCALL(stubs::EnterBlock);
    frame.enterBlock(n);

    uintN base = JSSLOT_FREE(&js_BlockClass);
    uintN count = OBJ_BLOCK_COUNT(cx, obj);
    uintN limit = base + count;
    for (uintN slot = base, i = 0; slot < limit; slot++, i++) {
        const Value &v = obj->getSlotRef(slot);
        if (v.isBoolean() && v.toBoolean())
            frame.setClosedVar(oldFrameDepth + i);
    }
}

void
mjit::Compiler::leaveBlock()
{
    



    uint32 n = js_GetVariableStackUses(JSOP_LEAVEBLOCK, PC);
    JSObject *obj = script->getObject(fullAtomIndex(PC + UINT16_LEN));
    prepareStubCall(Uses(n));
    masm.move(ImmPtr(obj), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::LeaveBlock);
    frame.leaveBlock(n);
}









bool
mjit::Compiler::constructThis()
{
    JS_ASSERT(isConstructing);

    
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

        
        INLINE_STUBCALL(stubs::TableSwitch);
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
        OOL_STUBCALL(stubs::TableSwitch);
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

    if (recompiling) {
        OOL_STUBCALL(ic::CallElement);
        stubcc.rejoin(Changes(2));
    }
}

void
mjit::Compiler::jsop_forprop(JSAtom *atom)
{
    
    
    frame.dupAt(-2);

    
    
    iterNext();

    
    
    frame.shimmy(1);

    
    
    jsop_setprop(atom, false);

    
    
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

    
    
    jsop_setgname(atom, false);

    
    
    frame.pop();
}








void
mjit::Compiler::fixDoubleTypes(Uses uses)
{
    if (!cx->typeInferenceEnabled())
        return;

    for (uint32 i = 0; fun && i < fun->nargs; i++) {
        JSValueType type = knownArgumentType(i);
        if (type == JSVAL_TYPE_DOUBLE && !analysis->argEscapes(i)) {
            FrameEntry *fe = frame.getArg(i);
            if (!fe->isType(JSVAL_TYPE_DOUBLE))
                frame.ensureDouble(fe);
        }
    }

    for (uint32 i = 0; i < script->nfixed; i++) {
        JSValueType type = knownLocalType(i);
        if (type == JSVAL_TYPE_DOUBLE && !analysis->localEscapes(i)) {
            FrameEntry *fe = frame.getLocal(i);
            if (!fe->isType(JSVAL_TYPE_DOUBLE))
                frame.ensureDouble(fe);
        }
    }
}

void
mjit::Compiler::restoreAnalysisTypes(uint32 stackDepth)
{
    if (!cx->typeInferenceEnabled())
        return;

    
    for (uint32 i = 0; i < script->nfixed; i++) {
        JSValueType type = knownLocalType(i);
        if (type != JSVAL_TYPE_UNKNOWN && (type != JSVAL_TYPE_DOUBLE || !analysis->localEscapes(i))) {
            FrameEntry *fe = frame.getLocal(i);
            JS_ASSERT_IF(fe->isTypeKnown(), fe->isType(type));
            if (!fe->isTypeKnown())
                frame.learnType(fe, type, false);
        }
    }
    for (uint32 i = 0; fun && i < fun->nargs; i++) {
        JSValueType type = knownArgumentType(i);
        if (type != JSVAL_TYPE_UNKNOWN && (type != JSVAL_TYPE_DOUBLE || !analysis->argEscapes(i))) {
            FrameEntry *fe = frame.getArg(i);
            JS_ASSERT_IF(fe->isTypeKnown(), fe->isType(type));
            if (!fe->isTypeKnown())
                frame.learnType(fe, type, false);
        }
    }
}

JSValueType
mjit::Compiler::knownThisType()
{
    if (!cx->typeInferenceEnabled())
        return JSVAL_TYPE_UNKNOWN;
    if (hasThisType)
        return thisType;
    hasThisType = true;
    thisType = script->thisTypes()->getKnownTypeTag(cx, script);
    return thisType;
}

JSValueType
mjit::Compiler::knownArgumentType(uint32 arg)
{
    if (!cx->typeInferenceEnabled())
        return JSVAL_TYPE_UNKNOWN;
    JS_ASSERT(fun && arg < fun->nargs);
    return argumentTypes[arg];
}

void
mjit::Compiler::markArgumentOverflow(uint32 arg)
{
    script->typeSetArgument(cx, arg, DoubleValue(0.0));
}

JSValueType
mjit::Compiler::knownLocalType(uint32 local)
{
    if (!cx->typeInferenceEnabled() || local >= script->nfixed)
        return JSVAL_TYPE_UNKNOWN;
    return localTypes[local];
}

void
mjit::Compiler::markLocalOverflow(uint32 local)
{
    script->typeSetLocal(cx, local, DoubleValue(0.0));
}

JSValueType
mjit::Compiler::knownPushedType(uint32 pushed)
{
    if (!cx->typeInferenceEnabled())
        return JSVAL_TYPE_UNKNOWN;
    types::TypeSet *types = script->types->pushed(PC - script->code, pushed);
    return types->getKnownTypeTag(cx, script);
}

bool
mjit::Compiler::mayPushUndefined(uint32 pushed)
{
    JS_ASSERT(cx->typeInferenceEnabled());

    





    types::TypeSet *types = script->types->pushed(PC - script->code, pushed);
    return types->hasType(types::TYPE_UNDEFINED);
}

types::TypeSet *
mjit::Compiler::argTypeSet(uint32 arg)
{
    return cx->typeInferenceEnabled() ? script->argTypes(arg) : NULL;
}

types::TypeSet *
mjit::Compiler::localTypeSet(uint32 local)
{
    if (!cx->typeInferenceEnabled() || local >= script->nfixed)
        return NULL;
    return script->localTypes(local);
}

bool
mjit::Compiler::monitored(jsbytecode *pc)
{
    return cx->typeInferenceEnabled() && script->types->monitored(pc - script->code);
}

void
mjit::Compiler::pushSyncedEntry(uint32 pushed)
{
    frame.pushSynced(knownPushedType(pushed));
}

void
mjit::Compiler::markPushedOverflow()
{
    
    script->typeMonitorResult(cx, PC, types::TYPE_DOUBLE);
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
    return arrayTypes->knownNonEmpty(cx, script)
        || objectTypes->knownNonEmpty(cx, script);
}
