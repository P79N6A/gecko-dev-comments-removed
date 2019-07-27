


#ifndef RemoteSpellcheckEngineParent_h_
#define RemoteSpellcheckEngineParent_h_

#include "mozISpellCheckingEngine.h"
#include "mozilla/PRemoteSpellcheckEngineParent.h"
#include "nsCOMPtr.h"

namespace mozilla {

class RemoteSpellcheckEngineParent : public mozilla::PRemoteSpellcheckEngineParent {

public:
  RemoteSpellcheckEngineParent();

  ~RemoteSpellcheckEngineParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy);

  bool RecvSetDictionary(const nsString& aDictionary, bool* success);

  bool RecvCheckForMisspelling( const nsString& aWord, bool* isMisspelled);

private:
  nsCOMPtr<mozISpellCheckingEngine> mEngine;
};

}
#endif
