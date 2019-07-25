







































#include "MethodJIT.h"
#include "jsnum.h"
#include "jsbool.h"
#include "jsiter.h"
#include "Compiler.h"
#include "StubCalls.h"
#include "MonoIC.h"
#include "PolyIC.h"
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

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;
#if defined JS_POLYIC
using namespace js::mjit::ic;
#endif

#define ADD_CALLSITE(stub) if (debugMode) addCallSite(__LINE__, (stub))

#define RETURN_IF_OOM(retval)                    \
    JS_BEGIN_MACRO                               \
        if (masm.oom() || stubcc.masm.oom()) {   \
            js_ReportOutOfMemory(cx);            \
            return retval;                       \
        }                                        \
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
    analysis(cx, script), jumpMap(NULL), frame(cx, script, masm),
    branchPatches(ContextAllocPolicy(cx)),
#if defined JS_MONOIC
    mics(ContextAllocPolicy(cx)),
    callICs(ContextAllocPolicy(cx)),
    equalityICs(ContextAllocPolicy(cx)),
    traceICs(ContextAllocPolicy(cx)),
#endif
#if defined JS_POLYIC
    pics(ContextAllocPolicy(cx)), 
#endif
    callPatches(ContextAllocPolicy(cx)),
    callSites(ContextAllocPolicy(cx)), 
    doubleList(ContextAllocPolicy(cx)),
    stubcc(cx, *this, frame, script),
    debugMode(cx->compartment->debugMode)
#if defined JS_TRACER
    ,addTraceHints(cx->traceJitEnabled)
#endif
{
}

CompileStatus
mjit::Compiler::compile()
{
    JS_ASSERT(!script->isEmpty());
    JS_ASSERT_IF(isConstructing, !script->jitCtor);
    JS_ASSERT_IF(!isConstructing, !script->jitNormal);

    JITScript **jit = isConstructing ? &script->jitCtor : &script->jitNormal;
    void **checkAddr = isConstructing
                       ? &script->jitArityCheckCtor
                       : &script->jitArityCheckNormal;

    CompileStatus status = performCompilation(jit);
    if (status == Compile_Okay) {
        
        
        
        
        *checkAddr = (*jit)->arityCheckEntry
                     ? (*jit)->arityCheckEntry
                     : (*jit)->invokeEntry;
    } else {
        *checkAddr = JS_UNJITTABLE_SCRIPT;
    }

    return status;
}

#define CHECK_STATUS(expr)              \
    JS_BEGIN_MACRO                      \
        CompileStatus status_ = (expr); \
        if (status_ != Compile_Okay)    \
            return status_;             \
    JS_END_MACRO

CompileStatus
mjit::Compiler::performCompilation(JITScript **jitp)
{
    JaegerSpew(JSpew_Scripts, "compiling script (file \"%s\") (line \"%d\") (length \"%d\")\n",
               script->filename, script->lineno, script->length);

    
    if (!analysis.analyze()) {
        if (analysis.OOM())
            return Compile_Error;
        JaegerSpew(JSpew_Abort, "couldn't analyze bytecode; probably switchX or OOM\n");
        return Compile_Abort;
    }

    uint32 nargs = fun ? fun->nargs : 0;
    if (!frame.init(nargs) || !stubcc.init(nargs))
        return Compile_Abort;

    jumpMap = (Label *)cx->malloc(sizeof(Label) * script->length);
    if (!jumpMap)
        return Compile_Error;
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
    script->debugMode = debugMode;
#endif

    for (uint32 i = 0; i < script->nClosedVars; i++)
        frame.setClosedVar(script->getClosedVar(i));

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

    return Compile_Okay;
}

#undef CHECK_STATUS

mjit::Compiler::~Compiler()
{
    cx->free(jumpMap);
}

CompileStatus JS_NEVER_INLINE
mjit::TryCompile(JSContext *cx, JSStackFrame *fp)
{
    JS_ASSERT(cx->fp() == fp);

    
    if (fp->isConstructing() && !fp->script()->nslots)
        fp->script()->nslots++;

    Compiler cc(cx, fp);

    return cc.compile();
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
            stubcc.crossJump(argMatch, fastPath);

            if (JSParamReg_Argc != Registers::ArgReg1)
                stubcc.masm.move(JSParamReg_Argc, Registers::ArgReg1);

            
            stubcc.masm.storePtr(ImmPtr(fun), Address(JSFrameReg, JSStackFrame::offsetOfExec()));
            stubcc.masm.storePtr(JSFrameReg, FrameAddress(offsetof(VMFrame, regs.fp)));
            stubcc.call(stubs::FixupArity);
            stubcc.masm.move(Registers::ReturnReg, JSFrameReg);
            stubcc.crossJump(stubcc.masm.jump(), fastPath);
        }

        



        masm.addPtr(Imm32((script->nslots + VALUES_PER_STACK_FRAME * 2) * sizeof(Value)),
                    JSFrameReg,
                    Registers::ReturnReg);
        Jump stackCheck = masm.branchPtr(Assembler::AboveOrEqual, Registers::ReturnReg,
                                         FrameAddress(offsetof(VMFrame, stackLimit)));

        
        {
            stubcc.linkExitDirect(stackCheck, stubcc.masm.label());
            stubcc.call(stubs::HitStackQuota);
            stubcc.crossJump(stubcc.masm.jump(), masm.label());
        }

        
        for (uint32 i = 0; i < script->nfixed; i++) {
            Address local(JSFrameReg, sizeof(JSStackFrame) + i * sizeof(Value));
            masm.storeValue(UndefinedValue(), local);
        }

        
        if (fun->isHeavyweight()) {
            prepareStubCall(Uses(0));
            stubCall(stubs::GetCallObject);
        }

        j.linkTo(masm.label(), &masm);

        if (analysis.usesScopeChain() && !fun->isHeavyweight()) {
            




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

    if (debugMode)
        stubCall(stubs::EnterScript);

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
                       doubleList.length() * sizeof(double);

    JSC::ExecutablePool *execPool = getExecPool(totalSize);
    if (!execPool)
        return Compile_Abort;

    uint8 *result = (uint8 *)execPool->alloc(totalSize);
    JSC::ExecutableAllocator::makeWritable(result, totalSize);
    masm.executableCopy(result);
    stubcc.masm.executableCopy(result + masm.size());
    
    JSC::LinkBuffer fullCode(result, totalSize);
    JSC::LinkBuffer stubCode(result + masm.size(), stubcc.size());

    size_t totalBytes = sizeof(JITScript) +
                        sizeof(void *) * script->length +
#if defined JS_MONOIC
                        sizeof(ic::MICInfo) * mics.length() +
                        sizeof(ic::CallICInfo) * callICs.length() +
                        sizeof(ic::EqualityICInfo) * equalityICs.length() +
                        sizeof(ic::TraceICInfo) * traceICs.length() +
#endif
#if defined JS_POLYIC
                        sizeof(ic::PICInfo) * pics.length() +
#endif
                        sizeof(CallSite) * callSites.length();

    uint8 *cursor = (uint8 *)cx->calloc(totalBytes);
    if (!cursor) {
        execPool->release();
        return Compile_Error;
    }

    JITScript *jit = (JITScript *)cursor;
    cursor += sizeof(JITScript);

    jit->code = JSC::MacroAssemblerCodeRef(result, execPool, masm.size() + stubcc.size());
    jit->nCallSites = callSites.length();
    jit->invokeEntry = result;

    
    void **nmap = (void **)cursor;
    cursor += sizeof(void *) * script->length;

    for (size_t i = 0; i < script->length; i++) {
        Label L = jumpMap[i];
        if (analysis[i].safePoint) {
            JS_ASSERT(L.isValid());
            nmap[i] = (uint8 *)(result + masm.distanceOf(L));
        }
    }

    if (fun) {
        jit->arityCheckEntry = stubCode.locationOf(arityLabel).executableAddress();
        jit->fastEntry = fullCode.locationOf(invokeLabel).executableAddress();
    }

#if defined JS_MONOIC
    jit->nMICs = mics.length();
    if (mics.length()) {
        jit->mics = (ic::MICInfo *)cursor;
        cursor += sizeof(ic::MICInfo) * mics.length();
    } else {
        jit->mics = NULL;
    }

    if (ic::MICInfo *scriptMICs = jit->mics) {
        for (size_t i = 0; i < mics.length(); i++) {
            scriptMICs[i].kind = mics[i].kind;
            scriptMICs[i].entry = fullCode.locationOf(mics[i].entry);
            switch (mics[i].kind) {
              case ic::MICInfo::GET:
              case ic::MICInfo::SET:
                scriptMICs[i].load = fullCode.locationOf(mics[i].load);
                scriptMICs[i].shape = fullCode.locationOf(mics[i].shape);
                scriptMICs[i].stubCall = stubCode.locationOf(mics[i].call);
                scriptMICs[i].stubEntry = stubCode.locationOf(mics[i].stubEntry);
                scriptMICs[i].u.name.typeConst = mics[i].u.name.typeConst;
                scriptMICs[i].u.name.dataConst = mics[i].u.name.dataConst;
#if defined JS_PUNBOX64
                scriptMICs[i].patchValueOffset = mics[i].patchValueOffset;
#endif
                break;
              default:
                JS_NOT_REACHED("Bad MIC kind");
            }
            stubCode.patch(mics[i].addrLabel, &scriptMICs[i]);
        }
    }

    jit->nCallICs = callICs.length();
    if (callICs.length()) {
        jit->callICs = (ic::CallICInfo *)cursor;
        cursor += sizeof(ic::CallICInfo) * callICs.length();
    } else {
        jit->callICs = NULL;
    }

    if (ic::CallICInfo *cics = jit->callICs) {
        for (size_t i = 0; i < callICs.length(); i++) {
            cics[i].reset();
            cics[i].funGuard = fullCode.locationOf(callICs[i].funGuard);
            cics[i].funJump = fullCode.locationOf(callICs[i].funJump);
            cics[i].slowPathStart = stubCode.locationOf(callICs[i].slowPathStart);

            
            uint32 offset = fullCode.locationOf(callICs[i].hotJump) -
                            fullCode.locationOf(callICs[i].funGuard);
            cics[i].hotJumpOffset = offset;
            JS_ASSERT(cics[i].hotJumpOffset == offset);

            
            offset = fullCode.locationOf(callICs[i].joinPoint) -
                     fullCode.locationOf(callICs[i].funGuard);
            cics[i].joinPointOffset = offset;
            JS_ASSERT(cics[i].joinPointOffset == offset);
                                            
            
            offset = stubCode.locationOf(callICs[i].oolCall) -
                     stubCode.locationOf(callICs[i].slowPathStart);
            cics[i].oolCallOffset = offset;
            JS_ASSERT(cics[i].oolCallOffset == offset);

            
            offset = stubCode.locationOf(callICs[i].oolJump) -
                     stubCode.locationOf(callICs[i].slowPathStart);
            cics[i].oolJumpOffset = offset;
            JS_ASSERT(cics[i].oolJumpOffset == offset);

            
            offset = stubCode.locationOf(callICs[i].slowJoinPoint) -
                     stubCode.locationOf(callICs[i].slowPathStart);
            cics[i].slowJoinOffset = offset;
            JS_ASSERT(cics[i].slowJoinOffset == offset);

            
            offset = stubCode.locationOf(callICs[i].hotPathLabel) -
                     stubCode.locationOf(callICs[i].funGuard);
            cics[i].hotPathOffset = offset;
            JS_ASSERT(cics[i].hotPathOffset == offset);

            cics[i].pc = callICs[i].pc;
            cics[i].argc = callICs[i].argc;
            cics[i].funObjReg = callICs[i].funObjReg;
            cics[i].funPtrReg = callICs[i].funPtrReg;
            cics[i].frameDepth = callICs[i].frameDepth;
            stubCode.patch(callICs[i].addrLabel1, &cics[i]);
            stubCode.patch(callICs[i].addrLabel2, &cics[i]);
        } 
    }

    jit->nEqualityICs = equalityICs.length();
    if (equalityICs.length()) {
        jit->equalityICs = (ic::EqualityICInfo *)cursor;
        cursor += sizeof(ic::EqualityICInfo) * equalityICs.length();
    } else {
        jit->equalityICs = NULL;
    }

    if (ic::EqualityICInfo *scriptEICs = jit->equalityICs) {
        for (size_t i = 0; i < equalityICs.length(); i++) {
            uint32 offs = uint32(equalityICs[i].jumpTarget - script->code);
            JS_ASSERT(jumpMap[offs].isValid());
            scriptEICs[i].target = fullCode.locationOf(jumpMap[offs]);
            scriptEICs[i].stubEntry = stubCode.locationOf(equalityICs[i].stubEntry);
            scriptEICs[i].stubCall = stubCode.locationOf(equalityICs[i].stubCall);
            scriptEICs[i].stub = equalityICs[i].stub;
            scriptEICs[i].lvr = equalityICs[i].lvr;
            scriptEICs[i].rvr = equalityICs[i].rvr;
            scriptEICs[i].tempReg = equalityICs[i].tempReg;
            scriptEICs[i].cond = equalityICs[i].cond;
            if (equalityICs[i].jumpToStub.isSet())
                scriptEICs[i].jumpToStub = fullCode.locationOf(equalityICs[i].jumpToStub.get());
            scriptEICs[i].fallThrough = fullCode.locationOf(equalityICs[i].fallThrough);
            
            stubCode.patch(equalityICs[i].addrLabel, &scriptEICs[i]);
        }
    }

    jit->nTraceICs = traceICs.length();
    if (traceICs.length()) {
        jit->traceICs = (ic::TraceICInfo *)cursor;
        cursor += sizeof(ic::TraceICInfo) * traceICs.length();
    } else {
        jit->traceICs = NULL;
    }

    if (ic::TraceICInfo *scriptTICs = jit->traceICs) {
        for (size_t i = 0; i < traceICs.length(); i++) {
            if (!traceICs[i].initialized)
                continue;

            uint32 offs = uint32(traceICs[i].jumpTarget - script->code);
            JS_ASSERT(jumpMap[offs].isValid());
            scriptTICs[i].traceHint = fullCode.locationOf(traceICs[i].traceHint);
            scriptTICs[i].jumpTarget = fullCode.locationOf(jumpMap[offs]);
            scriptTICs[i].stubEntry = stubCode.locationOf(traceICs[i].stubEntry);
#ifdef DEBUG
            scriptTICs[i].jumpTargetPC = traceICs[i].jumpTarget;
#endif
            scriptTICs[i].hasSlowTraceHint = traceICs[i].slowTraceHint.isSet();
            if (traceICs[i].slowTraceHint.isSet())
                scriptTICs[i].slowTraceHint = stubCode.locationOf(traceICs[i].slowTraceHint.get());
            
            stubCode.patch(traceICs[i].addrLabel, &scriptTICs[i]);
        }
    }
#endif 

    for (size_t i = 0; i < callPatches.length(); i++) {
        CallPatchInfo &patch = callPatches[i];

        fullCode.patch(patch.fastNcodePatch, fullCode.locationOf(patch.joinPoint));
        if (patch.hasSlowNcode)
            stubCode.patch(patch.slowNcodePatch, fullCode.locationOf(patch.joinPoint));
    }

#if defined JS_POLYIC
    jit->nPICs = pics.length();
    if (pics.length()) {
        jit->pics = (ic::PICInfo *)cursor;
        cursor += sizeof(ic::PICInfo) * pics.length();
    } else {
        jit->pics = NULL;
    }

    if (ic::PICInfo *scriptPICs = jit->pics) {
        for (size_t i = 0; i < pics.length(); i++) {
            pics[i].copySimpleMembersTo(scriptPICs[i]);
            scriptPICs[i].fastPathStart = fullCode.locationOf(pics[i].fastPathStart);
            scriptPICs[i].storeBack = fullCode.locationOf(pics[i].storeBack);
            scriptPICs[i].slowPathStart = stubCode.locationOf(pics[i].slowPathStart);
            scriptPICs[i].callReturn = uint16((uint8*)stubCode.locationOf(pics[i].callReturn).executableAddress() -
                                               (uint8*)scriptPICs[i].slowPathStart.executableAddress());
            scriptPICs[i].shapeGuard = masm.distanceOf(pics[i].shapeGuard) -
                                         masm.distanceOf(pics[i].fastPathStart);
            JS_ASSERT(scriptPICs[i].shapeGuard == masm.distanceOf(pics[i].shapeGuard) -
                                         masm.distanceOf(pics[i].fastPathStart));
            scriptPICs[i].shapeRegHasBaseShape = true;

# if defined JS_CPU_X64
            memcpy(&scriptPICs[i].labels, &pics[i].labels, sizeof(PICLabels));
# endif

            if (pics[i].kind == ic::PICInfo::SET ||
                pics[i].kind == ic::PICInfo::SETMETHOD) {
                scriptPICs[i].u.vr = pics[i].vr;
            } else if (pics[i].kind != ic::PICInfo::NAME) {
                if (pics[i].hasTypeCheck) {
                    int32 distance = stubcc.masm.distanceOf(pics[i].typeCheck) -
                                     stubcc.masm.distanceOf(pics[i].slowPathStart);
                    JS_ASSERT(distance <= 0);
                    scriptPICs[i].u.get.typeCheckOffset = distance;
                }
            }
            new (&scriptPICs[i].execPools) ic::PICInfo::ExecPoolVector(SystemAllocPolicy());
            scriptPICs[i].reset();
            stubCode.patch(pics[i].addrLabel, &scriptPICs[i]);
        }
    }
#endif 

    
    stubcc.fixCrossJumps(result, masm.size(), masm.size() + stubcc.size());

    
    size_t doubleOffset = masm.size() + stubcc.size();
    double *doubleVec = (double *)(result + doubleOffset);
    for (size_t i = 0; i < doubleList.length(); i++) {
        DoublePatch &patch = doubleList[i];
        doubleVec[i] = patch.d;
        if (patch.ool)
            stubCode.patch(patch.label, &doubleVec[i]);
        else
            fullCode.patch(patch.label, &doubleVec[i]);
    }

    
    masm.finalize(fullCode);
    stubcc.masm.finalize(stubCode);

    JSC::ExecutableAllocator::makeExecutable(result, masm.size() + stubcc.size());
    JSC::ExecutableAllocator::cacheFlush(result, masm.size() + stubcc.size());

    
    jit->nCallSites = callSites.length();
    if (callSites.length()) {
        CallSite *callSiteList = (CallSite *)cursor;
        cursor += sizeof(CallSite) * callSites.length();

        for (size_t i = 0; i < callSites.length(); i++) {
            if (callSites[i].stub)
                callSiteList[i].codeOffset = masm.size() + stubcc.masm.distanceOf(callSites[i].location);
            else
                callSiteList[i].codeOffset = masm.distanceOf(callSites[i].location);
            callSiteList[i].pcOffset = callSites[i].pc - script->code;
            callSiteList[i].id = callSites[i].id;
        }
        jit->callSites = callSiteList;
    } else {
        jit->callSites = NULL;
    }

    JS_ASSERT(size_t(cursor - (uint8*)jit) == totalBytes);

    jit->nmap = nmap;
    *jitp = jit;

    return Compile_Okay;
}

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

    for (;;) {
        JSOp op = JSOp(*PC);

        OpcodeStatus &opinfo = analysis[PC];
        frame.setInTryBlock(opinfo.inTryBlock);
        if (opinfo.nincoming || opinfo.trap) {
            frame.syncAndForgetEverything(opinfo.stackDepth);
            opinfo.safePoint = true;
        }
        jumpMap[uint32(PC - script->code)] = masm.label();

        if (opinfo.trap) {
            if (!trapper.untrap(PC))
                return Compile_Error;
            op = JSOp(*PC);
        }

        if (!opinfo.visited) {
            if (op == JSOP_STOP)
                break;
            if (js_CodeSpec[op].length != -1)
                PC += js_CodeSpec[op].length;
            else
                PC += js_GetVariableBytecodeLength(PC);
            continue;
        }

        SPEW_OPCODE();
        JS_ASSERT(frame.stackDepth() == opinfo.stackDepth);

        if (opinfo.trap) {
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(PC), Registers::ArgReg1);
            stubCall(stubs::Trap);
        }
