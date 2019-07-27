






#ifndef SkSmallAllocator_DEFINED
#define SkSmallAllocator_DEFINED

#include "SkTDArray.h"
#include "SkTypes.h"



template<typename T> void destroyT(void* ptr) {
   static_cast<T*>(ptr)->~T();
}












template<uint32_t kMaxObjects, size_t kTotalBytes>
class SkSmallAllocator : SkNoncopyable {
public:
    SkSmallAllocator()
    : fStorageUsed(0)
    , fNumObjects(0)
    {}

    ~SkSmallAllocator() {
        
        
        while (fNumObjects > 0) {
            fNumObjects--;
            Rec* rec = &fRecs[fNumObjects];
            rec->fKillProc(rec->fObj);
            
            
            sk_free(rec->fHeapStorage);
        }
    }

    







    template<typename T>
    T* createT() {
        void* buf = this->reserveT<T>();
        if (NULL == buf) {
            return NULL;
        }
        SkNEW_PLACEMENT(buf, T);
        return static_cast<T*>(buf);
    }

    template<typename T, typename A1> T* createT(const A1& a1) {
        void* buf = this->reserveT<T>();
        if (NULL == buf) {
            return NULL;
        }
        SkNEW_PLACEMENT_ARGS(buf, T, (a1));
        return static_cast<T*>(buf);
    }

    template<typename T, typename A1, typename A2>
    T* createT(const A1& a1, const A2& a2) {
        void* buf = this->reserveT<T>();
        if (NULL == buf) {
            return NULL;
        }
        SkNEW_PLACEMENT_ARGS(buf, T, (a1, a2));
        return static_cast<T*>(buf);
    }

    template<typename T, typename A1, typename A2, typename A3>
    T* createT(const A1& a1, const A2& a2, const A3& a3) {
        void* buf = this->reserveT<T>();
        if (NULL == buf) {
            return NULL;
        }
        SkNEW_PLACEMENT_ARGS(buf, T, (a1, a2, a3));
        return static_cast<T*>(buf);
    }

    template<typename T, typename A1, typename A2, typename A3, typename A4>
    T* createT(const A1& a1, const A2& a2, const A3& a3, const A4& a4) {
        void* buf = this->reserveT<T>();
        if (NULL == buf) {
            return NULL;
        }
        SkNEW_PLACEMENT_ARGS(buf, T, (a1, a2, a3, a4));
        return static_cast<T*>(buf);
    }

    






    template<typename T> void* reserveT(size_t storageRequired = sizeof(T)) {
        SkASSERT(fNumObjects < kMaxObjects);
        SkASSERT(storageRequired >= sizeof(T));
        if (kMaxObjects == fNumObjects) {
            return NULL;
        }
        const size_t storageRemaining = SkAlign4(kTotalBytes) - fStorageUsed;
        storageRequired = SkAlign4(storageRequired);
        Rec* rec = &fRecs[fNumObjects];
        if (storageRequired > storageRemaining) {
            
            
            
            SkASSERT(false);
            rec->fStorageSize = 0;
            rec->fHeapStorage = sk_malloc_throw(storageRequired);
            rec->fObj = static_cast<void*>(rec->fHeapStorage);
        } else {
            
            rec->fStorageSize = storageRequired;
            rec->fHeapStorage = NULL;
            SkASSERT(SkIsAlign4(fStorageUsed));
            rec->fObj = static_cast<void*>(fStorage + (fStorageUsed / 4));
            fStorageUsed += storageRequired;
        }
        rec->fKillProc = destroyT<T>;
        fNumObjects++;
        return rec->fObj;
    }

    




    void freeLast() {
        SkASSERT(fNumObjects > 0);
        Rec* rec = &fRecs[fNumObjects - 1];
        sk_free(rec->fHeapStorage);
        fStorageUsed -= rec->fStorageSize;

        fNumObjects--;
    }

private:
    struct Rec {
        size_t fStorageSize;  
        void*  fObj;
        void*  fHeapStorage;
        void   (*fKillProc)(void*);
    };

    
    size_t              fStorageUsed;
    
    uint32_t            fStorage[SkAlign4(kTotalBytes) >> 2];
    uint32_t            fNumObjects;
    Rec                 fRecs[kMaxObjects];
};

#endif 
