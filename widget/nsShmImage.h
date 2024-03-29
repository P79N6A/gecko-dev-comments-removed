





#ifndef __mozilla_widget_nsShmImage_h__
#define __mozilla_widget_nsShmImage_h__

#include "mozilla/ipc/SharedMemorySysV.h"

#if defined(MOZ_X11) && defined(MOZ_HAVE_SHAREDMEMORYSYSV)
#  define MOZ_HAVE_SHMIMAGE
#endif

#ifdef MOZ_HAVE_SHMIMAGE

#include "nsIWidget.h"
#include "gfxTypes.h"
#include "nsAutoPtr.h"

#include "mozilla/X11Util.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#if defined(MOZ_WIDGET_GTK)
#define DISPLAY gdk_x11_get_default_xdisplay
#elif defined(MOZ_WIDGET_QT)
#define DISPLAY mozilla::DefaultXDisplay
#endif

class QRect;
class QWindow;
class gfxASurface;

class nsShmImage {
    
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nsShmImage)

    typedef mozilla::ipc::SharedMemorySysV SharedMemorySysV;

public:
    typedef gfxImageFormat Format;

    static bool UseShm();
    static already_AddRefed<nsShmImage>
        Create(const gfxIntSize& aSize, Visual* aVisual, unsigned int aDepth);
    static already_AddRefed<gfxASurface>
        EnsureShmImage(const gfxIntSize& aSize, Visual* aVisual, unsigned int aDepth,
                       nsRefPtr<nsShmImage>& aImage);

private:
    ~nsShmImage() {
        if (mImage) {
            mozilla::FinishX(DISPLAY());
            if (mXAttached) {
                XShmDetach(DISPLAY(), &mInfo);
            }
            XDestroyImage(mImage);
        }
    }

public:
    already_AddRefed<gfxASurface> AsSurface();

#ifdef MOZ_WIDGET_GTK
    void Put(GdkWindow* aWindow, const nsIntRegion& aRegion);
#elif defined(MOZ_WIDGET_QT)
    void Put(QWindow* aWindow, QRect& aRect);
#endif

    gfxIntSize Size() const { return mSize; }

private:
    nsShmImage()
        : mImage(nullptr)
        , mXAttached(false)
    { mInfo.shmid = SharedMemorySysV::NULLHandle(); }

    nsRefPtr<SharedMemorySysV>   mSegment;
    XImage*                      mImage;
    XShmSegmentInfo              mInfo;
    gfxIntSize                   mSize;
    Format                       mFormat;
    bool                         mXAttached;
};

#endif 

#endif
