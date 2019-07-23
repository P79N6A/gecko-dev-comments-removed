




































#include "nsRegressionTestFontMetrics.h"

#define MAPPING_FACTOR_FOR_SPACE 0.02f
#define MAPPING_FACTOR_FOR_LOWER_CASE  0.50f
#define MAPPING_FACTOR_FOR_OTHERS  0.70f


nsresult
NS_NewRegressionTestFontMetrics(nsIFontMetrics** aMetrics)
{
  if (aMetrics == nsnull) {
    return NS_ERROR_NULL_POINTER;
  }

  nsRegressionTestFontMetrics  *fm = new nsRegressionTestFontMetrics();

  if (nsnull == fm) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(fm, aMetrics);
}

nsRegressionTestFontMetrics:: nsRegressionTestFontMetrics()
{
  mDeviceContext = nsnull;
  
  mHeight = 0; 
  mAscent = 0; 
  mDescent = 0; 
  mLeading = 0; 
  mMaxAscent = 0; 
  mMaxDescent = 0; 
  mMaxAdvance = 0; 
  mXHeight = 0; 
  mSuperscriptOffset = 0; 
  mSubscriptOffset = 0; 
  mStrikeoutSize = 0; 
  mStrikeoutOffset = 0; 
  mUnderlineSize = 0; 
  mUnderlineOffset = 0; 
}
  
NS_IMPL_ISUPPORTS1(nsRegressionTestFontMetrics, nsIFontMetrics)

nsRegressionTestFontMetrics::~nsRegressionTestFontMetrics()
{
  mDeviceContext = nsnull;
}


NS_IMETHODIMP
nsRegressionTestFontMetrics::Init(const nsFont& aFont, nsIDeviceContext *aContext)
{
  mFont = aFont;
  mDeviceContext = aContext;
  RealizeFont();
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::Destroy()
{
  mDeviceContext = nsnull;
  return NS_OK;
}

void
nsRegressionTestFontMetrics::RealizeFont()
{
  float dev2app;
  dev2app = mDeviceContext->DevUnitsToAppUnits();
  nscoord onepixel = NSToCoordRound(1 * dev2app);
  PRUint32 fontsize = mFont.size;
 
  
  

  mHeight = fontsize;
  mXHeight = NSToCoordRound((float)mHeight * 0.50f);
  mSuperscriptOffset = mXHeight;
  mSubscriptOffset = mXHeight;
  mStrikeoutSize = onepixel;
  mStrikeoutOffset = NSToCoordRound(mXHeight / 3.5f);

  
  mAscent = NSToCoordRound((float)fontsize * 0.60f);
  mDescent = NSToCoordRound((float)fontsize * 0.20f);

  mLeading = NSToCoordRound((float)fontsize * 0.10f);
  mMaxAscent = mAscent;
  mMaxDescent = mDescent;
  mMaxAdvance = NSToCoordRound((float)fontsize * 0.80f);
  mUnderlineSize = onepixel;
  mUnderlineOffset =  NSToCoordRound(fontsize / 100.0f);
  
}











NS_METHOD
nsRegressionTestFontMetrics::GetWidth(const char aChar, nscoord& aWidth)
{
  float size = (float)mFont.size;
  aWidth = 0;

  if(aChar == ' ')
    size *= MAPPING_FACTOR_FOR_SPACE;
  else if(aChar >= 'a' && aChar <= 'z')
    size *= MAPPING_FACTOR_FOR_LOWER_CASE;
  else
    size *= MAPPING_FACTOR_FOR_OTHERS;
    
  aWidth = NSToCoordRound(size);

  return NS_OK;
}










NS_METHOD
nsRegressionTestFontMetrics::GetWidth(const PRUnichar aChar,nscoord& aWidth)
{
  float size = (float)mFont.size;
  aWidth = 0;

  if(aChar == ' ')
    size *= MAPPING_FACTOR_FOR_SPACE;
  else if(aChar >= 'a' && aChar <= 'z')
    size *= MAPPING_FACTOR_FOR_LOWER_CASE;
  else 
    size *= MAPPING_FACTOR_FOR_OTHERS;
      
  aWidth = NSToCoordRound(size);

  return NS_OK;
}













NS_METHOD
nsRegressionTestFontMetrics::GetWidth(const PRUnichar* aString, PRUint32 aLength, nscoord& aWidth)
{
  aWidth = 0;
  
  if(aString == nsnull)
    return NS_ERROR_NULL_POINTER;
  
  float size;
  float totalsize = 0;
  
  for(PRUint32 index = 0; index < aLength; index++){
    size = (float)mFont.size;
    if(aString[index] == ' ')
      size *= MAPPING_FACTOR_FOR_SPACE;
    else if(aString[index] >= 'a' && aString[index] <= 'z')
      size *= MAPPING_FACTOR_FOR_LOWER_CASE;
    else 
      size *= MAPPING_FACTOR_FOR_OTHERS;
    totalsize += size;    
  }
  aWidth = NSToCoordRound(totalsize);
  return NS_OK;
}













NS_METHOD
nsRegressionTestFontMetrics::GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth)
{
  aWidth = 0;

  if(aString == nsnull)
    return NS_ERROR_NULL_POINTER;
  
  float size;
  float totalsize = 0;
 
  for(PRUint32 index=0; index < aLength; index++){
    size = (float)mFont.size;
    if(aString[index] == ' ')
      size *= MAPPING_FACTOR_FOR_SPACE;
    else if(aString[index] >= 'a' && aString[index] <= 'z')
      size *= MAPPING_FACTOR_FOR_LOWER_CASE;
    else
      size *= MAPPING_FACTOR_FOR_OTHERS;
    totalsize += size;    
  }
  aWidth += NSToCoordRound(totalsize);
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetXHeight(nscoord& aResult)
{
  aResult = mXHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetSuperscriptOffset(nscoord& aResult)
{
  aResult = mSuperscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetSubscriptOffset(nscoord& aResult)
{
  aResult = mSubscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mStrikeoutOffset;
  aSize = mStrikeoutSize;
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetUnderline(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mUnderlineOffset;
  aSize = mUnderlineSize;
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetHeight(nscoord &aHeight)
{
  aHeight = mHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetLeading(nscoord &aLeading)
{
  aLeading = mLeading;
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetMaxAscent(nscoord &aAscent)
{
  aAscent = mMaxAscent;
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetMaxDescent(nscoord &aDescent)
{
  aDescent = mMaxDescent;
  return NS_OK;
}

NS_IMETHODIMP
nsRegressionTestFontMetrics::GetMaxAdvance(nscoord &aAdvance)
{
  aAdvance = mMaxAdvance;
  return NS_OK;
}

nsRegressionTestFontMetrics::GetFontHandle(nsFontHandle &aHandle)
{
  
  aHandle = NULL;
  return NS_OK;
}

