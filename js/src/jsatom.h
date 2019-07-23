






































#ifndef jsatom_h___
#define jsatom_h___



#include <stddef.h>
#include "jsversion.h"
#include "jstypes.h"
#include "jshash.h" 
#include "jsdhash.h"
#include "jsapi.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jslock.h"

JS_BEGIN_EXTERN_C

#define ATOM_PINNED     0x1       /* atom is pinned against GC */
#define ATOM_INTERNED   0x2       /* pinned variant for JS_Intern* API */
#define ATOM_NOCOPY     0x4       /* don't copy atom string bytes */
#define ATOM_TMPSTR     0x8       /* internal, to avoid extra string */

#define ATOM_KEY(atom)            ((jsval)(atom))
#define ATOM_IS_DOUBLE(atom)      JSVAL_IS_DOUBLE(ATOM_KEY(atom))
#define ATOM_TO_DOUBLE(atom)      JSVAL_TO_DOUBLE(ATOM_KEY(atom))
#define ATOM_IS_STRING(atom)      JSVAL_IS_STRING(ATOM_KEY(atom))
#define ATOM_TO_STRING(atom)      JSVAL_TO_STRING(ATOM_KEY(atom))

#if JS_BYTES_PER_WORD == 4
# define ATOM_HASH(atom)          ((JSHashNumber)(atom) >> 2)
#elif JS_BYTES_PER_WORD == 8
# define ATOM_HASH(atom)          (((JSHashNumber)(jsuword)(atom) >> 3) ^     \
                                   (JSHashNumber)((jsuword)(atom) >> 32))
#else
# error "Unsupported configuration"
#endif






extern const char *
js_AtomToPrintableString(JSContext *cx, JSAtom *atom);

struct JSAtomListElement {
    JSHashEntry         entry;
};

#define ALE_ATOM(ale)   ((JSAtom *) (ale)->entry.key)
#define ALE_INDEX(ale)  ((jsatomid) JS_PTR_TO_UINT32((ale)->entry.value))
#define ALE_VALUE(ale)  ((jsval) (ale)->entry.value)
#define ALE_NEXT(ale)   ((JSAtomListElement *) (ale)->entry.next)






#define ALE_DEFN(ale)   ((JSDefinition *) (ale)->entry.value)

#define ALE_SET_ATOM(ale,atom)  ((ale)->entry.key = (const void *)(atom))
#define ALE_SET_INDEX(ale,index)((ale)->entry.value = JS_UINT32_TO_PTR(index))
#define ALE_SET_DEFN(ale, dn)   ((ale)->entry.value = (void *)(dn))
#define ALE_SET_VALUE(ale, v)   ((ale)->entry.value = (void *)(v))
#define ALE_SET_NEXT(ale,nxt)   ((ale)->entry.next = (JSHashEntry *)(nxt))










struct JSAtomSet {
    JSHashEntry         *list;          
    JSHashTable         *table;         
    jsuint              count;          
};

#ifdef __cplusplus

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

    JSAtomListElement *add(JSCompiler *jsc, JSAtom *atom, AddHow how = UNIQUE);

    void remove(JSCompiler *jsc, JSAtom *atom) {
        JSHashEntry **hep;
        JSAtomListElement *ale = rawLookup(atom, hep);
        if (ale)
            rawRemove(jsc, ale, hep);
    }

    void rawRemove(JSCompiler *jsc, JSAtomListElement *ale, JSHashEntry **hep);
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

#endif 

struct JSAtomMap {
    JSAtom              **vector;       
    jsatomid            length;         
};

