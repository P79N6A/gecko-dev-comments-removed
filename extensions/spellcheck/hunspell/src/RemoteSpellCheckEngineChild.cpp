



#include "RemoteSpellCheckEngineChild.h"

namespace mozilla {
RemoteSpellcheckEngineChild::RemoteSpellcheckEngineChild(mozSpellChecker *aOwner)
  :mOwner(aOwner)
{
}

RemoteSpellcheckEngineChild::~RemoteSpellcheckEngineChild()
{
  
  
  mOwner->DeleteRemoteEngine();

}

} 
