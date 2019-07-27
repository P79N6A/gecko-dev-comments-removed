
















#define LOG_TAG "GonkBufferQueue"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#define LOG_NDEBUG 0

#include "GonkBufferQueue.h"
#include "GonkBufferQueueConsumer.h"
#include "GonkBufferQueueCore.h"
#include "GonkBufferQueueProducer.h"

namespace android {

GonkBufferQueue::ProxyConsumerListener::ProxyConsumerListener(
        const wp<ConsumerListener>& consumerListener):
        mConsumerListener(consumerListener) {}

GonkBufferQueue::ProxyConsumerListener::~ProxyConsumerListener() {}

void GonkBufferQueue::ProxyConsumerListener::onFrameAvailable() {
    sp<ConsumerListener> listener(mConsumerListener.promote());
    if (listener != NULL) {
        listener->onFrameAvailable();
    }
}

void GonkBufferQueue::ProxyConsumerListener::onBuffersReleased() {
    sp<ConsumerListener> listener(mConsumerListener.promote());
    if (listener != NULL) {
        listener->onBuffersReleased();
    }
}

void GonkBufferQueue::ProxyConsumerListener::onSidebandStreamChanged() {
    sp<ConsumerListener> listener(mConsumerListener.promote());
    if (listener != NULL) {
        listener->onSidebandStreamChanged();
    }
}

void GonkBufferQueue::createBufferQueue(sp<IGraphicBufferProducer>* outProducer,
        sp<IGonkGraphicBufferConsumer>* outConsumer,
        const sp<IGraphicBufferAlloc>& allocator) {
    LOG_ALWAYS_FATAL_IF(outProducer == NULL,
            "GonkBufferQueue: outProducer must not be NULL");
    LOG_ALWAYS_FATAL_IF(outConsumer == NULL,
            "GonkBufferQueue: outConsumer must not be NULL");

    sp<GonkBufferQueueCore> core(new GonkBufferQueueCore(allocator));
    LOG_ALWAYS_FATAL_IF(core == NULL,
            "GonkBufferQueue: failed to create GonkBufferQueueCore");

    sp<IGraphicBufferProducer> producer(new GonkBufferQueueProducer(core));
    LOG_ALWAYS_FATAL_IF(producer == NULL,
            "GonkBufferQueue: failed to create GonkBufferQueueProducer");

    sp<IGonkGraphicBufferConsumer> consumer(new GonkBufferQueueConsumer(core));
    LOG_ALWAYS_FATAL_IF(consumer == NULL,
            "GonkBufferQueue: failed to create GonkBufferQueueConsumer");

    *outProducer = producer;
    *outConsumer = consumer;
}

}; 
