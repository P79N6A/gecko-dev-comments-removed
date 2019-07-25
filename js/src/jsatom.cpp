









































#include <stdlib.h>
#include <string.h>

#include "mozilla/RangedPtr.h"
#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jshash.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsstr.h"
#include "jsversion.h"
#include "jsxml.h"

#include "frontend/Parser.h"

#include "jsstrinlines.h"
#include "jsatominlines.h"
#include "jsobjinlines.h"

#include "vm/String-inl.h"

using namespace mozilla;
using namespace js;
using namespace js::gc;

const size_t JSAtomState::commonAtomsOffset = offsetof(JSAtomState, emptyAtom);
const size_t JSAtomState::lazyAtomsOffset = offsetof(JSAtomState, lazy);




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
    js_null_str,                

#define JS_PROTO(name,code,init) js_##name##_str,
#include "jsproto.tbl"
#undef JS_PROTO

    js_anonymous_str,           
    js_apply_str,               
    js_arguments_str,           
    js_arity_str,               
    js_BYTES_PER_ELEMENT_str,   
    js_call_str,                
    js_callee_str,              
    js_caller_str,              
    js_class_prototype_str,     
    js_constructor_str,         
    js_each_str,                
    js_eval_str,                
    js_fileName_str,            
    js_get_str,                 
    js_global_str,              
    js_ignoreCase_str,          
    js_index_str,               
    js_input_str,               
    "toISOString",              
    js_iterator_str,            
    js_join_str,                
    js_lastIndex_str,           
    js_length_str,              
    js_lineNumber_str,          
    js_message_str,             
    js_multiline_str,           
    js_name_str,                
    js_next_str,                
    js_noSuchMethod_str,        
    "[object Null]",            
    "[object Undefined]",       
    js_proto_str,               
    js_set_str,                 
    js_source_str,              
    js_stack_str,               
    js_sticky_str,              
    js_toGMTString_str,         
    js_toLocaleString_str,      
    js_toSource_str,            
    js_toString_str,            
    js_toUTCString_str,         
    js_valueOf_str,             
    js_toJSON_str,              
    "(void 0)",                 
    js_enumerable_str,          
    js_configurable_str,        
    js_writable_str,            
    js_value_str,               
    js_test_str,                
    "use strict",               
    "loc",                      
    "line",                     
    "Infinity",                 
    "NaN",                      
    "builder",                  

#if JS_HAS_XML_SUPPORT
    js_etago_str,               
    js_namespace_str,           
    js_ptagc_str,               
    js_qualifier_str,           
    js_space_str,               
    js_stago_str,               
    js_star_str,                
    js_starQualifier_str,       
    js_tagc_str,                
    js_xml_str,                 
    "@mozilla.org/js/function", 
#endif

    "Proxy",                    

    "getOwnPropertyDescriptor", 
    "getPropertyDescriptor",    
    "defineProperty",           
    "delete",                   
    "getOwnPropertyNames",      
    "enumerate",                
    "fix",                      

    "has",                      
    "hasOwn",                   
    "keys",                     
    "iterate",                  

    "WeakMap",                  

    "byteLength",               

    "return",                   
    "throw"                     
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

    JS_STATIC_ASSERT(JS_ARRAY_LENGTH(js_common_atom_names) * sizeof(JSAtom *) ==
                     lazyAtomsOffset - commonAtomsOffset);
}





JS_STATIC_ASSERT(JS_ARRAY_LENGTH(js_common_atom_names) < 256);

const size_t js_common_atom_count = JS_ARRAY_LENGTH(js_common_atom_names);

