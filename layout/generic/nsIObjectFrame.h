









































#ifndef nsIObjectFrame_h___
#define nsIObjectFrame_h___

#include "nsIFrame.h"

class nsIPluginInstance;


#define NS_IOBJECTFRAME_IID \
{ 0xf455b51f, 0x7a1f, 0x4bbe, { 0xb5, 0x5d, 0x67, 0x9f, 0x3, 0x3a, 0xd3, 0xfe } }

class nsIObjectFrame : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IOBJECTFRAME_IID)

  NS_IMETHOD GetPluginInstance(nsIPluginInstance*& aPluginInstance) = 0;

  



  virtual nsresult Instantiate(nsIChannel* aChannel, nsIStreamListener** aStreamListener) = 0;

  








  virtual nsresult Instantiate(const char* aMimeType, nsIURI* aURI) = 0;

  




  virtual void StopPlugin() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIObjectFrame, NS_IOBJECTFRAME_IID)

#endif 
