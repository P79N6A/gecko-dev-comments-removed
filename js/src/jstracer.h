








































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
class Queue : public avmplus::GCObject {
    T* _data;
    unsigned _len;
    unsigned _max;

    void ensure(unsigned size) {
        while (_max < size)
            _max <<= 1;
        _data = (T*)realloc(_data, _max * sizeof(T));
#if defined(DEBUG)
        memset(&_data[_len], 0xcd, _max - _len);
#endif
    }
public:
    Queue(unsigned max = 16) {
        this->_max = max;
        this->_len = 0;
        this->_data = (T*)malloc(max * sizeof(T));
    }

    ~Queue() {
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

    const T & get(unsigned i) const {
        return _data[i];
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

#ifdef JS_JIT_SPEW
extern bool js_verboseDebug;
#define debug_only_v(x) if (js_verboseDebug) { x; fflush(stdout); }
#else
#define debug_only_v(x)
#endif








#define ORACLE_SIZE 4096

class Oracle {
    avmplus::BitSet _stackDontDemote;
    avmplus::BitSet _globalDontDemote;
public:
    Oracle();

    JS_REQUIRES_STACK void markGlobalSlotUndemotable(JSContext* cx, unsigned slot);
    JS_REQUIRES_STACK bool isGlobalSlotUndemotable(JSContext* cx, unsigned slot) const;
    JS_REQUIRES_STACK void markStackSlotUndemotable(JSContext* cx, unsigned slot);
    JS_REQUIRES_STACK bool isStackSlotUndemotable(JSContext* cx, unsigned slot) const;
    void clearDemotability();
    void clear() {
        clearDemotability();
    }
};

typedef Queue<uint16> SlotList;

class TypeMap : public Queue<uint8> {
public:
    JS_REQUIRES_STACK void captureTypes(JSContext* cx, SlotList& slots, unsigned callDepth);
    JS_REQUIRES_STACK void captureMissingGlobalTypes(JSContext* cx,
                                                     SlotList& slots,
                                                     unsigned stackSlots);
    bool matches(TypeMap& other) const;
};

enum ExitType {
    





    BRANCH_EXIT,

    


    CASE_EXIT,

    


    DEFAULT_EXIT,

    LOOP_EXIT,
    NESTED_EXIT,

    







    MISMATCH_EXIT,

    


    OOM_EXIT,
    OVERFLOW_EXIT,
    UNSTABLE_LOOP_EXIT,
    TIMEOUT_EXIT,
    DEEP_BAIL_EXIT,
    STATUS_EXIT
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
};

static inline uint8* getStackTypeMap(nanojit::SideExit* exit)
{
    return (uint8*)(((VMSideExit*)exit) + 1);
}

static inline uint8* getGlobalTypeMap(nanojit::SideExit* exit)
{
    return getStackTypeMap(exit) + ((VMSideExit*)exit)->numStackSlots;
}

static inline uint8* getFullTypeMap(nanojit::SideExit* exit)
{
    return getStackTypeMap(exit);
}

struct FrameInfo {
    JSObject*       callee;     
    JSObject*       block;      
    jsbytecode*     pc;         
    jsbytecode*     imacpc;     
    union {
        struct {
            uint16  spdist;     
            uint16  argc;       
        } s;
        uint32      word;       
    };
};

struct UnstableExit
{
    nanojit::Fragment* fragment;
    VMSideExit* exit;
    UnstableExit* next;
};

class TreeInfo MMGC_SUBCLASS_DECL {
    nanojit::Fragment*      fragment;
public:
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
#ifdef DEBUG
    const char*             treeFileName;
    uintN                   treeLineNumber;
    uintN                   treePCOffset;
#endif

    TreeInfo(nanojit::Fragment* _fragment,
             SlotList* _globalSlots)
      : fragment(_fragment),
        script(NULL),
        maxNativeStackSlots(0),
        nativeStackBase(0),
        maxCallDepth(0),
        nStackTypes(0),
        globalSlots(_globalSlots),
        branchCount(0),
        unstableExits(NULL)
            {}
    ~TreeInfo();

    inline unsigned nGlobalTypes() {
        return typeMap.length() - nStackTypes;
    }
    inline uint8* globalTypeMap() {
        return typeMap.data() + nStackTypes;
    }
    inline uint8* stackTypeMap() {
        return typeMap.data();
    }
};

#if defined(JS_JIT_SPEW) && (defined(NANOJIT_IA32) || (defined(NANOJIT_AMD64) && defined(__GNUC__)))
# define EXECUTE_TREE_TIMER
#endif

struct InterpState
{
    double        *sp;                  
    void          *rp;                  
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
    VMSideExit** innermostNestedGuardp;
    void*          stackMark;
    VMSideExit*    innermost;
#ifdef EXECUTE_TREE_TIMER
    uint64         startTime;
#endif
#ifdef DEBUG
    bool           jsframe_pop_blocks_set_on_entry;
#endif
};

enum JSMonitorRecordingStatus {
    JSMRS_CONTINUE,
    JSMRS_STOP,
    JSMRS_IMACRO
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
    nanojit::LIns*          invokevp_ins;
    bool                    deepAborted;
    bool                    trashSelf;
    Queue<nanojit::Fragment*> whichTreesToTrash;
    Queue<jsbytecode*>      cfgMerges;
    jsval*                  global_dslots;
    JSTraceableNative*      generatedTraceableNative;
    JSTraceableNative*      pendingTraceableNative;
    TraceRecorder*          nextRecorderToAbort;
    bool                    wasRootFragment;
    jsbytecode*             outer;
    bool                    loop;

    bool isGlobal(jsval* p) const;
    ptrdiff_t nativeGlobalOffset(jsval* p) const;
    JS_REQUIRES_STACK ptrdiff_t nativeStackOffset(jsval* p) const;
    JS_REQUIRES_STACK void import(nanojit::LIns* base, ptrdiff_t offset, jsval* p, uint8& t,
                                  const char *prefix, uintN index, JSStackFrame *fp);
    JS_REQUIRES_STACK void import(TreeInfo* treeInfo, nanojit::LIns* sp, unsigned stackSlots,
                                  unsigned callDepth, unsigned ngslots, uint8* typeMap);
    void trackNativeStackUse(unsigned slots);

    JS_REQUIRES_STACK bool isValidSlot(JSScope* scope, JSScopeProperty* sprop);
    JS_REQUIRES_STACK bool lazilyImportGlobalSlot(unsigned slot);

    JS_REQUIRES_STACK nanojit::LIns* guard(bool expected, nanojit::LIns* cond,
                                           ExitType exitType);
    nanojit::LIns* guard(bool expected, nanojit::LIns* cond, nanojit::LIns* exit);

    nanojit::LIns* addName(nanojit::LIns* ins, const char* name);

    nanojit::LIns* writeBack(nanojit::LIns* i, nanojit::LIns* base, ptrdiff_t offset);
    JS_REQUIRES_STACK void set(jsval* p, nanojit::LIns* l, bool initializing = false);
    JS_REQUIRES_STACK nanojit::LIns* get(jsval* p);
    JS_REQUIRES_STACK bool known(jsval* p);
    JS_REQUIRES_STACK void checkForGlobalObjectReallocation();

    JS_REQUIRES_STACK bool checkType(jsval& v, uint8 t, jsval*& stage_val,
                                     nanojit::LIns*& stage_ins, unsigned& stage_count);
    JS_REQUIRES_STACK bool deduceTypeStability(nanojit::Fragment* root_peer,
                                               nanojit::Fragment** stable_peer,
                                               bool& demote);

    JS_REQUIRES_STACK jsval& argval(unsigned n) const;
    JS_REQUIRES_STACK jsval& varval(unsigned n) const;
    JS_REQUIRES_STACK jsval& stackval(int n) const;

    JS_REQUIRES_STACK nanojit::LIns* scopeChain() const;
    JS_REQUIRES_STACK bool activeCallOrGlobalSlot(JSObject* obj, jsval*& vp);

    JS_REQUIRES_STACK nanojit::LIns* arg(unsigned n);
    JS_REQUIRES_STACK void arg(unsigned n, nanojit::LIns* i);
    JS_REQUIRES_STACK nanojit::LIns* var(unsigned n);
    JS_REQUIRES_STACK void var(unsigned n, nanojit::LIns* i);
    JS_REQUIRES_STACK nanojit::LIns* stack(int n);
    JS_REQUIRES_STACK void stack(int n, nanojit::LIns* i);

    JS_REQUIRES_STACK nanojit::LIns* alu(nanojit::LOpcode op, jsdouble v0, jsdouble v1,
                                         nanojit::LIns* s0, nanojit::LIns* s1);
    nanojit::LIns* f2i(nanojit::LIns* f);
    JS_REQUIRES_STACK nanojit::LIns* makeNumberInt32(nanojit::LIns* f);
    JS_REQUIRES_STACK nanojit::LIns* stringify(jsval& v);

    JS_REQUIRES_STACK bool call_imacro(jsbytecode* imacro);

    JS_REQUIRES_STACK bool ifop();
    JS_REQUIRES_STACK bool switchop();
#ifdef NANOJIT_IA32
    JS_REQUIRES_STACK nanojit::LIns* tableswitch();
#endif
    JS_REQUIRES_STACK bool inc(jsval& v, jsint incr, bool pre = true);
    JS_REQUIRES_STACK bool inc(jsval& v, nanojit::LIns*& v_ins, jsint incr, bool pre = true);
    JS_REQUIRES_STACK bool incProp(jsint incr, bool pre = true);
    JS_REQUIRES_STACK bool incElem(jsint incr, bool pre = true);
    JS_REQUIRES_STACK bool incName(jsint incr, bool pre = true);

    JS_REQUIRES_STACK void strictEquality(bool equal, bool cmpCase);
    JS_REQUIRES_STACK bool equality(bool negate, bool tryBranchAfterCond);
    JS_REQUIRES_STACK bool equalityHelper(jsval l, jsval r,
                                          nanojit::LIns* l_ins, nanojit::LIns* r_ins,
                                          bool negate, bool tryBranchAfterCond,
                                          jsval& rval);
    JS_REQUIRES_STACK bool relational(nanojit::LOpcode op, bool tryBranchAfterCond);

    JS_REQUIRES_STACK bool unary(nanojit::LOpcode op);
    JS_REQUIRES_STACK bool binary(nanojit::LOpcode op);

    bool ibinary(nanojit::LOpcode op);
    bool iunary(nanojit::LOpcode op);
    bool bbinary(nanojit::LOpcode op);
    void demote(jsval& v, jsdouble result);

    JS_REQUIRES_STACK bool map_is_native(JSObjectMap* map, nanojit::LIns* map_ins,
                                         nanojit::LIns*& ops_ins, size_t op_offset = 0);
    JS_REQUIRES_STACK bool test_property_cache(JSObject* obj, nanojit::LIns* obj_ins,
                                               JSObject*& obj2, jsuword& pcval);
    JS_REQUIRES_STACK bool test_property_cache_direct_slot(JSObject* obj, nanojit::LIns* obj_ins,
                                                           uint32& slot);
    void stobj_set_slot(nanojit::LIns* obj_ins, unsigned slot, nanojit::LIns*& dslots_ins,
                        nanojit::LIns* v_ins);
    void stobj_set_dslot(nanojit::LIns *obj_ins, unsigned slot, nanojit::LIns*& dslots_ins,
                         nanojit::LIns* v_ins, const char *name);

    nanojit::LIns* stobj_get_fslot(nanojit::LIns* obj_ins, unsigned slot);
    nanojit::LIns* stobj_get_slot(nanojit::LIns* obj_ins, unsigned slot,
                                  nanojit::LIns*& dslots_ins);
    bool native_set(nanojit::LIns* obj_ins, JSScopeProperty* sprop,
                    nanojit::LIns*& dslots_ins, nanojit::LIns* v_ins);
    bool native_get(nanojit::LIns* obj_ins, nanojit::LIns* pobj_ins, JSScopeProperty* sprop,
                    nanojit::LIns*& dslots_ins, nanojit::LIns*& v_ins);

    JS_REQUIRES_STACK bool name(jsval*& vp);
    JS_REQUIRES_STACK bool prop(JSObject* obj, nanojit::LIns* obj_ins, uint32& slot,
                                nanojit::LIns*& v_ins);
    JS_REQUIRES_STACK bool elem(jsval& oval, jsval& idx, jsval*& vp, nanojit::LIns*& v_ins,
                                nanojit::LIns*& addr_ins);
    JS_REQUIRES_STACK bool getProp(JSObject* obj, nanojit::LIns* obj_ins);
    JS_REQUIRES_STACK bool getProp(jsval& v);
    JS_REQUIRES_STACK bool getThis(nanojit::LIns*& this_ins);

    JS_REQUIRES_STACK void box_jsval(jsval v, nanojit::LIns*& v_ins);
    JS_REQUIRES_STACK void unbox_jsval(jsval v, nanojit::LIns*& v_ins);
    JS_REQUIRES_STACK bool guardClass(JSObject* obj, nanojit::LIns* obj_ins, JSClass* clasp,
                                      nanojit::LIns* exit);
    JS_REQUIRES_STACK bool guardDenseArray(JSObject* obj, nanojit::LIns* obj_ins,
                                           ExitType exitType = MISMATCH_EXIT);
    JS_REQUIRES_STACK bool guardDenseArrayIndex(JSObject* obj, jsint idx, nanojit::LIns* obj_ins,
                                                nanojit::LIns* dslots_ins, nanojit::LIns* idx_ins,
                                                ExitType exitType);
    JS_REQUIRES_STACK bool guardNotGlobalObject(JSObject* obj, nanojit::LIns* obj_ins);
    void clearFrameSlotsFromCache();
    JS_REQUIRES_STACK bool guardCallee(jsval& callee);
    JS_REQUIRES_STACK bool getClassPrototype(JSObject* ctor, nanojit::LIns*& proto_ins);
    JS_REQUIRES_STACK bool newArray(JSObject* ctor, uint32 argc, jsval* argv, jsval* vp);
    JS_REQUIRES_STACK bool newString(JSObject* ctor, jsval& arg, jsval* rval);
    JS_REQUIRES_STACK bool interpretedFunctionCall(jsval& fval, JSFunction* fun, uintN argc,
                                                   bool constructing);
    JS_REQUIRES_STACK bool emitNativeCall(JSTraceableNative* known, uintN argc,
                                          nanojit::LIns* args[]);
    JS_REQUIRES_STACK bool callTraceableNative(JSFunction* fun, uintN argc, bool constructing);
    JS_REQUIRES_STACK bool callNative(JSFunction* fun, uintN argc, bool constructing);
    JS_REQUIRES_STACK bool functionCall(bool constructing, uintN argc);

    JS_REQUIRES_STACK void trackCfgMerges(jsbytecode* pc);
    JS_REQUIRES_STACK void emitIf(jsbytecode* pc, bool cond, nanojit::LIns* x);
    JS_REQUIRES_STACK void fuseIf(jsbytecode* pc, bool cond, nanojit::LIns* x);
    JS_REQUIRES_STACK bool checkTraceEnd(jsbytecode* pc);

    bool hasMethod(JSObject* obj, jsid id);
    JS_REQUIRES_STACK bool hasIteratorMethod(JSObject* obj);

public:
    JS_REQUIRES_STACK
    TraceRecorder(JSContext* cx, VMSideExit*, nanojit::Fragment*, TreeInfo*,
                  unsigned stackSlots, unsigned ngslots, uint8* typeMap,
                  VMSideExit* expectedInnerExit, jsbytecode* outerTree);
    ~TraceRecorder();

    static JS_REQUIRES_STACK JSMonitorRecordingStatus monitorRecording(JSContext* cx, TraceRecorder* tr, JSOp op);

    JS_REQUIRES_STACK uint8 determineSlotType(jsval* vp);
    JS_REQUIRES_STACK nanojit::LIns* snapshot(ExitType exitType);
    nanojit::Fragment* getFragment() const { return fragment; }
    TreeInfo* getTreeInfo() const { return treeInfo; }
    JS_REQUIRES_STACK void compile(JSTraceMonitor* tm);
    JS_REQUIRES_STACK void closeLoop(JSTraceMonitor* tm, bool& demote);
    JS_REQUIRES_STACK void endLoop(JSTraceMonitor* tm);
    JS_REQUIRES_STACK void joinEdgesToEntry(nanojit::Fragmento* fragmento,
                                            VMFragment* peer_root);
    void blacklist() { fragment->blacklist(); }
    JS_REQUIRES_STACK bool adjustCallerTypes(nanojit::Fragment* f);
    JS_REQUIRES_STACK nanojit::Fragment* findNestedCompatiblePeer(nanojit::Fragment* f,
                                                                  nanojit::Fragment** empty);
    JS_REQUIRES_STACK void prepareTreeCall(nanojit::Fragment* inner);
    JS_REQUIRES_STACK void emitTreeCall(nanojit::Fragment* inner, VMSideExit* exit);
    unsigned getCallDepth() const;
    void pushAbortStack();
    void popAbortStack();
    void removeFragmentoReferences();

    JS_REQUIRES_STACK bool record_EnterFrame();
    JS_REQUIRES_STACK bool record_LeaveFrame();
    JS_REQUIRES_STACK bool record_SetPropHit(JSPropCacheEntry* entry, JSScopeProperty* sprop);
    JS_REQUIRES_STACK bool record_SetPropMiss(JSPropCacheEntry* entry);
    JS_REQUIRES_STACK bool record_DefLocalFunSetSlot(uint32 slot, JSObject* obj);
    JS_REQUIRES_STACK bool record_FastNativeCallComplete();

    void deepAbort() { deepAborted = true; }
    bool wasDeepAborted() { return deepAborted; }
    TreeInfo* getTreeInfo() { return treeInfo; }

#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format)               \
    JS_REQUIRES_STACK bool record_##op();
# include "jsopcode.tbl"
#undef OPDEF
};
#define TRACING_ENABLED(cx)       JS_HAS_OPTION(cx, JSOPTION_JIT)
#define TRACE_RECORDER(cx)        (JS_TRACE_MONITOR(cx).recorder)
#define SET_TRACE_RECORDER(cx,tr) (JS_TRACE_MONITOR(cx).recorder = (tr))

#define JSOP_IS_BINARY(op) ((uintN)((op) - JSOP_BITOR) <= (uintN)(JSOP_MOD - JSOP_BITOR))
#define JSOP_IS_UNARY(op) ((uintN)((op) - JSOP_NEG) <= (uintN)(JSOP_POS - JSOP_NEG))
#define JSOP_IS_EQUALITY(op) ((uintN)((op) - JSOP_EQ) <= (uintN)(JSOP_NE - JSOP_EQ))

#define TRACE_ARGS_(x,args)                                                   \
    JS_BEGIN_MACRO                                                            \
        TraceRecorder* tr_ = TRACE_RECORDER(cx);                              \
        if (tr_ && !tr_->wasDeepAborted() && !tr_->record_##x args)           \
            js_AbortRecording(cx, #x);                                        \
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

extern void
js_FlushJITCache(JSContext* cx);

extern void
js_PurgeJITOracle();

extern JSObject *
js_GetBuiltinFunction(JSContext *cx, uintN index);

#else  

#define TRACE_0(x)              ((void)0)
#define TRACE_1(x,a)            ((void)0)
#define TRACE_2(x,a,b)          ((void)0)

#endif 

#endif 
