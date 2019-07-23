







































#ifndef jsscript_h___
#define jsscript_h___



#include "jsatom.h"
#include "jsprvtd.h"

JS_BEGIN_EXTERN_C








struct JSTryNote {
    ptrdiff_t    start;         
    ptrdiff_t    length;        
    ptrdiff_t    catchStart;    
};

#define JSTRYNOTE_GRAIN         sizeof(ptrdiff_t)
#define JSTRYNOTE_ALIGNMASK     (JSTRYNOTE_GRAIN - 1)

struct JSScript {
    jsbytecode   *code;         
    uint32       length;        
    jsbytecode   *main;         
    uint16       version;       
    uint16       numGlobalVars; 
    JSAtomMap    atomMap;       
    const char   *filename;     
    uintN        lineno;        
    uintN        depth;         
    JSTryNote    *trynotes;     
    JSPrincipals *principals;   
    JSObject     *object;       
};


#define SCRIPT_NOTES(script)    ((jssrcnote*)((script)->code+(script)->length))

#define SCRIPT_FIND_CATCH_START(script, pc, catchpc)                          \
    JS_BEGIN_MACRO                                                            \
        JSTryNote *tn_ = (script)->trynotes;                                  \
        jsbytecode *catchpc_ = NULL;                                          \
        if (tn_) {                                                            \
            ptrdiff_t off_ = PTRDIFF(pc, (script)->main, jsbytecode);         \
            if (off_ >= 0) {                                                  \
                while ((jsuword)(off_ - tn_->start) >= (jsuword)tn_->length)  \
                    ++tn_;                                                    \
                if (tn_->catchStart)                                          \
                    catchpc_ = (script)->main + tn_->catchStart;              \
            }                                                                 \
        }                                                                     \
        catchpc = catchpc_;                                                   \
    JS_END_MACRO






jsbytecode *
js_FindFinallyHandler(JSScript *script, jsbytecode *pc);

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
js_NewScript(JSContext *cx, uint32 length, uint32 snlength, uint32 tnlength);

extern JS_FRIEND_API(JSScript *)
js_NewScriptFromCG(JSContext *cx, JSCodeGenerator *cg, JSFunction *fun);







extern JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun);

extern JS_FRIEND_API(void)
js_CallDestroyScriptHook(JSContext *cx, JSScript *script);

extern void
js_DestroyScript(JSContext *cx, JSScript *script);

extern void
js_MarkScript(JSContext *cx, JSScript *script);






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