#if defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)
        
        
        
        else {
            masm.subPtr(Imm32(8), Registers::StackPointer);
            masm.callLabel = masm.label();
            masm.addPtr(Imm32(8), Registers::StackPointer);
        }
#elif defined(_WIN64)
        
        else {
            masm.subPtr(Imm32(32), Registers::StackPointer);
            masm.callLabel = masm.label();
            masm.addPtr(Imm32(32), Registers::StackPointer);
        }
#endif
        ADD_CALLSITE(false);

    

 

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
          END_CASE(JSOP_RETURN)

          BEGIN_CASE(JSOP_GOTO)
          {
            
            frame.syncAndForgetEverything();
            Jump j = masm.jump();
            jumpAndTrace(j, PC + GET_JUMP_OFFSET(PC));
          }
          END_CASE(JSOP_GOTO)

          BEGIN_CASE(JSOP_IFEQ)
          BEGIN_CASE(JSOP_IFNE)
            jsop_ifneq(op, PC + GET_JUMP_OFFSET(PC));
          END_CASE(JSOP_IFNE)

          BEGIN_CASE(JSOP_ARGUMENTS)
            prepareStubCall(Uses(0));
            stubCall(stubs::Arguments);
            frame.pushSynced();
          END_CASE(JSOP_ARGUMENTS)

          BEGIN_CASE(JSOP_FORLOCAL)
            iterNext();
            frame.storeLocal(GET_SLOTNO(PC), true);
            frame.pop();
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
            if ((fused != JSOP_IFEQ && fused != JSOP_IFNE) || analysis[next].nincoming)
                fused = JSOP_NOP;

            
            jsbytecode *target = NULL;
            if (fused != JSOP_NOP)
                target = next + GET_JUMP_OFFSET(next);

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
                            frame.syncAndForgetEverything();
                            Jump j = masm.jump();
                            jumpAndTrace(j, target);
                        }
                    }
                } else {
                    emitStubCmpOp(stub, target, fused);
                }
            } else {
                
                jsop_relational(op, stub, target, fused);
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
            jsop_binary(op, stubs::Add);
          END_CASE(JSOP_ADD)

          BEGIN_CASE(JSOP_SUB)
            jsop_binary(op, stubs::Sub);
          END_CASE(JSOP_SUB)

          BEGIN_CASE(JSOP_MUL)
            jsop_binary(op, stubs::Mul);
          END_CASE(JSOP_MUL)

          BEGIN_CASE(JSOP_DIV)
            jsop_binary(op, stubs::Div);
          END_CASE(JSOP_DIV)

          BEGIN_CASE(JSOP_MOD)
            jsop_mod();
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
                frame.pop();
                frame.push(NumberValue(d));
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
            stubCall(stubs::DelName);
            frame.pushSynced();
          }
          END_CASE(JSOP_DELNAME)

          BEGIN_CASE(JSOP_DELPROP)
          {
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(1));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            stubCall(STRICT_VARIANT(stubs::DelProp));
            frame.pop();
            frame.pushSynced();
          }
          END_CASE(JSOP_DELPROP) 

          BEGIN_CASE(JSOP_DELELEM)
            prepareStubCall(Uses(2));
            stubCall(STRICT_VARIANT(stubs::DelElem));
            frame.popn(2);
            frame.pushSynced();
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
            if (!jsop_getprop(script->getAtom(fullAtomIndex(PC))))
                return Compile_Error;
          END_CASE(JSOP_GETTHISPROP);

          BEGIN_CASE(JSOP_GETARGPROP)
            
            jsop_getarg(GET_SLOTNO(PC));
            if (!jsop_getprop(script->getAtom(fullAtomIndex(&PC[ARGNO_LEN]))))
                return Compile_Error;
          END_CASE(JSOP_GETARGPROP)

          BEGIN_CASE(JSOP_GETLOCALPROP)
            frame.pushLocal(GET_SLOTNO(PC));
            if (!jsop_getprop(script->getAtom(fullAtomIndex(&PC[SLOTNO_LEN]))))
                return Compile_Error;
          END_CASE(JSOP_GETLOCALPROP)

          BEGIN_CASE(JSOP_GETPROP)
            if (!jsop_getprop(script->getAtom(fullAtomIndex(PC))))
                return Compile_Error;
          END_CASE(JSOP_GETPROP)

          BEGIN_CASE(JSOP_LENGTH)
            if (!jsop_length())
                return Compile_Error;
          END_CASE(JSOP_LENGTH)

          BEGIN_CASE(JSOP_GETELEM)
            if (!jsop_getelem())
                return Compile_Error;
          END_CASE(JSOP_GETELEM)

          BEGIN_CASE(JSOP_SETELEM)
            jsop_setelem();
          END_CASE(JSOP_SETELEM);

          BEGIN_CASE(JSOP_CALLNAME)
            prepareStubCall(Uses(0));
            masm.move(Imm32(fullAtomIndex(PC)), Registers::ArgReg1);
            stubCall(stubs::CallName);
            frame.pushSynced();
            frame.pushSynced();
          END_CASE(JSOP_CALLNAME)

          BEGIN_CASE(JSOP_CALL)
          BEGIN_CASE(JSOP_EVAL)
          BEGIN_CASE(JSOP_APPLY)
          {
            JaegerSpew(JSpew_Insns, " --- SCRIPTED CALL --- \n");
            inlineCallHelper(GET_ARGC(PC), false);
            JaegerSpew(JSpew_Insns, " --- END SCRIPTED CALL --- \n");
          }
          END_CASE(JSOP_CALL)

          BEGIN_CASE(JSOP_NAME)
            jsop_name(script->getAtom(fullAtomIndex(PC)));
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
            jsop_andor(op, PC + GET_JUMP_OFFSET(PC));
          END_CASE(JSOP_AND)

          BEGIN_CASE(JSOP_TABLESWITCH)
            frame.syncAndForgetEverything();
            masm.move(ImmPtr(PC), Registers::ArgReg1);

            
            stubCall(stubs::TableSwitch);
            frame.pop();

            masm.jump(Registers::ReturnReg);
            PC += js_GetVariableBytecodeLength(PC);
            break;
          END_CASE(JSOP_TABLESWITCH)

          BEGIN_CASE(JSOP_LOOKUPSWITCH)
            frame.syncAndForgetEverything();
            masm.move(ImmPtr(PC), Registers::ArgReg1);

            
            stubCall(stubs::LookupSwitch);
            frame.pop();

            masm.jump(Registers::ReturnReg);
            PC += js_GetVariableBytecodeLength(PC);
            break;
          END_CASE(JSOP_LOOKUPSWITCH)

          BEGIN_CASE(JSOP_STRICTEQ)
            jsop_stricteq(op);
          END_CASE(JSOP_STRICTEQ)

          BEGIN_CASE(JSOP_STRICTNE)
            jsop_stricteq(op);
          END_CASE(JSOP_STRICTNE)

          BEGIN_CASE(JSOP_ITER)
