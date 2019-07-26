









#include "jsatominlines.h"

#include "mozilla/RangedPtr.h"
#include "mozilla/Util.h"

#include <string.h>

#include "jscntxt.h"
#include "jsstr.h"
#include "jstypes.h"

#include "gc/Marking.h"
#include "vm/Xdr.h"

#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"
#include "jsobjinlines.h"

#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::ArrayEnd;
using mozilla::ArrayLength;
using mozilla::RangedPtr;

const char *
js::AtomToPrintableString(ExclusiveContext *cx, JSAtom *atom, JSAutoByteString *bytes)
{
    JSString *str = js_QuoteString(cx, atom, 0);
    if (!str)
        return nullptr;
    return bytes->encodeLatin1(cx, str);
}

const char * const js::TypeStrings[] = {
    js_undefined_str,
    js_object_str,
    js_function_str,
    js_string_str,
    js_number_str,
    js_boolean_str,
    js_null_str,
};

#define DEFINE_PROTO_STRING(name,code,init) const char js_##name##_str[] = #name;
JS_FOR_EACH_PROTOTYPE(DEFINE_PROTO_STRING)
#undef DEFINE_PROTO_STRING

#define CONST_CHAR_STR(idpart, id, text) const char js_##idpart##_str[] = text;
FOR_EACH_COMMON_PROPERTYNAME(CONST_CHAR_STR)
#undef CONST_CHAR_STR


const char js_break_str[]           = "break";
const char js_case_str[]            = "case";
const char js_catch_str[]           = "catch";
const char js_class_str[]           = "class";
const char js_close_str[]           = "close";
const char js_const_str[]           = "const";
const char js_continue_str[]        = "continue";
const char js_debugger_str[]        = "debugger";
const char js_default_str[]         = "default";
const char js_do_str[]              = "do";
const char js_else_str[]            = "else";
const char js_enum_str[]            = "enum";
const char js_export_str[]          = "export";
const char js_extends_str[]         = "extends";
const char js_finally_str[]         = "finally";
const char js_for_str[]             = "for";
const char js_getter_str[]          = "getter";
const char js_if_str[]              = "if";
const char js_implements_str[]      = "implements";
const char js_import_str[]          = "import";
const char js_in_str[]              = "in";
const char js_instanceof_str[]      = "instanceof";
const char js_interface_str[]       = "interface";
const char js_let_str[]             = "let";
const char js_new_str[]             = "new";
const char js_package_str[]         = "package";
const char js_private_str[]         = "private";
const char js_protected_str[]       = "protected";
const char js_public_str[]          = "public";
const char js_send_str[]            = "send";
const char js_setter_str[]          = "setter";
const char js_static_str[]          = "static";
const char js_super_str[]           = "super";
const char js_switch_str[]          = "switch";
const char js_this_str[]            = "this";
const char js_try_str[]             = "try";
const char js_typeof_str[]          = "typeof";
const char js_void_str[]            = "void";
const char js_while_str[]           = "while";
const char js_with_str[]            = "with";







#define JS_STRING_HASH_COUNT   1024

bool
js::InitAtoms(JSRuntime *rt)
{
    AutoLockForExclusiveAccess lock(rt);
    return rt->atoms().init(JS_STRING_HASH_COUNT);
}

void
js::FinishAtoms(JSRuntime *rt)
{
    AtomSet &atoms = rt->atoms();
    if (!atoms.initialized()) {
        



        return;
    }

    FreeOp fop(rt, false);
    for (AtomSet::Range r = atoms.all(); !r.empty(); r.popFront())
        r.front().asPtr()->finalize(&fop);
}

struct CommonNameInfo
{
    const char *str;
    size_t length;
};

bool
js::InitCommonNames(JSContext *cx)
{
    static const CommonNameInfo cachedNames[] = {
#define COMMON_NAME_INFO(idpart, id, text) { js_##idpart##_str, sizeof(text) - 1 },
        FOR_EACH_COMMON_PROPERTYNAME(COMMON_NAME_INFO)
#undef COMMON_NAME_INFO
#define COMMON_NAME_INFO(name, code, init) { js_##name##_str, sizeof(#name) - 1 },
        JS_FOR_EACH_PROTOTYPE(COMMON_NAME_INFO)
#undef COMMON_NAME_INFO
    };

    FixedHeapPtr<PropertyName> *names = &cx->runtime()->firstCachedName;
    for (size_t i = 0; i < ArrayLength(cachedNames); i++, names++) {
        JSAtom *atom = Atomize(cx, cachedNames[i].str, cachedNames[i].length, InternAtom);
        if (!atom)
            return false;
        names->init(atom->asPropertyName());
    }
    JS_ASSERT(uintptr_t(names) == uintptr_t(&cx->runtime()->atomState + 1));

    cx->runtime()->emptyString = cx->names().empty;
    return true;
}

