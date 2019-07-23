






































#ifndef jsatom_h___
#define jsatom_h___



#include <stddef.h>
#include "jstypes.h"
#include "jshash.h" 
#include "jsapi.h"
#include "jsprvtd.h"
#include "jspubtd.h"

#ifdef JS_THREADSAFE
#include "jslock.h"
#endif

JS_BEGIN_EXTERN_C

#define ATOM_PINNED     0x01            /* atom is pinned against GC */
#define ATOM_INTERNED   0x02            /* pinned variant for JS_Intern* API */
#define ATOM_MARK       0x04            /* atom is reachable via GC */
#define ATOM_HIDDEN     0x08            /* atom is in special hidden subspace */
#define ATOM_NOCOPY     0x40            /* don't copy atom string bytes */
#define ATOM_TMPSTR     0x80            /* internal, to avoid extra string */

struct JSAtom {
    JSHashEntry         entry;          

    uint32              flags;          
    jsatomid            number;         
};

#define ATOM_KEY(atom)            ((jsval)(atom)->entry.key)
#define ATOM_IS_OBJECT(atom)      JSVAL_IS_OBJECT(ATOM_KEY(atom))
#define ATOM_TO_OBJECT(atom)      JSVAL_TO_OBJECT(ATOM_KEY(atom))
#define ATOM_IS_INT(atom)         JSVAL_IS_INT(ATOM_KEY(atom))
#define ATOM_TO_INT(atom)         JSVAL_TO_INT(ATOM_KEY(atom))
#define ATOM_IS_DOUBLE(atom)      JSVAL_IS_DOUBLE(ATOM_KEY(atom))
#define ATOM_TO_DOUBLE(atom)      JSVAL_TO_DOUBLE(ATOM_KEY(atom))
#define ATOM_IS_STRING(atom)      JSVAL_IS_STRING(ATOM_KEY(atom))
#define ATOM_TO_STRING(atom)      JSVAL_TO_STRING(ATOM_KEY(atom))
#define ATOM_IS_BOOLEAN(atom)     JSVAL_IS_BOOLEAN(ATOM_KEY(atom))
#define ATOM_TO_BOOLEAN(atom)     JSVAL_TO_BOOLEAN(ATOM_KEY(atom))






extern JS_FRIEND_API(const char *)
js_AtomToPrintableString(JSContext *cx, JSAtom *atom);

struct JSAtomListElement {
    JSHashEntry         entry;
};

#define ALE_ATOM(ale)   ((JSAtom *) (ale)->entry.key)
#define ALE_INDEX(ale)  ((jsatomid) JS_PTR_TO_UINT32((ale)->entry.value))
#define ALE_JSOP(ale)   ((JSOp) (ale)->entry.value)
#define ALE_VALUE(ale)  ((jsval) (ale)->entry.value)
#define ALE_NEXT(ale)   ((JSAtomListElement *) (ale)->entry.next)

#define ALE_SET_ATOM(ale,atom)  ((ale)->entry.key = (const void *)(atom))
#define ALE_SET_INDEX(ale,index)((ale)->entry.value = JS_UINT32_TO_PTR(index))
#define ALE_SET_JSOP(ale,op)    ((ale)->entry.value = JS_UINT32_TO_PTR(op))
#define ALE_SET_VALUE(ale,val)  ((ale)->entry.value = (JSHashEntry *)(val))
#define ALE_SET_NEXT(ale,link)  ((ale)->entry.next = (JSHashEntry *)(link))

struct JSAtomList {
    JSAtomListElement   *list;          
    JSHashTable         *table;         
    jsuint              count;          
};

#define ATOM_LIST_INIT(al)  ((al)->list = NULL, (al)->table = NULL,           \
                             (al)->count = 0)

#define ATOM_LIST_SEARCH(_ale,_al,_atom)                                      \
    JS_BEGIN_MACRO                                                            \
        JSHashEntry **_hep;                                                   \
        ATOM_LIST_LOOKUP(_ale, _hep, _al, _atom);                             \
    JS_END_MACRO

#define ATOM_LIST_LOOKUP(_ale,_hep,_al,_atom)                                 \
    JS_BEGIN_MACRO                                                            \
        if ((_al)->table) {                                                   \
            _hep = JS_HashTableRawLookup((_al)->table, _atom->number, _atom); \
            _ale = *_hep ? (JSAtomListElement *) *_hep : NULL;                \
        } else {                                                              \
            JSAtomListElement **_alep = &(_al)->list;                         \
            _hep = NULL;                                                      \
            while ((_ale = *_alep) != NULL) {                                 \
                if (ALE_ATOM(_ale) == (_atom)) {                              \
                    /* Hit, move atom's element to the front of the list. */  \
                    *_alep = ALE_NEXT(_ale);                                  \
                    ALE_SET_NEXT(_ale, (_al)->list);                          \
                    (_al)->list = _ale;                                       \
                    break;                                                    \
                }                                                             \
                _alep = (JSAtomListElement **)&_ale->entry.next;              \
            }                                                                 \
        }                                                                     \
    JS_END_MACRO

struct JSAtomMap {
    JSAtom              **vector;       
    jsatomid            length;         
};

