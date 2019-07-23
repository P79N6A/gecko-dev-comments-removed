








































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsinterp.h"

#include "nanojit/nanojit.h"







#define JSVAL_ERROR_COOKIE OBJECT_TO_JSVAL((void*)0x10)






#define INT32_ERROR_COOKIE 0xffffabcd





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

    nanojit::LIns*  get(const void* v) const;
    void            set(const void* v, nanojit::LIns* ins);
    void            clear();
};

class VMFragmentInfo {
public:
    VMFragmentInfo() {
        typeMap = NULL;
        gslots = NULL;
    }

    virtual ~VMFragmentInfo() {
        if (typeMap) free(typeMap);
        if (gslots) free(gslots);
    }
    
    unsigned                entryNativeFrameSlots;
    unsigned                maxNativeFrameSlots;
    size_t                  nativeStackBase;
    unsigned                maxCallDepth;
    uint32                  globalShape;
    unsigned                ngslots;
    uint8                  *typeMap;
    uint16                 *gslots;
};

extern struct nanojit::CallInfo builtins[];

#define TYPEMAP_GET_TYPE(x)         ((x) & JSVAL_TAGMASK)
#define TYPEMAP_SET_TYPE(x, t)      (x = (x & 0xf0) | t)
#define TYPEMAP_GET_FLAG(x, flag)   ((x) & flag)
#define TYPEMAP_SET_FLAG(x, flag)   do { (x) |= flag; } while (0)

#define TYPEMAP_TYPE_ANY            7

#define TYPEMAP_FLAG_DEMOTE 0x10 /* try to record as int */
#define TYPEMAP_FLAG_DONT_DEMOTE 0x20 /* do not try to record as int */

class TraceRecorder {
    JSContext*              cx;
    JSObject*               globalObj;
    Tracker                 tracker;
    char*                   entryTypeMap;
    struct JSStackFrame*    entryFrame;
    struct JSFrameRegs      entryRegs;
    JSAtom**                atoms;
    nanojit::Fragment*      fragment;
    VMFragmentInfo*         fragmentInfo;
    nanojit::LirBuffer*     lirbuf;
    nanojit::LirWriter*     lir;
    nanojit::LirBufWriter*  lir_buf_writer;
    nanojit::LirWriter*     verbose_filter;
    nanojit::LirWriter*     cse_filter;
    nanojit::LirWriter*     expr_filter;
    nanojit::LirWriter*     exit_filter;
    nanojit::LirWriter*     func_filter;
    nanojit::LIns*          cx_ins;
    nanojit::SideExit       exit;
    bool                    recompileFlag;

    JSStackFrame* findFrame(jsval* p) const;
    bool onFrame(jsval* p) const;
    bool isGlobal(jsval* p) const;
    size_t nativeFrameOffset(jsval* p) const;
    void import(jsval* p, uint8& t, const char *prefix, int index);
    void trackNativeFrameUse(unsigned slots);

    unsigned getCallDepth() const;
    void guard(bool expected, nanojit::LIns* cond);

    nanojit::LIns* get(jsval* p);
    void set(jsval* p, nanojit::LIns* l);

    bool checkType(jsval& v, uint8& type);
    bool verifyTypeStability(JSStackFrame* entryFrame, JSStackFrame* currentFrame, uint8* m);
    void closeLoop(nanojit::Fragmento* fragmento);

    jsval& argval(unsigned n) const;
    jsval& varval(unsigned n) const;
    jsval& stackval(int n) const;

    nanojit::LIns* arg(unsigned n);
    void arg(unsigned n, nanojit::LIns* i);
    nanojit::LIns* var(unsigned n);
    void var(unsigned n, nanojit::LIns* i);
    nanojit::LIns* stack(int n);
    void stack(int n, nanojit::LIns* i);

    nanojit::LIns* f2i(nanojit::LIns* f);

    bool ifop();
    bool inc(jsval& v, jsint incr, bool pre = true);
    bool cmp(nanojit::LOpcode op, bool negate = false);

    bool unary(nanojit::LOpcode op);
    bool binary(nanojit::LOpcode op);

    bool ibinary(nanojit::LOpcode op);
    bool iunary(nanojit::LOpcode op);
    bool bbinary(nanojit::LOpcode op);
    void demote(jsval& v, jsdouble result);

    bool map_is_native(JSObjectMap* map, nanojit::LIns* map_ins);
    bool test_property_cache(JSObject* obj, nanojit::LIns* obj_ins, JSObject*& obj2,
                             JSPropCacheEntry*& entry);
    bool test_property_cache_direct_slot(JSObject* obj, nanojit::LIns* obj_ins, uint32& slot);
    void stobj_set_slot(nanojit::LIns* obj_ins, unsigned slot,
                        nanojit::LIns*& dslots_ins, nanojit::LIns* v_ins);
    nanojit::LIns* stobj_get_slot(nanojit::LIns* obj_ins, unsigned slot,
                                  nanojit::LIns*& dslots_ins);
    bool native_set(nanojit::LIns* obj_ins, JSScopeProperty* sprop,
                    nanojit::LIns*& dslots_ins, nanojit::LIns* v_ins);
    bool native_get(nanojit::LIns* obj_ins, nanojit::LIns* pobj_ins, JSScopeProperty* sprop,
                    nanojit::LIns*& dslots_ins, nanojit::LIns*& v_ins);

    bool getProp(JSObject* obj, nanojit::LIns* obj_ins);
    bool getProp(jsval& v);
    bool getThis(nanojit::LIns*& this_ins);
    
    bool box_jsval(jsval v, nanojit::LIns*& v_ins);
    bool unbox_jsval(jsval v, nanojit::LIns*& v_ins);
    bool guardThatObjectHasClass(JSObject* obj, nanojit::LIns* obj_ins,
                                 JSClass* cls, nanojit::LIns*& dslots_ins);
    bool guardThatObjectIsDenseArray(JSObject* obj, nanojit::LIns* obj_ins,
                                     nanojit::LIns*& dslots_ins);
    bool guardDenseArrayIndexWithinBounds(JSObject* obj, jsint idx, nanojit::LIns* obj_ins,
                                          nanojit::LIns*& dslots_ins, nanojit::LIns* idx_ins);
public:
    TraceRecorder(JSContext* cx, nanojit::Fragment*, uint8* typemap);
    ~TraceRecorder();

    nanojit::SideExit* snapshot();

    bool loopEdge();
    void stop();

    bool record_after_JSOP_CALL();
    
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format)               \
    bool record_##op();
# include "jsopcode.tbl"
#undef OPDEF
};

#define TRACING_ENABLED(cx)       JS_HAS_OPTION(cx, JSOPTION_JIT)

#define RECORD(x)                                                             \
    JS_BEGIN_MACRO                                                            \
        TraceRecorder* r = JS_TRACE_MONITOR(cx).recorder;                     \
        if (!r->record_##x()) {                                               \
            js_AbortRecording(cx, #x);                                        \
            ENABLE_TRACER(0);                                                 \
        }                                                                     \
    JS_END_MACRO

extern bool
js_LoopEdge(JSContext* cx);

extern void
js_AbortRecording(JSContext* cx, const char* reason);

extern void
js_InitJIT(JSContext* cx);

#endif 
