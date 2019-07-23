






































#include "nsPrefBranch.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIDirectoryService.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"
#include "nsIStringBundle.h"
#include "prefapi.h"
#include "prmem.h"
#include "pldhash.h"
#include "nsPrefsCID.h"

#include "plstr.h"
#include "nsCRT.h"

#include "prefapi_private_data.h"


struct EnumerateData {
  const char  *parent;
  nsVoidArray *pref_list;
};

struct PrefCallbackData {
  nsPrefBranch     *pBranch;
  nsIObserver      *pObserver;
  nsIWeakReference *pWeakRef;
  char pDomain[1];
};



static PLDHashOperator
  pref_enumChild(PLDHashTable *table, PLDHashEntryHdr *heh,
                 PRUint32 i, void *arg);
static nsresult
  NotifyObserver(const char *newpref, void *data);





nsPrefBranch::nsPrefBranch(const char *aPrefRoot, PRBool aDefaultBranch)
  : mObservers(nsnull)
{
  mPrefRoot = aPrefRoot;
  mPrefRootLength = mPrefRoot.Length();
  mIsDefault = aDefaultBranch;

  nsCOMPtr<nsIObserverService> observerService = 
           do_GetService("@mozilla.org/observer-service;1");
  if (observerService) {
    ++mRefCnt;    
    
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_TRUE);
    --mRefCnt;
  }
}

nsPrefBranch::~nsPrefBranch()
{
  freeObserverList();
}






NS_IMPL_THREADSAFE_ADDREF(nsPrefBranch)
NS_IMPL_THREADSAFE_RELEASE(nsPrefBranch)

NS_INTERFACE_MAP_BEGIN(nsPrefBranch)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPrefBranch)
  NS_INTERFACE_MAP_ENTRY(nsIPrefBranch)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIPrefBranch2, !mIsDefault)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIPrefBranchInternal, !mIsDefault)
  NS_INTERFACE_MAP_ENTRY(nsISecurityPref)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END






NS_IMETHODIMP nsPrefBranch::GetRoot(char * *aRoot)
{
  NS_ENSURE_ARG_POINTER(aRoot);

  mPrefRoot.Truncate(mPrefRootLength);
  *aRoot = ToNewCString(mPrefRoot);
  return NS_OK;
}

NS_IMETHODIMP nsPrefBranch::GetPrefType(const char *aPrefName, PRInt32 *_retval)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_FAILED(rv))
    return rv;

  *_retval = PREF_GetPrefType(pref);
  return NS_OK;
}

