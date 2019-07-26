






#include "mozilla/DebugOnly.h"

#include "MethodJIT.h"
#include "jsnum.h"
#include "jsbool.h"
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
#include "jsopcodeinlines.h"

#include "builtin/RegExp.h"
#include "vm/RegExpStatics.h"
#include "vm/RegExpObject.h"

#include "jsautooplen.h"
#include "jstypedarrayinlines.h"
#include "vm/RegExpObject-inl.h"

#include "ion/Ion.h"

#if JS_TRACE_LOGGING
#include "TraceLogging.h"
#endif

using namespace js;
using namespace js::mjit;
#if defined(JS_POLYIC) || defined(JS_MONOIC)
using namespace js::mjit::ic;
#endif
using namespace js::analyze;

using mozilla::DebugOnly;

#define RETURN_IF_OOM(retval)                                   \
    JS_BEGIN_MACRO                                              \
        if (oomInVector || masm.oom() || stubcc.masm.oom())     \
            return retval;                                      \
    JS_END_MACRO

static inline bool IsIonEnabled(JSContext *cx)
{
#ifdef JS_ION
    return ion::IsEnabled(cx);
#else
    return false;
#endif
}





static const size_t USES_BEFORE_INLINING = 10240;

mjit::Compiler::Compiler(JSContext *cx, JSScript *outerScript,
                         unsigned chunkIndex, bool isConstructing)
  : BaseCompiler(cx),
    outerScript(cx, outerScript),
    chunkIndex(chunkIndex),
    isConstructing(isConstructing),
    outerChunk(outerJIT()->chunkDescriptor(chunkIndex)),
    ssa(cx, outerScript),
    globalObj(cx, outerScript->compileAndGo ? &outerScript->global() : NULL),
    globalSlots(globalObj ? globalObj->getRawSlots() : NULL),
    sps(&cx->runtime->spsProfiler),
    masm(&sps, &PC),
    frame(cx, *thisFromCtor(), masm, stubcc),
    a(NULL), outer(NULL), script_(cx), PC(NULL), loop(NULL),
    inlineFrames(CompilerAllocPolicy(cx, *thisFromCtor())),
    branchPatches(CompilerAllocPolicy(cx, *thisFromCtor())),
#if defined JS_MONOIC
    getGlobalNames(CompilerAllocPolicy(cx, *thisFromCtor())),
    setGlobalNames(CompilerAllocPolicy(cx, *thisFromCtor())),
    callICs(CompilerAllocPolicy(cx, *thisFromCtor())),
    equalityICs(CompilerAllocPolicy(cx, *thisFromCtor())),
#endif
#if defined JS_POLYIC
    pics(CompilerAllocPolicy(cx, *thisFromCtor())),
    getElemICs(CompilerAllocPolicy(cx, *thisFromCtor())),
    setElemICs(CompilerAllocPolicy(cx, *thisFromCtor())),
#endif
    callPatches(CompilerAllocPolicy(cx, *thisFromCtor())),
    callSites(CompilerAllocPolicy(cx, *thisFromCtor())),
    compileTriggers(CompilerAllocPolicy(cx, *thisFromCtor())),
    doubleList(CompilerAllocPolicy(cx, *thisFromCtor())),
    rootedTemplates(CompilerAllocPolicy(cx, *thisFromCtor())),
    rootedRegExps(CompilerAllocPolicy(cx, *thisFromCtor())),
    monitoredBytecodes(CompilerAllocPolicy(cx, *thisFromCtor())),
    typeBarrierBytecodes(CompilerAllocPolicy(cx, *thisFromCtor())),
    fixedIntToDoubleEntries(CompilerAllocPolicy(cx, *thisFromCtor())),
    fixedDoubleToAnyEntries(CompilerAllocPolicy(cx, *thisFromCtor())),
    jumpTables(CompilerAllocPolicy(cx, *thisFromCtor())),
    jumpTableEdges(CompilerAllocPolicy(cx, *thisFromCtor())),
    loopEntries(CompilerAllocPolicy(cx, *thisFromCtor())),
    chunkEdges(CompilerAllocPolicy(cx, *thisFromCtor())),
    stubcc(cx, *thisFromCtor(), frame),
    debugMode_(cx->compartment->debugMode()),
    inlining_(false),
    hasGlobalReallocation(false),
    oomInVector(false),
    overflowICSpace(false),
    gcNumber(cx->runtime->gcNumber),
    pcLengths(NULL)
{
    if (!IsIonEnabled(cx)) {
        
        if (!debugMode() && cx->typeInferenceEnabled() && globalObj &&
            (outerScript->getUseCount() >= USES_BEFORE_INLINING ||
             cx->hasOption(JSOPTION_METHODJIT_ALWAYS))) {
            inlining_ = true;
        }
    }
}

CompileStatus
mjit::Compiler::compile()
{
    JS_ASSERT(!outerChunkRef().chunk);

#if JS_TRACE_LOGGING
    AutoTraceLog logger(TraceLogging::defaultLogger(),
                        TraceLogging::JM_COMPILE_START,
                        TraceLogging::JM_COMPILE_STOP,
                        outerScript);
#endif

    CompileStatus status = performCompilation();
    if (status != Compile_Okay && status != Compile_Retry) {
        if (!outerScript->ensureHasMJITInfo(cx))
            return Compile_Error;
        JSScript::JITScriptHandle *jith = outerScript->jitHandle(isConstructing, cx->zone()->compileBarriers());
        JSScript::ReleaseCode(cx->runtime->defaultFreeOp(), jith);
        jith->setUnjittable();

        if (outerScript->function()) {
            outerScript->uninlineable = true;
            types::MarkTypeObjectFlags(cx, outerScript->function(),
                                       types::OBJECT_FLAG_UNINLINEABLE);
        }
    }

    return status;
}

CompileStatus
mjit::Compiler::checkAnalysis(HandleScript script)
{
    if (!script->ensureRanAnalysis(cx))
        return Compile_Error;

    if (!script->analysis()->jaegerCompileable()) {
        JaegerSpew(JSpew_Abort, "script has uncompileable opcodes\n");
        return Compile_Abort;
    }

    if (cx->typeInferenceEnabled() && !script->ensureRanInference(cx))
        return Compile_Error;

    ScriptAnalysis *analysis = script->analysis();
    analysis->assertMatchingDebugMode();
    if (analysis->failed()) {
        JaegerSpew(JSpew_Abort, "couldn't analyze bytecode; probably switchX or OOM\n");
        return Compile_Abort;
    }

    return Compile_Okay;
}

CompileStatus
mjit::Compiler::addInlineFrame(HandleScript script, uint32_t depth,
                               uint32_t parent, jsbytecode *parentpc)
{
    JS_ASSERT(inlining());

    CompileStatus status = checkAnalysis(script);
    if (status != Compile_Okay)
        return status;

    if (!ssa.addInlineFrame(script, depth, parent, parentpc))
        return Compile_Error;

    uint32_t index = ssa.iterFrame(ssa.numFrames() - 1).index;
    return scanInlineCalls(index, depth);
}

CompileStatus
mjit::Compiler::scanInlineCalls(uint32_t index, uint32_t depth)
{
    
    static const uint32_t INLINE_SITE_LIMIT = 5;

    JS_ASSERT(inlining() && globalObj);

    
    if (isConstructing)
        return Compile_Okay;

    JSScript *script = ssa.getFrame(index).script;
    ScriptAnalysis *analysis = script->analysis();

    
    if (!script->compileAndGo ||
        &script->global() != globalObj ||
        (script->function() && script->function()->getParent() != globalObj) ||
        (script->function() && script->function()->isHeavyweight()) ||
        script->isActiveEval) {
        return Compile_Okay;
    }

    uint32_t nextOffset = 0;
    uint32_t lastOffset = script->length;

    if (index == CrossScriptSSA::OUTER_FRAME) {
        nextOffset = outerChunk.begin;
        lastOffset = outerChunk.end;
    }

    while (nextOffset < lastOffset) {
        uint32_t offset = nextOffset;
        jsbytecode *pc = script->code + offset;
        nextOffset = offset + GetBytecodeLength(pc);

        Bytecode *code = analysis->maybeCode(pc);
        if (!code)
            continue;

        
        if (JSOp(*pc) != JSOP_CALL)
            continue;

        
        if (code->monitoredTypes || code->monitoredTypesReturn || analysis->typeBarriers(cx, pc) != NULL)
            continue;

        uint32_t argc = GET_ARGC(pc);
        types::StackTypeSet *calleeTypes = analysis->poppedTypes(pc, argc + 1);

        if (calleeTypes->getKnownTypeTag() != JSVAL_TYPE_OBJECT)
            continue;

        if (calleeTypes->getObjectCount() >= INLINE_SITE_LIMIT)
            continue;

        





        uint32_t stackLimit = outerScript->nslots + StackSpace::STACK_JIT_EXTRA
            - VALUES_PER_STACK_FRAME - FrameState::TEMPORARY_LIMIT - 1;

        
        uint32_t nextDepth = depth + VALUES_PER_STACK_FRAME + script->nfixed + code->stackDepth;

        



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

            JSFunction *fun = obj->toFunction();
            if (!fun->isInterpreted()) {
                okay = false;
                break;
            }
            RootedScript script(cx, fun->nonLazyScript());

            





            if (!script->hasAnalysis() || !script->analysis()->ranInference()) {
                okay = false;
                break;
            }

            
            if (script->hasScriptCounts != outerScript->hasScriptCounts) {
                okay = false;
                break;
            }

            




            if (!globalObj ||
                fun->getParent() != globalObj ||
                outerScript->strict != script->strict) {
                okay = false;
                break;
            }

            
            uint32_t nindex = index;
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

            if (!script->types) {
                okay = false;
                break;
            }

            CompileStatus status = checkAnalysis(script);
            if (status != Compile_Okay)
                return status;

            if (!script->analysis()->jaegerInlineable(argc)) {
                okay = false;
                break;
            }

            types::TypeObject *funType = fun->getType(cx);
            if (!funType ||
                types::HeapTypeSet::HasObjectFlags(cx, funType, types::OBJECT_FLAG_UNINLINEABLE))
            {
                okay = false;
                break;
            }

            




            types::HeapTypeSet::WatchObjectStateChange(cx, funType);

            





            if (script->analysis()->usesThisValue() &&
                types::TypeScript::ThisTypes(script)->getKnownTypeTag() != JSVAL_TYPE_OBJECT) {
                okay = false;
                break;
            }
        }
        if (!okay)
            continue;

        



        for (unsigned i = 0; i < count; i++) {
            JSObject *obj = calleeTypes->getSingleObject(i);
            if (!obj)
                continue;

            JSFunction *fun = obj->toFunction();
            RootedScript script(cx, fun->nonLazyScript());

            CompileStatus status = addInlineFrame(script, nextDepth, index, pc);
            if (status != Compile_Okay)
                return status;
        }
    }

    return Compile_Okay;
}

CompileStatus
mjit::Compiler::pushActiveFrame(JSScript *scriptArg, uint32_t argc)
{
    RootedScript script(cx, scriptArg);
    if (cx->runtime->profilingScripts && !script->hasScriptCounts)
        script->initScriptCounts(cx);

    ActiveFrame *newa = js_new<ActiveFrame>(cx);
    if (!newa) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }

    newa->parent = a;
    if (a)
        newa->parentPC = PC;
    newa->script = script;
    newa->mainCodeStart = masm.size();
    newa->stubCodeStart = stubcc.size();

    if (outer) {
        newa->inlineIndex = uint32_t(inlineFrames.length());
        inlineFrames.append(newa);
    } else {
        newa->inlineIndex = CrossScriptSSA::OUTER_FRAME;
        outer = newa;
    }
    JS_ASSERT(ssa.getFrame(newa->inlineIndex).script == script);

    newa->inlinePCOffset = ssa.frameLength(newa->inlineIndex);

    ScriptAnalysis *newAnalysis = script->analysis();

#ifdef JS_METHODJIT_SPEW
    if (cx->typeInferenceEnabled() && IsJaegerSpewChannelActive(JSpew_Regalloc)) {
        unsigned nargs = script->function() ? script->function()->nargs : 0;
        for (unsigned i = 0; i < nargs; i++) {
            uint32_t slot = ArgSlot(i);
            if (!newAnalysis->slotEscapes(slot)) {
                JaegerSpew(JSpew_Regalloc, "Argument %u:", i);
                newAnalysis->liveness(slot).print();
            }
        }
        for (unsigned i = 0; i < script->nfixed; i++) {
            uint32_t slot = LocalSlot(script, i);
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

    newa->jumpMap = js_pod_malloc<Label>(script->length);
    if (!newa->jumpMap) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }
#ifdef DEBUG
    for (uint32_t i = 0; i < script->length; i++)
        newa->jumpMap[i] = Label();
#endif

    if (cx->typeInferenceEnabled()) {
        CompileStatus status = prepareInferenceTypes(script, newa);
        if (status != Compile_Okay)
            return status;
    }

    if (script != outerScript && !sps.enterInlineFrame())
        return Compile_Error;

    this->script_ = script;
    this->analysis = newAnalysis;
    this->PC = script->code;
    this->a = newa;

    return Compile_Okay;
}

void
mjit::Compiler::popActiveFrame()
{
    JS_ASSERT(a->parent);
    a->mainCodeEnd = masm.size();
    a->stubCodeEnd = stubcc.size();
    this->PC = a->parentPC;
    this->a = (ActiveFrame *) a->parent;
    this->script_ = a->script;
    this->analysis = this->script_->analysis();

    frame.popActiveFrame();
    sps.leaveInlineFrame();
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
mjit::Compiler::performCompilation()
{
    JaegerSpew(JSpew_Scripts,
               "compiling script (file \"%s\") (line \"%d\") (length \"%d\") (chunk \"%d\") (usecount \"%d\")\n",
               outerScript->filename(), outerScript->lineno, outerScript->length, chunkIndex, (int) outerScript->getUseCount());

    if (inlining()) {
        JaegerSpew(JSpew_Inlining,
                   "inlining calls in script (file \"%s\") (line \"%d\")\n",
                   outerScript->filename(), outerScript->lineno);
    }

#ifdef JS_METHODJIT_SPEW
    Profiler prof;
    prof.start();
#endif

#ifdef JS_METHODJIT
    outerScript->debugMode = debugMode();
#endif

    JS_ASSERT(cx->compartment->activeAnalysis);

    {
        types::AutoEnterCompilation enter(cx, types::CompilerOutput::MethodJIT);
        if (!enter.init(outerScript, isConstructing, chunkIndex)) {
            js_ReportOutOfMemory(cx);
            return Compile_Error;
        }

        CHECK_STATUS(checkAnalysis(outerScript));
        if (inlining())
            CHECK_STATUS(scanInlineCalls(CrossScriptSSA::OUTER_FRAME, 0));
        CHECK_STATUS(pushActiveFrame(outerScript, 0));

        if (outerScript->hasScriptCounts || Probes::wantNativeAddressInfo(cx)) {
            size_t length = ssa.frameLength(ssa.numFrames() - 1);
            pcLengths = js_pod_calloc<PCLengthEntry>(length);
            if (!pcLengths)
                return Compile_Error;
        }

        if (chunkIndex == 0)
            CHECK_STATUS(generatePrologue());
        else
            sps.setPushed(script_);
        CHECK_STATUS(generateMethod());
        if (outerJIT() && chunkIndex == outerJIT()->nchunks - 1)
            CHECK_STATUS(generateEpilogue());
        CHECK_STATUS(finishThisUp());
    }

#ifdef JS_METHODJIT_SPEW
    prof.stop();
    JaegerSpew(JSpew_Prof, "compilation took %d us\n", prof.time_us());
#endif

    JaegerSpew(JSpew_Scripts, "successfully compiled (code \"%p\") (size \"%u\")\n",
               outerChunkRef().chunk->code.m_code.executableAddress(),
               unsigned(outerChunkRef().chunk->code.m_size));

    return Compile_Okay;
}

#undef CHECK_STATUS

mjit::JSActiveFrame::JSActiveFrame()
    : parent(NULL), parentPC(NULL), script(NULL), inlineIndex(UINT32_MAX)
{
}

mjit::Compiler::ActiveFrame::ActiveFrame(JSContext *cx)
    : jumpMap(NULL),
      varTypes(NULL), needReturnValue(false),
      syncReturnValue(false), returnValueDouble(false), returnSet(false),
      returnEntry(NULL), returnJumps(NULL), exitState(NULL)
{}

mjit::Compiler::ActiveFrame::~ActiveFrame()
{
    js_free(jumpMap);
    if (varTypes)
        js_free(varTypes);
}

mjit::Compiler::~Compiler()
{
    if (outer)
        js_delete(outer);
    for (unsigned i = 0; i < inlineFrames.length(); i++)
        js_delete(inlineFrames[i]);
    while (loop) {
        LoopState *nloop = loop->outer;
        js_delete(loop);
        loop = nloop;
    }
}

CompileStatus
mjit::Compiler::prepareInferenceTypes(JSScript *script, ActiveFrame *a)
{
    





















    a->varTypes = js_pod_calloc<VarType>(TotalSlots(script));
    if (!a->varTypes) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }

    for (uint32_t slot = ArgSlot(0); slot < TotalSlots(script); slot++) {
        VarType &vt = a->varTypes[slot];
        vt.setTypes(types::TypeScript::SlotTypes(script, slot));
    }

    return Compile_Okay;
}






static const size_t USES_BEFORE_COMPILE       = 16;
static const size_t INFER_USES_BEFORE_COMPILE = 43;


static uint32_t CHUNK_LIMIT = 1500;

void
mjit::SetChunkLimit(uint32_t limit)
{
    if (limit)
        CHUNK_LIMIT = limit;
}

JITScript *
MakeJITScript(JSContext *cx, JSScript *script)
{
    if (!script->ensureRanAnalysis(cx))
        return NULL;

    ScriptAnalysis *analysis = script->analysis();

    Vector<ChunkDescriptor> chunks(cx);
    Vector<CrossChunkEdge> edges(cx);

    if (script->length < CHUNK_LIMIT || !cx->typeInferenceEnabled()) {
        ChunkDescriptor desc;
        desc.begin = 0;
        desc.end = script->length;
        if (!chunks.append(desc))
            return NULL;
    } else {
        if (!script->ensureRanInference(cx))
            return NULL;

        
        Vector<CrossChunkEdge> currentEdges(cx);
        uint32_t chunkStart = 0;

        bool preserveNextChunk = false;
        unsigned offset, nextOffset = 0;
        while (nextOffset < script->length) {
            offset = nextOffset;

            jsbytecode *pc = script->code + offset;
            JSOp op = JSOp(*pc);

            nextOffset = offset + GetBytecodeLength(pc);

            Bytecode *code = analysis->maybeCode(offset);
            if (!code)
                op = JSOP_NOP; 

            
            bool finishChunk = false;

            
            bool preserveChunk = preserveNextChunk;
            preserveNextChunk = false;

            



            uint32_t type = JOF_TYPE(js_CodeSpec[op].format);
            if (type == JOF_JUMP && op != JSOP_LABEL) {
                CrossChunkEdge edge;
                edge.source = offset;
                edge.target = FollowBranch(cx, script, pc - script->code);
                if (edge.target < offset) {
                    
                    finishChunk = true;
                    if (edge.target < chunkStart) {
                        analysis->getCode(edge.target).safePoint = true;
                        if (!edges.append(edge))
                            return NULL;
                    }
                } else if (edge.target == nextOffset) {
                    





                    preserveChunk = true;
                } else {
                    if (!currentEdges.append(edge))
                        return NULL;
                }
            }

            if (op == JSOP_TABLESWITCH) {
                jsbytecode *pc2 = pc;
                unsigned defaultOffset = offset + GET_JUMP_OFFSET(pc);
                pc2 += JUMP_OFFSET_LEN;
                int32_t low = GET_JUMP_OFFSET(pc2);
                pc2 += JUMP_OFFSET_LEN;
                int32_t high = GET_JUMP_OFFSET(pc2);
                pc2 += JUMP_OFFSET_LEN;

                CrossChunkEdge edge;
                edge.source = offset;
                edge.target = defaultOffset;
                if (!currentEdges.append(edge))
                    return NULL;

                for (int32_t i = low; i <= high; i++) {
                    unsigned targetOffset = offset + GET_JUMP_OFFSET(pc2);
                    if (targetOffset != offset) {
                        



                        CrossChunkEdge edge;
                        edge.source = offset;
                        edge.target = targetOffset;
                        if (!currentEdges.append(edge))
                            return NULL;
                    }
                    pc2 += JUMP_OFFSET_LEN;
                }
            }

            if (unsigned(offset - chunkStart) > CHUNK_LIMIT)
                finishChunk = true;

            if (nextOffset >= script->length || !analysis->maybeCode(nextOffset)) {
                
                preserveChunk = true;
            } else {
                




                jsbytecode *nextpc = script->code + nextOffset;

                



                switch (JSOp(*nextpc)) {
                  case JSOP_POP:
                  case JSOP_IFNE:
                  case JSOP_IFEQ:
                    preserveChunk = true;
                    break;
                  default:
                    break;
                }

                uint32_t afterOffset = nextOffset + GetBytecodeLength(nextpc);
                if (afterOffset < script->length) {
                    if (analysis->maybeCode(afterOffset) &&
                        JSOp(script->code[afterOffset]) == JSOP_LOOPHEAD &&
                        analysis->getLoop(afterOffset))
                    {
                        if (preserveChunk)
                            preserveNextChunk = true;
                        else
                            finishChunk = true;
                    }
                }
            }

            if (finishChunk && !preserveChunk) {
                ChunkDescriptor desc;
                desc.begin = chunkStart;
                desc.end = nextOffset;
                if (!chunks.append(desc))
                    return NULL;

                
                if (!BytecodeNoFallThrough(op)) {
                    CrossChunkEdge edge;
                    edge.source = offset;
                    edge.target = nextOffset;
                    analysis->getCode(edge.target).safePoint = true;
                    if (!edges.append(edge))
                        return NULL;
                }

                chunkStart = nextOffset;
                for (unsigned i = 0; i < currentEdges.length(); i++) {
                    const CrossChunkEdge &edge = currentEdges[i];
                    if (edge.target >= nextOffset) {
                        analysis->getCode(edge.target).safePoint = true;
                        if (!edges.append(edge))
                            return NULL;
                    }
                }
                currentEdges.clear();
            }
        }

        if (chunkStart != script->length) {
            ChunkDescriptor desc;
            desc.begin = chunkStart;
            desc.end = script->length;
            if (!chunks.append(desc))
                return NULL;
        }
    }

    size_t dataSize = sizeof(JITScript)
        + (chunks.length() * sizeof(ChunkDescriptor))
        + (edges.length() * sizeof(CrossChunkEdge));
    uint8_t *cursor = (uint8_t *) js_calloc(dataSize);
    if (!cursor)
        return NULL;

    JITScript *jit = (JITScript *) cursor;
    cursor += sizeof(JITScript);

    jit->script = script;
    JS_INIT_CLIST(&jit->callers);

    jit->nchunks = chunks.length();
    for (unsigned i = 0; i < chunks.length(); i++) {
        const ChunkDescriptor &a = chunks[i];
        ChunkDescriptor &b = jit->chunkDescriptor(i);
        b.begin = a.begin;
        b.end = a.end;

        if (chunks.length() == 1) {
            
            b.counter = INFER_USES_BEFORE_COMPILE;
        }
    }

    if (edges.empty())
        return jit;

    jit->nedges = edges.length();
    CrossChunkEdge *jitEdges = jit->edges();
    for (unsigned i = 0; i < edges.length(); i++) {
        const CrossChunkEdge &a = edges[i];
        CrossChunkEdge &b = jitEdges[i];
        b.source = a.source;
        b.target = a.target;
    }

    
    jsbytecode *pc;
    MJITInstrumentation sps(&cx->runtime->spsProfiler);
    Assembler masm(&sps, &pc);
    sps.setPushed(script);
    for (unsigned i = 0; i < jit->nedges; i++) {
        pc = script->code + jitEdges[i].target;
        jitEdges[i].shimLabel = (void *) masm.distanceOf(masm.label());
        masm.move(JSC::MacroAssembler::ImmPtr(&jitEdges[i]), Registers::ArgReg1);
        masm.fallibleVMCall(true, JS_FUNC_TO_DATA_PTR(void *, stubs::CrossChunkShim),
                            pc, NULL, script->nfixed + analysis->getCode(pc).stackDepth);
    }
    LinkerHelper linker(masm, JSC::JAEGER_CODE);
    JSC::ExecutablePool *ep = linker.init(cx);
    if (!ep)
        return NULL;
    jit->shimPool = ep;

    masm.finalize(linker);
    uint8_t *shimCode = (uint8_t *) linker.finalizeCodeAddendum().executableAddress();

    JS_ALWAYS_TRUE(linker.verifyRange(JSC::JITCode(shimCode, masm.size())));

    JaegerSpew(JSpew_PICs, "generated SHIM POOL stub %p (%lu bytes)\n",
               shimCode, (unsigned long)masm.size());

    for (unsigned i = 0; i < jit->nedges; i++) {
        CrossChunkEdge &edge = jitEdges[i];
        edge.shimLabel = shimCode + (size_t) edge.shimLabel;
    }

    return jit;
}

