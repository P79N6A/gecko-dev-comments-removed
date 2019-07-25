







































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
InlineReturn(VMFrame &f, JSBool ok);

static jsbytecode *
FindExceptionHandler(JSContext *cx)
{
    JSStackFrame *fp = cx->fp();
    JSScript *script = fp->getScript();

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
                  JS_ASSERT(js_GetOpcode(cx, fp->getScript(), pc) == JSOP_ENTERBLOCK);

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
                  JS_ASSERT(js_GetOpcode(cx, fp->getScript(), pc) == JSOP_ENDITER);
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

static inline void
FixVMFrame(VMFrame &f, JSStackFrame *fp)
{
    JS_ASSERT(f.fp() == fp->down);
    f.fp() = fp;
}

static inline bool
CreateFrame(VMFrame &f, uint32 flags, uint32 argc)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = f.fp();
    Value *vp = f.regs.sp - (argc + 2);
    JSObject *funobj = &vp->toObject();
    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

    JS_ASSERT(FUN_INTERPRETED(fun));

    JSScript *newscript = fun->u.i.script;

    
    StackSpace &stack = cx->stack();
    uintN nslots = newscript->nslots;
    uintN funargs = fun->nargs;
    Value *argv = vp + 2;
    JSStackFrame *newfp;
    if (argc < funargs) {
        uintN missing = funargs - argc;
        if (!f.ensureSpace(missing, nslots))
            return false;
        newfp = stack.getInlineFrameUnchecked(cx, f.regs.sp, missing);
        if (!newfp)
            return false;
        for (Value *v = argv + argc, *end = v + missing; v != end; ++v)
            v->setUndefined();
    } else {
        if (!f.ensureSpace(0, nslots))
            return false;
        newfp = stack.getInlineFrameUnchecked(cx, f.regs.sp, 0);
        if (!newfp)
            return false;
    }

    
    newfp->ncode = NULL;
    newfp->setCallObj(NULL);
    newfp->setArgsObj(NULL);
    newfp->setScript(newscript);
    newfp->setFunction(fun);
    newfp->argc = argc;
    newfp->argv = vp + 2;
    newfp->clearReturnValue();
    newfp->setAnnotation(NULL);
    newfp->setScopeChain(funobj->getParent());
    newfp->flags = flags;
    newfp->setBlockChain(NULL);
    newfp->setThisValue(vp[1]);
    JS_ASSERT(!fp->hasIMacroPC());

    
    Value *newslots = newfp->slots();
    Value *newsp = newslots + fun->u.i.nvars;
    for (Value *v = newslots; v != newsp; ++v)
        v->setUndefined();

    
    if (fun->isHeavyweight() && !js_GetCallObject(cx, newfp))
        return false;

    
    newfp->setCallerVersion((JSVersion)cx->version);

    
    if (JSInterpreterHook hook = cx->debugHooks->callHook) {
        newfp->setHookData(hook(cx, fp, JS_TRUE, 0,
                                cx->debugHooks->callHookData));
        
    } else {
        newfp->setHookData(NULL);
    }

    stack.pushInlineFrame(cx, fp, cx->regs->pc, newfp);
    FixVMFrame(f, newfp);

    return true;
}

static inline bool
InlineCall(VMFrame &f, uint32 flags, void **pret, uint32 argc)
{
    if (!CreateFrame(f, flags, argc))
        return false;

    JSContext *cx = f.cx;
    JSStackFrame *fp = cx->fp();
    JSScript *script = fp->getScript();
    f.regs.pc = script->code;
    f.regs.sp = fp->base();

    if (cx->options & JSOPTION_METHODJIT) {
        if (!script->ncode) {
            if (mjit::TryCompile(cx, script, fp->getFunction(), fp->getScopeChain()) == Compile_Error) {
                InlineReturn(f, JS_FALSE);
                return false;
            }
        }
        JS_ASSERT(script->ncode);
        if (script->ncode != JS_UNJITTABLE_METHOD) {
            *pret = script->jit->invoke;
            return true;
        }
    }

    bool ok = !!Interpret(cx, cx->fp());
    InlineReturn(f, JS_TRUE);

    *pret = NULL;
    return ok;
}

