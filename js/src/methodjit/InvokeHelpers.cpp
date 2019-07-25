







































#include "jscntxt.h"
#include "jsscope.h"
#include "jsobj.h"
#include "jslibmath.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsxml.h"
#include "jsbool.h"
#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/assembler/CodeLocation.h"
#include "jsiter.h"
#include "jstypes.h"
#include "methodjit/StubCalls.h"
#include "jstracer.h"
#include "jspropertycache.h"
#include "methodjit/MonoIC.h"
#include "jsanalyze.h"
#include "methodjit/BaseCompiler.h"
#include "methodjit/ICRepatcher.h"
#include "vm/Debugger.h"

#include "jsinterpinlines.h"
#include "jspropertycacheinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsobjinlines.h"
#include "jscntxtinlines.h"
#include "jsatominlines.h"
#include "StubCalls-inl.h"
#include "MethodJIT-inl.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;
using namespace JSC;

using ic::Repatcher;

static jsbytecode *
FindExceptionHandler(JSContext *cx)
{
    StackFrame *fp = cx->fp();
    JSScript *script = fp->script();

top:
    if (cx->isExceptionPending() && JSScript::isValidOffset(script->trynotesOffset)) {
        
        unsigned offset = cx->regs().pc - script->main();

        JSTryNoteArray *tnarray = script->trynotes();
        for (unsigned i = 0; i < tnarray->length; ++i) {
            JSTryNote *tn = &tnarray->vector[i];

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (offset - tn->start > tn->length)
                continue;
            if (tn->stackDepth > cx->regs().sp - fp->base())
                continue;

            jsbytecode *pc = script->main() + tn->start + tn->length;
            cx->regs().pc = pc;
            JSBool ok = UnwindScope(cx, tn->stackDepth, JS_TRUE);
            JS_ASSERT(cx->regs().sp == fp->base() + tn->stackDepth);

            switch (tn->kind) {
                case JSTRY_CATCH:
                  JS_ASSERT(js_GetOpcode(cx, fp->script(), pc) == JSOP_ENTERBLOCK);

#if JS_HAS_GENERATORS
                  
                  if (JS_UNLIKELY(cx->getPendingException().isMagic(JS_GENERATOR_CLOSING)))
                      break;
#endif

                  




                  return pc;

                case JSTRY_FINALLY:
                  



                  cx->regs().sp[0].setBoolean(true);
                  cx->regs().sp[1] = cx->getPendingException();
                  cx->regs().sp += 2;
                  cx->clearPendingException();
                  return pc;

                case JSTRY_ITER:
                {
                  






                  Value v = cx->getPendingException();
                  JS_ASSERT(js_GetOpcode(cx, fp->script(), pc) == JSOP_ENDITER);
                  cx->clearPendingException();
                  ok = !!js_CloseIterator(cx, &cx->regs().sp[-1].toObject());
                  cx->regs().sp -= 1;
                  if (!ok)
                      goto top;
                  cx->setPendingException(v);
                }
            }
        }
    }

    return NULL;
}




static void
InlineReturn(VMFrame &f)
{
    JS_ASSERT(f.fp() != f.entryfp);
    JS_ASSERT(!IsActiveWithOrBlock(f.cx, f.fp()->scopeChain(), 0));
    f.cx->stack.popInlineFrame(f.regs);

    DebugOnly<JSOp> op = js_GetOpcode(f.cx, f.fp()->script(), f.regs.pc);
    JS_ASSERT(op == JSOP_CALL ||
              op == JSOP_NEW ||
              op == JSOP_EVAL ||
              op == JSOP_FUNCALL ||
              op == JSOP_FUNAPPLY);
    f.regs.pc += JSOP_CALL_LENGTH;
}

void JS_FASTCALL
stubs::SlowCall(VMFrame &f, uint32 argc)
{
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    if (!InvokeKernel(f.cx, args))
        THROW();

    types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
}

void JS_FASTCALL
stubs::SlowNew(VMFrame &f, uint32 argc)
{
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    if (!InvokeConstructorKernel(f.cx, args))
        THROW();

    types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
}

static inline bool
CheckStackQuota(VMFrame &f)
{
    JS_ASSERT(f.regs.sp == f.fp()->base());

    f.stackLimit = f.cx->stack.space().getStackLimit(f.cx, DONT_REPORT_ERROR);
    if (f.stackLimit)
        return true;

    
    f.cx->stack.popFrameAfterOverflow();
    js_ReportOverRecursed(f.cx);

    return false;
}





void JS_FASTCALL
stubs::HitStackQuota(VMFrame &f)
{
    if (!CheckStackQuota(f))
        THROW();
}





