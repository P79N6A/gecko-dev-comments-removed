



#include "RemoteSpellCheckEngineParent.h"
#include "mozISpellCheckingEngine.h"
#include "nsServiceManagerUtils.h"

#define DEFAULT_SPELL_CHECKER "@mozilla.org/spellchecker/engine;1"

namespace mozilla {

RemoteSpellcheckEngineParent::RemoteSpellcheckEngineParent()
{
  mEngine = do_GetService(DEFAULT_SPELL_CHECKER);
}

RemoteSpellcheckEngineParent::~RemoteSpellcheckEngineParent()
{
}

bool
RemoteSpellcheckEngineParent::RecvSetDictionary(
  const nsString& aDictionary,
  bool* success)
{
  nsresult rv = mEngine->SetDictionary(aDictionary.get());
  *success = NS_SUCCEEDED(rv);
  return true;
}

bool
RemoteSpellcheckEngineParent::RecvCheckForMisspelling(
  const nsString& aWord,
  bool* isMisspelled)
{
  bool isCorrect = false;
  mEngine->Check(aWord.get(), &isCorrect);
  *isMisspelled = !isCorrect;
  return true;
}

void
RemoteSpellcheckEngineParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

} 

