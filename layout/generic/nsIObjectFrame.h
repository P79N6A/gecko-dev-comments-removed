









































#ifndef nsIObjectFrame_h___
#define nsIObjectFrame_h___

#include "nsIFrame.h"

class nsNPAPIPluginInstance;

class nsIObjectFrame : public nsQueryFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsIObjectFrame)

  NS_IMETHOD GetPluginInstance(nsNPAPIPluginInstance** aPluginInstance) = 0;

  


  virtual nsIWidget* GetWidget() = 0;

  




  virtual void SetIsDocumentActive(bool aIsActive) = 0;
};

#endif 