const char js_anonymous_str[]       = "anonymous";
const char js_apply_str[]           = "apply";
const char js_arguments_str[]       = "arguments";
const char js_arity_str[]           = "arity";
const char js_BYTES_PER_ELEMENT_str[] = "BYTES_PER_ELEMENT";
const char js_call_str[]            = "call";
const char js_callee_str[]          = "callee";
const char js_caller_str[]          = "caller";
const char js_class_prototype_str[] = "prototype";
const char js_constructor_str[]     = "constructor";
const char js_each_str[]            = "each";
const char js_eval_str[]            = "eval";
const char js_fileName_str[]        = "fileName";
const char js_get_str[]             = "get";
const char js_getter_str[]          = "getter";
const char js_global_str[]          = "global";
const char js_ignoreCase_str[]      = "ignoreCase";
const char js_index_str[]           = "index";
const char js_input_str[]           = "input";
const char js_iterator_str[]        = "__iterator__";
const char js_join_str[]            = "join";
const char js_lastIndex_str[]       = "lastIndex";
const char js_length_str[]          = "length";
const char js_lineNumber_str[]      = "lineNumber";
const char js_message_str[]         = "message";
const char js_multiline_str[]       = "multiline";
const char js_name_str[]            = "name";
const char js_next_str[]            = "next";
const char js_noSuchMethod_str[]    = "__noSuchMethod__";
const char js_object_str[]          = "object";
const char js_proto_str[]           = "__proto__";
const char js_setter_str[]          = "setter";
const char js_set_str[]             = "set";
const char js_source_str[]          = "source";
const char js_stack_str[]           = "stack";
const char js_sticky_str[]          = "sticky";
const char js_toGMTString_str[]     = "toGMTString";
const char js_toLocaleString_str[]  = "toLocaleString";
const char js_toSource_str[]        = "toSource";
const char js_toString_str[]        = "toString";
const char js_toUTCString_str[]     = "toUTCString";
const char js_undefined_str[]       = "undefined";
const char js_valueOf_str[]         = "valueOf";
const char js_toJSON_str[]          = "toJSON";
const char js_enumerable_str[]      = "enumerable";
const char js_configurable_str[]    = "configurable";
const char js_writable_str[]        = "writable";
const char js_value_str[]           = "value";
const char js_test_str[]            = "test";

#if JS_HAS_XML_SUPPORT
const char js_etago_str[]           = "</";
const char js_namespace_str[]       = "namespace";
const char js_ptagc_str[]           = "/>";
const char js_qualifier_str[]       = "::";
const char js_space_str[]           = " ";
const char js_stago_str[]           = "<";
const char js_star_str[]            = "*";
const char js_starQualifier_str[]   = "*::";
const char js_tagc_str[]            = ">";
const char js_xml_str[]             = "xml";
#endif

#if JS_HAS_GENERATORS
const char js_close_str[]           = "close";
const char js_send_str[]            = "send";
#endif







#define JS_STRING_HASH_COUNT   1024

JSBool
js_InitAtomState(JSRuntime *rt)
{
    JSAtomState *state = &rt->atomState;

    JS_ASSERT(!state->atoms.initialized());
    if (!state->atoms.init(JS_STRING_HASH_COUNT))
        return false;

#ifdef JS_THREADSAFE
    js_InitLock(&state->lock);
#endif
    JS_ASSERT(state->atoms.initialized());
    return JS_TRUE;
}

void
js_FinishAtomState(JSRuntime *rt)
{
    JSAtomState *state = &rt->atomState;

    if (!state->atoms.initialized()) {
        



        return;
    }

    for (AtomSet::Range r = state->atoms.all(); !r.empty(); r.popFront())
        r.front().asPtr()->finalize(rt);

#ifdef JS_THREADSAFE
    js_FinishLock(&state->lock);
#endif
}

bool
js_InitCommonAtoms(JSContext *cx)
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

    state->clearLazyAtoms();
    cx->runtime->emptyString = state->emptyAtom;
    return true;
}

void
js_FinishCommonAtoms(JSContext *cx)
{
    cx->runtime->emptyString = NULL;
    cx->runtime->atomState.junkAtoms();
}

