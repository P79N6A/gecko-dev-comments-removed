









#ifndef GrTBSearch_DEFINED
#define GrTBSearch_DEFINED

template <typename ELEM, typename KEY>
int GrTBSearch(const ELEM array[], int count, KEY target) {
    GrAssert(count >= 0);
    if (0 == count) {
        
        return ~0;
    }
    
    int high = count - 1;
    int low = 0;
    while (high > low) {
        int index = (low + high) >> 1;
        if (LT(array[index], target)) {
            low = index + 1;
        } else {
            high = index;
        }
    }
    
    
    if (EQ(array[high], target)) {
        return high;
    }
    
    
    if (LT(array[high], target)) {
        high += 1;
    }
    return ~high;
}

#endif

