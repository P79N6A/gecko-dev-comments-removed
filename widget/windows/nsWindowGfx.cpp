


















#include "mozilla/dom/ContentParent.h"
#include "mozilla/plugins/PluginInstanceParent.h"
using mozilla::plugins::PluginInstanceParent;

#include "nsWindowGfx.h"
#include "nsAppRunner.h"
#include <windows.h>
#include "gfxImageSurface.h"
#include "gfxUtils.h"
#include "gfxWindowsSurface.h"
#include "gfxWindowsPlatform.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/DataSurfaceHelpers.h"
#include "mozilla/gfx/Tools.h"
#include "mozilla/RefPtr.h"
#include "nsGfxCIID.h"
#include "gfxContext.h"
#include "prmem.h"
#include "WinUtils.h"
#include "nsIWidgetListener.h"
#include "mozilla/unused.h"
#include "nsDebug.h"
#include "nsIXULRuntime.h"

#include "mozilla/layers/CompositorParent.h"
#include "ClientLayerManager.h"

#include "nsUXThemeData.h"
#include "nsUXThemeConstants.h"

extern "C" {
#define PIXMAN_DONT_DEFINE_STDINT
#include "pixman.h"
}

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;
using namespace mozilla::widget;
using namespace mozilla::plugins;

















static nsAutoPtr<uint8_t>  sSharedSurfaceData;
static gfxIntSize          sSharedSurfaceSize;

struct IconMetrics {
  int32_t xMetric;
  int32_t yMetric;
  int32_t defaultSize;
};


static IconMetrics sIconMetrics[] = {
  {SM_CXSMICON, SM_CYSMICON, 16}, 
  {SM_CXICON,   SM_CYICON,   32}  
};











 bool
nsWindow::IsRenderMode(gfxWindowsPlatform::RenderMode rmode)
{
  return gfxWindowsPlatform::GetPlatform()->GetRenderMode() == rmode;
}












nsIntRegion nsWindow::GetRegionToPaint(bool aForceFullRepaint,
                                       PAINTSTRUCT ps, HDC aDC)
{
  if (aForceFullRepaint) {
    RECT paintRect;
    ::GetClientRect(mWnd, &paintRect);
    return nsIntRegion(WinUtils::ToIntRect(paintRect));
  }

  HRGN paintRgn = ::CreateRectRgn(0, 0, 0, 0);
  if (paintRgn != nullptr) {
    int result = GetRandomRgn(aDC, paintRgn, SYSRGN);
    if (result == 1) {
      POINT pt = {0,0};
      ::MapWindowPoints(nullptr, mWnd, &pt, 1);
      ::OffsetRgn(paintRgn, pt.x, pt.y);
    }
    nsIntRegion rgn(WinUtils::ConvertHRGNToRegion(paintRgn));
    ::DeleteObject(paintRgn);
    return rgn;
  }
  return nsIntRegion(WinUtils::ToIntRect(ps.rcPaint));
}

#define WORDSSIZE(x) ((x).width * (x).height)
static bool
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
    sSharedSurfaceData = nullptr;
    sSharedSurfaceData = (uint8_t *)malloc(WORDSSIZE(sSharedSurfaceSize) * 4);
  }

  return (sSharedSurfaceData != nullptr);
}

nsIWidgetListener* nsWindow::GetPaintListener()
{
  if (mDestroyCalled)
    return nullptr;
  return mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;
}

