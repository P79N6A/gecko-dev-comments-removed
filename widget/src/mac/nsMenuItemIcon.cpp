









































#include "nsMenuItemIcon.h"

#include "prmem.h"
#include "nsIMenu.h"
#include "nsIMenuItem.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsWidgetAtoms.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMElement.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMCSSValue.h"
#include "nsIDOMCSSPrimitiveValue.h"
#include "nsThreadUtils.h"
#include "nsToolkit.h"
#include "nsNetUtil.h"
#include "imgILoader.h"
#include "imgIRequest.h"
#include "gfxIImageFrame.h"
#include "nsIImage.h"
#include "nsIImageMac.h"

#include <Carbon/Carbon.h>


static const PRUint32 kIconWidth = 16;
static const PRUint32 kIconHeight = 16;
static const PRUint32 kIconBitsPerComponent = 8;
static const PRUint32 kIconComponents = 4;
static const PRUint32 kIconBitsPerPixel = kIconBitsPerComponent *
                                          kIconComponents;
static const PRUint32 kIconBytesPerRow = kIconWidth * kIconBitsPerPixel / 8;
static const PRUint32 kIconBytes = kIconBytesPerRow * kIconHeight;


static void
PRAllocCGFree(void* aInfo, const void* aData, size_t aSize) {
  PR_Free((void*)aData);
}


NS_IMPL_ISUPPORTS2(nsMenuItemIcon, imgIContainerObserver, imgIDecoderObserver)

nsMenuItemIcon::nsMenuItemIcon(nsISupports* aMenuItem,
                               nsIMenu*     aMenu,
                               nsIContent*  aContent)
: mContent(aContent)
, mMenuItem(aMenuItem)
, mMenu(aMenu)
, mMenuRef(NULL)
, mMenuItemIndex(0)
, mLoadedIcon(PR_FALSE)
, mSetIcon(PR_FALSE)
{
}


nsMenuItemIcon::~nsMenuItemIcon()
{
  if (mIconRequest)
    mIconRequest->Cancel(NS_BINDING_ABORTED);
}


nsresult
nsMenuItemIcon::SetupIcon()
{
  nsresult rv;
  if (!mMenuRef || !mMenuItemIndex) {
    
    
    
    
    rv = mMenu->GetMenuRefAndItemIndexForMenuItem(mMenuItem,
                                                  (void**)&mMenuRef,
                                                  &mMenuItemIndex);
    if (NS_FAILED(rv)) return rv;
  }

  nsCOMPtr<nsIURI> iconURI;
  rv = GetIconURI(getter_AddRefs(iconURI));
  if (NS_FAILED(rv)) {
    
    
    OSStatus err;
    err = ::SetMenuItemIconHandle(mMenuRef, mMenuItemIndex, kMenuNoIcon, NULL);
    if (err != noErr) return NS_ERROR_FAILURE;

    return NS_OK;
  }

  rv = LoadIcon(iconURI);

  return rv;
}


