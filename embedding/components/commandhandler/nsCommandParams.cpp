




#include "xpcom-config.h"
#include <new>    
#include "nscore.h"
#include "nsCRT.h"

#include "nsCommandParams.h"
#include "mozilla/HashFunctions.h"

using namespace mozilla;

const PLDHashTableOps nsCommandParams::sHashOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HashKey,
    HashMatchEntry,
    HashMoveEntry,
    HashClearEntry,
    PL_DHashFinalizeStub
};


NS_IMPL_ISUPPORTS(nsCommandParams, nsICommandParams)

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
  PL_DHashTableInit(&mValuesHash, &sHashOps, (void *)this,
                    sizeof(HashEntry), 2);
  return NS_OK;
}

#if 0
#pragma mark -
#endif


NS_IMETHODIMP nsCommandParams::GetValueType(const char * name, int16_t *_retval)
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


NS_IMETHODIMP nsCommandParams::GetBooleanValue(const char * name, bool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = false;

  HashEntry*  foundEntry = GetNamedEntry(name);
  if (foundEntry && foundEntry->mEntryType == eBooleanType)
  {
    *_retval = foundEntry->mData.mBoolean;
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsCommandParams::GetLongValue(const char * name, int32_t *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = false;

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
    *_retval = ToNewCString(*foundEntry->mData.mCString);
    return NS_OK;
  }
  
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsCommandParams::GetISupportsValue(const char * name, nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nullptr;

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


NS_IMETHODIMP nsCommandParams::SetBooleanValue(const char * name, bool value)
{
  HashEntry* foundEntry = GetOrMakeEntry(name, eBooleanType);
  if (!foundEntry) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  foundEntry->mData.mBoolean = value;

  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetLongValue(const char * name, int32_t value)
{
  HashEntry* foundEntry = GetOrMakeEntry(name, eLongType);
  if (!foundEntry) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  foundEntry->mData.mLong = value;
  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetDoubleValue(const char * name, double value)
{
  HashEntry* foundEntry = GetOrMakeEntry(name, eDoubleType);
  if (!foundEntry) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  foundEntry->mData.mDouble = value;
  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetStringValue(const char * name, const nsAString & value)
{
  HashEntry* foundEntry = GetOrMakeEntry(name, eWStringType);
  if (!foundEntry) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  foundEntry->mData.mString = new nsString(value);
  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetCStringValue(const char * name, const char * value)
{
  HashEntry* foundEntry = GetOrMakeEntry(name, eStringType);
  if (!foundEntry) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  foundEntry->mData.mCString = new nsCString(value);
  return NS_OK;
}


NS_IMETHODIMP nsCommandParams::SetISupportsValue(const char * name, nsISupports *value)
{
  HashEntry* foundEntry = GetOrMakeEntry(name, eISupportsType);
  if (!foundEntry) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
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
   
  return nullptr;
}


nsCommandParams::HashEntry*
nsCommandParams::GetIndexedEntry(int32_t index)
{
  HashEntry*  entry = reinterpret_cast<HashEntry*>(mValuesHash.entryStore);
  HashEntry*  limit = entry + PL_DHASH_TABLE_CAPACITY(&mValuesHash);
  uint32_t    entryCount = 0;

  do {
    if (!PL_DHASH_ENTRY_IS_LIVE(entry))
      continue;

    if ((int32_t)entryCount == index)
      return entry;

    entryCount ++;
  } while (++entry < limit);

  return nullptr;
}


uint32_t
nsCommandParams::GetNumEntries()
{
  HashEntry*  entry = reinterpret_cast<HashEntry*>(mValuesHash.entryStore);
  HashEntry*  limit = entry + PL_DHASH_TABLE_CAPACITY(&mValuesHash);
  uint32_t    entryCount = 0;

  do {
    if (PL_DHASH_ENTRY_IS_LIVE(entry))
      entryCount ++;
  } while (++entry < limit);

  return entryCount;
}

nsCommandParams::HashEntry*
nsCommandParams::GetOrMakeEntry(const char * name, uint8_t entryType)
{
  HashEntry *foundEntry = (HashEntry *)PL_DHashTableOperate(&mValuesHash, (void *)name, PL_DHASH_LOOKUP);
  if (PL_DHASH_ENTRY_IS_BUSY(foundEntry)) { 
    foundEntry->Reset(entryType);
    return foundEntry;
  }

  foundEntry = (HashEntry *)PL_DHashTableOperate(&mValuesHash, (void *)name, PL_DHASH_ADD);
  if (!foundEntry) {
    return nullptr;
  }

  
  new (foundEntry) HashEntry(entryType, name);
  return foundEntry;
}

#if 0
#pragma mark -
#endif

PLDHashNumber
nsCommandParams::HashKey(PLDHashTable *table, const void *key)
{
  return HashString((const char *)key);
}

bool
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

  new (toEntry) HashEntry(*fromEntry);

  fromEntry->~HashEntry();      
}

void
nsCommandParams::HashClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  HashEntry*    thisEntry = static_cast<HashEntry*>(entry);
  thisEntry->~HashEntry();      
}

#if 0
#pragma mark -
#endif


NS_IMETHODIMP
nsCommandParams::HasMoreElements(bool *_retval)
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
  
  *_retval = ToNewCString(thisEntry->mEntryName);
  mCurEntry++;
  return NS_OK;
}
