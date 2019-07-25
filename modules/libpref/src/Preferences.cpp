






































#include "mozilla/dom/ContentChild.h"
#include "nsXULAppAPI.h"

#include "mozilla/Preferences.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"
#include "nsNetUtil.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsIStringEnumerator.h"
#include "nsIZipReader.h"
#include "nsPrefBranch.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"
#include "nsCOMArray.h"
#include "nsXPCOMCID.h"
#include "nsAutoPtr.h"

#include "nsQuickSort.h"
#include "prmem.h"
#include "pldhash.h"

#include "prefapi.h"
#include "prefread.h"
#include "prefapi_private_data.h"
#include "PrefTuple.h"

#include "nsITimelineService.h"

#include "mozilla/Omnijar.h"
#include "nsZipArchive.h"

#include "nsTArray.h"
#include "nsRefPtrHashtable.h"

namespace mozilla {


#define INITIAL_PREF_FILES 10
static NS_DEFINE_CID(kZipReaderCID, NS_ZIPREADER_CID);


static nsresult openPrefFile(nsIFile* aFile);
static nsresult pref_InitInitialObjects(void);
static nsresult pref_LoadPrefsInDirList(const char *listId);

Preferences* Preferences::sPreferences = nsnull;
nsIPrefBranch2* Preferences::sRootBranch = nsnull;
nsIPrefBranch* Preferences::sDefaultRootBranch = nsnull;
PRBool Preferences::sShutdown = PR_FALSE;

class ValueObserverHashKey : public PLDHashEntryHdr {
public:
  typedef ValueObserverHashKey* KeyType;
  typedef const ValueObserverHashKey* KeyTypePointer;

  static const ValueObserverHashKey* KeyToPointer(ValueObserverHashKey *aKey)
  {
    return aKey;
  }

  static PLDHashNumber HashKey(const ValueObserverHashKey *aKey)
  {
    PRUint32 strHash = nsCRT::HashCode(aKey->mPrefName.BeginReading(),
                                       aKey->mPrefName.Length());
    return PR_ROTATE_LEFT32(strHash, 4) ^ NS_PTR_TO_UINT32(aKey->mCallback);
  }

  ValueObserverHashKey(const char *aPref, PrefChangedFunc aCallback) :
    mPrefName(aPref), mCallback(aCallback) { }

  ValueObserverHashKey(const ValueObserverHashKey *aOther) :
    mPrefName(aOther->mPrefName), mCallback(aOther->mCallback)
  { }

  PRBool KeyEquals(const ValueObserverHashKey *aOther) const
  {
    return mCallback == aOther->mCallback && mPrefName == aOther->mPrefName;
  }

  ValueObserverHashKey *GetKey() const
  {
    return const_cast<ValueObserverHashKey*>(this);
  }

  enum { ALLOW_MEMMOVE = PR_TRUE };

  nsCString mPrefName;
  PrefChangedFunc mCallback;
};

class ValueObserver : public nsIObserver,
                      public ValueObserverHashKey
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  ValueObserver(const char *aPref, PrefChangedFunc aCallback)
    : ValueObserverHashKey(aPref, aCallback) { }

  ~ValueObserver() {
    Preferences::RemoveObserver(this, mPrefName.get());
  }

  void AppendClosure(void *aClosure) {
    mClosures.AppendElement(aClosure);
  }

  void RemoveClosure(void *aClosure) {
    mClosures.RemoveElement(aClosure);
  }

  PRBool HasNoClosures() {
    return mClosures.Length() == 0;
  }

  nsTArray<void*> mClosures;
};

NS_IMPL_ISUPPORTS1(ValueObserver, nsIObserver)

NS_IMETHODIMP
ValueObserver::Observe(nsISupports     *aSubject,
                       const char      *aTopic,
                       const PRUnichar *aData)
{
  NS_ASSERTION(!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID),
               "invalid topic");
  NS_ConvertUTF16toUTF8 data(aData);
  for (PRUint32 i = 0; i < mClosures.Length(); i++) {
    mCallback(data.get(), mClosures.ElementAt(i));
  }

  return NS_OK;
}

struct CacheData {
  void* cacheLocation;
  union {
    PRBool defaultValueBool;
    PRInt32 defaultValueInt;
    PRUint32 defaultValueUint;
  };
};

static nsTArray<nsAutoPtr<CacheData> >* gCacheData = nsnull;
static nsRefPtrHashtable<ValueObserverHashKey,
                         ValueObserver>* gObserverTable = nsnull;


Preferences*
Preferences::GetInstanceForService()
{
  if (sPreferences) {
    NS_ADDREF(sPreferences);
    return sPreferences;
  }

  NS_ENSURE_TRUE(!sShutdown, nsnull);

  InitStaticMembers(PR_TRUE);
  NS_IF_ADDREF(sPreferences);
  return sPreferences;
}


