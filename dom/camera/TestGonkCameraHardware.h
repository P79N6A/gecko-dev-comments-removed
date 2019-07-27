















#ifndef DOM_CAMERA_TESTGONKCAMERAHARDWARE_H
#define DOM_CAMERA_TESTGONKCAMERAHARDWARE_H

#include "GonkCameraHwMgr.h"
#include "nsAutoPtr.h"
#include "nsIDOMEventListener.h"
#include "mozilla/CondVar.h"

namespace mozilla {

class TestGonkCameraHardware : public android::GonkCameraHardware
{
#ifndef MOZ_WIDGET_GONK
  NS_DECL_ISUPPORTS_INHERITED
#endif

public:
  virtual nsresult Init() MOZ_OVERRIDE;
  virtual int AutoFocus() MOZ_OVERRIDE;
  virtual int StartFaceDetection() MOZ_OVERRIDE;
  virtual int StopFaceDetection() MOZ_OVERRIDE;
  virtual int TakePicture() MOZ_OVERRIDE;
  virtual void CancelTakePicture() MOZ_OVERRIDE;
  virtual int StartPreview() MOZ_OVERRIDE;
  virtual void StopPreview() MOZ_OVERRIDE;
  virtual int PushParameters(const mozilla::GonkCameraParameters& aParams) MOZ_OVERRIDE;
  virtual nsresult PullParameters(mozilla::GonkCameraParameters& aParams) MOZ_OVERRIDE;
  virtual int StartRecording() MOZ_OVERRIDE;
  virtual int StopRecording() MOZ_OVERRIDE;
  virtual int StoreMetaDataInBuffers(bool aEnabled) MOZ_OVERRIDE;
#ifdef MOZ_WIDGET_GONK
  virtual int PushParameters(const android::CameraParameters& aParams) MOZ_OVERRIDE;
  virtual void PullParameters(android::CameraParameters& aParams) MOZ_OVERRIDE;
#endif

  TestGonkCameraHardware(mozilla::nsGonkCameraControl* aTarget,
                         uint32_t aCameraId,
                         const android::sp<android::Camera>& aCamera);

protected:
  virtual ~TestGonkCameraHardware();

  class ControlMessage;
  class PushParametersDelegate;
  class PullParametersDelegate;

  nsresult WaitWhileRunningOnMainThread(nsRefPtr<ControlMessage> aRunnable);

  nsCOMPtr<nsIDOMEventListener> mDomListener;
  nsCOMPtr<nsIThread> mCameraThread;
  mozilla::Mutex mMutex;
  mozilla::CondVar mCondVar;
  nsresult mStatus;

private:
  TestGonkCameraHardware(const TestGonkCameraHardware&) = delete;
  TestGonkCameraHardware& operator=(const TestGonkCameraHardware&) = delete;
};

} 

#endif 