void * JS_FASTCALL
stubs::FixupArity(VMFrame &f, uint32 nactual)
{
    JSContext *cx = f.cx;
    StackFrame *oldfp = f.fp();

    JS_ASSERT(nactual != oldfp->numFormalArgs());

    





    InitialFrameFlags initial = oldfp->initialFlags();
    JSFunction *fun           = oldfp->fun();
    JSScript *script          = fun->script();
    void *ncode               = oldfp->nativeReturnAddress();

    
    f.regs.popPartialFrame((Value *)oldfp);

    
    CallArgs args = CallArgsFromSp(nactual, f.regs.sp);
    StackFrame *fp = cx->stack.getFixupFrame(cx, DONT_REPORT_ERROR, args, fun,
                                             script, ncode, initial, &f.stackLimit);

    if (!fp) {
        f.regs.updateForNcode(f.jit(), ncode);
        js_ReportOverRecursed(cx);
        THROWV(NULL);
    }

    
    return fp;
}

struct ResetStubRejoin {
    VMFrame &f;
    ResetStubRejoin(VMFrame &f) : f(f) {}
    ~ResetStubRejoin() { f.stubRejoin = 0; }
};

void * JS_FASTCALL
stubs::CompileFunction(VMFrame &f, uint32 argc)
{
    




    JS_ASSERT_IF(f.cx->typeInferenceEnabled(), f.stubRejoin);
    ResetStubRejoin reset(f);

    InitialFrameFlags initial = f.fp()->initialFlags();
    f.regs.popPartialFrame((Value *)f.fp());

    if (InitialFrameFlagsAreConstructing(initial))
        return UncachedNew(f, argc);
    else if (InitialFrameFlagsAreLowered(initial))
        return UncachedLoweredCall(f, argc);
    else
        return UncachedCall(f, argc);
}

static inline bool
UncachedInlineCall(VMFrame &f, InitialFrameFlags initial,
                   void **pret, bool *unjittable, uint32 argc)
{
    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    JSObject &callee = args.callee();
    JSFunction *newfun = callee.getFunctionPrivate();
    JSScript *newscript = newfun->script();

    bool construct = InitialFrameFlagsAreConstructing(initial);

    bool newType = construct && cx->typeInferenceEnabled() &&
        types::UseNewType(cx, f.script(), f.pc());

    types::TypeMonitorCall(cx, args, construct);

    
    if (newscript->getJITStatus(construct) == JITScript_None) {
        CompileStatus status = CanMethodJIT(cx, newscript, construct, CompileRequest_Interpreter);
        if (status == Compile_Error) {
            
            return false;
        }
        if (status == Compile_Abort)
            *unjittable = true;
    }

    




    if (f.regs.inlined() && newfun->isHeavyweight()) {
        ExpandInlineFrames(cx->compartment);
        JS_ASSERT(!f.regs.inlined());
    }

    






    FrameRegs regs = f.regs;

    
    if (!cx->stack.pushInlineFrame(cx, regs, args, callee, newfun, newscript, initial, &f.stackLimit))
        return false;

    
    PreserveRegsGuard regsGuard(cx, regs);

    
    if (!regs.fp()->functionPrologue(cx))
        return false;

    



    if (!newType) {
        if (JITScript *jit = newscript->getJIT(regs.fp()->isConstructing())) {
            *pret = jit->invokeEntry;

            
            regs.popFrame((Value *) regs.fp());
            return true;
        }
    }

    





    if (f.regs.inlined()) {
        ExpandInlineFrames(cx->compartment);
        JS_ASSERT(!f.regs.inlined());
        regs.fp()->resetInlinePrev(f.fp(), f.regs.pc);
    }

    bool ok = !!Interpret(cx, cx->fp());
    f.cx->stack.popInlineFrame(regs);

    if (ok)
        types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());

    *pret = NULL;
    return ok;
}

void * JS_FASTCALL
stubs::UncachedNew(VMFrame &f, uint32 argc)
{
    UncachedCallResult ucr;
    UncachedNewHelper(f, argc, &ucr);
    return ucr.codeAddr;
}

void
stubs::UncachedNewHelper(VMFrame &f, uint32 argc, UncachedCallResult *ucr)
{
    ucr->init();
    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);

    
    if (IsFunctionObject(args.calleev(), &ucr->fun) && ucr->fun->isInterpretedConstructor()) {
        ucr->callee = &args.callee();
        if (!UncachedInlineCall(f, INITIAL_CONSTRUCT, &ucr->codeAddr, &ucr->unjittable, argc))
            THROW();
    } else {
        if (!InvokeConstructorKernel(cx, args))
            THROW();
        types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
    }
}

void * JS_FASTCALL
stubs::UncachedCall(VMFrame &f, uint32 argc)
{
    UncachedCallResult ucr;
    UncachedCallHelper(f, argc, false, &ucr);
    return ucr.codeAddr;
}

void * JS_FASTCALL
stubs::UncachedLoweredCall(VMFrame &f, uint32 argc)
{
    UncachedCallResult ucr;
    UncachedCallHelper(f, argc, true, &ucr);
    return ucr.codeAddr;
}

