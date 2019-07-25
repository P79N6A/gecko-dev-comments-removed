



































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
        virtual void ReceiveFrame(char* frame, PRUint32 length) = 0;
    };
    
    


    static CameraStreamImpl* GetInstance(PRUint32 aCamera);
    
    bool initNeeded() {
        return !mInit;
    }
    
    FrameCallback* GetFrameCallback() {
        return mCallback;
    }
    
    bool Init(const nsCString& contentType, const PRUint32& camera, const PRUint32& width, const PRUint32& height, FrameCallback* callback);
    void Close();
    
    PRUint32 GetWidth() { return mWidth; }
    PRUint32 GetHeight() { return mHeight; }
    PRUint32 GetFps() { return mFps; }
    
    void takePicture(const nsAString& aFileName);
    
    void transmitFrame(JNIEnv *env, jbyteArray *data);
    
private:
    CameraStreamImpl(PRUint32 aCamera);
    CameraStreamImpl(const CameraStreamImpl&);
    CameraStreamImpl& operator=(const CameraStreamImpl&);

    ~CameraStreamImpl();

    bool mInit;
    PRUint32 mCamera;
    PRUint32 mWidth;
    PRUint32 mHeight;
    PRUint32 mFps;
    FrameCallback* mCallback;
};

} 
} 

#endif
