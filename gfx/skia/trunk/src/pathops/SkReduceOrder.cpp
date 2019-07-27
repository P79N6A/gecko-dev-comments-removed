





#include "SkReduceOrder.h"

int SkReduceOrder::reduce(const SkDLine& line) {
    fLine[0] = line[0];
    int different = line[0] != line[1];
    fLine[1] = line[different];
    return 1 + different;
}

static int coincident_line(const SkDQuad& quad, SkDQuad& reduction) {
    reduction[0] = reduction[1] = quad[0];
    return 1;
}

static int reductionLineCount(const SkDQuad& reduction) {
    return 1 + !reduction[0].approximatelyEqual(reduction[1]);
}

static int vertical_line(const SkDQuad& quad, SkDQuad& reduction) {
    reduction[0] = quad[0];
    reduction[1] = quad[2];
    return reductionLineCount(reduction);
}

static int horizontal_line(const SkDQuad& quad, SkDQuad& reduction) {
    reduction[0] = quad[0];
    reduction[1] = quad[2];
    return reductionLineCount(reduction);
}

static int check_linear(const SkDQuad& quad,
        int minX, int maxX, int minY, int maxY, SkDQuad& reduction) {
    int startIndex = 0;
    int endIndex = 2;
    while (quad[startIndex].approximatelyEqual(quad[endIndex])) {
        --endIndex;
        if (endIndex == 0) {
            SkDebugf("%s shouldn't get here if all four points are about equal", __FUNCTION__);
            SkASSERT(0);
        }
    }
    if (!quad.isLinear(startIndex, endIndex)) {
        return 0;
    }
    
    reduction[0] = quad[0];
    reduction[1] = quad[2];
    return reductionLineCount(reduction);
}




    

    
int SkReduceOrder::reduce(const SkDQuad& quad) {
    int index, minX, maxX, minY, maxY;
    int minXSet, minYSet;
    minX = maxX = minY = maxY = 0;
    minXSet = minYSet = 0;
    for (index = 1; index < 3; ++index) {
        if (quad[minX].fX > quad[index].fX) {
            minX = index;
        }
        if (quad[minY].fY > quad[index].fY) {
            minY = index;
        }
        if (quad[maxX].fX < quad[index].fX) {
            maxX = index;
        }
        if (quad[maxY].fY < quad[index].fY) {
            maxY = index;
        }
    }
    for (index = 0; index < 3; ++index) {
        if (AlmostEqualUlps(quad[index].fX, quad[minX].fX)) {
            minXSet |= 1 << index;
        }
        if (AlmostEqualUlps(quad[index].fY, quad[minY].fY)) {
            minYSet |= 1 << index;
        }
    }
    if (minXSet == 0x7) {  
        if (minYSet == 0x7) {  
            return coincident_line(quad, fQuad);
        }
        return vertical_line(quad, fQuad);
    }
    if (minYSet == 0xF) {  
        return horizontal_line(quad, fQuad);
    }
    int result = check_linear(quad, minX, maxX, minY, maxY, fQuad);
    if (result) {
        return result;
    }
    fQuad = quad;
    return 3;
}



static int coincident_line(const SkDCubic& cubic, SkDCubic& reduction) {
    reduction[0] = reduction[1] = cubic[0];
    return 1;
}

static int reductionLineCount(const SkDCubic& reduction) {
    return 1 + !reduction[0].approximatelyEqual(reduction[1]);
}

static int vertical_line(const SkDCubic& cubic, SkDCubic& reduction) {
    reduction[0] = cubic[0];
    reduction[1] = cubic[3];
    return reductionLineCount(reduction);
}

static int horizontal_line(const SkDCubic& cubic, SkDCubic& reduction) {
    reduction[0] = cubic[0];
    reduction[1] = cubic[3];
    return reductionLineCount(reduction);
}


