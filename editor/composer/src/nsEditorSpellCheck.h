






































#ifndef nsEditorSpellCheck_h___
#define nsEditorSpellCheck_h___


#include "nsIEditorSpellCheck.h"
#include "nsISpellChecker.h"
#include "nsVoidArray.h"
#include "nsCOMPtr.h"

#define NS_EDITORSPELLCHECK_CID                     \
{ /* {75656ad9-bd13-4c5d-939a-ec6351eea0cc} */        \
    0x75656ad9, 0xbd13, 0x4c5d,                       \
    { 0x93, 0x9a, 0xec, 0x63, 0x51, 0xee, 0xa0, 0xcc }\
}

class nsEditorSpellCheck : public nsIEditorSpellCheck
{
public:
  nsEditorSpellCheck();
  virtual ~nsEditorSpellCheck();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIEDITORSPELLCHECK

protected:
  nsCOMPtr<nsISpellChecker> mSpellChecker;

  nsStringArray  mSuggestedWordList;
  PRInt32        mSuggestedWordIndex;

  
  
  nsStringArray  mDictionaryList;
  PRInt32        mDictionaryIndex;

  nsresult       DeleteSuggestedWordList();

  nsCOMPtr<nsITextServicesFilter> mTxtSrvFilter;
};

#endif 


