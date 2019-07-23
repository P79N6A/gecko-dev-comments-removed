





































#include "nsPermissionManager.h"
#include "nsPermission.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsILineInputStream.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsVoidArray.h"
#include "prprf.h"



#define PL_ARENA_CONST_ALIGN_MASK 3
#include "plarena.h"

static PLArenaPool *gHostArena = nsnull;



#define HOST_ARENA_SIZE 512



static char *
ArenaStrDup(const char* str, PLArenaPool* aArena)
{
  void* mem;
  const PRUint32 size = strlen(str) + 1;
  PL_ARENA_ALLOCATE(mem, aArena, size);
  if (mem)
    memcpy(mem, str, size);
  return NS_STATIC_CAST(char*, mem);
}

nsHostEntry::nsHostEntry(const char* aHost)
{
  mHost = ArenaStrDup(aHost, gHostArena);
  mPermissions[0] = mPermissions[1] = 0;
}

nsHostEntry::nsHostEntry(const nsHostEntry& toCopy)
{
  
  
  NS_NOTREACHED("nsHostEntry copy constructor is forbidden!");
}



class nsPermissionEnumerator : public nsISimpleEnumerator
{
  public:
    NS_DECL_ISUPPORTS
 
    nsPermissionEnumerator(const nsTHashtable<nsHostEntry> *aHostTable,
                           const char*   *aHostList,
                           const PRUint32 aHostCount,
                           const char*   *aTypeArray)
      : mHostCount(aHostCount),
        mHostIndex(0),
        mTypeIndex(0),
        mHostTable(aHostTable),
        mHostList(aHostList),
        mTypeArray(aTypeArray)
    {
      Prefetch();
    }
    
    NS_IMETHOD HasMoreElements(PRBool *aResult) 
    {
      *aResult = (mNextPermission != nsnull);
      return NS_OK;
    }

    NS_IMETHOD GetNext(nsISupports **aResult) 
    {
      *aResult = mNextPermission;
      if (!mNextPermission)
        return NS_ERROR_FAILURE;

      NS_ADDREF(*aResult);
      
      Prefetch();

      return NS_OK;
    }

    virtual ~nsPermissionEnumerator() 
    {
      delete[] mHostList;
    }

  protected:
    void Prefetch();

    PRInt32 mHostCount;
    PRInt32 mHostIndex;
    PRInt32 mTypeIndex;
    
    const nsTHashtable<nsHostEntry> *mHostTable;
    const char*                     *mHostList;
    nsCOMPtr<nsIPermission>          mNextPermission;
    const char*                     *mTypeArray;
};

NS_IMPL_ISUPPORTS1(nsPermissionEnumerator, nsISimpleEnumerator)



void
nsPermissionEnumerator::Prefetch() 
{
  
  mNextPermission = nsnull;

  
  PRUint32 permission;
  while (mHostIndex < mHostCount && !mNextPermission) {
    
    nsHostEntry *entry = mHostTable->GetEntry(mHostList[mHostIndex]);
    if (entry) {
      
      permission = entry->GetPermission(mTypeIndex);
      if (permission != nsIPermissionManager::UNKNOWN_ACTION && mTypeArray[mTypeIndex]) {
        mNextPermission = new nsPermission(entry->GetHost(), 
                                           nsDependentCString(mTypeArray[mTypeIndex]),
                                           permission);
      }
    }

    
    ++mTypeIndex;
    if (mTypeIndex == NUMBER_OF_TYPES) {
      mTypeIndex = 0;
      ++mHostIndex;
    }
  }
}




static const char kPermissionsFileName[] = "hostperm.1";
static const char kOldPermissionsFileName[] = "cookperm.txt";
static const char kPermissionChangeNotification[] = PERM_CHANGE_NOTIFICATION;

static const PRUint32 kLazyWriteTimeout = 2000; 

NS_IMPL_ISUPPORTS3(nsPermissionManager, nsIPermissionManager, nsIObserver, nsISupportsWeakReference)

nsPermissionManager::nsPermissionManager()
 : mHostCount(0),
   mChangedList(PR_FALSE)
{
}

