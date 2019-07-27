





#if !defined(WMFMediaDataDecoder_h_)
#define WMFMediaDataDecoder_h_


#include "WMF.h"
#include "MP4Reader.h"
#include "MFTDecoder.h"
#include "mozilla/RefPtr.h"

namespace mp4_demuxer {
class MP4Sample;
}

namespace mozilla {




class MFTManager {
public:
  virtual ~MFTManager() {}

  
  
  virtual TemporaryRef<MFTDecoder> Init() = 0;

  
  
  
  virtual HRESULT Input(mp4_demuxer::MP4Sample* aSample) = 0;

  
  
  
  
  
  
  virtual HRESULT Output(int64_t aStreamOffset,
                         nsAutoPtr<MediaData>& aOutput) = 0;

  
  virtual void Shutdown() = 0;
};






class WMFMediaDataDecoder : public MediaDataDecoder {
public:
  WMFMediaDataDecoder(MFTManager* aOutputSource,
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

  void ProcessShutdown();

  RefPtr<MediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  RefPtr<MFTDecoder> mDecoder;
  nsAutoPtr<MFTManager> mMFTManager;

  
  
  int64_t mLastStreamOffset;
};

} 

#endif 
