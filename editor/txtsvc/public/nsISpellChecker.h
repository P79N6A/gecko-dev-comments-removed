




































#ifndef nsISpellChecker_h__
#define nsISpellChecker_h__

#include "nsISupports.h"
#include "nsTArray.h"

#define NS_SPELLCHECKER_CONTRACTID "@mozilla.org/spellchecker;1"

#define NS_ISPELLCHECKER_IID                    \
{ /* E75AC48C-E948-452E-8DB3-30FEE29FE3D2 */    \
0xe75ac48c, 0xe948, 0x452e, \
  { 0x8d, 0xb3, 0x30, 0xfe, 0xe2, 0x9f, 0xe3, 0xd2 } }

class nsITextServicesDocument;
class nsString;




class nsISpellChecker  : public nsISupports{
public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISPELLCHECKER_IID)

  





  NS_IMETHOD SetDocument(nsITextServicesDocument *aDoc, PRBool aFromStartofDoc) = 0;

  





  NS_IMETHOD NextMisspelledWord(nsAString &aWord, nsTArray<nsString> *aSuggestions) = 0;

  







  NS_IMETHOD CheckWord(const nsAString &aWord, PRBool *aIsMisspelled, nsTArray<nsString> *aSuggestions) = 0;

  







  NS_IMETHOD Replace(const nsAString &aOldWord, const nsAString &aNewWord, PRBool aAllOccurrences) = 0;

  



  NS_IMETHOD IgnoreAll(const nsAString &aWord) = 0;

  



  NS_IMETHOD AddWordToPersonalDictionary(const nsAString &aWord) = 0;

  



  NS_IMETHOD RemoveWordFromPersonalDictionary(const nsAString &aWord) = 0;

  




  NS_IMETHOD GetPersonalDictionary(nsTArray<nsString> *aWordList) = 0;

  








  NS_IMETHOD GetDictionaryList(nsTArray<nsString> *aDictionaryList) = 0;

  





  NS_IMETHOD GetCurrentDictionary(nsAString &aDictionary) = 0;

  




  NS_IMETHOD SetCurrentDictionary(const nsAString &aDictionary) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsISpellChecker, NS_ISPELLCHECKER_IID)

#endif 

