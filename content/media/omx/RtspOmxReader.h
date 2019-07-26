




#if !defined(RtspOmxReader_h_)
#define RtspOmxReader_h_

#include "MediaResource.h"
#include "MediaDecoderReader.h"
#include "MediaOmxReader.h"

namespace mozilla {

namespace dom {
  class TimeRanges;
}

class AbstractMediaDecoder;
class RtspMediaResource;





class RtspOmxReader : public MediaOmxReader
{
protected:
  
  nsresult InitOmxDecoder() MOZ_FINAL MOZ_OVERRIDE;
  virtual void EnsureActive() MOZ_OVERRIDE;

public:
  RtspOmxReader(AbstractMediaDecoder* aDecoder)
    : MediaOmxReader(aDecoder) {
    MOZ_COUNT_CTOR(RtspOmxReader);
    NS_ASSERTION(mDecoder, "RtspOmxReader mDecoder is null.");
    NS_ASSERTION(mDecoder->GetResource(),
                 "RtspOmxReader mDecoder->GetResource() is null.");
    mRtspResource = mDecoder->GetResource()->GetRtspPointer();
    MOZ_ASSERT(mRtspResource);
  }

  virtual ~RtspOmxReader() MOZ_OVERRIDE {
    MOZ_COUNT_DTOR(RtspOmxReader);
  }

  
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime,
                        int64_t aCurrentTime) MOZ_FINAL MOZ_OVERRIDE;

  
  
  
  
  
  
  
  
  
  virtual nsresult GetBuffered(mozilla::dom::TimeRanges* aBuffered,
                               int64_t aStartTime) MOZ_FINAL MOZ_OVERRIDE {
    return NS_OK;
  }

  virtual void SetIdle() MOZ_OVERRIDE;

private:
  
  
  
  
  RtspMediaResource* mRtspResource;
};

} 

#endif
