









































#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jshash.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jsbit.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsparse.h"
#include "jsstr.h"
#include "jsversion.h"
#include "jsxml.h"

#include "jsstrinlines.h"
#include "jsatominlines.h"
#include "jsobjinlines.h"

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

    "WeakMap"                   
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
        r.front().toAtom()->finalize(rt);

#ifdef JS_THREADSAFE
    js_FinishLock(&state->lock);
#endif
}

bool
js_InitCommonAtoms(JSContext *cx)
{
    JSAtomState *state = &cx->runtime->atomState;
    JSAtom **atoms = state->commonAtomsStart();
    for (size_t i = 0; i < JS_ARRAY_LENGTH(js_common_atom_names); i++, atoms++) {
        *atoms = js_Atomize(cx, js_common_atom_names[i], strlen(js_common_atom_names[i]),
                            InternAtom);
        if (!*atoms)
            return false;
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
            MarkString(trc, r.front().toAtom());
        }
    } else {
        for (AtomSet::Range r = state->atoms.all(); !r.empty(); r.popFront()) {
            AtomStateEntry entry = r.front();
            if (!entry.isInterned())
                continue;

            JS_SET_TRACING_INDEX(trc, "interned_atom", number++);
            MarkString(trc, entry.toAtom());
        }
    }
}

void
js_SweepAtomState(JSContext *cx)
{
    JSAtomState *state = &cx->runtime->atomState;

    for (AtomSet::Enum e(state->atoms); !e.empty(); e.popFront()) {
        AtomStateEntry entry = e.front();

        if (entry.isInterned()) {
            
            JS_ASSERT(!IsAboutToBeFinalized(cx, entry.toAtom()));
            continue;
        }
        
        if (IsAboutToBeFinalized(cx, entry.toAtom()))
            e.removeFront();
    }
}

bool
AtomIsInterned(JSContext *cx, JSAtom *atom)
{
    if (atom->isStaticAtom())
        return true;

    AutoLockAtomsCompartment lock(cx);
    AtomSet::Ptr p = cx->runtime->atomState.atoms.lookup(atom);
    if (!p)
        return false;

    return p->isInterned();
}










static void
MakeInterned(const AutoLockAtomsCompartment &, const AtomStateEntry &entryRef, InternBehavior ib)
{
    AtomStateEntry *entry = const_cast<AtomStateEntry *>(&entryRef);
    AtomStateEntry::makeInterned(entry, ib);
    JS_ASSERT(entryRef.isInterned() >= ib);
}

enum OwnCharsBehavior
{
    CopyChars, 
    TakeCharOwnership
};