nsresult
nsMenuItemIcon::GetIconURI(nsIURI** aIconURI)
{
  
  
  
  
  
  
  nsCOMPtr<nsIMenuItem> menuItem = do_QueryInterface(mMenuItem);
  if (menuItem) {
    nsIMenuItem::EMenuItemType menuItemType;
    menuItem->GetMenuItemType(&menuItemType);
    if (menuItemType == nsIMenuItem::eCheckbox ||
        menuItemType == nsIMenuItem::eRadio)
      return NS_ERROR_FAILURE;
  }

  if (!mContent) return NS_ERROR_FAILURE;

  
  nsAutoString imageURIString;
  PRBool hasImageAttr = mContent->GetAttr(kNameSpaceID_None,
                                          nsWidgetAtoms::image,
                                          imageURIString);

  nsresult rv;
  if (!hasImageAttr) {
    
    
    nsCOMPtr<nsIDOMDocumentView> domDocumentView =
     do_QueryInterface(mContent->GetDocument());
    if (!domDocumentView) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDOMAbstractView> domAbstractView;
    rv = domDocumentView->GetDefaultView(getter_AddRefs(domAbstractView));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDOMViewCSS> domViewCSS = do_QueryInterface(domAbstractView);
    if (!domViewCSS) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(mContent);
    if (!domElement) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIDOMCSSStyleDeclaration> cssStyleDecl;
    nsAutoString empty;
    rv = domViewCSS->GetComputedStyle(domElement, empty,
                                      getter_AddRefs(cssStyleDecl));
    if (NS_FAILED(rv)) return rv;

    NS_NAMED_LITERAL_STRING(listStyleImage, "list-style-image");
    nsCOMPtr<nsIDOMCSSValue> cssValue;
    rv = cssStyleDecl->GetPropertyCSSValue(listStyleImage,
                                           getter_AddRefs(cssValue));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDOMCSSPrimitiveValue> primitiveValue =
     do_QueryInterface(cssValue);
    if (!primitiveValue) return NS_ERROR_FAILURE;

    PRUint16 primitiveType;
    rv = primitiveValue->GetPrimitiveType(&primitiveType);
    if (NS_FAILED(rv)) return rv;
    if (primitiveType != nsIDOMCSSPrimitiveValue::CSS_URI)
      return NS_ERROR_FAILURE;

    rv = primitiveValue->GetStringValue(imageURIString);
    if (NS_FAILED(rv)) return rv;
  }

  
  
  nsCOMPtr<nsIURI> iconURI;
  rv = NS_NewURI(getter_AddRefs(iconURI), imageURIString);
  if (NS_FAILED(rv)) return rv;

  *aIconURI = iconURI;
  NS_ADDREF(*aIconURI);
  return NS_OK;
}


nsresult
nsMenuItemIcon::LoadIcon(nsIURI* aIconURI)
{
  if (mIconRequest) {
    
    mIconRequest->Cancel(NS_BINDING_ABORTED);
    mIconRequest = nsnull;
  }

  mLoadedIcon = PR_FALSE;

  if (!mContent) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocument> document = mContent->GetOwnerDoc();
  if (!document) return NS_ERROR_FAILURE;

  nsCOMPtr<nsILoadGroup> loadGroup = document->GetDocumentLoadGroup();
  if (!loadGroup) return NS_ERROR_FAILURE;

  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<imgILoader> loader = do_GetService("@mozilla.org/image/loader;1",
                                              &rv);
  if (NS_FAILED(rv)) return rv;

  if (!mSetIcon) {
    
    
    
    

    static PRBool sInitializedPlaceholder;
    static CGImageRef sPlaceholderIconImage;
    if (!sInitializedPlaceholder) {
      sInitializedPlaceholder = PR_TRUE;

      PRUint8* bitmap = (PRUint8*)PR_Malloc(kIconBytes);

      CGColorSpaceRef colorSpace = ::CGColorSpaceCreateDeviceRGB();

      CGContextRef bitmapContext;
      bitmapContext = ::CGBitmapContextCreate(bitmap, kIconWidth, kIconHeight,
                                              kIconBitsPerComponent,
                                              kIconBytesPerRow,
                                              colorSpace,
                                              kCGImageAlphaPremultipliedFirst);
      if (!bitmapContext) {
        PR_Free(bitmap);
        ::CGColorSpaceRelease(colorSpace);
        return NS_ERROR_FAILURE;
      }

      CGRect iconRect = ::CGRectMake(0, 0, kIconWidth, kIconHeight);
      ::CGContextClearRect(bitmapContext, iconRect);
      ::CGContextRelease(bitmapContext);

      CGDataProviderRef provider;
      provider = ::CGDataProviderCreateWithData(NULL, bitmap, kIconBytes,
                                              PRAllocCGFree);
      if (!provider) {
        PR_Free(bitmap);
        ::CGColorSpaceRelease(colorSpace);
        return NS_ERROR_FAILURE;
      }

      sPlaceholderIconImage =
       ::CGImageCreate(kIconWidth, kIconHeight, kIconBitsPerComponent,
                       kIconBitsPerPixel, kIconBytesPerRow, colorSpace,
                       kCGImageAlphaPremultipliedFirst, provider, NULL, TRUE,
                       kCGRenderingIntentDefault);
      ::CGColorSpaceRelease(colorSpace);
      ::CGDataProviderRelease(provider);
    }

    if (!sPlaceholderIconImage) return NS_ERROR_FAILURE;

    OSStatus err;
    err = ::SetMenuItemIconHandle(mMenuRef, mMenuItemIndex, kMenuCGImageRefType,
                                  (Handle)sPlaceholderIconImage);
    if (err != noErr) return NS_ERROR_FAILURE;
  }

  rv = loader->LoadImage(aIconURI, nsnull, nsnull, loadGroup, this,
                         nsnull, nsIRequest::LOAD_NORMAL, nsnull,
                         nsnull, getter_AddRefs(mIconRequest));
  if (NS_FAILED(rv)) return rv;

  
  
  

  if (ShouldLoadSync(aIconURI)) {
    
    

    nsCOMPtr<nsIThread> thread = NS_GetCurrentThread();
    if (!thread) return NS_OK;

    rv = NS_OK;
    while (!mLoadedIcon && mIconRequest && NS_SUCCEEDED(rv)) {
      PRBool processed;
      rv = thread->ProcessNextEvent(PR_TRUE, &processed);
      if (NS_SUCCEEDED(rv) && !processed)
        rv = NS_ERROR_UNEXPECTED;
    }
  }

  return NS_OK;
}


