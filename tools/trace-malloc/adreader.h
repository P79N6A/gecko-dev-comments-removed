



































#include <stdlib.h>

#define ADLOG_ENTRY_BLOCK_SIZE 4096

class ADLog {

public:

    
    
    typedef const char* Pointer;

    struct Entry {
        Pointer address;
        const char *type;

        const char *data;         
        size_t datasize;

        const char *allocation_stack;
    };

    ADLog();
    ~ADLog();

    


    bool Read(const char *aFilename);

private:
    
    struct EntryBlock;
    struct EntryBlockLink {
        EntryBlock *mPrev;
        EntryBlock *mNext;
    };

    struct EntryBlock : public EntryBlockLink {
        Entry entries[ADLOG_ENTRY_BLOCK_SIZE];
    };

    size_t mEntryCount;
    EntryBlockLink mEntries;

public:

    class const_iterator {
        private:
            
            friend class ADLog;
            const_iterator(EntryBlock *aBlock, size_t aOffset);

        public:
            const Entry* operator*() { return mCur; }
            const Entry* operator->() { return mCur; }

            const_iterator& operator++();
            const_iterator& operator--();

            bool operator==(const const_iterator& aOther) const {
                return mCur == aOther.mCur;
            }

            bool operator!=(const const_iterator& aOther) const {
                return mCur != aOther.mCur;
            }

        private:
            void SetBlock(EntryBlock *aBlock) {
                mBlock = aBlock;
                mBlockStart = aBlock->entries;
                mBlockEnd = aBlock->entries + ADLOG_ENTRY_BLOCK_SIZE;
            }

            EntryBlock *mBlock;
            Entry *mCur, *mBlockStart, *mBlockEnd;

            
            const_iterator operator++(int);
            const_iterator operator--(int);
    };

    const_iterator begin() {
        return const_iterator(mEntries.mNext, 0);
    }
    const_iterator end() {
        return const_iterator(mEntries.mPrev,
                              mEntryCount % ADLOG_ENTRY_BLOCK_SIZE);
    }

    size_t count() { return mEntryCount; }
};
