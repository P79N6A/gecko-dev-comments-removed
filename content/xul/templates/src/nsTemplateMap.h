





































#ifndef nsTemplateMap_h__
#define nsTemplateMap_h__

#include "pldhash.h"
#include "nsXULElement.h"

class nsTemplateMap {
protected:
    struct Entry {
        PLDHashEntryHdr mHdr;
        nsIContent*     mContent;
        nsIContent*     mTemplate;
    };

    PLDHashTable mTable;

    void
    Init() { PL_DHashTableInit(&mTable, PL_DHashGetStubOps(), nsnull, sizeof(Entry), PL_DHASH_MIN_SIZE); }

    void
    Finish() { PL_DHashTableFinish(&mTable); }

public:
    nsTemplateMap() { Init(); }

    ~nsTemplateMap() { Finish(); }

    void
    Put(nsIContent* aContent, nsIContent* aTemplate) {
        NS_ASSERTION(PL_DHASH_ENTRY_IS_FREE(PL_DHashTableOperate(&mTable, aContent, PL_DHASH_LOOKUP)),
                     "aContent already in map");

        Entry* entry =
            NS_REINTERPRET_CAST(Entry*, PL_DHashTableOperate(&mTable, aContent, PL_DHASH_ADD));

        if (entry) {
            entry->mContent = aContent;
            entry->mTemplate = aTemplate;
        }
    }

    void
    Remove(nsIContent* aContent) {
        NS_ASSERTION(PL_DHASH_ENTRY_IS_BUSY(PL_DHashTableOperate(&mTable, aContent, PL_DHASH_LOOKUP)),
                     "aContent not in map");

        PL_DHashTableOperate(&mTable, aContent, PL_DHASH_REMOVE);

        PRUint32 count;

        
        
        
        
        nsXULElement *xulcontent = nsXULElement::FromContent(aContent);
        if (xulcontent) {
            count = xulcontent->PeekChildCount();
        }
        else {
            count = aContent->GetChildCount();
        }

        for (PRUint32 i = 0; i < count; ++i) {
            Remove(aContent->GetChildAt(i));
        }
    }


    void
    GetTemplateFor(nsIContent* aContent, nsIContent** aResult) {
        Entry* entry =
            NS_REINTERPRET_CAST(Entry*, PL_DHashTableOperate(&mTable, aContent, PL_DHASH_LOOKUP));

        if (PL_DHASH_ENTRY_IS_BUSY(&entry->mHdr))
            NS_IF_ADDREF(*aResult = entry->mTemplate);
        else
            *aResult = nsnull;
    }

    void
    Clear() { Finish(); Init(); }
};

#endif 

