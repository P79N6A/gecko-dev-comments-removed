








































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
#include "jstypedarrayinlines.h"

using namespace js;
using namespace js::mjit;
#if defined(JS_POLYIC) || defined(JS_MONOIC)
using namespace js::mjit::ic;
#endif
using namespace js::analyze;

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





static const size_t USES_BEFORE_INLINING = 10000;

mjit::Compiler::Compiler(JSContext *cx, JSScript *outerScript, bool isConstructing)
  : BaseCompiler(cx),
    outerScript(outerScript),
    isConstructing(isConstructing),
    ssa(cx, outerScript),
    globalObj(outerScript->hasGlobal() ? outerScript->global() : NULL),
    globalSlots(globalObj ? globalObj->getRawSlots() : NULL),
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
    doubleList(CompilerAllocPolicy(cx, *thisFromCtor())),
    fixedIntToDoubleEntries(CompilerAllocPolicy(cx, *thisFromCtor())),
    fixedDoubleToAnyEntries(CompilerAllocPolicy(cx, *thisFromCtor())),
    jumpTables(CompilerAllocPolicy(cx, *thisFromCtor())),
    jumpTableOffsets(CompilerAllocPolicy(cx, *thisFromCtor())),
    loopEntries(CompilerAllocPolicy(cx, *thisFromCtor())),
    stubcc(cx, *thisFromCtor(), frame),
    debugMode_(cx->compartment->debugMode()),
#if defined JS_TRACER
    addTraceHints(cx->traceJitEnabled),
#else
    addTraceHints(false),
#endif
    inlining_(false),
    hasGlobalReallocation(false),
    oomInVector(false),
    gcNumber(cx->runtime->gcNumber),
    applyTricks(NoApplyTricks),
    pcLengths(NULL)
{
    
    if (cx->typeInferenceEnabled())
        addTraceHints = false;

    
    if (!debugMode() && cx->typeInferenceEnabled() && globalObj &&
        (outerScript->getUseCount() >= USES_BEFORE_INLINING ||
         cx->hasRunOption(JSOPTION_METHODJIT_ALWAYS))) {
        inlining_ = true;
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
    } else if (status != Compile_Retry) {
        *checkAddr = JS_UNJITTABLE_SCRIPT;
        if (outerScript->hasFunction) {
            outerScript->uninlineable = true;
            types::MarkTypeObjectFlags(cx, outerScript->function(),
                                       types::OBJECT_FLAG_UNINLINEABLE);
        }
    }

    return status;
}

CompileStatus
mjit::Compiler::checkAnalysis(JSScript *script)
{
    if (script->hasClearedGlobal()) {
        JaegerSpew(JSpew_Abort, "script has a cleared global\n");
        return Compile_Abort;
    }

    if (!script->ensureRanAnalysis(cx))
        return Compile_Error;
    if (cx->typeInferenceEnabled() && !script->ensureRanInference(cx))
        return Compile_Error;

    ScriptAnalysis *analysis = script->analysis();
    if (analysis->failed()) {
        JaegerSpew(JSpew_Abort, "couldn't analyze bytecode; probably switchX or OOM\n");
        return Compile_Abort;
    }

    return Compile_Okay;
}

CompileStatus
mjit::Compiler::addInlineFrame(JSScript *script, uint32 depth,
                               uint32 parent, jsbytecode *parentpc)
{
    JS_ASSERT(inlining());

    CompileStatus status = checkAnalysis(script);
    if (status != Compile_Okay)
        return status;

    if (!ssa.addInlineFrame(script, depth, parent, parentpc))
        return Compile_Error;

    uint32 index = ssa.iterFrame(ssa.numFrames() - 1).index;
    return scanInlineCalls(index, depth);
}

CompileStatus
mjit::Compiler::scanInlineCalls(uint32 index, uint32 depth)
{
    
    static const uint32 INLINE_SITE_LIMIT = 5;

    JS_ASSERT(inlining() && globalObj);

    
    if (isConstructing)
        return Compile_Okay;

    JSScript *script = ssa.getFrame(index).script;
    ScriptAnalysis *analysis = script->analysis();

    
    if (!script->hasGlobal() ||
        script->global() != globalObj ||
        (script->hasFunction && script->function()->getParent() != globalObj) ||
        (script->hasFunction && script->function()->isHeavyweight()) ||
        script->isActiveEval) {
        return Compile_Okay;
    }

    uint32 nextOffset = 0;
    while (nextOffset < script->length) {
        uint32 offset = nextOffset;
        jsbytecode *pc = script->code + offset;
        nextOffset = offset + analyze::GetBytecodeLength(pc);

        Bytecode *code = analysis->maybeCode(pc);
        if (!code)
            continue;

        
        if (JSOp(*pc) != JSOP_CALL)
            continue;

        
        if (code->monitoredTypes || code->monitoredTypesReturn || analysis->typeBarriers(pc) != NULL)
            continue;

        uint32 argc = GET_ARGC(pc);
        types::TypeSet *calleeTypes = analysis->poppedTypes(pc, argc + 1);

        if (calleeTypes->getKnownTypeTag(cx) != JSVAL_TYPE_OBJECT)
            continue;

        if (calleeTypes->getObjectCount() >= INLINE_SITE_LIMIT)
            continue;

        





        uint32 stackLimit = outerScript->nslots + StackSpace::STACK_JIT_EXTRA
            - VALUES_PER_STACK_FRAME - FrameState::TEMPORARY_LIMIT - 1;

        
        uint32 nextDepth = depth + VALUES_PER_STACK_FRAME + script->nfixed + code->stackDepth;

        



        unsigned count = calleeTypes->getObjectCount();
        bool okay = true;
        for (unsigned i = 0; i < count; i++) {
            if (calleeTypes->getTypeObject(i) != NULL) {
                okay = false;
                break;
            }

            JSObject *obj = calleeTypes->getSingleObject(i);
            if (!obj)
                continue;

            if (!obj->isFunction()) {
                okay = false;
                break;
            }

            JSFunction *fun = obj->getFunctionPrivate();
            if (!fun->isInterpreted()) {
                okay = false;
                break;
            }
            JSScript *script = fun->script();

            




            if (!globalObj ||
                fun->getParent() != globalObj ||
                outerScript->strictModeCode != script->strictModeCode) {
                okay = false;
                break;
            }

            
            uint32 nindex = index;
            while (nindex != CrossScriptSSA::INVALID_FRAME) {
                if (ssa.getFrame(nindex).script == script)
                    okay = false;
                nindex = ssa.getFrame(nindex).parent;
            }
            if (!okay)
                break;

            
            if (nextDepth + script->nslots >= stackLimit) {
                okay = false;
                break;
            }

            if (!script->types || !script->types->hasScope()) {
                okay = false;
                break;
            }

            CompileStatus status = checkAnalysis(script);
            if (status != Compile_Okay)
                return status;

            if (!script->analysis()->inlineable(argc)) {
                okay = false;
                break;
            }

            if (types::TypeSet::HasObjectFlags(cx, fun->getType(cx),
                                               types::OBJECT_FLAG_UNINLINEABLE)) {
                okay = false;
                break;
            }

            





            if (script->analysis()->usesThisValue() &&
                types::TypeScript::ThisTypes(script)->getKnownTypeTag(cx) != JSVAL_TYPE_OBJECT) {
                okay = false;
                break;
            }
        }
        if (!okay)
            continue;

        calleeTypes->addFreeze(cx);

        



        for (unsigned i = 0; i < count; i++) {
            JSObject *obj = calleeTypes->getSingleObject(i);
            if (!obj)
                continue;

            JSFunction *fun = obj->getFunctionPrivate();
            JSScript *script = fun->script();

            CompileStatus status = addInlineFrame(script, nextDepth, index, pc);
            if (status != Compile_Okay)
                return status;
        }
    }

    return Compile_Okay;
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
        newa->inlineIndex = CrossScriptSSA::OUTER_FRAME;
        outer = newa;
    }
    JS_ASSERT(ssa.getFrame(newa->inlineIndex).script == script);

    ScriptAnalysis *newAnalysis = script->analysis();

#ifdef JS_METHODJIT_SPEW
    if (cx->typeInferenceEnabled() && IsJaegerSpewChannelActive(JSpew_Regalloc)) {
        unsigned nargs = script->hasFunction ? script->function()->nargs : 0;
        for (unsigned i = 0; i < nargs; i++) {
            uint32 slot = ArgSlot(i);
            if (!newAnalysis->slotEscapes(slot)) {
                JaegerSpew(JSpew_Regalloc, "Argument %u:", i);
                newAnalysis->liveness(slot).print();
            }
        }
        for (unsigned i = 0; i < script->nfixed; i++) {
            uint32 slot = LocalSlot(script, i);
            if (!newAnalysis->slotEscapes(slot)) {
                JaegerSpew(JSpew_Regalloc, "Local %u:", i);
                newAnalysis->liveness(slot).print();
            }
        }
    }
#endif

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

    return Compile_Okay;
}

void
mjit::Compiler::popActiveFrame()
{
    JS_ASSERT(a->parent);
    this->PC = a->parentPC;
    this->a = a->parent;
    this->script = a->script;
    this->analysis = this->script->analysis();

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

    if (inlining()) {
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

        CHECK_STATUS(checkAnalysis(outerScript));
        if (inlining())
            CHECK_STATUS(scanInlineCalls(CrossScriptSSA::OUTER_FRAME, 0));
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

    JaegerSpew(JSpew_Scripts, "successfully compiled (code \"%p\") (size \"%u\")\n",
               (*jitp)->code.m_code.executableAddress(), unsigned((*jitp)->code.m_size));

    if (!*jitp)
        return Compile_Abort;

    return Compile_Okay;
}

#undef CHECK_STATUS

mjit::Compiler::ActiveFrame::ActiveFrame(JSContext *cx)
    : parent(NULL), parentPC(NULL), script(NULL), jumpMap(NULL),
      inlineIndex(uint32(-1)), varTypes(NULL), needReturnValue(false),
      syncReturnValue(false), returnValueDouble(false), returnSet(false),
      returnEntry(NULL), returnJumps(NULL), exitState(NULL)
{}

mjit::Compiler::ActiveFrame::~ActiveFrame()
{
    js::Foreground::free_(jumpMap);
    if (varTypes)
        js::Foreground::free_(varTypes);
}

mjit::Compiler::~Compiler()
{
    if (outer)
        cx->delete_(outer);
    for (unsigned i = 0; i < inlineFrames.length(); i++)
        cx->delete_(inlineFrames[i]);
    while (loop) {
        LoopState *nloop = loop->outer;
        cx->delete_(loop);
        loop = nloop;
    }
}

CompileStatus
mjit::Compiler::prepareInferenceTypes(JSScript *script, ActiveFrame *a)
{
    





















    a->varTypes = (VarType *)
        cx->calloc_(TotalSlots(script) * sizeof(VarType));
    if (!a->varTypes)
        return Compile_Error;

    for (uint32 slot = ArgSlot(0); slot < TotalSlots(script); slot++) {
        VarType &vt = a->varTypes[slot];
        vt.types = types::TypeScript::SlotTypes(script, slot);
        vt.type = vt.types->getKnownTypeTag(cx);
    }

    return Compile_Okay;
}

CompileStatus JS_NEVER_INLINE
mjit::TryCompile(JSContext *cx, JSScript *script, bool construct)
{
#if JS_HAS_SHARP_VARS
    if (script->hasSharps)
        return Compile_Abort;
#endif
    bool ok = cx->compartment->ensureJaegerCompartmentExists(cx);
    if (!ok)
        return Compile_Abort;

    
    if (construct && !script->nslots)
        script->nslots++;

    CompileStatus status;
    {
        types::AutoEnterTypeInference enter(cx, true);

        Compiler cc(cx, script, construct);
        status = cc.compile();
    }

    if (status == Compile_Okay) {
        




        JITScriptStatus status = script->getJITStatus(construct);
        JS_ASSERT(status != JITScript_Invalid);
        return (status == JITScript_Valid) ? Compile_Okay : Compile_Retry;
    }

    
    JS_ASSERT_IF(status == Compile_Error,
                 cx->isExceptionPending() || cx->runtime->hadOutOfMemory);

    return status;
}

CompileStatus
mjit::Compiler::generatePrologue()
{
    invokeLabel = masm.label();

    



    if (script->hasFunction) {
        Jump j = masm.jump();

        



        invokeLabel = masm.label();

        Label fastPath = masm.label();

        
        masm.storePtr(ImmPtr(script->function()), Address(JSFrameReg, StackFrame::offsetOfExec()));

        {
            





            arityLabel = stubcc.masm.label();

            Jump argMatch = stubcc.masm.branch32(Assembler::Equal, JSParamReg_Argc,
                                                 Imm32(script->function()->nargs));

            if (JSParamReg_Argc != Registers::ArgReg1)
                stubcc.masm.move(JSParamReg_Argc, Registers::ArgReg1);

            
            stubcc.masm.storePtr(ImmPtr(script->function()),
                                 Address(JSFrameReg, StackFrame::offsetOfExec()));
            OOL_STUBCALL(stubs::FixupArity, REJOIN_NONE);
            stubcc.masm.move(Registers::ReturnReg, JSFrameReg);
            argMatch.linkTo(stubcc.masm.label(), &stubcc.masm);

            argsCheckLabel = stubcc.masm.label();

            
            if (cx->typeInferenceEnabled()) {
#ifdef JS_MONOIC
                this->argsCheckJump = stubcc.masm.jump();
                this->argsCheckStub = stubcc.masm.label();
                this->argsCheckJump.linkTo(this->argsCheckStub, &stubcc.masm);
#endif
                stubcc.masm.storePtr(ImmPtr(script->function()), Address(JSFrameReg, StackFrame::offsetOfExec()));
                OOL_STUBCALL(stubs::CheckArgumentTypes, REJOIN_CHECK_ARGUMENTS);
#ifdef JS_MONOIC
                this->argsCheckFallthrough = stubcc.masm.label();
#endif
            }

            stubcc.crossJump(stubcc.masm.jump(), fastPath);
        }

        




        uint32 nvals = VALUES_PER_STACK_FRAME + script->nslots + StackSpace::STACK_JIT_EXTRA;
        masm.addPtr(Imm32(nvals * sizeof(Value)), JSFrameReg, Registers::ReturnReg);
        Jump stackCheck = masm.branchPtr(Assembler::AboveOrEqual, Registers::ReturnReg,
                                         FrameAddress(offsetof(VMFrame, stackLimit)));

        






        {
            stubcc.linkExitDirect(stackCheck, stubcc.masm.label());
            OOL_STUBCALL(stubs::HitStackQuota, REJOIN_NONE);
            stubcc.crossJump(stubcc.masm.jump(), masm.label());
        }

        




        for (uint32 i = 0; i < script->nfixed; i++) {
            if (analysis->localHasUseBeforeDef(i) || addTraceHints) {
                Address local(JSFrameReg, sizeof(StackFrame) + i * sizeof(Value));
                masm.storeValue(UndefinedValue(), local);
            }
        }

        types::TypeScriptNesting *nesting = script->nesting();

        



        JS_ASSERT_IF(nesting && nesting->children, script->function()->isHeavyweight());
        if (script->function()->isHeavyweight()) {
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::FunctionFramePrologue, REJOIN_FUNCTION_PROLOGUE);
        } else {
            





            if (analysis->usesScopeChain() || nesting) {
                RegisterID t0 = Registers::ReturnReg;
                Jump hasScope = masm.branchTest32(Assembler::NonZero,
                                                  FrameFlagsAddress(), Imm32(StackFrame::HAS_SCOPECHAIN));
                masm.loadPayload(Address(JSFrameReg, StackFrame::offsetOfCallee(script->function())), t0);
                masm.loadPtr(Address(t0, offsetof(JSObject, parent)), t0);
                masm.storePtr(t0, Address(JSFrameReg, StackFrame::offsetOfScopeChain()));
                hasScope.linkTo(masm.label(), &masm);
            }

            if (nesting) {
                








                JSScript *parent = nesting->parent;
                JS_ASSERT(parent);
                JS_ASSERT_IF(parent->hasAnalysis() && parent->analysis()->ranBytecode(),
                             !parent->analysis()->addsScopeObjects());

                RegisterID t0 = Registers::ReturnReg;
                masm.move(ImmPtr(&parent->nesting()->activeCall), t0);
                masm.loadPtr(Address(t0), t0);

                Address scopeChain(JSFrameReg, StackFrame::offsetOfScopeChain());
                Jump mismatch = masm.branchPtr(Assembler::NotEqual, t0, scopeChain);
                masm.add32(Imm32(1), AbsoluteAddress(&nesting->activeFrames));

                stubcc.linkExitDirect(mismatch, stubcc.masm.label());
                OOL_STUBCALL(stubs::FunctionFramePrologue, REJOIN_FUNCTION_PROLOGUE);
                stubcc.crossJump(stubcc.masm.jump(), masm.label());
            }
        }

        if (outerScript->usesArguments && !script->function()->isHeavyweight()) {
            




            Jump hasArgs = masm.branchTest32(Assembler::NonZero, FrameFlagsAddress(),
                                             Imm32(StackFrame::OVERRIDE_ARGS |
                                                   StackFrame::UNDERFLOW_ARGS |
                                                   StackFrame::OVERFLOW_ARGS |
                                                   StackFrame::HAS_ARGS_OBJ));
            masm.storePtr(ImmPtr((void *)(size_t) script->function()->nargs),
                          Address(JSFrameReg, StackFrame::offsetOfArgs()));
            hasArgs.linkTo(masm.label(), &masm);
        }

        j.linkTo(masm.label(), &masm);
    }

    if (cx->typeInferenceEnabled()) {
#ifdef DEBUG
        if (script->hasFunction) {
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::AssertArgumentTypes, REJOIN_NONE);
        }
#endif
        ensureDoubleArguments();
    }

    if (isConstructing) {
        if (!constructThis())
            return Compile_Error;
    }

    if (debugMode()) {
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::ScriptDebugPrologue, REJOIN_RESUME);
    } else if (Probes::callTrackingActive(cx)) {
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::ScriptProbeOnlyPrologue, REJOIN_RESUME);
    }

    recompileCheckHelper();

    if (outerScript->pcCounters) {
        size_t length = ssa.frameLength(ssa.numFrames() - 1);
        pcLengths = (PCLengthEntry *) cx->calloc_(sizeof(pcLengths[0]) * length);
        if (!pcLengths)
            return Compile_Error;
    }

    return Compile_Okay;
}