void JS_FASTCALL
stubs::Eval(VMFrame &f, uint32 argc)
{
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);

    if (!IsBuiltinEvalForScope(&f.fp()->scopeChain(), args.calleev())) {
        if (!InvokeKernel(f.cx, args))
            THROW();

        types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
        return;
    }

    JS_ASSERT(f.fp() == f.cx->fp());
    if (!DirectEval(f.cx, args))
        THROW();

    types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
}

void
stubs::UncachedCallHelper(VMFrame &f, uint32 argc, bool lowered, UncachedCallResult *ucr)
{
    ucr->init();

    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);

    if (IsFunctionObject(args.calleev(), &ucr->callee)) {
        ucr->callee = &args.callee();
        ucr->fun = ucr->callee->getFunctionPrivate();

        if (ucr->fun->isInterpreted()) {
            InitialFrameFlags initial = lowered ? INITIAL_LOWERED : INITIAL_NONE;
            if (!UncachedInlineCall(f, initial, &ucr->codeAddr, &ucr->unjittable, argc))
                THROW();
            return;
        }

        if (ucr->fun->isNative()) {
            if (!CallJSNative(cx, ucr->fun->u.n.native, args))
                THROW();
            types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
            return;
        }
    }

    if (!InvokeKernel(f.cx, args))
        THROW();

    types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
    return;
}

static void
RemoveOrphanedNative(JSContext *cx, StackFrame *fp)
{
    





    JaegerCompartment *jc = cx->compartment->jaegerCompartment();
    if (jc->orphanedNativeFrames.empty())
        return;
    for (unsigned i = 0; i < jc->orphanedNativeFrames.length(); i++) {
        if (fp == jc->orphanedNativeFrames[i]) {
            jc->orphanedNativeFrames[i] = jc->orphanedNativeFrames.back();
            jc->orphanedNativeFrames.popBack();
            break;
        }
    }
    if (jc->orphanedNativeFrames.empty()) {
        for (unsigned i = 0; i < jc->orphanedNativePools.length(); i++)
            jc->orphanedNativePools[i]->release();
        jc->orphanedNativePools.clear();
    }
}

extern "C" void *
js_InternalThrow(VMFrame &f)
{
    JSContext *cx = f.cx;

    ExpandInlineFrames(cx->compartment);

    
    
    RemoveOrphanedNative(cx, f.fp());

    
    
    
    
    
    
    
    
    if (f.fp()->finishedInInterpreter()) {
        
        if (f.fp() == f.entryfp)
            return NULL;

        InlineReturn(f);
    }

    
    JS_ASSERT(&cx->regs() == &f.regs);

    jsbytecode *pc = NULL;
    for (;;) {
        if (cx->isExceptionPending()) {
            
            JSThrowHook handler = cx->debugHooks->throwHook;
            if (handler || !cx->compartment->getDebuggees().empty()) {
                Value rval;
                JSTrapStatus st = Debugger::onExceptionUnwind(cx, &rval);
                if (st == JSTRAP_CONTINUE && handler) {
                    st = handler(cx, cx->fp()->script(), cx->regs().pc, &rval,
                                 cx->debugHooks->throwHookData);
                }

                switch (st) {
                case JSTRAP_ERROR:
                    cx->clearPendingException();
                    return NULL;

                case JSTRAP_RETURN:
                    cx->clearPendingException();
                    cx->fp()->setReturnValue(rval);
                    return cx->jaegerCompartment()->forceReturnFromExternC();

                case JSTRAP_THROW:
                    cx->setPendingException(rval);
                    break;

                default:
                    break;
                }
            }
        }

        pc = FindExceptionHandler(cx);
        if (pc)
            break;

        
        
        
        
        
        JS_ASSERT(!f.fp()->finishedInInterpreter());
        UnwindScope(cx, 0, cx->isExceptionPending());
        ScriptEpilogue(f.cx, f.fp(), false);

        
        
        
        if (f.entryfp == f.fp())
            break;

        JS_ASSERT(f.regs.sp == cx->regs().sp);
        InlineReturn(f);
    }

    JS_ASSERT(f.regs.sp == cx->regs().sp);

    if (!pc)
        return NULL;

    StackFrame *fp = cx->fp();
    JSScript *script = fp->script();

    if (cx->typeInferenceEnabled() || !fp->jit()) {
        






        cx->compartment->jaegerCompartment()->setLastUnfinished(Jaeger_Unfinished);

        if (!script->ensureRanAnalysis(cx)) {
            js_ReportOutOfMemory(cx);
            return NULL;
        }

        analyze::AutoEnterAnalysis enter(cx);

        cx->regs().pc = pc;
        cx->regs().sp = fp->base() + script->analysis()->getCode(pc).stackDepth;

        




        if (cx->isExceptionPending()) {
            JS_ASSERT(js_GetOpcode(cx, script, pc) == JSOP_ENTERBLOCK);
            JSObject *obj = script->getObject(GET_SLOTNO(pc));
            Value *vp = cx->regs().sp + OBJ_BLOCK_COUNT(cx, obj);
            SetValueRangeToUndefined(cx->regs().sp, vp);
            cx->regs().sp = vp;
            JS_ASSERT(js_GetOpcode(cx, script, pc + JSOP_ENTERBLOCK_LENGTH) == JSOP_EXCEPTION);
            cx->regs().sp[0] = cx->getPendingException();
            cx->clearPendingException();
            cx->regs().sp++;
            cx->regs().pc = pc + JSOP_ENTERBLOCK_LENGTH + JSOP_EXCEPTION_LENGTH;
        }

        *f.oldregs = f.regs;

        return NULL;
    }

    return script->nativeCodeForPC(fp->isConstructing(), pc);
}

