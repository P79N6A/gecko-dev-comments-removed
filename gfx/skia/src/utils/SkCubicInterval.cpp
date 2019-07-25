






#include "SkCubicInterval.h"

static SkScalar eval_cubic(SkScalar c1, SkScalar c2, SkScalar c3,
                           SkScalar t) {
    return SkScalarMul(SkScalarMul(SkScalarMul(c3, t) + c2, t) + c1, t);
}

static SkScalar find_cubic_t(SkScalar c1, SkScalar c2, SkScalar c3,
                             SkScalar targetX) {
    SkScalar minT = 0;
    SkScalar maxT = SK_Scalar1;
    SkScalar t;

    for (;;) {
        t = SkScalarAve(minT, maxT);
        SkScalar x = eval_cubic(c1, c2, c3, t);
        if (SkScalarNearlyZero(x - targetX)) {
            break;
        }
        
        if (x < targetX) {
            minT = t;
        } else {
            maxT = t;
        }
    }
    return t;
}













SkScalar SkEvalCubicInterval(SkScalar x1, SkScalar y1,
                             SkScalar x2, SkScalar y2,
                             SkScalar unitX) {
    x1 = SkScalarPin(x1, 0, SK_Scalar1);
    x2 = SkScalarPin(x2, 0, SK_Scalar1);
    unitX = SkScalarPin(unitX, 0, SK_Scalar1);

    
    x1 *= 3;
    x2 *= 3;

    
    SkScalar t = find_cubic_t(x1, x2 - 2*x1, x1 - x2 + SK_Scalar1, unitX);
    
    
    y1 *= 3;
    y2 *= 3;
    return eval_cubic(y1, y2 - 2*y1, y1 - y2 + SK_Scalar1, t);
}

