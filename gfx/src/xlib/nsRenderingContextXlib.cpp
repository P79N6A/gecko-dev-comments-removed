










































#include "nsRenderingContextXlib.h"
#include "nsFontMetricsXlib.h"
#include "nsCompressedCharMap.h"
#include "xlibrgb.h"
#include "prprf.h"
#include "prmem.h"
#include "prlog.h"
#include "prenv.h"
#include "nsGCCache.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

#define NS_TO_XLIBRGB_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)

NS_IMPL_ISUPPORTS1(nsRenderingContextXlib, nsIRenderingContext)

#ifdef PR_LOGGING 
static PRLogModuleInfo * RenderingContextXlibLM = PR_NewLogModule("RenderingContextXlib");
#endif  


static void Widen8To16AndDraw(Drawable    drawable,
                              nsXFont     *font,
                              GC           gc,
                              int          x,
                              int          y,
                              const char  *text,
                              int          text_length);


nsresult CreateRenderingContextXlibContext(nsIDeviceContext *aDevice, nsRenderingContextXlibContext **aContext)
{
  nsRenderingContextXlibContext *rcctx;
  
  *aContext = nsnull;
  
  rcctx = new nsRenderingContextXlibContext();
  if (!rcctx)
    return NS_ERROR_OUT_OF_MEMORY;
  
   
  *aContext = rcctx;
  
  return NS_OK;
}

void DeleteRenderingContextXlibContext(nsRenderingContextXlibContext *aContext)
{
  if (aContext) {
    delete aContext;
  }
}

class GraphicsState
{
public:
  GraphicsState();
  ~GraphicsState();

  nsTransform2D           *mMatrix;
  nsCOMPtr<nsIRegion>      mClipRegion;
  nscolor                  mColor;
  nsLineStyle              mLineStyle;
  nsCOMPtr<nsIFontMetrics> mFontMetrics;
};

GraphicsState::GraphicsState() :
  mMatrix(nsnull),
  mColor(NS_RGB(0, 0, 0)),
  mLineStyle(nsLineStyle_kSolid),
  mFontMetrics(nsnull)
{
  MOZ_COUNT_CTOR(GraphicsState);

}

GraphicsState::~GraphicsState()
{
  MOZ_COUNT_DTOR(GraphicsState);
}


nsRenderingContextXlib::nsRenderingContextXlib() :
  nsRenderingContextImpl(),
  mP2T(1.0f),
  mGC(nsnull),
  mFunction(GXcopy),
  mCurrentColor(NS_RGB(0, 0, 0)), 



  mCurrentFont(nsnull),
  mCurrentLineStyle(nsLineStyle_kSolid)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::nsRenderingContextXlib()\n"));

  PushState();
}

nsRenderingContextXlib::~nsRenderingContextXlib()
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::~nsRenderingContextXlib()\n"));
  
  PRInt32 cnt = mStateCache.Count();

  while (--cnt >= 0)
    PopState();

  if (mTranMatrix)
    delete mTranMatrix;
  
  if (mGC)
    mGC->Release();
}

NS_IMETHODIMP
nsRenderingContextXlib::Init(nsIDeviceContext* aContext, nsIWidget *aWindow)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::Init(DeviceContext, Widget)\n"));
  nsDrawingSurfaceXlibImpl *surf; 

  NS_ENSURE_TRUE(aContext != nsnull, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aWindow  != nsnull, NS_ERROR_NULL_POINTER);
  mContext = aContext;
  
  nsIDeviceContext *dc = mContext;     
  NS_STATIC_CAST(nsDeviceContextX *, dc)->GetXlibRgbHandle(mXlibRgbHandle);
  mDisplay = xxlib_rgb_get_display(mXlibRgbHandle);

  surf = new nsDrawingSurfaceXlibImpl();

  if (surf) {
    Drawable  win = (Drawable)aWindow->GetNativeData(NS_NATIVE_WINDOW);
    xGC      *gc  = (xGC *)aWindow->GetNativeData(NS_NATIVE_GRAPHIC);

    surf->Init(mXlibRgbHandle, 
               win, 
               gc);

    mOffscreenSurface = mSurface = surf;
    
    gc->Release();
  }

  return CommonInit();
}

NS_IMETHODIMP
nsRenderingContextXlib::Init(nsIDeviceContext* aContext, nsIDrawingSurface* aSurface)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContxtXlib::Init(DeviceContext, DrawingSurface)\n"));

  NS_ENSURE_TRUE(nsnull != aContext, NS_ERROR_NULL_POINTER);
  mContext = aContext;
  
  nsIDeviceContext *dc = mContext;     
  NS_STATIC_CAST(nsDeviceContextX *, dc)->GetXlibRgbHandle(mXlibRgbHandle);
  mDisplay = xxlib_rgb_get_display(mXlibRgbHandle);

  mSurface = (nsIDrawingSurfaceXlib *)aSurface;
  mOffscreenSurface = mSurface;

  return CommonInit();
}

nsresult nsRenderingContextXlib::CommonInit(void)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContxtXlib::CommonInit()\n"));
  
  int x, y;
  unsigned int width, height, border, depth;
  Window root_win;

  Drawable drawable; mSurface->GetDrawable(drawable);

  ::XGetGeometry(mDisplay, 
                 drawable, 
                 &root_win,
                 &x, 
                 &y, 
                 &width, 
                 &height, 
                 &border, 
                 &depth);

  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, 
         ("XGetGeometry(display=%lx,drawable=%lx) ="
         " {root_win=%lx, x=%d, y=%d, width=%d, height=%d. border=%d, depth=%d}\n",
         (long)mDisplay, (long)drawable,
         (long)root_win, (int)x, (int)y, (int)width, (int)height, (int)border, (int)depth));

  mClipRegion = new nsRegionXlib();
  if (!mClipRegion)
    return NS_ERROR_OUT_OF_MEMORY;
  mClipRegion->Init();
  mClipRegion->SetTo(0, 0, width, height);

  mP2T = mContext->DevUnitsToAppUnits();
  float app2dev;
  app2dev = mContext->AppUnitsToDevUnits();
  mTranMatrix->AddScale(app2dev, app2dev);
  return NS_OK;
}


NS_IMETHODIMP
nsRenderingContextXlib::GetHints(PRUint32& aResult)
{
  PRUint32 result = 0;

  
  
  
  result |= NS_RENDERING_HINT_FAST_8BIT_TEXT;
  




#ifndef CPU_DOES_NOT_REQUIRE_NATURAL_ALIGNMENT
#if defined(__i386)
#define CPU_DOES_NOT_REQUIRE_NATURAL_ALIGNMENT 1
#endif 
#endif 

  static PRBool enable_fast_measure;
  static PRBool getenv_done = PR_FALSE;

  




  if (!getenv_done)
  {
#ifdef CPU_DOES_NOT_REQUIRE_NATURAL_ALIGNMENT
    enable_fast_measure = PR_TRUE;
#else
    enable_fast_measure = PR_FALSE;
#endif 

    if (PR_GetEnv("MOZILLA_GFX_ENABLE_FAST_MEASURE"))
    {
      enable_fast_measure = PR_TRUE;
    }

    if (PR_GetEnv("MOZILLA_GFX_DISABLE_FAST_MEASURE"))
    {
      enable_fast_measure = PR_FALSE;
    }
        
    getenv_done = PR_TRUE;
  } 

  if (enable_fast_measure) {
    
    result |= NS_RENDERING_HINT_FAST_MEASURE;
  }

  
  

  aResult = result;
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::LockDrawingSurface(PRInt32 aX, PRInt32 aY,
                                           PRUint32 aWidth, PRUint32 aHeight,
                                           void **aBits, PRInt32 *aStride,
                                           PRInt32 *aWidthBytes, PRUint32 aFlags)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::LockDrawingSurface()\n"));
  PushState();

  return mSurface->Lock(aX, aY, aWidth, aHeight,
                        aBits, aStride, aWidthBytes, aFlags);
}

