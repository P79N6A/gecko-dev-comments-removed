





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

inline types::StackTypeSet *
ScriptAnalysis::pushedTypes(uint32_t offset, uint32_t which)
{
    JS_ASSERT(offset < script_->length);
    JS_ASSERT(which < GetDefCount(script_, offset) +
              (ExtendedDef(script_->code + offset) ? 1 : 0));
    types::StackTypeSet *array = getCode(offset).pushedTypes;
    JS_ASSERT(array);
    return array + which;
}

inline types::StackTypeSet *
ScriptAnalysis::pushedTypes(const jsbytecode *pc, uint32_t which)
{
    return pushedTypes(pc - script_->code, which);
}

inline types::StackTypeSet *
ScriptAnalysis::getValueTypes(const SSAValue &v)
{
    switch (v.kind()) {
      case SSAValue::PUSHED:
        return pushedTypes(v.pushedOffset(), v.pushedIndex());
      case SSAValue::VAR:
        JS_ASSERT(!slotEscapes(v.varSlot()));
        if (v.varInitial()) {
            if (v.varSlot() < LocalSlot(script_, 0))
                return types::TypeScript::SlotTypes(script_, v.varSlot());
            return undefinedTypeSet;
        } else {
            





            return pushedTypes(v.varOffset(), 0);
        }
      case SSAValue::PHI:
        return &v.phiNode()->types;
      default:
        
        MOZ_ASSUME_NOT_REACHED("Bad SSA value");
        return NULL;
    }
}

inline types::StackTypeSet *
ScriptAnalysis::poppedTypes(uint32_t offset, uint32_t which)
{
    return getValueTypes(poppedValue(offset, which));
}

inline types::StackTypeSet *
ScriptAnalysis::poppedTypes(const jsbytecode *pc, uint32_t which)
{
    return getValueTypes(poppedValue(pc, which));
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

inline types::StackTypeSet *
CrossScriptSSA::getValueTypes(const CrossSSAValue &cv)
{
    return getFrame(cv.frame).script->analysis()->getValueTypes(cv.v);
}

} 
} 

#endif 
