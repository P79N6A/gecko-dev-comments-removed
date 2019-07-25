







































#define __STDC_LIMIT_MACROS

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
#include "jsiter.h"
#include "jstypes.h"
#include "methodjit/StubCalls.h"
#include "jstracer.h"
#include "jspropertycache.h"
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
        f.setReturnAddress(ReturnAddressPtr(FunctionPtr(ptr))); \
        return;  \
    } while (0)

#define THROWV(v)       \
    do {                \
        void *ptr = JS_FUNC_TO_DATA_PTR(void *, JaegerThrowpoline); \
        f.setReturnAddress(ReturnAddressPtr(FunctionPtr(ptr))); \
        return v;       \
    } while (0)

JSObject * JS_FASTCALL
mjit::stubs::BindName(VMFrame &f)
{
    PropertyCacheEntry *entry;

    
    JS_ASSERT(f.fp->scopeChain->getParent());

    JSAtom *atom;
    JSObject *obj2;
    JSContext *cx = f.cx;
    JSObject *obj = f.fp->scopeChain->getParent();
    JS_PROPERTY_CACHE(cx).test(cx, f.regs.pc, obj, obj2, entry, atom);
    if (!atom)
        return obj;

    jsid id = ATOM_TO_JSID(atom);
    obj = js_FindIdentifierBase(cx, f.fp->scopeChain, id);
    if (!obj)
        THROWV(NULL);
    return obj;
}

static bool
InlineReturn(JSContext *cx)
{
    bool ok = true;

    JSStackFrame *fp = cx->fp;

    JS_ASSERT(!fp->blockChain);
    JS_ASSERT(!js_IsActiveWithOrBlock(cx, fp->scopeChain, 0));

    if (fp->script->staticLevel < JS_DISPLAY_SIZE)
        cx->display[fp->script->staticLevel] = fp->displaySave;

    
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

    cx->stack().popInlineFrame(cx, fp, fp->down);
    cx->regs->sp[-1] = fp->rval;

    return ok;
}

void * JS_FASTCALL
mjit::stubs::Return(VMFrame &f)
{
    if (!f.inlineCallCount)
        return f.fp->ncode;

    JSContext *cx = f.cx;
    JS_ASSERT(f.fp == cx->fp);

#ifdef DEBUG
    bool wasInterp = f.fp->script->ncode == JS_UNJITTABLE_METHOD;
#endif

    bool ok = InlineReturn(cx);

    f.inlineCallCount--;
    JS_ASSERT(f.regs.sp == cx->regs->sp);
    f.fp = cx->fp;

    JS_ASSERT_IF(f.inlineCallCount > 1 && !wasInterp,
                 f.fp->down->script->isValidJitCode(f.fp->ncode));

    if (!ok)
        THROWV(NULL);

    return f.fp->ncode;
}

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
                  ok = !!js_CloseIterator(cx, cx->regs->sp[-1]);
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
        f.scriptedReturn = stubs::Return(f);
    }

    JS_ASSERT(f.regs.sp == cx->regs->sp);

    if (!pc) {
        *f.oldRegs = f.regs;
        f.cx->setCurrentRegs(f.oldRegs);
        return NULL;
    }

    return cx->fp->script->pcToNative(pc);
}

#define NATIVE_SET(cx,obj,sprop,entry,vp)                                     \
    JS_BEGIN_MACRO                                                            \
        if (sprop->hasDefaultSetter() &&                                      \
            (sprop)->slot != SPROP_INVALID_SLOT &&                            \
            !obj->scope()->brandedOrHasMethodBarrier()) {                     \
            /* Fast path for, e.g., plain Object instance properties. */      \
            obj->setSlot(sprop->slot, *vp);                                   \
        } else {                                                              \
            if (!js_NativeSet(cx, obj, sprop, false, vp))                     \
                THROW();                                                      \
        }                                                                     \
    JS_END_MACRO

static inline JSObject *
ValueToObject(JSContext *cx, Value *vp)
{
    if (vp->isObject())
        return &vp->asObject();
    if (!js_ValueToNonNullObject(cx, *vp, vp))
        return NULL;
    return &vp->asObject();
}

