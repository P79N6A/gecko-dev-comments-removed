















#ifndef _ANDROIDFW_INPUT_TRANSPORT_H
#define _ANDROIDFW_INPUT_TRANSPORT_H











#include "Input.h"
#include <utils/Errors.h>
#include <utils/Timers.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/BitSet.h>

namespace android {




struct InputMessage {
    enum {
        TYPE_KEY = 1,
        TYPE_MOTION = 2,
        TYPE_FINISHED = 3,
    };

    struct Header {
        uint32_t type;
        uint32_t padding; 
    } header;

    union Body {
        struct Key {
            uint32_t seq;
            nsecs_t eventTime;
            int32_t deviceId;
            int32_t source;
            int32_t action;
            int32_t flags;
            int32_t keyCode;
            int32_t scanCode;
            int32_t metaState;
            int32_t repeatCount;
            nsecs_t downTime;

            inline size_t size() const {
                return sizeof(Key);
            }
        } key;

        struct Motion {
            uint32_t seq;
            nsecs_t eventTime;
            int32_t deviceId;
            int32_t source;
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
            struct Pointer {
                PointerProperties properties;
                PointerCoords coords;
            } pointers[MAX_POINTERS];

            int32_t getActionId() const {
                uint32_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                        >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                return pointers[index].properties.id;
            }

            inline size_t size() const {
                return sizeof(Motion) - sizeof(Pointer) * MAX_POINTERS
                        + sizeof(Pointer) * pointerCount;
            }
        } motion;

        struct Finished {
            uint32_t seq;
            bool handled;

            inline size_t size() const {
                return sizeof(Finished);
            }
        } finished;
    } body;

    bool isValid(size_t actualSize) const;
    size_t size() const;
};









class InputChannel : public RefBase {
protected:
    virtual ~InputChannel();

public:
    InputChannel(const String8& name, int fd);

    



    static status_t openInputChannelPair(const String8& name,
            sp<InputChannel>& outServerChannel, sp<InputChannel>& outClientChannel);

    inline String8 getName() const { return mName; }
    inline int getFd() const { return mFd; }

    










    status_t sendMessage(const InputMessage* msg);

    









    status_t receiveMessage(InputMessage* msg);

    
    sp<InputChannel> dup() const;

private:
    String8 mName;
    int mFd;
};




class InputPublisher {
public:
    
    explicit InputPublisher(const sp<InputChannel>& channel);

    
    ~InputPublisher();

    
    inline sp<InputChannel> getChannel() { return mChannel; }

    







    status_t publishKeyEvent(
            uint32_t seq,
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
            uint32_t seq,
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

    










    status_t receiveFinishedSignal(uint32_t* outSeq, bool* outHandled);

private:
    sp<InputChannel> mChannel;
};




class InputConsumer {
public:
    
    explicit InputConsumer(const sp<InputChannel>& channel);

    
    ~InputConsumer();

    
    inline sp<InputChannel> getChannel() { return mChannel; }

    






















    status_t consume(InputEventFactoryInterface* factory, bool consumeBatches,
            nsecs_t frameTime, uint32_t* outSeq, InputEvent** outEvent);

    







    status_t sendFinishedSignal(uint32_t seq, bool handled);

    















    bool hasDeferredEvent() const;

    




    bool hasPendingBatch() const;

private:
    
    const bool mResampleTouch;

    
    sp<InputChannel> mChannel;

    
    InputMessage mMsg;

    
    
    bool mMsgDeferred;

    
    struct Batch {
        Vector<InputMessage> samples;
    };
    Vector<Batch> mBatches;

    
    struct History {
        nsecs_t eventTime;
        BitSet32 idBits;
        int32_t idToIndex[MAX_POINTER_ID + 1];
        PointerCoords pointers[MAX_POINTERS];

        void initializeFrom(const InputMessage* msg) {
            eventTime = msg->body.motion.eventTime;
            idBits.clear();
            for (size_t i = 0; i < msg->body.motion.pointerCount; i++) {
                uint32_t id = msg->body.motion.pointers[i].properties.id;
                idBits.markBit(id);
                idToIndex[id] = i;
                pointers[i].copyFrom(msg->body.motion.pointers[i].coords);
            }
        }

        const PointerCoords& getPointerById(uint32_t id) const {
            return pointers[idToIndex[id]];
        }
    };
    struct TouchState {
        int32_t deviceId;
        int32_t source;
        size_t historyCurrent;
        size_t historySize;
        History history[2];
        History lastResample;

        void initialize(int32_t deviceId, int32_t source) {
            this->deviceId = deviceId;
            this->source = source;
            historyCurrent = 0;
            historySize = 0;
            lastResample.eventTime = 0;
            lastResample.idBits.clear();
        }

        void addHistory(const InputMessage* msg) {
            historyCurrent ^= 1;
            if (historySize < 2) {
                historySize += 1;
            }
            history[historyCurrent].initializeFrom(msg);
        }

        const History* getHistory(size_t index) const {
            return &history[(historyCurrent + index) & 1];
        }
    };
    Vector<TouchState> mTouchStates;

    
    
    
    
    struct SeqChain {
        uint32_t seq;   
        uint32_t chain; 
    };
    Vector<SeqChain> mSeqChains;

    status_t consumeBatch(InputEventFactoryInterface* factory,
            nsecs_t frameTime, uint32_t* outSeq, InputEvent** outEvent);
    status_t consumeSamples(InputEventFactoryInterface* factory,
            Batch& batch, size_t count, uint32_t* outSeq, InputEvent** outEvent);

    void updateTouchState(InputMessage* msg);
    void rewriteMessage(const TouchState& state, InputMessage* msg);
    void resampleTouchState(nsecs_t frameTime, MotionEvent* event,
            const InputMessage *next);

    ssize_t findBatch(int32_t deviceId, int32_t source) const;
    ssize_t findTouchState(int32_t deviceId, int32_t source) const;

    status_t sendUnchainedFinishedSignal(uint32_t seq, bool handled);

    static void initializeKeyEvent(KeyEvent* event, const InputMessage* msg);
    static void initializeMotionEvent(MotionEvent* event, const InputMessage* msg);
    static void addSample(MotionEvent* event, const InputMessage* msg);
    static bool canAddSample(const Batch& batch, const InputMessage* msg);
    static ssize_t findSampleNoLaterThan(const Batch& batch, nsecs_t time);
    static bool shouldResampleTool(int32_t toolType);

    static bool isTouchResamplingEnabled();
};

} 

#endif 
