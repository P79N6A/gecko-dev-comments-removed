








#include <stdlib.h>
#include <string.h>

#include "mozilla/RangedPtr.h"
#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsutil.h"
#include "jshash.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsstr.h"
#include "jsversion.h"
#include "jsxml.h"

#include "frontend/Parser.h"
#include "gc/Marking.h"

#include "jsstrinlines.h"
#include "jsatominlines.h"
#include "jsobjinlines.h"

#include "vm/String-inl.h"
#include "vm/Xdr.h"

using namespace mozilla;
using namespace js;
using namespace js::gc;

const size_t JSAtomState::commonAtomsOffset = offsetof(JSAtomState, emptyAtom);




JS_STATIC_ASSERT(sizeof(JSHashNumber) == 4);
JS_STATIC_ASSERT(sizeof(JSAtom *) == JS_BYTES_PER_WORD);

const char *
js_AtomToPrintableString(JSContext *cx, JSAtom *atom, JSAutoByteString *bytes)
{
    return js_ValueToPrintable(cx, StringValue(atom), bytes);
}

#define JS_PROTO(name,code,init) const char js_##name##_str[] = #name;
#include "jsproto.tbl"
#undef JS_PROTO











JS_STATIC_ASSERT(JSTYPE_LIMIT == 8);
JS_STATIC_ASSERT(JSTYPE_VOID == 0);

const char *const js_common_atom_names[] = {
    "",                         
    js_false_str,               
    js_true_str,                
    js_undefined_str,           
    js_object_str,              
    js_function_str,            
    "string",                   
    "number",                   
    "boolean",                  
    js_null_str,                
    "xml",                      
    js_null_str                 

#define JS_PROTO(name,code,init) ,js_##name##_str
#include "jsproto.tbl"
#undef JS_PROTO

#define DEFINE_ATOM(id, text)          ,js_##id##_str
#define DEFINE_PROTOTYPE_ATOM(id)      ,js_##id##_str
#define DEFINE_KEYWORD_ATOM(id)        ,js_##id##_str
#include "jsatom.tbl"
#undef DEFINE_ATOM
#undef DEFINE_PROTOTYPE_ATOM
#undef DEFINE_KEYWORD_ATOM
};

void
JSAtomState::checkStaticInvariants()
{
    



    JS_STATIC_ASSERT(commonAtomsOffset % sizeof(JSAtom *) == 0);
    JS_STATIC_ASSERT(sizeof(*this) % sizeof(JSAtom *) == 0);

    



    JS_STATIC_ASSERT(1 * sizeof(JSAtom *) ==
                     offsetof(JSAtomState, booleanAtoms) - commonAtomsOffset);
    JS_STATIC_ASSERT((1 + 2) * sizeof(JSAtom *) ==
                     offsetof(JSAtomState, typeAtoms) - commonAtomsOffset);
}





JS_STATIC_ASSERT(JS_ARRAY_LENGTH(js_common_atom_names) < 256);

const size_t js_common_atom_count = JS_ARRAY_LENGTH(js_common_atom_names);

const char js_undefined_str[]       = "undefined";
const char js_object_str[]          = "object";

#define DEFINE_ATOM(id, text)          const char js_##id##_str[] = text;
#define DEFINE_PROTOTYPE_ATOM(id)
#define DEFINE_KEYWORD_ATOM(id)
#include "jsatom.tbl"
#undef DEFINE_ATOM
#undef DEFINE_PROTOTYPE_ATOM
#undef DEFINE_KEYWORD_ATOM

#if JS_HAS_GENERATORS
const char js_close_str[]           = "close";
const char js_send_str[]            = "send";
#endif


const char js_getter_str[]          = "getter";
const char js_setter_str[]          = "setter";







#define JS_STRING_HASH_COUNT   1024

JSBool
js_InitAtomState(JSRuntime *rt)
{
    JSAtomState *state = &rt->atomState;

    JS_ASSERT(!state->atoms.initialized());
    if (!state->atoms.init(JS_STRING_HASH_COUNT))
        return false;

    JS_ASSERT(state->atoms.initialized());
    return true;
}

