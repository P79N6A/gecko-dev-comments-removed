




#include "mozPersonalDictionary.h"
#include "nsIUnicharInputStream.h"
#include "nsReadableUtils.h"
#include "nsIFile.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsICharsetConverterManager.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIWeakReference.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsStringEnumerator.h"
#include "nsUnicharInputStream.h"

#define MOZ_PERSONAL_DICT_NAME "persdict.dat"

const int kMaxWordLen=256;













NS_IMPL_CYCLE_COLLECTING_ADDREF(mozPersonalDictionary)
NS_IMPL_CYCLE_COLLECTING_RELEASE(mozPersonalDictionary)

NS_INTERFACE_MAP_BEGIN(mozPersonalDictionary)
  NS_INTERFACE_MAP_ENTRY(mozIPersonalDictionary)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, mozIPersonalDictionary)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(mozPersonalDictionary)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_1(mozPersonalDictionary, mEncoder)

mozPersonalDictionary::mozPersonalDictionary()
 : mDirty(false)
{
}

mozPersonalDictionary::~mozPersonalDictionary()
{
}

nsresult mozPersonalDictionary::Init()
{
  mDictionaryTable.Init();
  mIgnoreTable.Init();

  nsresult rv;
  nsCOMPtr<nsIObserverService> svc = 
           do_GetService("@mozilla.org/observer-service;1", &rv);
   
  if (NS_SUCCEEDED(rv) && svc) 
    rv = svc->AddObserver(this, "profile-do-change", true); 

  if (NS_FAILED(rv)) return rv;

  Load();
  
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::Load()
{
  
  nsresult res;
  nsCOMPtr<nsIFile> theFile;
  bool dictExists;

  res = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(theFile));
  if(NS_FAILED(res)) return res;
  if(!theFile)return NS_ERROR_FAILURE;
  res = theFile->Append(NS_LITERAL_STRING(MOZ_PERSONAL_DICT_NAME));
  if(NS_FAILED(res)) return res;
  res = theFile->Exists(&dictExists);
  if(NS_FAILED(res)) return res;

  if (!dictExists) {
    
    return NS_OK;
  }
  
  nsCOMPtr<nsIInputStream> inStream;
  NS_NewLocalFileInputStream(getter_AddRefs(inStream), theFile);

  nsCOMPtr<nsIUnicharInputStream> convStream;
  res = nsSimpleUnicharStreamFactory::GetInstance()->
    CreateInstanceFromUTF8Stream(inStream, getter_AddRefs(convStream));
  if(NS_FAILED(res)) return res;
  
  
  mDictionaryTable.Clear();

  PRUnichar c;
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
  
  return res;
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

  if(!mDirty) return NS_OK;

  
  res = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(theFile));
  if(NS_FAILED(res)) return res;
  if(!theFile)return NS_ERROR_FAILURE;
  res = theFile->Append(NS_LITERAL_STRING(MOZ_PERSONAL_DICT_NAME));
  if(NS_FAILED(res)) return res;

  nsCOMPtr<nsIOutputStream> outStream;
  NS_NewLocalFileOutputStream(getter_AddRefs(outStream), theFile, PR_CREATE_FILE | PR_WRONLY | PR_TRUNCATE ,0664);

  
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
  return res;
}


NS_IMETHODIMP mozPersonalDictionary::GetWordList(nsIStringEnumerator **aWords)
{
  NS_ENSURE_ARG_POINTER(aWords);
  *aWords = nullptr;

  nsTArray<nsString> *array = new nsTArray<nsString>(mDictionaryTable.Count());
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  mDictionaryTable.EnumerateEntries(AddHostToStringArray, array);

  array->Sort();

  return NS_NewAdoptingStringEnumerator(aWords, array);
}


NS_IMETHODIMP mozPersonalDictionary::Check(const PRUnichar *aWord, const PRUnichar *aLanguage, bool *aResult)
{
  NS_ENSURE_ARG_POINTER(aWord);
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = (mDictionaryTable.GetEntry(aWord) || mIgnoreTable.GetEntry(aWord));
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::AddWord(const PRUnichar *aWord, const PRUnichar *aLang)
{
  mDictionaryTable.PutEntry(aWord);
  mDirty = true;
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::RemoveWord(const PRUnichar *aWord, const PRUnichar *aLang)
{
  mDictionaryTable.RemoveEntry(aWord);
  mDirty = true;
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::IgnoreWord(const PRUnichar *aWord)
{
  
  if (aWord && !mIgnoreTable.GetEntry(aWord)) 
    mIgnoreTable.PutEntry(aWord);
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::EndSession()
{
  Save(); 
  mIgnoreTable.Clear();
  return NS_OK;
}


NS_IMETHODIMP mozPersonalDictionary::AddCorrection(const PRUnichar *word, const PRUnichar *correction, const PRUnichar *lang)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozPersonalDictionary::RemoveCorrection(const PRUnichar *word, const PRUnichar *correction, const PRUnichar *lang)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozPersonalDictionary::GetCorrection(const PRUnichar *word, PRUnichar ***words, uint32_t *count)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP mozPersonalDictionary::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    Load();  
  }

  return NS_OK;
}

