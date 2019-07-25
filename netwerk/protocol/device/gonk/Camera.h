















#ifndef ANDROID_HARDWARE_CAMERA_H
#define ANDROID_HARDWARE_CAMERA_H

#include <utils/Timers.h>
#include "ICameraClient.h"

namespace android {

class ISurface;
































#define FRAME_CALLBACK_FLAG_ENABLE_MASK              0x01
#define FRAME_CALLBACK_FLAG_ONE_SHOT_MASK            0x02
#define FRAME_CALLBACK_FLAG_COPY_OUT_MASK            0x04


#define FRAME_CALLBACK_FLAG_NOOP                     0x00
#define FRAME_CALLBACK_FLAG_CAMCORDER                0x01
#define FRAME_CALLBACK_FLAG_CAMERA                   0x05
#define FRAME_CALLBACK_FLAG_BARCODE_SCANNER          0x07


enum {
    CAMERA_MSG_ERROR            = 0x001,
    CAMERA_MSG_SHUTTER          = 0x002,
    CAMERA_MSG_FOCUS            = 0x004,
    CAMERA_MSG_ZOOM             = 0x008,
    CAMERA_MSG_PREVIEW_FRAME    = 0x010,
    CAMERA_MSG_VIDEO_FRAME      = 0x020,
    CAMERA_MSG_POSTVIEW_FRAME   = 0x040,
    CAMERA_MSG_RAW_IMAGE        = 0x080,
    CAMERA_MSG_COMPRESSED_IMAGE = 0x100,
    CAMERA_MSG_ALL_MSGS         = 0x1FF
};


enum {
    CAMERA_CMD_START_SMOOTH_ZOOM     = 1,
    CAMERA_CMD_STOP_SMOOTH_ZOOM      = 2,
    
    
    
    
    
    
    
    
    
    
    
    
    CAMERA_CMD_SET_DISPLAY_ORIENTATION = 3,
};


enum {
    CAMERA_ERROR_UKNOWN  = 1,
    CAMERA_ERROR_SERVER_DIED = 100
};

enum {
    CAMERA_FACING_BACK = 0, 
    CAMERA_FACING_FRONT = 1 
};

struct CameraInfo {

    



    int facing;

    











    int orientation;
};

class ICameraService;
class ICamera;
class Surface;
class Mutex;
class String8;


class CameraListener: virtual public RefBase
{
public:
    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2) = 0;
    virtual void postData(int32_t msgType, const sp<IMemory>& dataPtr) = 0;
    virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) = 0;
};

class Camera : public BnCameraClient, public IBinder::DeathRecipient
{
public:
            
    static  sp<Camera>  create(const sp<ICamera>& camera);
    static  int32_t     getNumberOfCameras();
    static  status_t    getCameraInfo(int cameraId,
                                      struct CameraInfo* cameraInfo);
    static  sp<Camera>  connect(int cameraId);
                        ~Camera();
            void        init();

            status_t    reconnect();
            void        disconnect();
            status_t    lock();
            status_t    unlock();

            status_t    getStatus() { return mStatus; }

            
            status_t    setPreviewDisplay(const sp<Surface>& surface);
            status_t    setPreviewDisplay(const sp<ISurface>& surface);

            
            status_t    startPreview();

            
            void        stopPreview();

            
            bool        previewEnabled();

            
            status_t    startRecording();

            
            void        stopRecording();

            
            bool        recordingEnabled();

            
            void        releaseRecordingFrame(const sp<IMemory>& mem);

            
            status_t    autoFocus();

            
            status_t    cancelAutoFocus();

            
            status_t    takePicture();

            
            status_t    setParameters(const String8& params);

            
            String8     getParameters() const;

            
            status_t    sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);

            void        setListener(const sp<CameraListener>& listener);
            void        setPreviewCallbackFlags(int preview_callback_flag);

    
    virtual void        notifyCallback(int32_t msgType, int32_t ext, int32_t ext2);
    virtual void        dataCallback(int32_t msgType, const sp<IMemory>& dataPtr);
    virtual void        dataCallbackTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);

    sp<ICamera>         remote();

private:
                        Camera();
                        Camera(const Camera&);
                        Camera& operator=(const Camera);
                        virtual void binderDied(const wp<IBinder>& who);

            class DeathNotifier: public IBinder::DeathRecipient
            {
            public:
                DeathNotifier() {
                }

                virtual void binderDied(const wp<IBinder>& who);
            };

            static sp<DeathNotifier> mDeathNotifier;

            
            static const sp<ICameraService>& getCameraService();

            sp<ICamera>         mCamera;
            status_t            mStatus;

            sp<CameraListener>  mListener;

            friend class DeathNotifier;

            static  Mutex               mLock;
            static  sp<ICameraService>  mCameraService;

};

}; 

#endif
