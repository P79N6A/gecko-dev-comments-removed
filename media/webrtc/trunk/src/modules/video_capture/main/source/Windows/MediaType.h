





#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_MEDIATYPE_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_MEDIATYPE_H_

#include "dshow.h"

namespace mozilla {
namespace media {





class MediaType : public AM_MEDIA_TYPE
{
public:
  
  MediaType();
  MediaType(const AM_MEDIA_TYPE* aMediaType);
  MediaType(const MediaType& aMediaType);

  ~MediaType();

  
  
  HRESULT Assign(const AM_MEDIA_TYPE* aMediaType);
  HRESULT Assign(const AM_MEDIA_TYPE& aMediaType) {
    return Assign(&aMediaType);
  }

  
  
  bool IsEqual(const AM_MEDIA_TYPE* aMediaType);
  bool IsEqual(const AM_MEDIA_TYPE& aMediaType) {
    return IsEqual(&aMediaType);
  }

  
  
  bool MatchesPartial(const AM_MEDIA_TYPE* aMediaType) const;
  bool MatchesPartial(const AM_MEDIA_TYPE& aMediaType) const {
    return MatchesPartial(&aMediaType);
  }

  
  
  bool IsPartiallySpecified() const;

  
  
  void Clear();

  
  
  void Forget();

  const GUID* Type() const
  {
    return &majortype;
  }
  void SetType(const GUID* aType)
  {
    majortype = *aType;
  }

  const GUID* Subtype() const
  {
    return &subtype;
  }
  void SetSubtype(const GUID* aType)
  {
    subtype = *aType;
  }

  const GUID* FormatType() const
  {
    return &formattype;
  }
  void SetFormatType(const GUID* aType)
  {
    formattype = *aType;
  }

  BOOL TemporalCompression() const { return bTemporalCompression; }
  void SetTemporalCompression(BOOL aCompression)
  {
    bTemporalCompression = aCompression;
  }

  ULONG SampleSize() const { return lSampleSize; }
  void SetSampleSize(ULONG aSampleSize)
  {
    lSampleSize = aSampleSize;
  }

  BYTE* AllocFormatBuffer(SIZE_T aSize);
  BYTE* Format() const { return pbFormat; }

  
  
  
  void operator delete(void* ptr);
  void* operator new(size_t sz) throw();

  operator AM_MEDIA_TYPE* () {
    return static_cast<AM_MEDIA_TYPE*>(this);
  }

private:
  
  MediaType& operator=(const MediaType&);
  MediaType& operator=(const AM_MEDIA_TYPE&);
};

}
}

#endif