struct JSAtomState {
    JSRuntime           *runtime;       
    JSHashTable         *table;         
    jsatomid            number;         
    jsatomid            liveAtoms;      

    
    JSAtom              *emptyAtom;

    
    JSAtom              *typeAtoms[JSTYPE_LIMIT];
    JSAtom              *booleanAtoms[2];
    JSAtom              *nullAtom;

    
    JSAtom              *classAtoms[JSProto_LIMIT];

    
    JSAtom              *anonymousAtom;
    JSAtom              *argumentsAtom;
    JSAtom              *arityAtom;
    JSAtom              *calleeAtom;
    JSAtom              *callerAtom;
    JSAtom              *classPrototypeAtom;
    JSAtom              *closeAtom;
    JSAtom              *constructorAtom;
    JSAtom              *countAtom;
    JSAtom              *eachAtom;
    JSAtom              *etagoAtom;
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
    JSAtom              *namespaceAtom;
    JSAtom              *nextAtom;
    JSAtom              *noSuchMethodAtom;
    JSAtom              *parentAtom;
    JSAtom              *protoAtom;
    JSAtom              *ptagcAtom;
    JSAtom              *qualifierAtom;
    JSAtom              *setAtom;
    JSAtom              *setterAtom;
    JSAtom              *spaceAtom;
    JSAtom              *stackAtom;
    JSAtom              *stagoAtom;
    JSAtom              *starAtom;
    JSAtom              *starQualifierAtom;
    JSAtom              *tagcAtom;
    JSAtom              *toLocaleStringAtom;
    JSAtom              *toSourceAtom;
    JSAtom              *toStringAtom;
    JSAtom              *valueOfAtom;
    JSAtom              *xmlAtom;

    
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

#ifdef JS_THREADSAFE
    JSThinLock          lock;
    volatile uint32     tablegen;
#endif
#ifdef NARCISSUS
    JSAtom              *callAtom;
    JSAtom              *constructAtom;
    JSAtom              *hasInstanceAtom;
    JSAtom              *ExecutionContextAtom;
    JSAtom              *currentAtom;
#endif
};

#define CLASS_ATOM(cx,name) \
    ((cx)->runtime->atomState.classAtoms[JSProto_##name])


extern const char   *js_type_strs[];
extern const char   *js_boolean_strs[];
extern const char   *js_proto_strs[];

#define JS_PROTO(name,code,init) extern const char js_##name##_str[];
#include "jsproto.tbl"
#undef JS_PROTO

extern const char   js_anonymous_str[];
extern const char   js_arguments_str[];
extern const char   js_arity_str[];
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
extern const char   js_private_str[];
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
extern const char   js_valueOf_str[];
extern const char   js_xml_str[];

#ifdef NARCISSUS
extern const char   js_call_str[];
extern const char   js_construct_str[];
extern const char   js_hasInstance_str[];
extern const char   js_ExecutionContext_str[];
extern const char   js_current_str[];
#endif





extern JSBool
js_InitAtomState(JSContext *cx, JSAtomState *state);




extern void
js_FreeAtomState(JSContext *cx, JSAtomState *state);














extern void
js_FinishAtomState(JSAtomState *state);




typedef void
(*JSGCThingMarker)(void *thing, void *data);

extern void
js_MarkAtomState(JSAtomState *state, JSBool keepAtoms, JSGCThingMarker mark,
                 void *data);

extern void
js_SweepAtomState(JSAtomState *state);

extern JSBool
js_InitPinnedAtoms(JSContext *cx, JSAtomState *state);

extern void
js_UnpinPinnedAtoms(JSAtomState *state);





extern JSAtom *
js_AtomizeObject(JSContext *cx, JSObject *obj, uintN flags);





extern JSAtom *
js_AtomizeBoolean(JSContext *cx, JSBool b, uintN flags);





extern JSAtom *
js_AtomizeInt(JSContext *cx, jsint i, uintN flags);





extern JSAtom *
js_AtomizeDouble(JSContext *cx, jsdouble d, uintN flags);





extern JSAtom *
js_AtomizeString(JSContext *cx, JSString *str, uintN flags);

extern JS_FRIEND_API(JSAtom *)
js_Atomize(JSContext *cx, const char *bytes, size_t length, uintN flags);

extern JS_FRIEND_API(JSAtom *)
js_AtomizeChars(JSContext *cx, const jschar *chars, size_t length, uintN flags);





extern JSAtom *
js_GetExistingStringAtom(JSContext *cx, const jschar *chars, size_t length);




extern JSAtom *
js_AtomizeValue(JSContext *cx, jsval value, uintN flags);




extern JSAtom *
js_ValueToStringAtom(JSContext *cx, jsval v);




extern JSAtomListElement *
js_IndexAtom(JSContext *cx, JSAtom *atom, JSAtomList *al);




extern JS_FRIEND_API(JSAtom *)
js_GetAtom(JSContext *cx, JSAtomMap *map, jsatomid i);








extern JS_FRIEND_API(JSBool)
js_InitAtomMap(JSContext *cx, JSAtomMap *map, JSAtomList *al);




extern JS_FRIEND_API(void)
js_FreeAtomMap(JSContext *cx, JSAtomMap *map);

JS_END_EXTERN_C

#endif 
