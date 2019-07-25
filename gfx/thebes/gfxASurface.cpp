





































#include "nsIMemoryReporter.h"
#include "nsMemory.h"
#include "CheckedInt.h"

#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"

#include "nsRect.h"

#include "cairo.h"

#ifdef CAIRO_HAS_WIN32_SURFACE
#include "gfxWindowsSurface.h"
#endif
#ifdef CAIRO_HAS_D2D_SURFACE
#include "gfxD2DSurface.h"
#endif

#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#endif

#ifdef CAIRO_HAS_QUARTZ_SURFACE
#include "gfxQuartzSurface.h"
#include "gfxQuartzImageSurface.h"
#endif

#ifdef MOZ_DFB
#include "gfxDirectFBSurface.h"
#endif

#if defined(CAIRO_HAS_QT_SURFACE) && defined(MOZ_WIDGET_QT)
#include "gfxQPainterSurface.h"
#endif

#include <stdio.h>
#include <limits.h>

#include "imgIEncoder.h"
#include "nsComponentManagerUtils.h"
#include "gfxContext.h"
#include "prmem.h"
#include "nsISupportsUtils.h"
#include "plbase64.h"
#include "nsCOMPtr.h"
#include "nsIConsoleService.h"
#include "nsServiceManagerUtils.h"

using mozilla::CheckedInt;

static cairo_user_data_key_t gfxasurface_pointer_key;



nsrefcnt
gfxASurface::AddRef(void)
{
    if (mSurfaceValid) {
        if (mFloatingRefs) {
            
            mFloatingRefs--;
        } else {
            cairo_surface_reference(mSurface);
        }

        return (nsrefcnt) cairo_surface_get_reference_count(mSurface);
    } else {
        
        
        return ++mFloatingRefs;
    }
}

nsrefcnt
gfxASurface::Release(void)
{
    if (mSurfaceValid) {
        NS_ASSERTION(mFloatingRefs == 0, "gfxASurface::Release with floating refs still hanging around!");

        
        
        
        nsrefcnt refcnt = (nsrefcnt) cairo_surface_get_reference_count(mSurface);
        cairo_surface_destroy(mSurface);

        

        return --refcnt;
    } else {
        if (--mFloatingRefs == 0) {
            delete this;
            return 0;
        }

        return mFloatingRefs;
    }
}

void
gfxASurface::SurfaceDestroyFunc(void *data) {
    gfxASurface *surf = (gfxASurface*) data;
    
    delete surf;
}

gfxASurface*
gfxASurface::GetSurfaceWrapper(cairo_surface_t *csurf)
{
    return (gfxASurface*) cairo_surface_get_user_data(csurf, &gfxasurface_pointer_key);
}

void
gfxASurface::SetSurfaceWrapper(cairo_surface_t *csurf, gfxASurface *asurf)
{
    cairo_surface_set_user_data(csurf, &gfxasurface_pointer_key, asurf, SurfaceDestroyFunc);
}

already_AddRefed<gfxASurface>
gfxASurface::Wrap (cairo_surface_t *csurf)
{
    gfxASurface *result;

    
    result = GetSurfaceWrapper(csurf);
    if (result) {
        
        NS_ADDREF(result);
        return result;
    }

    
    cairo_surface_type_t stype = cairo_surface_get_type(csurf);

    if (stype == CAIRO_SURFACE_TYPE_IMAGE) {
        result = new gfxImageSurface(csurf);
    }
#ifdef CAIRO_HAS_WIN32_SURFACE
    else if (stype == CAIRO_SURFACE_TYPE_WIN32 ||
             stype == CAIRO_SURFACE_TYPE_WIN32_PRINTING) {
        result = new gfxWindowsSurface(csurf);
    }
#endif
#ifdef CAIRO_HAS_D2D_SURFACE
    else if (stype == CAIRO_SURFACE_TYPE_D2D) {
        result = new gfxD2DSurface(csurf);
    }
#endif
#ifdef MOZ_X11
    else if (stype == CAIRO_SURFACE_TYPE_XLIB) {
        result = new gfxXlibSurface(csurf);
    }
#endif
#ifdef CAIRO_HAS_QUARTZ_SURFACE
    else if (stype == CAIRO_SURFACE_TYPE_QUARTZ) {
        result = new gfxQuartzSurface(csurf);
    }
    else if (stype == CAIRO_SURFACE_TYPE_QUARTZ_IMAGE) {
        result = new gfxQuartzImageSurface(csurf);
    }
#endif
#ifdef MOZ_DFB
    else if (stype == CAIRO_SURFACE_TYPE_DIRECTFB) {
        result = new gfxDirectFBSurface(csurf);
    }
#endif
#if defined(CAIRO_HAS_QT_SURFACE) && defined(MOZ_WIDGET_QT)
    else if (stype == CAIRO_SURFACE_TYPE_QT) {
        result = new gfxQPainterSurface(csurf);
    }
#endif
    else {
        result = new gfxUnknownSurface(csurf);
    }

    

    NS_ADDREF(result);
    return result;
}