static inline bool
IonGetsFirstChance(JSContext *cx, JSScript *script, jsbytecode *pc, CompileRequest request)
{
#ifdef JS_ION
    if (!ion::IsEnabled(cx))
        return false;

    
    
    if (script->getUseCount() < ion::js_IonOptions.usesBeforeCompile)
        return false;

    
    if (request == CompileRequest_JIT)
        return false;

    
    if (!script->canIonCompile())
        return false;

    
    if (script->hasIonScript() && script->ion->bailoutExpected())
        return false;

    
    
    
    
    if (ion::js_IonOptions.parallelCompilation && script->hasIonScript() &&
        pc && script->ionScript()->osrPc() && script->ionScript()->osrPc() != pc &&
        script->getUseCount() >= ion::js_IonOptions.usesBeforeCompile * 2)
    {
        return false;
    }

    
    
    if (script->ion == ION_COMPILING_SCRIPT)
        return false;

    return true;
#endif
    return false;
}

CompileStatus
mjit::CanMethodJIT(JSContext *cx, JSScript *script, jsbytecode *pc,
                   bool construct, CompileRequest request, StackFrame *frame)
{
    bool compiledOnce = false;
  checkOutput:
    if (!cx->methodJitEnabled)
        return Compile_Abort;

    













    if (frame->script() == script && pc != script->code) {
        if (frame->hasPushedSPSFrame() != cx->runtime->spsProfiler.enabled())
            return Compile_Skipped;
    }

    if (IonGetsFirstChance(cx, script, pc, request)) {
        if (script->hasIonScript())
            script->incUseCount();
        return Compile_Skipped;
    }

    if (script->hasMJITInfo()) {
        JSScript::JITScriptHandle *jith = script->jitHandle(construct, cx->zone()->compileBarriers());
        if (jith->isUnjittable())
            return Compile_Abort;
    }

    if (!cx->hasOption(JSOPTION_METHODJIT_ALWAYS) &&
        (cx->typeInferenceEnabled()
         ? script->incUseCount() <= INFER_USES_BEFORE_COMPILE
         : script->incUseCount() <= USES_BEFORE_COMPILE))
    {
        return Compile_Skipped;
    }

    if (!cx->runtime->getJaegerRuntime(cx))
        return Compile_Error;

    
    if (construct && !script->nslots)
        script->nslots++;

    uint64_t gcNumber = cx->runtime->gcNumber;

    if (!script->ensureHasMJITInfo(cx))
        return Compile_Error;

    JSScript::JITScriptHandle *jith = script->jitHandle(construct, cx->zone()->compileBarriers());

    JITScript *jit;
    if (jith->isEmpty()) {
        jit = MakeJITScript(cx, script);
        if (!jit)
            return Compile_Error;

        
        if (gcNumber != cx->runtime->gcNumber) {
            FreeOp *fop = cx->runtime->defaultFreeOp();
            jit->destroy(fop);
            fop->free_(jit);
            return Compile_Skipped;
        }

        jith->setValid(jit);
    } else {
        jit = jith->getValid();
    }

    unsigned chunkIndex = jit->chunkIndex(pc);
    ChunkDescriptor &desc = jit->chunkDescriptor(chunkIndex);

    if (jit->mustDestroyEntryChunk) {
        
        
        
        JS_ASSERT(jit->nchunks == 1);
        jit->mustDestroyEntryChunk = false;

        if (desc.chunk) {
            jit->destroyChunk(cx->runtime->defaultFreeOp(), chunkIndex,  false);
            return Compile_Skipped;
        }
    }

    if (desc.chunk)
        return Compile_Okay;
    if (compiledOnce)
        return Compile_Skipped;

    if (!cx->hasOption(JSOPTION_METHODJIT_ALWAYS) &&
        ++desc.counter <= INFER_USES_BEFORE_COMPILE)
    {
        return Compile_Skipped;
    }

    CompileStatus status;
    {
        types::AutoEnterAnalysis enter(cx);

        Compiler cc(cx, script, chunkIndex, construct);
        status = cc.compile();
    }

    




    cx->compartment->types.maybePurgeAnalysis(cx);

    if (status == Compile_Okay) {
        



        compiledOnce = true;
        goto checkOutput;
    }

    
    JS_ASSERT_IF(status == Compile_Error,
                 cx->isExceptionPending() || cx->runtime->hadOutOfMemory);

    return status;
}

CompileStatus
mjit::Compiler::generatePrologue()
{
    fastEntryLabel = masm.label();

    



    if (script_->function()) {
        Jump j = masm.jump();

        



        fastEntryLabel = masm.label();

        
        masm.storePtr(ImmPtr(script_->function()),
                      Address(JSFrameReg, StackFrame::offsetOfExec()));
        if (script_->isCallsiteClone) {
            masm.storeValue(ObjectValue(*script_->function()),
                            Address(JSFrameReg, StackFrame::offsetOfCallee(script_->function())));
        }

        {
            





            arityLabel = stubcc.masm.label();

            Jump argMatch = stubcc.masm.branch32(Assembler::Equal, JSParamReg_Argc,
                                                 Imm32(script_->function()->nargs));

            if (JSParamReg_Argc != Registers::ArgReg1)
                stubcc.masm.move(JSParamReg_Argc, Registers::ArgReg1);

            
            stubcc.masm.storePtr(ImmPtr(script_->function()),
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
                stubcc.masm.storePtr(ImmPtr(script_->function()),
                                     Address(JSFrameReg, StackFrame::offsetOfExec()));
                OOL_STUBCALL(stubs::CheckArgumentTypes, REJOIN_CHECK_ARGUMENTS);
#ifdef JS_MONOIC
                this->argsCheckFallthrough = stubcc.masm.label();
#endif
            }

            stubcc.crossJump(stubcc.masm.jump(), fastEntryLabel);
        }

        




        uint32_t nvals = VALUES_PER_STACK_FRAME + script_->nslots + StackSpace::STACK_JIT_EXTRA;
        masm.addPtr(Imm32(nvals * sizeof(Value)), JSFrameReg, Registers::ReturnReg);
        Jump stackCheck = masm.branchPtr(Assembler::AboveOrEqual, Registers::ReturnReg,
                                         FrameAddress(offsetof(VMFrame, stackLimit)));

        






        {
            stubcc.linkExitDirect(stackCheck, stubcc.masm.label());
            OOL_STUBCALL(stubs::HitStackQuota, REJOIN_NONE);
            stubcc.crossJump(stubcc.masm.jump(), masm.label());
        }

        markUndefinedLocals();

        





        if (!script_->function()->isHeavyweight() && analysis->usesScopeChain()) {
            RegisterID t0 = Registers::ReturnReg;
            Jump hasScope = masm.branchTest32(Assembler::NonZero,
                                              FrameFlagsAddress(), Imm32(StackFrame::HAS_SCOPECHAIN));
            masm.loadPayload(Address(JSFrameReg, StackFrame::offsetOfCallee(script_->function())), t0);
            masm.loadPtr(Address(t0, JSFunction::offsetOfEnvironment()), t0);
            masm.storePtr(t0, Address(JSFrameReg, StackFrame::offsetOfScopeChain()));
            hasScope.linkTo(masm.label(), &masm);
        }

        





        if (script_->argumentsHasVarBinding()) {
            Jump hasArgs = masm.branchTest32(Assembler::NonZero, FrameFlagsAddress(),
                                             Imm32(StackFrame::UNDERFLOW_ARGS |
                                                   StackFrame::OVERFLOW_ARGS));
            masm.storePtr(ImmPtr((void *)(size_t) script_->function()->nargs),
                          Address(JSFrameReg, StackFrame::offsetOfNumActual()));
            hasArgs.linkTo(masm.label(), &masm);
        }

        j.linkTo(masm.label(), &masm);
    }

    if (cx->typeInferenceEnabled()) {
#ifdef DEBUG
        if (script_->function()) {
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::AssertArgumentTypes, REJOIN_NONE);
        }
#endif
        ensureDoubleArguments();
    }

    
    if (script_->isActiveEval && script_->strict) {
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::StrictEvalPrologue, REJOIN_EVAL_PROLOGUE);
    } else if (script_->function()) {
        if (script_->function()->isHeavyweight()) {
            prepareStubCall(Uses(0));
            INLINE_STUBCALL(stubs::HeavyweightFunctionPrologue, REJOIN_FUNCTION_PROLOGUE);
        }

        if (isConstructing && !constructThis())
            return Compile_Error;
    }

    CompileStatus status = methodEntryHelper();
    if (status == Compile_Okay) {
        if (IsIonEnabled(cx))
            ionCompileHelper();
        else
            inliningCompileHelper();
    }

    return status;
}

void
mjit::Compiler::ensureDoubleArguments()
{
    
    for (uint32_t i = 0; script_->function() && i < script_->function()->nargs; i++) {
        uint32_t slot = ArgSlot(i);
        if (a->varTypes[slot].getTypeTag() == JSVAL_TYPE_DOUBLE && analysis->trackSlot(slot))
            frame.ensureDouble(frame.getArg(i));
    }
}

void
mjit::Compiler::markUndefinedLocal(uint32_t offset, uint32_t i)
{
    uint32_t depth = ssa.getFrame(a->inlineIndex).depth;
    Address local(JSFrameReg, sizeof(StackFrame) + (depth + i) * sizeof(Value));
    masm.storeValue(UndefinedValue(), local);
}

void
mjit::Compiler::markUndefinedLocals()
{
    



    for (uint32_t i = 0; i < script_->nfixed; i++)
        markUndefinedLocal(0, i);

#ifdef DEBUG
    uint32_t depth = ssa.getFrame(a->inlineIndex).depth;
    for (uint32_t i = script_->nfixed; i < script_->nslots; i++) {
        Address local(JSFrameReg, sizeof(StackFrame) + (depth + i) * sizeof(Value));
        masm.storeValue(ObjectValueCrashOnTouch(), local);
    }
#endif
}

