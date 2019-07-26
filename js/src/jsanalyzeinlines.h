





#ifndef jsanalyzeinlines_h
#define jsanalyzeinlines_h

#include "jsanalyze.h"

#include "jsopcodeinlines.h"

namespace js {
namespace analyze {

inline const SSAValue &
ScriptAnalysis::poppedValue(uint32_t offset, uint32_t which)
{
    JS_ASSERT(offset < script_->length);
    JS_ASSERT(which < GetUseCount(script_, offset) +
              (ExtendedUse(script_->code + offset) ? 1 : 0));
    return getCode(offset).poppedValues[which];
}

inline const SSAValue &
ScriptAnalysis::poppedValue(const jsbytecode *pc, uint32_t which)
{
    return poppedValue(pc - script_->code, which);
}

inline SSAUseChain *&
ScriptAnalysis::useChain(const SSAValue &v)
{
    JS_ASSERT(trackUseChain(v));
    if (v.kind() == SSAValue::PUSHED)
        return getCode(v.pushedOffset()).pushedUses[v.pushedIndex()];
    if (v.kind() == SSAValue::VAR)
        return getCode(v.varOffset()).pushedUses[GetDefCount(script_, v.varOffset())];
    return v.phiNode()->uses;
}

inline jsbytecode *
ScriptAnalysis::getCallPC(jsbytecode *pc)
{
    SSAUseChain *uses = useChain(SSAValue::PushedValue(pc - script_->code, 0));
    JS_ASSERT(uses && uses->popped);
    JS_ASSERT(js_CodeSpec[script_->code[uses->offset]].format & JOF_INVOKE);
    return script_->code + uses->offset;
}

} 
} 

#endif 
