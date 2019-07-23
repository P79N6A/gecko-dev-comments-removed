









































#include "jsstddef.h"
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsutil.h" 
#include "jshash.h" 
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsscan.h"
#include "jsstr.h"

JS_FRIEND_API(const char *)
js_AtomToPrintableString(JSContext *cx, JSAtom *atom)
{
    return js_ValueToPrintableString(cx, ATOM_KEY(atom));
}

#define JS_PROTO(name,code,init) const char js_##name##_str[] = #name;
#include "jsproto.tbl"
#undef JS_PROTO










JS_STATIC_ASSERT(JSTYPE_LIMIT == 8);
const char *const js_common_atom_names[] = {
    "",                         
    js_undefined_str,           
    js_object_str,              
    js_function_str,            
    "string",                   
    "number",                   
    "boolean",                  
    js_null_str,                
    "xml",                      
    js_false_str,               
    js_true_str,                
    js_null_str,                

#define JS_PROTO(name,code,init) js_##name##_str,
#include "jsproto.tbl"
#undef JS_PROTO

    js_anonymous_str,           
    js_arguments_str,           
    js_arity_str,               
    js_callee_str,              
    js_caller_str,              
    js_class_prototype_str,     
    js_constructor_str,         
    js_count_str,               
    js_each_str,                
    js_eval_str,                
    js_fileName_str,            
    js_get_str,                 
    js_getter_str,              
    js_index_str,               
    js_input_str,               
    js_iterator_str,            
    js_length_str,              
    js_lineNumber_str,          
    js_message_str,             
    js_name_str,                
    js_next_str,                
    js_noSuchMethod_str,        
    js_parent_str,              
    js_proto_str,               
    js_set_str,                 
    js_setter_str,              
    js_stack_str,               
    js_toLocaleString_str,      
    js_toSource_str,            
    js_toString_str,            
    js_valueOf_str,             
    "(void 0)",                 

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
#endif

#ifdef NARCISSUS
    js_call_str,                
    js_construct_str,           
    js_hasInstance_str,         
    js_ExecutionContext_str,    
    js_current_str,             
#endif
};
JS_STATIC_ASSERT(JS_ARRAY_LENGTH(js_common_atom_names) * sizeof(JSAtom *) ==
                 LAZY_ATOM_OFFSET_START - ATOM_OFFSET_START);

const char js_anonymous_str[]       = "anonymous";
const char js_arguments_str[]       = "arguments";
const char js_arity_str[]           = "arity";
const char js_callee_str[]          = "callee";
const char js_caller_str[]          = "caller";
const char js_class_prototype_str[] = "prototype";
const char js_constructor_str[]     = "constructor";
const char js_count_str[]           = "__count__";
const char js_each_str[]            = "each";
const char js_eval_str[]            = "eval";
const char js_fileName_str[]        = "fileName";
const char js_get_str[]             = "get";
const char js_getter_str[]          = "getter";
const char js_index_str[]           = "index";
const char js_input_str[]           = "input";
const char js_iterator_str[]        = "__iterator__";
const char js_length_str[]          = "length";
const char js_lineNumber_str[]      = "lineNumber";
const char js_message_str[]         = "message";
const char js_name_str[]            = "name";
const char js_next_str[]            = "next";
const char js_noSuchMethod_str[]    = "__noSuchMethod__";
const char js_object_str[]          = "object";
const char js_parent_str[]          = "__parent__";
const char js_proto_str[]           = "__proto__";
const char js_setter_str[]          = "setter";
const char js_set_str[]             = "set";
const char js_stack_str[]           = "stack";
const char js_toSource_str[]        = "toSource";
const char js_toString_str[]        = "toString";
const char js_toLocaleString_str[]  = "toLocaleString";
const char js_undefined_str[]       = "undefined";
const char js_valueOf_str[]         = "valueOf";

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

#ifdef NARCISSUS
const char js_call_str[]             = "__call__";
const char js_construct_str[]        = "__construct__";
const char js_hasInstance_str[]      = "__hasInstance__";
const char js_ExecutionContext_str[] = "ExecutionContext";
const char js_current_str[]          = "current";
#endif






