





#ifndef WMFUtils_h
#define WMFUtils_h

#include "WMF.h"
#include "nsString.h"
#include "nsRect.h"
#include "VideoUtils.h"



namespace mozilla {

nsCString
GetGUIDName(const GUID& guid);





bool
SourceReaderHasStream(IMFSourceReader* aReader, const DWORD aIndex);


class AutoPropVar {
public:
  AutoPropVar() {
    PropVariantInit(&mVar);
  }
  ~AutoPropVar() {
    PropVariantClear(&mVar);
  }
  operator PROPVARIANT&() {
    return mVar;
  }
  PROPVARIANT* operator->() {
    return &mVar;
  }
  PROPVARIANT* operator&() {
    return &mVar;
  }
private:
  PROPVARIANT mVar;
};




inline int64_t
UsecsToHNs(int64_t aUsecs) {
  return aUsecs * 10;
}




inline int64_t
HNsToUsecs(int64_t hNanoSecs) {
  return hNanoSecs / 10;
}



HRESULT
DoGetInterface(IUnknown* aUnknown, void** aInterface);

HRESULT
HNsToFrames(int64_t aHNs, uint32_t aRate, int64_t* aOutFrames);

HRESULT
FramesToUsecs(int64_t aSamples, uint32_t aRate, int64_t* aOutUsecs);

HRESULT
GetDefaultStride(IMFMediaType *aType, uint32_t* aOutStride);

int32_t
MFOffsetToInt32(const MFOffset& aOffset);



HRESULT
GetPictureRegion(IMFMediaType* aMediaType, nsIntRect& aOutPictureRegion);



int64_t
GetSampleDuration(IMFSample* aSample);



int64_t
GetSampleTime(IMFSample* aSample);

inline bool
IsFlagSet(DWORD flags, DWORD pattern) {
  return (flags & pattern) == pattern;
}

} 

#endif
