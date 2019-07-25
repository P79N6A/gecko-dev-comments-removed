






































#ifndef jsatom_h___
#define jsatom_h___



#include <stddef.h>
#include "jsversion.h"
#include "jsapi.h"
#include "jsprvtd.h"
#include "jshash.h"
#include "jshashtable.h"
#include "jsnum.h"
#include "jspubtd.h"
#include "jsstr.h"
#include "jslock.h"
#include "jsvalue.h"

#define ATOM_PINNED     0x1       /* atom is pinned against GC */
#define ATOM_INTERNED   0x2       /* pinned variant for JS_Intern* API */
#define ATOM_NOCOPY     0x4       /* don't copy atom string bytes */



static JS_ALWAYS_INLINE jsid
JSID_FROM_BITS(size_t bits)
{
    jsid id;
    JSID_BITS(id) = bits;
    return id;
}

static JS_ALWAYS_INLINE jsid
ATOM_TO_JSID(JSAtom *atom)
{
    JS_ASSERT(((size_t)atom & 0x7) == 0);
    return JSID_FROM_BITS((size_t)atom);
}


static JS_ALWAYS_INLINE JSBool
JSID_IS_ATOM(jsid id)
{
    return JSID_IS_STRING(id);
}

static JS_ALWAYS_INLINE JSBool
JSID_IS_ATOM(jsid id, JSAtom *atom)
{
    return JSID_BITS(id) == JSID_BITS(ATOM_TO_JSID(atom));
}

static JS_ALWAYS_INLINE JSAtom *
JSID_TO_ATOM(jsid id)
{
    return (JSAtom *)JSID_TO_STRING(id);
}

namespace js {

static JS_ALWAYS_INLINE Value
IdToValue(jsid id)
{
    if (JSID_IS_STRING(id))
        return StringValue(JSID_TO_STRING(id));
    if (JS_LIKELY(JSID_IS_INT(id)))
        return Int32Value(JSID_TO_INT(id));
    if (JS_LIKELY(JSID_IS_OBJECT(id)))
        return ObjectValue(*JSID_TO_OBJECT(id));
    JS_ASSERT(JSID_IS_DEFAULT_XML_NAMESPACE(id) || JSID_IS_VOID(id));
    return UndefinedValue();
}

static JS_ALWAYS_INLINE jsval
IdToJsval(jsid id)
{
    return Jsvalify(IdToValue(id));
}

static JS_ALWAYS_INLINE JSString *
IdToString(JSContext *cx, jsid id)
{
    if (JSID_IS_STRING(id))
        return JSID_TO_STRING(id);
    if (JS_LIKELY(JSID_IS_INT(id)))
        return js_IntToString(cx, JSID_TO_INT(id));
    return js_ValueToString(cx, IdToValue(id));
}

}

#if JS_BYTES_PER_WORD == 4
# define ATOM_HASH(atom)          ((JSHashNumber)(atom) >> 2)
#elif JS_BYTES_PER_WORD == 8
# define ATOM_HASH(atom)          (((JSHashNumber)(jsuword)(atom) >> 3) ^     \
                                   (JSHashNumber)((jsuword)(atom) >> 32))
#else
# error "Unsupported configuration"
#endif





extern const char *
js_AtomToPrintableString(JSContext *cx, JSAtom *atom, JSAutoByteString *bytes);

struct JSAtomListElement {
    JSHashEntry         entry;
};

#define ALE_ATOM(ale)   ((JSAtom *) (ale)->entry.key)
#define ALE_INDEX(ale)  (jsatomid(uintptr_t((ale)->entry.value)))
#define ALE_VALUE(ale)  ((jsboxedword) (ale)->entry.value)
#define ALE_NEXT(ale)   ((JSAtomListElement *) (ale)->entry.next)






#define ALE_DEFN(ale)   ((JSDefinition *) (ale)->entry.value)

#define ALE_SET_ATOM(ale,atom)  ((ale)->entry.key = (const void *)(atom))
#define ALE_SET_INDEX(ale,index)((ale)->entry.value = (void *)(index))
#define ALE_SET_DEFN(ale, dn)   ((ale)->entry.value = (void *)(dn))
#define ALE_SET_VALUE(ale, v)   ((ale)->entry.value = (void *)(v))
#define ALE_SET_NEXT(ale,nxt)   ((ale)->entry.next = (JSHashEntry *)(nxt))










struct JSAtomSet {
    JSHashEntry         *list;          
    JSHashTable         *table;         
    jsuint              count;          
};

