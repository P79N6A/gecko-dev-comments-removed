















#ifndef A_HANDLER_H_

#define A_HANDLER_H_

#include <media/stagefright/foundation/ALooper.h>
#include <utils/RefBase.h>

namespace android {

struct AMessage;

struct AHandler : public RefBase {
    AHandler()
        : mID(0) {
    }

    ALooper::handler_id id() const {
        return mID;
    }

    sp<ALooper> looper();

protected:
    virtual void onMessageReceived(const sp<AMessage> &msg) = 0;

private:
    friend struct ALooperRoster;

    ALooper::handler_id mID;

    void setID(ALooper::handler_id id) {
        mID = id;
    }

    DISALLOW_EVIL_CONSTRUCTORS(AHandler);
};

}  

#endif  
