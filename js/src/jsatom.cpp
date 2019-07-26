








#include <stdlib.h>
#include <string.h>

#include "mozilla/RangedPtr.h"
#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsstr.h"
#include "jsversion.h"

#include "frontend/Parser.h"
#include "gc/Marking.h"
#include "vm/Xdr.h"

#include "jsstrinlines.h"
#include "jsatominlines.h"
#include "jsobjinlines.h"

#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::ArrayEnd;
using mozilla::ArrayLength;
using mozilla::RangedPtr;

const char *
js_AtomToPrintableString(JSContext *cx, JSAtom *atom, JSAutoByteString *bytes)
{
    return js_ValueToPrintable(cx, StringValue(atom), bytes);
}

const char * js::TypeStrings[] = {
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
const char js_yield_str[]           = "yield";
#if JS_HAS_GENERATORS
const char js_close_str[]           = "close";
const char js_send_str[]            = "send";
#endif







#define JS_STRING_HASH_COUNT   1024

JSBool
js::InitAtoms(JSRuntime *rt)
{
    return rt->atoms.init(JS_STRING_HASH_COUNT);
}

void
js::FinishAtoms(JSRuntime *rt)
{
    AtomSet &atoms = rt->atoms;
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

    FixedHeapPtr<PropertyName> *names = &cx->runtime->firstCachedName;
    for (size_t i = 0; i < ArrayLength(cachedNames); i++, names++) {
        JSAtom *atom = Atomize(cx, cachedNames[i].str, cachedNames[i].length, InternAtom);
        if (!atom)
            return false;
        names->init(atom->asPropertyName());
    }
    JS_ASSERT(uintptr_t(names) == uintptr_t(&cx->runtime->atomState + 1));

    cx->runtime->emptyString = cx->names().empty;
    return true;
}

void
js::FinishCommonNames(JSRuntime *rt)
{
    rt->emptyString = NULL;
#ifdef DEBUG
    memset(&rt->atomState, JS_FREE_PATTERN, sizeof(JSAtomState));
#endif
}

void
js::MarkAtoms(JSTracer *trc)
{
    JSRuntime *rt = trc->runtime;
    for (AtomSet::Range r = rt->atoms.all(); !r.empty(); r.popFront()) {
        AtomStateEntry entry = r.front();
        if (!entry.isTagged())
            continue;

        JSAtom *tmp = entry.asPtr();
        MarkStringRoot(trc, &tmp, "interned_atom");
        JS_ASSERT(tmp == entry.asPtr());
    }
}

void
js::SweepAtoms(JSRuntime *rt)
{
    for (AtomSet::Enum e(rt->atoms); !e.empty(); e.popFront()) {
        AtomStateEntry entry = e.front();
        JSAtom *atom = entry.asPtr();
        bool isDying = IsStringAboutToBeFinalized(&atom);

        
        JS_ASSERT_IF(entry.isTagged(), !isDying);

        if (isDying)
            e.removeFront();
    }
}

bool
AtomIsInterned(JSContext *cx, JSAtom *atom)
{
    
    if (StaticStrings::isStatic(atom))
        return true;

    AtomSet::Ptr p = cx->runtime->atoms.lookup(atom);
    if (!p)
        return false;

    return p->isTagged();
}

enum OwnCharsBehavior
{
    CopyChars, 
    TakeCharOwnership
};






JS_ALWAYS_INLINE
static RawAtom
AtomizeAndTakeOwnership(JSContext *cx, const jschar *tbchars, size_t length,
                           InternBehavior ib)
{
    JS_ASSERT(tbchars[length] == 0);

    if (RawAtom s = cx->runtime->staticStrings.lookup(tbchars, length)) {
        js_free((void*)tbchars);
        return s;
    }

    





    AtomHasher::Lookup lookup(tbchars, length);
    AtomSet::AddPtr p = cx->runtime->atoms.lookupForAdd(lookup);
    SkipRoot skipHash(cx, &p); 
    if (p) {
        RawAtom atom = p->asPtr();
        p->setTagged(bool(ib));
        js_free((void*)tbchars);
        return atom;
    }

    AutoEnterAtomsCompartment ac(cx);

    RawFlatString flat = js_NewString<CanGC>(cx, const_cast<jschar*>(tbchars), length);
    if (!flat) {
        js_free((void*)tbchars);
        return NULL;
    }

    RawAtom atom = flat->morphAtomizedStringIntoAtom();

    if (!cx->runtime->atoms.relookupOrAdd(p, lookup, AtomStateEntry(atom, bool(ib)))) {
        JS_ReportOutOfMemory(cx); 
        return NULL;
    }

    return atom;
}


template <AllowGC allowGC>
JS_ALWAYS_INLINE
static RawAtom
AtomizeAndCopyChars(JSContext *cx, const jschar *tbchars, size_t length, InternBehavior ib)
{
    if (RawAtom s = cx->runtime->staticStrings.lookup(tbchars, length))
         return s;

    





    AtomHasher::Lookup lookup(tbchars, length);
    AtomSet::AddPtr p = cx->runtime->atoms.lookupForAdd(lookup);
    SkipRoot skipHash(cx, &p); 
    if (p) {
        RawAtom atom = p->asPtr();
        p->setTagged(bool(ib));
        return atom;
    }

    AutoEnterAtomsCompartment ac(cx);

    RawFlatString flat = js_NewStringCopyN<allowGC>(cx, tbchars, length);
    if (!flat)
        return NULL;

    RawAtom atom = flat->morphAtomizedStringIntoAtom();

    if (!cx->runtime->atoms.relookupOrAdd(p, lookup, AtomStateEntry(atom, bool(ib)))) {
        JS_ReportOutOfMemory(cx); 
        return NULL;
    }

    return atom;
}

template <AllowGC allowGC>
RawAtom
js::AtomizeString(JSContext *cx, JSString *str, js::InternBehavior ib )
{
    AssertCanGC();

    if (str->isAtom()) {
        JSAtom &atom = str->asAtom();
        
        if (ib != InternAtom || js::StaticStrings::isStatic(&atom))
            return &atom;

        AtomSet::Ptr p = cx->runtime->atoms.lookup(AtomHasher::Lookup(&atom));
        JS_ASSERT(p); 
        JS_ASSERT(p->asPtr() == &atom);
        JS_ASSERT(ib == InternAtom);
        p->setTagged(bool(ib));
        return &atom;
    }

    const jschar *chars = str->getChars(cx);
    if (!chars)
        return NULL;

    if (JSAtom *atom = AtomizeAndCopyChars<NoGC>(cx, chars, str->length(), ib))
        return atom;

    if (!allowGC)
        return NULL;

    JSLinearString *linear = str->ensureLinear(cx);
    if (!linear)
        return NULL;

    JS_ASSERT(linear->length() <= JSString::MAX_LENGTH);
    return AtomizeAndCopyChars<CanGC>(cx, linear->chars(), linear->length(), ib);
}

template RawAtom
js::AtomizeString<CanGC>(JSContext *cx, JSString *str, js::InternBehavior ib);

template RawAtom
js::AtomizeString<NoGC>(JSContext *cx, JSString *str, js::InternBehavior ib);

RawAtom
js::Atomize(JSContext *cx, const char *bytes, size_t length, InternBehavior ib)
{
    AssertCanGC();
    CHECK_REQUEST(cx);

    if (!JSString::validateLength(cx, length))
        return NULL;

    static const unsigned ATOMIZE_BUF_MAX = 32;
    if (length < ATOMIZE_BUF_MAX) {
        






        jschar inflated[ATOMIZE_BUF_MAX];
        size_t inflatedLength = ATOMIZE_BUF_MAX - 1;
        InflateStringToBuffer(cx, bytes, length, inflated, &inflatedLength);
        return AtomizeAndCopyChars<CanGC>(cx, inflated, inflatedLength, ib);
    }

    jschar *tbcharsZ = InflateString(cx, bytes, &length);
    if (!tbcharsZ)
        return NULL;
    return AtomizeAndTakeOwnership(cx, tbcharsZ, length, ib);
}

template <AllowGC allowGC>
RawAtom
js::AtomizeChars(JSContext *cx, const jschar *chars, size_t length, InternBehavior ib)
{
    CHECK_REQUEST(cx);

    if (!JSString::validateLength(cx, length))
        return NULL;

    return AtomizeAndCopyChars<allowGC>(cx, chars, length, ib);
}

template RawAtom
js::AtomizeChars<CanGC>(JSContext *cx, const jschar *chars, size_t length, InternBehavior ib);

template RawAtom
js::AtomizeChars<NoGC>(JSContext *cx, const jschar *chars, size_t length, InternBehavior ib);

template <AllowGC allowGC>
bool
js::IndexToIdSlow(JSContext *cx, uint32_t index,
                  typename MaybeRooted<jsid, allowGC>::MutableHandleType idp)
{
    JS_ASSERT(index > JSID_INT_MAX);

    jschar buf[UINT32_CHAR_BUFFER_LENGTH];
    RangedPtr<jschar> end(ArrayEnd(buf), buf, ArrayEnd(buf));
    RangedPtr<jschar> start = BackfillIndexInCharBuffer(index, end);

    JSAtom *atom = AtomizeChars<allowGC>(cx, start.get(), end - start);
    if (!atom)
        return false;

    idp.set(JSID_FROM_BITS((size_t)atom));
    return true;
}

template bool
js::IndexToIdSlow<CanGC>(JSContext *cx, uint32_t index, MutableHandleId idp);

template bool
js::IndexToIdSlow<NoGC>(JSContext *cx, uint32_t index, FakeMutableHandle<jsid> idp);

template <AllowGC allowGC>
bool
js::InternNonIntElementId(JSContext *cx, JSObject *obj, const Value &idval,
                          typename MaybeRooted<jsid, allowGC>::MutableHandleType idp,
                          typename MaybeRooted<Value, allowGC>::MutableHandleType vp)
{
    JSAtom *atom = ToAtom<allowGC>(cx, idval);
    if (!atom)
        return false;

    idp.set(AtomToId(atom));
    vp.setString(atom);
    return true;
}

template bool
js::InternNonIntElementId<CanGC>(JSContext *cx, JSObject *obj, const Value &idval,
                                 MutableHandleId idp, MutableHandleValue vp);

template bool
js::InternNonIntElementId<NoGC>(JSContext *cx, JSObject *obj, const Value &idval,
                                FakeMutableHandle<jsid> idp, FakeMutableHandle<Value> vp);

template<XDRMode mode>
bool
js::XDRAtom(XDRState<mode> *xdr, MutableHandleAtom atomp)
{
    AssertCanGC();
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
    atom = AtomizeChars<CanGC>(cx, chars, nchars);
#else
    



    jschar *chars;
    jschar stackChars[256];
    if (nchars <= ArrayLength(stackChars)) {
        chars = stackChars;
    } else {
        




        chars = cx->runtime->pod_malloc<jschar>(nchars);
        if (!chars)
            return false;
    }

    JS_ALWAYS_TRUE(xdr->codeChars(chars, nchars));
    atom = AtomizeChars<CanGC>(cx, chars, nchars);
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

