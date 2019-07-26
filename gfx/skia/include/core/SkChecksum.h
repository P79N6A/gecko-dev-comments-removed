






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
    







    static uint32_t Compute(const uint32_t* data, size_t size) {
        SkASSERT(SkIsAlign4(size));

        





        uintptr_t result = 0;
        const uintptr_t* ptr = reinterpret_cast<const uintptr_t*>(data);

        




        size_t n4 = size / (sizeof(uintptr_t) << 2);
        for (size_t i = 0; i < n4; ++i) {
            result = Mash(result, *ptr++);
            result = Mash(result, *ptr++);
            result = Mash(result, *ptr++);
            result = Mash(result, *ptr++);
        }
        size &= ((sizeof(uintptr_t) << 2) - 1);

        data = reinterpret_cast<const uint32_t*>(ptr);
        const uint32_t* stop = data + (size >> 2);
        while (data < stop) {
            result = Mash(result, *data++);
        }

        







        if (8 == sizeof(result)) {
            result ^= result >> HALFBITS;
        }
        return static_cast<uint32_t>(result);
    }
};

#endif

