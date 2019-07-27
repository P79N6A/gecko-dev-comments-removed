




#include "mozPersonalDictionary.h"
#include "nsIUnicharInputStream.h"
#include "nsReadableUtils.h"
#include "nsIFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIWeakReference.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsStringEnumerator.h"
#include "nsUnicharInputStream.h"
#include "nsIRunnable.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"

#define MOZ_PERSONAL_DICT_NAME "persdict.dat"












NS_IMPL_CYCLE_COLLECTING_ADDREF(mozPersonalDictionary)
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozPersonalDictionary)

NS_INTERFACE_MAP_BEGIN(mozPersonalDictionary)
  NS_INTERFACE_MAP_ENTRY(mozIPersonalDictionary)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, mozIPersonalDictionary)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(mozPersonalDictionary)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION(mozPersonalDictionary, mEncoder)

class mozPersonalDictionaryLoader MOZ_FINAL : public nsRunnable
{
public:
  explicit mozPersonalDictionaryLoader(mozPersonalDictionary *dict) : mDict(dict)
  {
  }

  NS_IMETHOD Run()
  {
    mDict->SyncLoad();

    
    mozPersonalDictionary *dict;
    mDict.forget(&dict);

    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    if (mainThread) {
      NS_ProxyRelease(mainThread, static_cast<mozIPersonalDictionary *>(dict));
    } else {
      
      NS_WARNING("Cannot get main thread, leaking mozPersonalDictionary.");
    }

    return NS_OK;
  }

private:
  nsRefPtr<mozPersonalDictionary> mDict;
};

mozPersonalDictionary::mozPersonalDictionary()
 : mDirty(false),
   mIsLoaded(false),
   mMonitor("mozPersonalDictionary::mMonitor")
{
}

mozPersonalDictionary::~mozPersonalDictionary()
{
}

nsresult mozPersonalDictionary::Init()
{
  nsCOMPtr<nsIObserverService> svc =
    do_GetService("@mozilla.org/observer-service;1");

  NS_ENSURE_STATE(svc);
  
  nsresult rv = svc->AddObserver(this, "profile-do-change", true);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = svc->AddObserver(this, "profile-before-change", true);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  Load();

  return NS_OK;
}

void mozPersonalDictionary::WaitForLoad()
{
  if (mIsLoaded) {
    return;
  }

  mozilla::MonitorAutoLock mon(mMonitor);

  if (!mIsLoaded) {
    mon.Wait();
  }
}

nsresult mozPersonalDictionary::LoadInternal()
{
  nsresult rv;
  mozilla::MonitorAutoLock mon(mMonitor);

  if (mIsLoaded) {
    return NS_OK;
  }

  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!mFile) {
    return NS_ERROR_FAILURE;
  }

  rv = mFile->Append(NS_LITERAL_STRING(MOZ_PERSONAL_DICT_NAME));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIEventTarget> target = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsCOMPtr<nsIRunnable> runnable = new mozPersonalDictionaryLoader(this);
  rv = target->Dispatch(runnable, NS_DISPATCH_NORMAL);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP mozPersonalDictionary::Load()
{
  nsresult rv = LoadInternal();

  if (NS_FAILED(rv)) {
    mIsLoaded = true;
  }

  return rv;
}

void mozPersonalDictionary::SyncLoad()
{
  MOZ_ASSERT(!NS_IsMainThread());

  mozilla::MonitorAutoLock mon(mMonitor);

  if (mIsLoaded) {
    return;
  }

  SyncLoadInternal();
  mIsLoaded = true;
  mon.Notify();
}

void mozPersonalDictionary::SyncLoadInternal()
{
  MOZ_ASSERT(!NS_IsMainThread());

  
  nsresult rv;
  bool dictExists;

  rv = mFile->Exists(&dictExists);
  if (NS_FAILED(rv)) {
    return;
  }

  if (!dictExists) {
    
    return;
  }

  nsCOMPtr<nsIInputStream> inStream;
  NS_NewLocalFileInputStream(getter_AddRefs(inStream), mFile);

  nsCOMPtr<nsIUnicharInputStream> convStream;
  rv = nsSimpleUnicharStreamFactory::GetInstance()->
    CreateInstanceFromUTF8Stream(inStream, getter_AddRefs(convStream));
  if (NS_FAILED(rv)) {
    return;
  }

  
  mDictionaryTable.Clear();

  char16_t c;
  uint32_t nRead;
  bool done = false;
  do{  
    if( (NS_OK != convStream->Read(&c, 1, &nRead)) || (nRead != 1)) break;
    while(!done && ((c == '\n') || (c == '\r'))){
      if( (NS_OK != convStream->Read(&c, 1, &nRead)) || (nRead != 1)) done = true;
    }
    if (!done){ 
      nsAutoString word;
      while((c != '\n') && (c != '\r') && !done){
        word.Append(c);
        if( (NS_OK != convStream->Read(&c, 1, &nRead)) || (nRead != 1)) done = true;
      }
      mDictionaryTable.PutEntry(word.get());
    }
  } while(!done);
  mDirty = false;
}




