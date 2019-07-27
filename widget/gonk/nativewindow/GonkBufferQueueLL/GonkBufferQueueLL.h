
















#ifndef NATIVEWINDOW_GONKBUFFERQUEUE_LL_H
#define NATIVEWINDOW_GONKBUFFERQUEUE_LL_H

#include "GonkBufferQueueDefs.h"
#include "IGonkGraphicBufferConsumerLL.h"
#include <gui/IGraphicBufferProducer.h>
#include <gui/IConsumerListener.h>



#include <gui/IGraphicBufferAlloc.h>

namespace android {

class GonkBufferQueue {
public:
    
    
    enum { NUM_BUFFER_SLOTS = GonkBufferQueueDefs::NUM_BUFFER_SLOTS };
    
    enum { INVALID_BUFFER_SLOT = IGonkGraphicBufferConsumer::BufferItem::INVALID_BUFFER_SLOT };
    
    enum {
        NO_BUFFER_AVAILABLE = IGonkGraphicBufferConsumer::NO_BUFFER_AVAILABLE,
        PRESENT_LATER = IGonkGraphicBufferConsumer::PRESENT_LATER,
    };

    
    
    enum { MAX_MAX_ACQUIRED_BUFFERS = NUM_BUFFER_SLOTS - 2 };

    
    typedef ::android::ConsumerListener ConsumerListener;
    typedef IGonkGraphicBufferConsumer::BufferItem BufferItem;

    
    
    
    
    
    
    
    
    
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
            sp<IGonkGraphicBufferConsumer>* outConsumer,
            const sp<IGraphicBufferAlloc>& allocator = NULL);

private:
    GonkBufferQueue(); 
};


}; 

#endif
