







































#ifndef jsscript_h___
#define jsscript_h___



#include "jsatom.h"
#include "jsprvtd.h"
#include "jsdbgapi.h"
#include "jsclist.h"
#include "jsinfer.h"





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

struct Shape;

enum BindingKind { NONE, ARGUMENT, VARIABLE, CONSTANT, UPVAR };







class Bindings {
    js::Shape *lastBinding;
    uint16 nargs;
    uint16 nvars;
    uint16 nupvars;

  public:
    inline Bindings(JSContext *cx)
        : lastBinding(NULL), nargs(0), nvars(0), nupvars(0)
    {
    }

    




    inline void transfer(JSContext *cx, Bindings *bindings);

    




    inline void clone(JSContext *cx, Bindings *bindings);

    uint16 countArgs() const { return nargs; }
    uint16 countVars() const { return nvars; }
    uint16 countUpvars() const { return nupvars; }

    uintN countArgsAndVars() const { return nargs + nvars; }

    uintN countLocalNames() const { return nargs + nvars + nupvars; }

    bool hasUpvars() const { return nupvars > 0; }
    bool hasLocalNames() const { return countLocalNames() > 0; }

    
    inline bool ensureShape(JSContext *cx);

    
    inline js::Shape *lastShape() const;

    
    inline bool extensibleParents();
    bool setExtensibleParents(JSContext *cx);

    bool setParent(JSContext *cx, JSObject *obj);

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

    








    bool getLocalNameArray(JSContext *cx, Vector<JSAtom *> *namesp);

    



    int sharpSlotBase(JSContext *cx);

    




    void makeImmutable();

    








    const js::Shape *lastArgument() const;
    const js::Shape *lastVariable() const;
    const js::Shape *lastUpvar() const;

    void trace(JSTracer *trc);
};

} 

#define JS_OBJECT_ARRAY_SIZE(length)                                          \
    (offsetof(JSObjectArray, vector) + sizeof(JSObject *) * (length))

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

namespace js { namespace mjit { struct JITScript; } }
#endif

namespace js { namespace analyze { class ScriptAnalysis; } }

class JSPCCounters {
    size_t numBytecodes;
    double *counts;

 public:

    enum {
        INTERP = 0,
        TRACEJIT,
        METHODJIT,
        METHODJIT_STUBS,
        METHODJIT_CODE,
        METHODJIT_PICS,
        NUM_COUNTERS
    };

    JSPCCounters() : numBytecodes(0), counts(NULL) {
    }

    ~JSPCCounters() {
        JS_ASSERT(!counts);
    }

    bool init(JSContext *cx, size_t numBytecodes);
    void destroy(JSContext *cx);

    
    operator void*() const {
        return counts;
    }

    double *get(int runmode) {
        JS_ASSERT(runmode >= 0 && runmode < NUM_COUNTERS);
        return counts ? &counts[numBytecodes * runmode] : NULL;
    }

    double& get(int runmode, size_t offset) {
        JS_ASSERT(offset < numBytecodes);
        JS_ASSERT(counts);
        return get(runmode)[offset];
    }
};

static const uint32 JS_SCRIPT_COOKIE = 0xc00cee;

struct JSScript : public js::gc::Cell {
    










    static JSScript *NewScript(JSContext *cx, uint32 length, uint32 nsrcnotes, uint32 natoms,
                               uint32 nobjects, uint32 nupvars, uint32 nregexps,
                               uint32 ntrynotes, uint32 nconsts, uint32 nglobals,
                               uint16 nClosedArgs, uint16 nClosedVars, uint32 nTypeSets,
                               JSVersion version);

    static JSScript *NewScriptFromEmitter(JSContext *cx, js::BytecodeEmitter *bce);

#ifdef JS_CRASH_DIAGNOSTICS
    



    uint32          cookie1[Cell::CellSize / sizeof(uint32)];
#endif
    jsbytecode      *code;      
    uint8           *data;      

    uint32          length;     
  private:
    uint16          version;    

  public:
    uint16          nfixed;     

    



    uint8           objectsOffset;  


