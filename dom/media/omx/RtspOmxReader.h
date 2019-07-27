




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
  
  nsresult InitOmxDecoder() final override;
  virtual void EnsureActive() override;

public:
  RtspOmxReader(AbstractMediaDecoder* aDecoder)
    : MediaOmxReader(aDecoder)
    , mEnsureActiveFromSeek(false)
  {
    MOZ_COUNT_CTOR(RtspOmxReader);
    NS_ASSERTION(mDecoder, "RtspOmxReader mDecoder is null.");
    NS_ASSERTION(mDecoder->GetResource(),
                 "RtspOmxReader mDecoder->GetResource() is null.");
    mRtspResource = mDecoder->GetResource()->GetRtspPointer();
    MOZ_ASSERT(mRtspResource);
  }

  virtual ~RtspOmxReader() override {
    MOZ_COUNT_DTOR(RtspOmxReader);
  }

  
  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) final override;

  
  
  
  
  
  
  
  
  
  virtual media::TimeIntervals GetBuffered() final override {
    return media::TimeIntervals::Invalid();
  }

  virtual void SetIdle() override;

  virtual nsRefPtr<MediaDecoderReader::MetadataPromise> AsyncReadMetadata()
    override;

  virtual void HandleResourceAllocated() override;

private:
  
  
  
  
  RtspMediaResource* mRtspResource;

  bool mEnsureActiveFromSeek;
};

} 

#endif