PRBool
nsMenuItemIcon::ShouldLoadSync(nsIURI* aURI)
{
#if 0 
  
  
  
  
  
  
  
  
  
  
  
  
  
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
  return PR_FALSE;
#else
  static PRBool sNeedsSync;

  static PRBool sInitialized;
  if (!sInitialized) {
    sInitialized = PR_TRUE;
    sNeedsSync = (nsToolkit::OSXVersion() < MAC_OS_X_VERSION_10_4_HEX);
  }

  if (sNeedsSync) {
    PRBool isLocalScheme;
    if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isLocalScheme)) &&
        isLocalScheme)
      return PR_TRUE;
    if (NS_SUCCEEDED(aURI->SchemeIs("data", &isLocalScheme)) &&
        isLocalScheme)
      return PR_TRUE;
    if (NS_SUCCEEDED(aURI->SchemeIs("moz-anno", &isLocalScheme)) &&
        isLocalScheme)
      return PR_TRUE;
  }

  return PR_FALSE;
#endif
#else 
  
  
  
  
  PRBool isLocalScheme;
  if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isLocalScheme)) &&
      isLocalScheme)
    return PR_TRUE;
  if (NS_SUCCEEDED(aURI->SchemeIs("data", &isLocalScheme)) &&
      isLocalScheme)
    return PR_TRUE;
  if (NS_SUCCEEDED(aURI->SchemeIs("moz-anno", &isLocalScheme)) &&
      isLocalScheme)
    return PR_TRUE;

  return PR_FALSE;
#endif 
}





NS_IMETHODIMP
nsMenuItemIcon::FrameChanged(imgIContainer*  aContainer,
                             gfxIImageFrame* aFrame,
                             nsIntRect*      aDirtyRect)
{
  return NS_OK;
}





