















#include "base/basictypes.h"
#include "nsDebug.h"
#include "GonkCameraHwMgr.h"
#include "GonkNativeWindow.h"
#include "CameraCommon.h"
#include <sys/system_properties.h>

using namespace mozilla;
using namespace mozilla::layers;
using namespace android;

#if GIHM_TIMING_RECEIVEFRAME
#define INCLUDE_TIME_H                  1
#endif
#if GIHM_TIMING_OVERALL
#define INCLUDE_TIME_H                  1
#endif

#if INCLUDE_TIME_H
#include <time.h>

static __inline void timespecSubtract(struct timespec* a, struct timespec* b)
{
  
  if (b->tv_nsec < a->tv_nsec) {
    b->tv_nsec += 1000000000;
    b->tv_sec -= 1;
  }
  a->tv_nsec = b->tv_nsec - a->tv_nsec;
  a->tv_sec = b->tv_sec - a->tv_sec;
}
#endif

GonkCameraHardware::GonkCameraHardware(GonkCamera* aTarget, uint32_t aCamera)
  : mCamera(aCamera)
  , mClosing(false)
  , mMonitor("GonkCameraHardware.Monitor")
  , mNumFrames(0)
  , mTarget(aTarget)
  , mInitialized(false)
{
  DOM_CAMERA_LOGT( "%s:%d : this=%p (aTarget=%p)\n", __func__, __LINE__, (void*)this, (void*)aTarget );
  Init();
}

void
GonkCameraHardware::OnNewFrame()
{
  if (mClosing) {
    return;
  }
  GonkNativeWindow* window = static_cast<GonkNativeWindow*>(mWindow.get());
  nsRefPtr<GraphicBufferLocked> buffer = window->getCurrentBuffer();
  ReceiveFrame(mTarget, buffer);
}


void
GonkCameraHardware::DataCallback(int32_t aMsgType, const sp<IMemory> &aDataPtr, camera_frame_metadata_t* aMetadata, void* aUser)
{
  GonkCameraHardware* hw = GetHardware((uint32_t)aUser);
  if (!hw) {
    DOM_CAMERA_LOGW("%s:aUser = %d resolved to no camera hw\n", __func__, (uint32_t)aUser);
    return;
  }
  if (hw->mClosing) {
    return;
  }

  GonkCamera* camera = hw->mTarget;
  if (camera) {
    switch (aMsgType) {
      case CAMERA_MSG_PREVIEW_FRAME:
        
        break;

      case CAMERA_MSG_COMPRESSED_IMAGE:
        ReceiveImage(camera, (uint8_t*)aDataPtr->pointer(), aDataPtr->size());
        break;

      default:
        DOM_CAMERA_LOGE("Unhandled data callback event %d\n", aMsgType);
        break;
    }
  } else {
    DOM_CAMERA_LOGW("%s: hw = %p (camera = NULL)\n", __func__, hw);
  }
}


void
GonkCameraHardware::NotifyCallback(int32_t aMsgType, int32_t ext1, int32_t ext2, void* aUser)
{
  bool bSuccess;
  GonkCameraHardware* hw = GetHardware((uint32_t)aUser);
  if (!hw) {
    DOM_CAMERA_LOGW("%s:aUser = %d resolved to no camera hw\n", __func__, (uint32_t)aUser);
    return;
  }
  if (hw->mClosing) {
    return;
  }

  GonkCamera* camera = hw->mTarget;
  if (!camera) {
    return;
  }

  switch (aMsgType) {
    case CAMERA_MSG_FOCUS:
      if (ext1) {
        DOM_CAMERA_LOGI("Autofocus complete");
        bSuccess = true;
      } else {
        DOM_CAMERA_LOGW("Autofocus failed");
        bSuccess = false;
      }
      AutoFocusComplete(camera, bSuccess);
      break;

    case CAMERA_MSG_SHUTTER:
      OnShutter(camera);
      break;

    default:
      DOM_CAMERA_LOGE("Unhandled notify callback event %d\n", aMsgType);
      break;
  }
}

