





#ifndef mozilla_dom_FakeTVService_h
#define mozilla_dom_FakeTVService_h

#include "nsCOMPtr.h"
#include "nsITVService.h"

#define FAKE_TV_SERVICE_CONTRACTID \
  "@mozilla.org/tv/faketvservice;1"
#define FAKE_TV_SERVICE_CID \
  { 0x60fb3c53, 0x017f, 0x4340, { 0x91, 0x1b, 0xd5, 0x5c, 0x31, 0x28, 0x88, 0xb6 } }

namespace mozilla {
namespace dom {

class FakeTVService MOZ_FINAL : public nsITVService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITVSERVICE

  FakeTVService() {}

  

private:
  ~FakeTVService() {}

  nsCOMPtr<nsITVSourceListener> mSourceListener;
};

} 
} 

#endif 
