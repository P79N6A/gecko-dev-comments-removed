








































#ifndef jstracer_h___
#define jstracer_h___

#ifdef JS_TRACER

#include "jscntxt.h"
#include "jstypes.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsinterp.h"
#include "jsbuiltins.h"

#if defined(DEBUG) && !defined(JS_JIT_SPEW)
#define JS_JIT_SPEW
#endif

template <typename T>
class Queue {
    T* _data;
    unsigned _len;
    unsigned _max;
    nanojit::Allocator* alloc;

public:
    void ensure(unsigned size) {
        if (!_max)
            _max = 16;
        while (_max < size)
            _max <<= 1;
        if (alloc) {
            T* tmp = new (*alloc) T[_max];
            memcpy(tmp, _data, _len * sizeof(T));
            _data = tmp;
        } else {
            _data = (T*)realloc(_data, _max * sizeof(T));
        }
#if defined(DEBUG)
        memset(&_data[_len], 0xcd, _max - _len);
#endif
    }

    Queue(nanojit::Allocator* alloc)
        : alloc(alloc)
    {
        this->_max =
        this->_len = 0;
        this->_data = NULL;
    }

    ~Queue() {
        if (!alloc)
            free(_data);
    }

    bool contains(T a) {
        for (unsigned n = 0; n < _len; ++n) {
            if (_data[n] == a)
                return true;
        }
        return false;
    }

    void add(T a) {
        ensure(_len + 1);
        JS_ASSERT(_len <= _max);
        _data[_len++] = a;
    }

    void add(T* chunk, unsigned size) {
        ensure(_len + size);
        JS_ASSERT(_len <= _max);
        memcpy(&_data[_len], chunk, size * sizeof(T));
        _len += size;
    }

    void addUnique(T a) {
        if (!contains(a))
            add(a);
    }

    void setLength(unsigned len) {
        ensure(len + 1);
        _len = len;
    }

    void clear() {
        _len = 0;
    }

    T & get(unsigned i) {
        JS_ASSERT(i < length());
        return _data[i];
    }

    const T & get(unsigned i) const {
        JS_ASSERT(i < length());
        return _data[i];
    }

    T & operator [](unsigned i) {
        return get(i);
    }

    const T & operator [](unsigned i) const {
        return get(i);
    }

    unsigned length() const {
        return _len;
    }

    T* data() const {
        return _data;
    }
};





class Tracker {
    struct Page {
        struct Page*    next;
        jsuword         base;
        nanojit::LIns*  map[1];
    };
    struct Page* pagelist;

    jsuword         getPageBase(const void* v) const;
    struct Page*    findPage(const void* v) const;
    struct Page*    addPage(const void* v);
public:
    Tracker();
    ~Tracker();

    bool            has(const void* v) const;
    nanojit::LIns*  get(const void* v) const;
    void            set(const void* v, nanojit::LIns* ins);
    void            clear();
};

#if defined(JS_JIT_SPEW) || defined(MOZ_NO_VARADIC_MACROS)

enum LC_TMBits {
    



    LC_TMMinimal  = 1<<16,
    LC_TMTracer   = 1<<17,
    LC_TMRecorder = 1<<18,
    LC_TMAbort    = 1<<19,
    LC_TMStats    = 1<<20,
    LC_TMRegexp   = 1<<21,
    LC_TMTreeVis  = 1<<22
};

#endif

#ifdef MOZ_NO_VARADIC_MACROS

#define debug_only_stmt(action)
static void debug_only_printf(int mask, const char *fmt, ...) {}
#define debug_only_print0(mask, str)

#elif defined(JS_JIT_SPEW)


extern nanojit::LogControl js_LogController;

#define debug_only_stmt(stmt) \
    stmt

#define debug_only_printf(mask, fmt, ...)                                      \
    JS_BEGIN_MACRO                                                             \
        if ((js_LogController.lcbits & (mask)) > 0) {                          \
            js_LogController.printf(fmt, __VA_ARGS__);                         \
            fflush(stdout);                                                    \
        }                                                                      \
    JS_END_MACRO

#define debug_only_print0(mask, str)                                           \
    JS_BEGIN_MACRO                                                             \
        if ((js_LogController.lcbits & (mask)) > 0) {                          \
            js_LogController.printf("%s", str);                                \
            fflush(stdout);                                                    \
        }                                                                      \
    JS_END_MACRO

#else

#define debug_only_stmt(action)
#define debug_only_printf(mask, fmt, ...)
#define debug_only_print0(mask, str)

#endif








#define ORACLE_SIZE 4096

class Oracle {
    avmplus::BitSet _stackDontDemote;
    avmplus::BitSet _globalDontDemote;
    avmplus::BitSet _pcDontDemote;
public:
    Oracle();

    JS_REQUIRES_STACK void markGlobalSlotUndemotable(JSContext* cx, unsigned slot);
    JS_REQUIRES_STACK bool isGlobalSlotUndemotable(JSContext* cx, unsigned slot) const;
    JS_REQUIRES_STACK void markStackSlotUndemotable(JSContext* cx, unsigned slot);
    JS_REQUIRES_STACK bool isStackSlotUndemotable(JSContext* cx, unsigned slot) const;
    void markInstructionUndemotable(jsbytecode* pc);
    bool isInstructionUndemotable(jsbytecode* pc) const;

    void clearDemotability();
    void clear() {
        clearDemotability();
    }
};

#if defined(_MSC_VER) && _MSC_VER >= 1400 || (defined(__GNUC__) && __GNUC__ >= 4)
#define USE_TRACE_TYPE_ENUM
#endif