void
js_FinishAtomState(JSRuntime *rt)
{
    JSAtomState *state = &rt->atomState;

    if (!state->atoms.initialized()) {
        



        return;
    }

    FreeOp fop(rt, false, false);
    for (AtomSet::Range r = state->atoms.all(); !r.empty(); r.popFront())
        r.front().asPtr()->finalize(&fop);
}

bool
js::InitCommonAtoms(JSContext *cx)
{
    JSAtomState *state = &cx->runtime->atomState;
    JSAtom **atoms = state->commonAtomsStart();
    for (size_t i = 0; i < ArrayLength(js_common_atom_names); i++, atoms++) {
        JSAtom *atom = js_Atomize(cx, js_common_atom_names[i], strlen(js_common_atom_names[i]),
                                  InternAtom);
        if (!atom)
            return false;
        *atoms = atom->asPropertyName();
    }

    cx->runtime->emptyString = state->emptyAtom;
    return true;
}

void
js::FinishCommonAtoms(JSRuntime *rt)
{
    rt->emptyString = NULL;
    rt->atomState.junkAtoms();
}

void
js::MarkAtomState(JSTracer *trc, bool markAll)
{
    JSRuntime *rt = trc->runtime;
    JSAtomState *state = &rt->atomState;

    if (markAll) {
        for (AtomSet::Range r = state->atoms.all(); !r.empty(); r.popFront()) {
            JSAtom *tmp = r.front().asPtr();
            MarkStringRoot(trc, &tmp, "locked_atom");
            JS_ASSERT(tmp == r.front().asPtr());
        }
    } else {
        for (AtomSet::Range r = state->atoms.all(); !r.empty(); r.popFront()) {
            AtomStateEntry entry = r.front();
            if (!entry.isTagged())
                continue;

            JSAtom *tmp = entry.asPtr();
            MarkStringRoot(trc, &tmp, "interned_atom");
            JS_ASSERT(tmp == entry.asPtr());
        }
    }
}

void
js::SweepAtomState(JSRuntime *rt)
{
    JSAtomState *state = &rt->atomState;

    for (AtomSet::Enum e(state->atoms); !e.empty(); e.popFront()) {
        AtomStateEntry entry = e.front();
        JSAtom *atom = entry.asPtr();
        bool isMarked = IsStringMarked(&atom);

        
        JS_ASSERT_IF(entry.isTagged(), isMarked);

        if (!isMarked)
            e.removeFront();
        else
            e.rekeyFront(AtomHasher::Lookup(atom), AtomStateEntry(atom, entry.isTagged()));
    }
}