void JS_FASTCALL
stubs::CreateThis(VMFrame &f, JSObject *proto)
{
    JSContext *cx = f.cx;
    StackFrame *fp = f.fp();
    JSObject *callee = &fp->callee();
    JSObject *obj = js_CreateThisForFunctionWithProto(cx, callee, proto);
    if (!obj)
        THROW();
    fp->formalArgs()[-1].setObject(*obj);
}

void JS_FASTCALL
stubs::ScriptDebugPrologue(VMFrame &f)
{
    Probes::enterJSFun(f.cx, f.fp()->maybeFun(), f.fp()->script());
    js::ScriptDebugPrologue(f.cx, f.fp());
}

void JS_FASTCALL
stubs::ScriptDebugEpilogue(VMFrame &f)
{
    Probes::exitJSFun(f.cx, f.fp()->maybeFun(), f.fp()->script());
    if (!js::ScriptDebugEpilogue(f.cx, f.fp(), JS_TRUE))
        THROW();
}

void JS_FASTCALL
stubs::ScriptProbeOnlyPrologue(VMFrame &f)
{
    Probes::enterJSFun(f.cx, f.fp()->fun(), f.fp()->script());
}

void JS_FASTCALL
stubs::ScriptProbeOnlyEpilogue(VMFrame &f)
{
    Probes::exitJSFun(f.cx, f.fp()->fun(), f.fp()->script());
}

#ifdef JS_TRACER






static inline bool
HandleErrorInExcessFrame(VMFrame &f, StackFrame *stopFp, bool searchedTopmostFrame = true)
{
    JSContext *cx = f.cx;

    






    StackFrame *fp = cx->fp();
    if (searchedTopmostFrame) {
        






        if (fp == stopFp)
            return false;

        



        InlineReturn(f);
    }

    
    bool returnOK = false;
    for (;;) {
        fp = cx->fp();

        
        if (fp->hasImacropc()) {
            cx->regs().pc = fp->imacropc();
            fp->clearImacropc();
        }
        JS_ASSERT(!fp->hasImacropc());

        
        if (cx->isExceptionPending()) {
            jsbytecode *pc = FindExceptionHandler(cx);
            if (pc) {
                cx->regs().pc = pc;
                returnOK = true;
                break;
            }
        }

        
        if (fp == stopFp)
            break;

        
        returnOK &= UnwindScope(cx, 0, returnOK || cx->isExceptionPending());
        returnOK = ScriptEpilogue(cx, fp, returnOK);
        InlineReturn(f);
    }

    JS_ASSERT(&f.regs == &cx->regs());
    JS_ASSERT_IF(!returnOK, cx->fp() == stopFp);

    return returnOK;
}


static inline void *
AtSafePoint(JSContext *cx)
{
    StackFrame *fp = cx->fp();
    if (fp->hasImacropc())
        return NULL;

    JSScript *script = fp->script();
    return script->maybeNativeCodeForPC(fp->isConstructing(), cx->regs().pc);
}





static inline JSBool
PartialInterpret(VMFrame &f)
{
    JSContext *cx = f.cx;
    StackFrame *fp = cx->fp();

#ifdef DEBUG
    JSScript *script = fp->script();
    JS_ASSERT(!fp->finishedInInterpreter());
    JS_ASSERT(fp->hasImacropc() ||
              !script->maybeNativeCodeForPC(fp->isConstructing(), cx->regs().pc));
#endif

    JSBool ok = JS_TRUE;
    ok = Interpret(cx, fp, JSINTERP_SAFEPOINT);

    return ok;
}

JS_STATIC_ASSERT(JSOP_NOP == 0);









static inline bool
FrameIsFinished(JSContext *cx)
{
    JSOp op = JSOp(*cx->regs().pc);
    return (op == JSOP_RETURN ||
            op == JSOP_RETRVAL ||
            op == JSOP_STOP)
        ? true
        : cx->fp()->finishedInInterpreter();
}








