









































#include "nsEditorSpellCheck.h"

#include "nsStyleUtil.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsITextServicesDocument.h"
#include "nsISpellChecker.h"
#include "nsISelection.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"
#include "nsIHTMLEditor.h"

#include "nsIComponentManager.h"
#include "nsIContentPrefService.h"
#include "nsServiceManagerUtils.h"
#include "nsIChromeRegistry.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsITextServicesFilter.h"
#include "nsUnicharUtils.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

class UpdateDictionnaryHolder {
  private:
    nsEditorSpellCheck* mSpellCheck;
  public:
    UpdateDictionnaryHolder(nsEditorSpellCheck* esc): mSpellCheck(esc) {
      if (mSpellCheck) {
        mSpellCheck->BeginUpdateDictionary();
      }
    }
    ~UpdateDictionnaryHolder() {
      if (mSpellCheck) {
        mSpellCheck->EndUpdateDictionary();
      }
    }
};

#define CPS_PREF_NAME NS_LITERAL_STRING("spellcheck.lang")

class LastDictionary {
public:
  



  NS_IMETHOD StoreCurrentDictionary(nsIEditor* aEditor, const nsAString& aDictionary);

  


  NS_IMETHOD FetchLastDictionary(nsIEditor* aEditor, nsAString& aDictionary);

  


  NS_IMETHOD ClearCurrentDictionary(nsIEditor* aEditor);

  



  static nsresult GetDocumentURI(nsIEditor* aEditor, nsIURI * *aURI);
};


