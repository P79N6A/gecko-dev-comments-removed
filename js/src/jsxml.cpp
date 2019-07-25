






































#include "jsversion.h"

#if JS_HAS_XML_SUPPORT

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsprf.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcmark.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jsxml.h"

#include "frontend/Parser.h"
#include "frontend/TokenStream.h"
#include "vm/GlobalObject.h"
#include "vm/MethodGuard.h"

#include "jsatominlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsstrinlines.h"

#include "vm/Stack-inl.h"
#include "vm/String-inl.h"

#ifdef DEBUG
#include <string.h>     
#endif

using namespace mozilla;
using namespace js;
using namespace js::gc;
using namespace js::types;

template<class T, class U>
struct IdentityOp
{
    typedef JSBool (* compare)(const T *a, const U *b);
};

template<class T>
static JSBool
pointer_match(const T *a, const T *b)
{
    return a == b;
}













static inline bool
js_EnterLocalRootScope(JSContext *cx)
{
    return true;
}

static inline void
js_LeaveLocalRootScope(JSContext *cx)
{
}

static inline void
js_LeaveLocalRootScopeWithResult(JSContext *cx, Value rval)
{
}

static inline void
js_LeaveLocalRootScopeWithResult(JSContext *cx, void *rval)
{
}




const char js_AttributeName_str[] = "AttributeName";
const char js_isXMLName_str[]     = "isXMLName";
const char js_XMLList_str[]       = "XMLList";
const char js_localName_str[]     = "localName";
const char js_xml_parent_str[]    = "parent";
const char js_prefix_str[]        = "prefix";
const char js_toXMLString_str[]   = "toXMLString";
const char js_uri_str[]           = "uri";

const char js_amp_entity_str[]    = "&amp;";
const char js_gt_entity_str[]     = "&gt;";
const char js_lt_entity_str[]     = "&lt;";
const char js_quot_entity_str[]   = "&quot;";
const char js_leftcurly_entity_str[]   = "&#123;";

#define IS_STAR(str)  ((str)->length() == 1 && *(str)->chars() == '*')

static JSBool
GetXMLFunction(JSContext *cx, JSObject *obj, jsid id, jsval *vp);

static JSBool
IsDeclared(const JSObject *obj)
{
    jsval v;

    JS_ASSERT(obj->getClass() == &NamespaceClass);
    v = obj->getNamespaceDeclared();
    JS_ASSERT(JSVAL_IS_VOID(v) || v == JSVAL_TRUE);
    return v == JSVAL_TRUE;
}

static JSBool
xml_isXMLName(JSContext *cx, unsigned argc, jsval *vp)
{
    *vp = BOOLEAN_TO_JSVAL(js_IsXMLName(cx, argc ? vp[2] : JSVAL_VOID));
    return JS_TRUE;
}

size_t sE4XObjectsCreated = 0;






static inline JSObject *
NewBuiltinClassInstanceXML(JSContext *cx, Class *clasp)
{
    if (!cx->runningWithTrustedPrincipals())
        ++sE4XObjectsCreated;

    return NewBuiltinClassInstance(cx, clasp);
}

#define DEFINE_GETTER(name,code)                                               \
    static JSBool                                                              \
    name(JSContext *cx, JSObject *obj, jsid id, jsval *vp)                     \
    {                                                                          \
        code;                                                                  \
        return true;                                                           \
    }




DEFINE_GETTER(NamePrefix_getter,
              if (obj->getClass() == &NamespaceClass) *vp = obj->getNamePrefixVal())
DEFINE_GETTER(NameURI_getter,
              if (obj->getClass() == &NamespaceClass) *vp = obj->getNameURIVal())

static JSBool
namespace_equality(JSContext *cx, JSObject *obj, const Value *v, JSBool *bp)
{
    JSObject *obj2;

    JS_ASSERT(v->isObjectOrNull());
    obj2 = v->toObjectOrNull();
    *bp = (!obj2 || obj2->getClass() != &NamespaceClass)
          ? JS_FALSE
          : EqualStrings(obj->getNameURI(), obj2->getNameURI());
    return JS_TRUE;
}

JS_FRIEND_DATA(Class) js::NamespaceClass = {
    "Namespace",
    JSCLASS_HAS_RESERVED_SLOTS(JSObject::NAMESPACE_CLASS_RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Namespace),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    {
        namespace_equality,
        NULL,                
        NULL,                
        NULL,                
        NULL,                
    }
};

#define NAMESPACE_ATTRS                                                       \
    (JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED)

static JSPropertySpec namespace_props[] = {
    {js_prefix_str, 0, NAMESPACE_ATTRS, NamePrefix_getter, 0},
    {js_uri_str,    0, NAMESPACE_ATTRS, NameURI_getter,    0},
    {0,0,0,0,0}
};

static JSBool
namespace_toString(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    if (!obj->isNamespace()) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &NamespaceClass);
        return JS_FALSE;
    }
    *vp = obj->getNameURIVal();
    return JS_TRUE;
}

static JSFunctionSpec namespace_methods[] = {
    JS_FN(js_toString_str,  namespace_toString,        0,0),
    JS_FS_END
};

static JSObject *
NewXMLNamespace(JSContext *cx, JSLinearString *prefix, JSLinearString *uri, JSBool declared)
{
    JSObject *obj;

    obj = NewBuiltinClassInstanceXML(cx, &NamespaceClass);
    if (!obj)
        return NULL;
    JS_ASSERT(JSVAL_IS_VOID(obj->getNamePrefixVal()));
    JS_ASSERT(JSVAL_IS_VOID(obj->getNameURIVal()));
    JS_ASSERT(JSVAL_IS_VOID(obj->getNamespaceDeclared()));

    
    if (!JS_DefineProperties(cx, obj, namespace_props))
        return NULL;

    if (prefix)
        obj->setNamePrefix(prefix);
    if (uri)
        obj->setNameURI(uri);
    if (declared)
        obj->setNamespaceDeclared(JSVAL_TRUE);
    return obj;
}




DEFINE_GETTER(QNameNameURI_getter,
              if (obj->getClass() == &QNameClass)
                  *vp = JSVAL_IS_VOID(obj->getNameURIVal()) ? JSVAL_NULL : obj->getNameURIVal())
DEFINE_GETTER(QNameLocalName_getter,
              if (obj->getClass() == &QNameClass)
                  *vp = obj->getQNameLocalNameVal())

static JSBool
qname_identity(JSObject *qna, const JSObject *qnb)
{
    JSLinearString *uri1 = qna->getNameURI();
    JSLinearString *uri2 = qnb->getNameURI();

    if (!uri1 ^ !uri2)
        return JS_FALSE;
    if (uri1 && !EqualStrings(uri1, uri2))
        return JS_FALSE;
    return EqualStrings(qna->getQNameLocalName(), qnb->getQNameLocalName());
}

static JSBool
qname_equality(JSContext *cx, JSObject *qn, const Value *v, JSBool *bp)
{
    JSObject *obj2;

    obj2 = v->toObjectOrNull();
    *bp = (!obj2 || obj2->getClass() != &QNameClass)
          ? JS_FALSE
          : qname_identity(qn, obj2);
    return JS_TRUE;
}

JS_FRIEND_DATA(Class) js::QNameClass = {
    "QName",
    JSCLASS_HAS_RESERVED_SLOTS(JSObject::QNAME_CLASS_RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_QName),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    {
        qname_equality,
        NULL,                
        NULL,                
        NULL,                
        NULL,                
    }
};







JS_FRIEND_DATA(Class) js::AttributeNameClass = {
    js_AttributeName_str,
    JSCLASS_HAS_RESERVED_SLOTS(JSObject::QNAME_CLASS_RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

JS_FRIEND_DATA(Class) js::AnyNameClass = {
    js_AnyName_str,
    JSCLASS_HAS_RESERVED_SLOTS(JSObject::QNAME_CLASS_RESERVED_SLOTS) |
    JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

#define QNAME_ATTRS (JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED)

static JSPropertySpec qname_props[] = {
    {js_uri_str,       0, QNAME_ATTRS, QNameNameURI_getter,   0},
    {js_localName_str, 0, QNAME_ATTRS, QNameLocalName_getter, 0},
    {0,0,0,0,0}
};

static JSString *
ConvertQNameToString(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isQName());
    JSString *uri = obj->getNameURI();
    JSString *str;
    if (!uri) {
        
        str = cx->runtime->atomState.starQualifierAtom;
    } else if (uri->empty()) {
        
        str = cx->runtime->emptyString;
    } else {
        JSString *qualstr = cx->runtime->atomState.qualifierAtom;
        str = js_ConcatStrings(cx, uri, qualstr);
        if (!str)
            return NULL;
    }
    str = js_ConcatStrings(cx, str, obj->getQNameLocalName());
    if (!str)
        return NULL;

    if (obj->getClass() == &AttributeNameClass) {
        JS::Anchor<JSString *> anchor(str);
        size_t length = str->length();
        jschar *chars = (jschar *) cx->malloc_((length + 2) * sizeof(jschar));
        if (!chars)
            return JS_FALSE;
        *chars = '@';
        const jschar *strChars = str->getChars(cx);
        if (!strChars) {
            cx->free_(chars);
            return NULL;
        }
        js_strncpy(chars + 1, strChars, length);
        chars[++length] = 0;
        str = js_NewString(cx, chars, length);
        if (!str) {
            cx->free_(chars);
            return NULL;
        }
    }
    return str;
}

static JSBool
qname_toString(JSContext *cx, unsigned argc, Value *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    if (!obj->isQName()) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &QNameClass);
        return false;
    }

    JSString *str = ConvertQNameToString(cx, obj);
    if (!str)
        return false;

    vp->setString(str);
    return true;
}

static JSFunctionSpec qname_methods[] = {
    JS_FN(js_toString_str,  qname_toString,    0,0),
    JS_FS_END
};


static bool
InitXMLQName(JSContext *cx, JSObject *obj, JSLinearString *uri, JSLinearString *prefix,
             JSAtom *localName)
{
    JS_ASSERT(obj->isQName());
    JS_ASSERT(JSVAL_IS_VOID(obj->getNamePrefixVal()));
    JS_ASSERT(JSVAL_IS_VOID(obj->getNameURIVal()));
    JS_ASSERT(JSVAL_IS_VOID(obj->getQNameLocalNameVal()));

    
    if (!JS_DefineProperties(cx, obj, qname_props))
        return false;

    if (uri)
        obj->setNameURI(uri);
    if (prefix)
        obj->setNamePrefix(prefix);
    if (localName)
        obj->setQNameLocalName(localName);
    return true;
}

static JSObject *
NewXMLQName(JSContext *cx, JSLinearString *uri, JSLinearString *prefix,
            JSAtom *localName)
{
    JSObject *obj = NewBuiltinClassInstanceXML(cx, &QNameClass);
    if (!obj)
        return NULL;
    if (!InitXMLQName(cx, obj, uri, prefix, localName))
        return NULL;
    return obj;
}

static JSObject *
NewXMLAttributeName(JSContext *cx, JSLinearString *uri, JSLinearString *prefix,
                    JSAtom *localName)
{
    



    JSObject *parent = GetGlobalForScopeChain(cx);
    JSObject *obj = NewObjectWithGivenProto(cx, &AttributeNameClass, NULL, parent);
    if (!obj)
        return NULL;
    JS_ASSERT(obj->isQName());
    if (!InitXMLQName(cx, obj, uri, prefix, localName))
        return NULL;
    return obj;
}

JSObject *
js_ConstructXMLQNameObject(JSContext *cx, const Value &nsval, const Value &lnval)
{
    Value argv[2];

    




    if (nsval.isObject() &&
        nsval.toObject().getClass() == &AnyNameClass) {
        argv[0].setNull();
    } else {
        argv[0] = nsval;
    }
    argv[1] = lnval;
    return JS_ConstructObjectWithArguments(cx, Jsvalify(&QNameClass), NULL, 2, argv);
}

static JSBool
IsXMLName(const jschar *cp, size_t n)
{
    JSBool rv;
    jschar c;

    rv = JS_FALSE;
    if (n != 0 && unicode::IsXMLNamespaceStart(*cp)) {
        while (--n != 0) {
            c = *++cp;
            if (!unicode::IsXMLNamespacePart(c))
                return rv;
        }
        rv = JS_TRUE;
    }
    return rv;
}

JSBool
js_IsXMLName(JSContext *cx, jsval v)
{
    JSLinearString *name = NULL;
    JSErrorReporter older;

    





    if (!JSVAL_IS_PRIMITIVE(v) &&
        JSVAL_TO_OBJECT(v)->isQName()) {
        name = JSVAL_TO_OBJECT(v)->getQNameLocalName();
    } else {
        older = JS_SetErrorReporter(cx, NULL);
        JSString *str = ToString(cx, v);
        if (str)
            name = str->ensureLinear(cx);
        JS_SetErrorReporter(cx, older);
        if (!name) {
            JS_ClearPendingException(cx);
            return JS_FALSE;
        }
    }

    return IsXMLName(name->chars(), name->length());
}





static JSBool
NamespaceHelper(JSContext *cx, int argc, jsval *argv, jsval *rval)
{
    jsval urival, prefixval;
    JSObject *uriobj;
    JSBool isNamespace, isQName;
    Class *clasp;
    JSLinearString *empty, *prefix, *uri;

    isNamespace = isQName = JS_FALSE;
#ifdef __GNUC__         
    uriobj = NULL;
#endif
    if (argc <= 0) {
        urival = JSVAL_VOID;
    } else {
        urival = argv[argc > 1];
        if (!JSVAL_IS_PRIMITIVE(urival)) {
            uriobj = JSVAL_TO_OBJECT(urival);
            clasp = uriobj->getClass();
            isNamespace = (clasp == &NamespaceClass);
            isQName = (clasp == &QNameClass);
        }
    }

    
    if (argc == 1 && isNamespace) {
        
        *rval = urival;
        return JS_TRUE;
    }

    JSObject *obj = NewBuiltinClassInstanceXML(cx, &NamespaceClass);
    if (!obj)
        return JS_FALSE;

    
    if (!JS_DefineProperties(cx, obj, namespace_props))
        return JS_FALSE;

    *rval = OBJECT_TO_JSVAL(obj);

    empty = cx->runtime->emptyString;
    obj->setNamePrefix(empty);
    obj->setNameURI(empty);

    if (argc == 1 || argc == -1) {
        if (isNamespace) {
            obj->setNameURI(uriobj->getNameURI());
            obj->setNamePrefix(uriobj->getNamePrefix());
        } else if (isQName && (uri = uriobj->getNameURI())) {
            obj->setNameURI(uri);
            obj->setNamePrefix(uriobj->getNamePrefix());
        } else {
            JSString *str = ToString(cx, urival);
            if (!str)
                return JS_FALSE;
            uri = str->ensureLinear(cx);
            if (!uri)
                return JS_FALSE;
            obj->setNameURI(uri);
            if (!uri->empty())
                obj->clearNamePrefix();
        }
    } else if (argc == 2) {
        if (!isQName || !(uri = uriobj->getNameURI())) {
            JSString *str = ToString(cx, urival);
            if (!str)
                return JS_FALSE;
            uri = str->ensureLinear(cx);
            if (!uri)
                return JS_FALSE;
        }
        obj->setNameURI(uri);

        prefixval = argv[0];
        if (uri->empty()) {
            if (!JSVAL_IS_VOID(prefixval)) {
                JSString *str = ToString(cx, prefixval);
                if (!str)
                    return JS_FALSE;
                if (!str->empty()) {
                    JSAutoByteString bytes;
                    if (js_ValueToPrintable(cx, StringValue(str), &bytes)) {
                        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                             JSMSG_BAD_XML_NAMESPACE, bytes.ptr());
                    }
                    return JS_FALSE;
                }
            }
        } else if (JSVAL_IS_VOID(prefixval) || !js_IsXMLName(cx, prefixval)) {
            obj->clearNamePrefix();
        } else {
            JSString *str = ToString(cx, prefixval);
            if (!str)
                return JS_FALSE;
            prefix = str->ensureLinear(cx);
            if (!prefix)
                return JS_FALSE;
            obj->setNamePrefix(prefix);
        }
    }
    return JS_TRUE;
}

static JSBool
Namespace(JSContext *cx, unsigned argc, Value *vp)
{
    return NamespaceHelper(cx, argc, vp + 2, vp);
}





static JSBool
QNameHelper(JSContext *cx, int argc, jsval *argv, jsval *rval)
{
    jsval nameval, nsval;
    JSBool isQName, isNamespace;
    JSObject *qn;
    JSLinearString *uri, *prefix;
    JSObject *obj2;

    JSAtom *name;
    if (argc <= 0) {
        nameval = JSVAL_VOID;
        isQName = JS_FALSE;
    } else {
        nameval = argv[argc > 1];
        isQName =
            !JSVAL_IS_PRIMITIVE(nameval) &&
            JSVAL_TO_OBJECT(nameval)->getClass() == &QNameClass;
    }

    
    if (argc == 1 && isQName) {
        
        *rval = nameval;
        return JS_TRUE;
    }

        
    JSObject *obj = NewBuiltinClassInstanceXML(cx, &QNameClass);
    if (!obj)
        return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(obj);

    if (isQName) {
        
        qn = JSVAL_TO_OBJECT(nameval);
        if (argc == 1) {
            uri = qn->getNameURI();
            prefix = qn->getNamePrefix();
            name = qn->getQNameLocalName();
            goto out;
        }

        
        nameval = qn->getQNameLocalNameVal();
    }

    if (argc == 0) {
        name = cx->runtime->emptyString;
    } else if (argc < 0) {
        name = cx->runtime->atomState.typeAtoms[JSTYPE_VOID];
    } else {
        if (!js_ValueToAtom(cx, nameval, &name))
            return false;
    }

    if (argc > 1 && !JSVAL_IS_VOID(argv[0])) {
        nsval = argv[0];
    } else if (IS_STAR(name)) {
        nsval = JSVAL_NULL;
    } else {
        if (!js_GetDefaultXMLNamespace(cx, &nsval))
            return JS_FALSE;
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(nsval));
        JS_ASSERT(JSVAL_TO_OBJECT(nsval)->getClass() ==
                  &NamespaceClass);
    }

    if (JSVAL_IS_NULL(nsval)) {
        
        prefix = uri = NULL;
    } else {
        






        isNamespace = isQName = JS_FALSE;
        if (!JSVAL_IS_PRIMITIVE(nsval)) {
            obj2 = JSVAL_TO_OBJECT(nsval);
            isNamespace = (obj2->getClass() == &NamespaceClass);
            isQName = (obj2->getClass() == &QNameClass);
        }
#ifdef __GNUC__         
        else obj2 = NULL;
#endif

        if (isNamespace) {
            uri = obj2->getNameURI();
            prefix = obj2->getNamePrefix();
        } else if (isQName && (uri = obj2->getNameURI())) {
            JS_ASSERT(argc > 1);
            prefix = obj2->getNamePrefix();
        } else {
            JS_ASSERT(argc > 1);
            JSString *str = ToString(cx, nsval);
            if (!str)
                return JS_FALSE;
            uri = str->ensureLinear(cx);
            if (!uri)
                return JS_FALSE;
            argv[0] = STRING_TO_JSVAL(uri);     

            
            prefix = uri->empty() ? cx->runtime->emptyString : NULL;
        }
    }

out:
    return InitXMLQName(cx, obj, uri, prefix, name);
}

static JSBool
QName(JSContext *cx, unsigned argc, Value *vp)
{
    return QNameHelper(cx, argc, vp + 2, vp);
}




static JSBool
namespace_identity(const JSObject *nsa, const JSObject *nsb)
{
    JSLinearString *prefixa = nsa->getNamePrefix();
    JSLinearString *prefixb = nsb->getNamePrefix();

    if (prefixa && prefixb) {
        if (!EqualStrings(prefixa, prefixb))
            return JS_FALSE;
    } else {
        if (prefixa || prefixb)
            return JS_FALSE;
    }
    return EqualStrings(nsa->getNameURI(), nsb->getNameURI());
}

static JSBool
attr_identity(const JSXML *xmla, const JSXML *xmlb)
{
    return qname_identity(xmla->name, xmlb->name);
}

void
js_XMLArrayCursorTrace(JSTracer *trc, JSXMLArrayCursor<JSXML> *cursor)
{
    for (; cursor; cursor = cursor->next) {
        if (cursor->root)
            MarkXML(trc, &(HeapPtr<JSXML> &)cursor->root, "cursor_root");
    }
}

void
js_XMLArrayCursorTrace(JSTracer *trc, JSXMLArrayCursor<JSObject> *cursor)
{
    for (; cursor; cursor = cursor->next) {
        if (cursor->root)
            MarkObject(trc, &(HeapPtr<JSObject> &)cursor->root, "cursor_root");
    }
}

template<class T>
static HeapPtr<T> *
ReallocateVector(HeapPtr<T> *vector, size_t count)
{
#if JS_BITS_PER_WORD == 32
    if (count > ~(size_t)0 / sizeof(HeapPtr<T>))
        return NULL;
#endif

    size_t size = count * sizeof(HeapPtr<T>);
    return (HeapPtr<T> *) OffTheBooks::realloc_(vector, size);
}


template<class T>
bool
JSXMLArray<T>::setCapacity(JSContext *cx, uint32_t newCapacity)
{
    if (newCapacity == 0) {
        
        if (vector) {
            if (cx)
                cx->free_(vector);
            else
                Foreground::free_(vector);
        }
        vector = NULL;
    } else {
        HeapPtr<T> *tmp = ReallocateVector(vector, newCapacity);
        if (!tmp) {
            if (cx)
                JS_ReportOutOfMemory(cx);
            return false;
        }
        vector = tmp;
    }
    capacity = JSXML_PRESET_CAPACITY | newCapacity;
    return true;
}

template<class T>
void
JSXMLArray<T>::trim()
{
    if (capacity & JSXML_PRESET_CAPACITY)
        return;
    if (length < capacity)
        setCapacity(NULL, length);
}

template<class T>
void
JSXMLArray<T>::finish(JSContext *cx)
{
    if (!cx->runtime->gcRunning) {
        
        for (uint32_t i = 0; i < length; i++)
            vector[i].~HeapPtr<T>();
    }

    cx->free_(vector);

    while (JSXMLArrayCursor<T> *cursor = cursors)
        cursor->disconnect();

#ifdef DEBUG
    memset(this, 0xd5, sizeof *this);
#endif
}

#define XML_NOT_FOUND   UINT32_MAX

template<class T, class U>
static uint32_t
XMLArrayFindMember(const JSXMLArray<T> *array, U *elt, typename IdentityOp<T, U>::compare identity)
{
    HeapPtr<T> *vector;
    uint32_t i, n;

    
    vector = array->vector;
    for (i = 0, n = array->length; i < n; i++) {
        if (identity(vector[i].get(), elt))
            return i;
    }
    return XML_NOT_FOUND;
}






#define LINEAR_THRESHOLD        256
#define LINEAR_INCREMENT        32

template<class T>
static JSBool
XMLArrayAddMember(JSContext *cx, JSXMLArray<T> *array, uint32_t index, T *elt)
{
    uint32_t capacity, i;
    int log2;
    HeapPtr<T> *vector;

    if (index >= array->length) {
        if (index >= JSXML_CAPACITY(array)) {
            
            capacity = index + 1;
            if (index >= LINEAR_THRESHOLD) {
                capacity = JS_ROUNDUP(capacity, LINEAR_INCREMENT);
            } else {
                JS_CEILING_LOG2(log2, capacity);
                capacity = JS_BIT(log2);
            }
            if (!(vector = ReallocateVector(array->vector, capacity))) {
                JS_ReportOutOfMemory(cx);
                return JS_FALSE;
            }
            array->capacity = capacity;
            array->vector = vector;
            for (i = array->length; i < index; i++)
                vector[i].init(NULL);
        }
        array->vector[index].init(NULL);
        array->length = index + 1;
    }

    array->vector[index] = elt;
    return JS_TRUE;
}

template<class T>
static JSBool
XMLArrayInsert(JSContext *cx, JSXMLArray<T> *array, uint32_t i, uint32_t n)
{
    uint32_t j, k;
    JSXMLArrayCursor<T> *cursor;

    j = array->length;
    JS_ASSERT(i <= j);
    if (!array->setCapacity(cx, j + n))
        return JS_FALSE;

    k = j;
    while (k != j + n) {
        array->vector[k].init(NULL);
        k++;
    }

    array->length = j + n;
    JS_ASSERT(n != (uint32_t)-1);
    while (j != i) {
        --j;
        array->vector[j + n] = array->vector[j];
    }

    for (cursor = array->cursors; cursor; cursor = cursor->next) {
        if (cursor->index > i)
            cursor->index += n;
    }
    return JS_TRUE;
}

template<class T>
static T *
XMLArrayDelete(JSContext *cx, JSXMLArray<T> *array, uint32_t index, JSBool compress)
{
    uint32_t length;
    HeapPtr<T> *vector;
    T *elt;
    JSXMLArrayCursor<T> *cursor;

    length = array->length;
    if (index >= length)
        return NULL;

    vector = array->vector;
    elt = vector[index];
    if (compress) {
        vector[length - 1].~HeapPtr<T>();
        while (++index < length)
            vector[index-1] = vector[index];
        array->length = length - 1;
        array->capacity = JSXML_CAPACITY(array);
    } else {
        vector[index] = NULL;
    }

    for (cursor = array->cursors; cursor; cursor = cursor->next) {
        if (cursor->index > index)
            --cursor->index;
    }
    return elt;
}

template<class T>
static void
XMLArrayTruncate(JSContext *cx, JSXMLArray<T> *array, uint32_t length)
{
    HeapPtr<T> *vector;

    JS_ASSERT(!array->cursors);
    if (length >= array->length)
        return;

    for (uint32_t i = length; i < array->length; i++)
        array->vector[i].~HeapPtr<T>();

    if (length == 0) {
        if (array->vector)
            cx->free_(array->vector);
        vector = NULL;
    } else {
        vector = ReallocateVector(array->vector, length);
        if (!vector)
            return;
    }

    if (array->length > length)
        array->length = length;
    array->capacity = length;
    array->vector = vector;
}

#define XMLARRAY_FIND_MEMBER(a,e,f) XMLArrayFindMember(a, e, f)
#define XMLARRAY_HAS_MEMBER(a,e,f)  (XMLArrayFindMember(a, e, f) !=           \
                                     XML_NOT_FOUND)
#define XMLARRAY_MEMBER(a,i,t)      (((i) < (a)->length)                      \
                                     ? (a)->vector[i].get()                   \
                                     : NULL)
#define XMLARRAY_SET_MEMBER(a,i,e)  JS_BEGIN_MACRO                            \
                                        if ((a)->length <= (i)) {             \
                                            (a)->length = (i) + 1;            \
                                            ((a)->vector[i].init(e));         \
                                        } else {                              \
                                            ((a)->vector[i] = e);             \
                                        }                                     \
                                    JS_END_MACRO
#define XMLARRAY_ADD_MEMBER(x,a,i,e)XMLArrayAddMember(x, a, i, e)
#define XMLARRAY_INSERT(x,a,i,n)    XMLArrayInsert(x, a, i, n)
#define XMLARRAY_APPEND(x,a,e)      XMLARRAY_ADD_MEMBER(x, a, (a)->length, (e))
#define XMLARRAY_DELETE(x,a,i,c,t)  (XMLArrayDelete<t>(x, a, i, c))
#define XMLARRAY_TRUNCATE(x,a,n)    XMLArrayTruncate(x, a, n)





