




































#ifndef nsISpellChecker_h__
#define nsISpellChecker_h__

#include "nsISupports.h"
#include "nsTArray.h"

#define NS_SPELLCHECKER_CONTRACTID "@mozilla.org/spellchecker;1"

#define NS_ISPELLCHECKER_IID                    \
{ /* 27bff957-b486-40ae-9f5d-af0cdd211868 */    \
0x27bff957, 0xb486, 0x40ae, \
  { 0x9f, 0x5d, 0xaf, 0x0c, 0xdd, 0x21, 0x18, 0x68 } }

class nsITextServicesDocument;
class nsString;




class nsISpellChecker  : public nsISupports{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISPELLCHECKER_IID)

  





  NS_IMETHOD SetDocument(nsITextServicesDocument *aDoc, bool aFromStartofDoc) = 0;

  





  NS_IMETHOD NextMisspelledWord(nsAString &aWord, nsTArray<nsString> *aSuggestions) = 0;

  







  NS_IMETHOD CheckWord(const nsAString &aWord, bool *aIsMisspelled, nsTArray<nsString> *aSuggestions) = 0;

  







  NS_IMETHOD Replace(const nsAString &aOldWord, const nsAString &aNewWord, bool aAllOccurrences) = 0;

  



  NS_IMETHOD IgnoreAll(const nsAString &aWord) = 0;

  



  NS_IMETHOD AddWordToPersonalDictionary(const nsAString &aWord) = 0;

  



  NS_IMETHOD RemoveWordFromPersonalDictionary(const nsAString &aWord) = 0;

  




  NS_IMETHOD GetPersonalDictionary(nsTArray<nsString> *aWordList) = 0;

  








  NS_IMETHOD GetDictionaryList(nsTArray<nsString> *aDictionaryList) = 0;

  





  NS_IMETHOD GetCurrentDictionary(nsAString &aDictionary) = 0;

  





  NS_IMETHOD SetCurrentDictionary(const nsAString &aDictionary) = 0;

  



  NS_IMETHOD CheckCurrentDictionary() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISpellChecker, NS_ISPELLCHECKER_IID)

#endif 

