







































#include "jscntxt.h"
#include "jsscope.h"
#include "jsobj.h"
#include "jslibmath.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsxml.h"
#include "jsstaticcheck.h"
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

#include "jsinterpinlines.h"
#include "jspropertycacheinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsstrinlines.h"
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
        
        unsigned offset = cx->regs().pc - script->main;

        JSTryNoteArray *tnarray = script->trynotes();
        for (unsigned i = 0; i < tnarray->length; ++i) {
            JSTryNote *tn = &tnarray->vector[i];

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (offset - tn->start > tn->length)
                continue;
            if (tn->stackDepth > cx->regs().sp - fp->base())
                continue;

            jsbytecode *pc = script->main + tn->start + tn->length;
            JSBool ok = js_UnwindScope(cx, tn->stackDepth, JS_TRUE);
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
    JS_ASSERT(!js_IsActiveWithOrBlock(f.cx, &f.fp()->scopeChain(), 0));
    f.cx->stack.popInlineFrame();
}

void JS_FASTCALL
stubs::SlowCall(VMFrame &f, uint32 argc)
{
    if (!Invoke(f.cx, CallArgsFromSp(argc, f.regs.sp)))
        THROW();
}

void JS_FASTCALL
stubs::SlowNew(VMFrame &f, uint32 argc)
{
    if (!InvokeConstructor(f.cx, CallArgsFromSp(argc, f.regs.sp)))
        THROW();
}





static inline void
RemovePartialFrame(JSContext *cx, StackFrame *fp)
{
    cx->stack.popInlineFrame();
}





void JS_FASTCALL
stubs::HitStackQuota(VMFrame &f)
{
    
    uintN nvals = f.fp()->script()->nslots + VALUES_PER_STACK_FRAME;
    JS_ASSERT(f.regs.sp == f.fp()->base());
    if (f.cx->stack.space().tryBumpLimit(NULL, f.regs.sp, nvals, &f.stackLimit))
        return;

    
    RemovePartialFrame(f.cx, f.fp());
    js_ReportOverRecursed(f.cx);
    THROW();
}





void * JS_FASTCALL
stubs::FixupArity(VMFrame &f, uint32 nactual)
{
    JSContext *cx = f.cx;
    StackFrame *oldfp = f.fp();

    JS_ASSERT(nactual != oldfp->numFormalArgs());

    





    MaybeConstruct construct = oldfp->isConstructing();
    JSFunction *fun          = oldfp->fun();
    JSScript *script         = fun->script();
    void *ncode              = oldfp->nativeReturnAddress();

    
    f.regs.popPartialFrame((Value *)oldfp);

    
    CallArgs args = CallArgsFromSp(nactual, f.regs.sp);
    StackFrame *fp = cx->stack.getFixupFrame(cx, f.regs, args, fun, script, ncode,
                                             construct, LimitCheck(&f.stackLimit));
    if (!fp) {
        



        f.regs.pc = f.jit()->nativeToPC(ncode);
        THROWV(NULL);
    }

    
    return fp;
}

void * JS_FASTCALL
stubs::CompileFunction(VMFrame &f, uint32 nactual)
{
    



    JSContext *cx = f.cx;
    StackFrame *fp = f.fp();

    



    JSObject &callee = fp->formalArgsEnd()[-(int(nactual) + 2)].toObject();
    JSFunction *fun = callee.getFunctionPrivate();
    JSScript *script = fun->script();

    



    fp->initJitFrameEarlyPrologue(fun, nactual);

    if (nactual != fp->numFormalArgs()) {
        fp = (StackFrame *)FixupArity(f, nactual);
        if (!fp)
            return NULL;
    }

    
    fp->initJitFrameLatePrologue();

    
    f.regs.prepareToRun(fp, script);

    if (fun->isHeavyweight() && !js::CreateFunCallObject(cx, fp))
        THROWV(NULL);

    CompileStatus status = CanMethodJIT(cx, script, fp, CompileRequest_JIT);
    if (status == Compile_Okay)
        return script->getJIT(fp->isConstructing())->invokeEntry;

    
    JSBool ok = Interpret(cx, fp);
    InlineReturn(f);

    if (!ok)
        THROWV(NULL);

    return NULL;
}

static inline bool
UncachedInlineCall(VMFrame &f, MaybeConstruct construct, void **pret, bool *unjittable, uint32 argc)
{
    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    JSObject &callee = args.callee();
    JSFunction *newfun = callee.getFunctionPrivate();
    JSScript *newscript = newfun->script();

    
    LimitCheck check(&f.stackLimit);
    if (!cx->stack.pushInlineFrame(cx, f.regs, args, callee, newfun, newscript, construct, check))
        return false;

    
    if (newfun->isHeavyweight() && !js::CreateFunCallObject(cx, f.fp()))
        return false;

    
    if (newscript->getJITStatus(f.fp()->isConstructing()) == JITScript_None) {
        CompileStatus status = CanMethodJIT(cx, newscript, f.fp(), CompileRequest_Interpreter);
        if (status == Compile_Error) {
            
            InlineReturn(f);
            return false;
        }
        if (status == Compile_Abort)
            *unjittable = true;
    }

    
    if (JITScript *jit = newscript->getJIT(f.fp()->isConstructing())) {
        *pret = jit->invokeEntry;
        return true;
    }

    
    bool ok = !!Interpret(cx, cx->fp());
    InlineReturn(f);

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
        if (!UncachedInlineCall(f, CONSTRUCT, &ucr->codeAddr, &ucr->unjittable, argc))
            THROW();
    } else {
        if (!InvokeConstructor(cx, args))
            THROW();
    }
}

