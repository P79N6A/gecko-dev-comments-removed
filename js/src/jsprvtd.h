






































#ifndef jsprvtd_h___
#define jsprvtd_h___















#include "jspubtd.h"



#define JSID_IS_ATOM(id)            JSVAL_IS_STRING((jsval)(id))
#define JSID_TO_ATOM(id)            ((JSAtom *)(id))
#define ATOM_TO_JSID(atom)          (JS_ASSERT(ATOM_IS_STRING(atom)),         \
                                     (jsid)(atom))

#define JSID_IS_INT(id)             JSVAL_IS_INT((jsval)(id))
#define JSID_TO_INT(id)             JSVAL_TO_INT((jsval)(id))
#define INT_TO_JSID(i)              ((jsid)INT_TO_JSVAL(i))
#define INT_JSVAL_TO_JSID(v)        ((jsid)(v))
#define INT_JSID_TO_JSVAL(id)       ((jsval)(id))

#define JSID_IS_OBJECT(id)          JSVAL_IS_OBJECT((jsval)(id))
#define JSID_TO_OBJECT(id)          JSVAL_TO_OBJECT((jsval)(id))
#define OBJECT_TO_JSID(obj)         ((jsid)OBJECT_TO_JSVAL(obj))
#define OBJECT_JSVAL_TO_JSID(v)     ((jsid)v)






#define JSID_IS_HIDDEN(id)          (JSVAL_TAG((jsval)(id)) == JSVAL_BOOLEAN)

#define JSID_HIDE_NAME(id)                                                    \
    (JS_ASSERT(JSID_IS_ATOM(id)),                                             \
     (jsid)((jsval)(id) ^ (JSVAL_STRING ^ JSVAL_BOOLEAN)))

#define JSID_UNHIDE_NAME(id)                                                  \
    (JS_ASSERT(JSID_IS_HIDDEN(id)),                                           \
     (jsid)((jsval)(id) ^ (JSVAL_BOOLEAN ^ JSVAL_STRING)))


typedef uint8  jsbytecode;
typedef uint8  jssrcnote;
typedef uint32 jsatomid;


typedef struct JSArgumentFormatMap  JSArgumentFormatMap;
typedef struct JSCodeGenerator      JSCodeGenerator;
typedef struct JSDependentString    JSDependentString;
typedef struct JSGCThing            JSGCThing;
typedef struct JSGenerator          JSGenerator;
typedef struct JSParseContext       JSParseContext;
typedef struct JSParsedObjectBox    JSParsedObjectBox;
typedef struct JSParseNode          JSParseNode;
typedef struct JSSharpObjectMap     JSSharpObjectMap;
typedef struct JSTempValueRooter    JSTempValueRooter;
typedef struct JSThread             JSThread;
typedef struct JSToken              JSToken;
typedef struct JSTokenPos           JSTokenPos;
typedef struct JSTokenPtr           JSTokenPtr;
typedef struct JSTokenStream        JSTokenStream;
typedef struct JSTreeContext        JSTreeContext;
typedef struct JSTryNote            JSTryNote;
typedef struct JSWeakRoots          JSWeakRoots;


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

typedef struct JSDebugHooks {
    JSTrapHandler       interruptHandler;
    void                *interruptHandlerData;
    JSNewScriptHook     newScriptHook;
    void                *newScriptHookData;
    JSDestroyScriptHook destroyScriptHook;
    void                *destroyScriptHookData;
    JSTrapHandler       debuggerHandler;
    void                *debuggerHandlerData;
    JSSourceHandler     sourceHandler;
    void                *sourceHandlerData;
    JSInterpreterHook   executeHook;
    void                *executeHookData;
    JSInterpreterHook   callHook;
    void                *callHookData;
    JSObjectHook        objectHook;
    void                *objectHookData;
    JSTrapHandler       throwHook;
    void                *throwHookData;
    JSDebugErrorHook    debugErrorHook;
    void                *debugErrorHookData;
} JSDebugHooks;





typedef void
(* JS_DLL_CALLBACK JSTempValueTrace)(JSTracer *trc, JSTempValueRooter *tvr);

typedef union JSTempValueUnion {
    jsval               value;
    JSObject            *object;
    JSString            *string;
    void                *gcthing;
    JSTempValueTrace    trace;
    JSScopeProperty     *sprop;
    JSWeakRoots         *weakRoots;
    JSParseContext      *parseContext;
    jsval               *array;
} JSTempValueUnion;

struct JSTempValueRooter {
    JSTempValueRooter   *down;
    ptrdiff_t           count;
    JSTempValueUnion    u;
};



#endif 
