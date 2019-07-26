















#ifndef A_LOOPER_H_

#define A_LOOPER_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/foundation/AString.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <utils/RefBase.h>
#include <utils/threads.h>

namespace android {

struct AHandler;
struct AMessage;

struct ALooper : public RefBase {
    typedef int32_t event_id;
    typedef int32_t handler_id;

    ALooper();

    
    void setName(const char *name);

    handler_id registerHandler(const sp<AHandler> &handler);
    void unregisterHandler(handler_id handlerID);

    status_t start(
            bool runOnCallingThread = false,
            bool canCallJava = false,
            int32_t priority = PRIORITY_DEFAULT
            );

    status_t stop();

    static int64_t GetNowUs();

protected:
    virtual ~ALooper();

private:
    friend struct ALooperRoster;

    struct Event {
        int64_t mWhenUs;
        sp<AMessage> mMessage;
    };

    Mutex mLock;
    Condition mQueueChangedCondition;

    AString mName;

    List<Event> mEventQueue;

    struct LooperThread;
    sp<LooperThread> mThread;
    bool mRunningLocally;

    void post(const sp<AMessage> &msg, int64_t delayUs);
    bool loop();

    DISALLOW_EVIL_CONSTRUCTORS(ALooper);
};

}  

#endif  
