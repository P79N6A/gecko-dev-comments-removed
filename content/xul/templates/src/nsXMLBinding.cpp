



































#include "nsXULTemplateQueryProcessorXML.h"
#include "nsXULTemplateResultXML.h"
#include "nsXMLBinding.h"

NS_IMPL_ADDREF(nsXMLBindingSet)
NS_IMPL_RELEASE(nsXMLBindingSet)

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

PRInt32
nsXMLBindingSet::LookupTargetIndex(nsIAtom* aTargetVariable,
                                   nsXMLBinding** aBinding)
{
  PRInt32 idx = 0;
  nsXMLBinding* binding = mFirst;

  while (binding) {
    if (binding->mVar == aTargetVariable) {
      *aBinding = binding;
      return idx;
    }
    idx++;
    binding = binding->mNext;
  }

  *aBinding = nsnull;
  return -1;
}

void
nsXMLBindingValues::GetAssignmentFor(nsXULTemplateResultXML* aResult,
                                     nsXMLBinding* aBinding,
                                     PRInt32 aIndex,
                                     PRUint16 aType,
                                     nsIDOMXPathResult** aValue)
{
  *aValue = mValues.SafeObjectAt(aIndex);

  if (!*aValue) {
    nsCOMPtr<nsIDOMNode> contextNode;
    aResult->GetNode(getter_AddRefs(contextNode));
    if (contextNode) {
      nsCOMPtr<nsISupports> resultsupports;
      aBinding->mExpr->Evaluate(contextNode, aType,
                                nsnull, getter_AddRefs(resultsupports));

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
                                         PRInt32 aIndex,
                                         nsIDOMNode** aNode)
{
  nsCOMPtr<nsIDOMXPathResult> result;
  GetAssignmentFor(aResult, aBinding, aIndex,
                   nsIDOMXPathResult::FIRST_ORDERED_NODE_TYPE,
                   getter_AddRefs(result));

  if (result)
    result->GetSingleNodeValue(aNode);
  else
    *aNode = nsnull;
}

void
nsXMLBindingValues::GetStringAssignmentFor(nsXULTemplateResultXML* aResult,
                                           nsXMLBinding* aBinding,
                                           PRInt32 aIndex,
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
