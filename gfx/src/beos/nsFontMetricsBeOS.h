






































#ifndef nsFontMetricsBeOS_h__
#define nsFontMetricsBeOS_h__

#include "nsDeviceContextBeOS.h" 
#include "nsIFontMetrics.h" 
#include "nsIFontEnumerator.h" 
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsRenderingContextBeOS.h" 
#include "nsICharRepresentable.h" 
#include "nsDataHashtable.h"

#include <Font.h>

class nsFontMetricsBeOS : public nsIFontMetrics
{
public:
  nsFontMetricsBeOS();
  virtual ~nsFontMetricsBeOS();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLangGroup,
                   nsIDeviceContext* aContext);
  NS_IMETHOD  Destroy();

  NS_IMETHOD  GetXHeight(nscoord& aResult);
  NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetSubscriptOffset(nscoord& aResult);
  NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize);
  NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize);

  NS_IMETHOD  GetHeight(nscoord &aHeight);
  NS_IMETHOD  GetNormalLineHeight(nscoord &aHeight); 
  NS_IMETHOD  GetLeading(nscoord &aLeading); 
  NS_IMETHOD  GetEmHeight(nscoord &aHeight); 
  NS_IMETHOD  GetEmAscent(nscoord &aAscent); 
  NS_IMETHOD  GetEmDescent(nscoord &aDescent); 
  NS_IMETHOD  GetMaxHeight(nscoord &aHeight); 
  NS_IMETHOD  GetMaxAscent(nscoord &aAscent);
  NS_IMETHOD  GetMaxDescent(nscoord &aDescent);
  NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance);
  NS_IMETHOD  GetAveCharWidth(nscoord &aAveCharWidth);
  NS_IMETHOD  GetLangGroup(nsIAtom** aLangGroup);
  NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle);

  NS_IMETHOD  GetSpaceWidth(nscoord &aSpaceWidth); 
 
  static nsresult FamilyExists(const nsString& aFontName);
  inline PRBool   IsBold() { return mIsBold; } 
  static int FontMatchesGenericType(font_family family, uint32 flags, const char* aGeneric,  const char* aLangGroup);
  nsCOMPtr<nsIAtom>   mLangGroup; 
  static int MatchesLangGroup(font_family family,  const char* aLangGroup);
  float GetStringWidth(char *string, uint32 len);

protected:
  void RealizeFont(nsIDeviceContext* aContext);

  nsIDeviceContext    *mDeviceContext;
  BFont               mFontHandle;

  nscoord             mLeading;
  nscoord             mEmHeight; 
  nscoord             mEmAscent; 
  nscoord             mEmDescent; 
  nscoord             mMaxHeight; 
  nscoord             mMaxAscent;
  nscoord             mMaxDescent;
  nscoord             mMaxAdvance;
  nscoord             mAveCharWidth;
  nscoord             mXHeight;
  nscoord             mSuperscriptOffset;
  nscoord             mSubscriptOffset;
  nscoord             mStrikeoutSize;
  nscoord             mStrikeoutOffset;
  nscoord             mUnderlineSize;
  nscoord             mUnderlineOffset;
  nscoord             mSpaceWidth; 
 
  PRUint16            mPixelSize; 
  PRUint8             mStretchIndex; 
  PRUint8             mStyleIndex;
  PRBool              mIsBold;
  nsDataHashtable<nsUint32HashKey, float>         mFontWidthCache; 
}; 
 
class nsFontEnumeratorBeOS : public nsIFontEnumerator 
{ 
public: 
  nsFontEnumeratorBeOS(); 
  NS_DECL_ISUPPORTS 
  NS_DECL_NSIFONTENUMERATOR 
};

#endif
