







































#include "CAccessibleHyperlink.h"

#include "Accessible2.h"
#include "AccessibleHyperlink.h"
#include "AccessibleHyperlink_i.c"

#include "nsIAccessible.h"
#include "nsIAccessibleHyperlink.h"
#include "nsIWinAccessNode.h"

#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsIURI.h"



STDMETHODIMP
CAccessibleHyperlink::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleHyperlink == iid) {
    nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
    if (!acc)
      return E_FAIL;

    *ppv = NS_STATIC_CAST(IAccessibleHyperlink*, this);
    (NS_REINTERPRET_CAST(IUnknown*, *ppv))->AddRef();
    return S_OK;
  }

  return CAccessibleAction::QueryInterface(iid, ppv);
}



STDMETHODIMP
CAccessibleHyperlink::get_anchor(long aIndex, VARIANT *aAnchor)
{
  VariantInit(aAnchor);

  nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsCOMPtr<nsIAccessible> anchor;
  acc->GetObject(aIndex, getter_AddRefs(anchor));
  if (!anchor)
    return E_FAIL;

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryInterface(anchor));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  nsresult rv =  winAccessNode->QueryNativeInterface(IID_IUnknown,
                                                     &instancePtr);
  if (NS_FAILED(rv))
    return E_FAIL;

  IUnknown *unknownPtr = NS_STATIC_CAST(IUnknown*, instancePtr);
  aAnchor->ppunkVal = &unknownPtr;
  aAnchor->vt = VT_UNKNOWN;

  return S_OK;
}

STDMETHODIMP
CAccessibleHyperlink::get_anchorTarget(long aIndex, VARIANT *aAnchorTarget)
{
  VariantInit(aAnchorTarget);

  nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = acc->GetURI(aIndex, getter_AddRefs(uri));
  if (NS_SUCCEEDED(rv))
    return E_FAIL;

  nsCAutoString prePath;
  rv = uri->GetPrePath(prePath);
  if (NS_SUCCEEDED(rv))
    return E_FAIL;

  nsCAutoString path;
  rv = uri->GetPath(path);
  if (NS_SUCCEEDED(rv))
    return E_FAIL;

  nsAutoString stringURI;
  AppendUTF8toUTF16(prePath, stringURI);
  AppendUTF8toUTF16(path, stringURI);

  aAnchorTarget->vt = VT_BSTR;
  return ::SysReAllocStringLen(&aAnchorTarget->bstrVal, stringURI.get(),
                               stringURI.Length());
}

STDMETHODIMP
CAccessibleHyperlink::get_startIndex(long *aIndex)
{
  *aIndex = 0;

  nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRInt32 index = 0;
  nsresult rv = acc->GetStartIndex(&index);
  *aIndex = index;

  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleHyperlink::get_endIndex(long *aIndex)
{
  *aIndex = 0;

  nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRInt32 index = 0;
  nsresult rv = acc->GetEndIndex(&index);
  *aIndex = index;

  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

STDMETHODIMP
CAccessibleHyperlink::get_valid(boolean *aValid)
{
  nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRBool isValid = PR_FALSE;
  nsresult rv = acc->IsValid(&isValid);
  *aValid = isValid;

  return NS_FAILED(rv) ? E_FAIL : S_OK;
}

