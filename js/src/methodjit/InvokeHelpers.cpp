







































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

#include "jspropertycacheinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsstrinlines.h"
#include "jsobjinlines.h"
#include "jscntxtinlines.h"
#include "jsatominlines.h"

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;
using namespace JSC;

#define THROW()  \
    do {         \
        void *ptr = JS_FUNC_TO_DATA_PTR(void *, JaegerThrowpoline); \
        *f.returnAddressLocation() = ptr; \
        return;  \
    } while (0)

#define THROWV(v)       \
    do {                \
        void *ptr = JS_FUNC_TO_DATA_PTR(void *, JaegerThrowpoline); \
        *f.returnAddressLocation() = ptr; \
        return v;       \
    } while (0)

static bool
InlineReturn(JSContext *cx, JSBool ok);

static jsbytecode *
FindExceptionHandler(JSContext *cx)
{
    JSStackFrame *fp = cx->fp;
    JSScript *script = fp->script;

top:
    if (cx->throwing && script->trynotesOffset) {
        
        unsigned offset = cx->regs->pc - script->main;

        JSTryNoteArray *tnarray = script->trynotes();
        for (unsigned i = 0; i < tnarray->length; ++i) {
            JSTryNote *tn = &tnarray->vector[i];
            JS_ASSERT(offset < script->length);
            if (offset - tn->start >= tn->length)
                continue;
            if (tn->stackDepth > cx->regs->sp - fp->base())
                continue;

            jsbytecode *pc = script->main + tn->start + tn->length;
            JSBool ok = js_UnwindScope(cx, tn->stackDepth, JS_TRUE);
            JS_ASSERT(cx->regs->sp == fp->base() + tn->stackDepth);

            switch (tn->kind) {
                case JSTRY_CATCH:
                  JS_ASSERT(js_GetOpcode(cx, fp->script, pc) == JSOP_ENTERBLOCK);

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
                  JS_ASSERT(js_GetOpcode(cx, fp->script, pc) == JSOP_ENDITER);
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

static inline bool
CreateFrame(VMFrame &f, uint32 flags, uint32 argc)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = f.fp;
    Value *vp = f.regs.sp - (argc + 2);
    JSObject *funobj = &vp->toObject();
    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

    JS_ASSERT(FUN_INTERPRETED(fun));

    JSScript *newscript = fun->u.i.script;

    if (f.inlineCallCount >= JS_MAX_INLINE_CALL_COUNT) {
        js_ReportOverRecursed(cx);
        return false;
    }

    
    StackSpace &stack = cx->stack();
    uintN nslots = newscript->nslots;
    uintN funargs = fun->nargs;
    Value *argv = vp + 2;
    JSStackFrame *newfp;
    if (argc < funargs) {
        uintN missing = funargs - argc;
        newfp = stack.getInlineFrame(cx, f.regs.sp, missing, nslots);
        if (!newfp)
            return false;
        for (Value *v = argv + argc, *end = v + missing; v != end; ++v)
            v->setUndefined();
    } else {
        newfp = stack.getInlineFrame(cx, f.regs.sp, 0, nslots);
        if (!newfp)
            return false;
    }

    
    newfp->ncode = NULL;
    newfp->callobj = NULL;
    newfp->argsobj = NULL;
    newfp->script = newscript;
    newfp->fun = fun;
    newfp->argc = argc;
    newfp->argv = vp + 2;
    newfp->rval.setUndefined();
    newfp->annotation = NULL;
    newfp->scopeChain = funobj->getParent();
    newfp->flags = flags;
    newfp->blockChain = NULL;
    JS_ASSERT(!JSFUN_BOUND_METHOD_TEST(fun->flags));
    newfp->thisv = vp[1];
    newfp->imacpc = NULL;

    
    Value *newslots = newfp->slots();
    Value *newsp = newslots + fun->u.i.nvars;
    for (Value *v = newslots; v != newsp; ++v)
        v->setUndefined();

    
    if (fun->isHeavyweight() && !js_GetCallObject(cx, newfp))
        return false;

    
    newfp->callerVersion = (JSVersion)cx->version;

    
    if (JSInterpreterHook hook = cx->debugHooks->callHook) {
        newfp->hookData = hook(cx, fp, JS_TRUE, 0,
                               cx->debugHooks->callHookData);
        
    } else {
        newfp->hookData = NULL;
    }

    stack.pushInlineFrame(cx, fp, cx->regs->pc, newfp);

    return true;
}

static inline void
FixVMFrame(VMFrame &f, JSStackFrame *fp)
{
    f.inlineCallCount++;
    f.fp->ncode = f.scriptedReturn;
    JS_ASSERT(f.fp == fp->down);
    f.fp = fp;
}

static inline bool
InlineCall(VMFrame &f, uint32 flags, void **pret, uint32 argc)
{
    if (!CreateFrame(f, flags, argc))
        return false;

    JSContext *cx = f.cx;
    JSStackFrame *fp = cx->fp;
    JSScript *script = fp->script;
    if (cx->options & JSOPTION_METHODJIT) {
        if (!script->ncode) {
            if (mjit::TryCompile(cx, script, fp->fun, fp->scopeChain) == Compile_Error)
                return false;
        }
        JS_ASSERT(script->ncode);
        if (script->ncode != JS_UNJITTABLE_METHOD) {
            FixVMFrame(f, fp);
            *pret = script->nmap[-1];
            return true;
        }
    }

    f.regs.pc = script->code;
    f.regs.sp = fp->base();

    bool ok = !!Interpret(cx, cx->fp);
    InlineReturn(cx, JS_TRUE);

    *pret = NULL;
    return ok;
}

static bool
InlineReturn(JSContext *cx, JSBool ok)
{
    JSStackFrame *fp = cx->fp;

    JS_ASSERT(!fp->blockChain);
    JS_ASSERT(!js_IsActiveWithOrBlock(cx, fp->scopeChain, 0));

    
    void *hookData = fp->hookData;
    if (JS_UNLIKELY(hookData != NULL)) {
        JSInterpreterHook hook;
        JSBool status;

        hook = cx->debugHooks->callHook;
        if (hook) {
            



            status = ok;
            hook(cx, fp, JS_FALSE, &status, hookData);
            ok = (status == JS_TRUE);
            
        }
    }

    fp->putActivationObjects(cx);

    

    if (fp->flags & JSFRAME_CONSTRUCTING && fp->rval.isPrimitive())
        fp->rval = fp->thisv;

    Value *newsp = fp->argv - 1;

    cx->stack().popInlineFrame(cx, fp, fp->down);

    cx->regs->sp = newsp;
    cx->regs->sp[-1] = fp->rval;

    return ok;
}

static inline JSObject *
InlineConstruct(VMFrame &f, uint32 argc)
{
    JSContext *cx = f.cx;
    Value *vp = f.regs.sp - (argc + 2);

    JSObject *funobj = &vp[0].toObject();
    JS_ASSERT(funobj->isFunction());

    jsid id = ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom);
    if (!funobj->getProperty(cx, id, &vp[1]))
        return NULL;

    JSObject *proto = vp[1].isObject() ? &vp[1].toObject() : NULL;
    return NewObject(cx, &js_ObjectClass, proto, funobj->getParent());
}

void * JS_FASTCALL
stubs::SlowCall(VMFrame &f, uint32 argc)
{
    JSContext *cx = f.cx;
    Value *vp = f.regs.sp - (argc + 2);

    JSObject *obj;
    if (IsFunctionObject(*vp, &obj)) {
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);

        if (fun->isInterpreted()) {
            void *ret;

            if (fun->u.i.script->isEmpty()) {
                vp->setUndefined();
                f.regs.sp = vp + 1;
                return NULL;
            }

            if (!InlineCall(f, 0, &ret, argc))
                THROWV(NULL);

            return ret;
        }

        if (fun->isFastNative()) {
            FastNative fn = (FastNative)fun->u.n.native;
            if (!fn(cx, argc, vp))
                THROWV(NULL);
            return NULL;
        }
    }

    if (!Invoke(f.cx, InvokeArgsGuard(vp, argc), 0))
        THROWV(NULL);

    return NULL;
}

void * JS_FASTCALL
stubs::SlowNew(VMFrame &f, uint32 argc)
{
    JSContext *cx = f.cx;
    Value *vp = f.regs.sp - (argc + 2);

    JSObject *obj;
    if (IsFunctionObject(*vp, &obj)) {
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);

        if (fun->isInterpreted()) {
            JSScript *script = fun->u.i.script;
            JSObject *obj2 = InlineConstruct(f, argc);
            if (!obj2)
                THROWV(NULL);

            if (script->isEmpty()) {
                vp[0].setObject(*obj2);
                return NULL;
            }

            void *ret;
            vp[1].setObject(*obj2);
            if (!InlineCall(f, JSFRAME_CONSTRUCTING, &ret, argc))
                THROWV(NULL);

            return ret;
        }
    }

    if (!InvokeConstructor(cx, InvokeArgsGuard(vp, argc)))
        THROWV(NULL);

    return NULL;
}

