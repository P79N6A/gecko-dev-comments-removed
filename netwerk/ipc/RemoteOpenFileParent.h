






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
  RemoteOpenFileParent(nsIFileURL* aURI);

 ~RemoteOpenFileParent();

  virtual bool RecvAsyncOpenFile();

private:
  nsCOMPtr<nsIFileURL> mURI;

#if !defined(XP_WIN) && !defined(MOZ_WIDGET_COCOA)
  int mFd;
#endif
};

} 
} 

#endif 
