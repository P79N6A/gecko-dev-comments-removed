









































#ifndef nsIObjectFrame_h___
#define nsIObjectFrame_h___

#include "nsIFrame.h"

class nsIPluginInstance;

class nsIObjectFrame : public nsQueryFrame
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsIObjectFrame)

  NS_IMETHOD GetPluginInstance(nsIPluginInstance*& aPluginInstance) = 0;

  






  virtual nsresult Instantiate(nsIChannel* aChannel, nsIStreamListener** aStreamListener) = 0;

  



  virtual void TryNotifyContentObjectWrapper() = 0;

  











  virtual nsresult Instantiate(const char* aMimeType, nsIURI* aURI) = 0;

  




  virtual void StopPlugin() = 0;

  


  virtual nsIWidget* GetWidget() = 0;
};

#endif 
