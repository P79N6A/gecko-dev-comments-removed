






#include "mozilla/DebugOnly.h"

#include "jsanalyze.h"
#include "jscntxt.h"
#include "jsobj.h"
#include "jslibmath.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsbool.h"
#include "jstypes.h"

#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/assembler/CodeLocation.h"
#include "builtin/Eval.h"
#include "methodjit/StubCalls.h"
#include "methodjit/MonoIC.h"
#include "methodjit/BaseCompiler.h"
#include "methodjit/ICRepatcher.h"
#include "vm/Debugger.h"
#include "vm/Shape.h"

#include "jsinterpinlines.h"
#include "jsscriptinlines.h"
#include "jsobjinlines.h"
#include "jscntxtinlines.h"
#include "jsatominlines.h"

#include "StubCalls-inl.h"

#include "vm/Shape-inl.h"

#include "jsautooplen.h"

#include "ion/Ion.h"

using namespace js;
using namespace js::mjit;
using namespace JSC;

using mozilla::DebugOnly;

using ic::Repatcher;

static jsbytecode *
FindExceptionHandler(JSContext *cx)
{
    StackFrame *fp = cx->fp();
    RootedScript script(cx, fp->script());

    if (!script->hasTrynotes())
        return NULL;

  error:
    if (cx->isExceptionPending()) {
        for (TryNoteIter tni(cx, cx->regs()); !tni.done(); ++tni) {
            JSTryNote *tn = *tni;

            UnwindScope(cx, cx->fp(), tn->stackDepth);

            




            jsbytecode *pc = script->main() + tn->start + tn->length;
            cx->regs().pc = pc;
            cx->regs().sp = cx->regs().spForStackDepth(tn->stackDepth);

            switch (tn->kind) {
                case JSTRY_CATCH:
                  JS_ASSERT(JSOp(*pc) == JSOP_ENTERBLOCK);

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
                  






                  JS_ASSERT(JSOp(*pc) == JSOP_ENDITER);
                  RootedObject obj(cx, &cx->regs().sp[-1].toObject());
                  bool ok = UnwindIteratorForException(cx, obj);
                  cx->regs().sp -= 1;
                  if (!ok)
                      goto error;
                }
            }
        }
    } else {
        UnwindForUncatchableException(cx, cx->regs());
    }

    return NULL;
}





static inline bool
MaybeCloneAndPatchCallee(JSContext *cx, CallArgs args, HandleScript script, jsbytecode *pc)
{
    if (cx->typeInferenceEnabled() && !args.calleev().isPrimitive() &&
        args.callee().isFunction() && args.callee().toFunction()->hasScript() &&
        args.callee().toFunction()->nonLazyScript()->shouldCloneAtCallsite)
    {
        RootedFunction fun(cx, args.callee().toFunction());
        fun = CloneFunctionAtCallsite(cx, fun, script, pc);
        if (!fun)
            return false;
        args.setCallee(ObjectValue(*fun));
    }

    return true;
}

void JS_FASTCALL
stubs::SlowCall(VMFrame &f, uint32_t argc)
{
    if (*f.regs.pc == JSOP_FUNAPPLY && !GuardFunApplyArgumentsOptimization(f.cx))
        THROW();

    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    RootedScript fscript(f.cx, f.script());

    if (!MaybeCloneAndPatchCallee(f.cx, args, fscript, f.pc()))
        THROW();
    if (!InvokeKernel(f.cx, args))
        THROW();

    types::TypeScript::Monitor(f.cx, fscript, f.pc(), args.rval());
}

void JS_FASTCALL
stubs::SlowNew(VMFrame &f, uint32_t argc)
{
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    RootedScript fscript(f.cx, f.script());

    if (!MaybeCloneAndPatchCallee(f.cx, args, fscript, f.pc()))
        THROW();
    if (!InvokeConstructorKernel(f.cx, args))
        THROW();

    types::TypeScript::Monitor(f.cx, fscript, f.pc(), args.rval());
}

