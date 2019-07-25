















#ifndef DOM_CAMERA_GONKCAMERAHWMGR_H
#define DOM_CAMERA_GONKCAMERAHWMGR_H

#include "libcameraservice/CameraHardwareInterface.h"
#include "binder/IMemory.h"
#include "mozilla/ReentrantMonitor.h"

#include "GonkCameraControl.h"

#define DOM_CAMERA_LOG_LEVEL  3
#include "CameraCommon.h"

#include "GonkNativeWindow.h"


#define GIHM_TIMING_RECEIVEFRAME    0
#define GIHM_TIMING_OVERALL         1

using namespace mozilla;
using namespace android;

namespace mozilla {

typedef class nsGonkCameraControl GonkCamera;

class GonkCameraHardware : GonkNativeWindowNewFrameCallback
{
protected:
  GonkCameraHardware(GonkCamera* aTarget, uint32_t aCamera);
  ~GonkCameraHardware();
  void init();

  static void                   DataCallback(int32_t aMsgType, const sp<IMemory> &aDataPtr, camera_frame_metadata_t* aMetadata, void* aUser);
  static void                   NotifyCallback(int32_t aMsgType, int32_t ext1, int32_t ext2, void* aUser);

public:
  virtual void OnNewFrame() MOZ_OVERRIDE;

  static void                   ReleaseHandle(uint32_t aHwHandle);
  static uint32_t               GetHandle(GonkCamera* aTarget, uint32_t aCamera);
  static uint32_t               GetFps(uint32_t aHwHandle);
  static void                   GetPreviewSize(uint32_t aHwHandle, uint32_t* aWidth, uint32_t* aHeight);
  static void                   SetPreviewSize(uint32_t aHwHandle, uint32_t aWidth, uint32_t aHeight);
  static int                    AutoFocus(uint32_t aHwHandle);
  static void                   CancelAutoFocus(uint32_t aHwHandle);
  static int                    TakePicture(uint32_t aHwHandle);
  static void                   CancelTakePicture(uint32_t aHwHandle);
  static int                    StartPreview(uint32_t aHwHandle);
  static void                   StopPreview(uint32_t aHwHandle);
  static int                    PushParameters(uint32_t aHwHandle, const CameraParameters& aParams);
  static void                   PullParameters(uint32_t aHwHandle, CameraParameters& aParams);

  enum {
    PREVIEW_FORMAT_UNKNOWN,
    PREVIEW_FORMAT_YUV420P,
    PREVIEW_FORMAT_YUV420SP
  };
  
  static uint32_t               GetPreviewFormat(uint32_t aHwHandle);

protected:
  static GonkCameraHardware*    sHw;
  static uint32_t               sHwHandle;

  static GonkCameraHardware*    GetHardware(uint32_t aHwHandle)
  {
    if (aHwHandle == sHwHandle) {
      




      return sHw;
    }
    return nullptr;
  }

  
  void SetPreviewSize(uint32_t aWidth, uint32_t aHeight);
  int StartPreview();

  uint32_t                      mCamera;
  uint32_t                      mWidth;
  uint32_t                      mHeight;
  uint32_t                      mFps;
  uint32_t                      mPreviewFormat;
  bool                          mClosing;
  mozilla::ReentrantMonitor     mMonitor;
  uint32_t                      mNumFrames;
  sp<CameraHardwareInterface>   mHardware;
  GonkCamera*                   mTarget;
  camera_module_t*              mModule;
  sp<ANativeWindow>             mWindow;
  CameraParameters              mParams;
#if GIHM_TIMING_OVERALL
  struct timespec               mStart;
  struct timespec               mAutoFocusStart;
#endif
  bool                          mInitialized;

  bool IsInitialized()
  {
    return mInitialized;
  }

private:
  GonkCameraHardware(const GonkCameraHardware&) MOZ_DELETE;
  GonkCameraHardware& operator=(const GonkCameraHardware&) MOZ_DELETE;
};

} 

#endif 
