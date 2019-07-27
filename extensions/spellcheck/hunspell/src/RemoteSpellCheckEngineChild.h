



#ifndef RemoteSpellcheckEngineChild_h_
#define RemoteSpellcheckEngineChild_h_

#include "mozilla/PRemoteSpellcheckEngineChild.h"
#include "mozSpellChecker.h"

class mozSpellChecker;

namespace mozilla {
class RemoteSpellcheckEngineChild : public mozilla::PRemoteSpellcheckEngineChild
{
public:
  explicit RemoteSpellcheckEngineChild(mozSpellChecker *aOwner);
  ~RemoteSpellcheckEngineChild();

private:
  mozSpellChecker *mOwner;
};

} 

#endif 