enum JSTraceType_
#if defined(_MSC_VER) && _MSC_VER >= 1400
: int8_t
#endif
{
    TT_OBJECT         = 0, 
    TT_INT32          = 1, 
    TT_DOUBLE         = 2, 
    TT_JSVAL          = 3, 
    TT_STRING         = 4, 
    TT_NULL           = 5, 
    TT_PSEUDOBOOLEAN  = 6, 
    TT_FUNCTION       = 7  
}
#if defined(__GNUC__) && defined(USE_TRACE_TYPE_ENUM)
__attribute__((packed))
#endif
;

#ifdef USE_TRACE_TYPE_ENUM
typedef JSTraceType_ JSTraceType;
#else
typedef int8_t JSTraceType;
#endif






const uint32 TT_INVALID = uint32(-1);

typedef Queue<uint16> SlotList;

class TypeMap : public Queue<JSTraceType> {
public:
    TypeMap(nanojit::Allocator* alloc) : Queue<JSTraceType>(alloc) {}
    JS_REQUIRES_STACK void captureTypes(JSContext* cx, JSObject* globalObj, SlotList& slots, unsigned callDepth);
    JS_REQUIRES_STACK void captureMissingGlobalTypes(JSContext* cx, JSObject* globalObj, SlotList& slots,
                                                     unsigned stackSlots);
    bool matches(TypeMap& other) const;
    void fromRaw(JSTraceType* other, unsigned numSlots);
};

#define JS_TM_EXITCODES(_)    \
    /*                                                                          \
     * An exit at a possible branch-point in the trace at which to attach a     \
     * future secondary trace. Therefore the recorder must generate different   \
     * code to handle the other outcome of the branch condition from the        \
     * primary trace's outcome.                                                 \
     */                                                                         \
    _(BRANCH)                                                                   \
    /*                                                                          \
     * Exit at a tableswitch via a numbered case.                               \
     */                                                                         \
    _(CASE)                                                                     \
    /*                                                                          \
     * Exit at a tableswitch via the default case.                              \
     */                                                                         \
    _(DEFAULT)                                                                  \
    _(LOOP)                                                                     \
    _(NESTED)                                                                   \
    /*                                                                          \
     * An exit from a trace because a condition relied upon at recording time   \
     * no longer holds, where the alternate path of execution is so rare or     \
     * difficult to address in native code that it is not traced at all, e.g.   \
     * negative array index accesses, which differ from positive indexes in     \
     * that they require a string-based property lookup rather than a simple    \
     * memory access.                                                           \
     */                                                                         \
    _(MISMATCH)                                                                 \
    /*                                                                          \
     * A specialization of MISMATCH_EXIT to handle allocation failures.         \
     */                                                                         \
    _(OOM)                                                                      \
    _(OVERFLOW)                                                                 \
    _(UNSTABLE_LOOP)                                                            \
    _(TIMEOUT)                                                                  \
    _(DEEP_BAIL)                                                                \
    _(STATUS)

enum ExitType {
    #define MAKE_EXIT_CODE(x) x##_EXIT,
    JS_TM_EXITCODES(MAKE_EXIT_CODE)
    #undef MAKE_EXIT_CODE
    TOTAL_EXIT_TYPES
};

struct VMSideExit : public nanojit::SideExit
{
    JSObject* block;
    jsbytecode* pc;
    jsbytecode* imacpc;
    intptr_t sp_adj;
    intptr_t rp_adj;
    int32_t calldepth;
    uint32 numGlobalSlots;
    uint32 numStackSlots;
    uint32 numStackSlotsBelowCurrentFrame;
    ExitType exitType;
    uintN lookupFlags;

    



    uintptr_t nativeCalleeWord;

    JSObject * nativeCallee() {
        return (JSObject *) (nativeCalleeWord & ~1);
    }

    bool constructing() {
        return bool(nativeCalleeWord & 1);
    }

    void setNativeCallee(JSObject *callee, bool constructing) {
        nativeCalleeWord = uintptr_t(callee) | (constructing ? 1 : 0);
    }

    inline JSTraceType* stackTypeMap() {
        return (JSTraceType*)(this + 1);
    }

    inline JSTraceType* globalTypeMap() {
        return (JSTraceType*)(this + 1) + this->numStackSlots;
    }

    inline JSTraceType* fullTypeMap() {
        return stackTypeMap();
    }

    inline VMFragment* root() {
        return (VMFragment*)from->root;
    }
};

class VMAllocator : public nanojit::Allocator
{

public:
    VMAllocator() : mOutOfMemory(false), mSize(0)
    {}

    size_t size() {
        return mSize;
    }

    bool outOfMemory() {
        return mOutOfMemory;
    }

    bool mOutOfMemory;
    size_t mSize;

    






    uintptr_t mReserve[0x10000];
};


struct REHashKey {
    size_t re_length;
    uint16 re_flags;
    const jschar* re_chars;

    REHashKey(size_t re_length, uint16 re_flags, const jschar *re_chars)
        : re_length(re_length)
        , re_flags(re_flags)
        , re_chars(re_chars)
    {}

    bool operator==(const REHashKey& other) const
    {
        return ((this->re_length == other.re_length) &&
                (this->re_flags == other.re_flags) &&
                !memcmp(this->re_chars, other.re_chars,
                        this->re_length * sizeof(jschar)));
    }
};

struct REHashFn {
    static size_t hash(const REHashKey& k) {
        return
            k.re_length +
            k.re_flags +
            nanojit::murmurhash(k.re_chars, k.re_length * sizeof(jschar));
    }
};

struct FrameInfo {
    JSObject*       block;      
    jsbytecode*     pc;         
    jsbytecode*     imacpc;     
    uint32          spdist;     

    




