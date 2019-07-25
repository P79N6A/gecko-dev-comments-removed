






















































#ifdef MOZ_IPC
#include "mozilla/plugins/PluginInstanceParent.h"
using mozilla::plugins::PluginInstanceParent;
#endif

#include "nsWindowGfx.h"
#include <windows.h>
#include "nsIRegion.h"
#include "gfxImageSurface.h"
#include "gfxWindowsSurface.h"
#include "gfxWindowsPlatform.h"
#include "nsGfxCIID.h"
#include "gfxContext.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "prmem.h"

#include "LayerManagerOGL.h"
#include "BasicLayers.h"
#ifdef MOZ_ENABLE_D3D9_LAYER
#include "LayerManagerD3D9.h"
#endif
#ifdef MOZ_ENABLE_D3D10_LAYER
#include "LayerManagerD3D10.h"
#endif

#ifndef WINCE
#include "nsUXThemeData.h"
#include "nsUXThemeConstants.h"
#endif

extern "C" {
#include "pixman.h"
}

using namespace mozilla::layers;

















static nsAutoPtr<PRUint8>  sSharedSurfaceData;
static gfxIntSize          sSharedSurfaceSize;







#ifdef CAIRO_HAS_DDRAW_SURFACE

static LPDIRECTDRAW glpDD                         = NULL;
static LPDIRECTDRAWSURFACE glpDDPrimary           = NULL;
static LPDIRECTDRAWCLIPPER glpDDClipper           = NULL;
static LPDIRECTDRAWSURFACE glpDDSecondary         = NULL;
static nsAutoPtr<gfxDDrawSurface> gpDDSurf        = NULL;
static DDSURFACEDESC gDDSDSecondary;
#endif

static NS_DEFINE_CID(kRegionCID,                  NS_REGION_CID);
static NS_DEFINE_IID(kRenderingContextCID,        NS_RENDERING_CONTEXT_CID);











static PRBool
IsRenderMode(gfxWindowsPlatform::RenderMode rmode)
{
  return gfxWindowsPlatform::GetPlatform()->GetRenderMode() == rmode;
}

nsIntRegion
nsWindowGfx::ConvertHRGNToRegion(HRGN aRgn)
{
  NS_ASSERTION(aRgn, "Don't pass NULL region here");

  nsIntRegion rgn;

  DWORD size = ::GetRegionData(aRgn, 0, NULL);
  nsAutoTArray<PRUint8,100> buffer;
  if (!buffer.SetLength(size))
    return rgn;

  RGNDATA* data = reinterpret_cast<RGNDATA*>(buffer.Elements());
  if (!::GetRegionData(aRgn, size, data))
    return rgn;

  if (data->rdh.nCount > MAX_RECTS_IN_REGION) {
    rgn = ToIntRect(data->rdh.rcBound);
    return rgn;
  }

  RECT* rects = reinterpret_cast<RECT*>(data->Buffer);
  for (PRUint32 i = 0; i < data->rdh.nCount; ++i) {
    RECT* r = rects + i;
    rgn.Or(rgn, ToIntRect(*r));
  }

  return rgn;
}