#define NATIVE_GET(cx,obj,pobj,sprop,getHow,vp)                               \
    JS_BEGIN_MACRO                                                            \
        if (sprop->hasDefaultGetter()) {                                      \
            /* Fast path for Object instance properties. */                   \
            JS_ASSERT((sprop)->slot != SPROP_INVALID_SLOT ||                  \
                      !sprop->hasDefaultSetter());                            \
            if (((sprop)->slot != SPROP_INVALID_SLOT))                        \
                *(vp) = (pobj)->lockedGetSlot((sprop)->slot);                 \
            else                                                              \
                (vp)->setUndefined();                                         \
        } else {                                                              \
            if (!js_NativeGet(cx, obj, pobj, sprop, getHow, vp))              \
                return NULL;                                                  \
        }                                                                     \
    JS_END_MACRO

void JS_FASTCALL
mjit::stubs::SetName(VMFrame &f, uint32 index)
{
    JSContext *cx = f.cx;

    Value &rref = f.regs.sp[-1];
    Value &lref = f.regs.sp[-2];
    JSObject *obj = ValueToObject(cx, &lref);
    if (!obj)
        THROW();

    do {
        PropertyCache *cache = &JS_PROPERTY_CACHE(cx);

        


















        PropertyCacheEntry *entry;
        JSObject *obj2;
        JSAtom *atom;
        if (cache->testForSet(cx, f.regs.pc, obj, &entry, &obj2, &atom)) {
            








            JS_ASSERT(entry->vword.isSprop());
            JSScopeProperty *sprop = entry->vword.toSprop();
            JS_ASSERT_IF(sprop->isDataDescriptor(), sprop->writable());
            JS_ASSERT_IF(sprop->hasSlot(), entry->vcapTag() == 0);

            JSScope *scope = obj->scope();
            JS_ASSERT(!scope->sealed());

            





            bool checkForAdd;
            if (!sprop->hasSlot()) {
                if (entry->vcapTag() == 0 ||
                    ((obj2 = obj->getProto()) &&
                     obj2->isNative() &&
                     obj2->shape() == entry->vshape())) {
                    goto fast_set_propcache_hit;
                }

                
                checkForAdd = false;
            } else if (!scope->isSharedEmpty()) {
                if (sprop == scope->lastProperty() || scope->hasProperty(sprop)) {
                  fast_set_propcache_hit:
                    PCMETER(cache->pchits++);
                    PCMETER(cache->setpchits++);
                    NATIVE_SET(cx, obj, sprop, entry, &rref);
                    break;
                }
                checkForAdd = sprop->hasSlot() && sprop->parent == scope->lastProperty();
            } else {
                




                JS_ASSERT(CX_OWNS_OBJECT_TITLE(cx, obj));
                scope = js_GetMutableScope(cx, obj);
                JS_ASSERT(CX_OWNS_OBJECT_TITLE(cx, obj));
                if (!scope)
                    THROW();
                checkForAdd = !sprop->parent;
            }

            uint32 slot;
            if (checkForAdd &&
                entry->vshape() == cx->runtime->protoHazardShape &&
                sprop->hasDefaultSetter() &&
                (slot = sprop->slot) == scope->freeslot) {
                









                PCMETER(cache->pchits++);
                PCMETER(cache->addpchits++);

                




                if (slot < obj->numSlots() &&
                    !obj->getClass()->reserveSlots) {
                    ++scope->freeslot;
                } else {
                    if (!js_AllocSlot(cx, obj, &slot))
                        THROW();
                }

                









                if (slot != sprop->slot || scope->table) {
                    JSScopeProperty *sprop2 =
                        scope->putProperty(cx, sprop->id,
                                           sprop->getter(), sprop->setter(),
                                           slot, sprop->attributes(),
                                           sprop->getFlags(), sprop->shortid);
                    if (!sprop2) {
                        js_FreeSlot(cx, obj, slot);
                        THROW();
                    }
                    sprop = sprop2;
                } else {
                    scope->extend(cx, sprop);
                }

                





                TRACE_2(SetPropHit, entry, sprop);
                obj->lockedSetSlot(slot, rref);

                




                js_PurgeScopeChain(cx, obj, sprop->id);
                break;
            }
            PCMETER(cache->setpcmisses++);
            atom = NULL;
        } else if (!atom) {
            



            JSScopeProperty *sprop = NULL;
            if (obj == obj2) {
                sprop = entry->vword.toSprop();
                JS_ASSERT(sprop->writable());
                JS_ASSERT(!obj2->scope()->sealed());
                NATIVE_SET(cx, obj, sprop, entry, &rref);
            }
            if (sprop)
                break;
        }

        if (!atom)
            atom = f.fp->script->getAtom(index);
        jsid id = ATOM_TO_JSID(atom);
        if (entry && JS_LIKELY(obj->map->ops->setProperty == js_SetProperty)) {
            uintN defineHow;
            JSOp op = JSOp(*f.regs.pc);
            if (op == JSOP_SETMETHOD)
                defineHow = JSDNP_CACHE_RESULT | JSDNP_SET_METHOD;
            else if (op == JSOP_SETNAME)
                defineHow = JSDNP_CACHE_RESULT | JSDNP_UNQUALIFIED;
            else
                defineHow = JSDNP_CACHE_RESULT;
            if (!js_SetPropertyHelper(cx, obj, id, defineHow, &rref))
                THROW();
        } else {
            if (!obj->setProperty(cx, id, &rref))
                THROW();
        }
    } while (0);

    f.regs.sp[-2] = f.regs.sp[-1];
}