static inline bool
CreateLightFrame(VMFrame &f, uint32 flags, uint32 argc)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = f.fp;
    Value *vp = f.regs.sp - (argc + 2);
    JSObject *funobj = &vp->toObject();
    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

    JS_ASSERT(FUN_INTERPRETED(fun));

    JSScript *newscript = fun->u.i.script;

    if (f.inlineCallCount >= JS_MAX_INLINE_CALL_COUNT) {
        js_ReportOverRecursed(cx);
        return false;
    }

    
    StackSpace &stack = cx->stack();
    uintN nslots = newscript->nslots;
    uintN funargs = fun->nargs;
    Value *argv = vp + 2;
    JSStackFrame *newfp;
    if (argc < funargs) {
        uintN missing = funargs - argc;
        newfp = stack.getInlineFrame(cx, f.regs.sp, missing, nslots);
        if (!newfp)
            return false;
        for (Value *v = argv + argc, *end = v + missing; v != end; ++v)
            v->setUndefined();
    } else {
        newfp = stack.getInlineFrame(cx, f.regs.sp, 0, nslots);
        if (!newfp)
            return false;
    }

    
    newfp->ncode = NULL;
    newfp->callobj = NULL;
    newfp->argsobj = NULL;
    newfp->script = newscript;
    newfp->fun = fun;
    newfp->argc = argc;
    newfp->argv = vp + 2;
    newfp->rval.setUndefined();
    newfp->annotation = NULL;
    newfp->scopeChain = funobj->getParent();
    newfp->flags = flags;
    newfp->blockChain = NULL;
    JS_ASSERT(!JSFUN_BOUND_METHOD_TEST(fun->flags));
    newfp->thisv = vp[1];
    newfp->imacpc = NULL;
    newfp->hookData = NULL;

