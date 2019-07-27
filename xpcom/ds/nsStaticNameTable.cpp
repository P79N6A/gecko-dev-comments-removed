







#include "nsCRT.h"

#include "nscore.h"
#include "mozilla/HashFunctions.h"
#include "nsISupportsImpl.h"

#define PL_ARENA_CONST_ALIGN_MASK 3
#include "nsStaticNameTable.h"

using namespace mozilla;

struct NameTableKey
{
  explicit NameTableKey(const nsAFlatCString* aKeyStr)
    : mIsUnichar(false)
  {
    mKeyStr.m1b = aKeyStr;
  }

  explicit NameTableKey(const nsAFlatString* aKeyStr)
    : mIsUnichar(true)
  {
    mKeyStr.m2b = aKeyStr;
  }

  bool mIsUnichar;
  union
  {
    const nsAFlatCString* m1b;
    const nsAFlatString* m2b;
  } mKeyStr;
};

struct NameTableEntry : public PLDHashEntryHdr
{
  
  const nsAFlatCString* mString;
  int32_t mIndex;
};

static bool
matchNameKeysCaseInsensitive(PLDHashTable*, const PLDHashEntryHdr* aHdr,
                             const void* aKey)
{
  const NameTableEntry* entry = static_cast<const NameTableEntry*>(aHdr);
  const NameTableKey* keyValue = static_cast<const NameTableKey*>(aKey);
  const nsAFlatCString* entryKey = entry->mString;

  if (keyValue->mIsUnichar) {
    return keyValue->mKeyStr.m2b->LowerCaseEqualsASCII(entryKey->get(),
                                                       entryKey->Length());
  }

  return keyValue->mKeyStr.m1b->LowerCaseEqualsASCII(entryKey->get(),
                                                     entryKey->Length());
}









static PLDHashNumber
caseInsensitiveStringHashKey(PLDHashTable* aTable, const void* aKey)
{
  PLDHashNumber h = 0;
  const NameTableKey* tableKey = static_cast<const NameTableKey*>(aKey);
  if (tableKey->mIsUnichar) {
    for (const char16_t* s = tableKey->mKeyStr.m2b->get();
         *s != '\0';
         s++) {
      h = AddToHash(h, *s & ~0x20);
    }
  } else {
    for (const unsigned char* s = reinterpret_cast<const unsigned char*>(
           tableKey->mKeyStr.m1b->get());
         *s != '\0';
         s++) {
      h = AddToHash(h, *s & ~0x20);
    }
  }
  return h;
}

static const struct PLDHashTableOps nametable_CaseInsensitiveHashTableOps = {
  caseInsensitiveStringHashKey,
  matchNameKeysCaseInsensitive,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  nullptr,
};

nsStaticCaseInsensitiveNameTable::nsStaticCaseInsensitiveNameTable()
  : mNameArray(nullptr)
  , mNullStr("")
{
  MOZ_COUNT_CTOR(nsStaticCaseInsensitiveNameTable);
}

nsStaticCaseInsensitiveNameTable::~nsStaticCaseInsensitiveNameTable()
{
  if (mNameArray) {
    
    for (uint32_t index = 0; index < mNameTable.EntryCount(); index++) {
      mNameArray[index].~nsDependentCString();
    }
    free((void*)mNameArray);
  }
  if (mNameTable.IsInitialized()) {
    PL_DHashTableFinish(&mNameTable);
  }
  MOZ_COUNT_DTOR(nsStaticCaseInsensitiveNameTable);
}

bool
nsStaticCaseInsensitiveNameTable::Init(const char* const aNames[],
                                       int32_t aLength)
{
  NS_ASSERTION(!mNameArray, "double Init");
  NS_ASSERTION(!mNameTable.IsInitialized(), "double Init");
  NS_ASSERTION(aNames, "null name table");
  NS_ASSERTION(aLength, "0 length");

  mNameArray = (nsDependentCString*)
    moz_xmalloc(aLength * sizeof(nsDependentCString));
  if (!mNameArray) {
    return false;
  }

  PL_DHashTableInit(&mNameTable, &nametable_CaseInsensitiveHashTableOps,
                    sizeof(NameTableEntry), aLength);

  for (int32_t index = 0; index < aLength; ++index) {
    const char* raw = aNames[index];
#ifdef DEBUG
    {
      
      nsAutoCString temp1(raw);
      nsDependentCString temp2(raw);
      ToLowerCase(temp1);
      NS_ASSERTION(temp1.Equals(temp2), "upper case char in table");
      NS_ASSERTION(nsCRT::IsAscii(raw),
                   "non-ascii string in table -- "
                   "case-insensitive matching won't work right");
    }
#endif
    
    nsDependentCString* strPtr = &mNameArray[index];
    new (strPtr) nsDependentCString(raw);

    NameTableKey key(strPtr);

    NameTableEntry* entry = static_cast<NameTableEntry*>
      (PL_DHashTableAdd(&mNameTable, &key, fallible));
    if (!entry) {
      continue;
    }

    NS_ASSERTION(entry->mString == 0, "Entry already exists!");

    entry->mString = strPtr;      
    entry->mIndex = index;
  }
#ifdef DEBUG
  PL_DHashMarkTableImmutable(&mNameTable);
#endif
  return true;
}

int32_t
nsStaticCaseInsensitiveNameTable::Lookup(const nsACString& aName)
{
  NS_ASSERTION(mNameArray, "not inited");
  NS_ASSERTION(mNameTable.IsInitialized(), "not inited");

  const nsAFlatCString& str = PromiseFlatCString(aName);

  NameTableKey key(&str);
  NameTableEntry* entry =
    static_cast<NameTableEntry*>(PL_DHashTableSearch(&mNameTable, &key));

  return entry ? entry->mIndex : nsStaticCaseInsensitiveNameTable::NOT_FOUND;
}

int32_t
nsStaticCaseInsensitiveNameTable::Lookup(const nsAString& aName)
{
  NS_ASSERTION(mNameArray, "not inited");
  NS_ASSERTION(mNameTable.IsInitialized(), "not inited");

  const nsAFlatString& str = PromiseFlatString(aName);

  NameTableKey key(&str);
  NameTableEntry* entry =
    static_cast<NameTableEntry*>(PL_DHashTableSearch(&mNameTable, &key));

  return entry ? entry->mIndex : nsStaticCaseInsensitiveNameTable::NOT_FOUND;
}

const nsAFlatCString&
nsStaticCaseInsensitiveNameTable::GetStringValue(int32_t aIndex)
{
  NS_ASSERTION(mNameArray, "not inited");
  NS_ASSERTION(mNameTable.IsInitialized(), "not inited");

  if ((NOT_FOUND < aIndex) && ((uint32_t)aIndex < mNameTable.EntryCount())) {
    return mNameArray[aIndex];
  }
  return mNullStr;
}
