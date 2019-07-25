





































#ifndef jsxml_h___
#define jsxml_h___

#include "jspubtd.h"
#include "jsobj.h"
#include "jscell.h"

extern const char js_AnyName_str[];
extern const char js_AttributeName_str[];
extern const char js_isXMLName_str[];
extern const char js_XMLList_str[];

extern const char js_amp_entity_str[];
extern const char js_gt_entity_str[];
extern const char js_lt_entity_str[];
extern const char js_quot_entity_str[];

typedef JSBool
(* JSIdentityOp)(const void *a, const void *b);

struct JSXMLArray {
    uint32              length;
    uint32              capacity;
    void                **vector;
    JSXMLArrayCursor    *cursors;

    void init() {
        length = capacity = 0;
        vector = NULL;
        cursors = NULL;
    }

    void finish(JSContext *cx);

    bool setCapacity(JSContext *cx, uint32 capacity);
    void trim();
};

struct JSXMLArrayCursor
{
    JSXMLArray       *array;
    uint32           index;
    JSXMLArrayCursor *next;
    JSXMLArrayCursor **prevp;
    void             *root;

    JSXMLArrayCursor(JSXMLArray *array)
      : array(array), index(0), next(array->cursors), prevp(&array->cursors),
        root(NULL)
    {
        if (next)
            next->prevp = &next;
        array->cursors = this;
    }

    ~JSXMLArrayCursor() { disconnect(); }

    void disconnect() {
        if (!array)
            return;
        if (next)
            next->prevp = prevp;
        *prevp = next;
        array = NULL;
    }

    void *getNext() {
        if (!array || index >= array->length)
            return NULL;
        return root = array->vector[index++];
    }

    void *getCurrent() {
        if (!array || index >= array->length)
            return NULL;
        return root = array->vector[index];
    }

    void trace(JSTracer *trc);
};

#define JSXML_PRESET_CAPACITY   JS_BIT(31)
#define JSXML_CAPACITY_MASK     JS_BITMASK(31)
#define JSXML_CAPACITY(array)   ((array)->capacity & JSXML_CAPACITY_MASK)





typedef enum JSXMLClass {
    JSXML_CLASS_LIST,
    JSXML_CLASS_ELEMENT,
    JSXML_CLASS_ATTRIBUTE,
    JSXML_CLASS_PROCESSING_INSTRUCTION,
    JSXML_CLASS_TEXT,
    JSXML_CLASS_COMMENT,
    JSXML_CLASS_LIMIT
} JSXMLClass;

#define JSXML_CLASS_HAS_KIDS(class_)    ((class_) < JSXML_CLASS_ATTRIBUTE)
#define JSXML_CLASS_HAS_VALUE(class_)   ((class_) >= JSXML_CLASS_ATTRIBUTE)
#define JSXML_CLASS_HAS_NAME(class_)                                          \
    ((uintN)((class_) - JSXML_CLASS_ELEMENT) <=                               \
     (uintN)(JSXML_CLASS_PROCESSING_INSTRUCTION - JSXML_CLASS_ELEMENT))

#ifdef DEBUG_notme
#include "jsclist.h"
#endif

typedef struct JSXMLListVar {
    JSXMLArray          kids;           
    JSXML               *target;
    JSObject            *targetprop;
} JSXMLListVar;

typedef struct JSXMLElemVar {
    JSXMLArray          kids;           
    JSXMLArray          namespaces;
    JSXMLArray          attrs;
} JSXMLElemVar;


#define xml_kids        u.list.kids
#define xml_target      u.list.target
#define xml_targetprop  u.list.targetprop
#define xml_namespaces  u.elem.namespaces
#define xml_attrs       u.elem.attrs
#define xml_value       u.value


#define JSXML_HAS_KIDS(xml)     JSXML_CLASS_HAS_KIDS((xml)->xml_class)
#define JSXML_HAS_VALUE(xml)    JSXML_CLASS_HAS_VALUE((xml)->xml_class)
#define JSXML_HAS_NAME(xml)     JSXML_CLASS_HAS_NAME((xml)->xml_class)
#define JSXML_LENGTH(xml)       (JSXML_CLASS_HAS_KIDS((xml)->xml_class)       \
                                 ? (xml)->xml_kids.length                     \
                                 : 0)

struct JSXML : js::gc::Cell {
#ifdef DEBUG_notme
    JSCList             links;
    uint32              serial;
#endif
    JSObject            *object;
    void                *domnode;       
    JSXML               *parent;
    JSObject            *name;
    uint32              xml_class;      
    uint32              xml_flags;      
    union {
        JSXMLListVar    list;
        JSXMLElemVar    elem;
        JSString        *value;
    } u;
    
