






































#ifndef jsatom_h___
#define jsatom_h___

#include <stddef.h>
#include "jsversion.h"
#include "jsalloc.h"
#include "jsapi.h"
#include "jsprvtd.h"
#include "jshash.h"
#include "jspubtd.h"
#include "jslock.h"

#include "js/HashTable.h"
#include "vm/String.h"



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

extern jsid
js_CheckForStringIndex(jsid id);

JS_STATIC_ASSERT(sizeof(JSHashNumber) == 4);
JS_STATIC_ASSERT(sizeof(jsid) == JS_BYTES_PER_WORD);

namespace js {

static JS_ALWAYS_INLINE JSHashNumber
HashId(jsid id)
{
    JS_ASSERT(js_CheckForStringIndex(id) == id);
    JSHashNumber n =
#if JS_BYTES_PER_WORD == 4
        JSHashNumber(JSID_BITS(id));
#elif JS_BYTES_PER_WORD == 8
        JSHashNumber(JSID_BITS(id)) ^ JSHashNumber(JSID_BITS(id) >> 32);
#else
# error "Unsupported configuration"
#endif
    return n * JS_GOLDEN_RATIO;
}

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
    return IdToValue(id);
}

template<>
struct DefaultHasher<jsid>
{
    typedef jsid Lookup;
    static HashNumber hash(const Lookup &l) {
        JS_ASSERT(l == js_CheckForStringIndex(l));
        return HashNumber(JSID_BITS(l));
    }
    static bool match(const jsid &id, const Lookup &l) {
        JS_ASSERT(l == js_CheckForStringIndex(l));
        return id == l;
    }
};

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

namespace js {


inline uint32
HashChars(const jschar *chars, size_t length)
{
    uint32 h = 0;
    for (; length; chars++, length--)
        h = JS_ROTATE_LEFT32(h, 4) ^ *chars;
    return h;
}

typedef TaggedPointerEntry<JSAtom> AtomStateEntry;

struct AtomHasher
{
    struct Lookup
    {
        const jschar    *chars;
        size_t          length;
        const JSAtom    *atom; 

        Lookup(const jschar *chars, size_t length) : chars(chars), length(length), atom(NULL) {}
        Lookup(const JSAtom *atom) : chars(atom->chars()), length(atom->length()), atom(atom) {}
    };

    static HashNumber hash(const Lookup &l) {
        return HashChars(l.chars, l.length);
    }

    static bool match(const AtomStateEntry &entry, const Lookup &lookup) {
        JSAtom *key = entry.asPtr();

        if (lookup.atom)
            return lookup.atom == key;
        if (key->length() != lookup.length)
            return false;
        return PodEqual(key->chars(), lookup.chars, lookup.length);
    }
};

typedef HashSet<AtomStateEntry, AtomHasher, SystemAllocPolicy> AtomSet;


















enum FlationCoding
{
    NormalEncoding,
    CESU8Encoding
};

}  

struct JSAtomState
{
    js::AtomSet         atoms;

#ifdef JS_THREADSAFE
    JSThinLock          lock;
#endif

    









    
    js::PropertyName    *emptyAtom;

    



    js::PropertyName    *booleanAtoms[2];
    js::PropertyName    *typeAtoms[JSTYPE_LIMIT];
    js::PropertyName    *nullAtom;

    
    js::PropertyName    *classAtoms[JSProto_LIMIT];

    
    js::PropertyName    *anonymousAtom;
    js::PropertyName    *applyAtom;
    js::PropertyName    *argumentsAtom;
    js::PropertyName    *arityAtom;
    js::PropertyName    *BYTES_PER_ELEMENTAtom;
    js::PropertyName    *callAtom;
    js::PropertyName    *calleeAtom;
    js::PropertyName    *callerAtom;
    js::PropertyName    *classPrototypeAtom;
    js::PropertyName    *constructorAtom;
    js::PropertyName    *eachAtom;
    js::PropertyName    *evalAtom;
    js::PropertyName    *fileNameAtom;
    js::PropertyName    *getAtom;
    js::PropertyName    *globalAtom;
    js::PropertyName    *ignoreCaseAtom;
    js::PropertyName    *indexAtom;
    js::PropertyName    *inputAtom;
    js::PropertyName    *toISOStringAtom;
    js::PropertyName    *iteratorAtom;
    js::PropertyName    *joinAtom;
    js::PropertyName    *lastIndexAtom;
    js::PropertyName    *lengthAtom;
    js::PropertyName    *lineNumberAtom;
    js::PropertyName    *messageAtom;
    js::PropertyName    *multilineAtom;
    js::PropertyName    *nameAtom;
    js::PropertyName    *nextAtom;
    js::PropertyName    *noSuchMethodAtom;
    js::PropertyName    *objectNullAtom;
    js::PropertyName    *objectUndefinedAtom;
    js::PropertyName    *protoAtom;
    js::PropertyName    *setAtom;
    js::PropertyName    *sourceAtom;
    js::PropertyName    *stackAtom;
    js::PropertyName    *stickyAtom;
    js::PropertyName    *toGMTStringAtom;
    js::PropertyName    *toLocaleStringAtom;
    js::PropertyName    *toSourceAtom;
    js::PropertyName    *toStringAtom;
    js::PropertyName    *toUTCStringAtom;
    js::PropertyName    *valueOfAtom;
    js::PropertyName    *toJSONAtom;
    js::PropertyName    *void0Atom;
    js::PropertyName    *enumerableAtom;
    js::PropertyName    *configurableAtom;
    js::PropertyName    *writableAtom;
    js::PropertyName    *valueAtom;
    js::PropertyName    *testAtom;
    js::PropertyName    *useStrictAtom;
    js::PropertyName    *locAtom;
    js::PropertyName    *lineAtom;
    js::PropertyName    *InfinityAtom;
    js::PropertyName    *NaNAtom;
    js::PropertyName    *builderAtom;

#if JS_HAS_XML_SUPPORT
    js::PropertyName    *etagoAtom;
    js::PropertyName    *namespaceAtom;
    js::PropertyName    *ptagcAtom;
    js::PropertyName    *qualifierAtom;
    js::PropertyName    *spaceAtom;
    js::PropertyName    *stagoAtom;
    js::PropertyName    *starAtom;
    js::PropertyName    *starQualifierAtom;
    js::PropertyName    *tagcAtom;
    js::PropertyName    *xmlAtom;

    
    js::PropertyName    *functionNamespaceURIAtom;
#endif