    uint8           upvarsOffset;   

    uint8           regexpsOffset;  

    uint8           trynotesOffset; 
    uint8           globalsOffset;  
    uint8           constOffset;    

    uint16          nTypeSets;      


    






    uint32          stepMode;

    uint32          lineno;     

    uint32          mainOffset; 

    bool            noScriptRval:1; 

    bool            savedCallerFun:1; 
    bool            hasSharps:1;      
    bool            strictModeCode:1; 
    bool            compileAndGo:1;   
    bool            usesEval:1;       
    bool            usesArguments:1;  
    bool            warnedAboutTwoArgumentEval:1; 


    bool            warnedAboutUndefinedProp:1; 


    bool            hasSingletons:1;  
    bool            isOuterFunction:1; 
    bool            isInnerFunction:1; 

    bool            isActiveEval:1;   
    bool            isCachedEval:1;   
    bool            usedLazyArgs:1;   
    bool            createdArgs:1;    
    bool            uninlineable:1;   
    bool            reentrantOuterFunction:1; 
    bool            typesPurged:1;    
#ifdef JS_METHODJIT
    bool            debugMode:1;      
    bool            failedBoundsCheck:1; 
#endif
    bool            callDestroyHook:1;

    uint32          natoms;     
    uint16          nslots;     
    uint16          staticLevel;

    uint16          nClosedArgs; 
    uint16          nClosedVars; 

    




#define JS_SCRIPT_INLINE_DATA_LIMIT 4
    uint8           inlineData[JS_SCRIPT_INLINE_DATA_LIMIT];

    const char      *filename;  
    JSAtom          **atoms;    
  private:
    size_t          useCount;  


  public:
    js::Bindings    bindings;   

    JSPrincipals    *principals;
    jschar          *sourceMap; 

    union {
        










        js::GlobalObject    *globalObject;

        
        JSScript            *evalHashLink;
    } u;

    uint32          *closedSlots; 

    
    JSPCCounters    pcCounters;

    
    JSFunction      *function_;

    JSFunction *function() const { return function_; }

#ifdef JS_CRASH_DIAGNOSTICS
    
    uint32          cookie2[Cell::CellSize / sizeof(uint32)];
#endif

#ifdef DEBUG
    



    uint32 id_;
    uint32 idpad;
    unsigned id();
#else
    unsigned id() { return 0; }
#endif

    
    js::types::TypeScript *types;

    
    inline bool ensureHasTypes(JSContext *cx);

    





    inline bool ensureRanAnalysis(JSContext *cx, JSObject *scope);

    
    inline bool ensureRanInference(JSContext *cx);

    inline bool hasAnalysis();
    inline void clearAnalysis();
    inline js::analyze::ScriptAnalysis *analysis();

    



    bool typeSetFunction(JSContext *cx, JSFunction *fun, bool singleton = false);

    inline bool hasGlobal() const;
    inline bool hasClearedGlobal() const;

    inline js::GlobalObject *global() const;
    inline js::types::TypeScriptNesting *nesting() const;

    inline void clearNesting();

    
    js::GlobalObject *getGlobalObjectOrNull() const {
        return isCachedEval ? NULL : u.globalObject;
    }

  private:
    bool makeTypes(JSContext *cx);
    bool makeAnalysis(JSContext *cx);
  public:

#ifdef JS_METHODJIT
    
    
    
    
    void *jitArityCheckNormal;
    void *jitArityCheckCtor;

    js::mjit::JITScript *jitNormal;   
    js::mjit::JITScript *jitCtor;     
#endif

#ifdef JS_METHODJIT
    bool hasJITCode() {
        return jitNormal || jitCtor;
    }

    
    inline void **nativeMap(bool constructing);
    inline void *maybeNativeCodeForPC(bool constructing, jsbytecode *pc);
    inline void *nativeCodeForPC(bool constructing, jsbytecode *pc);

    js::mjit::JITScript *getJIT(bool constructing) {
        return constructing ? jitCtor : jitNormal;
    }

