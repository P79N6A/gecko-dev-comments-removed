




































#include "nsATSUIUtils.h"
#include "nsIDeviceContext.h"
#include "nsDrawingSurfaceMac.h"
#include "nsDeviceContextMac.h"
#include "nsTransform2D.h"
#include "plhash.h"
#include "nsFontUtils.h"

#include <Gestalt.h>
#include <FixMath.h>











class ATSUILayoutCache
{
public:
	ATSUILayoutCache();
	~ATSUILayoutCache();

	PRBool	Get(short aFont, short aSize, PRBool aBold, PRBool aItalic, nscolor aColor, ATSUTextLayout *aTxlayout);
	void	Set(short aFont, short aSize, PRBool aBold, PRBool aItalic, nscolor aColor, ATSUTextLayout aTxlayout);

private:
	typedef struct
	{
		short	font;
		short	size;
		nscolor	color;
		short	boldItalic;
	} atsuiLayoutCacheKey;

	PRBool	Get(atsuiLayoutCacheKey *key, ATSUTextLayout *txlayout);
	void	Set(atsuiLayoutCacheKey *key, ATSUTextLayout txlayout);

	static PR_CALLBACK PLHashNumber HashKey(const void *aKey);
	static PR_CALLBACK PRIntn		CompareKeys(const void *v1, const void *v2);
	static PR_CALLBACK PRIntn		CompareValues(const void *v1, const void *v2);
	static PR_CALLBACK PRIntn		FreeHashEntries(PLHashEntry *he, PRIntn italic, void *arg);

	struct PLHashTable*		mTable;
	PRUint32				mCount;
};


ATSUILayoutCache::ATSUILayoutCache()
{
	mTable = PL_NewHashTable(8, (PLHashFunction)HashKey, 
								(PLHashComparator)CompareKeys, 
								(PLHashComparator)CompareValues,
								nsnull, nsnull);
	mCount = 0;
}






ATSUILayoutCache::~ATSUILayoutCache()
{
	if (mTable)
	{
		PL_HashTableEnumerateEntries(mTable, FreeHashEntries, 0);
		PL_HashTableDestroy(mTable);
		mTable = nsnull;
	}
}


PRBool ATSUILayoutCache::Get(short aFont, short aSize, PRBool aBold, PRBool aItalic, nscolor aColor, ATSUTextLayout *aTxlayout)
{
	atsuiLayoutCacheKey k = {aFont, aSize, aColor, (aBold ? 1 : 0) + (aItalic ? 2 : 0) };
	return Get(&k, aTxlayout);
}


void ATSUILayoutCache::Set(short aFont, short aSize, PRBool aBold, PRBool aItalic, nscolor aColor, ATSUTextLayout aTxlayout)
{
	atsuiLayoutCacheKey k = {aFont, aSize, aColor, (aBold ? 1 : 0) + (aItalic ? 2 : 0) };
	return Set(&k, aTxlayout);
}






PRBool ATSUILayoutCache::Get(atsuiLayoutCacheKey *key, ATSUTextLayout *txlayout)
{
	PLHashEntry **hep = PL_HashTableRawLookup(mTable, HashKey(key), key);
	PLHashEntry *he = *hep;
	if( he )
	{
		*txlayout = (ATSUTextLayout)he->value;
		return PR_TRUE;
	}
	return PR_FALSE;
}


void ATSUILayoutCache::Set(atsuiLayoutCacheKey *key, ATSUTextLayout txlayout)
{
	atsuiLayoutCacheKey *newKey = new atsuiLayoutCacheKey;
	if (newKey)
	{
		*newKey = *key;
		PL_HashTableAdd(mTable, newKey, txlayout);
		mCount ++;
	}
}


PR_CALLBACK PLHashNumber ATSUILayoutCache::HashKey(const void *aKey)
{
	atsuiLayoutCacheKey* key = (atsuiLayoutCacheKey*)aKey;	
	return 	key->font + (key->size << 7) + (key->boldItalic << 12) + key->color;
}