static bool
InlineReturn(VMFrame &f, JSBool ok)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = cx->fp();

    JS_ASSERT(f.fp() == cx->fp());
    JS_ASSERT(f.fp() != f.entryFp);

    JS_ASSERT(!fp->hasBlockChain());
    JS_ASSERT(!js_IsActiveWithOrBlock(cx, fp->getScopeChain(), 0));

    
    if (JS_UNLIKELY(fp->hasHookData())) {
        JSInterpreterHook hook;
        JSBool status;

        hook = cx->debugHooks->callHook;
        if (hook) {
            



            status = ok;
            hook(cx, fp, JS_FALSE, &status, fp->getHookData());
            ok = (status == JS_TRUE);
            
        }
    }

    fp->putActivationObjects(cx);

    

    if (fp->flags & JSFRAME_CONSTRUCTING && fp->getReturnValue().isPrimitive())
        fp->setReturnValue(fp->getThisValue());

    Value *newsp = fp->argv - 1;

    cx->stack().popInlineFrame(cx, fp, fp->down);
    f.fp() = cx->fp();

    cx->regs->sp = newsp;
    cx->regs->sp[-1] = fp->getReturnValue();

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
    return NewNonFunction<WithProto::Class>(cx, &js_ObjectClass, proto, funobj->getParent());
}

void * JS_FASTCALL
stubs::SlowCall(VMFrame &f, uint32 argc)
{
    JSContext *cx = f.cx;

#ifdef JS_MONOIC
    ic::MICInfo &mic = f.fp()->getScript()->mics[argc];
    argc = mic.argc;
#endif

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
#ifdef JS_MONOIC
#ifdef JS_CPU_X86
            ic::CallFastNative(cx, f.fp()->getScript(), mic, fun, false);
#endif
#endif

            FastNative fn = (FastNative)fun->u.n.native;
            if (!fn(cx, argc, vp))
                THROWV(NULL);
            return NULL;
        }
    }

    if (!Invoke(f.cx, InvokeArgsAlreadyOnTheStack(vp, argc), 0))
        THROWV(NULL);

    return NULL;
}

void * JS_FASTCALL
stubs::SlowNew(VMFrame &f, uint32 argc)
{
    JSContext *cx = f.cx;

#ifdef JS_MONOIC
    ic::MICInfo &mic = f.fp()->getScript()->mics[argc];
    argc = mic.argc;
#endif

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

        if (fun->isFastConstructor()) {
#ifdef JS_MONOIC
#ifdef JS_CPU_X86
            ic::CallFastNative(cx, f.fp()->getScript(), mic, fun, true);
#endif
#endif

            vp[1].setMagic(JS_FAST_CONSTRUCTOR);

            FastNative fn = (FastNative)fun->u.n.native;
            if (!fn(cx, argc, vp))
                THROWV(NULL);
            JS_ASSERT(!vp->isPrimitive());

            return NULL;
        }
    }

    if (!InvokeConstructor(cx, InvokeArgsAlreadyOnTheStack(vp, argc)))
        THROWV(NULL);

    return NULL;
}

static inline bool
CreateLightFrame(VMFrame &f, uint32 flags, uint32 argc)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = f.fp();
    Value *vp = f.regs.sp - (argc + 2);
    JSObject *funobj = &vp->toObject();
    JSFunction *fun = GET_FUNCTION_PRIVATE(cx, funobj);

    JS_ASSERT(FUN_INTERPRETED(fun));

    JSScript *newscript = fun->u.i.script;

    
    StackSpace &stack = cx->stack();
    uintN nslots = newscript->nslots;
    uintN funargs = fun->nargs;
    Value *argv = vp + 2;
    JSStackFrame *newfp;
    if (argc < funargs) {
        uintN missing = funargs - argc;
        if (!f.ensureSpace(missing, nslots))
            return false;
        newfp = stack.getInlineFrameUnchecked(cx, f.regs.sp, missing);
        if (!newfp)
            return false;
        for (Value *v = argv + argc, *end = v + missing; v != end; ++v)
            v->setUndefined();
    } else {
        if (!f.ensureSpace(0, nslots))
            return false;
        newfp = stack.getInlineFrameUnchecked(cx, f.regs.sp, 0);
        if (!newfp)
            return false;
    }

    
    newfp->setCallObj(NULL);
    newfp->setArgsObj(NULL);
    newfp->setScript(newscript);
    newfp->setFunction(fun);
    newfp->argc = argc;
    newfp->argv = vp + 2;
    newfp->clearReturnValue();
    newfp->setAnnotation(NULL);
    newfp->setScopeChain(funobj->getParent());
    newfp->flags = flags;
    newfp->setBlockChain(NULL);
    newfp->setThisValue(vp[1]);
    newfp->setHookData(NULL);
    JS_ASSERT(!fp->hasIMacroPC());

