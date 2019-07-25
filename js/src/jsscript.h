







































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

typedef struct JSConstArray {
    js::Value       *vector;    
    uint32          length;
} JSConstArray;

namespace js {

struct GlobalSlotArray {
    struct Entry {
        uint32      atomIndex;  
        uint32      slot;       
    };
    Entry           *vector;
    uint32          length;
};

} 

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

#ifdef JS_METHODJIT
namespace JSC {
    class ExecutablePool;
}
namespace js {
namespace mjit {
namespace ic {
    struct PICInfo;
    struct MICInfo;
}
}
}
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

    uint8           globalsOffset;  

    uint8           constOffset;    

    bool            noScriptRval:1; 

    bool            savedCallerFun:1; 
    bool            hasSharps:1;      
    bool            strictModeCode:1; 
    bool            compileAndGo:1;   
    bool            usesEval:1;       

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
#ifdef JS_METHODJIT
    
    
    void            *ncode;     
    void            **nmap;     
    JSC::ExecutablePool *execPool;  
    js::mjit::ic::PICInfo *pics; 
    js::mjit::ic::MICInfo *mics; 
# ifdef DEBUG
    uint32          jitLength;  

    inline bool isValidJitCode(void *jcode) {
        return (char*)jcode >= (char*)ncode &&
               (char*)jcode < (char*)ncode + jitLength;
    }
# endif

    inline uint32 numPICs() {
        return pics ? *(uint32*)((uint8 *)pics - sizeof(uint32)) : 0;
    }
#endif
#if 0 
    js::TraceTreeCache  *trees; 
    uint32          tmGen;      
#endif
    uint32          tracePoints; 

    
    jssrcnote *notes() { return (jssrcnote *)(code + length); }

    JSObjectArray *objects() {
        JS_ASSERT(objectsOffset != 0);
        return (JSObjectArray *)((uint8 *) this + objectsOffset);
    }

    JSUpvarArray *upvars() {
        JS_ASSERT(upvarsOffset != 0);
        return (JSUpvarArray *) ((uint8 *) this + upvarsOffset);
    }

    JSObjectArray *regexps() {
        JS_ASSERT(regexpsOffset != 0);
        return (JSObjectArray *) ((uint8 *) this + regexpsOffset);
    }

    JSTryNoteArray *trynotes() {
        JS_ASSERT(trynotesOffset != 0);
        return (JSTryNoteArray *) ((uint8 *) this + trynotesOffset);
    }

    js::GlobalSlotArray *globals() {
        JS_ASSERT(globalsOffset != 0);
        return (js::GlobalSlotArray *) ((uint8 *)this + globalsOffset);
    }

    JSConstArray *consts() {
        JS_ASSERT(constOffset != 0);
        return (JSConstArray *) ((uint8 *) this + constOffset);
    }

    JSAtom *getAtom(size_t index) {
        JS_ASSERT(index < atomMap.length);
        return atomMap.vector[index];
    }

    JSObject *getObject(size_t index) {
        JSObjectArray *arr = objects();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    uint32 getGlobalSlot(size_t index) {
        js::GlobalSlotArray *arr = globals();
        JS_ASSERT(index < arr->length);
        return arr->vector[index].slot;
    }

    JSAtom *getGlobalAtom(size_t index) {
        js::GlobalSlotArray *arr = globals();
        JS_ASSERT(index < arr->length);
        return getAtom(arr->vector[index].atomIndex);
    }

    inline JSFunction *getFunction(size_t index);

    inline JSObject *getRegExp(size_t index);

    const js::Value &getConst(size_t index) {
        JSConstArray *arr = consts();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    






    inline bool isEmpty() const;

    



    static JSScript *emptyScript() {
        return const_cast<JSScript *>(&emptyScriptConst);
    }

#ifdef JS_METHODJIT
    


    void *pcToNative(jsbytecode *pc) {
        JS_ASSERT(nmap);
        JS_ASSERT(nmap[pc - code]);
        return nmap[pc - code];
    }
#endif

  private:
    






    static const JSScript emptyScriptConst;
};

#define SHARP_NSLOTS            2       /* [#array, #depth] slots if the script
                                           uses sharp variables */

static JS_INLINE uintN
StackDepth(JSScript *script)
{
    return script->nslots - script->nfixed;
}






#define JS_GET_SCRIPT_ATOM(script_, pc_, index, atom)                         \
    JS_BEGIN_MACRO                                                            \
        if ((pc_) < (script_)->code ||                                        \
            (script_)->code + (script_)->length <= (pc_)) {                   \
            JS_ASSERT((size_t)(index) < js_common_atom_count);                \
            (atom) = COMMON_ATOMS_START(&cx->runtime->atomState)[index];      \
        } else {                                                              \
            (atom) = script_->getAtom(index);                                 \
        }                                                                     \
    JS_END_MACRO

extern JS_FRIEND_DATA(js::Class) js_ScriptClass;

extern JSObject *
js_InitScriptClass(JSContext *cx, JSObject *obj);





extern JSBool
js_InitRuntimeScriptState(JSRuntime *rt);







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
js_MarkScriptFilenames(JSRuntime *rt);

extern void
js_SweepScriptFilenames(JSRuntime *rt);


















extern JSScript *
js_NewScript(JSContext *cx, uint32 length, uint32 nsrcnotes, uint32 natoms,
             uint32 nobjects, uint32 nupvars, uint32 nregexps,
             uint32 ntrynotes, uint32 nconsts, uint32 nglobals);

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
js_XDRScript(JSXDRState *xdr, JSScript **scriptp, bool needMutableScript,
             JSBool *hasMagic);

JS_END_EXTERN_C

#endif 