static bool
HandleFinishedFrame(VMFrame &f, StackFrame *entryFrame)
{
    JSContext *cx = f.cx;

    JS_ASSERT(FrameIsFinished(cx));

    


























    bool returnOK = true;
    if (!cx->fp()->finishedInInterpreter()) {
        if (JSOp(*cx->regs().pc) == JSOP_RETURN)
            cx->fp()->setReturnValue(f.regs.sp[-1]);

        returnOK = ScriptEpilogue(cx, cx->fp(), true);
    }

    if (cx->fp() != entryFrame) {
        InlineReturn(f);
    }

    return returnOK;
}
















static bool
EvaluateExcessFrame(VMFrame &f, StackFrame *entryFrame)
{
    JSContext *cx = f.cx;
    StackFrame *fp = cx->fp();

    





    if (!fp->hasImacropc() && FrameIsFinished(cx))
        return HandleFinishedFrame(f, entryFrame);

    if (void *ncode = AtSafePoint(cx)) {
        if (!JaegerShotAtSafePoint(cx, ncode, false))
            return false;
        InlineReturn(f);
        return true;
    }

    return PartialInterpret(f);
}





static bool
FinishExcessFrames(VMFrame &f, StackFrame *entryFrame)
{
    JSContext *cx = f.cx;

    while (cx->fp() != entryFrame || entryFrame->hasImacropc()) {
        if (!EvaluateExcessFrame(f, entryFrame)) {
            if (!HandleErrorInExcessFrame(f, entryFrame))
                return false;
        }
    }

    return true;
}

#if defined JS_MONOIC
static void
UpdateTraceHintSingle(Repatcher &repatcher, JSC::CodeLocationJump jump, JSC::CodeLocationLabel target)
{
    




    repatcher.relink(jump, target);

    JaegerSpew(JSpew_PICs, "relinking trace hint %p to %p\n",
               jump.executableAddress(), target.executableAddress());
}

static void
DisableTraceHint(JITScript *jit, ic::TraceICInfo &ic)
{
    Repatcher repatcher(jit);
    UpdateTraceHintSingle(repatcher, ic.traceHint, ic.fastTarget);

    if (ic.hasSlowTraceHint)
        UpdateTraceHintSingle(repatcher, ic.slowTraceHint, ic.slowTarget);
}

static void
ResetTraceHintAt(JSScript *script, js::mjit::JITScript *jit,
                 jsbytecode *pc, uint16_t index, bool full)
{
    if (index >= jit->nTraceICs)
        return;
    ic::TraceICInfo &ic = jit->traceICs()[index];
    if (!ic.initialized)
        return;
    
    JS_ASSERT(ic.jumpTargetPC == pc);

    JaegerSpew(JSpew_PICs, "Enabling trace IC %u in script %p\n", index,
               static_cast<void*>(script));

    Repatcher repatcher(jit);

    UpdateTraceHintSingle(repatcher, ic.traceHint, ic.stubEntry);

    if (ic.hasSlowTraceHint)
        UpdateTraceHintSingle(repatcher, ic.slowTraceHint, ic.stubEntry);

    if (full) {
        ic.traceData = NULL;
        ic.loopCounterStart = 1;
        ic.loopCounter = ic.loopCounterStart;
    }
}
#endif

void
js::mjit::ResetTraceHint(JSScript *script, jsbytecode *pc, uint16_t index, bool full)
{
#if JS_MONOIC
    if (script->jitNormal)
        ResetTraceHintAt(script, script->jitNormal, pc, index, full);

    if (script->jitCtor)
        ResetTraceHintAt(script, script->jitCtor, pc, index, full);
#endif
}

