









































#include "nsEditorSpellCheck.h"

#include "nsITextServicesDocument.h"
#include "nsISpellChecker.h"
#include "nsISelection.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"

#include "nsIComponentManager.h"
#include "nsServiceManagerUtils.h"
#include "nsIChromeRegistry.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsITextServicesFilter.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsEditorSpellCheck)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsEditorSpellCheck)

NS_INTERFACE_MAP_BEGIN(nsEditorSpellCheck)
  NS_INTERFACE_MAP_ENTRY(nsIEditorSpellCheck)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEditorSpellCheck)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsEditorSpellCheck)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_2(nsEditorSpellCheck,
                           mSpellChecker,
                           mTxtSrvFilter)

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
  nsTArray<nsString> dictList;
  rv = spellChecker->GetDictionaryList(&dictList);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = (dictList.Length() > 0);
  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::InitSpellChecker(nsIEditor* aEditor, PRBool aEnableSelectionChecking)
{
  nsresult rv;

  
  nsCOMPtr<nsITextServicesDocument>tsDoc =
     do_CreateInstance("@mozilla.org/textservices/textservicesdocument;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(tsDoc, NS_ERROR_NULL_POINTER);

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

  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NULL_POINTER);

  rv = mSpellChecker->SetDocument(tsDoc, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  

  nsAdoptingString dictName =
    Preferences::GetLocalizedString("spellchecker.dictionary");

  if (dictName.IsEmpty())
  {
    
    

    nsCOMPtr<nsIXULChromeRegistry> packageRegistry =
      mozilla::services::GetXULChromeRegistryService();

    if (packageRegistry) {
      nsCAutoString utf8DictName;
      rv = packageRegistry->GetSelectedLocale(NS_LITERAL_CSTRING("global"),
                                              utf8DictName);
      AppendUTF8toUTF16(utf8DictName, dictName);
    }
  }

  PRBool setDictionary = PR_FALSE;
  if (NS_SUCCEEDED(rv) && !dictName.IsEmpty()) {
    rv = SetCurrentDictionary(dictName.get());

    
    if (NS_FAILED(rv)) {
      rv = SetCurrentDictionary(NS_LITERAL_STRING("en-US").get());
    }

    if (NS_SUCCEEDED(rv))
      setDictionary = PR_TRUE;
  }

  
  
  
  if (! setDictionary) {
    nsTArray<nsString> dictList;
    rv = mSpellChecker->GetDictionaryList(&dictList);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dictList.Length() > 0) {
      rv = SetCurrentDictionary(dictList[0].get());
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
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

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
  if ( mSuggestedWordIndex < PRInt32(mSuggestedWordList.Length()))
  {
    *aSuggestedWord = ToNewUnicode(mSuggestedWordList[mSuggestedWordIndex]);
    mSuggestedWordIndex++;
  } else {
    
    *aSuggestedWord = ToNewUnicode(EmptyString());
  }
  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::CheckCurrentWord(const PRUnichar *aSuggestedWord,
                                     PRBool *aIsMisspelled)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  DeleteSuggestedWordList();
  return mSpellChecker->CheckWord(nsDependentString(aSuggestedWord),
                                  aIsMisspelled, &mSuggestedWordList);
}

NS_IMETHODIMP    
nsEditorSpellCheck::CheckCurrentWordNoSuggest(const PRUnichar *aSuggestedWord,
                                              PRBool *aIsMisspelled)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->CheckWord(nsDependentString(aSuggestedWord),
                                  aIsMisspelled, nsnull);
}

NS_IMETHODIMP    
nsEditorSpellCheck::ReplaceWord(const PRUnichar *aMisspelledWord,
                                const PRUnichar *aReplaceWord,
                                PRBool           allOccurrences)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->Replace(nsDependentString(aMisspelledWord),
                                nsDependentString(aReplaceWord), allOccurrences);
}

NS_IMETHODIMP    
nsEditorSpellCheck::IgnoreWordAllOccurrences(const PRUnichar *aWord)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->IgnoreAll(nsDependentString(aWord));
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetPersonalDictionary()
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

   
  mDictionaryList.Clear();
  mDictionaryIndex = 0;
  return mSpellChecker->GetPersonalDictionary(&mDictionaryList);
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetPersonalDictionaryWord(PRUnichar **aDictionaryWord)
{
  if ( mDictionaryIndex < PRInt32( mDictionaryList.Length()))
  {
    *aDictionaryWord = ToNewUnicode(mDictionaryList[mDictionaryIndex]);
    mDictionaryIndex++;
  } else {
    
    *aDictionaryWord = ToNewUnicode(EmptyString());
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::AddWordToDictionary(const PRUnichar *aWord)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->AddWordToPersonalDictionary(nsDependentString(aWord));
}

NS_IMETHODIMP    
nsEditorSpellCheck::RemoveWordFromDictionary(const PRUnichar *aWord)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->RemoveWordFromPersonalDictionary(nsDependentString(aWord));
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetDictionaryList(PRUnichar ***aDictionaryList, PRUint32 *aCount)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_TRUE(aDictionaryList && aCount, NS_ERROR_NULL_POINTER);

  *aDictionaryList = 0;
  *aCount          = 0;

  nsTArray<nsString> dictList;

  nsresult rv = mSpellChecker->GetDictionaryList(&dictList);

  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar **tmpPtr = 0;

  if (dictList.Length() < 1)
  {
    
    

    tmpPtr = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *));

    NS_ENSURE_TRUE(tmpPtr, NS_ERROR_OUT_OF_MEMORY);

    *tmpPtr          = 0;
    *aDictionaryList = tmpPtr;
    *aCount          = 0;

    return NS_OK;
  }

  tmpPtr = (PRUnichar **)nsMemory::Alloc(sizeof(PRUnichar *) * dictList.Length());

  NS_ENSURE_TRUE(tmpPtr, NS_ERROR_OUT_OF_MEMORY);

  *aDictionaryList = tmpPtr;
  *aCount          = dictList.Length();

  PRUint32 i;

  for (i = 0; i < *aCount; i++)
  {
    tmpPtr[i] = ToNewUnicode(dictList[i]);
  }

  return rv;
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetCurrentDictionary(PRUnichar **aDictionary)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_TRUE(aDictionary, NS_ERROR_NULL_POINTER);

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
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_TRUE(aDictionary, NS_ERROR_NULL_POINTER);

  return mSpellChecker->SetCurrentDictionary(nsDependentString(aDictionary));
}

NS_IMETHODIMP    
nsEditorSpellCheck::UninitSpellChecker()
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  
  
#ifdef DEBUG
  nsresult rv =
#endif
  SaveDefaultDictionary();
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
  PRUnichar *dictName = nsnull;
  nsresult rv = GetCurrentDictionary(&dictName);

  if (NS_SUCCEEDED(rv) && dictName && *dictName) {
    rv = Preferences::SetString("spellchecker.dictionary", dictName);
  }

  if (dictName) {
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
