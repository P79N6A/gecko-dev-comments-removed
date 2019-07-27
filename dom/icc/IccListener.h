



#ifndef mozilla_dom_IccListener_h
#define mozilla_dom_IccListener_h

#include "nsAutoPtr.h"
#include "nsIIccProvider.h"

namespace mozilla {
namespace dom {

class IccManager;
class Icc;

class IccListener MOZ_FINAL : public nsIIccListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIICCLISTENER

  IccListener(IccManager* aIccManager, uint32_t aClientId);

  void
  Shutdown();

  Icc*
  GetIcc()
  {
    return mIcc;
  }

private:
  ~IccListener();

private:
  uint32_t mClientId;
  
  
  
  
  nsRefPtr<Icc> mIcc;
  nsRefPtr<IccManager> mIccManager;
  
  
  nsCOMPtr<nsIIccProvider> mProvider;
};

} 
} 

#endif 