PRBool
Preferences::InitStaticMembers(PRBool aForService)
{
  if (sShutdown || sPreferences) {
    return sPreferences != nsnull;
  }

  
  
  
  
  if (!aForService) {
    nsCOMPtr<nsIPrefService> prefService =
      do_GetService(NS_PREFSERVICE_CONTRACTID);
    return sPreferences != nsnull;
  }

  sRootBranch = new nsPrefBranch("", PR_FALSE);
  NS_ADDREF(sRootBranch);
  sDefaultRootBranch = new nsPrefBranch("", PR_TRUE);
  NS_ADDREF(sDefaultRootBranch);

  sPreferences = new Preferences();
  NS_ADDREF(sPreferences);

  if (NS_FAILED(sPreferences->Init())) {
    
    NS_RELEASE(sPreferences);
    return PR_FALSE;
  }

  gCacheData = new nsTArray<nsAutoPtr<CacheData> >();

  gObserverTable = new nsRefPtrHashtable<ValueObserverHashKey, ValueObserver>();
  gObserverTable->Init();

  return PR_TRUE;
}


void
Preferences::Shutdown()
{
  if (!sShutdown ) {
    sShutdown = PR_TRUE; 

    
    
    
    if (sPreferences) {
      sPreferences->Release();
    }
  }
}







Preferences::Preferences()
{
}

Preferences::~Preferences()
{
  NS_ASSERTION(sPreferences == this, "Isn't this the singleton instance?");

  delete gObserverTable;
  gObserverTable = nsnull;

  delete gCacheData;
  gCacheData = nsnull;

  NS_RELEASE(sRootBranch);
  NS_RELEASE(sDefaultRootBranch);

  sPreferences = nsnull;

  PREF_Cleanup();
}






NS_IMPL_THREADSAFE_ADDREF(Preferences)
NS_IMPL_THREADSAFE_RELEASE(Preferences)

NS_INTERFACE_MAP_BEGIN(Preferences)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPrefService)
    NS_INTERFACE_MAP_ENTRY(nsIPrefService)
    NS_INTERFACE_MAP_ENTRY(nsIPrefServiceInternal)
    NS_INTERFACE_MAP_ENTRY(nsIObserver)
    NS_INTERFACE_MAP_ENTRY(nsIPrefBranch)
    NS_INTERFACE_MAP_ENTRY(nsIPrefBranch2)
    NS_INTERFACE_MAP_ENTRY(nsIPrefBranchInternal)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END






nsresult
Preferences::Init()
{
  nsresult rv;

  rv = PREF_Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pref_InitInitialObjects();
  NS_ENSURE_SUCCESS(rv, rv);

  using mozilla::dom::ContentChild;
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    InfallibleTArray<PrefTuple> array;
    ContentChild::GetSingleton()->SendReadPrefsArray(&array);

    
    nsTArray<PrefTuple>::size_type index = array.Length();
    while (index-- > 0) {
      pref_SetPrefTuple(array[index], PR_TRUE);
    }
    return NS_OK;
  }

  nsXPIDLCString lockFileName;
  







  rv = sRootBranch->GetCharPref("general.config.filename", getter_Copies(lockFileName));
  if (NS_SUCCEEDED(rv))
    NS_CreateServicesFromCategory("pref-config-startup",
                                  static_cast<nsISupports *>(static_cast<void *>(this)),
                                  "pref-config-startup");    

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  rv = observerService->AddObserver(this, "profile-before-change", PR_TRUE);

  if (NS_SUCCEEDED(rv))
    rv = observerService->AddObserver(this, "profile-do-change", PR_TRUE);

  observerService->AddObserver(this, "load-extension-defaults", PR_TRUE);

  return(rv);
}

NS_IMETHODIMP
Preferences::Observe(nsISupports *aSubject, const char *aTopic,
                     const PRUnichar *someData)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content)
    return NS_ERROR_NOT_AVAILABLE;

  nsresult rv = NS_OK;

  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("shutdown-cleanse").get())) {
      if (mCurrentFile) {
        mCurrentFile->Remove(PR_FALSE);
        mCurrentFile = nsnull;
      }
    } else {
      rv = SavePrefFile(nsnull);
    }
  } else if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    ResetUserPrefs();
    rv = ReadUserPrefs(nsnull);
  } else if (!strcmp(aTopic, "load-extension-defaults")) {
    pref_LoadPrefsInDirList(NS_EXT_PREFS_DEFAULTS_DIR_LIST);
  } else if (!nsCRT::strcmp(aTopic, "reload-default-prefs")) {
    
    pref_InitInitialObjects();
  }
  return rv;
}


NS_IMETHODIMP
Preferences::ReadUserPrefs(nsIFile *aFile)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    NS_ERROR("cannot load prefs from content process");
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv;

  if (nsnull == aFile) {
    rv = UseDefaultPrefFile();
    UseUserPrefFile();

    NotifyServiceObservers(NS_PREFSERVICE_READ_TOPIC_ID);

  } else {
    rv = ReadAndOwnUserPrefFile(aFile);
  }
  return rv;
}

NS_IMETHODIMP
Preferences::ResetPrefs()
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    NS_ERROR("cannot set prefs from content process");
    return NS_ERROR_NOT_AVAILABLE;
  }

  NotifyServiceObservers(NS_PREFSERVICE_RESET_TOPIC_ID);
  PREF_CleanupPrefs();

  nsresult rv = PREF_Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return pref_InitInitialObjects();
}

NS_IMETHODIMP
Preferences::ResetUserPrefs()
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    NS_ERROR("cannot set prefs from content process");
    return NS_ERROR_NOT_AVAILABLE;
  }

  PREF_ClearAllUserPrefs();
  return NS_OK;    
}

NS_IMETHODIMP
Preferences::SavePrefFile(nsIFile *aFile)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    NS_ERROR("cannot save prefs from content process");
    return NS_ERROR_NOT_AVAILABLE;
  }

  return SavePrefFileInternal(aFile);
}