void
js::FinishCommonNames(JSRuntime *rt)
{
    rt->emptyString = nullptr;
#ifdef DEBUG
    memset(&rt->atomState, JS_FREE_PATTERN, sizeof(JSAtomState));
#endif
}

void
js::MarkAtoms(JSTracer *trc)
{
    JSRuntime *rt = trc->runtime;
    for (AtomSet::Enum e(rt->atoms()); !e.empty(); e.popFront()) {
        const AtomStateEntry &entry = e.front();
        if (!entry.isTagged())
            continue;

        JSAtom *atom = entry.asPtr();
        bool tagged = entry.isTagged();
        MarkStringRoot(trc, &atom, "interned_atom");
        if (entry.asPtr() != atom)
            e.rekeyFront(AtomHasher::Lookup(atom), AtomStateEntry(atom, tagged));
    }
}

void
js::SweepAtoms(JSRuntime *rt)
{
    for (AtomSet::Enum e(rt->atoms()); !e.empty(); e.popFront()) {
        AtomStateEntry entry = e.front();
        JSAtom *atom = entry.asPtr();
        bool isDying = IsStringAboutToBeFinalized(&atom);

        
        JS_ASSERT_IF(rt->hasContexts() && entry.isTagged(), !isDying);

        if (isDying)
            e.removeFront();
    }
}

bool
AtomIsInterned(JSContext *cx, JSAtom *atom)
{
    
    if (StaticStrings::isStatic(atom))
        return true;

    AutoLockForExclusiveAccess lock(cx);

    AtomSet::Ptr p = cx->runtime()->atoms().lookup(atom);
    if (!p)
        return false;

    return p->isTagged();
}






