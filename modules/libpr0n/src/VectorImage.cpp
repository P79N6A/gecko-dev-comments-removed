





































#include "VectorImage.h"

namespace mozilla {
namespace imagelib {

NS_IMPL_ISUPPORTS3(VectorImage,
                   imgIContainer,
                   nsIStreamListener,
                   nsIRequestObserver)




VectorImage::VectorImage(imgStatusTracker* aStatusTracker) :
  Image(aStatusTracker) 
{
}

VectorImage::~VectorImage()
{
}




nsresult
VectorImage::Init(imgIDecoderObserver* aObserver,
                  const char* aMimeType,
                  PRUint32 aFlags)
{
  NS_NOTYETIMPLEMENTED("VectorImage::Init");
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
VectorImage::GetCurrentFrameRect(nsIntRect& aRect)
{
  NS_NOTYETIMPLEMENTED("VectorImage::GetCurrentFrameRect");
}

PRUint32
VectorImage::GetDataSize()
{
  NS_NOTYETIMPLEMENTED("VectorImage::GetDataSize");
  return 0;
}

nsresult
VectorImage::StartAnimation()
{
  NS_NOTYETIMPLEMENTED("VectorImage::StartAnimation");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
VectorImage::StopAnimation()
{
  NS_NOTYETIMPLEMENTED("VectorImage::StopAnimation");
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP
VectorImage::GetWidth(PRInt32* aWidth)
{
  NS_NOTYETIMPLEMENTED("VectorImage::GetWidth");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
VectorImage::GetHeight(PRInt32* aHeight)
{
  NS_NOTYETIMPLEMENTED("VectorImage::GetHeight");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
VectorImage::GetType(PRUint16* aType)
{
  *aType = imgIContainer::TYPE_VECTOR;
  return NS_OK;
}



NS_IMETHODIMP
VectorImage::GetAnimated(PRBool* aAnimated)
{
  NS_NOTYETIMPLEMENTED("VectorImage::GetAnimated");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
VectorImage::GetCurrentFrameIsOpaque(PRBool* aIsOpaque)
{
  NS_NOTYETIMPLEMENTED("VectorImage::GetCurrentFrameIsOpaque");
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP
VectorImage::GetFrame(PRUint32 aWhichFrame,
                      PRUint32 aFlags,
                      gfxASurface** _retval)
{
  NS_NOTYETIMPLEMENTED("VectorImage::GetFrame");
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP
VectorImage::CopyFrame(PRUint32 aWhichFrame,
                       PRUint32 aFlags,
                       gfxImageSurface** _retval)
{
  NS_NOTYETIMPLEMENTED("VectorImage::CopyFrame");
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP
VectorImage::ExtractFrame(PRUint32 aWhichFrame,
                          const nsIntRect& aRegion,
                          PRUint32 aFlags,
                          imgIContainer** _retval)
{
  NS_NOTYETIMPLEMENTED("VectorImage::ExtractFrame");
  return NS_ERROR_NOT_IMPLEMENTED;
}










NS_IMETHODIMP
VectorImage::Draw(gfxContext* aContext,
                  gfxPattern::GraphicsFilter aFilter,
                  const gfxMatrix& aUserSpaceToImageSpace,
                  const gfxRect& aFill,
                  const nsIntRect& aSubimage,
                  const nsIntSize& aViewportSize,
                  PRUint32 aFlags)
{
  NS_NOTYETIMPLEMENTED("VectorImage::Draw");
  return NS_ERROR_NOT_IMPLEMENTED;
}



nsIFrame*
VectorImage::GetRootLayoutFrame()
{
  NS_NOTYETIMPLEMENTED("VectorImage::GetRootLayoutFrame");
  return nsnull;
}



NS_IMETHODIMP
VectorImage::RequestDecode()
{
  
  return NS_OK;
}



NS_IMETHODIMP
VectorImage::LockImage()
{
  NS_NOTYETIMPLEMENTED("VectorImage::LockImage");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
VectorImage::UnlockImage()
{
  NS_NOTYETIMPLEMENTED("VectorImage::UnlockImage");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
VectorImage::GetAnimationMode(PRUint16* aAnimationMode)
{
  NS_NOTYETIMPLEMENTED("VectorImage::GetAnimationMode");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
VectorImage::SetAnimationMode(PRUint16 aAnimationMode)
{
  NS_NOTYETIMPLEMENTED("VectorImage::SetAnimationMode");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
VectorImage::ResetAnimation()
{
  NS_NOTYETIMPLEMENTED("VectorImage::ResetAnimation");
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP
VectorImage::OnStartRequest(nsIRequest* aRequest, nsISupports* aCtxt)
{
  NS_NOTYETIMPLEMENTED("VectorImage::OnStartRequest");
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP
VectorImage::OnStopRequest(nsIRequest* aRequest, nsISupports* aCtxt,
                           nsresult aStatus)
{
  NS_NOTYETIMPLEMENTED("VectorImage::OnStopRequest");
  return NS_ERROR_NOT_IMPLEMENTED;
}








NS_IMETHODIMP
VectorImage::OnDataAvailable(nsIRequest* aRequest, nsISupports* aCtxt,
                             nsIInputStream* aInStr, PRUint32 aSourceOffset,
                             PRUint32 aCount)
{
  NS_NOTYETIMPLEMENTED("VectorImage::OnDataAvailable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
