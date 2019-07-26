






#include "ia2AccessibleComponent.h"

#include "AccessibleComponent_i.c"

#include "AccessibleWrap.h"
#include "States.h"

#include "nsIFrame.h"

using namespace mozilla::a11y;



STDMETHODIMP
ia2AccessibleComponent::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = nullptr;

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
  A11Y_TRYBLOCK_BEGIN

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

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleComponent::get_foreground(IA2Color* aForeground)
{
  A11Y_TRYBLOCK_BEGIN

  AccessibleWrap* acc = static_cast<AccessibleWrap*>(this);
  if (acc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsIFrame* frame = acc->GetFrame();
  if (frame)
    *aForeground = frame->StyleColor()->mColor;

  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleComponent::get_background(IA2Color* aBackground)
{
  A11Y_TRYBLOCK_BEGIN

  AccessibleWrap* acc = static_cast<AccessibleWrap*>(this);
  if (acc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsIFrame* frame = acc->GetFrame();
  if (frame)
    *aBackground = frame->StyleBackground()->mBackgroundColor;

  return S_OK;

  A11Y_TRYBLOCK_END
}

