






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
  RemoteOpenFileParent(nsIFileURL* aURI)
  : mURI(aURI)
  {}

  bool OpenSendCloseDelete();

private:
  nsCOMPtr<nsIFileURL> mURI;
};

} 
} 

#endif 
