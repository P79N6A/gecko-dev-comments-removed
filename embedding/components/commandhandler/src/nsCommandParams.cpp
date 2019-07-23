




































#include "xpcom-config.h"
#include NEW_H    
#include "nscore.h"
#include "nsCRT.h"

#include "nsCommandParams.h"


PLDHashTableOps nsCommandParams::sHashOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashKey,
    HashMatchEntry,
    HashMoveEntry,
    HashClearEntry,
    PL_DHashFinalizeStub
};


NS_IMPL_ISUPPORTS1(nsCommandParams, nsICommandParams)

nsCommandParams::nsCommandParams()
: mCurEntry(0)
, mNumEntries(eNumEntriesUnknown)
{
  
}

nsCommandParams::~nsCommandParams()
{
  PL_DHashTableFinish(&mValuesHash);
}

nsresult
nsCommandParams::Init()
{
  if (!PL_DHashTableInit(&mValuesHash, &sHashOps, (void *)this, sizeof(HashEntry), 4))
    return NS_ERROR_FAILURE;
    
  return NS_OK;
}

#if 0
#pragma mark -
#endif


NS_IMETHODIMP nsCommandParams::GetValueType(const char * name, PRInt16 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = eNoType;
  HashEntry*  foundEntry = GetNamedEntry(name);
  if (foundEntry)
  {
    *_retval = foundEntry->mEntryType;
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsCommandParams::GetBooleanValue(const char * name, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;

  HashEntry*  foundEntry = GetNamedEntry(name);
  if (foundEntry && foundEntry->mEntryType == eBooleanType)
  {
    *_retval = foundEntry->mData.mBoolean;
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsCommandParams::GetLongValue(const char * name, PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;

  HashEntry*  foundEntry = GetNamedEntry(name);
  if (foundEntry && foundEntry->mEntryType == eLongType)
  {
    *_retval = foundEntry->mData.mLong;
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsCommandParams::GetDoubleValue(const char * name, double *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 0.0;

  HashEntry*  foundEntry = GetNamedEntry(name);
  if (foundEntry && foundEntry->mEntryType == eDoubleType)
  {
    *_retval = foundEntry->mData.mDouble;
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsCommandParams::GetStringValue(const char *name, nsAString & _retval)
{
  _retval.Truncate();
  HashEntry*  foundEntry = GetNamedEntry(name);
  if (foundEntry && foundEntry->mEntryType == eWStringType)
  {
    NS_ASSERTION(foundEntry->mData.mString, "Null string");
    _retval.Assign(*foundEntry->mData.mString);
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsCommandParams::GetCStringValue(const char * name, char **_retval)
{
  HashEntry*  foundEntry = GetNamedEntry(name);
  if (foundEntry && foundEntry->mEntryType == eStringType)
  {
    NS_ASSERTION(foundEntry->mData.mCString, "Null string");
    *_retval= nsCRT::strdup((*foundEntry->mData.mCString).get());
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsCommandParams::GetISupportsValue(const char * name, nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  HashEntry*  foundEntry = GetNamedEntry(name);
  if (foundEntry && foundEntry->mEntryType == eISupportsType)
  {
    NS_IF_ADDREF(*_retval = foundEntry->mISupports.get());
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}

#if 0
#pragma mark -
#endif


NS_IMETHODIMP nsCommandParams::SetBooleanValue(const char * name, PRBool value)
{
  HashEntry*  foundEntry;
  GetOrMakeEntry(name, eBooleanType, foundEntry);
  if (!foundEntry) return NS_ERROR_OUT_OF_MEMORY;
  
  foundEntry->mData.mBoolean = value;
  
  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetLongValue(const char * name, PRInt32 value)
{
  HashEntry*  foundEntry;
  GetOrMakeEntry(name, eLongType, foundEntry);
  if (!foundEntry) return NS_ERROR_OUT_OF_MEMORY;
  
  foundEntry->mData.mLong = value;
  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetDoubleValue(const char * name, double value)
{
  HashEntry*  foundEntry;
  GetOrMakeEntry(name, eDoubleType, foundEntry);
  if (!foundEntry) return NS_ERROR_OUT_OF_MEMORY;
  
  foundEntry->mData.mDouble = value;
  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetStringValue(const char * name, const nsAString & value)
{
  HashEntry*  foundEntry;
  GetOrMakeEntry(name, eWStringType, foundEntry);
  if (!foundEntry) return NS_ERROR_OUT_OF_MEMORY;
  
  foundEntry->mData.mString = new nsString(value);
  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetCStringValue(const char * name, const char * value)
{
  HashEntry*  foundEntry;
  GetOrMakeEntry(name, eStringType, foundEntry);
  if (!foundEntry)
    return NS_ERROR_OUT_OF_MEMORY;
  foundEntry->mData.mCString = new nsCString(value);
  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetISupportsValue(const char * name, nsISupports *value)
{
  HashEntry*  foundEntry;
  GetOrMakeEntry(name, eISupportsType, foundEntry);
  if (!foundEntry) return NS_ERROR_OUT_OF_MEMORY;
  
  foundEntry->mISupports = value;   
  return NS_OK;
}


NS_IMETHODIMP
nsCommandParams::RemoveValue(const char * name)
{
  
  
  (void)PL_DHashTableOperate(&mValuesHash, (void *)name, PL_DHASH_REMOVE);

  
  mNumEntries = eNumEntriesUnknown;
  return NS_OK;
}

#if 0
#pragma mark -
#endif

nsCommandParams::HashEntry*
nsCommandParams::GetNamedEntry(const char * name)
{
  HashEntry *foundEntry = (HashEntry *)PL_DHashTableOperate(&mValuesHash, (void *)name, PL_DHASH_LOOKUP);

  if (PL_DHASH_ENTRY_IS_BUSY(foundEntry))
    return foundEntry;
   
  return nsnull;
}


nsCommandParams::HashEntry*
nsCommandParams::GetIndexedEntry(PRInt32 index)
{
  HashEntry*  entry = reinterpret_cast<HashEntry*>(mValuesHash.entryStore);
  HashEntry*  limit = entry + PL_DHASH_TABLE_SIZE(&mValuesHash);
  PRUint32    entryCount = 0;
  
  do
  {  
    if (!PL_DHASH_ENTRY_IS_LIVE(entry))
      continue;

    if ((PRInt32)entryCount == index)
      return entry;
    
    entryCount ++;
  } while (++entry < limit);

  return nsnull;
}


PRUint32
nsCommandParams::GetNumEntries()
{
  HashEntry*  entry = reinterpret_cast<HashEntry*>(mValuesHash.entryStore);
  HashEntry*  limit = entry + PL_DHASH_TABLE_SIZE(&mValuesHash);
  PRUint32    entryCount = 0;
  
  do
  {  
    if (PL_DHASH_ENTRY_IS_LIVE(entry))
      entryCount ++;
  } while (++entry < limit);

  return entryCount;
}

nsresult
nsCommandParams::GetOrMakeEntry(const char * name, PRUint8 entryType, HashEntry*& outEntry)
{

  HashEntry *foundEntry = (HashEntry *)PL_DHashTableOperate(&mValuesHash, (void *)name, PL_DHASH_LOOKUP);
  if (PL_DHASH_ENTRY_IS_BUSY(foundEntry))   
  {
    foundEntry->Reset(entryType);
    foundEntry->mEntryName.Assign(name);
    outEntry = foundEntry;
    return NS_OK;
  }

  foundEntry = (HashEntry *)PL_DHashTableOperate(&mValuesHash, (void *)name, PL_DHASH_ADD);
  if (!foundEntry) return NS_ERROR_OUT_OF_MEMORY;
  
  
  outEntry = new (foundEntry) HashEntry(entryType, name);  
  return NS_OK;
}

#if 0
#pragma mark -
#endif

PLDHashNumber
nsCommandParams::HashKey(PLDHashTable *table, const void *key)
{
  return nsCRT::HashCode((const char *)key);
}

PRBool
nsCommandParams::HashMatchEntry(PLDHashTable *table,
                                const PLDHashEntryHdr *entry, const void *key)
{
  const char*   keyString = (const char*)key;
  const HashEntry*   thisEntry = static_cast<const HashEntry*>(entry);
  
  return thisEntry->mEntryName.Equals(keyString);
}

void
nsCommandParams::HashMoveEntry(PLDHashTable *table, const PLDHashEntryHdr *from,
                                PLDHashEntryHdr *to)
{
  const HashEntry*   fromEntry  = static_cast<const HashEntry*>(from);
  HashEntry*         toEntry    = static_cast<HashEntry*>(to);
  
  *toEntry = *fromEntry;
  
}

void
nsCommandParams::HashClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  HashEntry*    thisEntry = static_cast<HashEntry*>(entry);
  thisEntry->~HashEntry();      
  memset(thisEntry, 0, sizeof(HashEntry));    
}

#if 0
#pragma mark -
#endif


NS_IMETHODIMP
nsCommandParams::HasMoreElements(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  if (mNumEntries == eNumEntriesUnknown)
    mNumEntries = GetNumEntries();
  
  *_retval = mCurEntry < mNumEntries;
  return NS_OK;
}


NS_IMETHODIMP
nsCommandParams::First()
{
  mCurEntry = 0;
  return NS_OK;
}


NS_IMETHODIMP
nsCommandParams::GetNext(char **_retval)
{
  HashEntry*    thisEntry = GetIndexedEntry(mCurEntry);
  if (!thisEntry)
    return NS_ERROR_FAILURE;
  
  *_retval = nsCRT::strdup(thisEntry->mEntryName.get());
  mCurEntry++;
  return NS_OK;
}