nsPermissionManager::~nsPermissionManager()
{
  if (mWriteTimer)
    mWriteTimer->Cancel();

  RemoveTypeStrings();
  RemoveAllFromMemory();
}

nsresult nsPermissionManager::Init()
{
  nsresult rv;

  if (!mHostTable.Init()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  memset(mTypeArray, nsnull, sizeof(mTypeArray));

  
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mPermissionsFile));
  if (NS_SUCCEEDED(rv)) {
    mPermissionsFile->AppendNative(NS_LITERAL_CSTRING(kPermissionsFileName));

    
    Read();
  }

  mObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    mObserverService->AddObserver(this, "profile-before-change", PR_TRUE);
    mObserverService->AddObserver(this, "profile-do-change", PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::Add(nsIURI     *aURI,
                         const char *aType,
                         PRUint32    aPermission)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aType);

  nsresult rv;

  nsCAutoString host;
  rv = GetHost(aURI, host);
  
  if (NS_FAILED(rv)) return NS_OK;

  PRInt32 typeIndex = GetTypeIndex(aType, PR_TRUE);
  if (typeIndex == -1 || aPermission >= NUMBER_OF_PERMISSIONS)
    return NS_ERROR_FAILURE;

  rv = AddInternal(host, typeIndex, aPermission, PR_TRUE);
  if (NS_FAILED(rv)) return rv;

  mChangedList = PR_TRUE;
  LazyWrite();

  return NS_OK;
}



