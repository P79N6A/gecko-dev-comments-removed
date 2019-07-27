






#ifndef mozilla_net_RemoteOpenFileParent_h
#define mozilla_net_RemoteOpenFileParent_h

#include "mozilla/net/PRemoteOpenFileParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsIFileURL.h"

namespace mozilla {
namespace net {

class RemoteOpenFileParent : public PRemoteOpenFileParent
{
public:
  explicit RemoteOpenFileParent(nsIFileURL* aURI)
  : mURI(aURI)
  {}

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  bool OpenSendCloseDelete();

private:
  nsCOMPtr<nsIFileURL> mURI;
};

} 
} 

#endif 