NS_IMETHODIMP
nsMenuItemIcon::OnStartRequest(imgIRequest* aRequest)
{
  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemIcon::OnStartDecode(imgIRequest* aRequest)
{
  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemIcon::OnStartContainer(imgIRequest*   aRequest,
                                 imgIContainer* aContainer)
{
  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemIcon::OnStartFrame(imgIRequest* aRequest, gfxIImageFrame* aFrame)
{
  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemIcon::OnDataAvailable(imgIRequest*     aRequest,
                                gfxIImageFrame*  aFrame,
                                const nsIntRect* aRect)
{
  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemIcon::OnStopFrame(imgIRequest*    aRequest,
                            gfxIImageFrame* aFrame)
{
  if (aRequest != mIconRequest) return NS_ERROR_FAILURE;

  
  if (mLoadedIcon)
    return NS_OK;

  nsCOMPtr<gfxIImageFrame> frame = aFrame;
  nsCOMPtr<nsIImage> iimage = do_GetInterface(frame);
  if (!iimage) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIImageMac> imageMac = do_QueryInterface(iimage);
  if (!imageMac) return NS_ERROR_FAILURE;

  CGImageRef cgImage;
  nsresult rv = imageMac->GetCGImageRef(&cgImage);
  if (NS_FAILED(rv)) return rv;
  ::CGImageRetain(cgImage);

  
  
  
  PRUint8* bitmap = (PRUint8*)PR_Malloc(kIconBytes);

  CGColorSpaceRef colorSpace = ::CGColorSpaceCreateDeviceRGB();
  CGImageAlphaInfo alphaInfo = ::CGImageGetAlphaInfo(cgImage);

  CGContextRef bitmapContext;
  bitmapContext = ::CGBitmapContextCreate(bitmap, kIconWidth, kIconHeight,
                                          kIconBitsPerComponent,
                                          kIconBytesPerRow,
                                          colorSpace,
                                          alphaInfo);
  if (!bitmapContext) {
    ::CGImageRelease(cgImage);
    PR_Free(bitmap);
    ::CGColorSpaceRelease(colorSpace);
    return NS_ERROR_FAILURE;
  }

  
  
  ::CGContextTranslateCTM(bitmapContext, 0, kIconHeight);
  ::CGContextScaleCTM(bitmapContext, 1, -1);

  CGRect iconRect = ::CGRectMake(0, 0, kIconWidth, kIconHeight);
  ::CGContextClearRect(bitmapContext, iconRect);
  ::CGContextDrawImage(bitmapContext, iconRect, cgImage);
  ::CGImageRelease(cgImage);
  ::CGContextRelease(bitmapContext);

  CGDataProviderRef provider;
  provider = ::CGDataProviderCreateWithData(NULL, bitmap, kIconBytes,
                                            PRAllocCGFree);
  if (!provider) {
    PR_Free(bitmap);
    ::CGColorSpaceRelease(colorSpace);
    return NS_ERROR_FAILURE;
  }

  CGImageRef iconImage =
   ::CGImageCreate(kIconWidth, kIconHeight, kIconBitsPerComponent,
                   kIconBitsPerPixel, kIconBytesPerRow, colorSpace, alphaInfo,
                   provider, NULL, TRUE, kCGRenderingIntentDefault);
  ::CGColorSpaceRelease(colorSpace);
  ::CGDataProviderRelease(provider);
  if (!iconImage) return NS_ERROR_FAILURE;

  OSStatus err;
  err = ::SetMenuItemIconHandle(mMenuRef, mMenuItemIndex, kMenuCGImageRefType,
                                (Handle)iconImage);
  ::CGImageRelease(iconImage);
  if (err != noErr) return NS_ERROR_FAILURE;

  mLoadedIcon = PR_TRUE;
  mSetIcon = PR_TRUE;

  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemIcon::OnStopContainer(imgIRequest*   aRequest,
                                imgIContainer* aContainer)
{
  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemIcon::OnStopDecode(imgIRequest*     aRequest,
                             nsresult         status,
                             const PRUnichar* statusArg)
{
  return NS_OK;
}


NS_IMETHODIMP
nsMenuItemIcon::OnStopRequest(imgIRequest* aRequest,
                              PRBool       aIsLastPart)
{
  mIconRequest->Cancel(NS_BINDING_ABORTED);
  mIconRequest = nsnull;
  return NS_OK;
}
