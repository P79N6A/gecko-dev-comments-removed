









#ifndef mozilla_RestyleManager_h
#define mozilla_RestyleManager_h

#include "nsISupportsImpl.h"

class nsPresContext;

namespace mozilla {

class RestyleManager {
public:
  RestyleManager(nsPresContext* aPresContext);

  NS_INLINE_DECL_REFCOUNTING(mozilla::RestyleManager)

  void Disconnect() {
    mPresContext = nullptr;
  }

  nsPresContext* PresContext() const {
    MOZ_ASSERT(mPresContext);
    return mPresContext;
  }

private:
  nsPresContext* mPresContext; 
};

} 

#endif 
