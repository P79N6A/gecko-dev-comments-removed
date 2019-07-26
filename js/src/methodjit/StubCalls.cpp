






#include "mozilla/FloatingPoint.h"

#include "jscntxt.h"
#include "jsscope.h"
#include "jsobj.h"
#include "jslibmath.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsxml.h"
#include "jsbool.h"
#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "jstypes.h"

#include "gc/Marking.h"
#include "vm/Debugger.h"
#include "vm/NumericConversions.h"
#include "vm/String.h"
#include "methodjit/Compiler.h"
#include "methodjit/StubCalls.h"
#include "methodjit/Retcon.h"

#include "jsinterpinlines.h"
#include "jsscopeinlines.h"
#include "jsscriptinlines.h"
#include "jsnuminlines.h"
#include "jsobjinlines.h"
#include "jscntxtinlines.h"
#include "jsatominlines.h"
#include "StubCalls-inl.h"
#include "jsfuninlines.h"
#include "jstypedarray.h"

#include "vm/RegExpObject-inl.h"
#include "vm/String-inl.h"

#ifdef JS_ION
#include "ion/Ion.h"
#endif

#ifdef XP_WIN
# include "jswin.h"
#endif

#include "jsautooplen.h"

using namespace js;
using namespace js::mjit;
using namespace js::types;
using namespace JSC;

void JS_FASTCALL
stubs::BindName(VMFrame &f, PropertyName *name_)
{
    Rooted<PropertyName*> name(f.cx, name_);
    JSObject *obj = FindIdentifierBase(f.cx, f.fp()->scopeChain(), name);
    if (!obj)
        THROW();
    f.regs.sp[0].setObject(*obj);
}

JSObject * JS_FASTCALL
stubs::BindGlobalName(VMFrame &f)
{
    return &f.fp()->global();
}

template<JSBool strict>
void JS_FASTCALL
stubs::SetName(VMFrame &f, PropertyName *name)
{
    JSContext *cx = f.cx;
    const Value &rval = f.regs.sp[-1];
    const Value &lval = f.regs.sp[-2];

    if (!SetPropertyOperation(cx, f.pc(), lval, rval))
        THROW();

    f.regs.sp[-2] = f.regs.sp[-1];
}

template void JS_FASTCALL stubs::SetName<true>(VMFrame &f, PropertyName *origName);
template void JS_FASTCALL stubs::SetName<false>(VMFrame &f, PropertyName *origName);

template<JSBool strict>
void JS_FASTCALL
stubs::SetGlobalName(VMFrame &f, PropertyName *name)
{
    SetName<strict>(f, name);
}

template void JS_FASTCALL stubs::SetGlobalName<true>(VMFrame &f, PropertyName *name);
template void JS_FASTCALL stubs::SetGlobalName<false>(VMFrame &f, PropertyName *name);

void JS_FASTCALL
stubs::Name(VMFrame &f)
{
    Value rval;
    if (!NameOperation(f.cx, f.pc(), &rval))
        THROW();
    f.regs.sp[0] = rval;
}

void JS_FASTCALL
stubs::GetElem(VMFrame &f)
{
    Value &lref = f.regs.sp[-2];
    Value &rref = f.regs.sp[-1];
    Value &rval = f.regs.sp[-2];

    if (!GetElementOperation(f.cx, JSOp(*f.pc()), lref, rref, &rval))
        THROW();
}

template<JSBool strict>
void JS_FASTCALL
stubs::SetElem(VMFrame &f)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;

    Value &objval = regs.sp[-3];
    Value &idval  = regs.sp[-2];
    RootedValue rval(cx, regs.sp[-1]);

    RootedId id(cx);

    Rooted<JSObject*> obj(cx, ValueToObject(cx, objval));
    if (!obj)
        THROW();

    if (!FetchElementId(f.cx, obj, idval, id.address(), &regs.sp[-2]))
        THROW();

    TypeScript::MonitorAssign(cx, obj, id);

    do {
        if (obj->isDenseArray() && JSID_IS_INT(id)) {
            uint32_t length = obj->getDenseArrayInitializedLength();
            int32_t i = JSID_TO_INT(id);
            if ((uint32_t)i < length) {
                if (obj->getDenseArrayElement(i).isMagic(JS_ARRAY_HOLE)) {
                    if (js_PrototypeHasIndexedProperties(cx, obj))
                        break;
                    if ((uint32_t)i >= obj->getArrayLength())
                        obj->setArrayLength(cx, i + 1);
                }
                obj->setDenseArrayElementWithType(cx, i, rval);
                goto end_setelem;
            } else {
                if (f.script()->hasAnalysis())
                    f.script()->analysis()->getCode(f.pc()).arrayWriteHole = true;
            }
        }
    } while (0);
    if (!obj->setGeneric(cx, obj, id, rval.address(), strict))
        THROW();
  end_setelem:
    



    regs.sp[-3] = regs.sp[-1];
}

template void JS_FASTCALL stubs::SetElem<true>(VMFrame &f);
template void JS_FASTCALL stubs::SetElem<false>(VMFrame &f);

void JS_FASTCALL
stubs::ToId(VMFrame &f)
{
    Value &objval = f.regs.sp[-2];
    Value &idval  = f.regs.sp[-1];

    JSObject *obj = ValueToObject(f.cx, objval);
    if (!obj)
        THROW();

    RootedId id(f.cx);
    if (!FetchElementId(f.cx, obj, idval, id.address(), &idval))
        THROW();

    if (!idval.isInt32())
        TypeScript::MonitorUnknown(f.cx, f.script(), f.pc());
}

void JS_FASTCALL
stubs::ImplicitThis(VMFrame &f, PropertyName *name_)
{
    RootedObject scopeObj(f.cx, f.cx->stack.currentScriptedScopeChain());
    RootedPropertyName name(f.cx, name_);

    RootedObject obj(f.cx), obj2(f.cx);
    RootedShape prop(f.cx);
    if (!FindPropertyHelper(f.cx, name, false, scopeObj, &obj, &obj2, &prop))
        THROW();

    if (!ComputeImplicitThis(f.cx, obj, &f.regs.sp[0]))
        THROW();
}

void JS_FASTCALL
stubs::BitOr(VMFrame &f)
{
    int32_t i, j;

    if (!ToInt32(f.cx, f.regs.sp[-2], &i) || !ToInt32(f.cx, f.regs.sp[-1], &j))
        THROW();

    i = i | j;
    f.regs.sp[-2].setInt32(i);
}

