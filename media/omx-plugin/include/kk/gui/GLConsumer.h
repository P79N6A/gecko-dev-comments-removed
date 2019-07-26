















#ifndef ANDROID_GUI_CONSUMER_H
#define ANDROID_GUI_CONSUMER_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <gui/IGraphicBufferProducer.h>
#include <gui/BufferQueue.h>
#include <gui/ConsumerBase.h>

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#define ANDROID_GRAPHICS_SURFACETEXTURE_JNI_ID "mSurfaceTexture"
#define ANDROID_GRAPHICS_FRAMEAVAILABLELISTENER_JNI_ID \
                                         "mFrameAvailableListener"

namespace android {



class String8;
















class GLConsumer : public ConsumerBase {
public:
    typedef ConsumerBase::FrameAvailableListener FrameAvailableListener;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    GLConsumer(GLuint tex, bool allowSynchronousMode = true,
            GLenum texTarget = GL_TEXTURE_EXTERNAL_OES, bool useFenceSync = true,
            const sp<BufferQueue> &bufferQueue = 0);

    virtual ~GLConsumer();

    
    
    
    
    
    
    
    status_t updateTexImage();

    
    
    
    
    
    void setReleaseFence(const sp<Fence>& fence);

    
    
    
    status_t setDefaultMaxBufferCount(int bufferCount);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    void getTransformMatrix(float mtx[16]);

    
    
    
    
    
    
    int64_t getTimestamp();

    
    
    
    
    
    
    
    
    status_t setDefaultBufferSize(uint32_t width, uint32_t height);

    
    
    void setFilteringEnabled(bool enabled);

    
    sp<GraphicBuffer> getCurrentBuffer() const;

    
    
    GLenum getCurrentTextureTarget() const;

    
    Rect getCurrentCrop() const;

    
    uint32_t getCurrentTransform() const;

    
    uint32_t getCurrentScalingMode() const;

    
    
    sp<Fence> getCurrentFence() const;

    
    
    
    status_t doGLFenceWait() const;

    
    
    bool isSynchronousMode() const;

    
    
    void setName(const String8& name);

    
    bool getTrickMode() const;

    
    uint32_t getVideoSessionID() const;

    
    
    status_t setDefaultBufferFormat(uint32_t defaultFormat);
    status_t setConsumerUsageBits(uint32_t usage);
    status_t setTransformHint(uint32_t hint);
    virtual status_t setSynchronousMode(bool enabled);

    
    
    sp<BufferQueue> getBufferQueue() const {
        return mBufferQueue;
    }

    
    
    
    
    
    
    
    
    
    
    status_t detachFromContext();

    
    
    
    
    
    
    
    
    
    
    
    
    
    status_t attachToContext(GLuint tex);

protected:

    
    
    virtual void abandonLocked();

    
    
    virtual void dumpLocked(String8& result, const char* prefix, char* buffer,
           size_t size) const;

    
    
    virtual status_t acquireBufferLocked(BufferQueue::BufferItem *item);

    
    
    virtual status_t releaseBufferLocked(int buf, EGLDisplay display,
           EGLSyncKHR eglFence);

    status_t releaseBufferLocked(int buf, EGLSyncKHR eglFence) {
        return releaseBufferLocked(buf, mEglDisplay, eglFence);
    }

    static bool isExternalFormat(uint32_t format);

    
    
    
    status_t releaseAndUpdateLocked(const BufferQueue::BufferItem& item);

    
    
    
    status_t bindTextureImageLocked();

    
    
    
    
    status_t checkAndUpdateEglStateLocked();

private:
    
    EGLImageKHR createImage(EGLDisplay dpy,
            const sp<GraphicBuffer>& graphicBuffer);

    
    
    
    
    
    virtual void freeBufferLocked(int slotIndex);

    
    
    
    
    void computeCurrentTransformMatrixLocked();

    
    
    
    status_t doGLFenceWaitLocked() const;

    
    
    
    
    status_t syncForReleaseLocked(EGLDisplay dpy);

    
    
    
    
    
    status_t bindUnslottedBufferLocked(EGLDisplay dpy);

    
    
    
    
    static const uint32_t DEFAULT_USAGE_FLAGS = GraphicBuffer::USAGE_HW_TEXTURE;

    
    
    
    sp<GraphicBuffer> mCurrentTextureBuf;

    
    
    Rect mCurrentCrop;

    
    
    uint32_t mCurrentTransform;

    
    
    uint32_t mCurrentScalingMode;

    
    sp<Fence> mCurrentFence;

    
    
    
    float mCurrentTransformMatrix[16];

    
    
    int64_t mCurrentTimestamp;

    uint32_t mDefaultWidth, mDefaultHeight;

    
    
    
    bool mFilteringEnabled;

    
    
    
    GLuint mTexName;

    
    
    
    
    const bool mUseFenceSync;

    
    
    
    
    
    
    
    const GLenum mTexTarget;

    
    
    struct EglSlot {
        EglSlot()
        : mEglImage(EGL_NO_IMAGE_KHR),
          mEglFence(EGL_NO_SYNC_KHR) {
        }

        
        EGLImageKHR mEglImage;

        
        
        
        
        EGLSyncKHR mEglFence;
    };

    
    
    
    
    EGLDisplay mEglDisplay;

    
    
    
    
    EGLContext mEglContext;

    
    
    
    
    
    
    
    EglSlot mEglSlots[BufferQueue::NUM_BUFFER_SLOTS];

    
    
    
    
    
    
    int mCurrentTexture;

    
    
    
    
    
    
    bool mAttached;

    
    
    bool mTrickMode;

    
    uint32_t mVideoSessionID;
};


}; 

#endif 
