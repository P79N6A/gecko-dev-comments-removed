






#include "ia2AccessibleComponent.h"

#include "AccessibleComponent_i.c"

#include "AccessibleWrap.h"
#include "States.h"
#include "IUnknownImpl.h"

#include "nsIFrame.h"

using namespace mozilla::a11y;



STDMETHODIMP
ia2AccessibleComponent::QueryInterface(REFIID iid, void** ppv)
{
  if (!ppv)
    return E_INVALIDARG;

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

  if (!aX || !aY)
    return E_INVALIDARG;

  *aX = 0;
  *aY = 0;

  AccessibleWrap* acc = static_cast<AccessibleWrap*>(this);
  if (acc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  
  uint64_t state = acc->State();
  if (state & states::INVISIBLE)
    return S_OK;

  nsIntRect rect = acc->Bounds();

  
  
  
  if (!acc->Parent()) {
    *aX = rect.x;
    *aY = rect.y;
    return S_OK;
  }

  
  
  nsIntRect parentRect = acc->Parent()->Bounds();
  *aX = rect.x - parentRect.x;
  *aY = rect.y - parentRect.y;
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
ia2AccessibleComponent::get_foreground(IA2Color* aForeground)
{
  A11Y_TRYBLOCK_BEGIN

  if (!aForeground)
    return E_INVALIDARG;

  *aForeground = 0;

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

  if (!aBackground)
    return E_INVALIDARG;

  *aBackground = 0;

  AccessibleWrap* acc = static_cast<AccessibleWrap*>(this);
  if (acc->IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsIFrame* frame = acc->GetFrame();
  if (frame)
    *aBackground = frame->StyleBackground()->mBackgroundColor;

  return S_OK;

  A11Y_TRYBLOCK_END
}

