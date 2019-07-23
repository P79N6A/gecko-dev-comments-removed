




































#include <MacMemory.h>

#include "nsDrawingSurfaceMac.h"
#include "nsGraphicState.h"
#include "nsRegionPool.h"

#ifdef MOZ_WIDGET_COCOA


extern CGContextRef Cocoa_LockFocus(void* view);
extern void Cocoa_UnlockFocus(void* view);
#endif

static NS_DEFINE_IID(kIDrawingSurfaceIID, NS_IDRAWING_SURFACE_IID);
static NS_DEFINE_IID(kIDrawingSurfaceMacIID, NS_IDRAWING_SURFACE_MAC_IID);







nsDrawingSurfaceMac::nsDrawingSurfaceMac()
{
  mPort = NULL;
  mGS = sGraphicStatePool.GetNewGS();	
  mWidth = mHeight = 0;
  mLockOffset = mLockHeight = 0;
  mLockFlags = 0;
  mIsOffscreen = PR_FALSE;
  mIsLocked = PR_FALSE;
#ifdef MOZ_WIDGET_COCOA
  mWidgetView = nsnull;
#endif
}






nsDrawingSurfaceMac::~nsDrawingSurfaceMac()
{
	if(mIsOffscreen && mPort){
  	GWorldPtr offscreenGWorld = (GWorldPtr)mPort;
		::UnlockPixels(::GetGWorldPixMap(offscreenGWorld));
		::DisposeGWorld(offscreenGWorld);
		
		nsGraphicsUtils::SetPortToKnownGoodPort();
	}

	if (mGS){
		sGraphicStatePool.ReleaseGS(mGS); 
	}
}






