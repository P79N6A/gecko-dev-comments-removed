



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

class RecorderProfileManager;

class CameraControlImpl : public ICameraControl
{
public:
  explicit CameraControlImpl(uint32_t aCameraId);
  virtual void AddListener(CameraControlListener* aListener) MOZ_OVERRIDE;
  virtual void RemoveListener(CameraControlListener* aListener) MOZ_OVERRIDE;

  
  virtual nsresult Start(const Configuration* aConfig = nullptr) MOZ_OVERRIDE;
  virtual nsresult Stop() MOZ_OVERRIDE;
  virtual nsresult SetConfiguration(const Configuration& aConfig) MOZ_OVERRIDE;
  virtual nsresult StartPreview() MOZ_OVERRIDE;
  virtual nsresult StopPreview() MOZ_OVERRIDE;
  virtual nsresult AutoFocus() MOZ_OVERRIDE;
  virtual nsresult StartFaceDetection() MOZ_OVERRIDE;
  virtual nsresult StopFaceDetection() MOZ_OVERRIDE;
  virtual nsresult TakePicture() MOZ_OVERRIDE;
  virtual nsresult StartRecording(DeviceStorageFileDescriptor* aFileDescriptor,
                                  const StartRecordingOptions* aOptions) MOZ_OVERRIDE;
  virtual nsresult StopRecording() MOZ_OVERRIDE;
  virtual nsresult ResumeContinuousFocus() MOZ_OVERRIDE;

  already_AddRefed<RecorderProfileManager> GetRecorderProfileManager();
  uint32_t GetCameraId() { return mCameraId; }

  virtual void Shutdown() MOZ_OVERRIDE;

  
  void OnShutter();
  void OnClosed();
  void OnUserError(CameraControlListener::UserContext aContext, nsresult aError);
  void OnSystemError(CameraControlListener::SystemContext aContext, nsresult aError);
  void OnAutoFocusMoving(bool aIsMoving);

protected:
  
  void OnAutoFocusComplete(bool aAutoFocusSucceeded);
  void OnFacesDetected(const nsTArray<Face>& aFaces);
  void OnTakePictureComplete(uint8_t* aData, uint32_t aLength, const nsAString& aMimeType);

  void OnRateLimitPreview(bool aLimit);
  bool OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight);
  void OnRecorderStateChange(CameraControlListener::RecorderState aState,
                             int32_t aStatus = -1, int32_t aTrackNumber = -1);
  void OnPreviewStateChange(CameraControlListener::PreviewState aState);
  void OnHardwareStateChange(CameraControlListener::HardwareState aState);
  void OnConfigurationChange();

  
  
  
  
  
  static nsWeakPtr sCameraThread;
  nsCOMPtr<nsIThread> mCameraThread;

  virtual ~CameraControlImpl();

  virtual void BeginBatchParameterSet() MOZ_OVERRIDE { }
  virtual void EndBatchParameterSet() MOZ_OVERRIDE { }

  
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

  virtual already_AddRefed<RecorderProfileManager> GetRecorderProfileManagerImpl() = 0;

  void OnShutterInternal();
  void OnClosedInternal();

  uint32_t mCameraId;

  CameraControlListener::CameraListenerConfiguration mCurrentConfiguration;

  CameraControlListener::PreviewState   mPreviewState;
  CameraControlListener::HardwareState  mHardwareState;

private:
  CameraControlImpl(const CameraControlImpl&) MOZ_DELETE;
  CameraControlImpl& operator=(const CameraControlImpl&) MOZ_DELETE;
};

} 

#endif 
