




#include "RemoteSpellCheckEngineParent.h"
#include "nsISpellChecker.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {

RemoteSpellcheckEngineParent::RemoteSpellcheckEngineParent()
{
  mSpellChecker = do_CreateInstance(NS_SPELLCHECKER_CONTRACTID);
}

RemoteSpellcheckEngineParent::~RemoteSpellcheckEngineParent()
{
}

bool
RemoteSpellcheckEngineParent::RecvSetDictionary(
  const nsString& aDictionary,
  bool* success)
{
  nsresult rv = mSpellChecker->SetCurrentDictionary(aDictionary);
  *success = NS_SUCCEEDED(rv);
  return true;
}

bool
RemoteSpellcheckEngineParent::RecvCheck(
  const nsString& aWord,
  bool* aIsMisspelled)
{
  nsresult rv = mSpellChecker->CheckWord(aWord, aIsMisspelled, nullptr);

  
  if (NS_FAILED(rv))
    *aIsMisspelled = false;
  return true;
}

bool
RemoteSpellcheckEngineParent::RecvCheckAndSuggest(
  const nsString& aWord,
  bool* aIsMisspelled,
  InfallibleTArray<nsString>* aSuggestions)
{
  nsresult rv = mSpellChecker->CheckWord(aWord, aIsMisspelled, aSuggestions);
  if (NS_FAILED(rv)) {
    aSuggestions->Clear();
    *aIsMisspelled = false;
  }
  return true;
}

void
RemoteSpellcheckEngineParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

} 