void
gfxASurface::Init(cairo_surface_t* surface, PRBool existingSurface)
{
    if (cairo_surface_status(surface)) {
        
        mSurfaceValid = PR_FALSE;
        cairo_surface_destroy(surface);
        return;
    }

    SetSurfaceWrapper(surface, this);

    mSurface = surface;
    mSurfaceValid = PR_TRUE;

    if (existingSurface) {
        mFloatingRefs = 0;
    } else {
        mFloatingRefs = 1;
#ifdef MOZ_TREE_CAIRO
        if (cairo_surface_get_content(surface) != CAIRO_CONTENT_COLOR) {
            cairo_surface_set_subpixel_antialiasing(surface, CAIRO_SUBPIXEL_ANTIALIASING_DISABLED);
        }
#endif
    }
}

gfxASurface::gfxSurfaceType
gfxASurface::GetType() const
{
    if (!mSurfaceValid)
        return (gfxSurfaceType)-1;

    return (gfxSurfaceType)cairo_surface_get_type(mSurface);
}

gfxASurface::gfxContentType
gfxASurface::GetContentType() const
{
    if (!mSurfaceValid)
        return (gfxContentType)-1;

    return (gfxContentType)cairo_surface_get_content(mSurface);
}

void
gfxASurface::SetDeviceOffset(const gfxPoint& offset)
{
    cairo_surface_set_device_offset(mSurface,
                                    offset.x, offset.y);
}

gfxPoint
gfxASurface::GetDeviceOffset() const
{
    gfxPoint pt;
    cairo_surface_get_device_offset(mSurface, &pt.x, &pt.y);
    return pt;
}

void
gfxASurface::Flush() const
{
    cairo_surface_flush(mSurface);
}

void
gfxASurface::MarkDirty()
{
    cairo_surface_mark_dirty(mSurface);
}

void
gfxASurface::MarkDirty(const gfxRect& r)
{
    cairo_surface_mark_dirty_rectangle(mSurface,
                                       (int) r.X(), (int) r.Y(),
                                       (int) r.Width(), (int) r.Height());
}

void
gfxASurface::SetData(const cairo_user_data_key_t *key,
                     void *user_data,
                     thebes_destroy_func_t destroy)
{
    cairo_surface_set_user_data(mSurface, key, user_data, destroy);
}

void *
gfxASurface::GetData(const cairo_user_data_key_t *key)
{
    return cairo_surface_get_user_data(mSurface, key);
}

void
gfxASurface::Finish()
{
    cairo_surface_finish(mSurface);
}

already_AddRefed<gfxASurface>
gfxASurface::CreateSimilarSurface(gfxContentType aContent,
                                  const gfxIntSize& aSize)
{
    if (!mSurface || !mSurfaceValid) {
      return nsnull;
    }
    
    cairo_surface_t *surface =
        cairo_surface_create_similar(mSurface, cairo_content_t(aContent),
                                     aSize.width, aSize.height);
    if (cairo_surface_status(surface)) {
        cairo_surface_destroy(surface);
        return nsnull;
    }

    nsRefPtr<gfxASurface> result = Wrap(surface);
    cairo_surface_destroy(surface);
    return result.forget();
}

int
gfxASurface::CairoStatus()
{
    if (!mSurfaceValid)
        return -1;

    return cairo_surface_status(mSurface);
}


