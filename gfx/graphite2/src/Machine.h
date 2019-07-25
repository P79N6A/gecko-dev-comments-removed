































#pragma once
#include <cstring>
#include <graphite2/Types.h>
#include "Main.h"

#if defined(__GNUC__)
#define     HOT             __attribute__((hot))
#if defined(__x86_64)
#define     REGPARM(n)      __attribute__((hot, regparm(n)))
#else
#define     REGPARM(n)
#endif
#else
#define     HOT
#define     REGPARM(n)
#endif

namespace graphite2 {


class Segment;
class Slot;
class SlotMap;


namespace vm 
{


typedef void * instr;
typedef Slot * slotref;

enum {VARARGS = 0xff, MAX_NAME_LEN=32};

enum opcode {
    NOP = 0,

    PUSH_BYTE,      PUSH_BYTEU,     PUSH_SHORT,     PUSH_SHORTU,    PUSH_LONG,

    ADD,            SUB,            MUL,            DIV,
    MIN_,           MAX_,
    NEG,
    TRUNC8,         TRUNC16,

    COND,

    AND,            OR,             NOT,
    EQUAL,          NOT_EQ,
    LESS,           GTR,            LESS_EQ,        GTR_EQ,

    NEXT,           NEXT_N,         COPY_NEXT,
    PUT_GLYPH_8BIT_OBS,              PUT_SUBS_8BIT_OBS,   PUT_COPY,
    INSERT,         DELETE,
    ASSOC,
    CNTXT_ITEM,

    ATTR_SET,       ATTR_ADD,       ATTR_SUB,
    ATTR_SET_SLOT,
    IATTR_SET_SLOT,
    PUSH_SLOT_ATTR,                 PUSH_GLYPH_ATTR_OBS,
    PUSH_GLYPH_METRIC,              PUSH_FEAT,
    PUSH_ATT_TO_GATTR_OBS,          PUSH_ATT_TO_GLYPH_METRIC,
    PUSH_ISLOT_ATTR,

    PUSH_IGLYPH_ATTR,    

    POP_RET,                        RET_ZERO,           RET_TRUE,
    IATTR_SET,                      IATTR_ADD,          IATTR_SUB,
    PUSH_PROC_STATE,                PUSH_VERSION,
    PUT_SUBS,                       PUT_SUBS2,          PUT_SUBS3,
    PUT_GLYPH,                      PUSH_GLYPH_ATTR,    PUSH_ATT_TO_GLYPH_ATTR,
    MAX_OPCODE,
    
    TEMP_COPY = MAX_OPCODE
};

struct opcode_t 
{
    instr           impl[2];
    uint8           param_sz;
    char            name[MAX_NAME_LEN];
};


class Machine
{
public:
    typedef int32  stack_t;
    static size_t const STACK_ORDER  = 10,
                        STACK_MAX    = 1 << STACK_ORDER,
                        STACK_GUARD  = 2;

    class Code;

    enum status_t {
        finished = 0,
        stack_underflow,
        stack_not_empty,
        stack_overflow,
        slot_offset_out_bounds
    };

    Machine(SlotMap &) throw();
    static const opcode_t *   getOpcodeTable() throw();

    CLASS_NEW_DELETE;

    SlotMap   & slotMap() const throw();
    status_t	status() const throw();
    operator bool () const throw();

private:
    void    check_final_stack(const stack_t * const sp);
    stack_t run(const instr * program, const byte * data,
                slotref * & map) HOT;

    SlotMap       & _map;
    stack_t         _stack[STACK_MAX + 2*STACK_GUARD];
    status_t		_status;
};

inline Machine::Machine(SlotMap & map) throw()
: _map(map), _status(finished)
{
	memset(_stack, 0, STACK_GUARD);
}

inline SlotMap& Machine::slotMap() const throw()
{
    return _map;
}

inline Machine::status_t Machine::status() const throw()
{
    return _status;
}

inline void Machine::check_final_stack(const int32 * const sp)
{
    stack_t const * const base  = _stack + STACK_GUARD,
                  * const limit = base + STACK_MAX;
    if      (sp <  base)    _status = stack_underflow;       
    else if (sp >= limit)   _status = stack_overflow;        
    else if (sp != base)    _status = stack_not_empty;
    else                    _status = finished;
}

} 
} 

#ifdef ENABLE_DEEP_TRACING
#define STARTTRACE(name,is) if (XmlTraceLog::get().active()) { \
                                XmlTraceLog::get().openElement(ElementOpCode); \
                                XmlTraceLog::get().addAttribute(AttrName, # name); \
                                XmlTraceLog::get().addAttribute(AttrIndex, unsigned(map - smap.begin())); \
                            }

#define ENDTRACE            XmlTraceLog::get().closeElement(ElementOpCode)
#else 
#define STARTTRACE(name,is)
#define ENDTRACE
#endif 



