







































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

struct JSArenaPool;

namespace js {

struct GlobalSlotArray {
    struct Entry {
        uint32      atomIndex;  
        uint32      slot;       
    };
    Entry           *vector;
    uint32          length;
};

struct Shape;

enum BindingKind { NONE, ARGUMENT, VARIABLE, CONSTANT, UPVAR };







class Bindings {
    js::Shape *lastBinding;
    uint16 nargs;
    uint16 nvars;
    uint16 nupvars;
    bool hasExtensibleParents;

  public:
    inline Bindings(JSContext *cx, EmptyShape *emptyCallShape);

    




    inline void transfer(JSContext *cx, Bindings *bindings);

    




    inline void clone(JSContext *cx, Bindings *bindings);

    uint16 countArgs() const { return nargs; }
    uint16 countVars() const { return nvars; }
    uint16 countUpvars() const { return nupvars; }

    uintN countArgsAndVars() const { return nargs + nvars; }

    uintN countLocalNames() const { return nargs + nvars + nupvars; }

    bool hasUpvars() const { return nupvars > 0; }
    bool hasLocalNames() const { return countLocalNames() > 0; }

    
    inline js::Shape *lastShape() const;

    enum {
        



        BINDING_COUNT_LIMIT = 0xFFFF
    };

    















    bool add(JSContext *cx, JSAtom *name, BindingKind kind);

    
    bool addVariable(JSContext *cx, JSAtom *name) {
        return add(cx, name, VARIABLE);
    }
    bool addConstant(JSContext *cx, JSAtom *name) {
        return add(cx, name, CONSTANT);
    }
    bool addUpvar(JSContext *cx, JSAtom *name) {
        return add(cx, name, UPVAR);
    }
    bool addArgument(JSContext *cx, JSAtom *name, uint16 *slotp) {
        JS_ASSERT(name != NULL); 
        *slotp = nargs;
        return add(cx, name, ARGUMENT);
    }
    bool addDestructuring(JSContext *cx, uint16 *slotp) {
        *slotp = nargs;
        return add(cx, NULL, ARGUMENT);
    }

    





    BindingKind lookup(JSContext *cx, JSAtom *name, uintN *indexp) const;

    
    bool hasBinding(JSContext *cx, JSAtom *name) const {
        return lookup(cx, name, NULL) != NONE;
    }

    
















    jsuword *
    getLocalNameArray(JSContext *cx, JSArenaPool *pool);

    



    int sharpSlotBase(JSContext *cx);

    




    void makeImmutable();

    












































    void setExtensibleParents() { hasExtensibleParents = true; }
    bool extensibleParents() const { return hasExtensibleParents; }

    










    const js::Shape *lastArgument() const;
    const js::Shape *lastVariable() const;
    const js::Shape *lastUpvar() const;

    void trace(JSTracer *trc);
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

#define JS_UNJITTABLE_SCRIPT (reinterpret_cast<void*>(1))

enum JITScriptStatus {
    JITScript_None,
    JITScript_Invalid,
    JITScript_Valid
};

namespace js {
namespace mjit {

struct JITScript;

}
}
#endif

struct JSScript {
    










    static JSScript *NewScript(JSContext *cx, uint32 length, uint32 nsrcnotes, uint32 natoms,
                               uint32 nobjects, uint32 nupvars, uint32 nregexps,
                               uint32 ntrynotes, uint32 nconsts, uint32 nglobals,
                               uint16 nClosedArgs, uint16 nClosedVars, JSVersion version);

    static JSScript *NewScriptFromCG(JSContext *cx, JSCodeGenerator *cg);

    
    JSCList         links;      
    jsbytecode      *code;      
    uint32          length;     

  private:
    uint16          version;    

    size_t          callCount_; 

  public:
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
    bool            usesArguments:1;  
    bool            warnedAboutTwoArgumentEval:1; 


    bool            hasSingletons:1;  
#ifdef JS_METHODJIT
    bool            debugMode:1;      
    bool            singleStepMode:1; 
#endif

    jsbytecode      *main;      
    JSAtomMap       atomMap;    
    JSCompartment   *compartment; 
    const char      *filename;  
    uint32          lineno;     
    uint16          nslots;     
    uint16          staticLevel;
    uint16          nClosedArgs; 
    uint16          nClosedVars; 
    js::Bindings    bindings;   

    JSPrincipals    *principals;
    union {
        















        JSObject    *object;
        JSScript    *nextToGC;  
    } u;

#ifdef CHECK_SCRIPT_OWNER
    JSThread        *owner;     
#endif

    uint32          *closedSlots; 

  public:
#ifdef JS_METHODJIT
    
    
    
    
    void *jitArityCheckNormal;
    void *jitArityCheckCtor;

    js::mjit::JITScript *jitNormal;   
    js::mjit::JITScript *jitCtor;     

    bool hasJITCode() {
        return jitNormal || jitCtor;
    }

    
    inline void **nativeMap(bool constructing);
    inline void *maybeNativeCodeForPC(bool constructing, jsbytecode *pc);
    inline void *nativeCodeForPC(bool constructing, jsbytecode *pc);