# if defined JS_CPU_X64
            prepareStubCall(Uses(1));
            masm.move(Imm32(PC[1]), Registers::ArgReg1);
            stubCall(stubs::Iter);
            frame.pop();
            frame.pushSynced();
#else
            iter(PC[1]);
#endif
          END_CASE(JSOP_ITER)

          BEGIN_CASE(JSOP_MOREITER)
            
            iterMore();
            break;
          END_CASE(JSOP_MOREITER)

          BEGIN_CASE(JSOP_ENDITER)
# if defined JS_CPU_X64
            prepareStubCall(Uses(1));
            stubCall(stubs::EndIter);
            frame.pop();
#else
            iterEnd();
#endif
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
            jsop_getarg(GET_SLOTNO(PC));
            if (op == JSOP_CALLARG)
                frame.push(UndefinedValue());
          }
          END_CASE(JSOP_GETARG)

          BEGIN_CASE(JSOP_BINDGNAME)
            jsop_bindgname();
          END_CASE(JSOP_BINDGNAME)

          BEGIN_CASE(JSOP_SETARG)
          {
            uint32 slot = GET_SLOTNO(PC);
            FrameEntry *top = frame.peek(-1);

            bool popped = PC[JSOP_SETARG_LENGTH] == JSOP_POP;

            RegisterID reg = frame.allocReg();
            Address address = Address(JSFrameReg, JSStackFrame::offsetOfFormalArg(fun, slot));
            frame.storeTo(top, address, popped);
            frame.freeReg(reg);
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
            jsbytecode *next = &PC[JSOP_SETLOCAL_LENGTH];
            bool pop = JSOp(*next) == JSOP_POP && !analysis[next].nincoming;
            frame.storeLocal(GET_SLOTNO(PC), pop);
            if (pop) {
                frame.pop();
                PC += JSOP_SETLOCAL_LENGTH + JSOP_POP_LENGTH;
                break;
            }
          }
          END_CASE(JSOP_SETLOCAL)

          BEGIN_CASE(JSOP_SETLOCALPOP)
            frame.storeLocal(GET_SLOTNO(PC), true);
            frame.pop();
          END_CASE(JSOP_SETLOCALPOP)

          BEGIN_CASE(JSOP_UINT16)
            frame.push(Value(Int32Value((int32_t) GET_UINT16(PC))));
          END_CASE(JSOP_UINT16)

          BEGIN_CASE(JSOP_NEWINIT)
          {
            jsint i = GET_UINT16(PC);
            uint32 count = GET_UINT16(PC + UINT16_LEN);

            JS_ASSERT(i == JSProto_Array || i == JSProto_Object);

            prepareStubCall(Uses(0));
            masm.move(Imm32(count), Registers::ArgReg1);
            if (i == JSProto_Array)
                stubCall(stubs::NewInitArray);
            else
                stubCall(stubs::NewInitObject);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
          }
          END_CASE(JSOP_NEWINIT)

          BEGIN_CASE(JSOP_ENDINIT)
          END_CASE(JSOP_ENDINIT)

          BEGIN_CASE(JSOP_INITPROP)
          {
            JSAtom *atom = script->getAtom(fullAtomIndex(PC));
            prepareStubCall(Uses(2));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            stubCall(stubs::InitProp);
            frame.pop();
          }
          END_CASE(JSOP_INITPROP)

          BEGIN_CASE(JSOP_INITELEM)
          {
            JSOp next = JSOp(PC[JSOP_INITELEM_LENGTH]);
            prepareStubCall(Uses(3));
            masm.move(Imm32(next == JSOP_ENDINIT ? 1 : 0), Registers::ArgReg1);
            stubCall(stubs::InitElem);
            frame.popn(2);
          }
          END_CASE(JSOP_INITELEM)

          BEGIN_CASE(JSOP_INCARG)
          BEGIN_CASE(JSOP_DECARG)
          BEGIN_CASE(JSOP_ARGINC)
          BEGIN_CASE(JSOP_ARGDEC)
          {
            jsbytecode *next = &PC[JSOP_ARGINC_LENGTH];
            bool popped = false;
            if (JSOp(*next) == JSOP_POP && !analysis[next].nincoming)
                popped = true;
            jsop_arginc(op, GET_SLOTNO(PC), popped);
            PC += JSOP_ARGINC_LENGTH;
            if (popped)
                PC += JSOP_POP_LENGTH;
            break;
          }
          END_CASE(JSOP_ARGDEC)

          BEGIN_CASE(JSOP_FORNAME)
            prepareStubCall(Uses(1));
            masm.move(ImmPtr(script->getAtom(fullAtomIndex(PC))), Registers::ArgReg1);
            stubCall(STRICT_VARIANT(stubs::ForName));
          END_CASE(JSOP_FORNAME)

          BEGIN_CASE(JSOP_INCLOCAL)
          BEGIN_CASE(JSOP_DECLOCAL)
          BEGIN_CASE(JSOP_LOCALINC)
          BEGIN_CASE(JSOP_LOCALDEC)
          {
            jsbytecode *next = &PC[JSOP_LOCALINC_LENGTH];
            bool popped = false;
            if (JSOp(*next) == JSOP_POP && !analysis[next].nincoming)
                popped = true;
            
            jsop_localinc(op, GET_SLOTNO(PC), popped);
            PC += JSOP_LOCALINC_LENGTH;
            if (popped)
                PC += JSOP_POP_LENGTH;
            break;
          }
          END_CASE(JSOP_LOCALDEC)

          BEGIN_CASE(JSOP_BINDNAME)
            jsop_bindname(fullAtomIndex(PC), true);
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
            stubCall(stubs::Throw);
            frame.pop();
          END_CASE(JSOP_THROW)

          BEGIN_CASE(JSOP_IN)
            prepareStubCall(Uses(2));
            stubCall(stubs::In);
            frame.popn(2);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
          END_CASE(JSOP_IN)

          BEGIN_CASE(JSOP_INSTANCEOF)
            if (!jsop_instanceof())
                return Compile_Error;
          END_CASE(JSOP_INSTANCEOF)

          BEGIN_CASE(JSOP_EXCEPTION)
          {
            JS_STATIC_ASSERT(sizeof(cx->throwing) == 4);
            RegisterID reg = frame.allocReg();
            masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), reg);
            masm.store32(Imm32(JS_FALSE), Address(reg, offsetof(JSContext, throwing)));

            Address excn(reg, offsetof(JSContext, exception));
            frame.freeReg(reg);
            frame.push(excn);
          }
          END_CASE(JSOP_EXCEPTION)

          BEGIN_CASE(JSOP_LINENO)
          END_CASE(JSOP_LINENO)

          BEGIN_CASE(JSOP_BLOCKCHAIN)
          END_CASE(JSOP_BLOCKCHAIN)

          BEGIN_CASE(JSOP_NULLBLOCKCHAIN)
          END_CASE(JSOP_NULLBLOCKCHAIN)

          BEGIN_CASE(JSOP_CONDSWITCH)
            
          END_CASE(JSOP_CONDSWITCH)

          BEGIN_CASE(JSOP_DEFFUN)
          {
            uint32 index = fullAtomIndex(PC);
            JSFunction *inner = script->getFunction(index);

            if (fun) {
                JSLocalKind localKind = fun->lookupLocal(cx, inner->atom, NULL);
                if (localKind != JSLOCAL_NONE)
                    frame.syncAndForgetEverything();
            }

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(inner), Registers::ArgReg1);
            stubCall(STRICT_VARIANT(stubs::DefFun));
          }
          END_CASE(JSOP_DEFFUN)

          BEGIN_CASE(JSOP_DEFVAR)
          {
            uint32 index = fullAtomIndex(PC);
            JSAtom *atom = script->getAtom(index);

            prepareStubCall(Uses(0));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            stubCall(stubs::DefVar);
          }
          END_CASE(JSOP_DEFVAR)

          BEGIN_CASE(JSOP_DEFLOCALFUN_FC)
          {
            uint32 slot = GET_SLOTNO(PC);
            JSFunction *fun = script->getFunction(fullAtomIndex(&PC[SLOTNO_LEN]));
            prepareStubCall(Uses(frame.frameDepth()));
            masm.move(ImmPtr(fun), Registers::ArgReg1);
            stubCall(stubs::DefLocalFun_FC);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
            frame.storeLocal(slot, true);
            frame.pop();
          }
          END_CASE(JSOP_DEFLOCALFUN_FC)

          BEGIN_CASE(JSOP_LAMBDA)
          {
            JSFunction *fun = script->getFunction(fullAtomIndex(PC));

            JSObjStubFun stub = stubs::Lambda;
            uint32 uses = 0;

            jsbytecode *pc2 = js_AdvanceOverBlockchain(PC + JSOP_LAMBDA_LENGTH);
            JSOp next = JSOp(*pc2);
            
            if (next == JSOP_INITMETHOD) {
                stub = stubs::LambdaForInit;
            } else if (next == JSOP_SETMETHOD) {
                stub = stubs::LambdaForSet;
                uses = 1;
            } else if (fun->joinable()) {
                if (next == JSOP_CALL) {
                    stub = stubs::LambdaJoinableForCall;
                    uses = frame.frameDepth();
                } else if (next == JSOP_NULL) {
                    stub = stubs::LambdaJoinableForNull;
                }
            }

            prepareStubCall(Uses(uses));
            masm.move(ImmPtr(fun), Registers::ArgReg1);

            if (stub == stubs::Lambda) {
                stubCall(stub);
            } else {
                jsbytecode *savedPC = PC;
                PC = pc2;
                stubCall(stub);
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
            
            RegisterID reg = frame.allocReg();
            masm.loadPayload(Address(JSFrameReg, JSStackFrame::offsetOfCallee(fun)), reg);
            
            masm.loadPtr(Address(reg, offsetof(JSObject, slots)), reg);
            Address upvarAddress(reg, JSObject::JSSLOT_FLAT_CLOSURE_UPVARS * sizeof(Value));
            masm.loadPrivate(upvarAddress, reg);
            
            frame.freeReg(reg);
            frame.push(Address(reg, index * sizeof(Value)));
            if (op == JSOP_CALLFCSLOT)
                frame.push(UndefinedValue());
          }
          END_CASE(JSOP_CALLFCSLOT)

          BEGIN_CASE(JSOP_ARGSUB)
            prepareStubCall(Uses(0));
            masm.move(Imm32(GET_ARGNO(PC)), Registers::ArgReg1);
            stubCall(stubs::ArgSub);
            frame.pushSynced();
          END_CASE(JSOP_ARGSUB)

          BEGIN_CASE(JSOP_ARGCNT)
            prepareStubCall(Uses(0));
            stubCall(stubs::ArgCnt);
            frame.pushSynced();
          END_CASE(JSOP_ARGCNT)

          BEGIN_CASE(JSOP_DEFLOCALFUN)
          {
            uint32 slot = GET_SLOTNO(PC);
            JSFunction *fun = script->getFunction(fullAtomIndex(&PC[SLOTNO_LEN]));
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(fun), Registers::ArgReg1);
            stubCall(stubs::DefLocalFun);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
            frame.storeLocal(slot, true);
            frame.pop();
          }
          END_CASE(JSOP_DEFLOCALFUN)

          BEGIN_CASE(JSOP_RETRVAL)
            emitReturn(NULL);
          END_CASE(JSOP_RETRVAL)

          BEGIN_CASE(JSOP_GETGNAME)
          BEGIN_CASE(JSOP_CALLGNAME)
            jsop_getgname(fullAtomIndex(PC));
            if (op == JSOP_CALLGNAME)
                frame.push(UndefinedValue());
          END_CASE(JSOP_GETGNAME)

          BEGIN_CASE(JSOP_SETGNAME)
            jsop_setgname(fullAtomIndex(PC));
          END_CASE(JSOP_SETGNAME)

          BEGIN_CASE(JSOP_REGEXP)
          {
            JSObject *regex = script->getRegExp(fullAtomIndex(PC));
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(regex), Registers::ArgReg1);
            stubCall(stubs::RegExp);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
          }
          END_CASE(JSOP_REGEXP)

          BEGIN_CASE(JSOP_CALLPROP)
            if (!jsop_callprop(script->getAtom(fullAtomIndex(PC))))
                return Compile_Error;
          END_CASE(JSOP_CALLPROP)

          BEGIN_CASE(JSOP_GETUPVAR)
          BEGIN_CASE(JSOP_CALLUPVAR)
          {
            uint32 index = GET_UINT16(PC);
            JSUpvarArray *uva = script->upvars();
            JS_ASSERT(index < uva->length);

            prepareStubCall(Uses(0));
            masm.move(Imm32(uva->vector[index].asInteger()), Registers::ArgReg1);
            stubCall(stubs::GetUpvar);
            frame.pushSynced();
            if (op == JSOP_CALLUPVAR)
                frame.push(UndefinedValue());
          }
          END_CASE(JSOP_CALLUPVAR)

          BEGIN_CASE(JSOP_UINT24)
            frame.push(Value(Int32Value((int32_t) GET_UINT24(PC))));
          END_CASE(JSOP_UINT24)

          BEGIN_CASE(JSOP_CALLELEM)
            prepareStubCall(Uses(2));
            stubCall(stubs::CallElem);
            frame.popn(2);
            frame.pushSynced();
            frame.pushSynced();
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
            frame.pushLocal(GET_SLOTNO(PC));
            frame.push(UndefinedValue());
          END_CASE(JSOP_CALLLOCAL)

          BEGIN_CASE(JSOP_INT8)
            frame.push(Value(Int32Value(GET_INT8(PC))));
          END_CASE(JSOP_INT8)

          BEGIN_CASE(JSOP_INT32)
            frame.push(Value(Int32Value(GET_INT32(PC))));
          END_CASE(JSOP_INT32)

          BEGIN_CASE(JSOP_NEWARRAY)
          {
            uint32 len = GET_UINT16(PC);
            prepareStubCall(Uses(len));
            masm.move(Imm32(len), Registers::ArgReg1);
            stubCall(stubs::NewArray);
            frame.popn(len);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
          }
          END_CASE(JSOP_NEWARRAY)

          BEGIN_CASE(JSOP_HOLE)
            frame.push(MagicValue(JS_ARRAY_HOLE));
          END_CASE(JSOP_HOLE)

          BEGIN_CASE(JSOP_LAMBDA_FC)
          {
            JSFunction *fun = script->getFunction(fullAtomIndex(PC));
            prepareStubCall(Uses(frame.frameDepth()));
            masm.move(ImmPtr(fun), Registers::ArgReg1);
            stubCall(stubs::FlatLambda);
            frame.takeReg(Registers::ReturnReg);
            frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
          }
          END_CASE(JSOP_LAMBDA_FC)

          BEGIN_CASE(JSOP_TRACE)
          BEGIN_CASE(JSOP_NOTRACE)
          {
            if (analysis[PC].nincoming > 0)
                interruptCheckHelper();
          }
          END_CASE(JSOP_TRACE)

          BEGIN_CASE(JSOP_DEBUGGER)
            prepareStubCall(Uses(0));
            masm.move(ImmPtr(PC), Registers::ArgReg1);
            stubCall(stubs::Debugger);
          END_CASE(JSOP_DEBUGGER)

          BEGIN_CASE(JSOP_INITMETHOD)
          {
            JSAtom *atom = script->getAtom(fullAtomIndex(PC));
            prepareStubCall(Uses(2));
            masm.move(ImmPtr(atom), Registers::ArgReg1);
            stubCall(stubs::InitMethod);
            frame.pop();
          }
          END_CASE(JSOP_INITMETHOD)

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

          BEGIN_CASE(JSOP_SETGLOBAL)
            jsop_setglobal(GET_SLOTNO(PC));
          END_CASE(JSOP_SETGLOBAL)

          BEGIN_CASE(JSOP_INCGLOBAL)
          BEGIN_CASE(JSOP_DECGLOBAL)
          BEGIN_CASE(JSOP_GLOBALINC)
          BEGIN_CASE(JSOP_GLOBALDEC)
            
            jsop_globalinc(op, GET_SLOTNO(PC));
            break;
          END_CASE(JSOP_GLOBALINC)

          default:
           
