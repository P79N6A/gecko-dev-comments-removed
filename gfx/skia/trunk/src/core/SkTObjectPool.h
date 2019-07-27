






#ifndef SkFreeList_DEFINED
#define SkFreeList_DEFINED

#include "SkTInternalSList.h"












template<typename T, int numItemsPerBlock = 4096/sizeof(T)> class SkTObjectPool {
public:
    SkTObjectPool() {}
    ~SkTObjectPool() {
        while (!fBlocks.isEmpty()) {
            SkDELETE(fBlocks.pop());
        }
    }

    







    T* acquire() {
        if (fAvailable.isEmpty()) {
            grow();
        }
        return fAvailable.pop();
    }

    





    void release(T* entry) {
        fAvailable.push(entry);
    }

    



    void releaseAll(SkTInternalSList<T>* other) {
        fAvailable.pushAll(other);
    }

    



    int available() const { return fAvailable.getCount(); }

    


    int blocks() const { return fBlocks.getCount(); }

    


    int allocated() const { return fBlocks.getCount() * numItemsPerBlock; }

private:
    


    struct Block {
        T entries[numItemsPerBlock];
        SK_DECLARE_INTERNAL_SLIST_INTERFACE(Block);
    };
    SkTInternalSList<Block> fBlocks;
    SkTInternalSList<T> fAvailable;

    





    void grow() {
        Block* block = SkNEW(Block);
        fBlocks.push(block);
        for(int index = 0; index < numItemsPerBlock; ++index) {
            fAvailable.push(&block->entries[index]);
        }
    }

};

#endif
