





#ifndef ion_CompileInfo_inl_h
#define ion_CompileInfo_inl_h

#include "ion/CompileInfo.h"

#include "jsscriptinlines.h"

using namespace js;
using namespace ion;

inline RegExpObject *
CompileInfo::getRegExp(jsbytecode *pc) const
{
    return script_->getRegExp(GET_UINT32_INDEX(pc));
}

inline JSFunction *
CompileInfo::getFunction(jsbytecode *pc) const
{
    return script_->getFunction(GET_UINT32_INDEX(pc));
}

#endif 
