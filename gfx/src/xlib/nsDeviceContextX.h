





































#ifndef nsDeviceContextX_h__
#define nsDeviceContextX_h__

#include "nsDeviceContext.h"
#include "xlibrgb.h"

class nsFontMetricsXlibContext;
class nsRenderingContextXlibContext;


class nsDeviceContextX : public DeviceContextImpl
{
public:
  nsDeviceContextX()
    :  DeviceContextImpl()
  { 
  }
  
  virtual ~nsDeviceContextX() {}


  NS_IMETHOD GetXlibRgbHandle(XlibRgbHandle *&aHandle) = 0;

  virtual void GetFontMetricsContext(nsFontMetricsXlibContext *&aContext) = 0;
  virtual void GetRCContext(nsRenderingContextXlibContext *&aContext) = 0;
};

#endif 