NS_IMETHODIMP
Preferences::ReadExtensionPrefs(nsILocalFile *aFile)
{
  nsresult rv;
  nsCOMPtr<nsIZipReader> reader = do_CreateInstance(kZipReaderCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = reader->Open(aFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUTF8StringEnumerator> files;
  rv = reader->FindEntries("defaults/preferences/*.(J|j)(S|s)$",
                           getter_AddRefs(files));
  NS_ENSURE_SUCCESS(rv, rv);

  char buffer[4096];

  PRBool more;
  while (NS_SUCCEEDED(rv = files->HasMore(&more)) && more) {
    nsCAutoString entry;
    rv = files->GetNext(entry);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIInputStream> stream;
    rv = reader->GetInputStream(entry.get(), getter_AddRefs(stream));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 avail, read;

    PrefParseState ps;
    PREF_InitParseState(&ps, PREF_ReaderCallback, NULL);
    while (NS_SUCCEEDED(rv = stream->Available(&avail)) && avail) {
      rv = stream->Read(buffer, 4096, &read);
      if (NS_FAILED(rv)) {
        NS_WARNING("Pref stream read failed");
        break;
      }

      rv = PREF_ParseBuf(&ps, buffer, read);
      if (NS_FAILED(rv)) {
        NS_WARNING("Pref stream parse failed");
        break;
      }
    }
    PREF_FinalizeParseState(&ps);
  }
  return rv;
}

NS_IMETHODIMP
Preferences::PrefHasUserValue(const nsACString& aPrefName, PRBool* aHasValue)
{
  *aHasValue = PREF_HasUserPref(aPrefName.BeginReading());
  return NS_OK;
}

NS_IMETHODIMP
Preferences::SetPreference(const PrefTuple *aPref)
{
  return pref_SetPrefTuple(*aPref, PR_TRUE);
}

NS_IMETHODIMP
Preferences::ClearContentPref(const nsACString& aPrefName)
{
  return PREF_ClearUserPref(aPrefName.BeginReading());
}

NS_IMETHODIMP
Preferences::MirrorPreference(const nsACString& aPrefName, PrefTuple *aPref)
{
  PrefHashEntry *pref = pref_HashTableLookup(PromiseFlatCString(aPrefName).get());

  if (!pref)
    return NS_ERROR_NOT_AVAILABLE;

  pref_GetTupleFromEntry(pref, aPref);

  return NS_OK;
}

NS_IMETHODIMP
Preferences::MirrorPreferences(nsTArray<PrefTuple,
                                        nsTArrayInfallibleAllocator> *aArray)
{
  aArray->SetCapacity(PL_DHASH_TABLE_SIZE(&gHashTable));

  PL_DHashTableEnumerate(&gHashTable, pref_MirrorPrefs, aArray);

  return NS_OK;
}

NS_IMETHODIMP
Preferences::GetBranch(const char *aPrefRoot, nsIPrefBranch **_retval)
{
  nsresult rv;

  if ((nsnull != aPrefRoot) && (*aPrefRoot != '\0')) {
    
    nsPrefBranch* prefBranch = new nsPrefBranch(aPrefRoot, PR_FALSE);
    if (!prefBranch)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = CallQueryInterface(prefBranch, _retval);
  } else {
    
    rv = CallQueryInterface(sRootBranch, _retval);
  }
  return rv;
}

NS_IMETHODIMP
Preferences::GetDefaultBranch(const char *aPrefRoot, nsIPrefBranch **_retval)
{
  if (!aPrefRoot || !aPrefRoot[0]) {
    return CallQueryInterface(sDefaultRootBranch, _retval);
  }

  
  nsPrefBranch* prefBranch = new nsPrefBranch(aPrefRoot, PR_TRUE);
  if (!prefBranch)
    return NS_ERROR_OUT_OF_MEMORY;

  return CallQueryInterface(prefBranch, _retval);
}


nsresult
Preferences::NotifyServiceObservers(const char *aTopic)
{
  nsCOMPtr<nsIObserverService> observerService = 
    mozilla::services::GetObserverService();  
  if (!observerService)
    return NS_ERROR_FAILURE;

  nsISupports *subject = (nsISupports *)((nsIPrefService *)this);
  observerService->NotifyObservers(subject, aTopic, nsnull);
  
  return NS_OK;
}

nsresult
Preferences::UseDefaultPrefFile()
{
  nsresult rv, rv2;
  nsCOMPtr<nsIFile> aFile;

  rv = NS_GetSpecialDirectory(NS_APP_PREFS_50_FILE, getter_AddRefs(aFile));
  if (NS_SUCCEEDED(rv)) {
    rv = ReadAndOwnUserPrefFile(aFile);
    
    
    
    if (NS_FAILED(rv)) {
      rv2 = SavePrefFileInternal(aFile);
      NS_ASSERTION(NS_SUCCEEDED(rv2), "Failed to save new shared pref file");
    }
  }
  
  return rv;
}

nsresult
Preferences::UseUserPrefFile()
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIFile> aFile;
  nsDependentCString prefsDirProp(NS_APP_PREFS_50_DIR);

  rv = NS_GetSpecialDirectory(prefsDirProp.get(), getter_AddRefs(aFile));
  if (NS_SUCCEEDED(rv) && aFile) {
    rv = aFile->AppendNative(NS_LITERAL_CSTRING("user.js"));
    if (NS_SUCCEEDED(rv)) {
      PRBool exists = PR_FALSE;
      aFile->Exists(&exists);
      if (exists) {
        rv = openPrefFile(aFile);
      } else {
        rv = NS_ERROR_FILE_NOT_FOUND;
      }
    }
  }
  return rv;
}

nsresult
Preferences::MakeBackupPrefFile(nsIFile *aFile)
{
  
  
  nsAutoString newFilename;
  nsresult rv = aFile->GetLeafName(newFilename);
  NS_ENSURE_SUCCESS(rv, rv);
  newFilename.Insert(NS_LITERAL_STRING("Invalid"), 0);
  nsCOMPtr<nsIFile> newFile;
  rv = aFile->GetParent(getter_AddRefs(newFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = newFile->Append(newFilename);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool exists = PR_FALSE;
  newFile->Exists(&exists);
  if (exists) {
    rv = newFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = aFile->CopyTo(nsnull, newFilename);
  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

nsresult
Preferences::ReadAndOwnUserPrefFile(nsIFile *aFile)
{
  NS_ENSURE_ARG(aFile);
  
  if (mCurrentFile == aFile)
    return NS_OK;
  mCurrentFile = aFile;

  nsresult rv = NS_OK;
  PRBool exists = PR_FALSE;
  mCurrentFile->Exists(&exists);
  if (exists) {
    rv = openPrefFile(mCurrentFile);
    if (NS_FAILED(rv)) {
      
      
      
      MakeBackupPrefFile(mCurrentFile);
    }
  } else {
    rv = NS_ERROR_FILE_NOT_FOUND;
  }

  return rv;
}

nsresult
Preferences::SavePrefFileInternal(nsIFile *aFile)
{
  if (nsnull == aFile) {
    
    
    if (!gDirty)
      return NS_OK;
    
    
    nsresult rv = NS_OK;
    if (mCurrentFile)
      rv = WritePrefFile(mCurrentFile);

    return rv;
  } else {
    return WritePrefFile(aFile);
  }
}

nsresult
Preferences::WritePrefFile(nsIFile* aFile)
{
  const char                outHeader[] =
    "# Mozilla User Preferences"
    NS_LINEBREAK
    NS_LINEBREAK
    "













"
    NS_LINEBREAK
    NS_LINEBREAK;

  nsCOMPtr<nsIOutputStream> outStreamSink;
  nsCOMPtr<nsIOutputStream> outStream;
  PRUint32                  writeAmount;
  nsresult                  rv;

  if (!gHashTable.ops)
    return NS_ERROR_NOT_INITIALIZED;

  
  rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(outStreamSink),
                                       aFile,
                                       -1,
                                       0600);
  if (NS_FAILED(rv)) 
      return rv;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(outStream), outStreamSink, 4096);
  if (NS_FAILED(rv)) 
      return rv;  

  char** valueArray = (char **)PR_Calloc(sizeof(char *), gHashTable.entryCount);
  if (!valueArray)
    return NS_ERROR_OUT_OF_MEMORY;
  
  pref_saveArgs saveArgs;
  saveArgs.prefArray = valueArray;
  saveArgs.saveTypes = SAVE_ALL;
  
  
  PL_DHashTableEnumerate(&gHashTable, pref_savePref, &saveArgs);
    
  
  NS_QuickSort(valueArray, gHashTable.entryCount, sizeof(char *), pref_CompareStrings, NULL);
  
  
  outStream->Write(outHeader, sizeof(outHeader) - 1, &writeAmount);

  char** walker = valueArray;
  for (PRUint32 valueIdx = 0; valueIdx < gHashTable.entryCount; valueIdx++, walker++) {
    if (*walker) {
      outStream->Write(*walker, strlen(*walker), &writeAmount);
      outStream->Write(NS_LINEBREAK, NS_LINEBREAK_LEN, &writeAmount);
      NS_Free(*walker);
    }
  }
  PR_Free(valueArray);

  
  
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(outStream);
  NS_ASSERTION(safeStream, "expected a safe output stream!");
  if (safeStream) {
    rv = safeStream->Finish();
    if (NS_FAILED(rv)) {
      NS_WARNING("failed to save prefs file! possible dataloss");
      return rv;
    }
  }

  gDirty = PR_FALSE;
  return NS_OK;
}

static nsresult openPrefFile(nsIFile* aFile)
{
  nsCOMPtr<nsIInputStream> inStr;

#if MOZ_TIMELINE
  {
    nsCAutoString str;
    aFile->GetNativePath(str);
    NS_TIMELINE_MARK_FUNCTION1("load pref file", str.get());
  }
#endif

  nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(inStr), aFile);
  if (NS_FAILED(rv)) 
    return rv;        

  PRUint32 fileSize;
  rv = inStr->Available(&fileSize);
  if (NS_FAILED(rv))
    return rv;

  nsAutoArrayPtr<char> fileBuffer(new char[fileSize]);
  if (fileBuffer == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  PrefParseState ps;
  PREF_InitParseState(&ps, PREF_ReaderCallback, NULL);

  
  
  nsresult rv2 = NS_OK;
  for (;;) {
    PRUint32 amtRead = 0;
    rv = inStr->Read((char*)fileBuffer, fileSize, &amtRead);
    if (NS_FAILED(rv) || amtRead == 0)
      break;
    if (!PREF_ParseBuf(&ps, fileBuffer, amtRead))
      rv2 = NS_ERROR_FILE_CORRUPTED;
  }

  PREF_FinalizeParseState(&ps);

  return NS_FAILED(rv) ? rv : rv2;
}





static int
pref_CompareFileNames(nsIFile* aFile1, nsIFile* aFile2, void* )
{
  nsCAutoString filename1, filename2;
  aFile1->GetNativeLeafName(filename1);
  aFile2->GetNativeLeafName(filename2);

  return Compare(filename2, filename1);
}






static nsresult
pref_LoadPrefsInDir(nsIFile* aDir, char const *const *aSpecialFiles, PRUint32 aSpecialFilesCount)
{
  nsresult rv, rv2;
  PRBool hasMoreElements;

  nsCOMPtr<nsISimpleEnumerator> dirIterator;

  
  rv = aDir->GetDirectoryEntries(getter_AddRefs(dirIterator));
  if (NS_FAILED(rv)) {
    
    
    if (rv == NS_ERROR_FILE_NOT_FOUND)
      rv = NS_OK;
    return rv;
  }

  rv = dirIterator->HasMoreElements(&hasMoreElements);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsIFile> prefFiles(INITIAL_PREF_FILES);
  nsCOMArray<nsIFile> specialFiles(aSpecialFilesCount);
  nsCOMPtr<nsIFile> prefFile;

  while (hasMoreElements && NS_SUCCEEDED(rv)) {
    nsCAutoString leafName;

    rv = dirIterator->GetNext(getter_AddRefs(prefFile));
    if (NS_FAILED(rv)) {
      break;
    }

    prefFile->GetNativeLeafName(leafName);
    NS_ASSERTION(!leafName.IsEmpty(), "Failure in default prefs: directory enumerator returned empty file?");

    
    if (StringEndsWith(leafName, NS_LITERAL_CSTRING(".js"),
                       nsCaseInsensitiveCStringComparator())) {
      PRBool shouldParse = PR_TRUE;
      
      for (PRUint32 i = 0; i < aSpecialFilesCount; ++i) {
        if (leafName.Equals(nsDependentCString(aSpecialFiles[i]))) {
          shouldParse = PR_FALSE;
          
          
          specialFiles.ReplaceObjectAt(prefFile, i);
        }
      }

      if (shouldParse) {
        prefFiles.AppendObject(prefFile);
      }
    }

    rv = dirIterator->HasMoreElements(&hasMoreElements);
  }

  if (prefFiles.Count() + specialFiles.Count() == 0) {
    NS_WARNING("No default pref files found.");
    if (NS_SUCCEEDED(rv)) {
      rv = NS_SUCCESS_FILE_DIRECTORY_EMPTY;
    }
    return rv;
  }

  prefFiles.Sort(pref_CompareFileNames, nsnull);
  
  PRUint32 arrayCount = prefFiles.Count();
  PRUint32 i;
  for (i = 0; i < arrayCount; ++i) {
    rv2 = openPrefFile(prefFiles[i]);
    if (NS_FAILED(rv2)) {
      NS_ERROR("Default pref file not parsed successfully.");
      rv = rv2;
    }
  }

  arrayCount = specialFiles.Count();
  for (i = 0; i < arrayCount; ++i) {
    
    nsIFile* file = specialFiles[i];
    if (file) {
      rv2 = openPrefFile(file);
      if (NS_FAILED(rv2)) {
        NS_ERROR("Special default pref file not parsed successfully.");
        rv = rv2;
      }
    }
  }

  return rv;
}

static nsresult pref_LoadPrefsInDirList(const char *listId)
{
  nsresult rv;
  nsCOMPtr<nsIProperties> dirSvc(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsISimpleEnumerator> dirList;
  dirSvc->Get(listId,
              NS_GET_IID(nsISimpleEnumerator),
              getter_AddRefs(dirList));
  if (dirList) {
    PRBool hasMore;
    while (NS_SUCCEEDED(dirList->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> elem;
      dirList->GetNext(getter_AddRefs(elem));
      if (elem) {
        nsCOMPtr<nsIFile> dir = do_QueryInterface(elem);
        if (dir) {
          
          pref_LoadPrefsInDir(dir, nsnull, 0); 
        }
      }
    }
  }
  return NS_OK;
}

static nsresult pref_ReadPrefFromJar(nsZipArchive* jarReader, const char *name)
{
  nsZipItemPtr<char> manifest(jarReader, name, true);
  NS_ENSURE_TRUE(manifest.Buffer(), NS_ERROR_NOT_AVAILABLE);

  PrefParseState ps;
  PREF_InitParseState(&ps, PREF_ReaderCallback, NULL);
  nsresult rv = PREF_ParseBuf(&ps, manifest, manifest.Length());
  PREF_FinalizeParseState(&ps);

  return rv;
}





static nsresult pref_InitInitialObjects()
{
  nsresult rv;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsZipFind *findPtr;
  nsAutoPtr<nsZipFind> find;
  nsTArray<nsCString> prefEntries;
  const char *entryName;
  PRUint16 entryNameLen;

  nsZipArchive* jarReader = mozilla::Omnijar::GetReader(mozilla::Omnijar::GRE);
  if (jarReader) {
    
    rv = pref_ReadPrefFromJar(jarReader, "greprefs.js");
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = jarReader->FindInit("defaults/pref/*.js$", &findPtr);
    NS_ENSURE_SUCCESS(rv, rv);

    find = findPtr;
    while (NS_SUCCEEDED(find->FindNext(&entryName, &entryNameLen))) {
      prefEntries.AppendElement(Substring(entryName, entryName + entryNameLen));
    }

    prefEntries.Sort();
    for (PRUint32 i = prefEntries.Length(); i--; ) {
      rv = pref_ReadPrefFromJar(jarReader, prefEntries[i].get());
      if (NS_FAILED(rv))
        NS_WARNING("Error parsing preferences.");
    }
  } else {
    
    nsCOMPtr<nsIFile> greprefsFile;
    rv = NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(greprefsFile));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = greprefsFile->AppendNative(NS_LITERAL_CSTRING("greprefs.js"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = openPrefFile(greprefsFile);
    if (NS_FAILED(rv))
      NS_WARNING("Error parsing GRE default preferences. Is this an old-style embedding app?");
  }

  if (!mozilla::Omnijar::HasOmnijar(mozilla::Omnijar::APP)) {
    
    nsCOMPtr<nsIFile> defaultPrefDir;

    rv = NS_GetSpecialDirectory(NS_APP_PREF_DEFAULTS_50_DIR, getter_AddRefs(defaultPrefDir));
    NS_ENSURE_SUCCESS(rv, rv);

    
    static const char* specialFiles[] = {
#if defined(XP_MACOSX)
      "macprefs.js"
#elif defined(XP_WIN)
      "winpref.js"
#elif defined(XP_UNIX)
      "unix.js"
#if defined(VMS)
      , "openvms.js"
#elif defined(_AIX)
      , "aix.js"
#endif
#elif defined(XP_OS2)
      "os2pref.js"
#elif defined(XP_BEOS)
      "beos.js"
#endif
    };

    rv = pref_LoadPrefsInDir(defaultPrefDir, specialFiles, NS_ARRAY_LENGTH(specialFiles));
    if (NS_FAILED(rv))
      NS_WARNING("Error parsing application default preferences.");
  }

  
  nsZipArchive *appJarReader = mozilla::Omnijar::GetReader(mozilla::Omnijar::APP);
  if (appJarReader) {
    rv = appJarReader->FindInit("defaults/preferences/*.js$", &findPtr);
    NS_ENSURE_SUCCESS(rv, rv);
    find = findPtr;
    prefEntries.Clear();
    while (NS_SUCCEEDED(find->FindNext(&entryName, &entryNameLen))) {
      prefEntries.AppendElement(Substring(entryName, entryName + entryNameLen));
    }
    prefEntries.Sort();
    for (PRUint32 i = prefEntries.Length(); i--; ) {
      rv = pref_ReadPrefFromJar(appJarReader, prefEntries[i].get());
      if (NS_FAILED(rv))
        NS_WARNING("Error parsing preferences.");
    }
  }

  rv = pref_LoadPrefsInDirList(NS_APP_PREFS_DEFAULTS_DIR_LIST);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_CreateServicesFromCategory(NS_PREFSERVICE_APPDEFAULTS_TOPIC_ID,
                                nsnull, NS_PREFSERVICE_APPDEFAULTS_TOPIC_ID);

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->NotifyObservers(nsnull, NS_PREFSERVICE_APPDEFAULTS_TOPIC_ID, nsnull);

  return pref_LoadPrefsInDirList(NS_EXT_PREFS_DEFAULTS_DIR_LIST);
}









nsresult
Preferences::GetBool(const char* aPref, PRBool* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->GetBoolPref(aPref, aResult);
}


nsresult
Preferences::GetInt(const char* aPref, PRInt32* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->GetIntPref(aPref, aResult);
}


nsAdoptingCString
Preferences::GetCString(const char* aPref)
{
  nsAdoptingCString result;
  GetCString(aPref, &result);
  return result;
}


nsAdoptingString
Preferences::GetString(const char* aPref)
{
  nsAdoptingString result;
  GetString(aPref, &result);
  return result;
}


nsresult
Preferences::GetCString(const char* aPref, nsACString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsCAutoString result;
  nsresult rv = sRootBranch->GetCharPref(aPref, getter_Copies(result));
  if (NS_SUCCEEDED(rv)) {
    *aResult = result;
  }
  return rv;
}


nsresult
Preferences::GetString(const char* aPref, nsAString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsCAutoString result;
  nsresult rv = sRootBranch->GetCharPref(aPref, getter_Copies(result));
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(result, *aResult);
  }
  return rv;
}


nsAdoptingCString
Preferences::GetLocalizedCString(const char* aPref)
{
  nsAdoptingCString result;
  GetLocalizedCString(aPref, &result);
  return result;
}


nsAdoptingString
Preferences::GetLocalizedString(const char* aPref)
{
  nsAdoptingString result;
  GetLocalizedString(aPref, &result);
  return result;
}


nsresult
Preferences::GetLocalizedCString(const char* aPref, nsACString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  nsAutoString result;
  nsresult rv = GetLocalizedString(aPref, &result);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF16toUTF8(result, *aResult);
  }
  return rv;
}


nsresult
Preferences::GetLocalizedString(const char* aPref, nsAString* aResult)
{
  NS_PRECONDITION(aResult, "aResult must not be NULL");
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsCOMPtr<nsIPrefLocalizedString> prefLocalString;
  nsresult rv = sRootBranch->GetComplexValue(aPref,
                                             NS_GET_IID(nsIPrefLocalizedString),
                                             getter_AddRefs(prefLocalString));
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(prefLocalString, "Succeeded but the result is NULL");
    prefLocalString->GetData(getter_Copies(*aResult));
  }
  return rv;
}


nsresult
Preferences::GetComplex(const char* aPref, const nsIID &aType, void** aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->GetComplexValue(aPref, aType, aResult);
}


nsresult
Preferences::SetCString(const char* aPref, const char* aValue)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->SetCharPref(aPref, aValue);
}


nsresult
Preferences::SetCString(const char* aPref, const nsACString &aValue)
{
  return SetCString(aPref, PromiseFlatCString(aValue).get());
}


nsresult
Preferences::SetString(const char* aPref, const PRUnichar* aValue)
{
  NS_ConvertUTF16toUTF8 utf8(aValue);
  return SetCString(aPref, utf8.get());
}


nsresult
Preferences::SetString(const char* aPref, const nsAString &aValue)
{
  NS_ConvertUTF16toUTF8 utf8(aValue);
  return SetCString(aPref, utf8.get());
}


nsresult
Preferences::SetBool(const char* aPref, PRBool aValue)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->SetBoolPref(aPref, aValue);
}


nsresult
Preferences::SetInt(const char* aPref, PRInt32 aValue)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->SetIntPref(aPref, aValue);
}


