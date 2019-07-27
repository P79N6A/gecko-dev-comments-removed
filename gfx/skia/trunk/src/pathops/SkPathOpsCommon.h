





#ifndef SkPathOpsCommon_DEFINED
#define SkPathOpsCommon_DEFINED

#include "SkOpAngle.h"
#include "SkOpContour.h"
#include "SkTDArray.h"

class SkPathWriter;

void Assemble(const SkPathWriter& path, SkPathWriter* simple);

SkOpSegment* FindChase(SkTDArray<SkOpSpan*>* chase, int* tIndex, int* endIndex);
SkOpSegment* FindSortableTop(const SkTArray<SkOpContour*, true>& , SkOpAngle::IncludeType ,
                             bool* firstContour, int* index, int* endIndex, SkPoint* topLeft,
                             bool* unsortable, bool* done, bool* onlyVertical, bool firstPass);
SkOpSegment* FindUndone(SkTArray<SkOpContour*, true>& contourList, int* start, int* end);
void MakeContourList(SkTArray<SkOpContour>& contours, SkTArray<SkOpContour*, true>& list,
                     bool evenOdd, bool oppEvenOdd);
bool HandleCoincidence(SkTArray<SkOpContour*, true>* , int );

#if DEBUG_ACTIVE_SPANS || DEBUG_ACTIVE_SPANS_FIRST_ONLY
void DebugShowActiveSpans(SkTArray<SkOpContour*, true>& contourList);
#endif

#endif
