



#ifndef __CAMERASTREAMIMPL_H__
#define __CAMERASTREAMIMPL_H__

#include "nsString.h"
#include "AndroidBridge.h"






namespace mozilla {
namespace net {
    
class CameraStreamImpl {
public:
    class FrameCallback {
    public:
        virtual void ReceiveFrame(char* frame, uint32_t length) = 0;
    };
    
    


    static CameraStreamImpl* GetInstance(uint32_t aCamera);
    
    bool initNeeded() {
        return !mInit;
    }
    
    FrameCallback* GetFrameCallback() {
        return mCallback;
    }
    
    bool Init(const nsCString& contentType, const uint32_t& camera, const uint32_t& width, const uint32_t& height, FrameCallback* callback);
    void Close();
    
    uint32_t GetWidth() { return mWidth; }
    uint32_t GetHeight() { return mHeight; }
    uint32_t GetFps() { return mFps; }
    
    void takePicture(const nsAString& aFileName);
    
    void transmitFrame(JNIEnv *env, jbyteArray *data);
    
private:
    CameraStreamImpl(uint32_t aCamera);
    CameraStreamImpl(const CameraStreamImpl&);
    CameraStreamImpl& operator=(const CameraStreamImpl&);

    ~CameraStreamImpl();

    bool mInit;
    uint32_t mCamera;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFps;
    FrameCallback* mCallback;
};

} 
} 

#endif
