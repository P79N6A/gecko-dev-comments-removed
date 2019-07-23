









































#include "nsEditorSpellCheck.h"

#include "nsITextServicesDocument.h"
#include "nsISpellChecker.h"
#include "nsISelection.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"

#include "nsIComponentManager.h"
#include "nsXPIDLString.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsISupportsPrimitives.h"
#include "nsServiceManagerUtils.h"
#include "nsIChromeRegistry.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsITextServicesFilter.h"

NS_IMPL_ISUPPORTS1(nsEditorSpellCheck,
                   nsIEditorSpellCheck)

nsEditorSpellCheck::nsEditorSpellCheck()
  : mSuggestedWordIndex(0)
  , mDictionaryIndex(0)
{
}

nsEditorSpellCheck::~nsEditorSpellCheck()
{
  
  
  mSpellChecker = nsnull;
}






NS_IMETHODIMP
nsEditorSpellCheck::CanSpellCheck(PRBool* _retval)
{
  nsresult rv;
  nsCOMPtr<nsISpellChecker> spellChecker;
  if (! mSpellChecker) {
    spellChecker = do_CreateInstance(NS_SPELLCHECKER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    spellChecker = mSpellChecker;
  }
  nsStringArray dictList;
  rv = spellChecker->GetDictionaryList(&dictList);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = (dictList.Count() > 0);
  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::InitSpellChecker(nsIEditor* aEditor, PRBool aEnableSelectionChecking)
{
  nsresult rv;

  
  nsCOMPtr<nsITextServicesDocument>tsDoc =
     do_CreateInstance("@mozilla.org/textservices/textservicesdocument;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!tsDoc)
    return NS_ERROR_NULL_POINTER;

  tsDoc->SetFilter(mTxtSrvFilter);

  
  rv = tsDoc->InitWithEditor(aEditor);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aEnableSelectionChecking) {
    
    

    nsCOMPtr<nsISelection> selection;

    rv = aEditor->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

    PRInt32 count = 0;

    rv = selection->GetRangeCount(&count);
    NS_ENSURE_SUCCESS(rv, rv);

    if (count > 0) {
      nsCOMPtr<nsIDOMRange> range;

      rv = selection->GetRangeAt(0, getter_AddRefs(range));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool collapsed = PR_FALSE;
      rv = range->GetCollapsed(&collapsed);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!collapsed) {
        
        

        nsCOMPtr<nsIDOMRange> rangeBounds;
        rv =  range->CloneRange(getter_AddRefs(rangeBounds));
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_TRUE(rangeBounds, NS_ERROR_FAILURE);

        

        rv = tsDoc->ExpandRangeToWordBoundaries(rangeBounds);
        NS_ENSURE_SUCCESS(rv, rv);

        
        

        rv = tsDoc->SetExtent(rangeBounds);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  mSpellChecker = do_CreateInstance(NS_SPELLCHECKER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mSpellChecker)
    return NS_ERROR_NULL_POINTER;

  rv = mSpellChecker->SetDocument(tsDoc, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  

  nsXPIDLString dictName;

  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);

  PRBool hasPreference = PR_FALSE;
  if (NS_SUCCEEDED(rv) && prefBranch) {
    nsCOMPtr<nsISupportsString> prefString;
    rv = prefBranch->GetComplexValue("spellchecker.dictionary",
                                     NS_GET_IID(nsISupportsString),
                                     getter_AddRefs(prefString));
    if (NS_SUCCEEDED(rv) && prefString) {
      hasPreference = PR_TRUE;
      prefString->ToString(getter_Copies(dictName));
    }
  }

  if (! hasPreference || dictName.IsEmpty())
  {
    
    

    nsCOMPtr<nsIXULChromeRegistry> packageRegistry =
      do_GetService(NS_CHROMEREGISTRY_CONTRACTID, &rv);

    if (NS_SUCCEEDED(rv) && packageRegistry) {
      nsCAutoString utf8DictName;
      rv = packageRegistry->GetSelectedLocale(NS_LITERAL_CSTRING("editor"),
                                              utf8DictName);
      AppendUTF8toUTF16(utf8DictName, dictName);
    }
  }

  PRBool setDictionary = PR_FALSE;
  if (NS_SUCCEEDED(rv) && !dictName.IsEmpty()) {
    rv = SetCurrentDictionary(dictName.get());
    if (NS_SUCCEEDED(rv))
      setDictionary = PR_TRUE;
  }

  
  
  
  
  
  
  if (! hasPreference && ! setDictionary) {
    nsStringArray dictList;
    rv = mSpellChecker->GetDictionaryList(&dictList);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dictList.Count() > 0) {
      rv = SetCurrentDictionary(dictList[0]->get());
      if (NS_SUCCEEDED(rv))
        SaveDefaultDictionary();
    }
  }

  
  
  
  

  DeleteSuggestedWordList();

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetNextMisspelledWord(PRUnichar **aNextMisspelledWord)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  nsAutoString nextMisspelledWord;
  
  DeleteSuggestedWordList();
  nsresult rv = mSpellChecker->NextMisspelledWord(nextMisspelledWord,
                                                  &mSuggestedWordList);

  *aNextMisspelledWord = ToNewUnicode(nextMisspelledWord);
  return rv;
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetSuggestedWord(PRUnichar **aSuggestedWord)
{
  nsAutoString word;
  if ( mSuggestedWordIndex < mSuggestedWordList.Count())
  {
    mSuggestedWordList.StringAt(mSuggestedWordIndex, word);
    mSuggestedWordIndex++;
  } else {
    
    word.Truncate();
  }

  *aSuggestedWord = ToNewUnicode(word);
  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::CheckCurrentWord(const PRUnichar *aSuggestedWord,
                                     PRBool *aIsMisspelled)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  DeleteSuggestedWordList();
  return mSpellChecker->CheckWord(nsDependentString(aSuggestedWord),
                                  aIsMisspelled, &mSuggestedWordList);
}

NS_IMETHODIMP    
nsEditorSpellCheck::CheckCurrentWordNoSuggest(const PRUnichar *aSuggestedWord,
                                              PRBool *aIsMisspelled)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  return mSpellChecker->CheckWord(nsDependentString(aSuggestedWord),
                                  aIsMisspelled, nsnull);
}

NS_IMETHODIMP    
nsEditorSpellCheck::ReplaceWord(const PRUnichar *aMisspelledWord,
                                const PRUnichar *aReplaceWord,
                                PRBool           allOccurrences)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  return mSpellChecker->Replace(nsDependentString(aMisspelledWord),
                                nsDependentString(aReplaceWord), allOccurrences);
}

NS_IMETHODIMP    
nsEditorSpellCheck::IgnoreWordAllOccurrences(const PRUnichar *aWord)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  return mSpellChecker->IgnoreAll(nsDependentString(aWord));
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetPersonalDictionary()
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

   
  mDictionaryList.Clear();
  mDictionaryIndex = 0;
  return mSpellChecker->GetPersonalDictionary(&mDictionaryList);
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetPersonalDictionaryWord(PRUnichar **aDictionaryWord)
{
  nsAutoString word;
  if ( mDictionaryIndex < mDictionaryList.Count())
  {
    mDictionaryList.StringAt(mDictionaryIndex, word);
    mDictionaryIndex++;
  } else {
    
    word.Truncate();
  }

  *aDictionaryWord = ToNewUnicode(word);
  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::AddWordToDictionary(const PRUnichar *aWord)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  return mSpellChecker->AddWordToPersonalDictionary(nsDependentString(aWord));
}

NS_IMETHODIMP    
nsEditorSpellCheck::RemoveWordFromDictionary(const PRUnichar *aWord)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  return mSpellChecker->RemoveWordFromPersonalDictionary(nsDependentString(aWord));
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetDictionaryList(PRUnichar ***aDictionaryList, PRUint32 *aCount)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  if (!aDictionaryList || !aCount)
    return NS_ERROR_NULL_POINTER;

  *aDictionaryList = 0;
  *aCount          = 0;

  nsStringArray dictList;

  nsresult rv = mSpellChecker->GetDictionaryList(&dictList);

  if (NS_FAILED(rv))
    return rv;

  PRUnichar **tmpPtr = 0;

  if (dictList.Count() < 1)
  {
    
    

    tmpPtr = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *));

    if (!tmpPtr)
      return NS_ERROR_OUT_OF_MEMORY;

    *tmpPtr          = 0;
    *aDictionaryList = tmpPtr;
    *aCount          = 0;

    return NS_OK;
  }

  tmpPtr = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * dictList.Count());

  if (!tmpPtr)
    return NS_ERROR_OUT_OF_MEMORY;

  *aDictionaryList = tmpPtr;
  *aCount          = dictList.Count();

  nsAutoString dictStr;

  PRUint32 i;

  for (i = 0; i < *aCount; i++)
  {
    dictList.StringAt(i, dictStr);
    tmpPtr[i] = ToNewUnicode(dictStr);
  }

  return rv;
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetCurrentDictionary(PRUnichar **aDictionary)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  if (!aDictionary)
    return NS_ERROR_NULL_POINTER;

  *aDictionary = 0;

  nsAutoString dictStr;
  nsresult rv = mSpellChecker->GetCurrentDictionary(dictStr);
  NS_ENSURE_SUCCESS(rv, rv);

  *aDictionary = ToNewUnicode(dictStr);

  return rv;
}