nsresult
nsPermissionManager::AddInternal(const nsAFlatCString &aHost,
                                 PRInt32               aTypeIndex,
                                 PRUint32              aPermission,
                                 PRBool                aNotify)
{
  if (!gHostArena) {
    gHostArena = new PLArenaPool;
    if (!gHostArena)
      return NS_ERROR_OUT_OF_MEMORY;    
    PL_INIT_ARENA_POOL(gHostArena, "PermissionHostArena", HOST_ARENA_SIZE);
  }

  
  
  nsHostEntry *entry = mHostTable.PutEntry(aHost.get());
  if (!entry) return NS_ERROR_FAILURE;
  if (!entry->GetKey()) {
    mHostTable.RawRemoveEntry(entry);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (entry->PermissionsAreEmpty()) {
    ++mHostCount;
  }

  PRUint32 oldPermission = entry->GetPermission(aTypeIndex);
  entry->SetPermission(aTypeIndex, aPermission);

  
  
  if (entry->PermissionsAreEmpty()) {
    mHostTable.RawRemoveEntry(entry);
    --mHostCount;
  }

  
  
  
  
  
  if (aNotify) {
    if (aPermission == nsIPermissionManager::UNKNOWN_ACTION) {
      if (oldPermission != nsIPermissionManager::UNKNOWN_ACTION)
        
        NotifyObserversWithPermission(aHost,
                                      mTypeArray[aTypeIndex],
                                      oldPermission,
                                      NS_LITERAL_STRING("deleted").get());
    } else {
      if (oldPermission == nsIPermissionManager::UNKNOWN_ACTION)
        
        NotifyObserversWithPermission(aHost,
                                      mTypeArray[aTypeIndex],
                                      aPermission,
                                      NS_LITERAL_STRING("added").get());
      else
        
        NotifyObserversWithPermission(aHost,
                                      mTypeArray[aTypeIndex],
                                      aPermission,
                                      NS_LITERAL_STRING("changed").get());
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::Remove(const nsACString &aHost,
                            const char       *aType)
{
  NS_ENSURE_ARG_POINTER(aType);
  PRInt32 typeIndex = GetTypeIndex(aType, PR_FALSE);
  
  
  if (typeIndex == -1) return NS_OK;

  nsHostEntry *entry = GetHostEntry(PromiseFlatCString(aHost), typeIndex, PR_FALSE);
  if (entry) {
    
    PRUint32 oldPermission = entry->GetPermission(typeIndex);

    entry->SetPermission(typeIndex, nsIPermissionManager::UNKNOWN_ACTION);

    
    if (entry->PermissionsAreEmpty()) {
      mHostTable.RawRemoveEntry(entry);
      --mHostCount;
    }
    mChangedList = PR_TRUE;
    LazyWrite();

    
    if (oldPermission != nsIPermissionManager::UNKNOWN_ACTION)
      NotifyObserversWithPermission(PromiseFlatCString(aHost),
                                    aType,
                                    oldPermission,
                                    NS_LITERAL_STRING("deleted").get());
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::RemoveAll()
{
  RemoveAllFromMemory();
  NotifyObservers(nsnull, NS_LITERAL_STRING("cleared").get());
  LazyWrite();
  return NS_OK;
}

NS_IMETHODIMP
nsPermissionManager::TestExactPermission(nsIURI     *aURI,
                                         const char *aType,
                                         PRUint32   *aPermission)
{
  return CommonTestPermission(aURI, aType, aPermission, PR_TRUE);
}

NS_IMETHODIMP
nsPermissionManager::TestPermission(nsIURI     *aURI,
                                    const char *aType,
                                    PRUint32   *aPermission)
{
  return CommonTestPermission(aURI, aType, aPermission, PR_FALSE);
}

nsresult
nsPermissionManager::CommonTestPermission(nsIURI     *aURI,
                                          const char *aType,
                                          PRUint32   *aPermission,
                                          PRBool      aExactHostMatch)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aType);

  
  *aPermission = nsIPermissionManager::UNKNOWN_ACTION;

  nsCAutoString host;
  nsresult rv = GetHost(aURI, host);
  
  if (NS_FAILED(rv)) return NS_OK;
  
  PRInt32 typeIndex = GetTypeIndex(aType, PR_FALSE);
  
  
  if (typeIndex == -1) return NS_OK;

  nsHostEntry *entry = GetHostEntry(host, typeIndex, aExactHostMatch);
  if (entry)
    *aPermission = entry->GetPermission(typeIndex);

  return NS_OK;
}




nsHostEntry *
nsPermissionManager::GetHostEntry(const nsAFlatCString &aHost,
                                  PRUint32              aType,
                                  PRBool                aExactHostMatch)
{
  PRUint32 offset = 0;
  nsHostEntry *entry;
  do {
    entry = mHostTable.GetEntry(aHost.get() + offset);
    if (entry) {
      if (entry->GetPermission(aType) != nsIPermissionManager::UNKNOWN_ACTION)
        break;

      
      entry = nsnull;
    }
    if (aExactHostMatch)
      break; 

    offset = aHost.FindChar('.', offset) + 1;

  
  
  } while (offset > 0);
  return entry;
}



PR_STATIC_CALLBACK(PLDHashOperator)
AddHostToList(nsHostEntry *entry, void *arg)
{
  
  
  
  
  const char*** elementPtr = NS_STATIC_CAST(const char***, arg);
  **elementPtr = entry->GetKey();
  ++(*elementPtr);
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP nsPermissionManager::GetEnumerator(nsISimpleEnumerator **aEnum)
{
  *aEnum = nsnull;
  
  

  
  const char* *hostList = new const char*[mHostCount];
  if (!hostList) return NS_ERROR_OUT_OF_MEMORY;

  
  
  const char** hostListCopy = hostList;
  mHostTable.EnumerateEntries(AddHostToList, &hostListCopy);

  nsPermissionEnumerator* permissionEnum = new nsPermissionEnumerator(&mHostTable, hostList, mHostCount, NS_CONST_CAST(const char**, mTypeArray));
  if (!permissionEnum) {
    delete[] hostList;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(permissionEnum);
  *aEnum = permissionEnum;
  return NS_OK;
}

NS_IMETHODIMP nsPermissionManager::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  nsresult rv = NS_OK;

  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    

    if (mWriteTimer) {
      mWriteTimer->Cancel();
      mWriteTimer = 0;
    }
    
    
    
    
    
    
    
    

    if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("shutdown-cleanse").get())) {
      if (mPermissionsFile) {
        mPermissionsFile->Remove(PR_FALSE);
      }
    } else {
      Write();
    }
    RemoveTypeStrings();
    RemoveAllFromMemory();
  }  
  else if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    
    

    
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mPermissionsFile));
    if (NS_SUCCEEDED(rv)) {
      mPermissionsFile->AppendNative(NS_LITERAL_CSTRING(kPermissionsFileName));
      Read();
    }
  }

  return rv;
}