#ifdef CAIRO_HAS_DDRAW_SURFACE
PRBool
nsWindowGfx::InitDDraw()
{
  HRESULT hr;

  hr = DirectDrawCreate(NULL, &glpDD, NULL);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  hr = glpDD->SetCooperativeLevel(NULL, DDSCL_NORMAL);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  DDSURFACEDESC ddsd;
  memset(&ddsd, 0, sizeof(ddsd));
  ddsd.dwSize = sizeof(ddsd);
  ddsd.dwFlags = DDSD_CAPS;
  ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

  hr = glpDD->CreateSurface(&ddsd, &glpDDPrimary, NULL);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  hr = glpDD->CreateClipper(0, &glpDDClipper, NULL);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  hr = glpDDPrimary->SetClipper(glpDDClipper);
  NS_ENSURE_SUCCESS(hr, PR_FALSE);

  
  
  if (!IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_DDRAW16)) {
    gfxIntSize screen_size(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    gpDDSurf = new gfxDDrawSurface(glpDD, screen_size, gfxASurface::ImageFormatRGB24);
    if (!gpDDSurf) {
      
      fprintf(stderr, "couldn't create ddsurf\n");
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}
#endif











void nsWindowGfx::OnSettingsChangeGfx(WPARAM wParam)
{
#if defined(WINCE_WINDOWS_MOBILE)
  if (wParam == SETTINGCHANGE_RESET) {
    if (glpDDSecondary) {
      glpDDSecondary->Release();
      glpDDSecondary = NULL;
    }

    if(glpDD)
      glpDD->RestoreAllSurfaces();
  }
#endif
}





nsIntRegion nsWindow::GetRegionToPaint(PRBool aForceFullRepaint,
                                       PAINTSTRUCT ps, HDC aDC)
{
  if (aForceFullRepaint) {
    RECT paintRect;
    ::GetClientRect(mWnd, &paintRect);
    return nsIntRegion(nsWindowGfx::ToIntRect(paintRect));
  }

#if defined(WINCE_WINDOWS_MOBILE) || !defined(WINCE)
  HRGN paintRgn = ::CreateRectRgn(0, 0, 0, 0);
  if (paintRgn != NULL) {
# ifdef WINCE
    int result = GetUpdateRgn(mWnd, paintRgn, FALSE);
# else
    int result = GetRandomRgn(aDC, paintRgn, SYSRGN);
# endif
    if (result == 1) {
      POINT pt = {0,0};
      ::MapWindowPoints(NULL, mWnd, &pt, 1);
      ::OffsetRgn(paintRgn, pt.x, pt.y);
    }
    nsIntRegion rgn(nsWindowGfx::ConvertHRGNToRegion(paintRgn));
    ::DeleteObject(paintRgn);
# ifdef WINCE
    if (!rgn.IsEmpty())
# endif
      return rgn;
  }
#endif
  return nsIntRegion(nsWindowGfx::ToIntRect(ps.rcPaint));
}

#define WORDSSIZE(x) ((x).width * (x).height)
static PRBool
EnsureSharedSurfaceSize(gfxIntSize size)
{
  gfxIntSize screenSize;
  screenSize.height = GetSystemMetrics(SM_CYSCREEN);
  screenSize.width = GetSystemMetrics(SM_CXSCREEN);

  if (WORDSSIZE(screenSize) > WORDSSIZE(size))
    size = screenSize;

  if (WORDSSIZE(screenSize) < WORDSSIZE(size))
    NS_WARNING("Trying to create a shared surface larger than the screen");

  if (!sSharedSurfaceData || (WORDSSIZE(size) > WORDSSIZE(sSharedSurfaceSize))) {
    sSharedSurfaceSize = size;
    sSharedSurfaceData = nsnull;
    sSharedSurfaceData = (PRUint8 *)malloc(WORDSSIZE(sSharedSurfaceSize) * 4);
  }

  return (sSharedSurfaceData != nsnull);
}

PRBool nsWindow::OnPaint(HDC aDC, PRUint32 aNestingLevel)
{
#ifdef MOZ_IPC
  
  
  
  
  if (mozilla::ipc::RPCChannel::IsSpinLoopActive() && mPainting)
    return PR_FALSE;

  if (mWindowType == eWindowType_plugin) {

    





    RECT updateRect;
    if (!GetUpdateRect(mWnd, &updateRect, FALSE) ||
        (updateRect.left == updateRect.right &&
         updateRect.top == updateRect.bottom)) {
      PAINTSTRUCT ps;
      BeginPaint(mWnd, &ps);
      EndPaint(mWnd, &ps);
      return PR_TRUE;
    }

    PluginInstanceParent* instance = reinterpret_cast<PluginInstanceParent*>(
      ::GetPropW(mWnd, L"PluginInstanceParentProperty"));
    if (instance) {
      instance->CallUpdateWindow();
      ValidateRect(mWnd, NULL);
      return PR_TRUE;
    }
  }
#endif

#ifdef MOZ_IPC
  
  
  
  
  if (mozilla::ipc::RPCChannel::IsSpinLoopActive() && mPainting)
    return PR_FALSE;
#endif

  nsPaintEvent willPaintEvent(PR_TRUE, NS_WILL_PAINT, this);
  willPaintEvent.willSendDidPaint = PR_TRUE;
  DispatchWindowEvent(&willPaintEvent);

#ifdef CAIRO_HAS_DDRAW_SURFACE
  if (IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_DDRAW16)) {
    return OnPaintImageDDraw16();
  }
#endif

  PRBool result = PR_TRUE;
  PAINTSTRUCT ps;
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

#ifdef MOZ_XUL
  if (!aDC && (eTransparencyTransparent == mTransparencyMode))
  {
    
    
    
    
    
    
    ::BeginPaint(mWnd, &ps);
    ::EndPaint(mWnd, &ps);

    aDC = mMemoryDC;
  }
#endif

  mPainting = PR_TRUE;

#ifdef WIDGET_DEBUG_OUTPUT
  HRGN debugPaintFlashRegion = NULL;
  HDC debugPaintFlashDC = NULL;

  if (debug_WantPaintFlashing())
  {
    debugPaintFlashRegion = ::CreateRectRgn(0, 0, 0, 0);
    ::GetUpdateRgn(mWnd, debugPaintFlashRegion, TRUE);
    debugPaintFlashDC = ::GetDC(mWnd);
  }
#endif 

  HDC hDC = aDC ? aDC : (::BeginPaint(mWnd, &ps));
  if (!IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D)) {
    mPaintDC = hDC;
  }

  
  nsPaintEvent event(PR_TRUE, NS_PAINT, this);
  InitEvent(event);

#ifdef MOZ_XUL
  PRBool forceRepaint = aDC || (eTransparencyTransparent == mTransparencyMode);
#else
  PRBool forceRepaint = NULL != aDC;
#endif
  event.region = GetRegionToPaint(forceRepaint, ps, hDC);
  event.willSendDidPaint = PR_TRUE;

  if (!event.region.IsEmpty() && mEventCallback)
  {
    
    

#ifdef WIDGET_DEBUG_OUTPUT
    debug_DumpPaintEvent(stdout,
                         this,
                         &event,
                         nsCAutoString("noname"),
                         (PRInt32) mWnd);
#endif 

    switch (GetLayerManager()->GetBackendType()) {
      case LayerManager::LAYERS_BASIC:
        {
          nsRefPtr<gfxASurface> targetSurface;

#if defined(MOZ_XUL)
          
          if ((IsRenderMode(gfxWindowsPlatform::RENDER_GDI) ||
               IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D)) &&
              eTransparencyTransparent == mTransparencyMode) {
            if (mTransparentSurface == nsnull)
              SetupTranslucentWindowMemoryBitmap(mTransparencyMode);
            targetSurface = mTransparentSurface;
          }
#endif

          nsRefPtr<gfxWindowsSurface> targetSurfaceWin;
          if (!targetSurface &&
              IsRenderMode(gfxWindowsPlatform::RENDER_GDI))
          {
            PRUint32 flags = (mTransparencyMode == eTransparencyOpaque) ? 0 :
                gfxWindowsSurface::FLAG_IS_TRANSPARENT;
            targetSurfaceWin = new gfxWindowsSurface(hDC, flags);
            targetSurface = targetSurfaceWin;
          }
#ifdef CAIRO_HAS_D2D_SURFACE
          if (!targetSurface &&
              IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D))
          {
            if (!mD2DWindowSurface) {
              gfxASurface::gfxContentType content = gfxASurface::CONTENT_COLOR;
#if defined(MOZ_XUL)
              if (mTransparencyMode != eTransparencyOpaque) {
                content = gfxASurface::CONTENT_COLOR_ALPHA;
              }
#endif
              mD2DWindowSurface = new gfxD2DSurface(mWnd, content);
            }
            targetSurface = mD2DWindowSurface;
          }
#endif
#ifdef CAIRO_HAS_DDRAW_SURFACE
          nsRefPtr<gfxDDrawSurface> targetSurfaceDDraw;
          if (!targetSurface &&
              (IsRenderMode(gfxWindowsPlatform::RENDER_DDRAW) ||
               IsRenderMode(gfxWindowsPlatform::RENDER_DDRAW_GL)))
          {
            if (!glpDD) {
              if (!nsWindowGfx::InitDDraw()) {
                NS_WARNING("DirectDraw init failed; falling back to RENDER_IMAGE_STRETCH24");
                gfxWindowsPlatform::GetPlatform()->SetRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH24);
                goto DDRAW_FAILED;
              }
            }

            
            
            RECT winrect;
            GetClientRect(mWnd, &winrect);
            MapWindowPoints(mWnd, NULL, (LPPOINT)&winrect, 2);

            targetSurfaceDDraw = new gfxDDrawSurface(gpDDSurf.get(), winrect);
            targetSurface = targetSurfaceDDraw;
          }

DDRAW_FAILED:
#endif
          nsRefPtr<gfxImageSurface> targetSurfaceImage;
          if (!targetSurface &&
              (IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH32) ||
               IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH24)))
          {
            gfxIntSize surfaceSize(ps.rcPaint.right - ps.rcPaint.left,
                                   ps.rcPaint.bottom - ps.rcPaint.top);

            if (!EnsureSharedSurfaceSize(surfaceSize)) {
              NS_ERROR("Couldn't allocate a shared image surface!");
              return NS_ERROR_FAILURE;
            }

            
            
            targetSurfaceImage = new gfxImageSurface(sSharedSurfaceData.get(),
                                                     surfaceSize,
                                                     surfaceSize.width * 4,
                                                     gfxASurface::ImageFormatRGB24);

            if (targetSurfaceImage && !targetSurfaceImage->CairoStatus()) {
              targetSurfaceImage->SetDeviceOffset(gfxPoint(-ps.rcPaint.left, -ps.rcPaint.top));
              targetSurface = targetSurfaceImage;
            }
          }

          if (!targetSurface) {
            NS_ERROR("Invalid RenderMode!");
            return NS_ERROR_FAILURE;
          }

          nsRefPtr<gfxContext> thebesContext = new gfxContext(targetSurface);
          thebesContext->SetFlag(gfxContext::FLAG_DESTINED_FOR_SCREEN);
          if (IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D)) {
            const nsIntRect* r;
            for (nsIntRegionRectIterator iter(event.region);
                 (r = iter.Next()) != nsnull;) {
              thebesContext->Rectangle(gfxRect(r->x, r->y, r->width, r->height), PR_TRUE);
            }
            thebesContext->Clip();
            thebesContext->SetOperator(gfxContext::OPERATOR_CLEAR);
            thebesContext->Paint();
            thebesContext->SetOperator(gfxContext::OPERATOR_OVER);
          }
