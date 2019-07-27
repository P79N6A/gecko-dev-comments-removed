




#include <stdlib.h>                     

#include "mozilla/Attributes.h"         
#include "mozilla/Preferences.h"        
#include "mozilla/Services.h"           
#include "mozilla/dom/Element.h"        
#include "mozilla/mozalloc.h"           
#include "nsAString.h"                  
#include "nsComponentManagerUtils.h"    
#include "nsDebug.h"                    
#include "nsDependentSubstring.h"       
#include "nsEditorSpellCheck.h"
#include "nsError.h"                    
#include "nsIChromeRegistry.h"          
#include "nsIContent.h"                 
#include "nsIContentPrefService.h"      
#include "nsIContentPrefService2.h"     
#include "nsIDOMDocument.h"             
#include "nsIDOMElement.h"              
#include "nsIDOMRange.h"                
#include "nsIDocument.h"                
#include "nsIEditor.h"                  
#include "nsIHTMLEditor.h"              
#include "nsILoadContext.h"
#include "nsISelection.h"               
#include "nsISpellChecker.h"            
#include "nsISupportsBase.h"            
#include "nsISupportsUtils.h"           
#include "nsITextServicesDocument.h"    
#include "nsITextServicesFilter.h"      
#include "nsIURI.h"                     
#include "nsIVariant.h"                 
#include "nsLiteralString.h"            
#include "nsMemory.h"                   
#include "nsReadableUtils.h"            
#include "nsServiceManagerUtils.h"      
#include "nsString.h"                   
#include "nsStringFwd.h"                
#include "nsStyleUtil.h"                
#include "nsXULAppAPI.h"                

using namespace mozilla;