void
js_TraceAtomState(JSTracer *trc)
{
    JSRuntime *rt = trc->context->runtime;
    JSAtomState *state = &rt->atomState;

#ifdef DEBUG
    size_t number = 0;
#endif

    if (rt->gcKeepAtoms) {
        for (AtomSet::Range r = state->atoms.all(); !r.empty(); r.popFront()) {
            JS_SET_TRACING_INDEX(trc, "locked_atom", number++);
            MarkString(trc, r.front().asPtr());
        }
    } else {
        for (AtomSet::Range r = state->atoms.all(); !r.empty(); r.popFront()) {
            AtomStateEntry entry = r.front();
            if (!entry.isTagged())
                continue;

            JS_SET_TRACING_INDEX(trc, "interned_atom", number++);
            MarkString(trc, entry.asPtr());
        }
    }
}

void
js_SweepAtomState(JSContext *cx)
{
    JSAtomState *state = &cx->runtime->atomState;

    for (AtomSet::Enum e(state->atoms); !e.empty(); e.popFront()) {
        AtomStateEntry entry = e.front();

        if (entry.isTagged()) {
            
            JS_ASSERT(!IsAboutToBeFinalized(cx, entry.asPtr()));
            continue;
        }

        if (IsAboutToBeFinalized(cx, entry.asPtr()))
            e.removeFront();
    }
}

bool
AtomIsInterned(JSContext *cx, JSAtom *atom)
{
    
    if (StaticStrings::isStatic(atom))
        return true;

    AutoLockAtomsCompartment lock(cx);
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

    AutoLockAtomsCompartment lock(cx);

    AtomSet &atoms = cx->runtime->atomState.atoms;
    AtomSet::AddPtr p = atoms.lookupForAdd(AtomHasher::Lookup(chars, length));

    if (p) {
        JSAtom *atom = p->asPtr();
        p->setTagged(bool(ib));
        return atom;
    }

    SwitchToCompartment sc(cx, cx->runtime->atomsCompartment);

    JSFixedString *key;

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

        
        AutoLockAtomsCompartment lock(cx);

        AtomSet &atoms = cx->runtime->atomState.atoms;
        AtomSet::Ptr p = atoms.lookup(AtomHasher::Lookup(&atom));
        JS_ASSERT(p); 
        JS_ASSERT(p->asPtr() == &atom);
        JS_ASSERT(ib == InternAtom);
        p->setTagged(bool(ib));
        return &atom;
    }

    if (str->isAtom())
        return &str->asAtom();

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
    AutoLockAtomsCompartment lock(cx);
    AtomSet::Ptr p = cx->runtime->atomState.atoms.lookup(AtomHasher::Lookup(chars, length));
    return p ? p->asPtr() : NULL;
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

#if JS_BITS_PER_WORD == 32
# define TEMP_SIZE_START_LOG2   5
#else
# define TEMP_SIZE_START_LOG2   6
#endif
#define TEMP_SIZE_LIMIT_LOG2    (TEMP_SIZE_START_LOG2 + NUM_TEMP_FREELISTS)

#define TEMP_SIZE_START         JS_BIT(TEMP_SIZE_START_LOG2)
#define TEMP_SIZE_LIMIT         JS_BIT(TEMP_SIZE_LIMIT_LOG2)

JS_STATIC_ASSERT(TEMP_SIZE_START >= sizeof(JSHashTable));

void
js_InitAtomMap(JSContext *cx, AtomIndexMap *indices, JSAtom **atoms)
{
    if (indices->isMap()) {
        typedef AtomIndexMap::WordMap WordMap;
        const WordMap &wm = indices->asMap();
        for (WordMap::Range r = wm.all(); !r.empty(); r.popFront()) {
            JSAtom *atom = r.front().key;
            jsatomid index = r.front().value;
            JS_ASSERT(index < indices->count());
            atoms[index] = atom;
        }
    } else {
        for (const AtomIndexMap::InlineElem *it = indices->asInline(), *end = indices->inlineEnd();
             it != end; ++it) {
            JSAtom *atom = it->key;
            if (!atom)
                continue;
            JS_ASSERT(it->value < indices->count());
            atoms[it->value] = atom;
        }
    }
}