CompileStatus
mjit::Compiler::generateEpilogue()
{
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::finishThisUp()
{
#ifdef JS_CPU_X64
    
    for (unsigned i = 0; i < chunkEdges.length(); i++) {
        chunkEdges[i].sourceTrampoline = stubcc.masm.label();
        stubcc.masm.move(ImmPtr(NULL), Registers::ScratchReg);
        stubcc.masm.jump(Registers::ScratchReg);
    }
#endif

    RETURN_IF_OOM(Compile_Error);

    



    if (globalSlots && globalObj->getRawSlots() != globalSlots)
        return Compile_Retry;

    



    if (cx->runtime->gcNumber != gcNumber)
        return Compile_Retry;

    
    JITScript *jit = outerJIT();
    JS_ASSERT(jit != NULL);

    if (overflowICSpace) {
        JaegerSpew(JSpew_Scripts, "dumped a constant pool while generating an IC\n");
        return Compile_Abort;
    }

    a->mainCodeEnd = masm.size();
    a->stubCodeEnd = stubcc.size();

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
#if defined(JS_CPU_MIPS)
                      stubcc.size() + sizeof(double) +
#else
                      stubcc.size() +
#endif
                      (masm.numDoubles() * sizeof(double)) +
                      (stubcc.masm.numDoubles() * sizeof(double)) +
                      jumpTableEdges.length() * sizeof(void *);

    Vector<ChunkJumpTableEdge> chunkJumps(cx);
    if (!chunkJumps.reserve(jumpTableEdges.length()))
        return Compile_Error;

    JSC::ExecutableAllocator &execAlloc = cx->runtime->execAlloc();
    JSC::ExecutablePool *execPool;
    uint8_t *result = (uint8_t *)execAlloc.alloc(codeSize, &execPool, JSC::JAEGER_CODE);
    if (!result) {
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }
    JS_ASSERT(execPool);
    JSC::ExecutableAllocator::makeWritable(result, codeSize);
    masm.executableCopy(result);
    stubcc.masm.executableCopy(result + masm.size());

    JSC::LinkBuffer fullCode(result, codeSize, JSC::JAEGER_CODE);
    JSC::LinkBuffer stubCode(result + masm.size(), stubcc.size(), JSC::JAEGER_CODE);

    JS_ASSERT(!loop);

    size_t nNmapLive = loopEntries.length();
    for (size_t i = outerChunk.begin; i < outerChunk.end; i++) {
        Bytecode *opinfo = analysis->maybeCode(i);
        if (opinfo && opinfo->safePoint)
            nNmapLive++;
    }

    
    size_t dataSize = sizeof(JITChunk) +
                      sizeof(NativeMapEntry) * nNmapLive +
                      sizeof(InlineFrame) * inlineFrames.length() +
                      sizeof(CallSite) * callSites.length() +
                      sizeof(CompileTrigger) * compileTriggers.length() +
                      sizeof(JSObject*) * rootedTemplates.length() +
                      sizeof(RegExpShared*) * rootedRegExps.length() +
                      sizeof(uint32_t) * monitoredBytecodes.length() +
                      sizeof(uint32_t) * typeBarrierBytecodes.length() +
#if defined JS_MONOIC
                      sizeof(ic::GetGlobalNameIC) * getGlobalNames.length() +
                      sizeof(ic::SetGlobalNameIC) * setGlobalNames.length() +
                      sizeof(ic::CallICInfo) * callICs.length() +
                      sizeof(ic::EqualityICInfo) * equalityICs.length() +
#endif
#if defined JS_POLYIC
                      sizeof(ic::PICInfo) * pics.length() +
                      sizeof(ic::GetElementIC) * getElemICs.length() +
                      sizeof(ic::SetElementIC) * setElemICs.length() +
#endif
                      0;

    uint8_t *cursor = (uint8_t *)js_calloc(dataSize);
    if (!cursor) {
        execPool->release();
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }

    JITChunk *chunk = new(cursor) JITChunk;
    cursor += sizeof(JITChunk);

    JS_ASSERT(outerScript == script_);

    chunk->code = JSC::MacroAssemblerCodeRef(result, execPool, masm.size() + stubcc.size());
    chunk->pcLengths = pcLengths;

    if (chunkIndex == 0) {
        jit->invokeEntry = result;
        if (script_->function()) {
            jit->arityCheckEntry = stubCode.locationOf(arityLabel).executableAddress();
            jit->argsCheckEntry = stubCode.locationOf(argsCheckLabel).executableAddress();
            jit->fastEntry = fullCode.locationOf(fastEntryLabel).executableAddress();
        }
    }

    




    
    Label *jumpMap = a->jumpMap;

    
    NativeMapEntry *jitNmap = (NativeMapEntry *)cursor;
    chunk->nNmapPairs = nNmapLive;
    cursor += sizeof(NativeMapEntry) * chunk->nNmapPairs;
    size_t ix = 0;
    if (chunk->nNmapPairs > 0) {
        for (size_t i = outerChunk.begin; i < outerChunk.end; i++) {
            Bytecode *opinfo = analysis->maybeCode(i);
            if (opinfo && opinfo->safePoint) {
                Label L = jumpMap[i];
                JS_ASSERT(L.isSet());
                jitNmap[ix].bcOff = i;
                jitNmap[ix].ncode = (uint8_t *)(result + masm.distanceOf(L));
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
            jitNmap[j].ncode = (uint8_t *) stubCode.locationOf(entry.label).executableAddress();
            ix++;
        }
    }
    JS_ASSERT(ix == chunk->nNmapPairs);

    
    InlineFrame *jitInlineFrames = (InlineFrame *)cursor;
    chunk->nInlineFrames = inlineFrames.length();
    cursor += sizeof(InlineFrame) * chunk->nInlineFrames;
    for (size_t i = 0; i < chunk->nInlineFrames; i++) {
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
    chunk->nCallSites = callSites.length();
    cursor += sizeof(CallSite) * chunk->nCallSites;
    for (size_t i = 0; i < chunk->nCallSites; i++) {
        CallSite &to = jitCallSites[i];
        InternalCallSite &from = callSites[i];

        
        if (cx->typeInferenceEnabled() &&
            from.rejoin != REJOIN_TRAP &&
            from.rejoin != REJOIN_SCRIPTED &&
            from.inlineIndex != UINT32_MAX) {
            if (from.ool)
                stubCode.patch(from.inlinePatch, &to);
            else
                fullCode.patch(from.inlinePatch, &to);
        }

        JSScript *script =
            (from.inlineIndex == UINT32_MAX) ? outerScript.get()
                                             : inlineFrames[from.inlineIndex]->script;
        uint32_t codeOffset = from.ool
                            ? masm.size() + from.returnOffset
                            : from.returnOffset;
        to.initialize(codeOffset, from.inlineIndex, from.inlinepc - script->code, from.rejoin);

        




        if (from.loopPatch.hasPatch)
            stubCode.patch(from.loopPatch.codePatch, result + codeOffset);
    }

    CompileTrigger *jitCompileTriggers = (CompileTrigger *)cursor;
    chunk->nCompileTriggers = compileTriggers.length();
    cursor += sizeof(CompileTrigger) * chunk->nCompileTriggers;
    for (size_t i = 0; i < chunk->nCompileTriggers; i++) {
        const InternalCompileTrigger &trigger = compileTriggers[i];
        jitCompileTriggers[i].initialize(trigger.pc - outerScript->code,
                                         fullCode.locationOf(trigger.inlineJump),
                                         stubCode.locationOf(trigger.stubLabel));
    }

    JSObject **jitRootedTemplates = (JSObject **)cursor;
    chunk->nRootedTemplates = rootedTemplates.length();
    cursor += sizeof(JSObject*) * chunk->nRootedTemplates;
    for (size_t i = 0; i < chunk->nRootedTemplates; i++)
        jitRootedTemplates[i] = rootedTemplates[i];

    RegExpShared **jitRootedRegExps = (RegExpShared **)cursor;
    chunk->nRootedRegExps = rootedRegExps.length();
    cursor += sizeof(RegExpShared*) * chunk->nRootedRegExps;
    for (size_t i = 0; i < chunk->nRootedRegExps; i++) {
        jitRootedRegExps[i] = rootedRegExps[i];
        jitRootedRegExps[i]->incRef();
    }

    uint32_t *jitMonitoredBytecodes = (uint32_t *)cursor;
    chunk->nMonitoredBytecodes = monitoredBytecodes.length();
    cursor += sizeof(uint32_t) * chunk->nMonitoredBytecodes;
    for (size_t i = 0; i < chunk->nMonitoredBytecodes; i++)
        jitMonitoredBytecodes[i] = monitoredBytecodes[i];

    uint32_t *jitTypeBarrierBytecodes = (uint32_t *)cursor;
    chunk->nTypeBarrierBytecodes = typeBarrierBytecodes.length();
    cursor += sizeof(uint32_t) * chunk->nTypeBarrierBytecodes;
    for (size_t i = 0; i < chunk->nTypeBarrierBytecodes; i++)
        jitTypeBarrierBytecodes[i] = typeBarrierBytecodes[i];

#if defined JS_MONOIC
    if (chunkIndex == 0 && script_->function()) {
        JS_ASSERT(jit->argsCheckPool == NULL);
        if (cx->typeInferenceEnabled()) {
            jit->argsCheckStub = stubCode.locationOf(argsCheckStub);
            jit->argsCheckFallthrough = stubCode.locationOf(argsCheckFallthrough);
            jit->argsCheckJump = stubCode.locationOf(argsCheckJump);
        }
    }

    ic::GetGlobalNameIC *getGlobalNames_ = (ic::GetGlobalNameIC *)cursor;
    chunk->nGetGlobalNames = getGlobalNames.length();
    cursor += sizeof(ic::GetGlobalNameIC) * chunk->nGetGlobalNames;
    for (size_t i = 0; i < chunk->nGetGlobalNames; i++) {
        ic::GetGlobalNameIC &to = getGlobalNames_[i];
        GetGlobalNameICInfo &from = getGlobalNames[i];
        from.copyTo(to, fullCode, stubCode);

        int offset = fullCode.locationOf(from.load) - to.fastPathStart;
        to.loadStoreOffset = offset;
        JS_ASSERT(to.loadStoreOffset == offset);

        stubCode.patch(from.addrLabel, &to);
    }

    ic::SetGlobalNameIC *setGlobalNames_ = (ic::SetGlobalNameIC *)cursor;
    chunk->nSetGlobalNames = setGlobalNames.length();
    cursor += sizeof(ic::SetGlobalNameIC) * chunk->nSetGlobalNames;
    for (size_t i = 0; i < chunk->nSetGlobalNames; i++) {
        ic::SetGlobalNameIC &to = setGlobalNames_[i];
        SetGlobalNameICInfo &from = setGlobalNames[i];
        from.copyTo(to, fullCode, stubCode);
        to.slowPathStart = stubCode.locationOf(from.slowPathStart);

        int offset = fullCode.locationOf(from.store).labelAtOffset(0) -
                     to.fastPathStart;
        to.loadStoreOffset = offset;
        JS_ASSERT(to.loadStoreOffset == offset);

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
    chunk->nCallICs = callICs.length();
    cursor += sizeof(ic::CallICInfo) * chunk->nCallICs;
    for (size_t i = 0; i < chunk->nCallICs; i++) {
        jitCallICs[i].funGuardLabel = fullCode.locationOf(callICs[i].funGuardLabel);
        jitCallICs[i].funGuard = fullCode.locationOf(callICs[i].funGuard);
        jitCallICs[i].funJump = fullCode.locationOf(callICs[i].funJump);
        jitCallICs[i].slowPathStart = stubCode.locationOf(callICs[i].slowPathStart);
        jitCallICs[i].typeMonitored = callICs[i].typeMonitored;

        
        uint32_t offset = fullCode.locationOf(callICs[i].hotJump) -
                        fullCode.locationOf(callICs[i].funGuard);
        jitCallICs[i].hotJumpOffset = offset;
        JS_ASSERT(jitCallICs[i].hotJumpOffset == offset);

        
        offset = fullCode.locationOf(callICs[i].joinPoint) -
                 fullCode.locationOf(callICs[i].funGuard);
        jitCallICs[i].joinPointOffset = offset;
        JS_ASSERT(jitCallICs[i].joinPointOffset == offset);

        offset = fullCode.locationOf(callICs[i].ionJoinPoint) -
                 fullCode.locationOf(callICs[i].funGuard);
        jitCallICs[i].ionJoinOffset = offset;
        JS_ASSERT(jitCallICs[i].ionJoinOffset == offset);

        
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
        stubCode.patch(callICs[i].addrLabel1, &jitCallICs[i]);
        stubCode.patch(callICs[i].addrLabel2, &jitCallICs[i]);
    }

    ic::EqualityICInfo *jitEqualityICs = (ic::EqualityICInfo *)cursor;
    chunk->nEqualityICs = equalityICs.length();
    cursor += sizeof(ic::EqualityICInfo) * chunk->nEqualityICs;
    for (size_t i = 0; i < chunk->nEqualityICs; i++) {
        if (equalityICs[i].trampoline) {
            jitEqualityICs[i].target = stubCode.locationOf(equalityICs[i].trampolineStart);
        } else {
            uint32_t offs = uint32_t(equalityICs[i].jumpTarget - script_->code);
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
    chunk->nGetElems = getElemICs.length();
    cursor += sizeof(ic::GetElementIC) * chunk->nGetElems;
    for (size_t i = 0; i < chunk->nGetElems; i++) {
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
        int inlineShapeGuard = fullCode.locationOf(from.shapeGuard) -
                               fullCode.locationOf(from.fastPathStart);
        to.inlineShapeGuard = inlineShapeGuard;
        JS_ASSERT(to.inlineShapeGuard == inlineShapeGuard);

        stubCode.patch(from.paramAddr, &to);
    }

    ic::SetElementIC *jitSetElems = (ic::SetElementIC *)cursor;
    chunk->nSetElems = setElemICs.length();
    cursor += sizeof(ic::SetElementIC) * chunk->nSetElems;
    for (size_t i = 0; i < chunk->nSetElems; i++) {
        ic::SetElementIC &to = jitSetElems[i];
        SetElementICInfo &from = setElemICs[i];

        new (&to) ic::SetElementIC();
        from.copyTo(to, fullCode, stubCode);

        to.strictMode = script_->strict;
        to.vr = from.vr;
        to.objReg = from.objReg;
        to.objRemat = from.objRemat.toInt32();
        JS_ASSERT(to.objRemat == from.objRemat.toInt32());

        to.hasConstantKey = from.key.isConstant();
        if (from.key.isConstant())
            to.keyValue = from.key.index();
        else
            to.keyReg = from.key.reg();

        int inlineShapeGuard = fullCode.locationOf(from.shapeGuard) -
                               fullCode.locationOf(from.fastPathStart);
        to.inlineShapeGuard = inlineShapeGuard;
        JS_ASSERT(to.inlineShapeGuard == inlineShapeGuard);

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
    chunk->nPICs = pics.length();
    cursor += sizeof(ic::PICInfo) * chunk->nPICs;
    for (size_t i = 0; i < chunk->nPICs; i++) {
        new (&jitPics[i]) ic::PICInfo();
        pics[i].copyTo(jitPics[i], fullCode, stubCode);
        pics[i].copySimpleMembersTo(jitPics[i]);

        jitPics[i].shapeGuard = masm.distanceOf(pics[i].shapeGuard) -
                                masm.distanceOf(pics[i].fastPathStart);
        JS_ASSERT(jitPics[i].shapeGuard == masm.distanceOf(pics[i].shapeGuard) -
                                           masm.distanceOf(pics[i].fastPathStart));
        jitPics[i].shapeRegHasBaseShape = true;
        jitPics[i].pc = pics[i].pc;

        if (pics[i].kind == ic::PICInfo::SET) {
            jitPics[i].u.vr = pics[i].vr;
        } else if (pics[i].kind != ic::PICInfo::NAME) {
            if (pics[i].hasTypeCheck) {
                int32_t distance = stubcc.masm.distanceOf(pics[i].typeCheck) -
                                 stubcc.masm.distanceOf(pics[i].slowPathStart);
                JS_ASSERT(distance <= 0);
                jitPics[i].u.get.typeCheckOffset = distance;
            }
        }
        stubCode.patch(pics[i].paramAddr, &jitPics[i]);
    }
#endif

    JS_ASSERT(size_t(cursor - (uint8_t*)chunk) == dataSize);
    
    JS_ASSERT(chunk->computedSizeOfIncludingThis() == dataSize);

    
    stubcc.fixCrossJumps(result, masm.size(), masm.size() + stubcc.size());

#if defined(JS_CPU_MIPS)
    
    size_t doubleOffset = (((size_t)result + masm.size() + stubcc.size() +
                            sizeof(double) - 1) & (~(sizeof(double) - 1))) -
                          (size_t)result;
    JS_ASSERT((((size_t)result + doubleOffset) & 7) == 0);
#else
    size_t doubleOffset = masm.size() + stubcc.size();
#endif

    double *inlineDoubles = (double *) (result + doubleOffset);
    double *oolDoubles = (double*) (result + doubleOffset +
                                    masm.numDoubles() * sizeof(double));

    
    void **jumpVec = (void **)(oolDoubles + stubcc.masm.numDoubles());

    for (size_t i = 0; i < jumpTableEdges.length(); i++) {
        JumpTableEdge edge = jumpTableEdges[i];
        if (bytecodeInChunk(script_->code + edge.target)) {
            JS_ASSERT(jumpMap[edge.target].isSet());
            jumpVec[i] = (void *)(result + masm.distanceOf(jumpMap[edge.target]));
        } else {
            ChunkJumpTableEdge nedge;
            nedge.edge = edge;
            nedge.jumpTableEntry = &jumpVec[i];
            chunkJumps.infallibleAppend(nedge);
            jumpVec[i] = NULL;
        }
    }

    
    for (size_t i = 0; i < jumpTables.length(); i++) {
        JumpTable &jumpTable = jumpTables[i];
        fullCode.patch(jumpTable.label, &jumpVec[jumpTable.offsetIndex]);
    }

    
    masm.finalize(fullCode, inlineDoubles);
    stubcc.masm.finalize(stubCode, oolDoubles);

    JSC::ExecutableAllocator::makeExecutable(result, masm.size() + stubcc.size());
    JSC::ExecutableAllocator::cacheFlush(result, masm.size() + stubcc.size());

    a->mainCodeStart = size_t(result);
    a->mainCodeEnd   = size_t(result + masm.size());
    a->stubCodeStart = a->mainCodeEnd;
    a->stubCodeEnd   = a->mainCodeEnd + stubcc.size();
    if (!Probes::registerMJITCode(cx, chunk,
                                  a, (JSActiveFrame**) inlineFrames.begin())) {
        execPool->release();
        js_free(chunk);
        js_ReportOutOfMemory(cx);
        return Compile_Error;
    }

    outerChunkRef().chunk = chunk;

    
    CrossChunkEdge *crossEdges = jit->edges();
    for (unsigned i = 0; i < jit->nedges; i++) {
        CrossChunkEdge &edge = crossEdges[i];
        if (bytecodeInChunk(outerScript->code + edge.source)) {
            JS_ASSERT(!edge.sourceJump1 && !edge.sourceJump2);
            void *label = edge.targetLabel ? edge.targetLabel : edge.shimLabel;
            CodeLocationLabel targetLabel(label);
            JSOp op = JSOp(script_->code[edge.source]);
            if (op == JSOP_TABLESWITCH) {
                if (edge.jumpTableEntries)
                    js_free(edge.jumpTableEntries);
                CrossChunkEdge::JumpTableEntryVector *jumpTableEntries = NULL;
                bool failed = false;
                for (unsigned j = 0; j < chunkJumps.length(); j++) {
                    ChunkJumpTableEdge nedge = chunkJumps[j];
                    if (nedge.edge.source == edge.source && nedge.edge.target == edge.target) {
                        if (!jumpTableEntries) {
                            jumpTableEntries = js_new<CrossChunkEdge::JumpTableEntryVector>();
                            if (!jumpTableEntries)
                                failed = true;
                        }
                        if (!jumpTableEntries->append(nedge.jumpTableEntry))
                            failed = true;
                        *nedge.jumpTableEntry = label;
                    }
                }
                if (failed) {
                    execPool->release();
                    js_free(chunk);
                    js_ReportOutOfMemory(cx);
                    return Compile_Error;
                }
                edge.jumpTableEntries = jumpTableEntries;
            }
            for (unsigned j = 0; j < chunkEdges.length(); j++) {
                const OutgoingChunkEdge &oedge = chunkEdges[j];
                if (oedge.source == edge.source && oedge.target == edge.target) {
                    






                    edge.sourceJump1 = fullCode.locationOf(oedge.fastJump).executableAddress();
                    if (oedge.slowJump.isSet()) {
                        edge.sourceJump2 =
                            stubCode.locationOf(oedge.slowJump.get()).executableAddress();
                    }
#ifdef JS_CPU_X64
                    edge.sourceTrampoline =
                        stubCode.locationOf(oedge.sourceTrampoline).executableAddress();
#endif
                    jit->patchEdge(edge, label);
                    break;
                }
            }
        } else if (bytecodeInChunk(outerScript->code + edge.target)) {
            JS_ASSERT(!edge.targetLabel);
            JS_ASSERT(jumpMap[edge.target].isSet());
            edge.targetLabel = fullCode.locationOf(jumpMap[edge.target]).executableAddress();
            jit->patchEdge(edge, edge.targetLabel);
        }
    }

    chunk->recompileInfo = cx->compartment->types.compiledInfo;
    return Compile_Okay;
}

#ifdef DEBUG
#define SPEW_OPCODE()                                                         \
    JS_BEGIN_MACRO                                                            \
        if (IsJaegerSpewChannelActive(JSpew_JSOps)) {                         \
            Sprinter sprinter(cx);                                            \
            sprinter.init();                                                  \
            RootedScript script(cx, script_);                                 \
            js_Disassemble1(cx, script, PC, PC - script_->code,               \
                            JS_TRUE, &sprinter);                              \
            JaegerSpew(JSpew_JSOps, "    %2d %s",                             \
                       frame.stackDepth(), sprinter.string());                \
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

inline bool
mjit::Compiler::shouldStartLoop(jsbytecode *head)
{
    



    if (*head == JSOP_LOOPHEAD && analysis->getLoop(head)) {
        uint32_t backedge = analysis->getLoop(head)->backedge;
        if (!bytecodeInChunk(script_->code + backedge))
            return false;
        return true;
    }
    return false;
}

CompileStatus
mjit::Compiler::generateMethod()
{
    SrcNoteLineScanner scanner(script_->notes(), script_->lineno);

    
    bool fallthrough = true;

    
    jsbytecode *lastPC = NULL;

    if (!outerJIT())
        return Compile_Retry;

    uint32_t chunkBegin = 0, chunkEnd = script_->length;
    if (!a->parent) {
        const ChunkDescriptor &desc =
            outerJIT()->chunkDescriptor(chunkIndex);
        chunkBegin = desc.begin;
        chunkEnd = desc.end;

        while (PC != script_->code + chunkBegin) {
            Bytecode *opinfo = analysis->maybeCode(PC);
            if (opinfo) {
                if (opinfo->jumpTarget) {
                    
                    const SlotValue *newv = analysis->newValues(PC);
                    if (newv) {
                        while (newv->slot) {
                            if (newv->slot < TotalSlots(script_)) {
                                VarType &vt = a->varTypes[newv->slot];
                                vt.setTypes(analysis->getValueTypes(newv->value));
                            }
                            newv++;
                        }
                    }
                }
                if (analyze::BytecodeUpdatesSlot(JSOp(*PC))) {
                    uint32_t slot = GetBytecodeSlot(script_, PC);
                    if (analysis->trackSlot(slot)) {
                        VarType &vt = a->varTypes[slot];
                        vt.setTypes(analysis->pushedTypes(PC, 0));
                    }
                }
            }

            PC += GetBytecodeLength(PC);
        }

        if (chunkIndex != 0) {
            uint32_t depth = analysis->getCode(PC).stackDepth;
            for (uint32_t i = 0; i < depth; i++)
                frame.pushSynced(JSVAL_TYPE_UNKNOWN);
        }
    }

    
    RootedPropertyName name0(cx);

    for (;;) {
        JSOp op = JSOp(*PC);
        int trap = stubs::JSTRAP_NONE;

        if (script_->hasBreakpointsAt(PC))
            trap |= stubs::JSTRAP_TRAP;

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

        if (PC >= script_->code + script_->length)
            break;

        scanner.advanceTo(PC - script_->code);
        if (script_->stepModeEnabled() &&
            (scanner.isLineHeader() || opinfo->jumpTarget))
        {
            trap |= stubs::JSTRAP_SINGLESTEP;
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

        if (PC >= script_->code + chunkEnd) {
            if (fallthrough) {
                if (opinfo->jumpTarget)
                    fixDoubleTypes(PC);
                frame.syncAndForgetEverything();
                jsbytecode *curPC = PC;
                do {
                    PC--;
                } while (!analysis->maybeCode(PC));
                if (!jumpAndRun(masm.jump(), curPC, NULL, NULL,  true))
                    return Compile_Error;
                PC = curPC;
            }
            break;
        }

        if (opinfo->jumpTarget || trap) {
            if (fallthrough) {
                fixDoubleTypes(PC);
                fixedIntToDoubleEntries.clear();
                fixedDoubleToAnyEntries.clear();

                





                if (cx->typeInferenceEnabled() && shouldStartLoop(PC)) {
                    frame.syncAndForgetEverything();
                    Jump j = masm.jump();
                    if (!startLoop(PC, j, PC))
                        return Compile_Error;
                } else {
                    Label start = masm.label();
                    if (!frame.syncForBranch(PC, Uses(0)))
                        return Compile_Error;
                    if (pcLengths && lastPC) {
                        
                        size_t length = masm.size() - masm.distanceOf(start);
                        uint32_t offset = ssa.frameLength(a->inlineIndex) + lastPC - script_->code;
                        pcLengths[offset].codeLengthAugment += length;
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
        } else if (opinfo->safePoint) {
            frame.syncAndForgetEverything();
        }
        frame.assertValidRegisterState();
        a->jumpMap[uint32_t(PC - script_->code)] = masm.label();

        if (cx->typeInferenceEnabled() && opinfo->safePoint) {
            





            const SlotValue *newv = analysis->newValues(PC);
            if (newv) {
                while (newv->slot) {
                    if (newv->value.kind() == SSAValue::PHI &&
                        newv->value.phiOffset() == uint32_t(PC - script_->code) &&
                        analysis->trackSlot(newv->slot) &&
                        a->varTypes[newv->slot].getTypeTag() == JSVAL_TYPE_DOUBLE) {
                        FrameEntry *fe = frame.getSlotEntry(newv->slot);
                        masm.ensureInMemoryDouble(frame.addressOf(fe));
                    }
                    newv++;
                }
            }
        }

        
        
        
        if (loop && !a->parent)
            loop->setOuterPC(PC);

        SPEW_OPCODE();
        JS_ASSERT(frame.stackDepth() == opinfo->stackDepth);

        if (op == JSOP_LOOPHEAD && analysis->getLoop(PC)) {
            jsbytecode *backedge = script_->code + analysis->getLoop(PC)->backedge;
            if (!bytecodeInChunk(backedge)){
                for (uint32_t slot = ArgSlot(0); slot < TotalSlots(script_); slot++) {
                    if (a->varTypes[slot].getTypeTag() == JSVAL_TYPE_DOUBLE) {
                        FrameEntry *fe = frame.getSlotEntry(slot);
                        masm.ensureInMemoryDouble(frame.addressOf(fe));
                    }
                }
            }
        }

        
        
        
        
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

        Label inlineStart = masm.label();
        Label stubStart = stubcc.masm.label();
        bool countsUpdated = false;
        bool arithUpdated = false;

        JSValueType arithFirstUseType = JSVAL_TYPE_UNKNOWN;
        JSValueType arithSecondUseType = JSVAL_TYPE_UNKNOWN;
        if (script_->hasScriptCounts && !!(js_CodeSpec[op].format & JOF_ARITH)) {
            if (GetUseCount(script_, PC - script_->code) == 1) {
                FrameEntry *use = frame.peek(-1);
                



                if (use->isTypeKnown())
                    arithFirstUseType = arithSecondUseType = use->getKnownType();
            } else {
                FrameEntry *use = frame.peek(-1);
                if (use->isTypeKnown())
                    arithFirstUseType = use->getKnownType();
                use = frame.peek(-2);
                if (use->isTypeKnown())
                    arithSecondUseType = use->getKnownType();
            }
        }

        




        if (script_->hasScriptCounts && JOF_OPTYPE(op) == JOF_JUMP)
            updatePCCounts(PC, &countsUpdated);

    



        lastPC = PC;

        switch (op) {
          BEGIN_CASE(JSOP_NOP)
          END_CASE(JSOP_NOP)

          BEGIN_CASE(JSOP_NOTEARG)
          END_CASE(JSOP_NOTEARG)

          BEGIN_CASE(JSOP_UNDEFINED)
            frame.push(UndefinedValue());
          END_CASE(JSOP_UNDEFINED)

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
            if (script_->hasScriptCounts)
                updatePCCounts(PC, &countsUpdated);
            emitReturn(frame.peek(-1));
            fallthrough = false;
          END_CASE(JSOP_RETURN)

          BEGIN_CASE(JSOP_GOTO)
          BEGIN_CASE(JSOP_DEFAULT)
          {
            unsigned targetOffset = FollowBranch(cx, script_, PC - script_->code);
            jsbytecode *target = script_->code + targetOffset;

            fixDoubleTypes(target);

            




            jsbytecode *next = PC + js_CodeSpec[op].length;
            if (cx->typeInferenceEnabled() &&
                analysis->maybeCode(next) &&
                shouldStartLoop(next))
            {
                frame.syncAndForgetEverything();
                Jump j = masm.jump();
                if (!startLoop(next, j, target))
                    return Compile_Error;
            } else {
                if (!frame.syncForBranch(target, Uses(0)))
                    return Compile_Error;
                Jump j = masm.jump();
                if (!jumpAndRun(j, target))
                    return Compile_Error;
            }
            fallthrough = false;
            PC += js_CodeSpec[op].length;
            break;
          }
          END_CASE(JSOP_GOTO)

          BEGIN_CASE(JSOP_IFEQ)
          BEGIN_CASE(JSOP_IFNE)
          {
            jsbytecode *target = PC + GET_JUMP_OFFSET(PC);
            fixDoubleTypes(target);
            if (!jsop_ifneq(op, target))
                return Compile_Error;
            PC += js_CodeSpec[op].length;
            break;
          }
          END_CASE(JSOP_IFNE)

          BEGIN_CASE(JSOP_ARGUMENTS)
            if (script_->needsArgsObj()) {
                prepareStubCall(Uses(0));
                INLINE_STUBCALL(stubs::Arguments, REJOIN_FALLTHROUGH);
                pushSyncedEntry(0);
            } else {
                frame.push(MagicValue(JS_OPTIMIZED_ARGUMENTS));
            }
          END_CASE(JSOP_ARGUMENTS)

          BEGIN_CASE(JSOP_ITERNEXT)
            iterNext();
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
            uint32_t amt = GET_UINT8(PC);

            
            
            
            frame.dupAt(-int32_t(amt + 1));

            
            
            
            
            
            
            for (int32_t i = -int32_t(amt); i < 0; i++) {
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
           if (script_->hasScriptCounts) {
               updateArithCounts(PC, NULL, arithFirstUseType, arithSecondUseType);
               arithUpdated = true;
           }

            
            jsbytecode *next = &PC[JSOP_GE_LENGTH];
            JSOp fused = JSOp(*next);
            if ((fused != JSOP_IFEQ && fused != JSOP_IFNE) || analysis->jumpTarget(next))
                fused = JSOP_NOP;

            
            jsbytecode *target = NULL;
            if (fused != JSOP_NOP) {
                if (script_->hasScriptCounts)
                    updatePCCounts(PC, &countsUpdated);
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
                JS_ALWAYS_TRUE(ToInt32(cx, top->getValue(), &i));
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
                    RootedScript script(cx, script_);
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
            uint32_t index = GET_UINT32_INDEX(PC);
            name0 = script_->getName(index);

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(name0), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DelName, REJOIN_FALLTHROUGH);
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_DELNAME)

          BEGIN_CASE(JSOP_DELPROP)
          {
            uint32_t index = GET_UINT32_INDEX(PC);
            name0 = script_->getName(index);

            prepareStubCall(Uses(1));
            masm.move(ImmPtr(name0), Registers::ArgReg1);
            INLINE_STUBCALL(STRICT_VARIANT(script_, stubs::DelProp), REJOIN_FALLTHROUGH);
            frame.pop();
            pushSyncedEntry(0);
          }
          END_CASE(JSOP_DELPROP)

          BEGIN_CASE(JSOP_DELELEM)
          {
            prepareStubCall(Uses(2));
            INLINE_STUBCALL(STRICT_VARIANT(script_, stubs::DelElem), REJOIN_FALLTHROUGH);
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
          BEGIN_CASE(JSOP_CALLPROP)
          BEGIN_CASE(JSOP_LENGTH)
            name0 = script_->getName(GET_UINT32_INDEX(PC));
            if (!jsop_getprop(name0, knownPushedType(0)))
                return Compile_Error;
          END_CASE(JSOP_GETPROP)

          BEGIN_CASE(JSOP_GETELEM)
          BEGIN_CASE(JSOP_CALLELEM)
            if (script_->hasScriptCounts)
                updateElemCounts(PC, frame.peek(-2), frame.peek(-1));
            if (!jsop_getelem())
                return Compile_Error;
          END_CASE(JSOP_GETELEM)

          BEGIN_CASE(JSOP_TOID)
            jsop_toid();
          END_CASE(JSOP_TOID)

          BEGIN_CASE(JSOP_SETELEM)
          {
            if (script_->hasScriptCounts)
                updateElemCounts(PC, frame.peek(-3), frame.peek(-2));
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
                if (script_->hasScriptCounts) {
                    
                    countsUpdated = true;
                }
            }

            FrameSize frameSize;
            frameSize.initStatic(frame.totalDepth(), GET_ARGC(PC));

            if (!done) {
                JaegerSpew(JSpew_Insns, " --- SCRIPTED CALL --- \n");
                if (!inlineCallHelper(GET_ARGC(PC), callingNew, frameSize))
                    return Compile_Error;
                JaegerSpew(JSpew_Insns, " --- END SCRIPTED CALL --- \n");
            }
          }
          END_CASE(JSOP_CALL)

          BEGIN_CASE(JSOP_NAME)
          BEGIN_CASE(JSOP_CALLNAME)
          {
            name0 = script_->getName(GET_UINT32_INDEX(PC));
            jsop_name(name0, knownPushedType(0));
            frame.extra(frame.peek(-1)).name = name0;
          }
          END_CASE(JSOP_NAME)

          BEGIN_CASE(JSOP_GETINTRINSIC)
          BEGIN_CASE(JSOP_CALLINTRINSIC)
          {
            name0 = script_->getName(GET_UINT32_INDEX(PC));
            if (!jsop_intrinsic(name0, knownPushedType(0)))
                return Compile_Error;
            frame.extra(frame.peek(-1)).name = name0;
          }
          END_CASE(JSOP_GETINTRINSIC)

          BEGIN_CASE(JSOP_IMPLICITTHIS)
          {
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(script_->getName(GET_UINT32_INDEX(PC))), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::ImplicitThis, REJOIN_FALLTHROUGH);
            frame.pushSynced(JSVAL_TYPE_UNKNOWN);
          }
          END_CASE(JSOP_IMPLICITTHIS)

          BEGIN_CASE(JSOP_DOUBLE)
          {
            double d = script_->getConst(GET_UINT32_INDEX(PC)).toDouble();
            frame.push(Value(DoubleValue(d)));
          }
          END_CASE(JSOP_DOUBLE)

          BEGIN_CASE(JSOP_STRING)
            frame.push(StringValue(script_->getAtom(GET_UINT32_INDEX(PC))));
          END_CASE(JSOP_STRING)

          BEGIN_CASE(JSOP_ZERO)
            frame.push(JSVAL_ZERO);
          END_CASE(JSOP_ZERO)

          BEGIN_CASE(JSOP_ONE)
            frame.push(JSVAL_ONE);
          END_CASE(JSOP_ONE)

          BEGIN_CASE(JSOP_NULL)
            frame.push(NullValue());
          END_CASE(JSOP_NULL)

          BEGIN_CASE(JSOP_CALLEE)
            frame.pushCallee();
          END_CASE(JSOP_CALLEE)

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
            






            if (script_->hasScriptCounts)
                updatePCCounts(PC, &countsUpdated);
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

          BEGIN_CASE(JSOP_CASE)
            

            frame.dupAt(-2);
            

            jsop_stricteq(JSOP_STRICTEQ);
            

            if (!jsop_ifneq(JSOP_IFNE, PC + GET_JUMP_OFFSET(PC)))
                return Compile_Error;
          END_CASE(JSOP_CASE)

          BEGIN_CASE(JSOP_STRICTEQ)
          BEGIN_CASE(JSOP_STRICTNE)
            if (script_->hasScriptCounts) {
                updateArithCounts(PC, NULL, arithFirstUseType, arithSecondUseType);
                arithUpdated = true;
            }
            jsop_stricteq(op);
          END_CASE(JSOP_STRICTEQ)

          BEGIN_CASE(JSOP_ITER)
            if (!iter(GET_UINT8(PC)))
                return Compile_Error;
          END_CASE(JSOP_ITER)

          BEGIN_CASE(JSOP_MOREITER)
          {
            
            if (script_->hasScriptCounts)
                updatePCCounts(PC, &countsUpdated);
            jsbytecode *target = &PC[JSOP_MOREITER_LENGTH];
            JSOp next = JSOp(*target);
            JS_ASSERT(next == JSOP_IFNE);

            target += GET_JUMP_OFFSET(target);

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
          BEGIN_CASE(JSOP_CALLARG)
          {
            restoreVarType();
            uint32_t arg = GET_SLOTNO(PC);
            if (JSObject *singleton = pushedSingleton(0))
                frame.push(ObjectValue(*singleton));
            else if (script_->argsObjAliasesFormals())
                jsop_aliasedArg(arg,  true);
            else
                frame.pushArg(arg);
          }
          END_CASE(JSOP_GETARG)

          BEGIN_CASE(JSOP_BINDGNAME)
            jsop_bindgname();
          END_CASE(JSOP_BINDGNAME)

          BEGIN_CASE(JSOP_SETARG)
          {
            jsbytecode *next = &PC[JSOP_SETARG_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);

            uint32_t arg = GET_SLOTNO(PC);
            if (script_->argsObjAliasesFormals())
                jsop_aliasedArg(arg,  false, pop);
            else
                frame.storeArg(arg, pop);

            updateVarType();

            if (pop) {
                frame.pop();
                PC += JSOP_SETARG_LENGTH + JSOP_POP_LENGTH;
                break;
            }
          }
          END_CASE(JSOP_SETARG)

          BEGIN_CASE(JSOP_GETLOCAL)
          BEGIN_CASE(JSOP_CALLLOCAL)
          {
            




            jsbytecode *next = &PC[JSOP_GETLOCAL_LENGTH];
            if (JSOp(*next) != JSOP_POP || analysis->jumpTarget(next))
                restoreVarType();
            if (JSObject *singleton = pushedSingleton(0))
                frame.push(ObjectValue(*singleton));
            else
                frame.pushLocal(GET_SLOTNO(PC));
          }
          END_CASE(JSOP_GETLOCAL)

          BEGIN_CASE(JSOP_GETALIASEDVAR)
          BEGIN_CASE(JSOP_CALLALIASEDVAR)
            jsop_aliasedVar(ScopeCoordinate(PC),  true);
          END_CASE(JSOP_GETALIASEDVAR);

          BEGIN_CASE(JSOP_SETLOCAL)
          BEGIN_CASE(JSOP_SETALIASEDVAR)
          {
            jsbytecode *next = &PC[GetBytecodeLength(PC)];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            if (JOF_OPTYPE(*PC) == JOF_SCOPECOORD) {
                jsop_aliasedVar(ScopeCoordinate(PC),  false, pop);
            } else {
                frame.storeLocal(GET_SLOTNO(PC), pop);
                updateVarType();
            }

            if (pop) {
                frame.pop();
                PC = next + JSOP_POP_LENGTH;
                break;
            }

            PC = next;
            break;
          }
          END_CASE(JSOP_SETLOCAL)

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

          BEGIN_CASE(JSOP_INITPROP)
            jsop_initprop();
            frame.pop();
          END_CASE(JSOP_INITPROP)

          BEGIN_CASE(JSOP_INITELEM_ARRAY)
            jsop_initelem_array();
          END_CASE(JSOP_INITELEM_ARRAY)

          BEGIN_CASE(JSOP_INITELEM)
            prepareStubCall(Uses(3));
            INLINE_STUBCALL(stubs::InitElem, REJOIN_FALLTHROUGH);
            frame.popn(2);
          END_CASE(JSOP_INITELEM)

          BEGIN_CASE(JSOP_BINDNAME)
            name0 = script_->getName(GET_UINT32_INDEX(PC));
            jsop_bindname(name0);
          END_CASE(JSOP_BINDNAME)

          BEGIN_CASE(JSOP_SETPROP)
          {
            jsbytecode *next = &PC[JSOP_SETPROP_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            name0 = script_->getName(GET_UINT32_INDEX(PC));
            if (!jsop_setprop(name0, pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETPROP)

          BEGIN_CASE(JSOP_SETNAME)
          {
            jsbytecode *next = &PC[JSOP_SETNAME_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            name0 = script_->getName(GET_UINT32_INDEX(PC));
            if (!jsop_setprop(name0, pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETNAME)

          BEGIN_CASE(JSOP_THROW)
            prepareStubCall(Uses(1));
            INLINE_STUBCALL(stubs::Throw, REJOIN_NONE);
            frame.pop();
            fallthrough = false;
          END_CASE(JSOP_THROW)

          BEGIN_CASE(JSOP_IN)
          {
            jsop_in();
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

          BEGIN_CASE(JSOP_CONDSWITCH)
            
          END_CASE(JSOP_CONDSWITCH)

          BEGIN_CASE(JSOP_LABEL)
          END_CASE(JSOP_LABEL)

          BEGIN_CASE(JSOP_DEFFUN)
          {
            JSFunction *innerFun = script_->getFunction(GET_UINT32_INDEX(PC));

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(innerFun), Registers::ArgReg1);
            INLINE_STUBCALL(STRICT_VARIANT(script_, stubs::DefFun), REJOIN_FALLTHROUGH);
          }
          END_CASE(JSOP_DEFFUN)

          BEGIN_CASE(JSOP_DEFVAR)
          BEGIN_CASE(JSOP_DEFCONST)
          {
            name0 = script_->getName(GET_UINT32_INDEX(PC));

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(name0), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DefVarOrConst, REJOIN_FALLTHROUGH);
          }
          END_CASE(JSOP_DEFVAR)

          BEGIN_CASE(JSOP_SETCONST)
          {
            name0 = script_->getName(GET_UINT32_INDEX(PC));

            prepareStubCall(Uses(1));
            masm.move(ImmPtr(name0), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::SetConst, REJOIN_FALLTHROUGH);
          }
          END_CASE(JSOP_SETCONST)

          BEGIN_CASE(JSOP_LAMBDA)
          {
            JSFunction *fun = script_->getFunction(GET_UINT32_INDEX(PC));

            JSObjStubFun stub = stubs::Lambda;
            uint32_t uses = 0;

            prepareStubCall(Uses(uses));
            masm.move(ImmPtr(fun), Registers::ArgReg1);

            INLINE_STUBCALL(stub, REJOIN_PUSH_OBJECT);

            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
          }
          END_CASE(JSOP_LAMBDA)

          BEGIN_CASE(JSOP_TRY)
            frame.syncAndForgetEverything();
          END_CASE(JSOP_TRY)

          BEGIN_CASE(JSOP_RETRVAL)
            emitReturn(NULL);
            fallthrough = false;
          END_CASE(JSOP_RETRVAL)

          BEGIN_CASE(JSOP_GETGNAME)
          BEGIN_CASE(JSOP_CALLGNAME)
          {
            uint32_t index = GET_UINT32_INDEX(PC);
            if (!jsop_getgname(index))
                return Compile_Error;
            frame.extra(frame.peek(-1)).name = script_->getName(index);
          }
          END_CASE(JSOP_GETGNAME)

          BEGIN_CASE(JSOP_SETGNAME)
          {
            jsbytecode *next = &PC[JSOP_SETGNAME_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis->jumpTarget(next);
            name0 = script_->getName(GET_UINT32_INDEX(PC));
            if (!jsop_setgname(name0, pop))
                return Compile_Error;
          }
          END_CASE(JSOP_SETGNAME)

          BEGIN_CASE(JSOP_REGEXP)
            if (!jsop_regexp())
                return Compile_Error;
          END_CASE(JSOP_REGEXP)

          BEGIN_CASE(JSOP_OBJECT)
          {
            JSObject *object = script_->getObject(GET_UINT32_INDEX(PC));
            RegisterID reg = frame.allocReg();
            masm.move(ImmPtr(object), reg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, reg);
          }
          END_CASE(JSOP_OBJECT)

          BEGIN_CASE(JSOP_UINT24)
            frame.push(Value(Int32Value((int32_t) GET_UINT24(PC))));
          END_CASE(JSOP_UINT24)

          BEGIN_CASE(JSOP_STOP)
            if (script_->hasScriptCounts)
                updatePCCounts(PC, &countsUpdated);
            emitReturn(NULL);
            goto done;
          END_CASE(JSOP_STOP)

          BEGIN_CASE(JSOP_GETXPROP)
            name0 = script_->getName(GET_UINT32_INDEX(PC));
            if (!jsop_xname(name0))
                return Compile_Error;
          END_CASE(JSOP_GETXPROP)

          BEGIN_CASE(JSOP_ENTERBLOCK)
          BEGIN_CASE(JSOP_ENTERLET0)
          BEGIN_CASE(JSOP_ENTERLET1)
            enterBlock(&script_->getObject(GET_UINT32_INDEX(PC))->asStaticBlock());
          END_CASE(JSOP_ENTERBLOCK);

          BEGIN_CASE(JSOP_LEAVEBLOCK)
            leaveBlock();
          END_CASE(JSOP_LEAVEBLOCK)

          BEGIN_CASE(JSOP_INT8)
            frame.push(Value(Int32Value(GET_INT8(PC))));
          END_CASE(JSOP_INT8)

          BEGIN_CASE(JSOP_INT32)
            frame.push(Value(Int32Value(GET_INT32(PC))));
          END_CASE(JSOP_INT32)

          BEGIN_CASE(JSOP_HOLE)
            frame.push(MagicValue(JS_ELEMENTS_HOLE));
          END_CASE(JSOP_HOLE)

          BEGIN_CASE(JSOP_LOOPHEAD)
            if (analysis->jumpTarget(PC))
                interruptCheckHelper();
          END_CASE(JSOP_LOOPHEAD)

          BEGIN_CASE(JSOP_LOOPENTRY)
            
            
            
            if (loop) {
                if (IsIonEnabled(cx))
                    ionCompileHelper();
                else
                    inliningCompileHelper();
            }
          END_CASE(JSOP_LOOPENTRY)

          BEGIN_CASE(JSOP_DEBUGGER)
          {
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(PC), Registers::ArgReg1);
            INLINE_STUBCALL(stubs::DebuggerStatement, REJOIN_FALLTHROUGH);
          }
          END_CASE(JSOP_DEBUGGER)

          default:
            JS_NOT_REACHED("Opcode not implemented");
        }

    



        if (cx->typeInferenceEnabled() && PC == lastPC + GetBytecodeLength(lastPC)) {
            




            unsigned nuses = GetUseCount(script_, lastPC - script_->code);
            unsigned ndefs = GetDefCount(script_, lastPC - script_->code);
            for (unsigned i = 0; i < ndefs; i++) {
                FrameEntry *fe = frame.getStack(opinfo->stackDepth - nuses + i);
                if (fe) {
                    
                    frame.extra(fe).types = analysis->pushedTypes(lastPC - script_->code, i);
                }
            }
        }

        if (script_->hasScriptCounts) {
            size_t length = masm.size() - masm.distanceOf(inlineStart);
            bool typesUpdated = false;

            
            if ((js_CodeSpec[op].format & JOF_ARITH) && !arithUpdated) {
                FrameEntry *pushed = NULL;
                if (PC == lastPC + GetBytecodeLength(lastPC))
                    pushed = frame.peek(-1);
                updateArithCounts(lastPC, pushed, arithFirstUseType, arithSecondUseType);
                typesUpdated = true;
            }

            
            if (PCCounts::accessOp(op) &&
                op != JSOP_SETPROP && op != JSOP_SETELEM) {
                FrameEntry *fe = (GetDefCount(script_, lastPC - script_->code) == 1)
                    ? frame.peek(-1)
                    : frame.peek(-2);
                updatePCTypes(lastPC, fe);
                typesUpdated = true;
            }

            if (!countsUpdated && (typesUpdated || length != 0))
                updatePCCounts(lastPC, &countsUpdated);
        }
        
        if (pcLengths) {
            size_t length     = masm.size() - masm.distanceOf(inlineStart);
            size_t stubLength = stubcc.size() - stubcc.masm.distanceOf(stubStart);
            uint32_t offset   = ssa.frameLength(a->inlineIndex) + lastPC - script_->code;
            pcLengths[offset].inlineLength += length;
            pcLengths[offset].stubLength   += stubLength;
        }

        frame.assertValidRegisterState();
    }

  done:
    return Compile_Okay;
}

#undef END_CASE
#undef BEGIN_CASE

void
mjit::Compiler::updatePCCounts(jsbytecode *pc, bool *updated)
{
    JS_ASSERT(script_->hasScriptCounts);

    Label start = masm.label();

    





    uint32_t offset = ssa.frameLength(a->inlineIndex) + pc - script_->code;

    





    RegisterID reg = Registers::ReturnReg;
    masm.storePtr(reg, frame.addressOfTop());

    PCCounts counts = script_->getPCCounts(pc);

    





    double *code = &counts.get(PCCounts::BASE_METHODJIT_CODE);
    masm.addCount(&pcLengths[offset].inlineLength, code, reg);
    masm.addCount(&pcLengths[offset].codeLengthAugment, code, reg);

    double *pics = &counts.get(PCCounts::BASE_METHODJIT_PICS);
    double *picsLength = &pcLengths[offset].picsLength;
    masm.addCount(picsLength, pics, reg);

    double *count = &counts.get(PCCounts::BASE_METHODJIT);
    masm.bumpCount(count, reg);

    
    masm.loadPtr(frame.addressOfTop(), reg);

    
    pcLengths[offset].codeLengthAugment -= masm.size() - masm.distanceOf(start);

    *updated = true;
}

static inline bool
HasPayloadType(types::TypeSet *types)
{
    if (types->unknown())
        return false;

    types::TypeFlags flags = types->baseFlags();
    bool objects = !!(flags & types::TYPE_FLAG_ANYOBJECT) || !!types->getObjectCount();

    if (objects && !!(flags & types::TYPE_FLAG_STRING))
        return false;

    flags = flags & ~(types::TYPE_FLAG_ANYOBJECT | types::TYPE_FLAG_STRING);

    return (flags == types::TYPE_FLAG_UNDEFINED)
        || (flags == types::TYPE_FLAG_NULL)
        || (flags == types::TYPE_FLAG_BOOLEAN);
}

void
mjit::Compiler::updatePCTypes(jsbytecode *pc, FrameEntry *fe)
{
    JS_ASSERT(script_->hasScriptCounts);

    



    RegisterID reg = Registers::ReturnReg;
    if (frame.peekTypeInRegister(fe) && reg == frame.tempRegForType(fe)) {
        JS_STATIC_ASSERT(Registers::ReturnReg != Registers::ArgReg1);
        reg = Registers::ArgReg1;
    }
    masm.push(reg);

    PCCounts counts = script_->getPCCounts(pc);

    
    if (fe->isTypeKnown()) {
        masm.bumpCount(&counts.get(PCCounts::ACCESS_MONOMORPHIC), reg);
        PCCounts::AccessCounts count = PCCounts::ACCESS_OBJECT;
        switch (fe->getKnownType()) {
          case JSVAL_TYPE_UNDEFINED:  count = PCCounts::ACCESS_UNDEFINED;  break;
          case JSVAL_TYPE_NULL:       count = PCCounts::ACCESS_NULL;       break;
          case JSVAL_TYPE_BOOLEAN:    count = PCCounts::ACCESS_BOOLEAN;    break;
          case JSVAL_TYPE_INT32:      count = PCCounts::ACCESS_INT32;      break;
          case JSVAL_TYPE_DOUBLE:     count = PCCounts::ACCESS_DOUBLE;     break;
          case JSVAL_TYPE_STRING:     count = PCCounts::ACCESS_STRING;     break;
          case JSVAL_TYPE_OBJECT:     count = PCCounts::ACCESS_OBJECT;     break;
          default:;
        }
        if (count)
            masm.bumpCount(&counts.get(count), reg);
    } else {
        types::TypeSet *types = frame.extra(fe).types;
        if (types && HasPayloadType(types))
            masm.bumpCount(&counts.get(PCCounts::ACCESS_DIMORPHIC), reg);
        else
            masm.bumpCount(&counts.get(PCCounts::ACCESS_POLYMORPHIC), reg);

        frame.loadTypeIntoReg(fe, reg);

        Jump j = masm.testUndefined(Assembler::NotEqual, reg);
        masm.bumpCount(&counts.get(PCCounts::ACCESS_UNDEFINED), reg);
        frame.loadTypeIntoReg(fe, reg);
        j.linkTo(masm.label(), &masm);

        j = masm.testNull(Assembler::NotEqual, reg);
        masm.bumpCount(&counts.get(PCCounts::ACCESS_NULL), reg);
        frame.loadTypeIntoReg(fe, reg);
        j.linkTo(masm.label(), &masm);

        j = masm.testBoolean(Assembler::NotEqual, reg);
        masm.bumpCount(&counts.get(PCCounts::ACCESS_BOOLEAN), reg);
        frame.loadTypeIntoReg(fe, reg);
        j.linkTo(masm.label(), &masm);

        j = masm.testInt32(Assembler::NotEqual, reg);
        masm.bumpCount(&counts.get(PCCounts::ACCESS_INT32), reg);
        frame.loadTypeIntoReg(fe, reg);
        j.linkTo(masm.label(), &masm);

        j = masm.testDouble(Assembler::NotEqual, reg);
        masm.bumpCount(&counts.get(PCCounts::ACCESS_DOUBLE), reg);
        frame.loadTypeIntoReg(fe, reg);
        j.linkTo(masm.label(), &masm);

        j = masm.testString(Assembler::NotEqual, reg);
        masm.bumpCount(&counts.get(PCCounts::ACCESS_STRING), reg);
        frame.loadTypeIntoReg(fe, reg);
        j.linkTo(masm.label(), &masm);

        j = masm.testObject(Assembler::NotEqual, reg);
        masm.bumpCount(&counts.get(PCCounts::ACCESS_OBJECT), reg);
        frame.loadTypeIntoReg(fe, reg);
        j.linkTo(masm.label(), &masm);
    }

    
    if (js_CodeSpec[*pc].format & JOF_TYPESET) {
        double *count = &counts.get(hasTypeBarriers(pc)
                                      ? PCCounts::ACCESS_BARRIER
                                      : PCCounts::ACCESS_NOBARRIER);
        masm.bumpCount(count, reg);
    }

    
    masm.pop(reg);
}

void
mjit::Compiler::updateArithCounts(jsbytecode *pc, FrameEntry *fe,
                                  JSValueType firstUseType, JSValueType secondUseType)
{
    JS_ASSERT(script_->hasScriptCounts);

    RegisterID reg = Registers::ReturnReg;
    masm.push(reg);

    










    PCCounts::ArithCounts count;
    if (firstUseType == JSVAL_TYPE_INT32 && secondUseType == JSVAL_TYPE_INT32 &&
        (!fe || fe->isNotType(JSVAL_TYPE_DOUBLE))) {
        count = PCCounts::ARITH_INT;
    } else if (firstUseType == JSVAL_TYPE_INT32 || firstUseType == JSVAL_TYPE_DOUBLE ||
               secondUseType == JSVAL_TYPE_INT32 || secondUseType == JSVAL_TYPE_DOUBLE) {
        count = PCCounts::ARITH_DOUBLE;
    } else if (firstUseType != JSVAL_TYPE_UNKNOWN && secondUseType != JSVAL_TYPE_UNKNOWN &&
               (!fe || fe->isTypeKnown())) {
        count = PCCounts::ARITH_OTHER;
    } else {
        count = PCCounts::ARITH_UNKNOWN;
    }

    masm.bumpCount(&script_->getPCCounts(pc).get(count), reg);
    masm.pop(reg);
}

void
mjit::Compiler::updateElemCounts(jsbytecode *pc, FrameEntry *obj, FrameEntry *id)
{
    JS_ASSERT(script_->hasScriptCounts);

    RegisterID reg = Registers::ReturnReg;
    masm.push(reg);

    PCCounts counts = script_->getPCCounts(pc);

    PCCounts::ElementCounts count;
    if (id->isTypeKnown()) {
        switch (id->getKnownType()) {
          case JSVAL_TYPE_INT32:   count = PCCounts::ELEM_ID_INT;     break;
          case JSVAL_TYPE_DOUBLE:  count = PCCounts::ELEM_ID_DOUBLE;  break;
          default:                 count = PCCounts::ELEM_ID_OTHER;   break;
        }
    } else {
        count = PCCounts::ELEM_ID_UNKNOWN;
    }
    masm.bumpCount(&counts.get(count), reg);

    if (obj->mightBeType(JSVAL_TYPE_OBJECT)) {
        types::StackTypeSet *types = frame.extra(obj).types;
        if (types && types->getTypedArrayType() != TypedArray::TYPE_MAX) {
            count = PCCounts::ELEM_OBJECT_TYPED;
        } else if (types && types->getKnownClass() == &ArrayClass &&
                   !types->hasObjectFlags(cx, types::OBJECT_FLAG_SPARSE_INDEXES |
                                          types::OBJECT_FLAG_LENGTH_OVERFLOW)) {
            if (!types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED))
                count = PCCounts::ELEM_OBJECT_PACKED;
            else
                count = PCCounts::ELEM_OBJECT_DENSE;
        } else {
            count = PCCounts::ELEM_OBJECT_OTHER;
        }
        masm.bumpCount(&counts.get(count), reg);
    } else {
        masm.bumpCount(&counts.get(PCCounts::ELEM_OBJECT_OTHER), reg);
    }

    masm.pop(reg);
}

void
mjit::Compiler::bumpPropCount(jsbytecode *pc, int count)
{
    
    if (!(js_CodeSpec[*pc].format & JOF_PROP))
        return;
    RegisterID reg = Registers::ReturnReg;
    masm.push(reg);
    masm.bumpCount(&script_->getPCCounts(pc).get(count), reg);
    masm.pop(reg);
}

JSC::MacroAssembler::Label
mjit::Compiler::labelOf(jsbytecode *pc, uint32_t inlineIndex)
{
    ActiveFrame *a = (inlineIndex == UINT32_MAX) ? outer : inlineFrames[inlineIndex];
    JS_ASSERT(uint32_t(pc - a->script->code) < a->script->length);

    uint32_t offs = uint32_t(pc - a->script->code);
    JS_ASSERT(a->jumpMap[offs].isSet());
    return a->jumpMap[offs];
}

bool
mjit::Compiler::knownJump(jsbytecode *pc)
{
    return pc < PC;
}

bool
mjit::Compiler::jumpInScript(Jump j, jsbytecode *pc)
{
    JS_ASSERT(pc >= script_->code && uint32_t(pc - script_->code) < script_->length);

    if (pc < PC) {
        j.linkTo(a->jumpMap[uint32_t(pc - script_->code)], &masm);
        return true;
    }
    return branchPatches.append(BranchPatch(j, pc, a->inlineIndex));
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
    Address thisv(JSFrameReg, StackFrame::offsetOfThis(script_->function()));

    
    
    
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
    JS_ASSERT_IF(!script_->function(), JSOp(*PC) == JSOP_STOP);

    
    JS_ASSERT_IF(fe, fe == frame.peek(-1));

    if (debugMode()) {
        
        if (fe) {
            frame.storeTo(fe, Address(JSFrameReg, StackFrame::offsetOfReturnValue()), true);

            
            RegisterID reg = frame.allocReg();
            masm.load32(FrameFlagsAddress(), reg);
            masm.or32(Imm32(StackFrame::HAS_RVAL), reg);
            masm.store32(reg, FrameFlagsAddress());
            frame.freeReg(reg);

            
            fe = NULL;
        }

        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::ScriptDebugEpilogue, REJOIN_RESUME);
    }

    if (a != outer) {
        JS_ASSERT(!debugMode());
        profilingPopHelper();

        





        if (a->needReturnValue)
            emitInlineReturnValue(fe);

        if (a->exitState) {
            




            frame.syncForAllocation(a->exitState, true, Uses(0));
        }

        



        bool endOfScript =
            (JSOp(*PC) == JSOP_STOP) ||
            (JSOp(*PC) == JSOP_RETURN &&
             (JSOp(PC[JSOP_RETURN_LENGTH]) == JSOP_STOP &&
              !analysis->maybeCode(PC + JSOP_RETURN_LENGTH)));
        if (!endOfScript)
            a->returnJumps->append(masm.jump());

        if (a->returnSet)
            frame.freeReg(a->returnRegister);
        return;
    }

    
    if (debugMode()) {
        sps.skipNextReenter();
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::Epilogue, REJOIN_NONE);
    } else {
        profilingPopHelper();
    }

    emitReturnValue(&masm, fe);
    emitFinalReturn(masm);

    





    frame.discardFrame();
}

void
mjit::Compiler::prepareStubCall(Uses uses)
{
    JaegerSpew(JSpew_Insns, " ---- STUB CALL, SYNCING FRAME ---- \n");
    frame.syncAndKill(Registers(Registers::AvailAnyRegs), uses);
    JaegerSpew(JSpew_Insns, " ---- FRAME SYNCING DONE ---- \n");
}

JSC::MacroAssembler::Call
mjit::Compiler::emitStubCall(void *ptr, DataLabelPtr *pinline)
{
    JaegerSpew(JSpew_Insns, " ---- CALLING STUB ---- \n");

    masm.bumpStubCount(script_, PC, Registers::tempCallReg());

    Call cl = masm.fallibleVMCall(cx->typeInferenceEnabled(),
                                  ptr, outerPC(), pinline, frame.totalDepth());
    JaegerSpew(JSpew_Insns, " ---- END STUB CALL ---- \n");
    return cl;
}

void
mjit::Compiler::interruptCheckHelper()
{
    Jump jump;
    if (cx->runtime->gcZeal() == js::gc::ZealVerifierPreValue ||
        cx->runtime->gcZeal() == js::gc::ZealVerifierPostValue)
    {
        
        jump = masm.jump();
    } else {
        void *interrupt = (void*) &cx->runtime->interrupt;
#if defined(JS_CPU_X86) || defined(JS_CPU_ARM) || defined(JS_CPU_MIPS)
        jump = masm.branch32(Assembler::NotEqual, AbsoluteAddress(interrupt), Imm32(0));
#else
        
        RegisterID reg = frame.allocReg();
        masm.move(ImmPtr(interrupt), reg);
        jump = masm.branchTest32(Assembler::NonZero, Address(reg, 0));
        frame.freeReg(reg);
#endif
    }

    stubcc.linkExitDirect(jump, stubcc.masm.label());

    frame.sync(stubcc.masm, Uses(0));
    stubcc.masm.move(ImmPtr(PC), Registers::ArgReg1);
    OOL_STUBCALL(stubs::Interrupt, REJOIN_RESUME);
    stubcc.rejoin(Changes(0));
}

static inline bool
MaybeIonCompileable(JSContext *cx, JSScript *script, bool *recompileCheckForIon)
{
#ifdef JS_ION
    *recompileCheckForIon = true;

    if (!ion::IsEnabled(cx))
        return false;
    if (!script->canIonCompile())
        return false;

    
    
    
    if (script->isShortRunning())
        *recompileCheckForIon = false;

    return true;
#endif
    return false;
}

void
mjit::Compiler::ionCompileHelper()
{
    JS_ASSERT(script_ == outerScript);

    JS_ASSERT(IsIonEnabled(cx));
    JS_ASSERT(!inlining());

#ifdef JS_ION
    if (debugMode() || !globalObj || !cx->typeInferenceEnabled() || outerScript->hasIonScript())
        return;

    bool recompileCheckForIon = false;
    if (!MaybeIonCompileable(cx, outerScript, &recompileCheckForIon))
        return;

    uint32_t minUses = ion::UsesBeforeIonRecompile(outerScript, PC);

    uint32_t *useCountAddress = script_->addressOfUseCount();
    masm.add32(Imm32(1), AbsoluteAddress(useCountAddress));

    
    
    
    if (isConstructing && outerScript->code == PC)
        return;

    
    
    
    if (!recompileCheckForIon)
        return;

    void *ionScriptAddress = &script_->ion;

#ifdef JS_CPU_X64
    
    
    RegisterID reg = frame.allocReg();
#endif

    InternalCompileTrigger trigger;
    trigger.pc = PC;
    trigger.stubLabel = stubcc.syncExitAndJump(Uses(0));

    
    
    
    
    
    
    
    
    
    
    
    

    Label secondTest = stubcc.masm.label();

#if defined(JS_CPU_X86) || defined(JS_CPU_ARM)
    trigger.inlineJump = masm.branch32(Assembler::GreaterThanOrEqual,
                                       AbsoluteAddress(useCountAddress),
                                       Imm32(minUses));
    Jump scriptJump = stubcc.masm.branch32(Assembler::Equal, AbsoluteAddress(ionScriptAddress),
                                           Imm32(0));
#elif defined(JS_CPU_X64)
    
    masm.move(ImmPtr(useCountAddress), reg);
    trigger.inlineJump = masm.branch32(Assembler::GreaterThanOrEqual,
                                       Address(reg),
                                       Imm32(minUses));
    stubcc.masm.move(ImmPtr(ionScriptAddress), reg);
    Jump scriptJump = stubcc.masm.branchPtr(Assembler::Equal, Address(reg), ImmPtr(NULL));
    frame.freeReg(reg);
#else
#error "Unknown platform"
#endif

    stubcc.linkExitDirect(trigger.inlineJump,
                          ion::js_IonOptions.parallelCompilation
                          ? secondTest
                          : trigger.stubLabel);

    scriptJump.linkTo(trigger.stubLabel, &stubcc.masm);
    stubcc.crossJump(stubcc.masm.jump(), masm.label());

    stubcc.leave();
    OOL_STUBCALL(stubs::TriggerIonCompile, REJOIN_RESUME);
    stubcc.rejoin(Changes(0));

    compileTriggers.append(trigger);
#endif 
}

void
mjit::Compiler::inliningCompileHelper()
{
    JS_ASSERT(!IsIonEnabled(cx));

    if (inlining() || debugMode() || !globalObj ||
        !analysis->hasFunctionCalls() || !cx->typeInferenceEnabled()) {
        return;
    }

    uint32_t *addr = script_->addressOfUseCount();
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

CompileStatus
mjit::Compiler::methodEntryHelper()
{
    if (debugMode()) {
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::ScriptDebugPrologue, REJOIN_RESUME);

    




    } else if (Probes::callTrackingActive(cx) ||
               (sps.slowAssertions() && a->inlineIndex == UINT32_MAX)) {
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::ScriptProbeOnlyPrologue, REJOIN_RESUME);
    } else {
        return profilingPushHelper();
    }
    
    if (sps.enabled()) {
        RegisterID reg = frame.allocReg();
        sps.pushManual(script_, masm, reg);
        frame.freeReg(reg);
    }
    return Compile_Okay;
}

CompileStatus
mjit::Compiler::profilingPushHelper()
{
    if (!sps.enabled())
        return Compile_Okay;
    RegisterID reg = frame.allocReg();
    if (!sps.push(cx, script_, masm, reg))
        return Compile_Error;

    
    masm.load32(FrameFlagsAddress(), reg);
    masm.or32(Imm32(StackFrame::HAS_PUSHED_SPS_FRAME), reg);
    masm.store32(reg, FrameFlagsAddress());
    frame.freeReg(reg);

    return Compile_Okay;
}

void
mjit::Compiler::profilingPopHelper()
{
    if (Probes::callTrackingActive(cx) || sps.slowAssertions()) {
        sps.skipNextReenter();
        prepareStubCall(Uses(0));
        INLINE_STUBCALL(stubs::ScriptProbeOnlyEpilogue, REJOIN_RESUME);
    } else if (cx->runtime->spsProfiler.enabled()) {
        RegisterID reg = frame.allocReg();
        sps.pop(masm, reg);
        frame.freeReg(reg);
    }
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
mjit::Compiler::emitUncachedCall(uint32_t argc, bool callingNew)
{
    CallPatchInfo callPatch;

    RegisterID r0 = Registers::ReturnReg;
    VoidPtrStubUInt32 stub = callingNew ? stubs::UncachedNew : stubs::UncachedCall;

    frame.syncAndKill(Uses(argc + 2));
    prepareStubCall(Uses(argc + 2));
    masm.move(Imm32(argc), Registers::ArgReg1);
    INLINE_STUBCALL(stub, REJOIN_CALL_PROLOGUE);

    Jump notCompiled = masm.branchTestPtr(Assembler::Zero, r0, r0);

    masm.loadPtr(FrameAddress(VMFrame::offsetOfRegsSp()), JSFrameReg);
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
    if (sps.enabled()) {
        RegisterID reg = frame.allocReg();
        sps.reenter(masm, reg);
        frame.freeReg(reg);
    }
}

void
mjit::Compiler::checkCallApplySpeculation(uint32_t argc, FrameEntry *origCallee, FrameEntry *origThis,
                                          MaybeRegisterID origCalleeType, RegisterID origCalleeData,
                                          MaybeRegisterID origThisType, RegisterID origThisData,
                                          Jump *uncachedCallSlowRejoin, CallPatchInfo *uncachedCallPatch)
{
    JS_ASSERT(IsLowerableFunCallOrApply(PC));

    RegisterID temp;
    Registers tempRegs(Registers::AvailRegs);
    if (origCalleeType.isSet())
        tempRegs.takeReg(origCalleeType.reg());
    tempRegs.takeReg(origCalleeData);
    if (origThisType.isSet())
        tempRegs.takeReg(origThisType.reg());
    tempRegs.takeReg(origThisData);
    temp = tempRegs.takeAnyReg().reg();

    




    MaybeJump isObj;
    if (origCalleeType.isSet())
        isObj = masm.testObject(Assembler::NotEqual, origCalleeType.reg());
    Jump isFun = masm.testFunction(Assembler::NotEqual, origCalleeData, temp);
    Native native = *PC == JSOP_FUNCALL ? js_fun_call : js_fun_apply;
    Jump isNative = masm.branchPtr(Assembler::NotEqual,
                                   Address(origCalleeData, JSFunction::offsetOfNativeOrScript()),
                                   ImmPtr(JS_FUNC_TO_DATA_PTR(void *, native)));

    



    {
        if (isObj.isSet())
            stubcc.linkExitDirect(isObj.getJump(), stubcc.masm.label());
        stubcc.linkExitDirect(isFun, stubcc.masm.label());
        stubcc.linkExitDirect(isNative, stubcc.masm.label());

        stubcc.masm.move(Imm32(argc), Registers::ArgReg1);
        JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW CALL CODE ---- \n");
        OOL_STUBCALL(stubs::SlowCall, REJOIN_FALLTHROUGH);
        JaegerSpew(JSpew_Insns, " ---- END SLOW CALL CODE ---- \n");

        




        *uncachedCallSlowRejoin = stubcc.masm.jump();
    }
}


bool
mjit::Compiler::inlineCallHelper(uint32_t argc, bool callingNew, FrameSize &callFrameSize)
{
    





    interruptCheckHelper();
    if (sps.enabled()) {
        RegisterID reg = frame.allocReg();
        sps.leave(PC, masm, reg);
        frame.freeReg(reg);
    }

    FrameEntry *origCallee = frame.peek(-(int(argc) + 2));
    FrameEntry *origThis = frame.peek(-(int(argc) + 1));

    




    if (callingNew) {
        frame.discardFe(origThis);

        








        masm.storeValue(NullValue(), frame.addressOf(origThis));
    }

    











    bool lowerFunCallOrApply = IsLowerableFunCallOrApply(PC);

    RootedScript script(cx, script_);
    bool newType = callingNew && cx->typeInferenceEnabled() && types::UseNewType(cx, script, PC);

#ifdef JS_MONOIC
    if (debugMode() || newType) {
#endif
        emitUncachedCall(argc, callingNew);
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

                
                frame.syncAndKill(Uses(argc + 2));
            }

            checkCallApplySpeculation(argc, origCallee, origThis,
                                      origCalleeType, origCalleeData,
                                      origThisType, origThisData,
                                      &uncachedCallSlowRejoin, &uncachedCallPatch);

            icCalleeType = origThisType;
            icCalleeData = origThisData;
            icRvalAddr = frame.addressOf(origThis);

            





            if (*PC == JSOP_FUNCALL)
                callIC.frameSize.initStatic(frame.totalDepth(), argc - 1);
            else
                callIC.frameSize.initDynamic();
        } else {
            
            frame.syncAndKill(Uses(argc + 2));

            icCalleeType = origCalleeType;
            icCalleeData = origCalleeData;
            icRvalAddr = frame.addressOf(origCallee);
            callIC.frameSize.initStatic(frame.totalDepth(), argc);
        }
    }

    callFrameSize = callIC.frameSize;

    callIC.typeMonitored = monitored(PC) || hasTypeBarriers(PC);

    if (script_ == outerScript) {
        if (monitored(PC))
            monitoredBytecodes.append(PC - script_->code);
        if (hasTypeBarriers(PC))
            typeBarrierBytecodes.append(PC - script_->code);
    }

    
    MaybeJump notObjectJump;
    if (icCalleeType.isSet())
        notObjectJump = masm.testObject(Assembler::NotEqual, icCalleeType.reg());

    Registers tempRegs(Registers::AvailRegs);
    tempRegs.takeReg(icCalleeData);

    
    RESERVE_IC_SPACE(masm);

    




    callIC.funGuardLabel = masm.label();
    Jump j = masm.branchPtrWithPatch(Assembler::NotEqual, icCalleeData, callIC.funGuard);
    callIC.funJump = j;

    
    RESERVE_OOL_SPACE(stubcc.masm);

    Jump rejoin1, rejoin2;
    {
        RESERVE_OOL_SPACE(stubcc.masm);
        stubcc.linkExitDirect(j, stubcc.masm.label());
        callIC.slowPathStart = stubcc.masm.label();

        RegisterID tmp = tempRegs.takeAnyReg().reg();

        



        Jump notFunction = stubcc.masm.testFunction(Assembler::NotEqual, icCalleeData, tmp);

        
        stubcc.masm.load16(Address(icCalleeData, offsetof(JSFunction, flags)), tmp);
        Jump isNative = stubcc.masm.branchTest32(Assembler::Zero, tmp,
                                                 Imm32(JSFunction::INTERPRETED));
        tempRegs.putReg(tmp);

        




        if (callIC.frameSize.isDynamic()) {
            OOL_STUBCALL(ic::SplatApplyArgs, REJOIN_CALL_SPLAT);

            



            stubcc.masm.loadPayload(frame.addressOf(origThis), icCalleeData);
        }

        



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

        



        rejoin1 = stubcc.masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                            Registers::ReturnReg);
        if (callIC.frameSize.isStatic())
            stubcc.masm.move(Imm32(callIC.frameSize.staticArgc()), JSParamReg_Argc);
        else
            stubcc.masm.load32(FrameAddress(VMFrame::offsetOfDynamicArgc()), JSParamReg_Argc);
        stubcc.masm.loadPtr(FrameAddress(VMFrame::offsetOfRegsSp()), JSFrameReg);
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

    uint32_t flags = 0;
    if (callingNew)
        flags |= StackFrame::CONSTRUCTING;

    InlineFrameAssembler inlFrame(masm, callIC, flags);
    callPatch.hasFastNcode = true;
    callPatch.fastNcodePatch = inlFrame.assemble(NULL, PC);

    callIC.hotJump = masm.jump();
    callIC.joinPoint = callPatch.joinPoint = masm.label();
    callIC.callIndex = callSites.length();
    addReturnSite();
    callIC.ionJoinPoint = masm.label();
    if (lowerFunCallOrApply)
        uncachedCallPatch.joinPoint = callIC.joinPoint;

    



    CHECK_IC_SPACE();

    JSValueType type = knownPushedType(0);

    frame.popn(argc + 2);
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

    if (lowerFunCallOrApply) {
        uncachedCallSlowRejoin.linkTo(stubcc.masm.label(), &stubcc.masm);
        JaegerSpew(JSpew_Insns, " ---- BEGIN SLOW RESTORE CODE ---- \n");
        Address uncachedRvalAddr = frame.addressOf(origCallee);
        if (knownPushedType(0) == JSVAL_TYPE_DOUBLE)
            stubcc.masm.ensureInMemoryDouble(uncachedRvalAddr);
        frame.reloadEntry(stubcc.masm, uncachedRvalAddr, frame.peek(-1));
        stubcc.crossJump(stubcc.masm.jump(), masm.label());
        JaegerSpew(JSpew_Insns, " ---- END SLOW RESTORE CODE ---- \n");
    }

    callICs.append(callIC);
    callPatches.append(callPatch);
    if (lowerFunCallOrApply)
        callPatches.append(uncachedCallPatch);

    finishBarrier(barrier, REJOIN_FALLTHROUGH, 0);
    if (sps.enabled()) {
        RegisterID reg = frame.allocReg();
        sps.reenter(masm, reg);
        frame.freeReg(reg);
    }
    return true;
#endif
}


static const uint32_t INLINE_SITE_LIMIT = 5;

CompileStatus
mjit::Compiler::inlineScriptedFunction(uint32_t argc, bool callingNew)
{
    JS_ASSERT(inlining());

    
    bool calleeMultipleReturns = false;
    Vector<JSScript *> inlineCallees(CompilerAllocPolicy(cx, *this));
    for (unsigned i = 0; i < ssa.numFrames(); i++) {
        if (ssa.iterFrame(i).parent == a->inlineIndex && ssa.iterFrame(i).parentpc == PC) {
            JSScript *script_ = ssa.iterFrame(i).script;

            
            if (script_->shouldCloneAtCallsite)
                return Compile_InlineAbort;

            inlineCallees.append(script_);
            if (script_->analysis()->numReturnSites() > 1)
                calleeMultipleReturns = true;
        }
    }

    if (inlineCallees.empty())
        return Compile_InlineAbort;

    if (sps.enabled()) {
        RegisterID reg = frame.allocReg();
        sps.leave(PC, masm, reg);
        frame.freeReg(reg);
    }

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
                   script->filename(), script->lineno);

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

        markUndefinedLocals();

        status = methodEntryHelper();
        if (status == Compile_Okay)
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
        js_free(entrySnapshot);

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
               script_->filename(), script_->lineno);

    if (sps.enabled()) {
        RegisterID reg = frame.allocReg();
        sps.reenter(masm, reg);
        frame.freeReg(reg);
    }

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
        int32_t cmp;
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
        if (!jumpAndRun(j, target))
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
    return jumpAndRun(j, target);
}

void
mjit::Compiler::jsop_setprop_slow(HandlePropertyName name)
{
    JS_ASSERT(*PC == JSOP_SETPROP || *PC == JSOP_SETNAME);

    prepareStubCall(Uses(2));
    masm.move(ImmPtr(name), Registers::ArgReg1);

    if (*PC == JSOP_SETPROP)
        INLINE_STUBCALL(stubs::SetProp, REJOIN_FALLTHROUGH);
    else
        INLINE_STUBCALL(stubs::SetName, REJOIN_FALLTHROUGH);

    JS_STATIC_ASSERT(JSOP_SETNAME_LENGTH == JSOP_SETPROP_LENGTH);
    frame.shimmy(1);
    if (script_->hasScriptCounts)
        bumpPropCount(PC, PCCounts::PROP_OTHER);
}

void
mjit::Compiler::jsop_getprop_slow(HandlePropertyName name, bool forPrototype)
{
    
    RejoinState rejoin = forPrototype ? REJOIN_THIS_PROTOTYPE : REJOIN_GETTER;

    prepareStubCall(Uses(1));
    masm.move(ImmPtr(name), Registers::ArgReg1);
    INLINE_STUBCALL(forPrototype ? stubs::GetPropNoCache : stubs::GetProp, rejoin);

    if (!forPrototype)
        testPushedType(rejoin, -1,  false);

    frame.pop();
    frame.pushSynced(JSVAL_TYPE_UNKNOWN);

    if (script_->hasScriptCounts)
        bumpPropCount(PC, PCCounts::PROP_OTHER);
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
mjit::Compiler::jsop_getprop(HandlePropertyName name, JSValueType knownType,
                             bool doTypeCheck, bool forPrototype)
{
    FrameEntry *top = frame.peek(-1);

    




    RejoinState rejoin = REJOIN_GETTER;
    if (forPrototype) {
        JS_ASSERT(top->isType(JSVAL_TYPE_OBJECT) && name == cx->names().classPrototype);
        rejoin = REJOIN_THIS_PROTOTYPE;
    }

    
    if (name == cx->names().length &&
        top->isType(JSVAL_TYPE_STRING) &&
        (!cx->typeInferenceEnabled() || knownPushedType(0) == JSVAL_TYPE_INT32)) {
        if (top->isConstant()) {
            JSString *str = top->getValue().toString();
            Value v;
            v.setNumber(uint32_t(str->length()));
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

    
    if (name == cx->names().length &&
        cx->typeInferenceEnabled() &&
        analysis->poppedTypes(PC, 0)->isMagicArguments() &&
        knownPushedType(0) == JSVAL_TYPE_INT32)
    {
        frame.pop();
        RegisterID reg = frame.allocReg();
        masm.load32(Address(JSFrameReg, StackFrame::offsetOfNumActual()), reg);
        frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);
        if (script_->hasScriptCounts)
            bumpPropCount(PC, PCCounts::PROP_DEFINITE);
        return true;
    }

    if (top->mightBeType(JSVAL_TYPE_OBJECT) &&
        JSOp(*PC) == JSOP_LENGTH && cx->typeInferenceEnabled() &&
        !hasTypeBarriers(PC) && knownPushedType(0) == JSVAL_TYPE_INT32) {
        
        if (loop && loop->generatingInvariants()) {
            CrossSSAValue topv(a->inlineIndex, analysis->poppedValue(PC, 0));
            FrameEntry *fe = loop->invariantLength(topv);
            if (fe) {
                frame.learnType(fe, JSVAL_TYPE_INT32, false);
                frame.pop();
                frame.pushCopyOf(fe);
                if (script_->hasScriptCounts)
                    bumpPropCount(PC, PCCounts::PROP_STATIC);
                return true;
            }
        }

        types::StackTypeSet *types = analysis->poppedTypes(PC, 0);

        



        if (types->getKnownClass() == &ArrayClass &&
            !types->hasObjectFlags(cx, types::OBJECT_FLAG_LENGTH_OVERFLOW))
        {
            frame.forgetMismatchedObject(top);
            bool isObject = top->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, top);
                stubcc.linkExit(notObject, Uses(1));
                stubcc.leave();
                stubcc.masm.move(ImmPtr(name), Registers::ArgReg1);
                OOL_STUBCALL(stubs::GetProp, rejoin);
                if (rejoin == REJOIN_GETTER)
                    testPushedType(rejoin, -1);
            }
            RegisterID result = frame.allocReg();
            RegisterID reg = frame.tempRegForData(top);
            frame.pop();
            masm.loadPtr(Address(reg, JSObject::offsetOfElements()), result);
            masm.load32(Address(result, ObjectElements::offsetOfLength()), result);
            frame.pushTypedPayload(JSVAL_TYPE_INT32, result);
            if (script_->hasScriptCounts)
                bumpPropCount(PC, PCCounts::PROP_DEFINITE);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            return true;
        }

        



        if (types->getTypedArrayType() != TypedArray::TYPE_MAX) {
            if (top->isConstant()) {
                JSObject *obj = &top->getValue().toObject();
                uint32_t length = TypedArray::length(obj);
                frame.pop();
                frame.push(Int32Value(length));
                return true;
            }
            bool isObject = top->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, top);
                stubcc.linkExit(notObject, Uses(1));
                stubcc.leave();
                stubcc.masm.move(ImmPtr(name), Registers::ArgReg1);
                OOL_STUBCALL(stubs::GetProp, rejoin);
                if (rejoin == REJOIN_GETTER)
                    testPushedType(rejoin, -1);
            }
            RegisterID reg = frame.copyDataIntoReg(top);
            frame.pop();
            masm.loadPayload(Address(reg, TypedArray::lengthOffset()), reg);
            frame.pushTypedPayload(JSVAL_TYPE_INT32, reg);
            if (script_->hasScriptCounts)
                bumpPropCount(PC, PCCounts::PROP_DEFINITE);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            return true;
        }
    }

    
    bool testObject;
    JSObject *singleton =
        (*PC == JSOP_GETPROP || *PC == JSOP_CALLPROP) ? pushedSingleton(0) : NULL;
    if (singleton && singleton->isFunction() && !hasTypeBarriers(PC)) {
        Rooted<jsid> id(cx, NameToId(name));
        if (testSingletonPropertyTypes(top, id, &testObject)) {
            if (testObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, top);
                stubcc.linkExit(notObject, Uses(1));
                stubcc.leave();
                stubcc.masm.move(ImmPtr(name), Registers::ArgReg1);
                OOL_STUBCALL(stubs::GetProp, REJOIN_FALLTHROUGH);
                testPushedType(REJOIN_FALLTHROUGH, -1);
            }

            frame.pop();
            frame.push(ObjectValue(*singleton));

            if (script_->hasScriptCounts && cx->typeInferenceEnabled())
                bumpPropCount(PC, PCCounts::PROP_STATIC);

            if (testObject)
                stubcc.rejoin(Changes(1));

            return true;
        }
    }

    
    if (loop && loop->generatingInvariants() && !hasTypeBarriers(PC)) {
        CrossSSAValue topv(a->inlineIndex, analysis->poppedValue(PC, 0));
        RootedId id(cx, NameToId(name));
        if (FrameEntry *fe = loop->invariantProperty(topv, id)) {
            if (knownType != JSVAL_TYPE_UNKNOWN && knownType != JSVAL_TYPE_DOUBLE)
                frame.learnType(fe, knownType, false);
            frame.pop();
            frame.pushCopyOf(fe);
            if (script_->hasScriptCounts)
                bumpPropCount(PC, PCCounts::PROP_STATIC);
            return true;
        }
    }

    
    if (top->isNotType(JSVAL_TYPE_OBJECT)) {
        jsop_getprop_slow(name, forPrototype);
        return true;
    }

    frame.forgetMismatchedObject(top);

    




    RootedId id(cx, NameToId(name));
    types::TypeSet *types = frame.extra(top).types;
    if (types && !types->unknownObject() &&
        types->getObjectCount() == 1 &&
        types->getTypeObject(0) != NULL &&
        !types->getTypeObject(0)->unknownProperties() &&
        id == types::IdToTypeId(id))
    {
        JS_ASSERT(!forPrototype);
        types::TypeObject *object = types->getTypeObject(0);
        types::HeapTypeSet *propertyTypes = object->getProperty(cx, id, false);
        if (!propertyTypes)
            return false;
        if (propertyTypes->definiteProperty() &&
            !propertyTypes->isOwnProperty(cx, object, true)) {
            uint32_t slot = propertyTypes->definiteSlot();
            bool isObject = top->isTypeKnown();
            if (!isObject) {
                Jump notObject = frame.testObject(Assembler::NotEqual, top);
                stubcc.linkExit(notObject, Uses(1));
                stubcc.leave();
                stubcc.masm.move(ImmPtr(name), Registers::ArgReg1);
                OOL_STUBCALL(stubs::GetProp, rejoin);
                if (rejoin == REJOIN_GETTER)
                    testPushedType(rejoin, -1);
            }
            RegisterID reg = frame.tempRegForData(top);
            frame.pop();

            if (script_->hasScriptCounts)
                bumpPropCount(PC, PCCounts::PROP_DEFINITE);

            Address address(reg, JSObject::getFixedSlotOffset(slot));
            BarrierState barrier = pushAddressMaybeBarrier(address, knownType, false);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            finishBarrier(barrier, rejoin, 0);

            return true;
        }
    }

    
    if (cx->typeInferenceEnabled()) {
        if (*PC == JSOP_CALLPROP && jsop_getprop_dispatch(name))
            return true;
    }

    if (script_->hasScriptCounts)
        bumpPropCount(PC, PCCounts::PROP_OTHER);

    




    RegisterID objReg = frame.copyDataIntoReg(top);
    RegisterID shapeReg = frame.allocReg();

    RESERVE_IC_SPACE(masm);

    PICGenInfo pic(ic::PICInfo::GET, PC);

    







    pic.canCallHook = pic.forcedTypeBarrier =
        !forPrototype &&
        (JSOp(*PC) == JSOP_GETPROP || JSOp(*PC) == JSOP_LENGTH) &&
        analysis->getCode(PC).accessGetter;

    
    Label typeCheck;
    if (doTypeCheck && !top->isTypeKnown()) {
        RegisterID reg = frame.tempRegForType(top);
        pic.typeReg = reg;

        if (pic.canCallHook) {
            PinRegAcrossSyncAndKill p1(frame, reg);
            frame.syncAndKillEverything();
        }

        
        pic.fastPathStart = masm.label();
        Jump j = masm.testObject(Assembler::NotEqual, reg);
        typeCheck = masm.label();
        RETURN_IF_OOM(false);

        pic.typeCheck = stubcc.linkExit(j, Uses(1));
        pic.hasTypeCheck = true;
    } else {
        if (pic.canCallHook)
            frame.syncAndKillEverything();

        pic.fastPathStart = masm.label();
        pic.hasTypeCheck = false;
        pic.typeReg = Registers::ReturnReg;
    }

    pic.shapeReg = shapeReg;
    pic.name = name;

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();

    DataLabelPtr inlineShapeLabel;
    Jump j = masm.branchPtrWithPatch(Assembler::NotEqual, shapeReg,
                                     inlineShapeLabel, ImmPtr(NULL));
    Label inlineShapeJump = masm.label();

    RESERVE_OOL_SPACE(stubcc.masm);
    pic.slowPathStart = stubcc.linkExit(j, Uses(1));
    pic.cached = !forPrototype;

    stubcc.leave();
    passICAddress(&pic);
    pic.slowPathCall = OOL_STUBCALL(ic::GetProp, rejoin);
    CHECK_OOL_SPACE();
    if (rejoin == REJOIN_GETTER)
        testPushedType(rejoin, -1);

    
    Label dslotsLoadLabel = masm.loadPtrWithPatchToLEA(Address(objReg, JSObject::offsetOfSlots()),
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
    labels.setInlineShapeJump(masm, pic.shapeGuard, inlineShapeJump);

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
mjit::Compiler::testSingletonProperty(HandleObject obj, HandleId id)
{
    












    JSObject *nobj = obj;
    while (nobj) {
        if (!nobj->isNative())
            return false;
        if (nobj->getClass()->ops.lookupGeneric)
            return false;
        nobj = nobj->getProto();
    }

    RootedObject holder(cx);
    RootedShape shape(cx);
    if (!JSObject::lookupGeneric(cx, obj, id, &holder, &shape))
        return false;
    if (!shape)
        return false;

    if (shape->hasDefaultGetter()) {
        if (!shape->hasSlot())
            return false;
        if (holder->getSlot(shape->slot()).isUndefined())
            return false;
    } else {
        return false;
    }

    return true;
}

bool
mjit::Compiler::testSingletonPropertyTypes(FrameEntry *top, HandleId id, bool *testObject)
{
    *testObject = false;

    types::StackTypeSet *types = frame.extra(top).types;
    if (!types || types->unknownObject())
        return false;

    RootedObject singleton(cx, types->getSingleton());
    if (singleton)
        return testSingletonProperty(singleton, id);

    if (!globalObj)
        return false;

    JSProtoKey key;
    JSValueType type = types->getKnownTypeTag();
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
                Rooted<JSObject*> proto(cx, object->proto);
                if (!testSingletonProperty(proto, id))
                    return false;

                
                *testObject = (type != JSVAL_TYPE_OBJECT) && !top->isTypeKnown();
                return true;
            }
        }
        return false;

      default:
        return false;
    }

    RootedObject proto(cx);
    if (!js_GetClassPrototype(cx, key, &proto, NULL))
        return false;

    return testSingletonProperty(proto, id);
}

bool
mjit::Compiler::jsop_getprop_dispatch(HandlePropertyName name)
{
    





    FrameEntry *top = frame.peek(-1);
    if (top->isNotType(JSVAL_TYPE_OBJECT))
        return false;

    RootedId id(cx, NameToId(name));
    if (id.get() != types::IdToTypeId(id))
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

    
    Vector<JSObject *> results(CompilerAllocPolicy(cx, *this));

    



    uint32_t last = 0;
    Rooted<JSObject*> proto(cx);
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
        types::HeapTypeSet *ownTypes = object->getProperty(cx, id, false);
        if (ownTypes->isOwnProperty(cx, object, false))
            return false;

        proto = object->proto;
        if (!testSingletonProperty(proto, id))
            return false;

        types::TypeObject *protoType = proto->getType(cx);
        if (!protoType)
            return false;
        if (protoType->unknownProperties())
            return false;
        types::HeapTypeSet *protoTypes = proto->type()->getProperty(cx, id, false);
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
    stubcc.masm.move(ImmPtr(name), Registers::ArgReg1);
    OOL_STUBCALL(stubs::GetProp, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, -1);

    frame.pop();
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, pushreg);

    if (script_->hasScriptCounts)
        bumpPropCount(PC, PCCounts::PROP_DEFINITE);

    stubcc.rejoin(Changes(2));
    return true;
}

bool
mjit::Compiler::jsop_setprop(HandlePropertyName name, bool popGuaranteed)
{
    FrameEntry *lhs = frame.peek(-2);
    FrameEntry *rhs = frame.peek(-1);

    
    if (lhs->isTypeKnown() && lhs->getKnownType() != JSVAL_TYPE_OBJECT) {
        jsop_setprop_slow(name);
        return true;
    }

    



    RootedId id(cx, NameToId(name));
    types::StackTypeSet *types = frame.extra(lhs).types;
    if (JSOp(*PC) == JSOP_SETPROP && id == types::IdToTypeId(id) &&
        types && !types->unknownObject() &&
        types->getObjectCount() == 1 &&
        types->getTypeObject(0) != NULL &&
        !types->getTypeObject(0)->unknownProperties())
    {
        types::TypeObject *object = types->getTypeObject(0);
        types::HeapTypeSet *propertyTypes = object->getProperty(cx, id, false);
        if (!propertyTypes)
            return false;
        if (propertyTypes->definiteProperty() &&
            !propertyTypes->isOwnProperty(cx, object, true)) {
            uint32_t slot = propertyTypes->definiteSlot();
            RegisterID reg = frame.tempRegForData(lhs);
            frame.pinReg(reg);
            bool isObject = lhs->isTypeKnown();
            MaybeJump notObject;
            if (!isObject)
                notObject = frame.testObject(Assembler::NotEqual, lhs);
#ifdef JSGC_INCREMENTAL_MJ
            if (cx->zone()->compileBarriers() && propertyTypes->needsBarrier(cx)) {
                
                Jump j = masm.testGCThing(Address(reg, JSObject::getFixedSlotOffset(slot)));
                stubcc.linkExit(j, Uses(0));
                stubcc.leave();
                stubcc.masm.addPtr(Imm32(JSObject::getFixedSlotOffset(slot)),
                                   reg, Registers::ArgReg1);
                OOL_STUBCALL(stubs::GCThingWriteBarrier, REJOIN_NONE);
                stubcc.rejoin(Changes(0));
            }
#endif
            if (!isObject) {
                stubcc.linkExit(notObject.get(), Uses(2));
                stubcc.leave();
                stubcc.masm.move(ImmPtr(name), Registers::ArgReg1);
                OOL_STUBCALL(stubs::SetProp, REJOIN_FALLTHROUGH);
            }
            frame.storeTo(rhs, Address(reg, JSObject::getFixedSlotOffset(slot)), popGuaranteed);
            frame.unpinReg(reg);
            frame.shimmy(1);
            if (!isObject)
                stubcc.rejoin(Changes(1));
            if (script_->hasScriptCounts)
                bumpPropCount(PC, PCCounts::PROP_DEFINITE);
            return true;
        }
    }

    if (script_->hasScriptCounts)
        bumpPropCount(PC, PCCounts::PROP_OTHER);

#ifdef JSGC_INCREMENTAL_MJ
    
    if (cx->zone()->compileBarriers() &&
        (!types || JSOp(*PC) == JSOP_SETNAME || types->propertyNeedsBarrier(cx, id)))
    {
        jsop_setprop_slow(name);
        return true;
    }
#endif

    PICGenInfo pic(ic::PICInfo::SET, PC);
    pic.name = name;

    if (monitored(PC)) {
        if (script_ == outerScript)
            monitoredBytecodes.append(PC - script_->code);
        pic.typeMonitored = true;
    } else {
        pic.typeMonitored = false;
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

        stubcc.masm.move(ImmPtr(name), Registers::ArgReg1);

        if (*PC == JSOP_SETPROP)
            OOL_STUBCALL(stubs::SetProp, REJOIN_FALLTHROUGH);
        else
            OOL_STUBCALL(stubs::SetName, REJOIN_FALLTHROUGH);

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
    DataLabelPtr inlineShapeData;
    Jump j = masm.branchPtrWithPatch(Assembler::NotEqual, shapeReg,
                                     inlineShapeData, ImmPtr(NULL));
    Label afterInlineShapeJump = masm.label();

    
    {
        pic.slowPathStart = stubcc.linkExit(j, Uses(2));

        stubcc.leave();
        passICAddress(&pic);
        pic.slowPathCall = OOL_STUBCALL(ic::SetPropOrName, REJOIN_FALLTHROUGH);
        CHECK_OOL_SPACE();
    }

    
    Label dslotsLoadLabel = masm.loadPtrWithPatchToLEA(Address(objReg, JSObject::offsetOfSlots()),
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
    labels.setDslotsLoad(masm, pic.fastPathRejoin, dslotsLoadLabel);
    labels.setInlineValueStore(masm, pic.fastPathRejoin, inlineValueStore);
    labels.setInlineShapeJump(masm, pic.shapeGuard, afterInlineShapeJump);

    pics.append(pic);
    return true;
}

bool
mjit::Compiler::jsop_intrinsic(HandlePropertyName name, JSValueType type)
{
    if (type == JSVAL_TYPE_UNKNOWN) {
        prepareStubCall(Uses(0));
        masm.move(ImmPtr(name), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::IntrinsicName, REJOIN_FALLTHROUGH);
        testPushedType(REJOIN_FALLTHROUGH, 0,  false);
        frame.pushSynced(JSVAL_TYPE_UNKNOWN);
        return true;
    }

    RootedValue vp(cx, NullValue());
    if (!cx->global().get()->getIntrinsicValue(cx, name, &vp))
        return false;
    frame.push(vp);
    return true;
}

void
mjit::Compiler::jsop_name(HandlePropertyName name, JSValueType type)
{
    PICGenInfo pic(ic::PICInfo::NAME, PC);

    RESERVE_IC_SPACE(masm);

    pic.shapeReg = frame.allocReg();
    pic.objReg = frame.allocReg();
    pic.typeReg = Registers::ReturnReg;
    pic.name = name;
    pic.hasTypeCheck = false;
    pic.fastPathStart = masm.label();

    
    pic.shapeGuard = masm.label();
    Jump inlineJump = masm.jump();
    {
        RESERVE_OOL_SPACE(stubcc.masm);
        pic.slowPathStart = stubcc.linkExit(inlineJump, Uses(0));
        stubcc.leave();
        passICAddress(&pic);
        pic.slowPathCall = OOL_STUBCALL(ic::Name, REJOIN_GETTER);
        CHECK_OOL_SPACE();
        testPushedType(REJOIN_GETTER, 0);
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
    BarrierState barrier = testBarrier(pic.shapeReg, pic.objReg,  true);

    stubcc.rejoin(Changes(1));

    pics.append(pic);

    finishBarrier(barrier, REJOIN_GETTER, 0);
}

bool
mjit::Compiler::jsop_xname(HandlePropertyName name)
{
    PICGenInfo pic(ic::PICInfo::XNAME, PC);

    FrameEntry *fe = frame.peek(-1);
    if (fe->isNotType(JSVAL_TYPE_OBJECT)) {
        return jsop_getprop(name, knownPushedType(0));
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
    pic.name = name;
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
mjit::Compiler::jsop_bindname(HandlePropertyName name)
{
    PICGenInfo pic(ic::PICInfo::BIND, PC);

    
    
    
    
    JS_ASSERT(analysis->usesScopeChain());

    pic.shapeReg = frame.allocReg();
    pic.objReg = frame.allocReg();
    pic.typeReg = Registers::ReturnReg;
    pic.name = name;
    pic.hasTypeCheck = false;

    RESERVE_IC_SPACE(masm);
    pic.fastPathStart = masm.label();

    masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), pic.objReg);
    masm.loadPtr(Address(pic.objReg, JSObject::offsetOfShape()), pic.shapeReg);
    masm.loadPtr(Address(pic.shapeReg, Shape::offsetOfBase()), pic.shapeReg);
    Address parent(pic.shapeReg, BaseShape::offsetOfParent());

    pic.shapeGuard = masm.label();
    Jump inlineJump = masm.branchPtr(Assembler::NotEqual, parent, ImmPtr(NULL));
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
mjit::Compiler::jsop_name(HandlePropertyName name, JSValueType type, bool isCall)
{
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(isCall ? stubs::CallName : stubs::Name, REJOIN_FALLTHROUGH);
    testPushedType(REJOIN_FALLTHROUGH, 0,  false);
    frame.pushSynced(type);
    if (isCall)
        frame.pushSynced(JSVAL_TYPE_UNKNOWN);
}

bool
mjit::Compiler::jsop_xname(HandlePropertyName name)
{
    return jsop_getprop(name, knownPushedType(0), pushedTypeSet(0));
}

bool
mjit::Compiler::jsop_getprop(HandlePropertyName name, JSValueType knownType, types::TypeSet *typeSet,
                             bool typecheck, bool forPrototype)
{
    jsop_getprop_slow(name, forPrototype);
    return true;
}

bool
mjit::Compiler::jsop_setprop(HandlePropertyName name)
{
    jsop_setprop_slow(name);
    return true;
}

void
mjit::Compiler::jsop_bindname(HandlePropertyName name)
{
    RegisterID reg = frame.allocReg();
    Address scopeChain(JSFrameReg, StackFrame::offsetOfScopeChain());
    masm.loadPtr(scopeChain, reg);

    Address address(reg, offsetof(JSObject, parent));

    Jump j = masm.branchPtr(Assembler::NotEqual, address, ImmPtr(0));

    stubcc.linkExit(j, Uses(0));
    stubcc.leave();
    stubcc.masm.move(ImmPtr(name), Registers::ArgReg1);
    OOL_STUBCALL(stubs::BindName, REJOIN_FALLTHROUGH);

    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, reg);

    stubcc.rejoin(Changes(1));
}
#endif

void
mjit::Compiler::jsop_aliasedArg(unsigned arg, bool get, bool poppedAfter)
{
    RegisterID reg = frame.allocReg(Registers::SavedRegs).reg();
    masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfArgsObj()), reg);
    size_t dataOff = ArgumentsObject::getDataSlotOffset();
    masm.loadPrivate(Address(reg, dataOff), reg);
    int32_t argsOff = ArgumentsData::offsetOfArgs() + arg * sizeof(Value);
    masm.addPtr(Imm32(argsOff), reg, reg);
    if (get) {
        FrameEntry *fe = frame.getArg(arg);
        JSValueType type = fe->isTypeKnown() ? fe->getKnownType() : JSVAL_TYPE_UNKNOWN;
        frame.push(Address(reg), type, true );
    } else {
#ifdef JSGC_INCREMENTAL_MJ
        if (cx->zone()->compileBarriers()) {
            
            stubcc.linkExit(masm.testGCThing(Address(reg)), Uses(0));
            stubcc.leave();
            stubcc.masm.move(reg, Registers::ArgReg1);
            OOL_STUBCALL(stubs::GCThingWriteBarrier, REJOIN_NONE);
            stubcc.rejoin(Changes(0));
        }
#endif
        frame.storeTo(frame.peek(-1), Address(reg), poppedAfter);
        frame.freeReg(reg);
    }
}

void
mjit::Compiler::jsop_aliasedVar(ScopeCoordinate sc, bool get, bool poppedAfter)
{
    RegisterID reg = frame.allocReg(Registers::SavedRegs).reg();
    masm.loadPtr(Address(JSFrameReg, StackFrame::offsetOfScopeChain()), reg);
    for (unsigned i = 0; i < sc.hops; i++)
        masm.loadPayload(Address(reg, ScopeObject::offsetOfEnclosingScope()), reg);

    RawShape shape = ScopeCoordinateToStaticScopeShape(cx, script_, PC);
    Address addr;
    if (shape->numFixedSlots() <= sc.slot) {
        masm.loadPtr(Address(reg, JSObject::offsetOfSlots()), reg);
        addr = Address(reg, (sc.slot - shape->numFixedSlots()) * sizeof(Value));
    } else {
        addr = Address(reg, JSObject::getFixedSlotOffset(sc.slot));
    }

    if (get) {
        JSValueType type = knownPushedType(0);
        RegisterID typeReg, dataReg;
        frame.loadIntoRegisters(addr,  true, &typeReg, &dataReg);
        frame.pushRegs(typeReg, dataReg, type);
        BarrierState barrier = testBarrier(typeReg, dataReg,
                                            false,
                                            false,
                                            true);
        finishBarrier(barrier, REJOIN_FALLTHROUGH, 0);
    } else {
#ifdef JSGC_INCREMENTAL_MJ
        if (cx->zone()->compileBarriers()) {
            
            stubcc.linkExit(masm.testGCThing(addr), Uses(0));
            stubcc.leave();
            stubcc.masm.addPtr(Imm32(addr.offset), addr.base, Registers::ArgReg1);
            OOL_STUBCALL(stubs::GCThingWriteBarrier, REJOIN_NONE);
            stubcc.rejoin(Changes(0));
        }
#endif
        frame.storeTo(frame.peek(-1), addr, poppedAfter);
        frame.freeReg(reg);
    }
}

void
mjit::Compiler::jsop_this()
{
    frame.pushThis();

    




    if (script_->function() && !script_->strict &&
        !script_->function()->isSelfHostedBuiltin())
    {
        FrameEntry *thisFe = frame.peek(-1);

        if (!thisFe->isType(JSVAL_TYPE_OBJECT)) {
            






            if (cx->typeInferenceEnabled() && knownPushedType(0) != JSVAL_TYPE_OBJECT) {
                prepareStubCall(Uses(1));
                INLINE_STUBCALL(stubs::This, REJOIN_FALLTHROUGH);
                return;
            }

            JSValueType type = cx->typeInferenceEnabled()
                ? types::TypeScript::ThisTypes(script_)->getKnownTypeTag()
                : JSVAL_TYPE_UNKNOWN;
            if (type != JSVAL_TYPE_OBJECT) {
                Jump notObj = frame.testObject(Assembler::NotEqual, thisFe);
                stubcc.linkExit(notObj, Uses(1));
                stubcc.leave();
                OOL_STUBCALL(stubs::This, REJOIN_FALLTHROUGH);
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
mjit::Compiler::iter(unsigned flags)
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

    
    masm.loadPtr(&cx->runtime->nativeIterCache.last, ioreg);

    
    Jump nullIterator = masm.branchTest32(Assembler::Zero, ioreg, ioreg);
    stubcc.linkExit(nullIterator, Uses(1));

    
    masm.loadObjPrivate(ioreg, nireg, JSObject::ITER_CLASS_NFIXED_SLOTS);

    
    Address flagsAddr(nireg, offsetof(NativeIterator, flags));
    masm.load32(flagsAddr, T1);
    Jump activeIterator = masm.branchTest32(Assembler::NonZero, T1,
                                            Imm32(JSITER_ACTIVE|JSITER_UNREUSABLE));
    stubcc.linkExit(activeIterator, Uses(1));

    
    masm.loadShape(reg, T1);
    masm.loadPtr(Address(nireg, offsetof(NativeIterator, shapes_array)), T2);
    masm.loadPtr(Address(T2, 0), T2);
    Jump mismatchedObject = masm.branchPtr(Assembler::NotEqual, T1, T2);
    stubcc.linkExit(mismatchedObject, Uses(1));

    
    masm.loadPtr(Address(reg, JSObject::offsetOfType()), T1);
    masm.loadPtr(Address(T1, offsetof(types::TypeObject, proto)), T1);
    masm.loadShape(T1, T1);
    masm.loadPtr(Address(nireg, offsetof(NativeIterator, shapes_array)), T2);
    masm.loadPtr(Address(T2, sizeof(RawShape)), T2);
    Jump mismatchedProto = masm.branchPtr(Assembler::NotEqual, T1, T2);
    stubcc.linkExit(mismatchedProto, Uses(1));

    





    masm.loadPtr(Address(reg, JSObject::offsetOfType()), T1);
    masm.loadPtr(Address(T1, offsetof(types::TypeObject, proto)), T1);
    masm.loadPtr(Address(T1, JSObject::offsetOfType()), T1);
    masm.loadPtr(Address(T1, offsetof(types::TypeObject, proto)), T1);
    Jump overlongChain = masm.branchPtr(Assembler::NonZero, T1, T1);
    stubcc.linkExit(overlongChain, Uses(1));

    
    Address elementsAddress(reg, JSObject::offsetOfElements());
    Jump hasElements = masm.branchPtr(Assembler::NotEqual, elementsAddress,
                                      ImmPtr(js::emptyObjectElements));
    stubcc.linkExit(hasElements, Uses(1));

#ifdef JSGC_INCREMENTAL_MJ
    



    if (cx->zone()->compileBarriers()) {
        Jump j = masm.branchPtr(Assembler::NotEqual,
                                Address(nireg, offsetof(NativeIterator, obj)), reg);
        stubcc.linkExit(j, Uses(1));
    }
#endif

    

    
    masm.storePtr(reg, Address(nireg, offsetof(NativeIterator, obj)));
    masm.load32(flagsAddr, T1);
    masm.or32(Imm32(JSITER_ACTIVE), T1);
    masm.store32(T1, flagsAddr);

    
    masm.move(ImmPtr(cx->compartment), T1);
    masm.loadPtr(Address(T1, offsetof(JSCompartment, enumerators)), T1);

    
    masm.storePtr(T1, Address(nireg, NativeIterator::offsetOfNext()));

    
    masm.loadPtr(Address(T1, NativeIterator::offsetOfPrev()), T2);
    masm.storePtr(T2, Address(nireg, NativeIterator::offsetOfPrev()));

    
    masm.storePtr(nireg, Address(T2, NativeIterator::offsetOfNext()));

    
    masm.storePtr(nireg, Address(T1, NativeIterator::offsetOfPrev()));

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
mjit::Compiler::iterNext()
{
    FrameEntry *fe = frame.peek(-1);
    RegisterID reg = frame.tempRegForData(fe);

    
    frame.pinReg(reg);
    RegisterID T1 = frame.allocReg();
    frame.unpinReg(reg);

    
    Jump notFast = masm.testObjClass(Assembler::NotEqual, reg, T1,
                                     &PropertyIteratorObject::class_);
    stubcc.linkExit(notFast, Uses(1));

    
    masm.loadObjPrivate(reg, T1, JSObject::ITER_CLASS_NFIXED_SLOTS);

    RegisterID T3 = frame.allocReg();
    RegisterID T4 = frame.allocReg();

    
    masm.load32(Address(T1, offsetof(NativeIterator, flags)), T3);
    notFast = masm.branchTest32(Assembler::NonZero, T3, Imm32(JSITER_FOREACH));
    stubcc.linkExit(notFast, Uses(1));

    RegisterID T2 = frame.allocReg();

    
    masm.loadPtr(Address(T1, offsetof(NativeIterator, props_cursor)), T2);

    
    masm.loadPtr(T2, T3);

    
    masm.addPtr(Imm32(sizeof(JSString*)), T2, T4);
    masm.storePtr(T4, Address(T1, offsetof(NativeIterator, props_cursor)));

    frame.freeReg(T4);
    frame.freeReg(T1);
    frame.freeReg(T2);

    stubcc.leave();
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

    
    Jump notFast = masm.testObjClass(Assembler::NotEqual, reg, tempreg,
                                     &PropertyIteratorObject::class_);
    stubcc.linkExitForBranch(notFast);

    
    masm.loadObjPrivate(reg, reg, JSObject::ITER_CLASS_NFIXED_SLOTS);

    
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

    return jumpAndRun(jFast, target, &j);
}

void
mjit::Compiler::iterEnd()
{
    FrameEntry *fe= frame.peek(-1);
    RegisterID reg = frame.tempRegForData(fe);

    frame.pinReg(reg);
    RegisterID T1 = frame.allocReg();
    frame.unpinReg(reg);

    
    Jump notIterator = masm.testObjClass(Assembler::NotEqual, reg, T1,
                                         &PropertyIteratorObject::class_);
    stubcc.linkExit(notIterator, Uses(1));

    
    masm.loadObjPrivate(reg, T1, JSObject::ITER_CLASS_NFIXED_SLOTS);

    RegisterID T2 = frame.allocReg();

    
    Address flagAddr(T1, offsetof(NativeIterator, flags));
    masm.loadPtr(flagAddr, T2);

    
    Jump notEnumerate = masm.branchTest32(Assembler::Zero, T2, Imm32(JSITER_ENUMERATE));
    stubcc.linkExit(notEnumerate, Uses(1));

    
    masm.and32(Imm32(~JSITER_ACTIVE), T2);
    masm.storePtr(T2, flagAddr);

    
    masm.loadPtr(Address(T1, offsetof(NativeIterator, props_array)), T2);
    masm.storePtr(T2, Address(T1, offsetof(NativeIterator, props_cursor)));

    
    RegisterID prev = T2;
    RegisterID next = frame.allocReg();

    masm.loadPtr(Address(T1, NativeIterator::offsetOfNext()), next);
    masm.loadPtr(Address(T1, NativeIterator::offsetOfPrev()), prev);
    masm.storePtr(prev, Address(next, NativeIterator::offsetOfPrev()));
    masm.storePtr(next, Address(prev, NativeIterator::offsetOfNext()));
#ifdef DEBUG
    masm.storePtr(ImmPtr(NULL), Address(T1, NativeIterator::offsetOfNext()));
    masm.storePtr(ImmPtr(NULL), Address(T1, NativeIterator::offsetOfPrev()));
#endif

    frame.freeReg(T1);
    frame.freeReg(T2);
    frame.freeReg(next);

    stubcc.leave();
    OOL_STUBCALL(stubs::EndIter, REJOIN_FALLTHROUGH);

    frame.pop();

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_getgname_slow(uint32_t index)
{
    prepareStubCall(Uses(0));
    INLINE_STUBCALL(stubs::Name, REJOIN_GETTER);
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

bool
mjit::Compiler::jsop_getgname(uint32_t index)
{
    
    PropertyName *name = script_->getName(index);
    if (name == cx->names().undefined) {
        frame.push(UndefinedValue());
        return true;
    }
    if (name == cx->names().NaN) {
        frame.push(cx->runtime->NaNValue);
        return true;
    }
    if (name == cx->names().Infinity) {
        frame.push(cx->runtime->positiveInfinityValue);
        return true;
    }

    
    JSObject *obj = pushedSingleton(0);
    if (obj && !hasTypeBarriers(PC)) {
        Rooted<jsid> id(cx, NameToId(name));
        if (testSingletonProperty(globalObj, id)) {
            frame.push(ObjectValue(*obj));
            return true;
        }
    }

    RootedId id(cx, NameToId(name));
    JSValueType type = knownPushedType(0);

    types::TypeObject *globalType = NULL;
    if (cx->typeInferenceEnabled() && globalObj->isGlobal() && id == types::IdToTypeId(id)) {
        globalType = globalObj->getType(cx);
        if (globalType && globalType->unknownProperties())
            globalType = NULL;
    }

    if (globalType) {
        types::HeapTypeSet *propertyTypes = globalType->getProperty(cx, id, false);
        if (!propertyTypes)
            return false;

        




        RootedId id(cx, NameToId(name));
        RawShape shape = globalObj->nativeLookup(cx, id);
        if (shape && shape->hasDefaultGetter() && shape->hasSlot()) {
            HeapSlot *value = &globalObj->getSlotRef(shape->slot());
            if (!value->isUndefined() && !propertyTypes->isOwnProperty(cx, globalType, true)) {
                if (!watchGlobalReallocation())
                    return false;
                RegisterID reg = frame.allocReg();
                masm.move(ImmPtr(value), reg);

                BarrierState barrier = pushAddressMaybeBarrier(Address(reg), type, true);
                finishBarrier(barrier, REJOIN_GETTER, 0);
                return true;
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

    ic.fastPathStart = masm.label();
    if (fe->isConstant()) {
        JSObject *obj = &fe->getValue().toObject();
        frame.pop();
        JS_ASSERT(obj->isNative());

        objReg = frame.allocReg();

        masm.loadPtrFromImm(obj->addressOfShape(), objReg);
        shapeGuard = masm.branchPtrWithPatch(Assembler::NotEqual, objReg,
                                             ic.shape, ImmPtr(NULL));
        masm.move(ImmPtr(obj), objReg);
    } else {
        objReg = frame.ownRegForData(fe);
        frame.pop();
        RegisterID reg = frame.allocReg();

        masm.loadShape(objReg, reg);
        shapeGuard = masm.branchPtrWithPatch(Assembler::NotEqual, reg,
                                             ic.shape, ImmPtr(NULL));
        frame.freeReg(reg);
    }
    stubcc.linkExit(shapeGuard, Uses(0));

    stubcc.leave();
    passMICAddress(ic);
    ic.slowPathCall = OOL_STUBCALL(ic::GetGlobalName, REJOIN_GETTER);

    CHECK_IC_SPACE();

    testPushedType(REJOIN_GETTER, 0);

    
    uint32_t slot = 1 << 24;

    masm.loadPtr(Address(objReg, JSObject::offsetOfSlots()), objReg);
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
    return true;
}

void
mjit::Compiler::jsop_setgname_slow(HandlePropertyName name)
{
    prepareStubCall(Uses(2));
    masm.move(ImmPtr(name), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::SetName, REJOIN_FALLTHROUGH);
    frame.popn(2);
    pushSyncedEntry(0);
}

bool
mjit::Compiler::jsop_setgname(HandlePropertyName name, bool popGuaranteed)
{
    if (monitored(PC)) {
        if (script_ == outerScript)
            monitoredBytecodes.append(PC - script_->code);

        
        jsop_setgname_slow(name);
        return true;
    }

    RootedId id(cx, NameToId(name));
    types::TypeObject *globalType = NULL;
    if (cx->typeInferenceEnabled() && globalObj->isGlobal() && id == types::IdToTypeId(id)) {
        globalType = globalObj->getType(cx);
        if (globalType && globalType->unknownProperties())
            globalType = NULL;
    }

    if (globalType) {
        





        types::HeapTypeSet *types = globalType->getProperty(cx, id, false);
        if (!types)
            return false;
        RootedId id(cx, NameToId(name));
        RootedShape shape(cx, globalObj->nativeLookup(cx, id));
        if (shape && shape->hasDefaultSetter() &&
            shape->writable() && shape->hasSlot() &&
            !types->isOwnProperty(cx, globalType, true))
        {
            if (!watchGlobalReallocation())
                return false;
            HeapSlot *value = &globalObj->getSlotRef(shape->slot());
            RegisterID reg = frame.allocReg();
#ifdef JSGC_INCREMENTAL_MJ
            
            if (cx->zone()->compileBarriers() && types->needsBarrier(cx)) {
                stubcc.linkExit(masm.jump(), Uses(0));
                stubcc.leave();
                stubcc.masm.move(ImmPtr(value), Registers::ArgReg1);
                OOL_STUBCALL(stubs::WriteBarrier, REJOIN_NONE);
                stubcc.rejoin(Changes(0));
            }
#endif
            masm.move(ImmPtr(value), reg);
            frame.storeTo(frame.peek(-1), Address(reg), popGuaranteed);
            frame.shimmy(1);
            frame.freeReg(reg);
            return true;
        }
    }

#ifdef JSGC_INCREMENTAL_MJ
    
    if (cx->zone()->compileBarriers()) {
        jsop_setgname_slow(name);
        return true;
    }
#endif

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

        masm.loadPtrFromImm(obj->addressOfShape(), ic.shapeReg);
        shapeGuard = masm.branchPtrWithPatch(Assembler::NotEqual, ic.shapeReg,
                                             ic.shape, ImmPtr(NULL));
        masm.move(ImmPtr(obj), ic.objReg);
    } else {
        ic.objReg = frame.copyDataIntoReg(objFe);
        ic.shapeReg = frame.allocReg();
        ic.objConst = false;

        masm.loadShape(ic.objReg, ic.shapeReg);
        shapeGuard = masm.branchPtrWithPatch(Assembler::NotEqual, ic.shapeReg,
                                             ic.shape, ImmPtr(NULL));
        frame.freeReg(ic.shapeReg);
    }
    ic.shapeGuardJump = shapeGuard;
    ic.slowPathStart = stubcc.linkExit(shapeGuard, Uses(2));

    stubcc.leave();
    passMICAddress(ic);
    ic.slowPathCall = OOL_STUBCALL(ic::SetGlobalName, REJOIN_FALLTHROUGH);

    
    uint32_t slot = 1 << 24;

    masm.loadPtr(Address(ic.objReg, JSObject::offsetOfSlots()), ic.objReg);
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
    jsop_setgname_slow(name);
#endif
    return true;
}

void
mjit::Compiler::jsop_setelem_slow()
{
    prepareStubCall(Uses(3));
    INLINE_STUBCALL(STRICT_VARIANT(script_, stubs::SetElem), REJOIN_FALLTHROUGH);
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

    RegisterID tmp = frame.allocReg();
    RegisterID obj = frame.tempRegForData(rhs);

    masm.loadPtr(Address(obj, JSObject::offsetOfType()), tmp);
    Jump notFunction = masm.branchPtr(Assembler::NotEqual,
                                      Address(tmp, offsetof(types::TypeObject, clasp)),
                                      ImmPtr(&FunctionClass));

    stubcc.linkExit(notFunction, Uses(2));

    
    masm.loadBaseShape(obj, tmp);
    Jump isBound = masm.branchTest32(Assembler::NonZero,
                                     Address(tmp, BaseShape::offsetOfFlags()),
                                     Imm32(BaseShape::BOUND_FUNCTION));
    {
        stubcc.linkExit(isBound, Uses(2));
        stubcc.leave();
        OOL_STUBCALL(stubs::InstanceOf, REJOIN_FALLTHROUGH);
        firstSlow = stubcc.masm.jump();
    }

    frame.freeReg(tmp);

    
    frame.dup();

    if (!jsop_getprop(cx->names().classPrototype, JSVAL_TYPE_UNKNOWN))
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
    Jump isLazy = masm.branch32(Assembler::Equal, obj, Imm32(1));
    stubcc.linkExit(isLazy, Uses(2));
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
mjit::Compiler::emitEval(uint32_t argc)
{
    
    interruptCheckHelper();

    frame.syncAndKill(Uses(argc + 2));
    prepareStubCall(Uses(argc + 2));
    masm.move(Imm32(argc), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::Eval, REJOIN_FALLTHROUGH);
    frame.popn(argc + 2);
    pushSyncedEntry(0);
}

Compiler::Jump
Compiler::getNewObject(JSContext *cx, RegisterID result, JSObject *templateObject)
{
    rootedTemplates.append(templateObject);
    return masm.getNewObject(cx, result, templateObject);
}

bool
mjit::Compiler::jsop_newinit()
{
    bool isArray;
    unsigned count = 0;
    RootedObject baseobj(cx);
    switch (*PC) {
      case JSOP_NEWINIT:
        isArray = (GET_UINT8(PC) == JSProto_Array);
        break;
      case JSOP_NEWARRAY:
        isArray = true;
        count = GET_UINT24(PC);
        break;
      case JSOP_NEWOBJECT:
        




        isArray = false;
        baseobj = globalObj ? script_->getObject(GET_UINT32_INDEX(PC)) : NULL;
        break;
      default:
        JS_NOT_REACHED("Bad op");
        return false;
    }

    void *stub, *stubArg;
    if (isArray) {
        stub = JS_FUNC_TO_DATA_PTR(void *, stubs::NewInitArray);
        stubArg = (void *) uintptr_t(count);
    } else {
        stub = JS_FUNC_TO_DATA_PTR(void *, stubs::NewInitObject);
        stubArg = (void *) baseobj;
    }

    JSProtoKey key = isArray ? JSProto_Array : JSProto_Object;

    



    RootedScript script(cx, script_);
    RootedTypeObject type(cx);
    if (globalObj && !types::UseNewTypeForInitializer(cx, script, PC, key)) {
        type = types::TypeScript::InitObject(cx, script, PC, key);
        if (!type)
            return false;
    }

    size_t maxArraySlots =
        gc::GetGCKindSlots(gc::FINALIZE_OBJECT_LAST) - ObjectElements::VALUES_PER_HEADER;

    if (!cx->typeInferenceEnabled() ||
        !type ||
        (isArray && count > maxArraySlots) ||
        (!isArray && !baseobj) ||
        (!isArray && baseobj->hasDynamicSlots()))
    {
        prepareStubCall(Uses(0));
        masm.storePtr(ImmPtr(type), FrameAddress(offsetof(VMFrame, scratch)));
        masm.move(ImmPtr(stubArg), Registers::ArgReg1);
        INLINE_STUBCALL(stub, REJOIN_FALLTHROUGH);
        frame.pushSynced(knownPushedType(0));

        frame.extra(frame.peek(-1)).initObject = baseobj;
        return true;
    }

    JSObject *templateObject;
    if (isArray) {
        templateObject = NewDenseUnallocatedArray(cx, count);
        types::StackTypeSet::DoubleConversion conversion =
            script->analysis()->pushedTypes(PC, 0)->convertDoubleElements(cx);
        if (templateObject && conversion == types::StackTypeSet::AlwaysConvertToDoubles)
            templateObject->setShouldConvertDoubleElements();
    } else {
        templateObject = CopyInitializerObject(cx, baseobj);
    }
    if (!templateObject)
        return false;
    templateObject->setType(type);

    RegisterID result = frame.allocReg();
    Jump emptyFreeList = getNewObject(cx, result, templateObject);

    stubcc.linkExit(emptyFreeList, Uses(0));
    stubcc.leave();

    stubcc.masm.storePtr(ImmPtr(type), FrameAddress(offsetof(VMFrame, scratch)));
    stubcc.masm.move(ImmPtr(stubArg), Registers::ArgReg1);
    OOL_STUBCALL(stub, REJOIN_FALLTHROUGH);

    frame.pushTypedPayload(knownPushedType(0), result);

    stubcc.rejoin(Changes(1));

    frame.extra(frame.peek(-1)).initObject = baseobj;
    return true;
}

bool
mjit::Compiler::jsop_regexp()
{
    JSObject *obj = script_->getRegExp(GET_UINT32_INDEX(PC));
    RegExpStatics *res = globalObj ? globalObj->getRegExpStatics() : NULL;

    types::TypeObject *globalType;
    if (globalObj) {
        globalType = globalObj->getType(cx);
        if (!globalType)
            return false;
    }
    if (!globalObj ||
        &obj->global() != globalObj ||
        !cx->typeInferenceEnabled() ||
        analysis->localsAliasStack() ||
        types::HeapTypeSet::HasObjectFlags(cx, globalType, types::OBJECT_FLAG_REGEXP_FLAGS_SET))
    {
        prepareStubCall(Uses(0));
        masm.move(ImmPtr(obj), Registers::ArgReg1);
        INLINE_STUBCALL(stubs::RegExp, REJOIN_FALLTHROUGH);
        frame.pushSynced(JSVAL_TYPE_OBJECT);
        return true;
    }

    RegExpObject *reobj = &obj->asRegExp();

    DebugOnly<uint32_t> origFlags = reobj->getFlags();
    DebugOnly<uint32_t> staticsFlags = res->getFlags();
    JS_ASSERT((origFlags & staticsFlags) == staticsFlags);

    








    analyze::SSAUseChain *uses =
        analysis->useChain(analyze::SSAValue::PushedValue(PC - script_->code, 0));
    if (uses && uses->popped && !uses->next && !reobj->global() && !reobj->sticky()) {
        jsbytecode *use = script_->code + uses->offset;
        uint32_t which = uses->u.which;
        if (JSOp(*use) == JSOP_CALLPROP) {
            JSObject *callee = analysis->pushedTypes(use, 0)->getSingleton();
            if (callee && callee->isFunction()) {
                Native native = callee->toFunction()->maybeNative();
                if (native == js::regexp_exec || native == js::regexp_test) {
                    frame.push(ObjectValue(*obj));
                    return true;
                }
            }
        } else if (JSOp(*use) == JSOP_CALL && which == 0) {
            uint32_t argc = GET_ARGC(use);
            JSObject *callee = analysis->poppedTypes(use, argc + 1)->getSingleton();
            if (callee && callee->isFunction() && argc >= 1 && which == argc - 1) {
                Native native = callee->toFunction()->maybeNative();
                if (native == js::str_match ||
                    native == js::str_search ||
                    native == js::str_replace ||
                    native == js::str_split) {
                    frame.push(ObjectValue(*obj));
                    return true;
                }
            }
        }
    }

    






    RegExpGuard g(cx);
    if (!reobj->getShared(cx, &g))
        return false;

    rootedRegExps.append(g.re());

    RegisterID result = frame.allocReg();
    Jump emptyFreeList = getNewObject(cx, result, obj);

    stubcc.linkExit(emptyFreeList, Uses(0));
    stubcc.leave();

    stubcc.masm.move(ImmPtr(obj), Registers::ArgReg1);
    OOL_STUBCALL(stubs::RegExp, REJOIN_FALLTHROUGH);

    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, result);

    stubcc.rejoin(Changes(1));
    return true;
}

bool
mjit::Compiler::startLoop(jsbytecode *head, Jump entry, jsbytecode *entryTarget)
{
    JS_ASSERT(cx->typeInferenceEnabled() && script_ == outerScript);
    JS_ASSERT(shouldStartLoop(head));

    if (loop) {
        





        loop->clearLoopRegisters();
    }

    LoopState *nloop = js_new<LoopState>(cx, &ssa, this, &frame);
    if (!nloop || !nloop->init(head, entry, entryTarget)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    nloop->outer = loop;
    loop = nloop;
    frame.setLoop(loop);

    return true;
}

bool
mjit::Compiler::finishLoop(jsbytecode *head)
{
    if (!cx->typeInferenceEnabled() || !bytecodeInChunk(head))
        return true;

    




    JS_ASSERT(loop && loop->headOffset() == uint32_t(head - script_->code));

    jsbytecode *entryTarget = script_->code + loop->entryOffset();

    




    Jump fallthrough = masm.jump();

#ifdef DEBUG
    if (IsJaegerSpewChannelActive(JSpew_Regalloc)) {
        RegisterAllocation *alloc = analysis->getAllocation(head);
        JaegerSpew(JSpew_Regalloc, "loop allocation at %u:", unsigned(head - script_->code));
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
        entry.pcOffset = head - script_->code;

        OOL_STUBCALL(stubs::MissedBoundsCheckHead, REJOIN_RESUME);

        if (loop->generatingInvariants()) {
            if (oomInVector)
                return false;
            entry.label = callSites[callSites.length() - 1].loopJumpLabel;
        } else {
            entry.label = stubcc.masm.label();
        }

        






        for (uint32_t slot = ArgSlot(0); slot < TotalSlots(script_); slot++) {
            if (a->varTypes[slot].getTypeTag() == JSVAL_TYPE_DOUBLE) {
                FrameEntry *fe = frame.getSlotEntry(slot);
                stubcc.masm.ensureInMemoryDouble(frame.addressOf(fe));
            }
        }

        



        const SlotValue *newv = analysis->newValues(head);
        if (newv) {
            while (newv->slot) {
                if (newv->value.kind() == SSAValue::PHI &&
                    newv->value.phiOffset() == uint32_t(head - script_->code) &&
                    analysis->trackSlot(newv->slot))
                {
                    JS_ASSERT(newv->slot < TotalSlots(script_));
                    types::StackTypeSet *targetTypes = analysis->getValueTypes(newv->value);
                    if (targetTypes->getKnownTypeTag() == JSVAL_TYPE_DOUBLE) {
                        FrameEntry *fe = frame.getSlotEntry(newv->slot);
                        stubcc.masm.ensureInMemoryDouble(frame.addressOf(fe));
                    }
                }
                newv++;
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
    js_delete(loop);
    loop = nloop;
    frame.setLoop(loop);

    fallthrough.linkTo(masm.label(), &masm);

    



    frame.clearTemporaries();

    return true;
}















bool
mjit::Compiler::jumpAndRun(Jump j, jsbytecode *target, Jump *slow, bool *trampoline,
                           bool fallthrough)
{
    if (trampoline)
        *trampoline = false;

    if (!a->parent && !bytecodeInChunk(target)) {
        




        OutgoingChunkEdge edge;
        edge.source = PC - outerScript->code;
        JSOp op = JSOp(*PC);
        if (!fallthrough && !(js_CodeSpec[op].format & JOF_JUMP) && op != JSOP_TABLESWITCH)
            edge.source += GetBytecodeLength(PC);
        edge.target = target - outerScript->code;
        edge.fastJump = j;
        if (slow)
            edge.slowJump = *slow;
        chunkEdges.append(edge);
        return true;
    }

    



    RegisterAllocation *lvtarget = NULL;
    bool consistent = true;
    if (cx->typeInferenceEnabled()) {
        RegisterAllocation *&alloc = analysis->getAllocation(target);
        if (!alloc) {
            alloc = cx->analysisLifoAlloc().new_<RegisterAllocation>(false);
            if (!alloc) {
                js_ReportOutOfMemory(cx);
                return false;
            }
        }
        lvtarget = alloc;
        consistent = frame.consistentRegisters(target);
    }

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
            if (pcLengths) {
                



                uint32_t offset = ssa.frameLength(a->inlineIndex) + PC - script_->code;
                size_t length = stubcc.masm.size() - stubcc.masm.distanceOf(start);
                pcLengths[offset].codeLengthAugment += length;
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

void
mjit::Compiler::enterBlock(StaticBlockObject *block)
{
    
    frame.syncAndForgetEverything();
    masm.move(ImmPtr(block), Registers::ArgReg1);
    INLINE_STUBCALL(stubs::EnterBlock, REJOIN_FALLTHROUGH);
    if (*PC == JSOP_ENTERBLOCK)
        frame.enterBlock(StackDefs(script_, PC));
}

void
mjit::Compiler::leaveBlock()
{
    



    uint32_t n = StackUses(script_, PC);
    prepareStubCall(Uses(n));
    INLINE_STUBCALL(stubs::LeaveBlock, REJOIN_NONE);
    frame.leaveBlock(n);
}









bool
mjit::Compiler::constructThis()
{
    JS_ASSERT(isConstructing);

    RootedFunction fun(cx, script_->function());

    do {
        if (!cx->typeInferenceEnabled() ||
            !fun->hasSingletonType())
        {
            break;
        }

        types::TypeObject *funType = fun->getType(cx);
        if (!funType)
            return false;
        if (funType->unknownProperties())
            break;

        Rooted<jsid> id(cx, NameToId(cx->names().classPrototype));
        types::HeapTypeSet *protoTypes = funType->getProperty(cx, HandleId(id), false);

        JSObject *proto = protoTypes->getSingleton(cx);
        if (!proto)
            break;

        




        types::TypeObject *type = proto->getNewType(cx, &ObjectClass, fun);
        if (!type)
            return false;
        if (!types::TypeScript::ThisTypes(script_)->hasType(types::Type::ObjectType(type)))
            break;

        JSObject *templateObject = CreateThisForFunctionWithProto(cx, fun, proto);
        if (!templateObject)
            return false;

        




        if (templateObject->type()->newScript)
            types::HeapTypeSet::WatchObjectStateChange(cx, templateObject->type());

        RegisterID result = frame.allocReg();
        Jump emptyFreeList = getNewObject(cx, result, templateObject);

        stubcc.linkExit(emptyFreeList, Uses(0));
        stubcc.leave();

        stubcc.masm.move(ImmPtr(proto), Registers::ArgReg1);
        OOL_STUBCALL(stubs::CreateThis, REJOIN_THIS_CREATED);

        frame.setThis(result);

        stubcc.rejoin(Changes(1));
        return true;
    } while (false);

    
    frame.pushCallee();

    
    if (!jsop_getprop(cx->names().classPrototype, JSVAL_TYPE_UNKNOWN, false,  true))
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
    INLINE_STUBCALL(stubs::CreateThis, REJOIN_THIS_CREATED);
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
    DebugOnly<JSOp> op = JSOp(*originalPC);
    JS_ASSERT(op == JSOP_TABLESWITCH);

    uint32_t defaultTarget = GET_JUMP_OFFSET(pc);
    pc += JUMP_OFFSET_LEN;

    int32_t low = GET_JUMP_OFFSET(pc);
    pc += JUMP_OFFSET_LEN;
    int32_t high = GET_JUMP_OFFSET(pc);
    pc += JUMP_OFFSET_LEN;
    int numJumps = high + 1 - low;
    JS_ASSERT(numJumps >= 0);

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
    jt.offsetIndex = jumpTableEdges.length();
    jt.label = masm.moveWithPatch(ImmPtr(NULL), reg);
    jumpTables.append(jt);

    for (int i = 0; i < numJumps; i++) {
        uint32_t target = GET_JUMP_OFFSET(pc);
        if (!target)
            target = defaultTarget;
        JumpTableEdge edge;
        edge.source = originalPC - script_->code;
        edge.target = (originalPC + target) - script_->code;
        jumpTableEdges.append(edge);
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
        OOL_STUBCALL(stubs::TableSwitch, REJOIN_NONE);
        stubcc.masm.jump(Registers::ReturnReg);
    }
    frame.pop();
    return jumpAndRun(defaultCase, originalPC + defaultTarget);
#endif
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
mjit::Compiler::jsop_in()
{
    FrameEntry *obj = frame.peek(-1);
    FrameEntry *id = frame.peek(-2);

    if (cx->typeInferenceEnabled() && id->isType(JSVAL_TYPE_INT32)) {
        types::StackTypeSet *types = analysis->poppedTypes(PC, 0);

        if (obj->mightBeType(JSVAL_TYPE_OBJECT) &&
            types->getKnownClass() == &ArrayClass &&
            !types->hasObjectFlags(cx, types::OBJECT_FLAG_SPARSE_INDEXES) &&
            !types::ArrayPrototypeHasIndexedProperty(cx, outerScript))
        {
            frame.forgetMismatchedObject(obj);
            bool isPacked = !types->hasObjectFlags(cx, types::OBJECT_FLAG_NON_PACKED);

            if (!obj->isTypeKnown()) {
                Jump guard = frame.testObject(Assembler::NotEqual, obj);
                stubcc.linkExit(guard, Uses(2));
            }

            RegisterID dataReg = frame.copyDataIntoReg(obj);

            Int32Key key = id->isConstant()
                         ? Int32Key::FromConstant(id->getValue().toInt32())
                         : Int32Key::FromRegister(frame.tempRegForData(id));

            masm.loadPtr(Address(dataReg, JSObject::offsetOfElements()), dataReg);

            
            Jump initlenGuard = masm.guardArrayExtent(ObjectElements::offsetOfInitializedLength(),
                                                      dataReg, key, Assembler::BelowOrEqual);

            
            MaybeJump holeCheck;
            if (!isPacked)
                holeCheck = masm.guardElementNotHole(dataReg, key);

            masm.move(Imm32(1), dataReg);
            Jump done = masm.jump();

            Label falseBranch = masm.label();
            initlenGuard.linkTo(falseBranch, &masm);
            if (!isPacked)
                holeCheck.getJump().linkTo(falseBranch, &masm);
            masm.move(Imm32(0), dataReg);

            done.linkTo(masm.label(), &masm);

            stubcc.leave();
            OOL_STUBCALL_USES(stubs::In, REJOIN_PUSH_BOOLEAN, Uses(2));

            frame.popn(2);
            if (dataReg != Registers::ReturnReg)
                stubcc.masm.move(Registers::ReturnReg, dataReg);

            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, dataReg);

            stubcc.rejoin(Changes(2));

            return;
        }
    }

    prepareStubCall(Uses(2));
    INLINE_STUBCALL(stubs::In, REJOIN_PUSH_BOOLEAN);
    frame.popn(2);
    frame.takeReg(Registers::ReturnReg);
    frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
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
                newv->value.phiOffset() != uint32_t(target - script_->code) ||
                !analysis->trackSlot(newv->slot)) {
                newv++;
                continue;
            }
            JS_ASSERT(newv->slot < TotalSlots(script_));
            types::StackTypeSet *targetTypes = analysis->getValueTypes(newv->value);
            FrameEntry *fe = frame.getSlotEntry(newv->slot);
            VarType &vt = a->varTypes[newv->slot];
            JSValueType type = vt.getTypeTag();
            if (targetTypes->getKnownTypeTag() == JSVAL_TYPE_DOUBLE) {
                if (type == JSVAL_TYPE_INT32) {
                    fixedIntToDoubleEntries.append(newv->slot);
                    frame.ensureDouble(fe);
                    frame.forgetLoopReg(fe);
                } else if (type == JSVAL_TYPE_UNKNOWN) {
                    





                    frame.ensureDouble(fe);
                } else {
                    JS_ASSERT(type == JSVAL_TYPE_DOUBLE);
                }
            } else if (type == JSVAL_TYPE_DOUBLE) {
                fixedDoubleToAnyEntries.append(newv->slot);
                frame.syncAndForgetFe(fe);
                frame.forgetLoopReg(fe);
            }
            newv++;
        }
    }
}

bool
mjit::Compiler::watchGlobalReallocation()
{
    JS_ASSERT(cx->typeInferenceEnabled());
    if (hasGlobalReallocation)
        return true;
    types::TypeObject *globalType = globalObj->getType(cx);
    if (!globalType)
        return false;
    types::HeapTypeSet::WatchObjectStateChange(cx, globalType);
    hasGlobalReallocation = true;
    return true;
}

void
mjit::Compiler::updateVarType()
{
    if (!cx->typeInferenceEnabled())
        return;

    






    types::StackTypeSet *types = pushedTypeSet(0);
    uint32_t slot = GetBytecodeSlot(script_, PC);

    if (analysis->trackSlot(slot)) {
        VarType &vt = a->varTypes[slot];
        vt.setTypes(types);

        




        if (vt.getTypeTag() == JSVAL_TYPE_DOUBLE)
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
            if (newv->slot < TotalSlots(script_)) {
                VarType &vt = a->varTypes[newv->slot];
                JSValueType type = vt.getTypeTag();
                vt.setTypes(analysis->getValueTypes(newv->value));
                if (vt.getTypeTag() != type) {
                    




                    FrameEntry *fe = frame.getSlotEntry(newv->slot);
                    frame.forgetLoopReg(fe);
                }
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

    uint32_t slot = GetBytecodeSlot(script_, PC);

    if (slot >= analyze::TotalSlots(script_))
        return;

    




    JSValueType type = a->varTypes[slot].getTypeTag();
    if (type != JSVAL_TYPE_UNKNOWN &&
        (type != JSVAL_TYPE_DOUBLE || analysis->trackSlot(slot))) {
        FrameEntry *fe = frame.getSlotEntry(slot);
        JS_ASSERT_IF(fe->isTypeKnown(), fe->isType(type));
        if (!fe->isTypeKnown())
            frame.learnType(fe, type, false);
    }
}

JSValueType
mjit::Compiler::knownPushedType(uint32_t pushed)
{
    if (!cx->typeInferenceEnabled())
        return JSVAL_TYPE_UNKNOWN;
    types::StackTypeSet *types = analysis->pushedTypes(PC, pushed);
    return types->getKnownTypeTag();
}

bool
mjit::Compiler::mayPushUndefined(uint32_t pushed)
{
    JS_ASSERT(cx->typeInferenceEnabled());

    





    types::TypeSet *types = analysis->pushedTypes(PC, pushed);
    return types->hasType(types::Type::UndefinedType());
}

types::StackTypeSet *
mjit::Compiler::pushedTypeSet(uint32_t pushed)
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

    return analysis->typeBarriers(cx, pc) != NULL;
}

void
mjit::Compiler::pushSyncedEntry(uint32_t pushed)
{
    frame.pushSynced(knownPushedType(pushed));
}

JSObject *
mjit::Compiler::pushedSingleton(unsigned pushed)
{
    if (!cx->typeInferenceEnabled())
        return NULL;

    types::StackTypeSet *types = analysis->pushedTypes(PC, pushed);
    return types->getSingleton();
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
mjit::Compiler::trySingleTypeTest(types::StackTypeSet *types, RegisterID typeReg)
{
    





    MaybeJump res;

    switch (types->getKnownTypeTag()) {
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
mjit::Compiler::addTypeTest(types::StackTypeSet *types, RegisterID typeReg, RegisterID dataReg)
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

        for (unsigned i = 0; i < count; i++) {
            if (JSObject *object = types->getSingleObject(i))
                matches.append(masm.branchPtr(Assembler::Equal, dataReg, ImmPtr(object)));
        }

        for (unsigned i = 0; i < count; i++) {
            if (types::TypeObject *object = types->getTypeObject(i))
                matches.append(masm.branchPtr(Assembler::Equal, typeAddress, ImmPtr(object)));
        }

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

    types::StackTypeSet *types = analysis->bytecodeTypes(PC);
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

    if (hasTypeBarriers(PC))
        typeBarrierBytecodes.append(PC - script_->code);

    
    JS_ASSERT(!types->unknown());

    state.jump = trySingleTypeTest(types, typeReg);
    if (!state.jump.isSet())
        state.jump.setJump(addTypeTest(types, typeReg, dataReg));

    return state;
}

void
mjit::Compiler::finishBarrier(const BarrierState &barrier, RejoinState rejoin, uint32_t which)
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

    stubcc.masm.move(ImmIntPtr(intptr_t(which)), Registers::ArgReg1);
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
