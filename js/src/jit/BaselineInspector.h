





#ifndef jit_BaselineInspector_h
#define jit_BaselineInspector_h

#ifdef JS_ION

#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"
#include "jit/MIR.h"

namespace js {
namespace jit {

class BaselineInspector;

class ICInspector
{
  protected:
    BaselineInspector *inspector_;
    jsbytecode *pc_;
    ICEntry *icEntry_;

    ICInspector(BaselineInspector *inspector, jsbytecode *pc, ICEntry *icEntry)
      : inspector_(inspector), pc_(pc), icEntry_(icEntry)
    { }
};

class SetElemICInspector : public ICInspector
{
  public:
    SetElemICInspector(BaselineInspector *inspector, jsbytecode *pc, ICEntry *icEntry)
      : ICInspector(inspector, pc, icEntry)
    { }

    bool sawOOBDenseWrite() const;
    bool sawOOBTypedArrayWrite() const;
    bool sawDenseWrite() const;
    bool sawTypedArrayWrite() const;
};

class BaselineInspector
{
  private:
    RootedScript script;
    ICEntry *prevLookedUpEntry;

  public:
    BaselineInspector(JSContext *cx, JSScript *rawScript)
      : script(cx, rawScript), prevLookedUpEntry(nullptr)
    {
        JS_ASSERT(script);
    }

    bool hasBaselineScript() const {
        return script->hasBaselineScript();
    }

    BaselineScript *baselineScript() const {
        return script->baselineScript();
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
        JS_ASSERT(ent.isForOp());
        prevLookedUpEntry = &ent;
        return ent;
    }

    template <typename ICInspectorType>
    ICInspectorType makeICInspector(jsbytecode *pc, ICStub::Kind expectedFallbackKind) {
        ICEntry *ent = nullptr;
        if (hasBaselineScript()) {
            ent = &icEntryFromPC(pc);
            JS_ASSERT(ent->fallbackStub()->kind() == expectedFallbackKind);
        }
        return ICInspectorType(this, pc, ent);
    }

    ICStub *monomorphicStub(jsbytecode *pc);
    bool dimorphicStub(jsbytecode *pc, ICStub **pfirst, ICStub **psecond);

  public:
    typedef Vector<Shape *, 4, IonAllocPolicy> ShapeVector;
    bool maybeShapesForPropertyOp(jsbytecode *pc, ShapeVector &shapes);

    SetElemICInspector setElemICInspector(jsbytecode *pc) {
        return makeICInspector<SetElemICInspector>(pc, ICStub::SetElem_Fallback);
    }

    MIRType expectedResultType(jsbytecode *pc);
    MCompare::CompareType expectedCompareType(jsbytecode *pc);
    MIRType expectedBinaryArithSpecialization(jsbytecode *pc);

    bool hasSeenNonNativeGetElement(jsbytecode *pc);
    bool hasSeenNegativeIndexGetElement(jsbytecode *pc);
    bool hasSeenAccessedGetter(jsbytecode *pc);
    bool hasSeenDoubleResult(jsbytecode *pc);
    bool hasSeenNonStringIterNext(jsbytecode *pc);

    JSObject *getTemplateObject(jsbytecode *pc);
    JSObject *getTemplateObjectForNative(jsbytecode *pc, Native native);
};

} 
} 

#endif 

#endif 
