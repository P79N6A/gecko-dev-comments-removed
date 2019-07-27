













































#include "SkPathOpsCubic.h"
#include "SkPathOpsLine.h"
#include "SkPathOpsQuad.h"
#include "SkReduceOrder.h"
#include "SkTArray.h"
#include "SkTSort.h"

#define USE_CUBIC_END_POINTS 1

static double calc_t_div(const SkDCubic& cubic, double precision, double start) {
    const double adjust = sqrt(3.) / 36;
    SkDCubic sub;
    const SkDCubic* cPtr;
    if (start == 0) {
        cPtr = &cubic;
    } else {
        
        sub = cubic.subDivide(start, 1);
        cPtr = &sub;
    }
    const SkDCubic& c = *cPtr;
    double dx = c[3].fX - 3 * (c[2].fX - c[1].fX) - c[0].fX;
    double dy = c[3].fY - 3 * (c[2].fY - c[1].fY) - c[0].fY;
    double dist = sqrt(dx * dx + dy * dy);
    double tDiv3 = precision / (adjust * dist);
    double t = SkDCubeRoot(tDiv3);
    if (start > 0) {
        t = start + (1 - start) * t;
    }
    return t;
}

SkDQuad SkDCubic::toQuad() const {
    SkDQuad quad;
    quad[0] = fPts[0];
    const SkDPoint fromC1 = {(3 * fPts[1].fX - fPts[0].fX) / 2, (3 * fPts[1].fY - fPts[0].fY) / 2};
    const SkDPoint fromC2 = {(3 * fPts[2].fX - fPts[3].fX) / 2, (3 * fPts[2].fY - fPts[3].fY) / 2};
    quad[1].fX = (fromC1.fX + fromC2.fX) / 2;
    quad[1].fY = (fromC1.fY + fromC2.fY) / 2;
    quad[2] = fPts[3];
    return quad;
}

static bool add_simple_ts(const SkDCubic& cubic, double precision, SkTArray<double, true>* ts) {
    double tDiv = calc_t_div(cubic, precision, 0);
    if (tDiv >= 1) {
        return true;
    }
    if (tDiv >= 0.5) {
        ts->push_back(0.5);
        return true;
    }
    return false;
}

static void addTs(const SkDCubic& cubic, double precision, double start, double end,
        SkTArray<double, true>* ts) {
    double tDiv = calc_t_div(cubic, precision, 0);
    double parts = ceil(1.0 / tDiv);
    for (double index = 0; index < parts; ++index) {
        double newT = start + (index / parts) * (end - start);
        if (newT > 0 && newT < 1) {
            ts->push_back(newT);
        }
    }
}





void SkDCubic::toQuadraticTs(double precision, SkTArray<double, true>* ts) const {
    SkReduceOrder reducer;
    int order = reducer.reduce(*this, SkReduceOrder::kAllow_Quadratics);
    if (order < 3) {
        return;
    }
    double inflectT[5];
    int inflections = findInflections(inflectT);
    SkASSERT(inflections <= 2);
    if (!endsAreExtremaInXOrY()) {
        inflections += findMaxCurvature(&inflectT[inflections]);
        SkASSERT(inflections <= 5);
    }
    SkTQSort<double>(inflectT, &inflectT[inflections - 1]);
    
    
    while (inflections && approximately_less_than_zero(inflectT[0])) {
        memmove(inflectT, &inflectT[1], sizeof(inflectT[0]) * --inflections);
    }
    int start = 0;
    int next = 1;
    while (next < inflections) {
        if (!approximately_equal(inflectT[start], inflectT[next])) {
            ++start;
        ++next;
            continue;
        }
        memmove(&inflectT[start], &inflectT[next], sizeof(inflectT[0]) * (--inflections - start));
    }

    while (inflections && approximately_greater_than_one(inflectT[inflections - 1])) {
        --inflections;
    }
    SkDCubicPair pair;
    if (inflections == 1) {
        pair = chopAt(inflectT[0]);
        int orderP1 = reducer.reduce(pair.first(), SkReduceOrder::kNo_Quadratics);
        if (orderP1 < 2) {
            --inflections;
        } else {
            int orderP2 = reducer.reduce(pair.second(), SkReduceOrder::kNo_Quadratics);
            if (orderP2 < 2) {
                --inflections;
            }
        }
    }
    if (inflections == 0 && add_simple_ts(*this, precision, ts)) {
        return;
    }
    if (inflections == 1) {
        pair = chopAt(inflectT[0]);
        addTs(pair.first(), precision, 0, inflectT[0], ts);
        addTs(pair.second(), precision, inflectT[0], 1, ts);
        return;
    }
    if (inflections > 1) {
        SkDCubic part = subDivide(0, inflectT[0]);
        addTs(part, precision, 0, inflectT[0], ts);
        int last = inflections - 1;
        for (int idx = 0; idx < last; ++idx) {
            part = subDivide(inflectT[idx], inflectT[idx + 1]);
            addTs(part, precision, inflectT[idx], inflectT[idx + 1], ts);
        }
        part = subDivide(inflectT[last], 1);
        addTs(part, precision, inflectT[last], 1, ts);
        return;
    }
    addTs(*this, precision, 0, 1, ts);
}
