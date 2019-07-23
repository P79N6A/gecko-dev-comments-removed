




















































#include "nsGfxDefs.h"

#include "nsRenderingContextOS2.h"
#include "nsFontMetricsOS2.h"
#include "nsRegionOS2.h"
#include "nsDeviceContextOS2.h"
#include "prprf.h"
#include "nsGfxCIID.h"
#include "nsUnicharUtils.h"



LONG OS2_CombineClipRegion( HPS hps, HRGN hrgnCombine, LONG lMode);
HRGN OS2_CopyClipRegion( HPS hps);
#define OS2_SetClipRegion(hps,hrgn) OS2_CombineClipRegion(hps, hrgn, CRGN_COPY)

#define FLAG_CLIP_VALID       0x0001
#define FLAG_CLIP_CHANGED     0x0002
#define FLAG_LOCAL_CLIP_VALID 0x0004

#define FLAGS_ALL             (FLAG_CLIP_VALID | FLAG_CLIP_CHANGED | FLAG_LOCAL_CLIP_VALID)

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
  nsIFontMetrics  *mFontMetrics;
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
  mFontMetrics = nsnull;
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
  mFontMetrics = nsnull;
  mFlags = ~FLAGS_ALL;
  mLineStyle = aState.mLineStyle;
}

GraphicsState :: ~GraphicsState()
{
  if (NULL != mClipRegion)
  {
    printf( "oops, leaked a region from rc-gs\n");
    mClipRegion = NULL;
  }
}





nsRenderingContextOS2::nsRenderingContextOS2()
{
  mContext = nsnull;
  mSurface = nsnull;
  mPS = 0;
  mMainSurface = nsnull;
  mColor = NS_RGB( 0, 0, 0);
  mFontMetrics = nsnull;
  mLineStyle = nsLineStyle_kSolid;
  mPreservedInitialClipRegion = PR_FALSE;
  mPaletteMode = PR_FALSE;
  mCurrFontOS2 = nsnull;

  mStateCache = new nsVoidArray();
  mRightToLeftText = PR_FALSE;

  

  PushState();

  mP2T = 1.0f;
}

nsRenderingContextOS2::~nsRenderingContextOS2()
{
  NS_IF_RELEASE(mContext);
  NS_IF_RELEASE(mFontMetrics);

  
  PopState ();

  if (nsnull != mStateCache)
  {
    PRInt32 cnt = mStateCache->Count();

    while (--cnt >= 0)
    {
      GraphicsState *state = (GraphicsState *)mStateCache->ElementAt(cnt);
      if (state->mClipRegion) {
         GFX (::GpiDestroyRegion (mPS, state->mClipRegion), FALSE);
         state->mClipRegion = 0;
      }
      mStateCache->RemoveElementAt(cnt);

      if (nsnull != state)
        delete state;
    }

    delete mStateCache;
    mStateCache = nsnull;
  }

   
   NS_IF_RELEASE(mMainSurface);
   NS_IF_RELEASE(mSurface);
}

NS_IMPL_ISUPPORTS2(nsRenderingContextOS2,
                   nsIRenderingContext,
                   nsIRenderingContextOS2)

NS_IMETHODIMP
nsRenderingContextOS2::Init( nsIDeviceContext *aContext,
                             nsIWidget *aWindow)
{
   mContext = aContext;
   NS_IF_ADDREF(mContext);

   
   nsWindowSurface *surf = new nsWindowSurface;
   if (!surf)
     return NS_ERROR_OUT_OF_MEMORY;

   surf->Init(aWindow);

   mSurface = surf;
   mPS = mSurface->GetPS ();
   NS_ADDREF(mSurface);

   mDCOwner = aWindow;
   NS_IF_ADDREF(mDCOwner);

   
   mMainSurface = mSurface;
   NS_ADDREF(mMainSurface);

   return CommonInit();
}

NS_IMETHODIMP
nsRenderingContextOS2::Init( nsIDeviceContext *aContext,
                             nsIDrawingSurface* aSurface)
{
   mContext = aContext;
   NS_IF_ADDREF(mContext);

   
   mSurface = (nsDrawingSurfaceOS2 *) aSurface;

  if (nsnull != mSurface)
  {
    mPS = mSurface->GetPS ();
    NS_ADDREF(mSurface);

    mMainSurface = mSurface;
    NS_ADDREF(mMainSurface);
  }

   return CommonInit();
}

nsresult nsRenderingContextOS2::SetupPS(void)
{
   LONG BlackColor, WhiteColor;

   
   if (((nsDeviceContextOS2*)mContext)->IsPaletteDevice())
   {
      BlackColor = GFX (::GpiQueryColorIndex(mPS, 0, MK_RGB (0x00, 0x00, 0x00)), GPI_ALTERROR);    
      WhiteColor = GFX (::GpiQueryColorIndex(mPS, 0, MK_RGB (0xFF, 0xFF, 0xFF)), GPI_ALTERROR);    

      mPaletteMode = PR_TRUE;
   }
   else
   {
      GFX (::GpiCreateLogColorTable(mPS, 0, LCOLF_RGB, 0, 0, 0), FALSE);

      BlackColor = MK_RGB(0x00, 0x00, 0x00);
      WhiteColor = MK_RGB(0xFF, 0xFF, 0xFF);

      mPaletteMode = PR_FALSE;
   }

   
   
   IMAGEBUNDLE ib;
   ib.lColor     = BlackColor;           
   ib.lBackColor = WhiteColor;           
   ib.usMixMode  = FM_OVERPAINT;
   ib.usBackMixMode = BM_OVERPAINT;
   GFX (::GpiSetAttrs(mPS, PRIM_IMAGE, IBB_COLOR | IBB_BACK_COLOR | IBB_MIX_MODE | IBB_BACK_MIX_MODE, 0, (PBUNDLE)&ib), FALSE);

   return NS_OK;
}




nsresult nsRenderingContextOS2::CommonInit()
{
   float app2dev;

   app2dev = mContext->AppUnitsToDevUnits();
   mTranMatrix->AddScale( app2dev, app2dev);
   mP2T = mContext->DevUnitsToAppUnits();

   return SetupPS();
}



NS_IMETHODIMP nsRenderingContextOS2::LockDrawingSurface( PRInt32 aX, PRInt32 aY,
                                       PRUint32 aWidth, PRUint32 aHeight,
                                       void **aBits,
                                       PRInt32 *aStride, PRInt32 *aWidthBytes,
                                       PRUint32 aFlags)
{
  PushState();

  return mSurface->Lock( aX, aY, aWidth, aHeight, aBits,
                         aStride, aWidthBytes, aFlags);
}

