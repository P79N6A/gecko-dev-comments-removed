







































#include "CAccessibleHyperlink.h"

#include "Accessible2.h"
#include "AccessibleHyperlink.h"
#include "AccessibleHyperlink_i.c"

#include "nsIAccessible.h"
#include "nsIAccessibleHyperlink.h"
#include "nsIWinAccessNode.h"
#include "nsAccessNodeWrap.h"

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
      return E_NOINTERFACE;

    *ppv = static_cast<IAccessibleHyperlink*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return CAccessibleAction::QueryInterface(iid, ppv);
}



STDMETHODIMP
CAccessibleHyperlink::get_anchor(long aIndex, VARIANT *aAnchor)
{
__try {
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

  IUnknown *unknownPtr = static_cast<IUnknown*>(instancePtr);
  aAnchor->ppunkVal = &unknownPtr;
  aAnchor->vt = VT_UNKNOWN;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleHyperlink::get_anchorTarget(long aIndex, VARIANT *aAnchorTarget)
{
__try {
  VariantInit(aAnchorTarget);

  nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = acc->GetURI(aIndex, getter_AddRefs(uri));
  if (NS_FAILED(rv) || !uri)
    return E_FAIL;

  nsCAutoString prePath;
  rv = uri->GetPrePath(prePath);
  if (NS_FAILED(rv))
    return E_FAIL;

  nsCAutoString path;
  rv = uri->GetPath(path);
  if (NS_FAILED(rv))
    return E_FAIL;

  nsAutoString stringURI;
  AppendUTF8toUTF16(prePath, stringURI);
  AppendUTF8toUTF16(path, stringURI);

  aAnchorTarget->vt = VT_BSTR;
  aAnchorTarget->bstrVal = ::SysAllocStringLen(stringURI.get(),
                                               stringURI.Length());
  if (!aAnchorTarget->bstrVal)
    return E_OUTOFMEMORY;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP
CAccessibleHyperlink::get_startIndex(long *aIndex)
{
__try {
  *aIndex = 0;

  nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRInt32 index = 0;
  nsresult rv = acc->GetStartIndex(&index);
  *aIndex = index;
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleHyperlink::get_endIndex(long *aIndex)
{
__try {
  *aIndex = 0;

  nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRInt32 index = 0;
  nsresult rv = acc->GetEndIndex(&index);
  *aIndex = index;

  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleHyperlink::get_valid(boolean *aValid)
{
__try {
  nsCOMPtr<nsIAccessibleHyperLink> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  PRBool isValid = PR_FALSE;
  nsresult rv = acc->IsValid(&isValid);
  *aValid = isValid;
  if (NS_SUCCEEDED(rv))
    return S_OK;
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