    uint32          argc;

    






    uint32          callerHeight;

    
    uint32          callerArgc;

    
    enum { CONSTRUCTING_FLAG = 0x10000 };
    void   set_argc(uint16 argc, bool constructing) {
        this->argc = uint32(argc) | (constructing ? CONSTRUCTING_FLAG: 0);
    }
    uint16 get_argc() const { return argc & ~CONSTRUCTING_FLAG; }
    bool   is_constructing() const { return (argc & CONSTRUCTING_FLAG) != 0; }

    
    JSTraceType* get_typemap() { return (JSTraceType*) (this+1); }
};

struct UnstableExit
{
    nanojit::Fragment* fragment;
    VMSideExit* exit;
    UnstableExit* next;
};

class TreeInfo {
public:
    nanojit::Fragment* const      fragment;
    JSScript*               script;
    unsigned                maxNativeStackSlots;
    ptrdiff_t               nativeStackBase;
    unsigned                maxCallDepth;
    TypeMap                 typeMap;
    unsigned                nStackTypes;
    SlotList*               globalSlots;
    
    Queue<nanojit::Fragment*> dependentTrees;
    
    Queue<nanojit::Fragment*> linkedTrees;
    unsigned                branchCount;
    Queue<VMSideExit*>      sideExits;
    UnstableExit*           unstableExits;
    
    Queue<jsval>            gcthings;
    Queue<JSScopeProperty*> sprops;
#ifdef DEBUG
    const char*             treeFileName;
    uintN                   treeLineNumber;
    uintN                   treePCOffset;
#endif

    TreeInfo(nanojit::Allocator* alloc,
             nanojit::Fragment* _fragment,
             SlotList* _globalSlots)
        : fragment(_fragment),
          script(NULL),
          maxNativeStackSlots(0),
          nativeStackBase(0),
          maxCallDepth(0),
          typeMap(alloc),
          nStackTypes(0),
          globalSlots(_globalSlots),
          dependentTrees(alloc),
          linkedTrees(alloc),
          branchCount(0),
          sideExits(alloc),
          unstableExits(NULL),
          gcthings(alloc),
          sprops(alloc)
    {}

    inline unsigned nGlobalTypes() {
        return typeMap.length() - nStackTypes;
    }
    inline JSTraceType* globalTypeMap() {
        return typeMap.data() + nStackTypes;
    }
    inline JSTraceType* stackTypeMap() {
        return typeMap.data();
    }

    UnstableExit* removeUnstableExit(VMSideExit* exit);
};

#if defined(JS_JIT_SPEW) && (defined(NANOJIT_IA32) || (defined(NANOJIT_AMD64) && defined(__GNUC__)))
# define EXECUTE_TREE_TIMER
#endif

typedef enum JSBuiltinStatus {
    JSBUILTIN_BAILED = 1,
    JSBUILTIN_ERROR = 2
} JSBuiltinStatus;

struct InterpState
{
    double        *sp;                  
    FrameInfo**   rp;                   
    JSContext     *cx;                  
    double        *eos;                 
    void          *eor;                 
    VMSideExit*    lastTreeExitGuard;   
    VMSideExit*    lastTreeCallGuard;   
                                        
    void*          rpAtLastTreeCall;    
    TreeInfo*      outermostTree;       
    double*        stackBase;           
    FrameInfo**    callstackBase;       
    uintN*         inlineCallCountp;    
    VMSideExit**   innermostNestedGuardp;
    void*          stackMark;
    VMSideExit*    innermost;
#ifdef EXECUTE_TREE_TIMER
    uint64         startTime;
#endif
    InterpState*   prev;

    




    uint32         builtinStatus;

    
    double*        deepBailSp;
};

static JS_INLINE void
js_SetBuiltinError(JSContext *cx)
{
    cx->interpState->builtinStatus |= JSBUILTIN_ERROR;
}

#ifdef DEBUG_JSRS_NOT_BOOL
struct JSRecordingStatus {
    int code;
    bool operator==(JSRecordingStatus &s) { return this->code == s.code; };
    bool operator!=(JSRecordingStatus &s) { return this->code != s.code; };
};
enum JSRScodes {
    JSRS_ERROR_code,
    JSRS_STOP_code,
    JSRS_CONTINUE_code,
    JSRS_IMACRO_code
};
struct JSRecordingStatus JSRS_CONTINUE = { JSRS_CONTINUE_code };
struct JSRecordingStatus JSRS_STOP     = { JSRS_STOP_code };
struct JSRecordingStatus JSRS_IMACRO   = { JSRS_IMACRO_code };
struct JSRecordingStatus JSRS_ERROR    = { JSRS_ERROR_code };
#define STATUS_ABORTS_RECORDING(s) ((s) == JSRS_STOP || (s) == JSRS_ERROR)
#else
enum JSRecordingStatus {
    JSRS_ERROR,        
    JSRS_STOP,         
    JSRS_CONTINUE,     
    JSRS_IMACRO        
                       
};
#define STATUS_ABORTS_RECORDING(s) ((s) <= JSRS_STOP)
#endif

class SlotMap;


enum TypeConsensus
{
    TypeConsensus_Okay,         
    TypeConsensus_Undemotes,    
    TypeConsensus_Bad           
};

