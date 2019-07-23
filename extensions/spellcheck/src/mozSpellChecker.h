




































#ifndef mozSpellChecker_h__
#define mozSpellChecker_h__

#include "nsCOMPtr.h"
#include "nsISpellChecker.h"
#include "nsString.h"
#include "nsITextServicesDocument.h"
#include "mozIPersonalDictionary.h"
#include "mozISpellCheckingEngine.h"
#include "nsClassHashtable.h"
#include "nsVoidArray.h"
#include "nsTArray.h"
#include "mozISpellI18NUtil.h"

class mozSpellChecker : public nsISpellChecker
{
public:
  NS_DECL_ISUPPORTS

  mozSpellChecker();
  virtual ~mozSpellChecker();

  nsresult Init();

  
  NS_IMETHOD SetDocument(nsITextServicesDocument *aDoc, PRBool aFromStartofDoc);
  NS_IMETHOD NextMisspelledWord(nsAString &aWord, nsTArray<nsString> *aSuggestions);
  NS_IMETHOD CheckWord(const nsAString &aWord, PRBool *aIsMisspelled, nsTArray<nsString> *aSuggestions);
  NS_IMETHOD Replace(const nsAString &aOldWord, const nsAString &aNewWord, PRBool aAllOccurrences);
  NS_IMETHOD IgnoreAll(const nsAString &aWord);

  NS_IMETHOD AddWordToPersonalDictionary(const nsAString &aWord);
  NS_IMETHOD RemoveWordFromPersonalDictionary(const nsAString &aWord);
  NS_IMETHOD GetPersonalDictionary(nsTArray<nsString> *aWordList);

  NS_IMETHOD GetDictionaryList(nsTArray<nsString> *aDictionaryList);
  NS_IMETHOD GetCurrentDictionary(nsAString &aDictionary);
  NS_IMETHOD SetCurrentDictionary(const nsAString &aDictionary);

protected:
  nsCOMPtr<mozISpellI18NUtil> mConverter;
  nsCOMPtr<nsITextServicesDocument> mTsDoc;
  nsCOMPtr<mozIPersonalDictionary> mPersonalDictionary;

  
  nsClassHashtable<nsStringHashKey, nsCString> mDictionariesMap;

  nsString mDictionaryName;
  nsCString *mCurrentEngineContractId;
  nsCOMPtr<mozISpellCheckingEngine>  mSpellCheckingEngine;
  PRBool mFromStart;
  nsTArray<nsString> mIgnoreList;

  nsresult SetupDoc(PRInt32 *outBlockOffset);

  nsresult GetCurrentBlockIndex(nsITextServicesDocument *aDoc, PRInt32 *outBlockIndex);

  nsresult InitSpellCheckDictionaryMap();
};
#endif 
