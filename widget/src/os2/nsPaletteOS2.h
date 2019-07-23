


































#ifndef _nspaletteos2_h
#define _nspaletteos2_h

#include "nsIDeviceContext.h"
#include "nscolor.h"

class nsIDeviceContext;

class nsIPaletteOS2 : public nsISupports
{
 public:
   virtual long     GetGPIColor( nsIDeviceContext *aContext, HPS hps, nscolor rgb) = 0;
   virtual nsresult Select( HPS hps, nsIDeviceContext *aContext) = 0;
   virtual nsresult Deselect( HPS hps) = 0;
   virtual nsresult GetNSPalette( nsPalette &aPalette) const = 0;
};




nsresult NS_CreatePalette( nsIDeviceContext *aContext,
                           nsIPaletteOS2 *&aPalette);

#endif
