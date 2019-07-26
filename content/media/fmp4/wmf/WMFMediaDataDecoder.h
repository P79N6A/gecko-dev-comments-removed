





#if !defined(WMFMediaDataDecoder_h_)
#define WMFMediaDataDecoder_h_


#include "WMF.h"
#include "MP4Reader.h"
#include "MFTDecoder.h"
#include "mozilla/RefPtr.h"

class mp4_demuxer::MP4Sample;

namespace mozilla {




class WMFOutputSource {
public:
  virtual ~WMFOutputSource() {}

  
  
  virtual TemporaryRef<MFTDecoder> Init() = 0;

  
  
  
  virtual HRESULT Input(mp4_demuxer::MP4Sample* aSample) = 0;

  
  
  
  
  
  
  virtual HRESULT Output(int64_t aStreamOffset,
                         nsAutoPtr<MediaData>& aOutput) = 0;
};






class WMFMediaDataDecoder : public MediaDataDecoder {
public:
  WMFMediaDataDecoder(WMFOutputSource* aOutputSource,
                      MediaTaskQueue* aAudioTaskQueue,
                      MediaDataDecoderCallback* aCallback);
  ~WMFMediaDataDecoder();

  virtual nsresult Init() MOZ_OVERRIDE;

  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample);

  virtual nsresult Flush() MOZ_OVERRIDE;

  virtual nsresult Drain() MOZ_OVERRIDE;

  virtual nsresult Shutdown() MOZ_OVERRIDE;

private:

  
  
  void ProcessDecode(mp4_demuxer::MP4Sample* aSample);

  
  
  void ProcessOutput();

  
  
  void ProcessDrain();

  RefPtr<MediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  RefPtr<MFTDecoder> mDecoder;
  nsAutoPtr<WMFOutputSource> mSource;

  
  
  int64_t mLastStreamOffset;
};

} 

#endif 
