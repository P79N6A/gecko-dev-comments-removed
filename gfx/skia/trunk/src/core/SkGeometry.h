








#ifndef SkGeometry_DEFINED
#define SkGeometry_DEFINED

#include "SkMatrix.h"





typedef SkPoint SkXRay;







bool SkXRayCrossesLine(const SkXRay& pt, const SkPoint pts[2],
                       bool* ambiguous = NULL);




int SkFindUnitQuadRoots(SkScalar A, SkScalar B, SkScalar C, SkScalar roots[2]);






void SkEvalQuadAt(const SkPoint src[3], SkScalar t, SkPoint* pt,
                  SkVector* tangent = NULL);
void SkEvalQuadAtHalf(const SkPoint src[3], SkPoint* pt,
                      SkVector* tangent = NULL);





void SkChopQuadAt(const SkPoint src[3], SkPoint dst[5], SkScalar t);




void SkChopQuadAtHalf(const SkPoint src[3], SkPoint dst[5]);









int SkFindQuadExtrema(SkScalar a, SkScalar b, SkScalar c, SkScalar tValues[1]);







int SkChopQuadAtYExtrema(const SkPoint src[3], SkPoint dst[5]);
int SkChopQuadAtXExtrema(const SkPoint src[3], SkPoint dst[5]);





float SkFindQuadMaxCurvature(const SkPoint src[3]);








int SkChopQuadAtMaxCurvature(const SkPoint src[3], SkPoint dst[5]);





SK_API void SkConvertQuadToCubic(const SkPoint src[3], SkPoint dst[4]);






void SkGetCubicCoeff(const SkPoint pts[4], SkScalar cx[4], SkScalar cy[4]);




void SkEvalCubicAt(const SkPoint src[4], SkScalar t, SkPoint* locOrNull,
                   SkVector* tangentOrNull, SkVector* curvatureOrNull);





void SkChopCubicAt(const SkPoint src[4], SkPoint dst[7], SkScalar t);




void SkChopCubicAt(const SkPoint src[4], SkPoint dst[], const SkScalar t[],
                   int t_count);




void SkChopCubicAtHalf(const SkPoint src[4], SkPoint dst[7]);










int SkFindCubicExtrema(SkScalar a, SkScalar b, SkScalar c, SkScalar d,
                       SkScalar tValues[2]);









int SkChopCubicAtYExtrema(const SkPoint src[4], SkPoint dst[10]);
int SkChopCubicAtXExtrema(const SkPoint src[4], SkPoint dst[10]);




int SkFindCubicInflections(const SkPoint src[4], SkScalar tValues[2]);





int SkChopCubicAtInflections(const SkPoint src[4], SkPoint dst[10]);

int SkFindCubicMaxCurvature(const SkPoint src[4], SkScalar tValues[3]);
int SkChopCubicAtMaxCurvature(const SkPoint src[4], SkPoint dst[13],
                              SkScalar tValues[3] = NULL);













bool SkXRayCrossesMonotonicCubic(const SkXRay& pt, const SkPoint cubic[4],
                                 bool* ambiguous = NULL);













int SkNumXRayCrossingsForCubic(const SkXRay& pt, const SkPoint cubic[4],
                               bool* ambiguous = NULL);



enum SkRotationDirection {
    kCW_SkRotationDirection,
    kCCW_SkRotationDirection
};




#define kSkBuildQuadArcStorage  17







int SkBuildQuadArc(const SkVector& unitStart, const SkVector& unitStop,
                   SkRotationDirection, const SkMatrix*, SkPoint quadPoints[]);


struct SkConic {
    SkPoint  fPts[3];
    SkScalar fW;

    void set(const SkPoint pts[3], SkScalar w) {
        memcpy(fPts, pts, 3 * sizeof(SkPoint));
        fW = w;
    }

    






    void evalAt(SkScalar t, SkPoint* pos, SkVector* tangent = NULL) const;
    void chopAt(SkScalar t, SkConic dst[2]) const;
    void chop(SkConic dst[2]) const;

    void computeAsQuadError(SkVector* err) const;
    bool asQuadTol(SkScalar tol) const;

    



    int computeQuadPOW2(SkScalar tol) const;

    



    int chopIntoQuadsPOW2(SkPoint pts[], int pow2) const;

    bool findXExtrema(SkScalar* t) const;
    bool findYExtrema(SkScalar* t) const;
    bool chopAtXExtrema(SkConic dst[2]) const;
    bool chopAtYExtrema(SkConic dst[2]) const;

    void computeTightBounds(SkRect* bounds) const;
    void computeFastBounds(SkRect* bounds) const;

    






    bool findMaxCurvature(SkScalar* t) const;
};

#include "SkTemplates.h"




class SkAutoConicToQuads {
public:
    SkAutoConicToQuads() : fQuadCount(0) {}

    











    const SkPoint* computeQuads(const SkConic& conic, SkScalar tol) {
        int pow2 = conic.computeQuadPOW2(tol);
        fQuadCount = 1 << pow2;
        SkPoint* pts = fStorage.reset(1 + 2 * fQuadCount);
        conic.chopIntoQuadsPOW2(pts, pow2);
        return pts;
    }

    const SkPoint* computeQuads(const SkPoint pts[3], SkScalar weight,
                                SkScalar tol) {
        SkConic conic;
        conic.set(pts, weight);
        return computeQuads(conic, tol);
    }

    int countQuads() const { return fQuadCount; }

private:
    enum {
        kQuadCount = 8, 
        kPointCount = 1 + 2 * kQuadCount,
    };
    SkAutoSTMalloc<kPointCount, SkPoint> fStorage;
    int fQuadCount; 
};

#endif
