





#include "mozilla/IncrementalClearCOMRuleArray.h"

#include "nsCycleCollector.h"
#include "mozilla/DeferredFinalize.h"
#include "nsTArray.h"
#include "nsCCUncollectableMarker.h"

using namespace mozilla;

typedef nsCOMArray<css::Rule> RuleArray;
typedef nsTArray<RuleArray> RuleArrayArray;




static void*
AppendRulesArrayPointer(void* aData, void* aObject)
{
  RuleArrayArray* rulesArray = static_cast<RuleArrayArray*>(aData);
  RuleArray* oldRules = static_cast<RuleArray*>(aObject);

  if (!rulesArray) {
    rulesArray = new RuleArrayArray();
  }

  RuleArray* newRules = rulesArray->AppendElement();
  newRules->SwapElements(*oldRules);

  return rulesArray;
}



static bool
DeferredFinalizeRulesArray(uint32_t aSliceBudget, void* aData)
{
  MOZ_ASSERT(aSliceBudget > 0, "nonsensical/useless call with aSliceBudget == 0");
  RuleArrayArray* rulesArray = static_cast<RuleArrayArray*>(aData);

  size_t newOuterLen = rulesArray->Length();

  while (aSliceBudget > 0 && newOuterLen > 0) {
    RuleArray& lastArray = rulesArray->ElementAt(newOuterLen - 1);
    uint32_t innerLen = lastArray.Length();
    uint32_t currSlice = std::min(innerLen, aSliceBudget);
    uint32_t newInnerLen = innerLen - currSlice;
    lastArray.TruncateLength(newInnerLen);
    aSliceBudget -= currSlice;
    if (newInnerLen == 0) {
      newOuterLen -= 1;
    }
  }

  rulesArray->TruncateLength(newOuterLen);

  if (newOuterLen == 0) {
    delete rulesArray;
    return true;
  }
  return false;
}

void
IncrementalClearCOMRuleArray::Clear()
{
  
  
  if (Length() > 10 && nsCCUncollectableMarker::sGeneration) {
    DeferredFinalize(AppendRulesArrayPointer, DeferredFinalizeRulesArray, this);
  } else {
    nsCOMArray<css::Rule>::Clear();
  }
  MOZ_ASSERT(Length() == 0);
}
