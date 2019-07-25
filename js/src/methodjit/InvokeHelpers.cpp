







































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
#include "assembler/assembler/RepatchBuffer.h"
#include "jsiter.h"
#include "jstypes.h"
#include "methodjit/StubCalls.h"
#include "jstracer.h"
#include "jspropertycache.h"
#include "methodjit/MonoIC.h"

#include "jsinterpinlines.h"
#include "jspropertycacheinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsstrinlines.h"
#include "jsobjinlines.h"
#include "jscntxtinlines.h"
#include "jsatominlines.h"
#include "StubCalls-inl.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;
using namespace JSC;

static jsbytecode *
FindExceptionHandler(JSContext *cx)
{
    JSStackFrame *fp = cx->fp();
    JSScript *script = fp->script();

top:
    if (cx->throwing && script->trynotesOffset) {
        
        unsigned offset = cx->regs->pc - script->main;

        JSTryNoteArray *tnarray = script->trynotes();
        for (unsigned i = 0; i < tnarray->length; ++i) {
            JSTryNote *tn = &tnarray->vector[i];
            JS_ASSERT(offset < script->length);
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (offset - tn->start > tn->length)
                continue;
            if (tn->stackDepth > cx->regs->sp - fp->base())
                continue;

            jsbytecode *pc = script->main + tn->start + tn->length;
            JSBool ok = js_UnwindScope(cx, tn->stackDepth, JS_TRUE);
            JS_ASSERT(cx->regs->sp == fp->base() + tn->stackDepth);

            switch (tn->kind) {
                case JSTRY_CATCH:
                  JS_ASSERT(js_GetOpcode(cx, fp->script(), pc) == JSOP_ENTERBLOCK);

#if JS_HAS_GENERATORS
                  
                  if (JS_UNLIKELY(cx->exception.isMagic(JS_GENERATOR_CLOSING)))
                      break;
#endif

                  




                  return pc;

                case JSTRY_FINALLY:
                  



                  cx->regs->sp[0].setBoolean(true);
                  cx->regs->sp[1] = cx->exception;
                  cx->regs->sp += 2;
                  cx->throwing = JS_FALSE;
                  return pc;

                case JSTRY_ITER:
                {
                  






                  AutoValueRooter tvr(cx, cx->exception);
                  JS_ASSERT(js_GetOpcode(cx, fp->script(), pc) == JSOP_ENDITER);
                  cx->throwing = JS_FALSE;
                  ok = !!js_CloseIterator(cx, &cx->regs->sp[-1].toObject());
                  cx->regs->sp -= 1;
                  if (!ok)
                      goto top;
                  cx->throwing = JS_TRUE;
                  cx->exception = tvr.value();
                }
            }
        }
    }

    return NULL;
}









static void
InlineReturn(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = f.regs.fp;

    JS_ASSERT(f.fp() != f.entryFp);

    JS_ASSERT(!js_IsActiveWithOrBlock(cx, &fp->scopeChain(), 0));

    Value *newsp = fp->actualArgs() - 1;
    newsp[-1] = fp->returnValue();
    cx->stack().popInlineFrame(cx, fp->prev(), newsp);
}

void JS_FASTCALL
stubs::SlowCall(VMFrame &f, uint32 argc)
{
    Value *vp = f.regs.sp - (argc + 2);

    if (!Invoke(f.cx, InvokeArgsAlreadyOnTheStack(vp, argc), 0))
        THROW();
}

void JS_FASTCALL
stubs::SlowNew(VMFrame &f, uint32 argc)
{
    JSContext *cx = f.cx;
    Value *vp = f.regs.sp - (argc + 2);

    if (!InvokeConstructor(cx, InvokeArgsAlreadyOnTheStack(vp, argc)))
        THROW();
}





static inline void
RemovePartialFrame(JSContext *cx, JSStackFrame *fp)
{
    JSStackFrame *prev = fp->prev();
    Value *newsp = (Value *)fp;
    cx->stack().popInlineFrame(cx, prev, newsp);
}