void JS_FASTCALL
stubs::BitXor(VMFrame &f)
{
    int32_t i, j;

    if (!ToInt32(f.cx, f.regs.sp[-2], &i) || !ToInt32(f.cx, f.regs.sp[-1], &j))
        THROW();

    i = i ^ j;
    f.regs.sp[-2].setInt32(i);
}

void JS_FASTCALL
stubs::BitAnd(VMFrame &f)
{
    int32_t i, j;

    if (!ToInt32(f.cx, f.regs.sp[-2], &i) || !ToInt32(f.cx, f.regs.sp[-1], &j))
        THROW();

    i = i & j;
    f.regs.sp[-2].setInt32(i);
}

void JS_FASTCALL
stubs::BitNot(VMFrame &f)
{
    int32_t i;

    if (!ToInt32(f.cx, f.regs.sp[-1], &i))
        THROW();
    i = ~i;
    f.regs.sp[-1].setInt32(i);
}

void JS_FASTCALL
stubs::Lsh(VMFrame &f)
{
    int32_t i, j;
    if (!ToInt32(f.cx, f.regs.sp[-2], &i))
        THROW();
    if (!ToInt32(f.cx, f.regs.sp[-1], &j))
        THROW();
    i = i << (j & 31);
    f.regs.sp[-2].setInt32(i);
}

void JS_FASTCALL
stubs::Rsh(VMFrame &f)
{
    int32_t i, j;
    if (!ToInt32(f.cx, f.regs.sp[-2], &i))
        THROW();
    if (!ToInt32(f.cx, f.regs.sp[-1], &j))
        THROW();
    i = i >> (j & 31);
    f.regs.sp[-2].setInt32(i);
}

void JS_FASTCALL
stubs::Ursh(VMFrame &f)
{
    uint32_t u;
    if (!ToUint32(f.cx, f.regs.sp[-2], &u))
        THROW();
    int32_t j;
    if (!ToInt32(f.cx, f.regs.sp[-1], &j))
        THROW();

    u >>= (j & 31);

	if (!f.regs.sp[-2].setNumber(uint32_t(u)))
        TypeScript::MonitorOverflow(f.cx, f.script(), f.pc());
}

template<JSBool strict>
void JS_FASTCALL
stubs::DefFun(VMFrame &f, JSFunction *fun_)
{
    





    JSContext *cx = f.cx;
    StackFrame *fp = f.fp();
    RootedFunction fun(f.cx, fun_);

    








    HandleObject scopeChain = f.fp()->scopeChain();
    if (fun->environment() != scopeChain) {
        fun = CloneFunctionObjectIfNotSingleton(cx, fun, scopeChain);
        if (!fun)
            THROW();
    } else {
        JS_ASSERT(f.script()->compileAndGo);
        JS_ASSERT(f.fp()->isGlobalFrame() || f.fp()->isEvalInFunction());
    }

    



    unsigned attrs = fp->isEvalFrame()
                  ? JSPROP_ENUMERATE
                  : JSPROP_ENUMERATE | JSPROP_PERMANENT;

    




    Rooted<JSObject*> parent(cx, &fp->varObj());

    
    RootedPropertyName name(cx, fun->atom->asPropertyName());
    RootedShape shape(cx);
    RootedObject pobj(cx);
    if (!parent->lookupProperty(cx, name, &pobj, &shape))
        THROW();

    RootedValue rval(cx, ObjectValue(*fun));

    do {
        
        if (!shape || pobj != parent) {
            if (!parent->defineProperty(cx, name, rval,
                                        JS_PropertyStub, JS_StrictPropertyStub, attrs))
            {
                THROW();
            }
            break;
        }

        
        JS_ASSERT(parent->isNative());
        if (parent->isGlobal()) {
            if (shape->configurable()) {
                if (!parent->defineProperty(cx, name, rval,
                                            JS_PropertyStub, JS_StrictPropertyStub, attrs))
                {
                    THROW();
                }
                break;
            }

            if (shape->isAccessorDescriptor() || !shape->writable() || !shape->enumerable()) {
                JSAutoByteString bytes;
                if (js_AtomToPrintableString(cx, name, &bytes)) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_CANT_REDEFINE_PROP, bytes.ptr());
                }
                THROW();
            }
        }

        






        
        if (!parent->setProperty(cx, parent, name, rval.address(), strict))
            THROW();
    } while (false);
}

template void JS_FASTCALL stubs::DefFun<true>(VMFrame &f, JSFunction *fun);
template void JS_FASTCALL stubs::DefFun<false>(VMFrame &f, JSFunction *fun);