static const char js_ignoreComments_str[]   = "ignoreComments";
static const char js_ignoreProcessingInstructions_str[]
                                            = "ignoreProcessingInstructions";
static const char js_ignoreWhitespace_str[] = "ignoreWhitespace";
static const char js_prettyPrinting_str[]   = "prettyPrinting";
static const char js_prettyIndent_str[]     = "prettyIndent";

#define XSF_IGNORE_COMMENTS                JS_BIT(0)
#define XSF_IGNORE_PROCESSING_INSTRUCTIONS JS_BIT(1)
#define XSF_IGNORE_WHITESPACE              JS_BIT(2)
#define XSF_PRETTY_PRINTING                JS_BIT(3)

static JSPropertySpec xml_static_props[] = {
    {js_ignoreComments_str, 0, JSPROP_PERMANENT, NULL, NULL},
    {js_ignoreProcessingInstructions_str, 0, JSPROP_PERMANENT, NULL, NULL},
    {js_ignoreWhitespace_str, 0, JSPROP_PERMANENT, NULL, NULL},
    {js_prettyPrinting_str, 0, JSPROP_PERMANENT, NULL, NULL},
    {js_prettyIndent_str, 0, JSPROP_PERMANENT, NULL, NULL},
    {0,0,0,0,0}
};


#define IS_XML(str)                                                           \
    (str->length() == 3 && IS_XML_CHARS(str->chars()))

#define IS_XMLNS(str)                                                         \
    (str->length() == 5 && IS_XMLNS_CHARS(str->chars()))

static inline bool
IS_XML_CHARS(const jschar *chars)
{
    return (chars[0] == 'x' || chars[0] == 'X') &&
           (chars[1] == 'm' || chars[1] == 'M') &&
           (chars[2] == 'l' || chars[2] == 'L');
}

static inline bool
HAS_NS_AFTER_XML(const jschar *chars)
{
    return (chars[3] == 'n' || chars[3] == 'N') &&
           (chars[4] == 's' || chars[4] == 'S');
}

#define IS_XMLNS_CHARS(chars)                                                 \
    (IS_XML_CHARS(chars) && HAS_NS_AFTER_XML(chars))

#define STARTS_WITH_XML(chars,length)                                         \
    (length >= 3 && IS_XML_CHARS(chars))

static const char xml_namespace_str[] = "http://www.w3.org/XML/1998/namespace";
static const char xmlns_namespace_str[] = "http://www.w3.org/2000/xmlns/";

void
JSXML::finalize(JSContext *cx, bool builtin)
{
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

static JSObject *
ParseNodeToQName(Parser *parser, ParseNode *pn,
                 JSXMLArray<JSObject> *inScopeNSes, JSBool isAttributeName)
{
    JSContext *cx = parser->context;
    JSLinearString *uri, *prefix;
    size_t length, offset;
    const jschar *start, *limit, *colon;
    uint32_t n;
    JSObject *ns;
    JSLinearString *nsprefix;

    JS_ASSERT(pn->isArity(PN_NULLARY));
    JSAtom *str = pn->pn_atom;
    start = str->chars();
    length = str->length();
    JS_ASSERT(length != 0 && *start != '@');
    JS_ASSERT(length != 1 || *start != '*');

    JSAtom *localName;

    uri = cx->runtime->emptyString;
    limit = start + length;
    colon = js_strchr_limit(start, ':', limit);
    if (colon) {
        offset = colon - start;
        prefix = js_NewDependentString(cx, str, 0, offset);
        if (!prefix)
            return NULL;

        if (STARTS_WITH_XML(start, offset)) {
            if (offset == 3) {
                uri = JS_ASSERT_STRING_IS_FLAT(JS_InternString(cx, xml_namespace_str));
                if (!uri)
                    return NULL;
            } else if (offset == 5 && HAS_NS_AFTER_XML(start)) {
                uri = JS_ASSERT_STRING_IS_FLAT(JS_InternString(cx, xmlns_namespace_str));
                if (!uri)
                    return NULL;
            } else {
                uri = NULL;
            }
        } else {
            uri = NULL;
            n = inScopeNSes->length;
            while (n != 0) {
                --n;
                ns = XMLARRAY_MEMBER(inScopeNSes, n, JSObject);
                nsprefix = ns->getNamePrefix();
                if (nsprefix && EqualStrings(nsprefix, prefix)) {
                    uri = ns->getNameURI();
                    break;
                }
            }
        }

        if (!uri) {
            Value v = StringValue(prefix);
            JSAutoByteString bytes;
            if (js_ValueToPrintable(parser->context, v, &bytes)) {
                ReportCompileErrorNumber(parser->context, &parser->tokenStream, pn,
                                         JSREPORT_ERROR, JSMSG_BAD_XML_NAMESPACE, bytes.ptr());
            }
            return NULL;
        }

        localName = js_AtomizeChars(parser->context, colon + 1, length - (offset + 1));
        if (!localName)
            return NULL;
    } else {
        if (isAttributeName) {
            



            prefix = uri;
        } else {
            



            n = inScopeNSes->length;
            while (n != 0) {
                --n;
                ns = XMLARRAY_MEMBER(inScopeNSes, n, JSObject);
                nsprefix = ns->getNamePrefix();
                if (!nsprefix || nsprefix->empty()) {
                    uri = ns->getNameURI();
                    break;
                }
            }
            prefix = uri->empty() ? parser->context->runtime->emptyString : NULL;
        }
        localName = str;
    }

    return NewXMLQName(parser->context, uri, prefix, localName);
}

static JSString *
ChompXMLWhitespace(JSContext *cx, JSString *str)
{
    size_t length, newlength, offset;
    const jschar *cp, *start, *end;
    jschar c;

    length = str->length();
    start = str->getChars(cx);
    if (!start)
        return NULL;

    for (cp = start, end = cp + length; cp < end; cp++) {
        c = *cp;
        if (!unicode::IsXMLSpace(c))
            break;
    }
    while (end > cp) {
        c = end[-1];
        if (!unicode::IsXMLSpace(c))
            break;
        --end;
    }
    newlength = end - cp;
    if (newlength == length)
        return str;
    offset = cp - start;
    return js_NewDependentString(cx, str, offset, newlength);
}

static JSXML *
ParseNodeToXML(Parser *parser, ParseNode *pn,
               JSXMLArray<JSObject> *inScopeNSes, unsigned flags)
{
    JSContext *cx = parser->context;
    JSXML *xml, *kid, *attr, *attrj;
    JSLinearString *str;
    uint32_t length, n, i, j;
    ParseNode *pn2, *pn3, *head, **pnp;
    JSObject *ns;
    JSObject *qn, *attrjqn;
    JSXMLClass xml_class;
    int stackDummy;

    if (!JS_CHECK_STACK_SIZE(cx->runtime->nativeStackLimit, &stackDummy)) {
        ReportCompileErrorNumber(cx, &parser->tokenStream, pn, JSREPORT_ERROR,
                                 JSMSG_OVER_RECURSED);
        return NULL;
    }

#define PN2X_SKIP_CHILD ((JSXML *) 1)

    




    xml = NULL;
    if (!js_EnterLocalRootScope(cx))
        return NULL;
    switch (pn->getKind()) {
      case PNK_XMLELEM:
        length = inScopeNSes->length;
        pn2 = pn->pn_head;
        xml = ParseNodeToXML(parser, pn2, inScopeNSes, flags);
        if (!xml)
            goto fail;

        n = pn->pn_count;
        JS_ASSERT(n >= 2);
        n -= 2;
        if (!xml->xml_kids.setCapacity(cx, n))
            goto fail;

        i = 0;
        while ((pn2 = pn2->pn_next) != NULL) {
            if (!pn2->pn_next) {
                
                JS_ASSERT(pn2->isKind(PNK_XMLETAGO));
                break;
            }

            if ((flags & XSF_IGNORE_WHITESPACE) &&
                n > 1 && pn2->isKind(PNK_XMLSPACE)) {
                --n;
                continue;
            }

            kid = ParseNodeToXML(parser, pn2, inScopeNSes, flags);
            if (kid == PN2X_SKIP_CHILD) {
                --n;
                continue;
            }

            if (!kid)
                goto fail;

            
            XMLARRAY_SET_MEMBER(&xml->xml_kids, i, kid);
            kid->parent = xml;
            ++i;

            
            if ((flags & XSF_IGNORE_WHITESPACE) &&
                n > 1 && kid->xml_class == JSXML_CLASS_TEXT) {
                JSString *str = ChompXMLWhitespace(cx, kid->xml_value);
                if (!str)
                    goto fail;
                kid->xml_value = str;
            }
        }

        JS_ASSERT(i == n);
        if (n < pn->pn_count - 2)
            xml->xml_kids.trim();
        XMLARRAY_TRUNCATE(cx, inScopeNSes, length);
        break;

      case PNK_XMLLIST:
        xml = js_NewXML(cx, JSXML_CLASS_LIST);
        if (!xml)
            goto fail;

        n = pn->pn_count;
        if (!xml->xml_kids.setCapacity(cx, n))
            goto fail;

        i = 0;
        for (pn2 = pn->pn_head; pn2; pn2 = pn2->pn_next) {
            




            if (pn2->isKind(PNK_XMLSPACE)) {
                --n;
                continue;
            }

            kid = ParseNodeToXML(parser, pn2, inScopeNSes, flags);
            if (kid == PN2X_SKIP_CHILD) {
                --n;
                continue;
            }

            if (!kid)
                goto fail;

            XMLARRAY_SET_MEMBER(&xml->xml_kids, i, kid);
            ++i;
        }

        if (n < pn->pn_count)
            xml->xml_kids.trim();
        break;

      case PNK_XMLSTAGO:
      case PNK_XMLPTAGC:
        length = inScopeNSes->length;
        pn2 = pn->pn_head;
        JS_ASSERT(pn2->isKind(PNK_XMLNAME));
        if (pn2->isArity(PN_LIST))
            goto syntax;

        xml = js_NewXML(cx, JSXML_CLASS_ELEMENT);
        if (!xml)
            goto fail;

        
        JS_ASSERT(pn->pn_count >= 1);
        n = pn->pn_count - 1;
        pnp = &pn2->pn_next;
        head = *pnp;
        while ((pn2 = *pnp) != NULL) {
            size_t length;
            const jschar *chars;

            if (!pn2->isKind(PNK_XMLNAME) || !pn2->isArity(PN_NULLARY))
                goto syntax;

            
            for (pn3 = head; pn3 != pn2; pn3 = pn3->pn_next->pn_next) {
                if (pn3->pn_atom == pn2->pn_atom) {
                    Value v = StringValue(pn2->pn_atom);
                    JSAutoByteString bytes;
                    if (js_ValueToPrintable(cx, v, &bytes)) {
                        ReportCompileErrorNumber(cx, &parser->tokenStream, pn2,
                                                 JSREPORT_ERROR, JSMSG_DUPLICATE_XML_ATTR,
                                                 bytes.ptr());
                    }
                    goto fail;
                }
            }

            JSAtom *atom = pn2->pn_atom;
            pn2 = pn2->pn_next;
            JS_ASSERT(pn2);
            if (!pn2->isKind(PNK_XMLATTR))
                goto syntax;

            chars = atom->chars();
            length = atom->length();
            if (length >= 5 &&
                IS_XMLNS_CHARS(chars) &&
                (length == 5 || chars[5] == ':')) {
                JSLinearString *uri, *prefix;

                uri = pn2->pn_atom;
                if (length == 5) {
                    
                    prefix = cx->runtime->emptyString;
                } else {
                    prefix = js_NewStringCopyN(cx, chars + 6, length - 6);
                    if (!prefix)
                        goto fail;
                }

                





                ns = NewXMLNamespace(cx, prefix, uri, JS_TRUE);
                if (!ns)
                    goto fail;

                






                if (!XMLARRAY_HAS_MEMBER(inScopeNSes, ns, namespace_identity)) {
                    if (!XMLARRAY_APPEND(cx, inScopeNSes, ns) ||
                        !XMLARRAY_APPEND(cx, &xml->xml_namespaces, ns)) {
                        goto fail;
                    }
                }

                JS_ASSERT(n >= 2);
                n -= 2;
                *pnp = pn2->pn_next;
                
                continue;
            }

            pnp = &pn2->pn_next;
        }

        xml->xml_namespaces.trim();

        
        pn2 = pn->pn_head;
        qn = ParseNodeToQName(parser, pn2, inScopeNSes, JS_FALSE);
        if (!qn)
            goto fail;
        xml->name = qn;

        JS_ASSERT((n & 1) == 0);
        n >>= 1;
        if (!xml->xml_attrs.setCapacity(cx, n))
            goto fail;

        for (i = 0; (pn2 = pn2->pn_next) != NULL; i++) {
            qn = ParseNodeToQName(parser, pn2, inScopeNSes, JS_TRUE);
            if (!qn) {
                xml->xml_attrs.length = i;
                goto fail;
            }

            



            for (j = 0; j < i; j++) {
                attrj = XMLARRAY_MEMBER(&xml->xml_attrs, j, JSXML);
                attrjqn = attrj->name;
                if (EqualStrings(attrjqn->getNameURI(), qn->getNameURI()) &&
                    EqualStrings(attrjqn->getQNameLocalName(), qn->getQNameLocalName())) {
                    Value v = StringValue(pn2->pn_atom);
                    JSAutoByteString bytes;
                    if (js_ValueToPrintable(cx, v, &bytes)) {
                        ReportCompileErrorNumber(cx, &parser->tokenStream, pn2,
                                                 JSREPORT_ERROR, JSMSG_DUPLICATE_XML_ATTR,
                                                 bytes.ptr());
                    }
                    goto fail;
                }
            }

            pn2 = pn2->pn_next;
            JS_ASSERT(pn2);
            JS_ASSERT(pn2->isKind(PNK_XMLATTR));

            attr = js_NewXML(cx, JSXML_CLASS_ATTRIBUTE);
            if (!attr)
                goto fail;

            XMLARRAY_SET_MEMBER(&xml->xml_attrs, i, attr);
            attr->parent = xml;
            attr->name = qn;
            attr->xml_value = pn2->pn_atom;
        }

        
        if (pn->isKind(PNK_XMLPTAGC))
            XMLARRAY_TRUNCATE(cx, inScopeNSes, length);
        break;

      case PNK_XMLSPACE:
      case PNK_XMLTEXT:
      case PNK_XMLCDATA:
      case PNK_XMLCOMMENT:
      case PNK_XMLPI:
        str = pn->pn_atom;
        qn = NULL;
        if (pn->isKind(PNK_XMLCOMMENT)) {
            if (flags & XSF_IGNORE_COMMENTS)
                goto skip_child;
            xml_class = JSXML_CLASS_COMMENT;
        } else if (pn->isKind(PNK_XMLPI)) {
            XMLProcessingInstruction &pi = pn->asXMLProcessingInstruction();
            if (IS_XML(str)) {
                Value v = StringValue(str);
                JSAutoByteString bytes;
                if (js_ValueToPrintable(cx, v, &bytes)) {
                    ReportCompileErrorNumber(cx, &parser->tokenStream, &pi,
                                             JSREPORT_ERROR, JSMSG_RESERVED_ID, bytes.ptr());
                }
                goto fail;
            }

            if (flags & XSF_IGNORE_PROCESSING_INSTRUCTIONS)
                goto skip_child;

            qn = ParseNodeToQName(parser, &pi, inScopeNSes, JS_FALSE);
            if (!qn)
                goto fail;

            str = pi.data();
            xml_class = JSXML_CLASS_PROCESSING_INSTRUCTION;
        } else {
            
            xml_class = JSXML_CLASS_TEXT;
        }

        xml = js_NewXML(cx, xml_class);
        if (!xml)
            goto fail;
        xml->name = qn;
        if (pn->isKind(PNK_XMLSPACE))
            xml->xml_flags |= XMLF_WHITESPACE_TEXT;
        xml->xml_value = str;
        break;

      default:
        goto syntax;
    }

    js_LeaveLocalRootScopeWithResult(cx, xml);
    return xml;

skip_child:
    js_LeaveLocalRootScope(cx);
    return PN2X_SKIP_CHILD;

#undef PN2X_SKIP_CHILD

syntax:
    ReportCompileErrorNumber(cx, &parser->tokenStream, pn, JSREPORT_ERROR, JSMSG_BAD_XML_MARKUP);
fail:
    js_LeaveLocalRootScope(cx);
    return NULL;
}





static JSBool
GetXMLSetting(JSContext *cx, const char *name, jsval *vp)
{
    jsval v;

    if (!js_FindClassObject(cx, NULL, JSProto_XML, &v))
        return JS_FALSE;
    if (JSVAL_IS_PRIMITIVE(v) || !JSVAL_TO_OBJECT(v)->isFunction()) {
        *vp = JSVAL_VOID;
        return JS_TRUE;
    }
    return JS_GetProperty(cx, JSVAL_TO_OBJECT(v), name, vp);
}

static JSBool
GetBooleanXMLSetting(JSContext *cx, const char *name, JSBool *bp)
{
    jsval v;

    return GetXMLSetting(cx, name, &v) && JS_ValueToBoolean(cx, v, bp);
}

static JSBool
GetUint32XMLSetting(JSContext *cx, const char *name, uint32_t *uip)
{
    jsval v;

    return GetXMLSetting(cx, name, &v) && JS_ValueToECMAUint32(cx, v, uip);
}

static JSBool
GetXMLSettingFlags(JSContext *cx, unsigned *flagsp)
{
    JSBool flag[4];

    if (!GetBooleanXMLSetting(cx, js_ignoreComments_str, &flag[0]) ||
        !GetBooleanXMLSetting(cx, js_ignoreProcessingInstructions_str, &flag[1]) ||
        !GetBooleanXMLSetting(cx, js_ignoreWhitespace_str, &flag[2]) ||
        !GetBooleanXMLSetting(cx, js_prettyPrinting_str, &flag[3])) {
        return false;
    }

    *flagsp = 0;
    for (size_t n = 0; n < 4; ++n)
        if (flag[n])
            *flagsp |= JS_BIT(n);
    return true;
}

static JSObject *
GetCurrentScopeChain(JSContext *cx)
{
    if (cx->hasfp())
        return &cx->fp()->scopeChain();
    return JS_ObjectToInnerObject(cx, cx->globalObject);
}

static JSXML *
ParseXMLSource(JSContext *cx, JSString *src)
{
    jsval nsval;
    JSLinearString *uri;
    size_t urilen, srclen, length, offset, dstlen;
    jschar *chars;
    const jschar *srcp, *endp;
    JSXML *xml;
    const char *filename;
    unsigned lineno;
    JSOp op;

    static const char prefix[] = "<parent xmlns=\"";
    static const char middle[] = "\">";
    static const char suffix[] = "</parent>";

#define constrlen(constr)   (sizeof(constr) - 1)

    if (!js_GetDefaultXMLNamespace(cx, &nsval))
        return NULL;
    uri = JSVAL_TO_OBJECT(nsval)->getNameURI();
    uri = js_EscapeAttributeValue(cx, uri, JS_FALSE);
    if (!uri)
        return NULL;

    urilen = uri->length();
    srclen = src->length();
    length = constrlen(prefix) + urilen + constrlen(middle) + srclen +
             constrlen(suffix);

    chars = (jschar *) cx->malloc_((length + 1) * sizeof(jschar));
    if (!chars)
        return NULL;

    dstlen = length;
    InflateStringToBuffer(cx, prefix, constrlen(prefix), chars, &dstlen);
    offset = dstlen;
    js_strncpy(chars + offset, uri->chars(), urilen);
    offset += urilen;
    dstlen = length - offset + 1;
    InflateStringToBuffer(cx, middle, constrlen(middle), chars + offset, &dstlen);
    offset += dstlen;
    srcp = src->getChars(cx);
    if (!srcp) {
        cx->free_(chars);
        return NULL;
    }
    js_strncpy(chars + offset, srcp, srclen);
    offset += srclen;
    dstlen = length - offset + 1;
    InflateStringToBuffer(cx, suffix, constrlen(suffix), chars + offset, &dstlen);
    chars [offset + dstlen] = 0;

    xml = NULL;
    filename = NULL;
    lineno = 1;
    FrameRegsIter i(cx);
    if (!i.done()) {
        op = (JSOp) *i.pc();
        if (op == JSOP_TOXML || op == JSOP_TOXMLLIST) {
            filename = i.fp()->script()->filename;
            lineno = js_PCToLineNumber(cx, i.fp()->script(), i.pc());
            for (endp = srcp + srclen; srcp < endp; srcp++) {
                if (*srcp == '\n')
                    --lineno;
            }
        }
    }

    {
        Parser parser(cx);
        if (parser.init(chars, length, filename, lineno, cx->findVersion())) {
            JSObject *scopeChain = GetCurrentScopeChain(cx);
            if (!scopeChain) {
                cx->free_(chars);
                return NULL;
            }

            ParseNode *pn = parser.parseXMLText(scopeChain, false);
            unsigned flags;
            if (pn && GetXMLSettingFlags(cx, &flags)) {
                AutoNamespaceArray namespaces(cx);
                if (namespaces.array.setCapacity(cx, 1))
                    xml = ParseNodeToXML(&parser, pn, &namespaces.array, flags);
            }
        }
    }

    cx->free_(chars);
    return xml;

#undef constrlen
}




















static JSXML *
OrphanXMLChild(JSContext *cx, JSXML *xml, uint32_t i)
{
    JSObject *ns;

    ns = XMLARRAY_MEMBER(&xml->xml_namespaces, 0, JSObject);
    xml = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
    if (!ns || !xml)
        return xml;
    if (xml->xml_class == JSXML_CLASS_ELEMENT) {
        if (!XMLARRAY_APPEND(cx, &xml->xml_namespaces, ns))
            return NULL;
        ns->setNamespaceDeclared(JSVAL_VOID);
    }
    xml->parent = NULL;
    return xml;
}

static JSObject *
ToXML(JSContext *cx, jsval v)
{
    JSObject *obj;
    JSXML *xml;
    Class *clasp;
    JSString *str;
    uint32_t length;

    if (JSVAL_IS_PRIMITIVE(v)) {
        if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v))
            goto bad;
    } else {
        obj = JSVAL_TO_OBJECT(v);
        if (obj->isXML()) {
            xml = (JSXML *) obj->getPrivate();
            if (xml->xml_class == JSXML_CLASS_LIST) {
                if (xml->xml_kids.length != 1)
                    goto bad;
                xml = XMLARRAY_MEMBER(&xml->xml_kids, 0, JSXML);
                if (xml) {
                    JS_ASSERT(xml->xml_class != JSXML_CLASS_LIST);
                    return js_GetXMLObject(cx, xml);
                }
            }
            return obj;
        }

        clasp = obj->getClass();
        if (clasp->flags & JSCLASS_DOCUMENT_OBSERVER) {
            JS_ASSERT(0);
        }

        if (clasp != &StringClass &&
            clasp != &NumberClass &&
            clasp != &BooleanClass) {
            goto bad;
        }
    }

    str = ToString(cx, v);
    if (!str)
        return NULL;
    if (str->empty()) {
        length = 0;
#ifdef __GNUC__         
        xml = NULL;
#endif
    } else {
        xml = ParseXMLSource(cx, str);
        if (!xml)
            return NULL;
        length = JSXML_LENGTH(xml);
    }

    if (length == 0) {
        obj = js_NewXMLObject(cx, JSXML_CLASS_TEXT);
        if (!obj)
            return NULL;
    } else if (length == 1) {
        xml = OrphanXMLChild(cx, xml, 0);
        if (!xml)
            return NULL;
        obj = js_GetXMLObject(cx, xml);
        if (!obj)
            return NULL;
    } else {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_SYNTAX_ERROR);
        return NULL;
    }
    return obj;

bad:
    js_ReportValueError(cx, JSMSG_BAD_XML_CONVERSION,
                        JSDVG_IGNORE_STACK, v, NULL);
    return NULL;
}

static JSBool
Append(JSContext *cx, JSXML *list, JSXML *kid);

static JSObject *
ToXMLList(JSContext *cx, jsval v)
{
    JSObject *obj, *listobj;
    JSXML *xml, *list, *kid;
    Class *clasp;
    JSString *str;
    uint32_t i, length;

    if (JSVAL_IS_PRIMITIVE(v)) {
        if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v))
            goto bad;
    } else {
        obj = JSVAL_TO_OBJECT(v);
        if (obj->isXML()) {
            xml = (JSXML *) obj->getPrivate();
            if (xml->xml_class != JSXML_CLASS_LIST) {
                listobj = js_NewXMLObject(cx, JSXML_CLASS_LIST);
                if (!listobj)
                    return NULL;
                list = (JSXML *) listobj->getPrivate();
                if (!Append(cx, list, xml))
                    return NULL;
                return listobj;
            }
            return obj;
        }

        clasp = obj->getClass();
        if (clasp->flags & JSCLASS_DOCUMENT_OBSERVER) {
            JS_ASSERT(0);
        }

        if (clasp != &StringClass &&
            clasp != &NumberClass &&
            clasp != &BooleanClass) {
            goto bad;
        }
    }

    str = ToString(cx, v);
    if (!str)
        return NULL;
    if (str->empty()) {
        xml = NULL;
        length = 0;
    } else {
        if (!js_EnterLocalRootScope(cx))
            return NULL;
        xml = ParseXMLSource(cx, str);
        if (!xml) {
            js_LeaveLocalRootScope(cx);
            return NULL;
        }
        length = JSXML_LENGTH(xml);
    }

    listobj = js_NewXMLObject(cx, JSXML_CLASS_LIST);
    if (listobj) {
        list = (JSXML *) listobj->getPrivate();
        for (i = 0; i < length; i++) {
            kid = OrphanXMLChild(cx, xml, i);
            if (!kid || !Append(cx, list, kid)) {
                listobj = NULL;
                break;
            }
        }
    }

    if (xml)
        js_LeaveLocalRootScopeWithResult(cx, listobj);
    return listobj;

bad:
    js_ReportValueError(cx, JSMSG_BAD_XMLLIST_CONVERSION,
                        JSDVG_IGNORE_STACK, v, NULL);
    return NULL;
}









static JSFlatString *
MakeXMLSpecialString(JSContext *cx, StringBuffer &sb,
                     JSString *str, JSString *str2,
                     const jschar *prefix, size_t prefixlength,
                     const jschar *suffix, size_t suffixlength)
{
    if (!sb.append(prefix, prefixlength) || !sb.append(str))
        return NULL;
    if (str2 && !str2->empty()) {
        if (!sb.append(' ') || !sb.append(str2))
            return NULL;
    }
    if (!sb.append(suffix, suffixlength))
        return NULL;

    return sb.finishString();
}

static JSFlatString *
MakeXMLCDATAString(JSContext *cx, StringBuffer &sb, JSString *str)
{
    static const jschar cdata_prefix_ucNstr[] = {'<', '!', '[',
                                                 'C', 'D', 'A', 'T', 'A',
                                                 '['};
    static const jschar cdata_suffix_ucNstr[] = {']', ']', '>'};

    return MakeXMLSpecialString(cx, sb, str, NULL,
                                cdata_prefix_ucNstr, 9,
                                cdata_suffix_ucNstr, 3);
}

static JSFlatString *
MakeXMLCommentString(JSContext *cx, StringBuffer &sb, JSString *str)
{
    static const jschar comment_prefix_ucNstr[] = {'<', '!', '-', '-'};
    static const jschar comment_suffix_ucNstr[] = {'-', '-', '>'};

    return MakeXMLSpecialString(cx, sb, str, NULL,
                                comment_prefix_ucNstr, 4,
                                comment_suffix_ucNstr, 3);
}

