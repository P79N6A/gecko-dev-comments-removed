






#include "jsanalyze.h"
#include "jscntxt.h"
#include "jsscope.h"
#include "jsobj.h"
#include "jslibmath.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsxml.h"
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

#include "jsinterpinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsobjinlines.h"
#include "jscntxtinlines.h"
#include "jsatominlines.h"

#include "StubCalls-inl.h"

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

    if (!script->hasTrynotes())
        return NULL;

  error:
    if (cx->isExceptionPending()) {
        for (TryNoteIter tni(cx->regs()); !tni.done(); ++tni) {
            JSTryNote *tn = *tni;

            UnwindScope(cx, tn->stackDepth);

            




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
                  bool ok = UnwindIteratorForException(cx, &cx->regs().sp[-1].toObject());
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





void JS_FASTCALL
stubs::SlowCall(VMFrame &f, uint32_t argc)
{
    if (*f.regs.pc == JSOP_FUNAPPLY && !GuardFunApplyArgumentsOptimization(f.cx))
        THROW();

    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    if (!InvokeKernel(f.cx, args))
        THROW();

    types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
}

void JS_FASTCALL
stubs::SlowNew(VMFrame &f, uint32_t argc)
{
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    if (!InvokeConstructorKernel(f.cx, args))
        THROW();

    types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
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
UncachedInlineCall(VMFrame &f, InitialFrameFlags initial,
                   void **pret, bool *unjittable, uint32_t argc)
{
    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);
    JSFunction *newfun = args.callee().toFunction();
    JSScript *newscript = newfun->script();

    bool construct = InitialFrameFlagsAreConstructing(initial);

    bool newType = construct && cx->typeInferenceEnabled() &&
        types::UseNewType(cx, f.script(), f.pc());

    if (!types::TypeMonitorCall(cx, args, construct))
        return false;

    
    CompileStatus status = CanMethodJIT(cx, newscript, newscript->code, construct,
                                        CompileRequest_Interpreter, f.fp());
    if (status == Compile_Error) {
        
        return false;
    }
    if (status == Compile_Abort)
        *unjittable = true;

    




    if (f.regs.inlined() && newfun->isHeavyweight()) {
        ExpandInlineFrames(cx->compartment);
        JS_ASSERT(!f.regs.inlined());
    }

    






    FrameRegs regs = f.regs;

    
    if (!cx->stack.pushInlineFrame(cx, regs, args, *newfun, newscript, initial, &f.stackLimit))
        return false;

    
    PreserveRegsGuard regsGuard(cx, regs);

    



    if (!newType) {
        if (JITScript *jit = newscript->getJIT(regs.fp()->isConstructing(), cx->compartment->needsBarrier())) {
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

    bool ok = Interpret(cx, cx->fp());
    f.cx->stack.popInlineFrame(regs);

    if (ok)
        types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());

    *pret = NULL;
    return ok;
}

void * JS_FASTCALL
stubs::UncachedNew(VMFrame &f, uint32_t argc)
{
    UncachedCallResult ucr;
    UncachedNewHelper(f, argc, &ucr);
    return ucr.codeAddr;
}

void
stubs::UncachedNewHelper(VMFrame &f, uint32_t argc, UncachedCallResult *ucr)
{
    ucr->init();
    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);

    
    if (IsFunctionObject(args.calleev(), &ucr->fun) && ucr->fun->isInterpretedConstructor()) {
        if (!UncachedInlineCall(f, INITIAL_CONSTRUCT, &ucr->codeAddr, &ucr->unjittable, argc))
            THROW();
    } else {
        if (!InvokeConstructorKernel(cx, args))
            THROW();
        types::TypeScript::Monitor(f.cx, f.script(), f.pc(), args.rval());
    }
}

void * JS_FASTCALL
stubs::UncachedCall(VMFrame &f, uint32_t argc)
{
    UncachedCallResult ucr;
    UncachedCallHelper(f, argc, false, &ucr);
    return ucr.codeAddr;
}

void * JS_FASTCALL
stubs::UncachedLoweredCall(VMFrame &f, uint32_t argc)
{
    UncachedCallResult ucr;
    UncachedCallHelper(f, argc, true, &ucr);
    return ucr.codeAddr;
}

void JS_FASTCALL
stubs::Eval(VMFrame &f, uint32_t argc)
{
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);

    if (!IsBuiltinEvalForScope(f.fp()->scopeChain(), args.calleev())) {
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
stubs::UncachedCallHelper(VMFrame &f, uint32_t argc, bool lowered, UncachedCallResult *ucr)
{
    ucr->init();

    JSContext *cx = f.cx;
    CallArgs args = CallArgsFromSp(argc, f.regs.sp);

    if (IsFunctionObject(args.calleev(), &ucr->fun)) {
        if (ucr->fun->isInterpreted()) {
            InitialFrameFlags initial = lowered ? INITIAL_LOWERED : INITIAL_NONE;
            if (!UncachedInlineCall(f, initial, &ucr->codeAddr, &ucr->unjittable, argc))
                THROW();
            return;
        }

        if (ucr->fun->isNative()) {
            if (!CallJSNative(cx, ucr->fun->native(), args))
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
                Value rval;
                JSTrapStatus st = Debugger::onExceptionUnwind(cx, &rval);
                if (st == JSTRAP_CONTINUE && handler) {
                    st = handler(cx, cx->fp()->script(), cx->regs().pc, &rval,
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
        UnwindScope(cx, 0);
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
    JSScript *script = fp->script();

    






    cx->jaegerRuntime().setLastUnfinished(Jaeger_Unfinished);

    if (!script->ensureRanAnalysis(cx)) {
        js_ReportOutOfMemory(cx);
        return NULL;
    }

    analyze::AutoEnterAnalysis enter(cx);

    




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
    JSObject *obj = js_CreateThisForFunctionWithProto(cx, callee, proto);
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

    JSScript *script = f.script();
    JS_ASSERT(edge->target < script->length);
    JS_ASSERT(script->code + edge->target == f.pc());

    CompileStatus status = CanMethodJIT(f.cx, script, f.pc(), f.fp()->isConstructing(),
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

static void
FinishVarIncOp(VMFrame &f, RejoinState rejoin, Value ov, Value nv, Value *vp)
{
    
    JS_ASSERT(rejoin == REJOIN_POS || rejoin == REJOIN_BINARY);

    JSContext *cx = f.cx;

    JSOp op = JSOp(*f.pc());
    JS_ASSERT(op == JSOP_LOCALINC || op == JSOP_INCLOCAL ||
              op == JSOP_LOCALDEC || op == JSOP_DECLOCAL ||
              op == JSOP_ARGINC || op == JSOP_INCARG ||
              op == JSOP_ARGDEC || op == JSOP_DECARG);
    const JSCodeSpec *cs = &js_CodeSpec[op];

    if (rejoin == REJOIN_POS) {
        double d = ov.toNumber();
        double N = (cs->format & JOF_INC) ? 1 : -1;
        if (!nv.setNumber(d + N))
            types::TypeScript::MonitorOverflow(cx, f.script(), f.pc());
    }

    unsigned i = GET_SLOTNO(f.pc());
    if (JOF_TYPE(cs->format) == JOF_LOCAL)
        f.fp()->unaliasedLocal(i) = nv;
    else if (f.fp()->script()->argsObjAliasesFormals())
        f.fp()->argsObj().setArg(i, nv);
    else
        f.fp()->unaliasedFormal(i) = nv;

    *vp = (cs->format & JOF_POST) ? ov : nv;
}

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
    JSScript *script = fp->script();

    jsbytecode *pc = f.regs.pc;

    JSOp op = JSOp(*pc);
    const JSCodeSpec *cs = &js_CodeSpec[op];

    if (!script->ensureRanAnalysis(cx)) {
        js_ReportOutOfMemory(cx);
        return js_InternalThrow(f);
    }

    analyze::AutoEnterAnalysis enter(cx);
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

    if ((cs->format & (JOF_INC | JOF_DEC)) &&
        (rejoin == REJOIN_POS || rejoin == REJOIN_BINARY)) {
        




        JS_ASSERT(cs->format & (JOF_LOCAL | JOF_QARG));

        nextDepth = analysis->getCode(nextpc).stackDepth;
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
        JSObject *obj = js_CreateThisForFunctionWithProto(cx, callee, proto);
        if (!obj)
            return js_InternalThrow(f);
        fp->thisValue() = ObjectValue(*obj);

        Probes::enterScript(f.cx, f.script(), f.script()->function(), fp);

        if (script->debugMode) {
            JSTrapStatus status = js::ScriptDebugPrologue(f.cx, f.fp());
            switch (status) {
              case JSTRAP_CONTINUE:
                break;
              case JSTRAP_RETURN:
                *f.returnAddressLocation() = f.cx->jaegerRuntime().forceReturnFromExternC();
                return NULL;
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
        if (!fp->prologue(cx, types::UseNewTypeAtEntry(cx, fp)))
            return js_InternalThrow(f);

        








        JS_ASSERT(!cx->compartment->debugMode());
        break;

      
      case REJOIN_FUNCTION_PROLOGUE:
        if (fp->isConstructing()) {
            RootedObject callee(cx, &fp->callee());
            JSObject *obj = js_CreateThisForFunction(cx, callee, types::UseNewTypeAtEntry(cx, fp));
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
          case JSOP_INSTANCEOF: {
            




            if (f.regs.sp[0].isPrimitive()) {
                RootedValue val(cx, f.regs.sp[-1]);
                js_ReportValueError(cx, JSMSG_BAD_PROTOTYPE, -1, val, NullPtr());
                return js_InternalThrow(f);
            }
            nextsp[-1].setBoolean(js_IsDelegate(cx, &f.regs.sp[0].toObject(), f.regs.sp[-2]));
            f.regs.pc = nextpc;
            break;
          }

          default:
            f.regs.pc = nextpc;
            break;
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
