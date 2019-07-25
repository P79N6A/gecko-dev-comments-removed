







































#ifndef mozilla_ipc_Ril_h
#define mozilla_ipc_Ril_h 1

#include "mozilla/RefPtr.h"

namespace base {
class MessageLoop;
}

class nsIThread;

namespace mozilla {
namespace ipc {








struct RilRawData
{
    static const size_t MAX_DATA_SIZE = 1024;
    uint8_t mData[MAX_DATA_SIZE];

    
    size_t mSize;
};

class RilConsumer : public RefCounted<RilConsumer>
{
public:
    virtual ~RilConsumer() { }
    virtual void MessageReceived(RilRawData* aMessage) { }
};

bool StartRil(RilConsumer* aConsumer);

bool SendRilRawData(RilRawData** aMessage);

void StopRil();

} 
} 

#endif 