nsresult
LastDictionary::GetDocumentURI(nsIEditor* aEditor, nsIURI * *aURI)
{
  NS_ENSURE_ARG_POINTER(aEditor);
  NS_ENSURE_ARG_POINTER(aURI);

  nsCOMPtr<nsIDOMDocument> domDoc;
  aEditor->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_TRUE(domDoc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIURI> docUri = doc->GetDocumentURI();
  NS_ENSURE_TRUE(docUri, NS_ERROR_FAILURE);

  *aURI = docUri;
  NS_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP
LastDictionary::FetchLastDictionary(nsIEditor* aEditor, nsAString& aDictionary)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  nsresult rv;

  nsCOMPtr<nsIURI> docUri;
  rv = GetDocumentURI(aEditor, getter_AddRefs(docUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContentPrefService> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(contentPrefService, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIWritableVariant> uri = do_CreateInstance(NS_VARIANT_CONTRACTID);
  NS_ENSURE_TRUE(uri, NS_ERROR_OUT_OF_MEMORY);
  uri->SetAsISupports(docUri);

  bool hasPref;
  if (NS_SUCCEEDED(contentPrefService->HasPref(uri, CPS_PREF_NAME, &hasPref)) && hasPref) {
    nsCOMPtr<nsIVariant> pref;
    contentPrefService->GetPref(uri, CPS_PREF_NAME, nsnull, getter_AddRefs(pref));
    pref->GetAsAString(aDictionary);
  } else {
    aDictionary.Truncate();
  }

  return NS_OK;
}

NS_IMETHODIMP
LastDictionary::StoreCurrentDictionary(nsIEditor* aEditor, const nsAString& aDictionary)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  nsresult rv;

  nsCOMPtr<nsIURI> docUri;
  rv = GetDocumentURI(aEditor, getter_AddRefs(docUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritableVariant> uri = do_CreateInstance(NS_VARIANT_CONTRACTID);
  NS_ENSURE_TRUE(uri, NS_ERROR_OUT_OF_MEMORY);
  uri->SetAsISupports(docUri);

  nsCOMPtr<nsIWritableVariant> prefValue = do_CreateInstance(NS_VARIANT_CONTRACTID);
  NS_ENSURE_TRUE(prefValue, NS_ERROR_OUT_OF_MEMORY);
  prefValue->SetAsAString(aDictionary);

  nsCOMPtr<nsIContentPrefService> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(contentPrefService, NS_ERROR_NOT_INITIALIZED);

  return contentPrefService->SetPref(uri, CPS_PREF_NAME, prefValue);
}

NS_IMETHODIMP
LastDictionary::ClearCurrentDictionary(nsIEditor* aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  nsresult rv;

  nsCOMPtr<nsIURI> docUri;
  rv = GetDocumentURI(aEditor, getter_AddRefs(docUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritableVariant> uri = do_CreateInstance(NS_VARIANT_CONTRACTID);
  NS_ENSURE_TRUE(uri, NS_ERROR_OUT_OF_MEMORY);
  uri->SetAsISupports(docUri);

  nsCOMPtr<nsIContentPrefService> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(contentPrefService, NS_ERROR_NOT_INITIALIZED);

  return contentPrefService->RemovePref(uri, CPS_PREF_NAME);
}

LastDictionary* nsEditorSpellCheck::gDictionaryStore = nsnull;

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsEditorSpellCheck)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsEditorSpellCheck)

NS_INTERFACE_MAP_BEGIN(nsEditorSpellCheck)
  NS_INTERFACE_MAP_ENTRY(nsIEditorSpellCheck)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEditorSpellCheck)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsEditorSpellCheck)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_3(nsEditorSpellCheck,
                           mEditor,
                           mSpellChecker,
                           mTxtSrvFilter)

nsEditorSpellCheck::nsEditorSpellCheck()
  : mSuggestedWordIndex(0)
  , mDictionaryIndex(0)
  , mEditor(nsnull)
  , mUpdateDictionaryRunning(false)
{
}

nsEditorSpellCheck::~nsEditorSpellCheck()
{
  
  
  mSpellChecker = nsnull;
}






NS_IMETHODIMP
nsEditorSpellCheck::CanSpellCheck(bool* _retval)
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
nsEditorSpellCheck::InitSpellChecker(nsIEditor* aEditor, bool aEnableSelectionChecking)
{
  NS_ENSURE_TRUE(aEditor, NS_ERROR_NULL_POINTER);
  mEditor = aEditor;

  nsresult rv;

  if (!gDictionaryStore) {
    gDictionaryStore = new LastDictionary();
  }


  
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

      bool collapsed = false;
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

  rv = mSpellChecker->SetDocument(tsDoc, true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  UpdateCurrentDictionary();
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
                                     bool *aIsMisspelled)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  DeleteSuggestedWordList();
  return mSpellChecker->CheckWord(nsDependentString(aSuggestedWord),
                                  aIsMisspelled, &mSuggestedWordList);
}

NS_IMETHODIMP    
nsEditorSpellCheck::CheckCurrentWordNoSuggest(const PRUnichar *aSuggestedWord,
                                              bool *aIsMisspelled)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->CheckWord(nsDependentString(aSuggestedWord),
                                  aIsMisspelled, nsnull);
}

NS_IMETHODIMP    
nsEditorSpellCheck::ReplaceWord(const PRUnichar *aMisspelledWord,
                                const PRUnichar *aReplaceWord,
                                bool             allOccurrences)
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
nsEditorSpellCheck::GetCurrentDictionary(nsAString& aDictionary)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->GetCurrentDictionary(aDictionary);
}

NS_IMETHODIMP    
nsEditorSpellCheck::SetCurrentDictionary(const nsAString& aDictionary)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  if (!mUpdateDictionaryRunning) {

    nsDefaultStringComparator comparator;
    nsAutoString langCode;
    PRInt32 dashIdx = aDictionary.FindChar('-');
    if (dashIdx != -1) {
      langCode.Assign(Substring(aDictionary, 0, dashIdx));
    } else {
      langCode.Assign(aDictionary);
    }

    if (mPreferredLang.IsEmpty() || !nsStyleUtil::DashMatchCompare(mPreferredLang, langCode, comparator)) {
      
      
      gDictionaryStore->StoreCurrentDictionary(mEditor, aDictionary);
    } else {
      
      
      gDictionaryStore->ClearCurrentDictionary(mEditor);
    }

    
    
    Preferences::SetString("spellchecker.dictionary", aDictionary);
  }
  return mSpellChecker->SetCurrentDictionary(aDictionary);
}

