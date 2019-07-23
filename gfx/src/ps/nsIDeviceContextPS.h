




































#include "nsISupports.h"

#ifndef __nsIDeviceContextPS_h
#define __nsIDeviceContextPS_h


#define NS_IDEVICECONTEXTPS_IID \
  {0x35efd8b6, 0x13cc, 0x11d3, \
    {0x9d, 0x3a, 0x00, 0x60, 0x08, 0x94, 0x80, 0x10}}

class nsIDeviceContextPS : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDEVICECONTEXTPS_IID)
  
  NS_IMETHOD SetSpec(nsIDeviceContextSpec *aSpec) = 0;

  NS_IMETHOD InitDeviceContextPS(nsIDeviceContext *aCreatingDeviceContext,
                                 nsIDeviceContext *aPrinterContext) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDeviceContextPS, NS_IDEVICECONTEXTPS_IID)

#endif
