







































#ifndef jsscript_h___
#define jsscript_h___



#include "jsatom.h"
#include "jsprvtd.h"
#include "jsdbgapi.h"
#include "jsclist.h"





typedef enum JSTryNoteKind {
    JSTRY_CATCH,
    JSTRY_FINALLY,
    JSTRY_ITER
} JSTryNoteKind;

namespace js {














class UpvarCookie 
{
    uint32 value;

    static const uint32 FREE_VALUE = 0xfffffffful;

    void checkInvariants() {
        JS_STATIC_ASSERT(sizeof(UpvarCookie) == sizeof(uint32));
        JS_STATIC_ASSERT(UPVAR_LEVEL_LIMIT < FREE_LEVEL);
    }

  public:
    



    static const uint16 FREE_LEVEL = 0x3fff;

    



    static const uint16 UPVAR_LEVEL_LIMIT = 16;
    static const uint16 CALLEE_SLOT = 0xffff;
    static bool isLevelReserved(uint16 level) { return level >= FREE_LEVEL; }

    bool isFree() const { return value == FREE_VALUE; }
    uint32 asInteger() const { return value; }
    
    uint16 level() const { JS_ASSERT(!isFree()); return uint16(value >> 16); }
    uint16 slot() const { JS_ASSERT(!isFree()); return uint16(value); }

    void set(const UpvarCookie &other) { set(other.level(), other.slot()); }
    void set(uint16 newLevel, uint16 newSlot) { value = (uint32(newLevel) << 16) | newSlot; }
    void makeFree() { set(0xffff, 0xffff); JS_ASSERT(isFree()); }
    void fromInteger(uint32 u32) { value = u32; }
};

}




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
    js::UpvarCookie *vector;    
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

struct JITScript;

namespace ic {
# if defined JS_POLYIC
    struct PICInfo;
# endif
# if defined JS_MONOIC
    struct MICInfo;
    struct CallICInfo;
# endif
}
struct CallSite;
}
}
#endif

struct JSScript {
    
    JSCList         links;      
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
    bool            warnedAboutTwoArgumentEval:1; 


#ifdef JS_METHODJIT
    bool            debugMode:1;      
#endif

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
    js::mjit::JITScript *jit;   
# if defined JS_POLYIC
    js::mjit::ic::PICInfo *pics; 
# endif
# if defined JS_MONOIC
    js::mjit::ic::MICInfo *mics; 
    js::mjit::ic::CallICInfo *callICs; 
# endif

    bool isValidJitCode(void *jcode);
#endif

    
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

    JSVersion getVersion() const {
        return JSVersion(version);
    }

    void setVersion(JSVersion newVersion) {
        JS_ASSERT((newVersion & JS_BITMASK(16)) == uint32(newVersion));
        version = newVersion;
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

extern JSBool
js_NewScriptObject(JSContext *cx, JSScript *script);






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

#endif 