bool nsWindow::OnPaint(HDC aDC, uint32_t aNestingLevel)
{
  
  
  
  
  if (mozilla::ipc::MessageChannel::IsSpinLoopActive() && mPainting)
    return false;

  if (gfxWindowsPlatform::GetPlatform()->DidRenderingDeviceReset()) {
    gfxWindowsPlatform::GetPlatform()->UpdateRenderMode();
    EnumAllWindows(ClearCompositor);
    return false;
  }

  
  
  
  
  if (IsPlugin()) {
    RECT updateRect;
    if (!GetUpdateRect(mWnd, &updateRect, FALSE) ||
        (updateRect.left == updateRect.right &&
         updateRect.top == updateRect.bottom)) {
      PAINTSTRUCT ps;
      BeginPaint(mWnd, &ps);
      EndPaint(mWnd, &ps);
      return true;
    }

    if (mWindowType == eWindowType_plugin_ipc_chrome) {
      
      mozilla::dom::ContentParent::SendAsyncUpdate(this);
      ValidateRect(mWnd, nullptr);
      return true;
    }

    PluginInstanceParent* instance = reinterpret_cast<PluginInstanceParent*>(
      ::GetPropW(mWnd, L"PluginInstanceParentProperty"));
    if (instance) {
      unused << instance->CallUpdateWindow();
    } else {
      
      
      
      NS_WARNING("Plugin failed to subclass our window");
    }

    ValidateRect(mWnd, nullptr);
    return true;
  }

  ClientLayerManager *clientLayerManager =
      (GetLayerManager()->GetBackendType() == LayersBackend::LAYERS_CLIENT)
      ? static_cast<ClientLayerManager*>(GetLayerManager())
      : nullptr;

  if (clientLayerManager && mCompositorParent &&
      !mBounds.IsEqualEdges(mLastPaintBounds))
  {
    
    
    mCompositorParent->ScheduleRenderOnCompositorThread();
  }
  mLastPaintBounds = mBounds;

  PAINTSTRUCT ps;

#ifdef MOZ_XUL
  if (!aDC && (eTransparencyTransparent == mTransparencyMode))
  {
    
    
    
    
    
    
    ::BeginPaint(mWnd, &ps);
    ::EndPaint(mWnd, &ps);

    aDC = mMemoryDC;
  }
#endif

  mPainting = true;

#ifdef WIDGET_DEBUG_OUTPUT
  HRGN debugPaintFlashRegion = nullptr;
  HDC debugPaintFlashDC = nullptr;

  if (debug_WantPaintFlashing())
  {
    debugPaintFlashRegion = ::CreateRectRgn(0, 0, 0, 0);
    ::GetUpdateRgn(mWnd, debugPaintFlashRegion, TRUE);
    debugPaintFlashDC = ::GetDC(mWnd);
  }
#endif 

  HDC hDC = aDC ? aDC : (::BeginPaint(mWnd, &ps));
  mPaintDC = hDC;

#ifdef MOZ_XUL
  bool forceRepaint = aDC || (eTransparencyTransparent == mTransparencyMode);
#else
  bool forceRepaint = nullptr != aDC;
#endif
  nsIntRegion region = GetRegionToPaint(forceRepaint, ps, hDC);

  if (clientLayerManager && mCompositorParent) {
    
    
    clientLayerManager->SetNeedsComposite(true);
    clientLayerManager->SendInvalidRegion(region);
  }

  nsIWidgetListener* listener = GetPaintListener();
  if (listener) {
    listener->WillPaintWindow(this);
  }
  
  listener = GetPaintListener();
  if (!listener) {
    return false;
  }

  if (clientLayerManager && mCompositorParent && clientLayerManager->NeedsComposite()) {
    mCompositorParent->ScheduleRenderOnCompositorThread();
    clientLayerManager->SetNeedsComposite(false);
  }

  bool result = true;
  if (!region.IsEmpty() && listener)
  {
    
    

#ifdef WIDGET_DEBUG_OUTPUT
    debug_DumpPaintEvent(stdout,
                         this,
                         region,
                         nsAutoCString("noname"),
                         (int32_t) mWnd);
#endif 

    switch (GetLayerManager()->GetBackendType()) {
      case LayersBackend::LAYERS_BASIC:
        {
          nsRefPtr<gfxASurface> targetSurface;

#if defined(MOZ_XUL)
          
          if ((IsRenderMode(gfxWindowsPlatform::RENDER_GDI) ||
               IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D)) &&
              eTransparencyTransparent == mTransparencyMode) {
            if (mTransparentSurface == nullptr)
              SetupTranslucentWindowMemoryBitmap(mTransparencyMode);
            targetSurface = mTransparentSurface;
          }
#endif

          nsRefPtr<gfxWindowsSurface> targetSurfaceWin;
          if (!targetSurface &&
              (IsRenderMode(gfxWindowsPlatform::RENDER_GDI) ||
               IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D)))
          {
            uint32_t flags = (mTransparencyMode == eTransparencyOpaque) ? 0 :
                gfxWindowsSurface::FLAG_IS_TRANSPARENT;
            targetSurfaceWin = new gfxWindowsSurface(hDC, flags);
            targetSurface = targetSurfaceWin;
          }

          nsRefPtr<gfxImageSurface> targetSurfaceImage;
          if (!targetSurface &&
              (IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH32) ||
               IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH24)))
          {
            gfxIntSize surfaceSize(ps.rcPaint.right - ps.rcPaint.left,
                                   ps.rcPaint.bottom - ps.rcPaint.top);

            if (!EnsureSharedSurfaceSize(surfaceSize)) {
              NS_ERROR("Couldn't allocate a shared image surface!");
              return false;
            }

            
            
            targetSurfaceImage = new gfxImageSurface(sSharedSurfaceData.get(),
                                                     surfaceSize,
                                                     surfaceSize.width * 4,
                                                     gfxImageFormat::RGB24);

            if (targetSurfaceImage && !targetSurfaceImage->CairoStatus()) {
              targetSurfaceImage->SetDeviceOffset(gfxPoint(-ps.rcPaint.left, -ps.rcPaint.top));
              targetSurface = targetSurfaceImage;
            }
          }

          if (!targetSurface) {
            NS_ERROR("Invalid RenderMode!");
            return false;
          }

          RECT paintRect;
          ::GetClientRect(mWnd, &paintRect);
          RefPtr<DrawTarget> dt =
            gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(targetSurface,
                                                                   IntSize(paintRect.right - paintRect.left,
                                                                   paintRect.bottom - paintRect.top));
          if (!dt) {
            gfxWarning() << "nsWindow::OnPaint failed in CreateDrawTargetForSurface";
            return false;
          }

          
          BufferMode doubleBuffering = mozilla::layers::BufferMode::BUFFER_NONE;
          if (IsRenderMode(gfxWindowsPlatform::RENDER_GDI) ||
              IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D)) {
#ifdef MOZ_XUL
            switch (mTransparencyMode) {
              case eTransparencyGlass:
              case eTransparencyBorderlessGlass:
              default:
                
                doubleBuffering = mozilla::layers::BufferMode::BUFFERED;
                break;
              case eTransparencyTransparent:
                
                
                dt->ClearRect(Rect(0.f, 0.f,
                                   dt->GetSize().width, dt->GetSize().height));
                break;
            }
#else
            doubleBuffering = mozilla::layers::BufferMode::BUFFERED;
#endif
          }

          nsRefPtr<gfxContext> thebesContext = new gfxContext(dt);

          {
            AutoLayerManagerSetup
                setupLayerManager(this, thebesContext, doubleBuffering);
            result = listener->PaintWindow(this, region);
          }