class TraceRecorder : public avmplus::GCObject {
    JSContext*              cx;
    JSTraceMonitor*         traceMonitor;
    JSObject*               globalObj;
    JSObject*               lexicalBlock;
    Tracker                 tracker;
    Tracker                 nativeFrameTracker;
    char*                   entryTypeMap;
    unsigned                callDepth;
    JSAtom**                atoms;
    VMSideExit*             anchor;
    nanojit::Fragment*      fragment;
    TreeInfo*               treeInfo;
    nanojit::LirBuffer*     lirbuf;
    nanojit::LirWriter*     lir;
    nanojit::LirBufWriter*  lir_buf_writer;
    nanojit::LirWriter*     verbose_filter;
    nanojit::LirWriter*     cse_filter;
    nanojit::LirWriter*     expr_filter;
    nanojit::LirWriter*     func_filter;
    nanojit::LirWriter*     float_filter;
    nanojit::LIns*          cx_ins;
    nanojit::LIns*          eos_ins;
    nanojit::LIns*          eor_ins;
    nanojit::LIns*          rval_ins;
    nanojit::LIns*          inner_sp_ins;
    nanojit::LIns*          native_rval_ins;
    nanojit::LIns*          newobj_ins;
    bool                    deepAborted;
    bool                    trashSelf;
    Queue<nanojit::Fragment*> whichTreesToTrash;
    Queue<jsbytecode*>      cfgMerges;
    jsval*                  global_dslots;
    JSSpecializedNative     generatedSpecializedNative;
    JSSpecializedNative*    pendingSpecializedNative;
    jsval*                  pendingUnboxSlot;
    nanojit::LIns*          pendingGuardCondition;
    TraceRecorder*          nextRecorderToAbort;
    bool                    wasRootFragment;
    jsbytecode*             outer;     
    uint32                  outerArgc; 
    bool                    loop;
    nanojit::LIns*          loopLabel;

    nanojit::LIns* insImmObj(JSObject* obj);
    nanojit::LIns* insImmFun(JSFunction* fun);
    nanojit::LIns* insImmStr(JSString* str);
    nanojit::LIns* insImmSprop(JSScopeProperty* sprop);

    bool isGlobal(jsval* p) const;
    ptrdiff_t nativeGlobalOffset(jsval* p) const;
    JS_REQUIRES_STACK ptrdiff_t nativeStackOffset(jsval* p) const;
    JS_REQUIRES_STACK void import(nanojit::LIns* base, ptrdiff_t offset, jsval* p, JSTraceType t,
                                  const char *prefix, uintN index, JSStackFrame *fp);
    JS_REQUIRES_STACK void import(TreeInfo* treeInfo, nanojit::LIns* sp, unsigned stackSlots,
                                  unsigned callDepth, unsigned ngslots, JSTraceType* typeMap);
    void trackNativeStackUse(unsigned slots);

    JS_REQUIRES_STACK bool isValidSlot(JSScope* scope, JSScopeProperty* sprop);
    JS_REQUIRES_STACK bool lazilyImportGlobalSlot(unsigned slot);

    JS_REQUIRES_STACK void guard(bool expected, nanojit::LIns* cond, ExitType exitType);
    JS_REQUIRES_STACK void guard(bool expected, nanojit::LIns* cond, VMSideExit* exit);

    nanojit::LIns* addName(nanojit::LIns* ins, const char* name);

    nanojit::LIns* writeBack(nanojit::LIns* i, nanojit::LIns* base, ptrdiff_t offset);
    JS_REQUIRES_STACK void set(jsval* p, nanojit::LIns* l, bool initializing = false);
    JS_REQUIRES_STACK nanojit::LIns* get(jsval* p);
    JS_REQUIRES_STACK nanojit::LIns* addr(jsval* p);

    JS_REQUIRES_STACK bool known(jsval* p);
    JS_REQUIRES_STACK void checkForGlobalObjectReallocation();

    JS_REQUIRES_STACK TypeConsensus selfTypeStability(SlotMap& smap);
    JS_REQUIRES_STACK TypeConsensus peerTypeStability(SlotMap& smap, VMFragment** peer);

    JS_REQUIRES_STACK jsval& argval(unsigned n) const;
    JS_REQUIRES_STACK jsval& varval(unsigned n) const;
    JS_REQUIRES_STACK jsval& stackval(int n) const;

    struct NameResult {
        
        
        
        bool             tracked;
        jsval            v;              
        JSObject         *obj;           
        nanojit::LIns    *obj_ins;       
        JSScopeProperty  *sprop;         
    };

    JS_REQUIRES_STACK nanojit::LIns* scopeChain() const;
    JS_REQUIRES_STACK JSStackFrame* frameIfInRange(JSObject* obj, unsigned* depthp = NULL) const;
    JS_REQUIRES_STACK JSRecordingStatus traverseScopeChain(JSObject *obj, nanojit::LIns *obj_ins, JSObject *obj2, nanojit::LIns *&obj2_ins);
    JS_REQUIRES_STACK JSRecordingStatus scopeChainProp(JSObject* obj, jsval*& vp, nanojit::LIns*& ins, NameResult& nr);
    JS_REQUIRES_STACK JSRecordingStatus callProp(JSObject* obj, JSProperty* sprop, jsid id, jsval*& vp, nanojit::LIns*& ins, NameResult& nr);

    JS_REQUIRES_STACK nanojit::LIns* arg(unsigned n);
    JS_REQUIRES_STACK void arg(unsigned n, nanojit::LIns* i);
    JS_REQUIRES_STACK nanojit::LIns* var(unsigned n);
    JS_REQUIRES_STACK void var(unsigned n, nanojit::LIns* i);
    JS_REQUIRES_STACK nanojit::LIns* upvar(JSScript* script, JSUpvarArray* uva, uintN index, jsval& v);
    nanojit::LIns* stackLoad(nanojit::LIns* addr, uint8 type);
    JS_REQUIRES_STACK nanojit::LIns* stack(int n);
    JS_REQUIRES_STACK void stack(int n, nanojit::LIns* i);

