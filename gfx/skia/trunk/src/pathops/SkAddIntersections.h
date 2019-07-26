





#ifndef SkAddIntersections_DEFINED
#define SkAddIntersections_DEFINED

#include "SkIntersectionHelper.h"
#include "SkIntersections.h"
#include "SkTArray.h"

bool AddIntersectTs(SkOpContour* test, SkOpContour* next);
void AddSelfIntersectTs(SkOpContour* test);
void CoincidenceCheck(SkTArray<SkOpContour*, true>* contourList, int total);

#endif