static void
ReportAtomNotDefined(JSContext *cx, JSAtom *atom)
{
    const char *printable = js_AtomToPrintableString(cx, atom);
    if (printable)
        js_ReportIsNotDefined(cx, printable);
}

static JSObject *
NameOp(VMFrame &f, uint32 index)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = f.fp;
    JSObject *obj = fp->scopeChain;

    JSScopeProperty *sprop;
    Value rval;

    PropertyCacheEntry *entry;
    JSObject *obj2;
    JSAtom *atom;
    JS_PROPERTY_CACHE(cx).test(cx, f.regs.pc, obj, obj2, entry, atom);
    if (!atom) {
        if (entry->vword.isFunObj()) {
            f.regs.sp++;
            f.regs.sp[-1].setFunObj(entry->vword.toFunObj());
            return obj;
        }

        if (entry->vword.isSlot()) {
            uintN slot = entry->vword.toSlot();
            JS_ASSERT(slot < obj2->scope()->freeslot);
            f.regs.sp++;
            f.regs.sp[-1] = obj2->lockedGetSlot(slot);
            return obj;
        }

        JS_ASSERT(entry->vword.isSprop());
        sprop = entry->vword.toSprop();
        goto do_native_get;
    }

    jsid id;
    id = ATOM_TO_JSID(atom);
    JSProperty *prop;
    if (!js_FindPropertyHelper(cx, id, true, &obj, &obj2, &prop))
        return NULL;
    if (!prop) {
        
        JSOp op2 = js_GetOpcode(cx, f.fp->script, f.regs.pc + JSOP_NAME_LENGTH);
        if (op2 == JSOP_TYPEOF) {
            f.regs.sp++;
            f.regs.sp[-1].setUndefined();
            return obj;
        }
        ReportAtomNotDefined(cx, atom);
        return NULL;
    }

    
    if (!obj->isNative() || !obj2->isNative()) {
        obj2->dropProperty(cx, prop);
        if (!obj->getProperty(cx, id, &rval))
            return NULL;
    } else {
        sprop = (JSScopeProperty *)prop;
  do_native_get:
        NATIVE_GET(cx, obj, obj2, sprop, JSGET_METHOD_BARRIER, &rval);
        obj2->dropProperty(cx, (JSProperty *) sprop);
    }

    f.regs.sp++;
    f.regs.sp[-1] = rval;
    return obj;
}

void JS_FASTCALL
stubs::Name(VMFrame &f, uint32 index)
{
    if (!NameOp(f, index))
        THROW();
}

void JS_FASTCALL
stubs::GetElem(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSFrameRegs &regs = f.regs;

    Value lval = regs.sp[-2];
    Value rval = regs.sp[-1];
    const Value *copyFrom;

    JSObject *obj;
    jsid id;
    int i;

    if (lval.isString() && rval.isInt32()) {
        Value retval;
        JSString *str = lval.asString();
        i = rval.asInt32();

        if ((size_t)i >= str->length())
            THROW();

        str = JSString::getUnitString(cx, str, (size_t)i);
        if (!str)
            THROW();
        f.regs.sp[-2].setString(str);
        return;
    }

    obj = ValueToObject(cx, &lval);
    if (!obj)
        THROW();

    if (rval.isInt32()) {
        if (obj->isDenseArray()) {
            jsuint idx = jsuint(rval.asInt32());
            
            if (idx < obj->getArrayLength() &&
                idx < obj->getDenseArrayCapacity()) {
                copyFrom = obj->addressOfDenseArrayElement(idx);
                if (!copyFrom->isMagic())
                    goto end_getelem;
                
            }
        } else if (obj->isArguments()
#ifdef JS_TRACER
                   && !GetArgsPrivateNative(obj)
#endif
                  ) {
            uint32 arg = uint32(rval.asInt32());

            if (arg < obj->getArgsLength()) {
                JSStackFrame *afp = (JSStackFrame *) obj->getPrivate();
                if (afp) {
                    copyFrom = &afp->argv[arg];
                    goto end_getelem;
                }

                copyFrom = obj->addressOfArgsElement(arg);
                if (!copyFrom->isMagic())
                    goto end_getelem;
                
            }
        }
        id = INT_TO_JSID(rval.asInt32());

    } else {
        if (!js_InternNonIntElementId(cx, obj, rval, &id))
            THROW();
    }

    if (!obj->getProperty(cx, id, &rval))
        THROW();
    copyFrom = &rval;

  end_getelem:
    f.regs.sp[-2] = *copyFrom;
}