#ifdef WINCE
          thebesContext->SetFlag(gfxContext::FLAG_SIMPLIFY_OPERATORS);
#endif

          
          BasicLayerManager::BufferMode doubleBuffering =
            BasicLayerManager::BUFFER_NONE;
          if (IsRenderMode(gfxWindowsPlatform::RENDER_GDI)) {
# if defined(MOZ_XUL) && !defined(WINCE)
            switch (mTransparencyMode) {
              case eTransparencyGlass:
              case eTransparencyBorderlessGlass:
              default:
                
                doubleBuffering = BasicLayerManager::BUFFER_BUFFERED;
                break;
              case eTransparencyTransparent:
                
                
                thebesContext->SetOperator(gfxContext::OPERATOR_CLEAR);
                thebesContext->Paint();
                thebesContext->SetOperator(gfxContext::OPERATOR_OVER);
                break;
            }
#else
            doubleBuffering = BasicLayerManager::BUFFER_BUFFERED;
#endif
          }

          {
            AutoLayerManagerSetup
                setupLayerManager(this, thebesContext, doubleBuffering);
            result = DispatchWindowEvent(&event, eventStatus);
          }

#ifdef MOZ_XUL
          if ((IsRenderMode(gfxWindowsPlatform::RENDER_GDI) ||
               IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D))&&
              eTransparencyTransparent == mTransparencyMode) {
            
            
            
            UpdateTranslucentWindow();
          } else
