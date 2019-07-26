






#ifndef BitArray_h__
#define BitArray_h__

#include "jstypes.h" 

#include "js/TemplateLib.h" 

namespace js {

template <size_t nbits>
class BitArray {
  private:
    static const size_t numSlots =
        nbits / JS_BITS_PER_WORD + (nbits % JS_BITS_PER_WORD == 0 ? 0 : 1);
    uintptr_t map[numSlots];

  public:
    void clear(bool value) {
        memset(map, value ? 0xFF : 0, sizeof(map));
    }

    inline bool get(size_t offset) const {
        uintptr_t index, mask;
        getMarkWordAndMask(offset, &index, &mask);
        return map[index] & mask;
    }

    inline void set(size_t offset) {
        uintptr_t index, mask;
        getMarkWordAndMask(offset, &index, &mask);
        map[index] |= mask;
    }

    inline void unset(size_t offset) {
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
        *indexp = offset >> tl::FloorLog2<JS_BITS_PER_WORD>::result;
        *maskp = uintptr_t(1) << (offset & (JS_BITS_PER_WORD - 1));
    }
};

} 

#endif
