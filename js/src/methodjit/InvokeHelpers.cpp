







































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
    JS_ASSERT(f.cx->fp() != f.entryfp);
    JS_ASSERT(!js_IsActiveWithOrBlock(f.cx, &f.cx->fp()->scopeChain(), 0));
    f.cx->stack.popInlineFrame();
}

void JS_FASTCALL
stubs::SlowCall(VMFrame &f, uint32 argc)
{
    Value *vp = f.regs.sp - (argc + 2);

    if (!Invoke(f.cx, InvokeArgsAlreadyOnTheStack(argc, vp)))
        THROW();
}

void JS_FASTCALL
stubs::SlowNew(VMFrame &f, uint32 argc)
{
    JSContext *cx = f.cx;
    Value *vp = f.regs.sp - (argc + 2);

    if (!InvokeConstructor(cx, InvokeArgsAlreadyOnTheStack(argc, vp)))
        THROW();
}





static inline void
RemovePartialFrame(JSContext *cx, StackFrame *fp)
{
    cx->stack.popInlineFrame();
}

static inline bool
CheckStackQuota(VMFrame &f)
{
    uint32 nvals = VALUES_PER_STACK_FRAME + f.fp()->script()->nslots + StackSpace::STACK_JIT_EXTRA;
    if ((Value *)f.fp() + nvals >= f.stackLimit) {
        StackSpace &space = f.cx->stack.space();
        if (!space.bumpLimitWithinQuota(NULL, f.entryfp, f.regs.sp, nvals, &f.stackLimit)) {
            
            RemovePartialFrame(f.cx, f.fp());
            js_ReportOverRecursed(f.cx);
            return false;
        }
    }
    return true;
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

    





    uint32 flags         = oldfp->isConstructingFlag();
    JSFunction *fun      = oldfp->fun();
    void *ncode          = oldfp->nativeReturnAddress();

    
    f.regs.popPartialFrame((Value *)oldfp);

    
    StackFrame *newfp = cx->stack.getInlineFrameWithinLimit(cx, (Value*) oldfp, nactual,
                                                            fun, fun->script(), &flags,
                                                            f.entryfp, &f.stackLimit, ncode);

    





    if (!newfp)
        THROWV(NULL);

    
    newfp->initCallFrameCallerHalf(cx, flags, ncode);

    
    newfp->initCallFrameEarlyPrologue(fun, nactual);

    
    return newfp;
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

    bool isConstructing = f.fp()->isConstructing();
    f.regs.popPartialFrame((Value *)f.fp());

    return isConstructing ? UncachedNew(f, argc) : UncachedCall(f, argc);
}

static inline bool
UncachedInlineCall(VMFrame &f, uint32 flags, void **pret, bool *unjittable, uint32 argc)
{
    JSContext *cx = f.cx;
    Value *vp = f.regs.sp - (argc + 2);
    JSObject &callee = vp->toObject();
    JSFunction *newfun = callee.getFunctionPrivate();
    JSScript *newscript = newfun->script();

    bool newType = (flags & StackFrame::CONSTRUCTING) && cx->typeInferenceEnabled() &&
        types::UseNewType(cx, f.script(), f.pc());

    CallArgs args = CallArgsFromVp(argc, vp);
    types::TypeMonitorCall(cx, args, flags & StackFrame::CONSTRUCTING);

    
    StackFrame *newfp = cx->stack.getInlineFrameWithinLimit(cx, f.regs.sp, argc,
                                                            newfun, newscript, &flags,
                                                            f.entryfp, &f.stackLimit, NULL);
    if (JS_UNLIKELY(!newfp))
        return false;

    
    newfp->initCallFrame(cx, callee, newfun, argc, flags);
    SetValueRangeToUndefined(newfp->slots(), newscript->nfixed);

    



    FrameRegs regs = f.regs;
    PreserveRegsGuard regsGuard(cx, regs);

    
    cx->stack.pushInlineFrame(newscript, newfp, regs);

    
    if (newfun->isHeavyweight() && !js::CreateFunCallObject(cx, newfp))
        return false;

    
    if (newscript->getJITStatus(newfp->isConstructing()) == JITScript_None) {
        CompileStatus status = CanMethodJIT(cx, newscript, newfp, CompileRequest_Interpreter);
        if (status == Compile_Error) {
            
            InlineReturn(f);
            return false;
        }
        if (status == Compile_Abort)
            *unjittable = true;
    }

    



    if (!newType) {
        if (JITScript *jit = newscript->getJIT(newfp->isConstructing())) {
            *pret = jit->invokeEntry;

            




            regs.popFrame((Value *) regs.fp());
            return true;
        }
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
    
    if (IsFunctionObject(*vp, &ucr->fun) && ucr->fun->isInterpretedConstructor()) {
        ucr->callee = &vp->toObject();
        if (!UncachedInlineCall(f, StackFrame::CONSTRUCTING, &ucr->codeAddr, &ucr->unjittable, argc))
            THROW();
    } else {
        if (!InvokeConstructor(cx, InvokeArgsAlreadyOnTheStack(argc, vp)))
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
    Value *vp = f.regs.sp - (argc + 2);

    if (!IsBuiltinEvalForScope(&f.fp()->scopeChain(), *vp)) {
        if (!Invoke(f.cx, InvokeArgsAlreadyOnTheStack(argc, vp)))
            THROW();
        return;
    }

    JS_ASSERT(f.fp() == f.cx->fp());
    if (!DirectEval(f.cx, CallArgsFromVp(argc, vp)))
        THROW();

    f.regs.sp = vp + 1;
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
            if (!UncachedInlineCall(f, 0, &ucr->codeAddr, &ucr->unjittable, argc))
                THROW();
            return;
        }

        if (ucr->fun->isNative()) {
            if (!CallJSNative(cx, ucr->fun->u.n.native, argc, vp))
                THROW();
            return;
        }
    }

    if (!Invoke(f.cx, InvokeArgsAlreadyOnTheStack(argc, vp)))
        THROW();

    return;
}

