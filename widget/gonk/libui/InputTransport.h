















#ifndef _UI_INPUT_TRANSPORT_H
#define _UI_INPUT_TRANSPORT_H














#include <semaphore.h>
#include "Input.h"
#include <utils/Errors.h>
#include "Timers.h"
#include <utils/RefBase.h>
#include "String8.h"

namespace android {










class InputChannel : public RefBase {
protected:
    virtual ~InputChannel();

public:
    InputChannel(const String8& name, int32_t ashmemFd, int32_t receivePipeFd,
            int32_t sendPipeFd);

    




    static status_t openInputChannelPair(const String8& name,
            sp<InputChannel>& outServerChannel, sp<InputChannel>& outClientChannel);

    inline String8 getName() const { return mName; }
    inline int32_t getAshmemFd() const { return mAshmemFd; }
    inline int32_t getReceivePipeFd() const { return mReceivePipeFd; }
    inline int32_t getSendPipeFd() const { return mSendPipeFd; }

    





    status_t sendSignal(char signal);

    







    status_t receiveSignal(char* outSignal);

private:
    String8 mName;
    int32_t mAshmemFd;
    int32_t mReceivePipeFd;
    int32_t mSendPipeFd;
};





struct InputMessage {
    



    sem_t semaphore;

    


    bool consumed;

    int32_t type;

    struct SampleData {
        nsecs_t eventTime;
        PointerCoords coords[0]; 
    };

    int32_t deviceId;
    int32_t source;

    union {
        struct {
            int32_t action;
            int32_t flags;
            int32_t keyCode;
            int32_t scanCode;
            int32_t metaState;
            int32_t repeatCount;
            nsecs_t downTime;
            nsecs_t eventTime;
        } key;

        struct {
            int32_t action;
            int32_t flags;
            int32_t metaState;
            int32_t buttonState;
            int32_t edgeFlags;
            nsecs_t downTime;
            float xOffset;
            float yOffset;
            float xPrecision;
            float yPrecision;
            size_t pointerCount;
            PointerProperties pointerProperties[MAX_POINTERS];
            size_t sampleCount;
            SampleData sampleData[0]; 
        } motion;
    };

    


    static inline size_t sampleDataStride(size_t pointerCount) {
        return sizeof(InputMessage::SampleData) + pointerCount * sizeof(PointerCoords);
    }

    
    static inline SampleData* sampleDataPtrIncrement(SampleData* ptr, size_t stride) {
        return reinterpret_cast<InputMessage::SampleData*>(reinterpret_cast<char*>(ptr) + stride);
    }
};





class InputPublisher {
public:
    
    explicit InputPublisher(const sp<InputChannel>& channel);

    
    ~InputPublisher();

    
    inline sp<InputChannel> getChannel() { return mChannel; }

    



    status_t initialize();

    





    status_t reset();

    




    status_t publishKeyEvent(
            int32_t deviceId,
            int32_t source,
            int32_t action,
            int32_t flags,
            int32_t keyCode,
            int32_t scanCode,
            int32_t metaState,
            int32_t repeatCount,
            nsecs_t downTime,
            nsecs_t eventTime);

    





    status_t publishMotionEvent(
            int32_t deviceId,
            int32_t source,
            int32_t action,
            int32_t flags,
            int32_t edgeFlags,
            int32_t metaState,
            int32_t buttonState,
            float xOffset,
            float yOffset,
            float xPrecision,
            float yPrecision,
            nsecs_t downTime,
            nsecs_t eventTime,
            size_t pointerCount,
            const PointerProperties* pointerProperties,
            const PointerCoords* pointerCoords);

    






    status_t appendMotionSample(
            nsecs_t eventTime,
            const PointerCoords* pointerCoords);

    




    status_t sendDispatchSignal();

    






    status_t receiveFinishedSignal(bool* outHandled);

private:
    sp<InputChannel> mChannel;

    size_t mAshmemSize;
    InputMessage* mSharedMessage;
    bool mPinned;
    bool mSemaphoreInitialized;
    bool mWasDispatched;

    size_t mMotionEventPointerCount;
    InputMessage::SampleData* mMotionEventSampleDataTail;
    size_t mMotionEventSampleDataStride;

    status_t publishInputEvent(
            int32_t type,
            int32_t deviceId,
            int32_t source);
};





class InputConsumer {
public:
    
    explicit InputConsumer(const sp<InputChannel>& channel);

    
    ~InputConsumer();

    
    inline sp<InputChannel> getChannel() { return mChannel; }

    
    status_t initialize();

    







    status_t consume(InputEventFactoryInterface* factory, InputEvent** outEvent);

    





    status_t sendFinishedSignal(bool handled);

    





    status_t receiveDispatchSignal();

private:
    sp<InputChannel> mChannel;

    size_t mAshmemSize;
    InputMessage* mSharedMessage;

    void populateKeyEvent(KeyEvent* keyEvent) const;
    void populateMotionEvent(MotionEvent* motionEvent) const;
};

} 

#endif 
