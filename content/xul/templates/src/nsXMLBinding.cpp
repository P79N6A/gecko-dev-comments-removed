




#include "nsXULTemplateQueryProcessorXML.h"
#include "nsXULTemplateResultXML.h"
#include "nsXMLBinding.h"

NS_IMPL_CYCLE_COLLECTING_NATIVE_ADDREF(nsXMLBindingSet)
NS_IMPL_CYCLE_COLLECTING_NATIVE_RELEASE(nsXMLBindingSet)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsXMLBindingSet)
  nsXMLBinding* binding = tmp->mFirst;
  while (binding) {
    binding->mExpr = nullptr;
    binding = binding->mNext;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsXMLBindingSet)
  nsXMLBinding* binding = tmp->mFirst;
  while (binding) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "nsXMLBinding::mExpr"); 
    cb.NoteXPCOMChild(binding->mExpr);
    binding = binding->mNext;
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(nsXMLBindingSet, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(nsXMLBindingSet, Release)

nsresult
nsXMLBindingSet::AddBinding(nsIAtom* aVar, nsIDOMXPathExpression* aExpr)
{
  nsAutoPtr<nsXMLBinding> newbinding(new nsXMLBinding(aVar, aExpr));
  NS_ENSURE_TRUE(newbinding, NS_ERROR_OUT_OF_MEMORY);

  if (mFirst) {
    nsXMLBinding* binding = mFirst;

    while (binding) {
      
      
      if (binding->mVar == aVar)
        return NS_OK;

      
      if (!binding->mNext) {
        binding->mNext = newbinding;
        break;
      }

      binding = binding->mNext;
    }
  }
  else {
    mFirst = newbinding;
  }

  return NS_OK;
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

void
nsXMLBindingValues::GetAssignmentFor(nsXULTemplateResultXML* aResult,
                                     nsXMLBinding* aBinding,
                                     int32_t aIndex,
                                     uint16_t aType,
                                     nsIDOMXPathResult** aValue)
{
  *aValue = mValues.SafeObjectAt(aIndex);

  if (!*aValue) {
    nsCOMPtr<nsIDOMNode> contextNode;
    aResult->GetNode(getter_AddRefs(contextNode));
    if (contextNode) {
      nsCOMPtr<nsISupports> resultsupports;
      aBinding->mExpr->Evaluate(contextNode, aType,
                                nullptr, getter_AddRefs(resultsupports));

      nsCOMPtr<nsIDOMXPathResult> result = do_QueryInterface(resultsupports);
      if (result && mValues.ReplaceObjectAt(result, aIndex))
        *aValue = result;
    }
  }

  NS_IF_ADDREF(*aValue);
}

void
nsXMLBindingValues::GetNodeAssignmentFor(nsXULTemplateResultXML* aResult,
                                         nsXMLBinding* aBinding,
                                         int32_t aIndex,
                                         nsIDOMNode** aNode)
{
  nsCOMPtr<nsIDOMXPathResult> result;
  GetAssignmentFor(aResult, aBinding, aIndex,
                   nsIDOMXPathResult::FIRST_ORDERED_NODE_TYPE,
                   getter_AddRefs(result));

  if (result)
    result->GetSingleNodeValue(aNode);
  else
    *aNode = nullptr;
}

void
nsXMLBindingValues::GetStringAssignmentFor(nsXULTemplateResultXML* aResult,
                                           nsXMLBinding* aBinding,
                                           int32_t aIndex,
                                           nsAString& aValue)
{
  nsCOMPtr<nsIDOMXPathResult> result;
  GetAssignmentFor(aResult, aBinding, aIndex,
                   nsIDOMXPathResult::STRING_TYPE, getter_AddRefs(result));

  if (result)
    result->GetStringValue(aValue);
  else
    aValue.Truncate();
}