JS_ALWAYS_INLINE
static JSAtom *
AtomizeAndTakeOwnership(ExclusiveContext *cx, jschar *tbchars, size_t length, InternBehavior ib)
{
    JS_ASSERT(tbchars[length] == 0);

    if (JSAtom *s = cx->staticStrings().lookup(tbchars, length)) {
        js_free(tbchars);
        return s;
    }

    AutoLockForExclusiveAccess lock(cx);

    





    AtomSet& atoms = cx->atoms();
    AtomSet::AddPtr p = atoms.lookupForAdd(AtomHasher::Lookup(tbchars, length));
    SkipRoot skipHash(cx, &p); 
    if (p) {
        JSAtom *atom = p->asPtr();
        p->setTagged(bool(ib));
        js_free(tbchars);
        return atom;
    }

    AutoCompartment ac(cx, cx->atomsCompartment());

    JSFlatString *flat = js_NewString<NoGC>(cx, tbchars, length);
    if (!flat) {
        js_free(tbchars);
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    JSAtom *atom = flat->morphAtomizedStringIntoAtom();

    if (!atoms.relookupOrAdd(p, AtomHasher::Lookup(tbchars, length),
                             AtomStateEntry(atom, bool(ib)))) {
        js_ReportOutOfMemory(cx); 
        return nullptr;
    }

    return atom;
}


JS_ALWAYS_INLINE
static JSAtom *
AtomizeAndCopyChars(ExclusiveContext *cx, const jschar *tbchars, size_t length, InternBehavior ib)
{
    if (JSAtom *s = cx->staticStrings().lookup(tbchars, length))
         return s;

    






    AutoLockForExclusiveAccess lock(cx);

    AtomSet& atoms = cx->atoms();
    AtomSet::AddPtr p = atoms.lookupForAdd(AtomHasher::Lookup(tbchars, length));
    SkipRoot skipHash(cx, &p); 
    if (p) {
        JSAtom *atom = p->asPtr();
        p->setTagged(bool(ib));
        return atom;
    }

    AutoCompartment ac(cx, cx->atomsCompartment());

    JSFlatString *flat = js_NewStringCopyN<NoGC>(cx, tbchars, length);
    if (!flat) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    JSAtom *atom = flat->morphAtomizedStringIntoAtom();

    if (!atoms.relookupOrAdd(p, AtomHasher::Lookup(tbchars, length),
                             AtomStateEntry(atom, bool(ib)))) {
        js_ReportOutOfMemory(cx); 
        return nullptr;
    }

    return atom;
}

JSAtom *
js::AtomizeString(ExclusiveContext *cx, JSString *str,
                  js::InternBehavior ib )
{
    if (str->isAtom()) {
        JSAtom &atom = str->asAtom();
        
        if (ib != InternAtom || js::StaticStrings::isStatic(&atom))
            return &atom;

        AutoLockForExclusiveAccess lock(cx);

        AtomSet::Ptr p = cx->atoms().lookup(AtomHasher::Lookup(&atom));
        JS_ASSERT(p); 
        JS_ASSERT(p->asPtr() == &atom);
        JS_ASSERT(ib == InternAtom);
        p->setTagged(bool(ib));
        return &atom;
    }

    const jschar *chars = str->getChars(cx);
    if (!chars)
        return nullptr;

    return AtomizeAndCopyChars(cx, chars, str->length(), ib);
}

JSAtom *
js::Atomize(ExclusiveContext *cx, const char *bytes, size_t length, InternBehavior ib)
{
    CHECK_REQUEST(cx);

    if (!JSString::validateLength(cx, length))
        return nullptr;

    static const unsigned ATOMIZE_BUF_MAX = 32;
    if (length < ATOMIZE_BUF_MAX) {
        






        jschar inflated[ATOMIZE_BUF_MAX];
        InflateStringToBuffer(bytes, length, inflated);
        return AtomizeAndCopyChars(cx, inflated, length, ib);
    }

    jschar *tbcharsZ = InflateString(cx, bytes, &length);
    if (!tbcharsZ)
        return nullptr;
    return AtomizeAndTakeOwnership(cx, tbcharsZ, length, ib);
}

JSAtom *
js::AtomizeChars(ExclusiveContext *cx, const jschar *chars, size_t length, InternBehavior ib)
{
    CHECK_REQUEST(cx);

    if (!JSString::validateLength(cx, length))
        return nullptr;

    return AtomizeAndCopyChars(cx, chars, length, ib);
}

bool
js::IndexToIdSlow(ExclusiveContext *cx, uint32_t index, jsid *idp)
{
    JS_ASSERT(index > JSID_INT_MAX);

    jschar buf[UINT32_CHAR_BUFFER_LENGTH];
    RangedPtr<jschar> end(ArrayEnd(buf), buf, ArrayEnd(buf));
    RangedPtr<jschar> start = BackfillIndexInCharBuffer(index, end);

    JSAtom *atom = AtomizeChars(cx, start.get(), end - start);
    if (!atom)
        return false;

    *idp = JSID_FROM_BITS((size_t)atom);
    return true;
}

template <AllowGC allowGC>
static JSAtom *
ToAtomSlow(ExclusiveContext *cx, typename MaybeRooted<Value, allowGC>::HandleType arg)
{
    JS_ASSERT(!arg.isString());

    Value v = arg;
    if (!v.isPrimitive()) {
        if (!cx->shouldBeJSContext() || !allowGC)
            return nullptr;
        RootedValue v2(cx, v);
        if (!ToPrimitive(cx->asJSContext(), JSTYPE_STRING, &v2))
            return nullptr;
        v = v2;
    }

    if (v.isString())
        return AtomizeString(cx, v.toString());
    if (v.isInt32())
        return Int32ToAtom(cx, v.toInt32());
    if (v.isDouble())
        return NumberToAtom(cx, v.toDouble());
    if (v.isBoolean())
        return v.toBoolean() ? cx->names().true_ : cx->names().false_;
    if (v.isNull())
        return cx->names().null;
    return cx->names().undefined;
}

template <AllowGC allowGC>
JSAtom *
js::ToAtom(ExclusiveContext *cx, typename MaybeRooted<Value, allowGC>::HandleType v)
{
    if (!v.isString())
        return ToAtomSlow<allowGC>(cx, v);

    JSString *str = v.toString();
    if (str->isAtom())
        return &str->asAtom();

    return AtomizeString(cx, str);
}

template JSAtom *
js::ToAtom<CanGC>(ExclusiveContext *cx, HandleValue v);

template JSAtom *
js::ToAtom<NoGC>(ExclusiveContext *cx, Value v);

template<XDRMode mode>
bool
js::XDRAtom(XDRState<mode> *xdr, MutableHandleAtom atomp)
{
    if (mode == XDR_ENCODE) {
        uint32_t nchars = atomp->length();
        if (!xdr->codeUint32(&nchars))
            return false;

        jschar *chars = const_cast<jschar *>(atomp->getChars(xdr->cx()));
        if (!chars)
            return false;

        return xdr->codeChars(chars, nchars);
    }

    
    uint32_t nchars;
    if (!xdr->codeUint32(&nchars))
        return false;

    JSContext *cx = xdr->cx();
    JSAtom *atom;
#if IS_LITTLE_ENDIAN
    
    const jschar *chars = reinterpret_cast<const jschar *>(xdr->buf.read(nchars * sizeof(jschar)));
    atom = AtomizeChars(cx, chars, nchars);
#else
    



    jschar *chars;
    jschar stackChars[256];
    if (nchars <= ArrayLength(stackChars)) {
        chars = stackChars;
    } else {
        




        chars = cx->runtime()->pod_malloc<jschar>(nchars);
        if (!chars)
            return false;
    }

    JS_ALWAYS_TRUE(xdr->codeChars(chars, nchars));
    atom = AtomizeChars(cx, chars, nchars);
    if (chars != stackChars)
        js_free(chars);
#endif 

    if (!atom)
        return false;
    atomp.set(atom);
    return true;
}

template bool
js::XDRAtom(XDRState<XDR_ENCODE> *xdr, MutableHandleAtom atomp);

template bool
js::XDRAtom(XDRState<XDR_DECODE> *xdr, MutableHandleAtom atomp);