void JS_FASTCALL
stubs::HitStackQuota(VMFrame &f)
{
    
    uintN nvals = f.fp()->script()->nslots + VALUES_PER_STACK_FRAME;
    JS_ASSERT(f.regs.sp == f.fp()->base());
    if (f.cx->stack().bumpCommitAndLimit(f.entryFp, f.regs.sp, nvals, &f.stackLimit))
        return;

    
    RemovePartialFrame(f.cx, f.fp());
    js_ReportOverRecursed(f.cx);
    THROW();
}





void * JS_FASTCALL
stubs::FixupArity(VMFrame &f, uint32 nactual)
{
    JSContext *cx = f.cx;
    JSStackFrame *oldfp = f.fp();

    JS_ASSERT(nactual != oldfp->numFormalArgs());

    





    uint32 flags         = oldfp->isConstructingFlag();
    JSFunction *fun      = oldfp->fun();
    void *ncode          = oldfp->nativeReturnAddress();

    
    f.fp() = oldfp->prev();
    f.regs.sp = (Value*) oldfp;

    
    JSStackFrame *newfp = cx->stack().getInlineFrameWithinLimit(cx, (Value*) oldfp, nactual,
                                                                fun, fun->script(), &flags,
                                                                f.entryFp, &f.stackLimit);
    if (!newfp)
        THROWV(NULL);

    
    newfp->initCallFrameCallerHalf(cx, nactual, flags);

    
    newfp->initCallFrameEarlyPrologue(fun, ncode);

    
    return newfp;
}

void * JS_FASTCALL
stubs::CompileFunction(VMFrame &f, uint32 nactual)
{
    



    JSContext *cx = f.cx;
    JSStackFrame *fp = f.fp();

    



    JSObject &callee = fp->formalArgsEnd()[-(int(nactual) + 2)].toObject();
    JSFunction *fun = callee.getFunctionPrivate();
    JSScript *script = fun->script();

    




    fp->initCallFrameEarlyPrologue(fun, fp->nativeReturnAddress());

    
    bool callingNew = fp->isConstructing();
    if (script->isEmpty()) {
        RemovePartialFrame(cx, fp);
        Value *vp = f.regs.sp - (nactual + 2);
        if (callingNew)
            vp[0] = vp[1];
        else
            vp[0].setUndefined();
        return NULL;
    }

    if (nactual != fp->numFormalArgs()) {
        fp = (JSStackFrame *)FixupArity(f, nactual);
        if (!fp)
            return NULL;
    }

    
    fp->initCallFrameLatePrologue();

    
    f.regs.fp = fp;
    f.regs.sp = fp->base();
    f.regs.pc = script->code;

    if (fun->isHeavyweight() && !js_GetCallObject(cx, fp))
        THROWV(NULL);

    CompileStatus status = CanMethodJIT(cx, script, fp);
    if (status == Compile_Okay)
        return script->getJIT(callingNew)->invokeEntry;

    
    JSBool ok = Interpret(cx, fp);
    InlineReturn(f);

    if (!ok)
        THROWV(NULL);

    return NULL;
}