static PLDHashOperator
AddHostToStringArray(nsUnicharPtrHashKey *aEntry, void *aArg)
{
  static_cast<nsTArray<nsString>*>(aArg)->AppendElement(nsDependentString(aEntry->GetKey()));
  return PL_DHASH_NEXT;
}


NS_IMETHODIMP mozPersonalDictionary::Save()
{
  nsCOMPtr<nsIFile> theFile;
  nsresult res;

  WaitForLoad();
  if(!mDirty) return NS_OK;

  
  res = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(theFile));
  if(NS_FAILED(res)) return res;
  if(!theFile)return NS_ERROR_FAILURE;
  res = theFile->Append(NS_LITERAL_STRING(MOZ_PERSONAL_DICT_NAME));
  if(NS_FAILED(res)) return res;

  nsCOMPtr<nsIOutputStream> outStream;
  NS_NewSafeLocalFileOutputStream(getter_AddRefs(outStream), theFile, PR_CREATE_FILE | PR_WRONLY | PR_TRUNCATE ,0664);

  
  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  res = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream), outStream, 4096);
  if (NS_FAILED(res)) return res;

  nsTArray<nsString> array(mDictionaryTable.Count());
  mDictionaryTable.EnumerateEntries(AddHostToStringArray, &array);

  uint32_t bytesWritten;
  nsAutoCString utf8Key;
  for (uint32_t i = 0; i < array.Length(); ++i ) {
    CopyUTF16toUTF8(array[i], utf8Key);

    bufferedOutputStream->Write(utf8Key.get(), utf8Key.Length(), &bytesWritten);
    bufferedOutputStream->Write("\n", 1, &bytesWritten);
  }
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(bufferedOutputStream);
  NS_ASSERTION(safeStream, "expected a safe output stream!");
  if (safeStream) {
    res = safeStream->Finish();
    if (NS_FAILED(res)) {
      NS_WARNING("failed to save personal dictionary file! possible data loss");
    }
  }
  return res;
}


NS_IMETHODIMP mozPersonalDictionary::GetWordList(nsIStringEnumerator **aWords)
{
  NS_ENSURE_ARG_POINTER(aWords);
  *aWords = nullptr;

  WaitForLoad();

  nsTArray<nsString> *array = new nsTArray<nsString>(mDictionaryTable.Count());
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  mDictionaryTable.EnumerateEntries(AddHostToStringArray, array);

  array->Sort();

  return NS_NewAdoptingStringEnumerator(aWords, array);
}


NS_IMETHODIMP mozPersonalDictionary::Check(const char16_t *aWord, const char16_t *aLanguage, bool *aResult)
{
  NS_ENSURE_ARG_POINTER(aWord);
  NS_ENSURE_ARG_POINTER(aResult);

  WaitForLoad();

  *aResult = (mDictionaryTable.GetEntry(aWord) || mIgnoreTable.GetEntry(aWord));
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::AddWord(const char16_t *aWord, const char16_t *aLang)
{
  WaitForLoad();

  mDictionaryTable.PutEntry(aWord);
  mDirty = true;
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::RemoveWord(const char16_t *aWord, const char16_t *aLang)
{
  WaitForLoad();

  mDictionaryTable.RemoveEntry(aWord);
  mDirty = true;
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::IgnoreWord(const char16_t *aWord)
{
  
  if (aWord && !mIgnoreTable.GetEntry(aWord)) 
    mIgnoreTable.PutEntry(aWord);
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::EndSession()
{
  WaitForLoad();

  Save(); 
  mIgnoreTable.Clear();
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::AddCorrection(const char16_t *word, const char16_t *correction, const char16_t *lang)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozPersonalDictionary::RemoveCorrection(const char16_t *word, const char16_t *correction, const char16_t *lang)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozPersonalDictionary::GetCorrection(const char16_t *word, char16_t ***words, uint32_t *count)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozPersonalDictionary::Observe(nsISupports *aSubject, const char *aTopic, const char16_t *aData)
{
  if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    
    
    WaitForLoad();
    mIsLoaded = false;
    Load(); 
  } else if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    Save();
  }

  return NS_OK;
}