nsresult
Preferences::SetComplex(const char* aPref, const nsIID &aType,
                        nsISupports* aValue)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->SetComplexValue(aPref, aType, aValue);
}


nsresult
Preferences::ClearUser(const char* aPref)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->ClearUserPref(aPref);
}


PRBool
Preferences::HasUserValue(const char* aPref)
{
  NS_ENSURE_TRUE(InitStaticMembers(), PR_FALSE);
  PRBool hasUserValue;
  nsresult rv = sRootBranch->PrefHasUserValue(aPref, &hasUserValue);
  if (NS_FAILED(rv)) {
    return PR_FALSE;
  }
  return hasUserValue;
}


nsresult
Preferences::AddStrongObserver(nsIObserver* aObserver,
                               const char* aPref)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->AddObserver(aPref, aObserver, PR_FALSE);
}


nsresult
Preferences::AddWeakObserver(nsIObserver* aObserver,
                             const char* aPref)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->AddObserver(aPref, aObserver, PR_TRUE);
}


nsresult
Preferences::RemoveObserver(nsIObserver* aObserver,
                            const char* aPref)
{
  if (!sPreferences && sShutdown) {
    return NS_OK; 
  }
  NS_ENSURE_TRUE(sPreferences, NS_ERROR_NOT_AVAILABLE);
  return sRootBranch->RemoveObserver(aPref, aObserver);
}