    JS_REQUIRES_STACK nanojit::LIns* alu(nanojit::LOpcode op, jsdouble v0, jsdouble v1,
                                         nanojit::LIns* s0, nanojit::LIns* s1);
    nanojit::LIns* f2i(nanojit::LIns* f);
    JS_REQUIRES_STACK nanojit::LIns* makeNumberInt32(nanojit::LIns* f);
    JS_REQUIRES_STACK nanojit::LIns* stringify(jsval& v);

    JS_REQUIRES_STACK JSRecordingStatus call_imacro(jsbytecode* imacro);

    JS_REQUIRES_STACK JSRecordingStatus ifop();
    JS_REQUIRES_STACK JSRecordingStatus switchop();
#ifdef NANOJIT_IA32
    JS_REQUIRES_STACK JSRecordingStatus tableswitch();
#endif
    JS_REQUIRES_STACK JSRecordingStatus inc(jsval& v, jsint incr, bool pre = true);
    JS_REQUIRES_STACK JSRecordingStatus inc(jsval v, nanojit::LIns*& v_ins, jsint incr,
                                            bool pre = true);
    JS_REQUIRES_STACK JSRecordingStatus incHelper(jsval v, nanojit::LIns* v_ins,
                                                  nanojit::LIns*& v_after, jsint incr);
    JS_REQUIRES_STACK JSRecordingStatus incProp(jsint incr, bool pre = true);
    JS_REQUIRES_STACK JSRecordingStatus incElem(jsint incr, bool pre = true);
    JS_REQUIRES_STACK JSRecordingStatus incName(jsint incr, bool pre = true);

    JS_REQUIRES_STACK void strictEquality(bool equal, bool cmpCase);
    JS_REQUIRES_STACK JSRecordingStatus equality(bool negate, bool tryBranchAfterCond);
    JS_REQUIRES_STACK JSRecordingStatus equalityHelper(jsval l, jsval r,
                                                       nanojit::LIns* l_ins, nanojit::LIns* r_ins,
                                                       bool negate, bool tryBranchAfterCond,
                                                       jsval& rval);
    JS_REQUIRES_STACK JSRecordingStatus relational(nanojit::LOpcode op, bool tryBranchAfterCond);

    JS_REQUIRES_STACK JSRecordingStatus unary(nanojit::LOpcode op);
    JS_REQUIRES_STACK JSRecordingStatus binary(nanojit::LOpcode op);

    bool ibinary(nanojit::LOpcode op);
    bool iunary(nanojit::LOpcode op);
    bool bbinary(nanojit::LOpcode op);
    void demote(jsval& v, jsdouble result);

    inline nanojit::LIns* map(nanojit::LIns *obj_ins);
    JS_REQUIRES_STACK bool map_is_native(JSObjectMap* map, nanojit::LIns* map_ins,
                                         nanojit::LIns*& ops_ins, size_t op_offset = 0);
    JS_REQUIRES_STACK JSRecordingStatus test_property_cache(JSObject* obj, nanojit::LIns* obj_ins,
                                                            JSObject*& obj2, jsuword& pcval);
    JS_REQUIRES_STACK JSRecordingStatus guardNativePropertyOp(JSObject* aobj,
                                                              nanojit::LIns* map_ins);
    JS_REQUIRES_STACK JSRecordingStatus guardPropertyCacheHit(nanojit::LIns* obj_ins,
                                                              nanojit::LIns* map_ins,
                                                              JSObject* aobj,
                                                              JSObject* obj2,
                                                              JSPropCacheEntry* entry,
                                                              jsuword& pcval);

    void stobj_set_fslot(nanojit::LIns *obj_ins, unsigned slot,
                         nanojit::LIns* v_ins);
    void stobj_set_dslot(nanojit::LIns *obj_ins, unsigned slot, nanojit::LIns*& dslots_ins,
                         nanojit::LIns* v_ins);
    void stobj_set_slot(nanojit::LIns* obj_ins, unsigned slot, nanojit::LIns*& dslots_ins,
                        nanojit::LIns* v_ins);

    nanojit::LIns* stobj_get_fslot(nanojit::LIns* obj_ins, unsigned slot);
    nanojit::LIns* stobj_get_dslot(nanojit::LIns* obj_ins, unsigned index,
                                   nanojit::LIns*& dslots_ins);
    nanojit::LIns* stobj_get_slot(nanojit::LIns* obj_ins, unsigned slot,
                                  nanojit::LIns*& dslots_ins);

    nanojit::LIns* stobj_get_private(nanojit::LIns* obj_ins, jsval mask=JSVAL_INT) {
        return lir->ins2(nanojit::LIR_piand,
                         stobj_get_fslot(obj_ins, JSSLOT_PRIVATE),
                         lir->insImmPtr((void*) ~mask));
    }

    nanojit::LIns* stobj_get_proto(nanojit::LIns* obj_ins) {
        return stobj_get_fslot(obj_ins, JSSLOT_PROTO);
    }

    nanojit::LIns* stobj_get_parent(nanojit::LIns* obj_ins) {
        return stobj_get_fslot(obj_ins, JSSLOT_PARENT);
    }

    nanojit::LIns* getStringLength(nanojit::LIns* str_ins);