struct JSAtomList : public JSAtomSet
{
#ifdef DEBUG
    const JSAtomSet* set;               
#endif

    JSAtomList() {
        list = NULL; table = NULL; count = 0;
#ifdef DEBUG
        set = NULL;
#endif
    }

    JSAtomList(const JSAtomSet& as) {
        list = as.list; table = as.table; count = as.count;
#ifdef DEBUG
        set = &as;
#endif
    }

    void clear() { JS_ASSERT(!set); list = NULL; table = NULL; count = 0; }

    JSAtomListElement *lookup(JSAtom *atom) {
        JSHashEntry **hep;
        return rawLookup(atom, hep);
    }

    JSAtomListElement *rawLookup(JSAtom *atom, JSHashEntry **&hep);

    enum AddHow { UNIQUE, SHADOW, HOIST };

    JSAtomListElement *add(js::Parser *parser, JSAtom *atom, AddHow how = UNIQUE);

    void remove(js::Parser *parser, JSAtom *atom) {
        JSHashEntry **hep;
        JSAtomListElement *ale = rawLookup(atom, hep);
        if (ale)
            rawRemove(parser, ale, hep);
    }

    void rawRemove(js::Parser *parser, JSAtomListElement *ale, JSHashEntry **hep);
};





struct JSAutoAtomList: public JSAtomList
{
    JSAutoAtomList(js::Parser *p): parser(p) {}
    ~JSAutoAtomList();
  private:
    js::Parser *parser;         
};







class JSAtomListIterator {
    JSAtomList*         list;
    JSAtomListElement*  next;
    uint32              index;

  public:
    JSAtomListIterator(JSAtomList* al) : list(al) { reset(); }

    void reset() {
        next = (JSAtomListElement *) list->list;
        index = 0;
    }

    JSAtomListElement* operator ()();
};

struct JSAtomMap {
    JSAtom              **vector;       
    jsatomid            length;         
};

namespace js {

#define ATOM_ENTRY_FLAG_MASK            ((size_t)(ATOM_PINNED | ATOM_INTERNED))

JS_STATIC_ASSERT(ATOM_ENTRY_FLAG_MASK < JS_GCTHING_ALIGN);

typedef uintptr_t AtomEntryType;

static JS_ALWAYS_INLINE JSAtom *
AtomEntryToKey(AtomEntryType entry)
{
    JS_ASSERT(entry != 0);
    return (JSAtom *)(entry & ~ATOM_ENTRY_FLAG_MASK);
}

struct AtomHasher
{
    struct Lookup
    {
        const jschar *chars;
        size_t length;
        Lookup(const jschar *chars, size_t length) : chars(chars), length(length) {}
    };

    static HashNumber hash(const Lookup &l) {
        return HashChars(l.chars, l.length);
    }

    static bool match(AtomEntryType entry, const Lookup &lookup) {
        JS_ASSERT(entry);
        JSAtom *key = AtomEntryToKey(entry);
        if (key->length() != lookup.length)
            return false;
        return PodEqual(key->chars(), lookup.chars, lookup.length);
    }
};

typedef HashSet<AtomEntryType, AtomHasher, SystemAllocPolicy> AtomSet;

}  

struct JSAtomState
{
    js::AtomSet         atoms;

#ifdef JS_THREADSAFE
    JSThinLock          lock;
#endif

    









    
    JSAtom              *emptyAtom;

    



