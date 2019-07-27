






#ifndef SkRecordOpts_DEFINED
#define SkRecordOpts_DEFINED

#include "SkRecord.h"


void SkRecordOptimize(SkRecord*);


void SkRecordNoopCulls(SkRecord*);


void SkRecordNoopSaveRestores(SkRecord*);



void SkRecordNoopSaveLayerDrawRestores(SkRecord*);


void SkRecordAnnotateCullingPairs(SkRecord*);


void SkRecordReduceDrawPosTextStrength(SkRecord*);


void SkRecordBoundDrawPosTextH(SkRecord*);

#endif
