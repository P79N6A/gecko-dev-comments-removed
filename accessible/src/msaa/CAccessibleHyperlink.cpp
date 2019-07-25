







































#include "CAccessibleHyperlink.h"

#include "Accessible2.h"
#include "AccessibleHyperlink.h"
#include "AccessibleHyperlink_i.c"

#include "nsAccessible.h"
#include "nsIWinAccessNode.h"



STDMETHODIMP
CAccessibleHyperlink::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleHyperlink == iid) {
    nsRefPtr<nsAccessible> thisObj = do_QueryObject(this);
    if (!thisObj->IsHyperLink())
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

  nsRefPtr<nsAccessible> thisObj = do_QueryObject(this);
  if (thisObj->IsDefunct() || !thisObj->IsHyperLink())
    return E_FAIL;

  if (aIndex < 0 || aIndex >= static_cast<long>(thisObj->AnchorCount()))
    return E_INVALIDARG;

  nsAccessible* anchor = thisObj->GetAnchor(aIndex);
  if (!anchor)
    return S_FALSE;

  nsCOMPtr<nsIWinAccessNode> winAccessNode(do_QueryObject(anchor));
  if (!winAccessNode)
    return E_FAIL;

  void *instancePtr = NULL;
  nsresult rv = winAccessNode->QueryNativeInterface(IID_IUnknown, &instancePtr);
  if (NS_FAILED(rv))
    return E_FAIL;

  IUnknown *unknownPtr = static_cast<IUnknown*>(instancePtr);
  aAnchor->ppunkVal = &unknownPtr;
  aAnchor->vt = VT_UNKNOWN;
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleHyperlink::get_anchorTarget(long aIndex, VARIANT *aAnchorTarget)
{
__try {
  VariantInit(aAnchorTarget);

  nsRefPtr<nsAccessible> thisObj = do_QueryObject(this);
  if (thisObj->IsDefunct() || !thisObj->IsHyperLink())
    return E_FAIL;

  if (aIndex < 0 || aIndex >= static_cast<long>(thisObj->AnchorCount()))
    return E_INVALIDARG;

  nsCOMPtr<nsIURI> uri = thisObj->GetAnchorURI(aIndex);
  if (!uri)
    return S_FALSE;

  nsCAutoString prePath;
  nsresult rv = uri->GetPrePath(prePath);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  nsCAutoString path;
  rv = uri->GetPath(path);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  nsAutoString stringURI;
  AppendUTF8toUTF16(prePath, stringURI);
  AppendUTF8toUTF16(path, stringURI);

  aAnchorTarget->vt = VT_BSTR;
  aAnchorTarget->bstrVal = ::SysAllocStringLen(stringURI.get(),
                                               stringURI.Length());
  return aAnchorTarget->bstrVal ? S_OK : E_OUTOFMEMORY;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleHyperlink::get_startIndex(long *aIndex)
{
__try {
  *aIndex = 0;

  nsRefPtr<nsAccessible> thisObj = do_QueryObject(this);
  if (thisObj->IsDefunct() || !thisObj->IsHyperLink())
    return E_FAIL;

  *aIndex = thisObj->StartOffset();
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleHyperlink::get_endIndex(long *aIndex)
{
__try {
  *aIndex = 0;

  nsRefPtr<nsAccessible> thisObj = do_QueryObject(this);
  if (thisObj->IsDefunct() || !thisObj->IsHyperLink())
    return E_FAIL;

  *aIndex = thisObj->EndOffset();
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleHyperlink::get_valid(boolean *aValid)
{
__try {
  *aValid = false;

  nsRefPtr<nsAccessible> thisObj = do_QueryObject(this);
  if (thisObj->IsDefunct() || !thisObj->IsHyperLink())
    return E_FAIL;

  *aValid = thisObj->IsLinkValid();
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

