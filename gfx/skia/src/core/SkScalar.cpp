








#include "SkMath.h"
#include "SkScalar.h"

SkScalar SkScalarInterpFunc(SkScalar searchKey, const SkScalar keys[],
                            const SkScalar values[], int length) {
    SkASSERT(length > 0);
    SkASSERT(keys != NULL);
    SkASSERT(values != NULL);
#ifdef SK_DEBUG
    for (int i = 1; i < length; i++)
        SkASSERT(keys[i] >= keys[i-1]);
#endif
    int right = 0;
    while (right < length && searchKey > keys[right])
        right++;
    
    
    if (length == right)
        return values[length-1];
    if (0 == right)
        return values[0];
    
    SkScalar rightKey = keys[right];
    SkScalar leftKey = keys[right-1];
    SkScalar fract = SkScalarDiv(searchKey-leftKey,rightKey-leftKey);
    return SkScalarInterp(values[right-1], values[right], fract);
}
