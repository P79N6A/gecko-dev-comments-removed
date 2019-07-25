

































#include <cassert>
#include <cstring>
#include <graphite2/Segment.h>
#include "Machine.h"
#include "Segment.h"
#include "XmlTraceLog.h"
#include "Slot.h"
#include "Rule.h"



#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define registers           const byte * & dp, vm::Machine::stack_t * & sp, \
                            vm::Machine::stack_t * const sb, regbank & reg


#define STARTOP(name)	    bool name(registers) REGPARM(4);\
                            bool name(registers) { \
                                STARTTRACE(name,is);
#define ENDOP                   ENDTRACE; \
                                return (sp - sb)/Machine::STACK_MAX==0; \
                            }

#define EXIT(status)        { push(status); ENDTRACE; return false; }


#define do_(name)           instr(name)


using namespace graphite2;
using namespace vm;

struct regbank  {
    slotref         is;
    slotref *       map;
    SlotMap       & smap;
    slotref * const map_base;
    const instr * & ip;
    int8            flags;
};

typedef bool        (* ip_t)(registers);




namespace {
#define smap    reg.smap
#define seg     smap.segment
#define is      reg.is
#define ip      reg.ip
#define map     reg.map
#define mapb    reg.map_base
#define flags   reg.flags

#include "opcodes.h"

#undef smap
#undef seg
#undef is
#undef ip
#undef map
#undef mapb
#undef flags
}

Machine::stack_t  Machine::run(const instr   * program,
                               const byte    * data,
                               slotref     * & map)

{
    assert(program != 0);
    assert(_status == finished);


    
    const instr   * ip = program-1;
    const byte    * dp = data;
    stack_t       * sp = _stack + Machine::STACK_GUARD,
            * const sb = sp;
    regbank         reg = {*map, map, _map, _map.begin()+_map.context(), ip, 0};

    
    while ((reinterpret_cast<ip_t>(*++ip))(dp, sp, sb, reg)) {}
    const stack_t ret = sp == _stack+STACK_GUARD+1 ? *sp-- : 0;

    check_final_stack(sp);
    map = reg.map;
    *map = reg.is;
    return ret;
}


namespace {
#include "opcode_table.h"
}

const opcode_t * Machine::getOpcodeTable() throw()
{
    return opcode_table;
}


