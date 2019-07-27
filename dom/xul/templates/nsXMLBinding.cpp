




#include "nsXULTemplateQueryProcessorXML.h"
#include "nsXULTemplateResultXML.h"
#include "nsXMLBinding.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/XPathResult.h"

using namespace mozilla;
using namespace mozilla::dom;

nsXMLBindingSet::~nsXMLBindingSet()
{}

void
nsXMLBindingSet::AddBinding(nsIAtom* aVar, nsAutoPtr<XPathExpression>&& aExpr)
{
  nsAutoPtr<nsXMLBinding> newbinding(new nsXMLBinding(aVar, Move(aExpr)));

  if (mFirst) {
    nsXMLBinding* binding = mFirst;

    while (binding) {
      
      
      if (binding->mVar == aVar)
        return;

      
      if (!binding->mNext) {
        binding->mNext = newbinding;
        return;
      }

      binding = binding->mNext;
    }
  }
  else {
    mFirst = newbinding;
  }
}

int32_t
nsXMLBindingSet::LookupTargetIndex(nsIAtom* aTargetVariable,
                                   nsXMLBinding** aBinding)
{
  int32_t idx = 0;
  nsXMLBinding* binding = mFirst;

  while (binding) {
    if (binding->mVar == aTargetVariable) {
      *aBinding = binding;
      return idx;
    }
    idx++;
    binding = binding->mNext;
  }

  *aBinding = nullptr;
  return -1;
}

XPathResult*
nsXMLBindingValues::GetAssignmentFor(nsXULTemplateResultXML* aResult,
                                     nsXMLBinding* aBinding,
                                     int32_t aIndex,
                                     uint16_t aType)
{
  XPathResult* value = mValues.SafeElementAt(aIndex);
  if (value) {
    return value;
  }

  nsINode* contextNode = aResult->Node();
  if (!contextNode) {
    return nullptr;
  }

  mValues.EnsureLengthAtLeast(aIndex + 1);

  ErrorResult ignored;
  mValues[aIndex] = aBinding->mExpr->Evaluate(*contextNode, aType, nullptr,
                                              ignored);

  return mValues[aIndex];
}

nsINode*
nsXMLBindingValues::GetNodeAssignmentFor(nsXULTemplateResultXML* aResult,
                                         nsXMLBinding* aBinding,
                                         int32_t aIndex)
{
  XPathResult* result = GetAssignmentFor(aResult, aBinding, aIndex,
                                         XPathResult::FIRST_ORDERED_NODE_TYPE);

  ErrorResult rv;
  return result ? result->GetSingleNodeValue(rv) : nullptr;
}

void
nsXMLBindingValues::GetStringAssignmentFor(nsXULTemplateResultXML* aResult,
                                           nsXMLBinding* aBinding,
                                           int32_t aIndex,
                                           nsAString& aValue)
{
  XPathResult* result = GetAssignmentFor(aResult, aBinding, aIndex,
                                         XPathResult::STRING_TYPE);

  if (result) {
    ErrorResult rv;
    result->GetStringValue(aValue, rv);
  } else {
    aValue.Truncate();
  }
}