static JSFlatString *
MakeXMLPIString(JSContext *cx, StringBuffer &sb, JSString *name,
                JSString *value)
{
    static const jschar pi_prefix_ucNstr[] = {'<', '?'};
    static const jschar pi_suffix_ucNstr[] = {'?', '>'};

    return MakeXMLSpecialString(cx, sb, name, value,
                                pi_prefix_ucNstr, 2,
                                pi_suffix_ucNstr, 2);
}






static bool
EscapeAttributeValueBuffer(JSContext *cx, StringBuffer &sb, JSString *str, JSBool quote)
{
    size_t length = str->length();
    const jschar *start = str->getChars(cx);
    if (!start)
        return false;

    if (quote && !sb.append('"'))
        return false;

    for (const jschar *cp = start, *end = start + length; cp != end; ++cp) {
        jschar c = *cp;
        switch (c) {
          case '"':
            if (!sb.append(js_quot_entity_str))
                return false;
            break;
          case '<':
            if (!sb.append(js_lt_entity_str))
                return false;
            break;
          case '&':
            if (!sb.append(js_amp_entity_str))
                return false;
            break;
          case '\n':
            if (!sb.append("&#xA;"))
                return false;
            break;
          case '\r':
            if (!sb.append("&#xD;"))
                return false;
            break;
          case '\t':
            if (!sb.append("&#x9;"))
                return false;
            break;
          default:
            if (!sb.append(c))
                return false;
        }
    }

    if (quote && !sb.append('"'))
        return false;

    return true;
}






static JSFlatString *
EscapeAttributeValue(JSContext *cx, StringBuffer &sb, JSString *str, JSBool quote)
{
    if (!EscapeAttributeValueBuffer(cx, sb, str, quote))
        return NULL;
    return sb.finishString();
}





static bool
AppendAttributeValue(JSContext *cx, StringBuffer &sb, JSString *valstr)
{
    if (!sb.append('='))
        return false;
    return EscapeAttributeValueBuffer(cx, sb, valstr, JS_TRUE);
}






static JSFlatString *
EscapeElementValue(JSContext *cx, StringBuffer &sb, JSString *str, uint32_t toSourceFlag)
{
    size_t length = str->length();
    const jschar *start = str->getChars(cx);
    if (!start)
        return NULL;

    for (const jschar *cp = start, *end = start + length; cp != end; ++cp) {
        jschar c = *cp;
        switch (*cp) {
          case '<':
            if (!sb.append(js_lt_entity_str))
                return NULL;
            break;
          case '>':
            if (!sb.append(js_gt_entity_str))
                return NULL;
            break;
          case '&':
            if (!sb.append(js_amp_entity_str))
                return NULL;
            break;
          case '{':
            



            if (toSourceFlag) {
                if (!sb.append(js_leftcurly_entity_str))
                    return NULL;
                break;
            }
            
          default:
            if (!sb.append(c))
                return NULL;
        }
    }
    return sb.finishString();
}


static JSObject *
GetNamespace(JSContext *cx, JSObject *qn, const JSXMLArray<JSObject> *inScopeNSes)
{
    JSLinearString *uri, *prefix, *nsprefix;
    JSObject *match, *ns;
    uint32_t i, n;
    jsval argv[2];

    uri = qn->getNameURI();
    prefix = qn->getNamePrefix();
    JS_ASSERT(uri);
    if (!uri) {
        JSAutoByteString bytes;
        const char *s = !prefix ?
                        js_undefined_str
                        : js_ValueToPrintable(cx, StringValue(prefix), &bytes);
        if (s)
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_XML_NAMESPACE, s);
        return NULL;
    }

    
    match = NULL;
    if (inScopeNSes) {
        for (i = 0, n = inScopeNSes->length; i < n; i++) {
            ns = XMLARRAY_MEMBER(inScopeNSes, i, JSObject);
            if (!ns)
                continue;

            


























            if (EqualStrings(ns->getNameURI(), uri)) {
                nsprefix = ns->getNamePrefix();
                if (nsprefix == prefix ||
                    ((nsprefix && prefix)
                     ? EqualStrings(nsprefix, prefix)
                     : (nsprefix ? nsprefix : prefix)->empty())) {
                    match = ns;
                    break;
                }
            }
        }
    }

    
    if (!match) {
        argv[0] = prefix ? STRING_TO_JSVAL(prefix) : JSVAL_VOID;
        argv[1] = STRING_TO_JSVAL(uri);
        ns = JS_ConstructObjectWithArguments(cx, Jsvalify(&NamespaceClass), NULL, 2, argv);
        if (!ns)
            return NULL;
        match = ns;
    }
    return match;
}

static JSLinearString *
GeneratePrefix(JSContext *cx, JSLinearString *uri, JSXMLArray<JSObject> *decls)
{
    const jschar *cp, *start, *end;
    size_t length, newlength, offset;
    uint32_t i, n, m, serial;
    jschar *bp, *dp;
    JSBool done;
    JSObject *ns;
    JSLinearString *nsprefix, *prefix;

    JS_ASSERT(!uri->empty());

    










    if (decls->length == 0)
        return js_NewStringCopyZ(cx, "a");

    





    start = uri->chars();
    end = start + uri->length();
    cp = end;
    while (--cp > start) {
        if (*cp == '.' || *cp == '/' || *cp == ':') {
            ++cp;
            length = end - cp;
            if (IsXMLName(cp, length) && !STARTS_WITH_XML(cp, length))
                break;
            end = --cp;
        }
    }
    length = end - cp;

    






    bp = (jschar *) cp;
    newlength = length;
    if (STARTS_WITH_XML(cp, length) || !IsXMLName(cp, length)) {
        newlength = length + 2 + (size_t) log10((double) decls->length);
        bp = (jschar *)
             cx->malloc_((newlength + 1) * sizeof(jschar));
        if (!bp)
            return NULL;

        bp[newlength] = 0;
        for (i = 0; i < newlength; i++)
             bp[i] = 'a';
    }

    



    serial = 0;
    do {
        done = JS_TRUE;
        for (i = 0, n = decls->length; i < n; i++) {
            ns = XMLARRAY_MEMBER(decls, i, JSObject);
            if (ns && (nsprefix = ns->getNamePrefix()) &&
                nsprefix->length() == newlength &&
                !memcmp(nsprefix->chars(), bp,
                        newlength * sizeof(jschar))) {
                if (bp == cp) {
                    newlength = length + 2 + (size_t) log10((double) n);
                    bp = (jschar *)
                         cx->malloc_((newlength + 1) * sizeof(jschar));
                    if (!bp)
                        return NULL;
                    js_strncpy(bp, cp, length);
                }

                ++serial;
                JS_ASSERT(serial <= n);
                dp = bp + length + 2 + (size_t) log10((double) serial);
                *dp = 0;
                for (m = serial; m != 0; m /= 10)
                    *--dp = (jschar)('0' + m % 10);
                *--dp = '-';
                JS_ASSERT(dp == bp + length);

                done = JS_FALSE;
                break;
            }
        }
    } while (!done);

    if (bp == cp) {
        offset = cp - start;
        prefix = js_NewDependentString(cx, uri, offset, length);
    } else {
        prefix = js_NewString(cx, bp, newlength);
        if (!prefix)
            cx->free_(bp);
    }
    return prefix;
}

static JSBool
namespace_match(const JSObject *nsa, const JSObject *nsb)
{
    JSLinearString *prefixa, *prefixb = nsb->getNamePrefix();

    if (prefixb) {
        prefixa = nsa->getNamePrefix();
        return prefixa && EqualStrings(prefixa, prefixb);
    }
    return EqualStrings(nsa->getNameURI(), nsb->getNameURI());
}


#define TO_SOURCE_FLAG 0x80000000

static JSString *
XMLToXMLString(JSContext *cx, JSXML *xml, const JSXMLArray<JSObject> *ancestorNSes,
               uint32_t indentLevel, JSBool pretty)
{
    JSBool indentKids;
    StringBuffer sb(cx);
    JSString *str;
    JSLinearString *prefix, *nsuri;
    uint32_t i, n, nextIndentLevel;
    JSObject *ns, *ns2;
    AutoNamespaceArray empty(cx), decls(cx), ancdecls(cx);

    if (pretty) {
        if (!sb.appendN(' ', indentLevel & ~TO_SOURCE_FLAG))
            return NULL;
    }

    str = NULL;

    switch (xml->xml_class) {
      case JSXML_CLASS_TEXT:
        
        if (pretty) {
            str = ChompXMLWhitespace(cx, xml->xml_value);
            if (!str)
                return NULL;
        } else {
            str = xml->xml_value;
        }
        return EscapeElementValue(cx, sb, str, indentLevel & TO_SOURCE_FLAG);

      case JSXML_CLASS_ATTRIBUTE:
        
        return EscapeAttributeValue(cx, sb, xml->xml_value,
                                    (indentLevel & TO_SOURCE_FLAG) != 0);

      case JSXML_CLASS_COMMENT:
        
        return MakeXMLCommentString(cx, sb, xml->xml_value);

      case JSXML_CLASS_PROCESSING_INSTRUCTION:
        
        return MakeXMLPIString(cx, sb, xml->name->getQNameLocalName(),
                               xml->xml_value);

      case JSXML_CLASS_LIST:
        
        {
            JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
            i = 0;
            while (JSXML *kid = cursor.getNext()) {
                if (pretty && i != 0) {
                    if (!sb.append('\n'))
                        return NULL;
                }

                JSString *kidstr = XMLToXMLString(cx, kid, ancestorNSes, indentLevel, pretty);
                if (!kidstr || !sb.append(kidstr))
                    return NULL;
                ++i;
            }
        }

        if (sb.empty())
            return cx->runtime->emptyString;
        return sb.finishString();

      default:;
    }

    
    if (!js_EnterLocalRootScope(cx))
        return NULL;

    
    if (!ancestorNSes) {
        
        
        
        
        JSObject *emptyns = NewXMLNamespace(cx, cx->runtime->emptyString, cx->runtime->emptyString, JS_FALSE);
        if (!emptyns || !XMLARRAY_APPEND(cx, &empty.array, emptyns))
            goto out;
        ancestorNSes = &empty.array;
    }

    
    {
        JSXMLArrayCursor<JSObject> cursor(&xml->xml_namespaces);
        while ((ns = cursor.getNext()) != NULL) {
            if (!IsDeclared(ns))
                continue;
            if (!XMLARRAY_HAS_MEMBER(ancestorNSes, ns, namespace_identity)) {
                
                ns2 = NewXMLNamespace(cx, ns->getNamePrefix(), ns->getNameURI(), JS_TRUE);
                if (!ns2 || !XMLARRAY_APPEND(cx, &decls.array, ns2))
                    goto out;
            }
        }
    }

    





    if (!ancdecls.array.setCapacity(cx, ancestorNSes->length + decls.length()))
        goto out;
    for (i = 0, n = ancestorNSes->length; i < n; i++) {
        ns2 = XMLARRAY_MEMBER(ancestorNSes, i, JSObject);
        if (!ns2)
            continue;
        JS_ASSERT(!XMLARRAY_HAS_MEMBER(&decls.array, ns2, namespace_identity));
        if (!XMLARRAY_APPEND(cx, &ancdecls.array, ns2))
            goto out;
    }
    for (i = 0, n = decls.length(); i < n; i++) {
        ns2 = XMLARRAY_MEMBER(&decls.array, i, JSObject);
        if (!ns2)
            continue;
        JS_ASSERT(!XMLARRAY_HAS_MEMBER(&ancdecls.array, ns2, namespace_identity));
        if (!XMLARRAY_APPEND(cx, &ancdecls.array, ns2))
            goto out;
    }

    
    ns = GetNamespace(cx, xml->name, &ancdecls.array);
    if (!ns)
        goto out;

    
    prefix = ns->getNamePrefix();
    if (!prefix) {
        













        nsuri = ns->getNameURI();
        if (!xml->name->getNamePrefix()) {
            prefix = cx->runtime->emptyString;
        } else {
            prefix = GeneratePrefix(cx, nsuri, &ancdecls.array);
            if (!prefix)
                goto out;
        }
        ns = NewXMLNamespace(cx, prefix, nsuri, JS_TRUE);
        if (!ns)
            goto out;

        












        if (prefix->empty()) {
            i = XMLArrayFindMember(&decls.array, ns, namespace_match);
            if (i != XML_NOT_FOUND)
                XMLArrayDelete(cx, &decls.array, i, JS_TRUE);
        }

        






        if (!XMLARRAY_APPEND(cx, &ancdecls.array, ns) ||
            !XMLARRAY_APPEND(cx, &decls.array, ns)) {
            goto out;
        }
    }

    
    if (!sb.append('<'))
        goto out;

    if (!prefix->empty()) {
        if (!sb.append(prefix) || !sb.append(':'))
            goto out;
    }
    if (!sb.append(xml->name->getQNameLocalName()))
        goto out;

    





    
    {
        JSXMLArrayCursor<JSXML> cursor(&xml->xml_attrs);
        while (JSXML *attr = cursor.getNext()) {
            if (!sb.append(' '))
                goto out;
            ns2 = GetNamespace(cx, attr->name, &ancdecls.array);
            if (!ns2)
                goto out;

            
            prefix = ns2->getNamePrefix();
            if (!prefix) {
                prefix = GeneratePrefix(cx, ns2->getNameURI(), &ancdecls.array);
                if (!prefix)
                    goto out;

                
                ns2 = NewXMLNamespace(cx, prefix, ns2->getNameURI(), JS_TRUE);
                if (!ns2)
                    goto out;

                






                if (!XMLARRAY_APPEND(cx, &ancdecls.array, ns2) ||
                    !XMLARRAY_APPEND(cx, &decls.array, ns2)) {
                    goto out;
                }
            }

            
            if (!prefix->empty()) {
                if (!sb.append(prefix) || !sb.append(':'))
                    goto out;
            }

            
            if (!sb.append(attr->name->getQNameLocalName()))
                goto out;

            
            if (!AppendAttributeValue(cx, sb, attr->xml_value))
                goto out;
        }
    }

    
    {
        JSXMLArrayCursor<JSObject> cursor(&decls.array);
        while (JSObject *ns3 = cursor.getNext()) {
            JS_ASSERT(IsDeclared(ns3));

            if (!sb.append(" xmlns"))
                goto out;

            
            prefix = ns3->getNamePrefix();
            if (!prefix) {
                prefix = GeneratePrefix(cx, ns3->getNameURI(), &ancdecls.array);
                if (!prefix)
                    goto out;
                ns3->setNamePrefix(prefix);
            }

            
            if (!prefix->empty()) {
                if (!sb.append(':') || !sb.append(prefix))
                    goto out;
            }

            
            if (!AppendAttributeValue(cx, sb, ns3->getNameURI()))
                goto out;
        }
    }

    
    n = xml->xml_kids.length;
    if (n == 0) {
        if (!sb.append("/>"))
            goto out;
    } else {
        
        if (!sb.append('>'))
            goto out;
        {
            JSXML *kid;
            indentKids = n > 1 ||
                         (n == 1 &&
                          (kid = XMLARRAY_MEMBER(&xml->xml_kids, 0, JSXML)) &&
                          kid->xml_class != JSXML_CLASS_TEXT);
        }

        if (pretty && indentKids) {
            if (!GetUint32XMLSetting(cx, js_prettyIndent_str, &i))
                goto out;
            nextIndentLevel = indentLevel + i;
        } else {
            nextIndentLevel = indentLevel & TO_SOURCE_FLAG;
        }

        {
            JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
            while (JSXML *kid = cursor.getNext()) {
                if (pretty && indentKids) {
                    if (!sb.append('\n'))
                        goto out;
                }

                JSString *kidstr = XMLToXMLString(cx, kid, &ancdecls.array, nextIndentLevel, pretty);
                if (!kidstr)
                    goto out;

                if (!sb.append(kidstr))
                    goto out;
            }
        }

        if (pretty && indentKids) {
            if (!sb.append('\n') ||
                !sb.appendN(' ', indentLevel & ~TO_SOURCE_FLAG))
                goto out;
        }
        if (!sb.append("</"))
            goto out;

        
        prefix = ns->getNamePrefix();
        if (prefix && !prefix->empty()) {
            if (!sb.append(prefix) || !sb.append(':'))
                goto out;
        }

        
        if (!sb.append(xml->name->getQNameLocalName()) || !sb.append('>'))
            goto out;
    }

    str = sb.finishString();
out:
    js_LeaveLocalRootScopeWithResult(cx, str);
    return str;
}


static JSString *
ToXMLString(JSContext *cx, jsval v, uint32_t toSourceFlag)
{
    if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_XML_CONVERSION,
                             JSVAL_IS_NULL(v) ? js_null_str : js_undefined_str);
        return NULL;
    }

    if (JSVAL_IS_BOOLEAN(v) || JSVAL_IS_NUMBER(v))
        return ToString(cx, v);

    if (JSVAL_IS_STRING(v)) {
        StringBuffer sb(cx);
        return EscapeElementValue(cx, sb, JSVAL_TO_STRING(v), toSourceFlag);
    }

    JSObject *obj = JSVAL_TO_OBJECT(v);
    if (!obj->isXML()) {
        if (!ToPrimitive(cx, JSTYPE_STRING, &v))
            return NULL;
        JSString *str = ToString(cx, v);
        if (!str)
            return NULL;
        StringBuffer sb(cx);
        return EscapeElementValue(cx, sb, str, toSourceFlag);
    }

    JSBool pretty;
    if (!GetBooleanXMLSetting(cx, js_prettyPrinting_str, &pretty))
        return NULL;

    
    JS::Anchor<JSObject *> anch(obj);
    JSXML *xml = reinterpret_cast<JSXML *>(obj->getPrivate());
    return XMLToXMLString(cx, xml, NULL, toSourceFlag | 0, pretty);
}

static JSObject *
ToAttributeName(JSContext *cx, jsval v)
{
    JSLinearString *uri, *prefix;
    JSObject *obj;
    Class *clasp;
    JSObject *qn;

    JSAtom *name;
    if (JSVAL_IS_STRING(v)) {
        if (!js_ValueToAtom(cx, v, &name))
            return NULL;
        uri = prefix = cx->runtime->emptyString;
    } else {
        if (JSVAL_IS_PRIMITIVE(v)) {
            js_ReportValueError(cx, JSMSG_BAD_XML_ATTR_NAME,
                                JSDVG_IGNORE_STACK, v, NULL);
            return NULL;
        }

        obj = JSVAL_TO_OBJECT(v);
        clasp = obj->getClass();
        if (clasp == &AttributeNameClass)
            return obj;

        if (clasp == &QNameClass) {
            qn = obj;
            uri = qn->getNameURI();
            prefix = qn->getNamePrefix();
            name = qn->getQNameLocalName();
        } else {
            if (clasp == &AnyNameClass) {
                name = cx->runtime->atomState.starAtom;
            } else {
                if (!js_ValueToAtom(cx, v, &name))
                    return NULL;
            }
            uri = prefix = cx->runtime->emptyString;
        }
    }

    qn = NewXMLAttributeName(cx, uri, prefix, name);
    if (!qn)
        return NULL;
    return qn;
}

static void
ReportBadXMLName(JSContext *cx, const Value &idval)
{
    js_ReportValueError(cx, JSMSG_BAD_XML_NAME, JSDVG_IGNORE_STACK, idval, NULL);
}

namespace js {

bool
GetLocalNameFromFunctionQName(JSObject *qn, JSAtom **namep, JSContext *cx)
{
    JSAtom *atom = cx->runtime->atomState.functionNamespaceURIAtom;
    JSLinearString *uri = qn->getNameURI();
    if (uri && (uri == atom || EqualStrings(uri, atom))) {
        *namep = qn->getQNameLocalName();
        return true;
    }
    return false;
}

} 

bool
js_GetLocalNameFromFunctionQName(JSObject *obj, jsid *funidp, JSContext *cx)
{
    if (!obj->isQName())
        return false;
    JSAtom *name;
    if (GetLocalNameFromFunctionQName(obj, &name, cx)) {
        *funidp = ATOM_TO_JSID(name);
        return true;
    }
    return false;
}

static JSObject *
ToXMLName(JSContext *cx, jsval v, jsid *funidp)
{
    JSAtom *atomizedName;
    JSString *name;
    JSObject *obj;
    Class *clasp;
    uint32_t index;

    if (JSVAL_IS_STRING(v)) {
        name = JSVAL_TO_STRING(v);
    } else {
        if (JSVAL_IS_PRIMITIVE(v)) {
            ReportBadXMLName(cx, v);
            return NULL;
        }

        obj = JSVAL_TO_OBJECT(v);
        clasp = obj->getClass();
        if (clasp == &AttributeNameClass || clasp == &QNameClass)
            goto out;
        if (clasp == &AnyNameClass) {
            name = cx->runtime->atomState.starAtom;
            goto construct;
        }
        name = ToStringSlow(cx, v);
        if (!name)
            return NULL;
    }

    atomizedName = js_AtomizeString(cx, name);
    if (!atomizedName)
        return NULL;

    












    if (js_IdIsIndex(ATOM_TO_JSID(atomizedName), &index))
        goto bad;

    if (*atomizedName->chars() == '@') {
        name = js_NewDependentString(cx, name, 1, name->length() - 1);
        if (!name)
            return NULL;
        *funidp = JSID_VOID;
        return ToAttributeName(cx, STRING_TO_JSVAL(name));
    }

construct:
    v = STRING_TO_JSVAL(name);
    obj = JS_ConstructObjectWithArguments(cx, Jsvalify(&QNameClass), NULL, 1, &v);
    if (!obj)
        return NULL;

out:
    JSAtom *localName;
    *funidp = GetLocalNameFromFunctionQName(obj, &localName, cx)
              ? ATOM_TO_JSID(localName)
              : JSID_VOID;
    return obj;

bad:
    JSAutoByteString bytes;
    if (js_ValueToPrintable(cx, StringValue(name), &bytes))
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_XML_NAME, bytes.ptr());
    return NULL;
}


static JSBool
AddInScopeNamespace(JSContext *cx, JSXML *xml, JSObject *ns)
{
    JSLinearString *prefix, *prefix2;
    JSObject *match, *ns2;
    uint32_t i, n, m;

    if (xml->xml_class != JSXML_CLASS_ELEMENT)
        return JS_TRUE;

    
    prefix = ns->getNamePrefix();
    if (!prefix) {
        match = NULL;
        for (i = 0, n = xml->xml_namespaces.length; i < n; i++) {
            ns2 = XMLARRAY_MEMBER(&xml->xml_namespaces, i, JSObject);
            if (ns2 && EqualStrings(ns2->getNameURI(), ns->getNameURI())) {
                match = ns2;
                break;
            }
        }
        if (!match && !XMLARRAY_ADD_MEMBER(cx, &xml->xml_namespaces, n, ns))
            return JS_FALSE;
    } else {
        if (prefix->empty() && xml->name->getNameURI()->empty())
            return JS_TRUE;
        match = NULL;
#ifdef __GNUC__         
        m = XML_NOT_FOUND;
#endif
        for (i = 0, n = xml->xml_namespaces.length; i < n; i++) {
            ns2 = XMLARRAY_MEMBER(&xml->xml_namespaces, i, JSObject);
            if (ns2 && (prefix2 = ns2->getNamePrefix()) &&
                EqualStrings(prefix2, prefix)) {
                match = ns2;
                m = i;
                break;
            }
        }
        if (match && !EqualStrings(match->getNameURI(), ns->getNameURI())) {
            ns2 = XMLARRAY_DELETE(cx, &xml->xml_namespaces, m, JS_TRUE,
                                  JSObject);
            JS_ASSERT(ns2 == match);
            match->clearNamePrefix();
            if (!AddInScopeNamespace(cx, xml, match))
                return JS_FALSE;
        }
        if (!XMLARRAY_APPEND(cx, &xml->xml_namespaces, ns))
            return JS_FALSE;
    }

    
    return JS_TRUE;
}


static JSBool
Append(JSContext *cx, JSXML *list, JSXML *xml)
{
    JS_ASSERT(list->xml_class == JSXML_CLASS_LIST);

    uint32_t i = list->xml_kids.length;
    if (xml->xml_class == JSXML_CLASS_LIST) {
        list->xml_target = xml->xml_target;
        list->xml_targetprop = xml->xml_targetprop;
        uint32_t n = JSXML_LENGTH(xml);
        if (!list->xml_kids.setCapacity(cx, i + n))
            return JS_FALSE;
        for (uint32_t j = 0; j < n; j++) {
            if (JSXML *kid = XMLARRAY_MEMBER(&xml->xml_kids, j, JSXML))
                XMLARRAY_SET_MEMBER(&list->xml_kids, i + j, kid);
        }
        return JS_TRUE;
    }

    list->xml_target = xml->parent;
    if (xml->xml_class == JSXML_CLASS_PROCESSING_INSTRUCTION)
        list->xml_targetprop = NULL;
    else
        list->xml_targetprop = xml->name;
    if (!XMLARRAY_ADD_MEMBER(cx, &list->xml_kids, i, xml))
        return JS_FALSE;
    return JS_TRUE;
}


static JSXML *
DeepCopyInLRS(JSContext *cx, JSXML *xml, unsigned flags);

static JSXML *
DeepCopy(JSContext *cx, JSXML *xml, JSObject *obj, unsigned flags)
{
    JSXML *copy;

    
    if (!js_EnterLocalRootScope(cx))
        return NULL;
    copy = DeepCopyInLRS(cx, xml, flags);
    if (copy) {
        if (obj) {
            
            obj->setPrivate(copy);
            copy->object = obj;
        } else if (!js_GetXMLObject(cx, copy)) {
            copy = NULL;
        }
    }
    js_LeaveLocalRootScopeWithResult(cx, copy);
    return copy;
}






static JSBool
DeepCopySetInLRS(JSContext *cx, JSXMLArray<JSXML> *from, JSXMLArray<JSXML> *to, JSXML *parent,
                 unsigned flags)
{
    uint32_t j, n;
    JSXML *kid2;
    JSString *str;

    n = from->length;
    if (!to->setCapacity(cx, n))
        return JS_FALSE;

    JSXMLArrayCursor<JSXML> cursor(from);
    j = 0;
    while (JSXML *kid = cursor.getNext()) {
        if ((flags & XSF_IGNORE_COMMENTS) &&
            kid->xml_class == JSXML_CLASS_COMMENT) {
            continue;
        }
        if ((flags & XSF_IGNORE_PROCESSING_INSTRUCTIONS) &&
            kid->xml_class == JSXML_CLASS_PROCESSING_INSTRUCTION) {
            continue;
        }
        if ((flags & XSF_IGNORE_WHITESPACE) &&
            (kid->xml_flags & XMLF_WHITESPACE_TEXT)) {
            continue;
        }
        kid2 = DeepCopyInLRS(cx, kid, flags);
        if (!kid2) {
            to->length = j;
            return JS_FALSE;
        }

        if ((flags & XSF_IGNORE_WHITESPACE) &&
            n > 1 && kid2->xml_class == JSXML_CLASS_TEXT) {
            str = ChompXMLWhitespace(cx, kid2->xml_value);
            if (!str) {
                to->length = j;
                return JS_FALSE;
            }
            kid2->xml_value = str;
        }

        XMLARRAY_SET_MEMBER(to, j, kid2);
        ++j;
        if (parent->xml_class != JSXML_CLASS_LIST)
            kid2->parent = parent;
    }

    if (j < n)
        to->trim();
    return JS_TRUE;
}

