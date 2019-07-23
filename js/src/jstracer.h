






































#ifndef jstracer_h___
#define jstracer_h___

#include "jsstddef.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsinterp.h"

#include "nanojit/nanojit.h"





class Tracker 
{
    struct Page {
        struct Page* next;
        long base;
        nanojit::LIns* map[0];
    };
    struct Page* pagelist;
    
    long            getPageBase(const void* v) const;
    struct Page*    findPage(const void* v) const;
    struct Page*    addPage(const void* v);
public:    
    Tracker();
    ~Tracker();
    
    nanojit::LIns*  get(const void* v) const;
    void            set(const void* v, nanojit::LIns* ins);
    void            clear();
};

class TraceRecorder {
    JSContext*              cx;
    Tracker                 tracker;
    char*                   entryTypeMap;
    unsigned                entryNativeFrameSlots;
    unsigned                maxNativeFrameSlots;
    struct JSStackFrame*    entryFrame;
    struct JSFrameRegs      entryRegs;
    nanojit::Fragment*      fragment;
    nanojit::LirBuffer*     lirbuf;
    nanojit::LirWriter*     lir;
    nanojit::LirBufWriter*  lir_buf_writer;
    nanojit::LirWriter*     verbose_filter;
    nanojit::LirWriter*     cse_filter;
    nanojit::LirWriter*     expr_filter;
    nanojit::LIns*          cx_ins;
    nanojit::SideExit       exit;
    
    JSStackFrame* findFrame(void* p) const;
    bool onFrame(void* p) const;
    unsigned nativeFrameSlots(JSStackFrame* fp, JSFrameRegs& regs) const;
    unsigned nativeFrameOffset(void* p) const;
    void import(jsval*, char *prefix = NULL, int index = 0);
    void trackNativeFrameUse(unsigned slots);
    
    inline jsbytecode* entryPC() const
    {
        return entryRegs.pc;
    }
    
    nanojit::SideExit* snapshot();

    unsigned calldepth() const;

    void set(void* p, nanojit::LIns* l);
    nanojit::LIns* get(void* p);
    
    void copy(void* a, void* v);
    void imm(jsint i, void* v);
    void imm(jsdouble d, void* v);
    void unary(nanojit::LOpcode op, void* a, void* v);
    void binary(nanojit::LOpcode op, void* a, void* b, void* v);
    void binary0(nanojit::LOpcode op, void* a, void* v);
    void choose(void* cond, void* iftrue, void* iffalse, void* v);
    void choose_eqi(void* a, int b, void* iftrue, void* iffalse, void* v);
    
    void call(int id, void* a, void* v);
    void call(int id, void* a, void* b, void* v);
    void call(int id, void* a, void* b, void* c, void* v);
    
    void iinc(void* a, int32_t incr, void* v);

    void load(void* a, int32_t i, void* v);

    void guard_0(bool expected, void* a);
    void guard_h(bool expected, void* a);
    void guard_ov(bool expected, void* a);
    void guard_eq(bool expected, void* a, void* b);
    void guard_eqi(bool expected, void* a, int i);
    
    void closeLoop(nanojit::Fragmento* fragmento);

public:
    TraceRecorder(JSContext* cx, nanojit::Fragmento*, nanojit::Fragment*);
    ~TraceRecorder();

    bool loopEdge(JSContext* cx, jsbytecode* pc);
    
