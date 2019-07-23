






































#ifndef jsprvtd_h___
#define jsprvtd_h___















#include "jspubtd.h"


#define JSID_ATOM                   0x0
#define JSID_INT                    0x1
#define JSID_OBJECT                 0x2
#define JSID_TAGMASK                0x3
#define JSID_TAG(id)                ((id) & JSID_TAGMASK)
#define JSID_SETTAG(id,t)           ((id) | (t))
#define JSID_CLRTAG(id)             ((id) & ~(jsid)JSID_TAGMASK)

#define JSID_IS_ATOM(id)            (JSID_TAG(id) == JSID_ATOM)
#define JSID_TO_ATOM(id)            ((JSAtom *)(id))
#define ATOM_TO_JSID(atom)          ((jsid)(atom))
#define ATOM_JSID_TO_JSVAL(id)      ATOM_KEY(JSID_TO_ATOM(id))

#define JSID_IS_INT(id)             ((id) & JSID_INT)
#define JSID_TO_INT(id)             ((jsint)(id) >> 1)
#define INT_TO_JSID(i)              (((jsint)(i) << 1) | JSID_INT)
#define INT_JSID_TO_JSVAL(id)       (id)
#define INT_JSVAL_TO_JSID(v)        (v)

#define JSID_IS_OBJECT(id)          (JSID_TAG(id) == JSID_OBJECT)
#define JSID_TO_OBJECT(id)          ((JSObject *) JSID_CLRTAG(id))
#define OBJECT_TO_JSID(obj)         ((jsid)(obj) | JSID_OBJECT)
#define OBJECT_JSID_TO_JSVAL(id)    OBJECT_TO_JSVAL(JSID_CLRTAG(id))
#define OBJECT_JSVAL_TO_JSID(v)     OBJECT_TO_JSID(JSVAL_TO_OBJECT(v))


typedef uint8  jsbytecode;
typedef uint8  jssrcnote;
typedef uint32 jsatomid;


typedef struct JSArgumentFormatMap  JSArgumentFormatMap;
typedef struct JSCodeGenerator      JSCodeGenerator;
typedef struct JSDependentString    JSDependentString;
typedef struct JSGCThing            JSGCThing;
typedef struct JSGenerator          JSGenerator;
typedef struct JSParseNode          JSParseNode;
typedef struct JSSharpObjectMap     JSSharpObjectMap;
typedef struct JSThread             JSThread;
typedef struct JSToken              JSToken;
typedef struct JSTokenPos           JSTokenPos;
typedef struct JSTokenPtr           JSTokenPtr;
typedef struct JSTokenStream        JSTokenStream;
typedef struct JSTreeContext        JSTreeContext;
typedef struct JSTryNote            JSTryNote;


typedef struct JSAtom               JSAtom;
typedef struct JSAtomList           JSAtomList;
typedef struct JSAtomListElement    JSAtomListElement;
typedef struct JSAtomMap            JSAtomMap;
typedef struct JSAtomState          JSAtomState;
typedef struct JSCodeSpec           JSCodeSpec;
typedef struct JSPrinter            JSPrinter;
typedef struct JSRegExp             JSRegExp;
typedef struct JSRegExpStatics      JSRegExpStatics;
typedef struct JSScope              JSScope;
typedef struct JSScopeOps           JSScopeOps;
typedef struct JSScopeProperty      JSScopeProperty;
typedef struct JSStackHeader        JSStackHeader;
typedef struct JSStringBuffer       JSStringBuffer;
typedef struct JSSubString          JSSubString;
typedef struct JSXML                JSXML;
typedef struct JSXMLNamespace       JSXMLNamespace;
typedef struct JSXMLQName           JSXMLQName;
typedef struct JSXMLArray           JSXMLArray;
typedef struct JSXMLArrayCursor     JSXMLArrayCursor;


typedef enum JSTrapStatus {
    JSTRAP_ERROR,
    JSTRAP_CONTINUE,
    JSTRAP_RETURN,
    JSTRAP_THROW,
    JSTRAP_LIMIT
} JSTrapStatus;

typedef JSTrapStatus
(* JS_DLL_CALLBACK JSTrapHandler)(JSContext *cx, JSScript *script,
                                  jsbytecode *pc, jsval *rval, void *closure);

typedef JSBool
(* JS_DLL_CALLBACK JSWatchPointHandler)(JSContext *cx, JSObject *obj, jsval id,
                                        jsval old, jsval *newp, void *closure);


typedef void
(* JS_DLL_CALLBACK JSNewScriptHook)(JSContext  *cx,
                                    const char *filename,  
                                    uintN      lineno,     
                                    JSScript   *script,
                                    JSFunction *fun,
                                    void       *callerdata);


typedef void
(* JS_DLL_CALLBACK JSDestroyScriptHook)(JSContext *cx,
                                        JSScript  *script,
                                        void      *callerdata);

typedef void
(* JS_DLL_CALLBACK JSSourceHandler)(const char *filename, uintN lineno,
                                    jschar *str, size_t length,
                                    void **listenerTSData, void *closure);


























typedef void *
(* JS_DLL_CALLBACK JSInterpreterHook)(JSContext *cx, JSStackFrame *fp, JSBool before,
                                      JSBool *ok, void *closure);

typedef void
(* JS_DLL_CALLBACK JSObjectHook)(JSContext *cx, JSObject *obj, JSBool isNew,
                                 void *closure);

typedef JSBool
(* JS_DLL_CALLBACK JSDebugErrorHook)(JSContext *cx, const char *message,
                                     JSErrorReport *report, void *closure);

#endif 
