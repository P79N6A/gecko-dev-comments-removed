








































#ifndef jstracer_h___
#define jstracer_h___

#ifdef JS_TRACER

#include "jstypes.h"
#include "jsbuiltins.h"
#include "jscntxt.h"
#include "jsdhash.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsvector.h"
#include "jscompartment.h"
#include "Writer.h"

namespace js {

template <typename T>
class Queue {
    T* _data;
    unsigned _len;
    unsigned _max;
    nanojit::Allocator* alloc;

public:
    void ensure(unsigned size) {
        if (_max > size)
            return;
        if (!_max)
            _max = 8;
        _max = JS_MAX(_max * 2, size);
        if (alloc) {
            T* tmp = new (*alloc) T[_max];
            memcpy(tmp, _data, _len * sizeof(T));
            _data = tmp;
        } else {
            _data = (T*) js::OffTheBooks::realloc_(_data, _max * sizeof(T));
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
            js::UnwantedForeground::free_(_data);
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

    int offsetOf(T slot) {
        T* p = _data;
        unsigned n = 0;
        for (n = 0; n < _len; ++n)
            if (*p++ == slot)
                return n;
        return -1;
    }

};




















class Tracker {
    #define TRACKER_PAGE_SZB        4096
    #define TRACKER_PAGE_ENTRIES    (TRACKER_PAGE_SZB >> 2)    // each slot is 4 bytes
    #define TRACKER_PAGE_MASK       jsuword(TRACKER_PAGE_SZB - 1)

    struct TrackerPage {
        struct TrackerPage* next;
        jsuword             base;
        nanojit::LIns*      map[TRACKER_PAGE_ENTRIES];
    };
    struct TrackerPage* pagelist;

    
    JSContext* cx;

    jsuword             getTrackerPageBase(const void* v) const;
    jsuword             getTrackerPageOffset(const void* v) const;
    struct TrackerPage* findTrackerPage(const void* v) const;
    struct TrackerPage* addTrackerPage(const void* v);
public:
    Tracker(JSContext* cx);
    ~Tracker();

    bool            has(const void* v) const;
    nanojit::LIns*  get(const void* v) const;
    void            set(const void* v, nanojit::LIns* ins);
    void            clear();
};

class VMFragment : public nanojit::Fragment {
public:
    VMFragment(const void* _ip verbose_only(, uint32_t profFragID))
      : Fragment(_ip verbose_only(, profFragID))
    {}

    



    TreeFragment* root;

    TreeFragment* toTreeFragment();
};

#if defined(JS_JIT_SPEW)


extern nanojit::Config NJConfig;


extern nanojit::LogControl LogController;



extern void FragProfiling_FragFinalizer(nanojit::Fragment* f, TraceMonitor*);

#define debug_only_stmt(stmt) \
    stmt

#define debug_only_printf(mask, fmt, ...)                                      \
    JS_BEGIN_MACRO                                                             \
        if ((LogController.lcbits & (mask)) > 0) {                             \
            LogController.printf(fmt, __VA_ARGS__);                            \
            fflush(stdout);                                                    \
        }                                                                      \
    JS_END_MACRO

#define debug_only_print0(mask, str)                                           \
    JS_BEGIN_MACRO                                                             \
        if ((LogController.lcbits & (mask)) > 0) {                             \
            LogController.printf("%s", str);                                   \
            fflush(stdout);                                                    \
        }                                                                      \
    JS_END_MACRO

#else

#define debug_only_stmt(action)
#define debug_only_printf(mask, fmt, ...)  JS_BEGIN_MACRO JS_END_MACRO
#define debug_only_print0(mask, str)       JS_BEGIN_MACRO JS_END_MACRO

#endif








#define ORACLE_SIZE 4096

class Oracle {
    nanojit::BitSet _stackDontDemote;
    nanojit::BitSet _globalDontDemote;
    nanojit::BitSet _pcDontDemote;
    nanojit::BitSet _pcSlowZeroTest;
public:
    Oracle(VMAllocator* allocator);

    JS_REQUIRES_STACK void markGlobalSlotUndemotable(JSContext* cx, unsigned slot);
    JS_REQUIRES_STACK bool isGlobalSlotUndemotable(JSContext* cx, unsigned slot) const;
    JS_REQUIRES_STACK void markStackSlotUndemotable(JSContext* cx, unsigned slot);
    JS_REQUIRES_STACK void markStackSlotUndemotable(JSContext* cx, unsigned slot, const void* pc);
    JS_REQUIRES_STACK bool isStackSlotUndemotable(JSContext* cx, unsigned slot) const;
    JS_REQUIRES_STACK bool isStackSlotUndemotable(JSContext* cx, unsigned slot, const void* pc) const;
    void markInstructionUndemotable(jsbytecode* pc);
    bool isInstructionUndemotable(jsbytecode* pc) const;
    void markInstructionSlowZeroTest(jsbytecode* pc);
    bool isInstructionSlowZeroTest(jsbytecode* pc) const;

    void clearDemotability();
    void clear() {
        clearDemotability();
    }
};

typedef Queue<uint16> SlotList;

class TypeMap : public Queue<JSValueType> {
    Oracle *oracle;
public:
    TypeMap(nanojit::Allocator* alloc, Oracle *oracle)
      : Queue<JSValueType>(alloc),
        oracle(oracle)
    {}
    void set(unsigned stackSlots, unsigned ngslots,
             const JSValueType* stackTypeMap, const JSValueType* globalTypeMap);
    JS_REQUIRES_STACK void captureTypes(JSContext* cx, JSObject* globalObj, SlotList& slots, unsigned callDepth,
                                        bool speculate);
    JS_REQUIRES_STACK void captureMissingGlobalTypes(JSContext* cx, JSObject* globalObj, SlotList& slots,
                                                     unsigned stackSlots, bool speculate);
    bool matches(TypeMap& other) const;
    void fromRaw(JSValueType* other, unsigned numSlots);
};

#define JS_TM_EXITCODES(_)    \
    /*                                                                          \
     * An exit at a possible branch-point in the trace at which to attach a     \
     * future secondary trace. Therefore the recorder must generate different   \
     * code to handle the other outcome of the branch condition from the        \
     * primary trace's outcome.                                                 \
     */                                                                         \
    _(BRANCH)                                                                   \
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
    _(MUL_ZERO)                                                                 \
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

struct FrameInfo;

struct VMSideExit : public nanojit::SideExit
{
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
    unsigned hitcount;

    inline JSValueType* stackTypeMap() {
        return (JSValueType*)(this + 1);
    }

    inline JSValueType& stackType(unsigned i) {
        JS_ASSERT(i < numStackSlots);
        return stackTypeMap()[i];
    }

    inline JSValueType* globalTypeMap() {
        return (JSValueType*)(this + 1) + this->numStackSlots;
    }

    inline JSValueType* fullTypeMap() {
        return stackTypeMap();
    }

    inline VMFragment* fromFrag() {
        return (VMFragment*)from;
    }

    inline TreeFragment* root() {
        return fromFrag()->root;
    }
};

class VMAllocator : public nanojit::Allocator
{
public:
    VMAllocator(JSRuntime *rt, char* reserve, size_t reserveSize)
      : mOutOfMemory(false)
      , mSize(0)
      , mReserve(reserve)
      , mReserveSize(reserveSize)
      , mReserveCurr(uintptr_t(reserve))
      , mReserveLimit(uintptr_t(reserve + reserveSize))
      , mRt(rt)
    {}

    ~VMAllocator() {
        js::UnwantedForeground::free_(mReserve);
    }

    size_t size() {
        return mSize;
    }

    bool outOfMemory() {
        return mOutOfMemory;
    }

    struct Mark
    {
        VMAllocator& vma;
        bool committed;
        nanojit::Allocator::Chunk* saved_chunk;
        char* saved_top;
        char* saved_limit;
        size_t saved_size;

        Mark(VMAllocator& vma) :
            vma(vma),
            committed(false),
            saved_chunk(vma.current_chunk),
            saved_top(vma.current_top),
            saved_limit(vma.current_limit),
            saved_size(vma.mSize)
        {}

        ~Mark()
        {
            if (!committed)
                vma.rewind(*this);
        }

        void commit() { committed = true; }
    };

    void rewind(const Mark& m) {
        while (current_chunk != m.saved_chunk) {
            Chunk *prev = current_chunk->prev;
            freeChunk(current_chunk);
            current_chunk = prev;
        }
        current_top = m.saved_top;
        current_limit = m.saved_limit;
        mSize = m.saved_size;
        memset(current_top, 0, current_limit - current_top);
    }

    bool mOutOfMemory;
    size_t mSize;

    
    char* mReserve;
    size_t mReserveSize;
    uintptr_t mReserveCurr;
    uintptr_t mReserveLimit;

    
    JSRuntime* mRt;
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
    uint16 get_argc() const { return uint16(argc & ~CONSTRUCTING_FLAG); }
    bool   is_constructing() const { return (argc & CONSTRUCTING_FLAG) != 0; }

    
    JSValueType* get_typemap() { return (JSValueType*) (this+1); }
    const JSValueType* get_typemap() const { return (JSValueType*) (this+1); }
};

struct UnstableExit
{
    VMFragment* fragment;
    VMSideExit* exit;
    UnstableExit* next;
};

struct LinkableFragment : public VMFragment
{
    LinkableFragment(const void* _ip, nanojit::Allocator* alloc, Oracle *oracle
                     verbose_only(, uint32_t profFragID))
        : VMFragment(_ip verbose_only(, profFragID)), typeMap(alloc, oracle), nStackTypes(0)
    { }

    uint32                  branchCount;
    TypeMap                 typeMap;
    unsigned                nStackTypes;
    unsigned                spOffsetAtEntry;
    SlotList*               globalSlots;
};










struct TreeFragment : public LinkableFragment
{
    TreeFragment(const void* _ip, nanojit::Allocator* alloc, Oracle *oracle, JSObject* _globalObj,
                 uint32 _globalShape, uint32 _argc verbose_only(, uint32_t profFragID)):
        LinkableFragment(_ip, alloc, oracle verbose_only(, profFragID)),
        first(NULL),
        next(NULL),
        peer(NULL),
        globalObj(_globalObj),
        globalShape(_globalShape),
        argc(_argc),
        dependentTrees(alloc),
        linkedTrees(alloc),
        sideExits(alloc),
        gcthings(alloc),
        shapes(alloc)
    { }

    TreeFragment* first;
    TreeFragment* next;
    TreeFragment* peer;
    JSObject* globalObj;
    uint32 globalShape;
    uint32 argc;
    
    Queue<TreeFragment*>    dependentTrees;
    
    Queue<TreeFragment*>    linkedTrees;
#ifdef DEBUG
    const char*             treeFileName;
    uintN                   treeLineNumber;
    uintN                   treePCOffset;
#endif
    JSScript*               script;
    UnstableExit*           unstableExits;
    Queue<VMSideExit*>      sideExits;
    ptrdiff_t               nativeStackBase;
    unsigned                maxCallDepth;
    
    Queue<Value>            gcthings;
    Queue<const js::Shape*> shapes;
    unsigned                maxNativeStackSlots;
    
    uintN                   execs;
    
    uintN                   iters;

    inline unsigned nGlobalTypes() {
        return typeMap.length() - nStackTypes;
    }
    inline JSValueType* globalTypeMap() {
        return typeMap.data() + nStackTypes;
    }
    inline JSValueType* stackTypeMap() {
        return typeMap.data();
    }

    JS_REQUIRES_STACK void initialize(JSContext* cx, SlotList *globalSlots, bool speculate);
    UnstableExit* removeUnstableExit(VMSideExit* exit);
};

inline TreeFragment*
VMFragment::toTreeFragment()
{
    JS_ASSERT(root == this);
    return static_cast<TreeFragment*>(this);
}

enum MonitorResult {
    MONITOR_RECORDING,
    MONITOR_NOT_RECORDING,
    MONITOR_ERROR
};

const uintN PROFILE_MAX_INNER_LOOPS = 8;
const uintN PROFILE_MAX_STACK = 6;





class LoopProfile
{
public:
    
    enum OpKind {
        OP_FLOAT, 
        OP_INT, 
        OP_BIT, 
        OP_EQ, 
        OP_EVAL, 
        OP_CALL, 
        OP_FWDJUMP, 
        OP_NEW, 
        OP_RECURSIVE, 
        OP_ARRAY_READ, 
        OP_TYPED_ARRAY, 
        OP_SCRIPTED_GETTER, 
        OP_LIMIT
    };

    
    TraceMonitor *traceMonitor;
    
    
    JSScript *entryScript;

    
    StackFrame *entryfp;

    
    jsbytecode *top, *bottom;

    
    uintN hits;

    
    bool profiled;

    
    bool undecided;

    
    bool traceOK;

    
    bool unprofitable;

    





    bool execOK;

    
    uintN allOps[OP_LIMIT];
    uintN numAllOps;

    
    uintN selfOps[OP_LIMIT];
    uintN numSelfOps;

    





    double numSelfOpsMult;

    





    double branchMultiplier;

    
    bool shortLoop;

    
    bool maybeShortLoop;

    



    struct InnerLoop {
        StackFrame *entryfp;
        jsbytecode *top, *bottom;
        uintN iters;

        InnerLoop() {}
        InnerLoop(StackFrame *entryfp, jsbytecode *top, jsbytecode *bottom)
            : entryfp(entryfp), top(top), bottom(bottom), iters(0) {}
    };

    
    InnerLoop innerLoops[PROFILE_MAX_INNER_LOOPS];
    uintN numInnerLoops;

    



    InnerLoop loopStack[PROFILE_MAX_INNER_LOOPS];
    uintN loopStackDepth;

    




    struct StackValue {
        bool isConst;
        bool hasValue;
        int value;

        StackValue() : isConst(false), hasValue(false) {}
        StackValue(bool isConst) : isConst(isConst), hasValue(false) {}
        StackValue(bool isConst, int value) : isConst(isConst), hasValue(true), value(value) {}
    };
    StackValue stack[PROFILE_MAX_STACK];
    uintN sp;

    inline void stackClear() { sp = 0; }
    
    inline void stackPush(const StackValue &v) {
        if (sp < PROFILE_MAX_STACK)
            stack[sp++] = v;
        else
            stackClear();
    }

    inline void stackPop() { if (sp > 0) sp--; }

    inline StackValue stackAt(int pos) {
        pos += sp;
        if (pos >= 0 && uintN(pos) < PROFILE_MAX_STACK)
            return stack[pos];
        else
            return StackValue(false);
    }
    
    LoopProfile(TraceMonitor *tm, StackFrame *entryfp, jsbytecode *top, jsbytecode *bottom);

    void reset();

    enum ProfileAction {
        ProfContinue,
        ProfComplete
    };

    
    inline void increment(OpKind kind)
    {
        allOps[kind]++;
        if (loopStackDepth == 0)
            selfOps[kind]++;
    }

    inline uintN count(OpKind kind) { return allOps[kind]; }

    
    MonitorResult profileLoopEdge(JSContext* cx, uintN& inlineCallCount);
    
    
    ProfileAction profileOperation(JSContext *cx, JSOp op);

    
    bool isCompilationExpensive(JSContext *cx, uintN depth);
    bool isCompilationUnprofitable(JSContext *cx, uintN goodOps);
    void decide(JSContext *cx);

    void stopProfiling(JSContext *cx);
};






typedef enum BuiltinStatus {
    BUILTIN_BAILED = 1,
    BUILTIN_ERROR = 2
} BuiltinStatus;

static JS_INLINE void
SetBuiltinError(TraceMonitor *tm)
{
    tm->tracerState->builtinStatus |= BUILTIN_ERROR;
}

static JS_INLINE bool
WasBuiltinSuccessful(TraceMonitor *tm)
{
    return tm->tracerState->builtinStatus == 0;
}

#ifdef DEBUG_RECORDING_STATUS_NOT_BOOL

struct RecordingStatus {
    int code;
    bool operator==(RecordingStatus &s) { return this->code == s.code; };
    bool operator!=(RecordingStatus &s) { return this->code != s.code; };
};
enum RecordingStatusCodes {
    RECORD_ERROR_code     = 0,
    RECORD_STOP_code      = 1,

    RECORD_CONTINUE_code  = 3,
    RECORD_IMACRO_code    = 4
};
RecordingStatus RECORD_CONTINUE = { RECORD_CONTINUE_code };
RecordingStatus RECORD_STOP     = { RECORD_STOP_code };
RecordingStatus RECORD_IMACRO   = { RECORD_IMACRO_code };
RecordingStatus RECORD_ERROR    = { RECORD_ERROR_code };

struct AbortableRecordingStatus {
    int code;
    bool operator==(AbortableRecordingStatus &s) { return this->code == s.code; };
    bool operator!=(AbortableRecordingStatus &s) { return this->code != s.code; };
};
enum AbortableRecordingStatusCodes {
    ARECORD_ERROR_code     = 0,
    ARECORD_STOP_code      = 1,
    ARECORD_ABORTED_code   = 2,
    ARECORD_CONTINUE_code  = 3,
    ARECORD_IMACRO_code    = 4,
    ARECORD_IMACRO_ABORTED_code = 5,
    ARECORD_COMPLETED_code = 6
};
AbortableRecordingStatus ARECORD_ERROR    = { ARECORD_ERROR_code };
AbortableRecordingStatus ARECORD_STOP     = { ARECORD_STOP_code };
AbortableRecordingStatus ARECORD_CONTINUE = { ARECORD_CONTINUE_code };
AbortableRecordingStatus ARECORD_IMACRO   = { ARECORD_IMACRO_code };
AbortableRecordingStatus ARECORD_IMACRO_ABORTED   = { ARECORD_IMACRO_ABORTED_code };
AbortableRecordingStatus ARECORD_ABORTED =  { ARECORD_ABORTED_code };
AbortableRecordingStatus ARECORD_COMPLETED =  { ARECORD_COMPLETED_code };

static inline AbortableRecordingStatus
InjectStatus(RecordingStatus rs)
{
    AbortableRecordingStatus ars = { rs.code };
    return ars;
}
static inline AbortableRecordingStatus
InjectStatus(AbortableRecordingStatus ars)
{
    return ars;
}

static inline bool
StatusAbortsRecorderIfActive(AbortableRecordingStatus ars)
{
    return ars == ARECORD_ERROR || ars == ARECORD_STOP;
}
#else



















enum RecordingStatus {
    RECORD_STOP       = 0,  
                            
    RECORD_ERROR      = 1,  
                            
                            
    RECORD_CONTINUE   = 2,  
    RECORD_IMACRO     = 3   
                            
};

enum AbortableRecordingStatus {
    ARECORD_STOP      = 0,  
    ARECORD_ERROR     = 1,  
                            
                            
                            
    ARECORD_CONTINUE  = 2,  
    ARECORD_IMACRO    = 3,  
    ARECORD_IMACRO_ABORTED = 4, 
    ARECORD_ABORTED   = 5,  
                            
    ARECORD_COMPLETED = 6   
                            
};

static JS_ALWAYS_INLINE AbortableRecordingStatus
InjectStatus(RecordingStatus rs)
{
    return static_cast<AbortableRecordingStatus>(rs);
}

static JS_ALWAYS_INLINE AbortableRecordingStatus
InjectStatus(AbortableRecordingStatus ars)
{
    return ars;
}







static JS_ALWAYS_INLINE bool
StatusAbortsRecorderIfActive(AbortableRecordingStatus ars)
{
    return ars <= ARECORD_ERROR;
}
#endif

class SlotMap;
class SlurpInfo;


enum TypeConsensus
{
    TypeConsensus_Okay,         
    TypeConsensus_Undemotes,    
    TypeConsensus_Bad           
};

enum TracePointAction {
    TPA_Nothing,
    TPA_RanStuff,
    TPA_Recorded,
    TPA_Error
};

typedef HashMap<nanojit::LIns*, JSObject*> GuardedShapeTable;

#ifdef DEBUG
# define AbortRecording(cx, reason) AbortRecordingImpl(cx, reason)
#else
# define AbortRecording(cx, reason) AbortRecordingImpl(cx)
#endif

void
AbortProfiling(JSContext *cx);

class TraceRecorder
{
    JS_DECLARE_ALLOCATION_FRIENDS_FOR_PRIVATE_CONSTRUCTOR;

    

    
    JSContext* const                cx;

    
    TraceMonitor* const             traceMonitor;

    
    Oracle*                         oracle;

    
    VMFragment* const               fragment;

    
    TreeFragment* const             tree;

    
    JSObject* const                 globalObj;

    
    JSScript* const                 outerScript;
    
    
    jsbytecode* const               outerPC;

    
    uint32 const                    outerArgc;

    
    VMSideExit* const               anchor;

    
    nanojit::LIns* const            cx_ins;
    nanojit::LIns* const            eos_ins;
    nanojit::LIns* const            eor_ins;
    nanojit::LIns* const            loopLabel;

    
    unsigned                        importStackSlots;
    unsigned                        importGlobalSlots;
    TypeMap                         importTypeMap;

    



    nanojit::LirBuffer* const       lirbuf;

    



    VMAllocator::Mark               mark;

    
    const unsigned                  numSideExitsBefore;

    

    
    Tracker                         tracker;

    
    Tracker                         nativeFrameTracker;

    
    Value*                          global_slots;

    
    unsigned                        callDepth;

    
    JSAtom**                        atoms;
    Value*                          consts;

    
    nanojit::LIns*                  strictModeCode_ins;

    
    Queue<jsbytecode*>              cfgMerges;

    
    bool                            trashSelf;

    
    Queue<TreeFragment*>            whichTreesToTrash;

    
    GuardedShapeTable               guardedShapeTable;

    
    int                             initDepth;
    bool                            hadNewInit;

#ifdef DEBUG
    



    const js::Shape* addPropShapeBefore;
#endif

    

    
    nanojit::LIns*                  rval_ins;

    
    nanojit::LIns*                  native_rval_ins;

    
    nanojit::LIns*                  newobj_ins;

    
    JSSpecializedNative*            pendingSpecializedNative;

    
    Value*                          pendingUnboxSlot;

    
    nanojit::LIns*                  pendingGuardCondition;

    
    js::Vector<unsigned>            pendingGlobalSlotsToSet;

    
    bool                            pendingLoop;

    
    JSSpecializedNative             generatedSpecializedNative;

    
    js::Vector<JSValueType, 256>    tempTypeMap;

    
    tjit::Writer w;

    

    




    nanojit::LIns* w_immpObjGC(JSObject* obj);
    nanojit::LIns* w_immpFunGC(JSFunction* fun);
    nanojit::LIns* w_immpStrGC(JSString* str);
    nanojit::LIns* w_immpShapeGC(const js::Shape* shape);
    nanojit::LIns* w_immpIdGC(jsid id);

    #define immpObjGC(obj)        name(w_immpObjGC(obj), #obj)
    #define immpFunGC(fun)        name(w_immpFunGC(fun), #fun)
    #define immpStrGC(str)        name(w_immpStrGC(str), #str)
    #define immpAtomGC(atom)      name(w_immpStrGC(atom), "ATOM(" #atom ")")
    #define immpShapeGC(shape)    name(w_immpShapeGC(shape), #shape)
    #define immpIdGC(id)          name(w_immpIdGC(id), #id)

    



    JS_REQUIRES_STACK VMSideExit* snapshot(ExitType exitType);

    



    JS_REQUIRES_STACK VMSideExit* copy(VMSideExit* exit);

    




    JS_REQUIRES_STACK nanojit::GuardRecord* createGuardRecord(VMSideExit* exit);

    JS_REQUIRES_STACK JS_INLINE void markSlotUndemotable(LinkableFragment* f, unsigned slot);

    JS_REQUIRES_STACK JS_INLINE void markSlotUndemotable(LinkableFragment* f, unsigned slot, const void* pc);

    JS_REQUIRES_STACK unsigned findUndemotesInTypemaps(const TypeMap& typeMap, LinkableFragment* f,
                            Queue<unsigned>& undemotes);

    JS_REQUIRES_STACK void assertDownFrameIsConsistent(VMSideExit* anchor, FrameInfo* fi);

    JS_REQUIRES_STACK void captureStackTypes(unsigned callDepth, JSValueType* typeMap);

    bool isVoidPtrGlobal(const void* p) const;
    bool isGlobal(const Value* p) const;
    ptrdiff_t nativeGlobalSlot(const Value *p) const;
    ptrdiff_t nativeGlobalOffset(const Value* p) const;
    JS_REQUIRES_STACK ptrdiff_t nativeStackOffsetImpl(const void* p) const;
    JS_REQUIRES_STACK ptrdiff_t nativeStackOffset(const Value* p) const;
    JS_REQUIRES_STACK ptrdiff_t nativeStackSlotImpl(const void* p) const;
    JS_REQUIRES_STACK ptrdiff_t nativeStackSlot(const Value* p) const;
    JS_REQUIRES_STACK ptrdiff_t nativespOffsetImpl(const void* p) const;
    JS_REQUIRES_STACK ptrdiff_t nativespOffset(const Value* p) const;
    JS_REQUIRES_STACK void importImpl(tjit::Address addr, const void* p, JSValueType t,
                                      const char *prefix, uintN index, StackFrame *fp);
    JS_REQUIRES_STACK void import(tjit::Address addr, const Value* p, JSValueType t,
                                  const char *prefix, uintN index, StackFrame *fp);
    JS_REQUIRES_STACK void import(TreeFragment* tree, nanojit::LIns* sp, unsigned stackSlots,
                                  unsigned callDepth, unsigned ngslots, JSValueType* typeMap);
    void trackNativeStackUse(unsigned slots);

    JS_REQUIRES_STACK bool isValidSlot(JSObject *obj, const js::Shape* shape);
    JS_REQUIRES_STACK bool lazilyImportGlobalSlot(unsigned slot);
    JS_REQUIRES_STACK void importGlobalSlot(unsigned slot);

    void ensureCond(nanojit::LIns** ins, bool* cond);

    JS_REQUIRES_STACK RecordingStatus guard(bool expected, nanojit::LIns* cond, ExitType exitType,
                                            bool abortIfAlwaysExits = false);
    JS_REQUIRES_STACK RecordingStatus guard(bool expected, nanojit::LIns* cond, VMSideExit* exit,
                                            bool abortIfAlwaysExits = false);

    nanojit::LIns* writeBack(nanojit::LIns* i, nanojit::LIns* base, ptrdiff_t offset,
                             bool shouldDemoteToInt32);

#ifdef DEBUG
    bool isValidFrameObjPtr(void *obj);
#endif
    void assertInsideLoop();

    JS_REQUIRES_STACK void setImpl(void* p, nanojit::LIns* l, bool shouldDemoteToInt32 = true);
    JS_REQUIRES_STACK void set(Value* p, nanojit::LIns* l, bool shouldDemoteToInt32 = true);
    JS_REQUIRES_STACK void setFrameObjPtr(void* p, nanojit::LIns* l,
                                          bool shouldDemoteToInt32 = true);
    nanojit::LIns* getFromTrackerImpl(const void *p);
    nanojit::LIns* getFromTracker(const Value* p);
    JS_REQUIRES_STACK nanojit::LIns* getImpl(const void* p);
    JS_REQUIRES_STACK nanojit::LIns* get(const Value* p);
    JS_REQUIRES_STACK nanojit::LIns* getFrameObjPtr(void* p);
    JS_REQUIRES_STACK nanojit::LIns* attemptImport(const Value* p);
    JS_REQUIRES_STACK nanojit::LIns* addr(Value* p);

    JS_REQUIRES_STACK bool knownImpl(const void* p);
    JS_REQUIRES_STACK bool known(const Value* p);
    JS_REQUIRES_STACK bool known(JSObject** p);
    




    JS_REQUIRES_STACK void checkForGlobalObjectReallocation() {
        if (global_slots != globalObj->getSlots())
            checkForGlobalObjectReallocationHelper();
    }
    JS_REQUIRES_STACK void checkForGlobalObjectReallocationHelper();

    JS_REQUIRES_STACK TypeConsensus selfTypeStability(SlotMap& smap);
    JS_REQUIRES_STACK TypeConsensus peerTypeStability(SlotMap& smap, const void* ip,
                                                      TreeFragment** peer);

    JS_REQUIRES_STACK Value& argval(unsigned n) const;
    JS_REQUIRES_STACK Value& varval(unsigned n) const;
    JS_REQUIRES_STACK Value& stackval(int n) const;

    JS_REQUIRES_STACK void updateAtoms();
    JS_REQUIRES_STACK void updateAtoms(JSScript *script);

    struct NameResult {
        
        
        
        bool             tracked;
        Value            v;              
        JSObject         *obj;           
        nanojit::LIns    *obj_ins;       
        js::Shape        *shape;         
    };

    JS_REQUIRES_STACK nanojit::LIns* scopeChain();
    JS_REQUIRES_STACK nanojit::LIns* entryScopeChain() const;
    JS_REQUIRES_STACK nanojit::LIns* entryFrameIns() const;
    JS_REQUIRES_STACK StackFrame* frameIfInRange(JSObject* obj, unsigned* depthp = NULL) const;
    JS_REQUIRES_STACK RecordingStatus traverseScopeChain(JSObject *obj, nanojit::LIns *obj_ins, JSObject *obj2, nanojit::LIns *&obj2_ins);
    JS_REQUIRES_STACK AbortableRecordingStatus scopeChainProp(JSObject* obj, Value*& vp, nanojit::LIns*& ins, NameResult& nr, JSObject **scopeObjp = NULL);
    JS_REQUIRES_STACK RecordingStatus callProp(JSObject* obj, JSProperty* shape, jsid id, Value*& vp, nanojit::LIns*& ins, NameResult& nr);

    JS_REQUIRES_STACK nanojit::LIns* arg(unsigned n);
    JS_REQUIRES_STACK void arg(unsigned n, nanojit::LIns* i);
    JS_REQUIRES_STACK nanojit::LIns* var(unsigned n);
    JS_REQUIRES_STACK void var(unsigned n, nanojit::LIns* i);
    JS_REQUIRES_STACK nanojit::LIns* upvar(JSScript* script, JSUpvarArray* uva, uintN index, Value& v);
    nanojit::LIns* stackLoad(tjit::Address addr, uint8 type);
    JS_REQUIRES_STACK nanojit::LIns* stack(int n);
    JS_REQUIRES_STACK void stack(int n, nanojit::LIns* i);

    JS_REQUIRES_STACK void guardNonNeg(nanojit::LIns* d0, nanojit::LIns* d1, VMSideExit* exit);
    JS_REQUIRES_STACK nanojit::LIns* tryToDemote(nanojit::LOpcode op, jsdouble v0, jsdouble v1,
                                                 nanojit::LIns* s0, nanojit::LIns* s1);

    nanojit::LIns* d2i(nanojit::LIns* f, bool resultCanBeImpreciseIfFractional = false);
    nanojit::LIns* d2u(nanojit::LIns* d);
    JS_REQUIRES_STACK RecordingStatus makeNumberInt32(nanojit::LIns* d, nanojit::LIns** num_ins);
    JS_REQUIRES_STACK RecordingStatus makeNumberUint32(nanojit::LIns* d, nanojit::LIns** num_ins);
    JS_REQUIRES_STACK nanojit::LIns* stringify(const Value& v);

    JS_REQUIRES_STACK nanojit::LIns* newArguments(nanojit::LIns* callee_ins);

    JS_REQUIRES_STACK bool canCallImacro() const;
    JS_REQUIRES_STACK RecordingStatus callImacro(jsbytecode* imacro);
    JS_REQUIRES_STACK RecordingStatus callImacroInfallibly(jsbytecode* imacro);

    JS_REQUIRES_STACK AbortableRecordingStatus ifop();
    JS_REQUIRES_STACK RecordingStatus switchop();
    JS_REQUIRES_STACK RecordingStatus inc(Value& v, jsint incr, bool pre = true);
    JS_REQUIRES_STACK RecordingStatus inc(const Value &v, nanojit::LIns*& v_ins,
                                          Value &v_out, jsint incr,
                                          bool pre = true);
    JS_REQUIRES_STACK RecordingStatus incHelper(const Value &v, nanojit::LIns*& v_ins,
                                                Value &v_after,
                                                nanojit::LIns*& v_ins_after,
                                                jsint incr);
    JS_REQUIRES_STACK AbortableRecordingStatus incProp(jsint incr, bool pre = true);
    JS_REQUIRES_STACK RecordingStatus incElem(jsint incr, bool pre = true);
    JS_REQUIRES_STACK AbortableRecordingStatus incName(jsint incr, bool pre = true);

    JS_REQUIRES_STACK RecordingStatus strictEquality(bool equal, bool cmpCase);
    JS_REQUIRES_STACK AbortableRecordingStatus equality(bool negate, bool tryBranchAfterCond);
    JS_REQUIRES_STACK AbortableRecordingStatus equalityHelper(Value& l, Value& r,
                                                                nanojit::LIns* l_ins, nanojit::LIns* r_ins,
                                                                bool negate, bool tryBranchAfterCond,
                                                                Value& rval);
    JS_REQUIRES_STACK AbortableRecordingStatus relational(nanojit::LOpcode op, bool tryBranchAfterCond);

    JS_REQUIRES_STACK RecordingStatus unaryIntOp(nanojit::LOpcode op);
    JS_REQUIRES_STACK RecordingStatus binary(nanojit::LOpcode op);

    JS_REQUIRES_STACK RecordingStatus guardShape(nanojit::LIns* obj_ins, JSObject* obj,
                                                 uint32 shape, const char* name, VMSideExit* exit);

#if defined DEBUG_notme && defined XP_UNIX
    void dumpGuardedShapes(const char* prefix);
#endif

    void forgetGuardedShapes();

    JS_REQUIRES_STACK AbortableRecordingStatus test_property_cache(JSObject* obj, nanojit::LIns* obj_ins,
                                                                     JSObject*& obj2, PCVal& pcval);
    JS_REQUIRES_STACK RecordingStatus guardPropertyCacheHit(nanojit::LIns* obj_ins,
                                                            JSObject* aobj,
                                                            JSObject* obj2,
                                                            PropertyCacheEntry* entry,
                                                            PCVal& pcval);

    void stobj_set_fslot(nanojit::LIns *obj_ins, unsigned slot, const Value &v,
                         nanojit::LIns* v_ins);
    void stobj_set_dslot(nanojit::LIns *obj_ins, unsigned slot,
                         nanojit::LIns*& slots_ins, const Value &v, nanojit::LIns* v_ins);
    void stobj_set_slot(JSObject *obj, nanojit::LIns* obj_ins, unsigned slot,
                        nanojit::LIns*& slots_ins, const Value &v, nanojit::LIns* v_ins);

    nanojit::LIns* unbox_slot(JSObject *obj, nanojit::LIns *obj_ins, uint32 slot,
                              VMSideExit *exit);

    JS_REQUIRES_STACK AbortableRecordingStatus name(Value*& vp, nanojit::LIns*& ins, NameResult& nr);
    JS_REQUIRES_STACK AbortableRecordingStatus prop(JSObject* obj, nanojit::LIns* obj_ins,
                                                    uint32 *slotp, nanojit::LIns** v_insp,
                                                    Value* outp);
    JS_REQUIRES_STACK RecordingStatus propTail(JSObject* obj, nanojit::LIns* obj_ins,
                                               JSObject* obj2, PCVal pcval,
                                               uint32 *slotp, nanojit::LIns** v_insp,
                                               Value* outp);
    JS_REQUIRES_STACK RecordingStatus denseArrayElement(Value& oval, Value& idx, Value*& vp,
                                                        nanojit::LIns*& v_ins,
                                                        nanojit::LIns*& addr_ins,
                                                        VMSideExit* exit);
    JS_REQUIRES_STACK nanojit::LIns *canonicalizeNaNs(nanojit::LIns *dval_ins);
    JS_REQUIRES_STACK AbortableRecordingStatus typedArrayElement(Value& oval, Value& idx, Value*& vp,
                                                                 nanojit::LIns*& v_ins);
    JS_REQUIRES_STACK AbortableRecordingStatus getProp(JSObject* obj, nanojit::LIns* obj_ins);
    JS_REQUIRES_STACK AbortableRecordingStatus getProp(Value& v);
    JS_REQUIRES_STACK RecordingStatus getThis(nanojit::LIns*& this_ins);

    JS_REQUIRES_STACK void storeMagic(JSWhyMagic why, tjit::Address addr);
    JS_REQUIRES_STACK AbortableRecordingStatus unboxNextValue(nanojit::LIns* &v_ins);

    JS_REQUIRES_STACK VMSideExit* enterDeepBailCall();
    JS_REQUIRES_STACK void leaveDeepBailCall();

    JS_REQUIRES_STACK RecordingStatus primitiveToStringInPlace(Value* vp);
    JS_REQUIRES_STACK void finishGetProp(nanojit::LIns* obj_ins, nanojit::LIns* vp_ins,
                                         nanojit::LIns* ok_ins, Value* outp);
    JS_REQUIRES_STACK RecordingStatus getPropertyByName(nanojit::LIns* obj_ins, Value* idvalp,
                                                        Value* outp);
    JS_REQUIRES_STACK RecordingStatus getPropertyByIndex(nanojit::LIns* obj_ins,
                                                         nanojit::LIns* index_ins, Value* outp);
    JS_REQUIRES_STACK RecordingStatus getPropertyById(nanojit::LIns* obj_ins, Value* outp);
    JS_REQUIRES_STACK RecordingStatus getPropertyWithNativeGetter(nanojit::LIns* obj_ins,
                                                                  const js::Shape* shape,
                                                                  Value* outp);
    JS_REQUIRES_STACK RecordingStatus getPropertyWithScriptGetter(JSObject *obj,
                                                                  nanojit::LIns* obj_ins,
                                                                  const js::Shape* shape);

    JS_REQUIRES_STACK RecordingStatus getCharCodeAt(JSString *str,
                                                    nanojit::LIns* str_ins, nanojit::LIns* idx_ins,
                                                    nanojit::LIns** out_ins);
    JS_REQUIRES_STACK nanojit::LIns* getUnitString(nanojit::LIns* str_ins, nanojit::LIns* idx_ins);
    JS_REQUIRES_STACK RecordingStatus getCharAt(JSString *str,
                                                nanojit::LIns* str_ins, nanojit::LIns* idx_ins,
                                                JSOp mode, nanojit::LIns** out_ins);

    JS_REQUIRES_STACK RecordingStatus initOrSetPropertyByName(nanojit::LIns* obj_ins,
                                                              Value* idvalp, Value* rvalp,
                                                              bool init);
    JS_REQUIRES_STACK RecordingStatus initOrSetPropertyByIndex(nanojit::LIns* obj_ins,
                                                               nanojit::LIns* index_ins,
                                                               Value* rvalp, bool init);
    JS_REQUIRES_STACK AbortableRecordingStatus setElem(int lval_spindex, int idx_spindex,
                                                       int v_spindex);

    JS_REQUIRES_STACK RecordingStatus lookupForSetPropertyOp(JSObject* obj, nanojit::LIns* obj_ins,
                                                             jsid id, bool* safep,
                                                             JSObject** pobjp,
                                                             const js::Shape** shapep);
    JS_REQUIRES_STACK RecordingStatus nativeSet(JSObject* obj, nanojit::LIns* obj_ins,
                                                const js::Shape* shape,
                                                const Value& v, nanojit::LIns* v_ins);
    JS_REQUIRES_STACK RecordingStatus addDataProperty(JSObject* obj);
    JS_REQUIRES_STACK RecordingStatus setCallProp(JSObject* callobj, nanojit::LIns* callobj_ins,
                                                  const js::Shape* shape, nanojit::LIns* v_ins,
                                                  const Value& v);
    JS_REQUIRES_STACK RecordingStatus setProperty(JSObject* obj, nanojit::LIns* obj_ins,
                                                  const Value& v, nanojit::LIns* v_ins,
                                                  bool* deferredp);
    JS_REQUIRES_STACK RecordingStatus recordSetPropertyOp();
    JS_REQUIRES_STACK RecordingStatus recordInitPropertyOp(jsbytecode op);

    void box_undefined_into(tjit::Address addr);
#if JS_BITS_PER_WORD == 32
    void box_null_into(tjit::Address addr);
    nanojit::LIns* unbox_number_as_double(tjit::Address addr, nanojit::LIns* tag_ins,
                                          VMSideExit* exit);
    nanojit::LIns* unbox_object(tjit::Address addr, nanojit::LIns* tag_ins, JSValueType type,
                                VMSideExit* exit);
    nanojit::LIns* unbox_non_double_object(tjit::Address addr, nanojit::LIns* tag_ins,
                                           JSValueType type, VMSideExit* exit);
#elif JS_BITS_PER_WORD == 64
    nanojit::LIns* non_double_object_value_has_type(nanojit::LIns* v_ins, JSValueType type);
    nanojit::LIns* unpack_ptr(nanojit::LIns* v_ins);
    nanojit::LIns* unbox_number_as_double(nanojit::LIns* v_ins, VMSideExit* exit);
    nanojit::LIns* unbox_object(nanojit::LIns* v_ins, JSValueType type, VMSideExit* exit);
    nanojit::LIns* unbox_non_double_object(nanojit::LIns* v_ins, JSValueType type, VMSideExit* exit);
#endif

    nanojit::LIns* unbox_value(const Value& v, tjit::Address addr, VMSideExit* exit,
                               bool force_double=false);
    void unbox_any_object(tjit::Address addr, nanojit::LIns** obj_ins, nanojit::LIns** is_obj_ins);
    nanojit::LIns* is_boxed_true(tjit::Address addr);
    nanojit::LIns* is_boxed_magic(tjit::Address addr, JSWhyMagic why);

    nanojit::LIns* is_string_id(nanojit::LIns* id_ins);
    nanojit::LIns* unbox_string_id(nanojit::LIns* id_ins);
    nanojit::LIns* unbox_int_id(nanojit::LIns* id_ins);

    
    void box_value_into(const Value& v, nanojit::LIns* v_ins, tjit::Address addr);

    





    nanojit::LIns* box_value_for_native_call(const Value& v, nanojit::LIns* v_ins);

    
    nanojit::LIns* box_value_into_alloc(const Value& v, nanojit::LIns* v_ins);

    JS_REQUIRES_STACK void guardClassHelper(bool cond, nanojit::LIns* obj_ins, Class* clasp,
                                            VMSideExit* exit, nanojit::LoadQual loadQual);
    JS_REQUIRES_STACK void guardClass(nanojit::LIns* obj_ins, Class* clasp,
                                      VMSideExit* exit, nanojit::LoadQual loadQual);
    JS_REQUIRES_STACK void guardNotClass(nanojit::LIns* obj_ins, Class* clasp,
                                         VMSideExit* exit, nanojit::LoadQual loadQual);
    JS_REQUIRES_STACK void guardDenseArray(nanojit::LIns* obj_ins, ExitType exitType);
    JS_REQUIRES_STACK void guardDenseArray(nanojit::LIns* obj_ins, VMSideExit* exit);
    JS_REQUIRES_STACK bool guardHasPrototype(JSObject* obj, nanojit::LIns* obj_ins,
                                             JSObject** pobj, nanojit::LIns** pobj_ins,
                                             VMSideExit* exit);
    JS_REQUIRES_STACK RecordingStatus guardPrototypeHasNoIndexedProperties(JSObject* obj,
                                                                           nanojit::LIns* obj_ins,
                                                                           VMSideExit* exit);
    JS_REQUIRES_STACK RecordingStatus guardNativeConversion(Value& v);
    JS_REQUIRES_STACK void clearReturningFrameFromNativeTracker();
    JS_REQUIRES_STACK AbortableRecordingStatus putActivationObjects();
    JS_REQUIRES_STACK RecordingStatus createThis(JSObject& ctor, nanojit::LIns* ctor_ins,
                                                 nanojit::LIns** thisobj_insp);
    JS_REQUIRES_STACK RecordingStatus guardCallee(Value& callee);
    JS_REQUIRES_STACK StackFrame *guardArguments(JSObject *obj, nanojit::LIns* obj_ins,
                                                 unsigned *depthp);
    JS_REQUIRES_STACK nanojit::LIns* guardArgsLengthNotAssigned(nanojit::LIns* argsobj_ins);
    JS_REQUIRES_STACK void guardNotHole(nanojit::LIns* argsobj_ins, nanojit::LIns* ids_ins);
    JS_REQUIRES_STACK RecordingStatus getClassPrototype(JSObject* ctor,
                                                          nanojit::LIns*& proto_ins);
    JS_REQUIRES_STACK RecordingStatus getClassPrototype(JSProtoKey key,
                                                          nanojit::LIns*& proto_ins);
    JS_REQUIRES_STACK RecordingStatus newArray(JSObject* ctor, uint32 argc, Value* argv,
                                                 Value* rval);
    JS_REQUIRES_STACK RecordingStatus newString(JSObject* ctor, uint32 argc, Value* argv,
                                                  Value* rval);
    JS_REQUIRES_STACK RecordingStatus interpretedFunctionCall(Value& fval, JSFunction* fun,
                                                                uintN argc, bool constructing);
    JS_REQUIRES_STACK void propagateFailureToBuiltinStatus(nanojit::LIns *ok_ins,
                                                           nanojit::LIns *&status_ins);
    JS_REQUIRES_STACK RecordingStatus emitNativeCall(JSSpecializedNative* sn, uintN argc,
                                                       nanojit::LIns* args[], bool rooted);
    JS_REQUIRES_STACK void emitNativePropertyOp(const js::Shape* shape,
                                                nanojit::LIns* obj_ins,
                                                bool setflag,
                                                nanojit::LIns* addr_boxed_val_ins);
    JS_REQUIRES_STACK RecordingStatus callSpecializedNative(JSNativeTraceInfo* trcinfo, uintN argc,
                                                              bool constructing);
    JS_REQUIRES_STACK RecordingStatus callNative(uintN argc, JSOp mode);
    JS_REQUIRES_STACK RecordingStatus callFloatReturningInt(uintN argc,
                                                            const nanojit::CallInfo *ci);
    JS_REQUIRES_STACK RecordingStatus functionCall(uintN argc, JSOp mode);

    JS_REQUIRES_STACK void trackCfgMerges(jsbytecode* pc);
    JS_REQUIRES_STACK void emitIf(jsbytecode* pc, bool cond, nanojit::LIns* x);
    JS_REQUIRES_STACK void fuseIf(jsbytecode* pc, bool cond, nanojit::LIns* x);
    JS_REQUIRES_STACK AbortableRecordingStatus checkTraceEnd(jsbytecode* pc);

    AbortableRecordingStatus hasMethod(JSObject* obj, jsid id, bool& found);
    JS_REQUIRES_STACK AbortableRecordingStatus hasIteratorMethod(JSObject* obj, bool& found);

    JS_REQUIRES_STACK jsatomid getFullIndex(ptrdiff_t pcoff = 0);

    JS_REQUIRES_STACK JSValueType determineSlotType(Value* vp);

    JS_REQUIRES_STACK RecordingStatus setUpwardTrackedVar(Value* stackVp, const Value& v,
                                                          nanojit::LIns* v_ins);

    JS_REQUIRES_STACK AbortableRecordingStatus compile();
    JS_REQUIRES_STACK AbortableRecordingStatus closeLoop();
    JS_REQUIRES_STACK AbortableRecordingStatus endLoop();
    JS_REQUIRES_STACK AbortableRecordingStatus endLoop(VMSideExit* exit);
    JS_REQUIRES_STACK bool joinEdgesToEntry(TreeFragment* peer_root);
    JS_REQUIRES_STACK void adjustCallerTypes(TreeFragment* f);
    JS_REQUIRES_STACK void prepareTreeCall(TreeFragment* inner);
    JS_REQUIRES_STACK void emitTreeCall(TreeFragment* inner, VMSideExit* exit);
    JS_REQUIRES_STACK void determineGlobalTypes(JSValueType* typeMap);
    JS_REQUIRES_STACK VMSideExit* downSnapshot(FrameInfo* downFrame);
    JS_REQUIRES_STACK TreeFragment* findNestedCompatiblePeer(TreeFragment* f);
    JS_REQUIRES_STACK AbortableRecordingStatus attemptTreeCall(TreeFragment* inner,
                                                               uintN& inlineCallCount);

    static JS_REQUIRES_STACK MonitorResult recordLoopEdge(JSContext* cx, TraceRecorder* r,
                                                          uintN& inlineCallCount);

    
    VMAllocator& tempAlloc() const { return *traceMonitor->tempAlloc; }
    VMAllocator& traceAlloc() const { return *traceMonitor->traceAlloc; }
    VMAllocator& dataAlloc() const { return *traceMonitor->dataAlloc; }

    
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format)               \
    JS_REQUIRES_STACK AbortableRecordingStatus record_##op();
# include "jsopcode.tbl"
#undef OPDEF

    JS_REQUIRES_STACK
    TraceRecorder(JSContext* cx, TraceMonitor *tm, VMSideExit*, VMFragment*,
                  unsigned stackSlots, unsigned ngslots, JSValueType* typeMap,
                  VMSideExit* expectedInnerExit, JSScript* outerScript, jsbytecode* outerPC,
                  uint32 outerArgc, bool speculate);

    
    ~TraceRecorder();
    JS_REQUIRES_STACK AbortableRecordingStatus finishSuccessfully();

    enum AbortResult { NORMAL_ABORT, JIT_RESET };
    JS_REQUIRES_STACK AbortResult finishAbort(const char* reason);

#ifdef DEBUG
    
    JS_REQUIRES_STACK void tprint(const char *format, int count, nanojit::LIns *insa[]);
    JS_REQUIRES_STACK void tprint(const char *format);
    JS_REQUIRES_STACK void tprint(const char *format, nanojit::LIns *ins);
    JS_REQUIRES_STACK void tprint(const char *format, nanojit::LIns *ins1,
                                  nanojit::LIns *ins2);
    JS_REQUIRES_STACK void tprint(const char *format, nanojit::LIns *ins1,
                                  nanojit::LIns *ins2, nanojit::LIns *ins3);
    JS_REQUIRES_STACK void tprint(const char *format, nanojit::LIns *ins1,
                                  nanojit::LIns *ins2, nanojit::LIns *ins3,
                                  nanojit::LIns *ins4);
    JS_REQUIRES_STACK void tprint(const char *format, nanojit::LIns *ins1,
                                  nanojit::LIns *ins2, nanojit::LIns *ins3,
                                  nanojit::LIns *ins4, nanojit::LIns *ins5);
    JS_REQUIRES_STACK void tprint(const char *format, nanojit::LIns *ins1,
                                  nanojit::LIns *ins2, nanojit::LIns *ins3,
                                  nanojit::LIns *ins4, nanojit::LIns *ins5,
                                  nanojit::LIns *ins6);
#endif

    friend class ImportBoxedStackSlotVisitor;
    friend class AdjustCallerGlobalTypesVisitor;
    friend class AdjustCallerStackTypesVisitor;
    friend class TypeCompatibilityVisitor;
    friend class SlotMap;
    friend class DefaultSlotMap;
    friend class DetermineTypesVisitor;
    friend MonitorResult RecordLoopEdge(JSContext*, TraceMonitor*, uintN&);
    friend TracePointAction RecordTracePoint(JSContext*, TraceMonitor*, uintN &inlineCallCount,
                                             bool *blacklist);
    friend AbortResult AbortRecording(JSContext*, const char*);
    friend class BoxArg;
    friend void TraceMonitor::sweep(JSContext *cx);

  public:
    static bool JS_REQUIRES_STACK
    startRecorder(JSContext*, TraceMonitor *, VMSideExit*, VMFragment*,
                  unsigned stackSlots, unsigned ngslots, JSValueType* typeMap,
                  VMSideExit* expectedInnerExit, JSScript* outerScript, jsbytecode* outerPC,
                  uint32 outerArgc, bool speculate);

    
    VMFragment*         getFragment() const { return fragment; }
    TreeFragment*       getTree() const { return tree; }
    bool                outOfMemory() const { return traceMonitor->outOfMemory(); }
    Oracle*             getOracle() const { return oracle; }
    JSObject*           getGlobal() const { return globalObj; }

    
    JS_REQUIRES_STACK AbortableRecordingStatus monitorRecording(JSOp op);
    JS_REQUIRES_STACK AbortableRecordingStatus record_EnterFrame();
    JS_REQUIRES_STACK AbortableRecordingStatus record_LeaveFrame();
    JS_REQUIRES_STACK AbortableRecordingStatus record_AddProperty(JSObject *obj);
    JS_REQUIRES_STACK AbortableRecordingStatus record_DefLocalFunSetSlot(uint32 slot,
                                                                         JSObject* obj);
    JS_REQUIRES_STACK AbortableRecordingStatus record_NativeCallComplete();
    void forgetGuardedShapesForObject(JSObject* obj);

    bool globalSetExpected(unsigned slot) {
        unsigned *pi = Find(pendingGlobalSlotsToSet, slot);
        if (pi == pendingGlobalSlotsToSet.end()) {
            



            Value *vp = globalObj->getSlots() + slot;

            
            if (tracker.has(vp))
                return false;
            
            






            return tree->globalSlots->offsetOf((uint16)nativeGlobalSlot(vp)) == -1;
        }
        pendingGlobalSlotsToSet.erase(pi);
        return true;
    }
};

#define TRACING_ENABLED(cx)       ((cx)->traceJitEnabled)
#define REGEX_JIT_ENABLED(cx)     ((cx)->traceJitEnabled || (cx)->methodJitEnabled)

#define JSOP_IN_RANGE(op,lo,hi)   (uintN((op) - (lo)) <= uintN((hi) - (lo)))
#define JSOP_IS_BINARY(op)        JSOP_IN_RANGE(op, JSOP_BITOR, JSOP_MOD)
#define JSOP_IS_UNARY(op)         JSOP_IN_RANGE(op, JSOP_NEG, JSOP_POS)
#define JSOP_IS_EQUALITY(op)      JSOP_IN_RANGE(op, JSOP_EQ, JSOP_NE)

#define TRACE_ARGS_(x,args)                                                   \
    JS_BEGIN_MACRO                                                            \
        if (TraceRecorder* tr_ = TRACE_RECORDER(cx)) {                        \
            AbortableRecordingStatus status = tr_->record_##x args;           \
            if (StatusAbortsRecorderIfActive(status)) {                       \
                if (TRACE_RECORDER(cx)) {                                     \
                    JS_ASSERT(TRACE_RECORDER(cx) == tr_);                     \
                    AbortRecording(cx, #x);                                   \
                }                                                             \
                if (status == ARECORD_ERROR)                                  \
                    goto error;                                               \
            }                                                                 \
            JS_ASSERT(status != ARECORD_IMACRO);                              \
        }                                                                     \
    JS_END_MACRO

#define TRACE_ARGS(x,args)      TRACE_ARGS_(x, args)
#define TRACE_0(x)              TRACE_ARGS(x, ())
#define TRACE_1(x,a)            TRACE_ARGS(x, (a))
#define TRACE_2(x,a,b)          TRACE_ARGS(x, (a, b))

extern JS_REQUIRES_STACK MonitorResult
MonitorLoopEdge(JSContext* cx, uintN& inlineCallCount, InterpMode interpMode);

extern JS_REQUIRES_STACK TracePointAction
RecordTracePoint(JSContext*, uintN& inlineCallCount, bool* blacklist);

extern JS_REQUIRES_STACK TracePointAction
MonitorTracePoint(JSContext*, uintN& inlineCallCount, bool* blacklist,
                  void** traceData, uintN *traceEpoch, uint32 *loopCounter, uint32 hits);

extern JS_REQUIRES_STACK TraceRecorder::AbortResult
AbortRecording(JSContext* cx, const char* reason);

extern void
InitJIT();

extern void
FinishJIT();

extern void
PurgeScriptFragments(TraceMonitor* tm, JSScript* script);

extern bool
OverfullJITCache(JSContext *cx, TraceMonitor* tm);

extern void
FlushJITCache(JSContext* cx, TraceMonitor* tm);

extern JSObject *
GetBuiltinFunction(JSContext *cx, uintN index);

extern void
SetMaxCodeCacheBytes(JSContext* cx, uint32 bytes);

extern void
ExternNativeToValue(JSContext* cx, Value& v, JSValueType type, double* slot);

#ifdef MOZ_TRACEVIS

extern JS_FRIEND_API(bool)
StartTraceVis(const char* filename);

extern JS_FRIEND_API(JSBool)
StartTraceVisNative(JSContext *cx, uintN argc, jsval *vp);

extern JS_FRIEND_API(bool)
StopTraceVis();

extern JS_FRIEND_API(JSBool)
StopTraceVisNative(JSContext *cx, uintN argc, jsval *vp);


enum TraceVisState {
    
    S_EXITLAST,
    
    S_INTERP,
    S_MONITOR,
    S_RECORD,
    S_COMPILE,
    S_EXECUTE,
    S_NATIVE,
    
    S_RESET = 8
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
    R_FAIL_SCOPE_CHAIN_CHECK,
    R_NO_EXTEND_OUTER,
    R_MISMATCH_EXIT,
    R_OOM_EXIT,
    R_TIMEOUT_EXIT,
    R_DEEP_BAIL_EXIT,
    R_STATUS_EXIT,
    R_OTHER_EXIT
};

enum TraceVisFlushReason {
    FR_DEEP_BAIL,
    FR_OOM,
    FR_GLOBAL_SHAPE_MISMATCH,
    FR_GLOBALS_FULL
};

const unsigned long long MS64_MASK = 0xfull << 60;
const unsigned long long MR64_MASK = 0x1full << 55;
const unsigned long long MT64_MASK = ~(MS64_MASK | MR64_MASK);

extern FILE* traceVisLogFile;
extern JSHashTable *traceVisScriptTable;

extern JS_FRIEND_API(void)
StoreTraceVisState(JSContext *cx, TraceVisState s, TraceVisExitReason r);

static inline void
LogTraceVisState(JSContext *cx, TraceVisState s, TraceVisExitReason r)
{
    if (traceVisLogFile) {
        unsigned long long sllu = s;
        unsigned long long rllu = r;
        unsigned long long d = (sllu << 60) | (rllu << 55) | (rdtsc() & MT64_MASK);
        fwrite(&d, sizeof(d), 1, traceVisLogFile);
    }
    if (traceVisScriptTable) {
        StoreTraceVisState(cx, s, r);
    }
}






static inline void
LogTraceVisEvent(JSContext *cx, TraceVisState s, TraceVisFlushReason r)
{
    LogTraceVisState(cx, s, (TraceVisExitReason) r);
}

static inline void
EnterTraceVisState(JSContext *cx, TraceVisState s, TraceVisExitReason r)
{
    LogTraceVisState(cx, s, r);
}

static inline void
ExitTraceVisState(JSContext *cx, TraceVisExitReason r)
{
    LogTraceVisState(cx, S_EXITLAST, r);
}

struct TraceVisStateObj {
    TraceVisExitReason r;
    JSContext *mCx;

    inline TraceVisStateObj(JSContext *cx, TraceVisState s) : r(R_NONE)
    {
        EnterTraceVisState(cx, s, R_NONE);
        mCx = cx;
    }
    inline ~TraceVisStateObj()
    {
        ExitTraceVisState(mCx, r);
    }
};

#endif 

}      

#else  

#define TRACE_0(x)              ((void)0)
#define TRACE_1(x,a)            ((void)0)
#define TRACE_2(x,a,b)          ((void)0)

#endif 

namespace js {











static JS_INLINE void
AbortRecordingIfUnexpectedGlobalWrite(JSContext *cx, JSObject *obj, unsigned slot)
{
#ifdef JS_TRACER
    if (TraceRecorder *tr = TRACE_RECORDER(cx)) {
        if (obj == tr->getGlobal() && !tr->globalSetExpected(slot))
            AbortRecording(cx, "Global slot written outside tracer supervision");
    }
#endif
}

}  

#endif 