#ifdef JS_METHODJIT_SPEW
            JaegerSpew(JSpew_Abort, "opcode %s not handled yet (%s line %d)\n", OpcodeNames[op],
                       script->filename, js_PCToLineNumber(cx, script, PC));
#endif
            return Compile_Abort;
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
        if (callSites[i].pc == script->code + callSite.pcOffset &&
            callSites[i].id == callSite.id) {
            if (callSites[i].stub) {
                return oolPath + stubcc.masm.distanceOf(callSites[i].location);
            }
            return ilPath + masm.distanceOf(callSites[i].location);
        }
    }

    
    JS_NOT_REACHED("Call site vanished.");
    return NULL;
}

void
mjit::Compiler::jumpInScript(Jump j, jsbytecode *pc)
{
    JS_ASSERT(pc >= script->code && uint32(pc - script->code) < script->length);

    

    if (pc < PC)
        j.linkTo(jumpMap[uint32(pc - script->code)], &masm);
    else
        branchPatches.append(BranchPatch(j, pc));
}

void
mjit::Compiler::jsop_setglobal(uint32 index)
{
    JS_ASSERT(globalObj);
    uint32 slot = script->getGlobalSlot(index);

    FrameEntry *fe = frame.peek(-1);
    bool popped = PC[JSOP_SETGLOBAL_LENGTH] == JSOP_POP;

    RegisterID reg = frame.allocReg();
    Address address = masm.objSlotRef(globalObj, reg, slot);
    frame.storeTo(fe, address, popped);
    frame.freeReg(reg);
}

void
mjit::Compiler::jsop_getglobal(uint32 index)
{
    JS_ASSERT(globalObj);
    uint32 slot = script->getGlobalSlot(index);

    RegisterID reg = frame.allocReg();
    Address address = masm.objSlotRef(globalObj, reg, slot);
    frame.freeReg(reg);
    frame.push(address);
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
                if (fe->isTypeKnown()) {
                    stubcc.masm.loadPayload(rval, dataReg);
                    stubcc.masm.move(ImmType(fe->getKnownType()), typeReg);
                } else {
                    stubcc.masm.loadValueAsComponents(rval, typeReg, dataReg);
                }
            }
        } else {
            frame.loadTo(fe, typeReg, dataReg, Registers::ReturnReg);
        }
    } else {
         
         
        masm->loadValueAsComponents(UndefinedValue(), typeReg, dataReg);
        if (analysis.usesReturnValue()) {
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

    Address thisv(JSFrameReg, JSStackFrame::offsetOfThis(fun));

    
    if (!fe || (fe->isTypeKnown() && fe->getKnownType() != JSVAL_TYPE_OBJECT)) {
        masm->loadValueAsComponents(thisv, JSReturnReg_Type, JSReturnReg_Data);
        return;
    }

    
    if (fe->isTypeKnown() && fe->getKnownType() == JSVAL_TYPE_OBJECT) {
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

    if (debugMode) {
        prepareStubCall(Uses(0));
        stubCall(stubs::LeaveScript);
    }

    








    if (fun) {
        if (fun->isHeavyweight()) {
            
            prepareStubCall(Uses(fe ? 1 : 0));
            stubCall(stubs::PutActivationObjects);

            if (fe) {
                emitReturnValue(&masm, fe);
                emitFinalReturn(masm);
                frame.discardFrame();
                return;
            }
        } else {
            
            Jump putObjs = masm.branchTest32(Assembler::NonZero,
                                             Address(JSFrameReg, JSStackFrame::offsetOfFlags()),
                                             Imm32(JSFRAME_HAS_CALL_OBJ | JSFRAME_HAS_ARGS_OBJ));
            stubcc.linkExit(putObjs, Uses(frame.frameDepth()));

            stubcc.leave();
            stubcc.call(stubs::PutActivationObjects);

            emitReturnValue(&stubcc.masm, fe);
            emitFinalReturn(stubcc.masm);
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
    frame.syncAndKill(Registers(Registers::TempRegs), uses);
    JaegerSpew(JSpew_Insns, " ---- FRAME SYNCING DONE ---- \n");
}

JSC::MacroAssembler::Call
mjit::Compiler::stubCall(void *ptr)
{
    JaegerSpew(JSpew_Insns, " ---- CALLING STUB ---- \n");
    Call cl = masm.stubCall(ptr, PC, frame.stackDepth() + script->nfixed);
    JaegerSpew(JSpew_Insns, " ---- END STUB CALL ---- \n");
    return cl;
}

void
mjit::Compiler::interruptCheckHelper()
{
    RegisterID cxreg = frame.allocReg();
    masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), cxreg);
#ifdef JS_THREADSAFE
    masm.loadPtr(Address(cxreg, offsetof(JSContext, thread)), cxreg);
    Address flag(cxreg, offsetof(JSThread, data.interruptFlags));
#else
    masm.loadPtr(Address(cxreg, offsetof(JSContext, runtime)), cxreg);
    Address flag(cxreg, offsetof(JSRuntime, threadData.interruptFlags));
#endif
    Jump jump = masm.branchTest32(Assembler::NonZero, flag);
    frame.freeReg(cxreg);
    stubcc.linkExit(jump, Uses(0));
    stubcc.leave();
    stubcc.masm.move(ImmPtr(PC), Registers::ArgReg1);
    stubcc.call(stubs::Interrupt);
    ADD_CALLSITE(true);
    stubcc.rejoin(Changes(0));
}

void
mjit::Compiler::emitUncachedCall(uint32 argc, bool callingNew)
{
    CallPatchInfo callPatch;
    callPatch.hasSlowNcode = false;

    RegisterID r0 = Registers::ReturnReg;
    VoidPtrStubUInt32 stub = callingNew ? stubs::UncachedNew : stubs::UncachedCall;

    frame.syncAndKill(Registers(Registers::AvailRegs), Uses(argc + 2));
    prepareStubCall(Uses(argc + 2));
    masm.move(Imm32(argc), Registers::ArgReg1);
    stubCall(stub);
    ADD_CALLSITE(false);

    Jump notCompiled = masm.branchTestPtr(Assembler::Zero, r0, r0);

    masm.loadPtr(FrameAddress(offsetof(VMFrame, regs.fp)), JSFrameReg);
    callPatch.fastNcodePatch =
        masm.storePtrWithPatch(ImmPtr(NULL),
                               Address(JSFrameReg, JSStackFrame::offsetOfncode()));

    masm.jump(r0);

#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
    masm.callLabel = masm.label();
#endif
    ADD_CALLSITE(false);

    callPatch.joinPoint = masm.label();
    masm.loadPtr(Address(JSFrameReg, JSStackFrame::offsetOfPrev()), JSFrameReg);

    frame.popn(argc + 2);
    frame.takeReg(JSReturnReg_Type);
    frame.takeReg(JSReturnReg_Data);
    frame.pushRegs(JSReturnReg_Type, JSReturnReg_Data);

    stubcc.linkExitDirect(notCompiled, stubcc.masm.label());
    stubcc.rejoin(Changes(0));
    callPatches.append(callPatch);
}


void
mjit::Compiler::inlineCallHelper(uint32 argc, bool callingNew)
{
    
    interruptCheckHelper();

    
    if (callingNew)
        frame.discardFe(frame.peek(-int(argc + 1)));

    FrameEntry *fe = frame.peek(-int(argc + 2));

    
    if (fe->isConstant() || fe->isNotType(JSVAL_TYPE_OBJECT) || debugMode) {
        emitUncachedCall(argc, callingNew);
        return;
    }

#ifdef JS_MONOIC
    CallGenInfo callIC(argc);
    CallPatchInfo callPatch;

    



    callIC.pc = PC;
    callIC.frameDepth = frame.frameDepth();

    
    MaybeRegisterID typeReg, maybeDataReg;
    frame.ensureFullRegs(fe, &typeReg, &maybeDataReg);
    RegisterID dataReg = maybeDataReg.reg();
    if (!fe->isTypeKnown())
        frame.pinReg(typeReg.reg());
    frame.pinReg(dataReg);

    



    frame.syncAndKill(Registers(Registers::AvailRegs), Uses(argc + 2));
    frame.unpinKilledReg(dataReg);
    if (typeReg.isSet())
        frame.unpinKilledReg(typeReg.reg());

    Registers tempRegs;

    
    MaybeJump notObjectJump;
    if (typeReg.isSet())
        notObjectJump = masm.testObject(Assembler::NotEqual, typeReg.reg());

    tempRegs.takeReg(dataReg);
    RegisterID t0 = tempRegs.takeAnyReg();
    RegisterID t1 = tempRegs.takeAnyReg();

    




    Jump j = masm.branchPtrWithPatch(Assembler::NotEqual, dataReg, callIC.funGuard);
    callIC.funJump = j;

    Jump rejoin1, rejoin2;
    {
        stubcc.linkExitDirect(j, stubcc.masm.label());
        callIC.slowPathStart = stubcc.masm.label();

        



        Jump notFunction = stubcc.masm.testFunction(Assembler::NotEqual, dataReg);

        
        stubcc.masm.loadFunctionPrivate(dataReg, t0);
        stubcc.masm.load16(Address(t0, offsetof(JSFunction, flags)), t1);
        stubcc.masm.and32(Imm32(JSFUN_KINDMASK), t1);
        Jump isNative = stubcc.masm.branch32(Assembler::Below, t1, Imm32(JSFUN_INTERPRETED));

        




        Jump toPatch = stubcc.masm.jump();
        toPatch.linkTo(stubcc.masm.label(), &stubcc.masm);
        callIC.oolJump = toPatch;

        
        callIC.addrLabel1 = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
        callIC.oolCall = stubcc.call(callingNew ? ic::New : ic::Call);

        callIC.funObjReg = dataReg;
        callIC.funPtrReg = t0;

        







        rejoin1 = stubcc.masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                            Registers::ReturnReg);
        stubcc.masm.move(Imm32(argc), JSParamReg_Argc);
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
        stubcc.call(callingNew ? ic::NativeNew : ic::NativeCall);

        rejoin2 = stubcc.masm.jump();
    }

    



    callIC.hotPathLabel = masm.label();

    uint32 flags = 0;
    if (callingNew)
        flags |= JSFRAME_CONSTRUCTING;

    InlineFrameAssembler inlFrame(masm, callIC, flags);
    callPatch.fastNcodePatch = inlFrame.assemble(NULL);

    callIC.hotJump = masm.jump();
    callIC.joinPoint = callPatch.joinPoint = masm.label();
    masm.loadPtr(Address(JSFrameReg, JSStackFrame::offsetOfPrev()), JSFrameReg);

    frame.popn(argc + 2);
    frame.takeReg(JSReturnReg_Type);
    frame.takeReg(JSReturnReg_Data);
    frame.pushRegs(JSReturnReg_Type, JSReturnReg_Data);

    callIC.slowJoinPoint = stubcc.masm.label();
    rejoin1.linkTo(callIC.slowJoinPoint, &stubcc.masm);
    rejoin2.linkTo(callIC.slowJoinPoint, &stubcc.masm);
    stubcc.rejoin(Changes(0));

    callICs.append(callIC);
    callPatches.append(callPatch);
#else
    emitUncachedCall(argc, callingNew);
#endif
}






void
mjit::Compiler::addCallSite(uint32 id, bool stub)
{
    InternalCallSite site;
    site.stub = stub;
#if (defined(JS_NO_FASTCALL) && defined(JS_CPU_X86)) || defined(_WIN64)
    site.location = stub ? stubcc.masm.callLabel : masm.callLabel;
#else
    site.location = stub ? stubcc.masm.label() : masm.label();
#endif

    site.pc = PC;
    site.id = id;
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
        int cmp = js_CompareStrings(lhs.toString(), rhs.toString());
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

void
mjit::Compiler::emitStubCmpOp(BoolStub stub, jsbytecode *target, JSOp fused)
{
    prepareStubCall(Uses(2));
    stubCall(stub);
    frame.pop();
    frame.pop();

    if (!target) {
        frame.takeReg(Registers::ReturnReg);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
    } else {
        JS_ASSERT(fused == JSOP_IFEQ || fused == JSOP_IFNE);

        frame.syncAndForgetEverything();
        Assembler::Condition cond = (fused == JSOP_IFEQ)
                                    ? Assembler::Zero
                                    : Assembler::NonZero;
        Jump j = masm.branchTest32(cond, Registers::ReturnReg,
                                   Registers::ReturnReg);
        jumpAndTrace(j, target);
    }
}

void
mjit::Compiler::jsop_setprop_slow(JSAtom *atom, bool usePropCache)
{
    prepareStubCall(Uses(2));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    if (usePropCache)
        stubCall(STRICT_VARIANT(stubs::SetName));
    else
        stubCall(STRICT_VARIANT(stubs::SetPropNoCache));
    JS_STATIC_ASSERT(JSOP_SETNAME_LENGTH == JSOP_SETPROP_LENGTH);
    frame.shimmy(1);
}

void
mjit::Compiler::jsop_getprop_slow(JSAtom *atom, bool usePropCache)
{
    prepareStubCall(Uses(1));
    if (usePropCache) {
        stubCall(stubs::GetProp);
    } else {
        masm.move(ImmPtr(atom), Registers::ArgReg1);
        stubCall(stubs::GetPropNoCache);
    }
    frame.pop();
    frame.pushSynced();
}

bool
mjit::Compiler::jsop_callprop_slow(JSAtom *atom)
{
    prepareStubCall(Uses(1));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    stubCall(stubs::CallProp);
    frame.pop();
    frame.pushSynced();
    frame.pushSynced();
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
            masm.loadPtr(Address(str, offsetof(JSString, mLengthAndFlags)), str);
            masm.rshiftPtr(Imm32(JSString::FLAGS_LENGTH_SHIFT), str);
            frame.pop();
            frame.pushTypedPayload(JSVAL_TYPE_INT32, str);
        }
        return true;
    }

#if defined JS_POLYIC
    return jsop_getprop(cx->runtime->atomState.lengthAtom);
#else
    prepareStubCall(Uses(1));
    stubCall(stubs::Length);
    frame.pop();
    frame.pushSynced();
    return true;
#endif
}