#define RELATIONAL(OP)                                                        \
    JS_BEGIN_MACRO                                                            \
        JSContext *cx = f.cx;                                                 \
        FrameRegs &regs = f.regs;                                             \
        Value &rval = regs.sp[-1];                                            \
        Value &lval = regs.sp[-2];                                            \
        bool cond;                                                            \
        if (!ToPrimitive(cx, JSTYPE_NUMBER, &lval))                           \
            THROWV(JS_FALSE);                                                 \
        if (!ToPrimitive(cx, JSTYPE_NUMBER, &rval))                           \
            THROWV(JS_FALSE);                                                 \
        if (lval.isString() && rval.isString()) {                             \
            JSString *l = lval.toString(), *r = rval.toString();              \
            int32_t cmp;                                                      \
            if (!CompareStrings(cx, l, r, &cmp))                              \
                THROWV(JS_FALSE);                                             \
            cond = cmp OP 0;                                                  \
        } else {                                                              \
            double l, r;                                                      \
            if (!ToNumber(cx, lval, &l) || !ToNumber(cx, rval, &r))           \
                THROWV(JS_FALSE);                                             \
            cond = (l OP r);                                                  \
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

void JS_FASTCALL
stubs::Not(VMFrame &f)
{
    JSBool b = !js_ValueToBoolean(f.regs.sp[-1]);
    f.regs.sp[-1].setBoolean(b);
}

template <bool EQ>
static inline bool
StubEqualityOp(VMFrame &f)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;

    RootedValue rval_(cx, regs.sp[-1]);
    RootedValue lval_(cx, regs.sp[-2]);
    Value &rval = rval_.get(), &lval = lval_.get();

    bool cond;

    
    if (lval.isString() && rval.isString()) {
        JSString *l = lval.toString();
        JSString *r = rval.toString();
        bool equal;
        if (!EqualStrings(cx, l, r, &equal))
            return false;
        cond = equal == EQ;
    } else
#if JS_HAS_XML_SUPPORT
    if ((lval.isObject() && lval.toObject().isXML()) ||
        (rval.isObject() && rval.toObject().isXML()))
    {
        JSBool equal;
        if (!js_TestXMLEquality(cx, lval, rval, &equal))
            return false;
        cond = !!equal == EQ;
    } else
#endif

    if (SameType(lval, rval)) {
        JS_ASSERT(!lval.isString());    
        if (lval.isDouble()) {
            double l = lval.toDouble();
            double r = rval.toDouble();
            if (EQ)
                cond = (l == r);
            else
                cond = (l != r);
        } else if (lval.isObject()) {
            JSObject *l = &lval.toObject(), *r = &rval.toObject();
            if (JSEqualityOp eq = l->getClass()->ext.equality) {
                JSBool equal;
                Rooted<JSObject*> lobj(cx, l);
                if (!eq(cx, lobj, &rval, &equal))
                    return false;
                cond = !!equal == EQ;
            } else {
                cond = (l == r) == EQ;
            }
        } else if (lval.isNullOrUndefined()) {
            cond = EQ;
        } else {
            cond = (lval.payloadAsRawUint32() == rval.payloadAsRawUint32()) == EQ;
        }
    } else {
        if (lval.isNullOrUndefined()) {
            cond = rval.isNullOrUndefined() == EQ;
        } else if (rval.isNullOrUndefined()) {
            cond = !EQ;
        } else {
            if (!ToPrimitive(cx, &lval))
                return false;
            if (!ToPrimitive(cx, &rval))
                return false;

            



            if (lval.isString() && rval.isString()) {
                JSString *l = lval.toString();
                JSString *r = rval.toString();
                bool equal;
                if (!EqualStrings(cx, l, r, &equal))
                    return false;
                cond = equal == EQ;
            } else {
                double l, r;
                if (!ToNumber(cx, lval, &l) || !ToNumber(cx, rval, &r))
                    return false;

                if (EQ)
                    cond = (l == r);
                else
                    cond = (l != r);
            }
        }
    }

    regs.sp[-2].setBoolean(cond);
    return true;
}

JSBool JS_FASTCALL
stubs::Equal(VMFrame &f)
{
    if (!StubEqualityOp<true>(f))
        THROWV(JS_FALSE);
    return f.regs.sp[-2].toBoolean();
}

JSBool JS_FASTCALL
stubs::NotEqual(VMFrame &f)
{
    if (!StubEqualityOp<false>(f))
        THROWV(JS_FALSE);
    return f.regs.sp[-2].toBoolean();
}

void JS_FASTCALL
stubs::Add(VMFrame &f)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;
    RootedValue rval_(cx, regs.sp[-1]);
    RootedValue lval_(cx, regs.sp[-2]);
    Value &rval = rval_.get(), &lval = lval_.get();

    
    bool lIsString = lval.isString();
    bool rIsString = rval.isString();
    RootedString lstr(cx), rstr(cx);
    if (lIsString && rIsString) {
        lstr = lval.toString();
        rstr = rval.toString();
        goto string_concat;

    } else
#if JS_HAS_XML_SUPPORT
    if (lval.isObject() && lval.toObject().isXML() &&
        rval.isObject() && rval.toObject().isXML()) {
        if (!js_ConcatenateXML(cx, &lval.toObject(), &rval.toObject(), &rval))
            THROW();
        regs.sp[-2] = rval;
        regs.sp--;
        TypeScript::MonitorUnknown(cx, f.script(), f.pc());
    } else
#endif
    {
        bool lIsObject = lval.isObject(), rIsObject = rval.isObject();
        if (!ToPrimitive(f.cx, &lval))
            THROW();
        if (!ToPrimitive(f.cx, &rval))
            THROW();
        if ((lIsString = lval.isString()) || (rIsString = rval.isString())) {
            if (lIsString) {
                lstr = lval.toString();
            } else {
                lstr = ToString(cx, lval);
                if (!lstr)
                    THROW();
                regs.sp[-2].setString(lstr);
            }
            if (rIsString) {
                rstr = rval.toString();
            } else {
                rstr = ToString(cx, rval);
                if (!rstr)
                    THROW();
                regs.sp[-1].setString(rstr);
            }
            if (lIsObject || rIsObject)
                TypeScript::MonitorString(cx, f.script(), f.pc());
            goto string_concat;

        } else {
            double l, r;
            if (!ToNumber(cx, lval, &l) || !ToNumber(cx, rval, &r))
                THROW();
            l += r;
            if (!regs.sp[-2].setNumber(l) &&
                (lIsObject || rIsObject || (!lval.isDouble() && !rval.isDouble()))) {
                TypeScript::MonitorOverflow(cx, f.script(), f.pc());
            }
        }
    }
    return;

  string_concat:
    JSString *str = js_ConcatStrings(cx, lstr, rstr);
    if (!str)
        THROW();
    regs.sp[-2].setString(str);
    regs.sp--;
}


void JS_FASTCALL
stubs::Sub(VMFrame &f)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;
    double d1, d2;
    if (!ToNumber(cx, regs.sp[-2], &d1) || !ToNumber(cx, regs.sp[-1], &d2))
        THROW();
    double d = d1 - d2;
    if (!regs.sp[-2].setNumber(d))
        TypeScript::MonitorOverflow(cx, f.script(), f.pc());
}

void JS_FASTCALL
stubs::Mul(VMFrame &f)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;
    double d1, d2;
    if (!ToNumber(cx, regs.sp[-2], &d1) || !ToNumber(cx, regs.sp[-1], &d2))
        THROW();
    double d = d1 * d2;
    if (!regs.sp[-2].setNumber(d))
        TypeScript::MonitorOverflow(cx, f.script(), f.pc());
}

