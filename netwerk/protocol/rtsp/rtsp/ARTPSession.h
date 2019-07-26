















#ifndef A_RTP_SESSION_H_

#define A_RTP_SESSION_H_

#include "mozilla/Types.h"
#include <media/stagefright/foundation/AHandler.h>

#include "prio.h"

namespace android {

struct APacketSource;
struct ARTPConnection;
struct ASessionDescription;
struct MOZ_EXPORT MediaSource;

struct ARTPSession : public AHandler {
    ARTPSession();

    status_t setup(const sp<ASessionDescription> &desc);

    size_t countTracks();
    sp<MediaSource> trackAt(size_t index);

protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);

    virtual ~ARTPSession();

private:
    enum {
        kWhatAccessUnitComplete = 'accu'
    };

    struct TrackInfo {
        PRFileDesc *mRTPSocket;
        PRFileDesc *mRTCPSocket;

        sp<APacketSource> mPacketSource;
    };

    status_t mInitCheck;
    sp<ASessionDescription> mDesc;
    sp<ARTPConnection> mRTPConn;

    Vector<TrackInfo> mTracks;

    bool validateMediaFormat(size_t index, unsigned *port) const;
    static int MakeUDPSocket(unsigned port);

    DISALLOW_EVIL_CONSTRUCTORS(ARTPSession);
};

}  

#endif  
