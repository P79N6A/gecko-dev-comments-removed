















#define LOG_TAG "BufferQueue"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS


#include <gui/BufferQueue.h>
#include <gui/BufferQueueConsumer.h>
#include <gui/BufferQueueCore.h>
#include <gui/BufferQueueProducer.h>

namespace android {

BufferQueue::ProxyConsumerListener::ProxyConsumerListener(
        const wp<ConsumerListener>& consumerListener):
        mConsumerListener(consumerListener) {}

BufferQueue::ProxyConsumerListener::~ProxyConsumerListener() {}

void BufferQueue::ProxyConsumerListener::onFrameAvailable() {
    sp<ConsumerListener> listener(mConsumerListener.promote());
    if (listener != NULL) {
        listener->onFrameAvailable();
    }
}

void BufferQueue::ProxyConsumerListener::onBuffersReleased() {
    sp<ConsumerListener> listener(mConsumerListener.promote());
    if (listener != NULL) {
        listener->onBuffersReleased();
    }
}

void BufferQueue::ProxyConsumerListener::onSidebandStreamChanged() {
    sp<ConsumerListener> listener(mConsumerListener.promote());
    if (listener != NULL) {
        listener->onSidebandStreamChanged();
    }
}

void BufferQueue::createBufferQueue(sp<IGraphicBufferProducer>* outProducer,
        sp<IGraphicBufferConsumer>* outConsumer,
        const sp<IGraphicBufferAlloc>& allocator) {
    LOG_ALWAYS_FATAL_IF(outProducer == NULL,
            "BufferQueue: outProducer must not be NULL");
    LOG_ALWAYS_FATAL_IF(outConsumer == NULL,
            "BufferQueue: outConsumer must not be NULL");

    sp<BufferQueueCore> core(new BufferQueueCore(allocator));
    LOG_ALWAYS_FATAL_IF(core == NULL,
            "BufferQueue: failed to create BufferQueueCore");

    sp<IGraphicBufferProducer> producer(new BufferQueueProducer(core));
    LOG_ALWAYS_FATAL_IF(producer == NULL,
            "BufferQueue: failed to create BufferQueueProducer");

    sp<IGraphicBufferConsumer> consumer(new BufferQueueConsumer(core));
    LOG_ALWAYS_FATAL_IF(consumer == NULL,
            "BufferQueue: failed to create BufferQueueConsumer");

    *outProducer = producer;
    *outConsumer = consumer;
}

}; 
