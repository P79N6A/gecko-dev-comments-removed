



#ifndef DOM_CAMERA_CAMERAPREVIEW_H
#define DOM_CAMERA_CAMERAPREVIEW_H

#include "MediaStreamGraph.h"
#include "StreamBuffer.h"
#include "nsDOMMediaStream.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

using namespace mozilla;
using namespace mozilla::layers;

namespace mozilla {

class CameraPreview : public nsDOMMediaStream
                    , public MediaStreamListener
{
public:
  NS_DECL_ISUPPORTS

  CameraPreview(nsIThread* aCameraThread, uint32_t aWidth, uint32_t aHeight);

  void SetFrameRate(uint32_t aFramesPerSecond);

  NS_IMETHODIMP
  GetCurrentTime(double* aCurrentTime) {
    return nsDOMMediaStream::GetCurrentTime(aCurrentTime);
  }

  void Start();
  void Stop();

  virtual nsresult StartImpl() = 0;
  virtual nsresult StopImpl() = 0;

protected:
  virtual ~CameraPreview();

  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mFramesPerSecond;
  SourceMediaStream* mInput;
  nsRefPtr<mozilla::layers::ImageContainer> mImageContainer;
  VideoSegment mVideoSegment;
  uint32_t mFrameCount;
  nsCOMPtr<nsIThread> mCameraThread;

  enum { TRACK_VIDEO = 1 };

private:
  CameraPreview(const CameraPreview&) MOZ_DELETE;
  CameraPreview& operator=(const CameraPreview&) MOZ_DELETE;
};

} 

#endif 
