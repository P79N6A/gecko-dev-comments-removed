








#ifndef GrPathUtils_DEFINED
#define GrPathUtils_DEFINED

#include "GrMatrix.h"
#include "GrPath.h"




namespace GrPathUtils {
    GrScalar scaleToleranceToSrc(GrScalar devTol,
                                 const GrMatrix& viewM,
                                 const GrRect& pathBounds);

    
    
    int worstCasePointCount(const GrPath&,
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

};
#endif