#if 0
    
    newfp->callerVersion = (JSVersion)cx->version;
#endif

#ifdef DEBUG
    newfp->savedPC = JSStackFrame::sInvalidPC;
#endif
    newfp->down = fp;
    fp->savedPC = f.regs.pc;
    cx->setCurrentFrame(newfp);

    return true;
}




void * JS_FASTCALL
stubs::Call(VMFrame &f, uint32 argc)
{
    if (!CreateLightFrame(f, 0, argc))
        THROWV(NULL);

    FixVMFrame(f, f.cx->fp);

    return f.fp->script->ncode;
}




void * JS_FASTCALL
stubs::New(VMFrame &f, uint32 argc)
{
    JSObject *obj = InlineConstruct(f, argc);
    if (!obj)
        THROWV(NULL);

    f.regs.sp[-int(argc + 1)].setObject(*obj);
    if (!CreateLightFrame(f, JSFRAME_CONSTRUCTING, argc))
        THROWV(NULL);

    FixVMFrame(f, f.cx->fp);

    return f.fp->script->ncode;
}

void JS_FASTCALL
stubs::PutCallObject(VMFrame &f)
{
    JS_ASSERT(f.fp->callobj);
    js_PutCallObject(f.cx, f.fp);
    JS_ASSERT(!f.fp->argsobj);
}