NS_IMETHODIMP nsDrawingSurfaceMac::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr)
    return NS_ERROR_NULL_POINTER;

  if (aIID.Equals(kIDrawingSurfaceIID)){
    nsIDrawingSurface* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  if (aIID.Equals(kIDrawingSurfaceMacIID)){
    nsIDrawingSurfaceMac* tmp = this;
    *aInstancePtr = (void*) tmp;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

  if (aIID.Equals(kISupportsIID)){
    nsIDrawingSurface* tmp = this;
    nsISupports* tmp2 = tmp;
    *aInstancePtr = (void*) tmp2;
    NS_ADDREF_THIS();
    return NS_OK;
  }

  return NS_NOINTERFACE;
}

NS_IMPL_ADDREF(nsDrawingSurfaceMac)
NS_IMPL_RELEASE(nsDrawingSurfaceMac)

#pragma mark-






NS_IMETHODIMP nsDrawingSurfaceMac::Lock(PRInt32 aX, PRInt32 aY,
                                          PRUint32 aWidth, PRUint32 aHeight,
                                          void **aBits, PRInt32 *aStride,
                                          PRInt32 *aWidthBytes, PRUint32 aFlags)
{

  if (!mIsLocked && mIsOffscreen && mPort)
  {
    
    GWorldPtr     offscreenGWorld = (GWorldPtr)mPort;

    
    PixMapHandle  thePixMap = ::GetGWorldPixMap(offscreenGWorld);
    Ptr           baseaddr  = ::GetPixBaseAddr(thePixMap);
    PRInt32       cmpSize   = ((**thePixMap).pixelSize >> 3);
    PRInt32       rowBytes  = (**thePixMap).rowBytes & 0x3FFF;

    *aBits = baseaddr + (aX * cmpSize) + aY * rowBytes;
    *aStride = rowBytes;
    *aWidthBytes = aWidth * cmpSize;

    mIsLocked = PR_TRUE;
  }
  else
  {
    NS_ASSERTION(0, "nested lock attempt");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}






NS_IMETHODIMP nsDrawingSurfaceMac::Unlock(void)
{
	mIsLocked = PR_FALSE;
  return NS_OK;
}






NS_IMETHODIMP nsDrawingSurfaceMac::GetDimensions(PRUint32 *aWidth, PRUint32 *aHeight)
{
  *aWidth = mWidth;
  *aHeight = mHeight;
  return NS_OK;
}






NS_IMETHODIMP nsDrawingSurfaceMac::IsPixelAddressable(PRBool *aAddressable)
{
  NS_ASSERTION(0, "Not implemented!");
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP nsDrawingSurfaceMac::GetPixelFormat(nsPixelFormat *aFormat)
{
  
  NS_ASSERTION(0, "Not implemented!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

#pragma mark -






NS_IMETHODIMP nsDrawingSurfaceMac::Init(nsIDrawingSurface*	aDS)
{
	nsDrawingSurfaceMac* surface = static_cast<nsDrawingSurfaceMac*>(aDS);
	surface->GetGrafPtr(&mPort);
	mGS->Init(surface);
	
  return NS_OK;
}






NS_IMETHODIMP nsDrawingSurfaceMac::Init(CGrafPtr aPort)
{
	
  mPort = aPort;
	mGS->Init(aPort);
  return NS_OK;
}






NS_IMETHODIMP nsDrawingSurfaceMac::Init(nsIWidget *aTheWidget)
{
	
 	mPort = reinterpret_cast<CGrafPtr>(aTheWidget->GetNativeData(NS_NATIVE_GRAPHIC));
	mGS->Init(aTheWidget);
#ifdef MOZ_WIDGET_COCOA
  mWidgetView = aTheWidget->GetNativeData(NS_NATIVE_WIDGET);
#endif
  return NS_OK;
}







NS_IMETHODIMP nsDrawingSurfaceMac::Init(PRUint32 aDepth, PRUint32 aWidth, PRUint32 aHeight, PRUint32 aFlags)
{
  PRUint32	depth;
  Rect			macRect;
  GWorldPtr offscreenGWorld = nsnull;
  Boolean   tryTempMemFirst = ((aFlags & NS_CREATEDRAWINGSURFACE_SHORTLIVED) != 0);
  
  depth = aDepth;
  mWidth = aWidth;
  mHeight = aHeight;
  

	
  if (aWidth != 0){
  	::SetRect(&macRect, 0, 0, aWidth, aHeight);
  }else{
  	::SetRect(&macRect, 0, 0, 2, 2);
	}

	

	
	
	
	const long kReserveHeapFreeSpace = (1024 * 1024);
	const long kReserveHeapContigSpace	= (512 * 1024);

  long	  totalSpace, contiguousSpace;
  
  if (tryTempMemFirst)
  {
	  ::NewGWorld(&offscreenGWorld, depth, &macRect, nsnull, nsnull, useTempMem);
    if (!offscreenGWorld)
    {
      
    	::PurgeSpace(&totalSpace, &contiguousSpace);		

    	if (totalSpace > kReserveHeapFreeSpace && contiguousSpace > kReserveHeapContigSpace)
     		::NewGWorld(&offscreenGWorld, depth, &macRect, nsnull, nsnull, 0);
    }
  }
  else    
  {
      
    	::PurgeSpace(&totalSpace, &contiguousSpace);		

    	if (totalSpace > kReserveHeapFreeSpace && contiguousSpace > kReserveHeapContigSpace)
     		::NewGWorld(&offscreenGWorld, depth, &macRect, nsnull, nsnull, 0);
  
      if (!offscreenGWorld)
	      ::NewGWorld(&offscreenGWorld, depth, &macRect, nsnull, nsnull, useTempMem);
  }
  
  if (!offscreenGWorld)
    return NS_ERROR_OUT_OF_MEMORY;  
  
	
	
  ::LockPixels(::GetGWorldPixMap(offscreenGWorld));

	
	{
	  StGWorldPortSetter  setter(offscreenGWorld);
	  ::EraseRect(&macRect);
  }

	Init(offscreenGWorld);
	mIsOffscreen = PR_TRUE;
  return NS_OK;
}




static OSStatus
CreatePathFromRectsProc(UInt16 aMessage, RgnHandle aRegion, const Rect* aRect,
                        void* aData)
{
  CGContextRef context = static_cast<CGContextRef>(aData);

  if (aMessage == kQDRegionToRectsMsgParse)
  {
    CGRect rect = ::CGRectMake(aRect->left, aRect->top,
                               aRect->right - aRect->left,
                               aRect->bottom - aRect->top);
    ::CGContextAddRect(context, rect);
  }

  return noErr;
}

NS_IMETHODIMP_(CGContextRef)
nsDrawingSurfaceMac::StartQuartzDrawing()
{
  CGContextRef context;
#ifdef MOZ_WIDGET_COCOA
  
  if (mWidgetView) {
    context = Cocoa_LockFocus(mWidgetView);
  } else
#endif
  {
    
    ::QDBeginCGContext(mPort, &context);

    
    Rect portRect;
    ::GetPortBounds(mPort, &portRect);
    ::CGContextTranslateCTM(context, 0, (float)(portRect.bottom - portRect.top));
    ::CGContextScaleCTM(context, 1, -1);
  }

  if (::IsPortClipRegionEmpty(mPort)) {
    
    CGRect rect = ::CGRectMake(0, 0, 0, 0);
    ::CGContextClipToRect(context, rect);
  } else {
    
    StRegionFromPool currentClipRgn;
    ::GetPortClipRegion(mPort, currentClipRgn);
    ::QDRegionToRects(currentClipRgn, kQDParseRegionFromTopLeft,
                      CreatePathFromRectsProc, context);
    ::CGContextClip(context);
  }

  return context;
}

NS_IMETHODIMP_(void)
nsDrawingSurfaceMac::EndQuartzDrawing(CGContextRef aContext)
{
  
  ::CGContextSynchronize(aContext);

#ifdef MOZ_WIDGET_COCOA
  if (mWidgetView)
    Cocoa_UnlockFocus(mWidgetView);
  else
#endif
    ::QDEndCGContext(mPort, &aContext);
}
