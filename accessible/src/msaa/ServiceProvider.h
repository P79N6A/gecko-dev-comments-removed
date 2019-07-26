





#ifndef mozilla_a11y_ServiceProvider_h_
#define mozilla_a11y_ServiceProvider_h_

#include <servprov.h>

#include "AccessibleWrap.h"

namespace mozilla {
namespace a11y {

class ServiceProvider MOZ_FINAL : public IServiceProvider
{
public:
  ServiceProvider(AccessibleWrap* aAcc) : mRefCnt(0), mAccessible(aAcc) {}
  ~ServiceProvider() {}

  DECL_IUNKNOWN

  
  virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID aGuidService,
                                                 REFIID aIID,
                                                 void** aInstancePtr);

private:
  nsRefPtr<AccessibleWrap> mAccessible;
};

}
}

#endif