NS_IMETHODIMP
nsRenderingContextXlib::UnlockDrawingSurface(void)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::UnlockDrawingSurface()\n"));
  PopState();

  mSurface->Unlock();
  
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::SelectOffScreenDrawingSurface(nsIDrawingSurface* aSurface)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::SelectOffScreenDrawingSurface()\n"));
  if (nsnull == aSurface)
    mSurface = mOffscreenSurface;
  else
    mSurface = (nsIDrawingSurfaceXlib *)aSurface;

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetDrawingSurface(nsIDrawingSurface* *aSurface)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::GetDrawingSurface()\n"));
  *aSurface = mSurface;
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::Reset()
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::Reset()\n"));
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextXlib::GetDeviceContext(nsIDeviceContext *&aContext)
{
  aContext = mContext;
  NS_IF_ADDREF(aContext);
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::PushState(void)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::PushState()\n"));
  GraphicsState *state = new GraphicsState();

  if (!state)
    return NS_ERROR_OUT_OF_MEMORY;

  state->mMatrix = mTranMatrix;
  
  mStateCache.AppendElement(state);
  
  if (nsnull == mTranMatrix)
    mTranMatrix = new nsTransform2D();
  else
    mTranMatrix = new nsTransform2D(mTranMatrix);
  
  if (mClipRegion) {
    state->mClipRegion = mClipRegion;
    mClipRegion = new nsRegionXlib();
    if (!mClipRegion)
      return NS_ERROR_OUT_OF_MEMORY;
    mClipRegion->Init();
    mClipRegion->SetTo(*state->mClipRegion);
  }
  
  state->mFontMetrics = mFontMetrics;
  state->mColor       = mCurrentColor;
  state->mLineStyle   = mCurrentLineStyle;
  
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::PopState(void)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::PopState()\n"));

  PRUint32 cnt = mStateCache.Count();
  GraphicsState *state;
  
  if (cnt > 0) {
    state = (GraphicsState *)mStateCache.ElementAt(cnt - 1);
    mStateCache.RemoveElementAt(cnt - 1);
    
    if (mTranMatrix)
      delete mTranMatrix;
    mTranMatrix = state->mMatrix;
    
    mClipRegion = state->mClipRegion;
    if (mFontMetrics != state->mFontMetrics)
      SetFont(state->mFontMetrics);

    if (state->mColor != mCurrentColor)
      SetColor(state->mColor);
    if (state->mLineStyle != mCurrentLineStyle)
      SetLineStyle(state->mLineStyle);

    delete state;
  }

  return NS_OK;
}

#ifdef DEBUG
#undef TRACE_SET_CLIP
#endif

#ifdef TRACE_SET_CLIP
static char *
nsClipCombine_to_string(nsClipCombine aCombine)
{
#ifdef TRACE_SET_CLIP
  printf("nsRenderingContextXlib::SetClipRect(x=%d,y=%d,w=%d,h=%d,%s)\n",
         trect.x,
         trect.y,
         trect.width,
         trect.height,
         nsClipCombine_to_string(aCombine));
#endif 

  switch(aCombine)
  {
    case nsClipCombine_kIntersect:
      return "nsClipCombine_kIntersect";
      break;

    case nsClipCombine_kUnion:
      return "nsClipCombine_kUnion";
      break;

    case nsClipCombine_kSubtract:
      return "nsClipCombine_kSubtract";
      break;

    case nsClipCombine_kReplace:
      return "nsClipCombine_kReplace";
      break;
  }

  return "something got screwed";
}
#endif 

NS_IMETHODIMP
nsRenderingContextXlib::IsVisibleRect(const nsRect& aRect, PRBool &aVisible)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::IsVisibleRect()\n"));
  aVisible = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP 
nsRenderingContextXlib::SetClipRect(const nsRect& aRect,
                                    nsClipCombine aCombine)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::SetClipRect()\n"));
  nsRect trect = aRect;
  mTranMatrix->TransformCoord(&trect.x, &trect.y,
                              &trect.width, &trect.height);
  SetClipRectInPixels(trect, aCombine);
  return NS_OK;
}

void nsRenderingContextXlib::SetClipRectInPixels(const nsRect& aRect,
                                                 nsClipCombine aCombine)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::SetClipRectInPixels()\n"));

  switch(aCombine)
  {
    case nsClipCombine_kIntersect:
      mClipRegion->Intersect(aRect.x,aRect.y,aRect.width,aRect.height);
      break;
    case nsClipCombine_kUnion:
      mClipRegion->Union(aRect.x,aRect.y,aRect.width,aRect.height);
      break;
    case nsClipCombine_kSubtract:
      mClipRegion->Subtract(aRect.x,aRect.y,aRect.width,aRect.height);
      break;
    case nsClipCombine_kReplace:
      mClipRegion->SetTo(aRect.x,aRect.y,aRect.width,aRect.height);
      break;
  }
}

