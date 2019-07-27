















#ifndef ANDROID_GUI_BUFFERQUEUE_H
#define ANDROID_GUI_BUFFERQUEUE_H

#include <gui/BufferQueueDefs.h>
#include <gui/IGraphicBufferConsumer.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/IConsumerListener.h>



#include <gui/IGraphicBufferAlloc.h>

namespace android {

class BufferQueue {
public:
    
    
    enum { NUM_BUFFER_SLOTS = BufferQueueDefs::NUM_BUFFER_SLOTS };
    
    enum { INVALID_BUFFER_SLOT = IGraphicBufferConsumer::BufferItem::INVALID_BUFFER_SLOT };
    
    enum {
        NO_BUFFER_AVAILABLE = IGraphicBufferConsumer::NO_BUFFER_AVAILABLE,
        PRESENT_LATER = IGraphicBufferConsumer::PRESENT_LATER,
    };

    
    
    enum { MAX_MAX_ACQUIRED_BUFFERS = NUM_BUFFER_SLOTS - 2 };

    
    typedef ::android::ConsumerListener ConsumerListener;
    typedef IGraphicBufferConsumer::BufferItem BufferItem;

    
    
    
    
    
    
    
    
    
    class ProxyConsumerListener : public BnConsumerListener {
    public:
        ProxyConsumerListener(const wp<ConsumerListener>& consumerListener);
        virtual ~ProxyConsumerListener();
        virtual void onFrameAvailable();
        virtual void onBuffersReleased();
        virtual void onSidebandStreamChanged();
    private:
        
        
        wp<ConsumerListener> mConsumerListener;
    };

    
    
    
    static void createBufferQueue(sp<IGraphicBufferProducer>* outProducer,
            sp<IGraphicBufferConsumer>* outConsumer,
            const sp<IGraphicBufferAlloc>& allocator = NULL);

private:
    BufferQueue(); 
};


}; 

#endif