#endif
#ifdef CAIRO_HAS_D2D_SURFACE
          if (result) {
            if (mD2DWindowSurface) {
              mD2DWindowSurface->Present();
            }
          }
#endif
          if (result) {
            if (IsRenderMode(gfxWindowsPlatform::RENDER_DDRAW) ||
                       IsRenderMode(gfxWindowsPlatform::RENDER_DDRAW_GL))
            {
#ifdef CAIRO_HAS_DDRAW_SURFACE
              
              HRESULT hr = glpDDClipper->SetHWnd(0, mWnd);

#ifdef DEBUG
              if (FAILED(hr))
                DDError("SetHWnd", hr);
#endif

              
              
              RECT dst_rect = ps.rcPaint;
              MapWindowPoints(mWnd, NULL, (LPPOINT)&dst_rect, 2);
              hr = glpDDPrimary->Blt(&dst_rect,
                                     gpDDSurf->GetDDSurface(),
                                     &dst_rect,
                                     DDBLT_WAITNOTBUSY,
                                     NULL);
#ifdef DEBUG
              if (FAILED(hr))
                DDError("SetHWnd", hr);
#endif
#endif
            } else if (IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH24) ||
                       IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH32)) 
            {
              gfxIntSize surfaceSize = targetSurfaceImage->GetSize();

              
              BITMAPINFOHEADER bi;
              memset(&bi, 0, sizeof(BITMAPINFOHEADER));
              bi.biSize = sizeof(BITMAPINFOHEADER);
              bi.biWidth = surfaceSize.width;
              bi.biHeight = - surfaceSize.height;
              bi.biPlanes = 1;
              bi.biBitCount = 32;
              bi.biCompression = BI_RGB;

              if (IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH24)) {
                
                
                
                
                
                int srcstride = surfaceSize.width*4;
                int dststride = surfaceSize.width*3;
                dststride = (dststride + 3) & ~3;

                
                for (int j = 0; j < surfaceSize.height; ++j) {
                  unsigned int *src = (unsigned int*) (targetSurfaceImage->Data() + j*srcstride);
                  unsigned int *dst = (unsigned int*) (targetSurfaceImage->Data() + j*dststride);

                  
                  
                  
                  
                  
                  
                  int width_left = surfaceSize.width;
                  while (width_left >= 4) {
                    unsigned int a = *src++;
                    unsigned int b = *src++;
                    unsigned int c = *src++;
                    unsigned int d = *src++;

                    *dst++ =  (a & 0x00ffffff)        | (b << 24);
                    *dst++ = ((b & 0x00ffff00) >> 8)  | (c << 16);
                    *dst++ = ((c & 0x00ff0000) >> 16) | (d << 8);

                    width_left -= 4;
                  }

                  
                  
                  unsigned char *bsrc = (unsigned char*) src;
                  unsigned char *bdst = (unsigned char*) dst;
                  switch (width_left) {
                    case 3:
                      *bdst++ = *bsrc++;
                      *bdst++ = *bsrc++;
                      *bdst++ = *bsrc++;
                      bsrc++;
                    case 2:
                      *bdst++ = *bsrc++;
                      *bdst++ = *bsrc++;
                      *bdst++ = *bsrc++;
                      bsrc++;
                    case 1:
                      *bdst++ = *bsrc++;
                      *bdst++ = *bsrc++;
                      *bdst++ = *bsrc++;
                      bsrc++;
                    case 0:
                      break;
                  }
                }

                bi.biBitCount = 24;
              }

              StretchDIBits(hDC,
                            ps.rcPaint.left, ps.rcPaint.top,
                            surfaceSize.width, surfaceSize.height,
                            0, 0,
                            surfaceSize.width, surfaceSize.height,
                            targetSurfaceImage->Data(),
                            (BITMAPINFO*) &bi,
                            DIB_RGB_COLORS,
                            SRCCOPY);
            }
          }
        }
        break;
      case LayerManager::LAYERS_OPENGL:
        static_cast<mozilla::layers::LayerManagerOGL*>(GetLayerManager())->
          SetClippingRegion(event.region);
        result = DispatchWindowEvent(&event, eventStatus);
        break;