static JSXML *
DeepCopyInLRS(JSContext *cx, JSXML *xml, unsigned flags)
{
    JSXML *copy;
    JSObject *qn;
    JSBool ok;
    uint32_t i, n;
    JSObject *ns, *ns2;

    JS_CHECK_RECURSION(cx, return NULL);

    copy = js_NewXML(cx, JSXMLClass(xml->xml_class));
    if (!copy)
        return NULL;
    qn = xml->name;
    if (qn) {
        qn = NewXMLQName(cx, qn->getNameURI(), qn->getNamePrefix(), qn->getQNameLocalName());
        if (!qn) {
            ok = JS_FALSE;
            goto out;
        }
    }
    copy->name = qn;
    copy->xml_flags = xml->xml_flags;

    if (JSXML_HAS_VALUE(xml)) {
        copy->xml_value = xml->xml_value;
        ok = JS_TRUE;
    } else {
        ok = DeepCopySetInLRS(cx, &xml->xml_kids, &copy->xml_kids, copy, flags);
        if (!ok)
            goto out;

        if (xml->xml_class == JSXML_CLASS_LIST) {
            copy->xml_target = xml->xml_target;
            copy->xml_targetprop = xml->xml_targetprop;
        } else {
            n = xml->xml_namespaces.length;
            ok = copy->xml_namespaces.setCapacity(cx, n);
            if (!ok)
                goto out;
            for (i = 0; i < n; i++) {
                ns = XMLARRAY_MEMBER(&xml->xml_namespaces, i, JSObject);
                if (!ns)
                    continue;
                ns2 = NewXMLNamespace(cx, ns->getNamePrefix(), ns->getNameURI(),
                                      IsDeclared(ns));
                if (!ns2) {
                    copy->xml_namespaces.length = i;
                    ok = JS_FALSE;
                    goto out;
                }
                XMLARRAY_SET_MEMBER(&copy->xml_namespaces, i, ns2);
            }

            ok = DeepCopySetInLRS(cx, &xml->xml_attrs, &copy->xml_attrs, copy,
                                  0);
            if (!ok)
                goto out;
        }
    }

out:
    if (!ok)
        return NULL;
    return copy;
}


static void
DeleteByIndex(JSContext *cx, JSXML *xml, uint32_t index)
{
    JSXML *kid;

    if (JSXML_HAS_KIDS(xml) && index < xml->xml_kids.length) {
        kid = XMLARRAY_MEMBER(&xml->xml_kids, index, JSXML);
        if (kid)
            kid->parent = NULL;
        XMLArrayDelete(cx, &xml->xml_kids, index, JS_TRUE);
    }
}

typedef JSBool (*JSXMLNameMatcher)(JSObject *nameqn, JSXML *xml);

static JSBool
MatchAttrName(JSObject *nameqn, JSXML *attr)
{
    JSObject *attrqn = attr->name;
    JSLinearString *localName = nameqn->getQNameLocalName();
    JSLinearString *uri;

    return (IS_STAR(localName) ||
            EqualStrings(attrqn->getQNameLocalName(), localName)) &&
           (!(uri = nameqn->getNameURI()) ||
            EqualStrings(attrqn->getNameURI(), uri));
}

static JSBool
MatchElemName(JSObject *nameqn, JSXML *elem)
{
    JSLinearString *localName = nameqn->getQNameLocalName();
    JSLinearString *uri;

    return (IS_STAR(localName) ||
            (elem->xml_class == JSXML_CLASS_ELEMENT &&
             EqualStrings(elem->name->getQNameLocalName(), localName))) &&
           (!(uri = nameqn->getNameURI()) ||
            (elem->xml_class == JSXML_CLASS_ELEMENT &&
             EqualStrings(elem->name->getNameURI(), uri)));
}


static JSBool
DescendantsHelper(JSContext *cx, JSXML *xml, JSObject *nameqn, JSXML *list)
{
    uint32_t i, n;
    JSXML *attr, *kid;

    JS_CHECK_RECURSION(cx, return JS_FALSE);

    if (xml->xml_class == JSXML_CLASS_ELEMENT &&
        nameqn->getClass() == &AttributeNameClass) {
        for (i = 0, n = xml->xml_attrs.length; i < n; i++) {
            attr = XMLARRAY_MEMBER(&xml->xml_attrs, i, JSXML);
            if (attr && MatchAttrName(nameqn, attr)) {
                if (!Append(cx, list, attr))
                    return JS_FALSE;
            }
        }
    }

    for (i = 0, n = JSXML_LENGTH(xml); i < n; i++) {
        kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
        if (!kid)
            continue;
        if (nameqn->getClass() != &AttributeNameClass &&
            MatchElemName(nameqn, kid)) {
            if (!Append(cx, list, kid))
                return JS_FALSE;
        }
        if (!DescendantsHelper(cx, kid, nameqn, list))
            return JS_FALSE;
    }
    return JS_TRUE;
}

static JSXML *
Descendants(JSContext *cx, JSXML *xml, jsval id)
{
    jsid funid;
    JSObject *nameqn;
    JSObject *listobj;
    JSXML *list, *kid;
    uint32_t i, n;
    JSBool ok;

    nameqn = ToXMLName(cx, id, &funid);
    if (!nameqn)
        return NULL;

    listobj = js_NewXMLObject(cx, JSXML_CLASS_LIST);
    if (!listobj)
        return NULL;
    list = (JSXML *) listobj->getPrivate();
    if (!JSID_IS_VOID(funid))
        return list;

    





    list->name = nameqn;
    if (!js_EnterLocalRootScope(cx))
        return NULL;
    if (xml->xml_class == JSXML_CLASS_LIST) {
        ok = JS_TRUE;
        for (i = 0, n = xml->xml_kids.length; i < n; i++) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_ELEMENT) {
                ok = DescendantsHelper(cx, kid, nameqn, list);
                if (!ok)
                    break;
            }
        }
    } else {
        ok = DescendantsHelper(cx, xml, nameqn, list);
    }
    js_LeaveLocalRootScopeWithResult(cx, list);
    if (!ok)
        return NULL;
    list->name = NULL;
    return list;
}


static JSBool
XMLEquals(JSContext *cx, JSXML *xml, JSXML *vxml, JSBool *bp)
{
    JSObject *qn, *vqn;
    uint32_t i, j, n;
    JSXML *kid, *vkid, *attr, *vattr;
    JSObject *xobj, *vobj;

retry:
    if (xml->xml_class != vxml->xml_class) {
        if (xml->xml_class == JSXML_CLASS_LIST && xml->xml_kids.length == 1) {
            xml = XMLARRAY_MEMBER(&xml->xml_kids, 0, JSXML);
            if (xml)
                goto retry;
        }
        if (vxml->xml_class == JSXML_CLASS_LIST && vxml->xml_kids.length == 1) {
            vxml = XMLARRAY_MEMBER(&vxml->xml_kids, 0, JSXML);
            if (vxml)
                goto retry;
        }
        *bp = JS_FALSE;
        return JS_TRUE;
    }

    qn = xml->name;
    vqn = vxml->name;
    if (qn) {
        *bp = vqn &&
              EqualStrings(qn->getQNameLocalName(), vqn->getQNameLocalName()) &&
              EqualStrings(qn->getNameURI(), vqn->getNameURI());
    } else {
        *bp = vqn == NULL;
    }
    if (!*bp)
        return JS_TRUE;

    if (JSXML_HAS_VALUE(xml)) {
        bool equal;
        if (!EqualStrings(cx, xml->xml_value, vxml->xml_value, &equal))
            return JS_FALSE;
        *bp = equal;
    } else if (xml->xml_kids.length != vxml->xml_kids.length) {
        *bp = JS_FALSE;
    } else {
        {
            JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
            JSXMLArrayCursor<JSXML> vcursor(&vxml->xml_kids);
            for (;;) {
                kid = cursor.getNext();
                vkid = vcursor.getNext();
                if (!kid || !vkid) {
                    *bp = !kid && !vkid;
                    break;
                }
                xobj = js_GetXMLObject(cx, kid);
                vobj = js_GetXMLObject(cx, vkid);
                if (!xobj || !vobj ||
                    !js_TestXMLEquality(cx, ObjectValue(*xobj), ObjectValue(*vobj), bp))
                    return JS_FALSE;
                if (!*bp)
                    break;
            }
        }

        if (*bp && xml->xml_class == JSXML_CLASS_ELEMENT) {
            n = xml->xml_attrs.length;
            if (n != vxml->xml_attrs.length)
                *bp = JS_FALSE;
            for (i = 0; *bp && i < n; i++) {
                attr = XMLARRAY_MEMBER(&xml->xml_attrs, i, JSXML);
                if (!attr)
                    continue;
                j = XMLARRAY_FIND_MEMBER(&vxml->xml_attrs, attr, attr_identity);
                if (j == XML_NOT_FOUND) {
                    *bp = JS_FALSE;
                    break;
                }
                vattr = XMLARRAY_MEMBER(&vxml->xml_attrs, j, JSXML);
                if (!vattr)
                    continue;
                bool equal;
                if (!EqualStrings(cx, attr->xml_value, vattr->xml_value, &equal))
                    return JS_FALSE;
                *bp = equal;
            }
        }
    }

    return JS_TRUE;
}


static JSBool
Equals(JSContext *cx, JSXML *xml, jsval v, JSBool *bp)
{
    JSObject *vobj;
    JSXML *vxml;

    if (JSVAL_IS_PRIMITIVE(v)) {
        *bp = JS_FALSE;
        if (xml->xml_class == JSXML_CLASS_LIST) {
            if (xml->xml_kids.length == 1) {
                vxml = XMLARRAY_MEMBER(&xml->xml_kids, 0, JSXML);
                if (!vxml)
                    return JS_TRUE;
                vobj = js_GetXMLObject(cx, vxml);
                if (!vobj)
                    return JS_FALSE;
                return js_TestXMLEquality(cx, ObjectValue(*vobj), v, bp);
            }
            if (JSVAL_IS_VOID(v) && xml->xml_kids.length == 0)
                *bp = JS_TRUE;
        }
    } else {
        vobj = JSVAL_TO_OBJECT(v);
        if (!vobj->isXML()) {
            *bp = JS_FALSE;
        } else {
            vxml = (JSXML *) vobj->getPrivate();
            if (!XMLEquals(cx, xml, vxml, bp))
                return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSBool
CheckCycle(JSContext *cx, JSXML *xml, JSXML *kid)
{
    JS_ASSERT(kid->xml_class != JSXML_CLASS_LIST);

    do {
        if (xml == kid) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CYCLIC_VALUE, js_XML_str);
            return JS_FALSE;
        }
    } while ((xml = xml->parent) != NULL);

    return JS_TRUE;
}


static JSBool
Insert(JSContext *cx, JSXML *xml, uint32_t i, jsval v)
{
    uint32_t j, n;
    JSXML *vxml, *kid;
    JSObject *vobj;
    JSString *str;

    if (!JSXML_HAS_KIDS(xml))
        return JS_TRUE;

    n = 1;
    vxml = NULL;
    if (!JSVAL_IS_PRIMITIVE(v)) {
        vobj = JSVAL_TO_OBJECT(v);
        if (vobj->isXML()) {
            vxml = (JSXML *) vobj->getPrivate();
            if (vxml->xml_class == JSXML_CLASS_LIST) {
                n = vxml->xml_kids.length;
                if (n == 0)
                    return JS_TRUE;
                for (j = 0; j < n; j++) {
                    kid = XMLARRAY_MEMBER(&vxml->xml_kids, j, JSXML);
                    if (!kid)
                        continue;
                    if (!CheckCycle(cx, xml, kid))
                        return JS_FALSE;
                }
            } else if (vxml->xml_class == JSXML_CLASS_ELEMENT) {
                
                if (!CheckCycle(cx, xml, vxml))
                    return JS_FALSE;
            }
        }
    }
    if (!vxml) {
        str = ToString(cx, v);
        if (!str)
            return JS_FALSE;

        vxml = js_NewXML(cx, JSXML_CLASS_TEXT);
        if (!vxml)
            return JS_FALSE;
        vxml->xml_value = str;
    }

    if (i > xml->xml_kids.length)
        i = xml->xml_kids.length;

    if (!XMLArrayInsert(cx, &xml->xml_kids, i, n))
        return JS_FALSE;

    if (vxml->xml_class == JSXML_CLASS_LIST) {
        for (j = 0; j < n; j++) {
            kid = XMLARRAY_MEMBER(&vxml->xml_kids, j, JSXML);
            if (!kid)
                continue;
            kid->parent = xml;
            XMLARRAY_SET_MEMBER(&xml->xml_kids, i + j, kid);

            
        }
    } else {
        vxml->parent = xml;
        XMLARRAY_SET_MEMBER(&xml->xml_kids, i, vxml);
    }
    return JS_TRUE;
}


static JSBool
Replace(JSContext *cx, JSXML *xml, uint32_t i, jsval v)
{
    uint32_t n;
    JSXML *vxml, *kid;
    JSObject *vobj;
    JSString *str;

    if (!JSXML_HAS_KIDS(xml))
        return JS_TRUE;

    




    n = xml->xml_kids.length;
    if (i > n)
        i = n;

    vxml = NULL;
    if (!JSVAL_IS_PRIMITIVE(v)) {
        vobj = JSVAL_TO_OBJECT(v);
        if (vobj->isXML())
            vxml = (JSXML *) vobj->getPrivate();
    }

    switch (vxml ? JSXMLClass(vxml->xml_class) : JSXML_CLASS_LIMIT) {
      case JSXML_CLASS_ELEMENT:
        
        if (!CheckCycle(cx, xml, vxml))
            return JS_FALSE;
      case JSXML_CLASS_COMMENT:
      case JSXML_CLASS_PROCESSING_INSTRUCTION:
      case JSXML_CLASS_TEXT:
        goto do_replace;

      case JSXML_CLASS_LIST:
        if (i < n)
            DeleteByIndex(cx, xml, i);
        if (!Insert(cx, xml, i, v))
            return JS_FALSE;
        break;

      default:
        str = ToString(cx, v);
        if (!str)
            return JS_FALSE;

        vxml = js_NewXML(cx, JSXML_CLASS_TEXT);
        if (!vxml)
            return JS_FALSE;
        vxml->xml_value = str;

      do_replace:
        vxml->parent = xml;
        if (i < n) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid)
                kid->parent = NULL;
        }
        if (!XMLARRAY_ADD_MEMBER(cx, &xml->xml_kids, i, vxml))
            return JS_FALSE;
        break;
    }

    return JS_TRUE;
}


static void
DeleteNamedProperty(JSContext *cx, JSXML *xml, JSObject *nameqn,
                    JSBool attributes)
{
    JSXMLArray<JSXML> *array;
    uint32_t index, deleteCount;
    JSXML *kid;
    JSXMLNameMatcher matcher;

    if (xml->xml_class == JSXML_CLASS_LIST) {
        array = &xml->xml_kids;
        for (index = 0; index < array->length; index++) {
            kid = XMLARRAY_MEMBER(array, index, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_ELEMENT)
                DeleteNamedProperty(cx, kid, nameqn, attributes);
        }
    } else if (xml->xml_class == JSXML_CLASS_ELEMENT) {
        if (attributes) {
            array = &xml->xml_attrs;
            matcher = MatchAttrName;
        } else {
            array = &xml->xml_kids;
            matcher = MatchElemName;
        }
        deleteCount = 0;
        for (index = 0; index < array->length; index++) {
            kid = XMLARRAY_MEMBER(array, index, JSXML);
            if (kid && matcher(nameqn, kid)) {
                kid->parent = NULL;
                XMLArrayDelete(cx, array, index, JS_FALSE);
                ++deleteCount;
            } else if (deleteCount != 0) {
                XMLARRAY_SET_MEMBER(array,
                                    index - deleteCount,
                                    array->vector[index]);
            }
        }
        array->length -= deleteCount;
    }
}


static void
DeleteListElement(JSContext *cx, JSXML *xml, uint32_t index)
{
    JSXML *kid, *parent;
    uint32_t kidIndex;

    JS_ASSERT(xml->xml_class == JSXML_CLASS_LIST);

    if (index < xml->xml_kids.length) {
        kid = XMLARRAY_MEMBER(&xml->xml_kids, index, JSXML);
        if (kid) {
            parent = kid->parent;
            if (parent) {
                JS_ASSERT(parent != xml);
                JS_ASSERT(JSXML_HAS_KIDS(parent));

                if (kid->xml_class == JSXML_CLASS_ATTRIBUTE) {
                    DeleteNamedProperty(cx, parent, kid->name, JS_TRUE);
                } else {
                    kidIndex = XMLARRAY_FIND_MEMBER(&parent->xml_kids, kid,
                                                    pointer_match);
                    JS_ASSERT(kidIndex != XML_NOT_FOUND);
                    DeleteByIndex(cx, parent, kidIndex);
                }
            }
            XMLArrayDelete(cx, &xml->xml_kids, index, JS_TRUE);
        }
    }
}

static JSBool
SyncInScopeNamespaces(JSContext *cx, JSXML *xml)
{
    JSXMLArray<JSObject> *nsarray;
    uint32_t i, n;
    JSObject *ns;

    nsarray = &xml->xml_namespaces;
    while ((xml = xml->parent) != NULL) {
        for (i = 0, n = xml->xml_namespaces.length; i < n; i++) {
            ns = XMLARRAY_MEMBER(&xml->xml_namespaces, i, JSObject);
            if (ns && !XMLARRAY_HAS_MEMBER(nsarray, ns, namespace_identity)) {
                if (!XMLARRAY_APPEND(cx, nsarray, ns))
                    return JS_FALSE;
            }
        }
    }
    return JS_TRUE;
}

static JSBool
GetNamedProperty(JSContext *cx, JSXML *xml, JSObject* nameqn, JSXML *list)
{
    JSXMLArray<JSXML> *array;
    JSXMLNameMatcher matcher;
    JSBool attrs;

    if (xml->xml_class == JSXML_CLASS_LIST) {
        JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
        while (JSXML *kid = cursor.getNext()) {
            if (kid->xml_class == JSXML_CLASS_ELEMENT &&
                !GetNamedProperty(cx, kid, nameqn, list)) {
                return JS_FALSE;
            }
        }
    } else if (xml->xml_class == JSXML_CLASS_ELEMENT) {
        attrs = (nameqn->getClass() == &AttributeNameClass);
        if (attrs) {
            array = &xml->xml_attrs;
            matcher = MatchAttrName;
        } else {
            array = &xml->xml_kids;
            matcher = MatchElemName;
        }

        JSXMLArrayCursor<JSXML> cursor(array);
        while (JSXML *kid = cursor.getNext()) {
            if (matcher(nameqn, kid)) {
                if (!attrs &&
                    kid->xml_class == JSXML_CLASS_ELEMENT &&
                    !SyncInScopeNamespaces(cx, kid)) {
                    return JS_FALSE;
                }
                if (!Append(cx, list, kid))
                    return JS_FALSE;
            }
        }
    }

    return JS_TRUE;
}


static JSBool
GetProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JSXML *xml, *list, *kid;
    uint32_t index;
    JSObject *kidobj, *listobj;
    JSObject *nameqn;
    jsid funid;

    if (!obj->isXML())
        return true;
    xml = (JSXML *) obj->getPrivate();
    if (!xml)
        return true;

    if (js_IdIsIndex(id, &index)) {
        if (!JSXML_HAS_KIDS(xml)) {
            *vp = (index == 0) ? OBJECT_TO_JSVAL(obj) : JSVAL_VOID;
        } else {
            






            if (index < xml->xml_kids.length) {
                kid = XMLARRAY_MEMBER(&xml->xml_kids, index, JSXML);
                if (!kid) {
                    *vp = JSVAL_VOID;
                    return true;
                }
                kidobj = js_GetXMLObject(cx, kid);
                if (!kidobj)
                    return false;

                *vp = OBJECT_TO_JSVAL(kidobj);
            } else {
                *vp = JSVAL_VOID;
            }
        }
        return true;
    }

    


    nameqn = ToXMLName(cx, IdToJsval(id), &funid);
    if (!nameqn)
        return false;
    if (!JSID_IS_VOID(funid))
        return GetXMLFunction(cx, obj, funid, vp);

    jsval roots[2] = { OBJECT_TO_JSVAL(nameqn), JSVAL_NULL };
    AutoArrayRooter tvr(cx, ArrayLength(roots), roots);

    listobj = js_NewXMLObject(cx, JSXML_CLASS_LIST);
    if (!listobj)
        return false;

    roots[1] = OBJECT_TO_JSVAL(listobj);

    list = (JSXML *) listobj->getPrivate();
    if (!GetNamedProperty(cx, xml, nameqn, list))
        return false;

    






    list->xml_target = xml;
    list->xml_targetprop = nameqn;
    *vp = OBJECT_TO_JSVAL(listobj);
    return true;
}

static JSXML *
CopyOnWrite(JSContext *cx, JSXML *xml, JSObject *obj)
{
    JS_ASSERT(xml->object != obj);

    xml = DeepCopy(cx, xml, obj, 0);
    if (!xml)
        return NULL;

    JS_ASSERT(xml->object == obj);
    return xml;
}

#define CHECK_COPY_ON_WRITE(cx,xml,obj)                                       \
    (xml->object == obj ? xml : CopyOnWrite(cx, xml, obj))

static JSString *
KidToString(JSContext *cx, JSXML *xml, uint32_t index)
{
    JSXML *kid;
    JSObject *kidobj;

    kid = XMLARRAY_MEMBER(&xml->xml_kids, index, JSXML);
    if (!kid)
        return cx->runtime->emptyString;
    kidobj = js_GetXMLObject(cx, kid);
    if (!kidobj)
        return NULL;
    return ToString(cx, ObjectValue(*kidobj));
}


static JSBool
ResolveValue(JSContext *cx, JSXML *list, JSXML **result);


