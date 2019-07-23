









































#ifndef nsIObjectFrame_h___
#define nsIObjectFrame_h___

#include "nsIFrame.h"

class nsIPluginInstance;


#define NS_IOBJECTFRAME_IID \
{ 0x3e2df1fe, 0xa898, 0x4e2e, { 0x87, 0x63, 0x4c, 0xa9, 0x4, 0xfa, 0x33, 0x8e } }

class nsIObjectFrame : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOBJECTFRAME_IID)

  NS_IMETHOD GetPluginInstance(nsIPluginInstance*& aPluginInstance) = 0;

  



  virtual nsresult Instantiate(nsIChannel* aChannel, nsIStreamListener** aStreamListener) = 0;

  virtual void TryNotifyContentObjectWrapper() = 0;

  








  virtual nsresult Instantiate(const char* aMimeType, nsIURI* aURI) = 0;

  




  virtual void StopPlugin() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIObjectFrame, NS_IOBJECTFRAME_IID)

#endif 