typedef struct JSAtomHashEntry {
    JSDHashEntryHdr hdr;
    jsuword         keyAndFlags;
} JSAtomHashEntry;

#define ATOM_ENTRY_FLAG_MASK            (ATOM_PINNED | ATOM_INTERNED)

JS_STATIC_ASSERT(ATOM_ENTRY_FLAG_MASK < JSVAL_ALIGN);




#define TO_ATOM_ENTRY(hdr)              ((JSAtomHashEntry *) hdr)
#define ATOM_ENTRY_KEY(entry)                                                 \
    ((void *)((entry)->keyAndFlags & ~ATOM_ENTRY_FLAG_MASK))
#define ATOM_ENTRY_FLAGS(entry)                                               \
    ((uintN)((entry)->keyAndFlags & ATOM_ENTRY_FLAG_MASK))
#define INIT_ATOM_ENTRY(entry, key)                                           \
    ((void)((entry)->keyAndFlags = (jsuword)(key)))
#define ADD_ATOM_ENTRY_FLAGS(entry, flags)                                    \
    ((void)((entry)->keyAndFlags |= (jsuword)(flags)))
#define CLEAR_ATOM_ENTRY_FLAGS(entry, flags)                                  \
    ((void)((entry)->keyAndFlags &= ~(jsuword)(flags)))

static const JSDHashTableOps DoubleHashOps;
static const JSDHashTableOps StringHashOps;

#define IS_DOUBLE_TABLE(table)      ((table)->ops == &DoubleHashOps)
#define IS_STRING_TABLE(table)      ((table)->ops == &StringHashOps)

#define IS_INITIALIZED_STATE(state) IS_DOUBLE_TABLE(&(state)->doubleAtoms)

JS_STATIC_DLL_CALLBACK(JSDHashNumber)
HashDouble(JSDHashTable *table, const void *key)
{
    jsdouble d;

    JS_ASSERT(IS_DOUBLE_TABLE(table));
    d = *(jsdouble *)key;
    return JSDOUBLE_HI32(d) ^ JSDOUBLE_LO32(d);
}

JS_STATIC_DLL_CALLBACK(JSDHashNumber)
HashString(JSDHashTable *table, const void *key)
{
    JS_ASSERT(IS_STRING_TABLE(table));
    return js_HashString((JSString *)key);
}

JS_STATIC_DLL_CALLBACK(JSBool)
MatchDouble(JSDHashTable *table, const JSDHashEntryHdr *hdr, const void *key)
{
    JSAtomHashEntry *entry = TO_ATOM_ENTRY(hdr);
    jsdouble d1, d2;

    JS_ASSERT(IS_DOUBLE_TABLE(table));
    if (entry->keyAndFlags == 0) {
        
        return JS_FALSE;
    }

    d1 = *(jsdouble *)ATOM_ENTRY_KEY(entry);
    d2 = *(jsdouble *)key;
    if (JSDOUBLE_IS_NaN(d1))
        return JSDOUBLE_IS_NaN(d2);
#if defined(XP_WIN)
    
    if (JSDOUBLE_IS_NaN(d2))
        return JS_FALSE;
#endif
    return d1 == d2;
}

JS_STATIC_DLL_CALLBACK(JSBool)
MatchString(JSDHashTable *table, const JSDHashEntryHdr *hdr, const void *key)
{
    JSAtomHashEntry *entry = TO_ATOM_ENTRY(hdr);

    JS_ASSERT(IS_STRING_TABLE(table));
    if (entry->keyAndFlags == 0) {
        











        return JS_FALSE;
    }
    return js_EqualStrings((JSString *)ATOM_ENTRY_KEY(entry), (JSString *)key);
}

static const JSDHashTableOps DoubleHashOps = {
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    HashDouble,
    MatchDouble,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub,
    NULL
};

static const JSDHashTableOps StringHashOps = {
    JS_DHashAllocTable,
    JS_DHashFreeTable,
    HashString,
    MatchString,
    JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub,
    JS_DHashFinalizeStub,
    NULL
};