NS_IMETHODIMP
nsRenderingContextXlib::GetClipRect(nsRect &aRect, PRBool &aClipState)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::GetClipRext()\n"));
  PRInt32 x, y, w, h;
  if (!mClipRegion->IsEmpty()) {
    mClipRegion->GetBoundingBox(&x,&y,&w,&h);
    aRect.SetRect(x,y,w,h);
    aClipState = PR_TRUE;
  } else {
    aRect.SetRect(0,0,0,0);
    aClipState = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::SetClipRegion()\n"));
  switch(aCombine)
  {
    case nsClipCombine_kIntersect:
      mClipRegion->Intersect(aRegion);
      break;
    case nsClipCombine_kUnion:
      mClipRegion->Union(aRegion);
      break;
    case nsClipCombine_kSubtract:
      mClipRegion->Subtract(aRegion);
      break;
    case nsClipCombine_kReplace:
      mClipRegion->SetTo(aRegion);
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::CopyClipRegion(nsIRegion &aRegion)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::CopyClipRegion()\n"));
  aRegion.SetTo(*mClipRegion);
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetClipRegion(nsIRegion **aRegion)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::GetClipRegion()\n"));
  nsresult  rv = NS_OK;
  
  NS_ASSERTION(!(nsnull == aRegion), "no region ptr");
  
  if (*aRegion) {
    (*aRegion)->SetTo(*mClipRegion);
  }
  return rv;
}

void nsRenderingContextXlib::UpdateGC()
{
   PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::UpdateGC()\n"));
   XGCValues     values;
   unsigned long valuesMask = 0;

   Drawable drawable; mOffscreenSurface->GetDrawable(drawable);
 
   if (mGC)
     mGC->Release();
 
   memset(&values, 0, sizeof(XGCValues));
 
   unsigned long color;
   color = xxlib_rgb_xpixel_from_rgb (mXlibRgbHandle,
                                      NS_RGB(NS_GET_B(mCurrentColor),
                                             NS_GET_G(mCurrentColor),
                                             NS_GET_R(mCurrentColor)));
   values.foreground = color;
   valuesMask |= GCForeground;

   if (mCurrentFont && mCurrentFont->GetXFontStruct()) {
     valuesMask |= GCFont;
     values.font = mCurrentFont->GetXFontStruct()->fid;
   }
 
   values.line_style = mLineStyle;
   valuesMask |= GCLineStyle;
 
   values.function = mFunction;
   valuesMask |= GCFunction;

   Region rgn = nsnull;
   if (mClipRegion) { 
     mClipRegion->GetNativeRegion((void*&)rgn);
   }

   nsRenderingContextXlibContext *rcContext;
   nsIDeviceContext *dc = mContext;
   NS_STATIC_CAST(nsDeviceContextX *, dc)->GetRCContext(rcContext);
   
   mGC = rcContext->mGcCache.GetGC(mDisplay, drawable,
                                  valuesMask, &values, rgn);
}

NS_IMETHODIMP
nsRenderingContextXlib::SetColor(nscolor aColor)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::SetColor(nscolor)\n"));
  NS_ENSURE_TRUE(mContext != nsnull, NS_ERROR_FAILURE);

  mCurrentColor = aColor;

  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("Setting color to %d %d %d\n", NS_GET_R(aColor), NS_GET_G(aColor), NS_GET_B(aColor)));
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetColor(nscolor &aColor) const
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::GetColor()\n"));

  aColor = mCurrentColor;
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::SetFont(const nsFont& aFont, nsIAtom* aLangGroup)
{
  nsCOMPtr<nsIFontMetrics> newMetrics;
  nsresult rv = mContext->GetMetricsFor( aFont, aLangGroup, *getter_AddRefs(newMetrics) );

  if (NS_SUCCEEDED(rv)) {
    rv = SetFont(newMetrics);
  }
  return rv;
}

NS_IMETHODIMP
nsRenderingContextXlib::SetFont(nsIFontMetrics *aFontMetrics)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::SetFont()\n"));

  mFontMetrics = aFontMetrics;

  if (mFontMetrics)
  {
    nsFontHandle  fontHandle;
    mFontMetrics->GetFontHandle(fontHandle);
    mCurrentFont = (nsFontXlib *)fontHandle;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::SetLineStyle(nsLineStyle aLineStyle)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::SetLineStyle()\n"));

  if (aLineStyle != mCurrentLineStyle) {
    





















    mCurrentLineStyle = aLineStyle ;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetLineStyle(nsLineStyle &aLineStyle)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::GetLineStyle()\n"));
  aLineStyle = mCurrentLineStyle;
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetFontMetrics(nsIFontMetrics *&aFontMetrics)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::GetFontMetrics()\n"));

  aFontMetrics = mFontMetrics;
  NS_IF_ADDREF(aFontMetrics);
  return NS_OK;
}


NS_IMETHODIMP
nsRenderingContextXlib::Translate(nscoord aX, nscoord aY)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::Translate()\n"));
  mTranMatrix->AddTranslation((float)aX,(float)aY);
  return NS_OK;
}


NS_IMETHODIMP
nsRenderingContextXlib::Scale(float aSx, float aSy)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::Scale()\n"));
  mTranMatrix->AddScale(aSx, aSy);
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetCurrentTransform(nsTransform2D *&aTransform)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::GetCurrentTransform()\n"));
  aTransform = mTranMatrix;
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::CreateDrawingSurface(const nsRect& aBounds,
                                             PRUint32 aSurfFlags,
                                             nsIDrawingSurface* &aSurface)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::CreateDrawingSurface()\n"));

  if (nsnull == mSurface) {
    aSurface = nsnull;
    return NS_ERROR_FAILURE;
  }

  NS_ENSURE_TRUE((aBounds.width > 0) && (aBounds.height > 0), NS_ERROR_FAILURE);
 
  nsresult rv = NS_ERROR_FAILURE;
  nsDrawingSurfaceXlibImpl *surf = new nsDrawingSurfaceXlibImpl();

  if (surf)
  {
    NS_ADDREF(surf);
    UpdateGC();
    rv = surf->Init(mXlibRgbHandle, mGC, aBounds.width, aBounds.height, aSurfFlags);    
  }

  aSurface = surf;

  return rv;
}

NS_IMETHODIMP
nsRenderingContextXlib::DestroyDrawingSurface(nsIDrawingSurface* aDS)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DestroyDrawingSurface()\n"));
  nsIDrawingSurfaceXlib *surf = NS_STATIC_CAST(nsIDrawingSurfaceXlib *, aDS);;

  NS_ENSURE_TRUE(surf != nsnull, NS_ERROR_FAILURE);

  NS_IF_RELEASE(surf);

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawLine()\n"));

  nscoord diffX, diffY;

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  mTranMatrix->TransformCoord(&aX0,&aY0);
  mTranMatrix->TransformCoord(&aX1,&aY1);
  
  diffX = aX1-aX0;
  diffY = aY1-aY0;

  if (0!=diffX) {
    diffX = (diffX>0?1:-1);
  }
  if (0!=diffY) {
    diffY = (diffY>0?1:-1);
  }

  UpdateGC();
  Drawable drawable; mSurface->GetDrawable(drawable);
  ::XDrawLine(mDisplay, drawable,
              *mGC, aX0, aY0, aX1 - diffX, aY1 - diffY);

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawPolyLine()\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  PRInt32  i;
  XPoint * xpoints;
  XPoint * thispoint;

  xpoints = (XPoint *) malloc(sizeof(XPoint) * aNumPoints);
  NS_ENSURE_TRUE(xpoints != nsnull, NS_ERROR_OUT_OF_MEMORY);

  for (i = 0; i < aNumPoints; i++){
    thispoint = (xpoints+i);
    thispoint->x = aPoints[i].x;
    thispoint->y = aPoints[i].y;
    mTranMatrix->TransformCoord((PRInt32*)&thispoint->x,(PRInt32*)&thispoint->y);
  }

  UpdateGC();
  Drawable drawable; mSurface->GetDrawable(drawable);
  ::XDrawLines(mDisplay,
               drawable,
               *mGC,
               xpoints, aNumPoints, CoordModeOrigin);

  free((void *)xpoints);

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawRect(const nsRect& aRect)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawRext(const nsRect& aRect)\n"));
  return DrawRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawRect(aX, aY, aWidth, aHeight)\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  nscoord x, y, w, h; 
  
  x = aX;
  y = aY; 
  w = aWidth;
  h = aHeight;
    
  mTranMatrix->TransformCoord(&x,&y,&w,&h);
    
  
  
  
  ConditionRect(x,y,w,h);
   
  
  
  if (w && h) 
  {
    UpdateGC();
    Drawable drawable; mSurface->GetDrawable(drawable);
    ::XDrawRectangle(mDisplay,
                     drawable,
                     *mGC,
                     x,
                     y,
                     w-1,
                     h-1);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsRenderingContextXlib::FillRect(const nsRect& aRect)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::FillRect()\n"));
  return FillRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
nsRenderingContextXlib::FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::FillRect()\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  nscoord x,y,w,h;
  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTranMatrix->TransformCoord(&x,&y,&w,&h);

  
  
  
  ConditionRect(x,y,w,h);

  Drawable drawable; mSurface->GetDrawable(drawable);
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("About to fill window 0x%lxd with rect %d %d %d %d\n",
                                                drawable, x, y, w, h));
  UpdateGC();
  ::XFillRectangle(mDisplay,
                   drawable,
                   *mGC,
                   x,y,w,h);
  
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::InvertRect(const nsRect& aRect)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::InvertRect()\n"));
  return InvertRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP 