#ifdef MOZ_ENABLE_D3D9_LAYER
      case LayerManager::LAYERS_D3D9:
        {
          LayerManagerD3D9 *layerManagerD3D9 =
            static_cast<mozilla::layers::LayerManagerD3D9*>(GetLayerManager());
          layerManagerD3D9->SetClippingRegion(event.region);
          result = DispatchWindowEvent(&event, eventStatus);
          if (layerManagerD3D9->DeviceWasRemoved()) {
            mLayerManager = nsnull;
            
            
            gfxWindowsPlatform::GetPlatform()->UpdateRenderMode();
            Invalidate(PR_FALSE);
          }
        }
        break;
#endif
#ifdef MOZ_ENABLE_D3D10_LAYER
      case LayerManager::LAYERS_D3D10:
        {
          gfxWindowsPlatform::GetPlatform()->UpdateRenderMode();
          LayerManagerD3D10 *layerManagerD3D10 = static_cast<mozilla::layers::LayerManagerD3D10*>(GetLayerManager());
          cairo_device_t *device = gfxWindowsPlatform::GetPlatform()->GetD2DDevice();

          if (!device || layerManagerD3D10->device() != cairo_d2d_device_get_device(device)) {
            mLayerManager = nsnull;
            Invalidate(PR_FALSE);
          } else {
            result = DispatchWindowEvent(&event, eventStatus);
          }
        }
        break;