#ifdef JS_MONOIC
void
mjit::Compiler::passMICAddress(MICGenInfo &mic)
{
    mic.addrLabel = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
}
#endif

#if defined JS_POLYIC
void
mjit::Compiler::passPICAddress(PICGenInfo &pic)
{
    pic.addrLabel = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
}

bool
mjit::Compiler::jsop_getprop(JSAtom *atom, bool doTypeCheck, bool usePropCache)
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

    PICGenInfo pic(ic::PICInfo::GET, usePropCache);

    
    Jump typeCheck;
    if (doTypeCheck && !top->isTypeKnown()) {
        RegisterID reg = frame.tempRegForType(top);
        pic.typeReg = reg;

        
        pic.fastPathStart = masm.label();
        Jump j = masm.testObject(Assembler::NotEqual, reg);

        
        RETURN_IF_OOM(false);
        JS_ASSERT(masm.differenceBetween(pic.fastPathStart, masm.label()) == GETPROP_INLINE_TYPE_GUARD);

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
    pic.objRemat = frame.dataRematInfo(top);

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();

    DataLabel32 inlineShapeLabel;
    Jump j = masm.branch32WithPatch(Assembler::NotEqual, shapeReg,
                                    Imm32(int32(JSObjectMap::INVALID_SHAPE)),
                                    inlineShapeLabel);
    DBGLABEL(dbgInlineShapeJump);

    pic.slowPathStart = stubcc.linkExit(j, Uses(1));

    stubcc.leave();
    passPICAddress(pic);
    pic.callReturn = stubcc.call(ic::GetProp);

    
#if defined JS_NUNBOX32
    DBGLABEL(dbgDslotsLoad);
#elif defined JS_PUNBOX64
    Label dslotsLoadLabel = masm.label();
#endif
    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);

    
    Address slot(objReg, 1 << 24);
    frame.pop();

#if defined JS_NUNBOX32
    masm.loadTypeTag(slot, shapeReg);
    DBGLABEL(dbgTypeLoad);

    masm.loadPayload(slot, objReg);
    DBGLABEL(dbgDataLoad);
#elif defined JS_PUNBOX64
    Label inlineValueLoadLabel =
        masm.loadValueAsComponents(slot, shapeReg, objReg);
#endif
    pic.storeBack = masm.label();

    
    RETURN_IF_OOM(false);
#if defined JS_NUNBOX32
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDslotsLoad) == GETPROP_DSLOTS_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgTypeLoad) == GETPROP_TYPE_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDataLoad) == GETPROP_DATA_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, inlineShapeLabel) == GETPROP_INLINE_SHAPE_OFFSET);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, dbgInlineShapeJump) == GETPROP_INLINE_SHAPE_JUMP);
#elif defined JS_PUNBOX64
    pic.labels.getprop.dslotsLoadOffset = masm.differenceBetween(pic.storeBack, dslotsLoadLabel);
    JS_ASSERT(pic.labels.getprop.dslotsLoadOffset == masm.differenceBetween(pic.storeBack, dslotsLoadLabel));

    pic.labels.getprop.inlineShapeOffset = masm.differenceBetween(pic.shapeGuard, inlineShapeLabel);
    JS_ASSERT(pic.labels.getprop.inlineShapeOffset == masm.differenceBetween(pic.shapeGuard, inlineShapeLabel));

    pic.labels.getprop.inlineValueOffset = masm.differenceBetween(pic.storeBack, inlineValueLoadLabel);
    JS_ASSERT(pic.labels.getprop.inlineValueOffset == masm.differenceBetween(pic.storeBack, inlineValueLoadLabel));

    JS_ASSERT(masm.differenceBetween(inlineShapeLabel, dbgInlineShapeJump) == GETPROP_INLINE_SHAPE_JUMP);
#endif
    

    pic.objReg = objReg;
    frame.pushRegs(shapeReg, objReg);

    stubcc.rejoin(Changes(1));

    pics.append(pic);
    return true;
}

#ifdef JS_POLYIC
bool
mjit::Compiler::jsop_getelem_pic(FrameEntry *obj, FrameEntry *id, RegisterID objReg,
                                 RegisterID idReg, RegisterID shapeReg)
{
    PICGenInfo pic(ic::PICInfo::GETELEM, true);

    pic.objRemat = frame.dataRematInfo(obj);
    pic.idRemat = frame.dataRematInfo(id);
    pic.shapeReg = shapeReg;
    pic.hasTypeCheck = false;

    pic.fastPathStart = masm.label();

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();

    DataLabel32 inlineShapeOffsetLabel;
    Jump jmpShapeGuard = masm.branch32WithPatch(Assembler::NotEqual, shapeReg,
                                 Imm32(int32(JSObjectMap::INVALID_SHAPE)),
                                 inlineShapeOffsetLabel);
    DBGLABEL(dbgInlineShapeJump);

    
#if defined JS_NUNBOX32
    static const void *BOGUS_ATOM = (void *)0xdeadbeef;
#elif defined JS_PUNBOX64
    static const void *BOGUS_ATOM = (void *)0xfeedfacedeadbeef;
#endif

    DataLabelPtr inlineAtomOffsetLabel;
    Jump idGuard = masm.branchPtrWithPatch(Assembler::NotEqual, idReg,
                                 inlineAtomOffsetLabel, ImmPtr(BOGUS_ATOM));
    DBGLABEL(dbgInlineAtomJump);

    



    stubcc.linkExit(idGuard, Uses(2));
    pic.slowPathStart = stubcc.linkExit(jmpShapeGuard, Uses(2));

    stubcc.leave();
    passPICAddress(pic);
    pic.callReturn = stubcc.call(ic::GetElem);

    
#if defined JS_NUNBOX32
    DBGLABEL(dbgDslotsLoad);
#elif defined JS_PUNBOX64
    Label dslotsLoadLabel = masm.label();
#endif
    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);

    
    Address slot(objReg, 1 << 24);
#if defined JS_NUNBOX32
    masm.loadTypeTag(slot, shapeReg);
    DBGLABEL(dbgTypeLoad);
    masm.loadPayload(slot, objReg);
    DBGLABEL(dbgDataLoad);
#elif defined JS_PUNBOX64
    Label inlineValueOffsetLabel =
        masm.loadValueAsComponents(slot, shapeReg, objReg);
#endif
    pic.storeBack = masm.label();

    pic.objReg = objReg;
    pic.idReg = idReg;

    RETURN_IF_OOM(false);
#if defined JS_NUNBOX32
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDslotsLoad) == GETELEM_DSLOTS_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgTypeLoad) == GETELEM_TYPE_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDataLoad) == GETELEM_DATA_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, inlineAtomOffsetLabel) == GETELEM_INLINE_ATOM_OFFSET);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, dbgInlineAtomJump) == GETELEM_INLINE_ATOM_JUMP);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, inlineShapeOffsetLabel) == GETELEM_INLINE_SHAPE_OFFSET);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, dbgInlineShapeJump) == GETELEM_INLINE_SHAPE_JUMP);
#elif defined JS_PUNBOX64
    pic.labels.getelem.dslotsLoadOffset = masm.differenceBetween(pic.storeBack, dslotsLoadLabel);
    JS_ASSERT(pic.labels.getelem.dslotsLoadOffset == masm.differenceBetween(pic.storeBack, dslotsLoadLabel));

    pic.labels.getelem.inlineShapeOffset = masm.differenceBetween(pic.shapeGuard, inlineShapeOffsetLabel);
    JS_ASSERT(pic.labels.getelem.inlineShapeOffset == masm.differenceBetween(pic.shapeGuard, inlineShapeOffsetLabel));

    pic.labels.getelem.inlineAtomOffset = masm.differenceBetween(pic.shapeGuard, inlineAtomOffsetLabel);
    JS_ASSERT(pic.labels.getelem.inlineAtomOffset == masm.differenceBetween(pic.shapeGuard, inlineAtomOffsetLabel));

    pic.labels.getelem.inlineValueOffset = masm.differenceBetween(pic.storeBack, inlineValueOffsetLabel);
    JS_ASSERT(pic.labels.getelem.inlineValueOffset == masm.differenceBetween(pic.storeBack, inlineValueOffsetLabel));

    JS_ASSERT(masm.differenceBetween(inlineShapeOffsetLabel, dbgInlineShapeJump) == GETELEM_INLINE_SHAPE_JUMP);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, dbgInlineAtomJump) ==
              pic.labels.getelem.inlineAtomOffset + GETELEM_INLINE_ATOM_JUMP);
#endif

    JS_ASSERT(pic.idReg != pic.objReg);
    JS_ASSERT(pic.idReg != pic.shapeReg);
    JS_ASSERT(pic.objReg != pic.shapeReg);

    pics.append(pic);
    return true;
}
#endif

bool
mjit::Compiler::jsop_callprop_generic(JSAtom *atom)
{
    FrameEntry *top = frame.peek(-1);

    




    RegisterID objReg = frame.copyDataIntoReg(top);
    RegisterID shapeReg = frame.allocReg();

    PICGenInfo pic(ic::PICInfo::CALL, true);

    
    pic.typeReg = frame.copyTypeIntoReg(top);

    
    pic.fastPathStart = masm.label();

    





    Jump typeCheck = masm.testObject(Assembler::NotEqual, pic.typeReg);
    DBGLABEL(dbgInlineTypeGuard);

    pic.typeCheck = stubcc.linkExit(typeCheck, Uses(1));
    pic.hasTypeCheck = true;
    pic.objReg = objReg;
    pic.shapeReg = shapeReg;
    pic.atom = atom;
    pic.objRemat = frame.dataRematInfo(top);

    



    uint32 thisvSlot = frame.frameDepth();
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
    DBGLABEL(dbgInlineShapeJump);

    pic.slowPathStart = stubcc.linkExit(j, Uses(1));

    
    stubcc.leave();
    passPICAddress(pic);
    pic.callReturn = stubcc.call(ic::CallProp);

    
    frame.pop();
    frame.pushRegs(shapeReg, objReg);
    frame.pushSynced();

    
#if defined JS_NUNBOX32
    DBGLABEL(dbgDslotsLoad);
#elif defined JS_PUNBOX64
    Label dslotsLoadLabel = masm.label();
#endif
    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);

    
    Address slot(objReg, 1 << 24);

