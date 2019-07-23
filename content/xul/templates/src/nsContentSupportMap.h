





































#ifndef nsContentSupportMap_h__
#define nsContentSupportMap_h__

#include "pldhash.h"
#include "nsFixedSizeAllocator.h"
#include "nsTemplateMatch.h"











class nsContentSupportMap {
public:
    nsContentSupportMap() { Init(); }
    ~nsContentSupportMap() { Finish(); }

    nsresult Put(nsIContent* aElement, nsTemplateMatch* aMatch) {
        if (!mMap.ops)
            return NS_ERROR_NOT_INITIALIZED;

        PLDHashEntryHdr* hdr = PL_DHashTableOperate(&mMap, aElement, PL_DHASH_ADD);
        if (!hdr)
            return NS_ERROR_OUT_OF_MEMORY;

        Entry* entry = NS_REINTERPRET_CAST(Entry*, hdr);
        NS_ASSERTION(entry->mMatch == nsnull, "over-writing entry");
        entry->mContent = aElement;
        entry->mMatch   = aMatch;
        return NS_OK; }

    PRBool Get(nsIContent* aElement, nsTemplateMatch** aMatch) {
        if (!mMap.ops)
            return PR_FALSE;

        PLDHashEntryHdr* hdr = PL_DHashTableOperate(&mMap, aElement, PL_DHASH_LOOKUP);
        if (PL_DHASH_ENTRY_IS_FREE(hdr))
            return PR_FALSE;

        Entry* entry = NS_REINTERPRET_CAST(Entry*, hdr);
        *aMatch = entry->mMatch;
        return PR_TRUE; }

    nsresult Remove(nsIContent* aElement);

    void Clear() { Finish(); Init(); }

protected:
    PLDHashTable mMap;

    void Init();
    void Finish();

    struct Entry {
        PLDHashEntryHdr  mHdr;
        nsIContent*      mContent;
        nsTemplateMatch* mMatch;
    };

    static PLDHashTableOps gOps;

    static void PR_CALLBACK
    ClearEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr);
};

#endif