NS_IMETHODIMP nsPrefBranch::GetBoolPref(const char *aPrefName, PRBool *_retval)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_GetBoolPref(pref, _retval, mIsDefault);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::SetBoolPref(const char *aPrefName, PRInt32 aValue)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_SetBoolPref(pref, aValue, mIsDefault);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::GetCharPref(const char *aPrefName, char **_retval)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_CopyCharPref(pref, _retval, mIsDefault);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::SetCharPref(const char *aPrefName, const char *aValue)
{
  const char *pref;
  nsresult   rv;

  NS_ENSURE_ARG_POINTER(aValue);
  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_SetCharPref(pref, aValue, mIsDefault);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::GetIntPref(const char *aPrefName, PRInt32 *_retval)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_GetIntPref(pref, _retval, mIsDefault);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::SetIntPref(const char *aPrefName, PRInt32 aValue)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_SetIntPref(pref, aValue, mIsDefault);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::GetComplexValue(const char *aPrefName, const nsIID & aType, void * *_retval)
{
  nsresult       rv;
  nsXPIDLCString utf8String;

  
  if (aType.Equals(NS_GET_IID(nsIPrefLocalizedString))) {
    nsCOMPtr<nsIPrefLocalizedString> theString(do_CreateInstance(NS_PREFLOCALIZEDSTRING_CONTRACTID, &rv));

    if (NS_SUCCEEDED(rv)) {
      const char *pref;
      PRBool  bNeedDefault = PR_FALSE;

      rv = getValidatedPrefName(aPrefName, &pref);
      if (NS_FAILED(rv))
        return rv;

      if (mIsDefault) {
        bNeedDefault = PR_TRUE;
      } else {
        
        if (!PREF_HasUserPref(pref) && !PREF_PrefIsLocked(pref)) {
          bNeedDefault = PR_TRUE;
        }
      }

      
      
      if (bNeedDefault) {
        nsXPIDLString utf16String;
        rv = GetDefaultFromPropertiesFile(pref, getter_Copies(utf16String));
        if (NS_SUCCEEDED(rv)) {
          rv = theString->SetData(utf16String.get());
        }
      } else {
        rv = GetCharPref(aPrefName, getter_Copies(utf8String));
        if (NS_SUCCEEDED(rv)) {
          rv = theString->SetData(NS_ConvertUTF8toUTF16(utf8String).get());
        }
      }
      if (NS_SUCCEEDED(rv)) {
        nsIPrefLocalizedString *temp = theString;

        NS_ADDREF(temp);
        *_retval = (void *)temp;
      }
    }

    return rv;
  }

  
  rv = GetCharPref(aPrefName, getter_Copies(utf8String));
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (aType.Equals(NS_GET_IID(nsILocalFile))) {
    nsCOMPtr<nsILocalFile> file(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv));

    if (NS_SUCCEEDED(rv)) {
      rv = file->SetPersistentDescriptor(utf8String);
      if (NS_SUCCEEDED(rv)) {
        nsILocalFile *temp = file;

        NS_ADDREF(temp);
        *_retval = (void *)temp;
        return NS_OK;
      }
    }
    return rv;
  }

  if (aType.Equals(NS_GET_IID(nsIRelativeFilePref))) {
    nsACString::const_iterator keyBegin, strEnd;
    utf8String.BeginReading(keyBegin);
    utf8String.EndReading(strEnd);    

    
    if (*keyBegin++ != '[')        
        return NS_ERROR_FAILURE;
    nsACString::const_iterator keyEnd(keyBegin);
    if (!FindCharInReadable(']', keyEnd, strEnd))
        return NS_ERROR_FAILURE;
    nsCAutoString key(Substring(keyBegin, keyEnd));
    
    nsCOMPtr<nsILocalFile> fromFile;        
    nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv));
    if (NS_FAILED(rv))
      return rv;
    rv = directoryService->Get(key.get(), NS_GET_IID(nsILocalFile), getter_AddRefs(fromFile));
    if (NS_FAILED(rv))
      return rv;
    
    nsCOMPtr<nsILocalFile> theFile;
    rv = NS_NewNativeLocalFile(EmptyCString(), PR_TRUE, getter_AddRefs(theFile));
    if (NS_FAILED(rv))
      return rv;
    rv = theFile->SetRelativeDescriptor(fromFile, Substring(++keyEnd, strEnd));
    if (NS_FAILED(rv))
      return rv;
    nsCOMPtr<nsIRelativeFilePref> relativePref;
    rv = NS_NewRelativeFilePref(theFile, key, getter_AddRefs(relativePref));
    if (NS_FAILED(rv))
      return rv;

    *_retval = relativePref;
    NS_ADDREF(static_cast<nsIRelativeFilePref*>(*_retval));
    return NS_OK;
  }

  if (aType.Equals(NS_GET_IID(nsISupportsString))) {
    nsCOMPtr<nsISupportsString> theString(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv));

    if (NS_SUCCEEDED(rv)) {
      rv = theString->SetData(NS_ConvertUTF8toUTF16(utf8String));
      if (NS_SUCCEEDED(rv)) {
        nsISupportsString *temp = theString;

        NS_ADDREF(temp);
        *_retval = (void *)temp;
        return NS_OK;
      }
    }
    return rv;
  }

  NS_WARNING("nsPrefBranch::GetComplexValue - Unsupported interface type");
  return NS_NOINTERFACE;
}