    void finalize(JSContext *cx) {
        if (JSXML_HAS_KIDS(this)) {
            xml_kids.finish(cx);
            if (xml_class == JSXML_CLASS_ELEMENT) {
                xml_namespaces.finish(cx);
                xml_attrs.finish(cx);
            }
        }
#ifdef DEBUG_notme
        JS_REMOVE_LINK(&links);
#endif
    }
};


#define XMLF_WHITESPACE_TEXT    0x1

extern JSXML *
js_NewXML(JSContext *cx, JSXMLClass xml_class);

extern void
js_TraceXML(JSTracer *trc, JSXML *xml);

extern JSObject *
js_NewXMLObject(JSContext *cx, JSXMLClass xml_class);

extern JSObject *
js_GetXMLObject(JSContext *cx, JSXML *xml);

extern JS_FRIEND_DATA(js::Class) js_XMLClass;
extern JS_FRIEND_DATA(js::Class) js_NamespaceClass;
extern JS_FRIEND_DATA(js::Class) js_QNameClass;
extern JS_FRIEND_DATA(js::Class) js_AttributeNameClass;
extern JS_FRIEND_DATA(js::Class) js_AnyNameClass;
extern js::Class                 js_XMLFilterClass;




inline bool
JSObject::isXML() const
{
    return getClass() == &js_XMLClass;
}

inline bool
JSObject::isXMLId() const
{
    js::Class *clasp = getClass();
    return clasp == &js_QNameClass ||
           clasp == &js_AttributeNameClass ||
           clasp == &js_AnyNameClass;
}

#define VALUE_IS_XML(v)      (!JSVAL_IS_PRIMITIVE(v) && JSVAL_TO_OBJECT(v)->isXML())

inline bool
JSObject::isNamespace() const
{
    return getClass() == &js_NamespaceClass;
}

inline bool
JSObject::isQName() const
{
    js::Class* clasp = getClass();
    return clasp == &js_QNameClass ||
           clasp == &js_AttributeNameClass ||
           clasp == &js_AnyNameClass;
}

static inline bool
IsXML(const js::Value &v)
{
    return v.isObject() && v.toObject().isXML();
}

extern JSObject *
js_InitNamespaceClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitQNameClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitXMLClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitXMLClasses(JSContext *cx, JSObject *obj);





JSBool
js_IsFunctionQName(JSContext *cx, JSObject *obj, jsid *funidp);

extern JSBool
js_GetDefaultXMLNamespace(JSContext *cx, jsval *vp);

extern JSBool
js_SetDefaultXMLNamespace(JSContext *cx, const js::Value &v);






extern JSBool
js_IsXMLName(JSContext *cx, jsval v);

extern JSBool
js_ToAttributeName(JSContext *cx, js::Value *vp);

extern JSFlatString *
js_EscapeAttributeValue(JSContext *cx, JSString *str, JSBool quote);

extern JSString *
js_AddAttributePart(JSContext *cx, JSBool isName, JSString *str,
                    JSString *str2);

extern JSFlatString *
js_EscapeElementValue(JSContext *cx, JSString *str);

extern JSString *
js_ValueToXMLString(JSContext *cx, const js::Value &v);

extern JSObject *
js_ConstructXMLQNameObject(JSContext *cx, const js::Value & nsval,
                           const js::Value & lnval);

extern JSBool
js_GetAnyName(JSContext *cx, jsid *idp);




extern JSBool
js_FindXMLProperty(JSContext *cx, const js::Value &nameval, JSObject **objp, jsid *idp);

extern JSBool
js_GetXMLMethod(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_GetXMLDescendants(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

extern JSBool
js_DeleteXMLListElements(JSContext *cx, JSObject *listobj);

extern JSBool
js_StepXMLListFilter(JSContext *cx, JSBool initialized);

extern JSObject *
js_ValueToXMLObject(JSContext *cx, const js::Value &v);

extern JSObject *
js_ValueToXMLListObject(JSContext *cx, const js::Value &v);

extern JSObject *
js_NewXMLSpecialObject(JSContext *cx, JSXMLClass xml_class, JSString *name,
                       JSString *value);

extern JSString *
js_MakeXMLCDATAString(JSContext *cx, JSString *str);

extern JSString *
js_MakeXMLCommentString(JSContext *cx, JSString *str);

extern JSString *
js_MakeXMLPIString(JSContext *cx, JSString *name, JSString *str);


extern JSBool
js_TestXMLEquality(JSContext *cx, const js::Value &v1, const js::Value &v2,
                   JSBool *bp);

extern JSBool
js_ConcatenateXML(JSContext *cx, JSObject *obj1, JSObject *obj2, js::Value *vp);

#endif 