PR_CALLBACK PRIntn ATSUILayoutCache::CompareKeys(const void *v1, const void *v2)
{
	atsuiLayoutCacheKey *k1 = (atsuiLayoutCacheKey *)v1;
	atsuiLayoutCacheKey *k2 = (atsuiLayoutCacheKey *)v2;
	return (k1->font == k2->font) && (k1->color == k2->color ) && (k1->size == k2->size) && (k1->boldItalic == k2->boldItalic);
}


PR_CALLBACK PRIntn ATSUILayoutCache::CompareValues(const void *v1, const void *v2)
{
	ATSUTextLayout t1 = (ATSUTextLayout)v1;
	ATSUTextLayout t2 = (ATSUTextLayout)v2;
	return (t1 == t2);
}


PR_CALLBACK PRIntn ATSUILayoutCache::FreeHashEntries(PLHashEntry *he, PRIntn italic, void *arg)
{
	delete (atsuiLayoutCacheKey*)he->key;
	ATSUDisposeTextLayout((ATSUTextLayout)he->value);
	return HT_ENUMERATE_REMOVE;
}


#pragma mark -










ATSUILayoutCache* nsATSUIUtils::gTxLayoutCache = nsnull;

PRBool	nsATSUIUtils::gIsAvailable = PR_FALSE;
PRBool	nsATSUIUtils::gInitialized = PR_FALSE;






void nsATSUIUtils::Initialize()
{
  if (!gInitialized)
  {
    long version;
    gIsAvailable = (::Gestalt(gestaltATSUVersion, &version) == noErr);

    gTxLayoutCache = new ATSUILayoutCache();
    if (!gTxLayoutCache)
      gIsAvailable = PR_FALSE;

    gInitialized = PR_TRUE;
  }
}






PRBool nsATSUIUtils::IsAvailable()
{
	return gIsAvailable;
}


#pragma mark -





nsATSUIToolkit::nsATSUIToolkit()
{
	nsATSUIUtils::Initialize();
}







#define FloatToFixed(a)		((Fixed)((float)(a) * fixed1))
#define ATTR_CNT 5


