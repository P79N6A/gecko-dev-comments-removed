








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

#endif
