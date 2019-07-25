









































#ifndef nsIObjectFrame_h___
#define nsIObjectFrame_h___

#include "nsIFrame.h"

class nsNPAPIPluginInstance;

class nsIObjectFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIObjectFrame)

  NS_IMETHOD GetPluginInstance(nsNPAPIPluginInstance** aPluginInstance) = 0;

  






  virtual nsresult Instantiate(nsIChannel* aChannel, nsIStreamListener** aStreamListener) = 0;

  



  virtual void TryNotifyContentObjectWrapper() = 0;

  











  virtual nsresult Instantiate(const char* aMimeType, nsIURI* aURI) = 0;

  




  virtual void StopPlugin() = 0;

  


  virtual nsIWidget* GetWidget() = 0;
};

#endif 
