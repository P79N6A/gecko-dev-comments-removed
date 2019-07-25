






#include "ia2AccessibleComponent.h"

#include "AccessibleComponent_i.c"

#include "AccessibleWrap.h"
#include "States.h"

#include "nsIFrame.h"

using namespace mozilla::a11y;



STDMETHODIMP
ia2AccessibleComponent::QueryInterface(REFIID iid, void** ppv)
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
ia2AccessibleComponent::get_locationInParent(long* aX, long* aY)
{
__try {
  *aX = 0;
  *aY = 0;

  AccessibleWrap* acc = static_cast<AccessibleWrap*>(this);
  if (acc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  
  uint64_t state = acc->State();
  if (state & states::INVISIBLE)
    return S_OK;

  int32_t x = 0, y = 0, width = 0, height = 0;
  nsresult rv = acc->GetBounds(&x, &y, &width, &height);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  Accessible* parentAcc = acc->Parent();

  
  
  
  if (!parentAcc) {
    *aX = x;
    *aY = y;
    return S_OK;
  }

  
  
  int32_t parentx = 0, parenty = 0;
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
ia2AccessibleComponent::get_foreground(IA2Color* aForeground)
{
__try {
  AccessibleWrap* acc = static_cast<AccessibleWrap*>(this);
  if (acc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsIFrame* frame = acc->GetFrame();
  if (frame)
    *aForeground = frame->GetStyleColor()->mColor;

  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
ia2AccessibleComponent::get_background(IA2Color* aBackground)
{
__try {
  AccessibleWrap* acc = static_cast<AccessibleWrap*>(this);
  if (acc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsIFrame* frame = acc->GetFrame();
  if (frame)
    *aBackground = frame->GetStyleBackground()->mBackgroundColor;

  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

