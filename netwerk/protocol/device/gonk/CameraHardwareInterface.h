















#ifndef ANDROID_HARDWARE_CAMERA_HARDWARE_INTERFACE_H
#define ANDROID_HARDWARE_CAMERA_HARDWARE_INTERFACE_H

#include <binder/IMemory.h>
#include <utils/RefBase.h>
#include <surfaceflinger/ISurface.h>
#include "Camera.h"
#include "CameraParameters.h"

namespace android {

class Overlay;




typedef struct image_rect_struct
{
  uint32_t width;      
  uint32_t height;     
} image_rect_type;


typedef void (*notify_callback)(int32_t msgType,
                                int32_t ext1,
                                int32_t ext2,
                                void* user);

typedef void (*data_callback)(int32_t msgType,
                              const sp<IMemory>& dataPtr,
                              void* user);

typedef void (*data_callback_timestamp)(nsecs_t timestamp,
                                        int32_t msgType,
                                        const sp<IMemory>& dataPtr,
                                        void* user);
































class CameraHardwareInterface : public virtual RefBase {
public:
    virtual ~CameraHardwareInterface() { }

    
    virtual sp<IMemoryHeap>         getPreviewHeap() const = 0;

    
    virtual sp<IMemoryHeap>         getRawHeap() const = 0;

    
    virtual void setCallbacks(notify_callback notify_cb,
                              data_callback data_cb,
                              data_callback_timestamp data_cb_timestamp,
                              void* user) = 0;

    





    


    virtual void        enableMsgType(int32_t msgType) = 0;

    


    virtual void        disableMsgType(int32_t msgType) = 0;

    




    virtual bool        msgTypeEnabled(int32_t msgType) = 0;

    


    virtual status_t    startPreview() = 0;

#ifdef USE_MAGURO_LIBCAMERA
    




    virtual status_t    getBufferInfo(sp<IMemory>& Frame, size_t *alignedSize) = 0;

    


    virtual void        encodeData() = 0;
#endif

    


    virtual bool         useOverlay() {return false;}
    virtual status_t     setOverlay(const sp<Overlay> &overlay) {return BAD_VALUE;}

#ifdef USE_GS2_LIBCAMERA
    


    virtual void        something() {}
#endif

    


    virtual void        stopPreview() = 0;

    


    virtual bool        previewEnabled() = 0;

    




    virtual status_t    startRecording() = 0;

    


    virtual void        stopRecording() = 0;

    


    virtual bool        recordingEnabled() = 0;

    


    virtual void        releaseRecordingFrame(const sp<IMemory>& mem) = 0;

    




    virtual status_t    autoFocus() = 0;

    





    virtual status_t    cancelAutoFocus() = 0;

    


    virtual status_t    takePicture() = 0;

    



    virtual status_t    cancelPicture() = 0;

    


    virtual status_t    setParameters(const CameraParameters& params) = 0;

    
    virtual CameraParameters  getParameters() const = 0;

    


    virtual status_t sendCommand(int32_t cmd, int32_t arg1, int32_t arg2) = 0;

    



    virtual void release() = 0;

    


    virtual status_t dump(int fd, const Vector<String16>& args) const = 0;

#ifdef USE_MAGURO_LIBCAMERA
    


    virtual status_t    takeLiveSnapshot() = 0;
#endif

};







extern "C" int HAL_getNumberOfCameras();
extern "C" void HAL_getCameraInfo(int cameraId, struct CameraInfo* cameraInfo);
#ifdef USE_MAGURO_LIBCAMERA

extern "C" sp<CameraHardwareInterface> HAL_openCameraHardware(int cameraId, int mode);

extern "C" int HAL_isIn3DMode();
#else

extern "C" sp<CameraHardwareInterface> HAL_openCameraHardware(int cameraId);
#endif

};  

#endif
