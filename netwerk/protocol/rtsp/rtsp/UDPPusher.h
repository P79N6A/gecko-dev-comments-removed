















#ifndef UDP_PUSHER_H_

#define UDP_PUSHER_H_

#include <media/stagefright/foundation/AHandler.h>

#include <stdio.h>
#include <arpa/inet.h>

#include "prio.h"

namespace android {

struct UDPPusher : public AHandler {
    UDPPusher(const char *filename, unsigned port);

    void start();

protected:
    virtual ~UDPPusher();
    virtual void onMessageReceived(const sp<AMessage> &msg);

private:
    enum {
        kWhatPush = 'push'
    };

    FILE *mFile;
    PRFileDesc *mSocket;
    PRNetAddr mRemoteAddr;

    uint32_t mFirstTimeMs;
    int64_t mFirstTimeUs;

    bool onPush();

    DISALLOW_EVIL_CONSTRUCTORS(UDPPusher);
};

}  

#endif
