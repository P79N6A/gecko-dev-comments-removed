















#ifndef A_PACKET_SOURCE_H_

#define A_PACKET_SOURCE_H_

#include <media/stagefright/foundation/ABase.h>
#include <media/stagefright/MetaData.h>
#include <utils/RefBase.h>

namespace android {

struct ASessionDescription;

struct APacketSource : public RefBase {
    APacketSource(const sp<ASessionDescription> &sessionDesc, size_t index);

    status_t initCheck() const;

    virtual sp<MetaData> getFormat();

protected:
    virtual ~APacketSource();

private:
    status_t mInitCheck;

    sp<MetaData> mFormat;

    DISALLOW_EVIL_CONSTRUCTORS(APacketSource);
};


}  

#endif  
