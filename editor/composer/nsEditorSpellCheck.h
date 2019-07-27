




#ifndef nsEditorSpellCheck_h___
#define nsEditorSpellCheck_h___


#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsIEditorSpellCheck.h"        
#include "nsISupportsImpl.h"
#include "nsString.h"                   
#include "nsTArray.h"                   
#include "nscore.h"                     

class nsIEditor;
class nsISpellChecker;
class nsITextServicesFilter;

#define NS_EDITORSPELLCHECK_CID                     \
{ /* {75656ad9-bd13-4c5d-939a-ec6351eea0cc} */        \
    0x75656ad9, 0xbd13, 0x4c5d,                       \
    { 0x93, 0x9a, 0xec, 0x63, 0x51, 0xee, 0xa0, 0xcc }\
}

class DictionaryFetcher;

class nsEditorSpellCheck MOZ_FINAL : public nsIEditorSpellCheck
{
  friend class DictionaryFetcher;

public:
  nsEditorSpellCheck();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsEditorSpellCheck)

  
  NS_DECL_NSIEDITORSPELLCHECK

protected:
  virtual ~nsEditorSpellCheck();

  nsCOMPtr<nsISpellChecker> mSpellChecker;

  nsTArray<nsString>  mSuggestedWordList;
  int32_t        mSuggestedWordIndex;

  
  
  nsTArray<nsString>  mDictionaryList;
  int32_t        mDictionaryIndex;

  nsresult       DeleteSuggestedWordList();

  nsCOMPtr<nsITextServicesFilter> mTxtSrvFilter;
  nsCOMPtr<nsIEditor> mEditor;

  nsString mPreferredLang;

  uint32_t mDictionaryFetcherGroup;

  bool mUpdateDictionaryRunning;

  nsresult DictionaryFetched(DictionaryFetcher* aFetchState);

public:
  void BeginUpdateDictionary() { mUpdateDictionaryRunning = true ;}
  void EndUpdateDictionary() { mUpdateDictionaryRunning = false ;}
};

#endif 


