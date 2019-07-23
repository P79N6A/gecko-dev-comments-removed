





































#include "nsRenderingContextWin.h"
#include "nsICharRepresentable.h"
#include "nsFontMetricsWin.h"
#include "nsRegionWin.h"
#include <math.h>
#include "nsDeviceContextWin.h"
#include "prprf.h"
#include "nsDrawingSurfaceWin.h"
#include "nsGfxCIID.h"
#include "nsUnicharUtils.h"



#ifdef GFX_DEBUG
  #define BREAK_TO_DEBUGGER           DebugBreak()
#else   
  #define BREAK_TO_DEBUGGER
#endif  

#ifdef GFX_DEBUG
  #define VERIFY(exp)                 ((exp) ? 0: (GetLastError(), BREAK_TO_DEBUGGER))
#else   
  #define VERIFY(exp)                 (exp)
#endif  


static NS_DEFINE_IID(kIRenderingContextIID, NS_IRENDERING_CONTEXT_IID);
static NS_DEFINE_IID(kIRenderingContextWinIID, NS_IRENDERING_CONTEXT_WIN_IID);

#define FLAG_CLIP_VALID       0x0001
#define FLAG_CLIP_CHANGED     0x0002
#define FLAG_LOCAL_CLIP_VALID 0x0004

#define FLAGS_ALL             (FLAG_CLIP_VALID | FLAG_CLIP_CHANGED | FLAG_LOCAL_CLIP_VALID)




#define PALETTERGB_COLORREF(c)  (0x02000000 | (c))

typedef struct lineddastructtag
{
   int   nDottedPixel;
   HDC   dc;
   COLORREF crColor;
} lineddastruct;

void CALLBACK LineDDAFunc(int x,int y,LONG lData)
{
  lineddastruct * dda_struct = (lineddastruct *) lData;
  
  dda_struct->nDottedPixel ^= 1; 

  if (dda_struct->nDottedPixel)
    SetPixel(dda_struct->dc, x, y, dda_struct->crColor);
}   



class GraphicsState
{
public:
  GraphicsState();
  GraphicsState(GraphicsState &aState);
  ~GraphicsState();

  GraphicsState   *mNext;
  nsTransform2D   mMatrix;
  nsRect          mLocalClip;
  HRGN            mClipRegion;
  nscolor         mColor;
  COLORREF        mColorREF;
  nsIFontMetrics  *mFontMetrics;
  HPEN            mSolidPen;
  HPEN            mDashedPen;
  HPEN            mDottedPen;
  PRInt32         mFlags;
  nsLineStyle     mLineStyle;
};

GraphicsState :: GraphicsState()
{
  mNext = nsnull;
  mMatrix.SetToIdentity();
  mLocalClip.x = mLocalClip.y = mLocalClip.width = mLocalClip.height = 0;
  mClipRegion = NULL;
  mColor = NS_RGB(0, 0, 0);
  mColorREF = RGB(0, 0, 0);
  mFontMetrics = nsnull;
  mSolidPen = NULL;
  mDashedPen = NULL;
  mDottedPen = NULL;
  mFlags = ~FLAGS_ALL;
  mLineStyle = nsLineStyle_kSolid;
}

GraphicsState :: GraphicsState(GraphicsState &aState) :
                               mMatrix(&aState.mMatrix),
                               mLocalClip(aState.mLocalClip)
{
  mNext = &aState;
  mClipRegion = NULL;
  mColor = NS_RGB(0, 0, 0);
  mColorREF = RGB(0, 0, 0);
  mFontMetrics = nsnull;
  mSolidPen = NULL;
  mDashedPen = NULL;
  mDottedPen = NULL;
  mFlags = ~FLAGS_ALL;
  mLineStyle = aState.mLineStyle;
}

GraphicsState :: ~GraphicsState()
{
  if (NULL != mClipRegion)
  {
    VERIFY(::DeleteObject(mClipRegion));
    mClipRegion = NULL;
  }

  
  mSolidPen = NULL;
  mDashedPen = NULL;
  mDottedPen = NULL;
}


#define NOT_SETUP 0x33
static PRBool gIsWIN95 = NOT_SETUP;

#define DONT_INIT 0
static DWORD gBidiInfo = NOT_SETUP;



static HFONT  gStockSystemFont = (HFONT)::GetStockObject(SYSTEM_FONT);
static HBRUSH gStockWhiteBrush = (HBRUSH)::GetStockObject(WHITE_BRUSH);
static HPEN   gStockBlackPen = (HPEN)::GetStockObject(BLACK_PEN);
static HPEN   gStockWhitePen = (HPEN)::GetStockObject(WHITE_PEN);

nsRenderingContextWin :: nsRenderingContextWin()
{
#ifdef WINCE
  gIsWIN95 = PR_FALSE;
  gBidiInfo = DONT_INIT;
#else
  
  if (NOT_SETUP == gIsWIN95) {
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(os);
    ::GetVersionEx(&os);
    
    if (VER_PLATFORM_WIN32_NT == os.dwPlatformId) {
      gIsWIN95 = PR_FALSE;
    }
    else {
      gIsWIN95 = PR_TRUE;

      if ( (os.dwMajorVersion < 4)
           || ( (os.dwMajorVersion == 4) && (os.dwMinorVersion == 0) ) ) {
        
        gBidiInfo = DONT_INIT;
      }
      else if (os.dwMajorVersion >= 4) {
        
        UINT cp = ::GetACP();
        if (1256 == cp) {
          gBidiInfo = GCP_REORDER | GCP_GLYPHSHAPE;
        }
        else if (1255 == cp) {
          gBidiInfo = GCP_REORDER;
        }
      }
    }
  }
#endif

  mDC = NULL;
  mMainDC = NULL;
  mDCOwner = nsnull;
  mFontMetrics = nsnull;
  mOrigSolidBrush = NULL;
  mOrigFont = NULL;
  mOrigSolidPen = NULL;
  mCurrBrushColor = RGB(255, 255, 255);
  mCurrFontWin = nsnull;
  mCurrPenColor = NULL;
  mCurrPen = NULL;
  mNullPen = NULL;
  mCurrTextColor = RGB(0, 0, 0);
  mCurrLineStyle = nsLineStyle_kSolid;
#ifdef NS_DEBUG
  mInitialized = PR_FALSE;
#endif
  mSurface = nsnull;
  mMainSurface = nsnull;

  mStateCache = new nsVoidArray();
  mRightToLeftText = PR_FALSE;

  

  PushState();

  mP2T = 1.0f;
}

nsRenderingContextWin :: ~nsRenderingContextWin()
{
  NS_IF_RELEASE(mContext);
  NS_IF_RELEASE(mFontMetrics);

  

  PopState();

  
  
  

  if (NULL != mDC)
  {
    if (NULL != mOrigSolidBrush)
    {
      ::SelectObject(mDC, mOrigSolidBrush);
      mOrigSolidBrush = NULL;
    }

    if (NULL != mOrigFont)
    {
      ::SelectObject(mDC, mOrigFont);
      mOrigFont = NULL;
    }

    if (NULL != mOrigSolidPen)
    {
      ::SelectObject(mDC, mOrigSolidPen);
      mOrigSolidPen = NULL;
    }
  }

  mCurrBrush = NULL;   

  
  mCurrFont = NULL;

  if (mCurrPen && (mCurrPen != gStockBlackPen) && (mCurrPen != gStockWhitePen)) {
    VERIFY(::DeleteObject(mCurrPen));
  }

  if ((NULL != mNullPen) && (mNullPen != mCurrPen)) {
    VERIFY(::DeleteObject(mNullPen));
  }

  mCurrPen = NULL;
  mNullPen = NULL;

  if (nsnull != mStateCache)
  {
    PRInt32 cnt = mStateCache->Count();

    while (--cnt >= 0)
    {
      GraphicsState *state = (GraphicsState *)mStateCache->ElementAt(cnt);
      mStateCache->RemoveElementAt(cnt);

      if (nsnull != state)
        delete state;
    }

    delete mStateCache;
    mStateCache = nsnull;
  }

  if (nsnull != mSurface)
  {
    mSurface->ReleaseDC();
    NS_RELEASE(mSurface);
  }

  if (nsnull != mMainSurface)
  {
    mMainSurface->ReleaseDC();
    NS_RELEASE(mMainSurface);
  }

  if (nsnull != mDCOwner)
  {
    ::ReleaseDC((HWND)mDCOwner->GetNativeData(NS_NATIVE_WINDOW), mMainDC);
    NS_RELEASE(mDCOwner);
  }

  mTranMatrix = nsnull;
  mDC = NULL;
  mMainDC = NULL;
}

