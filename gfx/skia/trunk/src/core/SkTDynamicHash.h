






#ifndef SkTDynamicHash_DEFINED
#define SkTDynamicHash_DEFINED

#include "SkMath.h"
#include "SkTemplates.h"
#include "SkTypes.h"





template <typename T,
          typename Key,
          typename Traits = T,
          int kGrowPercent = 75>  
class SkTDynamicHash {
public:
    SkTDynamicHash() : fCount(0), fDeleted(0), fCapacity(0), fArray(NULL) {
        SkASSERT(this->validate());
    }

    ~SkTDynamicHash() {
        sk_free(fArray);
    }

    class Iter {
    public:
        explicit Iter(SkTDynamicHash* hash) : fHash(hash), fCurrentIndex(-1) {
            SkASSERT(hash);
            ++(*this);
        }
        bool done() const {
            SkASSERT(fCurrentIndex <= fHash->fCapacity);
            return fCurrentIndex == fHash->fCapacity;
        }
        T& operator*() const {
            SkASSERT(!this->done());
            return *this->current();
        }
        void operator++() {
            do {
                fCurrentIndex++;
            } while (!this->done() && (this->current() == Empty() || this->current() == Deleted()));
        }

    private:
        T* current() const { return fHash->fArray[fCurrentIndex]; }

        SkTDynamicHash* fHash;
        int fCurrentIndex;
    };

    class ConstIter {
    public:
        explicit ConstIter(const SkTDynamicHash* hash) : fHash(hash), fCurrentIndex(-1) {
            SkASSERT(hash);
            ++(*this);
        }
        bool done() const {
            SkASSERT(fCurrentIndex <= fHash->fCapacity);
            return fCurrentIndex == fHash->fCapacity;
        }
        const T& operator*() const {
            SkASSERT(!this->done());
            return *this->current();
        }
        void operator++() {
            do {
                fCurrentIndex++;
            } while (!this->done() && (this->current() == Empty() || this->current() == Deleted()));
        }

    private:
        const T* current() const { return fHash->fArray[fCurrentIndex]; }

        const SkTDynamicHash* fHash;
        int fCurrentIndex;
    };

    int count() const { return fCount; }

    
    T* find(const Key& key) const {
        int index = this->firstIndex(key);
        for (int round = 0; round < fCapacity; round++) {
            SkASSERT(index >= 0 && index < fCapacity);
            T* candidate = fArray[index];
            if (Empty() == candidate) {
                return NULL;
            }
            if (Deleted() != candidate && GetKey(*candidate) == key) {
                return candidate;
            }
            index = this->nextIndex(index, round);
        }
        SkASSERT(fCapacity == 0);
        return NULL;
    }

    
    void add(T* newEntry) {
        SkASSERT(NULL == this->find(GetKey(*newEntry)));
        this->maybeGrow();
        this->innerAdd(newEntry);
        SkASSERT(this->validate());
    }

    
    void remove(const Key& key) {
        SkASSERT(NULL != this->find(key));
        this->innerRemove(key);
        SkASSERT(this->validate());
    }

    void rewind() {
        if (NULL != fArray) {
            sk_bzero(fArray, sizeof(T*)* fCapacity);
        }
        fCount = 0;
        fDeleted = 0;
    }

    void reset() { 
        fCount = 0; 
        fDeleted = 0; 
        fCapacity = 0; 
        sk_free(fArray); 
        fArray = NULL; 
    }

protected:
    

    int capacity() const { return fCapacity; }

    
    int countCollisions(const Key& key) const {
        int index = this->firstIndex(key);
        for (int round = 0; round < fCapacity; round++) {
            SkASSERT(index >= 0 && index < fCapacity);
            const T* candidate = fArray[index];
            if (Empty() == candidate || Deleted() == candidate || GetKey(*candidate) == key) {
                return round;
            }
            index = this->nextIndex(index, round);
        }
        SkASSERT(fCapacity == 0);
        return 0;
    }

private:
    