#endif
      default:
        NS_ERROR("Unknown layers backend used!");
        break;
    }
  }

  if (!aDC) {
    ::EndPaint(mWnd, &ps);
  }

  mPaintDC = nsnull;

#if defined(WIDGET_DEBUG_OUTPUT) && !defined(WINCE)
  if (debug_WantPaintFlashing())
  {
    
    
    
    if (nsEventStatus_eIgnore != eventStatus) {
      ::InvertRgn(debugPaintFlashDC, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));
      ::InvertRgn(debugPaintFlashDC, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));
    }
    ::ReleaseDC(mWnd, debugPaintFlashDC);
    ::DeleteObject(debugPaintFlashRegion);
  }
#endif 

  mPainting = PR_FALSE;

  nsPaintEvent didPaintEvent(PR_TRUE, NS_DID_PAINT, this);
  DispatchWindowEvent(&didPaintEvent);

  if (aNestingLevel == 0 && ::GetUpdateRect(mWnd, NULL, PR_FALSE)) {
    OnPaint(aDC, 1);
  }

  return result;
}

nsresult nsWindowGfx::CreateIcon(imgIContainer *aContainer,
                                  PRBool aIsCursor,
                                  PRUint32 aHotspotX,
                                  PRUint32 aHotspotY,
                                  HICON *aIcon) {

  
  nsRefPtr<gfxImageSurface> frame;
  aContainer->CopyFrame(imgIContainer::FRAME_CURRENT,
                        imgIContainer::FLAG_SYNC_DECODE,
                        getter_AddRefs(frame));
  if (!frame)
    return NS_ERROR_NOT_AVAILABLE;

  PRUint8 *data = frame->Data();

  PRInt32 width = frame->Width();
  PRInt32 height = frame->Height();

  HBITMAP bmp = DataToBitmap(data, width, -height, 32);
  PRUint8* a1data = Data32BitTo1Bit(data, width, height);
  if (!a1data) {
    return NS_ERROR_FAILURE;
  }

  HBITMAP mbmp = DataToBitmap(a1data, width, -height, 1);
  PR_Free(a1data);

  ICONINFO info = {0};
  info.fIcon = !aIsCursor;
  info.xHotspot = aHotspotX;
  info.yHotspot = aHotspotY;
  info.hbmMask = mbmp;
  info.hbmColor = bmp;

  HCURSOR icon = ::CreateIconIndirect(&info);
  ::DeleteObject(mbmp);
  ::DeleteObject(bmp);
  if (!icon)
    return NS_ERROR_FAILURE;
  *aIcon = icon;
  return NS_OK;
}


PRUint8* nsWindowGfx::Data32BitTo1Bit(PRUint8* aImageData,
                                      PRUint32 aWidth, PRUint32 aHeight)
{
  
  
  PRUint32 outBpr = ((aWidth + 31) / 8) & ~3;

  
  PRUint8* outData = (PRUint8*)PR_Calloc(outBpr, aHeight);
  if (!outData)
    return NULL;

  PRInt32 *imageRow = (PRInt32*)aImageData;
  for (PRUint32 curRow = 0; curRow < aHeight; curRow++) {
    PRUint8 *outRow = outData + curRow * outBpr;
    PRUint8 mask = 0x80;
    for (PRUint32 curCol = 0; curCol < aWidth; curCol++) {
      
      if (*imageRow++ < 0)
        *outRow |= mask;

      mask >>= 1;
      if (!mask) {
        outRow ++;
        mask = 0x80;
      }
    }
  }

  return outData;
}

PRBool nsWindowGfx::IsCursorTranslucencySupported()
{
#ifdef WINCE
  return PR_FALSE;
#else
  static PRBool didCheck = PR_FALSE;
  static PRBool isSupported = PR_FALSE;
  if (!didCheck) {
    didCheck = PR_TRUE;
    
    isSupported = nsWindow::GetWindowsVersion() >= 0x501;
  }

  return isSupported;
#endif
}
















