





#ifndef SkPathOps_DEFINED
#define SkPathOps_DEFINED

#include "SkPreConfig.h"

class SkPath;





enum SkPathOp {
    kDifference_PathOp,         
    kIntersect_PathOp,          
    kUnion_PathOp,              
    kXOR_PathOp,                
    kReverseDifference_PathOp,  
};
















bool SK_API Op(const SkPath& one, const SkPath& two, SkPathOp op, SkPath* result);













bool SK_API Simplify(const SkPath& path, SkPath* result);

#endif
