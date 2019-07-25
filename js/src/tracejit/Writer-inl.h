





































#ifndef tracejit_Writer_inl_h___
#define tracejit_Writer_inl_h___

#include "Writer.h"

namespace js {
namespace tjit {

namespace nj = nanojit;

nj::LIns *
Writer::getArgsLength(nj::LIns *args) const
{
    uint32 slot = ArgumentsObject::INITIAL_LENGTH_SLOT;
    nj::LIns *vaddr_ins = ldpObjSlots(args);
    return name(lir->insLoad(nj::LIR_ldi, vaddr_ins, slot * sizeof(Value) + sPayloadOffset,
                             ACCSET_SLOTS),
                "argsLength");
}

inline nj::LIns *
Writer::ldpIterCursor(nj::LIns *iter) const
{
    return name(lir->insLoad(nj::LIR_ldp, iter, offsetof(NativeIterator, props_cursor),
                             ACCSET_ITER),
                "cursor");
}

inline nj::LIns *
Writer::ldpIterEnd(nj::LIns *iter) const
{
    return name(lir->insLoad(nj::LIR_ldp, iter, offsetof(NativeIterator, props_end), ACCSET_ITER),
                "end");
}

inline nj::LIns *
Writer::stpIterCursor(nj::LIns *cursor, nj::LIns *iter) const
{
    return lir->insStore(nj::LIR_stp, cursor, iter, offsetof(NativeIterator, props_cursor),
                         ACCSET_ITER);
}

} 
} 

#endif 