void JS_FASTCALL
stubs::Div(VMFrame &f)
{
    JSContext *cx = f.cx;
    JSRuntime *rt = cx->runtime;
    FrameRegs &regs = f.regs;

    double d1, d2;
    if (!ToNumber(cx, regs.sp[-2], &d1) || !ToNumber(cx, regs.sp[-1], &d2))
        THROW();
    if (d2 == 0) {
        const Value *vp;
#ifdef XP_WIN
        
        if (MOZ_DOUBLE_IS_NaN(d2))
            vp = &rt->NaNValue;
        else
#endif
        if (d1 == 0 || MOZ_DOUBLE_IS_NaN(d1))
            vp = &rt->NaNValue;
        else if (MOZ_DOUBLE_IS_NEGATIVE(d1) != MOZ_DOUBLE_IS_NEGATIVE(d2))
            vp = &rt->negativeInfinityValue;
        else
            vp = &rt->positiveInfinityValue;
        regs.sp[-2] = *vp;
        TypeScript::MonitorOverflow(cx, f.script(), f.pc());
    } else {
        d1 /= d2;
        if (!regs.sp[-2].setNumber(d1))
            TypeScript::MonitorOverflow(cx, f.script(), f.pc());
    }
}

void JS_FASTCALL
stubs::Mod(VMFrame &f)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;

    Value &lref = regs.sp[-2];
    Value &rref = regs.sp[-1];
    int32_t l, r;
    if (lref.isInt32() && rref.isInt32() &&
        (l = lref.toInt32()) >= 0 && (r = rref.toInt32()) > 0) {
        int32_t mod = l % r;
        regs.sp[-2].setInt32(mod);
    } else {
        double d1, d2;
        if (!ToNumber(cx, regs.sp[-2], &d1) || !ToNumber(cx, regs.sp[-1], &d2))
            THROW();
        if (d2 == 0) {
            regs.sp[-2].setDouble(js_NaN);
        } else {
            d1 = js_fmod(d1, d2);
            regs.sp[-2].setDouble(d1);
        }
        TypeScript::MonitorOverflow(cx, f.script(), f.pc());
    }
}

void JS_FASTCALL
stubs::DebuggerStatement(VMFrame &f, jsbytecode *pc)
{
    JSDebuggerHandler handler = f.cx->runtime->debugHooks.debuggerHandler;
    if (handler || !f.cx->compartment->getDebuggees().empty()) {
        JSTrapStatus st = JSTRAP_CONTINUE;
        Value rval;
        if (handler)
            st = handler(f.cx, f.script(), pc, &rval, f.cx->runtime->debugHooks.debuggerHandlerData);
        if (st == JSTRAP_CONTINUE)
            st = Debugger::onDebuggerStatement(f.cx, &rval);

        switch (st) {
          case JSTRAP_THROW:
            f.cx->setPendingException(rval);
            THROW();

          case JSTRAP_RETURN:
            f.cx->clearPendingException();
            f.cx->fp()->setReturnValue(rval);
            *f.returnAddressLocation() = f.cx->jaegerRuntime().forceReturnFromFastCall();
            break;

          case JSTRAP_ERROR:
            f.cx->clearPendingException();
            THROW();

          default:
            break;
        }
    }
}

void JS_FASTCALL
stubs::Interrupt(VMFrame &f, jsbytecode *pc)
{
    gc::MaybeVerifyBarriers(f.cx);

    if (!js_HandleExecutionInterrupt(f.cx))
        THROW();
}

void JS_FASTCALL
stubs::RecompileForInline(VMFrame &f)
{
    JSScript *script = f.script();

    ExpandInlineFrames(f.cx->compartment);
    Recompiler::clearStackReferences(f.cx->runtime->defaultFreeOp(), script);

#ifdef JS_ION
    if (ion::IsEnabled(f.cx) && f.jit()->nchunks == 1 &&
        script->canIonCompile() && !script->hasIonScript())
    {
        
        
        
        JS_ASSERT(!f.jit()->mustDestroyEntryChunk);
        f.jit()->mustDestroyEntryChunk = true;
        f.jit()->disableScriptEntry();
        return;
    }
#endif

    f.jit()->destroyChunk(f.cx->runtime->defaultFreeOp(), f.chunkIndex(),  false);
}

void JS_FASTCALL
stubs::Trap(VMFrame &f, uint32_t trapTypes)
{
    Value rval;

    




    JSTrapStatus result = JSTRAP_CONTINUE;
    if (trapTypes & JSTRAP_SINGLESTEP) {
        



        JSInterruptHook hook = f.cx->runtime->debugHooks.interruptHook;
        if (hook)
            result = hook(f.cx, f.script(), f.pc(), &rval, f.cx->runtime->debugHooks.interruptHookData);

        if (result == JSTRAP_CONTINUE)
            result = Debugger::onSingleStep(f.cx, &rval);
    }

    if (result == JSTRAP_CONTINUE && (trapTypes & JSTRAP_TRAP))
        result = Debugger::onTrap(f.cx, &rval);

    switch (result) {
      case JSTRAP_THROW:
        f.cx->setPendingException(rval);
        THROW();

      case JSTRAP_RETURN:
        f.cx->clearPendingException();
        f.cx->fp()->setReturnValue(rval);
        *f.returnAddressLocation() = f.cx->jaegerRuntime().forceReturnFromFastCall();
        break;

      case JSTRAP_ERROR:
        f.cx->clearPendingException();
        THROW();

      default:
        break;
    }
}

void JS_FASTCALL
stubs::This(VMFrame &f)
{
    



    if (f.regs.inlined()) {
        f.script()->uninlineable = true;
        MarkTypeObjectFlags(f.cx, &f.fp()->callee(), OBJECT_FLAG_UNINLINEABLE);
    }

    if (!ComputeThis(f.cx, f.fp()))
        THROW();
    f.regs.sp[-1] = f.fp()->thisValue();
}

void JS_FASTCALL
stubs::Neg(VMFrame &f)
{
    double d;
    if (!ToNumber(f.cx, f.regs.sp[-1], &d))
        THROW();
    d = -d;
    if (!f.regs.sp[-1].setNumber(d))
        TypeScript::MonitorOverflow(f.cx, f.script(), f.pc());
}

void JS_FASTCALL
stubs::NewInitArray(VMFrame &f, uint32_t count)
{
    Rooted<TypeObject*> type(f.cx, (TypeObject *) f.scratch);
    RootedObject obj(f.cx, NewDenseAllocatedArray(f.cx, count));
    if (!obj)
        THROW();

    if (type) {
        obj->setType(type);
    } else {
        RootedScript script(f.cx, f.script());
        if (!SetInitializerObjectType(f.cx, script, f.pc(), obj))
            THROW();
    }

    f.regs.sp[0].setObject(*obj);
}

