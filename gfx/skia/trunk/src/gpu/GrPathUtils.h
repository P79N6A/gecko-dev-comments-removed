






#ifndef GrPathUtils_DEFINED
#define GrPathUtils_DEFINED

#include "SkRect.h"
#include "SkPath.h"
#include "SkTArray.h"

class SkMatrix;




namespace GrPathUtils {
    SkScalar scaleToleranceToSrc(SkScalar devTol,
                                 const SkMatrix& viewM,
                                 const SkRect& pathBounds);

    
    
    int worstCasePointCount(const SkPath&,
                            int* subpaths,
                            SkScalar tol);

    
    
    uint32_t quadraticPointCount(const SkPoint points[], SkScalar tol);

    uint32_t generateQuadraticPoints(const SkPoint& p0,
                                     const SkPoint& p1,
                                     const SkPoint& p2,
                                     SkScalar tolSqd,
                                     SkPoint** points,
                                     uint32_t pointsLeft);

    
    
    uint32_t cubicPointCount(const SkPoint points[], SkScalar tol);

    uint32_t generateCubicPoints(const SkPoint& p0,
                                 const SkPoint& p1,
                                 const SkPoint& p2,
                                 const SkPoint& p3,
                                 SkScalar tolSqd,
                                 SkPoint** points,
                                 uint32_t pointsLeft);

    
    
    
    class QuadUVMatrix {
    public:
        QuadUVMatrix() {};
        
        QuadUVMatrix(const SkPoint controlPts[3]) { this->set(controlPts); }
        void set(const SkPoint controlPts[3]);

        











        template <int N, size_t STRIDE, size_t UV_OFFSET>
        void apply(const void* vertices) {
            intptr_t xyPtr = reinterpret_cast<intptr_t>(vertices);
            intptr_t uvPtr = reinterpret_cast<intptr_t>(vertices) + UV_OFFSET;
            float sx = fM[0];
            float kx = fM[1];
            float tx = fM[2];
            float ky = fM[3];
            float sy = fM[4];
            float ty = fM[5];
            for (int i = 0; i < N; ++i) {
                const SkPoint* xy = reinterpret_cast<const SkPoint*>(xyPtr);
                SkPoint* uv = reinterpret_cast<SkPoint*>(uvPtr);
                uv->fX = sx * xy->fX + kx * xy->fY + tx;
                uv->fY = ky * xy->fX + sy * xy->fY + ty;
                xyPtr += STRIDE;
                uvPtr += STRIDE;
            }
        }
    private:
        float fM[6];
    };

    
    
    
    
    
    
    
    
    void getConicKLM(const SkPoint p[3], const SkScalar weight, SkScalar klm[9]);

    
    
    
    
    
    
    
    
    
    
    
    
    void convertCubicToQuads(const SkPoint p[4],
                             SkScalar tolScale,
                             bool constrainWithinTangents,
                             SkPath::Direction dir,
                             SkTArray<SkPoint, true>* quads);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    int chopCubicAtLoopIntersection(const SkPoint src[4], SkPoint dst[10] = NULL,
                                    SkScalar klm[9] = NULL, SkScalar klm_rev[3] = NULL);

    
    
    
    
    
    
    
    
    
    
    
    void getCubicKLM(const SkPoint p[4], SkScalar klm[9]);
};
#endif