PRBool
gfxASurface::CheckSurfaceSize(const gfxIntSize& sz, PRInt32 limit)
{
    if (sz.width < 0 || sz.height < 0) {
        NS_WARNING("Surface width or height < 0!");
        return PR_FALSE;
    }

    
    if (limit && (sz.width > limit || sz.height > limit)) {
        NS_WARNING("Surface size too large (exceeds caller's limit)!");
        return PR_FALSE;
    }

#if defined(XP_MACOSX)
    
    
    if (sz.height > SHRT_MAX) {
        NS_WARNING("Surface size too large (exceeds CoreGraphics limit)!");
        return PR_FALSE;
    }
#endif

    
    CheckedInt<PRInt32> tmp = sz.width;
    tmp *= sz.height;
    if (!tmp.valid()) {
        NS_WARNING("Surface size too large (would overflow)!");
        return PR_FALSE;
    }

    
    
    tmp *= 4;
    if (!tmp.valid()) {
        NS_WARNING("Allocation too large (would overflow)!");
        return PR_FALSE;
    }

    return PR_TRUE;
}


PRInt32
gfxASurface::FormatStrideForWidth(gfxImageFormat format, PRInt32 width)
{
    return cairo_format_stride_for_width((cairo_format_t)format, (int)width);
}

nsresult
gfxASurface::BeginPrinting(const nsAString& aTitle, const nsAString& aPrintToFileName)
{
    return NS_OK;
}

nsresult
gfxASurface::EndPrinting()
{
    return NS_OK;
}

nsresult
gfxASurface::AbortPrinting()
{
    return NS_OK;
}

nsresult
gfxASurface::BeginPage()
{
    return NS_OK;
}

nsresult
gfxASurface::EndPage()
{
    return NS_OK;
}

gfxASurface::gfxContentType
gfxASurface::ContentFromFormat(gfxImageFormat format)
{
    switch (format) {
        case ImageFormatARGB32:
            return CONTENT_COLOR_ALPHA;
        case ImageFormatRGB24:
        case ImageFormatRGB16_565:
            return CONTENT_COLOR;
        case ImageFormatA8:
        case ImageFormatA1:
            return CONTENT_ALPHA;

        case ImageFormatUnknown:
        default:
            return CONTENT_COLOR;
    }
}

gfxASurface::gfxImageFormat
gfxASurface::FormatFromContent(gfxASurface::gfxContentType type)
{
    switch (type) {
        case CONTENT_COLOR_ALPHA:
            return ImageFormatARGB32;
        case CONTENT_ALPHA:
            return ImageFormatA8;
        case CONTENT_COLOR:
        default:
            return ImageFormatRGB24;
    }
}

void
gfxASurface::SetSubpixelAntialiasingEnabled(PRBool aEnabled)
{
#ifdef MOZ_TREE_CAIRO
    if (!mSurfaceValid)
        return;
    cairo_surface_set_subpixel_antialiasing(mSurface,
        aEnabled ? CAIRO_SUBPIXEL_ANTIALIASING_ENABLED : CAIRO_SUBPIXEL_ANTIALIASING_DISABLED);
#endif
}

PRBool
gfxASurface::GetSubpixelAntialiasingEnabled()
{
    if (!mSurfaceValid)
      return PR_FALSE;
#ifdef MOZ_TREE_CAIRO
    return cairo_surface_get_subpixel_antialiasing(mSurface) == CAIRO_SUBPIXEL_ANTIALIASING_ENABLED;
#else
    return PR_TRUE;
#endif
}

PRInt32
gfxASurface::BytePerPixelFromFormat(gfxImageFormat format)
{
    switch (format) {
        case ImageFormatARGB32:
        case ImageFormatRGB24:
            return 4;
        case ImageFormatRGB16_565:
            return 2;
        case ImageFormatA8:
            return 1;
        default:
            NS_WARNING("Unknown byte per pixel value for Image format");
    }
    return 0;
}

void
gfxASurface::FastMovePixels(const nsIntRect& aSourceRect,
                            const nsIntPoint& aDestTopLeft)
{
    
    gfxIntSize size = GetSize();
    nsIntRect dest(aDestTopLeft, aSourceRect.Size());
    
    nsRefPtr<gfxContext> ctx = new gfxContext(this);
    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    nsIntPoint srcOrigin = dest.TopLeft() - aSourceRect.TopLeft();
    ctx->SetSource(this, gfxPoint(srcOrigin.x, srcOrigin.y));
    ctx->Rectangle(gfxRect(dest.x, dest.y, dest.width, dest.height));
    ctx->Fill();
}