    JSAtom              *booleanAtoms[2];
    JSAtom              *typeAtoms[JSTYPE_LIMIT];
    JSAtom              *nullAtom;

    
    JSAtom              *classAtoms[JSProto_LIMIT];

    
    JSAtom              *anonymousAtom;
    JSAtom              *applyAtom;
    JSAtom              *argumentsAtom;
    JSAtom              *arityAtom;
    JSAtom              *callAtom;
    JSAtom              *calleeAtom;
    JSAtom              *callerAtom;
    JSAtom              *classPrototypeAtom;
    JSAtom              *constructorAtom;
    JSAtom              *eachAtom;
    JSAtom              *evalAtom;
    JSAtom              *fileNameAtom;
    JSAtom              *getAtom;
    JSAtom              *globalAtom;
    JSAtom              *ignoreCaseAtom;
    JSAtom              *indexAtom;
    JSAtom              *inputAtom;
    JSAtom              *toISOStringAtom;
    JSAtom              *iteratorAtom;
    JSAtom              *joinAtom;
    JSAtom              *lastIndexAtom;
    JSAtom              *lengthAtom;
    JSAtom              *lineNumberAtom;
    JSAtom              *messageAtom;
    JSAtom              *multilineAtom;
    JSAtom              *nameAtom;
    JSAtom              *nextAtom;
    JSAtom              *noSuchMethodAtom;
    JSAtom              *objectNullAtom;
    JSAtom              *objectUndefinedAtom;
    JSAtom              *protoAtom;
    JSAtom              *setAtom;
    JSAtom              *sourceAtom;
    JSAtom              *stackAtom;
    JSAtom              *stickyAtom;
    JSAtom              *toGMTStringAtom;
    JSAtom              *toLocaleStringAtom;
    JSAtom              *toSourceAtom;
    JSAtom              *toStringAtom;
    JSAtom              *toUTCStringAtom;
    JSAtom              *valueOfAtom;
    JSAtom              *toJSONAtom;
    JSAtom              *void0Atom;
    JSAtom              *enumerableAtom;
    JSAtom              *configurableAtom;
    JSAtom              *writableAtom;
    JSAtom              *valueAtom;
    JSAtom              *testAtom;
    JSAtom              *useStrictAtom;
    JSAtom              *locAtom;
    JSAtom              *lineAtom;
    JSAtom              *InfinityAtom;
    JSAtom              *NaNAtom;
    JSAtom              *builderAtom;

#if JS_HAS_XML_SUPPORT
    JSAtom              *etagoAtom;
    JSAtom              *namespaceAtom;
    JSAtom              *ptagcAtom;
    JSAtom              *qualifierAtom;
    JSAtom              *spaceAtom;
    JSAtom              *stagoAtom;
    JSAtom              *starAtom;
    JSAtom              *starQualifierAtom;
    JSAtom              *tagcAtom;
    JSAtom              *xmlAtom;

    
    JSAtom              *functionNamespaceURIAtom;
#endif

    JSAtom              *ProxyAtom;

    JSAtom              *getOwnPropertyDescriptorAtom;
    JSAtom              *getPropertyDescriptorAtom;
    JSAtom              *definePropertyAtom;
    JSAtom              *deleteAtom;
    JSAtom              *getOwnPropertyNamesAtom;
    JSAtom              *enumerateAtom;
    JSAtom              *fixAtom;

    JSAtom              *hasAtom;
    JSAtom              *hasOwnAtom;
    JSAtom              *keysAtom;
    JSAtom              *iterateAtom;

    JSAtom              *WeakMapAtom;

    
    struct {
        JSAtom          *XMLListAtom;
        JSAtom          *decodeURIAtom;
        JSAtom          *decodeURIComponentAtom;
        JSAtom          *defineGetterAtom;
        JSAtom          *defineSetterAtom;
        JSAtom          *encodeURIAtom;
        JSAtom          *encodeURIComponentAtom;
        JSAtom          *escapeAtom;
        JSAtom          *hasOwnPropertyAtom;
        JSAtom          *isFiniteAtom;
        JSAtom          *isNaNAtom;
        JSAtom          *isPrototypeOfAtom;
        JSAtom          *isXMLNameAtom;
        JSAtom          *lookupGetterAtom;
        JSAtom          *lookupSetterAtom;
        JSAtom          *parseFloatAtom;
        JSAtom          *parseIntAtom;
        JSAtom          *propertyIsEnumerableAtom;
        JSAtom          *unescapeAtom;
        JSAtom          *unevalAtom;
        JSAtom          *unwatchAtom;
        JSAtom          *watchAtom;
    } lazy;
};

#define ATOM(name) cx->runtime->atomState.name##Atom

#define ATOM_OFFSET_START       offsetof(JSAtomState, emptyAtom)
#define LAZY_ATOM_OFFSET_START  offsetof(JSAtomState, lazy)
#define ATOM_OFFSET_LIMIT       (sizeof(JSAtomState))

#define COMMON_ATOMS_START(state)                                             \
    ((JSAtom **)((uint8 *)(state) + ATOM_OFFSET_START))
