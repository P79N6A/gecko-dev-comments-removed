
















#ifndef DOM_CAMERA_GONKNATIVEWINDOW_H
#define DOM_CAMERA_GONKNATIVEWINDOW_H

#include <stdint.h>
#include <sys/types.h>

#include <ui/egl/android_natives.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>

#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>
#include <utils/String8.h>
#include <utils/threads.h>

namespace android {

class GonkNativeWindow : public EGLNativeBase<ANativeWindow, GonkNativeWindow, RefBase>
{
public:
    enum { MIN_UNDEQUEUED_BUFFERS = 2 };
    enum { MIN_BUFFER_SLOTS = MIN_UNDEQUEUED_BUFFERS };
    enum { NUM_BUFFER_SLOTS = 32 };

    GonkNativeWindow();
    ~GonkNativeWindow(); 

    
    static int hook_cancelBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int hook_dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer** buffer);
    static int hook_lockBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int hook_perform(ANativeWindow* window, int operation, ...);
    static int hook_query(const ANativeWindow* window, int what, int* value);
    static int hook_queueBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int hook_setSwapInterval(ANativeWindow* window, int interval);

protected:
    virtual int cancelBuffer(ANativeWindowBuffer* buffer);
    virtual int dequeueBuffer(ANativeWindowBuffer** buffer);
    virtual int lockBuffer(ANativeWindowBuffer* buffer);
    virtual int perform(int operation, va_list args);
    virtual int query(int what, int* value) const;
    virtual int queueBuffer(ANativeWindowBuffer* buffer);
    virtual int setSwapInterval(int interval);

    virtual int setBufferCount(int bufferCount);
    virtual int setBuffersDimensions(int w, int h);
    virtual int setBuffersFormat(int format);
    virtual int setBuffersTimestamp(int64_t timestamp);
    virtual int setUsage(uint32_t reqUsage);

    
    
    void freeBufferLocked(int index);

    
    
    void freeAllBuffersLocked();

private:
    void init();

    int dispatchSetBufferCount(va_list args);
    int dispatchSetBuffersGeometry(va_list args);
    int dispatchSetBuffersDimensions(va_list args);
    int dispatchSetBuffersFormat(va_list args);
    int dispatchSetBuffersTimestamp(va_list args);
    int dispatchSetUsage(va_list args);

    int getSlotFromBufferLocked(android_native_buffer_t* buffer) const;

private:
    enum { INVALID_BUFFER_SLOT = -1 };

    struct BufferSlot {

        BufferSlot()
            : mGraphicBuffer(0),
              mBufferState(BufferSlot::FREE),
              mTimestamp(0),
              mFrameNumber(0){
        }

        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        
        enum BufferState {
            
            
            
            FREE = 0,

            
            
            
            
            
            
            
            
            
            
            DEQUEUED = 1,

            
            
            
            
            
            
            
            QUEUED = 2,
        };

        
        BufferState mBufferState;

        
        
        int64_t mTimestamp;

        
        uint64_t mFrameNumber;
    };

    
    
    
    
    
    BufferSlot mSlots[NUM_BUFFER_SLOTS];

    
    mutable Condition mDequeueCondition;

    
    
    
    int64_t mTimestamp;

    
    
    uint32_t mDefaultWidth;

    
    
    uint32_t mDefaultHeight;

    
    
    uint32_t mPixelFormat;

    
    uint32_t mUsage;

    
    
    
    int mBufferCount;

    
    
    mutable Mutex mMutex;

    
    uint64_t mFrameCounter;
};

}; 

#endif
