#ifndef SkBitmapProcState_utils_DEFINED
#define SkBitmapProcState_utils_DEFINED




static unsigned SK_USHIFT16(unsigned x) {
    return x >> 16;
}











static inline bool can_truncate_to_fixed_for_decal(SkFractionalInt frX,
                                                   SkFractionalInt frDx,
                                                   int count, unsigned max) {
    SkFixed dx = SkFractionalIntToFixed(frDx);

    
    
    
    if (dx <= SK_Fixed1 / 256) {
        return false;
    }

    
    
    SkFixed fx = SkFractionalIntToFixed(frX);
    return (unsigned)SkFixedFloorToInt(fx) <= max &&
           (unsigned)SkFixedFloorToInt(fx + dx * (count - 1)) < max;
}

#endif 
