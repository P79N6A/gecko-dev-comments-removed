



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
RemoteSpellcheckEngineParent::AnswerSetDictionary(
  const nsString& aDictionary,
  bool* success)
{
  nsresult rv = mEngine->SetDictionary(aDictionary.get());
  *success = NS_SUCCEEDED(rv);
  return true;
}

bool
RemoteSpellcheckEngineParent::AnswerCheck(
  const nsString& aWord,
  bool* aIsMisspelled)
{
  bool isCorrect = true;
  mEngine->Check(aWord.get(), &isCorrect);
  *aIsMisspelled = !isCorrect;
  return true;
}

bool
RemoteSpellcheckEngineParent::AnswerCheckAndSuggest(
  const nsString& aWord,
  bool* aIsMisspelled,
  InfallibleTArray<nsString>* aSuggestions)
{
  bool isCorrect = true;
  mEngine->Check(aWord.get(), &isCorrect);
  *aIsMisspelled = !isCorrect;
  if (!isCorrect) {
    char16_t **suggestions;
    uint32_t count = 0;
    mEngine->Suggest(aWord.get(), &suggestions, &count);

    for (uint32_t i=0; i<count; i++) {
      aSuggestions->AppendElement(nsDependentString(suggestions[i]));
    }
  }
  return true;
}

void
RemoteSpellcheckEngineParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

} 