void
mjit::Compiler::ensureDoubleArguments()
{
    
    for (uint32 i = 0; script->hasFunction && i < script->function()->nargs; i++) {
        uint32 slot = ArgSlot(i);
        if (a->varTypes[slot].type == JSVAL_TYPE_DOUBLE && analysis->trackSlot(slot))
            frame.ensureDouble(frame.getArg(i));
    }
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

    



    if (cx->runtime->gcNumber != gcNumber)
        return Compile_Retry;

    for (size_t i = 0; i < branchPatches.length(); i++) {
        Label label = labelOf(branchPatches[i].pc, branchPatches[i].inlineIndex);
        branchPatches[i].jump.linkTo(label, &masm);
    }

#ifdef JS_CPU_ARM
    masm.forceFlushConstantPool();
    stubcc.masm.forceFlushConstantPool();
#endif
    JaegerSpew(JSpew_Insns, "## Fast code (masm) size = %lu, Slow code (stubcc) size = %lu.\n",
               (unsigned long) masm.size(), (unsigned long) stubcc.size());

    size_t codeSize = masm.size() +
                      stubcc.size() +
                      (masm.numDoubles() * sizeof(double)) +
                      (stubcc.masm.numDoubles() * sizeof(double)) +
                      jumpTableOffsets.length() * sizeof(void *);

    JSC::ExecutablePool *execPool;
    uint8 *result = (uint8 *)script->compartment()->jaegerCompartment()->execAlloc()->
                    alloc(codeSize, &execPool, JSC::METHOD_CODE);
    if (!result) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }
    JS_ASSERT(execPool);
    JSC::ExecutableAllocator::makeWritable(result, codeSize);
    masm.executableCopy(result);
    stubcc.masm.executableCopy(result + masm.size());

    JSC::LinkBuffer fullCode(result, codeSize, JSC::METHOD_CODE);
    JSC::LinkBuffer stubCode(result + masm.size(), stubcc.size(), JSC::METHOD_CODE);

    size_t nNmapLive = loopEntries.length();
    for (size_t i = 0; i < script->length; i++) {
        Bytecode *opinfo = analysis->maybeCode(i);
        if (opinfo && opinfo->safePoint) {
            
            if (!cx->typeInferenceEnabled() || !opinfo->loopHead)
                nNmapLive++;
        }
    }

    
    size_t dataSize = sizeof(JITScript) +
                      sizeof(NativeMapEntry) * nNmapLive +
                      sizeof(InlineFrame) * inlineFrames.length() +
                      sizeof(CallSite) * callSites.length() +
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
                      0;

    uint8 *cursor = (uint8 *)cx->calloc_(dataSize);
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
    jit->singleStepMode = script->stepModeEnabled();
    if (script->hasFunction) {
        jit->arityCheckEntry = stubCode.locationOf(arityLabel).executableAddress();
        jit->argsCheckEntry = stubCode.locationOf(argsCheckLabel).executableAddress();
        jit->fastEntry = fullCode.locationOf(invokeLabel).executableAddress();
    }
    jit->pcLengths = pcLengths;

    




    
    Label *jumpMap = a->jumpMap;

    
    NativeMapEntry *jitNmap = (NativeMapEntry *)cursor;
    jit->nNmapPairs = nNmapLive;
    cursor += sizeof(NativeMapEntry) * jit->nNmapPairs;
    size_t ix = 0;
    if (jit->nNmapPairs > 0) {
        for (size_t i = 0; i < script->length; i++) {
            Bytecode *opinfo = analysis->maybeCode(i);
            if (opinfo && opinfo->safePoint) {
                if (cx->typeInferenceEnabled() && opinfo->loopHead)
                    continue;
                Label L = jumpMap[i];
                JS_ASSERT(L.isSet());
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
        to.fun = from->script->function();
        to.depth = ssa.getFrame(from->inlineIndex).depth;
    }

    
    CallSite *jitCallSites = (CallSite *)cursor;
    jit->nCallSites = callSites.length();
    cursor += sizeof(CallSite) * jit->nCallSites;
    for (size_t i = 0; i < jit->nCallSites; i++) {
        CallSite &to = jitCallSites[i];
        InternalCallSite &from = callSites[i];

        
        if (cx->typeInferenceEnabled() &&
            from.rejoin != REJOIN_TRAP &&
            from.rejoin != REJOIN_SCRIPTED &&
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
        to.initialize(codeOffset, from.inlineIndex, from.inlinepc - script->code, from.rejoin);

        




        if (from.loopPatch.hasPatch)
            stubCode.patch(from.loopPatch.codePatch, result + codeOffset);
    }

#if defined JS_MONOIC
    JS_INIT_CLIST(&jit->callers);

    if (script->hasFunction && cx->typeInferenceEnabled()) {
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
            JS_ASSERT(jumpMap[offs].isSet());
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
            JS_ASSERT(jumpMap[offs].isSet());
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
        } else if (pics[i].kind != ic::PICInfo::NAME &&
                   pics[i].kind != ic::PICInfo::CALLNAME) {
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

    JS_ASSERT(size_t(cursor - (uint8*)jit) == dataSize);
    
    JS_ASSERT(jit->scriptDataSize(NULL) == dataSize);

    
    stubcc.fixCrossJumps(result, masm.size(), masm.size() + stubcc.size());

    size_t doubleOffset = masm.size() + stubcc.size();
    double *inlineDoubles = (double *) (result + doubleOffset);
    double *oolDoubles = (double*) (result + doubleOffset +
                                    masm.numDoubles() * sizeof(double));

    
    void **jumpVec = (void **)(oolDoubles + stubcc.masm.numDoubles());

    for (size_t i = 0; i < jumpTableOffsets.length(); i++) {
        uint32 offset = jumpTableOffsets[i];
        JS_ASSERT(jumpMap[offset].isSet());
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

    
    jsbytecode *lastPC = NULL;

    for (;;) {
        JSOp op = JSOp(*PC);
        int trap = stubs::JSTRAP_NONE;
        if (op == JSOP_TRAP) {
            if (!trapper.untrap(PC))
                return Compile_Error;
            op = JSOp(*PC);
            trap |= stubs::JSTRAP_TRAP;
        }
        if (script->stepModeEnabled() && scanner.firstOpInLine(PC - script->code))
            trap |= stubs::JSTRAP_SINGLESTEP;

        Bytecode *opinfo = analysis->maybeCode(PC);

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

        if (fallthrough) {
            






            for (unsigned i = 0; i < fixedIntToDoubleEntries.length(); i++) {
                FrameEntry *fe = frame.getSlotEntry(fixedIntToDoubleEntries[i]);
                frame.ensureInteger(fe);
            }
            for (unsigned i = 0; i < fixedDoubleToAnyEntries.length(); i++) {
                FrameEntry *fe = frame.getSlotEntry(fixedDoubleToAnyEntries[i]);
                frame.syncAndForgetFe(fe);
            }
        }
        fixedIntToDoubleEntries.clear();
        fixedDoubleToAnyEntries.clear();

        if (opinfo->jumpTarget || trap) {
            if (fallthrough) {
                fixDoubleTypes(PC);
                fixedIntToDoubleEntries.clear();
                fixedDoubleToAnyEntries.clear();

                





                if (cx->typeInferenceEnabled() && analysis->getCode(PC).loopHead) {
                    frame.syncAndForgetEverything();
                    Jump j = masm.jump();
                    if (!startLoop(PC, j, PC))
                        return Compile_Error;
                } else {
                    Label start = masm.label();
                    if (!frame.syncForBranch(PC, Uses(0)))
                        return Compile_Error;
                    if (script->pcCounters) {
                        
                        size_t length = masm.size() - masm.distanceOf(start);
                        uint32 offset = ssa.frameLength(a->inlineIndex) + lastPC - script->code;
                        pcLengths[offset].codeLength += length;
                    }
                    JS_ASSERT(frame.consistentRegisters(PC));
                }
            }

            if (!frame.discardForJoin(analysis->getAllocation(PC), opinfo->stackDepth))
                return Compile_Error;
            updateJoinVarTypes();
            fallthrough = true;

            if (!cx->typeInferenceEnabled()) {
                
                opinfo->safePoint = true;
            }
        }
        frame.assertValidRegisterState();
        a->jumpMap[uint32(PC - script->code)] = masm.label();

        
        
        
        if (loop && !a->parent)
            loop->setOuterPC(PC);

        SPEW_OPCODE();
        JS_ASSERT(frame.stackDepth() == opinfo->stackDepth);

        
        
        
        
        if (op == JSOP_ENTERBLOCK && analysis->getCode(PC).exceptionEntry)
            masm.loadPtr(FrameAddress(VMFrame::offsetOfFp), JSFrameReg);

        if (trap) {
            prepareStubCall(Uses(0));
            masm.move(Imm32(trap), Registers::ArgReg1);
            Call cl = emitStubCall(JS_FUNC_TO_DATA_PTR(void *, stubs::Trap), NULL);
            InternalCallSite site(masm.callReturnOffset(cl), a->inlineIndex, PC,
                                  REJOIN_TRAP, false);
            addCallSite(site);
        }

        
        if (js_CodeSpec[op].format & JOF_DECOMPOSE) {
            PC += js_CodeSpec[op].length;
            continue;
        }

        Label codeStart = masm.label();
        bool countersUpdated = false;

        




        if (script->pcCounters && JOF_OPTYPE(op) == JOF_JUMP)
            updatePCCounters(PC, &codeStart, &countersUpdated);

    



        lastPC = PC;

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
            if (script->pcCounters)
                updatePCCounters(PC, &codeStart, &countersUpdated);
            emitReturn(frame.peek(-1));
            fallthrough = false;
          END_CASE(JSOP_RETURN)

          BEGIN_CASE(JSOP_GOTO)
          BEGIN_CASE(JSOP_GOTOX)
          BEGIN_CASE(JSOP_DEFAULT)
          {
            unsigned targetOffset = FollowBranch(cx, script, PC - script->code);
            jsbytecode *target = script->code + targetOffset;

            fixDoubleTypes(target);

            




            jsbytecode *next = PC + js_CodeSpec[op].length;
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
            PC += js_CodeSpec[op].length;
            break;
          }
          END_CASE(JSOP_GOTO)

          BEGIN_CASE(JSOP_IFEQ)
          BEGIN_CASE(JSOP_IFNE)
          BEGIN_CASE(JSOP_IFEQX)
          BEGIN_CASE(JSOP_IFNEX)
          {
            jsbytecode *target = PC + GetJumpOffset(PC, PC);
            fixDoubleTypes(target);
            if (!jsop_ifneq(op, target))
                return Compile_Error;
            PC += js_CodeSpec[op].length;
            break;
          }
          END_CASE(JSOP_IFNE)

          BEGIN_CASE(JSOP_ARGUMENTS)
            






            if (canUseApplyTricks()) {
                



                interruptCheckHelper();

                applyTricks = LazyArgsObj;
                pushSyncedEntry(0);
            } else if (cx->typeInferenceEnabled() && !script->strictModeCode &&
                       !script->function()->getType(cx)->hasAnyFlags(types::OBJECT_FLAG_CREATED_ARGUMENTS)) {
                frame.push(MagicValue(JS_LAZY_ARGUMENTS));
            } else {
                jsop_arguments(REJOIN_FALLTHROUGH);
                pushSyncedEntry(0);
            }
          END_CASE(JSOP_ARGUMENTS)

          BEGIN_CASE(JSOP_ITERNEXT)
            iterNext(GET_INT8(PC));
          END_CASE(JSOP_ITERNEXT)

          BEGIN_CASE(JSOP_DUP)
            frame.dup();
          END_CASE(JSOP_DUP)

          BEGIN_CASE(JSOP_DUP2)
            frame.dup2();
          END_CASE(JSOP_DUP2)

          BEGIN_CASE(JSOP_SWAP)
            frame.dup2();
            frame.shift(-3);
            frame.shift(-1);
          END_CASE(JSOP_SWAP)

          BEGIN_CASE(JSOP_PICK)
          {
            int32 amt = GET_INT8(PC);

            
            
            
            frame.dupAt(-(amt + 1));

            
            
            
            
            
            
            for (int32 i = -amt; i < 0; i++) {
                frame.dupAt(i - 1);
                frame.shift(i - 2);
            }

            
            
            
            frame.shimmy(1);
          }
          END_CASE(JSOP_PICK)

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
                if (script->pcCounters)
                    updatePCCounters(PC, &codeStart, &countersUpdated);
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

                if (lv.isPrimitive() && rv.isPrimitive()) {
                    bool result = compareTwoValues(cx, op, lv, rv);

                    frame.pop();
                    frame.pop();

                    if (!target) {
                        frame.push(Value(BooleanValue(result)));
                    } else {
                        if (fused == JSOP_IFEQ)
                            result = !result;
                        if (!constantFoldBranch(target, result))
                            return Compile_Error;
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
                JS_ALWAYS_TRUE(ValueToECMAInt32(cx, top->getValue(), &i));
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
                JS_ALWAYS_TRUE(ToNumber(cx, top->getValue(), &d));
                d = -d;
                Value v = NumberValue(d);

                
                types::TypeSet *pushed = pushedTypeSet(0);
                if (!v.isInt32() && pushed && !pushed->hasType(types::Type::DoubleType())) {
                    types::TypeScript::MonitorOverflow(cx, script, PC);
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
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DelName, REJOIN_FALLTHROUGH);
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_DELNAME)

          BEGIN_CASE(JSOP_DELPROP)
          {
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(1));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(STRICT_VARIANT(stubs::DelProp), REJOIN_FALLTHROUGH);
            frame.pop();
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_DELPROP)

          BEGIN_CASE(JSOP_DELELEM)
          {
            prepareStubCall(Uses(2));
            INLINE_STUBCALL(STRICT_VARIANT(stubs::DelElem), REJOIN_FALLTHROUGH);
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

          BEGIN_CASE(JSOP_GETPROP)
          BEGIN_CASE(JSOP_LENGTH)
            if (!jsop_getprop(script->getAtom(fullAtomIndex(PC)), knownPushedType(0)))
                return Compile_Error;
          END_CASE(JSOP_GETPROP)

          BEGIN_CASE(JSOP_GETELEM)
            if (!jsop_getelem(false))
                return Compile_Error;
          END_CASE(JSOP_GETELEM)

          BEGIN_CASE(JSOP_TOID)
            jsop_toid();
          END_CASE(JSOP_TOID)

          BEGIN_CASE(JSOP_SETELEM)
          {
            jsbytecode *next = &PC[JSOP_SETELEM_LENGTH];
            bool pop = (JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next));
            if (!jsop_setelem(pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETELEM);

          BEGIN_CASE(JSOP_EVAL)
          {
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
            bool callingNew = (op == JSOP_NEW);

            bool done = false;
            if ((op == JSOP_CALL || op == JSOP_NEW) && !monitored(PC)) {
                CompileStatus status = inlineNativeFunction(GET_ARGC(PC), callingNew);
                if (status == Compile_Okay)
                    done = true;
                else if (status != Compile_InlineAbort)
                    return status;
            }
            if (!done && inlining()) {
                CompileStatus status = inlineScriptedFunction(GET_ARGC(PC), callingNew);
                if (status == Compile_Okay)
                    done = true;
                else if (status != Compile_InlineAbort)
                    return status;
                if (script->pcCounters) {
                    
                    updatePCCounters(PC, &codeStart, &countersUpdated);
                }
            }

            FrameSize frameSize;
            frameSize.initStatic(frame.totalDepth(), GET_ARGC(PC));

            if (!done) {
                JaegerSpew(JSpew_Insns, " --- SCRIPTED CALL --- \n");
                inlineCallHelper(GET_ARGC(PC), callingNew, frameSize);
                JaegerSpew(JSpew_Insns, " --- END SCRIPTED CALL --- \n");
            }
          }
          END_CASE(JSOP_CALL)

          BEGIN_CASE(JSOP_NAME)
          {
            JSAtom *atom = script->getAtom(fullAtomIndex(PC));
            jsop_name(atom, knownPushedType(0), false);
            frame.extra(frame.peek(-1)).name = atom;
          }
          END_CASE(JSOP_NAME)

          BEGIN_CASE(JSOP_CALLNAME)
          {
            JSAtom *atom = script->getAtom(fullAtomIndex(PC));
            jsop_name(atom, knownPushedType(0), true);
            frame.extra(frame.peek(-2)).name = atom;
          }
          END_CASE(JSOP_CALLNAME)

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
          {
            jsbytecode *target = PC + GET_JUMP_OFFSET(PC);
            fixDoubleTypes(target);
            if (!jsop_andor(op, target))
                return Compile_Error;
          }
          END_CASE(JSOP_AND)

          BEGIN_CASE(JSOP_TABLESWITCH)
          BEGIN_CASE(JSOP_TABLESWITCHX)
            






            if (script->pcCounters)
                updatePCCounters(PC, &codeStart, &countersUpdated);
#if defined JS_CPU_ARM 
            frame.syncAndKillEverything();
            masm.move(ImmPtr(PC), Registers::ArgReg1);

            
            INLINE_STUBCALL(stubs::TableSwitch, REJOIN_NONE);
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
            if (script->pcCounters)
                updatePCCounters(PC, &codeStart, &countersUpdated);
            frame.syncAndForgetEverything();
            masm.move(ImmPtr(PC), Registers::ArgReg1);

            
            INLINE_STUBCALL(stubs::LookupSwitch, REJOIN_NONE);
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
            
            if (script->pcCounters)
                updatePCCounters(PC, &codeStart, &countersUpdated);
            jsbytecode *target = &PC[JSOP_MOREITER_LENGTH];
            JSOp next = JSOp(*target);
            JS_ASSERT(next == JSOP_IFNE || next == JSOP_IFNEX);

            target += (next == JSOP_IFNE)
                      ? GET_JUMP_OFFSET(target)
                      : GET_JUMPX_OFFSET(target);

            fixDoubleTypes(target);
            if (!iterMore(target))
                return Compile_Error;
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
            restoreVarType();
            uint32 arg = GET_SLOTNO(PC);
            frame.pushArg(arg);
          }
          END_CASE(JSOP_GETARG)

          BEGIN_CASE(JSOP_CALLARG)
          {
            restoreVarType();
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
            jsbytecode *next = &PC[JSOP_SETARG_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            frame.storeArg(GET_SLOTNO(PC), pop);
            updateVarType();

            if (pop) {
                frame.pop();
                PC += JSOP_SETARG_LENGTH + JSOP_POP_LENGTH;
                break;
            }
          }
          END_CASE(JSOP_SETARG)

          BEGIN_CASE(JSOP_GETLOCAL)
          {
            




            jsbytecode *next = &PC[JSOP_GETLOCAL_LENGTH];
            if (JSOp(*next) != JSOP_POP || analysis->jumpTarget(next))
                restoreVarType();
            uint32 slot = GET_SLOTNO(PC);
            frame.pushLocal(slot);
          }
          END_CASE(JSOP_GETLOCAL)

          BEGIN_CASE(JSOP_SETLOCAL)
          {
            jsbytecode *next = &PC[JSOP_SETLOCAL_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            frame.storeLocal(GET_SLOTNO(PC), pop);
            updateVarType();

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
            frame.storeLocal(slot, true);
            frame.pop();
            updateVarType();
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
            if (!jsop_arginc(op, GET_SLOTNO(PC)))
                return Compile_Retry;
          END_CASE(JSOP_ARGDEC)

          BEGIN_CASE(JSOP_INCLOCAL)
          BEGIN_CASE(JSOP_DECLOCAL)
          BEGIN_CASE(JSOP_LOCALINC)
          BEGIN_CASE(JSOP_LOCALDEC)
            if (!jsop_localinc(op, GET_SLOTNO(PC)))
                return Compile_Retry;
          END_CASE(JSOP_LOCALDEC)

          BEGIN_CASE(JSOP_BINDNAME)
            jsop_bindname(script->getAtom(fullAtomIndex(PC)), true);
          END_CASE(JSOP_BINDNAME)

          BEGIN_CASE(JSOP_SETPROP)
          {
            jsbytecode *next = &PC[JSOP_SETPROP_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            if (!jsop_setprop(script->getAtom(fullAtomIndex(PC)), true, pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETPROP)

          BEGIN_CASE(JSOP_SETNAME)
          BEGIN_CASE(JSOP_SETMETHOD)
          {
            jsbytecode *next = &PC[JSOP_SETNAME_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            if (!jsop_setprop(script->getAtom(fullAtomIndex(PC)), true, pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETNAME)

          BEGIN_CASE(JSOP_THROW)
            prepareStubCall(Uses(1));
            INLINE_STUBCALL(stubs::Throw, REJOIN_NONE);
            frame.pop();
          END_CASE(JSOP_THROW)

          BEGIN_CASE(JSOP_IN)
          {
            prepareStubCall(Uses(2));
            INLINE_STUBCALL(stubs::In, REJOIN_PUSH_BOOLEAN);
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
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::Exception, REJOIN_FALLTHROUGH);
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
            uint32 index = fullAtomIndex(PC);
            JSFunction *innerFun = script->getFunction(index);

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(innerFun), Registers::ArgReg1);
            INLINE_STUBCALL(STRICT_VARIANT(stubs::DefFun), REJOIN_FALLTHROUGH);
          }
          END_CASE(JSOP_DEFFUN)

          BEGIN_CASE(JSOP_DEFVAR)
          BEGIN_CASE(JSOP_DEFCONST)
          {
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DefVarOrConst, REJOIN_FALLTHROUGH);
          }
          END_CASE(JSOP_DEFVAR)

          BEGIN_CASE(JSOP_SETCONST)
          {
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(1));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::SetConst, REJOIN_FALLTHROUGH);
          }
          END_CASE(JSOP_SETCONST)

          BEGIN_CASE(JSOP_DEFLOCALFUN_FC)
          {
            uint32 slot = GET_SLOTNO(PC);
            JSFunction *fun = script->getFunction(fullAtomIndex(&PC[SLOTNO_LEN]));
            prepareStubCall(Uses(frame.frameSlots()));
            masm.move(ImmPtr(fun), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DefLocalFun_FC, REJOIN_DEFLOCALFUN);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
            frame.storeLocal(slot, true);
            frame.pop();
            updateVarType();
          }
          END_CASE(JSOP_DEFLOCALFUN_FC)

          BEGIN_CASE(JSOP_LAMBDA)
          {
            JSFunction *fun = script->getFunction(fullAtomIndex(PC));

            JSObjStubFun stub = stubs::Lambda;
            uint32 uses = 0;

            jsbytecode *pc2 = NULL;
            if (fun->joinable()) {
                pc2 = AdvanceOverBlockchainOp(PC + JSOP_LAMBDA_LENGTH);
                JSOp next = JSOp(*pc2);

                if (next == JSOP_INITMETHOD) {
                    stub = stubs::LambdaJoinableForInit;
                } else if (next == JSOP_SETMETHOD) {
                    stub = stubs::LambdaJoinableForSet;
                    uses = 1;
                } else if (next == JSOP_CALL) {
                    int iargc = GET_ARGC(pc2);
                    if (iargc == 1 || iargc == 2) {
                        stub = stubs::LambdaJoinableForCall;
                        uses = frame.frameSlots();
                    }
                } else if (next == JSOP_NULL) {
                    pc2 += JSOP_NULL_LENGTH;
                    if (JSOp(*pc2) == JSOP_CALL && GET_ARGC(pc2) == 0)
                        stub = stubs::LambdaJoinableForNull;
                }
            }

            prepareStubCall(Uses(uses));
            masm.move(ImmPtr(fun), Registers::ArgReg1);

            if (stub != stubs::Lambda)
                masm.storePtr(ImmPtr(pc2), FrameAddress(offsetof(VMFrame, scratch)));

            INLINE_STUBCALL(stub, REJOIN_PUSH_OBJECT);

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
            

            BarrierState barrier = pushAddressMaybeBarrier(Address(reg, index * sizeof(Value)),
                                                           knownPushedType(0), true);
            finishBarrier(barrier, REJOIN_GETTER, 0);

            if (op == JSOP_CALLFCSLOT)
                frame.push(UndefinedValue());
          }
          END_CASE(JSOP_CALLFCSLOT)

          BEGIN_CASE(JSOP_ARGSUB)
          {
            prepareStubCall(Uses(0));
            masm.move(Imm32(GET_ARGNO(PC)), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::ArgSub, REJOIN_FALLTHROUGH);
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_ARGSUB)

          BEGIN_CASE(JSOP_ARGCNT)
          {
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::ArgCnt, REJOIN_FALLTHROUGH);
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_ARGCNT)

          BEGIN_CASE(JSOP_DEFLOCALFUN)
          {
            uint32 slot = GET_SLOTNO(PC);
            JSFunction *fun = script->getFunction(fullAtomIndex(&PC[SLOTNO_LEN]));
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(fun), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DefLocalFun, REJOIN_DEFLOCALFUN);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
            frame.storeLocal(slot, true);
            frame.pop();
            updateVarType();
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
            jsbytecode *next = &PC[JSOP_SETGNAME_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            jsop_setgname(script->getAtom(fullAtomIndex(PC)), true, pop);
          }
          END_CASE(JSOP_SETGNAME)

          BEGIN_CASE(JSOP_REGEXP)
          {
            JSObject *regex = script->getRegExp(fullAtomIndex(PC));
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(regex), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::RegExp, REJOIN_PUSH_OBJECT);
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
            if (script->pcCounters)
                updatePCCounters(PC, &codeStart, &countersUpdated);
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
            restoreVarType();
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
            INLINE_STUBCALL(stubs::FlatLambda, REJOIN_PUSH_OBJECT);
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
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(PC), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DebuggerStatement, REJOIN_FALLTHROUGH);
          }
          END_CASE(JSOP_DEBUGGER)

          BEGIN_CASE(JSOP_UNBRAND)
            jsop_unbrand();
          END_CASE(JSOP_UNBRAND)

          BEGIN_CASE(JSOP_UNBRANDTHIS)
            prepareStubCall(Uses(1));
            INLINE_STUBCALL(stubs::UnbrandThis, REJOIN_FALLTHROUGH);
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

    



        if (cx->typeInferenceEnabled() && PC == lastPC + analyze::GetBytecodeLength(lastPC)) {
            




            unsigned nuses = GetUseCount(script, lastPC - script->code);
            unsigned ndefs = GetDefCount(script, lastPC - script->code);
            for (unsigned i = 0; i < ndefs; i++) {
                FrameEntry *fe = frame.getStack(opinfo->stackDepth - nuses + i);
                if (fe) {
                    
                    frame.extra(fe).types = analysis->pushedTypes(lastPC - script->code, i);
                }
            }
        }

        if (script->pcCounters) {
            size_t length = masm.size() - masm.distanceOf(codeStart);
            if (countersUpdated || length != 0) {
                if (!countersUpdated)
                    updatePCCounters(lastPC, &codeStart, &countersUpdated);

                
                uint32 offset = ssa.frameLength(a->inlineIndex) + lastPC - script->code;
                pcLengths[offset].codeLength += length;
            }
        }

        frame.assertValidRegisterState();
    }

  done:
    return Compile_Okay;
}

#undef END_CASE
#undef BEGIN_CASE

void
mjit::Compiler::updatePCCounters(jsbytecode *pc, Label *start, bool *updated)
{
    





    uint32 offset = ssa.frameLength(a->inlineIndex) + pc - script->code;

    





    RegisterID reg = Registers::ReturnReg;
    masm.storePtr(reg, frame.addressOfTop());

    double *code = &script->pcCounters.get(JSPCCounters::METHODJIT_CODE, pc - script->code);
    double *codeLength = &pcLengths[offset].codeLength;
    masm.addCounter(codeLength, code, reg);

    double *pics = &script->pcCounters.get(JSPCCounters::METHODJIT_PICS, pc - script->code);
    double *picsLength = &pcLengths[offset].picsLength;
    masm.addCounter(picsLength, pics, reg);

    static const double oneDouble = 1.0;
    double *counter = &script->pcCounters.get(JSPCCounters::METHODJIT, pc - script->code);
    masm.addCounter(&oneDouble, counter, reg);

    
    masm.loadPtr(frame.addressOfTop(), reg);

    
    *start = masm.label();
    *updated = true;
}

JSC::MacroAssembler::Label
mjit::Compiler::labelOf(jsbytecode *pc, uint32 inlineIndex)
{
    ActiveFrame *a = (inlineIndex == uint32(-1)) ? outer : inlineFrames[inlineIndex];
    JS_ASSERT(uint32(pc - a->script->code) < a->script->length);

    uint32 offs = uint32(pc - a->script->code);
    JS_ASSERT(a->jumpMap[offs].isSet());
    return a->jumpMap[offs];
}

uint32
mjit::Compiler::fullAtomIndex(jsbytecode *pc)
{
    return GET_SLOTNO(pc);

    
#if 0
    return GET_SLOTNO(pc) + (atoms - script->atoms);
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
    JS_ASSERT(globalObj);
    uint32 slot = script->getGlobalSlot(index);

    JSObject *singleton = pushedSingleton(0);
    if (singleton && !hasTypeBarriers(PC) && !globalObj->getSlot(slot).isUndefined()) {
        frame.push(ObjectValue(*singleton));
        return;
    }

    if (cx->typeInferenceEnabled() && globalObj->isGlobal() &&
        !globalObj->getType(cx)->unknownProperties()) {
        Value *value = &globalObj->getSlotRef(slot);
        if (!value->isUndefined()) {
            watchGlobalReallocation();
            RegisterID reg = frame.allocReg();
            masm.move(ImmPtr(value), reg);

            BarrierState barrier = pushAddressMaybeBarrier(Address(reg), knownPushedType(0), true);
            finishBarrier(barrier, REJOIN_GETTER, 0);
            return;
        }
    }

    RegisterID reg = frame.allocReg();
    Address address = masm.objSlotRef(globalObj, reg, slot);
    BarrierState barrier = pushAddressMaybeBarrier(address, knownPushedType(0), true);
    finishBarrier(barrier, REJOIN_GETTER, 0);
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
    Address thisv(JSFrameReg, StackFrame::offsetOfThis(script->function()));

    
    
    
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

    
    
    
    frame.syncThis();
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

    




    if (!a->exitState && fe && fe->isCopy() && frame.isOuterSlot(fe->backing())) {
        a->returnEntry = fe->backing();
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
            frame.syncAndForgetFe(fe, true);
            frame.takeReg(fpreg);
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
            frame.syncAndForgetFe(fe, true);
            frame.takeReg(reg);
        } else {
            reg = frame.allocReg(mask.freeMask).reg();
            Value val = fe ? fe->getValue() : UndefinedValue();
            masm.loadValuePayload(val, reg);
        }
        JS_ASSERT_IF(a->returnSet, reg == a->returnRegister.reg());
        a->returnRegister = reg;
    }

    a->returnSet = true;
    if (a->exitState)
        a->exitState->setUnassigned(a->returnRegister);
}

void
mjit::Compiler::emitReturn(FrameEntry *fe)
{
    JS_ASSERT_IF(!script->hasFunction, JSOp(*PC) == JSOP_STOP);

    
    JS_ASSERT_IF(fe, fe == frame.peek(-1));

    if (debugMode() || Probes::callTrackingActive(cx)) {
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::ScriptDebugEpilogue, REJOIN_RESUME);
    }

    if (a != outer) {
        





        if (a->needReturnValue)
            emitInlineReturnValue(fe);

        if (a->exitState) {
            




            frame.syncForAllocation(a->exitState, true, Uses(0));
        }

        



        bool endOfScript =
            (JSOp(*PC) == JSOP_STOP) ||
            (JSOp(*PC) == JSOP_RETURN &&
             (JSOp(*(PC + JSOP_RETURN_LENGTH)) == JSOP_STOP &&
              !analysis->maybeCode(PC + JSOP_RETURN_LENGTH)));
        if (!endOfScript)
            a->returnJumps->append(masm.jump());

        if (a->returnSet)
            frame.freeReg(a->returnRegister);
        return;
    }

    









    if (script->hasFunction) {
        types::TypeScriptNesting *nesting = script->nesting();
        if (script->function()->isHeavyweight() || (nesting && nesting->children)) {
            prepareStubCall(Uses(fe ? 1 : 0));
            INLINE_STUBCALL(stubs::FunctionFrameEpilogue, REJOIN_NONE);
        } else {
            
            Jump putObjs = masm.branchTest32(Assembler::NonZero,
                                             Address(JSFrameReg, StackFrame::offsetOfFlags()),
                                             Imm32(StackFrame::HAS_CALL_OBJ | StackFrame::HAS_ARGS_OBJ));
            stubcc.linkExit(putObjs, Uses(frame.frameSlots()));

            stubcc.leave();
            OOL_STUBCALL(stubs::FunctionFrameEpilogue, REJOIN_NONE);

            emitReturnValue(&stubcc.masm, fe);
            emitFinalReturn(stubcc.masm);

            



            if (nesting)
                masm.sub32(Imm32(1), AbsoluteAddress(&nesting->activeFrames));
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
mjit::Compiler::emitStubCall(void *ptr, DataLabelPtr *pinline)
{
    JaegerSpew(JSpew_Insns, " ---- CALLING STUB ---- \n");

    masm.bumpStubCounter(script, PC, Registers::tempCallReg());

    Call cl = masm.fallibleVMCall(cx->typeInferenceEnabled(),
                                  ptr, outerPC(), NULL, pinline, frame.totalDepth());
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
    OOL_STUBCALL(stubs::Interrupt, REJOIN_RESUME);
    stubcc.rejoin(Changes(0));
}

void
mjit::Compiler::recompileCheckHelper()
{
    if (inlining() || debugMode() || !globalObj ||
        !analysis->hasFunctionCalls() || !cx->typeInferenceEnabled()) {
        return;
    }

    size_t *addr = script->addressOfUseCount();
    masm.add32(Imm32(1), AbsoluteAddress(addr));
#if defined(JS_CPU_X86) || defined(JS_CPU_ARM)
    Jump jump = masm.branch32(Assembler::GreaterThanOrEqual, AbsoluteAddress(addr),
                              Imm32(USES_BEFORE_INLINING));
#else
    
    RegisterID reg = frame.allocReg();
    masm.move(ImmPtr(addr), reg);
    Jump jump = masm.branch32(Assembler::GreaterThanOrEqual, Address(reg, 0),
                              Imm32(USES_BEFORE_INLINING));
    frame.freeReg(reg);
#endif
    stubcc.linkExit(jump, Uses(0));
    stubcc.leave();

    OOL_STUBCALL(stubs::RecompileForInline, REJOIN_RESUME);
    stubcc.rejoin(Changes(0));
}

void
mjit::Compiler::addReturnSite()
{
    InternalCallSite site(masm.distanceOf(masm.label()), a->inlineIndex, PC,
                          REJOIN_SCRIPTED, false);
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
    INLINE_STUBCALL(stub, REJOIN_CALL_PROLOGUE);

    Jump notCompiled = masm.branchTestPtr(Assembler::Zero, r0, r0);

    masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.sp)), JSFrameReg);
    callPatch.hasFastNcode = true;
    callPatch.fastNcodePatch =
        masm.storePtrWithPatch(ImmPtr(NULL),
                               Address(JSFrameReg, StackFrame::offsetOfNcode()));

    masm.jump(r0);
    callPatch.joinPoint = masm.label();
    addReturnSite();

    frame.popn(argc + 2);

    frame.takeReg(JSReturnReg_Type);
    frame.takeReg(JSReturnReg_Data);
    frame.pushRegs(JSReturnReg_Type, JSReturnReg_Data, knownPushedType(0));

    BarrierState barrier = testBarrier(JSReturnReg_Type, JSReturnReg_Data,
                                        false,
                                        true);

    stubcc.linkExitDirect(notCompiled, stubcc.masm.label());
    stubcc.rejoin(Changes(1));
    callPatches.append(callPatch);

    finishBarrier(barrier, REJOIN_FALLTHROUGH, 0);
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
            OOL_STUBCALL(stubs::Arguments, REJOIN_RESUME);
            frameDepthAdjust = +1;
        } else {
            frameDepthAdjust = 0;
        }

        stubcc.masm.move(Imm32(callImmArgc), Registers::ArgReg1);
        JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW CALL CODE ---- \n");
        OOL_STUBCALL_LOCAL_SLOTS(JS_FUNC_TO_DATA_PTR(void *, stubs::SlowCall),
                                 REJOIN_FALLTHROUGH, frame.totalDepth() + frameDepthAdjust);
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
    int32 speculatedArgc;
    if (applyTricks == LazyArgsObj) {
        frame.pop();
        speculatedArgc = 1;
    } else {
        





        interruptCheckHelper();

        speculatedArgc = callImmArgc;
    }

    FrameEntry *origCallee = frame.peek(-(speculatedArgc + 2));
    FrameEntry *origThis = frame.peek(-(speculatedArgc + 1));

    




    if (callingNew) {
        frame.discardFe(origThis);

        







        if (cx->typeInferenceEnabled())
            masm.storeValue(NullValue(), frame.addressOf(origThis));
    }

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
            
            jsop_arguments(REJOIN_RESUME);
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

    callIC.typeMonitored = monitored(PC) || hasTypeBarriers(PC);

    
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
            OOL_STUBCALL(ic::SplatApplyArgs, REJOIN_CALL_SPLAT);

        



        Jump toPatch = stubcc.masm.jump();
        toPatch.linkTo(stubcc.masm.label(), &stubcc.masm);
        callIC.oolJump = toPatch;
        callIC.icCall = stubcc.masm.label();

        RejoinState rejoinState = callIC.frameSize.rejoinState(PC, false);

        




        callIC.addrLabel1 = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
        void *icFunPtr = JS_FUNC_TO_DATA_PTR(void *, callingNew ? ic::New : ic::Call);
        if (callIC.frameSize.isStatic()) {
            callIC.oolCall = OOL_STUBCALL_LOCAL_SLOTS(icFunPtr, rejoinState, frame.totalDepth());
        } else {
            callIC.oolCall = OOL_STUBCALL_LOCAL_SLOTS(icFunPtr, rejoinState, -1);
        }

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
        OOL_STUBCALL(callingNew ? ic::NativeNew : ic::NativeCall, rejoinState);

        rejoin2 = stubcc.masm.jump();
    }

    



    callIC.hotPathLabel = masm.label();

    uint32 flags = 0;
    if (callingNew)
        flags |= StackFrame::CONSTRUCTING;

    InlineFrameAssembler inlFrame(masm, callIC, flags);
    callPatch.hasFastNcode = true;
    callPatch.fastNcodePatch = inlFrame.assemble(NULL, PC);

    callIC.hotJump = masm.jump();
    callIC.joinPoint = callPatch.joinPoint = masm.label();
    callIC.callIndex = callSites.length();
    addReturnSite();
    if (lowerFunCallOrApply)
        uncachedCallPatch.joinPoint = callIC.joinPoint;

    



    CHECK_IC_SPACE();

    JSValueType type = knownPushedType(0);

    frame.popn(speculatedArgc + 2);
    frame.takeReg(JSReturnReg_Type);
    frame.takeReg(JSReturnReg_Data);
    frame.pushRegs(JSReturnReg_Type, JSReturnReg_Data, type);

    BarrierState barrier = testBarrier(JSReturnReg_Type, JSReturnReg_Data,
                                        false,
                                        true);

    





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

    finishBarrier(barrier, REJOIN_FALLTHROUGH, 0);

    applyTricks = NoApplyTricks;
    return true;
#endif
}

CompileStatus
mjit::Compiler::callArrayBuiltin(uint32 argc, bool callingNew)
{
    if (!globalObj)
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
    OOL_STUBCALL(callingNew ? stubs::SlowNew : stubs::SlowCall, REJOIN_FALLTHROUGH);

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
    INLINE_STUBCALL(stubs::NewDenseUnallocatedArray, REJOIN_PUSH_OBJECT);

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
    JS_ASSERT(inlining());

    
    bool calleeMultipleReturns = false;
    Vector<JSScript *> inlineCallees(CompilerAllocPolicy(cx, *this));
    for (unsigned i = 0; i < ssa.numFrames(); i++) {
        if (ssa.iterFrame(i).parent == a->inlineIndex && ssa.iterFrame(i).parentpc == PC) {
            JSScript *script = ssa.iterFrame(i).script;
            inlineCallees.append(script);
            if (script->analysis()->numReturnSites() > 1)
                calleeMultipleReturns = true;
        }
    }

    if (inlineCallees.empty())
        return Compile_InlineAbort;

    JS_ASSERT(!monitored(PC));

    



    frame.pruneDeadEntries();

    RegisterAllocation *exitState = NULL;
    if (inlineCallees.length() > 1 || calleeMultipleReturns) {
        



        exitState = frame.computeAllocation(PC + JSOP_CALL_LENGTH);
    }

    




    FrameEntry *origCallee = frame.peek(-((int)argc + 2));
    FrameEntry *entrySnapshot = NULL;
    MaybeRegisterID calleeReg;
    if (inlineCallees.length() > 1) {
        frame.forgetMismatchedObject(origCallee);
        calleeReg = frame.tempRegForData(origCallee);

        entrySnapshot = frame.snapshotState();
        if (!entrySnapshot)
            return Compile_Error;
    }
    MaybeJump calleePrevious;

    JSValueType returnType = knownPushedType(0);

    bool needReturnValue = JSOP_POP != (JSOp)*(PC + JSOP_CALL_LENGTH);
    bool syncReturnValue = needReturnValue && returnType == JSVAL_TYPE_UNKNOWN;

    
    bool returnSet = false;
    AnyRegisterID returnRegister;
    const FrameEntry *returnEntry = NULL;

    Vector<Jump, 4, CompilerAllocPolicy> returnJumps(CompilerAllocPolicy(cx, *this));

    for (unsigned i = 0; i < inlineCallees.length(); i++) {
        if (entrySnapshot)
            frame.restoreFromSnapshot(entrySnapshot);

        JSScript *script = inlineCallees[i];
        CompileStatus status;

        status = pushActiveFrame(script, argc);
        if (status != Compile_Okay)
            return status;

        a->exitState = exitState;

        JaegerSpew(JSpew_Inlining, "inlining call to script (file \"%s\") (line \"%d\")\n",
                   script->filename, script->lineno);

        if (calleePrevious.isSet()) {
            calleePrevious.get().linkTo(masm.label(), &masm);
            calleePrevious = MaybeJump();
        }

        if (i + 1 != inlineCallees.length()) {
            
            JS_ASSERT(calleeReg.isSet());
            calleePrevious = masm.branchPtr(Assembler::NotEqual, calleeReg.reg(), ImmPtr(script->function()));
        }

        a->returnJumps = &returnJumps;
        a->needReturnValue = needReturnValue;
        a->syncReturnValue = syncReturnValue;
        a->returnValueDouble = returnType == JSVAL_TYPE_DOUBLE;
        if (returnSet) {
            a->returnSet = true;
            a->returnRegister = returnRegister;
        }

        



        ensureDoubleArguments();

        status = generateMethod();
        if (status != Compile_Okay) {
            popActiveFrame();
            if (status == Compile_Abort) {
                
                script->uninlineable = true;
                types::MarkTypeObjectFlags(cx, script->function(),
                                           types::OBJECT_FLAG_UNINLINEABLE);
                return Compile_Retry;
            }
            return status;
        }

        if (needReturnValue && !returnSet) {
            if (a->returnSet) {
                returnSet = true;
                returnRegister = a->returnRegister;
            } else {
                returnEntry = a->returnEntry;
            }
        }

        popActiveFrame();

        if (i + 1 != inlineCallees.length())
            returnJumps.append(masm.jump());
    }

    for (unsigned i = 0; i < returnJumps.length(); i++)
        returnJumps[i].linkTo(masm.label(), &masm);

    frame.popn(argc + 2);

    if (entrySnapshot)
        cx->array_delete(entrySnapshot);

    if (exitState)
        frame.discardForJoin(exitState, analysis->getCode(PC).stackDepth - (argc + 2));

    if (returnSet) {
        frame.takeReg(returnRegister);
        if (returnRegister.isReg())
            frame.pushTypedPayload(returnType, returnRegister.reg());
        else
            frame.pushDouble(returnRegister.fpreg());
    } else if (returnEntry) {
        frame.pushCopyOf((FrameEntry *) returnEntry);
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
mjit::Compiler::inlineStubCall(void *stub, RejoinState rejoin, Uses uses)
{
    DataLabelPtr inlinePatch;
    Call cl = emitStubCall(stub, &inlinePatch);
    InternalCallSite site(masm.callReturnOffset(cl), a->inlineIndex, PC,
                          rejoin, false);
    site.inlinePatch = inlinePatch;
    if (loop && loop->generatingInvariants()) {
        Jump j = masm.jump();
        Label l = masm.label();
        loop->addInvariantCall(j, l, false, false, callSites.length(), uses);
    }
    addCallSite(site);
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

        
        JS_ALWAYS_TRUE(ToNumber(cx, lhs, &ld));
        JS_ALWAYS_TRUE(ToNumber(cx, rhs, &rd));
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
mjit::Compiler::constantFoldBranch(jsbytecode *target, bool taken)
{
    if (taken) {
        if (!frame.syncForBranch(target, Uses(0)))
            return false;
        Jump j = masm.jump();
        if (!jumpAndTrace(j, target))
            return false;
    } else {
        



        if (target < PC && !finishLoop(target))
            return false;
    }
    return true;
}

bool
mjit::Compiler::emitStubCmpOp(BoolStub stub, jsbytecode *target, JSOp fused)
{
    if (target)
        frame.syncAndKillEverything();
    else
        frame.syncAndKill(Uses(2));

    prepareStubCall(Uses(2));
    INLINE_STUBCALL(stub, target ? REJOIN_BRANCH : REJOIN_PUSH_BOOLEAN);
    frame.popn(2);

    if (!target) {
        frame.takeReg(Registers::ReturnReg);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
        return true;
    }

    JS_ASSERT(fused == JSOP_IFEQ || fused == JSOP_IFNE);
    Jump j = masm.branchTest32(GetStubCompareCondition(fused), Registers::ReturnReg,
                               Registers::ReturnReg);
    return jumpAndTrace(j, target);
}

void
mjit::Compiler::jsop_setprop_slow(JSAtom *atom, bool usePropCache)
{
    prepareStubCall(Uses(2));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    if (usePropCache)
        INLINE_STUBCALL(STRICT_VARIANT(stubs::SetName), REJOIN_FALLTHROUGH);
    else
        INLINE_STUBCALL(STRICT_VARIANT(stubs::SetPropNoCache), REJOIN_FALLTHROUGH);
    JS_STATIC_ASSERT(JSOP_SETNAME_LENGTH == JSOP_SETPROP_LENGTH);
    frame.shimmy(1);
}

void
mjit::Compiler::jsop_getprop_slow(JSAtom *atom, bool usePropCache)
{
    
    RejoinState rejoin = usePropCache ? REJOIN_GETTER : REJOIN_THIS_PROTOTYPE;

    prepareStubCall(Uses(1));
    if (usePropCache) {
        INLINE_STUBCALL(stubs::GetProp, rejoin);
        testPushedType(rejoin, -1,  false);
    } else {
        masm.move(ImmPtr(atom), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::GetPropNoCache, rejoin);
    }

    frame.pop();
    frame.pushSynced(JSVAL_TYPE_UNKNOWN);
}

bool
mjit::Compiler::jsop_callprop_slow(JSAtom *atom)
{
    prepareStubCall(Uses(1));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::CallProp, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, -1,  false);
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
    FrameEntry *top = frame.peek(-1);

    




    RejoinState rejoin = REJOIN_GETTER;
    if (!usePropCache) {
        JS_ASSERT(top->isType(JSVAL_TYPE_OBJECT) &&
                  atom == cx->runtime->atomState.classPrototypeAtom);
        rejoin = REJOIN_THIS_PROTOTYPE;
    }

    
    if (atom == cx->runtime->atomState.lengthAtom &&
        top->isType(JSVAL_TYPE_STRING) &&
        (!cx->typeInferenceEnabled() || knownPushedType(0) == JSVAL_TYPE_INT32)) {
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

    if (JSOp(*PC) == JSOP_LENGTH && cx->typeInferenceEnabled() &&
        !hasTypeBarriers(PC) && knownPushedType(0) == JSVAL_TYPE_INT32) {
        
        if (loop && loop->generatingInvariants()) {
            CrossSSAValue topv(a->inlineIndex, analysis->poppedValue(PC, 0));
            FrameEntry *fe = loop->invariantLength(topv);
            if (fe) {
                frame.learnType(fe, JSVAL_TYPE_INT32, false);
                frame.pop();
                frame.pushCopyOf(fe);
                return true;
            }
        }

        types::TypeSet *types = analysis->poppedTypes(PC, 0);

        




        if (!types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_DENSE_ARRAY)) {
            bool isObject = top->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, top);
                stubcc.linkExit(notObject, Uses(1));
                stubcc.leave();
                OOL_STUBCALL(stubs::GetProp, rejoin);
                if (rejoin == REJOIN_GETTER)
                    testPushedType(rejoin, -1);
            }
            RegisterID reg = frame.tempRegForData(top);
            frame.pop();
            frame.pushWord(Address(reg, offsetof(JSObject, privateData)), JSVAL_TYPE_INT32);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            return true;
        }

        



        if (!types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_TYPED_ARRAY)) {
            bool isObject = top->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, top);
                stubcc.linkExit(notObject, Uses(1));
                stubcc.leave();
                OOL_STUBCALL(stubs::GetProp, rejoin);
                if (rejoin == REJOIN_GETTER)
                    testPushedType(rejoin, -1);
            }
            RegisterID reg = frame.copyDataIntoReg(top);
            frame.pop();
            frame.pushWord(Address(reg, TypedArray::lengthOffset()), JSVAL_TYPE_INT32);
            frame.freeReg(reg);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            return true;
        }

        




        if (types->isLazyArguments(cx)) {
            frame.pop();
            frame.pushWord(Address(JSFrameReg, StackFrame::offsetOfArgs()), JSVAL_TYPE_INT32);
            return true;
        }
    }

    
    if (loop && loop->generatingInvariants() && !hasTypeBarriers(PC)) {
        CrossSSAValue topv(a->inlineIndex, analysis->poppedValue(PC, 0));
        FrameEntry *fe = loop->invariantProperty(topv, ATOM_TO_JSID(atom));
        if (fe) {
            if (knownType != JSVAL_TYPE_UNKNOWN && knownType != JSVAL_TYPE_DOUBLE)
                frame.learnType(fe, knownType, false);
            frame.pop();
            frame.pushCopyOf(fe);
            return true;
        }
    }

    




    jsid id = ATOM_TO_JSID(atom);
    types::TypeSet *types = frame.extra(top).types;
    if (types && !types->unknownObject() &&
        types->getObjectCount() == 1 &&
        types->getTypeObject(0) != NULL &&
        !types->getTypeObject(0)->unknownProperties() &&
        id == types::MakeTypeId(cx, id)) {
        JS_ASSERT(usePropCache);
        types::TypeObject *object = types->getTypeObject(0);
        types::TypeSet *propertyTypes = object->getProperty(cx, id, false);
        if (!propertyTypes)
            return false;
        if (propertyTypes->isDefiniteProperty() &&
            !propertyTypes->isOwnProperty(cx, object, true)) {
            types->addFreeze(cx);
            uint32 slot = propertyTypes->definiteSlot();
            bool isObject = top->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, top);
                stubcc.linkExit(notObject, Uses(1));
                stubcc.leave();
                OOL_STUBCALL(stubs::GetProp, rejoin);
                if (rejoin == REJOIN_GETTER)
                    testPushedType(rejoin, -1);
            }
            RegisterID reg = frame.tempRegForData(top);
            frame.pop();

            Address address(reg, JSObject::getFixedSlotOffset(slot));
            BarrierState barrier = pushAddressMaybeBarrier(address, knownType, false);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            finishBarrier(barrier, rejoin, 0);

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

    







    pic.canCallHook = pic.forcedTypeBarrier =
        usePropCache && JSOp(*PC) == JSOP_GETPROP && analysis->getCode(PC).accessGetter;
    if (pic.canCallHook)
        frame.syncAndKillEverything();

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
    pic.slowPathCall = OOL_STUBCALL(usePropCache ? ic::GetProp : ic::GetPropNoCache, rejoin);
    CHECK_OOL_SPACE();
    if (rejoin == REJOIN_GETTER)
        testPushedType(rejoin, -1);

    
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

    CHECK_IC_SPACE();

    pic.objReg = objReg;
    frame.pushRegs(shapeReg, objReg, knownType);
    BarrierState barrier = testBarrier(pic.shapeReg, pic.objReg, false, false,
                                        pic.canCallHook);

    stubcc.rejoin(Changes(1));
    pics.append(pic);

    finishBarrier(barrier, rejoin, 0);
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

    pic.canCallHook = pic.forcedTypeBarrier = analysis->getCode(PC).accessGetter;
    if (pic.canCallHook)
        frame.syncAndKillEverything();

    



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
    pic.slowPathCall = OOL_STUBCALL(ic::CallProp, REJOIN_FALLTHROUGH);
    CHECK_OOL_SPACE();

    testPushedType(REJOIN_FALLTHROUGH, -1);

    
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

    CHECK_IC_SPACE();

    
    frame.pop();
    frame.pushRegs(shapeReg, objReg, knownPushedType(0));
    BarrierState barrier = testBarrier(pic.shapeReg, pic.objReg, false, false,
                                        pic.canCallHook);

    pushSyncedEntry(1);

    stubcc.rejoin(Changes(2));
    pics.append(pic);

    finishBarrier(barrier, REJOIN_FALLTHROUGH, 1);
    return true;
}