    js::mjit::JITScript *getJIT(bool constructing) {
        return constructing ? jitCtor : jitNormal;
    }

    size_t callCount() const  { return callCount_; }
    size_t incCallCount() { return ++callCount_; }

    JITScriptStatus getJITStatus(bool constructing) {
        void *addr = constructing ? jitArityCheckCtor : jitArityCheckNormal;
        if (addr == NULL)
            return JITScript_None;
        if (addr == JS_UNJITTABLE_SCRIPT)
            return JITScript_Invalid;
        return JITScript_Valid;
    }
#endif

    
    jssrcnote *notes() { return (jssrcnote *)(code + length); }

    static const uint8 INVALID_OFFSET = 0xFF;
    static bool isValidOffset(uint8 offset) { return offset != INVALID_OFFSET; }

    JSObjectArray *objects() {
        JS_ASSERT(isValidOffset(objectsOffset));
        return (JSObjectArray *)((uint8 *) (this + 1) + objectsOffset);
    }

    JSUpvarArray *upvars() {
        JS_ASSERT(isValidOffset(upvarsOffset));
        return (JSUpvarArray *) ((uint8 *) (this + 1) + upvarsOffset);
    }

    JSObjectArray *regexps() {
        JS_ASSERT(isValidOffset(regexpsOffset));
        return (JSObjectArray *) ((uint8 *) (this + 1) + regexpsOffset);
    }

    JSTryNoteArray *trynotes() {
        JS_ASSERT(isValidOffset(trynotesOffset));
        return (JSTryNoteArray *) ((uint8 *) (this + 1) + trynotesOffset);
    }

    js::GlobalSlotArray *globals() {
        JS_ASSERT(isValidOffset(globalsOffset));
        return (js::GlobalSlotArray *) ((uint8 *) (this + 1) + globalsOffset);
    }

    JSConstArray *consts() {
        JS_ASSERT(isValidOffset(constOffset));
        return (JSConstArray *) ((uint8 *) (this + 1) + constOffset);
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

    inline JSFunction *getFunction(size_t index);

    inline JSObject *getRegExp(size_t index);

    const js::Value &getConst(size_t index) {
        JSConstArray *arr = consts();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    




    inline bool isEmpty() const;

    uint32 getClosedArg(uint32 index) {
        JS_ASSERT(index < nClosedArgs);
        return closedSlots[index];
    }

    uint32 getClosedVar(uint32 index) {
        JS_ASSERT(index < nClosedVars);
        return closedSlots[nClosedArgs + index];
    }

    void copyClosedSlotsTo(JSScript *other);
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
            (atom) = cx->runtime->atomState.commonAtomsStart()[index];        \
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

extern void
js_MarkScriptFilename(const char *filename);

extern void
js_MarkScriptFilenames(JSRuntime *rt);

extern void
js_SweepScriptFilenames(JSRuntime *rt);







extern JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun);

extern void
js_CallDestroyScriptHook(JSContext *cx, JSScript *script);





extern void
js_DestroyScript(JSContext *cx, JSScript *script);

extern void
js_DestroyScriptFromGC(JSContext *cx, JSScript *script);







extern void
js_DestroyCachedScript(JSContext *cx, JSScript *script);

extern void
js_TraceScript(JSTracer *trc, JSScript *script);

extern JSObject *
js_NewScriptObject(JSContext *cx, JSScript *script);






#define js_GetSrcNote(script,pc) js_GetSrcNoteCached(cx, script, pc)

extern jssrcnote *
js_GetSrcNoteCached(JSContext *cx, JSScript *script, jsbytecode *pc);






extern uintN
js_FramePCToLineNumber(JSContext *cx, js::StackFrame *fp, jsbytecode *pc);

extern uintN
js_PCToLineNumber(JSContext *cx, JSScript *script, jsbytecode *pc);

extern jsbytecode *
js_LineNumberToPC(JSScript *script, uintN lineno);

extern JS_FRIEND_API(uintN)
js_GetScriptLineExtent(JSScript *script);

namespace js {

extern uintN
CurrentLine(JSContext *cx);










enum LineOption {
    CALLED_FROM_JSOP_EVAL,
    NOT_CALLED_FROM_JSOP_EVAL
};

inline const char *
CurrentScriptFileAndLine(JSContext *cx, uintN *linenop, LineOption = NOT_CALLED_FROM_JSOP_EVAL);

}

static JS_INLINE JSOp
js_GetOpcode(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JSOp op = (JSOp) *pc;
    if (op == JSOP_TRAP)
        op = JS_GetTrapOpcode(cx, script, pc);
    return op;
}

extern JSScript *
js_CloneScript(JSContext *cx, JSScript *script);






extern JSBool
js_XDRScript(JSXDRState *xdr, JSScript **scriptp);

inline bool
JSObject::isScript() const
{
    return getClass() == &js_ScriptClass;
}

inline JSScript *
JSObject::getScript() const
{
    JS_ASSERT(isScript());
    return static_cast<JSScript *>(getPrivate());
}

#endif 