void JS_FASTCALL
stubs::SetElem(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSFrameRegs &regs = f.regs;

    Value &objval = regs.sp[-3];
    Value &idval  = regs.sp[-2];
    Value retval  = regs.sp[-1];
    
    JSObject *obj;
    jsid id;

    obj = ValueToObject(cx, &objval);
    if (!obj)
        THROW();

    
    int32_t i_;
    if (ValueFitsInInt32(idval, &i_)) {
        id = INT_TO_JSID(i_);
    } else if (!js_InternNonIntElementId(cx, obj, idval, &id, &regs.sp[-2])) {
        THROW();
    }

    if (obj->isDenseArray() && JSID_IS_INT(id)) {
        jsuint length = obj->getDenseArrayCapacity();
        jsint i = JSID_TO_INT(id);
        
        if ((jsuint)i < length) {
            if (obj->getDenseArrayElement(i).isMagic(JS_ARRAY_HOLE)) {
                if (js_PrototypeHasIndexedProperties(cx, obj))
                    goto mid_setelem;
                if ((jsuint)i >= obj->getArrayLength())
                    obj->setDenseArrayLength(i + 1);
                obj->incDenseArrayCountBy(1);
            }
            obj->setDenseArrayElement(i, regs.sp[-1]);
            goto end_setelem;
        }
    }

  mid_setelem:
    if (!obj->setProperty(cx, id, &regs.sp[-1]))
        THROW();

  end_setelem:
    



    regs.sp[-3] = retval;
}

void JS_FASTCALL
stubs::CallName(VMFrame &f, uint32 index)
{
    JSObject *obj = NameOp(f, index);
    if (!obj)
        THROW();
    f.regs.sp++;
    f.regs.sp[-1].setNonFunObj(*obj);
}

void JS_FASTCALL
stubs::BitAnd(VMFrame &f)
{
    int32_t i, j;

    if (!ValueToECMAInt32(f.cx, f.regs.sp[-2], &i) ||
        !ValueToECMAInt32(f.cx, f.regs.sp[-1], &j)) {
        THROW();
    }
    i = i & j;
    f.regs.sp[-2].setInt32(i);
}

void JS_FASTCALL
stubs::Lsh(VMFrame &f)
{
    int32_t i, j;
    if (!ValueToECMAInt32(f.cx, f.regs.sp[-2], &i))
        THROW();
    if (!ValueToECMAInt32(f.cx, f.regs.sp[-1], &j))
        THROW();
    i = i << (j & 31);
    f.regs.sp[-2].setInt32(i);
}

void JS_FASTCALL
stubs::Rsh(VMFrame &f)
{
    int32_t i, j;
    if (!ValueToECMAInt32(f.cx, f.regs.sp[-2], &i))
        THROW();
    if (!ValueToECMAInt32(f.cx, f.regs.sp[-1], &j))
        THROW();
    i = i >> (j & 31);
    f.regs.sp[-2].setInt32(i);
}

template <int32 N>
static inline bool
PostInc(VMFrame &f, Value *vp)
{
    double d;
    if (!ValueToNumber(f.cx, *vp, &d))
        return false;
    f.regs.sp++;
    f.regs.sp[-1].setDouble(d);
    d += N;
    vp->setDouble(d);
    return true;
}

template <int32 N>
static inline bool
PreInc(VMFrame &f, Value *vp)
{
    double d;
    if (!ValueToNumber(f.cx, *vp, &d))
        return false;
    d += N;
    vp->setDouble(d);
    f.regs.sp++;
    f.regs.sp[-1].setDouble(d);
    return true;
}

void JS_FASTCALL
stubs::VpInc(VMFrame &f, Value *vp)
{
    if (!PostInc<1>(f, vp))
        THROW();
}

void JS_FASTCALL
stubs::VpDec(VMFrame &f, Value *vp)
{
    if (!PostInc<-1>(f, vp))
        THROW();
}

