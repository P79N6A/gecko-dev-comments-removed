
















#ifndef DOM_CAMERA_GONKNATIVEWINDOWCLIENT_H
#define DOM_CAMERA_GONKNATIVEWINDOWCLIENT_H

#include <ui/egl/android_natives.h>

#include "GonkNativeWindow.h"

namespace android {

class GonkNativeWindowClient : public EGLNativeBase<ANativeWindow, GonkNativeWindowClient, RefBase>
{
public:
    GonkNativeWindowClient(const sp<GonkNativeWindow>& window);
    ~GonkNativeWindowClient(); 

private:
    void init();

    
    static int hook_cancelBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int hook_dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer** buffer);
    static int hook_lockBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int hook_perform(ANativeWindow* window, int operation, ...);
    static int hook_query(const ANativeWindow* window, int what, int* value);
    static int hook_queueBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int hook_setSwapInterval(ANativeWindow* window, int interval);

    int dispatchConnect(va_list args);
    int dispatchDisconnect(va_list args);
    int dispatchSetBufferCount(va_list args);
    int dispatchSetBuffersGeometry(va_list args);
    int dispatchSetBuffersDimensions(va_list args);
    int dispatchSetBuffersFormat(va_list args);
    int dispatchSetBuffersTimestamp(va_list args);
    int dispatchSetUsage(va_list args);

protected:
    virtual int cancelBuffer(ANativeWindowBuffer* buffer);
    virtual int dequeueBuffer(ANativeWindowBuffer** buffer);
    virtual int lockBuffer(ANativeWindowBuffer* buffer);
    virtual int perform(int operation, va_list args);
    virtual int query(int what, int* value) const;
    virtual int queueBuffer(ANativeWindowBuffer* buffer);
    virtual int setSwapInterval(int interval);

    virtual int connect(int api);
    virtual int disconnect(int api);
    virtual int setBufferCount(int bufferCount);
    virtual int setBuffersDimensions(int w, int h);
    virtual int setBuffersFormat(int format);
    virtual int setBuffersTimestamp(int64_t timestamp);
    virtual int setUsage(uint32_t reqUsage);

    int getNumberOfArgsForOperation(int operation);

    enum { MIN_UNDEQUEUED_BUFFERS = GonkNativeWindow::MIN_UNDEQUEUED_BUFFERS };
    enum { NUM_BUFFER_SLOTS = GonkNativeWindow::NUM_BUFFER_SLOTS };
    enum { DEFAULT_FORMAT = PIXEL_FORMAT_RGBA_8888 };
    enum { NATIVE_WINDOW_SET_BUFFERS_SIZE = 0x10000000 };

private:
    void freeAllBuffers();
    int getSlotFromBufferLocked(android_native_buffer_t* buffer) const;

    sp<GonkNativeWindow> mNativeWindow;

    
    
    
    
    
    
    sp<GraphicBuffer> mSlots[NUM_BUFFER_SLOTS];

    
    
    uint32_t mReqWidth;

    
    
    uint32_t mReqHeight;

    
    
    uint32_t mReqFormat;

    
    
    uint32_t mReqUsage;

    
    
    
    int64_t mTimestamp;

    
    
    uint32_t mDefaultWidth;

    
    
    uint32_t mDefaultHeight;

    
    
    uint32_t mTransformHint;

    
    
    
    mutable Mutex mMutex;

    bool                        mConnectedToCpu;
};


}; 

#endif 
