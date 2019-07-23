







































#include "nsRenderingContextPS.h"
#include "nsFontMetricsPS.h"
#include "nsDeviceContextPS.h"
#include "nsPostScriptObj.h"  
#include "nsIRegion.h"      
#include "nsIImage.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsEPSObjectPS.h"
#include "nsLocalFile.h"

#include <stdio.h>
#include <math.h>

#define NS_PIXELS_TO_POINTS(x) ((x) * 10)

#define FLAG_CLIP_VALID       0x0001
#define FLAG_CLIP_CHANGED     0x0002
#define FLAG_LOCAL_CLIP_VALID 0x0004

#define FLAGS_ALL             (FLAG_CLIP_VALID | FLAG_CLIP_CHANGED | FLAG_LOCAL_CLIP_VALID)






class PS_State
{
public:
  PS_State();
  PS_State(PS_State &aState);
  ~PS_State();

  PS_State        *mNext;
  nsTransform2D   mMatrix;
  nsRect          mLocalClip;
  nsCOMPtr<nsIFontMetrics>  mFontMetrics;
  nscolor         mCurrentColor;
  nscolor         mTextColor;
  nsLineStyle     mLineStyle;
  PRInt32         mFlags;
};





PS_State :: PS_State()
{
  mNext         = nsnull;
  mMatrix.SetToIdentity();  
  mLocalClip.x = mLocalClip.y = mLocalClip.width = mLocalClip.height = 0;
  mFontMetrics  = nsnull;
  mCurrentColor = NS_RGB(0, 0, 0);
  mTextColor    = NS_RGB(0, 0, 0);
  mLineStyle    = nsLineStyle_kSolid;
}






PS_State :: PS_State(PS_State &aState) : 
  mMatrix(&aState.mMatrix),
  mLocalClip(aState.mLocalClip)
{
  mNext = &aState;
  
  mCurrentColor = aState.mCurrentColor;
  mFontMetrics = nsnull;
  
  mFlags = ~FLAGS_ALL;
  mTextColor = aState.mTextColor;
  mLineStyle = aState.mLineStyle;
}





PS_State :: ~PS_State()
{
  
    
    
  

  
  
}


NS_IMPL_ISUPPORTS1(nsRenderingContextPS, nsIRenderingContext)





nsRenderingContextPS :: nsRenderingContextPS()
{
  mPSObj = nsnull;     
  mContext = nsnull;
  mFontMetrics = nsnull;

  mStateCache = new nsVoidArray();

  mP2T = 1.0f;

  PushState();
}





nsRenderingContextPS::~nsRenderingContextPS()
{
  if (mStateCache){
    PRInt32 cnt = mStateCache->Count();

    while (--cnt >= 0){
      PS_State *state = (PS_State *)mStateCache->ElementAt(cnt);
      mStateCache->RemoveElementAt(cnt);

      if (state)
        delete state;
    }

    delete mStateCache;
    mStateCache = nsnull;
  }

  mTranMatrix = nsnull;
}





NS_IMETHODIMP
nsRenderingContextPS::Init(nsIDeviceContext* aContext)
{
  NS_ENSURE_TRUE(nsnull != aContext, NS_ERROR_NULL_POINTER);

  mContext = aContext;
  mP2T = mContext->DevUnitsToAppUnits();

  mPSObj = NS_REINTERPRET_CAST(nsDeviceContextPS *, mContext.get())->GetPrintContext();

  NS_ENSURE_TRUE(nsnull != mPSObj, NS_ERROR_NULL_POINTER);

  
  
  
  mTranMatrix->SetToScale(1.0, -1.0);
  mTranMatrix->AddTranslation(0, -mPSObj->mPrintSetup->height);

  return NS_OK;
}





NS_IMETHODIMP nsRenderingContextPS :: LockDrawingSurface(PRInt32 aX, PRInt32 aY,
                                                          PRUint32 aWidth, PRUint32 aHeight,
                                                          void **aBits, PRInt32 *aStride,
                                                          PRInt32 *aWidthBytes, PRUint32 aFlags)
{
  return NS_OK;
}