    JS_REQUIRES_STACK JSRecordingStatus name(jsval*& vp, nanojit::LIns*& ins, NameResult& nr);
    JS_REQUIRES_STACK JSRecordingStatus prop(JSObject* obj, nanojit::LIns* obj_ins, uint32 *slotp,
                                             nanojit::LIns** v_insp, jsval* outp);
    JS_REQUIRES_STACK JSRecordingStatus denseArrayElement(jsval& oval, jsval& idx, jsval*& vp,
                                                          nanojit::LIns*& v_ins,
                                                          nanojit::LIns*& addr_ins);
    JS_REQUIRES_STACK JSRecordingStatus getProp(JSObject* obj, nanojit::LIns* obj_ins);
    JS_REQUIRES_STACK JSRecordingStatus getProp(jsval& v);
    JS_REQUIRES_STACK JSRecordingStatus getThis(nanojit::LIns*& this_ins);

    JS_REQUIRES_STACK void enterDeepBailCall();
    JS_REQUIRES_STACK void leaveDeepBailCall();

    JS_REQUIRES_STACK JSRecordingStatus primitiveToStringInPlace(jsval* vp);
    JS_REQUIRES_STACK void finishGetProp(nanojit::LIns* obj_ins, nanojit::LIns* vp_ins,
                                         nanojit::LIns* ok_ins, jsval* outp);
    JS_REQUIRES_STACK JSRecordingStatus getPropertyByName(nanojit::LIns* obj_ins, jsval* idvalp,
                                                          jsval* outp);
    JS_REQUIRES_STACK JSRecordingStatus getPropertyByIndex(nanojit::LIns* obj_ins,
                                                           nanojit::LIns* index_ins, jsval* outp);
    JS_REQUIRES_STACK JSRecordingStatus getPropertyById(nanojit::LIns* obj_ins, jsval* outp);
    JS_REQUIRES_STACK JSRecordingStatus getPropertyWithNativeGetter(nanojit::LIns* obj_ins,
                                                                    JSScopeProperty* sprop,
                                                                    jsval* outp);

    JS_REQUIRES_STACK JSRecordingStatus nativeSet(JSObject* obj, nanojit::LIns* obj_ins,
                                                  JSScopeProperty* sprop,
                                                  jsval v, nanojit::LIns* v_ins);
    JS_REQUIRES_STACK JSRecordingStatus setProp(jsval &l, JSPropCacheEntry* entry,
                                                JSScopeProperty* sprop,
                                                jsval &v, nanojit::LIns*& v_ins);
    JS_REQUIRES_STACK JSRecordingStatus setCallProp(JSObject *callobj, nanojit::LIns *callobj_ins,
                                                    JSScopeProperty *sprop, nanojit::LIns *v_ins,
                                                    jsval v);
    JS_REQUIRES_STACK JSRecordingStatus initOrSetPropertyByName(nanojit::LIns* obj_ins,
                                                                jsval* idvalp, jsval* rvalp,
                                                                bool init);
    JS_REQUIRES_STACK JSRecordingStatus initOrSetPropertyByIndex(nanojit::LIns* obj_ins,
                                                                 nanojit::LIns* index_ins,
                                                                 jsval* rvalp, bool init);

    JS_REQUIRES_STACK nanojit::LIns* box_jsval(jsval v, nanojit::LIns* v_ins);
    JS_REQUIRES_STACK nanojit::LIns* unbox_jsval(jsval v, nanojit::LIns* v_ins, VMSideExit* exit);
    JS_REQUIRES_STACK bool guardClass(JSObject* obj, nanojit::LIns* obj_ins, JSClass* clasp,
                                      VMSideExit* exit);
    JS_REQUIRES_STACK bool guardDenseArray(JSObject* obj, nanojit::LIns* obj_ins,
                                           ExitType exitType = MISMATCH_EXIT);
    JS_REQUIRES_STACK bool guardHasPrototype(JSObject* obj, nanojit::LIns* obj_ins,
                                             JSObject** pobj, nanojit::LIns** pobj_ins,
                                             VMSideExit* exit);
    JS_REQUIRES_STACK JSRecordingStatus guardPrototypeHasNoIndexedProperties(JSObject* obj,
                                                                             nanojit::LIns* obj_ins,
                                                                             ExitType exitType);
    JS_REQUIRES_STACK JSRecordingStatus guardNotGlobalObject(JSObject* obj,
                                                             nanojit::LIns* obj_ins);
    void clearFrameSlotsFromCache();
    JS_REQUIRES_STACK void putArguments();
    JS_REQUIRES_STACK JSRecordingStatus guardCallee(jsval& callee);
    JS_REQUIRES_STACK JSStackFrame      *guardArguments(JSObject *obj, nanojit::LIns* obj_ins,
                                                        unsigned *depthp);
    JS_REQUIRES_STACK JSRecordingStatus getClassPrototype(JSObject* ctor,
                                                          nanojit::LIns*& proto_ins);
    JS_REQUIRES_STACK JSRecordingStatus getClassPrototype(JSProtoKey key,
                                                          nanojit::LIns*& proto_ins);
    JS_REQUIRES_STACK JSRecordingStatus newArray(JSObject* ctor, uint32 argc, jsval* argv,
                                                 jsval* rval);
    JS_REQUIRES_STACK JSRecordingStatus newString(JSObject* ctor, uint32 argc, jsval* argv,
                                                  jsval* rval);
    JS_REQUIRES_STACK JSRecordingStatus interpretedFunctionCall(jsval& fval, JSFunction* fun,
                                                                uintN argc, bool constructing);
    JS_REQUIRES_STACK void propagateFailureToBuiltinStatus(nanojit::LIns *ok_ins,
                                                           nanojit::LIns *&status_ins);
    JS_REQUIRES_STACK JSRecordingStatus emitNativeCall(JSSpecializedNative* sn, uintN argc,
                                                       nanojit::LIns* args[]);
    JS_REQUIRES_STACK void emitNativePropertyOp(JSScope* scope,
                                                JSScopeProperty* sprop,
                                                nanojit::LIns* obj_ins,
                                                bool setflag,
                                                nanojit::LIns* boxed_ins);
    JS_REQUIRES_STACK JSRecordingStatus callSpecializedNative(JSNativeTraceInfo* trcinfo, uintN argc,
                                                              bool constructing);
    JS_REQUIRES_STACK JSRecordingStatus callNative(uintN argc, JSOp mode);
    JS_REQUIRES_STACK JSRecordingStatus functionCall(uintN argc, JSOp mode);