#if defined JS_NUNBOX32
    masm.loadTypeTag(slot, shapeReg);
    DBGLABEL(dbgTypeLoad);

    masm.loadPayload(slot, objReg);
    DBGLABEL(dbgDataLoad);
#elif defined JS_PUNBOX64
    Label inlineValueLoadLabel =
        masm.loadValueAsComponents(slot, shapeReg, objReg);
#endif
    pic.storeBack = masm.label();

    
    RETURN_IF_OOM(false);
    JS_ASSERT(masm.differenceBetween(pic.fastPathStart, dbgInlineTypeGuard) == GETPROP_INLINE_TYPE_GUARD);
#if defined JS_NUNBOX32
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDslotsLoad) == GETPROP_DSLOTS_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgTypeLoad) == GETPROP_TYPE_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDataLoad) == GETPROP_DATA_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, inlineShapeLabel) == GETPROP_INLINE_SHAPE_OFFSET);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, dbgInlineShapeJump) == GETPROP_INLINE_SHAPE_JUMP);
#elif defined JS_PUNBOX64
    pic.labels.getprop.dslotsLoadOffset = masm.differenceBetween(pic.storeBack, dslotsLoadLabel);
    JS_ASSERT(pic.labels.getprop.dslotsLoadOffset == masm.differenceBetween(pic.storeBack, dslotsLoadLabel));

    pic.labels.getprop.inlineShapeOffset = masm.differenceBetween(pic.shapeGuard, inlineShapeLabel);
    JS_ASSERT(pic.labels.getprop.inlineShapeOffset == masm.differenceBetween(pic.shapeGuard, inlineShapeLabel));

    pic.labels.getprop.inlineValueOffset = masm.differenceBetween(pic.storeBack, inlineValueLoadLabel);
    JS_ASSERT(pic.labels.getprop.inlineValueOffset == masm.differenceBetween(pic.storeBack, inlineValueLoadLabel));

    JS_ASSERT(masm.differenceBetween(inlineShapeLabel, dbgInlineShapeJump) == GETPROP_INLINE_SHAPE_JUMP);
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
    if (!js_GetClassPrototype(cx, NULL, JSProto_String, &obj))
        return false;

    
    RegisterID reg = frame.allocReg();

    masm.move(ImmPtr(obj), reg);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, reg);

    
    if (!jsop_getprop(atom))
        return false;

    
    frame.dup2();
    frame.shift(-3);
    frame.shift(-1);

    
#ifdef DEBUG
    FrameEntry *funFe = frame.peek(-2);
#endif
    JS_ASSERT(!funFe->isTypeKnown());

    




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

    PICGenInfo pic(ic::PICInfo::CALL, true);

    JS_ASSERT(top->isTypeKnown());
    JS_ASSERT(top->getKnownType() == JSVAL_TYPE_OBJECT);

    pic.fastPathStart = masm.label();
    pic.hasTypeCheck = false;
    pic.typeReg = Registers::ReturnReg;

    RegisterID objReg = frame.copyDataIntoReg(top);
    RegisterID shapeReg = frame.allocReg();

    pic.shapeReg = shapeReg;
    pic.atom = atom;
    pic.objRemat = frame.dataRematInfo(top);

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();

    DataLabel32 inlineShapeLabel;
    Jump j = masm.branch32WithPatch(Assembler::NotEqual, shapeReg,
                           Imm32(int32(JSObjectMap::INVALID_SHAPE)),
                           inlineShapeLabel);
    DBGLABEL(dbgInlineShapeJump);

    pic.slowPathStart = stubcc.linkExit(j, Uses(1));

    stubcc.leave();
    passPICAddress(pic);
    pic.callReturn = stubcc.call(ic::CallProp);

    
#if defined JS_NUNBOX32
    DBGLABEL(dbgDslotsLoad);
#elif defined JS_PUNBOX64
    Label dslotsLoadLabel = masm.label();
#endif
    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);

    
    Address slot(objReg, 1 << 24);

#if defined JS_NUNBOX32
    masm.loadTypeTag(slot, shapeReg);
    DBGLABEL(dbgTypeLoad);

    masm.loadPayload(slot, objReg);
    DBGLABEL(dbgDataLoad);
#elif defined JS_PUNBOX64
    Label inlineValueLoadLabel =
        masm.loadValueAsComponents(slot, shapeReg, objReg);
#endif

    pic.storeBack = masm.label();
    pic.objReg = objReg;

    








    frame.dup();
    frame.pushRegs(shapeReg, objReg);
    frame.shift(-2);

    



    RETURN_IF_OOM(false);
#if defined JS_NUNBOX32
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDslotsLoad) == GETPROP_DSLOTS_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgTypeLoad) == GETPROP_TYPE_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDataLoad) == GETPROP_DATA_LOAD);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, inlineShapeLabel) == GETPROP_INLINE_SHAPE_OFFSET);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, dbgInlineShapeJump) == GETPROP_INLINE_SHAPE_JUMP);
#elif defined JS_PUNBOX64
    pic.labels.getprop.dslotsLoadOffset = masm.differenceBetween(pic.storeBack, dslotsLoadLabel);
    JS_ASSERT(pic.labels.getprop.dslotsLoadOffset == masm.differenceBetween(pic.storeBack, dslotsLoadLabel));

    pic.labels.getprop.inlineShapeOffset = masm.differenceBetween(pic.shapeGuard, inlineShapeLabel);
    JS_ASSERT(pic.labels.getprop.inlineShapeOffset == masm.differenceBetween(pic.shapeGuard, inlineShapeLabel));

    pic.labels.getprop.inlineValueOffset = masm.differenceBetween(pic.storeBack, inlineValueLoadLabel);
    JS_ASSERT(pic.labels.getprop.inlineValueOffset == masm.differenceBetween(pic.storeBack, inlineValueLoadLabel));

    JS_ASSERT(masm.differenceBetween(inlineShapeLabel, dbgInlineShapeJump) == GETPROP_INLINE_SHAPE_JUMP);
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

    PICGenInfo pic(op == JSOP_SETMETHOD ? ic::PICInfo::SETMETHOD : ic::PICInfo::SET, usePropCache);
    pic.atom = atom;

    
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
            stubcc.call(STRICT_VARIANT(stubs::SetName));
        else
            stubcc.call(STRICT_VARIANT(stubs::SetPropNoCache));
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
    pic.objRemat = frame.dataRematInfo(lhs);

    frame.unpinEntry(vr);

    
    masm.loadShape(objReg, shapeReg);
    pic.shapeGuard = masm.label();
    DataLabel32 inlineShapeOffsetLabel;
    Jump j = masm.branch32WithPatch(Assembler::NotEqual, shapeReg,
                                    Imm32(int32(JSObjectMap::INVALID_SHAPE)),
                                    inlineShapeOffsetLabel);
    DBGLABEL(dbgInlineShapeJump);

    
    {
        pic.slowPathStart = stubcc.linkExit(j, Uses(2));

        stubcc.leave();
        passPICAddress(pic);
        pic.callReturn = stubcc.call(ic::SetProp);
    }

    
#if defined JS_NUNBOX32
    DBGLABEL(dbgDslots);
#elif defined JS_PUNBOX64
    Label dslotsLoadLabel = masm.label();
#endif
    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);

    
    Address slot(objReg, 1 << 24);
#if defined JS_NUNBOX32
    Label dbgInlineStoreType = masm.storeValue(vr, slot);
#elif defined JS_PUNBOX64
    masm.storeValue(vr, slot);
#endif
    DBGLABEL(dbgAfterValueStore);
    pic.storeBack = masm.label();

    frame.freeReg(objReg);
    frame.freeReg(shapeReg);

    
    frame.shimmy(1);

    
    {
        if (pic.hasTypeCheck)
            typeCheck.linkTo(stubcc.masm.label(), &stubcc.masm);
        stubcc.rejoin(Changes(1));
    }

    RETURN_IF_OOM(false);
#if defined JS_PUNBOX64
    pic.labels.setprop.dslotsLoadOffset = masm.differenceBetween(pic.storeBack, dslotsLoadLabel);
    pic.labels.setprop.inlineShapeOffset = masm.differenceBetween(pic.shapeGuard, inlineShapeOffsetLabel);
    JS_ASSERT(masm.differenceBetween(inlineShapeOffsetLabel, dbgInlineShapeJump) == SETPROP_INLINE_SHAPE_JUMP);
    JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgAfterValueStore) == SETPROP_INLINE_STORE_VALUE);
#elif defined JS_NUNBOX32
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, inlineShapeOffsetLabel) == SETPROP_INLINE_SHAPE_OFFSET);
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, dbgInlineShapeJump) == SETPROP_INLINE_SHAPE_JUMP);
    if (vr.isConstant) {
        
        JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgInlineStoreType)-4 == SETPROP_INLINE_STORE_CONST_TYPE);
        JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgAfterValueStore)-4 == SETPROP_INLINE_STORE_CONST_DATA);
        JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDslots) == SETPROP_DSLOTS_BEFORE_CONSTANT);
    } else if (vr.u.s.isTypeKnown) {
        JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgInlineStoreType)-4 == SETPROP_INLINE_STORE_KTYPE_TYPE);
        JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgAfterValueStore) == SETPROP_INLINE_STORE_KTYPE_DATA);
        JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDslots) == SETPROP_DSLOTS_BEFORE_KTYPE);
    } else {
        JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgInlineStoreType) == SETPROP_INLINE_STORE_DYN_TYPE);
        JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgAfterValueStore) == SETPROP_INLINE_STORE_DYN_DATA);
        JS_ASSERT(masm.differenceBetween(pic.storeBack, dbgDslots) == SETPROP_DSLOTS_BEFORE_DYNAMIC);
    }
#endif

    pics.append(pic);
    return true;
}

void
mjit::Compiler::jsop_name(JSAtom *atom)
{
    PICGenInfo pic(ic::PICInfo::NAME, true);

    pic.shapeReg = frame.allocReg();
    pic.objReg = frame.allocReg();
    pic.typeReg = Registers::ReturnReg;
    pic.atom = atom;
    pic.hasTypeCheck = false;
    pic.fastPathStart = masm.label();

    pic.shapeGuard = masm.label();
    Jump j = masm.jump();
    DBGLABEL(dbgJumpOffset);
    {
        pic.slowPathStart = stubcc.linkExit(j, Uses(0));
        stubcc.leave();
        passPICAddress(pic);
        pic.callReturn = stubcc.call(ic::Name);
    }

    pic.storeBack = masm.label();
    frame.pushRegs(pic.shapeReg, pic.objReg);

    JS_ASSERT(masm.differenceBetween(pic.fastPathStart, dbgJumpOffset) == SCOPENAME_JUMP_OFFSET);

    stubcc.rejoin(Changes(1));

    pics.append(pic);
}

bool
mjit::Compiler::jsop_xname(JSAtom *atom)
{
    PICGenInfo pic(ic::PICInfo::XNAME, true);

    FrameEntry *fe = frame.peek(-1);
    if (fe->isNotType(JSVAL_TYPE_OBJECT)) {
        return jsop_getprop(atom);
    }

    if (!fe->isTypeKnown()) {
        Jump notObject = frame.testObject(Assembler::NotEqual, fe);
        stubcc.linkExit(notObject, Uses(1));
    }

    pic.shapeReg = frame.allocReg();
    pic.objReg = frame.copyDataIntoReg(fe);
    pic.typeReg = Registers::ReturnReg;
    pic.atom = atom;
    pic.hasTypeCheck = false;
    pic.fastPathStart = masm.label();

    pic.shapeGuard = masm.label();
    Jump j = masm.jump();
    DBGLABEL(dbgJumpOffset);
    {
        pic.slowPathStart = stubcc.linkExit(j, Uses(1));
        stubcc.leave();
        passPICAddress(pic);
        pic.callReturn = stubcc.call(ic::XName);
    }

    pic.storeBack = masm.label();
    frame.pop();
    frame.pushRegs(pic.shapeReg, pic.objReg);

    JS_ASSERT(masm.differenceBetween(pic.fastPathStart, dbgJumpOffset) == SCOPENAME_JUMP_OFFSET);

    stubcc.rejoin(Changes(1));

    pics.append(pic);
    return true;
}

void
mjit::Compiler::jsop_bindname(uint32 index, bool usePropCache)
{
    PICGenInfo pic(ic::PICInfo::BIND, usePropCache);

    pic.shapeReg = frame.allocReg();
    pic.objReg = frame.allocReg();
    pic.typeReg = Registers::ReturnReg;
    pic.atom = script->getAtom(index);
    pic.hasTypeCheck = false;
    pic.fastPathStart = masm.label();

    Address parent(pic.objReg, offsetof(JSObject, parent));
    masm.loadPtr(Address(JSFrameReg, JSStackFrame::offsetOfScopeChain()), pic.objReg);

    pic.shapeGuard = masm.label();
#if defined JS_NUNBOX32
    Jump j = masm.branchPtr(Assembler::NotEqual, masm.payloadOf(parent), ImmPtr(0));
    DBGLABEL(inlineJumpOffset);
#elif defined JS_PUNBOX64
    masm.loadPayload(parent, Registers::ValueReg);
    Jump j = masm.branchPtr(Assembler::NotEqual, Registers::ValueReg, ImmPtr(0));
    Label inlineJumpOffset = masm.label();
#endif
    {
        pic.slowPathStart = stubcc.linkExit(j, Uses(0));
        stubcc.leave();
        passPICAddress(pic);
        pic.callReturn = stubcc.call(ic::BindName);
    }

    pic.storeBack = masm.label();
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, pic.objReg);
    frame.freeReg(pic.shapeReg);