void
GonkCameraHardware::DataCallbackTimestamp(nsecs_t aTimestamp, int32_t aMsgType, const sp<IMemory> &aDataPtr, void* aUser)
{
  DOM_CAMERA_LOGI("%s",__func__);
  GonkCameraHardware* hw = GetHardware((uint32_t)aUser);
  if (!hw) {
    DOM_CAMERA_LOGE("%s:aUser = %d resolved to no camera hw\n", __func__, (uint32_t)aUser);
    return;
  }
  if (hw->mClosing) {
    return;
  }

  sp<GonkCameraListener> listener;
  {
    
    
    listener = hw->mListener;
  }
  if (listener.get()) {
    DOM_CAMERA_LOGI("Listener registered, posting recording frame!");
    listener->postDataTimestamp(aTimestamp, aMsgType, aDataPtr);
  } else {
    DOM_CAMERA_LOGW("No listener was set. Drop a recording frame.");
    hw->mHardware->releaseRecordingFrame(aDataPtr);
  }
}

void
GonkCameraHardware::Init()
{
  DOM_CAMERA_LOGT("%s: this=%p\n", __func__, (void* )this);

  if (hw_get_module(CAMERA_HARDWARE_MODULE_ID, (const hw_module_t**)&mModule) < 0) {
    return;
  }
  char cameraDeviceName[4];
  snprintf(cameraDeviceName, sizeof(cameraDeviceName), "%d", mCamera);
  mHardware = new CameraHardwareInterface(cameraDeviceName);
  if (mHardware->initialize(&mModule->common) != OK) {
    mHardware.clear();
    return;
  }

  struct camera_info info;
  int rv = mModule->get_camera_info(mCamera, &info);
  if (rv != 0) {
    return;
  }
  mSensorOrientation = info.orientation;

  
  
  char propname[PROP_NAME_MAX];
  char prop[PROP_VALUE_MAX];
  snprintf(propname, sizeof(propname), "ro.moz.cam.%d.sensor_offset", mCamera);
  if (__system_property_get(propname, prop) > 0) {
    int offset = clamped(atoi(prop), 0, 270);
    mSensorOrientation += offset;
    mSensorOrientation %= 360;
  }

  if (sHwHandle == 0) {
    sHwHandle = 1;  
  }
  mHardware->setCallbacks(GonkCameraHardware::NotifyCallback, GonkCameraHardware::DataCallback, GonkCameraHardware::DataCallbackTimestamp, (void*)sHwHandle);
  mInitialized = true;
}

GonkCameraHardware::~GonkCameraHardware()
{
  DOM_CAMERA_LOGT( "%s:%d : this=%p\n", __func__, __LINE__, (void*)this );
  sHw = nullptr;

  



  if (mTarget) {
    OnClosed(mTarget);
  }
}

GonkCameraHardware* GonkCameraHardware::sHw         = nullptr;
uint32_t            GonkCameraHardware::sHwHandle   = 0;

void
GonkCameraHardware::ReleaseHandle(uint32_t aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  DOM_CAMERA_LOGI("%s: aHwHandle = %d, hw = %p (sHwHandle = %d)\n", __func__, aHwHandle, (void*)hw, sHwHandle);
  if (!hw) {
    return;
  }

  DOM_CAMERA_LOGT("%s: before: sHwHandle = %d\n", __func__, sHwHandle);
  sHwHandle += 1; 
  hw->mClosing = true;
  hw->mHardware->disableMsgType(CAMERA_MSG_ALL_MSGS);
  hw->mHardware->stopPreview();
  hw->mHardware->release();
  GonkNativeWindow* window = static_cast<GonkNativeWindow*>(hw->mWindow.get());
  if (window) {
    window->abandon();
  }
  DOM_CAMERA_LOGT("%s: after: sHwHandle = %d\n", __func__, sHwHandle);
  delete hw;     
}

uint32_t
GonkCameraHardware::GetHandle(GonkCamera* aTarget, uint32_t aCamera)
{
  ReleaseHandle(sHwHandle);

  sHw = new GonkCameraHardware(aTarget, aCamera);

  if (sHw->IsInitialized()) {
    return sHwHandle;
  }

  DOM_CAMERA_LOGE("failed to initialize camera hardware\n");
  delete sHw;
  sHw = nullptr;
  return 0;
}

int
GonkCameraHardware::GetSensorOrientation(uint32_t aHwHandle)
{
  DOM_CAMERA_LOGI("%s: aHwHandle = %d\n", __func__, aHwHandle);
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return 0;
  }
  return hw->mSensorOrientation;
}