NS_IMETHODIMP nsPrefBranch::SetComplexValue(const char *aPrefName, const nsIID & aType, nsISupports *aValue)
{
  nsresult   rv = NS_NOINTERFACE;

  if (aType.Equals(NS_GET_IID(nsILocalFile))) {
    nsCOMPtr<nsILocalFile> file = do_QueryInterface(aValue);
    if (!file)
      return NS_NOINTERFACE;
    nsCAutoString descriptorString;

    rv = file->GetPersistentDescriptor(descriptorString);
    if (NS_SUCCEEDED(rv)) {
      rv = SetCharPref(aPrefName, descriptorString.get());
    }
    return rv;
  }

  if (aType.Equals(NS_GET_IID(nsIRelativeFilePref))) {
    nsCOMPtr<nsIRelativeFilePref> relFilePref = do_QueryInterface(aValue);
    if (!relFilePref)
      return NS_NOINTERFACE;
    
    nsCOMPtr<nsILocalFile> file;
    relFilePref->GetFile(getter_AddRefs(file));
    if (!file)
      return NS_NOINTERFACE;
    nsCAutoString relativeToKey;
    (void) relFilePref->GetRelativeToKey(relativeToKey);

    nsCOMPtr<nsILocalFile> relativeToFile;        
    nsCOMPtr<nsIProperties> directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv));
    if (NS_FAILED(rv))
      return rv;
    rv = directoryService->Get(relativeToKey.get(), NS_GET_IID(nsILocalFile), getter_AddRefs(relativeToFile));
    if (NS_FAILED(rv))
      return rv;

    nsCAutoString relDescriptor;
    rv = file->GetRelativeDescriptor(relativeToFile, relDescriptor);
    if (NS_FAILED(rv))
      return rv;
    
    nsCAutoString descriptorString;
    descriptorString.Append('[');
    descriptorString.Append(relativeToKey);
    descriptorString.Append(']');
    descriptorString.Append(relDescriptor);
    return SetCharPref(aPrefName, descriptorString.get());
  }

  if (aType.Equals(NS_GET_IID(nsISupportsString))) {
    nsCOMPtr<nsISupportsString> theString = do_QueryInterface(aValue);

    if (theString) {
      nsAutoString wideString;

      rv = theString->GetData(wideString);
      if (NS_SUCCEEDED(rv)) {
        rv = SetCharPref(aPrefName, NS_ConvertUTF16toUTF8(wideString).get());
      }
    }
    return rv;
  }

  if (aType.Equals(NS_GET_IID(nsIPrefLocalizedString))) {
    nsCOMPtr<nsIPrefLocalizedString> theString = do_QueryInterface(aValue);

    if (theString) {
      nsXPIDLString wideString;

      rv = theString->GetData(getter_Copies(wideString));
      if (NS_SUCCEEDED(rv)) {
        rv = SetCharPref(aPrefName, NS_ConvertUTF16toUTF8(wideString).get());
      }
    }
    return rv;
  }

  NS_WARNING("nsPrefBranch::SetComplexValue - Unsupported interface type");
  return NS_NOINTERFACE;
}

NS_IMETHODIMP nsPrefBranch::ClearUserPref(const char *aPrefName)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_ClearUserPref(pref);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::PrefHasUserValue(const char *aPrefName, PRBool *_retval)
{
  const char *pref;
  nsresult   rv;

  NS_ENSURE_ARG_POINTER(_retval);

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    *_retval = PREF_HasUserPref(pref);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::LockPref(const char *aPrefName)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_LockPref(pref, PR_TRUE);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::PrefIsLocked(const char *aPrefName, PRBool *_retval)
{
  const char *pref;
  nsresult   rv;

  NS_ENSURE_ARG_POINTER(_retval);

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    *_retval = PREF_PrefIsLocked(pref);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::UnlockPref(const char *aPrefName)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aPrefName, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_LockPref(pref, PR_FALSE);
  }
  return rv;
}


NS_IMETHODIMP nsPrefBranch::ResetBranch(const char *aStartingAt)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsPrefBranch::DeleteBranch(const char *aStartingAt)
{
  const char *pref;
  nsresult   rv;

  rv = getValidatedPrefName(aStartingAt, &pref);
  if (NS_SUCCEEDED(rv)) {
    rv = PREF_DeleteBranch(pref);
  }
  return rv;
}