#if defined JS_NUNBOX32
    JS_ASSERT(masm.differenceBetween(pic.shapeGuard, inlineJumpOffset) == BINDNAME_INLINE_JUMP_OFFSET);
#elif defined JS_PUNBOX64
    pic.labels.bindname.inlineJumpOffset = masm.differenceBetween(pic.shapeGuard, inlineJumpOffset);
    JS_ASSERT(pic.labels.bindname.inlineJumpOffset == masm.differenceBetween(pic.shapeGuard, inlineJumpOffset));
#endif

    stubcc.rejoin(Changes(1));

    pics.append(pic);
}

#else 

void
mjit::Compiler::jsop_name(JSAtom *atom)
{
    prepareStubCall(Uses(0));
    stubCall(stubs::Name);
    frame.pushSynced();
}

bool
mjit::Compiler::jsop_xname(JSAtom *atom)
{
    return jsop_getprop(atom);
}

bool
mjit::Compiler::jsop_getprop(JSAtom *atom, bool typecheck, bool usePropCache)
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
mjit::Compiler::jsop_bindname(uint32 index, bool usePropCache)
{
    RegisterID reg = frame.allocReg();
    Address scopeChain(JSFrameReg, JSStackFrame::offsetOfScopeChain());
    masm.loadPtr(scopeChain, reg);

    Address address(reg, offsetof(JSObject, parent));

    Jump j = masm.branchPtr(Assembler::NotEqual, masm.payloadOf(address), ImmPtr(0));

    stubcc.linkExit(j, Uses(0));
    stubcc.leave();
    if (usePropCache) {
        stubcc.call(stubs::BindName);
    } else {
        masm.move(ImmPtr(script->getAtom(index)), Registers::ArgReg1);
        stubcc.call(stubs::BindNameNoCache);
    }

    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, reg);

    stubcc.rejoin(Changes(1));
}
#endif

void
mjit::Compiler::jsop_getarg(uint32 index)
{
    frame.push(Address(JSFrameReg, JSStackFrame::offsetOfFormalArg(fun, index)));
}

void
mjit::Compiler::jsop_this()
{
    Address thisvAddr(JSFrameReg, JSStackFrame::offsetOfThis(fun));
    frame.push(thisvAddr);
    




    if (fun && !script->strictModeCode) {
        Jump notObj = frame.testObject(Assembler::NotEqual, frame.peek(-1));
        stubcc.linkExit(notObj, Uses(1));
        stubcc.leave();
        stubcc.call(stubs::This);
        stubcc.rejoin(Changes(1));
    }
}

void
mjit::Compiler::jsop_gnameinc(JSOp op, VoidStubAtom stub, uint32 index)
{
#if defined JS_MONOIC
    jsbytecode *next = &PC[JSOP_GNAMEINC_LENGTH];
    bool pop = (JSOp(*next) == JSOP_POP) && !analysis[next].nincoming;
    int amt = (op == JSOP_GNAMEINC || op == JSOP_INCGNAME) ? -1 : 1;

    if (pop || (op == JSOP_INCGNAME || op == JSOP_DECGNAME)) {
        

        jsop_getgname(index);
        

        frame.push(Int32Value(amt));
        

        
        jsop_binary(JSOP_SUB, stubs::Sub);
        

        jsop_bindgname();
        

        frame.dup2();
        

        frame.shift(-3);
        

        frame.shift(-1);
        

        jsop_setgname(index);
        

        if (pop)
            frame.pop();
    } else {
        

        jsop_getgname(index);
        

        jsop_pos();
        

        frame.dup();
        

        frame.push(Int32Value(-amt));
        

        jsop_binary(JSOP_ADD, stubs::Add);
        

        jsop_bindgname();
        

        frame.dup2();
        

        frame.shift(-3);
        

        frame.shift(-1);
        

        jsop_setgname(index);
        

        frame.pop();
        
    }

    if (pop)
        PC += JSOP_POP_LENGTH;
#else
    JSAtom *atom = script->getAtom(index);
    prepareStubCall(Uses(0));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    stubCall(stub);
    frame.pushSynced();
#endif

    PC += JSOP_GNAMEINC_LENGTH;
}

bool
mjit::Compiler::jsop_nameinc(JSOp op, VoidStubAtom stub, uint32 index)
{
    JSAtom *atom = script->getAtom(index);
#if defined JS_POLYIC
    jsbytecode *next = &PC[JSOP_NAMEINC_LENGTH];
    bool pop = (JSOp(*next) == JSOP_POP) && !analysis[next].nincoming;
    int amt = (op == JSOP_NAMEINC || op == JSOP_INCNAME) ? -1 : 1;

    if (pop || (op == JSOP_INCNAME || op == JSOP_DECNAME)) {
        

        jsop_name(atom);
        

        frame.push(Int32Value(amt));
        

        
        jsop_binary(JSOP_SUB, stubs::Sub);
        

        jsop_bindname(index, false);
        

        frame.dup2();
        

        frame.shift(-3);
        

        frame.shift(-1);
        

        if (!jsop_setprop(atom, false))
            return false;
        

        if (pop)
            frame.pop();
    } else {
        

        jsop_name(atom);
        

        jsop_pos();
        

        frame.dup();
        

        frame.push(Int32Value(-amt));
        

        jsop_binary(JSOP_ADD, stubs::Add);
        

        jsop_bindname(index, false);
        

        frame.dup2();
        

        frame.shift(-3);
        

        frame.shift(-1);
        

        if (!jsop_setprop(atom, false))
            return false;
        

        frame.pop();
        
    }

    if (pop)
        PC += JSOP_POP_LENGTH;
#else
    prepareStubCall(Uses(0));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    stubCall(stub);
    frame.pushSynced();
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
        bool pop = (JSOp(*next) == JSOP_POP) && !analysis[next].nincoming;
        int amt = (op == JSOP_PROPINC || op == JSOP_INCPROP) ? -1 : 1;

        if (pop || (op == JSOP_INCPROP || op == JSOP_DECPROP)) {
            

            frame.dup();
            

            if (!jsop_getprop(atom))
                return false;
            

            frame.push(Int32Value(amt));
            

            
            jsop_binary(JSOP_SUB, stubs::Sub);
            

            if (!jsop_setprop(atom, false))
                return false;
            

            if (pop)
                frame.pop();
        } else {
            

            frame.dup();
            

            if (!jsop_getprop(atom))
                return false;
            

            jsop_pos();
            

            frame.dup();
            

            frame.push(Int32Value(-amt));
            

            jsop_binary(JSOP_ADD, stubs::Add);
            

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
        stubCall(stub);
        frame.pop();
        frame.pushSynced();
    }

    PC += JSOP_PROPINC_LENGTH;
    return true;
}

void
mjit::Compiler::iter(uintN flags)
{
    FrameEntry *fe = frame.peek(-1);

    



    if ((flags != JSITER_ENUMERATE) || fe->isNotType(JSVAL_TYPE_OBJECT)) {
        prepareStubCall(Uses(1));
        masm.move(Imm32(flags), Registers::ArgReg1);
        stubCall(stubs::Iter);
        frame.pop();
        frame.pushSynced();
        return;
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

    



    masm.loadPtr(FrameAddress(offsetof(VMFrame, cx)), T1);
#ifdef JS_THREADSAFE
    masm.loadPtr(Address(T1, offsetof(JSContext, thread)), T1);
    masm.loadPtr(Address(T1, offsetof(JSThread, data.lastNativeIterator)), ioreg);
#else
    masm.loadPtr(Address(T1, offsetof(JSContext, runtime)), T1);
    masm.loadPtr(Address(T1, offsetof(JSRuntime, threadData.lastNativeIterator)), ioreg);
#endif

    
    Jump nullIterator = masm.branchTest32(Assembler::Zero, ioreg, ioreg);
    stubcc.linkExit(nullIterator, Uses(1));

    
    masm.loadPtr(Address(ioreg, offsetof(JSObject, privateData)), nireg);

    
    Address flagsAddr(nireg, offsetof(NativeIterator, flags));
    masm.load32(flagsAddr, T1);
    Jump activeIterator = masm.branchTest32(Assembler::NonZero, T1, Imm32(JSITER_ACTIVE));
    stubcc.linkExit(activeIterator, Uses(1));

    
    masm.loadShape(reg, T1);
    masm.loadPtr(Address(nireg, offsetof(NativeIterator, shapes_array)), T2);
    masm.load32(Address(T2, 0), T2);
    Jump mismatchedObject = masm.branch32(Assembler::NotEqual, T1, T2);
    stubcc.linkExit(mismatchedObject, Uses(1));

    
    masm.loadPtr(Address(reg, offsetof(JSObject, proto)), T1);
    masm.loadShape(T1, T1);
    masm.loadPtr(Address(nireg, offsetof(NativeIterator, shapes_array)), T2);
    masm.load32(Address(T2, sizeof(uint32)), T2);
    Jump mismatchedProto = masm.branch32(Assembler::NotEqual, T1, T2);
    stubcc.linkExit(mismatchedProto, Uses(1));

    





    masm.loadPtr(Address(reg, offsetof(JSObject, proto)), T1);
    masm.loadPtr(Address(T1, offsetof(JSObject, proto)), T1);
    Jump overlongChain = masm.branchPtr(Assembler::NonZero, T1, T1);
    stubcc.linkExit(overlongChain, Uses(1));

    

    
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
    stubcc.call(stubs::Iter);

    
    frame.pop();
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, ioreg);

    stubcc.rejoin(Changes(1));
}





void
mjit::Compiler::iterNext()
{
    FrameEntry *fe = frame.peek(-1);
    RegisterID reg = frame.tempRegForData(fe);

    
    frame.pinReg(reg);
    RegisterID T1 = frame.allocReg();
    frame.unpinReg(reg);

    
    masm.loadPtr(Address(reg, offsetof(JSObject, clasp)), T1);
    Jump notFast = masm.branchPtr(Assembler::NotEqual, T1, ImmPtr(&js_IteratorClass));
    stubcc.linkExit(notFast, Uses(1));

    
    masm.loadFunctionPrivate(reg, T1);

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
    stubcc.call(stubs::IterNext);

    frame.pushUntypedPayload(JSVAL_TYPE_STRING, T3);

    
    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::iterMore()
{
    FrameEntry *fe= frame.peek(-1);
    RegisterID reg = frame.tempRegForData(fe);

    frame.pinReg(reg);
    RegisterID T1 = frame.allocReg();
    frame.unpinReg(reg);

    
    masm.loadPtr(Address(reg, offsetof(JSObject, clasp)), T1);
    Jump notFast = masm.branchPtr(Assembler::NotEqual, T1, ImmPtr(&js_IteratorClass));
    stubcc.linkExitForBranch(notFast);

    
    masm.loadFunctionPrivate(reg, T1);

    
    RegisterID T2 = frame.allocReg();
    frame.syncAndForgetEverything();
    masm.loadPtr(Address(T1, offsetof(NativeIterator, props_cursor)), T2);
    masm.loadPtr(Address(T1, offsetof(NativeIterator, props_end)), T1);
    Jump jFast = masm.branchPtr(Assembler::LessThan, T2, T1);

    jsbytecode *target = &PC[JSOP_MOREITER_LENGTH];
    JSOp next = JSOp(*target);
    JS_ASSERT(next == JSOP_IFNE || next == JSOP_IFNEX);

    target += (next == JSOP_IFNE)
              ? GET_JUMP_OFFSET(target)
              : GET_JUMPX_OFFSET(target);

    stubcc.leave();
    stubcc.call(stubs::IterMore);
    Jump j = stubcc.masm.branchTest32(Assembler::NonZero, Registers::ReturnReg,
                                      Registers::ReturnReg);

    PC += JSOP_MOREITER_LENGTH;
    PC += js_CodeSpec[next].length;

    stubcc.rejoin(Changes(1));

    jumpAndTrace(jFast, target, &j);
}

void
mjit::Compiler::iterEnd()
{
    FrameEntry *fe= frame.peek(-1);
    RegisterID reg = frame.tempRegForData(fe);

    frame.pinReg(reg);
    RegisterID T1 = frame.allocReg();
    frame.unpinReg(reg);

    
    masm.loadPtr(Address(reg, offsetof(JSObject, clasp)), T1);
    Jump notIterator = masm.branchPtr(Assembler::NotEqual, T1, ImmPtr(&js_IteratorClass));
    stubcc.linkExit(notIterator, Uses(1));

    
    masm.loadPtr(Address(reg, offsetof(JSObject, privateData)), T1);

    RegisterID T2 = frame.allocReg();

    
    Address flagAddr(T1, offsetof(NativeIterator, flags));
    masm.loadPtr(flagAddr, T2);

    
    Jump notEnumerate = masm.branch32(Assembler::NotEqual, T2,
                                      Imm32(JSITER_ENUMERATE | JSITER_ACTIVE));
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
    stubcc.call(stubs::EndIter);

    frame.pop();

    stubcc.rejoin(Changes(1));
}

void
mjit::Compiler::jsop_eleminc(JSOp op, VoidStub stub)
{
    prepareStubCall(Uses(2));
    stubCall(stub);
    frame.popn(2);
    frame.pushSynced();
}

void
mjit::Compiler::jsop_getgname_slow(uint32 index)
{
    prepareStubCall(Uses(0));
    stubCall(stubs::GetGlobalName);
    frame.pushSynced();
}

void
mjit::Compiler::jsop_bindgname()
{
    if (script->compileAndGo && globalObj) {
        frame.push(ObjectValue(*globalObj));
        return;
    }

    
    prepareStubCall(Uses(0));
    stubCall(stubs::BindGlobalName);
    frame.takeReg(Registers::ReturnReg);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, Registers::ReturnReg);
}

