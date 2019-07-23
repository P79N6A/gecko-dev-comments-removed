




































#ifndef nsPluginInstancePeer_h___
#define nsPluginInstancePeer_h___

#include "nsIPluginInstancePeer2.h"
#include "nsIWindowlessPlugInstPeer.h"
#include "nsIPluginTagInfo2.h"
#include "nsIPluginInstanceOwner.h"
#ifdef OJI
#include "nsIJVMPluginTagInfo.h"
#endif
#include "nsPIPluginInstancePeer.h"

class nsPluginInstancePeerImpl : public nsIPluginInstancePeer2,
                                 public nsIWindowlessPluginInstancePeer,
                                 public nsIPluginTagInfo2,
#ifdef OJI
                                 public nsIJVMPluginTagInfo,
#endif
                                 public nsPIPluginInstancePeer
								
{
public:
  nsPluginInstancePeerImpl();
  virtual ~nsPluginInstancePeerImpl();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGININSTANCEPEER
  NS_DECL_NSIWINDOWLESSPLUGININSTANCEPEER
  NS_DECL_NSIPLUGININSTANCEPEER2
  NS_DECL_NSIPLUGINTAGINFO
  NS_DECL_NSIPLUGINTAGINFO2

  
  

  NS_IMETHOD
  GetCode(const char* *result);

  NS_IMETHOD
  GetCodeBase(const char* *result);

  NS_IMETHOD
  GetArchive(const char* *result);

  NS_IMETHOD
  GetName(const char* *result);

  NS_IMETHOD
  GetMayScript(PRBool *result);

  NS_DECL_NSPIPLUGININSTANCEPEER

  

  nsresult Initialize(nsIPluginInstanceOwner *aOwner,
                      const nsMIMEType aMimeType);

  nsresult SetOwner(nsIPluginInstanceOwner *aOwner);

private:
  nsIPluginInstance       *mInstance; 
  nsIPluginInstanceOwner  *mOwner;    
  nsMIMEType              mMIMEType;
  PRUint32                mThreadID;
  PRBool                  mStopped;
};

#endif
