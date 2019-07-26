





#include "WMF.h"
#include "nsString.h"



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

} 