nsresult
nsPermissionManager::RemoveAllFromMemory()
{
  mHostTable.Clear();
  mHostCount = 0;
  if (gHostArena) {
    PL_FinishArenaPool(gHostArena);
    delete gHostArena;
  }
  gHostArena = nsnull;
  mChangedList = PR_TRUE;
  return NS_OK;
}

void
nsPermissionManager::RemoveTypeStrings()
{
  for (PRUint32 i = NUMBER_OF_TYPES; i--; ) {
    if (mTypeArray[i]) {
      PL_strfree(mTypeArray[i]);
      mTypeArray[i] = nsnull;
    }
  }
}


PRInt32
nsPermissionManager::GetTypeIndex(const char *aType,
                                  PRBool      aAdd)
{
  PRInt32 firstEmpty = -1;

  for (PRUint32 i = 0; i < NUMBER_OF_TYPES; ++i) {
    if (!mTypeArray[i]) {
      if (firstEmpty == -1)
        
        firstEmpty = i;
    } else if (!strcmp(aType, mTypeArray[i])) {
      return i;
    }
  }

  if (!aAdd || firstEmpty == -1)
    
    
    return -1;

  
  
  
  mTypeArray[firstEmpty] = PL_strdup(aType);
  if (!mTypeArray[firstEmpty])
    return -1;

  return firstEmpty;
}


void
nsPermissionManager::NotifyObserversWithPermission(const nsACString &aHost,
                                                   const char       *aType,
                                                   PRUint32          aPermission,
                                                   const PRUnichar  *aData)
{
  nsCOMPtr<nsIPermission> permission =
    new nsPermission(aHost, nsDependentCString(aType), aPermission);
  if (permission)
    NotifyObservers(permission, aData);
}







void
nsPermissionManager::NotifyObservers(nsIPermission   *aPermission,
                                     const PRUnichar *aData)
{
  if (mObserverService)
    mObserverService->NotifyObservers(aPermission,
                                      kPermissionChangeNotification,
                                      aData);
}

static const char kTab = '\t';
static const char kNew = '\n';
static const char kTrue = 'T';
static const char kFalse = 'F';
static const char kFirstLetter = 'a';
static const char kTypeSign = '%';

static const char kMatchTypeHost[] = "host";

