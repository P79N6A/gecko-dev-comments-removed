








#ifndef GrPathUtils_DEFINED
#define GrPathUtils_DEFINED

#include "GrMatrix.h"
#include "SkPath.h"
#include "SkTArray.h"




namespace GrPathUtils {
    GrScalar scaleToleranceToSrc(GrScalar devTol,
                                 const GrMatrix& viewM,
                                 const GrRect& pathBounds);

    
    
    int worstCasePointCount(const SkPath&,
                            int* subpaths,
                            GrScalar tol);

    
    
    uint32_t quadraticPointCount(const GrPoint points[], GrScalar tol);

    uint32_t generateQuadraticPoints(const GrPoint& p0,
                                     const GrPoint& p1,
                                     const GrPoint& p2,
                                     GrScalar tolSqd,
                                     GrPoint** points,
                                     uint32_t pointsLeft);

    
    
    uint32_t cubicPointCount(const GrPoint points[], GrScalar tol);

    uint32_t generateCubicPoints(const GrPoint& p0,
                                 const GrPoint& p1,
                                 const GrPoint& p2,
                                 const GrPoint& p3,
                                 GrScalar tolSqd,
                                 GrPoint** points,
                                 uint32_t pointsLeft);

    
    
    
    class QuadUVMatrix {
    public:
        QuadUVMatrix() {};
        
        QuadUVMatrix(const GrPoint controlPts[3]) { this->set(controlPts); }
        void set(const GrPoint controlPts[3]);

        











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
                const GrPoint* xy = reinterpret_cast<const GrPoint*>(xyPtr);
                GrPoint* uv = reinterpret_cast<GrPoint*>(uvPtr);
                uv->fX = sx * xy->fX + kx * xy->fY + tx;
                uv->fY = ky * xy->fX + sy * xy->fY + ty;
                xyPtr += STRIDE;
                uvPtr += STRIDE;
            }
        }
    private:
        float fM[6];
    };


    
    
    
    
    
    
    
    
    
    
    
    
    void convertCubicToQuads(const GrPoint p[4],
                             SkScalar tolScale,
                             bool constrainWithinTangents,
                             SkPath::Direction dir,
                             SkTArray<SkPoint, true>* quads);
};
#endif