nsresult
Preferences::AddStrongObservers(nsIObserver* aObserver,
                                const char** aPrefs)
{
  for (PRUint32 i = 0; aPrefs[i]; i++) {
    nsresult rv = AddStrongObserver(aObserver, aPrefs[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


nsresult
Preferences::AddWeakObservers(nsIObserver* aObserver,
                              const char** aPrefs)
{
  for (PRUint32 i = 0; aPrefs[i]; i++) {
    nsresult rv = AddWeakObserver(aObserver, aPrefs[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


nsresult
Preferences::RemoveObservers(nsIObserver* aObserver,
                             const char** aPrefs)
{
  if (!sPreferences && sShutdown) {
    return NS_OK; 
  }
  NS_ENSURE_TRUE(sPreferences, NS_ERROR_NOT_AVAILABLE);

  for (PRUint32 i = 0; aPrefs[i]; i++) {
    nsresult rv = RemoveObserver(aObserver, aPrefs[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


nsresult
Preferences::RegisterCallback(PrefChangedFunc aCallback,
                              const char* aPref,
                              void* aClosure)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);

  ValueObserverHashKey hashKey(aPref, aCallback);
  nsRefPtr<ValueObserver> observer;
  gObserverTable->Get(&hashKey, getter_AddRefs(observer));
  if (observer) {
    observer->AppendClosure(aClosure);
    return NS_OK;
  }

  observer = new ValueObserver(aPref, aCallback);
  observer->AppendClosure(aClosure);
  nsresult rv = AddStrongObserver(observer, aPref);
  NS_ENSURE_SUCCESS(rv, rv);
  return gObserverTable->Put(observer, observer) ? NS_OK :
                                                   NS_ERROR_OUT_OF_MEMORY;
}


nsresult
Preferences::UnregisterCallback(PrefChangedFunc aCallback,
                                const char* aPref,
                                void* aClosure)
{
  if (!sPreferences && sShutdown) {
    return NS_OK; 
  }
  NS_ENSURE_TRUE(sPreferences, NS_ERROR_NOT_AVAILABLE);

  ValueObserverHashKey hashKey(aPref, aCallback);
  nsRefPtr<ValueObserver> observer;
  gObserverTable->Get(&hashKey, getter_AddRefs(observer));
  if (!observer) {
    return NS_OK;
  }

  observer->RemoveClosure(aClosure);
  if (observer->HasNoClosures()) {
    
    gObserverTable->Remove(observer);
  }
  return NS_OK;
}

static int BoolVarChanged(const char* aPref, void* aClosure)
{
  CacheData* cache = static_cast<CacheData*>(aClosure);
  *((PRBool*)cache->cacheLocation) =
    Preferences::GetBool(aPref, cache->defaultValueBool);
  return 0;
}


nsresult
Preferences::AddBoolVarCache(PRBool* aCache,
                             const char* aPref,
                             PRBool aDefault)
{
  NS_ASSERTION(aCache, "aCache must not be NULL");
  *aCache = GetBool(aPref, aDefault);
  CacheData* data = new CacheData();
  data->cacheLocation = aCache;
  data->defaultValueBool = aDefault;
  gCacheData->AppendElement(data);
  return RegisterCallback(BoolVarChanged, aPref, data);
}

static int IntVarChanged(const char* aPref, void* aClosure)
{
  CacheData* cache = static_cast<CacheData*>(aClosure);
  *((PRInt32*)cache->cacheLocation) =
    Preferences::GetInt(aPref, cache->defaultValueInt);
  return 0;
}


nsresult
Preferences::AddIntVarCache(PRInt32* aCache,
                            const char* aPref,
                            PRInt32 aDefault)
{
  NS_ASSERTION(aCache, "aCache must not be NULL");
  *aCache = Preferences::GetInt(aPref, aDefault);
  CacheData* data = new CacheData();
  data->cacheLocation = aCache;
  data->defaultValueInt = aDefault;
  gCacheData->AppendElement(data);
  return RegisterCallback(IntVarChanged, aPref, data);
}

static int UintVarChanged(const char* aPref, void* aClosure)
{
  CacheData* cache = static_cast<CacheData*>(aClosure);
  *((PRUint32*)cache->cacheLocation) =
    Preferences::GetUint(aPref, cache->defaultValueUint);
  return 0;
}


nsresult
Preferences::AddUintVarCache(PRUint32* aCache,
                             const char* aPref,
                             PRUint32 aDefault)
{
  NS_ASSERTION(aCache, "aCache must not be NULL");
  *aCache = Preferences::GetUint(aPref, aDefault);
  CacheData* data = new CacheData();
  data->cacheLocation = aCache;
  data->defaultValueUint = aDefault;
  gCacheData->AppendElement(data);
  return RegisterCallback(UintVarChanged, aPref, data);
}


nsresult
Preferences::GetDefaultBool(const char* aPref, PRBool* aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sDefaultRootBranch->GetBoolPref(aPref, aResult);
}


nsresult
Preferences::GetDefaultInt(const char* aPref, PRInt32* aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sDefaultRootBranch->GetIntPref(aPref, aResult);
}


nsresult
Preferences::GetDefaultCString(const char* aPref, nsACString* aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsCAutoString result;
  nsresult rv = sDefaultRootBranch->GetCharPref(aPref, getter_Copies(result));
  if (NS_SUCCEEDED(rv)) {
    *aResult = result;
  }
  return rv;
}


nsresult
Preferences::GetDefaultString(const char* aPref, nsAString* aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsCAutoString result;
  nsresult rv = sDefaultRootBranch->GetCharPref(aPref, getter_Copies(result));
  if (NS_SUCCEEDED(rv)) {
    CopyUTF8toUTF16(result, *aResult);
  }
  return rv;
}


nsresult
Preferences::GetDefaultLocalizedCString(const char* aPref,
                                        nsACString* aResult)
{
  nsAutoString result;
  nsresult rv = GetDefaultLocalizedString(aPref, &result);
  if (NS_SUCCEEDED(rv)) {
    CopyUTF16toUTF8(result, *aResult);
  }
  return rv;
}


nsresult
Preferences::GetDefaultLocalizedString(const char* aPref,
                                       nsAString* aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  nsCOMPtr<nsIPrefLocalizedString> prefLocalString;
  nsresult rv =
    sDefaultRootBranch->GetComplexValue(aPref,
                                        NS_GET_IID(nsIPrefLocalizedString),
                                        getter_AddRefs(prefLocalString));
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(prefLocalString, "Succeeded but the result is NULL");
    prefLocalString->GetData(getter_Copies(*aResult));
  }
  return rv;
}


nsAdoptingString
Preferences::GetDefaultString(const char* aPref)
{
  nsAdoptingString result;
  GetDefaultString(aPref, &result);
  return result;
}


nsAdoptingCString
Preferences::GetDefaultCString(const char* aPref)
{
  nsAdoptingCString result;
  GetDefaultCString(aPref, &result);
  return result;
}


nsAdoptingString
Preferences::GetDefaultLocalizedString(const char* aPref)
{
  nsAdoptingString result;
  GetDefaultLocalizedString(aPref, &result);
  return result;
}


nsAdoptingCString
Preferences::GetDefaultLocalizedCString(const char* aPref)
{
  nsAdoptingCString result;
  GetDefaultLocalizedCString(aPref, &result);
  return result;
}


nsresult
Preferences::GetDefaultComplex(const char* aPref, const nsIID &aType,
                               void** aResult)
{
  NS_ENSURE_TRUE(InitStaticMembers(), NS_ERROR_NOT_AVAILABLE);
  return sDefaultRootBranch->GetComplexValue(aPref, aType, aResult);
}

} 
