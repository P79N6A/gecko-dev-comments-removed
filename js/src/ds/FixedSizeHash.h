





#ifndef jsfixedsizehash_h_
#define jsfixedsizehash_h_

#include "ds/LifoAlloc.h"

namespace js {
























template <class T, class HashPolicy, size_t Capacity>
class FixedSizeHashSet
{
    T entries[Capacity];
    uint32_t lastOperations[Capacity];
    uint32_t numOperations;

    static const size_t NumHashes = HashPolicy::NumHashes;

    static_assert(Capacity > 0, "an empty fixed-size hash set is meaningless");

  public:
    typedef typename HashPolicy::Lookup Lookup;

    FixedSizeHashSet()
      : entries(), lastOperations(), numOperations(0)
    {
        MOZ_ASSERT(HashPolicy::isCleared(entries[0]));
    }

    bool lookup(const Lookup &lookup, T *pentry)
    {
        size_t bucket;
        if (lookupReference(lookup, &bucket)) {
            *pentry = entries[bucket];
            lastOperations[bucket] = numOperations++;
            return true;
        }
        return false;
    }

    void insert(const Lookup &lookup, const T &entry)
    {
        size_t buckets[NumHashes];
        getBuckets(lookup, buckets);

        size_t min = buckets[0];
        for (size_t i = 0; i < NumHashes; i++) {
            const T &entry = entries[buckets[i]];
            if (HashPolicy::isCleared(entry)) {
                entries[buckets[i]] = entry;
                lastOperations[buckets[i]] = numOperations++;
                return;
            }
            if (i && lastOperations[min] > lastOperations[buckets[i]])
                min = buckets[i];
        }

        entries[min] = entry;
        lastOperations[min] = numOperations++;
    }

    template <typename S>
    void remove(const S &s)
    {
        size_t bucket;
        if (lookupReference(s, &bucket))
            HashPolicy::clear(&entries[bucket]);
    }

  private:
    template <typename S>
    bool lookupReference(const S &s, size_t *pbucket)
    {
        size_t buckets[NumHashes];
        getBuckets(s, buckets);

        for (size_t i = 0; i < NumHashes; i++) {
            const T &entry = entries[buckets[i]];
            if (!HashPolicy::isCleared(entry) && HashPolicy::match(entry, s)) {
                *pbucket = buckets[i];
                return true;
            }
        }

        return false;
    }

    template <typename S>
    void getBuckets(const S &s, size_t buckets[NumHashes])
    {
        HashNumber hashes[NumHashes];
        HashPolicy::hash(s, hashes);

        for (size_t i = 0; i < NumHashes; i++)
            buckets[i] = hashes[i] % Capacity;
    }
};

}  

#endif 