namespace js {

bool
IndexToIdSlow(JSContext *cx, uint32 index, jsid *idp)
{
    JS_ASSERT(index > JSID_INT_MAX);

    jschar buf[UINT32_CHAR_BUFFER_LENGTH];
    RangedPtr<jschar> end(ArrayEnd(buf), buf, ArrayEnd(buf));
    RangedPtr<jschar> start = BackfillIndexInCharBuffer(index, end);

    JSAtom *atom = js_AtomizeChars(cx, start.get(), end - start);
    if (!atom)
        return false;

    *idp = ATOM_TO_JSID(atom);
    JS_ASSERT(js_CheckForStringIndex(*idp) == *idp);
    return true;
}

} 


#define JSBOXEDWORD_INT_MAX_STRING "1073741823"







jsid
js_CheckForStringIndex(jsid id)
{
    if (!JSID_IS_ATOM(id))
        return id;

    JSAtom *atom = JSID_TO_ATOM(id);
    const jschar *s = atom->chars();
    jschar ch = *s;

    JSBool negative = (ch == '-');
    if (negative)
        ch = *++s;

    if (!JS7_ISDEC(ch))
        return id;

    size_t n = atom->length() - negative;
    if (n > sizeof(JSBOXEDWORD_INT_MAX_STRING) - 1)
        return id;

    const jschar *cp = s;
    const jschar *end = s + n;

    jsuint index = JS7_UNDEC(*cp++);
    jsuint oldIndex = 0;
    jsuint c = 0;

    if (index != 0) {
        while (JS7_ISDEC(*cp)) {
            oldIndex = index;
            c = JS7_UNDEC(*cp);
            index = 10 * index + c;
            cp++;
        }
    }

    



    if (cp != end || (negative && index == 0))
        return id;

    if (negative) {
        if (oldIndex < -(JSID_INT_MIN / 10) ||
            (oldIndex == -(JSID_INT_MIN / 10) && c <= (-JSID_INT_MIN % 10)))
        {
            id = INT_TO_JSID(-jsint(index));
        }
    } else {
        if (oldIndex < JSID_INT_MAX / 10 ||
            (oldIndex == JSID_INT_MAX / 10 && c <= (JSID_INT_MAX % 10)))
        {
            id = INT_TO_JSID(jsint(index));
        }
    }

    return id;
}

#if JS_HAS_XML_SUPPORT
bool
js_InternNonIntElementIdSlow(JSContext *cx, JSObject *obj, const Value &idval,
                             jsid *idp)
{
    JS_ASSERT(idval.isObject());
    if (obj->isXML()) {
        *idp = OBJECT_TO_JSID(&idval.toObject());
        return true;
    }

    if (js_GetLocalNameFromFunctionQName(&idval.toObject(), idp, cx))
        return true;

    return js_ValueToStringId(cx, idval, idp);
}

bool
js_InternNonIntElementIdSlow(JSContext *cx, JSObject *obj, const Value &idval,
                             jsid *idp, Value *vp)
{
    JS_ASSERT(idval.isObject());
    if (obj->isXML()) {
        JSObject &idobj = idval.toObject();
        *idp = OBJECT_TO_JSID(&idobj);
        vp->setObject(idobj);
        return true;
    }

    if (js_GetLocalNameFromFunctionQName(&idval.toObject(), idp, cx)) {
        *vp = IdToValue(*idp);
        return true;
    }

    if (js_ValueToStringId(cx, idval, idp)) {
        vp->setString(JSID_TO_STRING(*idp));
        return true;
    }
    return false;
}
#endif
