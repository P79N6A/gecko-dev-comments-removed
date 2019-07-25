






































#include <cassert>
#include <cstring>
#include "inc/Machine.h"
#include "inc/Segment.h"
#include "inc/Slot.h"
#include "inc/Rule.h"

#define STARTOP(name)           name: {
#define ENDOP                   }; goto *((sp - sb)/Machine::STACK_MAX ? &&end : *++ip);
#define EXIT(status)            { push(status); goto end; }

#define do_(name)               &&name


using namespace graphite2;
using namespace vm;

namespace {

const void * direct_run(const bool          get_table_mode,
                        const instr       * program,
                        const byte        * data,
                        Machine::stack_t  * stack,
                        slotref         * & __map,
                        SlotMap           * __smap=0)
{
    
    
    #include "inc/opcode_table.h"
    if (get_table_mode)
        return opcode_table;

    
    const instr       * ip = program;
    const byte        * dp = data;
    Machine::stack_t  * sp = stack + Machine::STACK_GUARD,
                * const sb = sp;
    SlotMap         & smap = *__smap;
    Segment          & seg = smap.segment;
    slotref             is = *__map,
                     * map = __map,
              * const mapb = smap.begin()+smap.context();
    int8             flags = 0;
    
    
    goto **ip;

    
    #include "inc/opcodes.h"
    
    end:
    __map  = map;
    *__map = is;
    return sp;
}

}

const opcode_t * Machine::getOpcodeTable() throw()
{
    slotref * dummy;
    return static_cast<const opcode_t *>(direct_run(true, 0, 0, 0, dummy, 0));
}


Machine::stack_t  Machine::run(const instr   * program,
                               const byte    * data,
                               slotref     * & is)
{
    assert(program != 0);
    assert(_status == finished);
    
    const stack_t *sp = static_cast<const stack_t *>(
                direct_run(false, program, data, _stack, is, &_map));
    const stack_t ret = sp == _stack+STACK_GUARD+1 ? *sp-- : 0;
    check_final_stack(sp);
    return ret;
}

