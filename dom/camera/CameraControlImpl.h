



#ifndef DOM_CAMERA_CAMERACONTROLIMPL_H
#define DOM_CAMERA_CAMERACONTROLIMPL_H

#include "nsTArray.h"
#include "nsWeakPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsIFile.h"
#include "nsProxyRelease.h"
#include "AutoRwLock.h"
#include "nsIDOMDeviceStorage.h"
#include "ICameraControl.h"
#include "CameraCommon.h"
#include "DeviceStorage.h"
#include "DeviceStorageFileDescriptor.h"
#include "CameraControlListener.h"

namespace mozilla {

namespace layers {
  class Image;
}

class CameraControlImpl : public ICameraControl
{
public:
  explicit CameraControlImpl();
  virtual void AddListener(CameraControlListener* aListener) override;
  virtual void RemoveListener(CameraControlListener* aListener) override;

  
  virtual nsresult Start(const Configuration* aConfig = nullptr) override;
  virtual nsresult Stop() override;
  virtual nsresult SetConfiguration(const Configuration& aConfig) override;
  virtual nsresult StartPreview() override;
  virtual nsresult StopPreview() override;
  virtual nsresult AutoFocus() override;
  virtual nsresult StartFaceDetection() override;
  virtual nsresult StopFaceDetection() override;
  virtual nsresult TakePicture() override;
  virtual nsresult StartRecording(DeviceStorageFileDescriptor* aFileDescriptor,
                                  const StartRecordingOptions* aOptions) override;
  virtual nsresult StopRecording() override;
  virtual nsresult ResumeContinuousFocus() override;

  
  void OnShutter();
  void OnUserError(CameraControlListener::UserContext aContext, nsresult aError);
  void OnSystemError(CameraControlListener::SystemContext aContext, nsresult aError);
  void OnAutoFocusMoving(bool aIsMoving);

protected:
  
  void OnAutoFocusComplete(bool aAutoFocusSucceeded);
  void OnFacesDetected(const nsTArray<Face>& aFaces);
  void OnTakePictureComplete(const uint8_t* aData, uint32_t aLength, const nsAString& aMimeType);

  void OnRateLimitPreview(bool aLimit);
  bool OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight);
  void OnRecorderStateChange(CameraControlListener::RecorderState aState,
                             int32_t aStatus = -1, int32_t aTrackNumber = -1);
  void OnPreviewStateChange(CameraControlListener::PreviewState aState);
  void OnHardwareStateChange(CameraControlListener::HardwareState aState,
                             nsresult aReason);
  void OnConfigurationChange();

  
  
  
  
  
  static StaticRefPtr<nsIThread> sCameraThread;
  nsCOMPtr<nsIThread> mCameraThread;

  virtual ~CameraControlImpl();

  virtual void BeginBatchParameterSet() override { }
  virtual void EndBatchParameterSet() override { }

  
  void AddListenerImpl(already_AddRefed<CameraControlListener> aListener);
  void RemoveListenerImpl(CameraControlListener* aListener);
  nsTArray<nsRefPtr<CameraControlListener> > mListeners;
  PRRWLock* mListenerLock;

  class ControlMessage;
  class ListenerMessage;

  nsresult Dispatch(ControlMessage* aMessage);

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult StartImpl(const Configuration* aConfig = nullptr) = 0;
  virtual nsresult StopImpl() = 0;
  virtual nsresult SetConfigurationImpl(const Configuration& aConfig) = 0;
  virtual nsresult StartPreviewImpl() = 0;
  virtual nsresult StopPreviewImpl() = 0;
  virtual nsresult AutoFocusImpl() = 0;
  virtual nsresult StartFaceDetectionImpl() = 0;
  virtual nsresult StopFaceDetectionImpl() = 0;
  virtual nsresult TakePictureImpl() = 0;
  virtual nsresult StartRecordingImpl(DeviceStorageFileDescriptor* aFileDescriptor,
                                      const StartRecordingOptions* aOptions) = 0;
  virtual nsresult StopRecordingImpl() = 0;
  virtual nsresult ResumeContinuousFocusImpl() = 0;
  virtual nsresult PushParametersImpl() = 0;
  virtual nsresult PullParametersImpl() = 0;

  void OnShutterInternal();
  void OnClosedInternal();

  CameraControlListener::CameraListenerConfiguration mCurrentConfiguration;

  CameraControlListener::PreviewState   mPreviewState;
  CameraControlListener::HardwareState  mHardwareState;
  nsresult                              mHardwareStateChangeReason;

private:
  CameraControlImpl(const CameraControlImpl&) = delete;
  CameraControlImpl& operator=(const CameraControlImpl&) = delete;
};

} 

#endif 