NS_IMETHODIMP nsRenderingContextPS :: UnlockDrawingSurface(void)
{
  return NS_OK;
}





NS_IMETHODIMP
nsRenderingContextPS :: SelectOffScreenDrawingSurface(nsIDrawingSurface* aSurface)
{
  return NS_OK;
}





NS_IMETHODIMP
nsRenderingContextPS :: GetDrawingSurface(nsIDrawingSurface* *aSurface)
{
  *aSurface = nsnull;
  return NS_OK;
}





NS_IMETHODIMP
nsRenderingContextPS :: GetHints(PRUint32& aResult)
{
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: Reset()
{
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: GetDeviceContext(nsIDeviceContext *&aContext)
{
  aContext = mContext;
  NS_IF_ADDREF(aContext);
  return NS_OK;
}






NS_IMETHODIMP 
nsRenderingContextPS :: PushState(void)
{
  PRInt32 cnt = mStateCache->Count();

  if (cnt == 0){
    if (nsnull == mStates)
      mStates = new PS_State();
    else
      mStates = new PS_State(*mStates);
  } else {
    PS_State *state = (PS_State *)mStateCache->ElementAt(cnt - 1);
    mStateCache->RemoveElementAt(cnt - 1);

    state->mNext = mStates;

    
    state->mMatrix = mStates->mMatrix;
    state->mLocalClip = mStates->mLocalClip;
    state->mCurrentColor = mStates->mCurrentColor;
    state->mFontMetrics = mStates->mFontMetrics;
    state->mTextColor = mStates->mTextColor;
    state->mLineStyle = mStates->mLineStyle;

    mStates = state;
  }

  mTranMatrix = &mStates->mMatrix;

  
  if(mPSObj)
    mPSObj->graphics_save();

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: PopState(void)
{
  if (nsnull == mStates){
    NS_ASSERTION(!(nsnull == mStates), "state underflow");
  } else {
    PS_State *oldstate = mStates;

    mStates = mStates->mNext;

    mStateCache->AppendElement(oldstate);

    if (nsnull != mStates){
      mTranMatrix = &mStates->mMatrix;
      SetLineStyle(mStates->mLineStyle);
    }
    else
      mTranMatrix = nsnull;
  }

  mPSObj->graphics_restore();

  return NS_OK;
}





NS_IMETHODIMP nsRenderingContextPS :: IsVisibleRect(const nsRect& aRect, PRBool &aVisible)
{
  aVisible = PR_TRUE;
  return NS_OK;
}





NS_IMETHODIMP nsRenderingContextPS :: SetClipRect(const nsRect& aRect, nsClipCombine aCombine)
{
  nsRect  trect = aRect;

  mStates->mLocalClip = aRect;

  mTranMatrix->TransformCoord(&trect.x, &trect.y,&trect.width, &trect.height);
  mStates->mFlags |= FLAG_LOCAL_CLIP_VALID;

  if (aCombine == nsClipCombine_kIntersect){
    mPSObj->newpath();
    mPSObj->box(trect.x, trect.y, trect.width, trect.height);
  } else if (aCombine == nsClipCombine_kUnion){
    mPSObj->newpath();
    mPSObj->box(trect.x, trect.y, trect.width, trect.height);
  }else if (aCombine == nsClipCombine_kSubtract){
    mPSObj->newpath();
    mPSObj->clippath();   
    mPSObj->box_subtract(trect.x, trect.y, trect.width, trect.height);
  }else if (aCombine == nsClipCombine_kReplace){
    mPSObj->initclip();
    mPSObj->newpath();
    mPSObj->box(trect.x, trect.y, trect.width, trect.height);
  }else{
    NS_ASSERTION(PR_FALSE, "illegal clip combination");
    return NS_ERROR_INVALID_ARG;
  }
  mPSObj->clip();
  mPSObj->newpath();

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: GetClipRect(nsRect &aRect, PRBool &aClipValid)
{
  if (mStates->mLocalClip.width !=0){
    aRect = mStates->mLocalClip;
    aClipValid = PR_TRUE;
  }else{
    aClipValid = PR_FALSE;
  }

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine)
{
  nsRect rect;
  nsIRegion* pRegion = (nsIRegion*)&aRegion;
  pRegion->GetBoundingBox(&rect.x, &rect.y, &rect.width, &rect.height);
  SetClipRect(rect, aCombine);

  return NS_OK; 
}





NS_IMETHODIMP 
nsRenderingContextPS :: CopyClipRegion(nsIRegion &aRegion)
{
  
  return NS_OK; 
}





NS_IMETHODIMP 
nsRenderingContextPS :: GetClipRegion(nsIRegion **aRegion)
{
  
  return NS_OK; 
}





NS_IMETHODIMP 
nsRenderingContextPS :: SetColor(nscolor aColor)
{
  mPSObj->setcolor(aColor);
  mCurrentColor = aColor;

  return NS_OK;
}





NS_IMETHODIMP nsRenderingContextPS :: GetColor(nscolor &aColor) const
{
  aColor = mCurrentColor;
  return NS_OK;
}





NS_IMETHODIMP nsRenderingContextPS :: SetLineStyle(nsLineStyle aLineStyle)
{
  mCurrLineStyle = aLineStyle;
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: GetLineStyle(nsLineStyle &aLineStyle)
{
  aLineStyle = mCurrLineStyle;
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS::SetFont(const nsFont& aFont, nsIAtom* aLangGroup)
{
  nsCOMPtr<nsIFontMetrics> newMetrics;
  nsresult rv = mContext->GetMetricsFor( aFont, aLangGroup, *getter_AddRefs(newMetrics) );

  if (NS_SUCCEEDED(rv)) {
    rv = SetFont(newMetrics);
  }
  return rv;
}





NS_IMETHODIMP 
nsRenderingContextPS::SetFont(nsIFontMetrics *aFontMetrics)
{
  mFontMetrics = (nsFontMetricsPS *)aFontMetrics;
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS::GetFontMetrics(nsIFontMetrics *&aFontMetrics)
{
  aFontMetrics = (nsIFontMetrics *)mFontMetrics;
  NS_IF_ADDREF(aFontMetrics);
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS::Translate(nscoord aX, nscoord aY)
{
  mTranMatrix->AddTranslation((float)aX,(float)aY);
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: Scale(float aSx, float aSy)
{
	mTranMatrix->AddScale(aSx, aSy);
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: GetCurrentTransform(nsTransform2D *&aTransform)
{
  aTransform = mTranMatrix;
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: CreateDrawingSurface(const nsRect& aBounds, PRUint32 aSurfFlags, nsIDrawingSurface* &aSurface)
{
  return NS_OK;   
}





NS_IMETHODIMP 
nsRenderingContextPS :: DestroyDrawingSurface(nsIDrawingSurface* aDS)
{
  return NS_OK;   
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  
  float scale;
  NS_REINTERPRET_CAST(DeviceContextImpl *, mContext.get())->GetCanonicalPixelScale(scale);
  int width = NSToCoordRound(TWIPS_PER_POINT_FLOAT * scale);

  
  
  
  if (aX0 == aX1) {
    
    
    return FillRect(aX0, aY0, width, aY1 - aY0);
  }
  else if (aY0 == aY1) {
    
    return FillRect(aX0, aY0, aX1 - aX0, width);
  }
  else {
    
    mTranMatrix->TransformCoord(&aX0,&aY0);
    mTranMatrix->TransformCoord(&aX1,&aY1);
    mPSObj->line(aX0, aY0, aX1, aY1, width);
    return NS_OK;
  }
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{

const nsPoint*  np;
nsPoint         pp;

  
  
  np = &aPoints[0];

  pp.x = np->x;
  pp.y = np->y;
  mTranMatrix->TransformCoord(&pp.x, &pp.y);
  mPSObj->moveto(pp.x, pp.y);
  np++;

  
	for (PRInt32 i = 1; i < aNumPoints; i++, np++){
		pp.x = np->x;
		pp.y = np->y;
                mTranMatrix->TransformCoord(&pp.x, &pp.y);
                mPSObj->lineto(pp.x, pp.y);
	}

  
  mPSObj->stroke();

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawRect(const nsRect& aRect)
{
  return DrawRect(aRect.x, aRect.y, aRect.width, aRect.height);
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{

	mTranMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
  mPSObj->newpath();
  mPSObj->box(aX, aY, aWidth, aHeight);
  mPSObj->stroke();
  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: FillRect(const nsRect& aRect)
{
  return FillRect(aRect.x, aRect.y, aRect.width, aRect.height);
}





NS_IMETHODIMP 
nsRenderingContextPS :: FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{

	mTranMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
  mPSObj->newpath();
  mPSObj->box(aX, aY, aWidth, aHeight);
  mPSObj->fill();
  return NS_OK;
}

NS_IMETHODIMP 
nsRenderingContextPS :: InvertRect(const nsRect& aRect)
{
	NS_NOTYETIMPLEMENTED("nsRenderingContextPS::InvertRect");

  return NS_OK;
}

NS_IMETHODIMP 
nsRenderingContextPS :: InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
	NS_NOTYETIMPLEMENTED("nsRenderingContextPS::InvertRect");

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
const nsPoint*  np;
nsPoint         pp;

  mPSObj->newpath();

  
  np = &aPoints[0];

  
	pp.x = np->x;
	pp.y = np->y;
  mTranMatrix->TransformCoord(&pp.x, &pp.y);
  mPSObj->moveto(pp.x, pp.y);
  np++;

  
	for (PRInt32 i = 1; i < aNumPoints; i++, np++){
		pp.x = np->x;
		pp.y = np->y;
                mTranMatrix->TransformCoord(&pp.x, &pp.y);
                mPSObj->lineto(pp.x, pp.y);
	}

  mPSObj->closepath();
  mPSObj->stroke();

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
const nsPoint*  np;
nsPoint         pp;

  mPSObj->newpath();

  
  np = &aPoints[0];

  
	pp.x = np->x;
	pp.y = np->y;
  mTranMatrix->TransformCoord(&pp.x, &pp.y);
  mPSObj->moveto(pp.x, pp.y);
  np++;

  
	for (PRInt32 i = 1; i < aNumPoints; i++, np++){
		pp.x = np->x;
		pp.y = np->y;
                mTranMatrix->TransformCoord(&pp.x, &pp.y);
                mPSObj->lineto(pp.x, pp.y);
	}

  mPSObj->closepath();
  mPSObj->fill();

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawEllipse(const nsRect& aRect)
{
  return DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  
  mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  mPSObj->comment("ellipse");
  mPSObj->newpath();
  mPSObj->moveto(aX, aY);
  mPSObj->arc(aWidth, aHeight, 0.0, 360.0);
  mPSObj->closepath();
  mPSObj->stroke();

  return NS_OK;
}

NS_IMETHODIMP 
nsRenderingContextPS :: FillEllipse(const nsRect& aRect)
{
  return FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}





NS_IMETHODIMP nsRenderingContextPS :: FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  mPSObj->comment("ellipse");
  mPSObj->newpath();
  mPSObj->moveto(aX, aY);
  mPSObj->arc(aWidth, aHeight, 0.0, 360.0);
  mPSObj->closepath();
  mPSObj->fill();

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  return DrawArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle);
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  
  mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  mPSObj->comment("arc");
  mPSObj->newpath();
  mPSObj->moveto(aX, aY);
  mPSObj->arc(aWidth, aHeight, aStartAngle, aEndAngle);
  mPSObj->closepath();
  mPSObj->stroke();

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: FillArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  return FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}





NS_IMETHODIMP 
nsRenderingContextPS :: FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  
  mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  mPSObj->comment("arc");
  mPSObj->newpath();
  mPSObj->moveto(aX, aY);
  mPSObj->arc(aWidth, aHeight, aStartAngle, aEndAngle);
  mPSObj->closepath();
  mPSObj->fill();

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: GetWidth(char ch, nscoord& aWidth)
{
  char buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth);
}





NS_IMETHODIMP 
nsRenderingContextPS::GetWidth(PRUnichar ch, nscoord &aWidth, PRInt32 *aFontID)
{
  PRUnichar buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth, aFontID);
}





NS_IMETHODIMP 
nsRenderingContextPS::GetWidth(const char* aString, nscoord& aWidth)
{
  return GetWidth(aString, strlen(aString),aWidth);
}





NS_IMETHODIMP 
nsRenderingContextPS::GetWidth(const char* aString,PRUint32 aLength,nscoord& aWidth)
{
  nsresult rv = NS_ERROR_FAILURE;
  if (mFontMetrics) {
    rv = NS_REINTERPRET_CAST(nsFontMetricsPS *, mFontMetrics.get())->GetStringWidth(aString,aWidth,aLength);
  }
  
  return rv;
}





NS_IMETHODIMP 
nsRenderingContextPS::GetWidth(const nsString& aString, nscoord& aWidth, PRInt32 *aFontID)
{
  return GetWidth(aString.get(), aString.Length(), aWidth, aFontID);
}





NS_IMETHODIMP 
nsRenderingContextPS :: GetWidth(const PRUnichar *aString,PRUint32 aLength,nscoord &aWidth, PRInt32 *aFontID)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mFontMetrics) {
    rv = NS_REINTERPRET_CAST(nsFontMetricsPS *, mFontMetrics.get())->GetStringWidth(aString, aWidth, aLength);
  }

  return rv;
}


NS_IMETHODIMP
nsRenderingContextPS::GetTextDimensions(const char*       aString,
                                        PRInt32           aLength,
                                        PRInt32           aAvailWidth,
                                        PRInt32*          aBreaks,
                                        PRInt32           aNumBreaks,
                                        nsTextDimensions& aDimensions,
                                        PRInt32&          aNumCharsFit,
                                        nsTextDimensions& aLastWordDimensions,
                                        PRInt32*          aFontID)
{
  NS_NOTYETIMPLEMENTED("nsRenderingContextPS::GetTextDimensions");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsRenderingContextPS::GetTextDimensions(const PRUnichar*  aString,
                                        PRInt32           aLength,
                                        PRInt32           aAvailWidth,
                                        PRInt32*          aBreaks,
                                        PRInt32           aNumBreaks,
                                        nsTextDimensions& aDimensions,
                                        PRInt32&          aNumCharsFit,
                                        nsTextDimensions& aLastWordDimensions,
                                        PRInt32*          aFontID)
{
  NS_NOTYETIMPLEMENTED("nsRenderingContextPS::GetTextDimensions");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsRenderingContextPS :: GetTextDimensions(const char* aString, PRUint32 aLength,
                                          nsTextDimensions& aDimensions)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mFontMetrics) {
    nsFontMetricsPS *metrics = NS_REINTERPRET_CAST(nsFontMetricsPS *, mFontMetrics.get());
    metrics->GetStringWidth(aString, aDimensions.width, aLength);
    metrics->GetMaxAscent(aDimensions.ascent);
    metrics->GetMaxDescent(aDimensions.descent);
    rv = NS_OK;
  }
  
  return rv;
}

NS_IMETHODIMP
nsRenderingContextPS :: GetTextDimensions(const PRUnichar* aString, PRUint32 aLength,
                                          nsTextDimensions& aDimensions, PRInt32* aFontID)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mFontMetrics) {
    nsFontMetricsPS *metrics = NS_REINTERPRET_CAST(nsFontMetricsPS *, mFontMetrics.get());
    metrics->GetStringWidth(aString, aDimensions.width, aLength);
     
    metrics->GetMaxAscent(aDimensions.ascent);
    metrics->GetMaxDescent(aDimensions.descent);
    rv = NS_OK;
  }

  return rv;
}





NS_IMETHODIMP
nsRenderingContextPS :: DrawString(const char *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        const nscoord* aSpacing)
{
  NS_ENSURE_TRUE(mTranMatrix && mPSObj && mFontMetrics, NS_ERROR_NULL_POINTER);

  nsFontMetricsPS *metrics = NS_REINTERPRET_CAST(nsFontMetricsPS *, mFontMetrics.get());
  NS_ENSURE_TRUE(metrics, NS_ERROR_FAILURE);

  
#if defined(MOZ_ENABLE_FREETYPE2) || defined(MOZ_ENABLE_XFT)
  if (!NS_REINTERPRET_CAST(nsDeviceContextPS *, mContext.get())->mFTPEnable) {
#endif
    nsCOMPtr<nsIAtom> langGroup;
    mFontMetrics->GetLangGroup(getter_AddRefs(langGroup));
    mPSObj->setlanggroup(langGroup);
#if defined(MOZ_ENABLE_FREETYPE2) || defined(MOZ_ENABLE_XFT)
  }
#endif

  if (aLength == 0)
    return NS_OK;
  nsFontPS* fontPS = nsFontPS::FindFont(aString[0], metrics->Font(), metrics);
  NS_ENSURE_TRUE(fontPS, NS_ERROR_FAILURE);
  fontPS->SetupFont(this);

  PRUint32 i, start = 0;
  for (i=0; i<aLength; i++) {
    nsFontPS* fontThisChar;
    fontThisChar = nsFontPS::FindFont(aString[i], metrics->Font(), metrics);
    NS_ENSURE_TRUE(fontThisChar, NS_ERROR_FAILURE);
    if (fontThisChar != fontPS) {
      
      aX += DrawString(aString+start, i-start, aX, aY, fontPS, 
                       aSpacing?aSpacing+start:nsnull);
      start = i;

      
      fontPS = fontThisChar;
      fontPS->SetupFont(this);
    }
  }

  
  if (aLength-start)
    DrawString(aString+start, aLength-start, aX, aY, fontPS, 
               aSpacing?aSpacing+start:nsnull);

  return NS_OK;
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawString(const PRUnichar *aString, PRUint32 aLength,
                                    nscoord aX, nscoord aY, PRInt32 aFontID,
                                    const nscoord* aSpacing)
{
  NS_ENSURE_TRUE(mTranMatrix && mPSObj && mFontMetrics, NS_ERROR_NULL_POINTER);
  
  nsFontMetricsPS *metrics = NS_REINTERPRET_CAST(nsFontMetricsPS *, mFontMetrics.get());
  NS_ENSURE_TRUE(metrics, NS_ERROR_FAILURE);

#if defined(MOZ_ENABLE_FREETYPE2) || defined(MOZ_ENABLE_XFT)
  
  if (!NS_REINTERPRET_CAST(nsDeviceContextPS *, mContext.get())->mFTPEnable) {
#endif
    nsCOMPtr<nsIAtom> langGroup = nsnull;
    mFontMetrics->GetLangGroup(getter_AddRefs(langGroup));
    mPSObj->setlanggroup(langGroup);
#if defined(MOZ_ENABLE_FREETYPE2) || defined(MOZ_ENABLE_XFT)
  }
#endif

  
  mPSObj->preshow(aString, aLength);

  if (aLength == 0)
    return NS_OK;
  nsFontPS* fontPS = nsFontPS::FindFont(aString[0], metrics->Font(), metrics);
  NS_ENSURE_TRUE(fontPS, NS_ERROR_FAILURE);
  fontPS->SetupFont(this);

  PRUint32 i, start = 0;
  for (i=0; i<aLength; i++) {
    nsFontPS* fontThisChar;
    fontThisChar = nsFontPS::FindFont(aString[i], metrics->Font(), metrics);
    NS_ENSURE_TRUE(fontThisChar, NS_ERROR_FAILURE);
    if (fontThisChar != fontPS) {
      
      aX += DrawString(aString+start, i-start, aX, aY, fontPS, 
                       aSpacing?aSpacing+start:nsnull);
      start = i;

      
      fontPS = fontThisChar;
      fontPS->SetupFont(this);
    }
  }

  
  if (aLength-start)
    DrawString(aString+start, aLength-start, aX, aY, fontPS, 
               aSpacing?aSpacing+start:nsnull);

  return NS_OK;
}

PRInt32 
nsRenderingContextPS::DrawString(const char *aString, PRUint32 aLength,
                                 nscoord &aX, nscoord &aY, nsFontPS* aFontPS,
                                 const nscoord* aSpacing)
{
  nscoord width = 0;
  PRInt32 x = aX;
  PRInt32 y = aY;

  PRInt32 dxMem[500];
  PRInt32* dx0 = 0;
  if (aSpacing) {
    dx0 = dxMem;
    if (aLength > 500) {
      dx0 = new PRInt32[aLength];
      NS_ENSURE_TRUE(dx0, NS_ERROR_OUT_OF_MEMORY);
    }
    mTranMatrix->ScaleXCoords(aSpacing, aLength, dx0);
  }

  mTranMatrix->TransformCoord(&x, &y);
  width = aFontPS->DrawString(this, x, y, aString, aLength);

  if ((aSpacing) && (dx0 != dxMem)) {
    delete [] dx0;
  }

  return width;
}


PRInt32 
nsRenderingContextPS::DrawString(const PRUnichar *aString, PRUint32 aLength,
                                 nscoord aX, nscoord aY, nsFontPS* aFontPS,
                                 const nscoord* aSpacing)
{
  nscoord width = 0;
  PRInt32 x = aX;
  PRInt32 y = aY;

  if (aSpacing) {
    
    const PRUnichar* end = aString + aLength;
    while (aString < end){
      x = aX;
      y = aY;
      mTranMatrix->TransformCoord(&x, &y);
      aFontPS->DrawString(this, x, y, aString, 1);
      aX += *aSpacing++;
      aString++;
    }
    width = aX;
  } else {
    mTranMatrix->TransformCoord(&x, &y);
    width = aFontPS->DrawString(this, x, y, aString, aLength);
  }

  return width;
}





NS_IMETHODIMP 
nsRenderingContextPS :: DrawString(const nsString& aString,nscoord aX, nscoord aY, PRInt32 aFontID,
                                    const nscoord* aSpacing)
{
  return DrawString(aString.get(), aString.Length(), aX, aY, aFontID, aSpacing);
}

NS_IMETHODIMP
nsRenderingContextPS::DrawImage(imgIContainer *aImage, const nsRect & aSrcRect, const nsRect & aDestRect)
{
  
  nsRect dr = aDestRect;
  mTranMatrix->TransformCoord(&dr.x, &dr.y, &dr.width, &dr.height);

  
  
  nsRect sr = aSrcRect;
  sr.x /= TWIPS_PER_POINT_INT;
  sr.y /= TWIPS_PER_POINT_INT;
  sr.width /= TWIPS_PER_POINT_INT;
  sr.height /= TWIPS_PER_POINT_INT;

  nsCOMPtr<gfxIImageFrame> iframe;
  aImage->GetCurrentFrame(getter_AddRefs(iframe));
  if (!iframe) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIImage> img(do_GetInterface(iframe));
  if (!img) return NS_ERROR_FAILURE;

  nsRect ir;
  iframe->GetRect(ir);
  mPSObj->draw_image(img, sr, ir, dr);
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextPS::DrawTile(imgIContainer *aImage,
                               nscoord aXImageStart,
                               nscoord aYImageStart,
                               const nsRect *aTargetRect)
{
  
  nscoord width, height;
  aImage->GetWidth(&width);
  aImage->GetHeight(&height);
  
  
  nsRect imgRect;
  imgRect.x = 0;
  imgRect.y = 0;
  imgRect.width = width;
  imgRect.height = height;
  
  
  width = NSToCoordRound(width*mP2T);
  height = NSToCoordRound(height*mP2T);

  
  nsCOMPtr<gfxIImageFrame> iframe;
  aImage->GetCurrentFrame(getter_AddRefs(iframe));
  if (!iframe) return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIImage> img(do_GetInterface(iframe));
  if (!img) return NS_ERROR_FAILURE;
  nsRect ir;
  iframe->GetRect(ir);
   
  
  mPSObj->graphics_save();

  
  nsRect targetRect = (*aTargetRect);
  mTranMatrix->TransformCoord(&targetRect.x, &targetRect.y, 
                              &targetRect.width, &targetRect.height);
  mPSObj->box(targetRect.x,targetRect.y,targetRect.width,targetRect.height);
  mPSObj->clip();

  
  nsRect dstRect;
  for(PRInt32 y = aYImageStart; y < aTargetRect->y + aTargetRect->height; y += height)
    for(PRInt32 x = aXImageStart; x < aTargetRect->x + aTargetRect->width; x += width)
    {
      dstRect.x = x;
      dstRect.y = y;
      dstRect.width = width;
      dstRect.height = height;
      mTranMatrix->TransformCoord(&dstRect.x, &dstRect.y, &dstRect.width, &dstRect.height);
      mPSObj->draw_image(img, imgRect, ir, dstRect);
    }

  
  mPSObj->graphics_restore();

  return NS_OK;
}


#ifdef MOZ_MATHML
  


NS_IMETHODIMP 
nsRenderingContextPS::GetBoundingMetrics(const char*        aString,
                                         PRUint32           aLength,
                                         nsBoundingMetrics& aBoundingMetrics)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

  


NS_IMETHODIMP 
nsRenderingContextPS::GetBoundingMetrics(const PRUnichar*   aString,
                                         PRUint32           aLength,
                                         nsBoundingMetrics& aBoundingMetrics,
                                         PRInt32*           aFontID)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}
#endif 





NS_IMETHODIMP nsRenderingContextPS :: CopyOffScreenBits(nsIDrawingSurface* aSrcSurf,
                                                         PRInt32 aSrcX, PRInt32 aSrcY,
                                                         const nsRect &aDestBounds,
                                                         PRUint32 aCopyFlags)
{
  return NS_OK;
}









NS_IMETHODIMP nsRenderingContextPS::RenderEPS(const nsRect& aRect, FILE *aDataFile)
{
  nsresult    rv;

  

  if ((aRect.width == 0) || (aRect.height == 0))
    return NS_OK;

  nsEPSObjectPS eps(aDataFile);
  if (NS_FAILED(eps.GetStatus())) {
    return NS_ERROR_INVALID_ARG;
  }
 
  nsRect trect = aRect;
  mTranMatrix->TransformCoord(&trect.x, &trect.y, &trect.width, &trect.height);
 
  rv = mPSObj->render_eps(trect, eps);

  return rv;
}

#ifdef NOTNOW
HPEN nsRenderingContextPS :: SetupSolidPen(void)
{
  return mCurrPen;
}

HPEN nsRenderingContextPS :: SetupDashedPen(void)
{
  return mCurrPen;
}

HPEN nsRenderingContextPS :: SetupDottedPen(void)
{
  return mCurrPen;
}

#endif 