void * JS_FASTCALL
stubs::UncachedCall(VMFrame &f, uint32 argc)
{
    UncachedCallResult ucr;
    UncachedCallHelper(f, argc, &ucr);
    return ucr.codeAddr;
}

void JS_FASTCALL
stubs::Eval(VMFrame &f, uint32 argc)
{
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);

    if (!IsBuiltinEvalForScope(&f.fp()->scopeChain(), args.calleev())) {
        if (!Invoke(f.cx, args))
            THROW();
        return;
    }

    JS_ASSERT(f.fp() == f.cx->fp());
    if (!DirectEval(f.cx, args))
        THROW();

    f.regs.sp = args.spAfterCall();
}

void
stubs::UncachedCallHelper(VMFrame &f, uint32 argc, UncachedCallResult *ucr)
{
    ucr->init();

    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);

    if (IsFunctionObject(args.calleev(), &ucr->callee)) {
        ucr->callee = &args.callee();
        ucr->fun = GET_FUNCTION_PRIVATE(cx, ucr->callee);

        if (ucr->fun->isInterpreted()) {
            if (!UncachedInlineCall(f, NO_CONSTRUCT, &ucr->codeAddr, &ucr->unjittable, argc))
                THROW();
            return;
        }

        if (ucr->fun->isNative()) {
            if (!CallJSNative(cx, ucr->fun->u.n.native, args))
                THROW();
            return;
        }
    }

    if (!Invoke(f.cx, args))
        THROW();

    return;
}

void JS_FASTCALL
stubs::PutActivationObjects(VMFrame &f)
{
    JS_ASSERT(f.fp()->hasCallObj() || f.fp()->hasArgsObj());
    f.fp()->putActivationObjects();
}

extern "C" void *
js_InternalThrow(VMFrame &f)
{
    JSContext *cx = f.cx;

    
    
    
    
    
    
    
    
    if (f.fp()->finishedInInterpreter()) {
        
        if (f.fp() == f.entryfp)
            return NULL;

        InlineReturn(f);
    }

    
    JS_ASSERT(&cx->regs() == &f.regs);

    
    JSThrowHook handler = f.cx->debugHooks->throwHook;
    if (handler) {
        Value rval;
        switch (handler(cx, cx->fp()->script(), cx->regs().pc, Jsvalify(&rval),
                        cx->debugHooks->throwHookData)) {
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

    jsbytecode *pc = NULL;
    for (;;) {
        pc = FindExceptionHandler(cx);
        if (pc)
            break;

        
        
        
        
        
        JS_ASSERT(!f.fp()->finishedInInterpreter());
        js_UnwindScope(cx, 0, cx->isExceptionPending());
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
    return script->nativeCodeForPC(fp->isConstructing(), pc);
}

void JS_FASTCALL
stubs::CreateFunCallObject(VMFrame &f)
{
    JS_ASSERT(f.fp()->fun()->isHeavyweight());
    if (!js::CreateFunCallObject(f.cx, f.fp()))
        THROW();
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
    js::ScriptDebugPrologue(f.cx, f.fp());
}

void JS_FASTCALL
stubs::ScriptDebugEpilogue(VMFrame &f)
{
    if (!js::ScriptDebugEpilogue(f.cx, f.fp(), JS_TRUE))
        THROW();
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

        
        returnOK &= bool(js_UnwindScope(cx, 0, returnOK || cx->isExceptionPending()));
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



static inline void
AdvanceReturnPC(JSContext *cx)
{
    JS_ASSERT(*cx->regs().pc == JSOP_CALL ||
              *cx->regs().pc == JSOP_NEW ||
              *cx->regs().pc == JSOP_EVAL ||
              *cx->regs().pc == JSOP_FUNCALL ||
              *cx->regs().pc == JSOP_FUNAPPLY);
    cx->regs().pc += JSOP_CALL_LENGTH;
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
        AdvanceReturnPC(cx);
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
        if (!JaegerShotAtSafePoint(cx, ncode))
            return false;
        InlineReturn(f);
        AdvanceReturnPC(cx);
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
    UpdateTraceHintSingle(repatcher, ic.traceHint, ic.jumpTarget);

    if (ic.hasSlowTraceHint)
        UpdateTraceHintSingle(repatcher, ic.slowTraceHint, ic.jumpTarget);
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

    JaegerSpew(JSpew_PICs, "Enabling trace IC %u in script %p\n", index, script);

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
    tpa = MonitorTracePoint(f.cx, &blacklist, traceData, traceEpoch,
                            loopCounter, hits);
    JS_ASSERT(!TRACE_RECORDER(cx));

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