void JS_FASTCALL
stubs::DecVp(VMFrame &f, Value *vp)
{
    if (!PreInc<-1>(f, vp))
        THROW();
}

void JS_FASTCALL
stubs::IncVp(VMFrame &f, Value *vp)
{
    if (!PreInc<1>(f, vp))
        THROW();
}

static inline bool
DoInvoke(VMFrame &f, uint32 argc, Value *vp)
{
    JSContext *cx = f.cx;

    if (vp->isFunObj()) {
        JSObject *obj = &vp->asFunObj();
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);

        JS_ASSERT(!FUN_INTERPRETED(fun));
        if (fun->flags & JSFUN_FAST_NATIVE) {
            JS_ASSERT(fun->u.n.extra == 0);
            JS_ASSERT(vp[1].isObjectOrNull() || PrimitiveValue::test(fun, vp[1]));
            JSBool ok = ((FastNative)fun->u.n.native)(cx, argc, vp);
            f.regs.sp = vp + 1;
            return ok ? true : false;
        }
    }

    JSBool ok = Invoke(cx, InvokeArgsGuard(vp, argc), 0);
    f.regs.sp = vp + 1;
    return ok ? true : false;
}

static inline bool
InlineCall(VMFrame &f, uint32 flags, void **pret, uint32 argc)
{
    JSContext *cx = f.cx;
    JSStackFrame *fp = f.fp;
    Value *vp = f.regs.sp - (argc + 2);
    JSObject *funobj = &vp->asFunObj();
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

    f.inlineCallCount++;
    f.fp = newfp;
    stack.pushInlineFrame(cx, fp, cx->regs->pc, newfp);

    if (newscript->staticLevel < JS_DISPLAY_SIZE) {
        JSStackFrame **disp = &cx->display[newscript->staticLevel];
        newfp->displaySave = *disp;
        *disp = newfp;
    }

    f.regs.pc = newscript->code;
    f.regs.sp = newsp;

    if (cx->options & JSOPTION_METHODJIT) {
        if (!newscript->ncode) {
            if (mjit::TryCompile(cx, newscript, fun, newfp->scopeChain) == Compile_Error)
                return false;
        }
        JS_ASSERT(newscript->ncode);
        if (newscript->ncode != JS_UNJITTABLE_METHOD) {
            fp->ncode = f.scriptedReturn;
            *pret = newscript->ncode;
            return true;
        }
    }

    bool ok = !!Interpret(cx); 
    stubs::Return(f);

    *pret = NULL;
    return ok;
}

void * JS_FASTCALL
stubs::Call(VMFrame &f, uint32 argc)
{
    Value *vp = f.regs.sp - (argc + 2);

    if (vp->isFunObj()) {
        JSObject *obj = &vp->asFunObj();
        JSFunction *fun = GET_FUNCTION_PRIVATE(cx, obj);
        if (FUN_INTERPRETED(fun)) {
            if (fun->u.i.script->isEmpty()) {
                vp->setUndefined();
                f.regs.sp = vp + 1;
                return NULL;
            }

            void *ret;
            if (!InlineCall(f, 0, &ret, argc))
                THROWV(NULL);

            f.cx->regs->pc = f.fp->script->code;

#ifdef JS_TRACER
            if (ret && f.cx->jitEnabled && IsTraceableRecursion(f.cx)) {
                
                f.u.tracer.traceId = 0;
                f.u.tracer.offs = 0;

                
                JS_ASSERT(f.cx->regs == &f.regs);

                











                f.scriptedReturn = GetReturnAddress(f, f.fp);

                void *newRet = InvokeTracer(f, Record_Recursion);

                




                if (newRet) {
                    void *ptr = JS_FUNC_TO_DATA_PTR(void *, JaegerFromTracer);
                    f.setReturnAddress(ReturnAddressPtr(FunctionPtr(ptr)));
                    return newRet;
                }
            }
#endif

            return ret;
        }
    }

    if (!DoInvoke(f, argc, vp))
        THROWV(NULL);

    return NULL;
}