NS_IMETHODIMP nsRenderingContextOS2::UnlockDrawingSurface()
{
  mSurface->Unlock();

  PopState();

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextOS2::SelectOffScreenDrawingSurface( nsIDrawingSurface* aSurface)
{
   if (aSurface != mSurface)
   {
      if(nsnull != aSurface)
      {
         NS_IF_RELEASE(mSurface);
         mSurface = (nsDrawingSurfaceOS2 *) aSurface;
         mPS = mSurface->GetPS ();

         SetupPS();
      }
      else 
      {
         NS_IF_RELEASE(mSurface);
         mSurface = mMainSurface;
         mPS = mSurface->GetPS ();

         SetupPS();
      }

      NS_ADDREF(mSurface);
   }

   return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextOS2::GetDrawingSurface( nsIDrawingSurface* *aSurface)
{
   *aSurface = mSurface;
   return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextOS2::GetHints(PRUint32& aResult)
{
  PRUint32 result = 0;
  
  result |= NS_RENDERING_HINT_FAST_MEASURE;

  aResult = result;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::Reset()
{
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::GetDeviceContext( nsIDeviceContext *&aContext)
{
  NS_IF_ADDREF(mContext);
  aContext = mContext;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2 :: PushState(void)
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




    state->mFlags = ~FLAGS_ALL;
    state->mLineStyle = mStates->mLineStyle;

    mStates = state;
  }

  if (nsnull != mStates->mNext)
  {
    mStates->mNext->mColor = mColor;
    mStates->mNext->mFontMetrics = mFontMetrics;
    NS_IF_ADDREF(mStates->mNext->mFontMetrics);
  }

  mTranMatrix = &mStates->mMatrix;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2 :: PopState(void)
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
        {
          
          HRGN hrgn = GFX (::GpiCreateRegion (mPS, 0, NULL), RGN_ERROR);
          GFX (::GpiCombineRegion (mPS, hrgn, pstate->mClipRegion, 0, CRGN_COPY), RGN_ERROR);
          OS2_SetClipRegion (mPS, hrgn);
        }
      }

      oldstate->mFlags &= ~FLAGS_ALL;

      NS_IF_RELEASE(mFontMetrics);
      mFontMetrics = mStates->mFontMetrics;

      mColor = mStates->mColor;

      SetLineStyle(mStates->mLineStyle);
    }
    else
      mTranMatrix = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::IsVisibleRect( const nsRect &aRect,
                                                    PRBool &aIsVisible)
{
   nsRect trect( aRect);
   mTranMatrix->TransformCoord( &trect.x, &trect.y,
                            &trect.width, &trect.height);
   RECTL rcl;
   mSurface->NS2PM_ININ (trect, rcl);

   LONG rc = GFX (::GpiRectVisible( mPS, &rcl), RVIS_ERROR);

   aIsVisible = (rc == RVIS_PARTIAL || rc == RVIS_VISIBLE) ? PR_TRUE : PR_FALSE;


   return NS_OK;
}


NS_IMETHODIMP nsRenderingContextOS2::SetClipRect( const nsRect& aRect, nsClipCombine aCombine)
{
  nsRect  trect = aRect;
  RECTL   rcl;

  mTranMatrix->TransformCoord(&trect.x, &trect.y,
			      &trect.width, &trect.height);

  mStates->mLocalClip = aRect;
  mStates->mFlags |= FLAG_LOCAL_CLIP_VALID;

  

  if( trect.width == 0 || trect.height == 0)
  {
    
    
    

    if( aCombine == nsClipCombine_kIntersect || aCombine == nsClipCombine_kReplace)
    {
      PushClipState();

      rcl.xLeft   = -10000;
      rcl.xRight  = -9999;
      rcl.yBottom = -10000;
      rcl.yTop    = -9999;

      HRGN hrgn = GFX (::GpiCreateRegion( mPS, 1, &rcl), RGN_ERROR);
      OS2_SetClipRegion (mPS, hrgn);
    }
    else if( aCombine == nsClipCombine_kUnion || aCombine == nsClipCombine_kSubtract)
    {
      PushClipState();

      
      POINTL Offset = { 0, 0 };

      GFX (::GpiOffsetClipRegion (mPS, &Offset), RGN_ERROR);
    }
    else
      NS_ASSERTION(PR_FALSE, "illegal clip combination");
  }
  else
  {
    if (aCombine == nsClipCombine_kIntersect)
    {
      PushClipState();

      mSurface->NS2PM_ININ (trect, rcl);
      GFX (::GpiIntersectClipRectangle(mPS, &rcl), RGN_ERROR);
    }
    else if (aCombine == nsClipCombine_kUnion)
    {
      PushClipState();

      mSurface->NS2PM_INEX (trect, rcl);
      HRGN hrgn = GFX (::GpiCreateRegion(mPS, 1, &rcl), RGN_ERROR);

      if( hrgn )
        OS2_CombineClipRegion(mPS, hrgn, CRGN_OR);
    }
    else if (aCombine == nsClipCombine_kSubtract)
    {
      PushClipState();

      mSurface->NS2PM_ININ (trect, rcl);
      GFX (::GpiExcludeClipRectangle(mPS, &rcl), RGN_ERROR);
    }
    else if (aCombine == nsClipCombine_kReplace)
    {
      PushClipState();

      mSurface->NS2PM_INEX (trect, rcl);
      HRGN hrgn = GFX (::GpiCreateRegion(mPS, 1, &rcl), RGN_ERROR);

      if( hrgn )
	OS2_SetClipRegion(mPS, hrgn);
    }
    else
      NS_ASSERTION(PR_FALSE, "illegal clip combination");
  }

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextOS2::GetClipRect( nsRect &aRect, PRBool &aClipValid)
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


NS_IMETHODIMP nsRenderingContextOS2::SetClipRegion( const nsIRegion &aRegion, nsClipCombine aCombine)
{
   nsRegionOS2 *pRegion = (nsRegionOS2 *) &aRegion;
   PRUint32     ulHeight = mSurface->GetHeight ();

   HRGN hrgn = pRegion->GetHRGN( ulHeight, mPS);
   LONG cmode = 0L;

   switch( aCombine)
   {
      case nsClipCombine_kIntersect:
         cmode = CRGN_AND;
         break;
      case nsClipCombine_kUnion:
         cmode = CRGN_OR;
         break;
      case nsClipCombine_kSubtract:
         cmode = CRGN_DIFF;
         break;
      case nsClipCombine_kReplace:
         cmode = CRGN_COPY;
         break;
      default:
         
         NS_ASSERTION( 0, "illegal clip combination");
         break;
  }

  if (NULL != hrgn)
  {
    mStates->mFlags &= ~FLAG_LOCAL_CLIP_VALID;
    PushClipState();
    OS2_CombineClipRegion( mPS, hrgn, cmode);
  }
  else
    return PR_FALSE;

  return NS_OK;
}




NS_IMETHODIMP nsRenderingContextOS2::CopyClipRegion(nsIRegion &aRegion)
{
#if 0
  HRGN hr = OS2_CopyClipRegion(mPS);

  if (hr == HRGN_ERROR)
    return NS_ERROR_FAILURE;

  ((nsRegionOS2 *)&aRegion)->mRegion = hr;

  return NS_OK;
#else
  NS_ASSERTION( 0, "nsRenderingContextOS2::CopyClipRegion() not implemented" );
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}


NS_IMETHODIMP nsRenderingContextOS2::GetClipRegion( nsIRegion **aRegion)
{
   if( !aRegion)
      return NS_ERROR_NULL_POINTER;

   *aRegion = 0;

   nsRegionOS2 *pRegion = new nsRegionOS2;
   if (!pRegion)
     return NS_ERROR_OUT_OF_MEMORY;
   NS_ADDREF(pRegion);

   
   HRGN hrgnClip = 0;

   GFX (::GpiSetClipRegion (mPS, 0, &hrgnClip), RGN_ERROR);
   
   if( hrgnClip && hrgnClip != HRGN_ERROR)
   {
      
      HRGN hrgnDummy = 0;
      PRUint32 ulHeight = mSurface->GetHeight ();

      pRegion->InitWithHRGN (hrgnClip, ulHeight, mPS);
      GFX (::GpiSetClipRegion (mPS, hrgnClip, &hrgnDummy), RGN_ERROR);
   }
   else
      pRegion->Init();

   *aRegion = pRegion;

   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::SetColor( nscolor aColor)
{
   mColor = aColor;
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::GetColor( nscolor &aColor) const
{
   aColor = mColor;
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::SetLineStyle( nsLineStyle aLineStyle)
{
   mLineStyle = aLineStyle;
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::GetLineStyle( nsLineStyle &aLineStyle)
{
   aLineStyle = mLineStyle;
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::SetFont( const nsFont &aFont, nsIAtom* aLangGroup)
{
  mCurrFontOS2 = nsnull; 
  NS_IF_RELEASE(mFontMetrics);
  mContext->GetMetricsFor(aFont, aLangGroup, mFontMetrics);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::SetFont( nsIFontMetrics *aFontMetrics)
{
  mCurrFontOS2 = nsnull; 
  NS_IF_RELEASE(mFontMetrics);
  mFontMetrics = aFontMetrics;
  NS_IF_ADDREF(mFontMetrics);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::GetFontMetrics( nsIFontMetrics*& aFontMetrics)
{
  NS_IF_ADDREF(mFontMetrics);
  aFontMetrics = mFontMetrics;

  return NS_OK;
}


NS_IMETHODIMP nsRenderingContextOS2::Translate( nscoord aX, nscoord aY)
{
   mTranMatrix->AddTranslation( (float) aX, (float) aY);
   return NS_OK;
}


NS_IMETHODIMP nsRenderingContextOS2::Scale( float aSx, float aSy)
{
   mTranMatrix->AddScale(aSx, aSy);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::GetCurrentTransform( nsTransform2D *&aTransform)
{
  aTransform = mTranMatrix;
  return NS_OK;
}









NS_IMETHODIMP nsRenderingContextOS2::CreateDrawingSurface(const nsRect& aBounds,
                             PRUint32 aSurfFlags, nsIDrawingSurface* &aSurface)
{
   nsresult rc = NS_ERROR_FAILURE;

   nsOffscreenSurface *surf = new nsOffscreenSurface;

   if (!surf)
     return NS_ERROR_OUT_OF_MEMORY;

   rc = surf->Init( mMainSurface->GetPS (), aBounds.width, aBounds.height, aSurfFlags);

   if(NS_SUCCEEDED(rc))
   {
      NS_ADDREF(surf);
      aSurface = surf;
   }
   else
      delete surf;

   return rc;
}

NS_IMETHODIMP nsRenderingContextOS2::CreateDrawingSurface(HPS aPS, nsIDrawingSurface* &aSurface, nsIWidget *aWidget)
{
  nsWindowSurface *surf = new nsWindowSurface();

  if (nsnull != surf)
  {
    NS_ADDREF(surf);
    surf->Init(aPS, aWidget);
  }

  aSurface = surf;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::DestroyDrawingSurface( nsIDrawingSurface* aDS)
{
   nsDrawingSurfaceOS2 *surf = (nsDrawingSurfaceOS2 *) aDS;
   nsresult rc = NS_ERROR_NULL_POINTER;

   
   
   if( surf && surf == mSurface)
   {
      NS_RELEASE(mSurface);    
      mSurface = mMainSurface;
      mPS = mSurface->GetPS ();
      NS_ADDREF(mSurface);
   }

   if( surf)
   {
      NS_RELEASE(surf);        
      rc = NS_OK;
   }

   return rc;
}



LONG nsRenderingContextOS2::GetGPIColor (void)
{
   LONG gcolor = MK_RGB (NS_GET_R (mColor),
                         NS_GET_G (mColor),
                         NS_GET_B (mColor));

   return (mPaletteMode) ? GFX (::GpiQueryColorIndex (mPS, 0, gcolor), GPI_ALTERROR) :
                           gcolor ;
}

void nsRenderingContextOS2::SetupLineColorAndStyle (void)
{
   
   LINEBUNDLE lineBundle;
   lineBundle.lColor = GetGPIColor ();

   GFX (::GpiSetAttrs (mPS, PRIM_LINE, LBB_COLOR, 0, (PBUNDLE)&lineBundle), FALSE);
   
   long ltype = 0;
   switch( mLineStyle)
   {
      case nsLineStyle_kNone:   ltype = LINETYPE_INVISIBLE; break;
      case nsLineStyle_kSolid:  ltype = LINETYPE_SOLID; break;
      case nsLineStyle_kDashed: ltype = LINETYPE_SHORTDASH; break;
      case nsLineStyle_kDotted: ltype = LINETYPE_DOT; break;
      default:
         NS_ASSERTION(0, "Unexpected line style");
         break;
   }
   GFX (::GpiSetLineType (mPS, ltype), FALSE);
   
}

void nsRenderingContextOS2::SetupFillColor (void)
{
   
   AREABUNDLE areaBundle;
   areaBundle.lColor = GetGPIColor ();

   GFX (::GpiSetAttrs (mPS, PRIM_AREA, ABB_COLOR, 0, (PBUNDLE)&areaBundle), FALSE);

}

NS_IMETHODIMP nsRenderingContextOS2::DrawLine( nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
   mTranMatrix->TransformCoord( &aX0, &aY0);
   mTranMatrix->TransformCoord( &aX1, &aY1);

   POINTL ptls[] = { { (long) aX0, (long) aY0 },
                     { (long) aX1, (long) aY1 } };
   mSurface->NS2PM (ptls, 2);

   if (ptls[0].x > ptls[1].x)
      ptls[0].x--;
   else if (ptls[1].x > ptls[0].x)
      ptls[1].x--;

   if (ptls[0].y < ptls[1].y)
      ptls[0].y++;
   else if (ptls[1].y < ptls[0].y)
      ptls[1].y++;

   SetupLineColorAndStyle ();

   GFX (::GpiMove (mPS, ptls), FALSE);
   GFX (::GpiLine (mPS, ptls + 1), GPI_ERROR);

   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
   PMDrawPoly( aPoints, aNumPoints, PR_FALSE);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::DrawPolygon( const nsPoint aPoints[], PRInt32 aNumPoints)
{
   PMDrawPoly( aPoints, aNumPoints, PR_FALSE);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::FillPolygon( const nsPoint aPoints[], PRInt32 aNumPoints)
{
   PMDrawPoly( aPoints, aNumPoints, PR_TRUE );
   return NS_OK;
}

 
void nsRenderingContextOS2::PMDrawPoly( const nsPoint aPoints[], PRInt32 aNumPoints, PRBool bFilled, PRBool bDoTransform)
{
   if( aNumPoints > 1)
   {
      
      POINTL  aptls[ 20];
      PPOINTL pts = aptls;

      if( aNumPoints > 20)
         pts = new POINTL[aNumPoints];

      PPOINTL pp = pts;
      const nsPoint *np = &aPoints[0];

      for( PRInt32 i = 0; i < aNumPoints; i++, pp++, np++)
      {
         pp->x = np->x;
         pp->y = np->y;
         if( bDoTransform )
           mTranMatrix->TransformCoord( (int*)&pp->x, (int*)&pp->y);
      }

      
      mSurface->NS2PM (pts, aNumPoints);

      
      
      

      GFX (::GpiMove (mPS, pts), FALSE);

      if( bFilled == PR_TRUE)
      {
         POLYGON pgon = { aNumPoints - 1, pts + 1 };
         
         
         
         SetupFillColor ();
         GFX (::GpiPolygons (mPS, 1, &pgon, POLYGON_NOBOUNDARY, POLYGON_EXCL), GPI_ERROR);
      }
      else
      {
         SetupLineColorAndStyle ();
         GFX (::GpiPolyLine (mPS, aNumPoints - 1, pts + 1), GPI_ERROR);
      }

      if( aNumPoints > 20)
         delete [] pts;
   }
}

NS_IMETHODIMP nsRenderingContextOS2::DrawRect( const nsRect& aRect)
{
   nsRect tr = aRect;
   PMDrawRect( tr, FALSE);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::DrawRect( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
   nsRect tr( aX, aY, aWidth, aHeight);
   PMDrawRect( tr, FALSE);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::FillRect( const nsRect& aRect)
{
   nsRect tr = aRect;
   PMDrawRect( tr, TRUE);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::FillRect( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
   nsRect tr( aX, aY, aWidth, aHeight);
   PMDrawRect( tr, TRUE);
   return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextOS2 :: InvertRect(const nsRect& aRect)
{
   return InvertRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

NS_IMETHODIMP
nsRenderingContextOS2 :: InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
   nsRect tr(aX, aY, aWidth, aHeight);
   LONG CurMix = GFX (::GpiQueryMix (mPS), FM_ERROR);
   GFX (::GpiSetMix (mPS, FM_INVERT), FALSE);
   PMDrawRect(tr, FALSE);
   GFX (::GpiSetMix (mPS, CurMix), FALSE);
   return NS_OK;
}

void nsRenderingContextOS2::PMDrawRect( nsRect &rect, BOOL fill)
{
   mTranMatrix->TransformCoord( &rect.x, &rect.y, &rect.width, &rect.height);

   
   if ( !rect.width || !rect.height )
      return;

   RECTL rcl;
   mSurface->NS2PM_ININ (rect, rcl);

   GFX (::GpiMove (mPS, (PPOINTL) &rcl), FALSE);

   if (rcl.xLeft == rcl.xRight || rcl.yTop == rcl.yBottom)
   {
      SetupLineColorAndStyle ();
      GFX (::GpiLine (mPS, ((PPOINTL)&rcl) + 1), GPI_ERROR);
   }
   else 
   {
      long lOps;

      if (fill)
      {
         lOps = DRO_FILL;
         SetupFillColor ();
      } else
      {
         lOps = DRO_OUTLINE;
         SetupLineColorAndStyle ();
      }

      GFX (::GpiBox (mPS, lOps, ((PPOINTL)&rcl) + 1, 0, 0), GPI_ERROR);
   }
}


NS_IMETHODIMP nsRenderingContextOS2::DrawEllipse( const nsRect& aRect)
{
   nsRect tRect( aRect);
   PMDrawArc( tRect, PR_FALSE, PR_TRUE);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::DrawEllipse( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
   nsRect tRect( aX, aY, aWidth, aHeight);
   PMDrawArc( tRect, PR_FALSE, PR_TRUE);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::FillEllipse( const nsRect& aRect)
{
   nsRect tRect( aRect);
   PMDrawArc( tRect, PR_TRUE, PR_TRUE);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::FillEllipse( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
   nsRect tRect( aX, aY, aWidth, aHeight);
   PMDrawArc( tRect, PR_TRUE, PR_TRUE);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::DrawArc( const nsRect& aRect,
                                         float aStartAngle, float aEndAngle)
{
   nsRect tRect( aRect);
   PMDrawArc( tRect, PR_FALSE, PR_FALSE, aStartAngle, aEndAngle);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::DrawArc( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                         float aStartAngle, float aEndAngle)
{
   nsRect tRect( aX, aY, aWidth, aHeight);
   PMDrawArc( tRect, PR_FALSE, PR_FALSE, aStartAngle, aEndAngle);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::FillArc( const nsRect& aRect,
                                         float aStartAngle, float aEndAngle)
{
   nsRect tRect( aRect);
   PMDrawArc( tRect, PR_TRUE, PR_FALSE, aStartAngle, aEndAngle);
   return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2::FillArc( nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                         float aStartAngle, float aEndAngle)
{
   nsRect tRect( aX, aY, aWidth, aHeight);
   PMDrawArc( tRect, PR_TRUE, PR_FALSE, aStartAngle, aEndAngle);
   return NS_OK;
}

void nsRenderingContextOS2::PMDrawArc( nsRect &rect, PRBool bFilled, PRBool bFull,
                                       float start, float end)
{
   
   mTranMatrix->TransformCoord( &rect.x, &rect.y, &rect.width, &rect.height);

   RECTL rcl;
   mSurface->NS2PM_ININ (rect, rcl);

   
   long lWidth = rect.width / 2;
   long lHeight = rect.height / 2;
   ARCPARAMS arcparams = { lWidth, lHeight, 0, 0 };
   GFX (::GpiSetArcParams (mPS, &arcparams), FALSE);

   
   rcl.xLeft += lWidth;
   rcl.yBottom += lHeight;
   GFX (::GpiMove (mPS, (PPOINTL)&rcl), FALSE);

   if (bFilled)
      SetupFillColor ();
   else
      SetupLineColorAndStyle ();


   if (bFull)
   {
      long lOps = (bFilled) ? DRO_FILL : DRO_OUTLINE;

      
      GFX (::GpiFullArc (mPS, lOps, MAKEFIXED(1,0)), GPI_ERROR);
   }
   else
   {
      FIXED StartAngle = (FIXED)(start * 65536.0) % MAKEFIXED (360, 0);
      FIXED EndAngle   = (FIXED)(end * 65536.0) % MAKEFIXED (360, 0);
      FIXED SweepAngle = EndAngle - StartAngle;

      if (SweepAngle < 0) SweepAngle += MAKEFIXED (360, 0);

      
      if (bFilled)
      {
         GFX (::GpiBeginArea (mPS, BA_NOBOUNDARY), FALSE);
         GFX (::GpiPartialArc (mPS, (PPOINTL)&rcl, MAKEFIXED(1,0), StartAngle, SweepAngle), GPI_ERROR);
         GFX (::GpiEndArea (mPS), GPI_ERROR);
      }
      else
      {
         
         long lLineType = GFX (::GpiQueryLineType (mPS), LINETYPE_ERROR);
         GFX (::GpiSetLineType (mPS, LINETYPE_INVISIBLE), FALSE);
         GFX (::GpiPartialArc (mPS, (PPOINTL)&rcl, MAKEFIXED(1,0), StartAngle, MAKEFIXED (0,0)), GPI_ERROR);
         
         GFX (::GpiSetLineType (mPS, lLineType), FALSE);
         GFX (::GpiPartialArc (mPS, (PPOINTL)&rcl, MAKEFIXED(1,0), StartAngle, SweepAngle), GPI_ERROR);
      }
   }
}

NS_IMETHODIMP nsRenderingContextOS2 :: GetWidth(char ch, nscoord& aWidth)
{
  char buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth);
}

NS_IMETHODIMP nsRenderingContextOS2 :: GetWidth(PRUnichar ch, nscoord &aWidth, PRInt32 *aFontID)
{
  PRUnichar buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth, aFontID);
}

NS_IMETHODIMP nsRenderingContextOS2 :: GetWidth(const char* aString, nscoord& aWidth)
{
  return GetWidth(aString, strlen(aString), aWidth);
}

NS_IMETHODIMP nsRenderingContextOS2 :: GetWidth(const char* aString,
                                                PRUint32 aLength,
                                                nscoord& aWidth)
{

  if (nsnull != mFontMetrics)
  {
    
    
    if ((1 == aLength) && (aString[0] == ' '))
    {
      return mFontMetrics->GetSpaceWidth(aWidth);
    }

    SetupFontAndColor();
    nscoord pxWidth = mCurrFontOS2->GetWidth(mPS, aString, aLength);
    aWidth = NSToCoordRound(float(pxWidth) * mP2T);

    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsRenderingContextOS2::GetWidth( const nsString &aString,
                                               nscoord &aWidth,
                                               PRInt32 *aFontID)
{
   return GetWidth( aString.get(), aString.Length(), aWidth, aFontID);
}

struct GetWidthData {
  HPS                   mPS;      
  nsDrawingSurfaceOS2*  mSurface; 
  nsFontOS2*            mFont;    
  LONG                  mWidth;   
};

static PRBool PR_CALLBACK
do_GetWidth(const nsFontSwitch* aFontSwitch,
            const PRUnichar*    aSubstring,
            PRUint32            aSubstringLength,
            void*               aData)
{
  nsFontOS2* font = aFontSwitch->mFont;

  GetWidthData* data = (GetWidthData*)aData;
  if (data->mFont != font) {
    
    data->mFont = font;
    data->mSurface->SelectFont(data->mFont);
  }
  data->mWidth += font->GetWidth(data->mPS, aSubstring, aSubstringLength);
  return PR_TRUE; 
}

NS_IMETHODIMP nsRenderingContextOS2::GetWidth( const PRUnichar *aString,
                                               PRUint32 aLength,
                                               nscoord &aWidth,
                                               PRInt32 *aFontID)
{
  if (!mFontMetrics)
    return NS_ERROR_FAILURE;

  SetupFontAndColor();

  nsFontMetricsOS2* metrics = (nsFontMetricsOS2*)mFontMetrics;
  GetWidthData data = {mPS, mSurface, mCurrFontOS2, 0};

  metrics->ResolveForwards(mPS, aString, aLength, do_GetWidth, &data);
  aWidth = NSToCoordRound(float(data.mWidth) * mP2T);

  if (data.mFont != mCurrFontOS2) {
    
    mSurface->SelectFont(mCurrFontOS2);
  }

  if (aFontID)
    *aFontID = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextOS2::GetTextDimensions(const char*       aString,
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
        pxWidth = mCurrFontOS2->GetWidth(mPS, &aString[start], numChars);
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
            pxWidth = mCurrFontOS2->GetWidth(mPS, &aString[start], numChars);
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
  HPS         mPS;                
  nsDrawingSurfaceOS2*  mSurface; 
  nsFontOS2*  mFont;              
  float       mP2T;               
  PRInt32     mAvailWidth;        
  PRInt32*    mBreaks;            
  PRInt32     mNumBreaks;         
  nscoord     mSpaceWidth;        
  nscoord     mAveCharWidth;      
  PRInt32     mEstimatedNumChars; 

  PRInt32     mNumCharsFit;  
  nscoord     mWidth;        

  
  
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
  nsFontOS2* font = aFontSwitch->mFont;

  
  BreakGetTextDimensionsData* data = (BreakGetTextDimensionsData*)aData;
  if (data->mFont != font) {
    
    data->mFont = font;
    data->mSurface->SelectFont(data->mFont);
  }

   
  if (font->mMaxAscent == 0)
  {
    FONTMETRICS fm;
    GFX (::GpiQueryFontMetrics(data->mPS, sizeof (fm), &fm), FALSE);
    
    font->mMaxAscent  = NSToCoordRound( (fm.lMaxAscender-1) * data->mP2T );
    font->mMaxDescent = NSToCoordRound( (fm.lMaxDescender+1) * data->mP2T );
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
      pxWidth = font->GetWidth(data->mPS, &pstr[start], numChars);
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
          
          pxWidth = font->GetWidth(data->mPS, &pstr[start], numChars);
          width -= NSToCoordRound(float(pxWidth) * data->mP2T);
        }
        else {
          
          
          
          
          end = data->mNumCharsFit; 
          data->mNumCharsFit = numCharsFit; 
          PRInt32 k = data->mFonts->Count() - 1;
          for ( ; k >= 0 && start < end; --k, end -= numChars) {
            font = (nsFontOS2*)data->mFonts->ElementAt(k);
            const PRUnichar* ps = (const PRUnichar*)data->mOffsets->ElementAt(k);
            if (ps < pstr + start)
              ps = pstr + start;

            numChars = pstr + end - ps;
            NS_ASSERTION(numChars > 0, "empty string");

            data->mFont = font;
            data->mSurface->SelectFont(data->mFont);
            pxWidth = font->GetWidth(data->mPS, ps, numChars);
            data->mWidth -= NSToCoordRound(float(pxWidth) * data->mP2T);

            
            
            data->mFonts->RemoveElementAt(k);
            data->mOffsets->RemoveElementAt(k+1);
          }

          
          
          data->mFonts->AppendElement(font);
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
    data->mFonts->AppendElement(font);
    data->mOffsets->AppendElement((void*)&pstr[numCharsFit]);
  }

  if (allDone) {
    
    return PR_FALSE;
  }

  return PR_TRUE; 
}

NS_IMETHODIMP
nsRenderingContextOS2::GetTextDimensions(const PRUnichar*  aString,
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

  SetupFontAndColor();

  nsFontMetricsOS2* metrics = (nsFontMetricsOS2*)mFontMetrics;

  nscoord spaceWidth, aveCharWidth;
  metrics->GetSpaceWidth(spaceWidth);
  metrics->GetAveCharWidth(aveCharWidth);

  
  
  
  

  
  

  nsAutoVoidArray fonts, offsets;
  offsets.AppendElement((void*)aString);

  BreakGetTextDimensionsData data = {mPS, mSurface, mCurrFontOS2, mP2T,
                                     aAvailWidth, aBreaks, aNumBreaks,
                                     spaceWidth, aveCharWidth, 0, 0, 0, -1, 0,
                                     &fonts, &offsets};

  metrics->ResolveForwards(mPS, aString, aLength, do_BreakGetTextDimensions, &data);

  if (data.mFont != mCurrFontOS2) {
    
    mSurface->SelectFont(mCurrFontOS2);
  }

  if (aFontID)
    *aFontID = 0;

  aNumCharsFit = data.mNumCharsFit;
  aDimensions.width = data.mWidth;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  aLastWordDimensions.Clear();
  aLastWordDimensions.width = -1;

  PRInt32 count = fonts.Count();
  if (!count)
    return NS_OK;
  nsFontOS2* font = (nsFontOS2*)fonts[0];
  NS_ASSERTION(font, "internal error in do_BreakGetTextDimensions");
  aDimensions.ascent = font->mMaxAscent;
  aDimensions.descent = font->mMaxDescent;

  
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
    font = (nsFontOS2*)fonts[currFont];
    PRUnichar* nextOffset = (PRUnichar*)offsets[++currFont]; 

    
    
    
    
    
    
    
    
    
    
    if (*pstr == ' ') {
      
      ++pstr;
      
      if (pstr == last) {
        break;
      }
      
      if (pstr == nextOffset) {
        font = (nsFontOS2*)fonts[currFont];
        nextOffset = (PRUnichar*)offsets[++currFont];
      } 
    }

    
    
    
    if (nextOffset > lastWord) {
      if (aLastWordDimensions.ascent < font->mMaxAscent) {
        aLastWordDimensions.ascent = font->mMaxAscent;
      }
      if (aLastWordDimensions.descent < font->mMaxDescent) {
        aLastWordDimensions.descent = font->mMaxDescent;
      }
    }

    
    if (pstr < lastWord) {
      if (aDimensions.ascent < font->mMaxAscent) {
        aDimensions.ascent = font->mMaxAscent;
      }
      if (aDimensions.descent < font->mMaxDescent) {
        aDimensions.descent = font->mMaxDescent;
      }
    }

    
    pstr = nextOffset;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextOS2::GetTextDimensions(const char*       aString,
                                         PRUint32          aLength,
                                         nsTextDimensions& aDimensions)
{
  GetWidth(aString, aLength, aDimensions.width);
  if (mFontMetrics)
  {
    mFontMetrics->GetMaxAscent(aDimensions.ascent);
    mFontMetrics->GetMaxDescent(aDimensions.descent);
  }
  return NS_OK;
}

struct GetTextDimensionsData {
  HPS                   mPS;      
  float                 mP2T;     
  nsDrawingSurfaceOS2*  mSurface; 
  nsFontOS2*            mFont;    
  LONG                  mWidth;   
  nscoord               mAscent;  
  nscoord               mDescent; 
};

static PRBool PR_CALLBACK
do_GetTextDimensions(const nsFontSwitch* aFontSwitch,
                     const PRUnichar*    aSubstring,
                     PRUint32            aSubstringLength,
                     void*               aData)
{
  nsFontOS2* font = aFontSwitch->mFont;
  
  GetTextDimensionsData* data = (GetTextDimensionsData*)aData;
  if (data->mFont != font) {
    
    data->mFont = font;
    data->mSurface->SelectFont(data->mFont);
  }

  data->mWidth += font->GetWidth(data->mPS, aSubstring, aSubstringLength);

   
  if (font->mMaxAscent == 0) {
    FONTMETRICS fm;
    GFX (::GpiQueryFontMetrics(data->mPS, sizeof (fm), &fm), FALSE);
    font->mMaxAscent  = NSToCoordRound( (fm.lMaxAscender-1) * data->mP2T );
    font->mMaxDescent = NSToCoordRound( (fm.lMaxDescender+1) * data->mP2T );
  }

  if (data->mAscent < font->mMaxAscent) {
    data->mAscent = font->mMaxAscent;
  }
  if (data->mDescent < font->mMaxDescent) {
    data->mDescent = font->mMaxDescent;
  }

  return PR_TRUE; 
}

NS_IMETHODIMP
nsRenderingContextOS2::GetTextDimensions(const PRUnichar*  aString,
                                         PRUint32          aLength,
                                         nsTextDimensions& aDimensions,
                                         PRInt32*          aFontID)
{
  aDimensions.Clear();

  if (!mFontMetrics)
    return NS_ERROR_FAILURE;

  SetupFontAndColor();

  nsFontMetricsOS2* metrics = (nsFontMetricsOS2*)mFontMetrics;
  GetTextDimensionsData data = {mPS, mP2T, mSurface, mCurrFontOS2, 0, 0, 0};

  metrics->ResolveForwards(mPS, aString, aLength, do_GetTextDimensions, &data);
  aDimensions.width = NSToCoordRound(float(data.mWidth) * mP2T);
  aDimensions.ascent = data.mAscent;
  aDimensions.descent = data.mDescent;

  if (data.mFont != mCurrFontOS2) {
    
    mSurface->SelectFont(mCurrFontOS2);
  }

  if (aFontID) *aFontID = 0;

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2 :: DrawString(const char *aString, PRUint32 aLength,
                                                  nscoord aX, nscoord aY,
                                                  const nscoord* aSpacing)
{
  NS_PRECONDITION(mFontMetrics,"Something is wrong somewhere");

  PRInt32 x = aX;
  PRInt32 y = aY;

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

  mCurrFontOS2->DrawString(mPS, mSurface, x, y, aString, aLength, dx0);

  if (dx0 && (dx0 != dxMem)) {
    delete [] dx0;
  }

  return NS_OK;
}

struct DrawStringData {
  HPS                   mPS;        
  nsDrawingSurfaceOS2*  mSurface;   
  nsFontOS2*            mFont;      
  nsTransform2D*        mTranMatrix;
  nscoord               mX;         
  nscoord               mY;         
  const nscoord*        mSpacing;   
  nscoord               mMaxLength; 
  nscoord               mLength;    
};

static PRBool PR_CALLBACK
do_DrawString(const nsFontSwitch* aFontSwitch,
              const PRUnichar*    aSubstring,
              PRUint32            aSubstringLength,
              void*               aData)
{
  nsFontOS2* font = aFontSwitch->mFont;

  PRInt32 x, y;
  DrawStringData* data = (DrawStringData*)aData;
  if (data->mFont != font) {
    
    data->mFont = font;
    data->mSurface->SelectFont(data->mFont);
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
        
        font->DrawString(data->mPS, data->mSurface, x, y, str, 2);
        
        data->mX += *data->mSpacing++;
        ++str;
      } else {
        font->DrawString(data->mPS, data->mSurface, x, y, str, 1);
      }
      data->mX += *data->mSpacing++;
      ++str;
    }
  }
  else {
    font->DrawString(data->mPS, data->mSurface, data->mX, data->mY, aSubstring,
                     aSubstringLength);
    
    if (data->mLength < data->mMaxLength) {
      data->mX += font->GetWidth(data->mPS, aSubstring, aSubstringLength);
    }
  }
  return PR_TRUE; 
}

NS_IMETHODIMP nsRenderingContextOS2 :: DrawString(const PRUnichar *aString, PRUint32 aLength,
                                                  nscoord aX, nscoord aY,
                                                  PRInt32 aFontID,
                                                  const nscoord* aSpacing)
{
  if (!mFontMetrics)
    return NS_ERROR_FAILURE;

  SetupFontAndColor();

  nsFontMetricsOS2* metrics = (nsFontMetricsOS2*)mFontMetrics;
  DrawStringData data = {mPS, mSurface, mCurrFontOS2, mTranMatrix, aX, aY,
                         aSpacing, aLength, 0};
  if (!aSpacing) { 
    mTranMatrix->TransformCoord(&data.mX, &data.mY);
  }

  if (mRightToLeftText) {
    metrics->ResolveBackwards(mPS, aString, aLength, do_DrawString, &data);
  }
  else
  {
    metrics->ResolveForwards(mPS, aString, aLength, do_DrawString, &data);
  }

  if (data.mFont != mCurrFontOS2) {
    
    mSurface->SelectFont(mCurrFontOS2);
  }

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextOS2 :: DrawString(const nsString& aString,
                                                  nscoord aX, nscoord aY,
                                                  PRInt32 aFontID,
                                                  const nscoord* aSpacing)
{
  return DrawString(aString.get(), aString.Length(), aX, aY, aFontID, aSpacing);
}

#ifdef MOZ_MATHML
NS_IMETHODIMP 
nsRenderingContextOS2::GetBoundingMetrics(const char*        aString,
                                          PRUint32           aLength,
                                          nsBoundingMetrics& aBoundingMetrics)
{
#if 0
  NS_PRECONDITION(mFontMetrics,"Something is wrong somewhere");

  aBoundingMetrics.Clear();
  if (!mFontMetrics) return NS_ERROR_FAILURE;

  SetupFontAndColor();

  
  MAT2 mat2;
  FIXED zero, one;
  zero.fract = 0; one.fract = 0;
  zero.value = 0; one.value = 1; 
  mat2.eM12 = mat2.eM21 = zero; 
  mat2.eM11 = mat2.eM22 = one; 

  
  nscoord descent;
  GLYPHMETRICS gm;
  DWORD len = GetGlyphOutline(mDC, aString[0], GGO_METRICS, &gm, 0, nsnull, &mat2);
  if (GDI_ERROR == len) {
    return NS_ERROR_UNEXPECTED;
  }
  
  descent = -(nscoord(gm.gmptGlyphOrigin.y) - nscoord(gm.gmBlackBoxY));
  aBoundingMetrics.leftBearing = gm.gmptGlyphOrigin.x;
  aBoundingMetrics.rightBearing = gm.gmptGlyphOrigin.x + gm.gmBlackBoxX;
  aBoundingMetrics.ascent = gm.gmptGlyphOrigin.y;
  aBoundingMetrics.descent = gm.gmptGlyphOrigin.y - gm.gmBlackBoxY;
  aBoundingMetrics.width = gm.gmCellIncX;

  if (1 < aLength) {
    
    PRUint32 i;
    for (i = 1; i < aLength; i++) {
      len = GetGlyphOutline(mDC, aString[i], GGO_METRICS, &gm, 0, nsnull, &mat2);
      if (GDI_ERROR == len) {
        return NS_ERROR_UNEXPECTED;
      }
      
      descent = -(nscoord(gm.gmptGlyphOrigin.y) - nscoord(gm.gmBlackBoxY));
      if (aBoundingMetrics.ascent < gm.gmptGlyphOrigin.y)
        aBoundingMetrics.ascent = gm.gmptGlyphOrigin.y;
      if (aBoundingMetrics.descent > descent)
        aBoundingMetrics.descent = descent;
    }
    
    SIZE size;
    ::GetTextExtentPoint32(mDC, aString, aLength, &size);
    aBoundingMetrics.width = size.cx;
    aBoundingMetrics.rightBearing = size.cx - gm.gmCellIncX + gm.gmBlackBoxX;
  }

  
  aBoundingMetrics.leftBearing = NSToCoordRound(float(aBoundingMetrics.leftBearing) * mP2T);
  aBoundingMetrics.rightBearing = NSToCoordRound(float(aBoundingMetrics.rightBearing) * mP2T);
  aBoundingMetrics.width = NSToCoordRound(float(aBoundingMetrics.width) * mP2T);
  aBoundingMetrics.ascent = NSToCoordRound(float(aBoundingMetrics.ascent) * mP2T);
  aBoundingMetrics.descent = NSToCoordRound(float(aBoundingMetrics.descent) * mP2T);

  return NS_OK;
#endif
  return NS_ERROR_FAILURE;
}

#if 0
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
#endif

NS_IMETHODIMP
nsRenderingContextOS2::GetBoundingMetrics(const PRUnichar*   aString,
                                          PRUint32           aLength,
                                          nsBoundingMetrics& aBoundingMetrics,
                                          PRInt32*           aFontID)
{
#if 0
  aBoundingMetrics.Clear();
  if (!mFontMetrics) return NS_ERROR_FAILURE;

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
#endif
  return NS_ERROR_FAILURE;
}
#endif 

NS_IMETHODIMP nsRenderingContextOS2::CopyOffScreenBits(
                     nsIDrawingSurface* aSrcSurf, PRInt32 aSrcX, PRInt32 aSrcY,
                     const nsRect &aDestBounds, PRUint32 aCopyFlags)
{
   NS_ASSERTION( aSrcSurf && mSurface && mMainSurface, "bad surfaces");

   nsDrawingSurfaceOS2 *SourceSurf = (nsDrawingSurfaceOS2 *) aSrcSurf;
   nsDrawingSurfaceOS2 *DestSurf;
   HPS DestPS;

   if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER)
   {
      DestSurf = mSurface;
      DestPS   = mPS;
   } else
   {
      DestSurf = mMainSurface;
      DestPS   = mMainSurface->GetPS();
   }

   PRUint32 DestHeight   = DestSurf->GetHeight();
   PRUint32 SourceHeight = SourceSurf->GetHeight();


   if (aCopyFlags & NS_COPYBITS_USE_SOURCE_CLIP_REGION)
   {
      HRGN SourceClipRegion = OS2_CopyClipRegion(SourceSurf->GetPS());

      
      
      

      if ( (SourceClipRegion) && (SourceHeight != DestHeight) )
      {
         POINTL Offset = { 0, -(SourceHeight - DestHeight) };

         GFX (::GpiOffsetRegion(DestPS, SourceClipRegion, &Offset), FALSE);
      }

      OS2_SetClipRegion(DestPS, SourceClipRegion);
   }


   nsRect drect(aDestBounds);

   if( aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
      mTranMatrix->TransformCoord( &aSrcX, &aSrcY);

   if( aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
      mTranMatrix->TransformCoord( &drect.x, &drect.y,
                               &drect.width, &drect.height);

   

   POINTL Points [3] = { {drect.x, DestHeight - drect.y - drect.height},    
                         {drect.x + drect.width, DestHeight - drect.y},     
                         {aSrcX, SourceHeight - aSrcY - drect.height} };    


   GFX (::GpiBitBlt(DestPS, SourceSurf->GetPS(), 3, Points, ROP_SRCCOPY, BBO_OR), GPI_ERROR);

   return NS_OK;
}

void*
nsRenderingContextOS2::GetNativeGraphicData(GraphicDataType aType)
{
  if (aType == NATIVE_WINDOWS_DC)
    return (void*)mPS;

  return nsnull;
}

void nsRenderingContextOS2::SetupFontAndColor(void)
{
  if (mFontMetrics) {
    
    nsFontHandle fontHandle;
    mFontMetrics->GetFontHandle(fontHandle);
    if (!mCurrFontOS2 || mCurrFontOS2 != fontHandle) {
      mCurrFontOS2 = (nsFontOS2*)fontHandle;
      mSurface->SelectFont(mCurrFontOS2);
    }
  }

  
  CHARBUNDLE cBundle;
  cBundle.lColor = GetGPIColor();
  cBundle.usMixMode = FM_OVERPAINT;
  cBundle.usBackMixMode = BM_LEAVEALONE;

  GFX (::GpiSetAttrs(mPS, PRIM_CHAR,
                     CBB_COLOR | CBB_MIX_MODE | CBB_BACK_MIX_MODE,
                     0, &cBundle),
       FALSE);
}

void nsRenderingContextOS2::PushClipState(void)
{                           
  if (!(mStates->mFlags & FLAG_CLIP_CHANGED))
  {
    GraphicsState *tstate = mStates->mNext;

    
    
    
    

    if (nsnull != tstate)
    {
      
      if( tstate->mClipRegion )
      {
         GpiDestroyRegion( mPS, tstate->mClipRegion );
      }

      tstate->mClipRegion = OS2_CopyClipRegion( mPS );

      if (tstate->mClipRegion != 0)
        tstate->mFlags |= FLAG_CLIP_VALID;
      else
        tstate->mFlags &= ~FLAG_CLIP_VALID;
    }
  
    mStates->mFlags |= FLAG_CLIP_CHANGED;
  }
}







LONG OS2_CombineClipRegion( HPS hps, HRGN hrgnCombine, LONG lMode)
{
   if (!hps) return RGN_ERROR;

   HRGN hrgnClip = 0;
   LONG rc = RGN_NULL;

   GFX (::GpiSetClipRegion (hps, 0, &hrgnClip), RGN_ERROR); 

   if (hrgnClip && hrgnClip != HRGN_ERROR)
   {
      if (lMode != CRGN_COPY)    
         GFX (::GpiCombineRegion (hps, hrgnCombine, hrgnClip, hrgnCombine, lMode), RGN_ERROR);
      
      GFX (::GpiDestroyRegion (hps, hrgnClip), FALSE);
   }

   if (hrgnCombine)
      rc = GFX (::GpiSetClipRegion (hps, hrgnCombine, NULL), RGN_ERROR);  

   return rc;
}


HRGN OS2_CopyClipRegion( HPS hps)
{
  if (!hps) return HRGN_ERROR;

  HRGN hrgn = 0, hrgnClip;

  
  GFX (::GpiSetClipRegion (hps, 0, &hrgnClip), RGN_ERROR);

  if (hrgnClip && hrgnClip != HRGN_ERROR)
  {
     hrgn = GFX (::GpiCreateRegion (hps, 0, NULL), RGN_ERROR);     
     GFX (::GpiCombineRegion (hps, hrgn, hrgnClip, 0, CRGN_COPY), RGN_ERROR);
     GFX (::GpiSetClipRegion (hps, hrgnClip, NULL), RGN_ERROR);    
  }

  return hrgn;
}


NS_IMETHODIMP nsRenderingContextOS2::ConditionRect(nscoord &x, nscoord &y, nscoord &w, nscoord &h)
{
  if (y < -134217728)
    y = -134217728;

  if (y + h > 134217727)
    h  = 134217727 - y;

  if (x < -134217728)
    x = -134217728;

  if (x + w > 134217727)
    w  = 134217727 - x;

  return NS_OK;
}