void JS_FASTCALL
stubs::PutActivationObjects(VMFrame &f)
{
    JS_ASSERT(f.fp()->hasCallObj() || f.fp()->hasArgsObj());
    f.fp()->putActivationObjects();
}

static void
RemoveOrphanedNative(JSContext *cx, StackFrame *fp)
{
    





    JaegerCompartment *jc = cx->compartment->jaegerCompartment;
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

    
    
    RemoveOrphanedNative(cx, f.fp());

    
    
    
    
    
    
    
    
    if (f.fp()->finishedInInterpreter()) {
        
        if (f.fp() == f.entryfp)
            return NULL;

        InlineReturn(f);
    }

    
    JS_ASSERT(&cx->regs() == &f.regs);

    
    JSThrowHook handler = f.cx->debugHooks->throwHook;
    if (handler) {
        Value rval;
        switch (handler(cx, f.script(), f.pc(), Jsvalify(&rval), cx->debugHooks->throwHookData)) {
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

    if (!fp->jit()) {
        





        CompileStatus status = TryCompile(cx, fp);
        if (status != Compile_Okay)
            return NULL;
    }

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
    ok = Interpret(cx, fp, 0, JSINTERP_SAFEPOINT);

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
    uintN inlineCallCount = 0;
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

        tpa = MonitorTracePoint(f.cx, inlineCallCount, &blacklist, traceData, traceEpoch,
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
            f.script()->typeMonitorOverflow(cx, f.pc());
    }

    *var = nv;
    *vp = (cs->format & JOF_POST) ? ov : nv;
}

static bool
FinishObjIncOp(VMFrame &f, RejoinState rejoin, Value objv, Value ov, Value nv, Value *vp)
{
    



    JS_ASSERT(rejoin == REJOIN_BINDNAME || rejoin == REJOIN_GETTER ||
              rejoin == REJOIN_POS || rejoin == REJOIN_BINARY);

    JSContext *cx = f.cx;

    JSObject *obj = ValueToObject(cx, &objv);
    if (!obj)
        return false;

    JSOp op = js_GetOpcode(cx, f.script(), f.pc());
    const JSCodeSpec *cs = &js_CodeSpec[op];
    JS_ASSERT(JOF_TYPE(cs->format) == JOF_ATOM);

    jsid id = ATOM_TO_JSID(f.script()->getAtom(GET_SLOTNO(f.pc())));

    if (rejoin == REJOIN_BINDNAME && !obj->getProperty(cx, id, &ov))
        return false;

    if (rejoin == REJOIN_BINDNAME || rejoin == REJOIN_GETTER) {
        double d;
        if (!ValueToNumber(cx, ov, &d))
            return false;
        ov.setNumber(d);
    }

    if (rejoin == REJOIN_BINDNAME || rejoin == REJOIN_GETTER || rejoin == REJOIN_POS) {
        double d = ov.toNumber();
        double N = (cs->format & JOF_INC) ? 1 : -1;
        if (!nv.setNumber(d + N))
            f.script()->typeMonitorOverflow(cx, f.pc());
    }

    uint32 setPropFlags = (cs->format & JOF_NAME)
                          ? JSRESOLVE_ASSIGNING
                          : JSRESOLVE_ASSIGNING | JSRESOLVE_QUALIFIED;

    {
        JSAutoResolveFlags rf(cx, setPropFlags);
        if (!obj->setProperty(cx, id, &nv, f.script()->strictModeCode))
            return false;
    }

    *vp = (cs->format & JOF_POST) ? ov : nv;
    return true;
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

    analyze::AutoEnterAnalysis enter(cx);

    analyze::ScriptAnalysis *analysis = script->analysis(cx);
    if (analysis && !analysis->ranBytecode())
        analysis->analyzeBytecode(cx);
    if (!analysis || analysis->OOM()) {
        js_ReportOutOfMemory(cx);
        return js_InternalThrow(f);
    }

    




    Value *oldsp = f.regs.sp;
    f.regs.sp = fp->base() + analysis->getCode(pc).stackDepth;

    jsbytecode *nextpc = pc + analyze::GetBytecodeLength(pc);
    Value *nextsp = NULL;
    if (nextpc != script->code + script->length)
        nextsp = fp->base() + analysis->getCode(nextpc).stackDepth;

    JS_ASSERT(&cx->regs() == &f.regs);

#ifdef JS_METHODJIT_SPEW
    JaegerSpew(JSpew_Recompile, "interpreter rejoin (file \"%s\") (line \"%d\") (op %s)\n",
               script->filename, script->lineno, OpcodeNames[op]);
#endif

    uint32 nextDepth = uint32(-1);

    InterpMode interpMode = JSINTERP_REJOIN;

    if ((cs->format & (JOF_INC | JOF_DEC)) &&
        rejoin != REJOIN_FALLTHROUGH &&
        rejoin != REJOIN_RESUME &&
        rejoin != REJOIN_THIS_PROTOTYPE &&
        rejoin != REJOIN_CHECK_ARGUMENTS) {
        
        nextDepth = analysis->getCode(nextpc).stackDepth;
        untrap.retrap();
        enter.leave();

        switch (op) {
          case JSOP_INCLOCAL:
          case JSOP_DECLOCAL:
          case JSOP_LOCALINC:
          case JSOP_LOCALDEC:
          case JSOP_INCARG:
          case JSOP_DECARG:
          case JSOP_ARGINC:
          case JSOP_ARGDEC:
            if (rejoin != REJOIN_BINARY || !analysis->incrementInitialValueObserved(pc)) {
                
                FinishVarIncOp(f, rejoin, nextsp[-1], nextsp[-1], &nextsp[-1]);
            } else {
                
                FinishVarIncOp(f, rejoin, nextsp[-1], nextsp[0], &nextsp[-1]);
            }
            break;

          case JSOP_INCGNAME:
          case JSOP_DECGNAME:
          case JSOP_GNAMEINC:
          case JSOP_GNAMEDEC:
          case JSOP_INCNAME:
          case JSOP_DECNAME:
          case JSOP_NAMEINC:
          case JSOP_NAMEDEC:
          case JSOP_INCPROP:
          case JSOP_DECPROP:
          case JSOP_PROPINC:
          case JSOP_PROPDEC:
            if (rejoin != REJOIN_BINARY || !analysis->incrementInitialValueObserved(pc)) {
                
                if (!FinishObjIncOp(f, rejoin, nextsp[-1], nextsp[0], nextsp[0], &nextsp[-1]))
                    return js_InternalThrow(f);
            } else {
                
                if (!FinishObjIncOp(f, rejoin, nextsp[0], nextsp[-1], nextsp[1], &nextsp[-1]))
                    return js_InternalThrow(f);
            }
            break;

          default:
            JS_NOT_REACHED("Bad op");
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
        f.regs.pc = nextpc;
        break;
      }

      case REJOIN_NONE:
        JS_NOT_REACHED("Unpossible rejoin!");
        break;

      case REJOIN_RESUME:
        break;

      case REJOIN_TRAP:
        
        interpMode = untrap.trap ? JSINTERP_SKIP_TRAP : JSINTERP_REJOIN;
        break;

      case REJOIN_FALLTHROUGH:
        f.regs.pc = nextpc;
        break;

      case REJOIN_NATIVE:
      case REJOIN_NATIVE_LOWERED: {
        





        RemoveOrphanedNative(cx, fp);
        if (rejoin == REJOIN_NATIVE_LOWERED) {
            



            nextsp[-1] = nextsp[0];
        }
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

      case REJOIN_CHECK_ARGUMENTS: {
        




        if (!CheckStackQuota(f))
            return js_InternalThrow(f);
        if (fp->fun()->isHeavyweight()) {
            if (!js::CreateFunCallObject(cx, fp))
                return js_InternalThrow(f);
        }
        fp->initCallFrameLatePrologue();

        



        interpMode = JSINTERP_NORMAL;
        break;
      }

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
        if (!Invoke(cx, InvokeArgsAlreadyOnTheStack(f.u.call.dynamicArgc, nextsp)))
            return js_InternalThrow(f);
        nextsp[-1] = nextsp[0];
        f.regs.pc = nextpc;
        break;
      }

      case REJOIN_GETTER:
        



        switch (op) {
          case JSOP_NAME:
          case JSOP_GETGNAME:
          case JSOP_GETGLOBAL:
          case JSOP_GETPROP:
          case JSOP_GETXPROP:
          case JSOP_LENGTH:
            
            f.regs.pc = nextpc;
            break;

          case JSOP_CALLGNAME:
            if (!ComputeImplicitThis(cx, &fp->scopeChain(), nextsp[-2], &nextsp[-1]))
                return js_InternalThrow(f);
            f.regs.pc = nextpc;
            break;

          case JSOP_CALLGLOBAL:
            
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
            f.regs.pc = nextpc + GET_JUMP_OFFSET(nextpc);
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

    
    untrap.retrap();

    
    enter.leave();

    if (!Interpret(cx, NULL, 0, interpMode))
        return js_InternalThrow(f);

    
    JS_ASSERT(f.regs.fp() == fp);

    
    fp->returnValue();

    



    fp->putActivationObjects();

    return fp->nativeReturnAddress();
}