void JS_FASTCALL
stubs::DefFun(VMFrame &f, uint32 index)
{
    bool doSet;
    JSObject *pobj, *obj2;
    JSProperty *prop;
    uint32 old;

    JSContext *cx = f.cx;
    JSStackFrame *fp = f.fp;
    JSScript *script = fp->script;

    





    JSFunction *fun = script->getFunction(index);
    JSObject *obj = FUN_OBJECT(fun);

    if (FUN_NULL_CLOSURE(fun)) {
        




        obj2 = fp->scopeChain;
    } else {
        JS_ASSERT(!FUN_FLAT_CLOSURE(fun));

        



        if (!fp->blockChain) {
            obj2 = fp->scopeChain;
        } else {
            obj2 = js_GetScopeChain(cx, fp);
            if (!obj2)
                THROW();
        }
    }

    








    if (obj->getParent() != obj2) {
        obj = CloneFunctionObject(cx, fun, obj2);
        if (!obj)
            THROW();
    }

    




    MUST_FLOW_THROUGH("restore_scope");
    fp->scopeChain = obj;

    Value rval;
    rval.setFunObj(*obj);

    



    uintN attrs;
    attrs = (fp->flags & JSFRAME_EVAL)
            ? JSPROP_ENUMERATE
            : JSPROP_ENUMERATE | JSPROP_PERMANENT;

    




    PropertyOp getter, setter;
    uintN flags;
    
    getter = setter = PropertyStub;
    flags = JSFUN_GSFLAG2ATTR(fun->flags);
    if (flags) {
        
        JS_ASSERT(flags == JSPROP_GETTER || flags == JSPROP_SETTER);
        attrs |= flags | JSPROP_SHARED;
        rval.setUndefined();
        if (flags == JSPROP_GETTER)
            getter = CastAsPropertyOp(obj);
        else
            setter = CastAsPropertyOp(obj);
    }

    jsid id;
    JSBool ok;
    JSObject *parent;

    




    parent = fp->varobj(cx);
    JS_ASSERT(parent);

    




    id = ATOM_TO_JSID(fun->atom);
    prop = NULL;
    ok = CheckRedeclaration(cx, parent, id, attrs, &pobj, &prop);
    if (!ok)
        goto restore_scope;

    










    doSet = (attrs == JSPROP_ENUMERATE);
    JS_ASSERT_IF(doSet, fp->flags & JSFRAME_EVAL);
    if (prop) {
        if (parent == pobj &&
            parent->getClass() == &js_CallClass &&
            (old = ((JSScopeProperty *) prop)->attributes(),
             !(old & (JSPROP_GETTER|JSPROP_SETTER)) &&
             (old & (JSPROP_ENUMERATE|JSPROP_PERMANENT)) == attrs)) {
            



            JS_ASSERT(!(attrs & ~(JSPROP_ENUMERATE|JSPROP_PERMANENT)));
            JS_ASSERT(!(old & JSPROP_READONLY));
            doSet = JS_TRUE;
        }
        pobj->dropProperty(cx, prop);
    }
    ok = doSet
         ? parent->setProperty(cx, id, &rval)
         : parent->defineProperty(cx, id, rval, getter, setter, attrs);

  restore_scope:
    
    fp->scopeChain = obj2;
    if (!ok)
        THROW();
}

#define DEFAULT_VALUE(cx, n, hint, v)                                         \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(v.isObject());                                              \
        JS_ASSERT(v == regs.sp[n]);                                           \
        if (!v.asObject().defaultValue(cx, hint, &regs.sp[n]))                \
            THROWV(JS_FALSE);                                                 \
        v = regs.sp[n];                                                       \
    JS_END_MACRO

#define RELATIONAL(OP)                                                        \
    JS_BEGIN_MACRO                                                            \
        JSContext *cx = f.cx;                                                 \
        JSFrameRegs &regs = f.regs;                                           \
        Value rval = regs.sp[-1];                                             \
        Value lval = regs.sp[-2];                                             \
        bool cond;                                                            \
        if (lval.isObject())                                                  \
            DEFAULT_VALUE(cx, -2, JSTYPE_NUMBER, lval);                       \
        if (rval.isObject())                                                  \
            DEFAULT_VALUE(cx, -1, JSTYPE_NUMBER, rval);                       \
        if (BothString(lval, rval)) {                                         \
            JSString *l = lval.asString(), *r = rval.asString();              \
            cond = js_CompareStrings(l, r) OP 0;                              \
        } else {                                                              \
            double l, r;                                                      \
            if (!ValueToNumber(cx, lval, &l) ||                               \
                !ValueToNumber(cx, rval, &r)) {                               \
                THROWV(JS_FALSE);                                             \
            }                                                                 \
            cond = JSDOUBLE_COMPARE(l, OP, r, false);                         \
        }                                                                     \
        regs.sp[-2].setBoolean(cond);                                         \
        return cond;                                                          \
    JS_END_MACRO

