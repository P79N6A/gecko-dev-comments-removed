









































#include "pldhash.h"
#include "nsDebug.h"

class nsIPresShell;







class SpanningCellSorter {
public:
    SpanningCellSorter(nsIPresShell *aPresShell);
    ~SpanningCellSorter();

    struct Item {
        PRInt32 row, col;
        Item *next;
    };

    




    bool AddCell(PRInt32 aColSpan, PRInt32 aRow, PRInt32 aCol);

    




    Item* GetNext(PRInt32 *aColSpan);
private:
    nsIPresShell *mPresShell;

    enum State { ADDING, ENUMERATING_ARRAY, ENUMERATING_HASH, DONE };
    State mState;

    
    

    enum { ARRAY_BASE = 2 };
    enum { ARRAY_SIZE = 8 };
    Item *mArray[ARRAY_SIZE];
    PRInt32 SpanToIndex(PRInt32 aSpan) { return aSpan - ARRAY_BASE; }
    PRInt32 IndexToSpan(PRInt32 aIndex) { return aIndex + ARRAY_BASE; }
    bool UseArrayForSpan(PRInt32 aSpan) {
        NS_ASSERTION(SpanToIndex(aSpan) >= 0, "cell without colspan");
        return SpanToIndex(aSpan) < ARRAY_SIZE;
    }

    PLDHashTable mHashTable;
    struct HashTableEntry : public PLDHashEntryHdr {
        PRInt32 mColSpan;
        Item *mItems;
    };

    static PLDHashTableOps HashTableOps;

    static PLDHashNumber
        HashTableHashKey(PLDHashTable *table, const void *key);
    static bool
        HashTableMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                            const void *key);

    static PLDHashOperator
        FillSortedArray(PLDHashTable *table, PLDHashEntryHdr *hdr,
                        PRUint32 number, void *arg);

    static int SortArray(const void *a, const void *b, void *closure);

    
    PRUint32 mEnumerationIndex; 
    HashTableEntry **mSortedHashTable;

    




    void* operator new(size_t sz) CPP_THROW_NEW { return nsnull; }
};

