




































#ifndef nsATSUIUtils_h___
#define nsATSUIUtils_h___


#include "nsCRT.h"
#include "nsError.h"
#include "nsCoord.h"
#include "nsColor.h"
#include "nsIRenderingContext.h"
#include <ATSUnicode.h>

class ATSUILayoutCache;
class nsDrawingSurfaceMac;
class nsIDeviceContext;


class nsATSUIUtils
{
public:
  static void Initialize();
  static PRBool IsAvailable();

  static ATSUILayoutCache*  gTxLayoutCache;

private:
  static PRBool gIsAvailable;
  static PRBool gInitialized;
};



class nsATSUIToolkit
{
public:
  nsATSUIToolkit();
  ~nsATSUIToolkit() {};

  void PrepareToDraw(CGrafPtr aPort, nsIDeviceContext* aContext);
 
  nsresult GetTextDimensions(const PRUnichar *aCharPt, PRUint32 aLen, nsTextDimensions &oDim, 
                             short aSize, short fontNum, PRBool aBold, 
                             PRBool aItalic, nscolor aColor);
  nsresult DrawString(const PRUnichar *aCharPt, PRUint32 aLen, PRInt32 x, PRInt32 y, short &oWidth, 
                      short aSize, short fontNum, PRBool aBold, PRBool aItalic, 
                      nscolor aColor);
#ifdef MOZ_MATHML
  nsresult GetBoundingMetrics(const PRUnichar *aCharPt, PRUint32 aLen, nsBoundingMetrics &aBoundingMetrics,
                              short aSize, short fontNum, PRBool aBold, 
                              PRBool aItalic, nscolor aColor);
#endif 

private:
  void StartDraw(const PRUnichar *aCharPt, PRUint32 aLen, short aSize, short fontNum, PRBool aBold,
                 PRBool aItalic, nscolor aColor, ATSUTextLayout& oLayout);

  ATSUTextLayout GetTextLayout(short aFontNum, short aSize, PRBool aBold, 
                               PRBool aItalic, nscolor aColor);
  
private:
  CGrafPtr        mPort;
  nsIDeviceContext*  mContext;
};

#endif 
