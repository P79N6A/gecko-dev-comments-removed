







































#include "nsAccessibleRelationWrap.h"

#include "AccessibleRelation_i.c"

#include "nsArrayUtils.h"

nsAccessibleRelationWrap::
  nsAccessibleRelationWrap(PRUint32 aType, nsIAccessible *aTarget) :
  nsAccessibleRelation(aType, aTarget)
{
}



NS_IMPL_ISUPPORTS_INHERITED1(nsAccessibleRelationWrap, nsAccessibleRelation,
                             nsIWinAccessNode)



NS_IMETHODIMP
nsAccessibleRelationWrap::QueryNativeInterface(REFIID aIID, void** aInstancePtr)
{
  return QueryInterface(aIID, aInstancePtr);
}



STDMETHODIMP
nsAccessibleRelationWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleRelation == iid || IID_IUnknown == iid) {
    *ppv = NS_STATIC_CAST(IAccessibleRelation*, this);
    (NS_REINTERPRET_CAST(IUnknown*, *ppv))->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
nsAccessibleRelationWrap::get_relationType(BSTR *aRelationType)
{
  *aRelationType = NULL;

  PRUint32 type = 0;
  nsresult rv = GetRelationType(&type);
  if (NS_FAILED(rv))
    return E_FAIL;

  switch (type) {
    case RELATION_CONTROLLED_BY:
      *aRelationType = ::SysAllocString(IA2_RELATION_CONTROLLED_BY);
      break;
    case RELATION_CONTROLLER_FOR:
      *aRelationType = ::SysAllocString(IA2_RELATION_CONTROLLER_FOR);
      break;
    case RELATION_DESCRIBED_BY:
      *aRelationType = ::SysAllocString(IA2_RELATION_DESCRIBED_BY);
      break;
    case RELATION_DESCRIPTION_FOR:
      *aRelationType = ::SysAllocString(IA2_RELATION_DESCRIPTION_FOR);
      break;
    case RELATION_EMBEDDED_BY:
      *aRelationType = ::SysAllocString(IA2_RELATION_EMBEDDED_BY);
      break;
    case RELATION_EMBEDS:
      *aRelationType = ::SysAllocString(IA2_RELATION_EMBEDS);
      break;
    case RELATION_FLOWS_FROM:
      *aRelationType = ::SysAllocString(IA2_RELATION_FLOWS_FROM);
      break;
    case RELATION_FLOWS_TO:
      *aRelationType = ::SysAllocString(IA2_RELATION_FLOWS_TO);
      break;
    case RELATION_LABEL_FOR:
      *aRelationType = ::SysAllocString(IA2_RELATION_LABEL_FOR);
      break;
    case RELATION_LABELLED_BY:
      *aRelationType = ::SysAllocString(IA2_RELATION_LABELED_BY);
      break;
    case RELATION_MEMBER_OF:
      *aRelationType = ::SysAllocString(IA2_RELATION_MEMBER_OF);
      break;
    case RELATION_NODE_CHILD_OF:
      *aRelationType = ::SysAllocString(IA2_RELATION_NODE_CHILD_OF);
      break;
    case RELATION_PARENT_WINDOW_OF:
      *aRelationType = ::SysAllocString(IA2_RELATION_PARENT_WINDOW_OF);
      break;
    case RELATION_POPUP_FOR:
      *aRelationType = ::SysAllocString(IA2_RELATION_POPUP_FOR);
      break;
    case RELATION_SUBWINDOW_OF:
      *aRelationType = ::SysAllocString(IA2_RELATION_SUBWINDOW_OF);
      break;
    default:
      return E_FAIL;
  }

  return !aRelationType ? E_OUTOFMEMORY : S_OK;
}

STDMETHODIMP
nsAccessibleRelationWrap::get_localizedRelationType(BSTR *aLocalizedRelationType)
{
  return E_NOTIMPL;
}

STDMETHODIMP
nsAccessibleRelationWrap::get_nTargets(long *aNTargets)
{
  PRUint32 count = 0;
  nsresult rv = GetTargetsCount(&count);
  *aNTargets = count;

  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
nsAccessibleRelationWrap::get_target(long aTargetIndex, IUnknown **aTarget)
{
  nsCOMPtr<nsIAccessible> accessible;
  nsresult rv = GetTarget(aTargetIndex, getter_AddRefs(accessible));

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(accessible));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  rv = winAccessNode->QueryNativeInterface(IID_IUnknown, &instancePtr);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aTarget = NS_STATIC_CAST(IUnknown*, instancePtr);
  return S_OK;
}

STDMETHODIMP
nsAccessibleRelationWrap::get_targets(long aMaxTargets, IUnknown **aTarget,
                                      long *aNTargets)
{
  *aNTargets = 0;

  nsCOMPtr<nsIArray> targets;
  nsresult rv = GetTargets(getter_AddRefs(targets));
  if (NS_FAILED(rv))
    return E_FAIL;

  PRUint32 length = 0;
  rv = targets->GetLength(&length);
  if (NS_FAILED(rv))
    return E_FAIL;

  PRUint32 count = length < PRUint32(aMaxTargets) ? length : aMaxTargets;

  PRUint32 index = 0;
  for (; index < count; index++) {
    nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryElementAt(targets, index, &rv));
    if (NS_FAILED(rv) || !winAccessNode)
      break;

    void *instancePtr = NULL;
    nsresult rv =  winAccessNode->QueryNativeInterface(IID_IUnknown,
                                                       &instancePtr);
    if (NS_FAILED(rv))
      break;

    aTarget[index] = NS_STATIC_CAST(IUnknown*, instancePtr);
  }

  if (NS_FAILED(rv)) {
    for (PRUint32 index2 = 0; index2 < index; index2++) {
      aTarget[index2]->Release();
      aTarget[index2] = NULL;
    }
    return E_FAIL;
  }

  *aNTargets = count;
  return S_OK;
}