NS_IMETHODIMP nsPrefBranch::GetChildList(const char *aStartingAt, PRUint32 *aCount, char ***aChildArray)
{
  char**          outArray;
  char*           theElement;
  PRInt32         numPrefs;
  PRInt32         dwIndex;
  EnumerateData   ed;
  nsAutoVoidArray prefArray;

  NS_ENSURE_ARG_POINTER(aStartingAt);
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aChildArray);

  if (!gHashTable.ops) {
    *aChildArray = nsnull;
    *aCount = 0;
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  

  ed.parent = getPrefName(aStartingAt);
  ed.pref_list = &prefArray;
  PL_DHashTableEnumerate(&gHashTable, pref_enumChild, &ed);

  
  
  numPrefs = prefArray.Count();

  if (numPrefs) {
    outArray = (char **)nsMemory::Alloc(numPrefs * sizeof(char *));
    if (!outArray)
      return NS_ERROR_OUT_OF_MEMORY;

    for (dwIndex = 0; dwIndex < numPrefs; ++dwIndex) {
      
      
      theElement = ((char *)prefArray.ElementAt(dwIndex)) + mPrefRootLength;
      outArray[dwIndex] = (char *)nsMemory::Clone(theElement, strlen(theElement) + 1);
 
      if (!outArray[dwIndex]) {
        
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(dwIndex, outArray);
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    *aChildArray = outArray;
  } else {
    *aChildArray = nsnull;
  } 
  *aCount = numPrefs;

  return NS_OK;
}






NS_IMETHODIMP nsPrefBranch::AddObserver(const char *aDomain, nsIObserver *aObserver, PRBool aHoldWeak)
{
  PrefCallbackData *pCallback;
  const char *pref;

  NS_ENSURE_ARG_POINTER(aDomain);
  NS_ENSURE_ARG_POINTER(aObserver);

  if (!mObservers) {
    mObservers = new nsAutoVoidArray();
    if (nsnull == mObservers)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  pCallback = (PrefCallbackData *)NS_Alloc(sizeof(PrefCallbackData) + strlen(aDomain));
  if (nsnull == pCallback)
    return NS_ERROR_OUT_OF_MEMORY;

  pCallback->pBranch = this;
  pCallback->pObserver = aObserver;

  
  if (aHoldWeak) {
    nsCOMPtr<nsISupportsWeakReference> weakRefFactory = do_QueryInterface(aObserver);
    if (!weakRefFactory) {
      
      nsMemory::Free(pCallback);
      return NS_ERROR_INVALID_ARG;
    }
    nsCOMPtr<nsIWeakReference> tmp = do_GetWeakReference(weakRefFactory);
    NS_ADDREF(pCallback->pWeakRef = tmp);
  } else {
    pCallback->pWeakRef = nsnull;
    NS_ADDREF(pCallback->pObserver);
  }

  strcpy(pCallback->pDomain, aDomain);
  mObservers->AppendElement(pCallback);

  
  pref = getPrefName(aDomain); 
  PREF_RegisterCallback(pref, NotifyObserver, pCallback);
  return NS_OK;
}

NS_IMETHODIMP nsPrefBranch::RemoveObserver(const char *aDomain, nsIObserver *aObserver)
{
  const char *pref;
  PrefCallbackData *pCallback;
  PRInt32 count;
  PRInt32 i;
  nsresult rv;
  nsCAutoString domain;

  NS_ENSURE_ARG_POINTER(aDomain);
  NS_ENSURE_ARG_POINTER(aObserver);

  if (!mObservers)
    return NS_OK;
    
  
  count = mObservers->Count();
  if (count == 0)
    return NS_OK;

  for (i = 0; i < count; i++) {
    pCallback = (PrefCallbackData *)mObservers->ElementAt(i);
    if (pCallback &&
        pCallback->pObserver == aObserver &&
        !strcmp(pCallback->pDomain, aDomain)) {
      
      pref = getPrefName(aDomain); 
      rv = PREF_UnregisterCallback(pref, NotifyObserver, pCallback);
      if (NS_SUCCEEDED(rv)) {
        
        
        mObservers->RemoveElementAt(i);
        if (pCallback->pWeakRef) {
          NS_RELEASE(pCallback->pWeakRef);
        } else {
          NS_RELEASE(pCallback->pObserver);
        }
        NS_Free(pCallback);
      }
      return rv;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsPrefBranch::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  
  if (!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    freeObserverList();
  }
  return NS_OK;
}

static nsresult NotifyObserver(const char *newpref, void *data)
{
  PrefCallbackData *pData = (PrefCallbackData *)data;

  
  
  PRUint32 len = pData->pBranch->GetRootLength();
  nsCAutoString suffix(newpref + len);  

  nsCOMPtr<nsIObserver> observer;
  if (pData->pWeakRef) {
    observer = do_QueryReferent(pData->pWeakRef);
    if (!observer) {
      
      pData->pBranch->RemoveObserver(pData->pDomain, pData->pObserver);
      return NS_OK;
    }
  } else {
    observer = pData->pObserver;
  }

  observer->Observe(static_cast<nsIPrefBranch *>(pData->pBranch),
                    NS_PREFBRANCH_PREFCHANGE_TOPIC_ID,
                    NS_ConvertASCIItoUTF16(suffix).get());
  return NS_OK;
}


void nsPrefBranch::freeObserverList(void)
{
  const char *pref;
  PrefCallbackData *pCallback;

  if (mObservers) {
    
    PRInt32 count;

    count = mObservers->Count();
    if (count > 0) {
      PRInt32 i;
      nsCAutoString domain;
      for (i = 0; i < count; ++i) {
        pCallback = (PrefCallbackData *)mObservers->ElementAt(i);
        if (pCallback) {
          
          pref = getPrefName(pCallback->pDomain);
          
          
          mObservers->ReplaceElementAt(nsnull, i);
          PREF_UnregisterCallback(pref, NotifyObserver, pCallback);
          if (pCallback->pWeakRef) {
            NS_RELEASE(pCallback->pWeakRef);
          } else {
            NS_RELEASE(pCallback->pObserver);
          }
          nsMemory::Free(pCallback);
        }
      }
    }
    delete mObservers;
    mObservers = 0;
  }
}
 
nsresult nsPrefBranch::GetDefaultFromPropertiesFile(const char *aPrefName, PRUnichar **return_buf)
{
  nsresult rv;

  
    
  nsXPIDLCString propertyFileURL;
  rv = PREF_CopyCharPref(aPrefName, getter_Copies(propertyFileURL), PR_TRUE);
  if (NS_FAILED(rv))
    return rv;
    
  nsCOMPtr<nsIStringBundleService> bundleService =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIStringBundle> bundle;
  rv = bundleService->CreateBundle(propertyFileURL,
                                   getter_AddRefs(bundle));
  if (NS_FAILED(rv))
    return rv;

  
  nsAutoString stringId;
  stringId.AssignASCII(aPrefName);

  return bundle->GetStringFromName(stringId.get(), return_buf);
}

const char *nsPrefBranch::getPrefName(const char *aPrefName)
{
  
  if (mPrefRoot.IsEmpty())
    return aPrefName;

  
  mPrefRoot.Truncate(mPrefRootLength);

  
  if ((nsnull != aPrefName) && (*aPrefName != '\0'))
    mPrefRoot.Append(aPrefName);

  return mPrefRoot.get();
}

nsresult nsPrefBranch::getValidatedPrefName(const char *aPrefName, const char **_retval)
{
  static const char capabilityPrefix[] = "capability.";

  NS_ENSURE_ARG_POINTER(aPrefName);
  const char *fullPref = getPrefName(aPrefName);

  
  if ((fullPref[0] == 'c') &&
    PL_strncmp(fullPref, capabilityPrefix, sizeof(capabilityPrefix)-1) == 0)
  {
    nsresult rv;
    nsCOMPtr<nsIPrefSecurityCheck> secCheck = 
             do_GetService(NS_GLOBAL_PREF_SECURITY_CHECK, &rv);

    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;

    PRBool enabled;
    rv = secCheck->CanAccessSecurityPreferences(&enabled);
    if (NS_FAILED(rv) || !enabled)
      return NS_ERROR_FAILURE;
  }

  *_retval = fullPref;
  return NS_OK;
}

static PLDHashOperator
pref_enumChild(PLDHashTable *table, PLDHashEntryHdr *heh,
               PRUint32 i, void *arg)
{
  PrefHashEntry *he = static_cast<PrefHashEntry*>(heh);
  EnumerateData *d = reinterpret_cast<EnumerateData *>(arg);
  if (PL_strncmp(he->key, d->parent, PL_strlen(d->parent)) == 0) {
    d->pref_list->AppendElement((void*)he->key);
  }
  return PL_DHASH_NEXT;
}










NS_IMETHODIMP nsPrefBranch::SecurityGetBoolPref(const char *pref, PRBool * return_val)
{
  return PREF_GetBoolPref(getPrefName(pref), return_val, PR_FALSE);
}

NS_IMETHODIMP nsPrefBranch::SecuritySetBoolPref(const char *pref, PRBool value)
{
  return PREF_SetBoolPref(getPrefName(pref), value);
}

NS_IMETHODIMP nsPrefBranch::SecurityGetCharPref(const char *pref, char ** return_buf)
{
  return PREF_CopyCharPref(getPrefName(pref), return_buf, PR_FALSE);
}

NS_IMETHODIMP nsPrefBranch::SecuritySetCharPref(const char *pref, const char* value)
{
  return PREF_SetCharPref(getPrefName(pref), value);
}

NS_IMETHODIMP nsPrefBranch::SecurityGetIntPref(const char *pref, PRInt32 * return_val)
{
  return PREF_GetIntPref(getPrefName(pref), return_val, PR_FALSE);
}

NS_IMETHODIMP nsPrefBranch::SecuritySetIntPref(const char *pref, PRInt32 value)
{
  return PREF_SetIntPref(getPrefName(pref), value);
}

NS_IMETHODIMP nsPrefBranch::SecurityClearUserPref(const char *pref_name)
{
  return PREF_ClearUserPref(getPrefName(pref_name));
}





nsPrefLocalizedString::nsPrefLocalizedString()
{
}

nsPrefLocalizedString::~nsPrefLocalizedString()
{
}






NS_IMPL_THREADSAFE_ADDREF(nsPrefLocalizedString)
NS_IMPL_THREADSAFE_RELEASE(nsPrefLocalizedString)

NS_INTERFACE_MAP_BEGIN(nsPrefLocalizedString)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPrefLocalizedString)
    NS_INTERFACE_MAP_ENTRY(nsIPrefLocalizedString)
    NS_INTERFACE_MAP_ENTRY(nsISupportsString)
NS_INTERFACE_MAP_END

nsresult nsPrefLocalizedString::Init()
{
  nsresult rv;
  mUnicodeString = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);

  return rv;
}

NS_IMETHODIMP
nsPrefLocalizedString::GetData(PRUnichar** _retval)
{
  nsAutoString data;

  nsresult rv = GetData(data);
  if (NS_FAILED(rv))
    return rv;
  
  *_retval = ToNewUnicode(data);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsPrefLocalizedString::SetData(const PRUnichar *aData)
{
  if (!aData)
    return SetData(EmptyString());
  return SetData(nsDependentString(aData));
}

NS_IMETHODIMP
nsPrefLocalizedString::SetDataWithLength(PRUint32 aLength,
                                         const PRUnichar* aData)
{
  if (!aData)
    return SetData(EmptyString());
  return SetData(Substring(aData, aData + aLength));
}





NS_IMPL_THREADSAFE_ISUPPORTS1(nsRelativeFilePref, nsIRelativeFilePref)

nsRelativeFilePref::nsRelativeFilePref()
{
}

nsRelativeFilePref::~nsRelativeFilePref()
{
}

NS_IMETHODIMP nsRelativeFilePref::GetFile(nsILocalFile * *aFile)
{
    NS_ENSURE_ARG_POINTER(aFile);
    *aFile = mFile;
    NS_IF_ADDREF(*aFile);
    return NS_OK;
}

NS_IMETHODIMP nsRelativeFilePref::SetFile(nsILocalFile * aFile)
{
    mFile = aFile;
    return NS_OK;
}

NS_IMETHODIMP nsRelativeFilePref::GetRelativeToKey(nsACString& aRelativeToKey)
{
    aRelativeToKey.Assign(mRelativeToKey);
    return NS_OK;
}

NS_IMETHODIMP nsRelativeFilePref::SetRelativeToKey(const nsACString& aRelativeToKey)
{
    mRelativeToKey.Assign(aRelativeToKey);
    return NS_OK;
}
