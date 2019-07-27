




#ifndef nsContentSupportMap_h__
#define nsContentSupportMap_h__

#include "pldhash.h"
#include "nsTemplateMatch.h"











class nsContentSupportMap {
public:
    nsContentSupportMap() { Init(); }
    ~nsContentSupportMap() { Finish(); }

    nsresult Put(nsIContent* aElement, nsTemplateMatch* aMatch) {
        if (!mMap.IsInitialized())
            return NS_ERROR_NOT_INITIALIZED;

        PLDHashEntryHdr* hdr = PL_DHashTableAdd(&mMap, aElement);
        if (!hdr)
            return NS_ERROR_OUT_OF_MEMORY;

        Entry* entry = reinterpret_cast<Entry*>(hdr);
        NS_ASSERTION(entry->mMatch == nullptr, "over-writing entry");
        entry->mContent = aElement;
        entry->mMatch   = aMatch;
        return NS_OK; }

    bool Get(nsIContent* aElement, nsTemplateMatch** aMatch) {
        if (!mMap.IsInitialized())
            return false;

        PLDHashEntryHdr* hdr = PL_DHashTableLookup(&mMap, aElement);
        if (PL_DHASH_ENTRY_IS_FREE(hdr))
            return false;

        Entry* entry = reinterpret_cast<Entry*>(hdr);
        *aMatch = entry->mMatch;
        return true; }

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
};

#endif