nsRenderingContextXlib::InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::InvertRect()\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);
  
  
  
  nscolor backupColor = mCurrentColor;
  mCurrentColor = NS_RGB(255, 255, 255);
  nscoord x,y,w,h;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTranMatrix->TransformCoord(&x,&y,&w,&h);

  
  
  
  ConditionRect(x,y,w,h);

  mFunction = GXxor;
  
  UpdateGC();
  Drawable drawable; mSurface->GetDrawable(drawable);
  ::XFillRectangle(mDisplay,
                   drawable,
                   *mGC,
                   x,
                   y,
                   w,
                   h);
  
  mFunction = GXcopy;

  
  mCurrentColor = backupColor;

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawPolygon()\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  PRInt32 i ;
  XPoint * xpoints;
  XPoint * thispoint;
  
  xpoints = (XPoint *) malloc(sizeof(XPoint) * aNumPoints);
  NS_ENSURE_TRUE(xpoints != nsnull, NS_ERROR_OUT_OF_MEMORY);
  
  for (i = 0; i < aNumPoints; i++){
    thispoint = (xpoints+i);
    thispoint->x = aPoints[i].x;
    thispoint->y = aPoints[i].y;
    mTranMatrix->TransformCoord((PRInt32*)&thispoint->x,(PRInt32*)&thispoint->y);
  }
  
  UpdateGC();
  Drawable drawable; mSurface->GetDrawable(drawable);
  ::XDrawLines(mDisplay,
               drawable,
               *mGC,
               xpoints, aNumPoints, CoordModeOrigin);

  free((void *)xpoints);
  
  return NS_OK;    
}

NS_IMETHODIMP
nsRenderingContextXlib::FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::FillPolygon()\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  PRInt32 i ;
  XPoint * xpoints;
   
  xpoints = (XPoint *) malloc(sizeof(XPoint) * aNumPoints);
  NS_ENSURE_TRUE(xpoints != nsnull, NS_ERROR_OUT_OF_MEMORY);
  
  for (i = 0; i < aNumPoints; ++i) {
    nsPoint p = aPoints[i];
    mTranMatrix->TransformCoord(&p.x, &p.y);
    xpoints[i].x = p.x;
    xpoints[i].y = p.y;
  } 
    
  UpdateGC();
  Drawable drawable; mSurface->GetDrawable(drawable);
  ::XFillPolygon(mDisplay,
                 drawable,
                 *mGC,
                 xpoints, aNumPoints, Complex, CoordModeOrigin);
               
  free((void *)xpoints);

  return NS_OK; 
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawEllipse(const nsRect& aRect)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawEllipse(const nsRect& aRect)\n"));
  return DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawEllipse(aX, aY, aWidth, aHeight)\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  nscoord x, y, w, h;
   
  x = aX; 
  y = aY;
  w = aWidth; 
  h = aHeight;
    
  mTranMatrix->TransformCoord(&x,&y,&w,&h);
    
  UpdateGC();
  Drawable drawable; mSurface->GetDrawable(drawable);
  ::XDrawArc(mDisplay,
             drawable,
             *mGC,
             x, y, w, h, 0, 360*64);
  
  return NS_OK;  
}

NS_IMETHODIMP
nsRenderingContextXlib::FillEllipse(const nsRect& aRect)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::FillEllipse()\n"));
  return FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
nsRenderingContextXlib::FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::FillEllipse()\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  nscoord x,y,w,h;
   
  x = aX; 
  y = aY;
  w = aWidth; 
  h = aHeight;
    
  mTranMatrix->TransformCoord(&x,&y,&w,&h);
    
  UpdateGC();
  Drawable drawable; mSurface->GetDrawable(drawable);
  if (w < 16 || h < 16) {
    


    ::XDrawArc(mDisplay,
               drawable,
               *mGC,
               x, y, w, h, 0, 360*64);
    
  }
  ::XFillArc(mDisplay,
             drawable,
             *mGC,
             x, y, w, h, 0, 360*64);
  
  return NS_OK;  
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawArc(const nsRect& aRect,
                                float aStartAngle, float aEndAngle)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawArc()\n"));
  return DrawArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle); 
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                float aStartAngle, float aEndAngle)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawArc()\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  nscoord x, y, w, h;
   
  x = aX; 
  y = aY;
  w = aWidth; 
  h = aHeight;
    
  mTranMatrix->TransformCoord(&x,&y,&w,&h);
    
  UpdateGC();
  Drawable drawable; mSurface->GetDrawable(drawable);
  ::XDrawArc(mDisplay,
             drawable,
             *mGC,
             x,y,w,h, NSToIntRound(aStartAngle * 64.0f),
             NSToIntRound(aEndAngle * 64.0f));
  
  return NS_OK;  
}

NS_IMETHODIMP
nsRenderingContextXlib::FillArc(const nsRect& aRect,
                                float aStartAngle, float aEndAngle)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::FillArc()\n"));
  return FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

NS_IMETHODIMP
nsRenderingContextXlib::FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                float aStartAngle, float aEndAngle)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::FillArc()\n"));

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);

  nscoord x,y,w,h;
   
  x = aX; 
  y = aY;
  w = aWidth; 
  h = aHeight;
    
  mTranMatrix->TransformCoord(&x,&y,&w,&h);
    
  UpdateGC();
  Drawable drawable; mSurface->GetDrawable(drawable);
  ::XFillArc(mDisplay,
             drawable,
             *mGC,
             x,y,w,h, NSToIntRound(aStartAngle * 64.0f),
             NSToIntRound(aEndAngle * 64.0f));
  
  return NS_OK;  
}



#define WIDEN_8_TO_16_BUF_SIZE (1024)


static
int Widen8To16AndMove(const char *char_p, 
                      int char_len, 
                      XChar2b *xchar2b_p)
{
  int i;
  for (i=0; i<char_len; i++) {
    (xchar2b_p)->byte1 = 0;
    (xchar2b_p++)->byte2 = *char_p++;
  }
  return(char_len*2);
}


static
int Widen8To16AndGetWidth(nsXFont    *xFont,
                          const char *text,
                          int         text_length)
{
  NS_ASSERTION(!xFont->IsSingleByte(), "Widen8To16AndGetWidth: wrong string/font size");
  XChar2b  buf[WIDEN_8_TO_16_BUF_SIZE];
  XChar2b *p = buf;
  int uchar_size;
  int rawWidth;

  if (text_length > WIDEN_8_TO_16_BUF_SIZE) {
    p = (XChar2b*)malloc(text_length*sizeof(XChar2b));
    if (!p)
      return(0); 
  }

  uchar_size = Widen8To16AndMove(text, text_length, p);
  rawWidth = xFont->TextWidth16(p, uchar_size/2);

  if (text_length > WIDEN_8_TO_16_BUF_SIZE) {
    free((char*)p);
  }
  return(rawWidth);
}