void
gfxASurface::MovePixels(const nsIntRect& aSourceRect,
                        const nsIntPoint& aDestTopLeft)
{
    
    
    nsRefPtr<gfxASurface> tmp = 
      CreateSimilarSurface(GetContentType(), 
                           gfxIntSize(aSourceRect.width, aSourceRect.height));
    nsRefPtr<gfxContext> ctx = new gfxContext(tmp);
    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->SetSource(this, gfxPoint(-aSourceRect.x, -aSourceRect.y));
    ctx->Paint();

    ctx = new gfxContext(this);
    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->SetSource(tmp, gfxPoint(aDestTopLeft.x, aDestTopLeft.y));
    ctx->Rectangle(gfxRect(aDestTopLeft.x, 
                           aDestTopLeft.y, 
                           aSourceRect.width, 
                           aSourceRect.height));
    ctx->Fill();
}



static const char *sSurfaceNamesForSurfaceType[] = {
    "gfx-surface-image",
    "gfx-surface-pdf",
    "gfx-surface-ps",
    "gfx-surface-xlib",
    "gfx-surface-xcb",
    "gfx-surface-glitz",
    "gfx-surface-quartz",
    "gfx-surface-win32",
    "gfx-surface-beos",
    "gfx-surface-directfb",
    "gfx-surface-svg",
    "gfx-surface-os2",
    "gfx-surface-win32printing",
    "gfx-surface-quartzimage",
    "gfx-surface-script",
    "gfx-surface-qpainter",
    "gfx-surface-recording",
    "gfx-surface-vg",
    "gfx-surface-gl",
    "gfx-surface-drm",
    "gfx-surface-tee",
    "gfx-surface-xml",
    "gfx-surface-skia",
    "gfx-surface-subsurface",
    "gfx-surface-d2d"
};

PR_STATIC_ASSERT(NS_ARRAY_LENGTH(sSurfaceNamesForSurfaceType) == gfxASurface::SurfaceTypeMax);
#ifdef CAIRO_HAS_D2D_SURFACE
PR_STATIC_ASSERT(PRUint32(CAIRO_SURFACE_TYPE_D2D) == PRUint32(gfxASurface::SurfaceTypeD2D));
#endif
PR_STATIC_ASSERT(PRUint32(CAIRO_SURFACE_TYPE_SKIA) == PRUint32(gfxASurface::SurfaceTypeSkia));

static const char *
SurfaceMemoryReporterPathForType(gfxASurface::gfxSurfaceType aType)
{
    if (aType < 0 ||
        aType >= gfxASurface::SurfaceTypeMax)
        return "gfx-surface-unknown";

    return sSurfaceNamesForSurfaceType[aType];
}


static nsIMemoryReporter *gSurfaceMemoryReporters[gfxASurface::SurfaceTypeMax] = { 0 };
static PRInt64 gSurfaceMemoryUsed[gfxASurface::SurfaceTypeMax] = { 0 };

class SurfaceMemoryReporter :
    public nsIMemoryReporter
{
public:
    SurfaceMemoryReporter(gfxASurface::gfxSurfaceType aType)
        : mType(aType)
    { }

    NS_DECL_ISUPPORTS

    NS_IMETHOD GetProcess(char **process) {
        *process = strdup("");
        return NS_OK;
    }

    NS_IMETHOD GetPath(char **memoryPath) {
        *memoryPath = strdup(SurfaceMemoryReporterPathForType(mType));
        return NS_OK;
    }

    NS_IMETHOD GetKind(PRInt32 *kind) {
        *kind = KIND_OTHER;
        return NS_OK;
    }
    
    NS_IMETHOD GetUnits(PRInt32 *units) {
        *units = UNITS_BYTES;
        return NS_OK;
    }

    NS_IMETHOD GetAmount(PRInt64 *amount) {
        *amount = gSurfaceMemoryUsed[mType];
        return NS_OK;
    }

    NS_IMETHOD GetDescription(char **desc) {
        *desc = strdup("Memory used by gfx surface of the given type.");
        return NS_OK;
    }

    gfxASurface::gfxSurfaceType mType;
};

NS_IMPL_ISUPPORTS1(SurfaceMemoryReporter, nsIMemoryReporter)