nsresult
nsPermissionManager::Read()
{
  nsresult rv;
  
  PRBool readingOldFile = PR_FALSE;

  nsCOMPtr<nsIInputStream> fileInputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), mPermissionsFile);
  if (rv == NS_ERROR_FILE_NOT_FOUND) {
    

    nsCOMPtr<nsIFile> oldPermissionsFile;
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(oldPermissionsFile));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = oldPermissionsFile->AppendNative(NS_LITERAL_CSTRING(kOldPermissionsFileName));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), oldPermissionsFile);

    readingOldFile = PR_TRUE;

    





  }
  
  
  if (NS_FAILED(rv))
    return rv;


  nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(fileInputStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  






  mHasUnknownTypes = PR_FALSE;

  nsCAutoString buffer;
  PRBool isMore = PR_TRUE;
  while (isMore && NS_SUCCEEDED(lineInputStream->ReadLine(buffer, &isMore))) {
    if (buffer.IsEmpty() || buffer.First() == '#') {
      continue;
    }

    if (!readingOldFile) {
      nsCStringArray lineArray;
      
      
      lineArray.ParseString(buffer.get(), "\t");
      
      if (lineArray[0]->EqualsLiteral(kMatchTypeHost) &&
          lineArray.Count() == 4) {
        
        PRInt32 error;
        PRUint32 permission = lineArray[2]->ToInteger(&error);
        if (error)
          continue;
        PRInt32 type = GetTypeIndex(lineArray[1]->get(), PR_TRUE);
        if (type < 0)
          continue;

        rv = AddInternal(*lineArray[3], type, permission, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        mHasUnknownTypes = PR_TRUE;
      }

    } else {
      
      nsASingleFragmentCString::char_iterator iter;
      if (buffer.First() == kTypeSign) {
        

        PRInt32 stringIndex;

        if ((stringIndex = buffer.FindChar(kTab) + 1) == 0) {
          continue;      
        }

        PRUint32 type;
        if (PR_sscanf(buffer.get() + 1, "%u", &type) != 1 || type >= NUMBER_OF_TYPES) {
          continue;
        }

        
        
        
        
        
        
        if (!strcmp(buffer.get() + stringIndex, "0F"))
          continue;

        NS_ASSERTION(GetTypeIndex(buffer.get() + stringIndex, PR_FALSE) == -1, "Corrupt cookperm.txt file");
        mTypeArray[type] = PL_strdup(buffer.get() + stringIndex);

        continue;
      }

      PRInt32 hostIndex, permissionIndex;
      PRUint32 nextPermissionIndex = 0;
      hostIndex = 0;

      if ((permissionIndex = buffer.FindChar('\t', hostIndex) + 1) == 0)
        continue;      

      
      while (hostIndex < permissionIndex && (buffer.CharAt(hostIndex) == '.'))
        ++hostIndex;

      
      buffer.BeginWriting(iter);
      *(iter += permissionIndex - 1) = char(0);
      nsDependentCString host(buffer.get() + hostIndex, iter);

      for (;;) {
        if (nextPermissionIndex == buffer.Length()+1)
          break;

        if ((nextPermissionIndex = buffer.FindChar(kTab, permissionIndex) + 1) == 0)
          nextPermissionIndex = buffer.Length()+1;

        const nsASingleFragmentCString &permissionString = Substring(buffer, permissionIndex, nextPermissionIndex - permissionIndex - 1);
        permissionIndex = nextPermissionIndex;

        PRInt32 type = 0;
        PRUint32 index = 0;

        if (permissionString.IsEmpty())
          continue; 

        
        char c = permissionString.CharAt(index);
        while (index < permissionString.Length() && c >= '0' && c <= '9') {
          type = 10*type + (c-'0');
          c = permissionString.CharAt(++index);
        }

        if (type >= NUMBER_OF_TYPES)
          continue; 

        if (index >= permissionString.Length())
          continue; 

        PRUint32 permission;
        if (permissionString.CharAt(index) == kTrue)
          permission = nsIPermissionManager::ALLOW_ACTION;
        else if (permissionString.CharAt(index) == kFalse)
          permission = nsIPermissionManager::DENY_ACTION;
        else
          permission = permissionString.CharAt(index) - kFirstLetter;

        
        if (permission >= NUMBER_OF_PERMISSIONS)
          continue;

        
        if (!permissionString.IsEmpty() && !host.EqualsLiteral("@@@@")) {
          rv = AddInternal(host, type, permission, PR_FALSE);
          if (NS_FAILED(rv)) return rv;
        }

      }
      
      
      
      GetTypeIndex("cookie", PR_TRUE);
      GetTypeIndex("image", PR_TRUE);
      GetTypeIndex("popup", PR_TRUE);

      
    }

  }


  mChangedList = PR_FALSE;

  return NS_OK;
}





PR_STATIC_CALLBACK(PLDHashOperator)
AddEntryToList(nsHostEntry *entry, void *arg)
{
  
  
  
  
  nsHostEntry*** elementPtr = NS_STATIC_CAST(nsHostEntry***, arg);
  **elementPtr = entry;
  ++(*elementPtr);
  return PL_DHASH_NEXT;
}

void
nsPermissionManager::LazyWrite()
{
  if (mWriteTimer) {
    mWriteTimer->SetDelay(kLazyWriteTimeout);
  } else {
    mWriteTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mWriteTimer) {
      mWriteTimer->InitWithFuncCallback(DoLazyWrite, this, kLazyWriteTimeout,
                                        nsITimer::TYPE_ONE_SHOT);
    }
  }
}

void
nsPermissionManager::DoLazyWrite(nsITimer *aTimer,
                                 void     *aClosure)
{
  nsPermissionManager *service = NS_REINTERPRET_CAST(nsPermissionManager*, aClosure);
  service->Write();
  service->mWriteTimer = 0;
}