NS_IMETHODIMP    
nsEditorSpellCheck::SetCurrentDictionary(const PRUnichar *aDictionary)
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  if (!aDictionary)
    return NS_ERROR_NULL_POINTER;

  return mSpellChecker->SetCurrentDictionary(nsDependentString(aDictionary));
}

NS_IMETHODIMP    
nsEditorSpellCheck::UninitSpellChecker()
{
  if (!mSpellChecker)
    return NS_ERROR_NOT_INITIALIZED;

  
  
  nsresult rv = SaveDefaultDictionary();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to set default dictionary");

  
  DeleteSuggestedWordList();
  mDictionaryList.Clear();
  mDictionaryIndex = 0;
  mSpellChecker = 0;
  return NS_OK;
}


NS_IMETHODIMP
nsEditorSpellCheck::SaveDefaultDictionary()
{
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);

  if (NS_SUCCEEDED(rv) && prefBranch)
  {
    PRUnichar *dictName = nsnull;
    rv = GetCurrentDictionary(&dictName);

    if (NS_SUCCEEDED(rv) && dictName && *dictName) {
      nsCOMPtr<nsISupportsString> prefString =
        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);

      if (NS_SUCCEEDED(rv) && prefString) {
        prefString->SetData(nsDependentString(dictName));
        rv = prefBranch->SetComplexValue("spellchecker.dictionary",
                                         NS_GET_IID(nsISupportsString),
                                         prefString);
      }
    }
    if (dictName)
      nsMemory::Free(dictName);
  }
  return rv;
}



NS_IMETHODIMP 
nsEditorSpellCheck::SetFilter(nsITextServicesFilter *filter)
{
  mTxtSrvFilter = filter;
  return NS_OK;
}

nsresult    
nsEditorSpellCheck::DeleteSuggestedWordList()
{
  mSuggestedWordList.Clear();
  mSuggestedWordIndex = 0;
  return NS_OK;
}