JSBool JS_FASTCALL
stubs::LessThan(VMFrame &f)
{
    RELATIONAL(<);
}

JSBool JS_FASTCALL
stubs::LessEqual(VMFrame &f)
{
    RELATIONAL(<=);
}

JSBool JS_FASTCALL
stubs::GreaterThan(VMFrame &f)
{
    RELATIONAL(>);
}

JSBool JS_FASTCALL
stubs::GreaterEqual(VMFrame &f)
{
    RELATIONAL(>=);
}

JSBool JS_FASTCALL
stubs::ValueToBoolean(VMFrame &f)
{
    return js_ValueToBoolean(f.regs.sp[-1]);
}






static inline bool
InlineEqualityOp(VMFrame &f, bool op, bool ifnan)
{
    Class *clasp;
    JSContext *cx = f.cx;
    JSFrameRegs &regs = f.regs;

    Value rval = regs.sp[-1];
    Value lval = regs.sp[-2];

    JSBool jscond;
    bool cond;

    #if JS_HAS_XML_SUPPORT
    
    if ((lval.isNonFunObj() && lval.asObject().isXML()) ||
        (rval.isNonFunObj() && rval.asObject().isXML())) {
        if (!js_TestXMLEquality(cx, lval, rval, &jscond))
            THROWV(false);
        cond = (jscond == JS_TRUE) == op;
    } else
    #endif 

    if (SamePrimitiveTypeOrBothObjects(lval, rval)) {
        if (lval.isString()) {
            JSString *l = lval.asString();
            JSString *r = rval.asString();
            cond = js_EqualStrings(l, r) == op;
        } else if (lval.isDouble()) {
            double l = lval.asDouble();
            double r = rval.asDouble();
            if (op) {
                cond = JSDOUBLE_COMPARE(l, ==, r, ifnan);
            } else {
                cond = JSDOUBLE_COMPARE(l, !=, r, ifnan);
            }
        } else {
            
            JSObject *lobj;
            if (lval.isObject() &&
                (lobj = &lval.asObject()) &&
                ((clasp = lobj->getClass())->flags & JSCLASS_IS_EXTENDED) &&
                ((ExtendedClass *)clasp)->equality) {
                if (!((ExtendedClass *)clasp)->equality(cx, lobj, &lval, &jscond))
                    THROWV(false);
                cond = (jscond == JS_TRUE) == op;
            } else {
                cond = (lval.asRawUint32() == rval.asRawUint32()) == op;
            }
        }
    } else {
        if (lval.isNullOrUndefined()) {
            cond = rval.isNullOrUndefined() == op;
        } else if (rval.isNullOrUndefined()) {
            cond = !op;
        } else {
            if (lval.isObject()) {
                if (!lval.asObject().defaultValue(cx, JSTYPE_VOID, &regs.sp[-2]))
                    THROWV(false);
                lval = regs.sp[-2];
            }

            if (rval.isObject()) {
                if (!rval.asObject().defaultValue(cx, JSTYPE_VOID, &regs.sp[-1]))
                    THROWV(false);
                rval = regs.sp[-1];
            }

            if (BothString(lval, rval)) {
                JSString *l = lval.asString();
                JSString *r = rval.asString();
                cond = js_EqualStrings(l, r) == op;
            } else {
                double l, r;
                if (!ValueToNumber(cx, lval, &l) ||
                    !ValueToNumber(cx, rval, &r)) {
                    THROWV(false);
                }
                
                if (op)
                    cond = JSDOUBLE_COMPARE(l, ==, r, ifnan);
                else
                    cond = JSDOUBLE_COMPARE(l, !=, r, ifnan);
            }
        }
    }

    regs.sp[-2].setBoolean(cond);
    return cond;
}

JSBool JS_FASTCALL
stubs::Equal(VMFrame &f)
{
    return InlineEqualityOp(f, true, false);
}

JSBool JS_FASTCALL
stubs::NotEqual(VMFrame &f)
{
    return InlineEqualityOp(f, false, true);
}

static inline bool
DefaultValue(VMFrame &f, JSType hint, Value &v, int n)
{
    JS_ASSERT(v.isObject());
    if (!v.asObject().defaultValue(f.cx, hint, &f.regs.sp[n]))
        return false;
    v = f.regs.sp[n];
    return true;
}

