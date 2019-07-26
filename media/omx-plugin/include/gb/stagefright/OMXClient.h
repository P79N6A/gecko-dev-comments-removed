















#ifndef OMX_CLIENT_H_

#define OMX_CLIENT_H_

#include <media/IOMX.h>

namespace android {

class OMXClient {
public:
    OMXClient();

    status_t connect();
    void disconnect();

    sp<IOMX> interface() {
        return mOMX;
    }

private:
    sp<IOMX> mOMX;

    OMXClient(const OMXClient &);
    OMXClient &operator=(const OMXClient &);
};

}  

#endif  
