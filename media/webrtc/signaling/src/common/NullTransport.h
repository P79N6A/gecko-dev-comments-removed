




#ifndef NULL_TRANSPORT_H_
#define NULL_TRANSPORT_H_

#include "mozilla/Attributes.h"

#include "common_types.h"

namespace mozilla {




class NullTransport : public webrtc::Transport
{
public:
  virtual int SendPacket(int channel, const void *data, int len)
  {
    (void) channel; (void) data;
    return len;
  }

  virtual int SendRTCPPacket(int channel, const void *data, int len)
  {
    (void) channel; (void) data;
    return len;
  }

  NullTransport() {}

  virtual ~NullTransport() {}

private:
  NullTransport(const NullTransport& other) MOZ_DELETE;
  void operator=(const NullTransport& other) MOZ_DELETE;
};

} 

#endif
