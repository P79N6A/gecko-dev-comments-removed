







































#include "CAccessibleComponent.h"

#include "AccessibleComponent_i.c"

#include "nsAccessible.h"
#include "nsCoreUtils.h"
#include "nsWinUtils.h"
#include "States.h"

#include "nsString.h"

#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMNSRGBAColor.h"

using namespace mozilla::a11y;



STDMETHODIMP
CAccessibleComponent::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleComponent == iid) {
    *ppv = static_cast<IAccessibleComponent*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleComponent::get_locationInParent(long *aX, long *aY)
{
__try {
  *aX = 0;
  *aY = 0;

  nsRefPtr<nsAccessible> acc(do_QueryObject(this));
  if (!acc)
    return E_FAIL;

  
  PRUint64 state = acc->State();
  if (state & states::INVISIBLE)
    return S_OK;

  PRInt32 x = 0, y = 0, width = 0, height = 0;
  nsresult rv = acc->GetBounds(&x, &y, &width, &height);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  nsCOMPtr<nsIAccessible> parentAcc;
  rv = acc->GetParent(getter_AddRefs(parentAcc));
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  
  
  
  if (!parentAcc) {
    *aX = x;
    *aY = y;
    return S_OK;
  }

  
  
  PRInt32 parentx = 0, parenty = 0;
  rv = acc->GetBounds(&parentx, &parenty, &width, &height);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  *aX = x - parentx;
  *aY = y - parenty;
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleComponent::get_foreground(IA2Color* aForeground)
{
__try {
  nsRefPtr<nsAccessible> acc(do_QueryObject(this));
  if (acc->IsDefunct())
    return E_FAIL;

  nsIFrame* frame = acc->GetFrame();
  if (frame)
    *aForeground = frame->GetStyleColor()->mColor;

  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleComponent::get_background(IA2Color* aBackground)
{
__try {
  nsRefPtr<nsAccessible> acc(do_QueryObject(this));
  if (acc->IsDefunct())
    return E_FAIL;

  nsIFrame* frame = acc->GetFrame();
  if (frame)
    *aBackground = frame->GetStyleBackground()->mBackgroundColor;

  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