#ifdef MOZ_XUL
          if ((IsRenderMode(gfxWindowsPlatform::RENDER_GDI) ||
               IsRenderMode(gfxWindowsPlatform::RENDER_DIRECT2D))&&
              eTransparencyTransparent == mTransparencyMode) {
            
            
            
            UpdateTranslucentWindow();
          } else
#endif

          if (result) {
            if (IsRenderMode(gfxWindowsPlatform::RENDER_IMAGE_STRETCH24) ||
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
      case LayersBackend::LAYERS_CLIENT:
        result = listener->PaintWindow(this, region);
        break;
      default:
        NS_ERROR("Unknown layers backend used!");
        break;
    }
  }

  if (!aDC) {
    ::EndPaint(mWnd, &ps);
  }

  mPaintDC = nullptr;
  mLastPaintEndTime = TimeStamp::Now();

#if defined(WIDGET_DEBUG_OUTPUT)
  if (debug_WantPaintFlashing())
  {
    
    
    
    if (result) {
      ::InvertRgn(debugPaintFlashDC, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));
      ::InvertRgn(debugPaintFlashDC, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));
    }
    ::ReleaseDC(mWnd, debugPaintFlashDC);
    ::DeleteObject(debugPaintFlashRegion);
  }
#endif 

  mPainting = false;

  
  listener = GetPaintListener();
  if (listener)
    listener->DidPaintWindow();

  if (aNestingLevel == 0 && ::GetUpdateRect(mWnd, nullptr, false)) {
    OnPaint(aDC, 1);
  }

  return result;
}

gfxIntSize nsWindowGfx::GetIconMetrics(IconSizeType aSizeType) {
  int32_t width = ::GetSystemMetrics(sIconMetrics[aSizeType].xMetric);
  int32_t height = ::GetSystemMetrics(sIconMetrics[aSizeType].yMetric);

  if (width == 0 || height == 0) {
    width = height = sIconMetrics[aSizeType].defaultSize;
  }

  return gfxIntSize(width, height);
}