void
mjit::Compiler::jsop_getgname(uint32 index)
{
#if defined JS_MONOIC
    jsop_bindgname();

    FrameEntry *fe = frame.peek(-1);
    JS_ASSERT(fe->isTypeKnown() && fe->getKnownType() == JSVAL_TYPE_OBJECT);

    MICGenInfo mic(ic::MICInfo::GET);
    RegisterID objReg;
    Jump shapeGuard;

    mic.entry = masm.label();
    if (fe->isConstant()) {
        JSObject *obj = &fe->getValue().toObject();
        frame.pop();
        JS_ASSERT(obj->isNative());

        objReg = frame.allocReg();

        masm.load32FromImm(&obj->objShape, objReg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, objReg,
                                            Imm32(int32(JSObjectMap::INVALID_SHAPE)), mic.shape);
        masm.move(ImmPtr(obj), objReg);
    } else {
        objReg = frame.ownRegForData(fe);
        frame.pop();
        RegisterID reg = frame.allocReg();

        masm.loadShape(objReg, reg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, reg,
                                            Imm32(int32(JSObjectMap::INVALID_SHAPE)), mic.shape);
        frame.freeReg(reg);
    }
    stubcc.linkExit(shapeGuard, Uses(0));

    stubcc.leave();
    passMICAddress(mic);
    mic.stubEntry = stubcc.masm.label();
    mic.call = stubcc.call(ic::GetGlobalName);

    
    uint32 slot = 1 << 24;

    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);
    Address address(objReg, slot);
    
    










    
    RegisterID dreg = frame.allocReg();
    
    RegisterID treg = objReg;

    mic.load = masm.label();
# if defined JS_NUNBOX32
#  if defined JS_CPU_ARM
    DataLabel32 offsetAddress = masm.load64WithAddressOffsetPatch(address, treg, dreg);
    JS_ASSERT(masm.differenceBetween(mic.load, offsetAddress) == 0);
#  else
    masm.loadPayload(address, dreg);
    masm.loadTypeTag(address, treg);
#  endif
# elif defined JS_PUNBOX64
    Label inlineValueLoadLabel =
        masm.loadValueAsComponents(address, treg, dreg);
    mic.patchValueOffset = masm.differenceBetween(mic.load, inlineValueLoadLabel);
    JS_ASSERT(mic.patchValueOffset == masm.differenceBetween(mic.load, inlineValueLoadLabel));
# endif

    frame.pushRegs(treg, dreg);

    stubcc.rejoin(Changes(1));
    mics.append(mic);

#else
    jsop_getgname_slow(index);
#endif
}

void
mjit::Compiler::jsop_setgname_slow(uint32 index)
{
    JSAtom *atom = script->getAtom(index);
    prepareStubCall(Uses(2));
    masm.move(ImmPtr(atom), Registers::ArgReg1);
    stubCall(STRICT_VARIANT(stubs::SetGlobalName));
    frame.popn(2);
    frame.pushSynced();
}

void
mjit::Compiler::jsop_setgname(uint32 index)
{
#if defined JS_MONOIC
    FrameEntry *objFe = frame.peek(-2);
    JS_ASSERT_IF(objFe->isTypeKnown(), objFe->getKnownType() == JSVAL_TYPE_OBJECT);

    MICGenInfo mic(ic::MICInfo::SET);
    RegisterID objReg;
    Jump shapeGuard;

    mic.entry = masm.label();
    if (objFe->isConstant()) {
        JSObject *obj = &objFe->getValue().toObject();
        JS_ASSERT(obj->isNative());

        objReg = frame.allocReg();

        masm.load32FromImm(&obj->objShape, objReg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, objReg,
                                            Imm32(int32(JSObjectMap::INVALID_SHAPE)),
                                            mic.shape);
        masm.move(ImmPtr(obj), objReg);
    } else {
        objReg = frame.copyDataIntoReg(objFe);
        RegisterID reg = frame.allocReg();

        masm.loadShape(objReg, reg);
        shapeGuard = masm.branch32WithPatch(Assembler::NotEqual, reg,
                                            Imm32(int32(JSObjectMap::INVALID_SHAPE)),
                                            mic.shape);
        frame.freeReg(reg);
    }
    stubcc.linkExit(shapeGuard, Uses(2));

    stubcc.leave();
    passMICAddress(mic);
    mic.stubEntry = stubcc.masm.label();
    mic.call = stubcc.call(ic::SetGlobalName);

    
    uint32 slot = 1 << 24;

    
    FrameEntry *fe = frame.peek(-1);

    Value v;
    RegisterID typeReg = Registers::ReturnReg;
    RegisterID dataReg = Registers::ReturnReg;
    JSValueType typeTag = JSVAL_TYPE_INT32;

    mic.u.name.typeConst = fe->isTypeKnown();
    mic.u.name.dataConst = fe->isConstant();

    if (!mic.u.name.dataConst) {
        dataReg = frame.ownRegForData(fe);
        if (!mic.u.name.typeConst)
            typeReg = frame.ownRegForType(fe);
        else
            typeTag = fe->getKnownType();
    } else {
        v = fe->getValue();
    }

    masm.loadPtr(Address(objReg, offsetof(JSObject, slots)), objReg);
    Address address(objReg, slot);

    mic.load = masm.label();

#if defined JS_CPU_ARM
    DataLabel32 offsetAddress;
    if (mic.u.name.dataConst) {
        offsetAddress = masm.moveWithPatch(Imm32(address.offset), JSC::ARMRegisters::S0);
        masm.add32(address.base, JSC::ARMRegisters::S0);
        masm.storeValue(v, Address(JSC::ARMRegisters::S0, 0));
    } else {
        if (mic.u.name.typeConst) {
            offsetAddress = masm.store64WithAddressOffsetPatch(ImmType(typeTag), dataReg, address);
        } else {
            offsetAddress = masm.store64WithAddressOffsetPatch(typeReg, dataReg, address);
        }
    }
    JS_ASSERT(masm.differenceBetween(mic.load, offsetAddress) == 0);
#else
    if (mic.u.name.dataConst) {
        masm.storeValue(v, address);
    } else if (mic.u.name.typeConst) {
        masm.storeValueFromComponents(ImmType(typeTag), dataReg, address);
    } else {
        masm.storeValueFromComponents(typeReg, dataReg, address);
    }
#endif

#if defined JS_PUNBOX64
    





    mic.patchValueOffset = masm.differenceBetween(mic.load, masm.label());
    JS_ASSERT(mic.patchValueOffset == masm.differenceBetween(mic.load, masm.label()));
#endif

    frame.freeReg(objReg);
    frame.popn(2);
    if (mic.u.name.dataConst) {
        frame.push(v);
    } else {
        if (mic.u.name.typeConst)
            frame.pushTypedPayload(typeTag, dataReg);
        else
            frame.pushRegs(typeReg, dataReg);
    }

    stubcc.rejoin(Changes(1));

    mics.append(mic);
#else
    jsop_setgname_slow(index);
#endif
}

void
mjit::Compiler::jsop_setelem_slow()
{
    prepareStubCall(Uses(3));
    stubCall(STRICT_VARIANT(stubs::SetElem));
    frame.popn(3);
    frame.pushSynced();
}

void
mjit::Compiler::jsop_getelem_slow()
{
    prepareStubCall(Uses(2));
    stubCall(stubs::GetElem);
    frame.popn(2);
    frame.pushSynced();
}

void
mjit::Compiler::jsop_unbrand()
{
    prepareStubCall(Uses(1));
    stubCall(stubs::Unbrand);
}

bool
mjit::Compiler::jsop_instanceof()
{
    FrameEntry *lhs = frame.peek(-2);
    FrameEntry *rhs = frame.peek(-1);

    
    if (rhs->isNotType(JSVAL_TYPE_OBJECT) || lhs->isNotType(JSVAL_TYPE_OBJECT)) {
        prepareStubCall(Uses(2));
        stubCall(stubs::InstanceOf);
        frame.popn(2);
        frame.takeReg(Registers::ReturnReg);
        frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, Registers::ReturnReg);
        return true;
    }

    MaybeJump firstSlow;
    if (!rhs->isTypeKnown()) {
        Jump j = frame.testObject(Assembler::NotEqual, rhs);
        stubcc.linkExit(j, Uses(2));
        RegisterID reg = frame.tempRegForData(rhs);
        j = masm.testFunction(Assembler::NotEqual, reg);
        stubcc.linkExit(j, Uses(2));
    }

    
    RegisterID obj = frame.tempRegForData(rhs);
    Jump isBound = masm.branchTest32(Assembler::NonZero, Address(obj, offsetof(JSObject, flags)),
                                     Imm32(JSObject::BOUND_FUNCTION));
    {
        stubcc.linkExit(isBound, Uses(2));
        stubcc.leave();
        stubcc.call(stubs::InstanceOf);
        firstSlow = stubcc.masm.jump();
    }
    

    
    frame.dup();

    if (!jsop_getprop(cx->runtime->atomState.classPrototypeAtom, false))
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

    
    masm.loadPtr(Address(obj, offsetof(JSObject, clasp)), temp);
    masm.loadPtr(Address(temp, offsetof(Class, ext) +
                              offsetof(ClassExtension, wrappedObject)), temp);
    j = masm.branchTestPtr(Assembler::NonZero, temp, temp);
    stubcc.linkExit(j, Uses(3));

    Address protoAddr(obj, offsetof(JSObject, proto));
    Label loop = masm.label();

    
    masm.loadPayload(protoAddr, obj);
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
    stubcc.call(stubs::FastInstanceOf);

    frame.popn(3);
    frame.pushTypedPayload(JSVAL_TYPE_BOOLEAN, temp);

    if (firstSlow.isSet())
        firstSlow.getJump().linkTo(stubcc.masm.label(), &stubcc.masm);
    stubcc.rejoin(Changes(1));
    return true;
}







void
mjit::Compiler::jumpAndTrace(Jump j, jsbytecode *target, Jump *slow)
{
#ifndef JS_TRACER
    jumpInScript(j, target);
    if (slow)
        stubcc.jumpInScript(*slow, target);
#else
    if (!addTraceHints || target >= PC || JSOp(*target) != JSOP_TRACE
#ifdef JS_MONOIC
        || GET_UINT16(target) == BAD_TRACEIC_INDEX
#endif
        )
    {
        jumpInScript(j, target);
        if (slow)
            stubcc.jumpInScript(*slow, target);
        return;
    }

# if JS_MONOIC
    TraceGenInfo ic;

    ic.initialized = true;
    ic.stubEntry = stubcc.masm.label();
    ic.jumpTarget = target;
    ic.traceHint = j;
    if (slow)
        ic.slowTraceHint = *slow;

    uint16 index = GET_UINT16(target);
    if (traceICs.length() <= index)
        traceICs.resize(index+1);
# endif

    Label traceStart = stubcc.masm.label();

    stubcc.linkExitDirect(j, traceStart);
    if (slow)
        slow->linkTo(traceStart, &stubcc.masm);
# if JS_MONOIC
    ic.addrLabel = stubcc.masm.moveWithPatch(ImmPtr(NULL), Registers::ArgReg1);
    traceICs[index] = ic;
# endif

    
    {
        jsbytecode* pc = PC;
        PC = target;

        stubcc.call(stubs::InvokeTracer);

        PC = pc;
    }

    Jump no = stubcc.masm.branchTestPtr(Assembler::Zero, Registers::ReturnReg,
                                        Registers::ReturnReg);
    restoreFrameRegs(stubcc.masm);
    stubcc.masm.jump(Registers::ReturnReg);
    no.linkTo(stubcc.masm.label(), &stubcc.masm);
    stubcc.jumpInScript(stubcc.masm.jump(), target);
#endif
}

void
mjit::Compiler::enterBlock(JSObject *obj)
{
    
    
    
    
    if (analysis[PC].exceptionEntry)
        restoreFrameRegs(masm);

    uint32 oldFrameDepth = frame.frameDepth();

    
    frame.syncAndForgetEverything();
    masm.move(ImmPtr(obj), Registers::ArgReg1);
    uint32 n = js_GetEnterBlockStackDefs(cx, script, PC);
    stubCall(stubs::EnterBlock);
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
    stubCall(stubs::LeaveBlock);
    frame.leaveBlock(n);
}









bool
mjit::Compiler::constructThis()
{
    JS_ASSERT(isConstructing);

    
    Address callee(JSFrameReg, JSStackFrame::offsetOfCallee(fun));
    RegisterID calleeReg = frame.allocReg();
    masm.loadPayload(callee, calleeReg);
    frame.pushTypedPayload(JSVAL_TYPE_OBJECT, calleeReg);

    
    if (!jsop_getprop(cx->runtime->atomState.classPrototypeAtom, false, false))
        return false;

    
    FrameEntry *protoFe = frame.peek(-1);
    RegisterID protoReg = frame.ownRegForData(protoFe);

    
    Jump isNotObject = frame.testObject(Assembler::NotEqual, protoFe);
    stubcc.linkExitDirect(isNotObject, stubcc.masm.label());
    stubcc.masm.move(ImmPtr(NULL), protoReg);
    stubcc.crossJump(stubcc.masm.jump(), masm.label());

    
    frame.pop();

    prepareStubCall(Uses(0));
    if (protoReg != Registers::ArgReg1)
        masm.move(protoReg, Registers::ArgReg1);
    stubCall(stubs::CreateThis);
    frame.freeReg(protoReg);
    return true;
}