void JS_FASTCALL
stubs::NewInitObject(VMFrame &f, JSObject *baseobj)
{
    JSContext *cx = f.cx;
    Rooted<TypeObject*> type(f.cx, (TypeObject *) f.scratch);

    RootedObject obj(cx);
    if (baseobj) {
        Rooted<JSObject*> base(cx, baseobj);
        obj = CopyInitializerObject(cx, base);
    } else {
        gc::AllocKind kind = GuessObjectGCKind(0);
        obj = NewBuiltinClassInstance(cx, &ObjectClass, kind);
    }

    if (!obj)
        THROW();

    if (type) {
        obj->setType(type);
    } else {
        RootedScript script(f.cx, f.script());
        if (!SetInitializerObjectType(cx, script, f.pc(), obj))
            THROW();
    }

    f.regs.sp[0].setObject(*obj);
}

void JS_FASTCALL
stubs::InitElem(VMFrame &f, uint32_t last)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;

    
    JS_ASSERT(regs.stackDepth() >= 3);
    const Value &rref = regs.sp[-1];

    
    const Value &lref = regs.sp[-3];
    JS_ASSERT(lref.isObject());
    RootedObject obj(cx, &lref.toObject());

    
    RootedId id(cx);
    const Value &idval = regs.sp[-2];
    if (!FetchElementId(f.cx, obj, idval, id.address(), &regs.sp[-2]))
        THROW();

    




    if (rref.isMagic(JS_ARRAY_HOLE)) {
        JS_ASSERT(obj->isArray());
        JS_ASSERT(JSID_IS_INT(id));
        JS_ASSERT(uint32_t(JSID_TO_INT(id)) < StackSpace::ARGS_LENGTH_MAX);
        if (last && !js_SetLengthProperty(cx, obj, (uint32_t) (JSID_TO_INT(id) + 1)))
            THROW();
    } else {
        if (!obj->defineGeneric(cx, id, rref, NULL, NULL, JSPROP_ENUMERATE))
            THROW();
    }
}

void JS_FASTCALL
stubs::RegExp(VMFrame &f, JSObject *regex)
{
    



    JSObject *proto = f.fp()->global().getOrCreateRegExpPrototype(f.cx);
    if (!proto)
        THROW();
    JS_ASSERT(proto);
    JSObject *obj = CloneRegExpObject(f.cx, regex, proto);
    if (!obj)
        THROW();
    f.regs.sp[0].setObject(*obj);
}

JSObject * JS_FASTCALL
stubs::Lambda(VMFrame &f, JSFunction *fun_)
{
    RootedFunction fun(f.cx, fun_);
    fun = CloneFunctionObjectIfNotSingleton(f.cx, fun, f.fp()->scopeChain());
    if (!fun)
        THROWV(NULL);

    return fun;
}

void JS_FASTCALL
stubs::GetProp(VMFrame &f, PropertyName *name)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;

    RootedValue rval(cx);
    if (!GetPropertyOperation(cx, f.script(), f.pc(), f.regs.sp[-1], rval.address()))
        THROW();

    regs.sp[-1] = rval;
}

void JS_FASTCALL
stubs::GetPropNoCache(VMFrame &f, PropertyName *name)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;

    const Value &lval = f.regs.sp[-1];

    
    JS_ASSERT(lval.isObject());
    JS_ASSERT(name == cx->runtime->atomState.classPrototypeAtom);

    JSObject *obj = &lval.toObject();

    Value rval;
    if (!obj->getProperty(cx, name, &rval))
        THROW();

    regs.sp[-1] = rval;
}

void JS_FASTCALL
stubs::Iter(VMFrame &f, uint32_t flags)
{
    if (!ValueToIterator(f.cx, flags, &f.regs.sp[-1]))
        THROW();
    JS_ASSERT(!f.regs.sp[-1].isPrimitive());
}

static void
InitPropOrMethod(VMFrame &f, PropertyName *name, JSOp op)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;

    
    JS_ASSERT(regs.stackDepth() >= 2);
    RootedValue rval(f.cx, regs.sp[-1]);

    
    RootedObject obj(cx, &regs.sp[-2].toObject());
    JS_ASSERT(obj->isNative());

    
    RootedId id(cx, NameToId(name));

    if (JS_UNLIKELY(name == cx->runtime->atomState.protoAtom)
        ? !baseops::SetPropertyHelper(cx, obj, obj, id, 0, rval.address(), false)
        : !DefineNativeProperty(cx, obj, id, rval, NULL, NULL,
                                JSPROP_ENUMERATE, 0, 0, 0)) {
        THROW();
    }
}

void JS_FASTCALL
stubs::InitProp(VMFrame &f, PropertyName *name)
{
    InitPropOrMethod(f, name, JSOP_INITPROP);
}

void JS_FASTCALL
stubs::IterNext(VMFrame &f, int32_t offset)
{
    JS_ASSERT(f.regs.stackDepth() >= unsigned(offset));
    JS_ASSERT(f.regs.sp[-offset].isObject());

    JSObject *iterobj = &f.regs.sp[-offset].toObject();
    f.regs.sp[0].setNull();
    f.regs.sp++;
    if (!js_IteratorNext(f.cx, iterobj, &f.regs.sp[-1]))
        THROW();
}

JSBool JS_FASTCALL
stubs::IterMore(VMFrame &f)
{
    JS_ASSERT(f.regs.stackDepth() >= 1);
    JS_ASSERT(f.regs.sp[-1].isObject());

    Value v;
    Rooted<JSObject*> iterobj(f.cx, &f.regs.sp[-1].toObject());
    if (!js_IteratorMore(f.cx, iterobj, &v))
        THROWV(JS_FALSE);

    return v.toBoolean();
}

void JS_FASTCALL
stubs::EndIter(VMFrame &f)
{
    JS_ASSERT(f.regs.stackDepth() >= 1);
    if (!CloseIterator(f.cx, &f.regs.sp[-1].toObject()))
        THROW();
}

JSString * JS_FASTCALL
stubs::TypeOf(VMFrame &f)
{
    const Value &ref = f.regs.sp[-1];
    JSType type = JS_TypeOfValue(f.cx, ref);
    return f.cx->runtime->atomState.typeAtoms[type];
}

void JS_FASTCALL
stubs::StrictEq(VMFrame &f)
{
    const Value &rhs = f.regs.sp[-1];
    const Value &lhs = f.regs.sp[-2];
    bool equal;
    if (!StrictlyEqual(f.cx, lhs, rhs, &equal))
        THROW();
    f.regs.sp--;
    f.regs.sp[-1].setBoolean(equal == JS_TRUE);
}

