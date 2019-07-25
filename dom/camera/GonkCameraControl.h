















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
  nsGonkCameraControl(PRUint32 aCameraId, nsIThread* aCameraThread);

  const char* GetParameter(const char* aKey);
  const char* GetParameterConstChar(PRUint32 aKey);
  double GetParameterDouble(PRUint32 aKey);
  void GetParameter(PRUint32 aKey, nsTArray<dom::CameraRegion>& aRegions);
  void SetParameter(const char* aKey, const char* aValue);
  void SetParameter(PRUint32 aKey, const char* aValue);
  void SetParameter(PRUint32 aKey, double aValue);
  void SetParameter(PRUint32 aKey, const nsTArray<dom::CameraRegion>& aRegions);
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

  PRUint32                  mHwHandle;
  double                    mExposureCompensationMin;
  double                    mExposureCompensationStep;
  bool                      mDeferConfigUpdate;
  PRRWLock*                 mRwLock;
  android::CameraParameters mParams;

private:
  nsGonkCameraControl(const nsGonkCameraControl&) MOZ_DELETE;
  nsGonkCameraControl& operator=(const nsGonkCameraControl&) MOZ_DELETE;
};


void ReceiveImage(nsGonkCameraControl* gc, PRUint8* aData, PRUint32 aLength);
void AutoFocusComplete(nsGonkCameraControl* gc, bool success);
void ReceiveFrame(nsGonkCameraControl* gc, layers::GraphicBufferLocked* aBuffer);

} 

#endif 
