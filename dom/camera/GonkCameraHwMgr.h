















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
  GonkCameraHardware(GonkCamera* aTarget, PRUint32 aCamera);
  ~GonkCameraHardware();
  void init();

  static void                   DataCallback(int32_t aMsgType, const sp<IMemory> &aDataPtr, camera_frame_metadata_t* aMetadata, void* aUser);
  static void                   NotifyCallback(int32_t aMsgType, int32_t ext1, int32_t ext2, void* aUser);

public:
  virtual void OnNewFrame() MOZ_OVERRIDE;

  static void                   ReleaseHandle(PRUint32 aHwHandle);
  static PRUint32               GetHandle(GonkCamera* aTarget, PRUint32 aCamera);
  static PRUint32               GetFps(PRUint32 aHwHandle);
  static void                   GetPreviewSize(PRUint32 aHwHandle, PRUint32* aWidth, PRUint32* aHeight);
  static void                   SetPreviewSize(PRUint32 aHwHandle, PRUint32 aWidth, PRUint32 aHeight);
  static int                    AutoFocus(PRUint32 aHwHandle);
  static void                   CancelAutoFocus(PRUint32 aHwHandle);
  static int                    TakePicture(PRUint32 aHwHandle);
  static void                   CancelTakePicture(PRUint32 aHwHandle);
  static int                    StartPreview(PRUint32 aHwHandle);
  static void                   StopPreview(PRUint32 aHwHandle);
  static int                    PushParameters(PRUint32 aHwHandle, const CameraParameters& aParams);
  static void                   PullParameters(PRUint32 aHwHandle, CameraParameters& aParams);

  enum {
    PREVIEW_FORMAT_UNKNOWN,
    PREVIEW_FORMAT_YUV420P,
    PREVIEW_FORMAT_YUV420SP
  };
  
  static PRUint32               GetPreviewFormat(PRUint32 aHwHandle);

protected:
  static GonkCameraHardware*    sHw;
  static PRUint32               sHwHandle;

  static GonkCameraHardware*    GetHardware(PRUint32 aHwHandle)
  {
    if (aHwHandle == sHwHandle) {
      




      return sHw;
    }
    return nullptr;
  }

  
  void SetPreviewSize(PRUint32 aWidth, PRUint32 aHeight);
  int StartPreview();

  PRUint32                      mCamera;
  PRUint32                      mWidth;
  PRUint32                      mHeight;
  PRUint32                      mFps;
  PRUint32                      mPreviewFormat;
  bool                          mClosing;
  mozilla::ReentrantMonitor     mMonitor;
  PRUint32                      mNumFrames;
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
