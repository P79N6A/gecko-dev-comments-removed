
















































#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsElementMap.h"
#include "nsISupportsArray.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsReadableUtils.h"
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gMapLog;
#endif

static void* PR_CALLBACK AllocTable(void* aPool, PRSize aSize)
{
    return new char[aSize];
}

static void PR_CALLBACK FreeTable(void* aPool, void* aItem)
{
    delete[] static_cast<char*>(aItem);
}

static PLHashEntry* PR_CALLBACK AllocEntry(void* aPool, const void* aKey)
{
    nsFixedSizeAllocator* pool = static_cast<nsFixedSizeAllocator*>(aPool);
    PLHashEntry* entry = static_cast<PLHashEntry*>(pool->Alloc(sizeof(PLHashEntry)));
    return entry;
}

static void PR_CALLBACK FreeEntry(void* aPool, PLHashEntry* aEntry, PRUintn aFlag) 
{
    nsFixedSizeAllocator* pool = static_cast<nsFixedSizeAllocator*>(aPool);
    if (aFlag == HT_FREE_ENTRY)
        pool->Free(aEntry, sizeof(PLHashEntry));
}

PLHashAllocOps nsElementMap::gAllocOps = {
    AllocTable, FreeTable, AllocEntry, FreeEntry };


nsElementMap::nsElementMap()
{
    MOZ_COUNT_CTOR(nsElementMap);

    
    static const size_t kBucketSizes[] = {
        sizeof(PLHashEntry), sizeof(ContentListItem)
    };

    static const PRInt32 kNumBuckets = sizeof(kBucketSizes) / sizeof(size_t);

    static const PRInt32 kInitialNumElements = 64;

    
    static const PRInt32 kInitialPoolSize = 512;

    mPool.Init("nsElementMap", kBucketSizes, kNumBuckets, kInitialPoolSize);

    mMap = PL_NewHashTable(kInitialNumElements,
                           Hash,
                           Compare,
                           PL_CompareValues,
                           &gAllocOps,
                           &mPool);

    NS_ASSERTION(mMap != nsnull, "could not create hash table for resources");

#ifdef PR_LOGGING
    if (! gMapLog)
        gMapLog = PR_NewLogModule("nsElementMap");


    PR_LOG(gMapLog, PR_LOG_NOTICE,
           ("xulelemap(%p) created", this));
#endif
}

nsElementMap::~nsElementMap()
{
    MOZ_COUNT_DTOR(nsElementMap);

    if (mMap) {
        PL_HashTableEnumerateEntries(mMap, ReleaseContentList, this);
        PL_HashTableDestroy(mMap);
    }

    PR_LOG(gMapLog, PR_LOG_NOTICE,
           ("xulelemap(%p) destroyed", this));
}


PRIntn
nsElementMap::ReleaseContentList(PLHashEntry* aHashEntry, PRIntn aIndex, void* aClosure)
{
    nsElementMap* self = static_cast<nsElementMap*>(aClosure);

    PRUnichar* id =
        reinterpret_cast<PRUnichar*>(const_cast<void*>(aHashEntry->key));

    nsMemory::Free(id);
        
    ContentListItem* head =
        reinterpret_cast<ContentListItem*>(aHashEntry->value);

    while (head) {
        ContentListItem* doomed = head;
        head = head->mNext;
        ContentListItem::Destroy(self->mPool, doomed);
    }

    return HT_ENUMERATE_NEXT;
}


nsresult
nsElementMap::Add(const nsAString& aID, nsIContent* aContent)
{
    NS_PRECONDITION(mMap != nsnull, "not initialized");
    if (! mMap)
        return NS_ERROR_NOT_INITIALIZED;

    const nsPromiseFlatString& flatID = PromiseFlatString(aID);
    const PRUnichar *id = flatID.get();

    ContentListItem* head =
        static_cast<ContentListItem*>(PL_HashTableLookup(mMap, id));

    if (! head) {
        head = ContentListItem::Create(mPool, aContent);
        if (! head)
            return NS_ERROR_OUT_OF_MEMORY;

        PRUnichar* key = ToNewUnicode(aID);
        if (! key)
            return NS_ERROR_OUT_OF_MEMORY;

        PL_HashTableAdd(mMap, key, head);
    }
    else {
        while (1) {
            if (head->mContent.get() == aContent) {
                
                
                
                
                
                
                
#ifdef PR_LOGGING
                if (PR_LOG_TEST(gMapLog, PR_LOG_NOTICE)) {
                    const char *tagname;
                    aContent->Tag()->GetUTF8String(&tagname);

                    nsCAutoString aidC; 
                    aidC.AssignWithConversion(id, aID.Length());
                    PR_LOG(gMapLog, PR_LOG_NOTICE,
                           ("xulelemap(%p) dup    %s[%p] <-- %s\n",
                            this,
                            tagname,
                            aContent,
                            aidC.get()));
                }
#endif

                return NS_OK;
            }
            if (! head->mNext)
                break;

            head = head->mNext;
        }

        head->mNext = nsElementMap::ContentListItem::Create(mPool, aContent);
        if (! head->mNext)
            return NS_ERROR_OUT_OF_MEMORY;
    }

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gMapLog, PR_LOG_NOTICE)) {
        const char *tagname;
        aContent->Tag()->GetUTF8String(&tagname);

        nsCAutoString aidC; 
        aidC.AssignWithConversion(id, aID.Length());
        PR_LOG(gMapLog, PR_LOG_NOTICE,
               ("xulelemap(%p) add    %s[%p] <-- %s\n",
                this,
                tagname,
                aContent,
                aidC.get()));
    }
