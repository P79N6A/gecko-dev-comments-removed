





#ifndef nsFakeSynthServices_h
#define nsFakeSynthServices_h

#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIObserver.h"
#include "nsIThread.h"
#include "nsISpeechService.h"
#include "nsRefPtrHashtable.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/Monitor.h"

namespace mozilla {
namespace dom {

class nsFakeSynthServices : public nsIObserver
{

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsFakeSynthServices();

  static nsFakeSynthServices* GetInstance();

  static already_AddRefed<nsFakeSynthServices> GetInstanceForService();

  static void Shutdown();

private:

  virtual ~nsFakeSynthServices();

  void Init();

  nsCOMPtr<nsISpeechService> mDirectService;

  nsCOMPtr<nsISpeechService> mIndirectService;

  static StaticRefPtr<nsFakeSynthServices> sSingleton;
};

} 
} 

#endif