void JS_FASTCALL
stubs::StrictNe(VMFrame &f)
{
    const Value &rhs = f.regs.sp[-1];
    const Value &lhs = f.regs.sp[-2];
    bool equal;
    if (!StrictlyEqual(f.cx, lhs, rhs, &equal))
        THROW();
    f.regs.sp--;
    f.regs.sp[-1].setBoolean(equal != JS_TRUE);
}

void JS_FASTCALL
stubs::Throw(VMFrame &f)
{
    JSContext *cx = f.cx;

    JS_ASSERT(!cx->isExceptionPending());
    cx->setPendingException(f.regs.sp[-1]);
    THROW();
}

void JS_FASTCALL
stubs::Arguments(VMFrame &f)
{
    ArgumentsObject *obj = ArgumentsObject::createExpected(f.cx, f.fp());
    if (!obj)
        THROW();
    f.regs.sp[0] = ObjectValue(*obj);
}

JSBool JS_FASTCALL
stubs::InstanceOf(VMFrame &f)
{
    JSContext *cx = f.cx;
    FrameRegs &regs = f.regs;

    const Value &rref = regs.sp[-1];
    if (rref.isPrimitive()) {
        js_ReportValueError(cx, JSMSG_BAD_INSTANCEOF_RHS,
                            -1, rref, NULL);
        THROWV(JS_FALSE);
    }
    RootedObject obj(cx, &rref.toObject());
    const Value &lref = regs.sp[-2];
    JSBool cond = JS_FALSE;
    if (!HasInstance(cx, obj, lref, &cond))
        THROWV(JS_FALSE);
    f.regs.sp[-2].setBoolean(cond);
    return cond;
}

void JS_FASTCALL
stubs::FastInstanceOf(VMFrame &f)
{
    const Value &lref = f.regs.sp[-1];

    if (lref.isPrimitive()) {
        



        js_ReportValueError(f.cx, JSMSG_BAD_PROTOTYPE, -1, f.regs.sp[-2], NULL);
        THROW();
    }

    f.regs.sp[-3].setBoolean(js_IsDelegate(f.cx, &lref.toObject(), f.regs.sp[-3]));
}

void JS_FASTCALL
stubs::EnterBlock(VMFrame &f, JSObject *obj)
{
    FrameRegs &regs = f.regs;
    JS_ASSERT(!f.regs.inlined());

    StaticBlockObject &blockObj = obj->asStaticBlock();

    if (*regs.pc == JSOP_ENTERBLOCK) {
        JS_ASSERT(regs.stackDepth() == blockObj.stackDepth());
        JS_ASSERT(regs.stackDepth() + blockObj.slotCount() <= f.fp()->script()->nslots);
        Value *vp = regs.sp + blockObj.slotCount();
        SetValueRangeToUndefined(regs.sp, vp);
        regs.sp = vp;
    }

    
    if (!regs.fp()->pushBlock(f.cx, blockObj))
        THROW();
}

void JS_FASTCALL
stubs::LeaveBlock(VMFrame &f)
{
    f.fp()->popBlock(f.cx);
}

inline void *
FindNativeCode(VMFrame &f, jsbytecode *target)
{
    void* native = f.fp()->script()->nativeCodeForPC(f.fp()->isConstructing(), target);
    if (native)
        return native;

    uint32_t sourceOffset = f.pc() - f.script()->code;
    uint32_t targetOffset = target - f.script()->code;

    CrossChunkEdge *edges = f.jit()->edges();
    for (size_t i = 0; i < f.jit()->nedges; i++) {
        const CrossChunkEdge &edge = edges[i];
        if (edge.source == sourceOffset && edge.target == targetOffset)
            return edge.shimLabel;
    }

    JS_NOT_REACHED("Missing edge");
    return NULL;
}

void * JS_FASTCALL
stubs::LookupSwitch(VMFrame &f, jsbytecode *pc)
{
    jsbytecode *jpc = pc;
    JSScript *script = f.fp()->script();

    
    Value lval = f.regs.sp[-1];

    if (!lval.isPrimitive())
        return FindNativeCode(f, pc + GET_JUMP_OFFSET(pc));

    JS_ASSERT(pc[0] == JSOP_LOOKUPSWITCH);

    pc += JUMP_OFFSET_LEN;
    uint32_t npairs = GET_UINT16(pc);
    pc += UINT16_LEN;

    JS_ASSERT(npairs);

    if (lval.isString()) {
        JSLinearString *str = lval.toString()->ensureLinear(f.cx);
        if (!str)
            THROWV(NULL);
        for (uint32_t i = 1; i <= npairs; i++) {
            Value rval = script->getConst(GET_UINT32_INDEX(pc));
            pc += UINT32_INDEX_LEN;
            if (rval.isString()) {
                JSLinearString *rhs = &rval.toString()->asLinear();
                if (rhs == str || EqualStrings(str, rhs))
                    return FindNativeCode(f, jpc + GET_JUMP_OFFSET(pc));
            }
            pc += JUMP_OFFSET_LEN;
        }
    } else if (lval.isNumber()) {
        double d = lval.toNumber();
        for (uint32_t i = 1; i <= npairs; i++) {
            Value rval = script->getConst(GET_UINT32_INDEX(pc));
            pc += UINT32_INDEX_LEN;
            if (rval.isNumber() && d == rval.toNumber())
                return FindNativeCode(f, jpc + GET_JUMP_OFFSET(pc));
            pc += JUMP_OFFSET_LEN;
        }
    } else {
        for (uint32_t i = 1; i <= npairs; i++) {
            Value rval = script->getConst(GET_UINT32_INDEX(pc));
            pc += UINT32_INDEX_LEN;
            if (lval == rval)
                return FindNativeCode(f, jpc + GET_JUMP_OFFSET(pc));
            pc += JUMP_OFFSET_LEN;
        }
    }

    return FindNativeCode(f, jpc + GET_JUMP_OFFSET(jpc));
}