nsresult nsWindowGfx::CreateIcon(imgIContainer *aContainer,
                                  bool aIsCursor,
                                  uint32_t aHotspotX,
                                  uint32_t aHotspotY,
                                  gfxIntSize aScaledSize,
                                  HICON *aIcon) {

  MOZ_ASSERT((aScaledSize.width > 0 && aScaledSize.height > 0) ||
             (aScaledSize.width == 0 && aScaledSize.height == 0));

  
  RefPtr<SourceSurface> surface =
    aContainer->GetFrame(imgIContainer::FRAME_CURRENT,
                         imgIContainer::FLAG_SYNC_DECODE);
  NS_ENSURE_TRUE(surface, NS_ERROR_NOT_AVAILABLE);

  IntSize frameSize = surface->GetSize();
  if (frameSize.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }

  IntSize iconSize(aScaledSize.width, aScaledSize.height);
  if (iconSize == IntSize(0, 0)) { 
    iconSize = frameSize;
  }

  RefPtr<DataSourceSurface> dataSurface;
  bool mappedOK;
  DataSourceSurface::MappedSurface map;

  if (iconSize != frameSize) {
    
    dataSurface = Factory::CreateDataSourceSurface(iconSize,
                                                   SurfaceFormat::B8G8R8A8);
    NS_ENSURE_TRUE(dataSurface, NS_ERROR_FAILURE);
    mappedOK = dataSurface->Map(DataSourceSurface::MapType::READ_WRITE, &map);
    NS_ENSURE_TRUE(mappedOK, NS_ERROR_FAILURE);

    RefPtr<DrawTarget> dt =
      Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                       map.mData,
                                       dataSurface->GetSize(),
                                       map.mStride,
                                       SurfaceFormat::B8G8R8A8);
    if (!dt) {
      gfxWarning() << "nsWindowGfx::CreatesIcon failed in CreateDrawTargetForData";
      return NS_ERROR_OUT_OF_MEMORY;
    }
    dt->DrawSurface(surface,
                    Rect(0, 0, iconSize.width, iconSize.height),
                    Rect(0, 0, frameSize.width, frameSize.height),
                    DrawSurfaceOptions(),
                    DrawOptions(1.0f, CompositionOp::OP_SOURCE));
  } else if (surface->GetFormat() != SurfaceFormat::B8G8R8A8) {
    
    dataSurface = gfxUtils::
      CopySurfaceToDataSourceSurfaceWithFormat(surface,
                                               SurfaceFormat::B8G8R8A8);
    NS_ENSURE_TRUE(dataSurface, NS_ERROR_FAILURE);
    mappedOK = dataSurface->Map(DataSourceSurface::MapType::READ, &map);
  } else {
    dataSurface = surface->GetDataSurface();
    NS_ENSURE_TRUE(dataSurface, NS_ERROR_FAILURE);
    mappedOK = dataSurface->Map(DataSourceSurface::MapType::READ, &map);
  }
  NS_ENSURE_TRUE(dataSurface && mappedOK, NS_ERROR_FAILURE);
  MOZ_ASSERT(dataSurface->GetFormat() == SurfaceFormat::B8G8R8A8);

  uint8_t* data = nullptr;
  nsAutoArrayPtr<uint8_t> autoDeleteArray;
  if (map.mStride == BytesPerPixel(dataSurface->GetFormat()) * iconSize.width) {
    
    data = map.mData;
  } else {
    
    
    
    
    
    dataSurface->Unmap();
    map.mData = nullptr;

    data = autoDeleteArray = SurfaceToPackedBGRA(dataSurface);
    NS_ENSURE_TRUE(data, NS_ERROR_FAILURE);
  }

  HBITMAP bmp = DataToBitmap(data, iconSize.width, -iconSize.height, 32);
  uint8_t* a1data = Data32BitTo1Bit(data, iconSize.width, iconSize.height);
  if (map.mData) {
    dataSurface->Unmap();
  }
  if (!a1data) {
    return NS_ERROR_FAILURE;
  }

  HBITMAP mbmp = DataToBitmap(a1data, iconSize.width, -iconSize.height, 1);
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


uint8_t* nsWindowGfx::Data32BitTo1Bit(uint8_t* aImageData,
                                      uint32_t aWidth, uint32_t aHeight)
{
  
  
  uint32_t outBpr = ((aWidth + 31) / 8) & ~3;

  
  uint8_t* outData = (uint8_t*)PR_Calloc(outBpr, aHeight);
  if (!outData)
    return nullptr;

  int32_t *imageRow = (int32_t*)aImageData;
  for (uint32_t curRow = 0; curRow < aHeight; curRow++) {
    uint8_t *outRow = outData + curRow * outBpr;
    uint8_t mask = 0x80;
    for (uint32_t curCol = 0; curCol < aWidth; curCol++) {
      
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















HBITMAP nsWindowGfx::DataToBitmap(uint8_t* aImageData,
                                  uint32_t aWidth,
                                  uint32_t aHeight,
                                  uint32_t aDepth)
{
  HDC dc = ::GetDC(nullptr);

  if (aDepth == 32) {
    
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
    ::ReleaseDC(nullptr, dc);
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
  ::ReleaseDC(nullptr, dc);
  return bmp;
}
