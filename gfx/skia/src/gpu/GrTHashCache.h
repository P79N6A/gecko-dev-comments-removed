









#ifndef GrTHashCache_DEFINED
#define GrTHashCache_DEFINED

#include "GrTypes.h"
#include "SkTDArray.h"




template <typename T> class GrTDefaultFindFunctor {
public:
    
    bool operator()(const T*) const { return true; }
};










template <typename T, typename Key, size_t kHashBits> class GrTHashTable {
public:
    GrTHashTable() { Gr_bzero(fHash, sizeof(fHash)); }
    ~GrTHashTable() {}

    int count() const { return fSorted.count(); }
    T*  find(const Key&) const;
    template <typename FindFuncType> T*  find(const Key&, const FindFuncType&) const;
    
    bool insert(const Key&, T*);
    void remove(const Key&, const T*);
    T* removeAt(int index, uint32_t hash);
    void removeAll();
    void deleteAll();
    void unrefAll();

    


    int slowFindIndex(T* elem) const { return fSorted.find(elem); }

#if GR_DEBUG
    void validate() const;
    bool contains(T*) const;
#endif

    
    const SkTDArray<T*>& getArray() const { return fSorted; }
    SkTDArray<T*>& getArray() { return fSorted; }
private:
    enum {
        kHashCount = 1 << kHashBits,
        kHashMask  = kHashCount - 1
    };
    static unsigned hash2Index(uint32_t hash) {
        hash ^= hash >> 16;
        if (kHashBits <= 8) {
            hash ^= hash >> 8;
        }
        return hash & kHashMask;
    }

    mutable T* fHash[kHashCount];
    SkTDArray<T*> fSorted;

    
    
    int searchArray(const Key&) const;
};



template <typename T, typename Key, size_t kHashBits>
int GrTHashTable<T, Key, kHashBits>::searchArray(const Key& key) const {
    int count = fSorted.count();
    if (0 == count) {
        
        return ~0;
    }

    const T* const* array = fSorted.begin();
    int high = count - 1;
    int low = 0;
    while (high > low) {
        int index = (low + high) >> 1;
        if (Key::LT(*array[index], key)) {
            low = index + 1;
        } else {
            high = index;
        }
    }

    
    if (Key::EQ(*array[high], key)) {
        
        
        GrAssert(0 == high || Key::LT(*array[high - 1], key));
        return high;
    }

    
    if (Key::LT(*array[high], key)) {
        high += 1;
    }
    return ~high;
}

template <typename T, typename Key, size_t kHashBits>
T* GrTHashTable<T, Key, kHashBits>::find(const Key& key) const {
    GrTDefaultFindFunctor<T> find;

    return this->find(key, find);
}

template <typename T, typename Key, size_t kHashBits>
template <typename FindFuncType>
T* GrTHashTable<T, Key, kHashBits>::find(const Key& key, const FindFuncType& findFunc) const {

    int hashIndex = hash2Index(key.getHash());
    T* elem = fHash[hashIndex];

    if (NULL != elem && Key::EQ(*elem, key) && findFunc(elem)) {
        return elem;
    }

    
    int index = this->searchArray(key);
    if (index < 0) {
        return NULL;
    }

    const T* const* array = fSorted.begin();

    
    
    GrAssert(0 == index || Key::LT(*array[index - 1], key));

    for ( ; index < count() && Key::EQ(*array[index], key); ++index) {
        if (findFunc(fSorted[index])) {
            
            fHash[hashIndex] = fSorted[index];
            return fSorted[index];
        }
    }

    return NULL;
}

template <typename T, typename Key, size_t kHashBits>
bool GrTHashTable<T, Key, kHashBits>::insert(const Key& key, T* elem) {
    int index = this->searchArray(key);
    bool first = index < 0;
    if (first) {
        
        index = ~index;
    }
    
    *fSorted.insert(index) = elem;
    
    fHash[hash2Index(key.getHash())] = elem;
    return first;
}

template <typename T, typename Key, size_t kHashBits>
void GrTHashTable<T, Key, kHashBits>::remove(const Key& key, const T* elem) {
    int index = hash2Index(key.getHash());
    if (fHash[index] == elem) {
        fHash[index] = NULL;
    }

    
    index = this->searchArray(key);
    GrAssert(index >= 0);
    
    
    while (elem != fSorted[index]) {
        ++index;
        GrAssert(index < fSorted.count());
    }
    GrAssert(elem == fSorted[index]);
    fSorted.remove(index);
}

template <typename T, typename Key, size_t kHashBits>
T* GrTHashTable<T, Key, kHashBits>::removeAt(int elemIndex, uint32_t hash) {
    int hashIndex = hash2Index(hash);
    if (fHash[hashIndex] == fSorted[elemIndex]) {
        fHash[hashIndex] = NULL;
    }
    
    T* elem = fSorted[elemIndex];
    fSorted.remove(elemIndex);
    return elem;
}

template <typename T, typename Key, size_t kHashBits>
void GrTHashTable<T, Key, kHashBits>::removeAll() {
    fSorted.reset();
    Gr_bzero(fHash, sizeof(fHash));
}

template <typename T, typename Key, size_t kHashBits>
void GrTHashTable<T, Key, kHashBits>::deleteAll() {
    fSorted.deleteAll();
    Gr_bzero(fHash, sizeof(fHash));
}

template <typename T, typename Key, size_t kHashBits>
void GrTHashTable<T, Key, kHashBits>::unrefAll() {
    fSorted.unrefAll();
    Gr_bzero(fHash, sizeof(fHash));
}

#if GR_DEBUG
template <typename T, typename Key, size_t kHashBits>
void GrTHashTable<T, Key, kHashBits>::validate() const {
    int count = fSorted.count();
    for (int i = 1; i < count; i++) {
        GrAssert(Key::LT(*fSorted[i - 1], *fSorted[i]) ||
                 Key::EQ(*fSorted[i - 1], *fSorted[i]));
    }
}

template <typename T, typename Key, size_t kHashBits>
bool GrTHashTable<T, Key, kHashBits>::contains(T* elem) const {
    int index = fSorted.find(elem);
    return index >= 0;
}

#endif

#endif
