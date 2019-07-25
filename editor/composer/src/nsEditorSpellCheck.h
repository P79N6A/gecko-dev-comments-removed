






































#ifndef nsEditorSpellCheck_h___
#define nsEditorSpellCheck_h___


#include "nsIEditorSpellCheck.h"
#include "nsISpellChecker.h"
#include "nsIURI.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDataHashtable.h"

#define NS_EDITORSPELLCHECK_CID                     \
{ /* {75656ad9-bd13-4c5d-939a-ec6351eea0cc} */        \
    0x75656ad9, 0xbd13, 0x4c5d,                       \
    { 0x93, 0x9a, 0xec, 0x63, 0x51, 0xee, 0xa0, 0xcc }\
}

class LastDictionary;

class nsEditorSpellCheck : public nsIEditorSpellCheck
{
public:
  nsEditorSpellCheck();
  virtual ~nsEditorSpellCheck();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsEditorSpellCheck)

  
  NS_DECL_NSIEDITORSPELLCHECK

  static LastDictionary* gDictionaryStore;

  static void ShutDown();

protected:
  nsCOMPtr<nsISpellChecker> mSpellChecker;

  nsTArray<nsString>  mSuggestedWordList;
  PRInt32        mSuggestedWordIndex;

  
  
  nsTArray<nsString>  mDictionaryList;
  PRInt32        mDictionaryIndex;

  nsresult       DeleteSuggestedWordList();

  nsCOMPtr<nsITextServicesFilter> mTxtSrvFilter;
  nsCOMPtr<nsIEditor> mEditor;

  nsString mPreferredLang;

  bool mUpdateDictionaryRunning;

public:
  void BeginUpdateDictionary() { mUpdateDictionaryRunning = true ;}
  void EndUpdateDictionary() { mUpdateDictionaryRunning = false ;}
};

#endif 