static inline bool
CheckStackQuota(VMFrame &f)
{
    JS_ASSERT(f.regs.stackDepth() == 0);

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
stubs::FixupArity(VMFrame &f, uint32_t nactual)
{
    JSContext *cx = f.cx;
    StackFrame *oldfp = f.fp();

    JS_ASSERT(nactual != oldfp->numFormalArgs());

    




    InitialFrameFlags initial = oldfp->initialFlags();
    RootedFunction fun(cx, oldfp->fun());
    RootedScript script(cx, fun->nonLazyScript());
    void *ncode = oldfp->nativeReturnAddress();

    
    f.regs.popPartialFrame((Value *)oldfp);

    
    CallArgs args = CallArgsFromSp(nactual, f.regs.sp);
    if (script->isCallsiteClone) {
        JS_ASSERT(args.callee().toFunction() == script->originalFunction());
        args.setCallee(ObjectValue(*fun));
    }
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
stubs::CompileFunction(VMFrame &f, uint32_t argc)
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
ShouldJaegerCompileCallee(JSContext *cx, JSScript *caller, JSScript *callee, JITScript *callerJit)
{
#ifdef JS_ION
    if (!ion::IsEnabled(cx))
        return true;

    
    if (!callee->canIonCompile())
        return true;

    
    
    if (!callee->hasAnalysis())
        return true;

    if (callee->isShortRunning())
        return true;

    return false;
#endif
    return true;
}

static inline bool
UncachedInlineCall(VMFrame &f, InitialFrameFlags initial,
                   void **pret, bool *unjittable, uint32_t argc)
{
    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    RootedFunction newfun(cx, args.callee().toFunction());

    RootedScript newscript(cx, newfun->nonLazyScript());
    if (!newscript)
        return false;

    bool construct = InitialFrameFlagsAreConstructing(initial);

    RootedScript fscript(cx, f.script());
    bool newType = construct && cx->typeInferenceEnabled() &&
        types::UseNewType(cx, fscript, f.pc());

    if (!types::TypeMonitorCall(cx, args, construct))
        return false;

    
    if (ShouldJaegerCompileCallee(cx, f.script(), newscript, f.jit())) {
        CompileStatus status = CanMethodJIT(cx, newscript, newscript->code, construct,
                                            CompileRequest_JIT, f.fp());
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

    
    if (!cx->stack.pushInlineFrame(cx, regs, args, newfun, newscript, initial, &f.stackLimit))
        return false;

    
    PreserveRegsGuard regsGuard(cx, regs);

    



    if (!newType) {
        if (JITScript *jit = newscript->getJIT(regs.fp()->isConstructing(), cx->zone()->compileBarriers())) {
            if (jit->invokeEntry) {
                *pret = jit->invokeEntry;

                
                regs.popFrame((Value *) regs.fp());
                return true;
            }
        }
    }

    





    if (f.regs.inlined()) {
        ExpandInlineFrames(cx->compartment);
        JS_ASSERT(!f.regs.inlined());
        regs.fp()->resetInlinePrev(f.fp(), f.regs.pc);
    }

    JS_CHECK_RECURSION(cx, return false);

    bool ok = RunScript(cx, cx->fp());
    f.cx->stack.popInlineFrame(regs);

    if (ok) {
        RootedScript fscript(cx, f.script());
        types::TypeScript::Monitor(f.cx, fscript, f.pc(), args.rval());
    }

    *pret = NULL;
    return ok;
}

void * JS_FASTCALL
stubs::UncachedNew(VMFrame &f, uint32_t argc)
{
    UncachedCallResult ucr(f.cx);
    UncachedNewHelper(f, argc, ucr);
    return ucr.codeAddr;
}

void
stubs::UncachedNewHelper(VMFrame &f, uint32_t argc, UncachedCallResult &ucr)
{
    ucr.init();
    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    RootedScript fscript(cx, f.script());

    if (!ucr.setFunction(cx, args, fscript, f.pc()))
        THROW();

    
    if (ucr.fun && ucr.fun->isInterpretedConstructor()) {
        if (!UncachedInlineCall(f, INITIAL_CONSTRUCT, &ucr.codeAddr, &ucr.unjittable, argc))
            THROW();
    } else {
        if (!InvokeConstructorKernel(cx, args))
            THROW();
        types::TypeScript::Monitor(f.cx, fscript, f.pc(), args.rval());
    }
}

void * JS_FASTCALL
stubs::UncachedCall(VMFrame &f, uint32_t argc)
{
    UncachedCallResult ucr(f.cx);
    UncachedCallHelper(f, argc, false, ucr);
    return ucr.codeAddr;
}

void * JS_FASTCALL
stubs::UncachedLoweredCall(VMFrame &f, uint32_t argc)
{
    UncachedCallResult ucr(f.cx);
    UncachedCallHelper(f, argc, true, ucr);
    return ucr.codeAddr;
}

void JS_FASTCALL
stubs::Eval(VMFrame &f, uint32_t argc)
{
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);

    if (!IsBuiltinEvalForScope(f.fp()->scopeChain(), args.calleev())) {
        if (!InvokeKernel(f.cx, args))
            THROW();

        RootedScript fscript(f.cx, f.script());
        types::TypeScript::Monitor(f.cx, fscript, f.pc(), args.rval());
        return;
    }

    JS_ASSERT(f.fp() == f.cx->fp());
    if (!DirectEval(f.cx, args))
        THROW();

    RootedScript fscript(f.cx, f.script());
    types::TypeScript::Monitor(f.cx, fscript, f.pc(), args.rval());
}

void
stubs::UncachedCallHelper(VMFrame &f, uint32_t argc, bool lowered, UncachedCallResult &ucr)
{
    ucr.init();

    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    RootedScript fscript(cx, f.script());

    if (!ucr.setFunction(cx, args, fscript, f.pc()))
        THROW();

    if (ucr.fun) {
        if (ucr.fun->isInterpreted()) {
            InitialFrameFlags initial = lowered ? INITIAL_LOWERED : INITIAL_NONE;
            if (!UncachedInlineCall(f, initial, &ucr.codeAddr, &ucr.unjittable, argc))
                THROW();
            return;
        }

        if (ucr.fun->isNative()) {
            if (!CallJSNative(cx, ucr.fun->native(), args))
                THROW();
            RootedScript fscript(cx, f.script());
            types::TypeScript::Monitor(f.cx, fscript, f.pc(), args.rval());
            return;
        }
    }

    if (!InvokeKernel(f.cx, args))
        THROW();

    types::TypeScript::Monitor(f.cx, fscript, f.pc(), args.rval());
    return;
}

static void
RemoveOrphanedNative(JSContext *cx, StackFrame *fp)
{
    





    JaegerRuntime &jr = cx->jaegerRuntime();
    if (jr.orphanedNativeFrames.empty())
        return;
    for (unsigned i = 0; i < jr.orphanedNativeFrames.length(); i++) {
        if (fp == jr.orphanedNativeFrames[i]) {
            jr.orphanedNativeFrames[i] = jr.orphanedNativeFrames.back();
            jr.orphanedNativeFrames.popBack();
            break;
        }
    }
    if (jr.orphanedNativeFrames.empty()) {
        for (unsigned i = 0; i < jr.orphanedNativePools.length(); i++)
            jr.orphanedNativePools[i]->release();
        jr.orphanedNativePools.clear();
    }
}

extern "C" void *
js_InternalThrow(VMFrame &f)
{
    JSContext *cx = f.cx;

    ExpandInlineFrames(cx->compartment);

    
    
    RemoveOrphanedNative(cx, f.fp());

    JS_ASSERT(!f.fp()->finishedInInterpreter());

    
    JS_ASSERT(&cx->regs() == &f.regs);

    jsbytecode *pc = NULL;
    for (;;) {
        if (cx->isExceptionPending()) {
            
            JSThrowHook handler = cx->runtime->debugHooks.throwHook;
            if (handler || !cx->compartment->getDebuggees().empty()) {
                RootedValue rval(cx);
                JSTrapStatus st = Debugger::onExceptionUnwind(cx, &rval);
                if (st == JSTRAP_CONTINUE && handler) {
                    RootedScript fscript(cx, cx->fp()->script());
                    st = handler(cx, fscript, cx->regs().pc, rval.address(),
                                 cx->runtime->debugHooks.throwHookData);
                }

                switch (st) {
                case JSTRAP_ERROR:
                    cx->clearPendingException();
                    break;

                case JSTRAP_CONTINUE:
                    break;

                case JSTRAP_RETURN:
                    cx->clearPendingException();
                    cx->fp()->setReturnValue(rval);
                    return cx->jaegerRuntime().forceReturnFromExternC();

                case JSTRAP_THROW:
                    cx->setPendingException(rval);
                    break;

                default:
                    JS_NOT_REACHED("bad onExceptionUnwind status");
                }
            }
        }

        pc = FindExceptionHandler(cx);
        if (pc)
            break;

        
        
        
        
        
        JS_ASSERT(!f.fp()->finishedInInterpreter());
        UnwindScope(cx, cx->fp(), 0);
        f.regs.setToEndOfScript();

        if (cx->compartment->debugMode()) {
            
            
            
            if (js::ScriptDebugEpilogue(cx, f.fp(), false))
                return cx->jaegerRuntime().forceReturnFromExternC();
        }


        f.fp()->epilogue(f.cx);

        
        
        
        if (f.entryfp == f.fp())
            break;

        f.cx->stack.popInlineFrame(f.regs);
        DebugOnly<JSOp> op = JSOp(*f.regs.pc);
        JS_ASSERT(op == JSOP_CALL ||
                  op == JSOP_NEW ||
                  op == JSOP_EVAL ||
                  op == JSOP_FUNCALL ||
                  op == JSOP_FUNAPPLY);
        f.regs.pc += JSOP_CALL_LENGTH;
    }

    JS_ASSERT(&cx->regs() == &f.regs);

    if (!pc)
        return NULL;

    StackFrame *fp = cx->fp();
    RootedScript script(cx, fp->script());

    






    cx->jaegerRuntime().setLastUnfinished(Jaeger_Unfinished);

    if (!script->ensureRanAnalysis(cx)) {
        js_ReportOutOfMemory(cx);
        return NULL;
    }

    types::AutoEnterAnalysis enter(cx);

    




    if (cx->isExceptionPending()) {
        JS_ASSERT(JSOp(*pc) == JSOP_ENTERBLOCK);
        StaticBlockObject &blockObj = script->getObject(GET_UINT32_INDEX(pc))->asStaticBlock();
        Value *vp = cx->regs().sp + blockObj.slotCount();
        SetValueRangeToUndefined(cx->regs().sp, vp);
        cx->regs().sp = vp;
        if (!cx->regs().fp()->pushBlock(cx, blockObj))
            return NULL;

        JS_ASSERT(JSOp(pc[JSOP_ENTERBLOCK_LENGTH]) == JSOP_EXCEPTION);
        cx->regs().sp[0] = cx->getPendingException();
        cx->clearPendingException();
        cx->regs().sp++;

        cx->regs().pc = pc + JSOP_ENTERBLOCK_LENGTH + JSOP_EXCEPTION_LENGTH;
    }

    *f.oldregs = f.regs;

    return NULL;
}

void JS_FASTCALL
stubs::CreateThis(VMFrame &f, JSObject *proto)
{
    JSContext *cx = f.cx;
    StackFrame *fp = f.fp();
    RootedObject callee(cx, &fp->callee());
    JSObject *obj = CreateThisForFunctionWithProto(cx, callee, proto);
    if (!obj)
        THROW();
    fp->thisValue() = ObjectValue(*obj);
}

void JS_FASTCALL
stubs::ScriptDebugPrologue(VMFrame &f)
{
    Probes::enterScript(f.cx, f.script(), f.script()->function(), f.fp());
    JSTrapStatus status = js::ScriptDebugPrologue(f.cx, f.fp());
    switch (status) {
      case JSTRAP_CONTINUE:
        break;
      case JSTRAP_RETURN:
        *f.returnAddressLocation() = f.cx->jaegerRuntime().forceReturnFromFastCall();
        return;
      case JSTRAP_ERROR:
      case JSTRAP_THROW:
        THROW();
      default:
        JS_NOT_REACHED("bad ScriptDebugPrologue status");
    }
}

void JS_FASTCALL
stubs::ScriptDebugEpilogue(VMFrame &f)
{
    if (!js::ScriptDebugEpilogue(f.cx, f.fp(), JS_TRUE))
        THROW();
}

void JS_FASTCALL
stubs::ScriptProbeOnlyPrologue(VMFrame &f)
{
    Probes::enterScript(f.cx, f.script(), f.script()->function(), f.fp());
}

void JS_FASTCALL
stubs::ScriptProbeOnlyEpilogue(VMFrame &f)
{
    Probes::exitScript(f.cx, f.script(), f.script()->function(), f.fp());
}

void JS_FASTCALL
stubs::CrossChunkShim(VMFrame &f, void *edge_)
{
    DebugOnly<CrossChunkEdge*> edge = (CrossChunkEdge *) edge_;

    mjit::ExpandInlineFrames(f.cx->compartment);

    RootedScript script(f.cx, f.script());
    JS_ASSERT(edge->target < script->length);
    JS_ASSERT(script->code + edge->target == f.pc());

    CompileStatus status = CanMethodJIT(f.cx, script, f.pc(),
                                        f.fp()->isConstructing(),
                                        CompileRequest_Interpreter, f.fp());
    if (status == Compile_Error)
        THROW();

    void **addr = f.returnAddressLocation();
    *addr = JS_FUNC_TO_DATA_PTR(void *, JaegerInterpoline);

    f.fp()->setRejoin(StubRejoin(REJOIN_RESUME));
}

JS_STATIC_ASSERT(JSOP_NOP == 0);


#if defined(JS_METHODJIT_SPEW)
static const char *OpcodeNames[] = {
# define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) #name,
# include "jsopcode.tbl"
# undef OPDEF
};
#endif

extern "C" void *
js_InternalInterpret(void *returnData, void *returnType, void *returnReg, js::VMFrame &f)
{
    FrameRejoinState jsrejoin = f.fp()->rejoin();
    RejoinState rejoin;
    if (jsrejoin & 0x1) {
        
        uint32_t pcOffset = jsrejoin >> 1;
        f.regs.pc = f.fp()->script()->code + pcOffset;
        f.regs.clearInlined();
        rejoin = REJOIN_SCRIPTED;
    } else {
        rejoin = (RejoinState) (jsrejoin >> 1);
    }

    JSContext *cx = f.cx;
    StackFrame *fp = f.regs.fp();
    RootedScript script(cx, fp->script());

    jsbytecode *pc = f.regs.pc;

    JSOp op = JSOp(*pc);

    if (!script->ensureRanAnalysis(cx)) {
        js_ReportOutOfMemory(cx);
        return js_InternalThrow(f);
    }

    mozilla::Maybe<types::AutoEnterAnalysis> enter;
    enter.construct(cx);

    analyze::ScriptAnalysis *analysis = script->analysis();

    




    Value *oldsp = f.regs.sp;
    f.regs.sp = f.regs.spForStackDepth(analysis->getCode(pc).stackDepth);

    jsbytecode *nextpc = pc + GetBytecodeLength(pc);
    Value *nextsp = NULL;
    if (nextpc != script->code + script->length && analysis->maybeCode(nextpc))
        nextsp = f.regs.spForStackDepth(analysis->getCode(nextpc).stackDepth);

    JS_ASSERT(&cx->regs() == &f.regs);

#ifdef JS_METHODJIT_SPEW
    JaegerSpew(JSpew_Recompile, "interpreter rejoin (file \"%s\") (line \"%d\") (op %s) (opline \"%d\")\n",
               script->filename, script->lineno, OpcodeNames[op], PCToLineNumber(script, pc));
#endif

    uint32_t nextDepth = UINT32_MAX;
    bool skipTrap = false;

    switch (rejoin) {
      case REJOIN_SCRIPTED: {
        jsval_layout rval;
#ifdef JS_NUNBOX32
        rval.asBits = ((uint64_t)returnType << 32) | (uint32_t)returnData;
#elif JS_PUNBOX64
        rval.asBits = (uint64_t)returnType | (uint64_t)returnData;
#else
#error "Unknown boxing format"
#endif

        nextsp[-1] = IMPL_TO_JSVAL(rval);

        



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
        



        if (script->hasBreakpointsAt(pc))
            skipTrap = true;
        break;

      case REJOIN_FALLTHROUGH:
        f.regs.pc = nextpc;
        break;

      case REJOIN_NATIVE:
      case REJOIN_NATIVE_LOWERED:
      case REJOIN_NATIVE_GETTER: {
        




        if (rejoin != REJOIN_NATIVE)
            nextsp[-1] = nextsp[0];

        
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

      case REJOIN_THIS_PROTOTYPE: {
        RootedObject callee(cx, &fp->callee());
        JSObject *proto = f.regs.sp[0].isObject() ? &f.regs.sp[0].toObject() : NULL;
        JSObject *obj = CreateThisForFunctionWithProto(cx, callee, proto);
        if (!obj)
            return js_InternalThrow(f);
        fp->thisValue() = ObjectValue(*obj);
        
      }

      case REJOIN_THIS_CREATED: {
        Probes::enterScript(f.cx, f.script(), f.script()->function(), fp);

        if (script->debugMode) {
            JSTrapStatus status = js::ScriptDebugPrologue(f.cx, f.fp());
            switch (status) {
              case JSTRAP_CONTINUE:
                break;
              case JSTRAP_RETURN: {
                
                f.regs.pc = script->code + script->length - 1;
                nextDepth = 0;
                JS_ASSERT(*f.regs.pc == JSOP_STOP);
                break;
              }
              case JSTRAP_THROW:
              case JSTRAP_ERROR:
                return js_InternalThrow(f);
              default:
                JS_NOT_REACHED("bad ScriptDebugPrologue status");
            }
        }

        break;
      }

      



      case REJOIN_CHECK_ARGUMENTS:
        if (!CheckStackQuota(f))
            return js_InternalThrow(f);
        fp->initVarsToUndefined();
        fp->scopeChain();
        if (types::UseNewTypeAtEntry(cx, fp))
            fp->setUseNewType();
        if (!fp->prologue(cx))
            return js_InternalThrow(f);

        








        JS_ASSERT(!cx->compartment->debugMode());
        break;

      
      case REJOIN_FUNCTION_PROLOGUE:
        if (fp->isConstructing()) {
            RootedObject callee(cx, &fp->callee());
            JSObject *obj = CreateThisForFunction(cx, callee, types::UseNewTypeAtEntry(cx, fp));
            if (!obj)
                return js_InternalThrow(f);
            fp->functionThis() = ObjectValue(*obj);
        }
        
      case REJOIN_EVAL_PROLOGUE:
        Probes::enterScript(cx, f.script(), f.script()->function(), fp);
        if (cx->compartment->debugMode()) {
            JSTrapStatus status = ScriptDebugPrologue(cx, fp);
            switch (status) {
              case JSTRAP_CONTINUE:
                break;
              case JSTRAP_RETURN:
                return f.cx->jaegerRuntime().forceReturnFromFastCall();
              case JSTRAP_ERROR:
              case JSTRAP_THROW:
                return js_InternalThrow(f);
              default:
                JS_NOT_REACHED("bad ScriptDebugPrologue status");
            }
        }
        break;

      case REJOIN_CALL_PROLOGUE:
      case REJOIN_CALL_PROLOGUE_LOWERED_CALL:
      case REJOIN_CALL_PROLOGUE_LOWERED_APPLY:
        if (returnReg) {
            uint32_t argc = 0;
            if (rejoin == REJOIN_CALL_PROLOGUE)
                argc = GET_ARGC(pc);
            else if (rejoin == REJOIN_CALL_PROLOGUE_LOWERED_CALL)
                argc = GET_ARGC(pc) - 1;
            else
                argc = f.u.call.dynamicArgc;

            







            f.regs.restorePartialFrame(oldsp); 
            f.scratch = (void *) uintptr_t(argc); 
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
        enter.destroy();
        f.regs.sp = nextsp + 2 + f.u.call.dynamicArgc;
        if (!InvokeKernel(cx, CallArgsFromSp(f.u.call.dynamicArgc, f.regs.sp)))
            return js_InternalThrow(f);
        nextsp[-1] = nextsp[0];
        f.regs.pc = nextpc;
        break;
      }

      case REJOIN_GETTER:
        



        switch (op) {
          case JSOP_INSTANCEOF: {
            




            if (f.regs.sp[0].isPrimitive()) {
                RootedValue val(cx, f.regs.sp[-1]);
                js_ReportValueError(cx, JSMSG_BAD_PROTOTYPE, -1, val, NullPtr());
                return js_InternalThrow(f);
            }
            bool isDelegate;
            RootedObject obj(cx, &f.regs.sp[0].toObject());
            if (!IsDelegate(cx, obj, f.regs.sp[-2], &isDelegate))
                return js_InternalThrow(f);
            nextsp[-1].setBoolean(isDelegate);
            f.regs.pc = nextpc;
            break;
          }

          default:
            f.regs.pc = nextpc;
            break;
        }
        break;

      case REJOIN_BRANCH: {
        




        bool takeBranch = false;
        switch (JSOp(*nextpc)) {
          case JSOP_IFNE:
            takeBranch = returnReg != NULL;
            break;
          case JSOP_IFEQ:
            takeBranch = returnReg == NULL;
            break;
          default:
            JS_NOT_REACHED("Bad branch op");
        }
        if (takeBranch)
            f.regs.pc = nextpc + GET_JUMP_OFFSET(nextpc);
        else
            f.regs.pc = nextpc + GetBytecodeLength(nextpc);
        break;
      }

      default:
        JS_NOT_REACHED("Missing rejoin");
    }

    if (nextDepth == UINT32_MAX)
        nextDepth = analysis->getCode(f.regs.pc).stackDepth;
    f.regs.sp = f.regs.spForStackDepth(nextDepth);

    




    if (f.regs.pc == nextpc && (js_CodeSpec[op].format & JOF_TYPESET))
        types::TypeScript::Monitor(cx, script, pc, f.regs.sp[-1]);

    
    JaegerStatus status = skipTrap ? Jaeger_UnfinishedAtTrap : Jaeger_Unfinished;
    cx->jaegerRuntime().setLastUnfinished(status);
    *f.oldregs = f.regs;

    return NULL;
}