    js::PropertyName    *ProxyAtom;

    js::PropertyName    *getOwnPropertyDescriptorAtom;
    js::PropertyName    *getPropertyDescriptorAtom;
    js::PropertyName    *definePropertyAtom;
    js::PropertyName    *deleteAtom;
    js::PropertyName    *getOwnPropertyNamesAtom;
    js::PropertyName    *enumerateAtom;
    js::PropertyName    *fixAtom;

    js::PropertyName    *hasAtom;
    js::PropertyName    *hasOwnAtom;
    js::PropertyName    *keysAtom;
    js::PropertyName    *iterateAtom;

    js::PropertyName    *WeakMapAtom;

    js::PropertyName    *byteLengthAtom;

    js::PropertyName    *returnAtom;
    js::PropertyName    *throwAtom;

    
    struct {
        js::PropertyName *XMLListAtom;
        js::PropertyName *decodeURIAtom;
        js::PropertyName *decodeURIComponentAtom;
        js::PropertyName *defineGetterAtom;
        js::PropertyName *defineSetterAtom;
        js::PropertyName *encodeURIAtom;
        js::PropertyName *encodeURIComponentAtom;
        js::PropertyName *escapeAtom;
        js::PropertyName *hasOwnPropertyAtom;
        js::PropertyName *isFiniteAtom;
        js::PropertyName *isNaNAtom;
        js::PropertyName *isPrototypeOfAtom;
        js::PropertyName *isXMLNameAtom;
        js::PropertyName *lookupGetterAtom;
        js::PropertyName *lookupSetterAtom;
        js::PropertyName *parseFloatAtom;
        js::PropertyName *parseIntAtom;
        js::PropertyName *propertyIsEnumerableAtom;
        js::PropertyName *unescapeAtom;
        js::PropertyName *unevalAtom;
        js::PropertyName *unwatchAtom;
        js::PropertyName *watchAtom;
    } lazy;

    static const size_t commonAtomsOffset;
    static const size_t lazyAtomsOffset;

    void clearLazyAtoms() {
        memset(&lazy, 0, sizeof(lazy));
    }

    void junkAtoms() {
#ifdef DEBUG
        memset(commonAtomsStart(), JS_FREE_PATTERN, sizeof(*this) - commonAtomsOffset);
#endif
    }

    JSAtom **commonAtomsStart() {
        return reinterpret_cast<JSAtom **>(&emptyAtom);
    }

    void checkStaticInvariants();
};

extern bool
AtomIsInterned(JSContext *cx, JSAtom *atom);

#define ATOM(name) cx->runtime->atomState.name##Atom

#define COMMON_ATOM_INDEX(name)                                               \
    ((offsetof(JSAtomState, name##Atom) - JSAtomState::commonAtomsOffset)     \
     / sizeof(JSAtom*))
#define COMMON_TYPE_ATOM_INDEX(type)                                          \
    ((offsetof(JSAtomState, typeAtoms[type]) - JSAtomState::commonAtomsOffset)\
     / sizeof(JSAtom*))

#define ATOM_OFFSET(name)       offsetof(JSAtomState, name##Atom)
#define OFFSET_TO_ATOM(rt,off)  (*(JSAtom **)((char*)&(rt)->atomState + (off)))
#define CLASS_ATOM_OFFSET(name) offsetof(JSAtomState, classAtoms[JSProto_##name])
#define CLASS_ATOM(cx,name)     ((cx)->runtime->atomState.classAtoms[JSProto_##name])

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
extern const char   js_BYTES_PER_ELEMENT_str[];
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

extern bool
js_InitCommonAtoms(JSContext *cx);

extern void
js_FinishCommonAtoms(JSContext *cx);

extern JSAtom *
js_Atomize(JSContext *cx, const char *bytes, size_t length,
           js::InternBehavior ib = js::DoNotInternAtom,
           js::FlationCoding fc = js::NormalEncoding);

extern JSAtom *
js_AtomizeChars(JSContext *cx, const jschar *chars, size_t length,
                js::InternBehavior ib = js::DoNotInternAtom);





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
js_InitAtomMap(JSContext *cx, js::AtomIndexMap *indices, JSAtom **atoms);

#endif 