class UpdateDictionnaryHolder {
  private:
    nsEditorSpellCheck* mSpellCheck;
  public:
    explicit UpdateDictionnaryHolder(nsEditorSpellCheck* esc): mSpellCheck(esc) {
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




static nsresult
GetDocumentURI(nsIEditor* aEditor, nsIURI * *aURI)
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

static already_AddRefed<nsILoadContext>
GetLoadContext(nsIEditor* aEditor)
{
  nsCOMPtr<nsIDOMDocument> domDoc;
  aEditor->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_TRUE(domDoc, nullptr);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  NS_ENSURE_TRUE(doc, nullptr);

  nsCOMPtr<nsILoadContext> loadContext = doc->GetLoadContext();
  return loadContext.forget();
}





class DictionaryFetcher MOZ_FINAL : public nsIContentPrefCallback2
{
public:
  NS_DECL_ISUPPORTS

  DictionaryFetcher(nsEditorSpellCheck* aSpellCheck,
                    nsIEditorSpellCheckCallback* aCallback,
                    uint32_t aGroup)
    : mCallback(aCallback), mGroup(aGroup), mSpellCheck(aSpellCheck) {}

  NS_IMETHOD Fetch(nsIEditor* aEditor);

  NS_IMETHOD HandleResult(nsIContentPref* aPref)
  {
    nsCOMPtr<nsIVariant> value;
    nsresult rv = aPref->GetValue(getter_AddRefs(value));
    NS_ENSURE_SUCCESS(rv, rv);
    value->GetAsAString(mDictionary);
    return NS_OK;
  }

  NS_IMETHOD HandleCompletion(uint16_t reason)
  {
    mSpellCheck->DictionaryFetched(this);
    return NS_OK;
  }

  NS_IMETHOD HandleError(nsresult error)
  {
    return NS_OK;
  }

  nsCOMPtr<nsIEditorSpellCheckCallback> mCallback;
  uint32_t mGroup;
  nsString mRootContentLang;
  nsString mRootDocContentLang;
  nsString mDictionary;

private:
  ~DictionaryFetcher() {}

  nsRefPtr<nsEditorSpellCheck> mSpellCheck;
};
NS_IMPL_ISUPPORTS(DictionaryFetcher, nsIContentPrefCallback2)

NS_IMETHODIMP
DictionaryFetcher::Fetch(nsIEditor* aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  nsresult rv;

  nsCOMPtr<nsIURI> docUri;
  rv = GetDocumentURI(aEditor, getter_AddRefs(docUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString docUriSpec;
  rv = docUri->GetSpec(docUriSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContentPrefService2> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(contentPrefService, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsILoadContext> loadContext = GetLoadContext(aEditor);
  rv = contentPrefService->GetByDomainAndName(NS_ConvertUTF8toUTF16(docUriSpec),
                                              CPS_PREF_NAME, loadContext,
                                              this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




static nsresult
StoreCurrentDictionary(nsIEditor* aEditor, const nsAString& aDictionary)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  nsresult rv;

  nsCOMPtr<nsIURI> docUri;
  rv = GetDocumentURI(aEditor, getter_AddRefs(docUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString docUriSpec;
  rv = docUri->GetSpec(docUriSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritableVariant> prefValue = do_CreateInstance(NS_VARIANT_CONTRACTID);
  NS_ENSURE_TRUE(prefValue, NS_ERROR_OUT_OF_MEMORY);
  prefValue->SetAsAString(aDictionary);

  nsCOMPtr<nsIContentPrefService2> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(contentPrefService, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsILoadContext> loadContext = GetLoadContext(aEditor);
  return contentPrefService->Set(NS_ConvertUTF8toUTF16(docUriSpec),
                                 CPS_PREF_NAME, prefValue, loadContext,
                                 nullptr);
}




static nsresult
ClearCurrentDictionary(nsIEditor* aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  nsresult rv;

  nsCOMPtr<nsIURI> docUri;
  rv = GetDocumentURI(aEditor, getter_AddRefs(docUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString docUriSpec;
  rv = docUri->GetSpec(docUriSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContentPrefService2> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(contentPrefService, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsILoadContext> loadContext = GetLoadContext(aEditor);
  return contentPrefService->RemoveByDomainAndName(
    NS_ConvertUTF8toUTF16(docUriSpec), CPS_PREF_NAME, loadContext, nullptr);
}

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsEditorSpellCheck)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsEditorSpellCheck)

NS_INTERFACE_MAP_BEGIN(nsEditorSpellCheck)
  NS_INTERFACE_MAP_ENTRY(nsIEditorSpellCheck)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEditorSpellCheck)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsEditorSpellCheck)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION(nsEditorSpellCheck,
                         mEditor,
                         mSpellChecker,
                         mTxtSrvFilter)

nsEditorSpellCheck::nsEditorSpellCheck()
  : mSuggestedWordIndex(0)
  , mDictionaryIndex(0)
  , mEditor(nullptr)
  , mDictionaryFetcherGroup(0)
  , mUpdateDictionaryRunning(false)
{
}

nsEditorSpellCheck::~nsEditorSpellCheck()
{
  
  
  mSpellChecker = nullptr;
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


class CallbackCaller MOZ_FINAL : public nsRunnable
{
public:
  explicit CallbackCaller(nsIEditorSpellCheckCallback* aCallback)
    : mCallback(aCallback) {}

  ~CallbackCaller()
  {
    Run();
  }

  NS_IMETHOD Run()
  {
    if (mCallback) {
      mCallback->EditorSpellCheckDone();
      mCallback = nullptr;
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIEditorSpellCheckCallback> mCallback;
};

NS_IMETHODIMP    
nsEditorSpellCheck::InitSpellChecker(nsIEditor* aEditor, bool aEnableSelectionChecking, nsIEditorSpellCheckCallback* aCallback)
{
  NS_ENSURE_TRUE(aEditor, NS_ERROR_NULL_POINTER);
  mEditor = aEditor;

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

    int32_t count = 0;

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

  
  
  rv = UpdateCurrentDictionary(aCallback);
  if (NS_FAILED(rv) && aCallback) {
    
    
    
    nsRefPtr<CallbackCaller> caller = new CallbackCaller(aCallback);
    NS_ENSURE_STATE(caller);
    rv = NS_DispatchToMainThread(caller);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetNextMisspelledWord(char16_t **aNextMisspelledWord)
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
nsEditorSpellCheck::GetSuggestedWord(char16_t **aSuggestedWord)
{
  nsAutoString word;
  if ( mSuggestedWordIndex < int32_t(mSuggestedWordList.Length()))
  {
    *aSuggestedWord = ToNewUnicode(mSuggestedWordList[mSuggestedWordIndex]);
    mSuggestedWordIndex++;
  } else {
    
    *aSuggestedWord = ToNewUnicode(EmptyString());
  }
  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::CheckCurrentWord(const char16_t *aSuggestedWord,
                                     bool *aIsMisspelled)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  DeleteSuggestedWordList();
  return mSpellChecker->CheckWord(nsDependentString(aSuggestedWord),
                                  aIsMisspelled, &mSuggestedWordList);
}

NS_IMETHODIMP    
nsEditorSpellCheck::CheckCurrentWordNoSuggest(const char16_t *aSuggestedWord,
                                              bool *aIsMisspelled)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->CheckWord(nsDependentString(aSuggestedWord),
                                  aIsMisspelled, nullptr);
}

NS_IMETHODIMP    
nsEditorSpellCheck::ReplaceWord(const char16_t *aMisspelledWord,
                                const char16_t *aReplaceWord,
                                bool             allOccurrences)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->Replace(nsDependentString(aMisspelledWord),
                                nsDependentString(aReplaceWord), allOccurrences);
}

NS_IMETHODIMP    
nsEditorSpellCheck::IgnoreWordAllOccurrences(const char16_t *aWord)
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
nsEditorSpellCheck::GetPersonalDictionaryWord(char16_t **aDictionaryWord)
{
  if ( mDictionaryIndex < int32_t( mDictionaryList.Length()))
  {
    *aDictionaryWord = ToNewUnicode(mDictionaryList[mDictionaryIndex]);
    mDictionaryIndex++;
  } else {
    
    *aDictionaryWord = ToNewUnicode(EmptyString());
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsEditorSpellCheck::AddWordToDictionary(const char16_t *aWord)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->AddWordToPersonalDictionary(nsDependentString(aWord));
}

NS_IMETHODIMP    
nsEditorSpellCheck::RemoveWordFromDictionary(const char16_t *aWord)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  return mSpellChecker->RemoveWordFromPersonalDictionary(nsDependentString(aWord));
}

NS_IMETHODIMP    
nsEditorSpellCheck::GetDictionaryList(char16_t ***aDictionaryList, uint32_t *aCount)
{
  NS_ENSURE_TRUE(mSpellChecker, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_TRUE(aDictionaryList && aCount, NS_ERROR_NULL_POINTER);

  *aDictionaryList = 0;
  *aCount          = 0;

  nsTArray<nsString> dictList;

  nsresult rv = mSpellChecker->GetDictionaryList(&dictList);

  NS_ENSURE_SUCCESS(rv, rv);

  char16_t **tmpPtr = 0;

  if (dictList.Length() < 1)
  {
    
    

    tmpPtr = (char16_t **)nsMemory::Alloc(sizeof(char16_t *));

    NS_ENSURE_TRUE(tmpPtr, NS_ERROR_OUT_OF_MEMORY);

    *tmpPtr          = 0;
    *aDictionaryList = tmpPtr;
    *aCount          = 0;

    return NS_OK;
  }

  tmpPtr = (char16_t **)nsMemory::Alloc(sizeof(char16_t *) * dictList.Length());

  NS_ENSURE_TRUE(tmpPtr, NS_ERROR_OUT_OF_MEMORY);

  *aDictionaryList = tmpPtr;
  *aCount          = dictList.Length();

  uint32_t i;

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

  nsRefPtr<nsEditorSpellCheck> kungFuDeathGrip = this;

  
  
  
  if (!mUpdateDictionaryRunning) {

    
    mDictionaryFetcherGroup++;

    nsDefaultStringComparator comparator;
    nsAutoString langCode;
    int32_t dashIdx = aDictionary.FindChar('-');
    if (dashIdx != -1) {
      langCode.Assign(Substring(aDictionary, 0, dashIdx));
    } else {
      langCode.Assign(aDictionary);
    }

    if (mPreferredLang.IsEmpty() || !nsStyleUtil::DashMatchCompare(mPreferredLang, langCode, comparator)) {
      
      
      StoreCurrentDictionary(mEditor, aDictionary);
    } else {
      
      
      ClearCurrentDictionary(mEditor);
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
nsEditorSpellCheck::UpdateCurrentDictionary(nsIEditorSpellCheckCallback* aCallback)
{
  nsresult rv;

  nsRefPtr<nsEditorSpellCheck> kungFuDeathGrip = this;

  
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

  DictionaryFetcher* fetcher = new DictionaryFetcher(this, aCallback,
                                                     mDictionaryFetcherGroup);
  rootContent->GetLang(fetcher->mRootContentLang);
  nsCOMPtr<nsIDocument> doc = rootContent->GetCurrentDoc();
  NS_ENSURE_STATE(doc);
  doc->GetContentLanguage(fetcher->mRootDocContentLang);

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    
    
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethodWithArg<uint16_t>(fetcher, &DictionaryFetcher::HandleCompletion, 0);
    NS_DispatchToMainThread(runnable);
  } else {
    rv = fetcher->Fetch(mEditor);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsEditorSpellCheck::DictionaryFetched(DictionaryFetcher* aFetcher)
{
  nsRefPtr<nsEditorSpellCheck> kungFuDeathGrip = this;

  nsresult rv = NS_OK;

  
  
  CallbackCaller callbackCaller(aFetcher->mCallback);
  UpdateDictionnaryHolder holder(this);

  if (aFetcher->mGroup < mDictionaryFetcherGroup) {
    
    
    return NS_OK;
  }

  nsAutoString dictName;
  uint32_t flags;
  mEditor->GetFlags(&flags);
  
  if (!(flags & nsIPlaintextEditor::eEditorMailMask)) {
    mPreferredLang.Assign(aFetcher->mRootContentLang);

    
    
    nsAutoString dictName;
    dictName.Assign(aFetcher->mDictionary);
    if (!dictName.IsEmpty()) {
      if (NS_FAILED(SetCurrentDictionary(dictName))) { 
        
        ClearCurrentDictionary(mEditor);
      }
      return NS_OK;
    }

    if (mPreferredLang.IsEmpty()) {
      mPreferredLang.Assign(aFetcher->mRootDocContentLang);
    }

    
    if (!mPreferredLang.IsEmpty()) {
      dictName.Assign(mPreferredLang);
    }
  }

  
  nsAutoString preferedDict(Preferences::GetLocalizedString("spellchecker.dictionary"));
  
  
  int32_t underScore = preferedDict.FindChar('_');
  if (underScore != -1) {
    preferedDict.Replace(underScore, 1, '-');
  }
  if (dictName.IsEmpty()) {
    dictName.Assign(preferedDict);
  }

  if (dictName.IsEmpty())
  {
    
    

    nsCOMPtr<nsIXULChromeRegistry> packageRegistry =
      mozilla::services::GetXULChromeRegistryService();

    if (packageRegistry) {
      nsAutoCString utf8DictName;
      rv = packageRegistry->GetSelectedLocale(NS_LITERAL_CSTRING("global"),
                                              utf8DictName);
      AppendUTF8toUTF16(utf8DictName, dictName);
    }
  }

  if (NS_SUCCEEDED(rv) && !dictName.IsEmpty()) {
    rv = SetCurrentDictionary(dictName);
    if (NS_FAILED(rv)) {
      
      

      nsAutoString langCode;
      int32_t dashIdx = dictName.FindChar('-');
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
        int32_t i, count = dictList.Length();
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
      
      char* env_lang = getenv("LANG");
      if (env_lang != nullptr) {
        nsString lang = NS_ConvertUTF8toUTF16(env_lang);
        
        int32_t dot_pos = lang.FindChar('.');
        if (dot_pos != -1) {
          lang = Substring(lang, 0, dot_pos - 1);
        }
        
        int32_t underScore = lang.FindChar('_');
        if (underScore != -1) {
          lang.Replace(underScore, 1, '-');
        }
        rv = SetCurrentDictionary(lang);
      }
      if (NS_FAILED(rv)) {
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
  }

  
  
  
  

  DeleteSuggestedWordList();

  return NS_OK;
}
