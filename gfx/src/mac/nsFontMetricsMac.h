




































#ifndef nsFontMetricsMac_h__
#define nsFontMetricsMac_h__

#include <TextEdit.h>	

#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
class nsUnicodeFontMappingMac;

class nsFontMetricsMac : public nsIFontMetrics
{
public:
  nsFontMetricsMac();
  virtual ~nsFontMetricsMac();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLangGroup, nsIDeviceContext* aContext);
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
  NS_IMETHOD  GetWidths(const nscoord *&aWidths);
  NS_IMETHOD  GetLangGroup(nsIAtom** aLangGroup);
  NS_IMETHOD  GetFontHandle(nsFontHandle& aHandle);
  NS_IMETHOD  GetSpaceWidth(nscoord& aSpaceCharWidth);
  virtual PRInt32 GetMaxStringLength();

  nsUnicodeFontMappingMac* GetUnicodeFontMapping();
	
protected:
	void				RealizeFont();

	short							mFontNum;
  nsUnicodeFontMappingMac *mFontMapping;
  nscoord           mEmHeight;
  nscoord           mEmAscent;
  nscoord           mEmDescent;
  nscoord           mMaxHeight;
  nscoord           mLeading;
  nscoord           mMaxAscent;
  nscoord           mMaxDescent;
  nscoord           mMaxAdvance;
  nscoord           mAveCharWidth;
  nscoord           mSpaceWidth;
  nscoord           mXHeight;
  PRInt32           mMaxStringLength;
  nsCOMPtr<nsIAtom> mLangGroup;
  nsIDeviceContext  *mContext;
};

#endif