struct JSAtomState {
    JSDHashTable        stringAtoms;    
    JSDHashTable        doubleAtoms;    
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
    JSAtom              *countAtom;
    JSAtom              *eachAtom;
    JSAtom              *evalAtom;
    JSAtom              *fileNameAtom;
    JSAtom              *getAtom;
    JSAtom              *getterAtom;
    JSAtom              *indexAtom;
    JSAtom              *inputAtom;
    JSAtom              *iteratorAtom;
    JSAtom              *lengthAtom;
    JSAtom              *lineNumberAtom;
    JSAtom              *messageAtom;
    JSAtom              *nameAtom;
    JSAtom              *nextAtom;
    JSAtom              *noSuchMethodAtom;
    JSAtom              *parentAtom;
    JSAtom              *protoAtom;
    JSAtom              *setAtom;
    JSAtom              *setterAtom;
    JSAtom              *stackAtom;
    JSAtom              *toLocaleStringAtom;
    JSAtom              *toSourceAtom;
    JSAtom              *toStringAtom;
    JSAtom              *valueOfAtom;
    JSAtom              *toJSONAtom;
    JSAtom              *void0Atom;
    JSAtom              *enumerableAtom;
    JSAtom              *configurableAtom;
    JSAtom              *writableAtom;
    JSAtom              *valueAtom;
    JSAtom              *useStrictAtom;

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
#endif

#ifdef NARCISSUS
    JSAtom              *__call__Atom;
    JSAtom              *__construct__Atom;
    JSAtom              *__hasInstance__Atom;
    JSAtom              *ExecutionContextAtom;
    JSAtom              *currentAtom;
#endif

    
    struct {
        JSAtom          *InfinityAtom;
        JSAtom          *NaNAtom;
        JSAtom          *XMLListAtom;
        JSAtom          *decodeURIAtom;
        JSAtom          *decodeURIComponentAtom;
        JSAtom          *defineGetterAtom;
        JSAtom          *defineSetterAtom;
        JSAtom          *encodeURIAtom;
        JSAtom          *encodeURIComponentAtom;
        JSAtom          *escapeAtom;
        JSAtom          *functionNamespaceURIAtom;
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
extern const char   js_index_str[];
extern const char   js_input_str[];
extern const char   js_iterator_str[];
extern const char   js_length_str[];
extern const char   js_lineNumber_str[];
extern const char   js_message_str[];
extern const char   js_name_str[];
extern const char   js_namespace_str[];
extern const char   js_next_str[];
extern const char   js_noSuchMethod_str[];
extern const char   js_object_str[];
extern const char   js_parent_str[];
extern const char   js_proto_str[];
extern const char   js_ptagc_str[];
extern const char   js_qualifier_str[];
extern const char   js_send_str[];
extern const char   js_setter_str[];
extern const char   js_set_str[];
extern const char   js_space_str[];
extern const char   js_stack_str[];
extern const char   js_stago_str[];
extern const char   js_star_str[];
extern const char   js_starQualifier_str[];
extern const char   js_tagc_str[];
extern const char   js_toSource_str[];
extern const char   js_toString_str[];
extern const char   js_toLocaleString_str[];
extern const char   js_undefined_str[];
extern const char   js_valueOf_str[];
extern const char   js_toJSON_str[];
extern const char   js_xml_str[];
extern const char   js_enumerable_str[];
extern const char   js_configurable_str[];
extern const char   js_writable_str[];
extern const char   js_value_str[];

#ifdef NARCISSUS
extern const char   js___call___str[];
extern const char   js___construct___str[];
extern const char   js___hasInstance___str[];
extern const char   js_ExecutionContext_str[];
extern const char   js_current_str[];
#endif






extern JSBool
js_InitAtomState(JSRuntime *rt);





extern void
js_FinishAtomState(JSRuntime *rt);





extern void
js_TraceAtomState(JSTracer *trc, JSBool allAtoms);

extern void
js_SweepAtomState(JSContext *cx);

extern JSBool
js_InitCommonAtoms(JSContext *cx);

extern void
js_FinishCommonAtoms(JSContext *cx);





extern JSAtom *
js_AtomizeDouble(JSContext *cx, jsdouble d);





extern JSAtom *
js_AtomizeString(JSContext *cx, JSString *str, uintN flags);

extern JSAtom *
js_Atomize(JSContext *cx, const char *bytes, size_t length, uintN flags);

extern JSAtom *
js_AtomizeChars(JSContext *cx, const jschar *chars, size_t length, uintN flags);





extern JSAtom *
js_GetExistingStringAtom(JSContext *cx, const jschar *chars, size_t length);




JSBool
js_AtomizePrimitiveValue(JSContext *cx, jsval v, JSAtom **atomp);

#ifdef DEBUG

extern JS_FRIEND_API(void)
js_DumpAtoms(JSContext *cx, FILE *fp);

#endif






extern void
js_InitAtomMap(JSContext *cx, JSAtomMap *map, JSAtomList *al);

JS_END_EXTERN_C

#endif 