#define JS_STRING_HASH_COUNT   1024
#define JS_DOUBLE_HASH_COUNT   64

JSBool
js_InitAtomState(JSRuntime *rt)
{
    JSAtomState *state = &rt->atomState;

   


    JS_ASSERT(!state->stringAtoms.ops);
    JS_ASSERT(!state->doubleAtoms.ops);
    JS_ASSERT(state->tablegen == 0);

    if (!JS_DHashTableInit(&state->stringAtoms, &StringHashOps,
                           NULL, sizeof(JSAtomHashEntry),
                           JS_DHASH_DEFAULT_CAPACITY(JS_STRING_HASH_COUNT))) {
        state->stringAtoms.ops = NULL;
        return JS_FALSE;
    }
    JS_ASSERT(IS_STRING_TABLE(&state->stringAtoms));

    if (!JS_DHashTableInit(&state->doubleAtoms, &DoubleHashOps,
                           NULL, sizeof(JSAtomHashEntry),
                           JS_DHASH_DEFAULT_CAPACITY(JS_DOUBLE_HASH_COUNT))) {
        state->doubleAtoms.ops = NULL;
        JS_DHashTableFinish(&state->stringAtoms);
        state->stringAtoms.ops = NULL;
        return JS_FALSE;
    }
    JS_ASSERT(IS_DOUBLE_TABLE(&state->doubleAtoms));

#ifdef JS_THREADSAFE
    js_InitLock(&state->lock);
#endif
    JS_ASSERT(IS_INITIALIZED_STATE(state));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
js_string_uninterner(JSDHashTable *table, JSDHashEntryHdr *hdr,
                     uint32 number, void *arg)
{
    JSAtomHashEntry *entry = TO_ATOM_ENTRY(hdr);
    JSRuntime *rt = (JSRuntime *)arg;

    



    JS_ASSERT(IS_STRING_TABLE(table));
    JS_ASSERT(entry->keyAndFlags != 0);
    js_FinalizeStringRT(rt, (JSString *)ATOM_ENTRY_KEY(entry));
    return JS_DHASH_NEXT;
}

void
js_FinishAtomState(JSRuntime *rt)
{
    JSAtomState *state = &rt->atomState;

    if (!IS_INITIALIZED_STATE(state)) {
        



        return;
    }

    JS_DHashTableEnumerate(&state->stringAtoms, js_string_uninterner, rt);
    JS_DHashTableFinish(&state->stringAtoms);
    JS_DHashTableFinish(&state->doubleAtoms);

#ifdef JS_THREADSAFE
    js_FinishLock(&state->lock);
#endif
#ifdef DEBUG
    memset(state, JS_FREE_PATTERN, sizeof *state);
#endif
}

JSBool
js_InitCommonAtoms(JSContext *cx)
{
    JSAtomState *state = &cx->runtime->atomState;
    uintN i;
    JSAtom **atoms;

    atoms = COMMON_ATOMS_START(state);
    for (i = 0; i < JS_ARRAY_LENGTH(js_common_atom_names); i++, atoms++) {
        *atoms = js_Atomize(cx, js_common_atom_names[i],
                            strlen(js_common_atom_names[i]), ATOM_PINNED);
        if (!*atoms)
            return JS_FALSE;
    }
    JS_ASSERT((uint8 *)atoms - (uint8 *)state == LAZY_ATOM_OFFSET_START);
    memset(atoms, 0, ATOM_OFFSET_LIMIT - LAZY_ATOM_OFFSET_START);

    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
js_atom_unpinner(JSDHashTable *table, JSDHashEntryHdr *hdr,
                 uint32 number, void *arg)
{
    JS_ASSERT(IS_STRING_TABLE(table));
    CLEAR_ATOM_ENTRY_FLAGS(TO_ATOM_ENTRY(hdr), ATOM_PINNED);
    return JS_DHASH_NEXT;
}

void
js_FinishCommonAtoms(JSContext *cx)
{
    JSAtomState *state = &cx->runtime->atomState;

    JS_DHashTableEnumerate(&state->stringAtoms, js_atom_unpinner, NULL);
#ifdef DEBUG
    memset(COMMON_ATOMS_START(state), JS_FREE_PATTERN,
           ATOM_OFFSET_LIMIT - ATOM_OFFSET_START);
#endif
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
js_locked_atom_tracer(JSDHashTable *table, JSDHashEntryHdr *hdr,
                      uint32 number, void *arg)
{
    JSAtomHashEntry *entry = TO_ATOM_ENTRY(hdr);
    JSTracer *trc = (JSTracer *)arg;

    if (entry->keyAndFlags == 0) {
        
        return JS_DHASH_NEXT;
    }
    JS_SET_TRACING_INDEX(trc, "locked_atom", (size_t)number);
    JS_CallTracer(trc, ATOM_ENTRY_KEY(entry),
                  IS_STRING_TABLE(table) ? JSTRACE_STRING : JSTRACE_DOUBLE);
    return JS_DHASH_NEXT;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
js_pinned_atom_tracer(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32 number, void *arg)
{
    JSAtomHashEntry *entry = TO_ATOM_ENTRY(hdr);
    JSTracer *trc = (JSTracer *)arg;
    uintN flags = ATOM_ENTRY_FLAGS(entry);

    JS_ASSERT(IS_STRING_TABLE(table));
    if (flags & (ATOM_PINNED | ATOM_INTERNED)) {
        JS_SET_TRACING_INDEX(trc,
                             flags & ATOM_PINNED
                             ? "pinned_atom"
                             : "interned_atom",
                             (size_t)number);
        JS_CallTracer(trc, ATOM_ENTRY_KEY(entry), JSTRACE_STRING);
    }
    return JS_DHASH_NEXT;
}

void
js_TraceAtomState(JSTracer *trc, JSBool allAtoms)
{
    JSAtomState *state;

    state = &trc->context->runtime->atomState;
    if (allAtoms) {
        JS_DHashTableEnumerate(&state->doubleAtoms, js_locked_atom_tracer, trc);
        JS_DHashTableEnumerate(&state->stringAtoms, js_locked_atom_tracer, trc);
    } else {
        JS_DHashTableEnumerate(&state->stringAtoms, js_pinned_atom_tracer, trc);
    }
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
js_atom_sweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
                uint32 number, void *arg)
{
    JSAtomHashEntry *entry = TO_ATOM_ENTRY(hdr);
    JSContext *cx = (JSContext *)arg;

    
    if (entry->keyAndFlags == 0)
        return JS_DHASH_REMOVE;

    if (ATOM_ENTRY_FLAGS(entry) & (ATOM_PINNED | ATOM_INTERNED)) {
        
        JS_ASSERT(!js_IsAboutToBeFinalized(cx, ATOM_ENTRY_KEY(entry)));
    } else if (js_IsAboutToBeFinalized(cx, ATOM_ENTRY_KEY(entry))) {
        
        return JS_DHASH_REMOVE;
    }
    return JS_DHASH_NEXT;
}

void
js_SweepAtomState(JSContext *cx)
{
    JSAtomState *state = &cx->runtime->atomState;

    JS_DHashTableEnumerate(&state->doubleAtoms, js_atom_sweeper, cx);
    JS_DHashTableEnumerate(&state->stringAtoms, js_atom_sweeper, cx);

    



    state->tablegen++;
}

JSAtom *
js_AtomizeDouble(JSContext *cx, jsdouble d)
{
    JSAtomState *state;
    JSDHashTable *table;
    JSAtomHashEntry *entry;
    uint32 gen;
    jsdouble *key;
    jsval v;

    state = &cx->runtime->atomState;
    table = &state->doubleAtoms;

    JS_LOCK(&state->lock, cx);
    entry = TO_ATOM_ENTRY(JS_DHashTableOperate(table, &d, JS_DHASH_ADD));
    if (!entry)
        goto failed_hash_add;
    if (entry->keyAndFlags == 0) {
        gen = ++state->tablegen;
        JS_UNLOCK(&state->lock, cx);

        key = js_NewDouble(cx, d, 0);
        if (!key)
            return NULL;

        JS_LOCK(&state->lock, cx);
        if (state->tablegen == gen) {
            JS_ASSERT(entry->keyAndFlags == 0);
        } else {
            entry = TO_ATOM_ENTRY(JS_DHashTableOperate(table, key,
                                                       JS_DHASH_ADD));
            if (!entry)
                goto failed_hash_add;
            if (entry->keyAndFlags != 0)
                goto finish;
            ++state->tablegen;
        }
        INIT_ATOM_ENTRY(entry, key);
    }

  finish:
    v = DOUBLE_TO_JSVAL((jsdouble *)ATOM_ENTRY_KEY(entry));
    cx->weakRoots.lastAtom = v;
    JS_UNLOCK(&state->lock,cx);

    return (JSAtom *)v;

  failed_hash_add:
    JS_UNLOCK(&state->lock,cx);
    JS_ReportOutOfMemory(cx);
    return NULL;
}

JSAtom *
js_AtomizeString(JSContext *cx, JSString *str, uintN flags)
{
    JSAtomState *state;
    JSDHashTable *table;
    JSAtomHashEntry *entry;
    JSString *key;
    uint32 gen;
    jsval v;

    JS_ASSERT((flags &
               ~(ATOM_PINNED | ATOM_INTERNED | ATOM_TMPSTR | ATOM_NOCOPY))
              == 0);
    state = &cx->runtime->atomState;
    table = &state->stringAtoms;

    JS_LOCK(&state->lock, cx);
    entry = TO_ATOM_ENTRY(JS_DHashTableOperate(table, str, JS_DHASH_ADD));
    if (!entry)
        goto failed_hash_add;
    if (entry->keyAndFlags == 0) {
        ++state->tablegen;
        gen = state->tablegen;
        JS_UNLOCK(&state->lock, cx);

        if (flags & ATOM_TMPSTR) {
            if (flags & ATOM_NOCOPY) {
                key = js_NewString(cx, str->u.chars, str->length);
                if (!key)
                    return NULL;

                
                str->u.chars = NULL;
            } else {
                key = js_NewStringCopyN(cx, str->u.chars, str->length);
                if (!key)
                    return NULL;
            }
        } else {
            JS_ASSERT((flags & ATOM_NOCOPY) == 0);
            if (!JS_MakeStringImmutable(cx, str))
                return NULL;
            key = str;
        }

        JS_LOCK(&state->lock, cx);
        if (state->tablegen == gen) {
            JS_ASSERT(entry->keyAndFlags == 0);
        } else {
            entry = TO_ATOM_ENTRY(JS_DHashTableOperate(table, key,
                                                       JS_DHASH_ADD));
            if (!entry)
                goto failed_hash_add;
            if (entry->keyAndFlags != 0)
                goto finish;
            ++state->tablegen;
        }
        INIT_ATOM_ENTRY(entry, key);
    }

  finish:
    ADD_ATOM_ENTRY_FLAGS(entry, flags & (ATOM_PINNED | ATOM_INTERNED));
    v = STRING_TO_JSVAL((JSString *)ATOM_ENTRY_KEY(entry));
    cx->weakRoots.lastAtom = v;
    JS_UNLOCK(&state->lock, cx);
    return (JSAtom *)v;

  failed_hash_add:
    JS_UNLOCK(&state->lock,cx);
    JS_ReportOutOfMemory(cx);
    return NULL;
}

JS_FRIEND_API(JSAtom *)
js_Atomize(JSContext *cx, const char *bytes, size_t length, uintN flags)
{
    jschar *chars;
    JSString str;
    JSAtom *atom;

    






#define ATOMIZE_BUF_MAX 32
    jschar inflated[ATOMIZE_BUF_MAX];
    size_t inflatedLength = ATOMIZE_BUF_MAX - 1;

    if (length < ATOMIZE_BUF_MAX) {
        js_InflateStringToBuffer(cx, bytes, length, inflated, &inflatedLength);
        inflated[inflatedLength] = 0;
        chars = inflated;
    } else {
        inflatedLength = length;
        chars = js_InflateString(cx, bytes, &inflatedLength);
        if (!chars)
            return NULL;
        flags |= ATOM_NOCOPY;
    }

    JSSTRING_INIT(&str, (jschar *)chars, inflatedLength);
    atom = js_AtomizeString(cx, &str, ATOM_TMPSTR | flags);
    if (chars != inflated && str.u.chars)
        JS_free(cx, chars);
    return atom;
}

JS_FRIEND_API(JSAtom *)
js_AtomizeChars(JSContext *cx, const jschar *chars, size_t length, uintN flags)
{
    JSString str;

    JSSTRING_INIT(&str, (jschar *)chars, length);
    return js_AtomizeString(cx, &str, ATOM_TMPSTR | flags);
}

JSAtom *
js_GetExistingStringAtom(JSContext *cx, const jschar *chars, size_t length)
{
    JSString str, *str2;
    JSAtomState *state;
    JSDHashEntryHdr *hdr;

    JSSTRING_INIT(&str, (jschar *)chars, length);
    state = &cx->runtime->atomState;

    JS_LOCK(&state->lock, cx);
    hdr = JS_DHashTableOperate(&state->stringAtoms, &str, JS_DHASH_LOOKUP);
    str2 = JS_DHASH_ENTRY_IS_BUSY(hdr)
           ? (JSString *)ATOM_ENTRY_KEY(TO_ATOM_ENTRY(hdr))
           : NULL;
    JS_UNLOCK(&state->lock, cx);

    return str2 ? (JSAtom *)STRING_TO_JSVAL(str2) : NULL;
}

JSBool
js_AtomizePrimitiveValue(JSContext *cx, jsval v, JSAtom **atomp)
{
    JSAtom *atom;

    if (JSVAL_IS_STRING(v)) {
        atom = js_AtomizeString(cx, JSVAL_TO_STRING(v), 0);
        if (!atom)
            return JS_FALSE;
    } else if (JSVAL_IS_DOUBLE(v)) {
        atom = js_AtomizeDouble(cx, *JSVAL_TO_DOUBLE(v));
        if (!atom)
            return JS_FALSE;
    } else {
        JS_ASSERT(JSVAL_IS_INT(v) || v == JSVAL_TRUE || v == JSVAL_FALSE ||
                  v == JSVAL_NULL || v == JSVAL_VOID);
        atom = (JSAtom *)v;
    }
    *atomp = atom;
    return JS_TRUE;
}

JSAtom *
js_ValueToStringAtom(JSContext *cx, jsval v)
{
    JSString *str;

    str = js_ValueToString(cx, v);
    if (!str)
        return NULL;
    return js_AtomizeString(cx, str, 0);
}

#ifdef DEBUG

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
atom_dumper(JSDHashTable *table, JSDHashEntryHdr *hdr,
            uint32 number, void *arg)
{
    JSAtomHashEntry *entry = TO_ATOM_ENTRY(hdr);
    FILE *fp = (FILE *)arg;
    void *key;
    uintN flags;

    fprintf(fp, "%3u %08x ", number, (uintN)entry->hdr.keyHash);
    if (entry->keyAndFlags == 0) {
        fputs("<uninitialized>", fp);
    } else {
        key = ATOM_ENTRY_KEY(entry);
        if (IS_DOUBLE_TABLE(table)) {
            fprintf(fp, "%.16g", *(jsdouble *)key);
        } else {
            JS_ASSERT(IS_STRING_TABLE(table));
            js_FileEscapedString(fp, (JSString *)key, '"');
        }
        flags = ATOM_ENTRY_FLAGS(entry);
        if (flags != 0) {
            fputs((flags & (ATOM_PINNED | ATOM_INTERNED))
                  ? " pinned | interned"
                  : (flags & ATOM_PINNED) ? " pinned" : " interned",
                  fp);
        }
    }
    putc('\n', fp);
    return JS_DHASH_NEXT;
}

JS_FRIEND_API(void)
js_DumpAtoms(JSContext *cx, FILE *fp)
{
    JSAtomState *state = &cx->runtime->atomState;

    fprintf(fp, "stringAtoms table contents:\n");
    JS_DHashTableEnumerate(&state->stringAtoms, atom_dumper, fp);
#ifdef JS_DHASHMETER
    JS_DHashTableDumpMeter(&state->stringAtoms, atom_dumper, fp);
#endif
    putc('\n', fp);

    fprintf(fp, "doubleAtoms table contents:\n");
    JS_DHashTableEnumerate(&state->doubleAtoms, atom_dumper, fp);
#ifdef JS_DHASHMETER
    JS_DHashTableDumpMeter(&state->doubleAtoms, atom_dumper, fp);
#endif
    putc('\n', fp);
}

#endif

JS_STATIC_DLL_CALLBACK(JSHashNumber)
js_hash_atom_ptr(const void *key)
{
    const JSAtom *atom = (const JSAtom *) key;
    return ATOM_HASH(atom);
}

JS_STATIC_DLL_CALLBACK(void *)
js_alloc_temp_space(void *priv, size_t size)
{
    JSContext *cx = (JSContext *) priv;
    void *space;

    JS_ARENA_ALLOCATE(space, &cx->tempPool, size);
    if (!space)
        JS_ReportOutOfMemory(cx);
    return space;
}

JS_STATIC_DLL_CALLBACK(void)
js_free_temp_space(void *priv, void *item)
{
}

JS_STATIC_DLL_CALLBACK(JSHashEntry *)
js_alloc_temp_entry(void *priv, const void *key)
{
    JSContext *cx = (JSContext *) priv;
    JSAtomListElement *ale;

    JS_ARENA_ALLOCATE_TYPE(ale, JSAtomListElement, &cx->tempPool);
    if (!ale) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }
    return &ale->entry;
}

JS_STATIC_DLL_CALLBACK(void)
js_free_temp_entry(void *priv, JSHashEntry *he, uintN flag)
{
}

static JSHashAllocOps temp_alloc_ops = {
    js_alloc_temp_space,    js_free_temp_space,
    js_alloc_temp_entry,    js_free_temp_entry
};

JSAtomListElement *
js_IndexAtom(JSContext *cx, JSAtom *atom, JSAtomList *al)
{
    JSAtomListElement *ale, *ale2, *next;
    JSHashEntry **hep;

    ATOM_LIST_LOOKUP(ale, hep, al, atom);
    if (!ale) {
        if (al->count < 10) {
            
            JS_ASSERT(!al->table);
            ale = (JSAtomListElement *)js_alloc_temp_entry(cx, atom);
            if (!ale)
                return NULL;
            ALE_SET_ATOM(ale, atom);
            ale->entry.next = al->list;
            al->list = &ale->entry;
        } else {
            
            if (!al->table) {
                
                JS_ASSERT(!hep);
                al->table = JS_NewHashTable(al->count + 1, js_hash_atom_ptr,
                                            JS_CompareValues, JS_CompareValues,
                                            &temp_alloc_ops, cx);
                if (!al->table)
                    return NULL;

                



                al->table->nentries = al->count;

                
                for (ale2 = (JSAtomListElement *)al->list; ale2; ale2 = next) {
                    next = ALE_NEXT(ale2);
                    ale2->entry.keyHash = ATOM_HASH(ALE_ATOM(ale2));
                    hep = JS_HashTableRawLookup(al->table, ale2->entry.keyHash,
                                                ale2->entry.key);
                    ale2->entry.next = *hep;
                    *hep = &ale2->entry;
                }
                al->list = NULL;

                
                hep = JS_HashTableRawLookup(al->table, ATOM_HASH(atom), atom);
            }

            
            ale = (JSAtomListElement *)
                  JS_HashTableRawAdd(al->table, hep, ATOM_HASH(atom), atom,
                                     NULL);
            if (!ale)
                return NULL;
        }

        ALE_SET_INDEX(ale, al->count++);
    }
    return ale;
}

JS_STATIC_DLL_CALLBACK(intN)
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

JS_FRIEND_API(void)
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
    ATOM_LIST_INIT(al);
}