static 
void Widen8To16AndDraw(Drawable     drawable,
                       nsXFont     *xFont,
                       GC           gc,
                       int          x,
                       int          y,
                       const char  *text,
                       int          text_length)
{
  NS_ASSERTION(!xFont->IsSingleByte(), "Widen8To16AndDraw: wrong string/font size");
  XChar2b buf[WIDEN_8_TO_16_BUF_SIZE];
  XChar2b *p = buf;
  int uchar_size;

  if (text_length > WIDEN_8_TO_16_BUF_SIZE) {
    p = (XChar2b*)malloc(text_length*sizeof(XChar2b));
    if (!p)
      return; 
  }

  uchar_size = Widen8To16AndMove(text, text_length, p);
  xFont->DrawText16(drawable, gc, x, y, p, uchar_size/2);

  if (text_length > WIDEN_8_TO_16_BUF_SIZE) {
    free((char*)p);
  }
}

NS_IMETHODIMP
nsRenderingContextXlib::GetWidth(char aC, nscoord &aWidth)
{
  
  
  if ((aC == ' ') && (nsnull != mFontMetrics)) {
    return mFontMetrics->GetSpaceWidth(aWidth);
  }
  return GetWidth(&aC, 1, aWidth);
}

NS_IMETHODIMP
nsRenderingContextXlib::GetWidth(PRUnichar aC, nscoord& aWidth,
                                PRInt32* aFontID)
{
  return GetWidth(&aC, 1, aWidth, aFontID);
}

NS_IMETHODIMP
nsRenderingContextXlib::GetWidth(const nsString& aString,
                                 nscoord& aWidth, PRInt32* aFontID)
{
  return GetWidth(aString.get(), aString.Length(), aWidth, aFontID);
}

NS_IMETHODIMP
nsRenderingContextXlib::GetWidth(const char* aString, nscoord& aWidth)
{
  return GetWidth(aString, strlen(aString), aWidth);
}

