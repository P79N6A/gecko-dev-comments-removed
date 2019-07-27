





#if !defined(nsDirectShowSource_h___)
#define nsDirectShowSource_h___

#include "BaseFilter.h"
#include "BasePin.h"
#include "MediaType.h"

#include "nsDeque.h"
#include "nsAutoPtr.h"
#include "DirectShowUtils.h"
#include "mozilla/RefPtr.h"

namespace mozilla {

class MediaResource;
class OutputPin;













class DECLSPEC_UUID("5c2a7ad0-ba82-4659-9178-c4719a2765d6")
SourceFilter : public media::BaseFilter
{
public:

  
  SourceFilter(const GUID& aMajorType, const GUID& aSubType);
  ~SourceFilter();

  nsresult Init(MediaResource *aResource, int64_t aMP3Offset);

  
  
  int GetPinCount() override { return 1; }

  media::BasePin* GetPin(int n) override;

  
  const media::MediaType* GetMediaType() const;

  uint32_t GetAndResetBytesConsumedCount();

protected:

  
  nsAutoPtr<OutputPin> mOutputPin;

  
  media::MediaType mMediaType;

};

} 


#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(mozilla::SourceFilter, 0x5c2a7ad0,0xba82,0x4659,0x91,0x78,0xc4,0x71,0x9a,0x27,0x65,0xd6);
#endif

#endif