#if 0
    
    newfp->setCallerVersion((JSVersion)cx->version);
#endif

#ifdef DEBUG
    newfp->savedPC = JSStackFrame::sInvalidPC;
#endif
    newfp->down = fp;
    fp->savedPC = f.regs.pc;
    FixVMFrame(f, newfp);

    return true;
}




void * JS_FASTCALL
stubs::Call(VMFrame &f, uint32 argc)
{
    if (!CreateLightFrame(f, 0, argc))
        THROWV(NULL);

    return f.fp()->getScript()->ncode;
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

    return f.fp()->getScript()->ncode;
}

void JS_FASTCALL
stubs::PutCallObject(VMFrame &f)
{
    JS_ASSERT(f.fp()->hasCallObj());
    js_PutCallObject(f.cx, f.fp());
    JS_ASSERT(!f.fp()->hasArgsObj());
}

void JS_FASTCALL
stubs::PutArgsObject(VMFrame &f)
{
    js_PutArgsObject(f.cx, f.fp());
}

void JS_FASTCALL
stubs::CopyThisv(VMFrame &f)
{
    JS_ASSERT(f.fp()->flags & JSFRAME_CONSTRUCTING);
    if (f.fp()->getReturnValue().isPrimitive())
        f.fp()->setReturnValue(f.fp()->getThisValue());
}