bool
AtomIsInterned(JSContext *cx, JSAtom *atom)
{
    
    if (StaticStrings::isStatic(atom))
        return true;

    AtomSet::Ptr p = cx->runtime->atomState.atoms.lookup(atom);
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
static JSAtom *
AtomizeInline(JSContext *cx, const jschar **pchars, size_t length,
              InternBehavior ib, OwnCharsBehavior ocb = CopyChars)
{
    const jschar *chars = *pchars;

    if (JSAtom *s = cx->runtime->staticStrings.lookup(chars, length))
        return s;

    AtomSet &atoms = cx->runtime->atomState.atoms;
    AtomSet::AddPtr p = atoms.lookupForAdd(AtomHasher::Lookup(chars, length));

    if (p) {
        JSAtom *atom = p->asPtr();
        p->setTagged(bool(ib));
        return atom;
    }

    SwitchToCompartment sc(cx, cx->runtime->atomsCompartment);

    JSFixedString *key;

    SkipRoot skip(cx, &chars);

    if (ocb == TakeCharOwnership) {
        key = js_NewString(cx, const_cast<jschar *>(chars), length);
        if (!key)
            return NULL;
        *pchars = NULL; 
    } else {
        JS_ASSERT(ocb == CopyChars);
        key = js_NewStringCopyN(cx, chars, length);
        if (!key)
            return NULL;
    }

    







    AtomHasher::Lookup lookup(chars, length);
    if (!atoms.relookupOrAdd(p, lookup, AtomStateEntry((JSAtom *) key, bool(ib)))) {
        JS_ReportOutOfMemory(cx); 
        return NULL;
    }

    return key->morphAtomizedStringIntoAtom();
}

static JSAtom *
Atomize(JSContext *cx, const jschar **pchars, size_t length,
        InternBehavior ib, OwnCharsBehavior ocb = CopyChars)
{
    return AtomizeInline(cx, pchars, length, ib, ocb);
}

JSAtom *
js_AtomizeString(JSContext *cx, JSString *str, InternBehavior ib)
{
    if (str->isAtom()) {
        JSAtom &atom = str->asAtom();
        
        if (ib != InternAtom || js::StaticStrings::isStatic(&atom))
            return &atom;

        AtomSet &atoms = cx->runtime->atomState.atoms;
        AtomSet::Ptr p = atoms.lookup(AtomHasher::Lookup(&atom));
        JS_ASSERT(p); 
        JS_ASSERT(p->asPtr() == &atom);
        JS_ASSERT(ib == InternAtom);
        p->setTagged(bool(ib));
        return &atom;
    }

    size_t length = str->length();
    const jschar *chars = str->getChars(cx);
    if (!chars)
        return NULL;

    JS_ASSERT(length <= JSString::MAX_LENGTH);
    return Atomize(cx, &chars, length, ib);
}

JSAtom *
js_Atomize(JSContext *cx, const char *bytes, size_t length, InternBehavior ib, FlationCoding fc)
{
    CHECK_REQUEST(cx);

    if (!JSString::validateLength(cx, length))
        return NULL;

    






    static const unsigned ATOMIZE_BUF_MAX = 32;
    jschar inflated[ATOMIZE_BUF_MAX];
    size_t inflatedLength = ATOMIZE_BUF_MAX - 1;

    const jschar *chars;
    OwnCharsBehavior ocb = CopyChars;
    if (length < ATOMIZE_BUF_MAX) {
        if (fc == CESU8Encoding)
            InflateUTF8StringToBuffer(cx, bytes, length, inflated, &inflatedLength, fc);
        else
            InflateStringToBuffer(cx, bytes, length, inflated, &inflatedLength);
        inflated[inflatedLength] = 0;
        chars = inflated;
    } else {
        inflatedLength = length;
        chars = InflateString(cx, bytes, &inflatedLength, fc);
        if (!chars)
            return NULL;
        ocb = TakeCharOwnership;
    }

    JSAtom *atom = Atomize(cx, &chars, inflatedLength, ib, ocb);
    if (ocb == TakeCharOwnership && chars)
        cx->free_((void *)chars);
    return atom;
}

JSAtom *
js_AtomizeChars(JSContext *cx, const jschar *chars, size_t length, InternBehavior ib)
{
    CHECK_REQUEST(cx);

    if (!JSString::validateLength(cx, length))
        return NULL;

    return AtomizeInline(cx, &chars, length, ib);
}

JSAtom *
js_GetExistingStringAtom(JSContext *cx, const jschar *chars, size_t length)
{
    if (JSAtom *atom = cx->runtime->staticStrings.lookup(chars, length))
        return atom;
    if (AtomSet::Ptr p = cx->runtime->atomState.atoms.lookup(AtomHasher::Lookup(chars, length)))
        return p->asPtr();
    return NULL;
}

#ifdef DEBUG
JS_FRIEND_API(void)
js_DumpAtoms(JSContext *cx, FILE *fp)
{
    JSAtomState *state = &cx->runtime->atomState;

    fprintf(fp, "atoms table contents:\n");
    unsigned number = 0;
    for (AtomSet::Range r = state->atoms.all(); !r.empty(); r.popFront()) {
        AtomStateEntry entry = r.front();
        fprintf(fp, "%3u ", number++);
        JSAtom *key = entry.asPtr();
        FileEscapedString(fp, key, '"');
        if (entry.isTagged())
            fputs(" interned", fp);
        putc('\n', fp);
    }
    putc('\n', fp);
}
#endif

namespace js {

void
InitAtomMap(JSContext *cx, AtomIndexMap *indices, HeapPtrAtom *atoms)
{
    if (indices->isMap()) {
        typedef AtomIndexMap::WordMap WordMap;
        const WordMap &wm = indices->asMap();
        for (WordMap::Range r = wm.all(); !r.empty(); r.popFront()) {
            JSAtom *atom = r.front().key;
            jsatomid index = r.front().value;
            JS_ASSERT(index < indices->count());
            atoms[index].init(atom);
        }
    } else {
        for (const AtomIndexMap::InlineElem *it = indices->asInline(), *end = indices->inlineEnd();
             it != end; ++it) {
            JSAtom *atom = it->key;
            if (!atom)
                continue;
            JS_ASSERT(it->value < indices->count());
            atoms[it->value].init(atom);
        }
    }
}

bool
IndexToIdSlow(JSContext *cx, uint32_t index, jsid *idp)
{
    JS_ASSERT(index > JSID_INT_MAX);

    jschar buf[UINT32_CHAR_BUFFER_LENGTH];
    RangedPtr<jschar> end(ArrayEnd(buf), buf, ArrayEnd(buf));
    RangedPtr<jschar> start = BackfillIndexInCharBuffer(index, end);

    JSAtom *atom = js_AtomizeChars(cx, start.get(), end - start);
    if (!atom)
        return false;

    *idp = JSID_FROM_BITS((size_t)atom);
    return true;
}

bool
InternNonIntElementId(JSContext *cx, JSObject *obj, const Value &idval,
                      jsid *idp, Value *vp)
{
#if JS_HAS_XML_SUPPORT
    if (idval.isObject()) {
        JSObject *idobj = &idval.toObject();

        if (obj && obj->isXML()) {
            *idp = OBJECT_TO_JSID(idobj);
            *vp = idval;
            return true;
        }

        if (js_GetLocalNameFromFunctionQName(idobj, idp, cx)) {
            *vp = IdToValue(*idp);
            return true;
        }

        if (!obj && idobj->isXMLId()) {
            *idp = OBJECT_TO_JSID(idobj);
            *vp = idval;
            return JS_TRUE;
        }
    }
#endif

    JSAtom *atom;
    if (!js_ValueToAtom(cx, idval, &atom))
        return false;

    *idp = AtomToId(atom);
    vp->setString(atom);
    return true;
}

} 

template<XDRMode mode>
bool
js::XDRAtom(XDRState<mode> *xdr, JSAtom **atomp)
{
    if (mode == XDR_ENCODE) {
        uint32_t nchars = (*atomp)->length();
        if (!xdr->codeUint32(&nchars))
            return false;

        jschar *chars = const_cast<jschar *>((*atomp)->getChars(xdr->cx()));
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
    atom = js_AtomizeChars(cx, chars, nchars);
#else
    



    jschar *chars;
    jschar stackChars[256];
    if (nchars <= ArrayLength(stackChars)) {
        chars = stackChars;
    } else {
        




        chars = static_cast<jschar *>(cx->runtime->malloc_(nchars * sizeof(jschar)));
        if (!chars)
            return false;
    }

    JS_ALWAYS_TRUE(xdr->codeChars(chars, nchars));
    atom = js_AtomizeChars(cx, chars, nchars);
    if (chars != stackChars)
        Foreground::free_(chars);
#endif 

    if (!atom)
        return false;
    *atomp = atom;
    return true;
}

template bool
js::XDRAtom(XDRState<XDR_ENCODE> *xdr, JSAtom **atomp);

template bool
js::XDRAtom(XDRState<XDR_DECODE> *xdr, JSAtom **atomp);