    static T* Empty()   { return reinterpret_cast<T*>(0); }  
    static T* Deleted() { return reinterpret_cast<T*>(1); }  

    bool validate() const {
        #define SKTDYNAMICHASH_CHECK(x) SkASSERT((x)); if (!(x)) return false
        static const int kLarge = 50;  

        
        
        SKTDYNAMICHASH_CHECK(SkIsPow2(fCapacity));

        
        if (fCount < kLarge * kLarge) {
            
            int count = 0, deleted = 0;
            for (int i = 0; i < fCapacity; i++) {
                if (Deleted() == fArray[i]) {
                    deleted++;
                } else if (Empty() != fArray[i]) {
                    count++;
                    SKTDYNAMICHASH_CHECK(NULL != this->find(GetKey(*fArray[i])));
                }
            }
            SKTDYNAMICHASH_CHECK(count == fCount);
            SKTDYNAMICHASH_CHECK(deleted == fDeleted);
        }

        
        if (fCount < kLarge) {
            
            for (int i = 0; i < fCapacity; i++) {
                if (Empty() == fArray[i] || Deleted() == fArray[i]) {
                    continue;
                }
                for (int j = i+1; j < fCapacity; j++) {
                    if (Empty() == fArray[j] || Deleted() == fArray[j]) {
                        continue;
                    }
                    SKTDYNAMICHASH_CHECK(fArray[i] != fArray[j]);
                    SKTDYNAMICHASH_CHECK(!(GetKey(*fArray[i]) == GetKey(*fArray[j])));
                }
            }
        }
        #undef SKTDYNAMICHASH_CHECK
        return true;
    }

    void innerAdd(T* newEntry) {
        const Key& key = GetKey(*newEntry);
        int index = this->firstIndex(key);
        for (int round = 0; round < fCapacity; round++) {
            SkASSERT(index >= 0 && index < fCapacity);
            const T* candidate = fArray[index];
            if (Empty() == candidate || Deleted() == candidate) {
                if (Deleted() == candidate) {
                    fDeleted--;
                }
                fCount++;
                fArray[index] = newEntry;
                return;
            }
            index = this->nextIndex(index, round);
        }
        SkASSERT(fCapacity == 0);
    }

    void innerRemove(const Key& key) {
        const int firstIndex = this->firstIndex(key);
        int index = firstIndex;
        for (int round = 0; round < fCapacity; round++) {
            SkASSERT(index >= 0 && index < fCapacity);
            const T* candidate = fArray[index];
            if (Deleted() != candidate && GetKey(*candidate) == key) {
                fDeleted++;
                fCount--;
                fArray[index] = Deleted();
                return;
            }
            index = this->nextIndex(index, round);
        }
        SkASSERT(fCapacity == 0);
    }

    void maybeGrow() {
        if (100 * (fCount + fDeleted + 1) > fCapacity * kGrowPercent) {
            this->resize(fCapacity > 0 ? fCapacity * 2 : 4);
        }
    }

    void resize(int newCapacity) {
        SkDEBUGCODE(int oldCount = fCount;)
        int oldCapacity = fCapacity;
        SkAutoTMalloc<T*> oldArray(fArray);

        fCount = fDeleted = 0;
        fCapacity = newCapacity;
        fArray = (T**)sk_calloc_throw(sizeof(T*) * fCapacity);

        for (int i = 0; i < oldCapacity; i++) {
            T* entry = oldArray[i];
            if (Empty() != entry && Deleted() != entry) {
                this->innerAdd(entry);
            }
        }
        SkASSERT(oldCount == fCount);
    }

    
    uint32_t hashMask() const { return fCapacity - 1; }

    int firstIndex(const Key& key) const {
        return Hash(key) & this->hashMask();
    }

    
    int nextIndex(int index, int round) const {
        
        return (index + round + 1) & this->hashMask();
    }

    static const Key& GetKey(const T& t) { return Traits::GetKey(t); }
    static uint32_t Hash(const Key& key) { return Traits::Hash(key); }

    int fCount;     
    int fDeleted;   
    int fCapacity;  
    T** fArray;
};

#endif