int
GonkCameraHardware::AutoFocus(uint32_t aHwHandle)
{
  DOM_CAMERA_LOGI("%s: aHwHandle = %d\n", __func__, aHwHandle);
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  hw->mHardware->enableMsgType(CAMERA_MSG_FOCUS);
  return hw->mHardware->autoFocus();
}

void
GonkCameraHardware::CancelAutoFocus(uint32_t aHwHandle)
{
  DOM_CAMERA_LOGI("%s: aHwHandle = %d\n", __func__, aHwHandle);
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (hw) {
    hw->mHardware->cancelAutoFocus();
  }
}

int
GonkCameraHardware::TakePicture(uint32_t aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  hw->mHardware->enableMsgType(CAMERA_MSG_COMPRESSED_IMAGE);
  return hw->mHardware->takePicture();
}

void
GonkCameraHardware::CancelTakePicture(uint32_t aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (hw) {
    hw->mHardware->cancelPicture();
  }
}

int
GonkCameraHardware::PushParameters(uint32_t aHwHandle, const CameraParameters& aParams)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  return hw->mHardware->setParameters(aParams);
}

void
GonkCameraHardware::PullParameters(uint32_t aHwHandle, CameraParameters& aParams)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (hw) {
    aParams = hw->mHardware->getParameters();
  }
}

int
GonkCameraHardware::StartPreview()
{
  if (mWindow.get()) {
    GonkNativeWindow* window = static_cast<GonkNativeWindow*>(mWindow.get());
    window->abandon();
  } else {
    mWindow = new GonkNativeWindow(this);
    mHardware->setPreviewWindow(mWindow);
  }

  mHardware->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);
  return mHardware->startPreview();
}

int
GonkCameraHardware::StartPreview(uint32_t aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  DOM_CAMERA_LOGI("%s:%d : aHwHandle = %d, hw = %p\n", __func__, __LINE__, aHwHandle, hw);
  if (!hw) {
    return DEAD_OBJECT;
  }

  return hw->StartPreview();
}

void
GonkCameraHardware::StopPreview(uint32_t aHwHandle)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (hw) {
    hw->mHardware->stopPreview();
  }
}

int
GonkCameraHardware::StartRecording(uint32_t aHwHandle)
{
  DOM_CAMERA_LOGI("%s: aHwHandle = %d\n", __func__, aHwHandle);
  int rv = OK;
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  if (hw->mHardware->recordingEnabled()) {
    return OK;
  }

  if (!hw->mHardware->previewEnabled()) {
    DOM_CAMERA_LOGW("Preview was not enabled, enabling now!\n");
    rv = StartPreview(aHwHandle);
    if (rv != OK) {
      return rv;
    }
  }

  
  hw->mHardware->enableMsgType(CAMERA_MSG_VIDEO_FRAME);
  DOM_CAMERA_LOGI("Calling hw->startRecording\n");
  rv = hw->mHardware->startRecording();
  if (rv != OK) {
    DOM_CAMERA_LOGE("mHardware->startRecording() failed with status %d", rv);
  }
  return rv;
}

int
GonkCameraHardware::StopRecording(uint32_t aHwHandle)
{
  DOM_CAMERA_LOGI("%s: aHwHandle = %d\n", __func__, aHwHandle);
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  hw->mHardware->disableMsgType(CAMERA_MSG_VIDEO_FRAME);
  hw->mHardware->stopRecording();
  return OK;
}

int
GonkCameraHardware::SetListener(uint32_t aHwHandle, const sp<GonkCameraListener>& aListener)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  hw->mListener = aListener;
  return OK;
}

void
GonkCameraHardware::ReleaseRecordingFrame(uint32_t aHwHandle, const sp<IMemory>& aFrame)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (hw) {
    hw->mHardware->releaseRecordingFrame(aFrame);
  }
}

int
GonkCameraHardware::StoreMetaDataInBuffers(uint32_t aHwHandle, bool aEnabled)
{
  GonkCameraHardware* hw = GetHardware(aHwHandle);
  if (!hw) {
    return DEAD_OBJECT;
  }

  return hw->mHardware->storeMetaDataInBuffers(aEnabled);
}