    bool JSOP_INTERRUPT();
    bool JSOP_PUSH();
    bool JSOP_POPV();
    bool JSOP_ENTERWITH();
    bool JSOP_LEAVEWITH();
    bool JSOP_RETURN();
    bool JSOP_GOTO();
    bool JSOP_IFEQ();
    bool JSOP_IFNE();
    bool JSOP_ARGUMENTS();
    bool JSOP_FORARG();
    bool JSOP_FORVAR();
    bool JSOP_DUP();
    bool JSOP_DUP2();
    bool JSOP_SETCONST();
    bool JSOP_BITOR();
    bool JSOP_BITXOR();
    bool JSOP_BITAND();
    bool JSOP_EQ();
    bool JSOP_NE();
    bool JSOP_LT();
    bool JSOP_LE();
    bool JSOP_GT();
    bool JSOP_GE();
    bool JSOP_LSH();
    bool JSOP_RSH();
    bool JSOP_URSH();
    bool JSOP_ADD();
    bool JSOP_SUB();
    bool JSOP_MUL();
    bool JSOP_DIV();
    bool JSOP_MOD();
    bool JSOP_NOT();
    bool JSOP_BITNOT();
    bool JSOP_NEG();
    bool JSOP_NEW();
    bool JSOP_DELNAME();
    bool JSOP_DELPROP();
    bool JSOP_DELELEM();
    bool JSOP_TYPEOF();
    bool JSOP_VOID();
    bool JSOP_INCNAME();
    bool JSOP_INCPROP();
    bool JSOP_INCELEM();
    bool JSOP_DECNAME();
    bool JSOP_DECPROP();
    bool JSOP_DECELEM();
    bool JSOP_NAMEINC();
    bool JSOP_PROPINC();
    bool JSOP_ELEMINC();
    bool JSOP_NAMEDEC();
    bool JSOP_PROPDEC();
    bool JSOP_ELEMDEC();
    bool JSOP_GETPROP();
    bool JSOP_SETPROP();
    bool JSOP_GETELEM();
    bool JSOP_SETELEM();
    bool JSOP_CALLNAME();
    bool JSOP_CALL();
    bool JSOP_NAME();
    bool JSOP_DOUBLE();
    bool JSOP_STRING();
    bool JSOP_ZERO();
    bool JSOP_ONE();
    bool JSOP_NULL();
    bool JSOP_THIS();
    bool JSOP_FALSE();
    bool JSOP_TRUE();
    bool JSOP_OR();
    bool JSOP_AND();
    bool JSOP_TABLESWITCH();
    bool JSOP_LOOKUPSWITCH();
    bool JSOP_STRICTEQ();
    bool JSOP_STRICTNE();
    bool JSOP_CLOSURE();
    bool JSOP_EXPORTALL();
    bool JSOP_EXPORTNAME();
    bool JSOP_IMPORTALL();
    bool JSOP_IMPORTPROP();
    bool JSOP_IMPORTELEM();
    bool JSOP_OBJECT();
    bool JSOP_POP();
    bool JSOP_POS();
    bool JSOP_TRAP();
    bool JSOP_GETARG();
    bool JSOP_SETARG();
    bool JSOP_GETVAR();
    bool JSOP_SETVAR();
    bool JSOP_UINT16();
    bool JSOP_NEWINIT();
    bool JSOP_ENDINIT();
    bool JSOP_INITPROP();
    bool JSOP_INITELEM();
    bool JSOP_DEFSHARP();
    bool JSOP_USESHARP();
    bool JSOP_INCARG();
    bool JSOP_INCVAR();
    bool JSOP_DECARG();
    bool JSOP_DECVAR();
    bool JSOP_ARGINC();
    bool JSOP_VARINC();
    bool JSOP_ARGDEC();
    bool JSOP_VARDEC();
    bool JSOP_ITER();
    bool JSOP_FORNAME();
    bool JSOP_FORPROP();
    bool JSOP_FORELEM();
    bool JSOP_POPN();
    bool JSOP_BINDNAME();
    bool JSOP_SETNAME();
    bool JSOP_THROW();
    bool JSOP_IN();
    bool JSOP_INSTANCEOF();
    bool JSOP_DEBUGGER();
    bool JSOP_GOSUB();
    bool JSOP_RETSUB();
    bool JSOP_EXCEPTION();
    bool JSOP_LINENO();
    bool JSOP_CONDSWITCH();
    bool JSOP_CASE();
    bool JSOP_DEFAULT();
    bool JSOP_EVAL();
    bool JSOP_ENUMELEM();
    bool JSOP_GETTER();
    bool JSOP_SETTER();
    bool JSOP_DEFFUN();
    bool JSOP_DEFCONST();
    bool JSOP_DEFVAR();
    bool JSOP_ANONFUNOBJ();
    bool JSOP_NAMEDFUNOBJ();
    bool JSOP_SETLOCALPOP();
    bool JSOP_GROUP();
    bool JSOP_SETCALL();
    bool JSOP_TRY();
    bool JSOP_FINALLY();
    bool JSOP_NOP();
    bool JSOP_ARGSUB();
    bool JSOP_ARGCNT();
    bool JSOP_DEFLOCALFUN();
    bool JSOP_GOTOX();
    bool JSOP_IFEQX();
    bool JSOP_IFNEX();
    bool JSOP_ORX();
    bool JSOP_ANDX();
    bool JSOP_GOSUBX();
    bool JSOP_CASEX();
    bool JSOP_DEFAULTX();
    bool JSOP_TABLESWITCHX();
    bool JSOP_LOOKUPSWITCHX();
    bool JSOP_BACKPATCH();
    bool JSOP_BACKPATCH_POP();
    bool JSOP_THROWING();
    bool JSOP_SETRVAL();
    bool JSOP_RETRVAL();
    bool JSOP_GETGVAR();
    bool JSOP_SETGVAR();
    bool JSOP_INCGVAR();
    bool JSOP_DECGVAR();
    bool JSOP_GVARINC();
    bool JSOP_GVARDEC();
    bool JSOP_REGEXP();
    bool JSOP_DEFXMLNS();
    bool JSOP_ANYNAME();
    bool JSOP_QNAMEPART();
    bool JSOP_QNAMECONST();
    bool JSOP_QNAME();
    bool JSOP_TOATTRNAME();
    bool JSOP_TOATTRVAL();
    bool JSOP_ADDATTRNAME();
    bool JSOP_ADDATTRVAL();
    bool JSOP_BINDXMLNAME();
    bool JSOP_SETXMLNAME();
    bool JSOP_XMLNAME();
    bool JSOP_DESCENDANTS();
    bool JSOP_FILTER();
    bool JSOP_ENDFILTER();
    bool JSOP_TOXML();
    bool JSOP_TOXMLLIST();
    bool JSOP_XMLTAGEXPR();
    bool JSOP_XMLELTEXPR();
    bool JSOP_XMLOBJECT();
    bool JSOP_XMLCDATA();
    bool JSOP_XMLCOMMENT();
    bool JSOP_XMLPI();
    bool JSOP_CALLPROP();
    bool JSOP_GETFUNNS();
    bool JSOP_UNUSED186();
    bool JSOP_DELDESC();
    bool JSOP_UINT24();
    bool JSOP_INDEXBASE();
    bool JSOP_RESETBASE();
    bool JSOP_RESETBASE0();
    bool JSOP_STARTXML();
    bool JSOP_STARTXMLEXPR();
    bool JSOP_CALLELEM();
    bool JSOP_STOP();
    bool JSOP_GETXPROP();
    bool JSOP_CALLXMLNAME();
    bool JSOP_TYPEOFEXPR();
    bool JSOP_ENTERBLOCK();
    bool JSOP_LEAVEBLOCK();
    bool JSOP_GETLOCAL();
    bool JSOP_SETLOCAL();
    bool JSOP_INCLOCAL();
    bool JSOP_DECLOCAL();
    bool JSOP_LOCALINC();
    bool JSOP_LOCALDEC();
    bool JSOP_FORLOCAL();
    bool JSOP_FORCONST();
    bool JSOP_ENDITER();
    bool JSOP_GENERATOR();
    bool JSOP_YIELD();
    bool JSOP_ARRAYPUSH();
    bool JSOP_UNUSED213();
    bool JSOP_ENUMCONSTELEM();
    bool JSOP_LEAVEBLOCKEXPR();
    bool JSOP_GETTHISPROP();
    bool JSOP_GETARGPROP();
    bool JSOP_GETVARPROP();
    bool JSOP_GETLOCALPROP();
    bool JSOP_INDEXBASE1();
    bool JSOP_INDEXBASE2();
    bool JSOP_INDEXBASE3();
    bool JSOP_CALLGVAR();
    bool JSOP_CALLVAR();
    bool JSOP_CALLARG();
    bool JSOP_CALLLOCAL();
    bool JSOP_INT8();
    bool JSOP_INT32();
    bool JSOP_LENGTH();
    bool JSOP_NEWARRAY();
    bool JSOP_HOLE();
};











struct JSTraceMonitor {
    int                     freq;
    nanojit::Fragmento*     fragmento;
    TraceRecorder*          recorder;
};

struct VMFragmentInfo {
    unsigned                maxNativeFrameSlots;
    char                    typeMap[0];
};

struct VMSideExitInfo {
    char                    typeMap[0];
};

#define TRACING_ENABLED(cx)       JS_HAS_OPTION(cx, JSOPTION_JIT)
#define TRACE_TRIGGER_MASK 0x3f

extern void
js_LoopEdge(JSContext* cx);

extern void
js_AbortRecording(JSContext* cx);

#endif 