static JSAtom *
Atomize(JSContext *cx, const jschar **pchars, size_t length,
        InternBehavior ib, OwnCharsBehavior ocb = CopyChars)
{
    const jschar *chars = *pchars;

    if (JSAtom *s = JSAtom::lookupStatic(chars, length))
        return s;

    AutoLockAtomsCompartment lock(cx);

    AtomSet &atoms = cx->runtime->atomState.atoms;
    AtomSet::AddPtr p = atoms.lookupForAdd(AtomHasher::Lookup(chars, length));

    if (p) {
        JSAtom *atom = p->toAtom();
        MakeInterned(lock, *p, ib);
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
    if (!atoms.relookupOrAdd(p, lookup, AtomStateEntry(key, ib))) {
        JS_ReportOutOfMemory(cx); 
        return NULL;
    }

    return key->morphAtomizedStringIntoAtom();
}

JSAtom *
js_AtomizeString(JSContext *cx, JSString *str, InternBehavior ib)
{
    if (str->isAtom()) {
        JSAtom &atom = str->asAtom();
        
        if (ib != InternAtom || atom.isStaticAtom())
            return &atom;

        
        AutoLockAtomsCompartment lock(cx);

        AtomSet &atoms = cx->runtime->atomState.atoms;
        AtomSet::Ptr p = atoms.lookup(AtomHasher::Lookup(&atom));
        JS_ASSERT(p); 
        JS_ASSERT(p->toAtom() == &atom);
        JS_ASSERT(ib == InternAtom);
        MakeInterned(lock, *p, ib);
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
js_Atomize(JSContext *cx, const char *bytes, size_t length, InternBehavior ib, bool useCESU8)
{
    CHECK_REQUEST(cx);

    if (!CheckStringLength(cx, length))
        return NULL;

    






    static const unsigned ATOMIZE_BUF_MAX = 32;
    jschar inflated[ATOMIZE_BUF_MAX];
    size_t inflatedLength = ATOMIZE_BUF_MAX - 1;

    const jschar *chars;
    OwnCharsBehavior ocb = CopyChars;
    if (length < ATOMIZE_BUF_MAX) {
        if (useCESU8)
            js_InflateUTF8StringToBuffer(cx, bytes, length, inflated, &inflatedLength, true);
        else
            js_InflateStringToBuffer(cx, bytes, length, inflated, &inflatedLength);
        inflated[inflatedLength] = 0;
        chars = inflated;
    } else {
        inflatedLength = length;
        chars = js_InflateString(cx, bytes, &inflatedLength, useCESU8);
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

    if (!CheckStringLength(cx, length))
        return NULL;

    return Atomize(cx, &chars, length, ib);
}

JSAtom *
js_GetExistingStringAtom(JSContext *cx, const jschar *chars, size_t length)
{
    if (JSAtom *atom = JSAtom::lookupStatic(chars, length))
        return atom;
    AutoLockAtomsCompartment lock(cx);
    AtomSet::Ptr p = cx->runtime->atomState.atoms.lookup(AtomHasher::Lookup(chars, length));
    return p ? p->toAtom() : NULL;
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
        JSAtom *key = entry.toAtom();
        FileEscapedString(fp, key, '"');
        if (entry.isInterned())
            fputs(" interned", fp);
        putc('\n', fp);
    }
    putc('\n', fp);
}
#endif

static JSHashNumber
js_hash_atom_ptr(const void *key)
{
    const JSAtom *atom = (const JSAtom *) key;
    return ATOM_HASH(atom);
}

#if JS_BITS_PER_WORD == 32
# define TEMP_SIZE_START_LOG2   5
#else
# define TEMP_SIZE_START_LOG2   6
#endif
#define TEMP_SIZE_LIMIT_LOG2    (TEMP_SIZE_START_LOG2 + NUM_TEMP_FREELISTS)

#define TEMP_SIZE_START         JS_BIT(TEMP_SIZE_START_LOG2)
#define TEMP_SIZE_LIMIT         JS_BIT(TEMP_SIZE_LIMIT_LOG2)

JS_STATIC_ASSERT(TEMP_SIZE_START >= sizeof(JSHashTable));

static void *
js_alloc_temp_space(void *priv, size_t size)
{
    Parser *parser = (Parser *) priv;

    void *space;
    if (size < TEMP_SIZE_LIMIT) {
        int bin = JS_CeilingLog2(size) - TEMP_SIZE_START_LOG2;
        JS_ASSERT(unsigned(bin) < NUM_TEMP_FREELISTS);

        space = parser->tempFreeList[bin];
        if (space) {
            parser->tempFreeList[bin] = *(void **)space;
            return space;
        }
    }

    JS_ARENA_ALLOCATE(space, &parser->context->tempPool, size);
    if (!space)
        js_ReportOutOfScriptQuota(parser->context);
    return space;
}

static void
js_free_temp_space(void *priv, void *item, size_t size)
{
    if (size >= TEMP_SIZE_LIMIT)
        return;

    Parser *parser = (Parser *) priv;
    int bin = JS_CeilingLog2(size) - TEMP_SIZE_START_LOG2;
    JS_ASSERT(unsigned(bin) < NUM_TEMP_FREELISTS);

    *(void **)item = parser->tempFreeList[bin];
    parser->tempFreeList[bin] = item;
}

static JSHashEntry *
js_alloc_temp_entry(void *priv, const void *key)
{
    Parser *parser = (Parser *) priv;
    JSAtomListElement *ale;

    ale = parser->aleFreeList;
    if (ale) {
        parser->aleFreeList = ALE_NEXT(ale);
        return &ale->entry;
    }

    JS_ARENA_ALLOCATE_TYPE(ale, JSAtomListElement, &parser->context->tempPool);
    if (!ale) {
        js_ReportOutOfScriptQuota(parser->context);
        return NULL;
    }
    return &ale->entry;
}

static void
js_free_temp_entry(void *priv, JSHashEntry *he, uintN flag)
{
    Parser *parser = (Parser *) priv;
    JSAtomListElement *ale = (JSAtomListElement *) he;

    ALE_SET_NEXT(ale, parser->aleFreeList);
    parser->aleFreeList = ale;
}

static JSHashAllocOps temp_alloc_ops = {
    js_alloc_temp_space,    js_free_temp_space,
    js_alloc_temp_entry,    js_free_temp_entry
};

JSAtomListElement *
JSAtomList::rawLookup(JSAtom *atom, JSHashEntry **&hep)
{
    if (table) {
        hep = JS_HashTableRawLookup(table, ATOM_HASH(atom), atom);
        return (JSAtomListElement *) *hep;
    }

    JSHashEntry **alep = &list;
    hep = NULL;
    JSAtomListElement *ale;
    while ((ale = (JSAtomListElement *)*alep) != NULL) {
        if (ALE_ATOM(ale) == atom) {
            
            *alep = ale->entry.next;
            ale->entry.next = list;
            list = &ale->entry;
            break;
        }
        alep = &ale->entry.next;
    }
    return ale;
}

#define ATOM_LIST_HASH_THRESHOLD        12

JSAtomListElement *
JSAtomList::add(Parser *parser, JSAtom *atom, AddHow how)
{
    JS_ASSERT(!set);

    JSAtomListElement *ale, *ale2, *next;
    JSHashEntry **hep;

    ale = rawLookup(atom, hep);
    if (!ale || how != UNIQUE) {
        if (count < ATOM_LIST_HASH_THRESHOLD && !table) {
            
            ale = (JSAtomListElement *)js_alloc_temp_entry(parser, atom);
            if (!ale)
                return NULL;
            ALE_SET_ATOM(ale, atom);

            if (how == HOIST) {
                ale->entry.next = NULL;
                hep = (JSHashEntry **) &list;
                while (*hep)
                    hep = &(*hep)->next;
                *hep = &ale->entry;
            } else {
                ale->entry.next = list;
                list = &ale->entry;
            }
        } else {
            




            if (!table) {
                
                JS_ASSERT(!hep);
                table = JS_NewHashTable(count + 1, js_hash_atom_ptr,
                                        JS_CompareValues, JS_CompareValues,
                                        &temp_alloc_ops, parser);
                if (!table)
                    return NULL;

                



                table->nentries = count;

                




                for (ale2 = (JSAtomListElement *)list; ale2; ale2 = next) {
                    next = ALE_NEXT(ale2);
                    ale2->entry.keyHash = ATOM_HASH(ALE_ATOM(ale2));
                    hep = JS_HashTableRawLookup(table, ale2->entry.keyHash,
                                                ale2->entry.key);
                    while (*hep)
                        hep = &(*hep)->next;
                    *hep = &ale2->entry;
                    ale2->entry.next = NULL;
                }
                list = NULL;

                
                hep = JS_HashTableRawLookup(table, ATOM_HASH(atom), atom);
            }

            
            ale = (JSAtomListElement *)
                  JS_HashTableRawAdd(table, hep, ATOM_HASH(atom), atom, NULL);
            if (!ale)
                return NULL;

            





            if (how == HOIST && ale->entry.next) {
                JS_ASSERT(*hep == &ale->entry);
                *hep = ale->entry.next;
                ale->entry.next = NULL;
                do {
                    hep = &(*hep)->next;
                } while (*hep);
                *hep = &ale->entry;
            }
        }

        ALE_SET_INDEX(ale, count++);
    }
    return ale;
}

void
JSAtomList::rawRemove(Parser *parser, JSAtomListElement *ale, JSHashEntry **hep)
{
    JS_ASSERT(!set);
    JS_ASSERT(count != 0);

    if (table) {
        JS_ASSERT(hep);
        JS_HashTableRawRemove(table, hep, &ale->entry);
    } else {
        JS_ASSERT(!hep);
        hep = &list;
        while (*hep != &ale->entry) {
            JS_ASSERT(*hep);
            hep = &(*hep)->next;
        }
        *hep = ale->entry.next;
        js_free_temp_entry(parser, &ale->entry, HT_FREE_ENTRY);
    }

    --count;
}

JSAutoAtomList::~JSAutoAtomList()
{
    if (table) {
        JS_HashTableDestroy(table);
    } else {
        JSHashEntry *hep = list;
        while (hep) {
            JSHashEntry *next = hep->next;
            js_free_temp_entry(parser, hep, HT_FREE_ENTRY);
            hep = next;
        }
    }
}

JSAtomListElement *
JSAtomListIterator::operator ()()
{
    JSAtomListElement *ale;
    JSHashTable *ht;

    if (index == uint32(-1))
        return NULL;

    ale = next;
    if (!ale) {
        ht = list->table;
        if (!ht)
            goto done;
        do {
            if (index == JS_BIT(JS_HASH_BITS - ht->shift))
                goto done;
            next = (JSAtomListElement *) ht->buckets[index++];
        } while (!next);
        ale = next;
    }

    next = ALE_NEXT(ale);
    return ale;

  done:
    index = uint32(-1);
    return NULL;
}

static intN
js_map_atom(JSHashEntry *he, intN i, void *arg)
{
    JSAtomListElement *ale = (JSAtomListElement *)he;
    JSAtom **vector = (JSAtom **) arg;

    vector[ALE_INDEX(ale)] = ALE_ATOM(ale);
    return HT_ENUMERATE_NEXT;
}

#ifdef DEBUG
static jsrefcount js_atom_map_count;
static jsrefcount js_atom_map_hash_table_count;
#endif

void
js_InitAtomMap(JSContext *cx, JSAtomMap *map, JSAtomList *al)
{
    JSAtom **vector;
    JSAtomListElement *ale;
    uint32 count;

    
    JS_ASSERT(al->count == map->length);
#ifdef DEBUG
    JS_ATOMIC_INCREMENT(&js_atom_map_count);
#endif
    ale = (JSAtomListElement *)al->list;
    if (!ale && !al->table) {
        JS_ASSERT(!map->vector);
        return;
    }

    count = al->count;
    vector = map->vector;
    if (al->table) {
#ifdef DEBUG
        JS_ATOMIC_INCREMENT(&js_atom_map_hash_table_count);
#endif
        JS_HashTableEnumerateEntries(al->table, js_map_atom, vector);
    } else {
        do {
            vector[ALE_INDEX(ale)] = ALE_ATOM(ale);
        } while ((ale = ALE_NEXT(ale)) != NULL);
    }
    al->clear();
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

    if (!js_IsFunctionQName(cx, &idval.toObject(), idp))
        return JS_FALSE;
    if (!JSID_IS_VOID(*idp))
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

    if (!js_IsFunctionQName(cx, &idval.toObject(), idp))
        return JS_FALSE;
    if (!JSID_IS_VOID(*idp)) {
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
