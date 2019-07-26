






#if !defined jslogic_h_inl__ && defined JS_METHODJIT
#define jslogic_h_inl__

namespace js {
namespace mjit {

static inline void
ThrowException(VMFrame &f)
{
    void *ptr = JS_FUNC_TO_DATA_PTR(void *, JaegerThrowpoline);
    *f.returnAddressLocation() = ptr;
}

#define THROW()   do { mjit::ThrowException(f); return; } while (0)
#define THROWV(v) do { mjit::ThrowException(f); return v; } while (0)

static inline void
ReportAtomNotDefined(JSContext *cx, JSAtom *atom)
{
    JSAutoByteString printable;
    if (js_AtomToPrintableString(cx, atom, &printable))
        js_ReportIsNotDefined(cx, printable.ptr());
}

inline bool
stubs::UncachedCallResult::setFunction(JSContext *cx, CallArgs &args,
                                       HandleScript callScript, jsbytecode *callPc)
{
    if (!IsFunctionObject(args.calleev(), fun.address()))
        return true;

    if (fun->isInterpretedLazy() && !fun->getOrCreateScript(cx))
        return false;

    if (cx->typeInferenceEnabled() && fun->isInterpreted() &&
        fun->nonLazyScript()->shouldCloneAtCallsite)
    {
        original = fun;
        fun = CloneFunctionAtCallsite(cx, original, callScript, callPc);
        if (!fun)
            return false;
        args.setCallee(ObjectValue(*fun));
    }

    return true;
}

} 
} 

#endif 

