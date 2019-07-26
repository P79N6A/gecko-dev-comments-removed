






#if !defined(jsion_baseline_inspector_h__) && defined(JS_ION)
#define jsion_baseline_inspector_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "BaselineJIT.h"

namespace js {
namespace ion {

class BaselineInspector
{
  private:
    RootedScript script;
    ICEntry *prevLookedUpEntry;

  public:
    BaselineInspector(JSContext *cx, RawScript rawScript)
      : script(cx, rawScript), prevLookedUpEntry(NULL)
    {
        JS_ASSERT(script);
    }

    bool hasBaselineScript() const {
        return script->hasBaselineScript();
    }

    BaselineScript *baselineScript() const {
        JS_ASSERT(hasBaselineScript());
        return script->baseline;
    }

  private:
#ifdef DEBUG
    bool isValidPC(jsbytecode *pc) {
        return (pc >= script->code) && (pc < script->code + script->length);
    }
#endif

    ICEntry &icEntryFromPC(jsbytecode *pc) {
        JS_ASSERT(hasBaselineScript());
        JS_ASSERT(isValidPC(pc));
        ICEntry &ent = baselineScript()->icEntryFromPCOffset(pc - script->code, prevLookedUpEntry);
        prevLookedUpEntry = &ent;
        return ent;
    }

  public:
    RawShape maybeMonomorphicShapeForPropertyOp(jsbytecode *pc);
};

} 
} 

#endif

