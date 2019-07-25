








#ifndef SkConcaveToTriangles_DEFINED
#define SkConcaveToTriangles_DEFINED

#include "SkPoint.h"
#include "SkTDArray.h"











bool SkConcaveToTriangles(size_t count,
                          const SkPoint pts[],
                          SkTDArray<SkPoint> *triangles);


#endif  