HBITMAP nsWindowGfx::DataToBitmap(PRUint8* aImageData,
                                  PRUint32 aWidth,
                                  PRUint32 aHeight,
                                  PRUint32 aDepth)
{
#ifndef WINCE
  HDC dc = ::GetDC(NULL);

  if (aDepth == 32 && IsCursorTranslucencySupported()) {
    
    BITMAPV4HEADER head = { 0 };
    head.bV4Size = sizeof(head);
    head.bV4Width = aWidth;
    head.bV4Height = aHeight;
    head.bV4Planes = 1;
    head.bV4BitCount = aDepth;
    head.bV4V4Compression = BI_BITFIELDS;
    head.bV4SizeImage = 0; 
    head.bV4XPelsPerMeter = 0;
    head.bV4YPelsPerMeter = 0;
    head.bV4ClrUsed = 0;
    head.bV4ClrImportant = 0;

    head.bV4RedMask   = 0x00FF0000;
    head.bV4GreenMask = 0x0000FF00;
    head.bV4BlueMask  = 0x000000FF;
    head.bV4AlphaMask = 0xFF000000;

    HBITMAP bmp = ::CreateDIBitmap(dc,
                                   reinterpret_cast<CONST BITMAPINFOHEADER*>(&head),
                                   CBM_INIT,
                                   aImageData,
                                   reinterpret_cast<CONST BITMAPINFO*>(&head),
                                   DIB_RGB_COLORS);
    ::ReleaseDC(NULL, dc);
    return bmp;
  }

  char reserved_space[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2];
  BITMAPINFOHEADER& head = *(BITMAPINFOHEADER*)reserved_space;

  head.biSize = sizeof(BITMAPINFOHEADER);
  head.biWidth = aWidth;
  head.biHeight = aHeight;
  head.biPlanes = 1;
  head.biBitCount = (WORD)aDepth;
  head.biCompression = BI_RGB;
  head.biSizeImage = 0; 
  head.biXPelsPerMeter = 0;
  head.biYPelsPerMeter = 0;
  head.biClrUsed = 0;
  head.biClrImportant = 0;
  
  BITMAPINFO& bi = *(BITMAPINFO*)reserved_space;

  if (aDepth == 1) {
    RGBQUAD black = { 0, 0, 0, 0 };
    RGBQUAD white = { 255, 255, 255, 0 };

    bi.bmiColors[0] = white;
    bi.bmiColors[1] = black;
  }

  HBITMAP bmp = ::CreateDIBitmap(dc, &head, CBM_INIT, aImageData, &bi, DIB_RGB_COLORS);
  ::ReleaseDC(NULL, dc);
  return bmp;
#else
  return nsnull;
#endif
}