NS_IMETHODIMP
nsEditorSpellCheck::CheckCurrentDictionary()
{
  mSpellChecker->CheckCurrentDictionary();

  
  nsAutoString currentDictionary;
  nsresult rv = GetCurrentDictionary(currentDictionary);
  if (NS_SUCCEEDED(rv) && !currentDictionary.IsEmpty()) {
    return NS_OK;
  }

  
  nsTArray<nsString> dictList;
  rv = mSpellChecker->GetDictionaryList(&dictList);
  NS_ENSURE_SUCCESS(rv, rv);

  if (dictList.Length() > 0) {
    rv = SetCurrentDictionary(dictList[0]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::UninitSpellChecker()
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  
  DeleteSuggestedWordList();
  mDictionaryList.Clear();
  mDictionaryIndex = 0;
  mSpellChecker = 0;
  return NS_OK;
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

NS_IMETHODIMP
nsEditorSpellCheck::UpdateCurrentDictionary()
{
  nsresult rv;

  UpdateDictionnaryHolder holder(this);

  
  nsCOMPtr<nsIContent> rootContent;
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(mEditor);
  if (htmlEditor) {
    rootContent = htmlEditor->GetActiveEditingHost();
  } else {
    nsCOMPtr<nsIDOMElement> rootElement;
    rv = mEditor->GetRootElement(getter_AddRefs(rootElement));
    NS_ENSURE_SUCCESS(rv, rv);
    rootContent = do_QueryInterface(rootElement);
  }
  NS_ENSURE_TRUE(rootContent, NS_ERROR_FAILURE);

  mPreferredLang.Truncate();
  rootContent->GetLang(mPreferredLang);

  

  
  
  nsAutoString dictName;
  rv = gDictionaryStore->FetchLastDictionary(mEditor, dictName);
  if (NS_SUCCEEDED(rv) && !dictName.IsEmpty()) {
    if (NS_FAILED(SetCurrentDictionary(dictName))) { 
      
      gDictionaryStore->ClearCurrentDictionary(mEditor);
    }
    return NS_OK;
  }

  if (mPreferredLang.IsEmpty()) {
    nsCOMPtr<nsIDocument> doc = rootContent->GetCurrentDoc();
    NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
    doc->GetContentLanguage(mPreferredLang);
  }

  
  if (!mPreferredLang.IsEmpty()) {
    dictName.Assign(mPreferredLang);
  }

  
  nsAutoString preferedDict(Preferences::GetLocalizedString("spellchecker.dictionary"));
  if (dictName.IsEmpty()) {
    dictName.Assign(preferedDict);
  }

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

  if (NS_SUCCEEDED(rv) && !dictName.IsEmpty()) {
    rv = SetCurrentDictionary(dictName);
    if (NS_FAILED(rv)) {
      
      

      nsAutoString langCode;
      PRInt32 dashIdx = dictName.FindChar('-');
      if (dashIdx != -1) {
        langCode.Assign(Substring(dictName, 0, dashIdx));
      } else {
        langCode.Assign(dictName);
      }

      nsDefaultStringComparator comparator;

      
      
      if (!preferedDict.IsEmpty() && !dictName.Equals(preferedDict) && 
          nsStyleUtil::DashMatchCompare(preferedDict, langCode, comparator)) {
        rv = SetCurrentDictionary(preferedDict);
      }

      
      if (NS_FAILED(rv)) {
        if (!dictName.Equals(langCode) && !preferedDict.Equals(langCode)) {
          rv = SetCurrentDictionary(langCode);
        }
      }

      
      if (NS_FAILED(rv)) {
        
        
        nsTArray<nsString> dictList;
        rv = mSpellChecker->GetDictionaryList(&dictList);
        NS_ENSURE_SUCCESS(rv, rv);
        PRInt32 i, count = dictList.Length();
        for (i = 0; i < count; i++) {
          nsAutoString dictStr(dictList.ElementAt(i));

          if (dictStr.Equals(dictName) ||
              dictStr.Equals(preferedDict) ||
              dictStr.Equals(langCode)) {
            
            continue;
          }

          if (nsStyleUtil::DashMatchCompare(dictStr, langCode, comparator) &&
              NS_SUCCEEDED(SetCurrentDictionary(dictStr))) {
              break;
          }
        }
      }
    }
  }

  
  
  
  if (mPreferredLang.IsEmpty()) {
    nsAutoString currentDictionary;
    rv = GetCurrentDictionary(currentDictionary);
    if (NS_FAILED(rv) || currentDictionary.IsEmpty()) {
      rv = SetCurrentDictionary(NS_LITERAL_STRING("en-US"));
      if (NS_FAILED(rv)) {
        nsTArray<nsString> dictList;
        rv = mSpellChecker->GetDictionaryList(&dictList);
        if (NS_SUCCEEDED(rv) && dictList.Length() > 0) {
          SetCurrentDictionary(dictList[0]);
        }
      }
    }
  }

  
  
  
  

  DeleteSuggestedWordList();

  return NS_OK;
}

void 
nsEditorSpellCheck::ShutDown() {
  delete gDictionaryStore;
}