void * JS_FASTCALL
stubs::TableSwitch(VMFrame &f, jsbytecode *origPc)
{
    jsbytecode * const originalPC = origPc;

    DebugOnly<JSOp> op = JSOp(*originalPC);
    JS_ASSERT(op == JSOP_TABLESWITCH);

    uint32_t jumpOffset = GET_JUMP_OFFSET(originalPC);
    jsbytecode *pc = originalPC + JUMP_OFFSET_LEN;

    
    Value rval = f.regs.sp[-1];

    int32_t tableIdx;
    if (rval.isInt32()) {
        tableIdx = rval.toInt32();
    } else if (rval.isDouble()) {
        double d = rval.toDouble();
        if (d == 0) {
            
            tableIdx = 0;
        } else if (!MOZ_DOUBLE_IS_INT32(d, &tableIdx)) {
            goto finally;
        }
    } else {
        goto finally;
    }

    {
        int32_t low = GET_JUMP_OFFSET(pc);
        pc += JUMP_OFFSET_LEN;
        int32_t high = GET_JUMP_OFFSET(pc);
        pc += JUMP_OFFSET_LEN;

        tableIdx -= low;
        if ((uint32_t) tableIdx < (uint32_t)(high - low + 1)) {
            pc += JUMP_OFFSET_LEN * tableIdx;
            if (uint32_t candidateOffset = GET_JUMP_OFFSET(pc))
                jumpOffset = candidateOffset;
        }
    }

finally:
    
    return FindNativeCode(f, originalPC + jumpOffset);
}

void JS_FASTCALL
stubs::Pos(VMFrame &f)
{
    if (!ToNumber(f.cx, &f.regs.sp[-1]))
        THROW();
    if (!f.regs.sp[-1].isInt32())
        TypeScript::MonitorOverflow(f.cx, f.script(), f.pc());
}

void JS_FASTCALL
stubs::DelName(VMFrame &f, PropertyName *name_)
{
    RootedObject scopeObj(f.cx, f.cx->stack.currentScriptedScopeChain());
    RootedPropertyName name(f.cx, name_);

    RootedObject obj(f.cx), obj2(f.cx);
    RootedShape prop(f.cx);
    if (!FindProperty(f.cx, name, scopeObj, &obj, &obj2, &prop))
        THROW();

    
    JS_ASSERT(!f.script()->strictModeCode);

    
    f.regs.sp++;
    f.regs.sp[-1] = BooleanValue(true);
    if (prop) {
        if (!obj->deleteProperty(f.cx, name, &f.regs.sp[-1], false))
            THROW();
    }
}

template<JSBool strict>
void JS_FASTCALL
stubs::DelProp(VMFrame &f, PropertyName *name_)
{
    JSContext *cx = f.cx;
    RootedPropertyName name(cx, name_);

    JSObject *obj = ValueToObject(cx, f.regs.sp[-1]);
    if (!obj)
        THROW();

    Value rval;
    if (!obj->deleteProperty(cx, name, &rval, strict))
        THROW();

    f.regs.sp[-1] = rval;
}

template void JS_FASTCALL stubs::DelProp<true>(VMFrame &f, PropertyName *name);
template void JS_FASTCALL stubs::DelProp<false>(VMFrame &f, PropertyName *name);

template<JSBool strict>
void JS_FASTCALL
stubs::DelElem(VMFrame &f)
{
    JSContext *cx = f.cx;

    JSObject *obj = ValueToObject(cx, f.regs.sp[-2]);
    if (!obj)
        THROW();

    const Value &propval = f.regs.sp[-1];
    Value &rval = f.regs.sp[-2];

    if (!obj->deleteByValue(cx, propval, &rval, strict))
        THROW();
}

void JS_FASTCALL
stubs::DefVarOrConst(VMFrame &f, PropertyName *dn)
{
    unsigned attrs = JSPROP_ENUMERATE;
    if (!f.fp()->isEvalFrame())
        attrs |= JSPROP_PERMANENT;
    if (JSOp(*f.regs.pc) == JSOP_DEFCONST)
        attrs |= JSPROP_READONLY;

    Rooted<JSObject*> varobj(f.cx, &f.fp()->varObj());
    RootedPropertyName name(f.cx, dn);

    if (!DefVarOrConstOperation(f.cx, varobj, name, attrs))
        THROW();
}

void JS_FASTCALL
stubs::SetConst(VMFrame &f, PropertyName *name)
{
    JSContext *cx = f.cx;

    JSObject *obj = &f.fp()->varObj();
    const Value &ref = f.regs.sp[-1];

    if (!obj->defineProperty(cx, name, ref, JS_PropertyStub, JS_StrictPropertyStub,
                             JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY))
    {
        THROW();
    }
}

JSBool JS_FASTCALL
stubs::In(VMFrame &f)
{
    JSContext *cx = f.cx;

    const Value &rref = f.regs.sp[-1];
    if (!rref.isObject()) {
        js_ReportValueError(cx, JSMSG_IN_NOT_OBJECT, -1, rref, NULL);
        THROWV(JS_FALSE);
    }

    RootedObject obj(cx, &rref.toObject());
    RootedId id(cx);
    if (!FetchElementId(f.cx, obj, f.regs.sp[-2], id.address(), &f.regs.sp[-2]))
        THROWV(JS_FALSE);

    RootedObject obj2(cx);
    RootedShape prop(cx);
    if (!obj->lookupGeneric(cx, id, &obj2, &prop))
        THROWV(JS_FALSE);

    return !!prop;
}

template void JS_FASTCALL stubs::DelElem<true>(VMFrame &f);
template void JS_FASTCALL stubs::DelElem<false>(VMFrame &f);

void JS_FASTCALL
stubs::TypeBarrierHelper(VMFrame &f, uint32_t which)
{
    JS_ASSERT(which == 0 || which == 1);

    
    Value &result = f.regs.sp[-1 - (int)which];
    result = f.regs.sp[0];

    





    if (f.script()->hasAnalysis() && f.script()->analysis()->ranInference()) {
        AutoEnterTypeInference enter(f.cx);
        f.script()->analysis()->breakTypeBarriers(f.cx, f.pc() - f.script()->code, false);
    }

    TypeScript::Monitor(f.cx, f.script(), f.pc(), result);
}

void JS_FASTCALL
stubs::StubTypeHelper(VMFrame &f, int32_t which)
{
    const Value &result = f.regs.sp[which];

    if (f.script()->hasAnalysis() && f.script()->analysis()->ranInference()) {
        AutoEnterTypeInference enter(f.cx);
        f.script()->analysis()->breakTypeBarriers(f.cx, f.pc() - f.script()->code, false);
    }

    TypeScript::Monitor(f.cx, f.script(), f.pc(), result);
}