static int check_quadratic(const SkDCubic& cubic, SkDCubic& reduction) {
    double dx10 = cubic[1].fX - cubic[0].fX;
    double dx23 = cubic[2].fX - cubic[3].fX;
    double midX = cubic[0].fX + dx10 * 3 / 2;
    double sideAx = midX - cubic[3].fX;
    double sideBx = dx23 * 3 / 2;
    if (approximately_zero(sideAx) ? !approximately_equal(sideAx, sideBx)
            : !AlmostEqualUlps(sideAx, sideBx)) {
        return 0;
    }
    double dy10 = cubic[1].fY - cubic[0].fY;
    double dy23 = cubic[2].fY - cubic[3].fY;
    double midY = cubic[0].fY + dy10 * 3 / 2;
    double sideAy = midY - cubic[3].fY;
    double sideBy = dy23 * 3 / 2;
    if (approximately_zero(sideAy) ? !approximately_equal(sideAy, sideBy)
            : !AlmostEqualUlps(sideAy, sideBy)) {
        return 0;
    }
    reduction[0] = cubic[0];
    reduction[1].fX = midX;
    reduction[1].fY = midY;
    reduction[2] = cubic[3];
    return 3;
}

static int check_linear(const SkDCubic& cubic,
        int minX, int maxX, int minY, int maxY, SkDCubic& reduction) {
    int startIndex = 0;
    int endIndex = 3;
    while (cubic[startIndex].approximatelyEqual(cubic[endIndex])) {
        --endIndex;
        if (endIndex == 0) {
            endIndex = 3;
            break;
        }
    }
    if (!cubic.isLinear(startIndex, endIndex)) {
        return 0;
    }
    
    reduction[0] = cubic[0];
    reduction[1] = cubic[3];
    return reductionLineCount(reduction);
}
























    

    
int SkReduceOrder::reduce(const SkDCubic& cubic, Quadratics allowQuadratics) {
    int index, minX, maxX, minY, maxY;
    int minXSet, minYSet;
    minX = maxX = minY = maxY = 0;
    minXSet = minYSet = 0;
    for (index = 1; index < 4; ++index) {
        if (cubic[minX].fX > cubic[index].fX) {
            minX = index;
        }
        if (cubic[minY].fY > cubic[index].fY) {
            minY = index;
        }
        if (cubic[maxX].fX < cubic[index].fX) {
            maxX = index;
        }
        if (cubic[maxY].fY < cubic[index].fY) {
            maxY = index;
        }
    }
    for (index = 0; index < 4; ++index) {
        double cx = cubic[index].fX;
        double cy = cubic[index].fY;
        double denom = SkTMax(fabs(cx), SkTMax(fabs(cy),
                SkTMax(fabs(cubic[minX].fX), fabs(cubic[minY].fY))));
        if (denom == 0) {
            minXSet |= 1 << index;
            minYSet |= 1 << index;
            continue;
        }
        double inv = 1 / denom;
        if (approximately_equal_half(cx * inv, cubic[minX].fX * inv)) {
            minXSet |= 1 << index;
        }
        if (approximately_equal_half(cy * inv, cubic[minY].fY * inv)) {
            minYSet |= 1 << index;
        }
    }
    if (minXSet == 0xF) {  
        if (minYSet == 0xF) {  
            return coincident_line(cubic, fCubic);
        }
        return vertical_line(cubic, fCubic);
    }
    if (minYSet == 0xF) {  
        return horizontal_line(cubic, fCubic);
    }
    int result = check_linear(cubic, minX, maxX, minY, maxY, fCubic);
    if (result) {
        return result;
    }
    if (allowQuadratics == SkReduceOrder::kAllow_Quadratics
            && (result = check_quadratic(cubic, fCubic))) {
        return result;
    }
    fCubic = cubic;
    return 4;
}

SkPath::Verb SkReduceOrder::Quad(const SkPoint a[3], SkPoint* reducePts) {
    SkDQuad quad;
    quad.set(a);
    SkReduceOrder reducer;
    int order = reducer.reduce(quad);
    if (order == 2) {  
        for (int index = 0; index < order; ++index) {
            *reducePts++ = reducer.fLine[index].asSkPoint();
        }
    }
    return SkPathOpsPointsToVerb(order - 1);
}

SkPath::Verb SkReduceOrder::Cubic(const SkPoint a[4], SkPoint* reducePts) {
    SkDCubic cubic;
    cubic.set(a);
    SkReduceOrder reducer;
    int order = reducer.reduce(cubic, kAllow_Quadratics);
    if (order == 2 || order == 3) {  
        for (int index = 0; index < order; ++index) {
            *reducePts++ = reducer.fQuad[index].asSkPoint();
        }
    }
    return SkPathOpsPointsToVerb(order - 1);
}