void JS_FASTCALL
stubs::PutArgsObject(VMFrame &f)
{
    js_PutArgsObject(f.cx, f.fp);
}

void JS_FASTCALL
stubs::CopyThisv(VMFrame &f)
{
    JS_ASSERT(f.fp->flags & JSFRAME_CONSTRUCTING);
    if (f.fp->rval.isPrimitive())
        f.fp->rval = f.fp->thisv;
}

extern "C" void *
js_InternalThrow(VMFrame &f)
{
    JSContext *cx = f.cx;

    
    JS_ASSERT(cx->regs == &f.regs);

    jsbytecode *pc = NULL;
    for (;;) {
        pc = FindExceptionHandler(cx);
        if (pc)
            break;

        
        
        
        
        bool lastFrame = (f.inlineCallCount == 0);
        js_UnwindScope(cx, 0, cx->throwing);
        if (lastFrame)
            break;

        JS_ASSERT(f.regs.sp == cx->regs->sp);
        InlineReturn(f.cx, JS_FALSE);
        f.inlineCallCount--;
        f.fp = cx->fp;
        f.scriptedReturn = cx->fp->ncode;
    }

    JS_ASSERT(f.regs.sp == cx->regs->sp);

    if (!pc) {
        *f.oldRegs = f.regs;
        f.cx->setCurrentRegs(f.oldRegs);
        return NULL;
    }

    return cx->fp->script->pcToNative(pc);
}