ATSUTextLayout nsATSUIToolkit::GetTextLayout(short aFontNum, short aSize, PRBool aBold, PRBool aItalic, nscolor aColor)
{ 
	ATSUTextLayout txLayout = nsnull;
	OSStatus err;
	if (nsATSUIUtils::gTxLayoutCache->Get(aFontNum, aSize, aBold, aItalic, aColor, &txLayout))
		return txLayout;
		
	UniChar dmy[1];
	err = ::ATSUCreateTextLayoutWithTextPtr (dmy, 0,0,0,0,NULL, NULL, &txLayout);
	if(noErr != err) {
		NS_WARNING("ATSUCreateTextLayoutWithTextPtr failed");
    
    return nsnull;
	}

	ATSUStyle				theStyle;
	err = ::ATSUCreateStyle(&theStyle);
	if(noErr != err) {
		NS_WARNING("ATSUCreateStyle failed");
    
  	err = ::ATSUDisposeTextLayout(txLayout);
    return nsnull;
	}

	ATSUAttributeTag 		theTag[ATTR_CNT];
	ByteCount				theValueSize[ATTR_CNT];
	ATSUAttributeValuePtr 	theValue[ATTR_CNT];

	
	ATSUFontID atsuFontID;
	
	
	
	FMFontStyle fbStyle;
	if (::FMGetFontFromFontFamilyInstance(aFontNum, 0, &atsuFontID, &fbStyle) == kFMInvalidFontErr) {
		NS_WARNING("FMGetFontFromFontFamilyInstance failed");
    
  	err = ::ATSUDisposeStyle(theStyle);
  	err = ::ATSUDisposeTextLayout(txLayout);
    return nsnull;
	}
	
	theTag[0] = kATSUFontTag;
	theValueSize[0] = (ByteCount) sizeof(ATSUFontID);
	theValue[0] = (ATSUAttributeValuePtr) &atsuFontID;
	
	
	
	float  dev2app;
	short fontsize = aSize;

	dev2app = mContext->DevUnitsToAppUnits();
  
  Fixed size = FloatToFixed( (float) rint(float(fontsize) / dev2app));
	if( FixRound ( size ) < 9  && !nsFontUtils::DisplayVerySmallFonts())
		size = X2Fix(9);

	theTag[1] = kATSUSizeTag;
	theValueSize[1] = (ByteCount) sizeof(Fixed);
	theValue[1] = (ATSUAttributeValuePtr) &size;
	
	
	
	RGBColor color;

	#define COLOR8TOCOLOR16(color8)	 ((color8 << 8) | color8) 		

	color.red = COLOR8TOCOLOR16(NS_GET_R(aColor));
	color.green = COLOR8TOCOLOR16(NS_GET_G(aColor));
	color.blue = COLOR8TOCOLOR16(NS_GET_B(aColor));				
	theTag[2] = kATSUColorTag;
	theValueSize[2] = (ByteCount) sizeof(RGBColor);
	theValue[2] = (ATSUAttributeValuePtr) &color;
	

	
	Boolean isBold = aBold ? true : false;
	theTag[3] = kATSUQDBoldfaceTag;
	theValueSize[3] = (ByteCount) sizeof(Boolean);
	theValue[3] = (ATSUAttributeValuePtr) &isBold;
	

	
	Boolean isItalic = aItalic ? true : false;
	theTag[4] = kATSUQDItalicTag;
	theValueSize[4] = (ByteCount) sizeof(Boolean);
	theValue[4] = (ATSUAttributeValuePtr) &isItalic;
	

	err =  ::ATSUSetAttributes(theStyle, ATTR_CNT, theTag, theValueSize, theValue);
	if(noErr != err) {
		NS_WARNING("ATSUSetAttributes failed");
    
  	err = ::ATSUDisposeStyle(theStyle);
  	err = ::ATSUDisposeTextLayout(txLayout);
    return nsnull;
	}
	 	
	err = ::ATSUSetRunStyle(txLayout, theStyle, kATSUFromTextBeginning, kATSUToTextEnd);
	if(noErr != err) {
		NS_WARNING("ATSUSetRunStyle failed");
    
  	err = ::ATSUDisposeStyle(theStyle);
  	err = ::ATSUDisposeTextLayout(txLayout);
    return nsnull;
	}
	
    err = ::ATSUSetTransientFontMatching(txLayout, true);
	if(noErr != err) {
		NS_WARNING( "ATSUSetTransientFontMatching failed");
    
  	err = ::ATSUDisposeStyle(theStyle);
  	err = ::ATSUDisposeTextLayout(txLayout);
    return nsnull;
	}
    	
	nsATSUIUtils::gTxLayoutCache->Set(aFontNum, aSize, aBold, aItalic, aColor,  txLayout);	

	return txLayout;
}





void nsATSUIToolkit::PrepareToDraw(CGrafPtr aPort, nsIDeviceContext* aContext)
{
   mPort = aPort;
   mContext = aContext;
}





void nsATSUIToolkit::StartDraw(
	const PRUnichar *aCharPt,
	PRUint32 aLen,
	short aSize, short aFontNum,
	PRBool aBold, PRBool aItalic, nscolor aColor, ATSUTextLayout& oLayout)
{
  OSStatus err = noErr;
  oLayout = GetTextLayout(aFontNum, aSize, aBold, aItalic, aColor);
  if (nsnull == oLayout) {
    NS_WARNING("GetTextLayout return nsnull");
  	return;
  }

  
  
  
  ::ATSUClearLayoutCache(oLayout, kATSUFromTextBeginning);
  
  err = ::ATSUSetTextPointerLocation( oLayout, (ConstUniCharArrayPtr)aCharPt, 0, aLen, aLen);
  if (noErr != err) {
    NS_WARNING("ATSUSetTextPointerLocation failed");
  	oLayout = nsnull;
  }
  return;
}