static JSBool
PutProperty(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
    JSBool ok, primitiveAssign;
    enum { OBJ_ROOT, ID_ROOT, VAL_ROOT };
    JSXML *xml, *vxml, *rxml, *kid, *attr, *parent, *copy, *kid2, *match;
    JSObject *vobj, *nameobj, *attrobj, *parentobj, *kidobj, *copyobj;
    JSObject *targetprop, *nameqn, *attrqn;
    uint32_t index, i, j, k, n, q, matchIndex;
    jsval attrval, nsval;
    jsid funid;
    JSObject *ns;

    if (!obj->isXML())
        return JS_TRUE;
    xml = (JSXML *) obj->getPrivate();
    if (!xml)
        return JS_TRUE;

    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;

    
    vxml = NULL;
    if (!JSVAL_IS_PRIMITIVE(*vp)) {
        vobj = JSVAL_TO_OBJECT(*vp);
        if (vobj->isXML())
            vxml = (JSXML *) vobj->getPrivate();
    }

    ok = js_EnterLocalRootScope(cx);
    if (!ok)
        return JS_FALSE;

    MUST_FLOW_THROUGH("out");
    jsval roots[3];
    roots[OBJ_ROOT] = OBJECT_TO_JSVAL(obj);
    roots[ID_ROOT] = IdToJsval(id);
    roots[VAL_ROOT] = *vp;
    AutoArrayRooter tvr(cx, ArrayLength(roots), roots);

    if (js_IdIsIndex(id, &index)) {
        if (xml->xml_class != JSXML_CLASS_LIST) {
            
            ReportBadXMLName(cx, IdToValue(id));
            goto bad;
        }

        


        i = index;

        
        if (xml->xml_target) {
            ok = ResolveValue(cx, xml->xml_target, &rxml);
            if (!ok)
                goto out;
            if (!rxml)
                goto out;
            JS_ASSERT(rxml->object);
        } else {
            rxml = NULL;
        }

        
        if (index >= xml->xml_kids.length) {
            
            if (rxml) {
                if (rxml->xml_class == JSXML_CLASS_LIST) {
                    if (rxml->xml_kids.length != 1)
                        goto out;
                    rxml = XMLARRAY_MEMBER(&rxml->xml_kids, 0, JSXML);
                    if (!rxml)
                        goto out;
                    ok = js_GetXMLObject(cx, rxml) != NULL;
                    if (!ok)
                        goto out;
                }

                














                if (!JSXML_HAS_KIDS(rxml))
                    goto out;
            }

            
            targetprop = xml->xml_targetprop;
            if (!targetprop || IS_STAR(targetprop->getQNameLocalName())) {
                
                kid = js_NewXML(cx, JSXML_CLASS_TEXT);
                if (!kid)
                    goto bad;
            } else {
                nameobj = targetprop;
                if (nameobj->getClass() == &AttributeNameClass) {
                    




                    ok = GetProperty(cx, rxml->object, id, &attrval);
                    if (!ok)
                        goto out;
                    if (JSVAL_IS_PRIMITIVE(attrval))    
                        goto out;
                    attrobj = JSVAL_TO_OBJECT(attrval);
                    attr = (JSXML *) attrobj->getPrivate();
                    if (JSXML_LENGTH(attr) != 0)
                        goto out;

                    kid = js_NewXML(cx, JSXML_CLASS_ATTRIBUTE);
                } else {
                    
                    kid = js_NewXML(cx, JSXML_CLASS_ELEMENT);
                }
                if (!kid)
                    goto bad;

                
                kid->name = targetprop;
            }

            
            kid->parent = rxml;

            
            i = xml->xml_kids.length;
            if (kid->xml_class != JSXML_CLASS_ATTRIBUTE) {
                





                if (rxml) {
                    JS_ASSERT(JSXML_HAS_KIDS(rxml));
                    n = rxml->xml_kids.length;
                    j = n - 1;
                    if (n != 0 && i != 0) {
                        for (n = j, j = 0; j < n; j++) {
                            if (rxml->xml_kids.vector[j] ==
                                xml->xml_kids.vector[i-1]) {
                                break;
                            }
                        }
                    }

                    kidobj = js_GetXMLObject(cx, kid);
                    if (!kidobj)
                        goto bad;
                    ok = Insert(cx, rxml, j + 1, OBJECT_TO_JSVAL(kidobj));
                    if (!ok)
                        goto out;
                }

                




                if (vxml) {
                    kid->name = (vxml->xml_class == JSXML_CLASS_LIST)
                        ? vxml->xml_targetprop
                        : vxml->name;
                }
            }

            
            ok = Append(cx, xml, kid);
            if (!ok)
                goto out;
        }

        
        if (!vxml ||
            vxml->xml_class == JSXML_CLASS_TEXT ||
            vxml->xml_class == JSXML_CLASS_ATTRIBUTE) {
            ok = JS_ConvertValue(cx, *vp, JSTYPE_STRING, vp);
            if (!ok)
                goto out;
            roots[VAL_ROOT] = *vp;
        }

        
        kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
        if (!kid)
            goto out;
        parent = kid->parent;
        if (kid->xml_class == JSXML_CLASS_ATTRIBUTE) {
            nameobj = kid->name;
            if (nameobj->getClass() != &AttributeNameClass) {
                nameobj = NewXMLAttributeName(cx, nameobj->getNameURI(), nameobj->getNamePrefix(),
                                              nameobj->getQNameLocalName());
                if (!nameobj)
                    goto bad;
            }
            id = OBJECT_TO_JSID(nameobj);

            if (parent) {
                
                parentobj = js_GetXMLObject(cx, parent);
                if (!parentobj)
                    goto bad;
                ok = PutProperty(cx, parentobj, id, strict, vp);
                if (!ok)
                    goto out;

                
                ok = GetProperty(cx, parentobj, id, vp);
                if (!ok)
                    goto out;
                attr = (JSXML *) JSVAL_TO_OBJECT(*vp)->getPrivate();

                
                if (attr->xml_kids.length != 0)
                    xml->xml_kids.vector[i] = attr->xml_kids.vector[0];
            }
        }

        
        else if (vxml && vxml->xml_class == JSXML_CLASS_LIST) {
            








            copy = DeepCopyInLRS(cx, vxml, 0);
            if (!copy)
                goto bad;
            copyobj = js_GetXMLObject(cx, copy);
            if (!copyobj)
                goto bad;

            JS_ASSERT(parent != xml);
            if (parent) {
                q = XMLARRAY_FIND_MEMBER(&parent->xml_kids, kid, pointer_match);
                JS_ASSERT(q != XML_NOT_FOUND);
                ok = Replace(cx, parent, q, OBJECT_TO_JSVAL(copyobj));
                if (!ok)
                    goto out;

#ifdef DEBUG
                
                for (j = 0, n = copy->xml_kids.length; j < n; j++) {
                    kid2 = XMLARRAY_MEMBER(&parent->xml_kids, q + j, JSXML);
                    JS_ASSERT(XMLARRAY_MEMBER(&copy->xml_kids, j, JSXML)
                              == kid2);
                }
#endif
            }

            




            n = copy->xml_kids.length;
            if (n == 0) {
                XMLArrayDelete(cx, &xml->xml_kids, i, JS_TRUE);
            } else {
                ok = XMLArrayInsert(cx, &xml->xml_kids, i + 1, n - 1);
                if (!ok)
                    goto out;

                for (j = 0; j < n; j++)
                    xml->xml_kids.vector[i + j] = copy->xml_kids.vector[j];
            }
        }

        
        else if (vxml || JSXML_HAS_VALUE(kid)) {
            if (parent) {
                q = XMLARRAY_FIND_MEMBER(&parent->xml_kids, kid, pointer_match);
                JS_ASSERT(q != XML_NOT_FOUND);
                ok = Replace(cx, parent, q, *vp);
                if (!ok)
                    goto out;

                vxml = XMLARRAY_MEMBER(&parent->xml_kids, q, JSXML);
                if (!vxml)
                    goto out;
                roots[VAL_ROOT] = *vp = OBJECT_TO_JSVAL(vxml->object);
            }

            









            if (!vxml) {
                JS_ASSERT(JSVAL_IS_STRING(*vp));
                vobj = ToXML(cx, *vp);
                if (!vobj)
                    goto bad;
                roots[VAL_ROOT] = *vp = OBJECT_TO_JSVAL(vobj);
                vxml = (JSXML *) vobj->getPrivate();
            }
            XMLARRAY_SET_MEMBER(&xml->xml_kids, i, vxml);
        }

        
        else {
            kidobj = js_GetXMLObject(cx, kid);
            if (!kidobj)
                goto bad;
            id = ATOM_TO_JSID(cx->runtime->atomState.starAtom);
            ok = PutProperty(cx, kidobj, id, strict, vp);
            if (!ok)
                goto out;
        }
    } else {
        


        nameqn = ToXMLName(cx, IdToJsval(id), &funid);
        if (!nameqn)
            goto bad;
        if (!JSID_IS_VOID(funid)) {
            ok = js_SetPropertyHelper(cx, obj, funid, 0, vp, false);
            goto out;
        }
        nameobj = nameqn;
        roots[ID_ROOT] = OBJECT_TO_JSVAL(nameobj);

        if (xml->xml_class == JSXML_CLASS_LIST) {
            




            n = JSXML_LENGTH(xml);
            if (n > 1)
                goto type_error;
            if (n == 0) {
                ok = ResolveValue(cx, xml, &rxml);
                if (!ok)
                    goto out;
                if (!rxml || JSXML_LENGTH(rxml) != 1)
                    goto type_error;
                ok = Append(cx, xml, rxml);
                if (!ok)
                    goto out;
            }
            JS_ASSERT(JSXML_LENGTH(xml) == 1);
            xml = XMLARRAY_MEMBER(&xml->xml_kids, 0, JSXML);
            if (!xml)
                goto out;
            JS_ASSERT(xml->xml_class != JSXML_CLASS_LIST);
            obj = js_GetXMLObject(cx, xml);
            if (!obj)
                goto bad;
            roots[OBJ_ROOT] = OBJECT_TO_JSVAL(obj);

            
        }

        





        if (JSXML_HAS_VALUE(xml))
            goto out;

        if (!vxml ||
            vxml->xml_class == JSXML_CLASS_TEXT ||
            vxml->xml_class == JSXML_CLASS_ATTRIBUTE) {
            ok = JS_ConvertValue(cx, *vp, JSTYPE_STRING, vp);
            if (!ok)
                goto out;
        } else {
            rxml = DeepCopyInLRS(cx, vxml, 0);
            if (!rxml || !js_GetXMLObject(cx, rxml))
                goto bad;
            vxml = rxml;
            *vp = OBJECT_TO_JSVAL(vxml->object);
        }
        roots[VAL_ROOT] = *vp;

        



        ok = js_GetDefaultXMLNamespace(cx, &nsval);
        if (!ok)
            goto out;

        if (nameobj->getClass() == &AttributeNameClass) {
            
            if (!js_IsXMLName(cx, OBJECT_TO_JSVAL(nameobj)))
                goto out;

            
            if (vxml && vxml->xml_class == JSXML_CLASS_LIST) {
                n = vxml->xml_kids.length;
                if (n == 0) {
                    *vp = STRING_TO_JSVAL(cx->runtime->emptyString);
                } else {
                    JSString *left = KidToString(cx, vxml, 0);
                    if (!left)
                        goto bad;

                    JSString *space = cx->runtime->atomState.spaceAtom;
                    for (i = 1; i < n; i++) {
                        left = js_ConcatStrings(cx, left, space);
                        if (!left)
                            goto bad;
                        JSString *right = KidToString(cx, vxml, i);
                        if (!right)
                            goto bad;
                        left = js_ConcatStrings(cx, left, right);
                        if (!left)
                            goto bad;
                    }

                    roots[VAL_ROOT] = *vp = STRING_TO_JSVAL(left);
                }
            } else {
                ok = JS_ConvertValue(cx, *vp, JSTYPE_STRING, vp);
                if (!ok)
                    goto out;
                roots[VAL_ROOT] = *vp;
            }

            
            match = NULL;
            for (i = 0, n = xml->xml_attrs.length; i < n; i++) {
                attr = XMLARRAY_MEMBER(&xml->xml_attrs, i, JSXML);
                if (!attr)
                    continue;
                attrqn = attr->name;
                if (EqualStrings(attrqn->getQNameLocalName(), nameqn->getQNameLocalName())) {
                    JSLinearString *uri = nameqn->getNameURI();
                    if (!uri || EqualStrings(attrqn->getNameURI(), uri)) {
                        if (!match) {
                            match = attr;
                        } else {
                            DeleteNamedProperty(cx, xml, attrqn, JS_TRUE);
                            --i;
                        }
                    }
                }
            }

            
            attr = match;
            if (!attr) {
                
                JSLinearString *uri = nameqn->getNameURI();
                JSLinearString *left, *right;
                if (!uri) {
                    left = right = cx->runtime->emptyString;
                } else {
                    left = uri;
                    right = nameqn->getNamePrefix();
                }
                nameqn = NewXMLQName(cx, left, right, nameqn->getQNameLocalName());
                if (!nameqn)
                    goto bad;

                
                attr = js_NewXML(cx, JSXML_CLASS_ATTRIBUTE);
                if (!attr)
                    goto bad;
                attr->parent = xml;
                attr->name = nameqn;

                
                ok = XMLARRAY_ADD_MEMBER(cx, &xml->xml_attrs, n, attr);
                if (!ok)
                    goto out;

                
                ns = GetNamespace(cx, nameqn, NULL);
                if (!ns)
                    goto bad;
                ok = AddInScopeNamespace(cx, xml, ns);
                if (!ok)
                    goto out;
            }

            
            attr->xml_value = JSVAL_TO_STRING(*vp);
            goto out;
        }

        
        if (!js_IsXMLName(cx, OBJECT_TO_JSVAL(nameobj)) &&
            !IS_STAR(nameqn->getQNameLocalName())) {
            goto out;
        }

        
        id = JSID_VOID;
        primitiveAssign = !vxml && !IS_STAR(nameqn->getQNameLocalName());

        
        k = n = xml->xml_kids.length;
        matchIndex = XML_NOT_FOUND;
        kid2 = NULL;
        while (k != 0) {
            --k;
            kid = XMLARRAY_MEMBER(&xml->xml_kids, k, JSXML);
            if (kid && MatchElemName(nameqn, kid)) {
                if (matchIndex != XML_NOT_FOUND)
                    DeleteByIndex(cx, xml, matchIndex);
                matchIndex = k;
                kid2 = kid;
            }
        }

        















        if (kid2) {
            JS_ASSERT(kid2->parent == xml || !kid2->parent);
            if (!kid2->parent)
                kid2->parent = xml;
        }

        
        if (matchIndex == XML_NOT_FOUND) {
            
            matchIndex = n;

            
            if (primitiveAssign) {
                JSLinearString *uri = nameqn->getNameURI();
                JSLinearString *left, *right;
                if (!uri) {
                    ns = JSVAL_TO_OBJECT(nsval);
                    left = ns->getNameURI();
                    right = ns->getNamePrefix();
                } else {
                    left = uri;
                    right = nameqn->getNamePrefix();
                }
                nameqn = NewXMLQName(cx, left, right, nameqn->getQNameLocalName());
                if (!nameqn)
                    goto bad;

                
                vobj = js_NewXMLObject(cx, JSXML_CLASS_ELEMENT);
                if (!vobj)
                    goto bad;
                vxml = (JSXML *) vobj->getPrivate();
                vxml->parent = xml;
                vxml->name = nameqn;

                
                ns = GetNamespace(cx, nameqn, NULL);
                if (!ns)
                    goto bad;
                ok = Replace(cx, xml, matchIndex, OBJECT_TO_JSVAL(vobj));
                if (!ok)
                    goto out;
                ok = AddInScopeNamespace(cx, vxml, ns);
                if (!ok)
                    goto out;
            }
        }

        
        if (primitiveAssign) {
            JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
            cursor.index = matchIndex;
            kid = cursor.getCurrent();
            if (JSXML_HAS_KIDS(kid)) {
                kid->xml_kids.finish(cx);
                kid->xml_kids.init();
                ok = kid->xml_kids.setCapacity(cx, 1);
            }

            
            
            if (ok) {
                ok = JS_ConvertValue(cx, *vp, JSTYPE_STRING, vp);
                if (ok && !JSVAL_TO_STRING(*vp)->empty()) {
                    roots[VAL_ROOT] = *vp;
                    if (cursor.getCurrent() == kid)
                        ok = Replace(cx, kid, 0, *vp);
                }
            }
        } else {
            
            ok = Replace(cx, xml, matchIndex, *vp);
        }
    }

out:
    js_LeaveLocalRootScope(cx);
    return ok;

type_error:
    {
        JSAutoByteString bytes;
        if (js_ValueToPrintable(cx, IdToValue(id), &bytes))
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_XMLLIST_PUT, bytes.ptr());
    }
bad:
    ok = JS_FALSE;
    goto out;
}


static JSBool
ResolveValue(JSContext *cx, JSXML *list, JSXML **result)
{
    JSXML *target, *base;
    JSObject *targetprop;
    jsid id;
    jsval tv;

    if (list->xml_class != JSXML_CLASS_LIST || list->xml_kids.length != 0) {
        if (!js_GetXMLObject(cx, list))
            return JS_FALSE;
        *result = list;
        return JS_TRUE;
    }

    target = list->xml_target;
    targetprop = list->xml_targetprop;
    if (!target || !targetprop || IS_STAR(targetprop->getQNameLocalName())) {
        *result = NULL;
        return JS_TRUE;
    }

    if (targetprop->getClass() == &AttributeNameClass) {
        *result = NULL;
        return JS_TRUE;
    }

    if (!ResolveValue(cx, target, &base))
        return JS_FALSE;
    if (!base) {
        *result = NULL;
        return JS_TRUE;
    }
    if (!js_GetXMLObject(cx, base))
        return JS_FALSE;

    id = OBJECT_TO_JSID(targetprop);
    if (!GetProperty(cx, base->object, id, &tv))
        return JS_FALSE;
    target = (JSXML *) JSVAL_TO_OBJECT(tv)->getPrivate();

    if (JSXML_LENGTH(target) == 0) {
        if (base->xml_class == JSXML_CLASS_LIST && JSXML_LENGTH(base) > 1) {
            *result = NULL;
            return JS_TRUE;
        }
        tv = STRING_TO_JSVAL(cx->runtime->emptyString);
        if (!PutProperty(cx, base->object, id, false, &tv))
            return JS_FALSE;
        if (!GetProperty(cx, base->object, id, &tv))
            return JS_FALSE;
        target = (JSXML *) JSVAL_TO_OBJECT(tv)->getPrivate();
    }

    *result = target;
    return JS_TRUE;
}

static JSBool
HasNamedProperty(JSXML *xml, JSObject *nameqn)
{
    JSBool found;
    JSXMLArray<JSXML> *array;
    JSXMLNameMatcher matcher;
    uint32_t i, n;

    if (xml->xml_class == JSXML_CLASS_LIST) {
        found = JS_FALSE;
        JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
        while (JSXML *kid = cursor.getNext()) {
            found = HasNamedProperty(kid, nameqn);
            if (found)
                break;
        }
        return found;
    }

    if (xml->xml_class == JSXML_CLASS_ELEMENT) {
        if (nameqn->getClass() == &AttributeNameClass) {
            array = &xml->xml_attrs;
            matcher = MatchAttrName;
        } else {
            array = &xml->xml_kids;
            matcher = MatchElemName;
        }
        for (i = 0, n = array->length; i < n; i++) {
            JSXML *kid = XMLARRAY_MEMBER(array, i, JSXML);
            if (kid && matcher(nameqn, kid))
                return JS_TRUE;
        }
    }

    return JS_FALSE;
}

static JSBool
HasIndexedProperty(JSXML *xml, uint32_t i)
{
    if (xml->xml_class == JSXML_CLASS_LIST)
        return i < JSXML_LENGTH(xml);

    if (xml->xml_class == JSXML_CLASS_ELEMENT)
        return i == 0;

    return JS_FALSE;
}

static JSBool
HasSimpleContent(JSXML *xml);

static JSBool
HasFunctionProperty(JSContext *cx, JSObject *obj, jsid funid, JSBool *found)
{
    JSObject *pobj;
    JSProperty *prop;
    JSXML *xml;

    JS_ASSERT(obj->getClass() == &XMLClass);

    if (!js_LookupProperty(cx, obj, funid, &pobj, &prop))
        return false;
    if (!prop) {
        xml = (JSXML *) obj->getPrivate();
        if (HasSimpleContent(xml)) {
            



            JSObject *proto = obj->global().getOrCreateStringPrototype(cx);
            if (!proto)
                return false;

            if (!js_LookupProperty(cx, proto, funid, &pobj, &prop))
                return false;
        }
    }
    *found = (prop != NULL);
    return true;
}

static bool
IdValIsIndex(JSContext *cx, jsval id, jsuint *indexp, bool *isIndex)
{
    if (JSVAL_IS_INT(id)) {
        jsint i;
        i = JSVAL_TO_INT(id);
        if (i < 0) {
            *isIndex = false;
            return true;
        }
        *indexp = (jsuint)i;
        *isIndex = true;
        return true;
    }

    if (!JSVAL_IS_STRING(id)) {
        *isIndex = false;
        return true;
    }

    JSLinearString *str = JSVAL_TO_STRING(id)->ensureLinear(cx);
    if (!str)
        return false;

    *isIndex = StringIsArrayIndex(str, indexp);
    return true;
}


static JSBool
HasProperty(JSContext *cx, JSObject *obj, jsval id, JSBool *found)
{
    JSXML *xml;
    bool isIndex;
    uint32_t i;
    JSObject *qn;
    jsid funid;

    xml = (JSXML *) obj->getPrivate();
    if (!IdValIsIndex(cx, id, &i, &isIndex))
        return JS_FALSE;

    if (isIndex) {
        *found = HasIndexedProperty(xml, i);
    } else {
        qn = ToXMLName(cx, id, &funid);
        if (!qn)
            return JS_FALSE;
        if (!JSID_IS_VOID(funid)) {
            if (!HasFunctionProperty(cx, obj, funid, found))
                return JS_FALSE;
        } else {
            *found = HasNamedProperty(xml, qn);
        }
    }
    return JS_TRUE;
}

static void
xml_finalize(JSContext *cx, JSObject *obj)
{
}
























static JSBool
xml_lookupGeneric(JSContext *cx, JSObject *obj, jsid id, JSObject **objp, JSProperty **propp)
{
    JSBool found;
    JSXML *xml;
    uint32_t i;
    JSObject *qn;
    jsid funid;

    xml = (JSXML *) obj->getPrivate();
    if (js_IdIsIndex(id, &i)) {
        found = HasIndexedProperty(xml, i);
    } else {
        qn = ToXMLName(cx, IdToJsval(id), &funid);
        if (!qn)
            return JS_FALSE;
        if (!JSID_IS_VOID(funid))
            return js_LookupProperty(cx, obj, funid, objp, propp);
        found = HasNamedProperty(xml, qn);
    }
    if (!found) {
        *objp = NULL;
        *propp = NULL;
    } else {
        const Shape *shape =
            js_AddNativeProperty(cx, obj, id, GetProperty, PutProperty,
                                 SHAPE_INVALID_SLOT, JSPROP_ENUMERATE,
                                 0, 0);
        if (!shape)
            return JS_FALSE;

        *objp = obj;
        *propp = (JSProperty *) shape;
    }
    return JS_TRUE;
}

static JSBool
xml_lookupProperty(JSContext *cx, JSObject *obj, PropertyName *name, JSObject **objp,
                   JSProperty **propp)
{
    return xml_lookupGeneric(cx, obj, ATOM_TO_JSID(name), objp, propp);
}

static JSBool
xml_lookupElement(JSContext *cx, JSObject *obj, uint32_t index, JSObject **objp,
                  JSProperty **propp)
{
    JSXML *xml = reinterpret_cast<JSXML *>(obj->getPrivate());
    if (!HasIndexedProperty(xml, index)) {
        *objp = NULL;
        *propp = NULL;
        return true;
    }

    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;

    const Shape *shape =
        js_AddNativeProperty(cx, obj, id, GetProperty, PutProperty,
                             SHAPE_INVALID_SLOT, JSPROP_ENUMERATE,
                             0, 0);
    if (!shape)
        return false;

    *objp = obj;
    *propp = (JSProperty *) shape;
    return true;
}

static JSBool
xml_lookupSpecial(JSContext *cx, JSObject *obj, SpecialId sid, JSObject **objp, JSProperty **propp)
{
    return xml_lookupGeneric(cx, obj, SPECIALID_TO_JSID(sid), objp, propp);
}

static JSBool
xml_defineGeneric(JSContext *cx, JSObject *obj, jsid id, const Value *v,
                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    if (IsFunctionObject(*v) || getter || setter ||
        (attrs & JSPROP_ENUMERATE) == 0 ||
        (attrs & (JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED))) {
        return js_DefineProperty(cx, obj, id, v, getter, setter, attrs);
    }

    jsval tmp = *v;
    return PutProperty(cx, obj, id, false, &tmp);
}

static JSBool
xml_defineProperty(JSContext *cx, JSObject *obj, PropertyName *name, const Value *v,
                   PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    return xml_defineGeneric(cx, obj, ATOM_TO_JSID(name), v, getter, setter, attrs);
}

static JSBool
xml_defineElement(JSContext *cx, JSObject *obj, uint32_t index, const Value *v,
                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return xml_defineGeneric(cx, obj, id, v, getter, setter, attrs);
}

static JSBool
xml_defineSpecial(JSContext *cx, JSObject *obj, SpecialId sid, const Value *v,
                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    return xml_defineGeneric(cx, obj, SPECIALID_TO_JSID(sid), v, getter, setter, attrs);
}

static JSBool
xml_getGeneric(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
{
    if (JSID_IS_DEFAULT_XML_NAMESPACE(id)) {
        vp->setUndefined();
        return JS_TRUE;
    }

    return GetProperty(cx, obj, id, vp);
}

static JSBool
xml_getProperty(JSContext *cx, JSObject *obj, JSObject *receiver, PropertyName *name, Value *vp)
{
    return xml_getGeneric(cx, obj, receiver, ATOM_TO_JSID(name), vp);
}

static JSBool
xml_getElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32_t index, Value *vp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return xml_getGeneric(cx, obj, receiver, id, vp);
}

static JSBool
xml_getSpecial(JSContext *cx, JSObject *obj, JSObject *receiver, SpecialId sid, Value *vp)
{
    return xml_getGeneric(cx, obj, receiver, SPECIALID_TO_JSID(sid), vp);
}

static JSBool
xml_setGeneric(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict)
{
    return PutProperty(cx, obj, id, strict, vp);
}

static JSBool
xml_setProperty(JSContext *cx, JSObject *obj, PropertyName *name, Value *vp, JSBool strict)
{
    return xml_setGeneric(cx, obj, ATOM_TO_JSID(name), vp, strict);
}

static JSBool
xml_setElement(JSContext *cx, JSObject *obj, uint32_t index, Value *vp, JSBool strict)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return xml_setGeneric(cx, obj, id, vp, strict);
}

static JSBool
xml_setSpecial(JSContext *cx, JSObject *obj, SpecialId sid, Value *vp, JSBool strict)
{
    return xml_setGeneric(cx, obj, SPECIALID_TO_JSID(sid), vp, strict);
}

static JSBool
xml_getGenericAttributes(JSContext *cx, JSObject *obj, jsid id, unsigned *attrsp)
{
    JSBool found;
    if (!HasProperty(cx, obj, IdToJsval(id), &found))
        return false;

    *attrsp = found ? JSPROP_ENUMERATE : 0;
    return JS_TRUE;
}

static JSBool
xml_getPropertyAttributes(JSContext *cx, JSObject *obj, PropertyName *name, unsigned *attrsp)
{
    return xml_getGenericAttributes(cx, obj, ATOM_TO_JSID(name), attrsp);
}

static JSBool
xml_getElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, unsigned *attrsp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return xml_getGenericAttributes(cx, obj, id, attrsp);
}

static JSBool
xml_getSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, unsigned *attrsp)
{
    return xml_getGenericAttributes(cx, obj, SPECIALID_TO_JSID(sid), attrsp);
}

static JSBool
xml_setGenericAttributes(JSContext *cx, JSObject *obj, jsid id, unsigned *attrsp)
{
    JSBool found;
    if (!HasProperty(cx, obj, IdToJsval(id), &found))
        return false;

    if (found) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_CANT_SET_XML_ATTRS);
        return false;
    }
    return true;
}

static JSBool
xml_setPropertyAttributes(JSContext *cx, JSObject *obj, PropertyName *name, unsigned *attrsp)
{
    return xml_setGenericAttributes(cx, obj, ATOM_TO_JSID(name), attrsp);
}

static JSBool
xml_setElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, unsigned *attrsp)
{
    jsid id;
    if (!IndexToId(cx, index, &id))
        return false;
    return xml_setGenericAttributes(cx, obj, id, attrsp);
}

static JSBool
xml_setSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, unsigned *attrsp)
{
    return xml_setGenericAttributes(cx, obj, SPECIALID_TO_JSID(sid), attrsp);
}

static JSBool
xml_deleteGeneric(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict)
{
    uint32_t index;
    JSObject *nameqn;
    jsid funid;

    Value idval = IdToValue(id);
    JSXML *xml = (JSXML *) obj->getPrivate();
    if (js_IdIsIndex(id, &index)) {
        if (xml->xml_class != JSXML_CLASS_LIST) {
            
            ReportBadXMLName(cx, IdToValue(id));
            return false;
        }

        
        DeleteListElement(cx, xml, index);
    } else {
        nameqn = ToXMLName(cx, idval, &funid);
        if (!nameqn)
            return false;
        if (!JSID_IS_VOID(funid))
            return js_DeleteGeneric(cx, obj, funid, rval, false);

        DeleteNamedProperty(cx, xml, nameqn,
                            nameqn->getClass() == &AttributeNameClass);
    }

    






    if (!obj->nativeEmpty() && !js_DeleteGeneric(cx, obj, id, rval, false))
        return false;

    rval->setBoolean(true);
    return true;
}

static JSBool
xml_deleteProperty(JSContext *cx, JSObject *obj, PropertyName *name, Value *rval, JSBool strict)
{
    return xml_deleteGeneric(cx, obj, ATOM_TO_JSID(name), rval, strict);
}

static JSBool
xml_deleteElement(JSContext *cx, JSObject *obj, uint32_t index, Value *rval, JSBool strict)
{
    JSXML *xml = reinterpret_cast<JSXML *>(obj->getPrivate());
    if (xml->xml_class != JSXML_CLASS_LIST) {
        
        ReportBadXMLName(cx, DoubleValue(index));
        return false;
    }

    
    DeleteListElement(cx, xml, index);

    






    if (!obj->nativeEmpty() && !js_DeleteElement(cx, obj, index, rval, false))
        return false;

    rval->setBoolean(true);
    return true;
}

static JSBool
xml_deleteSpecial(JSContext *cx, JSObject *obj, SpecialId sid, Value *rval, JSBool strict)
{
    return xml_deleteGeneric(cx, obj, SPECIALID_TO_JSID(sid), rval, strict);
}

static JSString *
xml_toString_helper(JSContext *cx, JSXML *xml);

JSBool
xml_convert(JSContext *cx, JSObject *obj, JSType hint, Value *rval)
{
    JS_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);
    JS_ASSERT(obj->isXML());

    JS::Anchor<JSObject *> anch(obj);
    JSString *str = xml_toString_helper(cx, reinterpret_cast<JSXML *>(obj->getPrivate()));
    if (!str)
        return false;
    *rval = StringValue(str);
    return true;
}

static JSBool
xml_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op, Value *statep, jsid *idp)
{
    JSXML *xml;
    uint32_t length, index;
    JSXMLArrayCursor<JSXML> *cursor;

    xml = (JSXML *)obj->getPrivate();
    length = JSXML_LENGTH(xml);

    switch (enum_op) {
      case JSENUMERATE_INIT:
      case JSENUMERATE_INIT_ALL:
        if (length == 0) {
            statep->setInt32(0);
        } else {
            cursor = cx->new_< JSXMLArrayCursor<JSXML> >(&xml->xml_kids);
            if (!cursor)
                return JS_FALSE;
            statep->setPrivate(cursor);
        }
        if (idp)
            *idp = INT_TO_JSID(length);
        break;

      case JSENUMERATE_NEXT:
        if (statep->isInt32(0)) {
            statep->setNull();
            break;
        }
        cursor = (JSXMLArrayCursor<JSXML> *) statep->toPrivate();
        if (cursor && cursor->array && (index = cursor->index) < length) {
            *idp = INT_TO_JSID(index);
            cursor->index = index + 1;
            break;
        }
        

      case JSENUMERATE_DESTROY:
        if (!statep->isInt32(0)) {
            cursor = (JSXMLArrayCursor<JSXML> *) statep->toPrivate();
            if (cursor)
                cx->delete_(cursor);
        }
        statep->setNull();
        break;
    }
    return JS_TRUE;
}

