





#ifndef SpanningCellSorter_h
#define SpanningCellSorter_h





#include "pldhash.h"
#include "nsDebug.h"
#include "StackArena.h"







class MOZ_STACK_CLASS SpanningCellSorter {
public:
    SpanningCellSorter();
    ~SpanningCellSorter();

    struct Item {
        int32_t row, col;
        Item *next;
    };

    




    bool AddCell(int32_t aColSpan, int32_t aRow, int32_t aCol);

    




    Item* GetNext(int32_t *aColSpan);
private:

    enum State { ADDING, ENUMERATING_ARRAY, ENUMERATING_HASH, DONE };
    State mState;

    
    

    enum { ARRAY_BASE = 2 };
    enum { ARRAY_SIZE = 8 };
    Item *mArray[ARRAY_SIZE];
    int32_t SpanToIndex(int32_t aSpan) { return aSpan - ARRAY_BASE; }
    int32_t IndexToSpan(int32_t aIndex) { return aIndex + ARRAY_BASE; }
    bool UseArrayForSpan(int32_t aSpan) {
        NS_ASSERTION(SpanToIndex(aSpan) >= 0, "cell without colspan");
        return SpanToIndex(aSpan) < ARRAY_SIZE;
    }

    PLDHashTable mHashTable;
    struct HashTableEntry : public PLDHashEntryHdr {
        int32_t mColSpan;
        Item *mItems;
    };

    static const PLDHashTableOps HashTableOps;

    static PLDHashNumber
        HashTableHashKey(PLDHashTable *table, const void *key);
    static bool
        HashTableMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                            const void *key);

    static PLDHashOperator
        FillSortedArray(PLDHashTable *table, PLDHashEntryHdr *hdr,
                        uint32_t number, void *arg);

    static int SortArray(const void *a, const void *b, void *closure);

    
    uint32_t mEnumerationIndex; 
    HashTableEntry **mSortedHashTable;

    




    void* operator new(size_t sz) CPP_THROW_NEW { return nullptr; }
};

#endif
