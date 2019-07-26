





#ifndef ion_CompileInfo_inl_h
#define ion_CompileInfo_inl_h

#include "ion/CompileInfo.h"
#include "jsgcinlines.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace ion;

CompileInfo::CompileInfo(JSScript *script, JSFunction *fun, jsbytecode *osrPc, bool constructing,
                         ExecutionMode executionMode)
  : script_(script), fun_(fun), osrPc_(osrPc), constructing_(constructing),
    executionMode_(executionMode)
{
    JS_ASSERT_IF(osrPc, JSOp(*osrPc) == JSOP_LOOPENTRY);

    
    
    if (fun_) {
        fun_ = fun_->nonLazyScript()->function();
        JS_ASSERT(fun_->isTenured());
    }

    nimplicit_ = StartArgSlot(script, fun)              
               + (fun ? 1 : 0);                         
    nargs_ = fun ? fun->nargs : 0;
    nlocals_ = script->nfixed;
    nstack_ = script->nslots - script->nfixed;
    nslots_ = nimplicit_ + nargs_ + nlocals_ + nstack_;
}

const char *
CompileInfo::filename() const
{
    return script_->filename();
}

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
