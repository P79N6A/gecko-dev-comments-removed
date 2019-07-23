








































#include "nsCRT.h"

#include "nscore.h"
#include "nsString.h"
#include "nsReadableUtils.h"

#define PL_ARENA_CONST_ALIGN_MASK 3
#include "nsStaticNameTable.h"

struct NameTableKey
{
    NameTableKey(const nsAFlatCString* aKeyStr)
        : mIsUnichar(PR_FALSE)
    {
        mKeyStr.m1b = aKeyStr;
    }
        
    NameTableKey(const nsAFlatString* aKeyStr)
        : mIsUnichar(PR_TRUE)
    {
        mKeyStr.m2b = aKeyStr;
    }

    PRBool mIsUnichar;
    union {
        const nsAFlatCString* m1b;
        const nsAFlatString* m2b;
    } mKeyStr;
};

struct NameTableEntry : public PLDHashEntryHdr
{
    
    const nsAFlatCString* mString;
    PRInt32 mIndex;
};

PR_STATIC_CALLBACK(PRBool)
matchNameKeysCaseInsensitive(PLDHashTable*, const PLDHashEntryHdr* aHdr,
                             const void* key)
{
    const NameTableEntry* entry =
        NS_STATIC_CAST(const NameTableEntry *, aHdr);
    const NameTableKey *keyValue = NS_STATIC_CAST(const NameTableKey*, key);

    const nsAFlatCString* entryKey = entry->mString;
    
    if (keyValue->mIsUnichar) {
        return keyValue->mKeyStr.m2b->
            LowerCaseEqualsASCII(entryKey->get(), entryKey->Length());
    }

    return keyValue->mKeyStr.m1b->
        LowerCaseEqualsASCII(entryKey->get(), entryKey->Length());
}









PR_STATIC_CALLBACK(PLDHashNumber)
caseInsensitiveStringHashKey(PLDHashTable *table, const void *key)
{
    PLDHashNumber h = 0;
    const NameTableKey* tableKey = NS_STATIC_CAST(const NameTableKey*, key);
    if (tableKey->mIsUnichar) {
        for (const PRUnichar* s = tableKey->mKeyStr.m2b->get();
             *s != '\0';
             s++)
            h = (h >> (PL_DHASH_BITS - 4)) ^ (h << 4) ^ (*s & ~0x20);
    } else {
        for (const unsigned char* s =
                 NS_REINTERPRET_CAST(const unsigned char*,
                                     tableKey->mKeyStr.m1b->get());
             *s != '\0';
             s++)
            h = (h >> (PL_DHASH_BITS - 4)) ^ (h << 4) ^ (*s & ~0x20);
    }
    return h;
}

static const struct PLDHashTableOps nametable_CaseInsensitiveHashTableOps = {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    caseInsensitiveStringHashKey,
    matchNameKeysCaseInsensitive,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub,
    nsnull,
};

nsStaticCaseInsensitiveNameTable::nsStaticCaseInsensitiveNameTable()
  : mNameArray(nsnull), mNullStr("")
{
    MOZ_COUNT_CTOR(nsStaticCaseInsensitiveNameTable);
    mNameTable.ops = nsnull;
}

nsStaticCaseInsensitiveNameTable::~nsStaticCaseInsensitiveNameTable()
{
    if (mNameArray) {
        
        for (PRUint32 index = 0; index < mNameTable.entryCount; index++) {
            mNameArray[index].~nsDependentCString();
        }
        nsMemory::Free((void*)mNameArray);
    }
    if (mNameTable.ops)
        PL_DHashTableFinish(&mNameTable);
    MOZ_COUNT_DTOR(nsStaticCaseInsensitiveNameTable);
}

PRBool 
nsStaticCaseInsensitiveNameTable::Init(const char* const aNames[], PRInt32 Count)
{
    NS_ASSERTION(!mNameArray, "double Init");
    NS_ASSERTION(!mNameTable.ops, "double Init");
    NS_ASSERTION(aNames, "null name table");
    NS_ASSERTION(Count, "0 count");

    mNameArray = (nsDependentCString*)
                   nsMemory::Alloc(Count * sizeof(nsDependentCString));
    if (!mNameArray)
        return PR_FALSE;

    if (!PL_DHashTableInit(&mNameTable,
                           &nametable_CaseInsensitiveHashTableOps,
                           nsnull, sizeof(NameTableEntry), Count)) {
        mNameTable.ops = nsnull;
        return PR_FALSE;
    }

    for (PRInt32 index = 0; index < Count; ++index) {
        const char* raw = aNames[index];
#ifdef DEBUG
        {
            
            nsCAutoString temp1(raw);
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

        NameTableEntry *entry =
          NS_STATIC_CAST(NameTableEntry*,
                         PL_DHashTableOperate(&mNameTable, &key,
                                              PL_DHASH_ADD));

        if (!entry) continue;

        NS_ASSERTION(entry->mString == 0, "Entry already exists!");

        entry->mString = strPtr;      
        entry->mIndex = index;
    }
    return PR_TRUE;
}

PRInt32
nsStaticCaseInsensitiveNameTable::Lookup(const nsACString& aName)
{
    NS_ASSERTION(mNameArray, "not inited");
    NS_ASSERTION(mNameTable.ops, "not inited");

    const nsAFlatCString& str = PromiseFlatCString(aName);

    NameTableKey key(&str);
    NameTableEntry *entry =
        NS_STATIC_CAST(NameTableEntry*,
                       PL_DHashTableOperate(&mNameTable, &key,
                                            PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_FREE(entry))
        return nsStaticCaseInsensitiveNameTable::NOT_FOUND;

    return entry->mIndex;
}

PRInt32
nsStaticCaseInsensitiveNameTable::Lookup(const nsAString& aName)
{
    NS_ASSERTION(mNameArray, "not inited");
    NS_ASSERTION(mNameTable.ops, "not inited");

    const nsAFlatString& str = PromiseFlatString(aName);

    NameTableKey key(&str);
    NameTableEntry *entry =
        NS_STATIC_CAST(NameTableEntry*,
                       PL_DHashTableOperate(&mNameTable, &key,
                                            PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_FREE(entry))
        return nsStaticCaseInsensitiveNameTable::NOT_FOUND;

    return entry->mIndex;
}

const nsAFlatCString& 
nsStaticCaseInsensitiveNameTable::GetStringValue(PRInt32 index)
{
    NS_ASSERTION(mNameArray, "not inited");
    NS_ASSERTION(mNameTable.ops, "not inited");

    if ((NOT_FOUND < index) && ((PRUint32)index < mNameTable.entryCount)) {
        return mNameArray[index];
    }
    return mNullStr;
}