NS_IMETHODIMP
nsRenderingContextXlib::GetWidth(const char* aString, PRUint32 aLength,
                                 nscoord& aWidth)
{
  if (0 == aLength) {
    aWidth = 0;
  }
  else {
    NS_ENSURE_TRUE(aString      != nsnull, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(mCurrentFont != nsnull, NS_ERROR_FAILURE);
    int rawWidth;
    nsXFont *xFont = mCurrentFont->GetXFont();
#ifdef USE_FREETYPE
    if (mCurrentFont->IsFreeTypeFont()) {
      
      rawWidth = mCurrentFont->
        GetWidth(NS_ConvertASCIItoUTF16(aString, aLength).get(), aLength);
    }
    else
#endif 
    if (!mCurrentFont->GetXFontIs10646()) {
      NS_ASSERTION(xFont->IsSingleByte(), "wrong string/font size");
      
      rawWidth = xFont->TextWidth8(aString, aLength);
    }
    else {
      NS_ASSERTION(!xFont->IsSingleByte(), "wrong string/font size");
      
      rawWidth = Widen8To16AndGetWidth (mCurrentFont->GetXFont(), aString, aLength);
    }
    aWidth = NSToCoordRound(rawWidth * mP2T);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetWidth(const PRUnichar* aString, PRUint32 aLength,
                                 nscoord& aWidth, PRInt32* aFontID)
{
  if (0 == aLength) {
    aWidth = 0;
  }
  else {
    NS_ENSURE_TRUE(aString != nsnull, NS_ERROR_FAILURE);

    nsFontMetricsXlib *metrics = NS_REINTERPRET_CAST(nsFontMetricsXlib *, mFontMetrics.get());

    NS_ENSURE_TRUE(metrics != nsnull, NS_ERROR_FAILURE);

    nsFontXlib *prevFont = nsnull;
    int rawWidth = 0;
    PRUint32 start = 0;
    PRUint32 i;
    for (i = 0; i < aLength; i++) {
      PRUnichar c = aString[i];
      nsFontXlib  *currFont = nsnull;
      nsFontXlib **font = metrics->mLoadedFonts;
      nsFontXlib **end = &metrics->mLoadedFonts[metrics->mLoadedFontsCount];
      while (font < end) {
        if (CCMAP_HAS_CHAR((*font)->mCCMap, c)) {
          currFont = *font;
          goto FoundFont; 
        }
        font++;
      }
      currFont = metrics->FindFont(c);
FoundFont:
      
      if (prevFont) {
        if (currFont != prevFont) {
          rawWidth += prevFont->GetWidth(&aString[start], i - start);
          prevFont = currFont;
          start = i;
        }
      }
      else {
        prevFont = currFont;
        start = i;
      }
    }

    if (prevFont) {
      rawWidth += prevFont->GetWidth(&aString[start], i - start);
    }

    aWidth = NSToCoordRound(rawWidth * mP2T);
  }
  if (nsnull != aFontID)
    *aFontID = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetTextDimensions(const char*       aString,
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

      
      nscoord twWidth = 0;
      if ((1 == numChars) && (aString[start] == ' ')) {
        mFontMetrics->GetSpaceWidth(twWidth);
      } 
      else if (numChars > 0)
        GetWidth( &aString[start], numChars, twWidth);

      
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
          else if (numChars > 0)
            GetWidth( &aString[start], numChars, twWidth);

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
do_BreakGetTextDimensions(const nsFontSwitchXlib *aFontSwitch,
                          const PRUnichar*        aSubstring,
                          PRUint32                aSubstringLength,
                          void*                   aData)
{
  nsFontXlib *fontXlib = aFontSwitch->mFontXlib;

  
  BreakGetTextDimensionsData* data = (BreakGetTextDimensionsData*)aData;

  
  
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
      pxWidth = fontXlib->GetWidth(&pstr[start], numChars);
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
          
          pxWidth = fontXlib->GetWidth(&pstr[start], numChars);
          width -= NSToCoordRound(float(pxWidth) * data->mP2T);
        }
        else {
          
          
          
          
          end = data->mNumCharsFit; 
          data->mNumCharsFit = numCharsFit; 
          PRInt32 k = data->mFonts->Count() - 1;
          for ( ; k >= 0 && start < end; --k, end -= numChars) {
            fontXlib = (nsFontXlib*)data->mFonts->ElementAt(k);
            const PRUnichar* ps = (const PRUnichar*)data->mOffsets->ElementAt(k);
            if (ps < pstr + start)
              ps = pstr + start;

            numChars = pstr + end - ps;
            NS_ASSERTION(numChars > 0, "empty string");

            pxWidth = fontXlib->GetWidth(ps, numChars);
            data->mWidth -= NSToCoordRound(float(pxWidth) * data->mP2T);

            
            
            data->mFonts->RemoveElementAt(k);
            data->mOffsets->RemoveElementAt(k+1);
          }

          
          
          data->mFonts->AppendElement(fontXlib);
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
    data->mFonts->AppendElement(fontXlib);
    data->mOffsets->AppendElement((void*)&pstr[numCharsFit]);
  }

  if (allDone) {
    
    return PR_FALSE;
  }

  return PR_TRUE; 
}

NS_IMETHODIMP
nsRenderingContextXlib::GetTextDimensions(const PRUnichar*  aString,
                                          PRInt32           aLength,
                                          PRInt32           aAvailWidth,
                                          PRInt32*          aBreaks,
                                          PRInt32           aNumBreaks,
                                          nsTextDimensions& aDimensions,
                                          PRInt32&          aNumCharsFit,
                                          nsTextDimensions& aLastWordDimensions,
                                          PRInt32*          aFontID)
{
  if (!mFontMetrics)
    return NS_ERROR_FAILURE;

  nsFontMetricsXlib *metrics = NS_REINTERPRET_CAST(nsFontMetricsXlib *, mFontMetrics.get());

  nscoord spaceWidth, aveCharWidth;
  metrics->GetSpaceWidth(spaceWidth);
  metrics->GetAveCharWidth(aveCharWidth);

  
  
  
  

  
  

  nsAutoVoidArray fonts, offsets;
  offsets.AppendElement((void*)aString);

  BreakGetTextDimensionsData data = { mP2T, aAvailWidth, aBreaks, aNumBreaks,
    spaceWidth, aveCharWidth, 0, 0, 0, -1, 0, &fonts, &offsets 
  };

  metrics->ResolveForwards(aString, aLength, do_BreakGetTextDimensions, &data);

  if (aFontID) *aFontID = 0;

  aNumCharsFit = data.mNumCharsFit;
  aDimensions.width = data.mWidth;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  aLastWordDimensions.Clear();
  aLastWordDimensions.width = -1;

  PRInt32 count = fonts.Count();
  if (!count)
    return NS_OK;
  nsFontXlib *fontXlib = (nsFontXlib *)fonts[0];
  NS_ASSERTION(fontXlib, "internal error in do_BreakGetTextDimensions");
  aDimensions.ascent = fontXlib->mMaxAscent;
  aDimensions.descent = fontXlib->mMaxDescent;

  
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
    fontXlib = (nsFontXlib*)fonts[currFont];
    PRUnichar* nextOffset = (PRUnichar*)offsets[++currFont]; 

    
    
    
    
    
    
    
    
    
    
    if (*pstr == ' ') {
      
      ++pstr;
      
      if (pstr == last) {
        break;
      }
      
      if (pstr == nextOffset) {
        fontXlib = (nsFontXlib*)fonts[currFont];
        nextOffset = (PRUnichar*)offsets[++currFont];
      } 
    }

    
    
    
    if (nextOffset > lastWord) {
      if (aLastWordDimensions.ascent < fontXlib->mMaxAscent) {
        aLastWordDimensions.ascent = fontXlib->mMaxAscent;
      }
      if (aLastWordDimensions.descent < fontXlib->mMaxDescent) {
        aLastWordDimensions.descent = fontXlib->mMaxDescent;
      }
    }

    
    if (pstr < lastWord) {
      if (aDimensions.ascent < fontXlib->mMaxAscent) {
        aDimensions.ascent = fontXlib->mMaxAscent;
      }
      if (aDimensions.descent < fontXlib->mMaxDescent) {
        aDimensions.descent = fontXlib->mMaxDescent;
      }
    }

    
    pstr = nextOffset;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetTextDimensions(const char* aString, PRUint32 aLength,
                                          nsTextDimensions& aDimensions)
{
  mFontMetrics->GetMaxAscent(aDimensions.ascent);
  mFontMetrics->GetMaxDescent(aDimensions.descent);
  return GetWidth(aString, aLength, aDimensions.width);
}

NS_IMETHODIMP
nsRenderingContextXlib::GetTextDimensions(const PRUnichar* aString, PRUint32 aLength,
                                          nsTextDimensions& aDimensions, PRInt32* aFontID)
{
  aDimensions.Clear();
  if (0 < aLength) {
    NS_ENSURE_TRUE(aString != nsnull, NS_ERROR_FAILURE);

    nsFontMetricsXlib *metrics = NS_REINTERPRET_CAST(nsFontMetricsXlib *, mFontMetrics.get());

    NS_ENSURE_TRUE(metrics != nsnull, NS_ERROR_FAILURE);

    nsFontXlib* prevFont = nsnull;
    int rawWidth = 0, rawAscent = 0, rawDescent = 0;
    PRUint32 start = 0;
    PRUint32 i;
    for (i = 0; i < aLength; i++) {
      PRUnichar c = aString[i];
      nsFontXlib  *currFont = nsnull;
      nsFontXlib **font = metrics->mLoadedFonts;
      nsFontXlib **end = &metrics->mLoadedFonts[metrics->mLoadedFontsCount];
      while (font < end) {
        if (CCMAP_HAS_CHAR((*font)->mCCMap, c)) {
          currFont = *font;
          goto FoundFont; 
        }
        font++;
      }
      currFont = metrics->FindFont(c);
FoundFont:
      
      if (prevFont) {
        if (currFont != prevFont) {
          rawWidth += prevFont->GetWidth(&aString[start], i - start);
          if (rawAscent < prevFont->mMaxAscent)
            rawAscent = prevFont->mMaxAscent;
          if (rawDescent < prevFont->mMaxDescent)
            rawDescent = prevFont->mMaxDescent;
          prevFont = currFont;
          start = i;
        }
      }
      else {
        prevFont = currFont;
        start = i;
      }
    }

    if (prevFont) {
      rawWidth += prevFont->GetWidth(&aString[start], i - start);
      if (rawAscent < prevFont->mMaxAscent)
        rawAscent = prevFont->mMaxAscent;
      if (rawDescent < prevFont->mMaxDescent)
        rawDescent = prevFont->mMaxDescent;
    }

    aDimensions.width = NSToCoordRound(rawWidth * mP2T);
    aDimensions.ascent = NSToCoordRound(rawAscent * mP2T);
    aDimensions.descent = NSToCoordRound(rawDescent * mP2T);
  }
  if (nsnull != aFontID)
    *aFontID = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawString(const char *aString, PRUint32 aLength,
                                   nscoord aX, nscoord aY,
                                   const nscoord* aSpacing)
{
  nsresult res = NS_OK;

  if (aLength) {
    NS_ENSURE_TRUE(mTranMatrix  != nsnull, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(mSurface     != nsnull, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(aString      != nsnull, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(mCurrentFont != nsnull, NS_ERROR_FAILURE);

    nscoord x = aX;
    nscoord y = aY;

    UpdateGC();

    nsXFont *xFont = mCurrentFont->GetXFont();
    if (nsnull != aSpacing) {
      
      const char* end = aString + aLength;
      while (aString < end) {
        char ch = *aString++;
        nscoord xx = x;
        nscoord yy = y;
        mTranMatrix->TransformCoord(&xx, &yy);
#ifdef USE_FREETYPE
        if (mCurrentFont->IsFreeTypeFont()) {
          
          res = mCurrentFont->DrawString(this, mSurface, xx, yy, 
                            NS_ConvertASCIItoUTF16(aString, aLength).get(),
                            aLength);
        }
        else
#endif 
        if (!mCurrentFont->GetXFontIs10646()) {
          
          NS_ASSERTION(xFont->IsSingleByte(),"wrong string/font size");
          xFont->DrawText8(mSurface->GetDrawable(), *mGC, xx, yy, &ch, 1);
        }
        else {
          
          NS_ASSERTION(!xFont->IsSingleByte(),"wrong string/font size");
          Widen8To16AndDraw(mSurface->GetDrawable(), xFont, *mGC,
                                                xx, yy, &ch, 1);
        }
        x += *aSpacing++;
      }
    }
    else {
      mTranMatrix->TransformCoord(&x, &y);
#ifdef USE_FREETYPE
      if (mCurrentFont->IsFreeTypeFont()) {
        
        res = mCurrentFont->DrawString(this, mSurface, x, y, 
                          NS_ConvertASCIItoUTF16(aString, aLength).get(),
                          aLength);
      }
      else
#endif 
      if (!mCurrentFont->GetXFontIs10646()) { 
        
        NS_ASSERTION(xFont->IsSingleByte(),"wrong string/font size");
        xFont->DrawText8(mSurface->GetDrawable(), *mGC, x, y, aString, aLength);
      }
      else {
        
        NS_ASSERTION(!xFont->IsSingleByte(),"wrong string/font size");
        Widen8To16AndDraw(mSurface->GetDrawable(), xFont, *mGC,
                                             x, y, aString, aLength);
      }
    }
  }

#ifdef DISABLED_FOR_NOW
  
  
  if (mFontMetrics)
  {
    PRUint8 deco = mFontMetrics->Font().decorations;

    if (deco & NS_FONT_DECORATION_OVERLINE)
      DrawLine(aX, aY, aX + aWidth, aY);

    if (deco & NS_FONT_DECORATION_UNDERLINE)
    {
      nscoord ascent,descent;

      mFontMetrics->GetMaxAscent(ascent);
      mFontMetrics->GetMaxDescent(descent);

      DrawLine(aX, aY + ascent + (descent >> 1),
               aX + aWidth, aY + ascent + (descent >> 1));
    }

    if (deco & NS_FONT_DECORATION_LINE_THROUGH)
    {
      nscoord height;

      mFontMetrics->GetHeight(height);

      DrawLine(aX, aY + (height >> 1), aX + aWidth, aY + (height >> 1));
    }
  }
#endif 

  return res;
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawString(const PRUnichar* aString, PRUint32 aLength,
                                   nscoord aX, nscoord aY,
                                   PRInt32 aFontID,
                                   const nscoord* aSpacing)
{
  if (aLength && mFontMetrics) {
    NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(aString     != nsnull, NS_ERROR_FAILURE);

    nscoord x = aX;
    nscoord y = aY;

    mTranMatrix->TransformCoord(&x, &y);

    nsFontMetricsXlib *metrics = NS_REINTERPRET_CAST(nsFontMetricsXlib *, mFontMetrics.get());
    nsFontXlib *prevFont = nsnull;
    PRUint32 start = 0;
    PRUint32 i;
    for (i = 0; i < aLength; i++) {
      PRUnichar c = aString[i];
      nsFontXlib* currFont = nsnull;
      nsFontXlib** font = metrics->mLoadedFonts;
      nsFontXlib** lastFont = &metrics->mLoadedFonts[metrics->mLoadedFontsCount];
      while (font < lastFont) {
        if (CCMAP_HAS_CHAR((*font)->mCCMap, c)) {
          currFont = *font;
          goto FoundFont; 
        }
        font++;
      }
      currFont = metrics->FindFont(c);
FoundFont:
      
      if (prevFont) {
        if (currFont != prevFont) {
          if (aSpacing) {
            const PRUnichar* str = &aString[start];
            const PRUnichar* end = &aString[i];

            
            nsFontXlib *oldFont = mCurrentFont;
            mCurrentFont = prevFont;
            UpdateGC();

            while (str < end) {
              x = aX;
              y = aY;
              mTranMatrix->TransformCoord(&x, &y);
              prevFont->DrawString(this, mSurface, x, y, str, 1);
              aX += *aSpacing++;
              str++;
            }
            mCurrentFont = oldFont;
          }
          else {
            nsFontXlib *oldFont = mCurrentFont;
            mCurrentFont = prevFont;
            UpdateGC();
            x += prevFont->DrawString(this, mSurface, x, y, &aString[start],
                                      i - start);
            mCurrentFont = oldFont;
          }
          prevFont = currFont;
          start = i;
        }
      }
      else {
        prevFont = currFont;
        start = i;
      }
    }

    if (prevFont) {
      nsFontXlib *oldFont = mCurrentFont;
      mCurrentFont = prevFont;
      UpdateGC();
    
      if (aSpacing) {
        const PRUnichar* str = &aString[start];
        const PRUnichar* end = &aString[i];
        while (str < end) {
          x = aX;
          y = aY;
          mTranMatrix->TransformCoord(&x, &y);
          prevFont->DrawString(this, mSurface, x, y, str, 1);
          aX += *aSpacing++;
          str++;
        }
      }
      else {
        prevFont->DrawString(this, mSurface, x, y, &aString[start], i - start);
      }

      mCurrentFont = oldFont;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextXlib::DrawString(const nsString& aString,
                                   nscoord aX, nscoord aY,
                                   PRInt32 aFontID,
                                   const nscoord* aSpacing)
{
  return DrawString(aString.get(), aString.Length(),
                    aX, aY, aFontID, aSpacing);
}

NS_IMETHODIMP
nsRenderingContextXlib::CopyOffScreenBits(nsIDrawingSurface* aSrcSurf, PRInt32 aSrcX, PRInt32 aSrcY,
                                          const nsRect &aDestBounds, PRUint32 aCopyFlags)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::CopyOffScreenBits()\n"));

  PRInt32                 srcX = aSrcX;
  PRInt32                 srcY = aSrcY;
  nsRect                  drect = aDestBounds;
  nsIDrawingSurfaceXlib  *destsurf;

  NS_ENSURE_TRUE(mTranMatrix != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mSurface    != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(aSrcSurf    != nsnull, NS_ERROR_FAILURE);
    
  if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER) {
    destsurf = mSurface;
  }
  else
  {
    NS_ENSURE_TRUE(mOffscreenSurface != nsnull, NS_ERROR_FAILURE);
    destsurf = mOffscreenSurface;
  }
  
  if (aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
    mTranMatrix->TransformCoord(&srcX, &srcY);
  
  if (aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
    mTranMatrix->TransformCoord(&drect.x, &drect.y, &drect.width, &drect.height);
  
  
  

  UpdateGC();
  Drawable destdrawable; destsurf->GetDrawable(destdrawable);
  Drawable srcdrawable; ((nsIDrawingSurfaceXlib *)aSrcSurf)->GetDrawable(srcdrawable);

  ::XCopyArea(mDisplay,
              srcdrawable,
              destdrawable,
              *mGC,
              srcX, srcY,
              drect.width, drect.height,
              drect.x, drect.y);

  return NS_OK;
}

#ifdef MOZ_MATHML
static
void Widen8To16AndGetTextExtents(nsXFont    *xFont,  
                                 const char *text,
                                 int         text_length,
                                 int        *lbearing,
                                 int        *rbearing,
                                 int        *width,
                                 int        *ascent,
                                 int        *descent)
{
  NS_ASSERTION(!xFont->IsSingleByte(),"wrong string/font size");
  XChar2b buf[WIDEN_8_TO_16_BUF_SIZE];
  XChar2b *p = buf;
  int uchar_size;

  if (text_length > WIDEN_8_TO_16_BUF_SIZE) {
    p = (XChar2b*)malloc(text_length*sizeof(XChar2b));
    if (!p) { 
      *lbearing = 0;
      *rbearing = 0;
      *width    = 0;
      *ascent   = 0;
      *descent  = 0;
      return;
    }
  }

  uchar_size = Widen8To16AndMove(text, text_length, p);
  xFont->TextExtents16(p, uchar_size/2,
                       lbearing, 
                       rbearing, 
                       width, 
                       ascent, 
                       descent);

  if (text_length > WIDEN_8_TO_16_BUF_SIZE) {
    free((char*)p);
  }
}

NS_IMETHODIMP
nsRenderingContextXlib::GetBoundingMetrics(const char*        aString, 
                                           PRUint32           aLength,
                                           nsBoundingMetrics& aBoundingMetrics)
{
  nsresult res = NS_OK;
  aBoundingMetrics.Clear();
  if (aString && 0 < aLength) {
    NS_ENSURE_TRUE(mCurrentFont != nsnull, NS_ERROR_FAILURE);
    nsXFont *xFont = mCurrentFont->GetXFont();
#ifdef USE_FREETYPE
    if (mCurrentFont->IsFreeTypeFont()) {
      
      res = mCurrentFont->GetBoundingMetrics(
                        NS_ConvertASCIItoUTF16(aString, aLength).get(),
                        aLength, aBoundingMetrics);
    }
    else
#endif 
    if (!mCurrentFont->GetXFontIs10646()) {
        
        NS_ASSERTION(xFont->IsSingleByte(), "GetBoundingMetrics: wrong string/font size");
        xFont->TextExtents8(aString, aLength,
                            &aBoundingMetrics.leftBearing, 
                            &aBoundingMetrics.rightBearing, 
                            &aBoundingMetrics.width, 
                            &aBoundingMetrics.ascent, 
                            &aBoundingMetrics.descent);
    }
    else {
        
        NS_ASSERTION(!xFont->IsSingleByte(), "GetBoundingMetrics: wrong string/font size");
        Widen8To16AndGetTextExtents(mCurrentFont->GetXFont(), aString, aLength,
                                    &aBoundingMetrics.leftBearing, 
                                    &aBoundingMetrics.rightBearing, 
                                    &aBoundingMetrics.width, 
                                    &aBoundingMetrics.ascent, 
                                    &aBoundingMetrics.descent);
    }

    aBoundingMetrics.leftBearing = NSToCoordRound(aBoundingMetrics.leftBearing * mP2T);
    aBoundingMetrics.rightBearing = NSToCoordRound(aBoundingMetrics.rightBearing * mP2T);
    aBoundingMetrics.width = NSToCoordRound(aBoundingMetrics.width * mP2T);
    aBoundingMetrics.ascent = NSToCoordRound(aBoundingMetrics.ascent * mP2T);
    aBoundingMetrics.descent = NSToCoordRound(aBoundingMetrics.descent * mP2T);
  }

  return res;
}

NS_IMETHODIMP
nsRenderingContextXlib::GetBoundingMetrics(const PRUnichar*   aString, 
                                           PRUint32           aLength,
                                           nsBoundingMetrics& aBoundingMetrics,
                                           PRInt32*           aFontID)
{
  aBoundingMetrics.Clear(); 
  if (0 < aLength) {
    NS_ENSURE_TRUE(aString != nsnull, NS_ERROR_FAILURE);

    nsFontMetricsXlib *metrics  = NS_REINTERPRET_CAST(nsFontMetricsXlib *, mFontMetrics.get());
    nsFontXlib        *prevFont = nsnull;

    nsBoundingMetrics rawbm;
    PRBool firstTime = PR_TRUE;
    PRUint32 start = 0;
    PRUint32 i;
    for (i = 0; i < aLength; i++) {
      PRUnichar c = aString[i];
      nsFontXlib  *currFont = nsnull;
      nsFontXlib **font = metrics->mLoadedFonts;
      nsFontXlib **end = &metrics->mLoadedFonts[metrics->mLoadedFontsCount];
      while (font < end) {
        if (CCMAP_HAS_CHAR((*font)->mCCMap, c)) {
          currFont = *font;
          goto FoundFont; 
        }
        font++;
      }
      currFont = metrics->FindFont(c);
  FoundFont:
      
      if (prevFont) {
        if (currFont != prevFont) {
          prevFont->GetBoundingMetrics((const PRUnichar*) &aString[start],
                                       i - start, rawbm);
          if (firstTime) {
            firstTime = PR_FALSE;
            aBoundingMetrics = rawbm;
          } 
          else {
            aBoundingMetrics += rawbm;
          }
          prevFont = currFont;
          start = i;
        }
      }
      else {
        prevFont = currFont;
        start = i;
      }
    }
    
    if (prevFont) {
      prevFont->GetBoundingMetrics((const PRUnichar*) &aString[start],
                                   i - start, rawbm);
      if (firstTime) {
        aBoundingMetrics = rawbm;
      }
      else {
        aBoundingMetrics += rawbm;
      }
    }
    
    aBoundingMetrics.leftBearing = NSToCoordRound(aBoundingMetrics.leftBearing * mP2T);
    aBoundingMetrics.rightBearing = NSToCoordRound(aBoundingMetrics.rightBearing * mP2T);
    aBoundingMetrics.width = NSToCoordRound(aBoundingMetrics.width * mP2T);
    aBoundingMetrics.ascent = NSToCoordRound(aBoundingMetrics.ascent * mP2T);
    aBoundingMetrics.descent = NSToCoordRound(aBoundingMetrics.descent * mP2T);
  }
  if (nsnull != aFontID)
    *aFontID = 0;

  return NS_OK;
}
#endif 

NS_IMETHODIMP
nsRenderingContextXlib::DrawImage(imgIContainer *aImage, const nsRect & aSrcRect, const nsRect & aDestRect)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::DrawImage()\n"));
  UpdateGC();
  return nsRenderingContextImpl::DrawImage(aImage, aSrcRect, aDestRect);
}

NS_IMETHODIMP
nsRenderingContextXlib::GetBackbuffer(const nsRect &aRequestedSize,
                                      const nsRect &aMaxSize,
                                      PRBool aForBlending,
                                      nsIDrawingSurface* &aBackbuffer)
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::GetBackbuffer()\n"));
  

  return AllocateBackbuffer(aRequestedSize, aMaxSize, aBackbuffer, PR_FALSE, 0);
}
 
NS_IMETHODIMP
nsRenderingContextXlib::ReleaseBackbuffer(void) 
{
  PR_LOG(RenderingContextXlibLM, PR_LOG_DEBUG, ("nsRenderingContextXlib::ReleaseBackbuffer()\n"));
  

  return DestroyCachedBackbuffer();
}

