






#ifndef SkChecksum_DEFINED
#define SkChecksum_DEFINED

#include "SkTypes.h"










class SkChecksum : SkNoncopyable {
private:
    



    enum {
        ROTR = 17,
        ROTL = sizeof(uintptr_t) * 8 - ROTR,
        HALFBITS = sizeof(uintptr_t) * 4
    };

    static inline uintptr_t Mash(uintptr_t total, uintptr_t value) {
        return ((total >> ROTR) | (total << ROTL)) ^ value;
    }

public:
    





    static uint32_t Mix(uint32_t hash) {
        hash ^= hash >> 16;
        hash *= 0x85ebca6b;
        hash ^= hash >> 13;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 16;
        return hash;
    }

    









    static uint32_t Murmur3(const uint32_t* data, size_t bytes, uint32_t seed=0) {
        
        
        typedef uint32_t SK_ATTRIBUTE(may_alias) aliased_uint32_t;
        const aliased_uint32_t* safe_data = (const aliased_uint32_t*)data;

        SkASSERTF(SkIsAlign4(bytes), "Expected 4-byte multiple, got %zu", bytes);
        const size_t words = bytes/4;


        uint32_t hash = seed;
        for (size_t i = 0; i < words; i++) {
            uint32_t k = safe_data[i];
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;

            hash ^= k;
            hash = (hash << 13) | (hash >> 19);
            hash *= 5;
            hash += 0xe6546b64;
        }
        hash ^= bytes;
        return Mix(hash);
    }

    












    static uint32_t Compute(const uint32_t* data, size_t size) {
        
        
        typedef uint32_t SK_ATTRIBUTE(may_alias) aliased_uint32_t;
        const aliased_uint32_t* safe_data = (const aliased_uint32_t*)data;

        SkASSERT(SkIsAlign4(size));

        





        uintptr_t result = 0;
        const uintptr_t* ptr = reinterpret_cast<const uintptr_t*>(safe_data);

        




        size_t n4 = size / (sizeof(uintptr_t) << 2);
        for (size_t i = 0; i < n4; ++i) {
            result = Mash(result, *ptr++);
            result = Mash(result, *ptr++);
            result = Mash(result, *ptr++);
            result = Mash(result, *ptr++);
        }
        size &= ((sizeof(uintptr_t) << 2) - 1);

        safe_data = reinterpret_cast<const aliased_uint32_t*>(ptr);
        const aliased_uint32_t* stop = safe_data + (size >> 2);
        while (safe_data < stop) {
            result = Mash(result, *safe_data++);
        }

        







        if (8 == sizeof(result)) {
            result ^= result >> HALFBITS;
        }
        return static_cast<uint32_t>(result);
    }
};

#endif