nsresult
nsRenderingContextWin :: QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr)
    return NS_ERROR_NULL_POINTER;

  if (aIID.Equals(kIRenderingContextIID))
  {
    nsIRenderingContext* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(kIRenderingContextWinIID))
  {
    nsIRenderingContextWin* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

  if (aIID.Equals(kISupportsIID))
  {
    nsIRenderingContext* tmp = this;
    nsISupports* tmp2 = tmp;
    *aInstancePtr = (void*) tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsRenderingContextWin)
NS_IMPL_RELEASE(nsRenderingContextWin)

NS_IMETHODIMP
nsRenderingContextWin :: Init(nsIDeviceContext* aContext,
                              nsIWidget *aWindow)
{
  NS_PRECONDITION(PR_FALSE == mInitialized, "double init");

  mContext = aContext;
  NS_IF_ADDREF(mContext);

  mSurface = (nsDrawingSurfaceWin *)new nsDrawingSurfaceWin();

  if (nsnull != mSurface)
  {
    HDC tdc = (HDC)aWindow->GetNativeData(NS_NATIVE_GRAPHIC);

    NS_ADDREF(mSurface);
    mSurface->Init(tdc);
    mDC = tdc;

    mMainDC = mDC;
    mMainSurface = mSurface;
    NS_ADDREF(mMainSurface);
  }

  mDCOwner = aWindow;

  NS_IF_ADDREF(mDCOwner);

  return CommonInit();
}

NS_IMETHODIMP
nsRenderingContextWin :: Init(nsIDeviceContext* aContext,
                              nsIDrawingSurface* aSurface)
{
  NS_PRECONDITION(PR_FALSE == mInitialized, "double init");

  mContext = aContext;
  NS_IF_ADDREF(mContext);

  mSurface = (nsDrawingSurfaceWin *)aSurface;

  if (nsnull != mSurface)
  {
    NS_ADDREF(mSurface);
    mSurface->GetDC(&mDC);

    mMainDC = mDC;
    mMainSurface = mSurface;
    NS_ADDREF(mMainSurface);
  }

  mDCOwner = nsnull;

  return CommonInit();
}

nsresult nsRenderingContextWin :: SetupDC(HDC aOldDC, HDC aNewDC)
{
  HBRUSH  prevbrush;
  HFONT   prevfont;
  HPEN    prevpen;

  ::SetTextColor(aNewDC, PALETTERGB_COLORREF(mColor));
  mCurrTextColor = mCurrentColor;
  ::SetBkMode(aNewDC, TRANSPARENT);
#ifndef WINCE
  ::SetPolyFillMode(aNewDC, WINDING);
#endif
  
  
  nsPaletteInfo palInfo;
  mContext->GetPaletteInfo(palInfo);
#ifndef WINCE
  if (palInfo.isPaletteDevice)
    ::SetStretchBltMode(aNewDC, HALFTONE);
  else
    ::SetStretchBltMode(aNewDC, COLORONCOLOR);                                  
#endif

  ::SetTextAlign(aNewDC, TA_BASELINE);

  if (nsnull != aOldDC)
  {
    if (nsnull != mOrigSolidBrush)
      prevbrush = (HBRUSH)::SelectObject(aOldDC, mOrigSolidBrush);

    if (nsnull != mOrigFont)
      prevfont = (HFONT)::SelectObject(aOldDC, mOrigFont);

    if (nsnull != mOrigSolidPen)
      prevpen = (HPEN)::SelectObject(aOldDC, mOrigSolidPen);

  }
  else
  {
    prevbrush = gStockWhiteBrush;
    prevfont = gStockSystemFont;
    prevpen = gStockBlackPen;
  }

  mOrigSolidBrush = (HBRUSH)::SelectObject(aNewDC, prevbrush);
  mOrigFont = (HFONT)::SelectObject(aNewDC, prevfont);
  mOrigSolidPen = (HPEN)::SelectObject(aNewDC, prevpen);

#if 0
  GraphicsState *pstate = mStates;

  while ((nsnull != pstate) && !(pstate->mFlags & FLAG_CLIP_VALID))
    pstate = pstate->mNext;

  if (nsnull != pstate)
    ::SelectClipRgn(aNewDC, pstate->mClipRegion);
#endif
  
  
  if (palInfo.isPaletteDevice && palInfo.palette)
  {
    
    ::SelectPalette(aNewDC, (HPALETTE)palInfo.palette, PR_TRUE);
    ::RealizePalette(aNewDC);
  }

  return NS_OK;
}

nsresult nsRenderingContextWin :: CommonInit(void)
{
  float app2dev;

  app2dev = mContext->AppUnitsToDevUnits();
	mTranMatrix->AddScale(app2dev, app2dev);
  mP2T = mContext->DevUnitsToAppUnits();

#ifdef NS_DEBUG
  mInitialized = PR_TRUE;
#endif

  return SetupDC(nsnull, mDC);
}

NS_IMETHODIMP nsRenderingContextWin :: LockDrawingSurface(PRInt32 aX, PRInt32 aY,
                                                          PRUint32 aWidth, PRUint32 aHeight,
                                                          void **aBits, PRInt32 *aStride,
                                                          PRInt32 *aWidthBytes, PRUint32 aFlags)
{
  PRBool        destructive;
  nsPaletteInfo palInfo;

  PushState();

  mSurface->IsReleaseDCDestructive(&destructive);

  if (destructive)
  {
    PushClipState();

    if (nsnull != mOrigSolidBrush)
      mCurrBrush = (HBRUSH)::SelectObject(mDC, mOrigSolidBrush);

    if (nsnull != mOrigFont)
      mCurrFont = (HFONT)::SelectObject(mDC, mOrigFont);

    if (nsnull != mOrigSolidPen)
      mCurrPen = (HPEN)::SelectObject(mDC, mOrigSolidPen);

    mContext->GetPaletteInfo(palInfo);
    if(palInfo.isPaletteDevice && palInfo.palette){
      ::SelectPalette(mDC,(HPALETTE)palInfo.palette,PR_TRUE);
      ::RealizePalette(mDC);
#ifndef WINCE
      ::UpdateColors(mDC);                                                      
#endif
    }
  }

  mSurface->ReleaseDC();

  return mSurface->Lock(aX, aY, aWidth, aHeight, aBits, aStride, aWidthBytes, aFlags);
}

NS_IMETHODIMP nsRenderingContextWin :: UnlockDrawingSurface(void)
{
  PRBool  clipstate;

  mSurface->Unlock();
  mSurface->GetDC(&mDC);

  PopState();

  mSurface->IsReleaseDCDestructive(&clipstate);

  if (clipstate)
  {
    ::SetTextColor(mDC, PALETTERGB_COLORREF(mColor));
    mCurrTextColor = mCurrentColor;

    ::SetBkMode(mDC, TRANSPARENT);
#ifndef WINCE
    ::SetPolyFillMode(mDC, WINDING);
    ::SetStretchBltMode(mDC, COLORONCOLOR);
#endif

    mOrigSolidBrush = (HBRUSH)::SelectObject(mDC, mCurrBrush);
    mOrigFont = (HFONT)::SelectObject(mDC, mCurrFont);
    mOrigSolidPen = (HPEN)::SelectObject(mDC, mCurrPen);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextWin :: SelectOffScreenDrawingSurface(nsIDrawingSurface* aSurface)
{
  nsresult  rv;

  

  if (aSurface != mSurface)
  {
    if (nsnull != aSurface)
    {
      HDC tdc;

      
      ((nsDrawingSurfaceWin *)aSurface)->GetDC(&tdc);

      rv = SetupDC(mDC, tdc);

      
      mSurface->ReleaseDC();

      NS_IF_RELEASE(mSurface);
      mSurface = (nsDrawingSurfaceWin *)aSurface;
    }
    else
    {
      if (NULL != mDC)
      {
        rv = SetupDC(mDC, mMainDC);

        
        mSurface->ReleaseDC();

        NS_IF_RELEASE(mSurface);
        mSurface = mMainSurface;
      }
    }

    NS_ADDREF(mSurface);
    mSurface->GetDC(&mDC);
  }
  else
    rv = NS_OK;

  return rv;
}

NS_IMETHODIMP
nsRenderingContextWin :: GetDrawingSurface(nsIDrawingSurface* *aSurface)
{
  *aSurface = mSurface;
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextWin :: GetHints(PRUint32& aResult)
{
  PRUint32 result = 0;
  
#ifndef WINCE
  result |= NS_RENDERING_HINT_FAST_MEASURE;
#endif

  if (gIsWIN95)
    result |= NS_RENDERING_HINT_FAST_8BIT_TEXT;

  if (NOT_SETUP == gBidiInfo) {
    InitBidiInfo();
  }
#ifndef WINCE
  if (GCP_REORDER == (gBidiInfo & GCP_REORDER) )
    result |= NS_RENDERING_HINT_BIDI_REORDERING;
  if (GCP_GLYPHSHAPE == (gBidiInfo & GCP_GLYPHSHAPE) )
    result |= NS_RENDERING_HINT_ARABIC_SHAPING;
#endif
  
  aResult = result;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: Reset()
{
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetDeviceContext(nsIDeviceContext *&aContext)
{
  NS_IF_ADDREF(mContext);
  aContext = mContext;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: PushState(void)
{
  PRInt32 cnt = mStateCache->Count();

  if (cnt == 0)
  {
    if (nsnull == mStates)
      mStates = new GraphicsState();
    else
      mStates = new GraphicsState(*mStates);
  }
  else
  {
    GraphicsState *state = (GraphicsState *)mStateCache->ElementAt(cnt - 1);
    mStateCache->RemoveElementAt(cnt - 1);

    state->mNext = mStates;

    

    state->mMatrix = mStates->mMatrix;
    state->mLocalClip = mStates->mLocalClip;




    state->mSolidPen = NULL;
    state->mDashedPen = NULL;
    state->mDottedPen = NULL;
    state->mFlags = ~FLAGS_ALL;
    state->mLineStyle = mStates->mLineStyle;

    mStates = state;
  }

  if (nsnull != mStates->mNext)
  {
    mStates->mNext->mColor = mCurrentColor;
    mStates->mNext->mColorREF = mColor;
    mStates->mNext->mFontMetrics = mFontMetrics;
    NS_IF_ADDREF(mStates->mNext->mFontMetrics);
  }

  mTranMatrix = &mStates->mMatrix;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: PopState(void)
{
  if (nsnull == mStates)
  {
    NS_ASSERTION(!(nsnull == mStates), "state underflow");
  }
  else
  {
    GraphicsState *oldstate = mStates;

    mStates = mStates->mNext;

    mStateCache->AppendElement(oldstate);

    if (nsnull != mStates)
    {
      mTranMatrix = &mStates->mMatrix;

      GraphicsState *pstate;

      if (oldstate->mFlags & FLAG_CLIP_CHANGED)
      {
        pstate = mStates;

        
        

        while ((nsnull != pstate) && !(pstate->mFlags & FLAG_CLIP_VALID))
          pstate = pstate->mNext;

        if (nsnull != pstate)
          ::SelectClipRgn(mDC, pstate->mClipRegion);
      }

      oldstate->mFlags &= ~FLAGS_ALL;
      oldstate->mSolidPen = NULL;
      oldstate->mDashedPen = NULL;
      oldstate->mDottedPen = NULL;

      NS_IF_RELEASE(mFontMetrics);
      mFontMetrics = mStates->mFontMetrics;

      mCurrentColor = mStates->mColor;
      mColor = mStates->mColorREF;

      SetLineStyle(mStates->mLineStyle);
    }
    else
      mTranMatrix = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: IsVisibleRect(const nsRect& aRect, PRBool &aVisible)
{
  aVisible = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetClipRect(const nsRect& aRect, nsClipCombine aCombine)
{
  nsRect  trect = aRect;

  mStates->mLocalClip = aRect;

	mTranMatrix->TransformCoord(&trect.x, &trect.y,
                           &trect.width, &trect.height);

  RECT nr;
  ConditionRect(trect, nr);

  mStates->mFlags |= FLAG_LOCAL_CLIP_VALID;

  

  if (aCombine == nsClipCombine_kIntersect)
  {
    PushClipState();

    ::IntersectClipRect(mDC, nr.left,
                        nr.top,
                        nr.right,
                        nr.bottom);
  }
  else if (aCombine == nsClipCombine_kUnion)
  {
    PushClipState();

    HRGN  tregion = ::CreateRectRgn(nr.left,
                                    nr.top,
                                    nr.right,
                                    nr.bottom);

    ::ExtSelectClipRgn(mDC, tregion, RGN_OR);
    ::DeleteObject(tregion);
  }
  else if (aCombine == nsClipCombine_kSubtract)
  {
    PushClipState();

    ::ExcludeClipRect(mDC, nr.left,
                      nr.top,
                      nr.right,
                      nr.bottom);
  }
  else if (aCombine == nsClipCombine_kReplace)
  {
    PushClipState();

    HRGN  tregion = ::CreateRectRgn(nr.left,
                                    nr.top,
                                    nr.right,
                                    nr.bottom);
    ::SelectClipRgn(mDC, tregion);
    ::DeleteObject(tregion);
  }
  else
    NS_ASSERTION(PR_FALSE, "illegal clip combination");

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetClipRect(nsRect &aRect, PRBool &aClipValid)
{
  if (mStates->mFlags & FLAG_LOCAL_CLIP_VALID)
  {
    aRect = mStates->mLocalClip;
    aClipValid = PR_TRUE;
  }
  else
    aClipValid = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine)
{
  HRGN        hrgn;
  int         cmode;

  aRegion.GetNativeRegion((void *&)hrgn);

  switch (aCombine)
  {
    case nsClipCombine_kIntersect:
      cmode = RGN_AND;
      break;

    case nsClipCombine_kUnion:
      cmode = RGN_OR;
      break;

    case nsClipCombine_kSubtract:
      cmode = RGN_DIFF;
      break;

    default:
    case nsClipCombine_kReplace:
      cmode = RGN_COPY;
      break;
  }

  if (NULL != hrgn)
  {
    mStates->mFlags &= ~FLAG_LOCAL_CLIP_VALID;
    PushClipState();
    ::ExtSelectClipRgn(mDC, hrgn, cmode);
  }
  else
    return PR_FALSE;

  return NS_OK;
}




NS_IMETHODIMP nsRenderingContextWin::CopyClipRegion(nsIRegion &aRegion)
{
  int rstat = ::GetClipRgn(mDC, ((nsRegionWin *)&aRegion)->mRegion);

  if (rstat == 1)
  {
    
    
    

    ((nsRegionWin *)&aRegion)->mRegionType = eRegionComplexity_complex;
  }
 
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetClipRegion(nsIRegion **aRegion)
{
  nsresult  rv = NS_OK;

  NS_ASSERTION(!(nsnull == aRegion), "no region ptr");

  if (nsnull == *aRegion)
  {
    nsRegionWin *rgn = new nsRegionWin();

    if (nsnull != rgn)
    {
      NS_ADDREF(rgn);

      rv = rgn->Init();

      if (NS_OK != rv)
        NS_RELEASE(rgn);
      else
        *aRegion = rgn;
    }
    else
      rv = NS_ERROR_OUT_OF_MEMORY;
  }

  if (rv == NS_OK) {
    rv = CopyClipRegion(**aRegion);
  }

  return rv;
}

NS_IMETHODIMP nsRenderingContextWin :: SetColor(nscolor aColor)
{
  mCurrentColor = aColor;
  mColor = RGB(NS_GET_R(aColor),
               NS_GET_G(aColor),
               NS_GET_B(aColor));
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetColor(nscolor &aColor) const
{
  aColor = mCurrentColor;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetLineStyle(nsLineStyle aLineStyle)
{
  mCurrLineStyle = aLineStyle;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetLineStyle(nsLineStyle &aLineStyle)
{
  aLineStyle = mCurrLineStyle;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetFont(const nsFont& aFont, nsIAtom* aLangGroup)
{
  mCurrFontWin = nsnull; 
  NS_IF_RELEASE(mFontMetrics);
  mContext->GetMetricsFor(aFont, aLangGroup, mFontMetrics);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: SetFont(nsIFontMetrics *aFontMetrics)
{
  mCurrFontWin = nsnull; 
  NS_IF_RELEASE(mFontMetrics);
  mFontMetrics = aFontMetrics;
  NS_IF_ADDREF(mFontMetrics);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetFontMetrics(nsIFontMetrics *&aFontMetrics)
{
  NS_IF_ADDREF(mFontMetrics);
  aFontMetrics = mFontMetrics;

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextWin :: Translate(nscoord aX, nscoord aY)
{
	mTranMatrix->AddTranslation((float)aX,(float)aY);
  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextWin :: Scale(float aSx, float aSy)
{
	mTranMatrix->AddScale(aSx, aSy);
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: GetCurrentTransform(nsTransform2D *&aTransform)
{
  aTransform = mTranMatrix;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: CreateDrawingSurface(const nsRect& aBounds, PRUint32 aSurfFlags, nsIDrawingSurface* &aSurface)
{
  nsDrawingSurfaceWin *surf = new nsDrawingSurfaceWin();

  if (nsnull != surf)
  {
    NS_ADDREF(surf);

    surf->Init(mMainDC, aBounds.width, aBounds.height, aSurfFlags);
  }

  aSurface = surf;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DestroyDrawingSurface(nsIDrawingSurface* aDS)
{
  nsDrawingSurfaceWin *surf = (nsDrawingSurfaceWin *)aDS;

  
  if (surf == mSurface)
  {
    
    NS_IF_RELEASE(mSurface);

    mDC = mMainDC;
    mSurface = mMainSurface;

    
    NS_IF_ADDREF(mSurface);
  }

  
  NS_IF_RELEASE(surf);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

	mTranMatrix->TransformCoord(&aX0,&aY0);
	mTranMatrix->TransformCoord(&aX1,&aY1);

  SetupPen();

#ifndef WINCE
  if (nsLineStyle_kDotted == mCurrLineStyle)
  {
    lineddastruct dda_struct;

    dda_struct.nDottedPixel = 1;
    dda_struct.dc = mDC;
    dda_struct.crColor = mColor;

    LineDDA((int)(aX0),(int)(aY0),(int)(aX1),(int)(aY1),(LINEDDAPROC) LineDDAFunc,(long)&dda_struct);
  }
  else
#endif
  {
    ::MoveToEx(mDC, (int)(aX0), (int)(aY0), NULL);
    ::LineTo(mDC, (int)(aX1), (int)(aY1));
  }

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextWin :: DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  
  
  POINT pts[20];
  POINT* pp0 = pts;

  if (aNumPoints > 20)
    pp0 = new POINT[aNumPoints];

  POINT* pp = pp0;
  const nsPoint* np = &aPoints[0];

	for (PRInt32 i = 0; i < aNumPoints; i++, pp++, np++)
  {
		pp->x = np->x;
		pp->y = np->y;
		mTranMatrix->TransformCoord((int*)&pp->x,(int*)&pp->y);
	}

  
  SetupPen();
  ::Polyline(mDC, pp0, int(aNumPoints));

  
  if (pp0 != pts)
    delete [] pp0;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawRect(const nsRect& aRect)
{
  RECT nr;
	nsRect	tr;

	tr = aRect;
	mTranMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
	nr.left = tr.x;
	nr.top = tr.y;
	nr.right = tr.x+tr.width;
	nr.bottom = tr.y+tr.height;

  ::FrameRect(mDC, &nr, SetupSolidBrush());

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  RECT nr;

	mTranMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
	nr.left = aX;
	nr.top = aY;
	nr.right = aX+aWidth;
	nr.bottom = aY+aHeight;

  ::FrameRect(mDC, &nr, SetupSolidBrush());

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillRect(const nsRect& aRect)
{
  RECT nr;
	nsRect tr;

	tr = aRect;
	mTranMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
  ConditionRect(tr, nr);
  ::FillRect(mDC, &nr, SetupSolidBrush());

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  RECT nr;
	nsRect	tr;

	mTranMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
	nr.left = aX;
	nr.top = aY;
	nr.right = aX+aWidth;
	nr.bottom = aY+aHeight;

  ::FillRect(mDC, &nr, SetupSolidBrush());

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: InvertRect(const nsRect& aRect)
{
  RECT nr;
	nsRect tr;

	tr = aRect;
	mTranMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
  ConditionRect(tr, nr);
  ::InvertRect(mDC, &nr);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  RECT nr;
	nsRect	tr;

	mTranMatrix->TransformCoord(&aX,&aY,&aWidth,&aHeight);
	nr.left = aX;
	nr.top = aY;
	nr.right = aX+aWidth;
	nr.bottom = aY+aHeight;

  ::InvertRect(mDC, &nr);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  
  
  POINT pts[20];
  POINT* pp0 = pts;

  if (aNumPoints > 20)
    pp0 = new POINT[aNumPoints];

  POINT* pp = pp0;
  const nsPoint* np = &aPoints[0];

	for (PRInt32 i = 0; i < aNumPoints; i++, pp++, np++)
  {
		pp->x = np->x;
		pp->y = np->y;
		mTranMatrix->TransformCoord((int*)&pp->x,(int*)&pp->y);
	}

  
  SetupSolidPen();
  HBRUSH oldBrush = (HBRUSH)::SelectObject(mDC, ::GetStockObject(NULL_BRUSH));
  ::Polygon(mDC, pp0, int(aNumPoints));
  ::SelectObject(mDC, oldBrush);

  
  if (pp0 != pts)
    delete [] pp0;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  
  

  POINT pts[20];
  POINT* pp0 = pts;

  if (aNumPoints > 20)
    pp0 = new POINT[aNumPoints];

  POINT* pp = pp0;
  const nsPoint* np = &aPoints[0];

	for (PRInt32 i = 0; i < aNumPoints; i++, pp++, np++)
	{
		pp->x = np->x;
		pp->y = np->y;
		mTranMatrix->TransformCoord((int*)&pp->x,(int*)&pp->y);
	}

  
  SetupSolidBrush();

  if (NULL == mNullPen)
    mNullPen = ::CreatePen(PS_NULL, 0, 0);

  HPEN oldPen = (HPEN)::SelectObject(mDC, mNullPen);
  ::Polygon(mDC, pp0, int(aNumPoints));
  ::SelectObject(mDC, oldPen);

  
  if (pp0 != pts)
    delete [] pp0;

  return NS_OK;
}

void*
nsRenderingContextWin::GetNativeGraphicData(GraphicDataType aType)
{
  if (aType == NATIVE_WINDOWS_DC)
    return mDC;

  return nsnull;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawEllipse(const nsRect& aRect)
{
  return DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextWin :: DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupPen();

  HBRUSH oldBrush = (HBRUSH)::SelectObject(mDC, ::GetStockObject(NULL_BRUSH));
  
  ::Ellipse(mDC, aX, aY, aX + aWidth, aY + aHeight);
  ::SelectObject(mDC, oldBrush);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillEllipse(const nsRect& aRect)
{
  return FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP nsRenderingContextWin :: FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupSolidPen();
  SetupSolidBrush();
  
  ::Ellipse(mDC, aX, aY, aX + aWidth, aY + aHeight);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  return DrawArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle);
}

NS_IMETHODIMP nsRenderingContextWin :: DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  if (nsLineStyle_kNone == mCurrLineStyle)
    return NS_OK;

  PRInt32 quad1, quad2, sx, sy, ex, ey, cx, cy;
  float   anglerad, distance;

  mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupPen();
  SetupSolidBrush();

  
  distance = (float)sqrt((float)(aWidth * aWidth + aHeight * aHeight));
  cx = aX + aWidth / 2;
  cy = aY + aHeight / 2;

  anglerad = (float)(aStartAngle / (180.0 / 3.14159265358979323846));
  quad1 = (PRInt32)(aStartAngle / 90.0);
  sx = (PRInt32)(distance * cos(anglerad) + cx);
  sy = (PRInt32)(cy - distance * sin(anglerad));

  anglerad = (float)(aEndAngle / (180.0 / 3.14159265358979323846));
  quad2 = (PRInt32)(aEndAngle / 90.0);
  ex = (PRInt32)(distance * cos(anglerad) + cx);
  ey = (PRInt32)(cy - distance * sin(anglerad));

  
#ifndef WINCE
  ::SetArcDirection(mDC, AD_COUNTERCLOCKWISE);
#endif

  ::Arc(mDC, aX, aY, aX + aWidth, aY + aHeight, sx, sy, ex, ey); 

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: FillArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  return FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

NS_IMETHODIMP nsRenderingContextWin :: FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
  PRInt32 quad1, quad2, sx, sy, ex, ey, cx, cy;
  float   anglerad, distance;

  mTranMatrix->TransformCoord(&aX, &aY, &aWidth, &aHeight);

  SetupSolidPen();
  SetupSolidBrush();

  
  distance = (float)sqrt((float)(aWidth * aWidth + aHeight * aHeight));
  cx = aX + aWidth / 2;
  cy = aY + aHeight / 2;

  anglerad = (float)(aStartAngle / (180.0 / 3.14159265358979323846));
  quad1 = (PRInt32)(aStartAngle / 90.0);
  sx = (PRInt32)(distance * cos(anglerad) + cx);
  sy = (PRInt32)(cy - distance * sin(anglerad));

  anglerad = (float)(aEndAngle / (180.0 / 3.14159265358979323846));
  quad2 = (PRInt32)(aEndAngle / 90.0);
  ex = (PRInt32)(distance * cos(anglerad) + cx);
  ey = (PRInt32)(cy - distance * sin(anglerad));

  
  
#ifndef WINCE
  ::SetArcDirection(mDC, AD_COUNTERCLOCKWISE);
#endif
  ::Pie(mDC, aX, aY, aX + aWidth, aY + aHeight, sx, sy, ex, ey); 

  return NS_OK;
}


inline void CheckLength(PRUint32 *aLength)
{
  if (gIsWIN95 && *aLength > 8192)
    *aLength = 8192;
}

NS_IMETHODIMP nsRenderingContextWin :: GetWidth(char ch, nscoord& aWidth)
{
  char buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth);
}

NS_IMETHODIMP nsRenderingContextWin :: GetWidth(PRUnichar ch, nscoord &aWidth, PRInt32 *aFontID)
{
  PRUnichar buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth, aFontID);
}

NS_IMETHODIMP nsRenderingContextWin :: GetWidthInternal(const char* aString,
                                                        PRUint32 aLength,
                                                        nscoord& aWidth)
{

  if (nsnull != mFontMetrics)
  {
    
    
    if ((1 == aLength) && (aString[0] == ' '))
    {
      return mFontMetrics->GetSpaceWidth(aWidth);
    }

    CheckLength(&aLength);
    SetupFontAndColor();
    nscoord pxWidth = mCurrFontWin->GetWidth(mDC, aString, aLength);
    aWidth = NSToCoordRound(float(pxWidth) * mP2T);

    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

struct GetWidthData {
  HDC   mDC;    
  HFONT mFont;  
  LONG  mWidth; 
};

static PRBool PR_CALLBACK
do_GetWidth(const nsFontSwitch* aFontSwitch,
            const PRUnichar*    aSubstring,
            PRUint32            aSubstringLength,
            void*               aData)
{
  nsFontWin* fontWin = aFontSwitch->mFontWin;

  GetWidthData* data = (GetWidthData*)aData;
  if (data->mFont != fontWin->mFont) {
    
    data->mFont = fontWin->mFont;
    ::SelectObject(data->mDC, data->mFont);
  }
  data->mWidth += fontWin->GetWidth(data->mDC, aSubstring, aSubstringLength);
  return PR_TRUE; 
}

NS_IMETHODIMP nsRenderingContextWin :: GetWidthInternal(const PRUnichar *aString,
                                                        PRUint32 aLength,
                                                        nscoord &aWidth,
                                                        PRInt32 *aFontID)
{
  if (!mFontMetrics) return NS_ERROR_FAILURE;

  CheckLength(&aLength);
  SetupFontAndColor();

  nsFontMetricsWin* metrics = (nsFontMetricsWin*)mFontMetrics;
  GetWidthData data = {mDC, mCurrFont, 0};

  metrics->ResolveForwards(mDC, aString, aLength, do_GetWidth, &data);
  aWidth = NSToCoordRound(float(data.mWidth) * mP2T);

  if (mCurrFont != data.mFont) {
    
    ::SelectObject(mDC, mCurrFont);
  }

  if (aFontID) *aFontID = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextWin::GetTextDimensionsInternal(const char*       aString,
                                                 PRInt32           aLength,
                                                 PRInt32           aAvailWidth,
                                                 PRInt32*          aBreaks,
                                                 PRInt32           aNumBreaks,
                                                 nsTextDimensions& aDimensions,
                                                 PRInt32&          aNumCharsFit,
                                                 nsTextDimensions& aLastWordDimensions,
                                                 PRInt32*          aFontID)
{
  NS_PRECONDITION(aBreaks[aNumBreaks - 1] == aLength, "invalid break array");

  if (nsnull != mFontMetrics) {
    
    CheckLength((PRUint32*)&aLength);
    SetupFontAndColor();

    
    
    PRInt32 prevBreakState_BreakIndex = -1; 
    nscoord prevBreakState_Width = 0; 

    
    mFontMetrics->GetMaxAscent(aLastWordDimensions.ascent);
    mFontMetrics->GetMaxDescent(aLastWordDimensions.descent);
    aLastWordDimensions.width = -1;
    aNumCharsFit = 0;

    
    nscoord width = 0;
    PRInt32 start = 0;
    nscoord aveCharWidth;
    mFontMetrics->GetAveCharWidth(aveCharWidth);

    while (start < aLength) {
      
      
      
      PRInt32 estimatedNumChars = 0;
      if (aveCharWidth > 0) {
        estimatedNumChars = (aAvailWidth - width) / aveCharWidth;
      }
      if (estimatedNumChars < 1) {
        estimatedNumChars = 1;
      }

      
      PRInt32 estimatedBreakOffset = start + estimatedNumChars;
      PRInt32 breakIndex;
      nscoord numChars;

      
      
      if (aLength <= estimatedBreakOffset) {
        
        numChars = aLength - start;
        breakIndex = aNumBreaks - 1;
      } 
      else {
        breakIndex = prevBreakState_BreakIndex;
        while (((breakIndex + 1) < aNumBreaks) &&
               (aBreaks[breakIndex + 1] <= estimatedBreakOffset)) {
          ++breakIndex;
        }
        if (breakIndex == prevBreakState_BreakIndex) {
          ++breakIndex; 
        }
        numChars = aBreaks[breakIndex] - start;
      }

      
      nscoord pxWidth, twWidth = 0;
      if ((1 == numChars) && (aString[start] == ' ')) {
        mFontMetrics->GetSpaceWidth(twWidth);
      } 
      else if (numChars > 0) {
        pxWidth = mCurrFontWin->GetWidth(mDC, &aString[start], numChars);
        twWidth = NSToCoordRound(float(pxWidth) * mP2T);
      }

      
      PRBool  textFits = (twWidth + width) <= aAvailWidth;

      
      
      if (textFits) {
        aNumCharsFit += numChars;
        width += twWidth;
        start += numChars;

        
        
        prevBreakState_BreakIndex = breakIndex;
        prevBreakState_Width = width;
      }
      else {
        
        
        if (prevBreakState_BreakIndex > 0) {
          
          
          if (prevBreakState_BreakIndex == (breakIndex - 1)) {
            aNumCharsFit = aBreaks[prevBreakState_BreakIndex];
            width = prevBreakState_Width;
            break;
          }
        }

        
        if (0 == breakIndex) {
          
          
          aNumCharsFit += numChars;
          width += twWidth;
          break;
        }

        
        
        width += twWidth;
        while ((breakIndex >= 1) && (width > aAvailWidth)) {
          twWidth = 0;
          start = aBreaks[breakIndex - 1];
          numChars = aBreaks[breakIndex] - start;
          
          if ((1 == numChars) && (aString[start] == ' ')) {
            mFontMetrics->GetSpaceWidth(twWidth);
          } 
          else if (numChars > 0) {
            pxWidth = mCurrFontWin->GetWidth(mDC, &aString[start], numChars);
            twWidth = NSToCoordRound(float(pxWidth) * mP2T);
          }

          width -= twWidth;
          aNumCharsFit = start;
          breakIndex--;
        }
        break;
      }
    }

    aDimensions.width = width;
    mFontMetrics->GetMaxAscent(aDimensions.ascent);
    mFontMetrics->GetMaxDescent(aDimensions.descent);

    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

struct BreakGetTextDimensionsData {
  HDC      mDC;                
  HFONT    mFont;              
  float    mP2T;               
  PRInt32  mAvailWidth;        
  PRInt32* mBreaks;            
  PRInt32  mNumBreaks;         
  nscoord  mSpaceWidth;        
  nscoord  mAveCharWidth;      
  PRInt32  mEstimatedNumChars; 

  PRInt32  mNumCharsFit;  
  nscoord  mWidth;        

  
  
  PRInt32 mPrevBreakState_BreakIndex; 
  nscoord mPrevBreakState_Width;      

  
  
  
  nsVoidArray* mFonts;   
  nsVoidArray* mOffsets; 

};

static PRBool PR_CALLBACK
do_BreakGetTextDimensions(const nsFontSwitch* aFontSwitch,
                          const PRUnichar*    aSubstring,
                          PRUint32            aSubstringLength,
                          void*               aData)
{
  nsFontWin* fontWin = aFontSwitch->mFontWin;

  
  BreakGetTextDimensionsData* data = (BreakGetTextDimensionsData*)aData;
  if (data->mFont != fontWin->mFont) {
    data->mFont = fontWin->mFont;
    ::SelectObject(data->mDC, data->mFont);
  }

  
  
  const PRUnichar* pstr = (const PRUnichar*)data->mOffsets->ElementAt(0);
  PRInt32 numCharsFit = data->mNumCharsFit;
  nscoord width = data->mWidth;
  PRInt32 start = (PRInt32)(aSubstring - pstr);
  PRInt32 end = start + aSubstringLength;
  PRBool allDone = PR_FALSE;

  while (start < end) {
    
    
    PRInt32 estimatedNumChars = data->mEstimatedNumChars;
    if (!estimatedNumChars && data->mAveCharWidth > 0) {
      estimatedNumChars = (data->mAvailWidth - width) / data->mAveCharWidth;
    }
    
    if (estimatedNumChars < 1) {
      estimatedNumChars = 1;
    }

    
    PRInt32 estimatedBreakOffset = start + estimatedNumChars;
    PRInt32 breakIndex = -1; 
    PRBool  inMiddleOfSegment = PR_FALSE;
    nscoord numChars;

    
    
    if (end <= estimatedBreakOffset) {
      
      numChars = end - start;
    }
    else {
      
      
      breakIndex = data->mPrevBreakState_BreakIndex;
      while (breakIndex + 1 < data->mNumBreaks &&
             data->mBreaks[breakIndex + 1] <= estimatedBreakOffset) {
        ++breakIndex;
      }

      if (breakIndex == -1)
        breakIndex = 0;

      
      
      
      if (start < data->mBreaks[breakIndex]) {
        
        
        numChars = PR_MIN(data->mBreaks[breakIndex], end) - start;
      } 
      else {
        
        
        if ((breakIndex < (data->mNumBreaks - 1)) && (data->mBreaks[breakIndex] < end)) {
          ++breakIndex;
          numChars = PR_MIN(data->mBreaks[breakIndex], end) - start;
        }
        else {
          NS_ASSERTION(end != data->mBreaks[breakIndex], "don't expect to be at segment boundary");

          
          numChars = end - start;

          
          
          inMiddleOfSegment = PR_TRUE;
        }
      }
    }

    
    nscoord twWidth, pxWidth;
    if ((1 == numChars) && (pstr[start] == ' ')) {
      twWidth = data->mSpaceWidth;
    }
    else {
      pxWidth = fontWin->GetWidth(data->mDC, &pstr[start], numChars);
      twWidth = NSToCoordRound(float(pxWidth) * data->mP2T);
    }

    
    PRBool textFits = (twWidth + width) <= data->mAvailWidth;

    
    
    if (textFits) {
      numCharsFit += numChars;
      width += twWidth;

      
      
      
      if ((breakIndex != -1) && !inMiddleOfSegment) {
        data->mPrevBreakState_BreakIndex = breakIndex;
        data->mPrevBreakState_Width = width;
      }
    }
    else {
      
      allDone = PR_TRUE;

      
      
      if (data->mPrevBreakState_BreakIndex != -1) {
        PRBool canBackup;

        
        
        
        
        if (inMiddleOfSegment) {
          canBackup = data->mPrevBreakState_BreakIndex == breakIndex;
        } else {
          canBackup = data->mPrevBreakState_BreakIndex == (breakIndex - 1);
        }

        if (canBackup) {
          numCharsFit = data->mBreaks[data->mPrevBreakState_BreakIndex];
          width = data->mPrevBreakState_Width;
          break;
        }
      }

      
      
      end = start + numChars;
      breakIndex = 0;
      if (data->mBreaks[breakIndex] < end) {
        while ((breakIndex + 1 < data->mNumBreaks) && (data->mBreaks[breakIndex + 1] < end)) {
          ++breakIndex;
        }
      }

      if ((0 == breakIndex) && (end <= data->mBreaks[0])) {
        
        
        numCharsFit += numChars;
        width += twWidth;

        
        
        
        if (numCharsFit < data->mBreaks[0]) {
          allDone = PR_FALSE;
          
          
          
          
          
          
          
          data->mEstimatedNumChars = data->mBreaks[0] - numCharsFit;
          start += numChars;
        }

        break;
      }

      
      
      width += twWidth;
      while ((breakIndex >= 0) && (width > data->mAvailWidth)) {
        start = data->mBreaks[breakIndex];
        numChars = end - start;
        numCharsFit = start;
        if ((1 == numChars) && (pstr[start] == ' ')) {
          width -= data->mSpaceWidth;
        }
        else if (pstr + start >= aSubstring) {
          
          pxWidth = fontWin->GetWidth(data->mDC, &pstr[start], numChars);
          width -= NSToCoordRound(float(pxWidth) * data->mP2T);
        }
        else {
          
          
          
          
          end = data->mNumCharsFit; 
          data->mNumCharsFit = numCharsFit; 
          PRInt32 k = data->mFonts->Count() - 1;
          for ( ; k >= 0 && start < end; --k, end -= numChars) {
            fontWin = (nsFontWin*)data->mFonts->ElementAt(k);
            const PRUnichar* ps = (const PRUnichar*)data->mOffsets->ElementAt(k);
            if (ps < pstr + start)
              ps = pstr + start;

            numChars = pstr + end - ps;
            NS_ASSERTION(numChars > 0, "empty string");

            data->mFont = fontWin->mFont;
            ::SelectObject(data->mDC, data->mFont);
            pxWidth = fontWin->GetWidth(data->mDC, ps, numChars);
            data->mWidth -= NSToCoordRound(float(pxWidth) * data->mP2T);

            
            
            data->mFonts->RemoveElementAt(k);
            data->mOffsets->RemoveElementAt(k+1);
          }

          
          
          data->mFonts->AppendElement(fontWin);
          data->mOffsets->AppendElement((void*)&pstr[numCharsFit]);
          break;
        }

        --breakIndex;
        end = start;
      }
    }

    start += numChars;
  }

#ifdef DEBUG_rbs
  NS_ASSERTION(allDone || start == end, "internal error");
  NS_ASSERTION(allDone || data->mNumCharsFit != numCharsFit, "internal error");
#endif 

  if (data->mNumCharsFit != numCharsFit) {
    
    data->mWidth = width;
    data->mNumCharsFit = numCharsFit;
    data->mFonts->AppendElement(fontWin);
    data->mOffsets->AppendElement((void*)&pstr[numCharsFit]);
  }

  if (allDone) {
    
    return PR_FALSE;
  }

  return PR_TRUE; 
}

NS_IMETHODIMP
nsRenderingContextWin::GetTextDimensionsInternal(const PRUnichar*  aString,
                                                 PRInt32           aLength,
                                                 PRInt32           aAvailWidth,
                                                 PRInt32*          aBreaks,
                                                 PRInt32           aNumBreaks,
                                                 nsTextDimensions& aDimensions,
                                                 PRInt32&          aNumCharsFit,
                                                 nsTextDimensions& aLastWordDimensions,
                                                 PRInt32*          aFontID)
{
  if (!mFontMetrics) return NS_ERROR_FAILURE;

  CheckLength((PRUint32*)&aLength);
  SetupFontAndColor();

  nsFontMetricsWin* metrics = (nsFontMetricsWin*)mFontMetrics;

  nscoord spaceWidth, aveCharWidth;
  metrics->GetSpaceWidth(spaceWidth);
  metrics->GetAveCharWidth(aveCharWidth);

  
  
  
  

  
  

  nsAutoVoidArray fonts, offsets;
  offsets.AppendElement((void*)aString);

  BreakGetTextDimensionsData data = {mDC, mCurrFont, mP2T, 
    aAvailWidth, aBreaks, aNumBreaks, spaceWidth, aveCharWidth,
    0, 0, 0, -1, 0, &fonts, &offsets 
  };

  metrics->ResolveForwards(mDC, aString, aLength, do_BreakGetTextDimensions, &data);

  if (mCurrFont != data.mFont) {
    
    ::SelectObject(mDC, mCurrFont);
  }

  if (aFontID) *aFontID = 0;

  aNumCharsFit = data.mNumCharsFit;
  aDimensions.width = data.mWidth;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  aLastWordDimensions.Clear();
  aLastWordDimensions.width = -1;

  PRInt32 count = fonts.Count();
  if (!count)
    return NS_OK;
  nsFontWin* fontWin = (nsFontWin*)fonts[0];
  NS_ASSERTION(fontWin, "internal error in do_BreakGetTextDimensions");
  aDimensions.ascent = fontWin->mMaxAscent;
  aDimensions.descent = fontWin->mMaxDescent;

  
  if (count == 1)
    return NS_OK;

  
  
  
  
  
  PRInt32 lastBreakIndex = 0;
  while (aBreaks[lastBreakIndex] < aNumCharsFit)
    ++lastBreakIndex;

  const PRUnichar* lastWord = (lastBreakIndex > 0) 
    ? aString + aBreaks[lastBreakIndex-1]
    : aString + aNumCharsFit; 

  
  

  PRInt32 currFont = 0;
  const PRUnichar* pstr = aString;
  const PRUnichar* last = aString + aNumCharsFit;

  while (pstr < last) {
    fontWin = (nsFontWin*)fonts[currFont];
    PRUnichar* nextOffset = (PRUnichar*)offsets[++currFont]; 

    
    
    
    
    
    
    
    
    
    
    if (*pstr == ' ') {
      
      ++pstr;
      
      if (pstr == last) {
        break;
      }
      
      if (pstr == nextOffset) {
        fontWin = (nsFontWin*)fonts[currFont];
        nextOffset = (PRUnichar*)offsets[++currFont];
      } 
    }

    
    
    
    if (nextOffset > lastWord) {
      if (aLastWordDimensions.ascent < fontWin->mMaxAscent) {
        aLastWordDimensions.ascent = fontWin->mMaxAscent;
      }
      if (aLastWordDimensions.descent < fontWin->mMaxDescent) {
        aLastWordDimensions.descent = fontWin->mMaxDescent;
      }
    }

    
    if (pstr < lastWord) {
      if (aDimensions.ascent < fontWin->mMaxAscent) {
        aDimensions.ascent = fontWin->mMaxAscent;
      }
      if (aDimensions.descent < fontWin->mMaxDescent) {
        aDimensions.descent = fontWin->mMaxDescent;
      }
    }

    
    pstr = nextOffset;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextWin::GetTextDimensionsInternal(const char*       aString,
                                                 PRUint32          aLength,
                                                 nsTextDimensions& aDimensions)
{
  if (!mFontMetrics) {
    aDimensions.Clear();
    return NS_ERROR_FAILURE;
  }
  GetWidth(aString, aLength, aDimensions.width);
  mFontMetrics->GetMaxAscent(aDimensions.ascent);
  mFontMetrics->GetMaxDescent(aDimensions.descent);
  return NS_OK;
}

struct GetTextDimensionsData {
  HDC     mDC;      
  HFONT   mFont;    
  LONG    mWidth;   
  nscoord mAscent;  
  nscoord mDescent; 
};

static PRBool PR_CALLBACK
do_GetTextDimensions(const nsFontSwitch* aFontSwitch,
                     const PRUnichar*    aSubstring,
                     PRUint32            aSubstringLength,
                     void*               aData)
{
  nsFontWin* fontWin = aFontSwitch->mFontWin;
  GetTextDimensionsData* data = (GetTextDimensionsData*)aData;
  if (data->mFont != fontWin->mFont) {
    data->mFont = fontWin->mFont;
    ::SelectObject(data->mDC, data->mFont);
  }
  data->mWidth += fontWin->GetWidth(data->mDC, aSubstring, aSubstringLength);
  if (data->mAscent < fontWin->mMaxAscent) {
    data->mAscent = fontWin->mMaxAscent;
  }
  if (data->mDescent < fontWin->mMaxDescent) {
    data->mDescent = fontWin->mMaxDescent;
  }

  return PR_TRUE; 
}

NS_IMETHODIMP
nsRenderingContextWin::GetTextDimensionsInternal(const PRUnichar*  aString,
                                                 PRUint32          aLength,
                                                 nsTextDimensions& aDimensions,
                                                 PRInt32*          aFontID)
{
  aDimensions.Clear();
  if (!mFontMetrics) return NS_ERROR_FAILURE;

  CheckLength(&aLength);
  SetupFontAndColor();

  nsFontMetricsWin* metrics = (nsFontMetricsWin*)mFontMetrics;
  GetTextDimensionsData data = {mDC, mCurrFont, 0, 0, 0};

  metrics->ResolveForwards(mDC, aString, aLength, do_GetTextDimensions, &data);
  aDimensions.width = NSToCoordRound(float(data.mWidth) * mP2T);
  aDimensions.ascent = data.mAscent;
  aDimensions.descent = data.mDescent;

  if (mCurrFont != data.mFont) {
    
    ::SelectObject(mDC, mCurrFont);
  }

  if (aFontID) *aFontID = 0;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextWin :: DrawStringInternal(const char *aString, PRUint32 aLength,
                                                          nscoord aX, nscoord aY,
                                                          const nscoord* aSpacing)
{
  NS_PRECONDITION(mFontMetrics,"Something is wrong somewhere");

  PRInt32 x = aX;
  PRInt32 y = aY;

  CheckLength(&aLength);
  SetupFontAndColor();

  INT dxMem[500];
  INT* dx0 = NULL;
  if (aSpacing) {
    dx0 = dxMem;
    if (aLength > 500) {
      dx0 = new INT[aLength];
    }
    mTranMatrix->ScaleXCoords(aSpacing, aLength, dx0);
  }
  mTranMatrix->TransformCoord(&x, &y);

  mCurrFontWin->DrawString(mDC, x, y, aString, aLength, dx0);

  if (dx0 && (dx0 != dxMem)) {
    delete [] dx0;
  }

  return NS_OK;
}

struct DrawStringData {
  HDC            mDC;         
  HFONT          mFont;       
  nsTransform2D* mTranMatrix; 
  nscoord        mX;          
  nscoord        mY;          
  const nscoord* mSpacing;    
  nscoord        mMaxLength;  
  nscoord        mLength;     
};

static PRBool PR_CALLBACK
do_DrawString(const nsFontSwitch* aFontSwitch,
              const PRUnichar*    aSubstring,
              PRUint32            aSubstringLength,
              void*               aData)
{
  nsFontWin* fontWin = aFontSwitch->mFontWin;

  PRInt32 x, y;
  DrawStringData* data = (DrawStringData*)aData;
  if (data->mFont != fontWin->mFont) {
    data->mFont = fontWin->mFont;
    ::SelectObject(data->mDC, data->mFont);
  }

  data->mLength += aSubstringLength;
  if (data->mSpacing) {
    
    
    

    
    const PRUnichar* str = aSubstring;
    const PRUnichar* end = aSubstring + aSubstringLength;
    while (str < end) {
      
      
      x = data->mX;
      y = data->mY;
      data->mTranMatrix->TransformCoord(&x, &y);
      if (NS_IS_HIGH_SURROGATE(*str) && 
          ((str+1)<end) && 
          NS_IS_LOW_SURROGATE(*(str+1))) 
      {
        
        fontWin->DrawString(data->mDC, x, y, str, 2);
        
        data->mX += *data->mSpacing++;
        ++str;
      } else {
        fontWin->DrawString(data->mDC, x, y, str, 1);
      }
      data->mX += *data->mSpacing++;
      ++str;
    }
  }
  else {
    fontWin->DrawString(data->mDC, data->mX, data->mY, aSubstring, aSubstringLength);
    
    if (data->mLength < data->mMaxLength) {
      data->mX += fontWin->GetWidth(data->mDC, aSubstring, aSubstringLength);
    }
  }
  return PR_TRUE; 
}

NS_IMETHODIMP nsRenderingContextWin :: DrawStringInternal(const PRUnichar *aString, PRUint32 aLength,
                                                          nscoord aX, nscoord aY,
                                                          PRInt32 aFontID,
                                                          const nscoord* aSpacing)
{
  if (!mFontMetrics) return NS_ERROR_FAILURE;

  CheckLength(&aLength);
  SetupFontAndColor();

  nsFontMetricsWin* metrics = (nsFontMetricsWin*)mFontMetrics;
  DrawStringData data = {mDC, mCurrFont, mTranMatrix, 
    aX, aY, aSpacing, aLength, 0
  };
  if (!aSpacing) { 
    mTranMatrix->TransformCoord(&data.mX, &data.mY);
  }

  if (mRightToLeftText) {
    metrics->ResolveBackwards(mDC, aString, aLength, do_DrawString, &data);
  }
  else
  {
    metrics->ResolveForwards(mDC, aString, aLength, do_DrawString, &data);
  }

  if (mCurrFont != data.mFont) {
    
    ::SelectObject(mDC, mCurrFont);
  }

  return NS_OK;
}

#ifdef MOZ_MATHML
NS_IMETHODIMP 
nsRenderingContextWin::GetBoundingMetricsInternal(const char*        aString,
                                                  PRUint32           aLength,
                                                  nsBoundingMetrics& aBoundingMetrics)
{
  NS_PRECONDITION(mFontMetrics,"Something is wrong somewhere");

  aBoundingMetrics.Clear();
  if (!mFontMetrics) return NS_ERROR_FAILURE;

  CheckLength(&aLength);
  SetupFontAndColor();
  nsresult rv = mCurrFontWin->GetBoundingMetrics(mDC, aString, aLength, aBoundingMetrics);

  
  aBoundingMetrics.leftBearing = NSToCoordRound(float(aBoundingMetrics.leftBearing) * mP2T);
  aBoundingMetrics.rightBearing = NSToCoordRound(float(aBoundingMetrics.rightBearing) * mP2T);
  aBoundingMetrics.width = NSToCoordRound(float(aBoundingMetrics.width) * mP2T);
  aBoundingMetrics.ascent = NSToCoordRound(float(aBoundingMetrics.ascent) * mP2T);
  aBoundingMetrics.descent = NSToCoordRound(float(aBoundingMetrics.descent) * mP2T);

  return rv;
}

struct GetBoundingMetricsData {
  HDC                mDC;              
  HFONT              mFont;            
  nsBoundingMetrics* mBoundingMetrics; 
  PRBool             mFirstTime;       
  nsresult           mStatus;          
};

static PRBool PR_CALLBACK
do_GetBoundingMetrics(const nsFontSwitch* aFontSwitch,
                      const PRUnichar*    aSubstring,
                      PRUint32            aSubstringLength,
                      void*               aData)
{
  nsFontWin* fontWin = aFontSwitch->mFontWin;

  GetBoundingMetricsData* data = (GetBoundingMetricsData*)aData;
  if (data->mFont != fontWin->mFont) {
    data->mFont = fontWin->mFont;
    ::SelectObject(data->mDC, data->mFont);
  }

  nsBoundingMetrics rawbm;
  data->mStatus = fontWin->GetBoundingMetrics(data->mDC, aSubstring, aSubstringLength, rawbm);
  if (NS_FAILED(data->mStatus)) {
    return PR_FALSE; 
  }

  if (data->mFirstTime) {
    data->mFirstTime = PR_FALSE;
    *data->mBoundingMetrics = rawbm;
  }
  else {
    *data->mBoundingMetrics += rawbm;
  }

  return PR_TRUE; 
}

NS_IMETHODIMP
nsRenderingContextWin::GetBoundingMetricsInternal(const PRUnichar*   aString,
                                                  PRUint32           aLength,
                                                  nsBoundingMetrics& aBoundingMetrics,
                                                  PRInt32*           aFontID)
{
  aBoundingMetrics.Clear();
  if (!mFontMetrics) return NS_ERROR_FAILURE;

  CheckLength(&aLength);
  SetupFontAndColor();

  nsFontMetricsWin* metrics = (nsFontMetricsWin*)mFontMetrics;
  GetBoundingMetricsData data = {mDC, mCurrFont, &aBoundingMetrics, PR_TRUE, NS_OK};

  nsresult rv = metrics->ResolveForwards(mDC, aString, aLength, do_GetBoundingMetrics, &data);
  if (NS_SUCCEEDED(rv)) {
    rv = data.mStatus;
  }

  
  aBoundingMetrics.leftBearing = NSToCoordRound(float(aBoundingMetrics.leftBearing) * mP2T);
  aBoundingMetrics.rightBearing = NSToCoordRound(float(aBoundingMetrics.rightBearing) * mP2T);
  aBoundingMetrics.width = NSToCoordRound(float(aBoundingMetrics.width) * mP2T);
  aBoundingMetrics.ascent = NSToCoordRound(float(aBoundingMetrics.ascent) * mP2T);
  aBoundingMetrics.descent = NSToCoordRound(float(aBoundingMetrics.descent) * mP2T);

  if (mCurrFont != data.mFont) {
    
    ::SelectObject(mDC, mCurrFont);
  }

  if (aFontID) *aFontID = 0;

  return rv;
}
#endif 

PRInt32 nsRenderingContextWin::GetMaxStringLength()
{
  if (!mFontMetrics)
    return 1;
  return static_cast<nsFontMetricsWin*>(mFontMetrics)->GetMaxStringLength();
}

NS_IMETHODIMP nsRenderingContextWin :: CopyOffScreenBits(nsIDrawingSurface* aSrcSurf,
                                                         PRInt32 aSrcX, PRInt32 aSrcY,
                                                         const nsRect &aDestBounds,
                                                         PRUint32 aCopyFlags)
{

  if ((nsnull != aSrcSurf) && (nsnull != mMainDC))
  {
    PRInt32 x = aSrcX;
    PRInt32 y = aSrcY;
    nsRect  drect = aDestBounds;
    HDC     destdc, srcdc;

    

    ((nsDrawingSurfaceWin *)aSrcSurf)->GetDC(&srcdc);

    if (nsnull != srcdc)
    {
      if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER)
      {
        NS_ASSERTION(!(nsnull == mDC), "no back buffer");
        destdc = mDC;
      }
      else
        destdc = mMainDC;

      if (aCopyFlags & NS_COPYBITS_USE_SOURCE_CLIP_REGION)
      {
        HRGN  tregion = ::CreateRectRgn(0, 0, 0, 0);

        if (::GetClipRgn(srcdc, tregion) == 1)
          ::SelectClipRgn(destdc, tregion);

        ::DeleteObject(tregion);
      }

      
      

      nsPaletteInfo palInfo;

      mContext->GetPaletteInfo(palInfo);

      if (palInfo.isPaletteDevice && palInfo.palette){
        ::SelectPalette(destdc, (HPALETTE)palInfo.palette, PR_TRUE);
        
        
        
        
        
        
#ifndef WINCE
        ::UpdateColors(mDC);                                                      
#endif
      }

      if (aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
        mTranMatrix->TransformCoord(&x, &y);

      if (aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
        mTranMatrix->TransformCoord(&drect.x, &drect.y, &drect.width, &drect.height);

      ::BitBlt(destdc, drect.x, drect.y,
               drect.width, drect.height,
               srcdc, x, y, SRCCOPY);


      
      ((nsDrawingSurfaceWin *)aSrcSurf)->ReleaseDC();
    }
    else
      NS_ASSERTION(0, "attempt to blit with bad DCs");
  }
  else
    NS_ASSERTION(0, "attempt to blit with bad DCs");

  return NS_OK;
}




static const int  BRUSH_CACHE_SIZE = 17;  

class SolidBrushCache {
public:
  SolidBrushCache();
  ~SolidBrushCache();

  HBRUSH  GetSolidBrush(HDC theHDC, COLORREF aColor);

private:
  struct CacheEntry {
    HBRUSH   mBrush;
    COLORREF mBrushColor;
  };

  CacheEntry  mCache[BRUSH_CACHE_SIZE];
  int         mIndexOldest;  
};

SolidBrushCache::SolidBrushCache()
  : mIndexOldest(2)
{
  
  mCache[0].mBrush = gStockWhiteBrush;
  mCache[0].mBrushColor = RGB(255, 255, 255);
  mCache[1].mBrush = (HBRUSH)::GetStockObject(BLACK_BRUSH);
  mCache[1].mBrushColor = RGB(0, 0, 0);
}

SolidBrushCache::~SolidBrushCache()
{
  
  for (int i = 2; i < BRUSH_CACHE_SIZE; i++) {
    if (mCache[i].mBrush) {
      ::DeleteObject(mCache[i].mBrush);
    }
  }
}

HBRUSH
SolidBrushCache::GetSolidBrush(HDC theHDC, COLORREF aColor)
{
  int     i;
  HBRUSH  result = NULL;
  
  
  for (i = 0; (i < BRUSH_CACHE_SIZE) && mCache[i].mBrush; i++) {
    if (mCache[i].mBrush && (mCache[i].mBrushColor == aColor)) {
      
      result = mCache[i].mBrush;
      ::SelectObject(theHDC, result);
      break;
    }
  }

  if (!result) {
    
    
    result = (HBRUSH)::CreateSolidBrush(PALETTERGB_COLORREF(aColor));

    
    
    ::SelectObject(theHDC, result);

    
    if (i >= BRUSH_CACHE_SIZE) {
      
      
      ::DeleteObject(mCache[mIndexOldest].mBrush);
      i = mIndexOldest;
      if (++mIndexOldest >= BRUSH_CACHE_SIZE) {
        mIndexOldest = 2;
      }
    }

    
    mCache[i].mBrush = result;
    mCache[i].mBrushColor = aColor;
  }

  return result;
}

static SolidBrushCache  gSolidBrushCache;

HBRUSH nsRenderingContextWin :: SetupSolidBrush(void)
{
  if ((mCurrentColor != mCurrBrushColor) || (NULL == mCurrBrush))
  {
    HBRUSH tbrush = gSolidBrushCache.GetSolidBrush(mDC, mColor);

    mCurrBrush = tbrush;
    mCurrBrushColor = mCurrentColor;
  }

  return mCurrBrush;
}

void nsRenderingContextWin :: SetupFontAndColor(void)
{
  if (mFontMetrics && (!mCurrFontWin || mCurrFontWin->mFont != mCurrFont)) {
    nsFontHandle  fontHandle;
    mFontMetrics->GetFontHandle(fontHandle);
    HFONT         tfont = (HFONT)fontHandle;

    ::SelectObject(mDC, tfont);

    mCurrFont = tfont;
    mCurrFontWin = ((nsFontMetricsWin*)mFontMetrics)->GetFontFor(mCurrFont);

    
    
    
    NS_ASSERTION(mCurrFontWin, "internal error");
  }

  if (mCurrentColor != mCurrTextColor)
  {
    ::SetTextColor(mDC, PALETTERGB_COLORREF(mColor));
    mCurrTextColor = mCurrentColor;
  }
}

HPEN nsRenderingContextWin :: SetupPen()
{
  HPEN pen;

  switch(mCurrLineStyle)
  {
    case nsLineStyle_kSolid:
      pen = SetupSolidPen();
      break;

    case nsLineStyle_kDashed:
      pen = SetupDashedPen();
      break;

    case nsLineStyle_kDotted:
      pen = SetupDottedPen();
      break;

    case nsLineStyle_kNone:
      pen = NULL;
      break;

    default:
      pen = SetupSolidPen();
      break;
  }

  return pen;
}


HPEN nsRenderingContextWin :: SetupSolidPen(void)
{
  if ((mCurrentColor != mCurrPenColor) || (NULL == mCurrPen) || (mCurrPen != mStates->mSolidPen))
  {
    HPEN  tpen;
     
    if (RGB(0, 0, 0) == mColor) {
      tpen = gStockBlackPen;
    } else if (RGB(255, 255, 255) == mColor) {
      tpen = gStockWhitePen;
    } else {
      tpen = ::CreatePen(PS_SOLID, 0, PALETTERGB_COLORREF(mColor));
    }

    ::SelectObject(mDC, tpen);

    if (mCurrPen && (mCurrPen != gStockBlackPen) && (mCurrPen != gStockWhitePen)) {
      VERIFY(::DeleteObject(mCurrPen));
    }

    mStates->mSolidPen = mCurrPen = tpen;
    mCurrPenColor = mCurrentColor;
  }

  return mCurrPen;
}

HPEN nsRenderingContextWin :: SetupDashedPen(void)
{
  if ((mCurrentColor != mCurrPenColor) || (NULL == mCurrPen) || (mCurrPen != mStates->mDashedPen))
  {
    HPEN  tpen = ::CreatePen(PS_DASH, 0, PALETTERGB_COLORREF(mColor));

    ::SelectObject(mDC, tpen);

    if (NULL != mCurrPen)
      VERIFY(::DeleteObject(mCurrPen));

    mStates->mDashedPen = mCurrPen = tpen;
    mCurrPenColor = mCurrentColor;
  }

  return mCurrPen;
}

HPEN nsRenderingContextWin :: SetupDottedPen(void)
{
  if ((mCurrentColor != mCurrPenColor) || (NULL == mCurrPen) || (mCurrPen != mStates->mDottedPen))
  {
    HPEN  tpen = ::CreatePen(
#ifndef WINCE
                             PS_DOT, 
#else
                             PS_DASH,
#endif
                             0, PALETTERGB_COLORREF(mColor));


    ::SelectObject(mDC, tpen);

    if (NULL != mCurrPen)
      VERIFY(::DeleteObject(mCurrPen));

    mStates->mDottedPen = mCurrPen = tpen;
    mCurrPenColor = mCurrentColor;
  }

  return mCurrPen;
}



NS_IMETHODIMP 
nsRenderingContextWin::SetPenMode(nsPenMode aPenMode)
{

  switch(aPenMode){
  case nsPenMode_kNone:
    ::SetROP2(mDC,R2_COPYPEN);
    mPenMode = nsPenMode_kNone;
    break;
  case nsPenMode_kInvert:
    ::SetROP2(mDC,R2_NOT);
    mPenMode = nsPenMode_kInvert;
    break;
  }

  return NS_OK;
}



NS_IMETHODIMP 
nsRenderingContextWin::GetPenMode(nsPenMode &aPenMode)
{
  
  aPenMode = mPenMode;

  return NS_OK;
}



void nsRenderingContextWin :: PushClipState(void)
{
  if (!(mStates->mFlags & FLAG_CLIP_CHANGED))
  {
    GraphicsState *tstate = mStates->mNext;

    
    
    
    

    if (nsnull != tstate)
    {
      if (NULL == tstate->mClipRegion)
        tstate->mClipRegion = ::CreateRectRgn(0, 0, 0, 0);

      if (::GetClipRgn(mDC, tstate->mClipRegion) == 1)
        tstate->mFlags |= FLAG_CLIP_VALID;
      else
        tstate->mFlags &= ~FLAG_CLIP_VALID;
    }
  
    mStates->mFlags |= FLAG_CLIP_CHANGED;
  }
}

NS_IMETHODIMP nsRenderingContextWin :: CreateDrawingSurface(HDC aDC, nsIDrawingSurface* &aSurface)
{
  nsDrawingSurfaceWin *surf = new nsDrawingSurfaceWin();

  if (nsnull != surf)
  {
    NS_ADDREF(surf);
    surf->Init(aDC);
  }

  aSurface = surf;

  return NS_OK;
}














void 
nsRenderingContextWin::ConditionRect(nsRect& aSrcRect, RECT& aDestRect)
{
  
  if (!gIsWIN95)
  {
    aDestRect.top = aSrcRect.y;
    aDestRect.bottom = aSrcRect.y + aSrcRect.height;
    aDestRect.left = aSrcRect.x;
    aDestRect.right = aSrcRect.x + aSrcRect.width;
    return;
  }

  
  const nscoord kBottomRightLimit = 16384;
  const nscoord kTopLeftLimit = -8192;

  aDestRect.top = (aSrcRect.y < kTopLeftLimit)
                      ? kTopLeftLimit
                      : aSrcRect.y;
  aDestRect.bottom = ((aSrcRect.y + aSrcRect.height) > kBottomRightLimit)
                      ? kBottomRightLimit
                      : (aSrcRect.y + aSrcRect.height);
  aDestRect.left = (aSrcRect.x < kTopLeftLimit)
                      ? kTopLeftLimit
                      : aSrcRect.x;
  aDestRect.right = ((aSrcRect.x + aSrcRect.width) > kBottomRightLimit)
                      ? kBottomRightLimit
                      : (aSrcRect.x + aSrcRect.width);
}






NS_IMETHODIMP
nsRenderingContextWin::SetRightToLeftText(PRBool aIsRTL)
{
#ifndef WINCE
  
  
  if (aIsRTL != mRightToLeftText) {
    UINT flags = ::GetTextAlign(mDC);
    if (aIsRTL) {
      flags |= TA_RTLREADING;
    }
    else {
      flags &= (~TA_RTLREADING);
    }
    ::SetTextAlign(mDC, flags);
  }

  mRightToLeftText = aIsRTL;
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextWin::GetRightToLeftText(PRBool* aIsRTL)
{
#ifndef WINCE
  *aIsRTL = mRightToLeftText;
#else
  *aIsRTL = PR_FALSE;
#endif
  return NS_OK;
}





void
nsRenderingContextWin::InitBidiInfo()
{
#ifndef WINCE
  if (NOT_SETUP == gBidiInfo) {
    gBidiInfo = DONT_INIT;

    const PRUnichar araAin  = 0x0639;
    const PRUnichar one     = 0x0031;

    int distanceArray[2];
    PRUnichar glyphArray[2];
    PRUnichar outStr[] = {0, 0};

    GCP_RESULTSW gcpResult;
    gcpResult.lStructSize = sizeof(GCP_RESULTS);
    gcpResult.lpOutString = outStr;     
    gcpResult.lpOrder = nsnull;         
    gcpResult.lpDx = distanceArray;     
    gcpResult.lpCaretPos = nsnull;      
    gcpResult.lpClass = nsnull;         
    gcpResult.lpGlyphs = glyphArray;    
    gcpResult.nGlyphs = 2;              

    PRUnichar inStr[] = {araAin, one};

    if (::GetCharacterPlacementW(mDC, inStr, 2, 0, &gcpResult, GCP_REORDER) 
        && (inStr[0] == outStr[1]) ) {
      gBidiInfo = GCP_REORDER | GCP_GLYPHSHAPE;
#ifdef NS_DEBUG
      printf("System has shaping\n");
#endif
    }
    else {
      const PRUnichar hebAlef = 0x05D0;
      inStr[0] = hebAlef;
      inStr[1] = one;
      if (::GetCharacterPlacementW(mDC, inStr, 2, 0, &gcpResult, GCP_REORDER) 
          && (inStr[0] == outStr[1]) ) {
        gBidiInfo = GCP_REORDER;
#ifdef NS_DEBUG
        printf("System has Bidi\n");
#endif
      }
    }
  }
#endif 
}



