









#ifndef GrTHashTable_DEFINED
#define GrTHashTable_DEFINED

#include "GrTypes.h"
#include "SkTDArray.h"












template <typename T, typename Key, size_t kHashBits> class GrTHashTable {
public:
    GrTHashTable() { this->clearHash(); }
    ~GrTHashTable() {}

    int count() const { return fSorted.count(); }

    struct Any {
        
        bool operator()(const T*) const { return true; }
    };

    T* find(const Key& key) const { return this->find(key, Any()); }
    template <typename Filter> T* find(const Key&, Filter filter) const;

    
    bool insert(const Key&, T*);
    void remove(const Key&, const T*);

    void deleteAll();

#ifdef SK_DEBUG
    void validate() const;
    bool contains(T*) const;
#endif

    
    const SkTDArray<T*>& getArray() const { return fSorted; }
    SkTDArray<T*>& getArray() { return fSorted; }
private:
    void clearHash() { sk_bzero(fHash, sizeof(fHash)); }

    enum {
        kHashCount = 1 << kHashBits,
        kHashMask  = kHashCount - 1
    };
    static unsigned hash2Index(intptr_t hash) {
#if 0
        if (sizeof(hash) == 8) {
            hash ^= hash >> 32;
        }
#endif
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
        if (Key::LessThan(*array[index], key)) {
            low = index + 1;
        } else {
            high = index;
        }
    }

    
    if (Key::Equals(*array[high], key)) {
        
        
        SkASSERT(0 == high || Key::LessThan(*array[high - 1], key));
        return high;
    }

    
    if (Key::LessThan(*array[high], key)) {
        high += 1;
    }
    return ~high;
}

template <typename T, typename Key, size_t kHashBits>
template <typename Filter>
T* GrTHashTable<T, Key, kHashBits>::find(const Key& key, Filter filter) const {

    int hashIndex = hash2Index(key.getHash());
    T* elem = fHash[hashIndex];

    if (NULL != elem && Key::Equals(*elem, key) && filter(elem)) {
        return elem;
    }

    
    int index = this->searchArray(key);
    if (index < 0) {
        return NULL;
    }

    const T* const* array = fSorted.begin();

    
    
    SkASSERT(0 == index || Key::LessThan(*array[index - 1], key));

    for ( ; index < count() && Key::Equals(*array[index], key); ++index) {
        if (filter(fSorted[index])) {
            
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
    SkASSERT(index >= 0);
    
    
    while (elem != fSorted[index]) {
        ++index;
        SkASSERT(index < fSorted.count());
    }
    SkASSERT(elem == fSorted[index]);
    fSorted.remove(index);
}

template <typename T, typename Key, size_t kHashBits>
void GrTHashTable<T, Key, kHashBits>::deleteAll() {
    fSorted.deleteAll();
    this->clearHash();
}

#ifdef SK_DEBUG
template <typename T, typename Key, size_t kHashBits>
void GrTHashTable<T, Key, kHashBits>::validate() const {
    int count = fSorted.count();
    for (int i = 1; i < count; i++) {
        SkASSERT(Key::LessThan(*fSorted[i - 1], *fSorted[i]) ||
                 Key::Equals(*fSorted[i - 1], *fSorted[i]));
    }
}

template <typename T, typename Key, size_t kHashBits>
bool GrTHashTable<T, Key, kHashBits>::contains(T* elem) const {
    int index = fSorted.find(elem);
    return index >= 0;
}

#endif

#endif