nsresult
nsPermissionManager::Write()
{
  nsresult rv;

  if (!mChangedList) {
    return NS_OK;
  }

  if (!mPermissionsFile) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  nsCStringArray rememberList;
  if (mHasUnknownTypes) {
    nsCOMPtr<nsIInputStream> fileInputStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), mPermissionsFile);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(fileInputStream, &rv);
      if (NS_SUCCEEDED(rv)) {
        nsCAutoString buffer;
        PRBool isMore = PR_TRUE;
        while (isMore && NS_SUCCEEDED(lineInputStream->ReadLine(buffer, &isMore))) {
          if (buffer.IsEmpty() || buffer.First() == '#' ||
              StringBeginsWith(buffer, NS_LITERAL_CSTRING(kMatchTypeHost)))
            continue;

          rememberList.AppendCString(buffer);
        }
      }
    }
  }

  nsCOMPtr<nsIOutputStream> fileOutputStream;
  rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(fileOutputStream),
                                       mPermissionsFile,
                                       -1,
                                       0600);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream), fileOutputStream, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  static const char kHeader[] = 
    "# Permission File\n"
    "# This is a generated file! Do not edit.\n\n";

  bufferedOutputStream->Write(kHeader, sizeof(kHeader) - 1, &rv);

  



  
  PRUint32 i;
  if (mHasUnknownTypes) {
    for (i = 0 ; i < rememberList.Count() ; ++i) {
      bufferedOutputStream->Write(rememberList[i]->get(), 
                                  rememberList[i]->Length(), &rv);
      bufferedOutputStream->Write(&kNew, 1, &rv);
    }
  }

  
  nsHostEntry* *hostList = new nsHostEntry*[mHostCount];
  if (!hostList) return NS_ERROR_OUT_OF_MEMORY;

  
  
  nsHostEntry** hostListCopy = hostList;
  mHostTable.EnumerateEntries(AddEntryToList, &hostListCopy);

  for (i = 0; i < mHostCount; ++i) {
    nsHostEntry *entry = NS_STATIC_CAST(nsHostEntry*, hostList[i]);
    NS_ASSERTION(entry, "corrupt permission list");

    for (PRInt32 type = 0; type < NUMBER_OF_TYPES; ++type) {
    
      PRUint32 permission = entry->GetPermission(type);
      if (permission && mTypeArray[type]) {
        
        
        bufferedOutputStream->Write(kMatchTypeHost, sizeof(kMatchTypeHost) - 1, &rv);

        
        bufferedOutputStream->Write(&kTab, 1, &rv);
        bufferedOutputStream->Write(mTypeArray[type], strlen(mTypeArray[type]), &rv);

        
        bufferedOutputStream->Write(&kTab, 1, &rv);
        char permissionString[5];
        PRUint32 len = PR_snprintf(permissionString, sizeof(permissionString) - 1, "%u", permission);
        bufferedOutputStream->Write(permissionString, len, &rv);

        
        bufferedOutputStream->Write(&kTab, 1, &rv);
        bufferedOutputStream->Write(entry->GetHost().get(), entry->GetHost().Length(), &rv);
        
        
        bufferedOutputStream->Write(&kNew, 1, &rv);
      }
    }
  }

  delete[] hostList;

  
  
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(bufferedOutputStream);
  NS_ASSERTION(safeStream, "expected a safe output stream!");
  if (safeStream) {
    rv = safeStream->Finish();
    if (NS_FAILED(rv)) {
      NS_WARNING("failed to save permissions file! possible dataloss");
      return rv;
    }
  }

  mChangedList = PR_FALSE;
  return NS_OK;
}

nsresult
nsPermissionManager::GetHost(nsIURI *aURI, nsACString &aResult)
{
  aURI->GetHost(aResult);

  
  
  if (aResult.IsEmpty()) {
    aURI->GetScheme(aResult);
    if (aResult.IsEmpty()) {
      
      return NS_ERROR_FAILURE;
    }
    aResult = NS_LITERAL_CSTRING("scheme:") + aResult;
  }

  return NS_OK;
}