#if defined(CAIRO_HAS_DDRAW_SURFACE)
PRBool nsWindow::OnPaintImageDDraw16()
{
  PRBool result = PR_FALSE;
  PAINTSTRUCT ps;
  nsPaintEvent event(PR_TRUE, NS_PAINT, this);
  gfxIntSize surfaceSize;
  nsRefPtr<gfxImageSurface> targetSurfaceImage;
  nsRefPtr<gfxContext> thebesContext;
  nsEventStatus eventStatus = nsEventStatus_eIgnore;
  gfxIntSize newSize;
  newSize.height = GetSystemMetrics(SM_CYSCREEN);
  newSize.width = GetSystemMetrics(SM_CXSCREEN);
  mPainting = PR_TRUE;

  HDC hDC = ::BeginPaint(mWnd, &ps);
  mPaintDC = hDC;
  nsIntRegion paintRgn = GetRegionToPaint(PR_FALSE, ps, hDC);

  if (paintRgn.IsEmpty() || !mEventCallback) {
    result = PR_TRUE;
    goto cleanup;
  }

  InitEvent(event);
  
  if (!glpDD) {
    if (!nsWindowGfx::InitDDraw()) {
      NS_WARNING("DirectDraw init failed.  Giving up.");
      goto cleanup;
    }
  }

  if (!glpDDSecondary) {

    memset(&gDDSDSecondary, 0, sizeof (gDDSDSecondary));
    memset(&gDDSDSecondary.ddpfPixelFormat, 0, sizeof(gDDSDSecondary.ddpfPixelFormat));
    
    gDDSDSecondary.dwSize = sizeof (gDDSDSecondary);
    gDDSDSecondary.ddpfPixelFormat.dwSize = sizeof(gDDSDSecondary.ddpfPixelFormat);
    
    gDDSDSecondary.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

    gDDSDSecondary.dwHeight = newSize.height;
    gDDSDSecondary.dwWidth  = newSize.width;

    gDDSDSecondary.ddpfPixelFormat.dwFlags = DDPF_RGB;
    gDDSDSecondary.ddpfPixelFormat.dwRGBBitCount = 16;
    gDDSDSecondary.ddpfPixelFormat.dwRBitMask = 0xf800;
    gDDSDSecondary.ddpfPixelFormat.dwGBitMask = 0x07e0;
    gDDSDSecondary.ddpfPixelFormat.dwBBitMask = 0x001f;
    
    HRESULT hr = glpDD->CreateSurface(&gDDSDSecondary, &glpDDSecondary, 0);
    if (FAILED(hr)) {
#ifdef DEBUG
      DDError("CreateSurface renderer", hr);
#endif
      goto cleanup;
    }
  }

  PRInt32 brx = paintRgn.GetBounds().x;
  PRInt32 bry = paintRgn.GetBounds().y;
  PRInt32 brw = paintRgn.GetBounds().width;
  PRInt32 brh = paintRgn.GetBounds().height;
  surfaceSize = gfxIntSize(brw, brh);
  
  if (!EnsureSharedSurfaceSize(surfaceSize))
    goto cleanup;

  targetSurfaceImage = new gfxImageSurface(sSharedSurfaceData.get(),
                                           surfaceSize,
                                           surfaceSize.width * 4,
                                           gfxASurface::ImageFormatRGB24);
    
  if (!targetSurfaceImage || targetSurfaceImage->CairoStatus())
    goto cleanup;
    
  targetSurfaceImage->SetDeviceOffset(gfxPoint(-brx, -bry));
  
  thebesContext = new gfxContext(targetSurfaceImage);
  thebesContext->SetFlag(gfxContext::FLAG_DESTINED_FOR_SCREEN);
  thebesContext->SetFlag(gfxContext::FLAG_SIMPLIFY_OPERATORS);
    
  {
    AutoLayerManagerSetup setupLayerManager(this, thebesContext);
    event.region = paintRgn;
    result = DispatchWindowEvent(&event, eventStatus);
  }
  
  if (!result && eventStatus  == nsEventStatus_eConsumeNoDefault)
    goto cleanup;

  HRESULT hr = glpDDSecondary->Lock(0, &gDDSDSecondary, DDLOCK_WAITNOTBUSY | DDLOCK_DISCARD, 0); 
  if (FAILED(hr))
    goto cleanup;

  pixman_image_t *srcPixmanImage = 
    pixman_image_create_bits(PIXMAN_x8r8g8b8, surfaceSize.width,
                             surfaceSize.height, 
                             (uint32_t*) sSharedSurfaceData.get(),
                             surfaceSize.width * 4);
  
  pixman_image_t *dstPixmanImage = 
    pixman_image_create_bits(PIXMAN_r5g6b5, gDDSDSecondary.dwWidth,
                             gDDSDSecondary.dwHeight,
                             (uint32_t*) gDDSDSecondary.lpSurface,
                             gDDSDSecondary.dwWidth * 2);
  

  const nsIntRect* r;
  for (nsIntRegionRectIterator iter(paintRgn);
       (r = iter.Next()) != nsnull;) {
    pixman_image_composite(PIXMAN_OP_SRC, srcPixmanImage, NULL, dstPixmanImage,
                           r->x - brx, r->y - bry,
                           0, 0,
                           r->x, r->y,
                           r->width, r->height);
  }
  
  pixman_image_unref(dstPixmanImage);
  pixman_image_unref(srcPixmanImage);

  hr = glpDDSecondary->Unlock(0);
  if (FAILED(hr))
    goto cleanup;
  
  hr = glpDDClipper->SetHWnd(0, mWnd);
  if (FAILED(hr))
    goto cleanup;
  
  for (nsIntRegionRectIterator iter(paintRgn);
       (r = iter.Next()) != nsnull;) {
    RECT wr = { r->x, r->y, r->XMost(), r->YMost() };
    RECT renderRect = wr;
    SetLastError(0); 
    MapWindowPoints(mWnd, 0, (LPPOINT)&renderRect, 2);
    hr = glpDDPrimary->Blt(&renderRect, glpDDSecondary, &wr, 0, NULL);
    if (FAILED(hr)) {
      NS_ERROR("this blt should never fail!");
      printf("#### %s blt failed: %08lx", __FUNCTION__, hr);
    }
  }
  result = PR_TRUE;

cleanup:
  NS_ASSERTION(result == PR_TRUE, "fatal drawing error");
  ::EndPaint(mWnd, &ps);
  mPaintDC = nsnull;
  mPainting = PR_FALSE;
  return result;

}
#endif 