#if JS_MONOIC
void *
RunTracer(VMFrame &f, ic::TraceICInfo &ic)
#else
void *
RunTracer(VMFrame &f)
#endif
{
    JSContext *cx = f.cx;
    StackFrame *entryFrame = f.fp();
    TracePointAction tpa;

    
    if (!cx->traceJitEnabled)
        return NULL;

    






    entryFrame->scopeChain();
    entryFrame->returnValue();

    bool blacklist;
    void **traceData;
    uintN *traceEpoch;
    uint32 *loopCounter;
    uint32 hits;
#if JS_MONOIC
    traceData = &ic.traceData;
    traceEpoch = &ic.traceEpoch;
    loopCounter = &ic.loopCounter;
    *loopCounter = 1;
    hits = ic.loopCounterStart;
#else
    traceData = NULL;
    traceEpoch = NULL;
    loopCounter = NULL;
    hits = 1;
#endif

    {
        







        FrameRegs regs = f.regs;
        PreserveRegsGuard regsGuard(cx, regs);

        tpa = MonitorTracePoint(f.cx, &blacklist, traceData, traceEpoch,
                                loopCounter, hits);
        JS_ASSERT(!TRACE_RECORDER(cx));
    }

#if JS_MONOIC
    ic.loopCounterStart = *loopCounter;
    if (blacklist)
        DisableTraceHint(entryFrame->jit(), ic);
#endif

    
    
    JS_ASSERT_IF(cx->isExceptionPending(), tpa == TPA_Error);

    JS_ASSERT(f.fp() == cx->fp());
    switch (tpa) {
      case TPA_Nothing:
        return NULL;

      case TPA_Error:
        if (!HandleErrorInExcessFrame(f, entryFrame, f.fp()->finishedInInterpreter()))
            THROWV(NULL);
        JS_ASSERT(!cx->fp()->hasImacropc());
        break;

      case TPA_RanStuff:
      case TPA_Recorded:
        break;
    }

    























  restart:
    
    if (!FinishExcessFrames(f, entryFrame))
        THROWV(NULL);

    
    JS_ASSERT(f.fp() == entryFrame);
    JS_ASSERT(!entryFrame->hasImacropc());

    
    if (FrameIsFinished(cx)) {
        if (!HandleFinishedFrame(f, entryFrame))
            THROWV(NULL);
        *f.returnAddressLocation() = cx->jaegerCompartment()->forceReturnFromFastCall();
        return NULL;
    }

    
    if (void *ncode = AtSafePoint(cx))
        return ncode;

    
    if (!PartialInterpret(f)) {
        if (!HandleErrorInExcessFrame(f, entryFrame))
            THROWV(NULL);
    }

    goto restart;
}

#endif 

#if defined JS_TRACER
# if defined JS_MONOIC
void *JS_FASTCALL
stubs::InvokeTracer(VMFrame &f, ic::TraceICInfo *ic)
{
    return RunTracer(f, *ic);
}

# else

void *JS_FASTCALL
stubs::InvokeTracer(VMFrame &f)
{
    return RunTracer(f);
}
# endif 
#endif 


#if defined(JS_METHODJIT_SPEW)
static const char *OpcodeNames[] = {
# define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) #name,
# include "jsopcode.tbl"
# undef OPDEF
};
#endif

static void
FinishVarIncOp(VMFrame &f, RejoinState rejoin, Value ov, Value nv, Value *vp)
{
    
    JS_ASSERT(rejoin == REJOIN_POS || rejoin == REJOIN_BINARY);

    JSContext *cx = f.cx;

    JSOp op = js_GetOpcode(cx, f.script(), f.pc());
    const JSCodeSpec *cs = &js_CodeSpec[op];

    unsigned i = GET_SLOTNO(f.pc());
    Value *var = (JOF_TYPE(cs->format) == JOF_LOCAL) ? f.fp()->slots() + i : &f.fp()->formalArg(i);

    if (rejoin == REJOIN_POS) {
        double d = ov.toNumber();
        double N = (cs->format & JOF_INC) ? 1 : -1;
        if (!nv.setNumber(d + N))
            types::TypeScript::MonitorOverflow(cx, f.script(), f.pc());
    }

    *var = nv;
    *vp = (cs->format & JOF_POST) ? ov : nv;
}

