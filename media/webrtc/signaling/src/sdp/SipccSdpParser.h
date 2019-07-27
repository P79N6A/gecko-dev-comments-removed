





#ifndef _SIPCCSDPPARSER_H_
#define _SIPCCSDPPARSER_H_

#include <string>

#include "mozilla/UniquePtr.h"

#include "signaling/src/sdp/Sdp.h"
#include "signaling/src/sdp/SdpErrorHolder.h"

namespace mozilla
{

class SipccSdpParser MOZ_FINAL : public SdpErrorHolder
{
public:
  SipccSdpParser() {}
  virtual ~SipccSdpParser() {}

  



  UniquePtr<Sdp> Parse(const std::string& sdpText);
};

} 

#endif
