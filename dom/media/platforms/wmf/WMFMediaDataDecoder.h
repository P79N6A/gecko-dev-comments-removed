





#if !defined(WMFMediaDataDecoder_h_)
#define WMFMediaDataDecoder_h_


#include "WMF.h"
#include "MP4Reader.h"
#include "MFTDecoder.h"
#include "mozilla/RefPtr.h"

namespace mozilla {




class MFTManager {
public:
  virtual ~MFTManager() {}

  
  
  virtual already_AddRefed<MFTDecoder> Init() = 0;

  
  
  
  virtual HRESULT Input(MediaRawData* aSample) = 0;

  
  
  
  
  
  
  virtual HRESULT Output(int64_t aStreamOffset,
                         nsRefPtr<MediaData>& aOutput) = 0;

  
  virtual void Shutdown() = 0;

  virtual bool IsHardwareAccelerated() const { return false; }

};






class WMFMediaDataDecoder : public MediaDataDecoder {
public:
  WMFMediaDataDecoder(MFTManager* aOutputSource,
                      FlushableTaskQueue* aAudioTaskQueue,
                      MediaDataDecoderCallback* aCallback);
  ~WMFMediaDataDecoder();

  virtual nsresult Init() override;

  virtual nsresult Input(MediaRawData* aSample);

  virtual nsresult Flush() override;

  virtual nsresult Drain() override;

  virtual nsresult Shutdown() override;

  virtual bool IsHardwareAccelerated() const override;

private:

  
  
  void ProcessDecode(MediaRawData* aSample);

  
  
  void ProcessOutput();

  
  
  void ProcessFlush();

  
  
  void ProcessDrain();

  void ProcessShutdown();

  RefPtr<FlushableTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  RefPtr<MFTDecoder> mDecoder;
  nsAutoPtr<MFTManager> mMFTManager;

  
  
  int64_t mLastStreamOffset;

  
  Monitor mMonitor;
  
  
  
  bool mIsFlushing;

  bool mIsShutDown;

  
  bool mHasSuccessfulOutput = false;
  bool mRecordedError = false;
};

} 

#endif 
