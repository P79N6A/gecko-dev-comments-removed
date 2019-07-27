




#ifndef mozSpellChecker_h__
#define mozSpellChecker_h__

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsISpellChecker.h"
#include "nsString.h"
#include "nsITextServicesDocument.h"
#include "mozIPersonalDictionary.h"
#include "mozISpellCheckingEngine.h"
#include "nsClassHashtable.h"
#include "nsTArray.h"
#include "mozISpellI18NUtil.h"
#include "nsCycleCollectionParticipant.h"
#include "RemoteSpellCheckEngineChild.h"

namespace mozilla {
class PRemoteSpellcheckEngineChild;
class RemoteSpellcheckEngineChild;
}

class mozSpellChecker : public nsISpellChecker
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(mozSpellChecker)

  mozSpellChecker();

  nsresult Init();

  
  NS_IMETHOD SetDocument(nsITextServicesDocument *aDoc, bool aFromStartofDoc) MOZ_OVERRIDE;
  NS_IMETHOD NextMisspelledWord(nsAString &aWord, nsTArray<nsString> *aSuggestions) MOZ_OVERRIDE;
  NS_IMETHOD CheckWord(const nsAString &aWord, bool *aIsMisspelled, nsTArray<nsString> *aSuggestions) MOZ_OVERRIDE;
  NS_IMETHOD Replace(const nsAString &aOldWord, const nsAString &aNewWord, bool aAllOccurrences) MOZ_OVERRIDE;
  NS_IMETHOD IgnoreAll(const nsAString &aWord) MOZ_OVERRIDE;

  NS_IMETHOD AddWordToPersonalDictionary(const nsAString &aWord) MOZ_OVERRIDE;
  NS_IMETHOD RemoveWordFromPersonalDictionary(const nsAString &aWord) MOZ_OVERRIDE;
  NS_IMETHOD GetPersonalDictionary(nsTArray<nsString> *aWordList) MOZ_OVERRIDE;

  NS_IMETHOD GetDictionaryList(nsTArray<nsString> *aDictionaryList) MOZ_OVERRIDE;
  NS_IMETHOD GetCurrentDictionary(nsAString &aDictionary) MOZ_OVERRIDE;
  NS_IMETHOD SetCurrentDictionary(const nsAString &aDictionary) MOZ_OVERRIDE;
  NS_IMETHOD CheckCurrentDictionary() MOZ_OVERRIDE;

  void DeleteRemoteEngine() {
    mEngine = nullptr;
  }

protected:
  virtual ~mozSpellChecker();

  nsCOMPtr<mozISpellI18NUtil> mConverter;
  nsCOMPtr<nsITextServicesDocument> mTsDoc;
  nsCOMPtr<mozIPersonalDictionary> mPersonalDictionary;

  nsCOMPtr<mozISpellCheckingEngine>  mSpellCheckingEngine;
  bool mFromStart;

  nsString mCurrentDictionary;

  nsresult SetupDoc(int32_t *outBlockOffset);

  nsresult GetCurrentBlockIndex(nsITextServicesDocument *aDoc, int32_t *outBlockIndex);

  nsresult GetEngineList(nsCOMArray<mozISpellCheckingEngine> *aDictionaryList);

  mozilla::PRemoteSpellcheckEngineChild *mEngine;
};
#endif 