#endif

    return NS_OK;
}


nsresult
nsElementMap::Remove(const nsAString& aID, nsIContent* aContent)
{
    NS_PRECONDITION(mMap != nsnull, "not initialized");
    if (! mMap)
        return NS_ERROR_NOT_INITIALIZED;

    const nsPromiseFlatString& flatID = PromiseFlatString(aID);
    const PRUnichar *id = flatID.get();

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gMapLog, PR_LOG_NOTICE)) {
        const char *tagname;
        aContent->Tag()->GetUTF8String(&tagname);

        nsCAutoString aidC; 
        aidC.AssignWithConversion(id);
        PR_LOG(gMapLog, PR_LOG_NOTICE,
               ("xulelemap(%p) remove  %s[%p] <-- %s\n",
                this,
                tagname,
                aContent,
                aidC.get()));
    }
#endif

    PLHashEntry** hep = PL_HashTableRawLookup(mMap,
                                              Hash(id),
                                              id);

    
    
    
    NS_ASSERTION(hep != nsnull && *hep != nsnull, "attempt to remove an element that was never added");
    if (!hep || !*hep)
        return NS_OK;

    ContentListItem* head = reinterpret_cast<ContentListItem*>((*hep)->value);

    if (head->mContent.get() == aContent) {
        ContentListItem* next = head->mNext;
        if (next) {
            (*hep)->value = next;
        }
        else {
            
            PRUnichar* key = reinterpret_cast<PRUnichar*>(const_cast<void*>((*hep)->key));
            PL_HashTableRawRemove(mMap, hep, *hep);
            nsMemory::Free(key);
        }
        ContentListItem::Destroy(mPool, head);
    }
    else {
        ContentListItem* item = head->mNext;
        while (item) {
            if (item->mContent.get() == aContent) {
                head->mNext = item->mNext;
                ContentListItem::Destroy(mPool, item);
                break;
            }
            head = item;
            item = item->mNext;
        }
    }

    return NS_OK;
}



nsresult
nsElementMap::Find(const nsAString& aID, nsCOMArray<nsIContent>& aResults)
{
    NS_PRECONDITION(mMap != nsnull, "not initialized");
    if (! mMap)
        return NS_ERROR_NOT_INITIALIZED;

    aResults.Clear();
    ContentListItem* item =
        reinterpret_cast<ContentListItem*>(PL_HashTableLookup(mMap, (const PRUnichar *)PromiseFlatString(aID).get()));

    while (item) {
        aResults.AppendObject(item->mContent);
        item = item->mNext;
    }
    return NS_OK;
}


nsresult
nsElementMap::FindFirst(const nsAString& aID, nsIContent** aResult)
{
    NS_PRECONDITION(mMap != nsnull, "not initialized");
    if (! mMap)
        return NS_ERROR_NOT_INITIALIZED;

    ContentListItem* item =
        reinterpret_cast<ContentListItem*>(PL_HashTableLookup(mMap, (const PRUnichar *)PromiseFlatString(aID).get()));

    if (item) {
        *aResult = item->mContent;
        NS_ADDREF(*aResult);
    }
    else {
        *aResult = nsnull;
    }

    return NS_OK;
}

nsresult
nsElementMap::Enumerate(nsElementMapEnumerator aEnumerator, void* aClosure)
{
    EnumerateClosure closure = { this, aEnumerator, aClosure };
    PL_HashTableEnumerateEntries(mMap, EnumerateImpl, &closure);
    return NS_OK;
}


PRIntn
nsElementMap::EnumerateImpl(PLHashEntry* aHashEntry, PRIntn aIndex, void* aClosure)
{
    
    
    
    
    EnumerateClosure* closure = reinterpret_cast<EnumerateClosure*>(aClosure);

    const PRUnichar* id =
        reinterpret_cast<const PRUnichar*>(aHashEntry->key);

    
    ContentListItem** link = 
        reinterpret_cast<ContentListItem**>(&aHashEntry->value);

    ContentListItem* item = *link;

    
    while (item) {
        ContentListItem* current = item;
        item = item->mNext;
        PRIntn result = (*closure->mEnumerator)(id, current->mContent, closure->mClosure);

        if (result == HT_ENUMERATE_REMOVE) {
            
            *link = item;
            ContentListItem::Destroy(closure->mSelf->mPool, current);

            if ((! *link) && (link == reinterpret_cast<ContentListItem**>(&aHashEntry->value))) {
                
                
                PRUnichar* key = const_cast<PRUnichar*>(id);
                nsMemory::Free(key);
                return HT_ENUMERATE_REMOVE;
            }
        }
        else {
            link = &current->mNext;
        }
    }

    return HT_ENUMERATE_NEXT;
}


PLHashNumber
nsElementMap::Hash(const void* aKey)
{
    PLHashNumber result = 0;
    const PRUnichar* s = reinterpret_cast<const PRUnichar*>(aKey);
    while (*s != nsnull) {
        result = (result >> 28) ^ (result << 4) ^ *s;
        ++s;
    }
    return result;
}


PRIntn
nsElementMap::Compare(const void* aLeft, const void* aRight)
{
    return 0 == nsCRT::strcmp(reinterpret_cast<const PRUnichar*>(aLeft),
                              reinterpret_cast<const PRUnichar*>(aRight));
}
