






#ifndef jsion_compileinfo_inl_h__
#define jsion_compileinfo_inl_h__

#include "CompileInfo.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace ion;

JSAtom *
CompileInfo::getAtom(jsbytecode *pc) const
{
    return script_->getAtom(GET_UINT32_INDEX(pc));
}

PropertyName *
CompileInfo::getName(jsbytecode *pc) const
{
    return script_->getName(GET_UINT32_INDEX(pc));
}

RegExpObject *
CompileInfo::getRegExp(jsbytecode *pc) const
{
    return script_->getRegExp(GET_UINT32_INDEX(pc));
}

JSFunction *
CompileInfo::getFunction(jsbytecode *pc) const
{
    return script_->getFunction(GET_UINT32_INDEX(pc));
}

JSObject *
CompileInfo::getObject(jsbytecode *pc) const
{
    return script_->getObject(GET_UINT32_INDEX(pc));
}

const Value &
CompileInfo::getConst(jsbytecode *pc) const
{
    return script_->getConst(GET_UINT32_INDEX(pc));
}

jssrcnote *
CompileInfo::getNote(JSContext *cx, jsbytecode *pc) const
{
    return js_GetSrcNote(cx, script(), pc);
}

#endif 