static inline bool
UncachedInlineCall(VMFrame &f, uint32 flags, void **pret, uint32 argc)
{
    JSContext *cx = f.cx;
    Value *vp = f.regs.sp - (argc + 2);
    JSObject &callee = vp->toObject();
    JSFunction *newfun = callee.getFunctionPrivate();
    JSScript *newscript = newfun->script();

    
    StackSpace &stack = cx->stack();
    JSStackFrame *newfp = stack.getInlineFrameWithinLimit(cx, f.regs.sp, argc,
                                                          newfun, newscript, &flags,
                                                          f.entryFp, &f.stackLimit);
    if (JS_UNLIKELY(!newfp))
        return false;
    JS_ASSERT_IF(!vp[1].isPrimitive() && !(flags & JSFRAME_CONSTRUCTING),
                 IsSaneThisObject(vp[1].toObject()));

    
    newfp->initCallFrame(cx, callee, newfun, argc, flags);
    SetValueRangeToUndefined(newfp->slots(), newscript->nfixed);

    
    stack.pushInlineFrame(cx, newscript, newfp, &f.regs);
    JS_ASSERT(newfp == f.regs.fp);

    
    if (newfun->isHeavyweight() && !js_GetCallObject(cx, newfp))
        return false;

    
    if (newscript->getJITStatus(newfp->isConstructing()) == JITScript_None) {
        if (mjit::TryCompile(cx, newfp) == Compile_Error) {
            
            InlineReturn(f);
            return false;
        }
    }

    
    if (JITScript *jit = newscript->getJIT(newfp->isConstructing())) {
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
    Value *vp = f.regs.sp - (argc + 2);

    
    if (IsFunctionObject(*vp, &ucr->fun) && ucr->fun->isInterpreted() && 
        !ucr->fun->script()->isEmpty())
    {
        ucr->callee = &vp->toObject();
        if (!UncachedInlineCall(f, JSFRAME_CONSTRUCTING, &ucr->codeAddr, argc))
            THROW();
    } else {
        if (!InvokeConstructor(cx, InvokeArgsAlreadyOnTheStack(vp, argc)))
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

void
stubs::UncachedCallHelper(VMFrame &f, uint32 argc, UncachedCallResult *ucr)
{
    ucr->init();

    JSContext *cx = f.cx;
    Value *vp = f.regs.sp - (argc + 2);

    if (IsFunctionObject(*vp, &ucr->callee)) {
        ucr->callee = &vp->toObject();
        ucr->fun = GET_FUNCTION_PRIVATE(cx, ucr->callee);

        if (ucr->fun->isInterpreted()) {
            if (ucr->fun->u.i.script->isEmpty()) {
                vp->setUndefined();
                f.regs.sp = vp + 1;
                return;
            }

            if (!UncachedInlineCall(f, 0, &ucr->codeAddr, argc))
                THROW();
            return;
        }

        if (ucr->fun->isNative()) {
            if (!CallJSNative(cx, ucr->fun->u.n.native, argc, vp))
                THROW();
            return;
        }
    }

    if (!Invoke(f.cx, InvokeArgsAlreadyOnTheStack(vp, argc), 0))
        THROW();

    return;
}

void JS_FASTCALL
stubs::PutCallObject(VMFrame &f)
{
    JS_ASSERT(f.fp()->hasCallObj());
    js_PutCallObject(f.cx, f.fp());
}

void JS_FASTCALL
stubs::PutActivationObjects(VMFrame &f)
{
    JS_ASSERT(f.fp()->hasCallObj() || f.fp()->hasArgsObj());
    js::PutActivationObjects(f.cx, f.fp());
}

extern "C" void *
js_InternalThrow(VMFrame &f)
{
    JSContext *cx = f.cx;

    
    JS_ASSERT(cx->regs == &f.regs);

    
    JSThrowHook handler = f.cx->debugHooks->throwHook;
    if (handler) {
        Value rval;
        switch (handler(cx, cx->fp()->script(), cx->regs->pc, Jsvalify(&rval),
                        cx->debugHooks->throwHookData)) {
          case JSTRAP_ERROR:
            cx->throwing = JS_FALSE;
            return NULL;

          case JSTRAP_RETURN:
            cx->throwing = JS_FALSE;
            cx->fp()->setReturnValue(rval);
            return JS_FUNC_TO_DATA_PTR(void *,
                   JS_METHODJIT_DATA(cx).trampolines.forceReturn);

          case JSTRAP_THROW:
            cx->exception = rval;
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

        
        
        
        
        bool lastFrame = (f.entryFp == f.fp());
        js_UnwindScope(cx, 0, cx->throwing);

        
        
        
        
        ScriptEpilogue(f.cx, f.fp(), false);

        if (lastFrame)
            break;

        JS_ASSERT(f.regs.sp == cx->regs->sp);
        InlineReturn(f);
    }

    JS_ASSERT(f.regs.sp == cx->regs->sp);

    if (!pc)
        return NULL;

    JSStackFrame *fp = cx->fp();
    JSScript *script = fp->script();
    return script->nativeCodeForPC(fp->isConstructing(), pc);
}

void JS_FASTCALL
stubs::GetCallObject(VMFrame &f)
{
    JS_ASSERT(f.fp()->fun()->isHeavyweight());
    if (!js_GetCallObject(f.cx, f.fp()))
        THROW();
}

void JS_FASTCALL
stubs::CreateThis(VMFrame &f, JSObject *proto)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = f.fp();
    JSObject *callee = &fp->callee();
    JSObject *obj = js_CreateThisForFunctionWithProto(cx, callee, proto);
    if (!obj)
        THROW();
    fp->formalArgs()[-1].setObject(*obj);
}

void JS_FASTCALL
stubs::EnterScript(VMFrame &f)
{
    JSStackFrame *fp = f.fp();
    JSContext *cx = f.cx;
    JSInterpreterHook hook = cx->debugHooks->callHook;
    if (JS_UNLIKELY(hook != NULL) && !fp->isExecuteFrame()) {
        fp->setHookData(hook(cx, fp, JS_TRUE, 0, cx->debugHooks->callHookData));
    }

    Probes::enterJSFun(cx, fp->maybeFun());
}

void JS_FASTCALL
stubs::LeaveScript(VMFrame &f)
{
    JSStackFrame *fp = f.fp();
    JSContext *cx = f.cx;
    Probes::exitJSFun(cx, fp->maybeFun());
    JSInterpreterHook hook = cx->debugHooks->callHook;

    if (hook && fp->hasHookData() && !fp->isExecuteFrame()) {
        JSBool ok = JS_TRUE;
        hook(cx, fp, JS_FALSE, &ok, fp->hookData());
        if (!ok)
            THROW();
    }
}

#ifdef JS_TRACER






static inline bool
HandleErrorInExcessFrame(VMFrame &f, JSStackFrame *stopFp, bool searchedTopmostFrame = true)
{
    JSContext *cx = f.cx;

    






    JSStackFrame *fp = cx->fp();
    if (searchedTopmostFrame) {
        if (fp == stopFp)
            return false;

        InlineReturn(f);
    }

    
    bool returnOK = false;
    for (;;) {
        fp = cx->fp();

        
        if (fp->hasImacropc()) {
            cx->regs->pc = fp->imacropc();
            fp->clearImacropc();
        }
        JS_ASSERT(!fp->hasImacropc());

        
        if (cx->throwing) {
            jsbytecode *pc = FindExceptionHandler(cx);
            if (pc) {
                cx->regs->pc = pc;
                returnOK = true;
                break;
            }
        }

        
        if (fp == stopFp)
            break;

        
        returnOK &= bool(js_UnwindScope(cx, 0, returnOK || cx->throwing));
        returnOK = ScriptEpilogue(cx, fp, returnOK);
        InlineReturn(f);
    }

    JS_ASSERT(&f.regs == cx->regs);
    JS_ASSERT_IF(!returnOK, cx->fp() == stopFp);

    return returnOK;
}


static inline void *
AtSafePoint(JSContext *cx)
{
    JSStackFrame *fp = cx->fp();
    if (fp->hasImacropc())
        return false;

    JSScript *script = fp->script();
    return script->maybeNativeCodeForPC(fp->isConstructing(), cx->regs->pc);
}





static inline JSBool
PartialInterpret(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = cx->fp();

#ifdef DEBUG
    JSScript *script = fp->script();
    JS_ASSERT(!fp->finishedInInterpreter());
    JS_ASSERT(fp->hasImacropc() ||
              !script->maybeNativeCodeForPC(fp->isConstructing(), cx->regs->pc));
#endif

    JSBool ok = JS_TRUE;
    ok = Interpret(cx, fp, 0, JSINTERP_SAFEPOINT);

    return ok;
}

JS_STATIC_ASSERT(JSOP_NOP == 0);


static inline JSOp
FrameIsFinished(JSContext *cx)
{
    JSOp op = JSOp(*cx->regs->pc);
    return (op == JSOP_RETURN ||
            op == JSOP_RETRVAL ||
            op == JSOP_STOP)
        ? op
        : JSOP_NOP;
}



static inline void
AdvanceReturnPC(JSContext *cx)
{
    JS_ASSERT(*cx->regs->pc == JSOP_CALL ||
              *cx->regs->pc == JSOP_NEW ||
              *cx->regs->pc == JSOP_EVAL ||
              *cx->regs->pc == JSOP_APPLY);
    cx->regs->pc += JSOP_CALL_LENGTH;
}









static bool
HandleFinishedFrame(VMFrame &f, JSStackFrame *entryFrame)
{
    JSContext *cx = f.cx;

    JS_ASSERT(FrameIsFinished(cx));

    


























    bool returnOK = true;
    if (!cx->fp()->finishedInInterpreter()) {
        if (JSOp(*cx->regs->pc) == JSOP_RETURN)
            cx->fp()->setReturnValue(f.regs.sp[-1]);

        returnOK = ScriptEpilogue(cx, cx->fp(), true);
    }

    JS_ASSERT_IF(cx->fp()->isFunctionFrame() &&
                 !cx->fp()->isEvalFrame(),
                 !cx->fp()->hasCallObj());

    if (cx->fp() != entryFrame) {
        InlineReturn(f);
        AdvanceReturnPC(cx);
    }

    return returnOK;
}
















static bool
EvaluateExcessFrame(VMFrame &f, JSStackFrame *entryFrame)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = cx->fp();

    





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
FinishExcessFrames(VMFrame &f, JSStackFrame *entryFrame)
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

#if JS_MONOIC
static void
UpdateTraceHintSingle(JSC::CodeLocationJump jump, JSC::CodeLocationLabel target)
{
    




    uint8 *addr = (uint8 *)(jump.executableAddress());
    JSC::RepatchBuffer repatch(addr - 64, 128);
    repatch.relink(jump, target);

    JaegerSpew(JSpew_PICs, "relinking trace hint %p to %p\n",
               jump.executableAddress(), target.executableAddress());
}

static void
DisableTraceHint(VMFrame &f, ic::TraceICInfo &tic)
{
    UpdateTraceHintSingle(tic.traceHint, tic.jumpTarget);

    if (tic.hasSlowTraceHint)
        UpdateTraceHintSingle(tic.slowTraceHint, tic.jumpTarget);
}

static void
EnableTraceHintAt(JSScript *script, js::mjit::JITScript *jit, jsbytecode *pc, uint16_t index)
{
    JS_ASSERT(index < jit->nTraceICs);
    ic::TraceICInfo &tic = jit->traceICs[index];

    JS_ASSERT(tic.jumpTargetPC == pc);

    JaegerSpew(JSpew_PICs, "Enabling trace IC %u in script %p\n", index, script);

    UpdateTraceHintSingle(tic.traceHint, tic.stubEntry);

    if (tic.hasSlowTraceHint)
        UpdateTraceHintSingle(tic.slowTraceHint, tic.stubEntry);
}
#endif

void
js::mjit::EnableTraceHint(JSScript *script, jsbytecode *pc, uint16_t index)
{
#if JS_MONOIC
    if (script->jitNormal)
        EnableTraceHintAt(script, script->jitNormal, pc, index);

    if (script->jitCtor)
        EnableTraceHintAt(script, script->jitCtor, pc, index);
#endif
}

#if JS_MONOIC
void *
RunTracer(VMFrame &f, ic::TraceICInfo &tic)
#else
void *
RunTracer(VMFrame &f)
#endif
{
    JSContext *cx = f.cx;
    JSStackFrame *entryFrame = f.fp();
    TracePointAction tpa;

    
    if (!cx->traceJitEnabled)
        return NULL;

    






    entryFrame->scopeChain();
    entryFrame->returnValue();

    bool blacklist;
    uintN inlineCallCount = 0;
    void **traceData;
    uintN *traceEpoch;
#if JS_MONOIC
    traceData = &tic.traceData;
    traceEpoch = &tic.traceEpoch;
#else
    traceData = NULL;
    traceEpoch = NULL;
#endif
    tpa = MonitorTracePoint(f.cx, inlineCallCount, &blacklist, traceData, traceEpoch);
    JS_ASSERT(!TRACE_RECORDER(cx));

#if JS_MONOIC
    if (blacklist)
        DisableTraceHint(f, tic);
#endif

    
    
    JS_ASSERT_IF(cx->throwing, tpa == TPA_Error);

	f.fp() = cx->fp();
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

        void *retPtr = JS_FUNC_TO_DATA_PTR(void *, InjectJaegerReturn);
        *f.returnAddressLocation() = retPtr;
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
stubs::InvokeTracer(VMFrame &f, ic::TraceICInfo *tic)
{
    return RunTracer(f, *tic);
}

# else

void *JS_FASTCALL
stubs::InvokeTracer(VMFrame &f)
{
    return RunTracer(f);
}
# endif 
#endif 