    JS_REQUIRES_STACK void trackCfgMerges(jsbytecode* pc);
    JS_REQUIRES_STACK void emitIf(jsbytecode* pc, bool cond, nanojit::LIns* x);
    JS_REQUIRES_STACK void fuseIf(jsbytecode* pc, bool cond, nanojit::LIns* x);
    JS_REQUIRES_STACK JSRecordingStatus checkTraceEnd(jsbytecode* pc);

    bool hasMethod(JSObject* obj, jsid id);
    JS_REQUIRES_STACK bool hasIteratorMethod(JSObject* obj);

    JS_REQUIRES_STACK jsatomid getFullIndex(ptrdiff_t pcoff = 0);

public:
    JS_REQUIRES_STACK
    TraceRecorder(JSContext* cx, VMSideExit*, nanojit::Fragment*, TreeInfo*,
                  unsigned stackSlots, unsigned ngslots, JSTraceType* typeMap,
                  VMSideExit* expectedInnerExit, jsbytecode* outerTree,
                  uint32 outerArgc);
    ~TraceRecorder();

    static JS_REQUIRES_STACK JSRecordingStatus monitorRecording(JSContext* cx, TraceRecorder* tr,
                                                                JSOp op);

    JS_REQUIRES_STACK JSTraceType determineSlotType(jsval* vp);

    



    JS_REQUIRES_STACK VMSideExit* snapshot(ExitType exitType);

    




    JS_REQUIRES_STACK VMSideExit* copy(VMSideExit* exit);

    




    JS_REQUIRES_STACK nanojit::LIns* createGuardRecord(VMSideExit* exit);

    nanojit::Fragment* getFragment() const { return fragment; }
    TreeInfo* getTreeInfo() const { return treeInfo; }
    JS_REQUIRES_STACK void compile(JSTraceMonitor* tm);
    JS_REQUIRES_STACK bool closeLoop(TypeConsensus &consensus);
    JS_REQUIRES_STACK bool closeLoop(SlotMap& slotMap, VMSideExit* exit, TypeConsensus &consensus);
    JS_REQUIRES_STACK void endLoop();
    JS_REQUIRES_STACK void endLoop(VMSideExit* exit);
    JS_REQUIRES_STACK void joinEdgesToEntry(VMFragment* peer_root);
    JS_REQUIRES_STACK void adjustCallerTypes(nanojit::Fragment* f);
    JS_REQUIRES_STACK VMFragment* findNestedCompatiblePeer(VMFragment* f);
    JS_REQUIRES_STACK void prepareTreeCall(VMFragment* inner);
    JS_REQUIRES_STACK void emitTreeCall(VMFragment* inner, VMSideExit* exit);
    unsigned getCallDepth() const;
    void pushAbortStack();
    void popAbortStack();
    void removeFragmentReferences();
    void deepAbort();

    JS_REQUIRES_STACK JSRecordingStatus record_EnterFrame();
    JS_REQUIRES_STACK JSRecordingStatus record_LeaveFrame();
    JS_REQUIRES_STACK JSRecordingStatus record_SetPropHit(JSPropCacheEntry* entry,
                                                          JSScopeProperty* sprop);
    JS_REQUIRES_STACK JSRecordingStatus record_DefLocalFunSetSlot(uint32 slot, JSObject* obj);
    JS_REQUIRES_STACK JSRecordingStatus record_NativeCallComplete();

    bool wasDeepAborted() { return deepAborted; }
    TreeInfo* getTreeInfo() { return treeInfo; }

#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format)               \
    JS_REQUIRES_STACK JSRecordingStatus record_##op();
# include "jsopcode.tbl"
#undef OPDEF

    friend class ImportBoxedStackSlotVisitor;
    friend class ImportUnboxedStackSlotVisitor;
    friend class ImportGlobalSlotVisitor;
    friend class AdjustCallerGlobalTypesVisitor;
    friend class AdjustCallerStackTypesVisitor;
    friend class TypeCompatibilityVisitor;
    friend class SlotMap;
    friend class DefaultSlotMap;
    friend jsval *js_ConcatPostImacroStackCleanup(uint32 argc, JSFrameRegs &regs,
                                                  TraceRecorder *recorder);
};
#define TRACING_ENABLED(cx)       JS_HAS_OPTION(cx, JSOPTION_JIT)
#define TRACE_RECORDER(cx)        (JS_TRACE_MONITOR(cx).recorder)
#define SET_TRACE_RECORDER(cx,tr) (JS_TRACE_MONITOR(cx).recorder = (tr))

#define JSOP_IN_RANGE(op,lo,hi)   (uintN((op) - (lo)) <= uintN((hi) - (lo)))
#define JSOP_IS_BINARY(op)        JSOP_IN_RANGE(op, JSOP_BITOR, JSOP_MOD)
#define JSOP_IS_UNARY(op)         JSOP_IN_RANGE(op, JSOP_NEG, JSOP_POS)
#define JSOP_IS_EQUALITY(op)      JSOP_IN_RANGE(op, JSOP_EQ, JSOP_NE)

