




































#ifndef nsRegressionTestFontMetrics_h__
#define nsRegressionTestFontMetrics_h__

#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"

class nsRegressionTestFontMetrics : public nsIFontMetrics
{
public:
   friend nsresult NS_NewRegressionTestFontMetrics(nsIFontMetrics** aMetrics);
   virtual ~nsRegressionTestFontMetrics();


  NS_DECL_ISUPPORTS
  
  NS_IMETHOD  Init(const nsFont& aFont, nsIDeviceContext *aContext);
  NS_IMETHOD  Destroy();

  NS_IMETHOD  GetXHeight(nscoord& aResult);
  NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetSubscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize);
  NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize);

  NS_IMETHOD  GetHeight(nscoord &aHeight);
  NS_IMETHOD  GetLeading(nscoord &aLeading);
  NS_IMETHOD  GetMaxAscent(nscoord &aAscent);
  NS_IMETHOD  GetMaxDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance);
  NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle);
  
  NS_METHOD GetWidth(const char aChar, nscoord& aWidth);
  NS_METHOD GetWidth(const PRUnichar aChar, nscoord& aWidth);
  NS_METHOD GetWidth(const PRUnichar *aString, PRUint32 aLength, nscoord& aWidth);
  NS_METHOD GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth);

protected:
  nsRegressionTestFontMetrics();
  void RealizeFont();

  nsIDeviceContext    *mDeviceContext;
  nscoord             mHeight;
  nscoord             mAscent;
  nscoord             mDescent;
  nscoord             mLeading;
  nscoord             mMaxAscent;
  nscoord             mMaxDescent;
  nscoord             mMaxAdvance;
  nscoord             mXHeight;
  nscoord             mSuperscriptOffset;
  nscoord             mSubscriptOffset;
  nscoord             mStrikeoutSize;
  nscoord             mStrikeoutOffset;
  nscoord             mUnderlineSize;
  nscoord             mUnderlineOffset;
};


#endif
