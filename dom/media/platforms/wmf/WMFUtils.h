





#ifndef WMFUtils_h
#define WMFUtils_h

#include "WMF.h"
#include "nsString.h"
#include "nsRect.h"
#include "TimeUnits.h"
#include "VideoUtils.h"



namespace mozilla {




inline int64_t
UsecsToHNs(int64_t aUsecs) {
  return aUsecs * 10;
}




inline int64_t
HNsToUsecs(int64_t hNanoSecs) {
  return hNanoSecs / 10;
}

HRESULT
HNsToFrames(int64_t aHNs, uint32_t aRate, int64_t* aOutFrames);

HRESULT
GetDefaultStride(IMFMediaType *aType, uint32_t* aOutStride);

int32_t
MFOffsetToInt32(const MFOffset& aOffset);



HRESULT
GetPictureRegion(IMFMediaType* aMediaType, nsIntRect& aOutPictureRegion);



media::TimeUnit
GetSampleDuration(IMFSample* aSample);



media::TimeUnit
GetSampleTime(IMFSample* aSample);

inline bool
IsFlagSet(DWORD flags, DWORD pattern) {
  return (flags & pattern) == pattern;
}

} 

#endif