void JS_FASTCALL
stubs::GetCallObject(VMFrame &f)
{
    JS_ASSERT(f.fp->fun->isHeavyweight());
    if (!js_GetCallObject(f.cx, f.fp))
        THROW();
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

#ifdef JS_TRACER

static inline bool
SwallowErrors(VMFrame &f, JSStackFrame *stopFp)
{
    JSContext *cx = f.cx;

    
    bool ok = false;
    for (;;) {
        JSStackFrame *fp = cx->fp;

        
        if (fp->imacpc && cx->throwing) {
            cx->regs->pc = fp->imacpc;
            fp->imacpc = NULL;
            if (ok)
                break;
        }
        JS_ASSERT(!fp->imacpc);

        
        jsbytecode *pc = FindExceptionHandler(cx);
        if (pc) {
            cx->regs->pc = pc;
            ok = true;
            break;
        }

        
        if (fp == stopFp)
            break;

        
        ok &= js_UnwindScope(cx, 0, cx->throwing);
        InlineReturn(cx, ok);
    }

    
    JS_ASSERT(&f.regs == cx->regs);

    JS_ASSERT_IF(!ok, cx->fp == stopFp);
    return ok;
}

static inline bool
AtSafePoint(JSContext *cx)
{
    JSStackFrame *fp = cx->fp;
    if (fp->imacpc)
        return false;

    JSScript *script = fp->script;
    if (!script->nmap)
        return false;

    JS_ASSERT(cx->regs->pc >= script->code && cx->regs->pc < script->code + script->length);
    return !!script->nmap[cx->regs->pc - script->code];
}

static inline JSBool
PartialInterpret(JSContext *cx)
{
    JSStackFrame *fp = cx->fp;

    JS_ASSERT(fp->imacpc || !fp->script->nmap ||
              !fp->script->nmap[cx->regs->pc - fp->script->code]);

    JSBool ok = JS_TRUE;
    fp->flags |= JSFRAME_BAILING;
    ok = Interpret(cx, fp);
    fp->flags &= ~JSFRAME_BAILING;

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

static bool
RemoveExcessFrames(VMFrame &f, JSStackFrame *entryFrame)
{
    JSContext *cx = f.cx;
    while (cx->fp != entryFrame) {
        JSStackFrame *fp = cx->fp;
        fp->flags &= ~JSFRAME_RECORDING;

        if (AtSafePoint(cx)) {
            if (!JaegerShot(cx)) {
                if (!SwallowErrors(f, entryFrame))
                    return false;

                
                continue;
            }
            InlineReturn(cx, JS_TRUE);
            AdvanceReturnPC(cx);
        } else {
            if (!PartialInterpret(cx)) {
                if (!SwallowErrors(f, entryFrame))
                    return false;
            } else {
                



                if (!cx->fp->imacpc && FrameIsFinished(cx)) {
                    InlineReturn(cx, JS_TRUE);
                    AdvanceReturnPC(cx);
                }
            }
        }
    }

    return true;
}

#if JS_MONOIC
static void
DisableTraceHint(VMFrame &f, ic::MICInfo &mic)
{
    JS_ASSERT(mic.kind == ic::MICInfo::TRACER);

    




    uint8 *addr = (uint8 *)(mic.traceHint.executableAddress());
    JSC::RepatchBuffer repatch(addr - 64, 128);
    repatch.relink(mic.traceHint, mic.load);

    JaegerSpew(JSpew_PICs, "relinking trace hint %p to %p\n", mic.traceHint.executableAddress(),
               mic.load.executableAddress());
}
#endif

#if JS_MONOIC
void *
RunTracer(VMFrame &f, ic::MICInfo &mic)
#else
void *
RunTracer(VMFrame &f)
#endif
{
    JSContext *cx = f.cx;
    JSStackFrame *entryFrame = f.fp;
    TracePointAction tpa;

    
    if (!cx->jitEnabled)
        return NULL;

    JS_ASSERT_IF(f.inlineCallCount,
                 entryFrame->down->script->isValidJitCode(f.scriptedReturn));

    bool blacklist;
    uintN inlineCallCount = f.inlineCallCount;
    tpa = MonitorTracePoint(f.cx, inlineCallCount, blacklist);
    JS_ASSERT(!TRACE_RECORDER(cx));

#if JS_MONOIC
    if (blacklist)
        DisableTraceHint(f, mic);
#endif

    if ((tpa == TPA_RanStuff || tpa == TPA_Recorded) && cx->throwing)
        tpa = TPA_Error;

    switch (tpa) {
      case TPA_Nothing:
        return NULL;

      case TPA_Error:
        if (!SwallowErrors(f, entryFrame))
            THROWV(NULL);
        JS_ASSERT(!cx->fp->imacpc);
        break;

      case TPA_RanStuff:
      case TPA_Recorded:
        break;
    }

    




















  restart:
    
    if (!RemoveExcessFrames(f, entryFrame))
        THROWV(NULL);

    
    entryFrame->flags &= ~JSFRAME_RECORDING;
    while (entryFrame->imacpc) {
        if (!PartialInterpret(cx)) {
            if (!SwallowErrors(f, entryFrame))
                THROWV(NULL);
        }

        
        goto restart;
    }

    
    if (AtSafePoint(cx)) {
        uint32 offs = uint32(cx->regs->pc - entryFrame->script->code);
        JS_ASSERT(entryFrame->script->nmap[offs]);
        return entryFrame->script->nmap[offs];
    }

    
    if (JSOp op = FrameIsFinished(cx)) {
        
        if (op == JSOP_RETURN)
            entryFrame->rval = f.regs.sp[-1];

        
        if (f.inlineCallCount) {
            if (!InlineReturn(cx, JS_TRUE))
                THROWV(NULL);
            f.inlineCallCount--;
        }
        f.fp = cx->fp;
        entryFrame->ncode = f.fp->ncode;
        void *retPtr = JS_FUNC_TO_DATA_PTR(void *, JaegerFromTracer);
        *f.returnAddressLocation() = retPtr;
        return NULL;
    }

    
    if (!PartialInterpret(cx)) {
        if (!SwallowErrors(f, entryFrame))
            THROWV(NULL);
    }

    goto restart;
}

#endif 

#if defined JS_TRACER
# if defined JS_MONOIC
void *JS_FASTCALL
stubs::InvokeTracer(VMFrame &f, uint32 index)
{
    JSScript *script = f.fp->script;
    ic::MICInfo &mic = script->mics[index];

    JS_ASSERT(mic.kind == ic::MICInfo::TRACER);

    return RunTracer(f, mic);
}

# else

void *JS_FASTCALL
stubs::InvokeTracer(VMFrame &f)
{
    return RunTracer(f);
}
# endif 
#endif 

