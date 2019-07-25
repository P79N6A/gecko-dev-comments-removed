




































#ifndef mozilla_net_WyciwygChannelParent_h
#define mozilla_net_WyciwygChannelParent_h

#include "mozilla/net/PWyciwygChannelParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsIStreamListener.h"

#include "nsIWyciwygChannel.h"

namespace mozilla {
namespace net {

class WyciwygChannelParent : public PWyciwygChannelParent
                           , public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

  WyciwygChannelParent();
  virtual ~WyciwygChannelParent();

protected:
  virtual bool RecvInit(const IPC::URI& uri);
  virtual bool RecvAsyncOpen(const IPC::URI& original,
                             const PRUint32& loadFlags);
  virtual bool RecvWriteToCacheEntry(const nsString& data);
  virtual bool RecvCloseCacheEntry(const nsresult& reason);
  virtual bool RecvSetCharsetAndSource(const PRInt32& source,
                                       const nsCString& charset);
  virtual bool RecvSetSecurityInfo(const nsCString& securityInfo);

  virtual void ActorDestroy(ActorDestroyReason why);

  nsCOMPtr<nsIWyciwygChannel> mChannel;
  bool mIPCClosed;
};

} 
} 

#endif 
