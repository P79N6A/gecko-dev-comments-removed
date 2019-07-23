




































#ifndef nsFontMetricsPh_h__
#define nsFontMetricsPh_h__

#include <Pt.h>

#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"
#include "nsFont.h"
#include "nsString.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsCRT.h"
#include "nsDeviceContextPh.h"
#include "nsCOMPtr.h"

class nsFontMetricsPh : public nsIFontMetrics
{
public:
  nsFontMetricsPh();
  virtual ~nsFontMetricsPh();

   NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLangGroup,
                   nsIDeviceContext* aContext);

	inline
  NS_IMETHODIMP GetLangGroup(nsIAtom** aLangGroup)
		{
		if( !aLangGroup ) return NS_ERROR_NULL_POINTER;
		*aLangGroup = mLangGroup;
		NS_IF_ADDREF(*aLangGroup);
		return NS_OK;
		}

  NS_IMETHODIMP  Destroy()
		{
		mDeviceContext = nsnull;
		return NS_OK;
		}

  inline NS_IMETHODIMP  GetXHeight(nscoord& aResult)
		{
		aResult = mXHeight;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetSuperscriptOffset(nscoord& aResult)
		{
		aResult = mSuperscriptOffset;
		return NS_OK;
		}
  inline NS_IMETHOD  GetSubscriptOffset(nscoord& aResult)
		{
		aResult = mSubscriptOffset;
		return NS_OK;
		}
  inline NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize)
		{
		aOffset = mStrikeoutOffset;
		aSize = mStrikeoutSize;
		return NS_OK;
		}
  inline NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize)
		{
		aOffset = mUnderlineOffset;
		aSize = mUnderlineSize;
		return NS_OK;
		}

  inline NS_IMETHODIMP  GetHeight(nscoord &aHeight)
		{
		aHeight = mHeight;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetNormalLineHeight(nscoord &aHeight)
		{
		aHeight = mEmHeight + mLeading;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetLeading(nscoord &aLeading)
		{
		aLeading = mLeading;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetEmHeight(nscoord &aHeight)
		{
		aHeight = mEmHeight;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetEmAscent(nscoord &aAscent)
		{
		aAscent = mEmAscent;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetEmDescent(nscoord &aDescent)
		{
		aDescent = mEmDescent;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetMaxHeight(nscoord &aHeight)
		{
		aHeight = mMaxHeight;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetMaxAscent(nscoord &aAscent)
		{
		aAscent = mMaxAscent;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetMaxDescent(nscoord &aDescent)
		{
		aDescent = mMaxDescent;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetMaxAdvance(nscoord &aAdvance)
		{
		aAdvance = mMaxAdvance;
		return NS_OK;
		}
	inline NS_IMETHODIMP  GetAveCharWidth(nscoord &aAveCharWidth)
		{
		aAveCharWidth = mAveCharWidth;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetFontHandle(nsFontHandle &aHandle)
		{
		aHandle = (nsFontHandle) mFontHandle;
		return NS_OK;
		}
  inline NS_IMETHODIMP  GetSpaceWidth(nscoord &aSpaceWidth)
		{
		aSpaceWidth = mSpaceWidth;
		return NS_OK;
		}
  
  virtual PRInt32 GetMaxStringLength() { return PR_INT32_MAX; }
  
protected:
  void RealizeFont();

  nsIDeviceContext    *mDeviceContext;
  char                *mFontHandle;		
  nscoord             mHeight;
  nscoord             mAscent;
  nscoord             mDescent;
  nscoord             mLeading;
  nscoord             mEmHeight;
  nscoord             mEmAscent;
  nscoord             mEmDescent;
  nscoord             mMaxHeight;
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
  nscoord             mSpaceWidth;
  nscoord			mAveCharWidth;
  
  virtual PRInt32 GetMaxStringLength() { return PR_INT32_MAX; }

  nsCOMPtr<nsIAtom>   mLangGroup;
};

class nsFontEnumeratorPh : public nsIFontEnumerator
{
public:
  nsFontEnumeratorPh();
  NS_DECL_ISUPPORTS


	inline NS_IMETHODIMP HaveFontFor(const char* aLangGroup, PRBool* aResult)
		{
		  NS_ENSURE_ARG_POINTER(aResult);
		  *aResult = PR_TRUE; 
		  return NS_OK;
		}


	inline NS_IMETHODIMP GetDefaultFont(const char *aLangGroup, const char *aGeneric, PRUnichar **aResult)
		{
		  
		  
		  NS_ENSURE_ARG_POINTER(aResult);
		  *aResult = nsnull;
		  return NS_OK;
		}

	inline NS_IMETHODIMP UpdateFontList(PRBool *updateFontList)
		{
		  *updateFontList = PR_FALSE; 
		  return NS_OK;
		}

	inline NS_IMETHODIMP EnumerateAllFonts(PRUint32* aCount, PRUnichar*** aResult)
		{
		  return EnumerateFonts( nsnull, nsnull, aCount, aResult );
		}

	NS_IMETHOD EnumerateFonts( const char* aLangGroup, const char* aGeneric,
								PRUint32* aCount, PRUnichar*** aResult );

};

#endif
