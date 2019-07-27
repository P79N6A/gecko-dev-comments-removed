






#ifndef MOZILLA_A11Y_ProxyWrappers_h
#define MOZILLA_A11Y_ProxyWrappers_h

#include "HyperTextAccessible.h"

namespace mozilla {
namespace a11y {

class ProxyAccessibleWrap : public AccessibleWrap
{
  public:
  ProxyAccessibleWrap(ProxyAccessible* aProxy) :
    AccessibleWrap(nullptr, nullptr)
  {
    mType = eProxyType;
    mBits.proxy = aProxy;
  }

  virtual void Shutdown() override
  {
    mBits.proxy = nullptr;
  }
};

class HyperTextProxyAccessibleWrap : public ProxyAccessibleWrap,
                                     public ia2AccessibleEditableText,
                                     public ia2AccessibleHypertext
{
  HyperTextProxyAccessibleWrap(ProxyAccessible* aProxy) :
    ProxyAccessibleWrap(aProxy) {}
};

template<typename T>
inline ProxyAccessible*
HyperTextProxyFor(T* aWrapper)
{
  static_assert(mozilla::IsBaseOf<IUnknown, T>::value, "only IAccessible* should be passed in");
  auto wrapper = static_cast<HyperTextProxyAccessibleWrap*>(aWrapper);
  return wrapper->IsProxy() ? wrapper->Proxy() : nullptr;
}

}
}

#endif