extern "C" void *
js_InternalInterpret(void *returnData, void *returnType, void *returnReg, js::VMFrame &f)
{
    JSRejoinState jsrejoin = f.fp()->rejoin();
    RejoinState rejoin;
    if (jsrejoin & 0x1) {
        
        uint32 pcOffset = jsrejoin >> 1;
        f.regs.pc = f.fp()->script()->code + pcOffset;
        f.regs.clearInlined();
        rejoin = REJOIN_SCRIPTED;
    } else {
        rejoin = (RejoinState) (jsrejoin >> 1);
    }

    JSContext *cx = f.cx;
    StackFrame *fp = f.regs.fp();
    JSScript *script = fp->script();

    jsbytecode *pc = f.regs.pc;
    analyze::UntrapOpcode untrap(cx, script, pc);

    JSOp op = JSOp(*pc);
    const JSCodeSpec *cs = &js_CodeSpec[op];

    if (!script->ensureRanAnalysis(cx)) {
        js_ReportOutOfMemory(cx);
        return js_InternalThrow(f);
    }

    analyze::AutoEnterAnalysis enter(cx);
    analyze::ScriptAnalysis *analysis = script->analysis();

    




    Value *oldsp = f.regs.sp;
    f.regs.sp = fp->base() + analysis->getCode(pc).stackDepth;

    jsbytecode *nextpc = pc + analyze::GetBytecodeLength(pc);
    Value *nextsp = NULL;
    if (nextpc != script->code + script->length && analysis->maybeCode(nextpc))
        nextsp = fp->base() + analysis->getCode(nextpc).stackDepth;

    JS_ASSERT(&cx->regs() == &f.regs);

#ifdef JS_METHODJIT_SPEW
    JaegerSpew(JSpew_Recompile, "interpreter rejoin (file \"%s\") (line \"%d\") (op %s) (opline \"%d\")\n",
               script->filename, script->lineno, OpcodeNames[op], js_PCToLineNumber(cx, script, pc));
#endif

    uint32 nextDepth = uint32(-1);
    bool skipTrap = false;

    if ((cs->format & (JOF_INC | JOF_DEC)) &&
        (rejoin == REJOIN_POS || rejoin == REJOIN_BINARY)) {
        




        JS_ASSERT(cs->format & (JOF_LOCAL | JOF_QARG));

        nextDepth = analysis->getCode(nextpc).stackDepth;
        untrap.retrap();
        enter.leave();

        if (rejoin != REJOIN_BINARY || !analysis->incrementInitialValueObserved(pc)) {
            
            FinishVarIncOp(f, rejoin, nextsp[-1], nextsp[-1], &nextsp[-1]);
        } else {
            
            FinishVarIncOp(f, rejoin, nextsp[-1], nextsp[0], &nextsp[-1]);
        }

        rejoin = REJOIN_FALLTHROUGH;
    }

    switch (rejoin) {
      case REJOIN_SCRIPTED: {
#ifdef JS_NUNBOX32
        uint64 rvalBits = ((uint64)returnType << 32) | (uint32)returnData;
#elif JS_PUNBOX64
        uint64 rvalBits = (uint64)returnType | (uint64)returnData;
#else
#error "Unknown boxing format"
#endif
        nextsp[-1].setRawBits(rvalBits);

        



        types::TypeScript::Monitor(cx, script, pc, nextsp[-1]);
        f.regs.pc = nextpc;
        break;
      }

      case REJOIN_NONE:
        JS_NOT_REACHED("Unpossible rejoin!");
        break;

      case REJOIN_RESUME:
        break;

      case REJOIN_TRAP:
        



        if (untrap.trap)
            skipTrap = true;
        break;

      case REJOIN_FALLTHROUGH:
        f.regs.pc = nextpc;
        break;

      case REJOIN_NATIVE:
      case REJOIN_NATIVE_LOWERED:
      case REJOIN_NATIVE_GETTER: {
        





        if (rejoin == REJOIN_NATIVE_LOWERED) {
            nextsp[-1] = nextsp[0];
        } else if (rejoin == REJOIN_NATIVE_GETTER) {
            if (js_CodeSpec[op].format & JOF_CALLOP) {
                




                if (nextsp[-2].isObject())
                    nextsp[-1] = nextsp[-2];
                nextsp[-2] = nextsp[0];
            } else {
                nextsp[-1] = nextsp[0];
            }
        }

        
        RemoveOrphanedNative(cx, fp);

        f.regs.pc = nextpc;
        break;
      }

      case REJOIN_PUSH_BOOLEAN:
        nextsp[-1].setBoolean(returnReg != NULL);
        f.regs.pc = nextpc;
        break;

      case REJOIN_PUSH_OBJECT:
        nextsp[-1].setObject(* (JSObject *) returnReg);
        f.regs.pc = nextpc;
        break;

      case REJOIN_DEFLOCALFUN:
        fp->slots()[GET_SLOTNO(pc)].setObject(* (JSObject *) returnReg);
        f.regs.pc = nextpc;
        break;

      case REJOIN_THIS_PROTOTYPE: {
        JSObject *callee = &fp->callee();
        JSObject *proto = f.regs.sp[0].isObject() ? &f.regs.sp[0].toObject() : NULL;
        JSObject *obj = js_CreateThisForFunctionWithProto(cx, callee, proto);
        if (!obj)
            return js_InternalThrow(f);
        fp->formalArgs()[-1].setObject(*obj);

        if (script->debugMode || Probes::callTrackingActive(cx))
            js::ScriptDebugPrologue(cx, fp);
        break;
      }

      case REJOIN_CHECK_ARGUMENTS:
        




        if (!CheckStackQuota(f))
            return js_InternalThrow(f);

        SetValueRangeToUndefined(fp->slots(), script->nfixed);

        if (!fp->functionPrologue(cx))
            return js_InternalThrow(f);
        

      case REJOIN_FUNCTION_PROLOGUE:
        fp->scopeChain();

        
        if (!ScriptPrologueOrGeneratorResume(cx, fp, types::UseNewTypeAtEntry(cx, fp)))
            return js_InternalThrow(f);
        break;

      case REJOIN_CALL_PROLOGUE:
      case REJOIN_CALL_PROLOGUE_LOWERED_CALL:
      case REJOIN_CALL_PROLOGUE_LOWERED_APPLY:
        if (returnReg) {
            uint32 argc = 0;
            if (rejoin == REJOIN_CALL_PROLOGUE)
                argc = GET_ARGC(pc);
            else if (rejoin == REJOIN_CALL_PROLOGUE_LOWERED_CALL)
                argc = GET_ARGC(pc) - 1;
            else
                argc = f.u.call.dynamicArgc;

            







            f.regs.restorePartialFrame(oldsp); 
            f.scratch = (void *) argc;         
            f.fp()->setNativeReturnAddress(JS_FUNC_TO_DATA_PTR(void *, JaegerInterpolineScripted));
            fp->setRejoin(REJOIN_SCRIPTED | ((pc - script->code) << 1));
            return returnReg;
        } else {
            




            f.regs.pc = nextpc;
            if (rejoin != REJOIN_CALL_PROLOGUE) {
                
                nextsp[-1] = nextsp[0];
            }
        }
        break;

      case REJOIN_CALL_SPLAT: {
        
        nextDepth = analysis->getCode(nextpc).stackDepth;
        untrap.retrap();
        enter.leave();
        f.regs.sp = nextsp + 2 + f.u.call.dynamicArgc;
        if (!InvokeKernel(cx, CallArgsFromSp(f.u.call.dynamicArgc, f.regs.sp)))
            return js_InternalThrow(f);
        nextsp[-1] = nextsp[0];
        f.regs.pc = nextpc;
        break;
      }

      case REJOIN_GETTER:
        



        switch (op) {
          case JSOP_NAME:
          case JSOP_GETGNAME:
          case JSOP_GETFCSLOT:
          case JSOP_GETPROP:
          case JSOP_GETXPROP:
          case JSOP_LENGTH:
            
            f.regs.pc = nextpc;
            break;

          case JSOP_CALLGNAME:
          case JSOP_CALLNAME:
            if (!ComputeImplicitThis(cx, &fp->scopeChain(), nextsp[-2], &nextsp[-1]))
                return js_InternalThrow(f);
            f.regs.pc = nextpc;
            break;

          case JSOP_CALLFCSLOT:
            
            nextsp[-1].setUndefined();
            f.regs.pc = nextpc;
            break;

          case JSOP_CALLPROP: {
            



            JS_ASSERT(nextsp[-2].isString());
            Value tmp = nextsp[-2];
            nextsp[-2] = nextsp[-1];
            nextsp[-1] = tmp;
            f.regs.pc = nextpc;
            break;
          }

          case JSOP_INSTANCEOF: {
            




            if (f.regs.sp[0].isPrimitive()) {
                js_ReportValueError(cx, JSMSG_BAD_PROTOTYPE, -1, f.regs.sp[-1], NULL);
                return js_InternalThrow(f);
            }
            nextsp[-1].setBoolean(js_IsDelegate(cx, &f.regs.sp[0].toObject(), f.regs.sp[-2]));
            f.regs.pc = nextpc;
            break;
          }

          default:
            JS_NOT_REACHED("Bad rejoin getter op");
        }
        break;

      case REJOIN_POS:
        
        JS_ASSERT(op == JSOP_POS);
        f.regs.pc = nextpc;
        break;

      case REJOIN_BINARY:
        
        JS_ASSERT(op == JSOP_ADD || op == JSOP_SUB || op == JSOP_MUL || op == JSOP_DIV);
        f.regs.pc = nextpc;
        break;

      case REJOIN_BRANCH: {
        




        bool takeBranch = false;
        analyze::UntrapOpcode untrap(cx, script, nextpc);
        switch (JSOp(*nextpc)) {
          case JSOP_IFNE:
          case JSOP_IFNEX:
            takeBranch = returnReg != NULL;
            break;
          case JSOP_IFEQ:
          case JSOP_IFEQX:
            takeBranch = returnReg == NULL;
            break;
          default:
            JS_NOT_REACHED("Bad branch op");
        }
        if (takeBranch)
            f.regs.pc = nextpc + analyze::GetJumpOffset(nextpc, nextpc);
        else
            f.regs.pc = nextpc + analyze::GetBytecodeLength(nextpc);
        break;
      }

      default:
        JS_NOT_REACHED("Missing rejoin");
    }

    if (nextDepth == uint32(-1))
        nextDepth = analysis->getCode(f.regs.pc).stackDepth;
    f.regs.sp = fp->base() + nextDepth;

    




    if (f.regs.pc == nextpc && (js_CodeSpec[op].format & JOF_TYPESET)) {
        int which = (js_CodeSpec[op].format & JOF_CALLOP) ? -2 : -1;  
        types::TypeScript::Monitor(cx, script, pc, f.regs.sp[which]);
    }

    
    JaegerStatus status = skipTrap ? Jaeger_UnfinishedAtTrap : Jaeger_Unfinished;
    cx->compartment->jaegerCompartment()->setLastUnfinished(status);
    *f.oldregs = f.regs;

    return NULL;
}