    size_t getUseCount() const  { return useCount; }
    size_t incUseCount() { return ++useCount; }
    size_t *addressOfUseCount() { return &useCount; }
    void resetUseCount() { useCount = 0; }

    JITScriptStatus getJITStatus(bool constructing) {
        void *addr = constructing ? jitArityCheckCtor : jitArityCheckNormal;
        if (addr == NULL)
            return JITScript_None;
        if (addr == JS_UNJITTABLE_SCRIPT)
            return JITScript_Invalid;
        return JITScript_Valid;
    }

    
    JS_FRIEND_API(size_t) jitDataSize(JSUsableSizeFun usf);

#endif

    jsbytecode *main() {
        return code + mainOffset;
    }

    




    JS_FRIEND_API(size_t) dataSize();                       
    JS_FRIEND_API(size_t) dataSize(JSUsableSizeFun usf);    
    uint32 numNotes();                  

    
    jssrcnote *notes() { return (jssrcnote *)(code + length); }

    static const uint8 INVALID_OFFSET = 0xFF;
    static bool isValidOffset(uint8 offset) { return offset != INVALID_OFFSET; }

    JSObjectArray *objects() {
        JS_ASSERT(isValidOffset(objectsOffset));
        return reinterpret_cast<JSObjectArray *>(data + objectsOffset);
    }

    JSUpvarArray *upvars() {
        JS_ASSERT(isValidOffset(upvarsOffset));
        return reinterpret_cast<JSUpvarArray *>(data + upvarsOffset);
    }

    JSObjectArray *regexps() {
        JS_ASSERT(isValidOffset(regexpsOffset));
        return reinterpret_cast<JSObjectArray *>(data + regexpsOffset);
    }

    JSTryNoteArray *trynotes() {
        JS_ASSERT(isValidOffset(trynotesOffset));
        return reinterpret_cast<JSTryNoteArray *>(data + trynotesOffset);
    }

    js::GlobalSlotArray *globals() {
        JS_ASSERT(isValidOffset(globalsOffset));
        return reinterpret_cast<js::GlobalSlotArray *>(data + globalsOffset);
    }

    JSConstArray *consts() {
        JS_ASSERT(isValidOffset(constOffset));
        return reinterpret_cast<JSConstArray *>(data + constOffset);
    }

    JSAtom *getAtom(size_t index) {
        JS_ASSERT(index < natoms);
        return atoms[index];
    }

    JSObject *getObject(size_t index) {
        JSObjectArray *arr = objects();
        JS_ASSERT(index < arr->length);
        return arr->vector[index];
    }

    JSVersion getVersion() const {
        return JSVersion(version);
    }

    inline JSFunction *getFunction(size_t index);
    inline JSFunction *getCallerFunction();

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

  private:
    static const uint32 stepFlagMask = 0x80000000U;
    static const uint32 stepCountMask = 0x7fffffffU;

    



    bool recompileForStepMode(JSContext *cx);

    
    bool tryNewStepMode(JSContext *cx, uint32 newValue);

  public:
    





    bool setStepModeFlag(JSContext *cx, bool step);

    





    bool changeStepModeCount(JSContext *cx, int delta);

    bool stepModeEnabled() { return !!stepMode; }

#ifdef DEBUG
    uint32 stepModeCount() { return stepMode & stepCountMask; }
#endif

    void finalize(JSContext *cx, bool background);
};

JS_STATIC_ASSERT(sizeof(JSScript) % js::gc::Cell::CellSize == 0);

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


extern void
js_MarkScriptFilename(const char *filename);

extern void
js_SweepScriptFilenames(JSCompartment *comp);







extern JS_FRIEND_API(void)
js_CallNewScriptHook(JSContext *cx, JSScript *script, JSFunction *fun);

extern void
js_CallDestroyScriptHook(JSContext *cx, JSScript *script);

namespace js {

#ifdef JS_CRASH_DIAGNOSTICS

void
CheckScript(JSScript *script, JSScript *prev);

#else

inline void
CheckScript(JSScript *script, JSScript *prev)
{
}

#endif 

} 






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

#endif 