nsresult
nsATSUIToolkit::GetTextDimensions(
  const PRUnichar *aCharPt,
  PRUint32 aLen, 
  nsTextDimensions& oDim,
  short aSize, short aFontNum,
  PRBool aBold, PRBool aItalic, nscolor aColor)
{
  if (!nsATSUIUtils::IsAvailable())
    return NS_ERROR_NOT_INITIALIZED;

  StPortSetter    setter(mPort);

  ATSUTextLayout aTxtLayout;
  StartDraw(aCharPt, aLen, aSize, aFontNum, aBold, aItalic, aColor, aTxtLayout);
  if (nsnull == aTxtLayout) 
     return NS_ERROR_FAILURE;

  OSStatus err = noErr;
  ATSUTextMeasurement after;
  ATSUTextMeasurement ascent;
  ATSUTextMeasurement descent;
  err = ::ATSUGetUnjustifiedBounds(aTxtLayout, 0, aLen, NULL, &after, &ascent,
                                   &descent);
  if (noErr != err)
  {
    NS_WARNING("ATSUGetUnjustifiedBounds failed");
    return NS_ERROR_FAILURE;
  }

  oDim.width = FixRound(after);
  oDim.ascent = FixRound(ascent);
  oDim.descent = FixRound(descent);
  
  return NS_OK;
}

#ifdef MOZ_MATHML




nsresult 
nsATSUIToolkit::GetBoundingMetrics(
  const PRUnichar *aCharPt, 
  PRUint32 aLen, 
  nsBoundingMetrics &oBoundingMetrics,
  short aSize, short aFontNum,
  PRBool aBold, PRBool aItalic, 
  nscolor aColor)
{
  if(!nsATSUIUtils::IsAvailable())
    return NS_ERROR_NOT_INITIALIZED;

  StPortSetter setter(mPort);

  ATSUTextLayout aTxtLayout;
  StartDraw(aCharPt, aLen, aSize, aFontNum, aBold, aItalic, aColor, aTxtLayout);
  if(nsnull == aTxtLayout)
    return NS_ERROR_FAILURE;

  OSStatus err = noErr;
  Rect rect;
  ATSUTextMeasurement width;

  if((err = ATSUMeasureTextImage(aTxtLayout, 
    kATSUFromTextBeginning, kATSUToTextEnd, 0, 0, &rect)) != noErr)
  {
    NS_WARNING("ATSUMeasureTextImage failed");
    return NS_ERROR_FAILURE;
  }

  
  oBoundingMetrics.leftBearing = rect.left;
  oBoundingMetrics.rightBearing = rect.right;
  oBoundingMetrics.ascent = -rect.top;
  oBoundingMetrics.descent = rect.bottom;

  err = ::ATSUGetUnjustifiedBounds(aTxtLayout, kATSUFromTextBeginning,
                                   kATSUToTextEnd, NULL, &width, NULL, NULL);
  if (err != noErr)
  {
    oBoundingMetrics.width = oBoundingMetrics.rightBearing;
  }
  else
    oBoundingMetrics.width = FixRound(width);

  return NS_OK;
}
#endif 





nsresult
nsATSUIToolkit::DrawString(
	const PRUnichar *aCharPt, 
	PRUint32 aLen, 
	PRInt32 x, PRInt32 y, 
	short &oWidth,
	short aSize, short aFontNum,
	PRBool aBold, PRBool aItalic, nscolor aColor)
{
  oWidth = 0;
  if (!nsATSUIUtils::IsAvailable())
  	return NS_ERROR_NOT_INITIALIZED;
  	
  StPortSetter    setter(mPort);

  ATSUTextLayout aTxtLayout;
  
  StartDraw(aCharPt, aLen, aSize, aFontNum, aBold, aItalic, aColor, aTxtLayout);
  if (nsnull == aTxtLayout)
    return NS_ERROR_FAILURE;

  OSStatus err = noErr;	
  ATSUTextMeasurement iAfter; 
  err = ::ATSUGetUnjustifiedBounds(aTxtLayout, 0, aLen, NULL, &iAfter, NULL,
                                   NULL);
  if (noErr != err) {
     NS_WARNING("MeasureText failed");
     return NS_ERROR_FAILURE;
  } 

  err = ::ATSUDrawText(aTxtLayout, 0, aLen, Long2Fix(x), Long2Fix(y));
  if (noErr != err) {
    NS_WARNING("ATSUDrawText failed");
    return NS_ERROR_FAILURE;
  } 

  oWidth = FixRound(iAfter);
  
  return NS_OK;
}