static JSType
xml_typeOf(JSContext *cx, JSObject *obj)
{
    return JSTYPE_XML;
}

static JSBool
xml_hasInstance(JSContext *cx, JSObject *obj, const Value *, JSBool *bp)
{
    return JS_TRUE;
}

static void
xml_trace(JSTracer *trc, JSObject *obj)
{
    JSXML *xml = (JSXML *) obj->getPrivate();
    



    if (xml)
        MarkXMLUnbarriered(trc, xml, "private");
}

static JSBool
xml_fix(JSContext *cx, JSObject *obj, bool *success, AutoIdVector *props)
{
    JS_ASSERT(obj->isExtensible());
    *success = false;
    return true;
}

static void
xml_clear(JSContext *cx, JSObject *obj)
{
}

static JSBool
HasSimpleContent(JSXML *xml)
{
    JSXML *kid;
    JSBool simple;
    uint32_t i, n;

again:
    switch (xml->xml_class) {
      case JSXML_CLASS_COMMENT:
      case JSXML_CLASS_PROCESSING_INSTRUCTION:
        return JS_FALSE;
      case JSXML_CLASS_LIST:
        if (xml->xml_kids.length == 0)
            return JS_TRUE;
        if (xml->xml_kids.length == 1) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, 0, JSXML);
            if (kid) {
                xml = kid;
                goto again;
            }
        }
        
      default:
        simple = JS_TRUE;
        for (i = 0, n = JSXML_LENGTH(xml); i < n; i++) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_ELEMENT) {
                simple = JS_FALSE;
                break;
            }
        }
        return simple;
    }
}




JSBool
js_GetXMLMethod(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JS_ASSERT(obj->isXML());

    if (JSID_IS_OBJECT(id))
        js_GetLocalNameFromFunctionQName(JSID_TO_OBJECT(id), &id, cx);

    



    AutoValueRooter tvr(cx);
    JSBool ok = GetXMLFunction(cx, obj, id, tvr.addr());
    *vp = tvr.value();
    return ok;
}

JSBool
js_TestXMLEquality(JSContext *cx, const Value &v1, const Value &v2, JSBool *bp)
{
    JSXML *xml, *vxml;
    JSObject *vobj;
    JSBool ok;
    JSString *str, *vstr;
    double d, d2;

    JSObject *obj;
    jsval v;
    if (v1.isObject() && v1.toObject().isXML()) {
        obj = &v1.toObject();
        v = v2;
    } else {
        v = v1;
        obj = &v2.toObject();
    }

    JS_ASSERT(obj->isXML());

    xml = (JSXML *) obj->getPrivate();
    vxml = NULL;
    if (!JSVAL_IS_PRIMITIVE(v)) {
        vobj = JSVAL_TO_OBJECT(v);
        if (vobj->isXML())
            vxml = (JSXML *) vobj->getPrivate();
    }

    if (xml->xml_class == JSXML_CLASS_LIST) {
        ok = Equals(cx, xml, v, bp);
    } else if (vxml) {
        if (vxml->xml_class == JSXML_CLASS_LIST) {
            ok = Equals(cx, vxml, OBJECT_TO_JSVAL(obj), bp);
        } else {
            if (((xml->xml_class == JSXML_CLASS_TEXT ||
                  xml->xml_class == JSXML_CLASS_ATTRIBUTE) &&
                 HasSimpleContent(vxml)) ||
                ((vxml->xml_class == JSXML_CLASS_TEXT ||
                  vxml->xml_class == JSXML_CLASS_ATTRIBUTE) &&
                 HasSimpleContent(xml))) {
                ok = js_EnterLocalRootScope(cx);
                if (ok) {
                    ok = (str = ToStringSlow(cx, ObjectValue(*obj))) &&
                         (vstr = ToString(cx, v));
                    if (ok) {
                        bool equal;
                        ok = EqualStrings(cx, str, vstr, &equal);
                        *bp = equal;
                    }
                    js_LeaveLocalRootScope(cx);
                }
            } else {
                ok = XMLEquals(cx, xml, vxml, bp);
            }
        }
    } else {
        ok = js_EnterLocalRootScope(cx);
        if (ok) {
            if (HasSimpleContent(xml)) {
                ok = (str = ToString(cx, ObjectValue(*obj))) &&
                     (vstr = ToString(cx, v));
                if (ok) {
                    bool equal;
                    ok = EqualStrings(cx, str, vstr, &equal);
                    *bp = equal;
                }
            } else if (JSVAL_IS_STRING(v) || JSVAL_IS_NUMBER(v)) {
                str = ToString(cx, ObjectValue(*obj));
                if (!str) {
                    ok = JS_FALSE;
                } else if (JSVAL_IS_STRING(v)) {
                    bool equal;
                    ok = EqualStrings(cx, str, JSVAL_TO_STRING(v), &equal);
                    if (ok)
                        *bp = equal;
                } else {
                    ok = JS_ValueToNumber(cx, STRING_TO_JSVAL(str), &d);
                    if (ok) {
                        d2 = JSVAL_IS_INT(v) ? JSVAL_TO_INT(v)
                                             : JSVAL_TO_DOUBLE(v);
                        *bp = (d == d2);
                    }
                }
            } else {
                *bp = JS_FALSE;
            }
            js_LeaveLocalRootScope(cx);
        }
    }
    return ok;
}

JSBool
js_ConcatenateXML(JSContext *cx, JSObject *obj, JSObject *robj, Value *vp)
{
    JSBool ok;
    JSObject *listobj;
    JSXML *list, *lxml, *rxml;

    JS_ASSERT(obj->isXML());
    ok = js_EnterLocalRootScope(cx);
    if (!ok)
        return JS_FALSE;

    listobj = js_NewXMLObject(cx, JSXML_CLASS_LIST);
    if (!listobj) {
        ok = JS_FALSE;
        goto out;
    }

    list = (JSXML *) listobj->getPrivate();
    lxml = (JSXML *) obj->getPrivate();
    ok = Append(cx, list, lxml);
    if (!ok)
        goto out;

    JS_ASSERT(robj->isXML());
    rxml = (JSXML *) robj->getPrivate();
    ok = Append(cx, list, rxml);
    if (!ok)
        goto out;

    vp->setObject(*listobj);
out:
    js_LeaveLocalRootScopeWithResult(cx, *vp);
    return ok;
}

JS_FRIEND_DATA(Class) js::XMLClass = {
    js_XML_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_CACHED_PROTO(JSProto_XML),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    xml_convert,
    xml_finalize,
    NULL,                 
    NULL,                 
    NULL,                 
    xml_hasInstance,
    xml_trace,
    JS_NULL_CLASS_EXT,
    {
        xml_lookupGeneric,
        xml_lookupProperty,
        xml_lookupElement,
        xml_lookupSpecial,
        xml_defineGeneric,
        xml_defineProperty,
        xml_defineElement,
        xml_defineSpecial,
        xml_getGeneric,
        xml_getProperty,
        xml_getElement,
        NULL, 
        xml_getSpecial,
        xml_setGeneric,
        xml_setProperty,
        xml_setElement,
        xml_setSpecial,
        xml_getGenericAttributes,
        xml_getPropertyAttributes,
        xml_getElementAttributes,
        xml_getSpecialAttributes,
        xml_setGenericAttributes,
        xml_setPropertyAttributes,
        xml_setElementAttributes,
        xml_setSpecialAttributes,
        xml_deleteProperty,
        xml_deleteElement,
        xml_deleteSpecial,
        xml_enumerate,
        xml_typeOf,
        xml_fix,
        NULL,       
        xml_clear
    }
};

static JSXML *
StartNonListXMLMethod(JSContext *cx, jsval *vp, JSObject **objp)
{
    JSXML *xml;
    JSFunction *fun;
    char numBuf[12];

    JS_ASSERT(!JSVAL_IS_PRIMITIVE(*vp));
    JS_ASSERT(JSVAL_TO_OBJECT(*vp)->isFunction());

    *objp = ToObject(cx, &vp[1]);
    if (!*objp)
        return NULL;
    if (!(*objp)->isXML()) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &XMLClass);
        return NULL;
    }
    xml = (JSXML *) (*objp)->getPrivate();
    if (!xml || xml->xml_class != JSXML_CLASS_LIST)
        return xml;

    if (xml->xml_kids.length == 1) {
        xml = XMLARRAY_MEMBER(&xml->xml_kids, 0, JSXML);
        if (xml) {
            *objp = js_GetXMLObject(cx, xml);
            if (!*objp)
                return NULL;
            vp[1] = OBJECT_TO_JSVAL(*objp);
            return xml;
        }
    }

    fun = JSVAL_TO_OBJECT(*vp)->toFunction();
    JS_snprintf(numBuf, sizeof numBuf, "%u", xml->xml_kids.length);
    JSAutoByteString funNameBytes;
    if (const char *funName = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NON_LIST_XML_METHOD,
                             funName, numBuf);
    }
    return NULL;
}


#define XML_METHOD_PROLOG                                                     \
    JSObject *obj = ToObject(cx, &vp[1]);                                     \
    if (!obj)                                                                 \
        return JS_FALSE;                                                      \
    if (!obj->isXML()) {                                                      \
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &XMLClass);      \
        return JS_FALSE;                                                      \
    }                                                                         \
    JSXML *xml = (JSXML *)obj->getPrivate();                                  \
    if (!xml)                                                                 \
        return JS_FALSE

#define NON_LIST_XML_METHOD_PROLOG                                            \
    JSObject *obj;                                                            \
    JSXML *xml = StartNonListXMLMethod(cx, vp, &obj);                         \
    if (!xml)                                                                 \
        return JS_FALSE;                                                      \
    JS_ASSERT(xml->xml_class != JSXML_CLASS_LIST)

static JSBool
xml_addNamespace(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *ns;

    NON_LIST_XML_METHOD_PROLOG;
    if (xml->xml_class != JSXML_CLASS_ELEMENT)
        goto done;
    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;

    if (!NamespaceHelper(cx, argc == 0 ? -1 : 1, vp + 2, vp))
        return JS_FALSE;
    JS_ASSERT(!JSVAL_IS_PRIMITIVE(*vp));

    ns = JSVAL_TO_OBJECT(*vp);
    if (!AddInScopeNamespace(cx, xml, ns))
        return JS_FALSE;
    ns->setNamespaceDeclared(JSVAL_TRUE);

  done:
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static JSBool
xml_appendChild(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval v;
    JSObject *vobj;
    JSXML *vxml;

    NON_LIST_XML_METHOD_PROLOG;
    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;

    jsid name;
    if (!js_GetAnyName(cx, &name))
        return JS_FALSE;

    if (!GetProperty(cx, obj, name, &v))
        return JS_FALSE;

    JS_ASSERT(!JSVAL_IS_PRIMITIVE(v));
    vobj = JSVAL_TO_OBJECT(v);
    JS_ASSERT(vobj->isXML());
    vxml = (JSXML *) vobj->getPrivate();
    JS_ASSERT(vxml->xml_class == JSXML_CLASS_LIST);

    if (!IndexToId(cx, vxml->xml_kids.length, &name))
        return JS_FALSE;
    *vp = (argc != 0) ? vp[2] : JSVAL_VOID;

    if (!PutProperty(cx, JSVAL_TO_OBJECT(v), name, false, vp))
        return JS_FALSE;

    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}


static JSBool
xml_attribute(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *qn;

    if (argc == 0) {
        js_ReportMissingArg(cx, *vp, 0);
        return JS_FALSE;
    }

    qn = ToAttributeName(cx, vp[2]);
    if (!qn)
        return JS_FALSE;
    vp[2] = OBJECT_TO_JSVAL(qn);        

    jsid id = OBJECT_TO_JSID(qn);
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    return GetProperty(cx, obj, id, vp);
}


static JSBool
xml_attributes(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval name = STRING_TO_JSVAL(cx->runtime->atomState.starAtom);
    JSObject *qn = ToAttributeName(cx, name);
    if (!qn)
        return JS_FALSE;

    jsid id = OBJECT_TO_JSID(qn);
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    return GetProperty(cx, obj, id, vp);
}

static JSXML *
xml_list_helper(JSContext *cx, JSXML *xml, jsval *rval)
{
    JSObject *listobj;
    JSXML *list;

    listobj = js_NewXMLObject(cx, JSXML_CLASS_LIST);
    if (!listobj)
        return NULL;

    *rval = OBJECT_TO_JSVAL(listobj);
    list = (JSXML *) listobj->getPrivate();
    list->xml_target = xml;
    return list;
}

static JSBool
ValueToId(JSContext *cx, jsval v, AutoIdRooter *idr)
{
    if (JSVAL_IS_INT(v)) {
        jsint i = JSVAL_TO_INT(v);
        if (INT_FITS_IN_JSID(i))
            *idr->addr() = INT_TO_JSID(i);
        else if (!js_ValueToStringId(cx, v, idr->addr()))
            return JS_FALSE;
    } else if (JSVAL_IS_STRING(v)) {
        JSAtom *atom = js_AtomizeString(cx, JSVAL_TO_STRING(v));
        if (!atom)
            return JS_FALSE;
        *idr->addr() = ATOM_TO_JSID(atom);
    } else if (!JSVAL_IS_PRIMITIVE(v)) {
        *idr->addr() = OBJECT_TO_JSID(JSVAL_TO_OBJECT(v));
    } else {
        ReportBadXMLName(cx, v);
        return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool
xml_child_helper(JSContext *cx, JSObject *obj, JSXML *xml, jsval name,
                 jsval *rval)
{
    bool isIndex;
    uint32_t index;
    JSXML *kid;
    JSObject *kidobj;

    
    JS_ASSERT(xml->xml_class != JSXML_CLASS_LIST);

    if (!IdValIsIndex(cx, name, &index, &isIndex))
        return JS_FALSE;

    if (isIndex) {
        if (index >= JSXML_LENGTH(xml)) {
            *rval = JSVAL_VOID;
        } else {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, index, JSXML);
            if (!kid) {
                *rval = JSVAL_VOID;
            } else {
                kidobj = js_GetXMLObject(cx, kid);
                if (!kidobj)
                    return JS_FALSE;
                *rval = OBJECT_TO_JSVAL(kidobj);
            }
        }
        return JS_TRUE;
    }

    AutoIdRooter idr(cx);
    if (!ValueToId(cx, name, &idr))
        return JS_FALSE;

    return GetProperty(cx, obj, idr.id(), rval);
}


static JSBool
xml_child(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval v;
    JSXML *list, *vxml;
    JSObject *kidobj;

    XML_METHOD_PROLOG;
    jsval name = argc != 0 ? vp[2] : JSVAL_VOID;
    if (xml->xml_class == JSXML_CLASS_LIST) {
        
        list = xml_list_helper(cx, xml, vp);
        if (!list)
            return JS_FALSE;

        JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
        while (JSXML *kid = cursor.getNext()) {
            kidobj = js_GetXMLObject(cx, kid);
            if (!kidobj)
                return JS_FALSE;
            if (!xml_child_helper(cx, kidobj, kid, name, &v))
                return JS_FALSE;
            if (JSVAL_IS_VOID(v)) {
                
                continue;
            }

            JS_ASSERT(!JSVAL_IS_PRIMITIVE(v));
            vxml = (JSXML *) JSVAL_TO_OBJECT(v)->getPrivate();
            if ((!JSXML_HAS_KIDS(vxml) || vxml->xml_kids.length != 0) &&
                !Append(cx, list, vxml)) {
                return JS_FALSE;
            }
        }
        return JS_TRUE;
    }

    
    if (!xml_child_helper(cx, obj, xml, name, vp))
        return JS_FALSE;
    if (JSVAL_IS_VOID(*vp) && !xml_list_helper(cx, xml, vp))
        return JS_FALSE;
    return JS_TRUE;
}

static JSBool
xml_childIndex(JSContext *cx, unsigned argc, jsval *vp)
{
    JSXML *parent;
    uint32_t i, n;

    NON_LIST_XML_METHOD_PROLOG;
    parent = xml->parent;
    if (!parent || xml->xml_class == JSXML_CLASS_ATTRIBUTE) {
        *vp = DOUBLE_TO_JSVAL(js_NaN);
        return JS_TRUE;
    }
    for (i = 0, n = JSXML_LENGTH(parent); i < n; i++) {
        if (XMLARRAY_MEMBER(&parent->xml_kids, i, JSXML) == xml)
            break;
    }
    JS_ASSERT(i < n);
    if (i <= JSVAL_INT_MAX)
        *vp = INT_TO_JSVAL(i);
    else
        *vp = DOUBLE_TO_JSVAL(i);
    return JS_TRUE;
}


static JSBool
xml_children(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    jsid name = ATOM_TO_JSID(cx->runtime->atomState.starAtom);
    return GetProperty(cx, obj, name, vp);
}


static JSBool
xml_comments_helper(JSContext *cx, JSObject *obj, JSXML *xml, jsval *vp)
{
    JSXML *list, *kid, *vxml;
    JSBool ok;
    uint32_t i, n;
    JSObject *kidobj;
    jsval v;

    list = xml_list_helper(cx, xml, vp);
    if (!list)
        return JS_FALSE;

    ok = JS_TRUE;

    if (xml->xml_class == JSXML_CLASS_LIST) {
        
        for (i = 0, n = JSXML_LENGTH(xml); i < n; i++) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_ELEMENT) {
                ok = js_EnterLocalRootScope(cx);
                if (!ok)
                    break;
                kidobj = js_GetXMLObject(cx, kid);
                if (kidobj) {
                    ok = xml_comments_helper(cx, kidobj, kid, &v);
                } else {
                    ok = JS_FALSE;
                    v = JSVAL_NULL;
                }
                js_LeaveLocalRootScopeWithResult(cx, v);
                if (!ok)
                    break;
                vxml = (JSXML *) JSVAL_TO_OBJECT(v)->getPrivate();
                if (JSXML_LENGTH(vxml) != 0) {
                    ok = Append(cx, list, vxml);
                    if (!ok)
                        break;
                }
            }
        }
    } else {
        
        for (i = 0, n = JSXML_LENGTH(xml); i < n; i++) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_COMMENT) {
                ok = Append(cx, list, kid);
                if (!ok)
                    break;
            }
        }
    }

    return ok;
}

static JSBool
xml_comments(JSContext *cx, unsigned argc, jsval *vp)
{
    XML_METHOD_PROLOG;
    return xml_comments_helper(cx, obj, xml, vp);
}


static JSBool
xml_contains(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval value;
    JSBool eq;
    JSObject *kidobj;

    XML_METHOD_PROLOG;
    value = argc != 0 ? vp[2] : JSVAL_VOID;
    if (xml->xml_class == JSXML_CLASS_LIST) {
        eq = JS_FALSE;
        JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
        while (JSXML *kid = cursor.getNext()) {
            kidobj = js_GetXMLObject(cx, kid);
            if (!kidobj || !js_TestXMLEquality(cx, ObjectValue(*kidobj), value, &eq))
                return JS_FALSE;
            if (eq)
                break;
        }
    } else {
        if (!js_TestXMLEquality(cx, ObjectValue(*obj), value, &eq))
            return JS_FALSE;
    }
    *vp = BOOLEAN_TO_JSVAL(eq);
    return JS_TRUE;
}


static JSBool
xml_copy(JSContext *cx, unsigned argc, jsval *vp)
{
    JSXML *copy;

    XML_METHOD_PROLOG;
    copy = DeepCopy(cx, xml, NULL, 0);
    if (!copy)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(copy->object);
    return JS_TRUE;
}


static JSBool
xml_descendants(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval name;
    JSXML *list;

    XML_METHOD_PROLOG;
    name = argc == 0 ? STRING_TO_JSVAL(cx->runtime->atomState.starAtom) : vp[2];
    list = Descendants(cx, xml, name);
    if (!list)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(list->object);
    return JS_TRUE;
}


static JSBool
xml_elements_helper(JSContext *cx, JSObject *obj, JSXML *xml,
                    JSObject *nameqn, jsval *vp)
{
    JSXML *list, *vxml;
    jsval v;
    JSBool ok;
    JSObject *kidobj;
    uint32_t i, n;

    list = xml_list_helper(cx, xml, vp);
    if (!list)
        return JS_FALSE;

    list->xml_targetprop = nameqn;
    ok = JS_TRUE;

    if (xml->xml_class == JSXML_CLASS_LIST) {
        
        JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
        while (JSXML *kid = cursor.getNext()) {
            if (kid->xml_class == JSXML_CLASS_ELEMENT) {
                ok = js_EnterLocalRootScope(cx);
                if (!ok)
                    break;
                kidobj = js_GetXMLObject(cx, kid);
                if (kidobj) {
                    ok = xml_elements_helper(cx, kidobj, kid, nameqn, &v);
                } else {
                    ok = JS_FALSE;
                    v = JSVAL_NULL;
                }
                js_LeaveLocalRootScopeWithResult(cx, v);
                if (!ok)
                    break;
                vxml = (JSXML *) JSVAL_TO_OBJECT(v)->getPrivate();
                if (JSXML_LENGTH(vxml) != 0) {
                    ok = Append(cx, list, vxml);
                    if (!ok)
                        break;
                }
            }
        }
    } else {
        for (i = 0, n = JSXML_LENGTH(xml); i < n; i++) {
            JSXML *kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_ELEMENT &&
                MatchElemName(nameqn, kid)) {
                ok = Append(cx, list, kid);
                if (!ok)
                    break;
            }
        }
    }

    return ok;
}

static JSBool
xml_elements(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval name;
    JSObject *nameqn;
    jsid funid;

    XML_METHOD_PROLOG;

    name = (argc == 0) ? STRING_TO_JSVAL(cx->runtime->atomState.starAtom) : vp[2];
    nameqn = ToXMLName(cx, name, &funid);
    if (!nameqn)
        return JS_FALSE;

    if (!JSID_IS_VOID(funid))
        return xml_list_helper(cx, xml, vp) != NULL;

    return xml_elements_helper(cx, obj, xml, nameqn, vp);
}


static JSBool
xml_hasOwnProperty(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval name;
    JSBool found;

    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    if (!obj->isXML()) {
        ReportIncompatibleMethod(cx, CallReceiverFromVp(vp), &XMLClass);
        return JS_FALSE;
    }

    name = argc != 0 ? vp[2] : JSVAL_VOID;
    if (!HasProperty(cx, obj, name, &found))
        return JS_FALSE;
    if (found) {
        *vp = JSVAL_TRUE;
        return JS_TRUE;
    }
    return js_HasOwnPropertyHelper(cx, js_LookupProperty, argc, vp);
}


static JSBool
xml_hasComplexContent(JSContext *cx, unsigned argc, jsval *vp)
{
    JSXML *kid;
    JSObject *kidobj;
    uint32_t i, n;

    XML_METHOD_PROLOG;
again:
    switch (xml->xml_class) {
      case JSXML_CLASS_ATTRIBUTE:
      case JSXML_CLASS_COMMENT:
      case JSXML_CLASS_PROCESSING_INSTRUCTION:
      case JSXML_CLASS_TEXT:
        *vp = JSVAL_FALSE;
        break;
      case JSXML_CLASS_LIST:
        if (xml->xml_kids.length == 0) {
            *vp = JSVAL_TRUE;
        } else if (xml->xml_kids.length == 1) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, 0, JSXML);
            if (kid) {
                kidobj = js_GetXMLObject(cx, kid);
                if (!kidobj)
                    return JS_FALSE;
                obj = kidobj;
                xml = (JSXML *) obj->getPrivate();
                goto again;
            }
        }
        
      default:
        *vp = JSVAL_FALSE;
        for (i = 0, n = xml->xml_kids.length; i < n; i++) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_ELEMENT) {
                *vp = JSVAL_TRUE;
                break;
            }
        }
        break;
    }
    return JS_TRUE;
}


static JSBool
xml_hasSimpleContent(JSContext *cx, unsigned argc, jsval *vp)
{
    XML_METHOD_PROLOG;
    *vp = BOOLEAN_TO_JSVAL(HasSimpleContent(xml));
    return JS_TRUE;
}

static JSBool
FindInScopeNamespaces(JSContext *cx, JSXML *xml, JSXMLArray<JSObject> *nsarray)
{
    uint32_t length, i, j, n;
    JSObject *ns, *ns2;
    JSLinearString *prefix, *prefix2;

    length = nsarray->length;
    do {
        if (xml->xml_class != JSXML_CLASS_ELEMENT)
            continue;
        for (i = 0, n = xml->xml_namespaces.length; i < n; i++) {
            ns = XMLARRAY_MEMBER(&xml->xml_namespaces, i, JSObject);
            if (!ns)
                continue;

            prefix = ns->getNamePrefix();
            for (j = 0; j < length; j++) {
                ns2 = XMLARRAY_MEMBER(nsarray, j, JSObject);
                if (ns2) {
                    prefix2 = ns2->getNamePrefix();
                    if ((prefix2 && prefix)
                        ? EqualStrings(prefix2, prefix)
                        : EqualStrings(ns2->getNameURI(), ns->getNameURI())) {
                        break;
                    }
                }
            }

            if (j == length) {
                if (!XMLARRAY_APPEND(cx, nsarray, ns))
                    return JS_FALSE;
                ++length;
            }
        }
    } while ((xml = xml->parent) != NULL);
    JS_ASSERT(length == nsarray->length);

    return JS_TRUE;
}





static bool
NamespacesToJSArray(JSContext *cx, JSXMLArray<JSObject> *array, jsval *rval)
{
    JSObject *arrayobj = NewDenseEmptyArray(cx);
    if (!arrayobj)
        return false;
    *rval = OBJECT_TO_JSVAL(arrayobj);

    AutoValueRooter tvr(cx);
    for (uint32_t i = 0, n = array->length; i < n; i++) {
        JSObject *ns = XMLARRAY_MEMBER(array, i, JSObject);
        if (!ns)
            continue;
        tvr.set(ObjectValue(*ns));
        if (!arrayobj->setElement(cx, i, tvr.addr(), false))
            return false;
    }
    return true;
}

static JSBool
xml_inScopeNamespaces(JSContext *cx, unsigned argc, jsval *vp)
{
    NON_LIST_XML_METHOD_PROLOG;

    AutoNamespaceArray namespaces(cx);
    return FindInScopeNamespaces(cx, xml, &namespaces.array) &&
           NamespacesToJSArray(cx, &namespaces.array, vp);
}

static JSBool
xml_insertChildAfter(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval arg;
    JSXML *kid;
    uint32_t i;

    NON_LIST_XML_METHOD_PROLOG;
    *vp = OBJECT_TO_JSVAL(obj);
    if (!JSXML_HAS_KIDS(xml) || argc == 0)
        return JS_TRUE;

    arg = vp[2];
    if (JSVAL_IS_NULL(arg)) {
        kid = NULL;
        i = 0;
    } else {
        if (!VALUE_IS_XML(arg))
            return JS_TRUE;
        kid = (JSXML *) JSVAL_TO_OBJECT(arg)->getPrivate();
        i = XMLARRAY_FIND_MEMBER(&xml->xml_kids, kid, pointer_match);
        if (i == XML_NOT_FOUND)
            return JS_TRUE;
        ++i;
    }

    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;
    return Insert(cx, xml, i, argc >= 2 ? vp[3] : JSVAL_VOID);
}

