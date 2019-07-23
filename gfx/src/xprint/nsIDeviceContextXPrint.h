





































#include "nsISupports.h"

#ifndef __nsIDeviceContextXp_h
#define __nsIDeviceContextXp_h


#define NS_IDEVICECONTEXTXP_IID \
  {0x35efd8b6, 0x13cc, 0x11d3, \
    {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}}

class nsXPrintContext;

class nsIDeviceContextXp : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDEVICECONTEXTXP_IID)
  
  NS_IMETHOD SetSpec(nsIDeviceContextSpec *aSpec) = 0;

  NS_IMETHOD InitDeviceContextXP(nsIDeviceContext *aCreatingDeviceContext,
                                 nsIDeviceContext *aPrinterContext) = 0;

  NS_IMETHOD GetPrintContext(nsXPrintContext*& aContext) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDeviceContextXp, NS_IDEVICECONTEXTXP_IID)

#endif 
