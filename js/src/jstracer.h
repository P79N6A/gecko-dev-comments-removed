






































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsinterp.h"

#include "nanojit/nanojit.h"







#define JSVAL_ERROR_COOKIE OBJECT_TO_JSVAL((void*)0x10)






#define INT32_ERROR_COOKIE 0xffffabcd





template <typename T>
class Tracker {
    struct Page {
        struct Page*    next;
        jsuword         base;
        T               map[0];
    };
    struct Page* pagelist;

    jsuword         getPageBase(const void* v) const;
    struct Page*    findPage(const void* v) const;
    struct Page*    addPage(const void* v);
public:
    Tracker();
    ~Tracker();

    T               get(const void* v) const;
    void            set(const void* v, T ins);
    void            clear();
};

struct VMFragmentInfo {
    unsigned                entryNativeFrameSlots;
    unsigned                maxNativeFrameSlots;
    size_t                  nativeStackBase;
    uint8                   typeMap[0];
};

struct VMSideExitInfo {
    uint8                   typeMap[0];
};

#define TYPEMAP_GET_TYPE(x)         ((x) & JSVAL_TAGMASK)
#define TYPEMAP_SET_TYPE(x, t)      (x = (x & 0xf0) | t)
#define TYPEMAP_GET_FLAG(x, flag)   ((x) & flag)
#define TYPEMAP_SET_FLAG(x, flag)   do { (x) |= flag; } while (0)

#define TYPEMAP_FLAG_DEMOTE 0x10 /* try to record as int */
#define TYPEMAP_FLAG_DONT_DEMOTE 0x20 /* do not try to record as int */

class TraceRecorder {
    JSContext*              cx;
    Tracker<nanojit::LIns*> tracker;
    char*                   entryTypeMap;
    struct JSStackFrame*    entryFrame;
    struct JSFrameRegs      entryRegs;
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

    JSStackFrame* findFrame(void* p) const;
    bool onFrame(void* p) const;
    unsigned nativeFrameSlots(JSStackFrame* fp, JSFrameRegs& regs) const;
    size_t nativeFrameOffset(void* p) const;
    void import(jsval*, uint8& t, char *prefix, int index);
    void trackNativeFrameUse(unsigned slots);

    unsigned getCallDepth() const;
    void guard(bool expected, nanojit::LIns* cond);

    void set(void* p, nanojit::LIns* l);

    bool checkType(jsval& v, uint8& type);
    bool verifyTypeStability(JSStackFrame* fp, JSFrameRegs& regs, uint8* m);
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

    bool ifop(bool sense);
    bool inc(jsval& v, jsint incr, bool pre);
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
    void stobj_set_slot(nanojit::LIns* obj_ins, unsigned slot,
                        nanojit::LIns*& dslots_ins, nanojit::LIns* v_ins);
    nanojit::LIns* stobj_get_slot(nanojit::LIns* obj_ins, unsigned slot,
                                  nanojit::LIns*& dslots_ins);
    bool native_set(nanojit::LIns* obj_ins, JSScopeProperty* sprop,
                    nanojit::LIns*& dslots_ins, nanojit::LIns* v_ins);
    bool native_get(nanojit::LIns* obj_ins, nanojit::LIns* pobj_ins, JSScopeProperty* sprop,
                    nanojit::LIns*& dslots_ins, nanojit::LIns*& v_ins);

    bool box_jsval(jsval v, nanojit::LIns*& v_ins);
    bool unbox_jsval(jsval v, nanojit::LIns*& v_ins);
    bool guardThatObjectIsDenseArray(JSObject* obj, nanojit::LIns* obj_ins,
                                     nanojit::LIns*& dslots_ins);
    bool guardDenseArrayIndexWithinBounds(JSObject* obj, jsint idx, nanojit::LIns* obj_ins,
                                          nanojit::LIns*& dslots_ins, nanojit::LIns* idx_ins);
public:
    TraceRecorder(JSContext* cx, nanojit::Fragmento*, nanojit::Fragment*);
    ~TraceRecorder();

    JSStackFrame* getEntryFrame() const;
    JSStackFrame* getFp() const;
    JSFrameRegs& getRegs() const;
    nanojit::Fragment* getFragment() const;
    nanojit::SideExit* snapshot();

    nanojit::LIns* get(void* p);

    bool loopEdge();
    void stop();

#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format)               \
    bool op();
# include "jsopcode.tbl"
#undef OPDEF
};

FASTCALL jsdouble builtin_dmod(jsdouble a, jsdouble b);
FASTCALL jsval    builtin_BoxDouble(JSContext* cx, jsdouble d);
FASTCALL jsval    builtin_BoxInt32(JSContext* cx, jsint i);
FASTCALL jsdouble builtin_UnboxDouble(jsval v);
FASTCALL jsint    builtin_UnboxInt32(jsval v);
FASTCALL int32    builtin_doubleToInt32(jsdouble d);
FASTCALL int32    builtin_doubleToUint32(jsdouble d);






struct JSTraceMonitor {
    nanojit::Fragmento*     fragmento;
    TraceRecorder*          recorder;
};

#define TRACING_ENABLED(cx)       JS_HAS_OPTION(cx, JSOPTION_JIT)

extern bool
js_LoopEdge(JSContext* cx);

extern void
js_AbortRecording(JSContext* cx, const char* reason);

extern void
js_InitJIT(JSContext* cx);

#endif 
