








































#ifndef jsscript_h___
#define jsscript_h___



#include "jsatom.h"
#include "jsprvtd.h"

JS_BEGIN_EXTERN_C





typedef enum JSTryNoteKind {
    JSTN_CATCH,
    JSTN_FINALLY,
    JSTN_ITER
} JSTryNoteKind;




struct JSTryNote {
    uint8           kind;       
    uint8           padding;    
    uint16          stackDepth; 
    uint32          start;      

    uint32          length;     
};

typedef struct JSTryNoteArray {
    JSTryNote       *vector;    
    uint32          length;     
} JSTryNoteArray;

typedef struct JSObjectArray {
    JSObject        **vector;   
    uint32          length;     
} JSObjectArray;

#define JS_OBJECT_ARRAY_SIZE(length)                                          \
    (offsetof(JSObjectArray, vector) + sizeof(JSObject *) * (length))

struct JSScript {
    jsbytecode      *code;      
    uint32          length;     
    uint16          version;    
    uint16          ngvars;     
    uint8           objectsOffset;  


    uint8           regexpsOffset;  

    uint8           trynotesOffset; 

    jsbytecode      *main;      
    JSAtomMap       atomMap;    
    const char      *filename;  
    uintN           lineno;     
    uintN           depth;      
    JSPrincipals    *principals;
    JSObject        *object;    
};


#define SCRIPT_NOTES(script)    ((jssrcnote*)((script)->code+(script)->length))

#define JS_SCRIPT_OBJECTS(script)                                             \
    (JS_ASSERT((script)->objectsOffset != 0),                                 \
     (JSObjectArray *)((uint8 *)(script) + (script)->objectsOffset))

#define JS_SCRIPT_REGEXPS(script)                                             \
    (JS_ASSERT((script)->regexpsOffset != 0),                                 \
     (JSObjectArray *)((uint8 *)(script) + (script)->regexpsOffset))

#define JS_SCRIPT_TRYNOTES(script)                                            \
    (JS_ASSERT((script)->trynotesOffset != 0),                                \
     (JSTryNoteArray *)((uint8 *)(script) + (script)->trynotesOffset))

#define JS_GET_SCRIPT_ATOM(script, index, atom)                               \
    JS_BEGIN_MACRO                                                            \
        JSAtomMap *atoms_ = &(script)->atomMap;                               \
        JS_ASSERT((uint32)(index) < atoms_->length);                          \
        (atom) = atoms_->vector[(index)];                                     \
    JS_END_MACRO

#define JS_GET_SCRIPT_OBJECT(script, index, obj)                              \
    JS_BEGIN_MACRO                                                            \
        JSObjectArray *objects_ = JS_SCRIPT_OBJECTS(script);                  \
        JS_ASSERT((uint32)(index) < objects_->length);                        \
        (obj) = objects_->vector[(index)];                                    \
    JS_END_MACRO

#define JS_GET_SCRIPT_REGEXP(script, index, obj)                              \
    JS_BEGIN_MACRO                                                            \
        JSObjectArray *regexps_ = JS_SCRIPT_REGEXPS(script);                  \
        JS_ASSERT((uint32)(index) < regexps_->length);                        \
        (obj) = regexps_->vector[(index)];                                    \
        JS_ASSERT(STOBJ_GET_CLASS(obj) == &js_RegExpClass);                   \
    JS_END_MACRO






JSBool
js_IsInsideTryWithFinally(JSScript *script, jsbytecode *pc);

extern JS_FRIEND_DATA(JSClass) js_ScriptClass;

extern JSObject *
js_InitScriptClass(JSContext *cx, JSObject *obj);





extern JSBool
js_InitRuntimeScriptState(JSRuntime *rt);





extern void
js_FinishRuntimeScriptState(JSRuntime *rt);








extern void
js_FreeRuntimeScriptState(JSRuntime *rt);

extern const char *
js_SaveScriptFilename(JSContext *cx, const char *filename);

extern const char *
js_SaveScriptFilenameRT(JSRuntime *rt, const char *filename, uint32 flags);

extern uint32
js_GetScriptFilenameFlags(const char *filename);

extern void
js_MarkScriptFilename(const char *filename);

extern void
js_MarkScriptFilenames(JSRuntime *rt, JSBool keepAtoms);

extern void
js_SweepScriptFilenames(JSRuntime *rt);












extern JSScript *
js_NewScript(JSContext *cx, uint32 length, uint32 nsrcnotes, uint32 natoms,
             uint32 nobjects, uint32 nregexps, uint32 ntrynotes);

extern JS_FRIEND_API(JSScript *)
js_NewScriptFromCG(JSContext *cx, JSCodeGenerator *cg, JSFunction *fun);







extern JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun);

extern JS_FRIEND_API(void)
js_CallDestroyScriptHook(JSContext *cx, JSScript *script);

extern void
js_DestroyScript(JSContext *cx, JSScript *script);

extern void
js_TraceScript(JSTracer *trc, JSScript *script);






#define js_GetSrcNote(script,pc) js_GetSrcNoteCached(cx, script, pc)

extern jssrcnote *
js_GetSrcNoteCached(JSContext *cx, JSScript *script, jsbytecode *pc);


extern uintN
js_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc);

extern jsbytecode *
js_LineNumberToPC(JSScript *script, uintN lineno);

extern JS_FRIEND_API(uintN)
js_GetScriptLineExtent(JSScript *script);










extern JSBool
js_XDRScript(JSXDRState *xdr, JSScript **scriptp, JSBool *magic);

JS_END_EXTERN_C

#endif 
