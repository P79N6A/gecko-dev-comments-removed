



#ifndef DOM_CAMERA_CAMERAPREVIEWMEDIASTREAM_H
#define DOM_CAMERA_CAMERAPREVIEWMEDIASTREAM_H

#include "VideoFrameContainer.h"
#include "MediaStreamGraph.h"
#include "mozilla/Mutex.h"

namespace mozilla {

class FakeMediaStreamGraph : public MediaStreamGraph
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FakeMediaStreamGraph)
public:
  FakeMediaStreamGraph()
    : MediaStreamGraph(16000)
  {
  }

  virtual void
  DispatchToMainThreadAfterStreamStateUpdate(already_AddRefed<nsIRunnable> aRunnable) override;

protected:
  ~FakeMediaStreamGraph()
  {}
};








class CameraPreviewMediaStream : public MediaStream
{
  typedef mozilla::layers::Image Image;

public:
  explicit CameraPreviewMediaStream(DOMMediaStream* aWrapper);

  virtual CameraPreviewMediaStream* AsCameraPreviewStream() override { return this; };
  virtual void AddAudioOutput(void* aKey) override;
  virtual void SetAudioOutputVolume(void* aKey, float aVolume) override;
  virtual void RemoveAudioOutput(void* aKey) override;
  virtual void AddVideoOutput(VideoFrameContainer* aContainer) override;
  virtual void RemoveVideoOutput(VideoFrameContainer* aContainer) override;
  virtual void ChangeExplicitBlockerCount(int32_t aDelta) override;
  virtual void AddListener(MediaStreamListener* aListener) override;
  virtual void RemoveListener(MediaStreamListener* aListener) override;
  virtual void Destroy() override;
  void OnPreviewStateChange(bool aActive);

  void Invalidate();

  
  void SetCurrentFrame(const gfxIntSize& aIntrinsicSize, Image* aImage);
  void ClearCurrentFrame();
  void RateLimit(bool aLimit);

protected:
  
  
  
  Mutex mMutex;
  int32_t mInvalidatePending;
  uint32_t mDiscardedFrames;
  bool mRateLimit;
  bool mTrackCreated;
  nsRefPtr<FakeMediaStreamGraph> mFakeMediaStreamGraph;
};

} 

#endif 
