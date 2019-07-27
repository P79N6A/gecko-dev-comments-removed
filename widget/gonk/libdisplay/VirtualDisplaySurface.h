















#ifndef ANDROID_SF_VIRTUAL_DISPLAY_SURFACE_H
#define ANDROID_SF_VIRTUAL_DISPLAY_SURFACE_H

#include <gui/IGraphicBufferProducer.h>

#include "DisplaySurface.h"


namespace android {


class HWComposer;
class IProducerListener;








































class VirtualDisplaySurface : public DisplaySurface,
                              public BnGraphicBufferProducer {
public:
    VirtualDisplaySurface(int32_t dispId,
            const sp<IGraphicBufferProducer>& sink,
            const sp<IGraphicBufferProducer>& bqProducer,
            const sp<StreamConsumer>& bqConsumer,
            const String8& name);

    
    
    
    virtual status_t beginFrame(bool mustRecompose);
    virtual status_t prepareFrame(CompositionType compositionType);
    virtual status_t compositionComplete();
    virtual status_t advanceFrame();
    virtual void onFrameCommitted();
    virtual void dump(String8& result) const;
    virtual void resizeBuffers(const uint32_t w, const uint32_t h);

    virtual status_t setReleaseFenceFd(int fenceFd) { return INVALID_OPERATION; }
    virtual int GetPrevDispAcquireFd() { return -1; };

private:
    enum Source {SOURCE_SINK = 0, SOURCE_SCRATCH = 1};

    virtual ~VirtualDisplaySurface();

    
    
    
    virtual status_t requestBuffer(int pslot, sp<GraphicBuffer>* outBuf);
    virtual status_t setBufferCount(int bufferCount);
    virtual status_t dequeueBuffer(int* pslot, sp<Fence>* fence, bool async,
            uint32_t w, uint32_t h, uint32_t format, uint32_t usage);
    virtual status_t detachBuffer(int slot);
    virtual status_t detachNextBuffer(sp<GraphicBuffer>* outBuffer,
            sp<Fence>* outFence);
    virtual status_t attachBuffer(int* slot, const sp<GraphicBuffer>& buffer);
    virtual status_t queueBuffer(int pslot,
            const QueueBufferInput& input, QueueBufferOutput* output);
    virtual void cancelBuffer(int pslot, const sp<Fence>& fence);
    virtual int query(int what, int* value);
#if ANDROID_VERSION >= 21
    virtual status_t connect(const sp<IProducerListener>& listener,
            int api, bool producerControlledByApp, QueueBufferOutput* output);
#else
    virtual status_t connect(const sp<IBinder>& token,
            int api, bool producerControlledByApp, QueueBufferOutput* output);
#endif
    virtual status_t disconnect(int api);
#if ANDROID_VERSION >= 21
    virtual status_t setSidebandStream(const sp<NativeHandle>& stream);
#endif
    virtual void allocateBuffers(bool async, uint32_t width, uint32_t height,
            uint32_t format, uint32_t usage);

    
    
    
    static Source fbSourceForCompositionType(CompositionType type);
    status_t dequeueBuffer(Source source, uint32_t format, uint32_t usage,
            int* sslot, sp<Fence>* fence);
    void updateQueueBufferOutput(const QueueBufferOutput& qbo);
    void resetPerFrameState();
    status_t refreshOutputBuffer();

    
    
    
    
    
    
    static int mapSource2ProducerSlot(Source source, int sslot);
    static int mapProducer2SourceSlot(Source source, int pslot);

    
    
    
    const int32_t mDisplayId;
    const String8 mDisplayName;
    sp<IGraphicBufferProducer> mSource[2]; 
    uint32_t mDefaultOutputFormat;

    
    
    

    
    
    
    
    uint32_t mOutputFormat;
    uint32_t mOutputUsage;

    
    
    
    
    
    
    uint64_t mProducerSlotSource;
    sp<GraphicBuffer> mProducerBuffers[BufferQueue::NUM_BUFFER_SLOTS];

    
    
    
    QueueBufferOutput mQueueBufferOutput;

    
    
    uint32_t mSinkBufferWidth, mSinkBufferHeight;

    
    
    

    
    
    CompositionType mCompositionType;

    
    
    sp<Fence> mFbFence;

    
    
    sp<Fence> mOutputFence;

    
    
    int mFbProducerSlot;
    int mOutputProducerSlot;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    enum DbgState {
        
        DBG_STATE_IDLE,
        
        DBG_STATE_BEGUN,
        
        
        DBG_STATE_PREPARED,
        
        DBG_STATE_GLES,
        
        DBG_STATE_GLES_DONE,
        
        DBG_STATE_HWC,
    };
    DbgState mDbgState;
    CompositionType mDbgLastCompositionType;

    const char* dbgStateStr() const;
    static const char* dbgSourceStr(Source s);

    bool mMustRecompose;
};


} 


#endif