#define COMMON_ATOM_INDEX(name)                                               \
    ((offsetof(JSAtomState, name##Atom) - ATOM_OFFSET_START)                  \
     / sizeof(JSAtom*))
#define COMMON_TYPE_ATOM_INDEX(type)                                          \
    ((offsetof(JSAtomState, typeAtoms[type]) - ATOM_OFFSET_START)             \
     / sizeof(JSAtom*))

#define ATOM_OFFSET(name)       offsetof(JSAtomState, name##Atom)
#define OFFSET_TO_ATOM(rt,off)  (*(JSAtom **)((char*)&(rt)->atomState + (off)))
#define CLASS_ATOM_OFFSET(name) offsetof(JSAtomState,classAtoms[JSProto_##name])

#define CLASS_ATOM(cx,name) \
    ((cx)->runtime->atomState.classAtoms[JSProto_##name])

extern const char *const js_common_atom_names[];
extern const size_t      js_common_atom_count;




#define JS_BOOLEAN_STR(type) (js_common_atom_names[1 + (type)])
#define JS_TYPE_STR(type)    (js_common_atom_names[1 + 2 + (type)])


#define JS_PROTO(name,code,init) extern const char js_##name##_str[];
#include "jsproto.tbl"
#undef JS_PROTO

extern const char   js_anonymous_str[];
extern const char   js_apply_str[];
extern const char   js_arguments_str[];
extern const char   js_arity_str[];
extern const char   js_call_str[];
extern const char   js_callee_str[];
extern const char   js_caller_str[];
extern const char   js_class_prototype_str[];
extern const char   js_close_str[];
extern const char   js_constructor_str[];
extern const char   js_count_str[];
extern const char   js_etago_str[];
extern const char   js_each_str[];
extern const char   js_eval_str[];
extern const char   js_fileName_str[];
extern const char   js_get_str[];
extern const char   js_getter_str[];
extern const char   js_global_str[];
extern const char   js_ignoreCase_str[];
extern const char   js_index_str[];
extern const char   js_input_str[];
extern const char   js_iterator_str[];
extern const char   js_join_str[];
extern const char   js_lastIndex_str[];
extern const char   js_length_str[];
extern const char   js_lineNumber_str[];
extern const char   js_message_str[];
extern const char   js_multiline_str[];
extern const char   js_name_str[];
extern const char   js_namespace_str[];
extern const char   js_next_str[];
extern const char   js_noSuchMethod_str[];
extern const char   js_object_str[];
extern const char   js_proto_str[];
extern const char   js_ptagc_str[];
extern const char   js_qualifier_str[];
extern const char   js_send_str[];
extern const char   js_setter_str[];
extern const char   js_set_str[];
extern const char   js_source_str[];
extern const char   js_space_str[];
extern const char   js_stack_str[];
extern const char   js_sticky_str[];
extern const char   js_stago_str[];
extern const char   js_star_str[];
extern const char   js_starQualifier_str[];
extern const char   js_tagc_str[];
extern const char   js_toGMTString_str[];
extern const char   js_toLocaleString_str[];
extern const char   js_toSource_str[];
extern const char   js_toString_str[];
extern const char   js_toUTCString_str[];
extern const char   js_undefined_str[];
extern const char   js_valueOf_str[];
extern const char   js_toJSON_str[];
extern const char   js_xml_str[];
extern const char   js_enumerable_str[];
extern const char   js_configurable_str[];
extern const char   js_writable_str[];
extern const char   js_value_str[];
extern const char   js_test_str[];






extern JSBool
js_InitAtomState(JSRuntime *rt);





extern void
js_FinishAtomState(JSRuntime *rt);





extern void
js_TraceAtomState(JSTracer *trc);

extern void
js_SweepAtomState(JSContext *cx);

extern JSBool
js_InitCommonAtoms(JSContext *cx);

extern void
js_FinishCommonAtoms(JSContext *cx);





extern JSAtom *
js_AtomizeString(JSContext *cx, JSString *str, uintN flags);

extern JSAtom *
js_Atomize(JSContext *cx, const char *bytes, size_t length, uintN flags, bool useCESU8 = false);

extern JSAtom *
js_AtomizeChars(JSContext *cx, const jschar *chars, size_t length, uintN flags);





extern JSAtom *
js_GetExistingStringAtom(JSContext *cx, const jschar *chars, size_t length);

#ifdef DEBUG

extern JS_FRIEND_API(void)
js_DumpAtoms(JSContext *cx, FILE *fp);

#endif

inline bool
js_ValueToAtom(JSContext *cx, const js::Value &v, JSAtom **atomp);

inline bool
js_ValueToStringId(JSContext *cx, const js::Value &v, jsid *idp);

inline bool
js_InternNonIntElementId(JSContext *cx, JSObject *obj, const js::Value &idval,
                         jsid *idp);
inline bool
js_InternNonIntElementId(JSContext *cx, JSObject *obj, const js::Value &idval,
                         jsid *idp, js::Value *vp);





extern void
js_InitAtomMap(JSContext *cx, JSAtomMap *map, JSAtomList *al);

#endif 
