





#ifndef ds_BitArray_h
#define ds_BitArray_h

#include "mozilla/TemplateLib.h"

#include <limits.h>

#include "jstypes.h"

namespace js {

template <size_t nbits>
class BitArray
{
  private:
    static const size_t bitsPerElement = sizeof(uintptr_t) * CHAR_BIT;
    static const size_t numSlots = nbits / bitsPerElement + (nbits % bitsPerElement == 0 ? 0 : 1);
    static const size_t paddingBits = (numSlots * bitsPerElement) - nbits;
    static_assert(paddingBits < bitsPerElement, "More padding bits than expected.");
    static const uintptr_t paddingMask = uintptr_t(-1) >> paddingBits;

    uintptr_t map[numSlots];

  public:
    void clear(bool value) {
        memset(map, value ? 0xFF : 0, sizeof(map));
        if (value)
            map[numSlots - 1] &= paddingMask;
    }

    inline bool get(size_t offset) const {
        uintptr_t index, mask;
        getMarkWordAndMask(offset, &index, &mask);
        return map[index] & mask;
    }

    void set(size_t offset) {
        uintptr_t index, mask;
        getMarkWordAndMask(offset, &index, &mask);
        map[index] |= mask;
    }

    void unset(size_t offset) {
        uintptr_t index, mask;
        getMarkWordAndMask(offset, &index, &mask);
        map[index] &= ~mask;
    }

    bool isAllClear() const {
        for (size_t i = 0; i < numSlots; i++) {
            if (map[i])
                return false;
        }
        return true;
    }

  private:
    inline void getMarkWordAndMask(size_t offset,
                                   uintptr_t *indexp, uintptr_t *maskp) const {
        static_assert(bitsPerElement == 32 || bitsPerElement == 64,
                      "unexpected bitsPerElement value");
        *indexp = offset / bitsPerElement;
        *maskp = uintptr_t(1) << (offset % bitsPerElement);
    }
};

} 

#endif 
