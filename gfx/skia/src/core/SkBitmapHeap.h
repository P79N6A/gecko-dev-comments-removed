






#ifndef SkBitmapHeap_DEFINED
#define SkBitmapHeap_DEFINED

#include "SkBitmap.h"
#include "SkFlattenable.h"
#include "SkRefCnt.h"
#include "SkTDArray.h"
#include "SkThread.h"
#include "SkTRefArray.h"






class SkBitmapHeapEntry : SkNoncopyable {
public:
    ~SkBitmapHeapEntry();

    int32_t getSlot() { return fSlot; }

    SkBitmap* getBitmap() { return &fBitmap; }

    void releaseRef() {
        sk_atomic_dec(&fRefCount);
    }

private:
    SkBitmapHeapEntry();

    void addReferences(int count);

    int32_t fSlot;
    int32_t fRefCount;

    SkBitmap fBitmap;
    
    
    
    size_t fBytesAllocated;

    friend class SkBitmapHeap;
};


class SkBitmapHeapReader : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkBitmapHeapReader)

    SkBitmapHeapReader() : INHERITED() {}
    virtual SkBitmap* getBitmap(int32_t slot) const = 0;
    virtual void releaseRef(int32_t slot) = 0;
private:
    typedef SkRefCnt INHERITED;
};





class SkBitmapHeap : public SkBitmapHeapReader {
public:
    class ExternalStorage : public SkRefCnt {
     public:
        SK_DECLARE_INST_COUNT(ExternalStorage)

        virtual bool insert(const SkBitmap& bitmap, int32_t slot) = 0;

     private:
        typedef SkRefCnt INHERITED;
    };

    static const int32_t UNLIMITED_SIZE = -1;
    static const int32_t IGNORE_OWNERS  = -1;
    static const int32_t INVALID_SLOT   = -1;

    











    SkBitmapHeap(int32_t preferredSize = UNLIMITED_SIZE, int32_t ownerCount = IGNORE_OWNERS);

    












    SkBitmapHeap(ExternalStorage* externalStorage, int32_t heapSize = UNLIMITED_SIZE);

    ~SkBitmapHeap();

    





    SkTRefArray<SkBitmap>* extractBitmaps() const;

    




    virtual SkBitmap* getBitmap(int32_t slot) const SK_OVERRIDE {
        SkASSERT(fExternalStorage == NULL);
        SkBitmapHeapEntry* entry = getEntry(slot);
        if (entry) {
            return &entry->fBitmap;
        }
        return NULL;
    }

    




    virtual void releaseRef(int32_t slot) SK_OVERRIDE {
        SkASSERT(fExternalStorage == NULL);
        if (fOwnerCount != IGNORE_OWNERS) {
            SkBitmapHeapEntry* entry = getEntry(slot);
            if (entry) {
                entry->releaseRef();
            }
        }
    }

    









    int32_t insert(const SkBitmap& bitmap);

    





    SkBitmapHeapEntry* getEntry(int32_t slot) const {
        SkASSERT(slot <= fStorage.count());
        if (fExternalStorage != NULL) {
            return NULL;
        }
        return fStorage[slot];
    }

    


    int count() const {
        SkASSERT(fExternalStorage != NULL ||
                 fStorage.count() - fUnusedSlots.count() == fLookupTable.count());
        return fLookupTable.count();
    }

    


    size_t bytesAllocated() const {
        return fBytesAllocated;
    }

    





    size_t freeMemoryIfPossible(size_t bytesToFree);

    






    void deferAddingOwners();

    




    void endAddingOwnersDeferral(bool add);

private:
    struct LookupEntry {
        LookupEntry(const SkBitmap& bm)
        : fGenerationId(bm.getGenerationID())
        , fPixelOffset(bm.pixelRefOffset())
        , fWidth(bm.width())
        , fHeight(bm.height())
        , fMoreRecentlyUsed(NULL)
        , fLessRecentlyUsed(NULL){}

        const uint32_t fGenerationId; 
        const size_t   fPixelOffset;
        const uint32_t fWidth;
        const uint32_t fHeight;

        
        LookupEntry* fMoreRecentlyUsed;
        LookupEntry* fLessRecentlyUsed;

        uint32_t fStorageSlot; 

        


        static int Compare(const LookupEntry* a, const LookupEntry* b);
    };

    






    int removeEntryFromLookupTable(LookupEntry*);

    







    int findInLookupTable(const LookupEntry& key, SkBitmapHeapEntry** entry);

    LookupEntry* findEntryToReplace(const SkBitmap& replacement);
    bool copyBitmap(const SkBitmap& originalBitmap, SkBitmap& copiedBitmap);

    





    void removeFromLRU(LookupEntry* entry);

    





    void appendToLRU(LookupEntry*);

    
    SkTDArray<LookupEntry*> fLookupTable;

    
    SkTDArray<SkBitmapHeapEntry*> fStorage;
    
    
    SkTDArray<int> fUnusedSlots;
    ExternalStorage* fExternalStorage;

    LookupEntry* fMostRecentlyUsed;
    LookupEntry* fLeastRecentlyUsed;

    const int32_t fPreferredCount;
    const int32_t fOwnerCount;
    size_t fBytesAllocated;

    bool fDeferAddingOwners;
    SkTDArray<int> fDeferredEntries;

    typedef SkBitmapHeapReader INHERITED;
};

#endif 
