




































#ifndef __inFlasher_h__
#define __inFlasher_h__

#include "inIFlasher.h"

#include "nsIDOMElement.h"
#include "nsIRenderingContext.h"

#include "nsCOMPtr.h"

#define BOUND_INNER 0
#define BOUND_OUTER 1

#define DIR_VERTICAL 0
#define DIR_HORIZONTAL 1 

class inFlasher : public inIFlasher
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INIFLASHER

  inFlasher();
  virtual ~inFlasher();

protected:
  void DrawOutline(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                   nsIRenderingContext* aRenderContext,
                   PRBool aDrawBegin, PRBool aDrawEnd);
  void DrawLine(nscoord aX, nscoord aY, nscoord aLength,
                PRBool aDir, PRBool aBounds,
                nsIRenderingContext* aRenderContext);

  nscolor mColor;

  PRUint16 mThickness;
  PRPackedBool mInvert;
};


#define IN_FLASHER_CID \
{ 0x9286e71a, 0x621a, 0x4b91, { 0x85, 0x1e, 0x99, 0x84, 0xc1, 0xa2, 0xe8, 0x1a } }

#endif 
