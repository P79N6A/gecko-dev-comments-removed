





































#ifndef jsxml_h___
#define jsxml_h___

#include "jspubtd.h"

JS_BEGIN_EXTERN_C

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

struct JSXML {
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
};

JS_STATIC_ASSERT(sizeof(JSXML) % JSVAL_ALIGN == 0);


#define xml_kids        u.list.kids
#define xml_target      u.list.target
#define xml_targetprop  u.list.targetprop
#define xml_namespaces  u.elem.namespaces
#define xml_attrs       u.elem.attrs
#define xml_value       u.value


#define XMLF_WHITESPACE_TEXT    0x1


#define JSXML_HAS_KIDS(xml)     JSXML_CLASS_HAS_KIDS((xml)->xml_class)
#define JSXML_HAS_VALUE(xml)    JSXML_CLASS_HAS_VALUE((xml)->xml_class)
#define JSXML_HAS_NAME(xml)     JSXML_CLASS_HAS_NAME((xml)->xml_class)
#define JSXML_LENGTH(xml)       (JSXML_CLASS_HAS_KIDS((xml)->xml_class)       \
                                 ? (xml)->xml_kids.length                     \
                                 : 0)

extern JSXML *
js_NewXML(JSContext *cx, JSXMLClass xml_class);

extern void
js_TraceXML(JSTracer *trc, JSXML *xml);

extern void
js_FinalizeXML(JSContext *cx, JSXML *xml);

extern JSObject *
js_ParseNodeToXMLObject(JSCompiler *jsc, JSParseNode *pn);

extern JSObject *
js_NewXMLObject(JSContext *cx, JSXMLClass xml_class);

extern JSObject *
js_GetXMLObject(JSContext *cx, JSXML *xml);

extern JS_FRIEND_DATA(JSObjectOps)      js_XMLObjectOps;
extern JS_FRIEND_DATA(JSClass)          js_XMLClass;
extern JS_FRIEND_DATA(JSExtendedClass)  js_NamespaceClass;
extern JS_FRIEND_DATA(JSExtendedClass)  js_QNameClass;
extern JS_FRIEND_DATA(JSClass)          js_AttributeNameClass;
extern JS_FRIEND_DATA(JSClass)          js_AnyNameClass;
extern JSClass                          js_XMLFilterClass;






#define OBJECT_IS_XML(cx,obj)   ((obj)->map->ops == &js_XMLObjectOps)
#define VALUE_IS_XML(cx,v)      (!JSVAL_IS_PRIMITIVE(v) &&                    \
                                 OBJECT_IS_XML(cx, JSVAL_TO_OBJECT(v)))

extern JSObject *
js_InitNamespaceClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitQNameClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitAttributeNameClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitAnyNameClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitXMLClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitXMLClasses(JSContext *cx, JSObject *obj);

extern JSBool
js_GetFunctionNamespace(JSContext *cx, jsval *vp);





JSBool
js_IsFunctionQName(JSContext *cx, JSObject *obj, jsid *funidp);

extern JSBool
js_GetDefaultXMLNamespace(JSContext *cx, jsval *vp);

extern JSBool
js_SetDefaultXMLNamespace(JSContext *cx, jsval v);






extern JSBool
js_IsXMLName(JSContext *cx, jsval v);

extern JSBool
js_ToAttributeName(JSContext *cx, jsval *vp);

extern JSString *
js_EscapeAttributeValue(JSContext *cx, JSString *str, JSBool quote);

extern JSString *
js_AddAttributePart(JSContext *cx, JSBool isName, JSString *str,
                    JSString *str2);

extern JSString *
js_EscapeElementValue(JSContext *cx, JSString *str);

extern JSString *
js_ValueToXMLString(JSContext *cx, jsval v);

extern JSObject *
js_ConstructXMLQNameObject(JSContext *cx, jsval nsval, jsval lnval);

extern JSBool
js_GetAnyName(JSContext *cx, jsval *vp);




extern JSBool
js_FindXMLProperty(JSContext *cx, jsval nameval, JSObject **objp, jsid *idp);

extern JSBool
js_GetXMLMethod(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

extern JSBool
js_GetXMLDescendants(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

extern JSBool
js_DeleteXMLListElements(JSContext *cx, JSObject *listobj);

extern JSBool
js_StepXMLListFilter(JSContext *cx, JSBool initialized);

extern JSObject *
js_ValueToXMLObject(JSContext *cx, jsval v);

extern JSObject *
js_ValueToXMLListObject(JSContext *cx, jsval v);

extern JSObject *
js_CloneXMLObject(JSContext *cx, JSObject *obj);

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
js_EnumerateXMLValues(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                      jsval *statep, jsid *idp, jsval *vp);

extern JSBool
js_TestXMLEquality(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

extern JSBool
js_ConcatenateXML(JSContext *cx, JSObject *obj, jsval v, jsval *vp);

JS_END_EXTERN_C

#endif 