bool
mjit::Compiler::jsop_callprop_str(JSAtom *atom)
{
    if (!globalObj) {
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

    pic.canCallHook = pic.forcedTypeBarrier = analysis->getCode(PC).accessGetter;
    if (pic.canCallHook)
        frame.syncAndKillEverything();

    
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
    pic.slowPathCall = OOL_STUBCALL(ic::CallProp, REJOIN_FALLTHROUGH);
    CHECK_OOL_SPACE();

    testPushedType(REJOIN_FALLTHROUGH, -1);

    
    Label dslotsLoadLabel = masm.loadPtrWithPatchToLEA(Address(objReg, offsetof(JSObject, slots)),
                                                               objReg);

    
    Address slot(objReg, 1 << 24);

    Label fastValueLoad = masm.loadValueWithAddressOffsetPatch(slot, shapeReg, objReg);

    pic.fastPathRejoin = masm.label();
    pic.objReg = objReg;

    CHECK_IC_SPACE();

    






    frame.dup();
    frame.storeRegs(-2, shapeReg, objReg, knownPushedType(0));
    BarrierState barrier = testBarrier(shapeReg, objReg, false, false,
                                        pic.canCallHook);

    



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

    finishBarrier(barrier, REJOIN_FALLTHROUGH, 1);
    return true;
}

bool
mjit::Compiler::testSingletonProperty(JSObject *obj, jsid id)
{
    












    JSObject *nobj = obj;
    while (nobj) {
        if (!nobj->isNative())
            return false;
        if (nobj->getClass()->ops.lookupProperty)
            return false;
        nobj = nobj->getProto();
    }

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
    if (!types || types->unknownObject())
        return false;

    JSObject *singleton = types->getSingleton(cx);
    if (singleton)
        return testSingletonProperty(singleton, id);

    if (!globalObj)
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
            types::TypeObject *object = types->getTypeObject(0);
            if (object && object->proto) {
                if (!testSingletonProperty(object->proto, id))
                    return false;
                types->addFreeze(cx);

                
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
mjit::Compiler::jsop_callprop_dispatch(JSAtom *atom)
{
    





    FrameEntry *top = frame.peek(-1);
    if (top->isNotType(JSVAL_TYPE_OBJECT))
        return false;

    jsid id = ATOM_TO_JSID(atom);
    if (id != types::MakeTypeId(cx, id))
        return false;

    types::TypeSet *pushedTypes = pushedTypeSet(0);
    if (pushedTypes->unknownObject() || pushedTypes->baseFlags() != 0)
        return false;

    
    for (unsigned i = 0; i < pushedTypes->getObjectCount(); i++) {
        if (pushedTypes->getTypeObject(i) != NULL)
            return false;
    }

    types::TypeSet *objTypes = analysis->poppedTypes(PC, 0);
    if (objTypes->unknownObject() || objTypes->getObjectCount() == 0)
        return false;

    pushedTypes->addFreeze(cx);

    
    Vector<JSObject *> results(CompilerAllocPolicy(cx, *this));

    



    uint32 last = 0;
    for (unsigned i = 0; i < objTypes->getObjectCount(); i++) {
        if (objTypes->getSingleObject(i) != NULL)
            return false;
        types::TypeObject *object = objTypes->getTypeObject(i);
        if (!object) {
            results.append((JSObject *) NULL);
            continue;
        }
        if (object->unknownProperties() || !object->proto)
            return false;
        types::TypeSet *ownTypes = object->getProperty(cx, id, false);
        if (ownTypes->isOwnProperty(cx, object, false))
            return false;

        if (!testSingletonProperty(object->proto, id))
            return false;

        if (object->proto->getType(cx)->unknownProperties())
            return false;
        types::TypeSet *protoTypes = object->proto->type()->getProperty(cx, id, false);
        if (!protoTypes)
            return false;
        JSObject *singleton = protoTypes->getSingleton(cx);
        if (!singleton)
            return false;

        results.append(singleton);
        last = i;
    }

    if (oomInVector)
        return false;

    objTypes->addFreeze(cx);

    

    frame.forgetMismatchedObject(top);

    if (!top->isType(JSVAL_TYPE_OBJECT)) {
        Jump notObject = frame.testObject(Assembler::NotEqual, top);
        stubcc.linkExit(notObject, Uses(1));
    }

    RegisterID reg = frame.tempRegForData(top);
    frame.pinReg(reg);
    RegisterID pushreg = frame.allocReg();
    frame.unpinReg(reg);

    Address typeAddress(reg, JSObject::offsetOfType());

    Vector<Jump> rejoins(CompilerAllocPolicy(cx, *this));
    MaybeJump lastMiss;

    for (unsigned i = 0; i < objTypes->getObjectCount(); i++) {
        types::TypeObject *object = objTypes->getTypeObject(i);
        if (!object) {
            JS_ASSERT(results[i] == NULL);
            continue;
        }
        if (lastMiss.isSet())
            lastMiss.get().linkTo(masm.label(), &masm);

        




        if (!pushedTypes->hasType(types::Type::ObjectType(results[i]))) {
            JS_ASSERT(hasTypeBarriers(PC));
            if (i == last) {
                stubcc.linkExit(masm.jump(), Uses(1));
                break;
            } else {
                lastMiss.setJump(masm.branchPtr(Assembler::NotEqual, typeAddress, ImmPtr(object)));
                stubcc.linkExit(masm.jump(), Uses(1));
                continue;
            }
        }

        if (i == last) {
            masm.move(ImmPtr(results[i]), pushreg);
            break;
        } else {
            lastMiss.setJump(masm.branchPtr(Assembler::NotEqual, typeAddress, ImmPtr(object)));
            masm.move(ImmPtr(results[i]), pushreg);
            rejoins.append(masm.jump());
        }
    }

    for (unsigned i = 0; i < rejoins.length(); i++)
        rejoins[i].linkTo(masm.label(), &masm);

    stubcc.leave();
    stubcc.masm.move(ImmPtr(atom), Registers::ArgReg1);
    OOL_STUBCALL(stubs::CallProp, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, -1);

    frame.dup();
    

    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, pushreg);
    

    frame.shift(-2);
    

    stubcc.rejoin(Changes(2));
    return true;
}

bool
mjit::Compiler::jsop_callprop(JSAtom *atom)
{
    FrameEntry *top = frame.peek(-1);

    
    bool testObject;
    JSObject *singleton = pushedSingleton(0);
    if (singleton && singleton->isFunction() && !hasTypeBarriers(PC) &&
        testSingletonPropertyTypes(top, ATOM_TO_JSID(atom), &testObject)) {
        if (testObject) {
            Jump notObject = frame.testObject(Assembler::NotEqual, top);
            stubcc.linkExit(notObject, Uses(1));
            stubcc.leave();
            stubcc.masm.move(ImmPtr(atom), Registers::ArgReg1);
            OOL_STUBCALL(stubs::CallProp, REJOIN_FALLTHROUGH);
            testPushedType(REJOIN_FALLTHROUGH, -1);
        }

        

        frame.dup();
        

        frame.push(ObjectValue(*singleton));
        

        frame.shift(-2);
        

        if (testObject)
            stubcc.rejoin(Changes(2));

        return true;
    }

    
    if (cx->typeInferenceEnabled()) {
        if (jsop_callprop_dispatch(atom))
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
    FrameEntry *lhs = frame.peek(-2);
    FrameEntry *rhs = frame.peek(-1);

    
    if (lhs->isTypeKnown() && lhs->getKnownType() != JSVAL_TYPE_OBJECT) {
        jsop_setprop_slow(atom, usePropCache);
        return true;
    }

    



    if (cx->typeInferenceEnabled() && js_CodeSpec[*PC].format & JOF_NAME) {
        ScriptAnalysis::NameAccess access =
            analysis->resolveNameAccess(cx, ATOM_TO_JSID(atom), true);
        if (access.nesting) {
            Address address = frame.loadNameAddress(access);
            frame.storeTo(rhs, address, popGuaranteed);
            frame.shimmy(1);
            frame.freeReg(address.base);
            return true;
        }
    }

    



    jsid id = ATOM_TO_JSID(atom);
    types::TypeSet *types = frame.extra(lhs).types;
    if (JSOp(*PC) == JSOP_SETPROP && id == types::MakeTypeId(cx, id) &&
        types && !types->unknownObject() &&
        types->getObjectCount() == 1 &&
        types->getTypeObject(0) != NULL &&
        !types->getTypeObject(0)->unknownProperties()) {
        JS_ASSERT(usePropCache);
        types::TypeObject *object = types->getTypeObject(0);
        types::TypeSet *propertyTypes = object->getProperty(cx, id, false);
        if (!propertyTypes)
            return false;
        if (propertyTypes->isDefiniteProperty() &&
            !propertyTypes->isOwnProperty(cx, object, true)) {
            types->addFreeze(cx);
            uint32 slot = propertyTypes->definiteSlot();
            bool isObject = lhs->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, lhs);
                stubcc.linkExit(notObject, Uses(2));
                stubcc.leave();
                stubcc.masm.move(ImmPtr(atom), Registers::ArgReg1);
                OOL_STUBCALL(STRICT_VARIANT(stubs::SetName), REJOIN_FALLTHROUGH);
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
        pic.typeMonitored = true;
        types::TypeSet *types = frame.extra(rhs).types;
        if (!types) {
            
            types = types::TypeSet::make(cx, "unknownRHS");
            if (!types)
                return false;
            types->addType(cx, types::Type::UnknownType());
        }
        pic.rhsTypes = types;
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
            OOL_STUBCALL(STRICT_VARIANT(stubs::SetName), REJOIN_FALLTHROUGH);
        else
            OOL_STUBCALL(STRICT_VARIANT(stubs::SetPropNoCache), REJOIN_FALLTHROUGH);
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
        pic.slowPathCall = OOL_STUBCALL(ic::SetProp, REJOIN_FALLTHROUGH);
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
mjit::Compiler::jsop_name(JSAtom *atom, JSValueType type, bool isCall)
{
    




    if (cx->typeInferenceEnabled()) {
        ScriptAnalysis::NameAccess access =
            analysis->resolveNameAccess(cx, ATOM_TO_JSID(atom), true);
        if (access.nesting) {
            Address address = frame.loadNameAddress(access);
            JSValueType type = knownPushedType(0);
            BarrierState barrier = pushAddressMaybeBarrier(address, type, true,
                                                            true);
            finishBarrier(barrier, REJOIN_GETTER, 0);
            if (isCall)
                jsop_callgname_epilogue();
            return;
        }
    }

    PICGenInfo pic(isCall ? ic::PICInfo::CALLNAME : ic::PICInfo::NAME, JSOp(*PC), true);

    RESERVE_IC_SPACE(masm);

    pic.shapeReg = frame.allocReg();
    pic.objReg = frame.allocReg();
    pic.typeReg = Registers::ReturnReg;
    pic.atom = atom;
    pic.hasTypeCheck = false;
    pic.fastPathStart = masm.label();

    RejoinState rejoin = isCall ? REJOIN_FALLTHROUGH : REJOIN_GETTER;

    
    pic.shapeGuard = masm.label();
    Jump inlineJump = masm.jump();
    {
        RESERVE_OOL_SPACE(stubcc.masm);
        pic.slowPathStart = stubcc.linkExit(inlineJump, Uses(0));
        stubcc.leave();
        passICAddress(&pic);
        pic.slowPathCall = OOL_STUBCALL(isCall ? ic::CallName : ic::Name, rejoin);
        CHECK_OOL_SPACE();
        testPushedType(rejoin, 0);
    }
    pic.fastPathRejoin = masm.label();

    
    ScopeNameLabels &labels = pic.scopeNameLabels();
    labels.setInlineJump(masm, pic.fastPathStart, inlineJump);

    CHECK_IC_SPACE();

    




    JSObject *singleton = pushedSingleton(0);
    if (singleton) {
        frame.push(ObjectValue(*singleton));
        frame.freeReg(pic.shapeReg);
        frame.freeReg(pic.objReg);
    } else {
        frame.pushRegs(pic.shapeReg, pic.objReg, type);
    }
    if (isCall)
        frame.pushSynced(JSVAL_TYPE_UNKNOWN);
    BarrierState barrier = testBarrier(pic.shapeReg, pic.objReg,  true);

    stubcc.rejoin(Changes(isCall ? 2 : 1));

    pics.append(pic);

    finishBarrier(barrier, rejoin, isCall ? 1 : 0);
}

bool
mjit::Compiler::jsop_xname(JSAtom *atom)
{
    



    if (cx->typeInferenceEnabled()) {
        ScriptAnalysis::NameAccess access =
            analysis->resolveNameAccess(cx, ATOM_TO_JSID(atom), true);
        if (access.nesting) {
            frame.pop();
            Address address = frame.loadNameAddress(access);
            JSValueType type = knownPushedType(0);
            BarrierState barrier = pushAddressMaybeBarrier(address, type, true,
                                                            true);
            finishBarrier(barrier, REJOIN_GETTER, 0);
            return true;
        }
    }

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
        pic.slowPathCall = OOL_STUBCALL(ic::XName, REJOIN_GETTER);
        CHECK_OOL_SPACE();
        testPushedType(REJOIN_GETTER, -1);
    }

    pic.fastPathRejoin = masm.label();

    RETURN_IF_OOM(false);

    
    ScopeNameLabels &labels = pic.scopeNameLabels();
    labels.setInlineJumpOffset(masm.differenceBetween(pic.fastPathStart, inlineJump));

    CHECK_IC_SPACE();

    frame.pop();
    frame.pushRegs(pic.shapeReg, pic.objReg, knownPushedType(0));

    BarrierState barrier = testBarrier(pic.shapeReg, pic.objReg,  true);

    stubcc.rejoin(Changes(1));

    pics.append(pic);

    finishBarrier(barrier, REJOIN_FALLTHROUGH, 0);
    return true;
}

void
mjit::Compiler::jsop_bindname(JSAtom *atom, bool usePropCache)
{
    



    if (cx->typeInferenceEnabled()) {
        ScriptAnalysis::NameAccess access =
            analysis->resolveNameAccess(cx, ATOM_TO_JSID(atom), true);
        if (access.nesting) {
            RegisterID reg = frame.allocReg();
            JSObject **pobj = &access.nesting->activeCall;
            masm.move(ImmPtr(pobj), reg);
            masm.loadPtr(Address(reg), reg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, reg);
            return;
        }
    }

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
        pic.slowPathCall = OOL_STUBCALL(ic::BindName, REJOIN_FALLTHROUGH);
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
mjit::Compiler::jsop_name(JSAtom *atom, JSValueType type, bool isCall)
{
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(isCall ? stubs::CallName : stubs::Name, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, 0,  false);
    frame.pushSynced(type);
    if (isCall)
        frame.pushSynced(JSVAL_TYPE_UNKNOWN);
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
    Address scopeChain(JSFrameReg, StackFrame::offsetOfScopeChain());
    masm.loadPtr(scopeChain, reg);

    Address address(reg, offsetof(JSObject, parent));

    Jump j = masm.branchPtr(Assembler::NotEqual, address, ImmPtr(0));

    stubcc.linkExit(j, Uses(0));
    stubcc.leave();
    if (usePropCache) {
        OOL_STUBCALL(stubs::BindName, REJOIN_FALLTHROUGH);
    } else {
        stubcc.masm.move(ImmPtr(atom), Registers::ArgReg1);
        OOL_STUBCALL(stubs::BindNameNoCache, REJOIN_FALLTHROUGH);
    }

    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, reg);

    stubcc.rejoin(Changes(1));
}
#endif

void
mjit::Compiler::jsop_this()
{
    frame.pushThis();

    




    if (script->hasFunction && !script->strictModeCode) {
        FrameEntry *thisFe = frame.peek(-1);

        



        JS_ASSERT(!thisFe->isNotType(JSVAL_TYPE_OBJECT));

        if (!thisFe->isType(JSVAL_TYPE_OBJECT)) {
            JSValueType type = cx->typeInferenceEnabled()
                ? types::TypeScript::ThisTypes(script)->getKnownTypeTag(cx)
                : JSVAL_TYPE_UNKNOWN;
            if (type != JSVAL_TYPE_OBJECT) {
                Jump notObj = frame.testObject(Assembler::NotEqual, thisFe);
                stubcc.linkExit(notObj, Uses(1));
                stubcc.leave();
                OOL_STUBCALL(stubs::This, REJOIN_FALLTHROUGH);
                stubcc.rejoin(Changes(1));
            }

            






            if (cx->typeInferenceEnabled() && knownPushedType(0) != JSVAL_TYPE_OBJECT)
                return;

            
            frame.pop();
            frame.learnThisIsObject(type != JSVAL_TYPE_OBJECT);
            frame.pushThis();
        }

        JS_ASSERT(thisFe->isType(JSVAL_TYPE_OBJECT));
    }
}

bool
mjit::Compiler::iter(uintN flags)
{
    FrameEntry *fe = frame.peek(-1);

    



    if ((flags != JSITER_ENUMERATE) || fe->isNotType(JSVAL_TYPE_OBJECT)) {
        prepareStubCall(Uses(1));
        masm.move(Imm32(flags), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::Iter, REJOIN_FALLTHROUGH);
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

    
    masm.loadPtr(&script->compartment()->nativeIterCache.last, ioreg);

    
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

    
    masm.loadPtr(Address(reg, JSObject::offsetOfType()), T1);
    masm.loadPtr(Address(T1, offsetof(types::TypeObject, proto)), T1);
    masm.loadShape(T1, T1);
    masm.loadPtr(Address(nireg, offsetof(NativeIterator, shapes_array)), T2);
    masm.load32(Address(T2, sizeof(uint32)), T2);
    Jump mismatchedProto = masm.branch32(Assembler::NotEqual, T1, T2);
    stubcc.linkExit(mismatchedProto, Uses(1));

    





    masm.loadPtr(Address(reg, JSObject::offsetOfType()), T1);
    masm.loadPtr(Address(T1, offsetof(types::TypeObject, proto)), T1);
    masm.loadPtr(Address(T1, JSObject::offsetOfType()), T1);
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
    OOL_STUBCALL(stubs::Iter, REJOIN_FALLTHROUGH);

    
    frame.pop();
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, ioreg);

    stubcc.rejoin(Changes(1));

    return true;
}





void
mjit::Compiler::iterNext(ptrdiff_t offset)
{
    FrameEntry *fe = frame.peek(-offset);
    RegisterID reg = frame.tempRegForData(fe);

    
    frame.pinReg(reg);
    RegisterID T1 = frame.allocReg();
    frame.unpinReg(reg);

    
    Jump notFast = masm.testObjClass(Assembler::NotEqual, reg, &IteratorClass);
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
    stubcc.masm.move(Imm32(offset), Registers::ArgReg1);
    OOL_STUBCALL(stubs::IterNext, REJOIN_FALLTHROUGH);

    frame.pushUntypedPayload(JSVAL_TYPE_STRING, T3);

    
    stubcc.rejoin(Changes(1));
}

bool
mjit::Compiler::iterMore(jsbytecode *target)
{
    if (!frame.syncForBranch(target, Uses(1)))
        return false;

    FrameEntry *fe = frame.peek(-1);
    RegisterID reg = frame.tempRegForData(fe);
    RegisterID tempreg = frame.allocReg();

    
    Jump notFast = masm.testObjClass(Assembler::NotEqual, reg, &IteratorClass);
    stubcc.linkExitForBranch(notFast);

    
    masm.loadObjPrivate(reg, reg);

    
    notFast = masm.branchTest32(Assembler::NonZero, Address(reg, offsetof(NativeIterator, flags)),
                                Imm32(JSITER_FOREACH));
    stubcc.linkExitForBranch(notFast);

    
    masm.loadPtr(Address(reg, offsetof(NativeIterator, props_cursor)), tempreg);
    masm.loadPtr(Address(reg, offsetof(NativeIterator, props_end)), reg);

    Jump jFast = masm.branchPtr(Assembler::LessThan, tempreg, reg);

    stubcc.leave();
    OOL_STUBCALL(stubs::IterMore, REJOIN_BRANCH);
    Jump j = stubcc.masm.branchTest32(Assembler::NonZero, Registers::ReturnReg,
                                      Registers::ReturnReg);

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

    
    Jump notIterator = masm.testObjClass(Assembler::NotEqual, reg, &IteratorClass);
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
    OOL_STUBCALL(stubs::EndIter, REJOIN_FALLTHROUGH);

    frame.pop();

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_getgname_slow(uint32 index)
{
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(stubs::GetGlobalName, REJOIN_GETTER);
    testPushedType(REJOIN_GETTER, 0,  false);
    frame.pushSynced(JSVAL_TYPE_UNKNOWN);
}

void
mjit::Compiler::jsop_bindgname()
{
    if (globalObj) {
        frame.push(ObjectValue(*globalObj));
        return;
    }

    
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(stubs::BindGlobalName, REJOIN_NONE);
    frame.takeReg(Registers::ReturnReg);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
}

void
mjit::Compiler::jsop_getgname(uint32 index)
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

    
    JSObject *obj = pushedSingleton(0);
    if (obj && !hasTypeBarriers(PC) && testSingletonProperty(globalObj, ATOM_TO_JSID(atom))) {
        frame.push(ObjectValue(*obj));
        return;
    }

    jsid id = ATOM_TO_JSID(atom);
    JSValueType type = knownPushedType(0);
    if (cx->typeInferenceEnabled() && globalObj->isGlobal() && id == types::MakeTypeId(cx, id) &&
        !globalObj->getType(cx)->unknownProperties()) {
        types::TypeSet *propertyTypes = globalObj->getType(cx)->getProperty(cx, id, false);
        if (!propertyTypes)
            return;

        




        const js::Shape *shape = globalObj->nativeLookup(ATOM_TO_JSID(atom));
        if (shape && shape->hasDefaultGetterOrIsMethod() && shape->hasSlot()) {
            Value *value = &globalObj->getSlotRef(shape->slot);
            if (!value->isUndefined() &&
                !propertyTypes->isOwnProperty(cx, globalObj->getType(cx), true)) {
                watchGlobalReallocation();
                RegisterID reg = frame.allocReg();
                masm.move(ImmPtr(value), reg);

                BarrierState barrier = pushAddressMaybeBarrier(Address(reg), type, true);
                finishBarrier(barrier, REJOIN_GETTER, 0);
                return;
            }
        }
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
    ic.slowPathCall = OOL_STUBCALL(ic::GetGlobalName, REJOIN_GETTER);

    CHECK_IC_SPACE();

    testPushedType(REJOIN_GETTER, 0);

    
    uint32 slot = 1 << 24;

    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);
    Address address(objReg, slot);

    
    RegisterID treg = frame.allocReg();
    
    RegisterID dreg = objReg;

    ic.load = masm.loadValueWithAddressOffsetPatch(address, treg, dreg);

    frame.pushRegs(treg, dreg, type);

    





    BarrierState barrier = testBarrier(treg, dreg);

    stubcc.rejoin(Changes(1));

    getGlobalNames.append(ic);
    finishBarrier(barrier, REJOIN_GETTER, 0);

#else
    jsop_getgname_slow(index);
#endif

}





void
mjit::Compiler::jsop_callgname_epilogue()
{
    


    if (!globalObj) {
        prepareStubCall(Uses(1));
        INLINE_STUBCALL(stubs::PushImplicitThisForGlobal, REJOIN_NONE);
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
        if (obj->getGlobal() == globalObj) {
            frame.push(UndefinedValue());
        } else {
            prepareStubCall(Uses(1));
            INLINE_STUBCALL(stubs::PushImplicitThisForGlobal, REJOIN_NONE);
            frame.pushSynced(JSVAL_TYPE_UNKNOWN);
        }
        return;
    }

    





    if (cx->typeInferenceEnabled()) {
        types::TypeSet *types = analysis->pushedTypes(PC, 0);
        if (types->hasGlobalObject(cx, globalObj)) {
            frame.push(UndefinedValue());
            return;
        }
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
    OOL_STUBCALL(stubs::PushImplicitThisForGlobal, REJOIN_NONE);

    
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
        INLINE_STUBCALL(STRICT_VARIANT(stubs::SetGlobalName), REJOIN_FALLTHROUGH);
    else
        INLINE_STUBCALL(STRICT_VARIANT(stubs::SetGlobalNameNoCache), REJOIN_FALLTHROUGH);
    frame.popn(2);
    pushSyncedEntry(0);
}

void
mjit::Compiler::jsop_setgname(JSAtom *atom, bool usePropertyCache, bool popGuaranteed)
{
    if (monitored(PC)) {
        
        jsop_setgname_slow(atom, usePropertyCache);
        return;
    }

    jsid id = ATOM_TO_JSID(atom);
    if (cx->typeInferenceEnabled() && globalObj->isGlobal() && id == types::MakeTypeId(cx, id) &&
        !globalObj->getType(cx)->unknownProperties()) {
        





        types::TypeSet *types = globalObj->getType(cx)->getProperty(cx, id, false);
        if (!types)
            return;
        const js::Shape *shape = globalObj->nativeLookup(ATOM_TO_JSID(atom));
        if (shape && !shape->isMethod() && shape->hasDefaultSetter() &&
            shape->writable() && shape->hasSlot() &&
            !types->isOwnProperty(cx, globalObj->getType(cx), true)) {
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
    ic.slowPathCall = OOL_STUBCALL(ic::SetGlobalName, REJOIN_FALLTHROUGH);

    
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
    INLINE_STUBCALL(STRICT_VARIANT(stubs::SetElem), REJOIN_FALLTHROUGH);
    frame.popn(3);
    frame.pushSynced(JSVAL_TYPE_UNKNOWN);
}

void
mjit::Compiler::jsop_getelem_slow()
{
    prepareStubCall(Uses(2));
    INLINE_STUBCALL(stubs::GetElem, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, -2,  false);
    frame.popn(2);
    pushSyncedEntry(0);
}

void
mjit::Compiler::jsop_unbrand()
{
    prepareStubCall(Uses(1));
    INLINE_STUBCALL(stubs::Unbrand, REJOIN_FALLTHROUGH);
}

bool
mjit::Compiler::jsop_instanceof()
{
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
        OOL_STUBCALL(stubs::InstanceOf, REJOIN_FALLTHROUGH);
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

    
    masm.loadPtr(Address(obj, JSObject::offsetOfType()), obj);
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
    OOL_STUBCALL(stubs::FastInstanceOf, REJOIN_FALLTHROUGH);

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
    INLINE_STUBCALL(stubs::Eval, REJOIN_FALLTHROUGH);
    frame.popn(argc + 2);
    pushSyncedEntry(0);
}

void
mjit::Compiler::jsop_arguments(RejoinState rejoin)
{
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(stubs::Arguments, rejoin);
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
        baseobj = globalObj ? script->getObject(fullAtomIndex(PC)) : NULL;
        break;
      default:
        JS_NOT_REACHED("Bad op");
        return false;
    }

    void *stub, *stubArg;
    if (isArray) {
        stub = JS_FUNC_TO_DATA_PTR(void *, stubs::NewInitArray);
        stubArg = (void *) count;
    } else {
        stub = JS_FUNC_TO_DATA_PTR(void *, stubs::NewInitObject);
        stubArg = (void *) baseobj;
    }

    
    types::TypeObject *type = NULL;
    if (globalObj) {
        type = types::TypeScript::InitObject(cx, script, PC,
                                             isArray ? JSProto_Array : JSProto_Object);
        if (!type)
            return false;
    }

    if (!cx->typeInferenceEnabled() ||
        !globalObj ||
        (isArray && count >= gc::GetGCKindSlots(gc::FINALIZE_OBJECT_LAST)) ||
        (!isArray && !baseobj) ||
        (!isArray && baseobj->hasSlotsArray())) {
        prepareStubCall(Uses(0));
        masm.storePtr(ImmPtr(type), FrameAddress(offsetof(VMFrame, scratch)));
        masm.move(ImmPtr(stubArg), Registers::ArgReg1);
        INLINE_STUBCALL(stub, REJOIN_FALLTHROUGH);
        frame.pushSynced(JSVAL_TYPE_OBJECT);

        frame.extra(frame.peek(-1)).initArray = (*PC == JSOP_NEWARRAY);
        frame.extra(frame.peek(-1)).initObject = baseobj;

        return true;
    }

    JSObject *templateObject;
    if (isArray) {
        templateObject = NewDenseUnallocatedArray(cx, count);
        if (!templateObject)
            return false;
        templateObject->setType(type);
    } else {
        templateObject = CopyInitializerObject(cx, baseobj, type);
        if (!templateObject)
            return false;
    }

    RegisterID result = frame.allocReg();
    Jump emptyFreeList = masm.getNewObject(cx, result, templateObject);

    stubcc.linkExit(emptyFreeList, Uses(0));
    stubcc.leave();

    stubcc.masm.storePtr(ImmPtr(type), FrameAddress(offsetof(VMFrame, scratch)));
    stubcc.masm.move(ImmPtr(stubArg), Registers::ArgReg1);
    OOL_STUBCALL(stub, REJOIN_FALLTHROUGH);

    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, result);

    stubcc.rejoin(Changes(1));

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

    LoopState *nloop = cx->new_<LoopState>(cx, &ssa, this, &frame);
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
        JaegerSpew(JSpew_Regalloc, "loop allocation at %u:", unsigned(head - script->code));
        frame.dumpAllocation(alloc);
    }
#endif

    loop->entryJump().linkTo(masm.label(), &masm);

    jsbytecode *oldPC = PC;

    PC = entryTarget;
    {
        OOL_STUBCALL(stubs::MissedBoundsCheckEntry, REJOIN_RESUME);

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

        OOL_STUBCALL(stubs::MissedBoundsCheckHead, REJOIN_RESUME);

        if (loop->generatingInvariants()) {
            if (oomInVector)
                return false;
            entry.label = callSites[callSites.length() - 1].loopJumpLabel;
        } else {
            entry.label = stubcc.masm.label();
        }

        






        for (uint32 slot = ArgSlot(0); slot < TotalSlots(script); slot++) {
            if (a->varTypes[slot].type == JSVAL_TYPE_DOUBLE) {
                FrameEntry *fe = frame.getSlotEntry(slot);
                stubcc.masm.ensureInMemoryDouble(frame.addressOf(fe));
            }
        }

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
                



                Label start = stubcc.masm.label();
                stubcc.linkExitDirect(j, start);
                frame.prepareForJump(target, stubcc.masm, false);
                if (!stubcc.jumpInScript(stubcc.masm.jump(), target))
                    return false;
                if (trampoline)
                    *trampoline = true;
                if (script->pcCounters) {
                    



                    uint32 offset = ssa.frameLength(a->inlineIndex) + PC - script->code;
                    size_t length = stubcc.masm.size() - stubcc.masm.distanceOf(start);
                    pcLengths[offset].codeLength += length;
                }
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

        OOL_STUBCALL(stubs::InvokeTracer, REJOIN_NONE);

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
    
    frame.syncAndForgetEverything();
    masm.move(ImmPtr(obj), Registers::ArgReg1);
    uint32 n = js_GetEnterBlockStackDefs(cx, script, PC);
    INLINE_STUBCALL(stubs::EnterBlock, REJOIN_NONE);
    frame.enterBlock(n);
}

void
mjit::Compiler::leaveBlock()
{
    



    uint32 n = js_GetVariableStackUses(JSOP_LEAVEBLOCK, PC);
    JSObject *obj = script->getObject(fullAtomIndex(PC + UINT16_LEN));
    prepareStubCall(Uses(n));
    masm.move(ImmPtr(obj), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::LeaveBlock, REJOIN_NONE);
    frame.leaveBlock(n);
}









bool
mjit::Compiler::constructThis()
{
    JS_ASSERT(isConstructing);

    JSFunction *fun = script->function();

    do {
        if (!cx->typeInferenceEnabled() || fun->getType(cx)->unknownProperties())
            break;

        jsid id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
        types::TypeSet *protoTypes = fun->getType(cx)->getProperty(cx, id, false);

        JSObject *proto = protoTypes->getSingleton(cx, true);
        if (!proto)
            break;

        




        types::TypeObject *type = proto->getNewType(cx, fun);
        if (!type)
            return false;
        if (!types::TypeScript::ThisTypes(script)->hasType(types::Type::ObjectType(type)))
            break;

        JSObject *templateObject = js_CreateThisForFunctionWithProto(cx, fun, proto);
        if (!templateObject)
            return false;

        




        if (templateObject->type()->newScript)
            types::TypeSet::WatchObjectStateChange(cx, templateObject->type());

        RegisterID result = frame.allocReg();
        Jump emptyFreeList = masm.getNewObject(cx, result, templateObject);

        stubcc.linkExit(emptyFreeList, Uses(0));
        stubcc.leave();

        stubcc.masm.move(ImmPtr(proto), Registers::ArgReg1);
        OOL_STUBCALL(stubs::CreateThis, REJOIN_RESUME);

        frame.setThis(result);

        stubcc.rejoin(Changes(1));
        return true;
    } while (false);

    
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
    INLINE_STUBCALL(stubs::CreateThis, REJOIN_RESUME);
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
    JSOp op = JSOp(*originalPC);
    JS_ASSERT(op == JSOP_TABLESWITCH || op == JSOP_TABLESWITCHX);

    uint32 defaultTarget = GetJumpOffset(pc, pc);
    unsigned jumpLength = (op == JSOP_TABLESWITCHX) ? JUMPX_OFFSET_LEN : JUMP_OFFSET_LEN;
    pc += jumpLength;

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

        
        INLINE_STUBCALL(stubs::TableSwitch, REJOIN_NONE);
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
        uint32 target = GetJumpOffset(originalPC, pc);
        if (!target)
            target = defaultTarget;
        uint32 offset = (originalPC + target) - script->code;
        jumpTableOffsets.append(offset);
        pc += jumpLength;
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
        OOL_STUBCALL(stubs::TableSwitch, REJOIN_NONE);
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
    INLINE_STUBCALL(stubs::CallElem, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, -2,  false);
    frame.popn(2);
    pushSyncedEntry(0);
    pushSyncedEntry(1);
}

void
mjit::Compiler::jsop_toid()
{
    
    FrameEntry *top = frame.peek(-1);

    if (top->isType(JSVAL_TYPE_INT32))
        return;

    if (top->isNotType(JSVAL_TYPE_INT32)) {
        prepareStubCall(Uses(2));
        INLINE_STUBCALL(stubs::ToId, REJOIN_FALLTHROUGH);
        frame.pop();
        pushSyncedEntry(0);
        return;
    }

    frame.syncAt(-1);

    Jump j = frame.testInt32(Assembler::NotEqual, top);
    stubcc.linkExit(j, Uses(2));

    stubcc.leave();
    OOL_STUBCALL(stubs::ToId, REJOIN_FALLTHROUGH);

    frame.pop();
    pushSyncedEntry(0);

    stubcc.rejoin(Changes(1));
}













void
mjit::Compiler::fixDoubleTypes(jsbytecode *target)
{
    if (!cx->typeInferenceEnabled())
        return;

    








    JS_ASSERT(fixedIntToDoubleEntries.empty());
    JS_ASSERT(fixedDoubleToAnyEntries.empty());
    const SlotValue *newv = analysis->newValues(target);
    if (newv) {
        while (newv->slot) {
            if (newv->value.kind() != SSAValue::PHI ||
                newv->value.phiOffset() != uint32(target - script->code) ||
                !analysis->trackSlot(newv->slot)) {
                newv++;
                continue;
            }
            JS_ASSERT(newv->slot < TotalSlots(script));
            types::TypeSet *targetTypes = analysis->getValueTypes(newv->value);
            FrameEntry *fe = frame.getSlotEntry(newv->slot);
            VarType &vt = a->varTypes[newv->slot];
            if (targetTypes->getKnownTypeTag(cx) == JSVAL_TYPE_DOUBLE) {
                if (vt.type == JSVAL_TYPE_INT32) {
                    fixedIntToDoubleEntries.append(newv->slot);
                    frame.ensureDouble(fe);
                    frame.forgetLoopReg(fe);
                } else if (vt.type == JSVAL_TYPE_UNKNOWN) {
                    





                    frame.ensureDouble(fe);
                } else {
                    JS_ASSERT(vt.type == JSVAL_TYPE_DOUBLE);
                }
            } else if (vt.type == JSVAL_TYPE_DOUBLE) {
                fixedDoubleToAnyEntries.append(newv->slot);
                frame.syncAndForgetFe(fe);
                frame.forgetLoopReg(fe);
            }
            newv++;
        }
    }
}

void
mjit::Compiler::watchGlobalReallocation()
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (hasGlobalReallocation)
        return;
    types::TypeSet::WatchObjectStateChange(cx, globalObj->getType(cx));
    hasGlobalReallocation = true;
}

void
mjit::Compiler::updateVarType()
{
    if (!cx->typeInferenceEnabled())
        return;

    






    types::TypeSet *types = pushedTypeSet(0);
    uint32 slot = GetBytecodeSlot(script, PC);

    if (analysis->trackSlot(slot)) {
        VarType &vt = a->varTypes[slot];
        vt.types = types;
        vt.type = types->getKnownTypeTag(cx);

        




        if (vt.type == JSVAL_TYPE_DOUBLE)
            frame.ensureDouble(frame.getSlotEntry(slot));
    }
}

void
mjit::Compiler::updateJoinVarTypes()
{
    if (!cx->typeInferenceEnabled())
        return;

    
    const SlotValue *newv = analysis->newValues(PC);
    if (newv) {
        while (newv->slot) {
            if (newv->slot < TotalSlots(script)) {
                VarType &vt = a->varTypes[newv->slot];
                vt.types = analysis->getValueTypes(newv->value);
                JSValueType newType = vt.types->getKnownTypeTag(cx);
                if (newType != vt.type) {
                    FrameEntry *fe = frame.getSlotEntry(newv->slot);
                    frame.forgetLoopReg(fe);
                }
                vt.type = newType;
            }
            newv++;
        }
    }
}

void
mjit::Compiler::restoreVarType()
{
    if (!cx->typeInferenceEnabled())
        return;

    uint32 slot = GetBytecodeSlot(script, PC);

    if (slot >= analyze::TotalSlots(script))
        return;

    




    JSValueType type = a->varTypes[slot].type;
    if (type != JSVAL_TYPE_UNKNOWN &&
        (type != JSVAL_TYPE_DOUBLE || analysis->trackSlot(slot))) {
        FrameEntry *fe = frame.getSlotEntry(slot);
        JS_ASSERT_IF(fe->isTypeKnown(), fe->isType(type));
        if (!fe->isTypeKnown())
            frame.learnType(fe, type, false);
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
    return types->hasType(types::Type::UndefinedType());
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
    if (!cx->typeInferenceEnabled())
        return false;
    return analysis->getCode(pc).monitoredTypes;
}

bool
mjit::Compiler::hasTypeBarriers(jsbytecode *pc)
{
    if (!cx->typeInferenceEnabled())
        return false;

    return analysis->typeBarriers(pc) != NULL;
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
    if (!cx->typeInferenceEnabled() || !globalObj)
        return true;

    JSObject *proto;
    if (!js_GetClassPrototype(cx, NULL, JSProto_Array, &proto, NULL))
        return false;

    




    if (proto->getType(cx)->unknownProperties())
        return true;
    types::TypeSet *arrayTypes = proto->getType(cx)->getProperty(cx, JSID_VOID, false);
    return !arrayTypes || arrayTypes->knownNonEmpty(cx);
}



































mjit::Compiler::BarrierState
mjit::Compiler::pushAddressMaybeBarrier(Address address, JSValueType type, bool reuseBase,
                                        bool testUndefined)
{
    if (!hasTypeBarriers(PC) && !testUndefined) {
        frame.push(address, type, reuseBase);
        return BarrierState();
    }

    RegisterID typeReg, dataReg;
    frame.loadIntoRegisters(address, reuseBase, &typeReg, &dataReg);

    frame.pushRegs(typeReg, dataReg, type);
    return testBarrier(typeReg, dataReg, testUndefined);
}

MaybeJump
mjit::Compiler::trySingleTypeTest(types::TypeSet *types, RegisterID typeReg)
{
    





    MaybeJump res;

    switch (types->getKnownTypeTag(cx)) {
      case JSVAL_TYPE_INT32:
        res.setJump(masm.testInt32(Assembler::NotEqual, typeReg));
        return res;

      case JSVAL_TYPE_DOUBLE:
        res.setJump(masm.testNumber(Assembler::NotEqual, typeReg));
        return res;

      case JSVAL_TYPE_BOOLEAN:
        res.setJump(masm.testBoolean(Assembler::NotEqual, typeReg));
        return res;

      case JSVAL_TYPE_STRING:
        res.setJump(masm.testString(Assembler::NotEqual, typeReg));
        return res;

      default:
        return res;
    }
}

JSC::MacroAssembler::Jump
mjit::Compiler::addTypeTest(types::TypeSet *types, RegisterID typeReg, RegisterID dataReg)
{
    





    Vector<Jump> matches(CompilerAllocPolicy(cx, *this));

    if (types->hasType(types::Type::Int32Type()))
        matches.append(masm.testInt32(Assembler::Equal, typeReg));

    if (types->hasType(types::Type::DoubleType()))
        matches.append(masm.testDouble(Assembler::Equal, typeReg));

    if (types->hasType(types::Type::UndefinedType()))
        matches.append(masm.testUndefined(Assembler::Equal, typeReg));

    if (types->hasType(types::Type::BooleanType()))
        matches.append(masm.testBoolean(Assembler::Equal, typeReg));

    if (types->hasType(types::Type::StringType()))
        matches.append(masm.testString(Assembler::Equal, typeReg));

    if (types->hasType(types::Type::NullType()))
        matches.append(masm.testNull(Assembler::Equal, typeReg));

    unsigned count = 0;
    if (types->hasType(types::Type::AnyObjectType()))
        matches.append(masm.testObject(Assembler::Equal, typeReg));
    else
        count = types->getObjectCount();

    if (count != 0) {
        Jump notObject = masm.testObject(Assembler::NotEqual, typeReg);
        Address typeAddress(dataReg, JSObject::offsetOfType());

        




        Jump notSingleton = masm.branchTest32(Assembler::Zero,
                                              Address(dataReg, offsetof(JSObject, flags)),
                                              Imm32(JSObject::SINGLETON_TYPE));

        for (unsigned i = 0; i < count; i++) {
            if (JSObject *object = types->getSingleObject(i))
                matches.append(masm.branchPtr(Assembler::Equal, dataReg, ImmPtr(object)));
        }

        Jump singletonMismatch = masm.jump();

        notSingleton.linkTo(masm.label(), &masm);

        for (unsigned i = 0; i < count; i++) {
            if (types::TypeObject *object = types->getTypeObject(i))
                matches.append(masm.branchPtr(Assembler::Equal, typeAddress, ImmPtr(object)));
        }

        singletonMismatch.linkTo(masm.label(), &masm);
        notObject.linkTo(masm.label(), &masm);
    }

    Jump mismatch = masm.jump();

    for (unsigned i = 0; i < matches.length(); i++)
        matches[i].linkTo(masm.label(), &masm);

    return mismatch;
}

mjit::Compiler::BarrierState
mjit::Compiler::testBarrier(RegisterID typeReg, RegisterID dataReg,
                            bool testUndefined, bool testReturn, bool force)
{
    BarrierState state;
    state.typeReg = typeReg;
    state.dataReg = dataReg;

    if (!cx->typeInferenceEnabled() || !(js_CodeSpec[*PC].format & JOF_TYPESET))
        return state;

    types::TypeSet *types = analysis->bytecodeTypes(PC);
    if (types->unknown()) {
        



        return state;
    }

    if (testReturn) {
        JS_ASSERT(!testUndefined);
        if (!analysis->getCode(PC).monitoredTypesReturn)
            return state;
    } else if (!hasTypeBarriers(PC) && !force) {
        if (testUndefined && !types->hasType(types::Type::UndefinedType()))
            state.jump.setJump(masm.testUndefined(Assembler::Equal, typeReg));
        return state;
    }

    types->addFreeze(cx);

    
    JS_ASSERT(!types->unknown());

    state.jump = trySingleTypeTest(types, typeReg);
    if (!state.jump.isSet())
        state.jump.setJump(addTypeTest(types, typeReg, dataReg));

    return state;
}

void
mjit::Compiler::finishBarrier(const BarrierState &barrier, RejoinState rejoin, uint32 which)
{
    if (!barrier.jump.isSet())
        return;

    stubcc.linkExitDirect(barrier.jump.get(), stubcc.masm.label());

    





    frame.pushSynced(JSVAL_TYPE_UNKNOWN);
    stubcc.masm.storeValueFromComponents(barrier.typeReg, barrier.dataReg,
                                         frame.addressOf(frame.peek(-1)));
    frame.pop();

    stubcc.syncExit(Uses(0));
    stubcc.leave();

    stubcc.masm.move(ImmPtr((void *) which), Registers::ArgReg1);
    OOL_STUBCALL(stubs::TypeBarrierHelper, rejoin);
    stubcc.rejoin(Changes(0));
}

void
mjit::Compiler::testPushedType(RejoinState rejoin, int which, bool ool)
{
    if (!cx->typeInferenceEnabled() || !(js_CodeSpec[*PC].format & JOF_TYPESET))
        return;

    types::TypeSet *types = analysis->bytecodeTypes(PC);
    if (types->unknown())
        return;

    Assembler &masm = ool ? stubcc.masm : this->masm;

    JS_ASSERT(which <= 0);
    Address address = (which == 0) ? frame.addressOfTop() : frame.addressOf(frame.peek(which));

    Vector<Jump> mismatches(cx);
    if (!masm.generateTypeCheck(cx, address, types, &mismatches)) {
        oomInVector = true;
        return;
    }

    Jump j = masm.jump();

    for (unsigned i = 0; i < mismatches.length(); i++)
        mismatches[i].linkTo(masm.label(), &masm);

    masm.move(Imm32(which), Registers::ArgReg1);
    if (ool)
        OOL_STUBCALL(stubs::StubTypeHelper, rejoin);
    else
        INLINE_STUBCALL(stubs::StubTypeHelper, rejoin);

    j.linkTo(masm.label(), &masm);
}