void JS_FASTCALL
stubs::TypeBarrierReturn(VMFrame &f, Value *vp)
{
    TypeScript::Monitor(f.cx, f.script(), f.pc(), vp[0]);
}

void JS_FASTCALL
stubs::NegZeroHelper(VMFrame &f)
{
    f.regs.sp[-1].setDouble(-0.0);
    TypeScript::MonitorOverflow(f.cx, f.script(), f.pc());
}

void JS_FASTCALL
stubs::CheckArgumentTypes(VMFrame &f)
{
    StackFrame *fp = f.fp();
    JSFunction *fun = fp->fun();
    JSScript *script = fun->script();
    RecompilationMonitor monitor(f.cx);

    {
        
        types::AutoEnterTypeInference enter(f.cx);

        if (!f.fp()->isConstructing())
            TypeScript::SetThis(f.cx, script, fp->thisValue());
        for (unsigned i = 0; i < fun->nargs; i++)
            TypeScript::SetArgument(f.cx, script, i, fp->unaliasedFormal(i, DONT_CHECK_ALIASING));
    }

    if (monitor.recompiled())
        return;

#ifdef JS_MONOIC
    ic::GenerateArgumentCheckStub(f);
#endif
}

#ifdef DEBUG
void JS_FASTCALL
stubs::AssertArgumentTypes(VMFrame &f)
{
    StackFrame *fp = f.fp();
    JSFunction *fun = fp->fun();
    JSScript *script = fun->script();

    



    if (!fp->isConstructing()) {
        Type type = GetValueType(f.cx, fp->thisValue());
        if (!TypeScript::ThisTypes(script)->hasType(type))
            TypeFailure(f.cx, "Missing type for this: %s", TypeString(type));
    }

    for (unsigned i = 0; i < fun->nargs; i++) {
        Type type = GetValueType(f.cx, fp->unaliasedFormal(i, DONT_CHECK_ALIASING));
        if (!TypeScript::ArgTypes(script, i)->hasType(type))
            TypeFailure(f.cx, "Missing type for arg %d: %s", i, TypeString(type));
    }
}
#endif





void JS_FASTCALL stubs::MissedBoundsCheckEntry(VMFrame &f) {}
void JS_FASTCALL stubs::MissedBoundsCheckHead(VMFrame &f) {}

void * JS_FASTCALL
stubs::InvariantFailure(VMFrame &f, void *rval)
{
    







    void *repatchCode = f.scratch;
    JS_ASSERT(repatchCode);
    void **frameAddr = f.returnAddressLocation();
    *frameAddr = repatchCode;

    
    JSScript *script = f.fp()->script();
    JS_ASSERT(!script->failedBoundsCheck);
    script->failedBoundsCheck = true;

    ExpandInlineFrames(f.cx->compartment);

    mjit::Recompiler::clearStackReferences(f.cx->runtime->defaultFreeOp(), script);
    mjit::ReleaseScriptCode(f.cx->runtime->defaultFreeOp(), script);

    
    return rval;
}

void JS_FASTCALL
stubs::Exception(VMFrame &f)
{
    
    
    if (f.cx->runtime->interrupt && !js_HandleExecutionInterrupt(f.cx))
        THROW();

    f.regs.sp[0] = f.cx->getPendingException();
    f.cx->clearPendingException();
}

void JS_FASTCALL
stubs::StrictEvalPrologue(VMFrame &f)
{
    if (!f.fp()->jitStrictEvalPrologue(f.cx))
        THROW();
}

void JS_FASTCALL
stubs::HeavyweightFunctionPrologue(VMFrame &f)
{
    if (!f.fp()->jitHeavyweightFunctionPrologue(f.cx))
        THROW();
}

void JS_FASTCALL
stubs::Epilogue(VMFrame &f)
{
    f.fp()->epilogue(f.cx);
}

void JS_FASTCALL
stubs::AnyFrameEpilogue(VMFrame &f)
{
    




    bool ok = true;
    if (f.cx->compartment->debugMode())
        ok = js::ScriptDebugEpilogue(f.cx, f.fp(), ok);
    f.fp()->epilogue(f.cx);
    if (!ok)
        THROW();
}

template <bool Clamped>
int32_t JS_FASTCALL
stubs::ConvertToTypedInt(JSContext *cx, Value *vp)
{
    JS_ASSERT(!vp->isInt32());

    if (vp->isDouble()) {
        if (Clamped)
            return ClampDoubleToUint8(vp->toDouble());
        return ToInt32(vp->toDouble());
    }

    if (vp->isNull() || vp->isObject() || vp->isUndefined())
        return 0;

    if (vp->isBoolean())
        return vp->toBoolean() ? 1 : 0;

    JS_ASSERT(vp->isString());

    int32_t i32 = 0;
#ifdef DEBUG
    bool success =
#endif
        StringToNumberType<int32_t>(cx, vp->toString(), &i32);
    JS_ASSERT(success);

    return i32;
}

template int32_t JS_FASTCALL stubs::ConvertToTypedInt<true>(JSContext *, Value *);
template int32_t JS_FASTCALL stubs::ConvertToTypedInt<false>(JSContext *, Value *);

void JS_FASTCALL
stubs::ConvertToTypedFloat(JSContext *cx, Value *vp)
{
    JS_ASSERT(!vp->isDouble() && !vp->isInt32());

    if (vp->isNull()) {
        vp->setDouble(0);
    } else if (vp->isObject() || vp->isUndefined()) {
        vp->setDouble(js_NaN);
    } else if (vp->isBoolean()) {
        vp->setDouble(vp->toBoolean() ? 1 : 0);
    } else {
        JS_ASSERT(vp->isString());
        double d = 0;
#ifdef DEBUG
        bool success =
#endif
            StringToNumberType<double>(cx, vp->toString(), &d);
        JS_ASSERT(success);
        vp->setDouble(d);
    }
}

void JS_FASTCALL
stubs::WriteBarrier(VMFrame &f, Value *addr)
{
    gc::MarkValueUnbarriered(f.cx->compartment->barrierTracer(), addr, "write barrier");
}

void JS_FASTCALL
stubs::GCThingWriteBarrier(VMFrame &f, Value *addr)
{
    gc::Cell *cell = (gc::Cell *)addr->toGCThing();
    if (cell && !cell->isMarked())
        gc::MarkValueUnbarriered(f.cx->compartment->barrierTracer(), addr, "write barrier");
}