static JSBool
xml_insertChildBefore(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval arg;
    JSXML *kid;
    uint32_t i;

    NON_LIST_XML_METHOD_PROLOG;
    *vp = OBJECT_TO_JSVAL(obj);
    if (!JSXML_HAS_KIDS(xml) || argc == 0)
        return JS_TRUE;

    arg = vp[2];
    if (JSVAL_IS_NULL(arg)) {
        kid = NULL;
        i = xml->xml_kids.length;
    } else {
        if (!VALUE_IS_XML(arg))
            return JS_TRUE;
        kid = (JSXML *) JSVAL_TO_OBJECT(arg)->getPrivate();
        i = XMLARRAY_FIND_MEMBER(&xml->xml_kids, kid, pointer_match);
        if (i == XML_NOT_FOUND)
            return JS_TRUE;
    }

    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;
    return Insert(cx, xml, i, argc >= 2 ? vp[3] : JSVAL_VOID);
}


static JSBool
xml_length(JSContext *cx, unsigned argc, jsval *vp)
{
    XML_METHOD_PROLOG;
    if (xml->xml_class != JSXML_CLASS_LIST) {
        *vp = JSVAL_ONE;
    } else {
        uint32_t l = xml->xml_kids.length;
        if (l <= JSVAL_INT_MAX)
            *vp = INT_TO_JSVAL(l);
        else
            *vp = DOUBLE_TO_JSVAL(l);
    }
    return JS_TRUE;
}

static JSBool
xml_localName(JSContext *cx, unsigned argc, jsval *vp)
{
    NON_LIST_XML_METHOD_PROLOG;
    *vp = xml->name ? xml->name->getQNameLocalNameVal() : JSVAL_NULL;
    return JS_TRUE;
}

static JSBool
xml_name(JSContext *cx, unsigned argc, jsval *vp)
{
    NON_LIST_XML_METHOD_PROLOG;
    *vp = OBJECT_TO_JSVAL(xml->name);
    return JS_TRUE;
}

static JSBool
xml_namespace(JSContext *cx, unsigned argc, jsval *vp)
{
    JSLinearString *prefix, *nsprefix;
    jsuint i, length;
    JSObject *ns;

    NON_LIST_XML_METHOD_PROLOG;
    if (argc == 0 && !JSXML_HAS_NAME(xml)) {
        *vp = JSVAL_NULL;
        return true;
    }

    if (argc == 0) {
        prefix = NULL;
    } else {
        JSString *str = ToString(cx, vp[2]);
        if (!str)
            return false;
        prefix = str->ensureLinear(cx);
        if (!prefix)
            return false;
        vp[2] = STRING_TO_JSVAL(prefix);      
    }

    AutoNamespaceArray inScopeNSes(cx);
    if (!FindInScopeNamespaces(cx, xml, &inScopeNSes.array))
        return false;

    if (!prefix) {
        ns = GetNamespace(cx, xml->name, &inScopeNSes.array);
        if (!ns)
            return false;
    } else {
        ns = NULL;
        for (i = 0, length = inScopeNSes.array.length; i < length; i++) {
            ns = XMLARRAY_MEMBER(&inScopeNSes.array, i, JSObject);
            if (ns) {
                nsprefix = ns->getNamePrefix();
                if (nsprefix && EqualStrings(nsprefix, prefix))
                    break;
                ns = NULL;
            }
        }
    }

    *vp = (!ns) ? JSVAL_VOID : OBJECT_TO_JSVAL(ns);
    return true;
}

static JSBool
xml_namespaceDeclarations(JSContext *cx, unsigned argc, jsval *vp)
{
    NON_LIST_XML_METHOD_PROLOG;
    if (JSXML_HAS_VALUE(xml))
        return true;

    AutoNamespaceArray ancestors(cx);
    AutoNamespaceArray declared(cx);

    JSXML *yml = xml;
    while ((yml = yml->parent) != NULL) {
        JS_ASSERT(yml->xml_class == JSXML_CLASS_ELEMENT);
        for (uint32_t i = 0, n = yml->xml_namespaces.length; i < n; i++) {
            JSObject *ns = XMLARRAY_MEMBER(&yml->xml_namespaces, i, JSObject);
            if (ns && !XMLARRAY_HAS_MEMBER(&ancestors.array, ns, namespace_match)) {
                if (!XMLARRAY_APPEND(cx, &ancestors.array, ns))
                    return false;
            }
        }
    }

    for (uint32_t i = 0, n = xml->xml_namespaces.length; i < n; i++) {
        JSObject *ns = XMLARRAY_MEMBER(&xml->xml_namespaces, i, JSObject);
        if (!ns)
            continue;
        if (!IsDeclared(ns))
            continue;
        if (!XMLARRAY_HAS_MEMBER(&ancestors.array, ns, namespace_match)) {
            if (!XMLARRAY_APPEND(cx, &declared.array, ns))
                return false;
        }
    }

    return NamespacesToJSArray(cx, &declared.array, vp);
}

static const char js_attribute_str[] = "attribute";
static const char js_text_str[]      = "text";


const char *js_xml_class_str[] = {
    "list",
    "element",
    js_attribute_str,
    "processing-instruction",
    js_text_str,
    "comment"
};

static JSBool
xml_nodeKind(JSContext *cx, unsigned argc, jsval *vp)
{
    JSString *str;

    NON_LIST_XML_METHOD_PROLOG;
    str = JS_InternString(cx, js_xml_class_str[xml->xml_class]);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static void
NormalizingDelete(JSContext *cx, JSXML *xml, uint32_t index)
{
    if (xml->xml_class == JSXML_CLASS_LIST)
        DeleteListElement(cx, xml, index);
    else
        DeleteByIndex(cx, xml, index);
}


static JSBool
xml_normalize_helper(JSContext *cx, JSObject *obj, JSXML *xml)
{
    JSXML *kid, *kid2;
    uint32_t i, n;
    JSObject *kidobj;
    JSString *str;

    if (!JSXML_HAS_KIDS(xml))
        return JS_TRUE;

    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;

    for (i = 0, n = xml->xml_kids.length; i < n; i++) {
        kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
        if (!kid)
            continue;
        if (kid->xml_class == JSXML_CLASS_ELEMENT) {
            kidobj = js_GetXMLObject(cx, kid);
            if (!kidobj || !xml_normalize_helper(cx, kidobj, kid))
                return JS_FALSE;
        } else if (kid->xml_class == JSXML_CLASS_TEXT) {
            while (i + 1 < n &&
                   (kid2 = XMLARRAY_MEMBER(&xml->xml_kids, i + 1, JSXML)) &&
                   kid2->xml_class == JSXML_CLASS_TEXT) {
                str = js_ConcatStrings(cx, kid->xml_value, kid2->xml_value);
                if (!str)
                    return JS_FALSE;
                NormalizingDelete(cx, xml, i + 1);
                n = xml->xml_kids.length;
                kid->xml_value = str;
            }
            if (kid->xml_value->empty()) {
                NormalizingDelete(cx, xml, i);
                n = xml->xml_kids.length;
                --i;
            }
        }
    }

    return JS_TRUE;
}

static JSBool
xml_normalize(JSContext *cx, unsigned argc, jsval *vp)
{
    XML_METHOD_PROLOG;
    *vp = OBJECT_TO_JSVAL(obj);
    return xml_normalize_helper(cx, obj, xml);
}


static JSBool
xml_parent(JSContext *cx, unsigned argc, jsval *vp)
{
    JSXML *parent, *kid;
    uint32_t i, n;
    JSObject *parentobj;

    XML_METHOD_PROLOG;
    parent = xml->parent;
    if (xml->xml_class == JSXML_CLASS_LIST) {
        *vp = JSVAL_VOID;
        n = xml->xml_kids.length;
        if (n == 0)
            return JS_TRUE;

        kid = XMLARRAY_MEMBER(&xml->xml_kids, 0, JSXML);
        if (!kid)
            return JS_TRUE;
        parent = kid->parent;
        for (i = 1; i < n; i++) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->parent != parent)
                return JS_TRUE;
        }
    }

    if (!parent) {
        *vp = JSVAL_NULL;
        return JS_TRUE;
    }

    parentobj = js_GetXMLObject(cx, parent);
    if (!parentobj)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(parentobj);
    return JS_TRUE;
}


static JSBool
xml_processingInstructions_helper(JSContext *cx, JSObject *obj, JSXML *xml,
                                  JSObject *nameqn, jsval *vp)
{
    JSXML *list, *vxml;
    JSBool ok;
    JSObject *kidobj;
    jsval v;
    uint32_t i, n;

    list = xml_list_helper(cx, xml, vp);
    if (!list)
        return JS_FALSE;

    list->xml_targetprop = nameqn;
    ok = JS_TRUE;

    if (xml->xml_class == JSXML_CLASS_LIST) {
        
        JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
        while (JSXML *kid = cursor.getNext()) {
            if (kid->xml_class == JSXML_CLASS_ELEMENT) {
                ok = js_EnterLocalRootScope(cx);
                if (!ok)
                    break;
                kidobj = js_GetXMLObject(cx, kid);
                if (kidobj) {
                    ok = xml_processingInstructions_helper(cx, kidobj, kid,
                                                           nameqn, &v);
                } else {
                    ok = JS_FALSE;
                    v = JSVAL_NULL;
                }
                js_LeaveLocalRootScopeWithResult(cx, v);
                if (!ok)
                    break;
                vxml = (JSXML *) JSVAL_TO_OBJECT(v)->getPrivate();
                if (JSXML_LENGTH(vxml) != 0) {
                    ok = Append(cx, list, vxml);
                    if (!ok)
                        break;
                }
            }
        }
    } else {
        
        for (i = 0, n = JSXML_LENGTH(xml); i < n; i++) {
            JSXML *kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_PROCESSING_INSTRUCTION) {
                JSLinearString *localName = nameqn->getQNameLocalName();
                if (IS_STAR(localName) ||
                    EqualStrings(localName, kid->name->getQNameLocalName())) {
                    ok = Append(cx, list, kid);
                    if (!ok)
                        break;
                }
            }
        }
    }

    return ok;
}

static JSBool
xml_processingInstructions(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval name;
    JSObject *nameqn;
    jsid funid;

    XML_METHOD_PROLOG;

    name = (argc == 0) ? STRING_TO_JSVAL(cx->runtime->atomState.starAtom) : vp[2];
    nameqn = ToXMLName(cx, name, &funid);
    if (!nameqn)
        return JS_FALSE;
    vp[2] = OBJECT_TO_JSVAL(nameqn);

    if (!JSID_IS_VOID(funid))
        return xml_list_helper(cx, xml, vp) != NULL;

    return xml_processingInstructions_helper(cx, obj, xml, nameqn, vp);
}

static JSBool
xml_prependChild(JSContext *cx, unsigned argc, jsval *vp)
{
    NON_LIST_XML_METHOD_PROLOG;
    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(obj);
    return Insert(cx, xml, 0, argc != 0 ? vp[2] : JSVAL_VOID);
}


static JSBool
xml_propertyIsEnumerable(JSContext *cx, unsigned argc, jsval *vp)
{
    bool isIndex;
    uint32_t index;

    XML_METHOD_PROLOG;
    *vp = JSVAL_FALSE;
    if (argc != 0) {
        if (!IdValIsIndex(cx, vp[2], &index, &isIndex))
            return JS_FALSE;

        if (isIndex) {
            if (xml->xml_class == JSXML_CLASS_LIST) {
                
                *vp = BOOLEAN_TO_JSVAL(index < xml->xml_kids.length);
            } else {
                
                *vp = BOOLEAN_TO_JSVAL(index == 0);
            }
        }
    }
    return JS_TRUE;
}

static JSBool
namespace_full_match(const JSObject *nsa, const JSObject *nsb)
{
    JSLinearString *prefixa = nsa->getNamePrefix();
    JSLinearString *prefixb;

    if (prefixa) {
        prefixb = nsb->getNamePrefix();
        if (prefixb && !EqualStrings(prefixa, prefixb))
            return JS_FALSE;
    }
    return EqualStrings(nsa->getNameURI(), nsb->getNameURI());
}

static JSBool
xml_removeNamespace_helper(JSContext *cx, JSXML *xml, JSObject *ns)
{
    JSObject *thisns, *attrns;
    uint32_t i, n;
    JSXML *attr, *kid;

    thisns = GetNamespace(cx, xml->name, &xml->xml_namespaces);
    JS_ASSERT(thisns);
    if (thisns == ns)
        return JS_TRUE;

    for (i = 0, n = xml->xml_attrs.length; i < n; i++) {
        attr = XMLARRAY_MEMBER(&xml->xml_attrs, i, JSXML);
        if (!attr)
            continue;
        attrns = GetNamespace(cx, attr->name, &xml->xml_namespaces);
        JS_ASSERT(attrns);
        if (attrns == ns)
            return JS_TRUE;
    }

    i = XMLARRAY_FIND_MEMBER(&xml->xml_namespaces, ns, namespace_full_match);
    if (i != XML_NOT_FOUND)
        XMLArrayDelete(cx, &xml->xml_namespaces, i, JS_TRUE);

    for (i = 0, n = xml->xml_kids.length; i < n; i++) {
        kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
        if (kid && kid->xml_class == JSXML_CLASS_ELEMENT) {
            if (!xml_removeNamespace_helper(cx, kid, ns))
                return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSBool
xml_removeNamespace(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *ns;

    NON_LIST_XML_METHOD_PROLOG;
    if (xml->xml_class != JSXML_CLASS_ELEMENT)
        goto done;
    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;

    if (!NamespaceHelper(cx, argc == 0 ? -1 : 1, vp + 2, vp))
        return JS_FALSE;
    JS_ASSERT(!JSVAL_IS_PRIMITIVE(*vp));
    ns = JSVAL_TO_OBJECT(*vp);

    
    if (!xml_removeNamespace_helper(cx, xml, ns))
        return JS_FALSE;
  done:
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static JSBool
xml_replace(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval value;
    JSXML *vxml, *kid;
    uint32_t index, i;
    JSObject *nameqn;

    NON_LIST_XML_METHOD_PROLOG;
    if (xml->xml_class != JSXML_CLASS_ELEMENT)
        goto done;

    if (argc <= 1) {
        value = STRING_TO_JSVAL(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
    } else {
        value = vp[3];
        vxml = VALUE_IS_XML(value)
               ? (JSXML *) JSVAL_TO_OBJECT(value)->getPrivate()
               : NULL;
        if (!vxml) {
            if (!JS_ConvertValue(cx, value, JSTYPE_STRING, &vp[3]))
                return JS_FALSE;
            value = vp[3];
        } else {
            vxml = DeepCopy(cx, vxml, NULL, 0);
            if (!vxml)
                return JS_FALSE;
            value = vp[3] = OBJECT_TO_JSVAL(vxml->object);
        }
    }

    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;

    bool haveIndex;
    if (argc == 0) {
        haveIndex = false;
    } else {
        if (!IdValIsIndex(cx, vp[2], &index, &haveIndex))
            return JS_FALSE;
    }

    if (!haveIndex) {
        



        if (!QNameHelper(cx, argc == 0 ? -1 : 1, vp + 2, vp))
            return JS_FALSE;
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(*vp));
        nameqn = JSVAL_TO_OBJECT(*vp);

        i = xml->xml_kids.length;
        index = XML_NOT_FOUND;
        while (i != 0) {
            --i;
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && MatchElemName(nameqn, kid)) {
                if (i != XML_NOT_FOUND)
                    DeleteByIndex(cx, xml, i);
                index = i;
            }
        }

        if (index == XML_NOT_FOUND)
            goto done;
    }

    if (!Replace(cx, xml, index, value))
        return JS_FALSE;

  done:
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static JSBool
xml_setChildren(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *obj;

    if (!StartNonListXMLMethod(cx, vp, &obj))
        return JS_FALSE;

    *vp = argc != 0 ? vp[2] : JSVAL_VOID;     
    if (!PutProperty(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.starAtom), false, vp))
        return JS_FALSE;

    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static JSBool
xml_setLocalName(JSContext *cx, unsigned argc, jsval *vp)
{
    NON_LIST_XML_METHOD_PROLOG;
    if (!JSXML_HAS_NAME(xml)) {
        vp[0] = JSVAL_VOID;
        return JS_TRUE;
    }

    JSAtom *namestr;
    if (argc == 0) {
        namestr = cx->runtime->atomState.typeAtoms[JSTYPE_VOID];
    } else {
        jsval name = vp[2];
        if (!JSVAL_IS_PRIMITIVE(name) && JSVAL_TO_OBJECT(name)->isQName()) {
            namestr = JSVAL_TO_OBJECT(name)->getQNameLocalName();
        } else {
            if (!js_ValueToAtom(cx, name, &namestr))
                return false;
        }
    }

    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;
    if (namestr)
        xml->name->setQNameLocalName(namestr);
    vp[0] = JSVAL_VOID;
    return JS_TRUE;
}

static JSBool
xml_setName(JSContext *cx, unsigned argc, jsval *vp)
{
    jsval name;
    JSObject *nameqn;
    JSXML *nsowner;
    JSXMLArray<JSObject> *nsarray;
    uint32_t i, n;
    JSObject *ns;

    NON_LIST_XML_METHOD_PROLOG;
    if (!JSXML_HAS_NAME(xml))
        return JS_TRUE;

    if (argc == 0) {
        name = STRING_TO_JSVAL(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
    } else {
        name = vp[2];
        if (!JSVAL_IS_PRIMITIVE(name) &&
            JSVAL_TO_OBJECT(name)->getClass() == &QNameClass &&
            !(nameqn = JSVAL_TO_OBJECT(name))->getNameURI()) {
            name = vp[2] = nameqn->getQNameLocalNameVal();
        }
    }

    nameqn = JS_ConstructObjectWithArguments(cx, Jsvalify(&QNameClass), NULL, 1, &name);
    if (!nameqn)
        return JS_FALSE;

    
    if (xml->xml_class == JSXML_CLASS_PROCESSING_INSTRUCTION)
        nameqn->setNameURI(cx->runtime->emptyString);

    xml = CHECK_COPY_ON_WRITE(cx, xml, obj);
    if (!xml)
        return JS_FALSE;
    xml->name = nameqn;

    






    if (xml->xml_class == JSXML_CLASS_ELEMENT) {
        nsowner = xml;
    } else {
        if (!xml->parent || xml->parent->xml_class != JSXML_CLASS_ELEMENT)
            return JS_TRUE;
        nsowner = xml->parent;
    }

    if (nameqn->getNamePrefix()) {
        









        ns = GetNamespace(cx, nameqn, &nsowner->xml_namespaces);
        if (!ns)
            return JS_FALSE;

        
        if (XMLARRAY_HAS_MEMBER(&nsowner->xml_namespaces, ns, pointer_match)) {
            vp[0] = JSVAL_VOID;
            return JS_TRUE;
        }
    } else {
        













        JS_ASSERT(!nameqn->getNameURI()->empty());

        nsarray = &nsowner->xml_namespaces;
        for (i = 0, n = nsarray->length; i < n; i++) {
            ns = XMLARRAY_MEMBER(nsarray, i, JSObject);
            if (ns && EqualStrings(ns->getNameURI(), nameqn->getNameURI())) {
                nameqn->setNamePrefix(ns->getNamePrefix());
                vp[0] = JSVAL_VOID;
                return JS_TRUE;
            }
        }

        ns = NewXMLNamespace(cx, NULL, nameqn->getNameURI(), JS_TRUE);
        if (!ns)
            return JS_FALSE;
    }

    if (!AddInScopeNamespace(cx, nsowner, ns))
        return JS_FALSE;
    vp[0] = JSVAL_VOID;
    return JS_TRUE;
}


static JSBool qn_match(const JSXML *xml, const JSObject *qn)
{
    return qname_identity(xml->name, qn);
}


static JSBool
xml_setNamespace(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *qn;
    JSObject *ns;
    jsval qnargv[2];
    JSXML *nsowner;

    NON_LIST_XML_METHOD_PROLOG;
    if (!JSXML_HAS_NAME(xml))
        return JS_TRUE;

    ns = JS_ConstructObjectWithArguments(cx, Jsvalify(&NamespaceClass), NULL,
                                         argc == 0 ? 0 : 1, vp + 2);
    if (!ns)
        return JS_FALSE;
    vp[0] = OBJECT_TO_JSVAL(ns);
    ns->setNamespaceDeclared(JSVAL_TRUE);

    qnargv[0] = OBJECT_TO_JSVAL(ns);
    qnargv[1] = OBJECT_TO_JSVAL(xml->name);
    qn = JS_ConstructObjectWithArguments(cx, Jsvalify(&QNameClass), NULL, 2, qnargv);
    if (!qn)
        return JS_FALSE;

    





    if (xml->xml_class == JSXML_CLASS_ATTRIBUTE &&
        xml->parent && xml->parent->xml_class == JSXML_CLASS_ELEMENT &&
        !qn_match(xml, qn))
    {
        JSXMLArray<JSXML> *array = &xml->parent->xml_attrs;
        uint32_t i = XMLArrayFindMember(array, qn, qn_match);
        if (i != XML_NOT_FOUND)
            XMLArrayDelete(cx, array, i, JS_TRUE);
    }

    xml->name = qn;

    



    if (xml->xml_class == JSXML_CLASS_ELEMENT) {
        nsowner = xml;
    } else {
        if (!xml->parent || xml->parent->xml_class != JSXML_CLASS_ELEMENT)
            return JS_TRUE;
        nsowner = xml->parent;
    }
    if (!AddInScopeNamespace(cx, nsowner, ns))
        return JS_FALSE;
    vp[0] = JSVAL_VOID;
    return JS_TRUE;
}


static JSBool
xml_text_helper(JSContext *cx, JSObject *obj, JSXML *xml, jsval *vp)
{
    JSXML *list, *kid, *vxml;
    uint32_t i, n;
    JSObject *kidobj;
    jsval v;

    list = xml_list_helper(cx, xml, vp);
    if (!list)
        return JS_FALSE;

    if (xml->xml_class == JSXML_CLASS_LIST) {
        for (i = 0, n = xml->xml_kids.length; i < n; i++) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_ELEMENT) {
                JSBool ok = js_EnterLocalRootScope(cx);
                if (!ok)
                    break;
                kidobj = js_GetXMLObject(cx, kid);
                if (kidobj) {
                    ok = xml_text_helper(cx, kidobj, kid, &v);
                } else {
                    ok = JS_FALSE;
                    v = JSVAL_NULL;
                }
                js_LeaveLocalRootScopeWithResult(cx, v);
                if (!ok)
                    return JS_FALSE;
                vxml = (JSXML *) JSVAL_TO_OBJECT(v)->getPrivate();
                if (JSXML_LENGTH(vxml) != 0 && !Append(cx, list, vxml))
                    return JS_FALSE;
            }
        }
    } else {
        for (i = 0, n = JSXML_LENGTH(xml); i < n; i++) {
            kid = XMLARRAY_MEMBER(&xml->xml_kids, i, JSXML);
            if (kid && kid->xml_class == JSXML_CLASS_TEXT) {
                if (!Append(cx, list, kid))
                    return JS_FALSE;
            }
        }
    }
    return JS_TRUE;
}

static JSBool
xml_text(JSContext *cx, unsigned argc, jsval *vp)
{
    XML_METHOD_PROLOG;
    return xml_text_helper(cx, obj, xml, vp);
}


static JSString *
xml_toString_helper(JSContext *cx, JSXML *xml)
{
    JSString *str, *kidstr;

    if (xml->xml_class == JSXML_CLASS_ATTRIBUTE ||
        xml->xml_class == JSXML_CLASS_TEXT) {
        return xml->xml_value;
    }

    if (!HasSimpleContent(xml))
        return ToXMLString(cx, OBJECT_TO_JSVAL(xml->object), 0);

    str = cx->runtime->emptyString;
    if (!js_EnterLocalRootScope(cx))
        return NULL;
    JSXMLArrayCursor<JSXML> cursor(&xml->xml_kids);
    while (JSXML *kid = cursor.getNext()) {
        if (kid->xml_class != JSXML_CLASS_COMMENT &&
            kid->xml_class != JSXML_CLASS_PROCESSING_INSTRUCTION) {
            kidstr = xml_toString_helper(cx, kid);
            if (!kidstr) {
                str = NULL;
                break;
            }
            str = js_ConcatStrings(cx, str, kidstr);
            if (!str)
                break;
        }
    }
    js_LeaveLocalRootScopeWithResult(cx, str);
    return str;
}

static JSBool
xml_toSource(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    JSString *str = ToXMLString(cx, OBJECT_TO_JSVAL(obj), TO_SOURCE_FLAG);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
xml_toString(JSContext *cx, unsigned argc, jsval *vp)
{
    JSString *str;

    XML_METHOD_PROLOG;
    str = xml_toString_helper(cx, xml);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}


static JSBool
xml_toXMLString(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    JSString *str = ToXMLString(cx, OBJECT_TO_JSVAL(obj), 0);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}


static JSBool
xml_valueOf(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    *vp = OBJECT_TO_JSVAL(obj);
    return true;
}

static JSFunctionSpec xml_methods[] = {
    JS_FN("addNamespace",          xml_addNamespace,          1,0),
    JS_FN("appendChild",           xml_appendChild,           1,0),
    JS_FN(js_attribute_str,        xml_attribute,             1,0),
    JS_FN("attributes",            xml_attributes,            0,0),
    JS_FN("child",                 xml_child,                 1,0),
    JS_FN("childIndex",            xml_childIndex,            0,0),
    JS_FN("children",              xml_children,              0,0),
    JS_FN("comments",              xml_comments,              0,0),
    JS_FN("contains",              xml_contains,              1,0),
    JS_FN("copy",                  xml_copy,                  0,0),
    JS_FN("descendants",           xml_descendants,           1,0),
    JS_FN("elements",              xml_elements,              1,0),
    JS_FN("hasOwnProperty",        xml_hasOwnProperty,        1,0),
    JS_FN("hasComplexContent",     xml_hasComplexContent,     1,0),
    JS_FN("hasSimpleContent",      xml_hasSimpleContent,      1,0),
    JS_FN("inScopeNamespaces",     xml_inScopeNamespaces,     0,0),
    JS_FN("insertChildAfter",      xml_insertChildAfter,      2,0),
    JS_FN("insertChildBefore",     xml_insertChildBefore,     2,0),
    JS_FN(js_length_str,           xml_length,                0,0),
    JS_FN(js_localName_str,        xml_localName,             0,0),
    JS_FN(js_name_str,             xml_name,                  0,0),
    JS_FN(js_namespace_str,        xml_namespace,             1,0),
    JS_FN("namespaceDeclarations", xml_namespaceDeclarations, 0,0),
    JS_FN("nodeKind",              xml_nodeKind,              0,0),
    JS_FN("normalize",             xml_normalize,             0,0),
    JS_FN(js_xml_parent_str,       xml_parent,                0,0),
    JS_FN("processingInstructions",xml_processingInstructions,1,0),
    JS_FN("prependChild",          xml_prependChild,          1,0),
    JS_FN("propertyIsEnumerable",  xml_propertyIsEnumerable,  1,0),
    JS_FN("removeNamespace",       xml_removeNamespace,       1,0),
    JS_FN("replace",               xml_replace,               2,0),
    JS_FN("setChildren",           xml_setChildren,           1,0),
    JS_FN("setLocalName",          xml_setLocalName,          1,0),
    JS_FN("setName",               xml_setName,               1,0),
    JS_FN("setNamespace",          xml_setNamespace,          1,0),
    JS_FN(js_text_str,             xml_text,                  0,0),
    JS_FN(js_toSource_str,         xml_toSource,              0,0),
    JS_FN(js_toString_str,         xml_toString,              0,0),
    JS_FN(js_toXMLString_str,      xml_toXMLString,           0,0),
    JS_FN(js_valueOf_str,          xml_valueOf,               0,0),
    JS_FS_END
};

static JSBool
CopyXMLSettings(JSContext *cx, JSObject *from, JSObject *to)
{
    int i;
    const char *name;
    jsval v;

    
    for (i = 0; xml_static_props[i].name; i++) {
        name = xml_static_props[i].name;
        if (!JS_GetProperty(cx, from, name, &v))
            return false;
        if (name == js_prettyIndent_str) {
            if (!JSVAL_IS_NUMBER(v))
                continue;
        } else {
            if (!JSVAL_IS_BOOLEAN(v))
                continue;
        }
        if (!JS_SetProperty(cx, to, name, &v))
            return false;
    }

    return true;
}

static JSBool
SetDefaultXMLSettings(JSContext *cx, JSObject *obj)
{
    int i;
    jsval v;

    
    for (i = 0; xml_static_props[i].name; i++) {
        v = (xml_static_props[i].name != js_prettyIndent_str)
            ? JSVAL_TRUE : INT_TO_JSVAL(2);
        if (!JS_SetProperty(cx, obj, xml_static_props[i].name, &v))
            return JS_FALSE;
    }
    return true;
}

static JSBool
xml_settings(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *settings = JS_NewObject(cx, NULL, NULL, NULL);
    if (!settings)
        return false;
    *vp = OBJECT_TO_JSVAL(settings);
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;
    return CopyXMLSettings(cx, obj, settings);
}

static JSBool
xml_setSettings(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *settings;
    jsval v;
    JSBool ok;

    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return JS_FALSE;
    v = (argc == 0) ? JSVAL_VOID : vp[2];
    if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v)) {
        ok = SetDefaultXMLSettings(cx, obj);
    } else {
        if (JSVAL_IS_PRIMITIVE(v)) {
            vp[0] = JSVAL_VOID;
            return JS_TRUE;
        }
        settings = JSVAL_TO_OBJECT(v);
        ok = CopyXMLSettings(cx, settings, obj);
    }
    vp[0] = JSVAL_VOID;
    return ok;
}

static JSBool
xml_defaultSettings(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *settings;

    settings = JS_NewObject(cx, NULL, NULL, NULL);
    if (!settings)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(settings);
    return SetDefaultXMLSettings(cx, settings);
}

static JSFunctionSpec xml_static_methods[] = {
    JS_FN("settings",         xml_settings,          0,0),
    JS_FN("setSettings",      xml_setSettings,       1,0),
    JS_FN("defaultSettings",  xml_defaultSettings,   0,0),
    JS_FS_END
};

static JSBool
XML(JSContext *cx, unsigned argc, Value *vp)
{
    JSXML *xml, *copy;
    JSObject *xobj, *vobj;
    Class *clasp;

    jsval v = argc ? vp[2] : JSVAL_VOID;

    if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v))
        v = STRING_TO_JSVAL(cx->runtime->emptyString);

    xobj = ToXML(cx, v);
    if (!xobj)
        return JS_FALSE;
    xml = (JSXML *) xobj->getPrivate();

    if (IsConstructing(vp) && !JSVAL_IS_PRIMITIVE(v)) {
        vobj = JSVAL_TO_OBJECT(v);
        clasp = vobj->getClass();
        if (clasp == &XMLClass ||
            (clasp->flags & JSCLASS_DOCUMENT_OBSERVER)) {
            copy = DeepCopy(cx, xml, NULL, 0);
            if (!copy)
                return JS_FALSE;
            vp->setObject(*copy->object);
            return JS_TRUE;
        }
    }

    vp->setObject(*xobj);
    return JS_TRUE;
}