extern "C" void *
js_InternalThrow(VMFrame &f)
{
    JSContext *cx = f.cx;

    
    JS_ASSERT(cx->regs == &f.regs);

    
    JSThrowHook handler = f.cx->debugHooks->throwHook;
    if (handler) {
        Value rval;
        switch (handler(cx, cx->fp()->getScript(), cx->regs->pc, Jsvalify(&rval),
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
        if (lastFrame)
            break;

        JS_ASSERT(f.regs.sp == cx->regs->sp);
        InlineReturn(f, JS_FALSE);
    }

    JS_ASSERT(f.regs.sp == cx->regs->sp);

    if (!pc) {
        *f.oldRegs = f.regs;
        f.cx->setCurrentRegs(f.oldRegs);
        return NULL;
    }

    return cx->fp()->getScript()->pcToNative(pc);
}

void JS_FASTCALL
stubs::GetCallObject(VMFrame &f)
{
    JS_ASSERT(f.fp()->getFunction()->isHeavyweight());
    if (!js_GetCallObject(f.cx, f.fp()))
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
        JSStackFrame *fp = cx->fp();

        
        if (fp->hasIMacroPC() && cx->throwing) {
            cx->regs->pc = fp->getIMacroPC();
            fp->clearIMacroPC();
            if (ok)
                break;
        }
        JS_ASSERT(!fp->hasIMacroPC());

        
        jsbytecode *pc = FindExceptionHandler(cx);
        if (pc) {
            cx->regs->pc = pc;
            ok = true;
            break;
        }

        
        if (fp == stopFp)
            break;

        
        ok &= bool(js_UnwindScope(cx, 0, cx->throwing));
        InlineReturn(f, ok);
    }

    
    JS_ASSERT(&f.regs == cx->regs);

    JS_ASSERT_IF(!ok, cx->fp() == stopFp);
    return ok;
}

static inline bool
AtSafePoint(JSContext *cx)
{
    JSStackFrame *fp = cx->fp();
    if (fp->hasIMacroPC())
        return false;

    JSScript *script = fp->getScript();
    if (!script->nmap)
        return false;

    JS_ASSERT(cx->regs->pc >= script->code && cx->regs->pc < script->code + script->length);
    return !!script->nmap[cx->regs->pc - script->code];
}

static inline JSBool
PartialInterpret(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = cx->fp();

    JS_ASSERT(fp->hasIMacroPC() || !fp->getScript()->nmap ||
              !fp->getScript()->nmap[cx->regs->pc - fp->getScript()->code]);

    JSBool ok = JS_TRUE;
    fp->flags |= JSFRAME_BAILING;
    ok = Interpret(cx, fp);
    fp->flags &= ~JSFRAME_BAILING;

    f.fp() = cx->fp();

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
    while (cx->fp() != entryFrame) {
        JSStackFrame *fp = cx->fp();
        fp->flags &= ~JSFRAME_RECORDING;

        if (AtSafePoint(cx)) {
            JSScript *script = fp->getScript();
            if (!JaegerShotAtSafePoint(cx, script->nmap[cx->regs->pc - script->code])) {
                if (!SwallowErrors(f, entryFrame))
                    return false;

                
                continue;
            }
            InlineReturn(f, JS_TRUE);
            AdvanceReturnPC(cx);
        } else {
            if (!PartialInterpret(f)) {
                if (!SwallowErrors(f, entryFrame))
                    return false;
            } else {
                



                if (!cx->fp()->hasIMacroPC() && FrameIsFinished(cx)) {
                    JSOp op = JSOp(*cx->regs->pc);
                    if (op == JSOP_RETURN && !(cx->fp()->flags & JSFRAME_BAILED_AT_RETURN))
                        fp->setReturnValue(f.regs.sp[-1]);
                    InlineReturn(f, JS_TRUE);
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

    if (mic.u.hasSlowTraceHint) {
        addr = (uint8 *)(mic.slowTraceHint.executableAddress());
        JSC::RepatchBuffer repatch(addr - 64, 128);
        repatch.relink(mic.slowTraceHint, mic.load);

        JaegerSpew(JSpew_PICs, "relinking trace hint %p to %p\n",
                   mic.slowTraceHint.executableAddress(),
                   mic.load.executableAddress());
    }
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
    JSStackFrame *entryFrame = f.fp();
    TracePointAction tpa;

    
    if (!cx->jitEnabled)
        return NULL;

    bool blacklist;
    uintN inlineCallCount = 0;
    tpa = MonitorTracePoint(f.cx, inlineCallCount, blacklist);
    JS_ASSERT(!TRACE_RECORDER(cx));

#if JS_MONOIC
    if (blacklist)
        DisableTraceHint(f, mic);
#endif

    if ((tpa == TPA_RanStuff || tpa == TPA_Recorded) && cx->throwing)
        tpa = TPA_Error;

	
	f.fp() = cx->fp();

    switch (tpa) {
      case TPA_Nothing:
        return NULL;

      case TPA_Error:
        if (!SwallowErrors(f, entryFrame))
            THROWV(NULL);
        JS_ASSERT(!cx->fp()->hasIMacroPC());
        break;

      case TPA_RanStuff:
      case TPA_Recorded:
        break;
    }

    





















  restart:
    
    if (!RemoveExcessFrames(f, entryFrame))
        THROWV(NULL);

    
    entryFrame->flags &= ~JSFRAME_RECORDING;
    while (entryFrame->hasIMacroPC()) {
        if (!PartialInterpret(f)) {
            if (!SwallowErrors(f, entryFrame))
                THROWV(NULL);
        }

        
        goto restart;
    }

    
    if (AtSafePoint(cx)) {
        uint32 offs = uint32(cx->regs->pc - entryFrame->getScript()->code);
        JS_ASSERT(entryFrame->getScript()->nmap[offs]);
        return entryFrame->getScript()->nmap[offs];
    }

    
    if (JSOp op = FrameIsFinished(cx)) {
        
        if (op == JSOP_RETURN && !(entryFrame->flags & JSFRAME_BAILED_AT_RETURN))
            entryFrame->setReturnValue(f.regs.sp[-1]);

        
        if (f.fp() != f.entryFp) {
            if (!InlineReturn(f, JS_TRUE))
                THROWV(NULL);
        }
        void *retPtr = JS_FUNC_TO_DATA_PTR(void *, InjectJaegerReturn);
        *f.returnAddressLocation() = retPtr;
        return NULL;
    }

    
    if (!PartialInterpret(f)) {
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
    JSScript *script = f.fp()->getScript();
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