#define TRACE_ARGS_(x,args)                                                   \
    JS_BEGIN_MACRO                                                            \
        TraceRecorder* tr_ = TRACE_RECORDER(cx);                              \
        if (tr_ && !tr_->wasDeepAborted()) {                                  \
            JSRecordingStatus status = tr_->record_##x args;                  \
            if (STATUS_ABORTS_RECORDING(status)) {                            \
                js_AbortRecording(cx, #x);                                    \
                if (status == JSRS_ERROR)                                     \
                    goto error;                                               \
            }                                                                 \
            JS_ASSERT(status != JSRS_IMACRO);                                 \
        }                                                                     \
    JS_END_MACRO

#define TRACE_ARGS(x,args)      TRACE_ARGS_(x, args)
#define TRACE_0(x)              TRACE_ARGS(x, ())
#define TRACE_1(x,a)            TRACE_ARGS(x, (a))
#define TRACE_2(x,a,b)          TRACE_ARGS(x, (a, b))

extern JS_REQUIRES_STACK bool
js_MonitorLoopEdge(JSContext* cx, uintN& inlineCallCount);

#ifdef DEBUG
# define js_AbortRecording(cx, reason) js_AbortRecordingImpl(cx, reason)
#else
# define js_AbortRecording(cx, reason) js_AbortRecordingImpl(cx)
#endif

extern JS_REQUIRES_STACK void
js_AbortRecording(JSContext* cx, const char* reason);

extern void
js_InitJIT(JSTraceMonitor *tm);

extern void
js_FinishJIT(JSTraceMonitor *tm);

extern void
js_PurgeScriptFragments(JSContext* cx, JSScript* script);

extern bool
js_OverfullJITCache(JSTraceMonitor* tm, bool reCache);

extern void
js_PurgeJITOracle();

extern JSObject *
js_GetBuiltinFunction(JSContext *cx, uintN index);

extern void
js_SetMaxCodeCacheBytes(JSContext* cx, uint32 bytes);

#ifdef MOZ_TRACEVIS

extern JS_FRIEND_API(bool)
JS_StartTraceVis(const char* filename);

extern JS_FRIEND_API(JSBool)
js_StartTraceVis(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                 jsval *rval);

extern JS_FRIEND_API(bool)
JS_StopTraceVis();

extern JS_FRIEND_API(JSBool)
js_StopTraceVis(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval);


enum TraceVisState {
    S_EXITLAST,
    S_INTERP,
    S_MONITOR,
    S_RECORD,
    S_COMPILE,
    S_EXECUTE,
    S_NATIVE
};


enum TraceVisExitReason {
    R_NONE,
    R_ABORT,
    
    R_INNER_SIDE_EXIT,
    R_DOUBLES,
    R_CALLBACK_PENDING,
    R_OOM_GETANCHOR,
    R_BACKED_OFF,
    R_COLD,
    R_FAIL_RECORD_TREE,
    R_MAX_PEERS,
    R_FAIL_EXECUTE_TREE,
    R_FAIL_STABILIZE,
    R_FAIL_EXTEND_FLUSH,
    R_FAIL_EXTEND_MAX_BRANCHES,
    R_FAIL_EXTEND_START,
    R_FAIL_EXTEND_COLD,
    R_NO_EXTEND_OUTER,
    R_MISMATCH_EXIT,
    R_OOM_EXIT,
    R_TIMEOUT_EXIT,
    R_DEEP_BAIL_EXIT,
    R_STATUS_EXIT,
    R_OTHER_EXIT
};

const unsigned long long MS64_MASK = 0xfull << 60;
const unsigned long long MR64_MASK = 0x1full << 55;
const unsigned long long MT64_MASK = ~(MS64_MASK | MR64_MASK);

extern FILE* traceVisLogFile;
extern JSHashTable *traceVisScriptTable;

extern JS_FRIEND_API(void)
js_StoreTraceVisState(JSContext *cx, TraceVisState s, TraceVisExitReason r);

static inline void
js_LogTraceVisState(JSContext *cx, TraceVisState s, TraceVisExitReason r)
{
    if (traceVisLogFile) {
        unsigned long long sllu = s;
        unsigned long long rllu = r;
        unsigned long long d = (sllu << 60) | (rllu << 55) | (rdtsc() & MT64_MASK);
        fwrite(&d, sizeof(d), 1, traceVisLogFile);
    }
    if (traceVisScriptTable) {
        js_StoreTraceVisState(cx, s, r);
    }
}

static inline void
js_EnterTraceVisState(JSContext *cx, TraceVisState s, TraceVisExitReason r)
{
    js_LogTraceVisState(cx, s, r);
}

static inline void
js_ExitTraceVisState(JSContext *cx, TraceVisExitReason r)
{
    js_LogTraceVisState(cx, S_EXITLAST, r);
}

struct TraceVisStateObj {
    TraceVisExitReason r;
    JSContext *mCx;

    inline TraceVisStateObj(JSContext *cx, TraceVisState s) : r(R_NONE)
    {
        js_EnterTraceVisState(cx, s, R_NONE);
        mCx = cx;
    }
    inline ~TraceVisStateObj()
    {
        js_ExitTraceVisState(mCx, r);
    }
};

#endif 

extern jsval *
js_ConcatPostImacroStackCleanup(uint32 argc, JSFrameRegs &regs,
                                TraceRecorder *recorder);

#else  

#define TRACE_0(x)              ((void)0)
#define TRACE_1(x,a)            ((void)0)
#define TRACE_2(x,a,b)          ((void)0)

#endif 

#endif 