static JSBool
XMLList(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *vobj, *listobj;
    JSXML *xml, *list;

    jsval v = argc ? vp[2] : JSVAL_VOID;

    if (JSVAL_IS_NULL(v) || JSVAL_IS_VOID(v))
        v = STRING_TO_JSVAL(cx->runtime->emptyString);

    if (IsConstructing(vp) && !JSVAL_IS_PRIMITIVE(v)) {
        vobj = JSVAL_TO_OBJECT(v);
        if (vobj->isXML()) {
            xml = (JSXML *) vobj->getPrivate();
            if (xml->xml_class == JSXML_CLASS_LIST) {
                listobj = js_NewXMLObject(cx, JSXML_CLASS_LIST);
                if (!listobj)
                    return JS_FALSE;
                *vp = OBJECT_TO_JSVAL(listobj);

                list = (JSXML *) listobj->getPrivate();
                if (!Append(cx, list, xml))
                    return JS_FALSE;
                return JS_TRUE;
            }
        }
    }

    
    listobj = ToXMLList(cx, v);
    if (!listobj)
        return JS_FALSE;

    *vp = OBJECT_TO_JSVAL(listobj);
    return JS_TRUE;
}

#ifdef DEBUG_notme
JSCList xml_leaks = JS_INIT_STATIC_CLIST(&xml_leaks);
uint32_t  xml_serial;
#endif

JSXML *
js_NewXML(JSContext *cx, JSXMLClass xml_class)
{
    JSXML *xml = js_NewGCXML(cx);
    if (!xml)
        return NULL;

    xml->object.init(NULL);
    xml->domnode = NULL;
    xml->parent.init(NULL);
    xml->name.init(NULL);
    xml->xml_class = xml_class;
    xml->xml_flags = 0;
    if (JSXML_CLASS_HAS_VALUE(xml_class)) {
        xml->xml_value.init(cx->runtime->emptyString);
    } else {
        xml->xml_value.init(NULL);
        xml->xml_kids.init();
        if (xml_class == JSXML_CLASS_LIST) {
            xml->xml_target.init(NULL);
            xml->xml_targetprop.init(NULL);
        } else {
            xml->xml_namespaces.init();
            xml->xml_attrs.init();
        }
    }

#ifdef DEBUG_notme
    JS_APPEND_LINK(&xml->links, &xml_leaks);
    xml->serial = xml_serial++;
#endif
    return xml;
}

void
JSXML::writeBarrierPre(JSXML *xml)
{
#ifdef JSGC_INCREMENTAL
    if (!xml)
        return;

    JSCompartment *comp = xml->compartment();
    if (comp->needsBarrier())
        MarkXMLUnbarriered(comp->barrierTracer(), xml, "write barrier");
#endif
}

void
JSXML::writeBarrierPost(JSXML *xml, void *addr)
{
}

void
js_TraceXML(JSTracer *trc, JSXML *xml)
{
    if (xml->object)
        MarkObject(trc, &xml->object, "object");
    if (xml->name)
        MarkObject(trc, &xml->name, "name");
    if (xml->parent)
        MarkXML(trc, &xml->parent, "xml_parent");

    if (JSXML_HAS_VALUE(xml)) {
        if (xml->xml_value)
            MarkString(trc, &xml->xml_value, "value");
        return;
    }

    MarkXMLRange(trc, xml->xml_kids.length, xml->xml_kids.vector, "xml_kids");
    js_XMLArrayCursorTrace(trc, xml->xml_kids.cursors);

    if (xml->xml_class == JSXML_CLASS_LIST) {
        if (xml->xml_target)
            MarkXML(trc, &xml->xml_target, "target");
        if (xml->xml_targetprop)
            MarkObject(trc, &xml->xml_targetprop, "targetprop");
    } else {
        MarkObjectRange(trc, xml->xml_namespaces.length,
                        xml->xml_namespaces.vector,
                        "xml_namespaces");
        js_XMLArrayCursorTrace(trc, xml->xml_namespaces.cursors);

        MarkXMLRange(trc, xml->xml_attrs.length, xml->xml_attrs.vector, "xml_attrs");
        js_XMLArrayCursorTrace(trc, xml->xml_attrs.cursors);
    }
}

JSObject *
js_NewXMLObject(JSContext *cx, JSXMLClass xml_class)
{
    JSXML *xml = js_NewXML(cx, xml_class);
    if (!xml)
        return NULL;

    AutoXMLRooter root(cx, xml);
    return js_GetXMLObject(cx, xml);
}

static JSObject *
NewXMLObject(JSContext *cx, JSXML *xml)
{
    JSObject *obj;

    JSObject *parent = GetGlobalForScopeChain(cx);
    obj = NewObjectWithClassProto(cx, &XMLClass, NULL, parent);
    if (!obj)
        return NULL;
    obj->setPrivate(xml);
    return obj;
}

JSObject *
js_GetXMLObject(JSContext *cx, JSXML *xml)
{
    JSObject *obj;

    obj = xml->object;
    if (obj) {
        JS_ASSERT(obj->getPrivate() == xml);
        return obj;
    }

    obj = NewXMLObject(cx, xml);
    if (!obj)
        return NULL;
    xml->object = obj;
    return obj;
}

JSObject *
js_InitNamespaceClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = &obj->asGlobal();

    JSObject *namespaceProto = global->createBlankPrototype(cx, &NamespaceClass);
    if (!namespaceProto)
        return NULL;
    JSFlatString *empty = cx->runtime->emptyString;
    namespaceProto->setNamePrefix(empty);
    namespaceProto->setNameURI(empty);

    const unsigned NAMESPACE_CTOR_LENGTH = 2;
    JSFunction *ctor = global->createConstructor(cx, Namespace, &NamespaceClass,
                                                 CLASS_ATOM(cx, Namespace),
                                                 NAMESPACE_CTOR_LENGTH);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, namespaceProto))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, namespaceProto, namespace_props, namespace_methods))
        return NULL;

    if (!DefineConstructorAndPrototype(cx, global, JSProto_Namespace, ctor, namespaceProto))
        return NULL;

    return namespaceProto;
}

JSObject *
js_InitQNameClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = &obj->asGlobal();

    JSObject *qnameProto = global->createBlankPrototype(cx, &QNameClass);
    if (!qnameProto)
        return NULL;
    JSAtom *empty = cx->runtime->emptyString;
    if (!InitXMLQName(cx, qnameProto, empty, empty, empty))
        return NULL;

    const unsigned QNAME_CTOR_LENGTH = 2;
    JSFunction *ctor = global->createConstructor(cx, QName, &QNameClass,
                                                 CLASS_ATOM(cx, QName), QNAME_CTOR_LENGTH);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, qnameProto))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, qnameProto, NULL, qname_methods))
        return NULL;

    if (!DefineConstructorAndPrototype(cx, global, JSProto_QName, ctor, qnameProto))
        return NULL;

    return qnameProto;
}

JSObject *
js_InitXMLClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = &obj->asGlobal();

    JSObject *xmlProto = global->createBlankPrototype(cx, &XMLClass);
    if (!xmlProto)
        return NULL;
    JSXML *xml = js_NewXML(cx, JSXML_CLASS_TEXT);
    if (!xml)
        return NULL;
    xmlProto->setPrivate(xml);
    xml->object = xmlProto;

    
    if (!cx->runningWithTrustedPrincipals()) {
        JS_ASSERT(sE4XObjectsCreated > 0);
        --sE4XObjectsCreated;
    }

    const unsigned XML_CTOR_LENGTH = 1;
    JSFunction *ctor = global->createConstructor(cx, XML, &XMLClass, CLASS_ATOM(cx, XML),
                                                 XML_CTOR_LENGTH);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, xmlProto))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, xmlProto, NULL, xml_methods) ||
        !DefinePropertiesAndBrand(cx, ctor, xml_static_props, xml_static_methods))
    {
        return NULL;
    }

    if (!SetDefaultXMLSettings(cx, ctor))
        return NULL;

    
    JSFunction *xmllist =
        JS_DefineFunction(cx, global, js_XMLList_str, XMLList, 1, JSFUN_CONSTRUCTOR);
    if (!xmllist)
        return NULL;
    if (!xmllist->defineProperty(cx, cx->runtime->atomState.classPrototypeAtom,
                                 ObjectValue(*xmlProto), JS_PropertyStub, JS_StrictPropertyStub,
                                 JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return NULL;
    }

    if (!DefineConstructorAndPrototype(cx, global, JSProto_XML, ctor, xmlProto))
        return NULL;

    
    if (!JS_DefineFunction(cx, obj, js_isXMLName_str, xml_isXMLName, 1, 0))
        return NULL;

    return xmlProto;
}

JSObject *
js_InitXMLClasses(JSContext *cx, JSObject *obj)
{
    if (!js_InitNamespaceClass(cx, obj))
        return NULL;
    if (!js_InitQNameClass(cx, obj))
        return NULL;
    return js_InitXMLClass(cx, obj);
}

namespace js {

bool
GlobalObject::getFunctionNamespace(JSContext *cx, Value *vp)
{
    HeapSlot &v = getSlotRef(FUNCTION_NS);
    if (v.isUndefined()) {
        JSRuntime *rt = cx->runtime;
        JSLinearString *prefix = rt->atomState.typeAtoms[JSTYPE_FUNCTION];
        JSLinearString *uri = rt->atomState.functionNamespaceURIAtom;
        JSObject *obj = NewXMLNamespace(cx, prefix, uri, JS_FALSE);
        if (!obj)
            return false;

        






        if (!obj->clearType(cx))
            return false;

        v.set(this, FUNCTION_NS, ObjectValue(*obj));
    }

    *vp = v;
    return true;
}

} 
















JSBool
js_GetDefaultXMLNamespace(JSContext *cx, jsval *vp)
{
    JSObject *ns, *obj, *tmp;
    jsval v;

    JSObject *scopeChain = GetCurrentScopeChain(cx);
    if (!scopeChain)
        return false;

    obj = NULL;
    for (tmp = scopeChain; tmp; tmp = tmp->enclosingScope()) {
        if (tmp->isBlock() || tmp->isWith())
            continue;
        if (!tmp->getSpecial(cx, tmp, SpecialId::defaultXMLNamespace(), &v))
            return JS_FALSE;
        if (!JSVAL_IS_PRIMITIVE(v)) {
            *vp = v;
            return JS_TRUE;
        }
        obj = tmp;
    }

    ns = JS_ConstructObjectWithArguments(cx, Jsvalify(&NamespaceClass), NULL, 0, NULL);
    if (!ns)
        return JS_FALSE;
    v = OBJECT_TO_JSVAL(ns);
    if (!obj->defineSpecial(cx, SpecialId::defaultXMLNamespace(), v,
                            JS_PropertyStub, JS_StrictPropertyStub, JSPROP_PERMANENT)) {
        return JS_FALSE;
    }
    *vp = v;
    return JS_TRUE;
}

JSBool
js_SetDefaultXMLNamespace(JSContext *cx, const Value &v)
{
    Value argv[2];
    argv[0].setString(cx->runtime->emptyString);
    argv[1] = v;
    JSObject *ns = JS_ConstructObjectWithArguments(cx, Jsvalify(&NamespaceClass), NULL, 2, argv);
    if (!ns)
        return JS_FALSE;

    JSObject &varobj = cx->fp()->varObj();
    if (!varobj.defineSpecial(cx, SpecialId::defaultXMLNamespace(), ObjectValue(*ns),
                              JS_PropertyStub, JS_StrictPropertyStub, JSPROP_PERMANENT)) {
        return JS_FALSE;
    }
    return JS_TRUE;
}

JSBool
js_ToAttributeName(JSContext *cx, Value *vp)
{
    JSObject *qn;

    qn = ToAttributeName(cx, *vp);
    if (!qn)
        return JS_FALSE;
    vp->setObject(*qn);
    return JS_TRUE;
}

JSFlatString *
js_EscapeAttributeValue(JSContext *cx, JSString *str, JSBool quote)
{
    StringBuffer sb(cx);
    return EscapeAttributeValue(cx, sb, str, quote);
}

JSString *
js_AddAttributePart(JSContext *cx, JSBool isName, JSString *str, JSString *str2)
{
    size_t len = str->length();
    const jschar *chars = str->getChars(cx);
    if (!chars)
        return NULL;

    size_t len2 = str2->length();
    const jschar *chars2 = str2->getChars(cx);
    if (!chars2)
        return NULL;

    size_t newlen = (isName) ? len + 1 + len2 : len + 2 + len2 + 1;
    jschar *newchars = (jschar *) cx->malloc_((newlen+1) * sizeof(jschar));
    if (!newchars)
        return NULL;

    js_strncpy(newchars, chars, len);
    newchars += len;
    if (isName) {
        *newchars++ = ' ';
        js_strncpy(newchars, chars2, len2);
        newchars += len2;
    } else {
        *newchars++ = '=';
        *newchars++ = '"';
        js_strncpy(newchars, chars2, len2);
        newchars += len2;
        *newchars++ = '"';
    }
    *newchars = 0;
    return js_NewString(cx, newchars - newlen, newlen);
}

JSFlatString *
js_EscapeElementValue(JSContext *cx, JSString *str)
{
    StringBuffer sb(cx);
    return EscapeElementValue(cx, sb, str, 0);
}

JSString *
js_ValueToXMLString(JSContext *cx, const Value &v)
{
    return ToXMLString(cx, v, 0);
}

JSBool
js_GetAnyName(JSContext *cx, jsid *idp)
{
    JSObject *global = cx->hasfp() ? &cx->fp()->scopeChain().global() : cx->globalObject;
    Value v = global->getReservedSlot(JSProto_AnyName);
    if (v.isUndefined()) {
        JSObject *obj = NewObjectWithGivenProto(cx, &AnyNameClass, NULL, global);
        if (!obj)
            return false;

        JS_ASSERT(!obj->getProto());

        JSRuntime *rt = cx->runtime;
        if (!InitXMLQName(cx, obj, rt->emptyString, rt->emptyString, rt->atomState.starAtom))
            return false;

        v.setObject(*obj);
        SetReservedSlot(global, JSProto_AnyName, v);
    }
    *idp = OBJECT_TO_JSID(&v.toObject());
    return true;
}

JSBool
js_FindXMLProperty(JSContext *cx, const Value &nameval, JSObject **objp, jsid *idp)
{
    JSObject *nameobj;
    jsval v;
    JSObject *qn;
    jsid funid;
    JSObject *obj, *target, *proto, *pobj;
    JSXML *xml;
    JSBool found;
    JSProperty *prop;

    JS_ASSERT(nameval.isObject());
    nameobj = &nameval.toObject();
    if (nameobj->getClass() == &AnyNameClass) {
        v = STRING_TO_JSVAL(cx->runtime->atomState.starAtom);
        nameobj = JS_ConstructObjectWithArguments(cx, Jsvalify(&QNameClass), NULL, 1, &v);
        if (!nameobj)
            return JS_FALSE;
    } else {
        JS_ASSERT(nameobj->getClass() == &AttributeNameClass ||
                  nameobj->getClass() == &QNameClass);
    }

    qn = nameobj;

    JSAtom *name;
    funid = GetLocalNameFromFunctionQName(qn, &name, cx)
            ? ATOM_TO_JSID(name)
            : JSID_VOID;

    obj = cx->stack.currentScriptedScopeChain();
    do {
        
        target = obj;
        while (target->getClass() == &WithClass) {
             proto = target->getProto();
             if (!proto)
                 break;
             target = proto;
        }

        if (target->isXML()) {
            if (JSID_IS_VOID(funid)) {
                xml = (JSXML *) target->getPrivate();
                found = HasNamedProperty(xml, qn);
            } else {
                if (!HasFunctionProperty(cx, target, funid, &found))
                    return JS_FALSE;
            }
            if (found) {
                *idp = OBJECT_TO_JSID(nameobj);
                *objp = target;
                return JS_TRUE;
            }
        } else if (!JSID_IS_VOID(funid)) {
            if (!target->lookupGeneric(cx, funid, &pobj, &prop))
                return JS_FALSE;
            if (prop) {
                *idp = funid;
                *objp = target;
                return JS_TRUE;
            }
        }
    } while ((obj = obj->enclosingScope()) != NULL);

    JSAutoByteString printable;
    JSString *str = ConvertQNameToString(cx, nameobj);
    if (str && js_ValueToPrintable(cx, StringValue(str), &printable)) {
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                     JSMSG_UNDEFINED_XML_NAME, printable.ptr());
    }
    return JS_FALSE;
}

static JSBool
GetXMLFunction(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JS_ASSERT(obj->isXML());

    



    JSObject *target = obj;
    for (;;) {
        if (!js_GetProperty(cx, target, id, vp))
            return false;
        if (!JSVAL_IS_PRIMITIVE(*vp) && JSVAL_TO_OBJECT(*vp)->isFunction())
            return true;
        target = target->getProto();
        if (target == NULL || !target->isNative())
            break;
    }

    JSXML *xml = (JSXML *) obj->getPrivate();
    if (!HasSimpleContent(xml))
        return true;

    
    JSObject *proto = obj->global().getOrCreateStringPrototype(cx);
    if (!proto)
        return false;

    return proto->getGeneric(cx, id, vp);
}

static JSXML *
GetPrivate(JSContext *cx, JSObject *obj, const char *method)
{
    if (!obj->isXML()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_METHOD,
                             js_XML_str, method, obj->getClass()->name);
        return NULL;
    }
    return (JSXML *)obj->getPrivate();
}

JSBool
js_GetXMLDescendants(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSXML *xml, *list;

    xml = GetPrivate(cx, obj, "descendants internal method");
    if (!xml)
        return JS_FALSE;

    list = Descendants(cx, xml, id);
    if (!list)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(list->object);
    return JS_TRUE;
}

JSBool
js_DeleteXMLListElements(JSContext *cx, JSObject *listobj)
{
    JSXML *list;
    uint32_t n;

    list = (JSXML *) listobj->getPrivate();
    for (n = list->xml_kids.length; n != 0; --n)
        DeleteListElement(cx, list, 0);

    return JS_TRUE;
}

struct JSXMLFilter
{
    HeapPtr<JSXML>             list;
    HeapPtr<JSXML>             result;
    HeapPtr<JSXML>             kid;
    JSXMLArrayCursor<JSXML>    cursor;

    JSXMLFilter(JSXML *list, JSXMLArray<JSXML> *array)
      : list(list), result(NULL), kid(NULL), cursor(array) {}

    ~JSXMLFilter() {}
};

static void
xmlfilter_trace(JSTracer *trc, JSObject *obj)
{
    JSXMLFilter *filter = (JSXMLFilter *) obj->getPrivate();
    if (!filter)
        return;

    JS_ASSERT(filter->list);
    MarkXML(trc, &filter->list, "list");
    if (filter->result)
        MarkXML(trc, &filter->result, "result");
    if (filter->kid)
        MarkXML(trc, &filter->kid, "kid");

    



}

static void
xmlfilter_finalize(JSContext *cx, JSObject *obj)
{
    JSXMLFilter *filter = (JSXMLFilter *) obj->getPrivate();
    if (!filter)
        return;

    cx->delete_(filter);
}

Class js_XMLFilterClass = {
    "XMLFilter",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_IS_ANONYMOUS,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    xmlfilter_finalize,
    NULL,                    
    NULL,                    
    NULL,                    
    NULL,                    
    xmlfilter_trace
};

JSBool
js_StepXMLListFilter(JSContext *cx, JSBool initialized)
{
    jsval *sp;
    JSObject *obj, *filterobj, *resobj, *kidobj;
    JSXML *xml, *list;
    JSXMLFilter *filter;

    sp = cx->regs().sp;
    if (!initialized) {
        



        if (!VALUE_IS_XML(sp[-2])) {
            js_ReportValueError(cx, JSMSG_NON_XML_FILTER, -2, sp[-2], NULL);
            return JS_FALSE;
        }
        obj = JSVAL_TO_OBJECT(sp[-2]);
        xml = (JSXML *) obj->getPrivate();

        if (xml->xml_class == JSXML_CLASS_LIST) {
            list = xml;
        } else {
            obj = js_NewXMLObject(cx, JSXML_CLASS_LIST);
            if (!obj)
                return JS_FALSE;

            



            sp[-1] = OBJECT_TO_JSVAL(obj);
            list = (JSXML *) obj->getPrivate();
            if (!Append(cx, list, xml))
                return JS_FALSE;
        }

        JSObject *parent = GetGlobalForScopeChain(cx);
        filterobj = NewObjectWithGivenProto(cx, &js_XMLFilterClass, NULL, parent);
        if (!filterobj)
            return JS_FALSE;

        



        filter = cx->new_<JSXMLFilter>(list, &list->xml_kids);
        if (!filter)
            return JS_FALSE;
        filterobj->setPrivate(filter);

        
        sp[-2] = OBJECT_TO_JSVAL(filterobj);

        resobj = js_NewXMLObject(cx, JSXML_CLASS_LIST);
        if (!resobj)
            return JS_FALSE;

        
        filter->result = (JSXML *) resobj->getPrivate();
    } else {
        
        JS_ASSERT(!JSVAL_IS_PRIMITIVE(sp[-2]));
        JS_ASSERT(JSVAL_TO_OBJECT(sp[-2])->getClass() == &js_XMLFilterClass);
        filter = (JSXMLFilter *) JSVAL_TO_OBJECT(sp[-2])->getPrivate();
        JS_ASSERT(filter->kid);

        
        if (js_ValueToBoolean(sp[-1]) &&
            !Append(cx, filter->result, filter->kid)) {
            return JS_FALSE;
        }
    }

    
    filter->kid = filter->cursor.getNext();
    if (!filter->kid) {
        



        filter->cursor.disconnect();
        JS_ASSERT(filter->result->object);
        sp[-2] = OBJECT_TO_JSVAL(filter->result->object);
        kidobj = NULL;
    } else {
        kidobj = js_GetXMLObject(cx, filter->kid);
        if (!kidobj)
            return JS_FALSE;
    }

    
    sp[-1] = OBJECT_TO_JSVAL(kidobj);
    return JS_TRUE;
}

JSObject *
js_ValueToXMLObject(JSContext *cx, const Value &v)
{
    return ToXML(cx, v);
}

JSObject *
js_ValueToXMLListObject(JSContext *cx, const Value &v)
{
    return ToXMLList(cx, v);
}

JSObject *
js_NewXMLSpecialObject(JSContext *cx, JSXMLClass xml_class, JSString *name,
                       JSString *value)
{
    unsigned flags;
    JSObject *obj;
    JSXML *xml;
    JSObject *qn;

    if (!GetXMLSettingFlags(cx, &flags))
        return NULL;

    if ((xml_class == JSXML_CLASS_COMMENT &&
         (flags & XSF_IGNORE_COMMENTS)) ||
        (xml_class == JSXML_CLASS_PROCESSING_INSTRUCTION &&
         (flags & XSF_IGNORE_PROCESSING_INSTRUCTIONS))) {
        return js_NewXMLObject(cx, JSXML_CLASS_TEXT);
    }

    obj = js_NewXMLObject(cx, xml_class);
    if (!obj)
        return NULL;
    xml = (JSXML *) obj->getPrivate();
    if (name) {
        JSAtom *atomName = js_AtomizeString(cx, name);
        if (!atomName)
            return NULL;
        qn = NewXMLQName(cx, cx->runtime->emptyString, NULL, atomName);
        if (!qn)
            return NULL;
        xml->name = qn;
    }
    xml->xml_value = value;
    return obj;
}

JSString *
js_MakeXMLCDATAString(JSContext *cx, JSString *str)
{
    StringBuffer sb(cx);
    return MakeXMLCDATAString(cx, sb, str);
}

JSString *
js_MakeXMLCommentString(JSContext *cx, JSString *str)
{
    StringBuffer sb(cx);
    return MakeXMLCommentString(cx, sb, str);
}

JSString *
js_MakeXMLPIString(JSContext *cx, JSString *name, JSString *str)
{
    StringBuffer sb(cx);
    return MakeXMLPIString(cx, sb, name, str);
}

#endif 
