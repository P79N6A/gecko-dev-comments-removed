














#if !defined(DASHRepReader_h_)
#define DASHRepReader_h_

#include "VideoUtils.h"
#include "MediaDecoderReader.h"
#include "DASHReader.h"

namespace mozilla {

class DASHReader;

class DASHRepReader : public MediaDecoderReader
{
public:
  DASHRepReader(AbstractMediaDecoder* aDecoder)
    : MediaDecoderReader(aDecoder) { }
  virtual ~DASHRepReader() { }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DASHRepReader)

  virtual void SetMainReader(DASHReader *aMainReader) = 0;

  
  virtual void SetInitByteRange(MediaByteRange &aByteRange) = 0;

  
  virtual void SetIndexByteRange(MediaByteRange &aByteRange) = 0;

  
  virtual int64_t GetSubsegmentForSeekTime(int64_t aSeekToTime) = 0;

  
  virtual nsresult GetSubsegmentByteRanges(nsTArray<MediaByteRange>& aByteRanges) = 0;

  
  virtual bool HasReachedSubsegment(uint32_t aSubsegmentIndex) = 0;

  
  virtual void RequestSeekToSubsegment(uint32_t aIdx) = 0;

  
  
  
  virtual void RequestSwitchAtSubsegment(int32_t aCluster,
                                         MediaDecoderReader* aNextReader) = 0;

  
  virtual bool IsDataCachedAtEndOfSubsegments() = 0;
};

}

#endif 
