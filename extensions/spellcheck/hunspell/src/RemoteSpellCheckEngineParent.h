


#ifndef RemoteSpellcheckEngineParent_h_
#define RemoteSpellcheckEngineParent_h_

#include "mozilla/PRemoteSpellcheckEngineParent.h"
#include "nsCOMPtr.h"

class nsISpellChecker;

namespace mozilla {

class RemoteSpellcheckEngineParent : public PRemoteSpellcheckEngineParent
{
public:
  RemoteSpellcheckEngineParent();

  virtual ~RemoteSpellcheckEngineParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool RecvSetDictionary(const nsString& aDictionary,
                                   bool* success) MOZ_OVERRIDE;

  virtual bool RecvCheck(const nsString& aWord, bool* aIsMisspelled) MOZ_OVERRIDE;

  virtual bool RecvCheckAndSuggest(const nsString& aWord,
                                     bool* aIsMisspelled,
                                     InfallibleTArray<nsString>* aSuggestions)
      MOZ_OVERRIDE;

private:
  nsCOMPtr<nsISpellChecker> mSpellChecker;
};

}
#endif
