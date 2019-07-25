















#ifndef DOM_CAMERA_GONKCAMERACONTROL_H
#define DOM_CAMERA_GONKCAMERACONTROL_H

#include "prtypes.h"
#include "prrwlock.h"
#include "CameraControl.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

namespace mozilla {

namespace layers {
class GraphicBufferLocked;
}

class nsGonkCameraControl : public nsCameraControl
{
public:
  nsGonkCameraControl(uint32_t aCameraId, nsIThread* aCameraThread);

  const char* GetParameter(const char* aKey);
  const char* GetParameterConstChar(uint32_t aKey);
  double GetParameterDouble(uint32_t aKey);
  void GetParameter(uint32_t aKey, nsTArray<dom::CameraRegion>& aRegions);
  void SetParameter(const char* aKey, const char* aValue);
  void SetParameter(uint32_t aKey, const char* aValue);
  void SetParameter(uint32_t aKey, double aValue);
  void SetParameter(uint32_t aKey, const nsTArray<dom::CameraRegion>& aRegions);
  void PushParameters();

  void ReceiveFrame(layers::GraphicBufferLocked* aBuffer);

protected:
  ~nsGonkCameraControl();

  nsresult GetPreviewStreamImpl(GetPreviewStreamTask* aGetPreviewStream);
  nsresult AutoFocusImpl(AutoFocusTask* aAutoFocus);
  nsresult TakePictureImpl(TakePictureTask* aTakePicture);
  nsresult StartRecordingImpl(StartRecordingTask* aStartRecording);
  nsresult StopRecordingImpl(StopRecordingTask* aStopRecording);
  nsresult PushParametersImpl(PushParametersTask* aPushParameters);
  nsresult PullParametersImpl(PullParametersTask* aPullParameters);

  uint32_t                  mHwHandle;
  double                    mExposureCompensationMin;
  double                    mExposureCompensationStep;
  bool                      mDeferConfigUpdate;
  PRRWLock*                 mRwLock;
  android::CameraParameters mParams;

private:
  nsGonkCameraControl(const nsGonkCameraControl&) MOZ_DELETE;
  nsGonkCameraControl& operator=(const nsGonkCameraControl&) MOZ_DELETE;
};


void ReceiveImage(nsGonkCameraControl* gc, uint8_t* aData, uint32_t aLength);
void AutoFocusComplete(nsGonkCameraControl* gc, bool success);
void ReceiveFrame(nsGonkCameraControl* gc, layers::GraphicBufferLocked* aBuffer);

} 

#endif 
