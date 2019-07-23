







































#ifndef jsscript_h___
#define jsscript_h___



#include "jsatom.h"
#include "jsprvtd.h"
#include "jsdbgapi.h"

JS_BEGIN_EXTERN_C





typedef enum JSTryNoteKind {
    JSTRY_CATCH,
    JSTRY_FINALLY,
    JSTRY_ITER
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

typedef struct JSUpvarArray {
    uint32          *vector;    
    uint32          length;     
} JSUpvarArray;

#define CALLEE_UPVAR_SLOT               0xffff
#define FREE_STATIC_LEVEL               0x3fff
#define FREE_UPVAR_COOKIE               0xffffffff
#define MAKE_UPVAR_COOKIE(skip,slot)    ((skip) << 16 | (slot))
#define UPVAR_FRAME_SKIP(cookie)        ((uint32)(cookie) >> 16)
#define UPVAR_FRAME_SLOT(cookie)        ((uint16)(cookie))

#define JS_OBJECT_ARRAY_SIZE(length)                                          \
    (offsetof(JSObjectArray, vector) + sizeof(JSObject *) * (length))

#if defined DEBUG && defined JS_THREADSAFE
# define CHECK_SCRIPT_OWNER 1
#endif

struct JSScript {
    jsbytecode      *code;      
    uint32          length;     
    uint16          version;    
    uint16          nfixed;     

    uint8           objectsOffset;  


    uint8           upvarsOffset;   

    uint8           regexpsOffset;  

    uint8           trynotesOffset; 

    uint8           flags;      
    jsbytecode      *main;      
    JSAtomMap       atomMap;    
    const char      *filename;  
    uint32          lineno;     
    uint16          nslots;     
    uint16          staticLevel;
    JSPrincipals    *principals;
    union {
        JSObject    *object;    
        JSScript    *nextToGC;  
    } u;
#ifdef CHECK_SCRIPT_OWNER
    JSThread        *owner;     
#endif
};

#define JSSF_NO_SCRIPT_RVAL     0x01    /* no need for result value of last
                                           expression statement */
#define JSSF_SAVED_CALLER_FUN   0x02    /* object 0 is caller function */

static JS_INLINE uintN
StackDepth(JSScript *script)
{
    return script->nslots - script->nfixed;
}


#define SCRIPT_NOTES(script)    ((jssrcnote*)((script)->code+(script)->length))

#define JS_SCRIPT_OBJECTS(script)                                             \
    (JS_ASSERT((script)->objectsOffset != 0),                                 \
     (JSObjectArray *)((uint8 *)(script) + (script)->objectsOffset))

#define JS_SCRIPT_UPVARS(script)                                              \
    (JS_ASSERT((script)->upvarsOffset != 0),                                  \
     (JSUpvarArray *)((uint8 *)(script) + (script)->upvarsOffset))

#define JS_SCRIPT_REGEXPS(script)                                             \
    (JS_ASSERT((script)->regexpsOffset != 0),                                 \
     (JSObjectArray *)((uint8 *)(script) + (script)->regexpsOffset))

#define JS_SCRIPT_TRYNOTES(script)                                            \
    (JS_ASSERT((script)->trynotesOffset != 0),                                \
     (JSTryNoteArray *)((uint8 *)(script) + (script)->trynotesOffset))






#define JS_GET_SCRIPT_ATOM(script_, pc_, index, atom)                         \
    JS_BEGIN_MACRO                                                            \
        if ((pc_) < (script_)->code ||                                        \
            (script_)->code + (script_)->length <= (pc_)) {                   \
            JS_ASSERT((size_t)(index) < js_common_atom_count);                \
            (atom) = COMMON_ATOMS_START(&cx->runtime->atomState)[index];      \
        } else {                                                              \
            JSAtomMap *atoms_ = &(script_)->atomMap;                          \
            JS_ASSERT((uint32)(index) < atoms_->length);                      \
            (atom) = atoms_->vector[index];                                   \
        }                                                                     \
    JS_END_MACRO

#define JS_GET_SCRIPT_OBJECT(script, index, obj)                              \
    JS_BEGIN_MACRO                                                            \
        JSObjectArray *objects_ = JS_SCRIPT_OBJECTS(script);                  \
        JS_ASSERT((uint32)(index) < objects_->length);                        \
        (obj) = objects_->vector[index];                                      \
    JS_END_MACRO

#define JS_GET_SCRIPT_FUNCTION(script, index, fun)                            \
    JS_BEGIN_MACRO                                                            \
        JSObject *funobj_;                                                    \
                                                                              \
        JS_GET_SCRIPT_OBJECT(script, index, funobj_);                         \
        JS_ASSERT(HAS_FUNCTION_CLASS(funobj_));                               \
        JS_ASSERT(funobj_ == (JSObject *) funobj_->getAssignedPrivate());     \
        (fun) = (JSFunction *) funobj_;                                       \
        JS_ASSERT(FUN_INTERPRETED(fun));                                      \
    JS_END_MACRO

#define JS_GET_SCRIPT_REGEXP(script, index, obj)                              \
    JS_BEGIN_MACRO                                                            \
        JSObjectArray *regexps_ = JS_SCRIPT_REGEXPS(script);                  \
        JS_ASSERT((uint32)(index) < regexps_->length);                        \
        (obj) = regexps_->vector[index];                                      \
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
             uint32 nobjects, uint32 nupvars, uint32 nregexps,
             uint32 ntrynotes);

extern JSScript *
js_NewScriptFromCG(JSContext *cx, JSCodeGenerator *cg);







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
js_FramePCToLineNumber(JSContext *cx, JSStackFrame *fp);

extern uintN
js_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc);

extern jsbytecode *
js_LineNumberToPC(JSScript *script, uintN lineno);

extern JS_FRIEND_API(uintN)
js_GetScriptLineExtent(JSScript *script);

static JS_INLINE JSOp
js_GetOpcode(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JSOp op = (JSOp) *pc;
    if (op == JSOP_TRAP)
        op = JS_GetTrapOpcode(cx, script, pc);
    return op;
}










extern JSBool
js_XDRScript(JSXDRState *xdr, JSScript **scriptp, JSBool *magic);

JS_END_EXTERN_C

#endif 
