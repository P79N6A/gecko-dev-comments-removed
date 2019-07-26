



#include "base/linked_ptr.h"

#include "mozilla/ModuleUtils.h"
#include "nsIClassInfoImpl.h"

#ifdef MOZ_WEBRTC

#include "PeerConnectionImpl.h"

#define PEERCONNECTION_CID \
{0xb93af7a1, 0x3411, 0x44a8, {0xbd, 0x0a, 0x8a, 0xf3, 0xdd, 0xe4, 0xd8, 0xd8}}

#define PEERCONNECTION_CONTRACTID "@mozilla.org/peerconnection;1"

namespace sipcc
{

NS_GENERIC_FACTORY_CONSTRUCTOR(PeerConnectionImpl)
}


NS_DEFINE_NAMED_CID(PEERCONNECTION_CID);

static const mozilla::Module::CIDEntry kCIDs[] = {
  { &kPEERCONNECTION_CID, false, NULL, sipcc::PeerConnectionImplConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kContracts[] = {
  { PEERCONNECTION_CONTRACTID, &kPEERCONNECTION_CID },
  { NULL }
};

static const mozilla::Module kModule = {
  mozilla::Module::kVersion,
  kCIDs,
  kContracts
};

NSMODULE_DEFN(peerconnection) = &kModule;

#endif
