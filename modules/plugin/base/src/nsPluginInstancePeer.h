




































#ifndef nsPluginInstancePeer_h_
#define nsPluginInstancePeer_h_

#include "nsIPluginTagInfo2.h"
#include "nsIPluginInstanceOwner.h"
#include "nsPIPluginInstancePeer.h"

#include "nsCOMPtr.h"

class nsPluginInstancePeerImpl : public nsIPluginTagInfo2,
                                 public nsPIPluginInstancePeer,
                                 public nsIPluginInstancePeer
{
public:
  nsPluginInstancePeerImpl();
  virtual ~nsPluginInstancePeerImpl();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGININSTANCEPEER
  NS_DECL_NSIPLUGINTAGINFO
  NS_DECL_NSIPLUGINTAGINFO2
  NS_DECL_NSPIPLUGININSTANCEPEER

  nsresult Initialize(nsIPluginInstanceOwner *aOwner);
  nsresult SetOwner(nsIPluginInstanceOwner *aOwner);

private:
  
  
  nsIPluginInstanceOwner  *mOwner;
};

#endif