void
gfxASurface::RecordMemoryUsedForSurfaceType(gfxASurface::gfxSurfaceType aType,
                                            PRInt32 aBytes)
{
    if (aType < 0 || aType >= SurfaceTypeMax) {
        NS_WARNING("Invalid type to RecordMemoryUsedForSurfaceType!");
        return;
    }

    if (gSurfaceMemoryReporters[aType] == 0) {
        gSurfaceMemoryReporters[aType] = new SurfaceMemoryReporter(aType);
        NS_RegisterMemoryReporter(gSurfaceMemoryReporters[aType]);
    }

    gSurfaceMemoryUsed[aType] += aBytes;
}

void
gfxASurface::RecordMemoryUsed(PRInt32 aBytes)
{
    RecordMemoryUsedForSurfaceType(GetType(), aBytes);
    mBytesRecorded += aBytes;
}

void
gfxASurface::RecordMemoryFreed()
{
    if (mBytesRecorded) {
        RecordMemoryUsedForSurfaceType(GetType(), -mBytesRecorded);
        mBytesRecorded = 0;
    }
}

void
gfxASurface::DumpAsDataURL()
{
  gfxIntSize size = GetSize();
  if (size.width == -1 && size.height == -1) {
    printf("Could not determine surface size\n");
    return;
  }

  nsAutoArrayPtr<PRUint8> imageBuffer(new (std::nothrow) PRUint8[size.width * 
                                                                 size.height * 
                                                                 4]);
  if (!imageBuffer) {
    printf("Could not allocate image buffer\n");
    return;
  }
 
  nsRefPtr<gfxImageSurface> imgsurf = 
    new gfxImageSurface(imageBuffer.get(),
                        gfxIntSize(size.width, size.height),
                        size.width * 4,
                        gfxASurface::ImageFormatARGB32);

  if (!imgsurf || imgsurf->CairoStatus()) {
    printf("Could not allocate image surface\n");
    return;
  }

  nsRefPtr<gfxContext> ctx = new gfxContext(imgsurf);
  if (!ctx || ctx->HasError()) {
    printf("Could not allocate image context\n");
    return;
  }

  ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx->SetSource(this, gfxPoint(0, 0));
  ctx->Paint();

  nsCOMPtr<imgIEncoder> encoder =
    do_CreateInstance("@mozilla.org/image/encoder;2?type=image/png");
  if (!encoder) {
    PRInt32 w = NS_MIN(size.width, 8);
    PRInt32 h = NS_MIN(size.height, 8);
    printf("Could not create encoder. Printing %dx%d pixels.\n", w, h);
    for (PRInt32 y = 0; y < h; ++y) {
      for (PRInt32 x = 0; x < w; ++x) {
        printf("%x ", reinterpret_cast<PRUint32*>(imageBuffer.get())[y*size.width + x]);
      }
      printf("\n");
    }
    return;
  }

  nsresult rv = encoder->InitFromData(imageBuffer.get(),
                                      size.width * size.height * 4, 
                                      size.width, 
                                      size.height, 
                                      size.width * 4,
                                      imgIEncoder::INPUT_FORMAT_HOSTARGB,
                                      NS_LITERAL_STRING(""));
  if (NS_FAILED(rv))
    return;

  nsCOMPtr<nsIInputStream> imgStream;
  CallQueryInterface(encoder.get(), getter_AddRefs(imgStream));
  if (!imgStream)
    return;

  PRUint32 bufSize;
  rv = imgStream->Available(&bufSize);
  if (NS_FAILED(rv))
    return;

  
  
  bufSize += 16;
  PRUint32 imgSize = 0;
  char* imgData = (char*)PR_Malloc(bufSize);
  if (!imgData)
    return;
  PRUint32 numReadThisTime = 0;
  while ((rv = imgStream->Read(&imgData[imgSize], 
                               bufSize - imgSize,
                               &numReadThisTime)) == NS_OK && numReadThisTime > 0) 
  {
    imgSize += numReadThisTime;
    if (imgSize == bufSize) {
      
      bufSize *= 2;
      char* newImgData = (char*)PR_Realloc(imgData, bufSize);
      if (!newImgData) {
        PR_Free(imgData);
        return;
      }
      imgData = newImgData;
    }
  }
  
  
  char* encodedImg = PL_Base64Encode(imgData, imgSize, nsnull);
  PR_Free(imgData);
  if (!encodedImg) 
    return;
 
  printf("data:image/png;base64,");
  printf("%s", encodedImg);
  printf("\n");
  PR_Free(encodedImg);

  return;
}

