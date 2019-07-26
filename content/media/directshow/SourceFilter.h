





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
class ReadRequest;
class OutputPin;













class DECLSPEC_UUID("5c2a7ad0-ba82-4659-9178-c4719a2765d6")
SourceFilter : public media::BaseFilter
{
public:

  
  SourceFilter(const GUID& aMajorType, const GUID& aSubType);
  ~SourceFilter();

  nsresult Init(MediaResource *aResource);

  
  
  int GetPinCount() MOZ_OVERRIDE { return 1; }

  media::BasePin* GetPin(int n) MOZ_OVERRIDE;

  
  const media::MediaType* GetMediaType() const;

  uint32_t GetAndResetBytesConsumedCount();

protected:

  
  nsAutoPtr<OutputPin> mOutputPin;

  
  media::MediaType mMediaType;

};

} 

#endif
