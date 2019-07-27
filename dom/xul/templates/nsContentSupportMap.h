




#ifndef nsContentSupportMap_h__
#define nsContentSupportMap_h__

#include "pldhash.h"
#include "nsTemplateMatch.h"











class nsContentSupportMap {
public:
    nsContentSupportMap() : mMap(PL_DHashGetStubOps(), sizeof(Entry)) { }
    ~nsContentSupportMap() { }

    nsresult Put(nsIContent* aElement, nsTemplateMatch* aMatch) {
        PLDHashEntryHdr* hdr =
            PL_DHashTableAdd(&mMap, aElement, mozilla::fallible);
        if (!hdr)
            return NS_ERROR_OUT_OF_MEMORY;

        Entry* entry = static_cast<Entry*>(hdr);
        NS_ASSERTION(entry->mMatch == nullptr, "over-writing entry");
        entry->mContent = aElement;
        entry->mMatch   = aMatch;
        return NS_OK;
    }

    bool Get(nsIContent* aElement, nsTemplateMatch** aMatch) {
        PLDHashEntryHdr* hdr = PL_DHashTableSearch(&mMap, aElement);
        if (!hdr)
            return false;

        Entry* entry = static_cast<Entry*>(hdr);
        *aMatch = entry->mMatch;
        return true;
    }

    nsresult Remove(nsIContent* aElement);

    void Clear() { mMap.Clear(); }

protected:
    PLDHashTable mMap;

    struct Entry : public PLDHashEntryHdr {
        nsIContent*      mContent;
        nsTemplateMatch* mMatch;
    };
};

#endif