void JS_FASTCALL
stubs::Add(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSFrameRegs &regs = f.regs;
    Value rval = regs.sp[-1];
    Value lval = regs.sp[-2];

    if (BothInt32(lval, rval)) {
        int32_t l = lval.asInt32(), r = rval.asInt32();
        int32_t sum = l + r;
        regs.sp--;
        if (JS_UNLIKELY(bool((l ^ sum) & (r ^ sum) & 0x80000000)))
            regs.sp[-1].setDouble(double(l) + double(r));
        else
            regs.sp[-1].setInt32(sum);
    } else
#if JS_HAS_XML_SUPPORT
    if (lval.isNonFunObj() && lval.asObject().isXML() &&
        rval.isNonFunObj() && rval.asObject().isXML()) {
        if (!js_ConcatenateXML(cx, &lval.asObject(), &rval.asObject(), &rval))
            THROW();
        regs.sp--;
        regs.sp[-1] = rval;
    } else
#endif
    {
        if (lval.isObject() && !DefaultValue(f, JSTYPE_VOID, lval, -2))
            THROW();
        if (rval.isObject() && !DefaultValue(f, JSTYPE_VOID, rval, -1))
            THROW();
        bool lIsString, rIsString;
        if ((lIsString = lval.isString()) | (rIsString = rval.isString())) {
            JSString *lstr, *rstr;
            if (lIsString) {
                lstr = lval.asString();
            } else {
                lstr = js_ValueToString(cx, lval);
                if (!lstr)
                    THROW();
                regs.sp[-2].setString(lstr);
            }
            if (rIsString) {
                rstr = rval.asString();
            } else {
                rstr = js_ValueToString(cx, rval);
                if (!rstr)
                    THROW();
                regs.sp[-1].setString(rstr);
            }
            JSString *str = js_ConcatStrings(cx, lstr, rstr);
            if (!str)
                THROW();
            regs.sp--;
            regs.sp[-1].setString(str);
        } else {
            double l, r;
            if (!ValueToNumber(cx, lval, &l) || !ValueToNumber(cx, rval, &r))
                THROW();
            l += r;
            regs.sp--;
            regs.sp[-1].setNumber(l);
        }
    }
}


void JS_FASTCALL
stubs::Sub(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSFrameRegs &regs = f.regs;
    double d1, d2;
    if (!ValueToNumber(cx, regs.sp[-2], &d1) ||
        !ValueToNumber(cx, regs.sp[-1], &d2)) {
        THROW();
    }
    double d = d1 - d2;
    regs.sp[-2].setNumber(d);
}

void JS_FASTCALL
stubs::Mul(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSFrameRegs &regs = f.regs;
    double d1, d2;
    if (!ValueToNumber(cx, regs.sp[-2], &d1) ||
        !ValueToNumber(cx, regs.sp[-1], &d2)) {
        THROW();
    }
    double d = d1 * d2;
    regs.sp[-2].setNumber(d);
}

void JS_FASTCALL
stubs::Div(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSRuntime *rt = cx->runtime;
    JSFrameRegs &regs = f.regs;

    double d1, d2;
    if (!ValueToNumber(cx, regs.sp[-2], &d1) ||
        !ValueToNumber(cx, regs.sp[-1], &d2)) {
        THROW();
    }
    if (d2 == 0) {
        const Value *vp;
#ifdef XP_WIN
        
        if (JSDOUBLE_IS_NaN(d2))
            vp = &rt->NaNValue;
        else
#endif
        if (d1 == 0 || JSDOUBLE_IS_NaN(d1))
            vp = &rt->NaNValue;
        else if (JSDOUBLE_IS_NEG(d1) != JSDOUBLE_IS_NEG(d2))
            vp = &rt->negativeInfinityValue;
        else
            vp = &rt->positiveInfinityValue;
        regs.sp[-2] = *vp;
    } else {
        d1 /= d2;
        regs.sp[-2].setNumber(d1);
    }
}

void JS_FASTCALL
stubs::Mod(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSFrameRegs &regs = f.regs;

    Value &lref = regs.sp[-2];
    Value &rref = regs.sp[-1];
    int32_t l, r;
    if (lref.isInt32() && rref.isInt32() &&
        (l = lref.asInt32()) >= 0 && (r = rref.asInt32()) > 0) {
        int32_t mod = l % r;
        regs.sp[-2].setInt32(mod);
    } else {
        double d1, d2;
        if (!ValueToNumber(cx, regs.sp[-2], &d1) ||
            !ValueToNumber(cx, regs.sp[-1], &d2)) {
            THROW();
        }
        if (d2 == 0) {
            regs.sp[-2].setDouble(js_NaN);
        } else {
            d1 = js_fmod(d1, d2);
            regs.sp[-2].setDouble(d1);
        }
    }
}

