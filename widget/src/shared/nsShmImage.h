







































#ifndef __mozilla_widget_nsShmImage_h__
#define __mozilla_widget_nsShmImage_h__

#include "mozilla/ipc/SharedMemorySysV.h"

#if defined(MOZ_X11) && defined(MOZ_HAVE_SHAREDMEMORYSYSV)
#  define MOZ_HAVE_SHMIMAGE
#endif

#ifdef MOZ_HAVE_SHMIMAGE

#include "nsIWidget.h"
#include "gfxASurface.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#if defined(MOZ_WIDGET_GTK2)
#define DISPLAY gdk_x11_get_default_xdisplay
#elif defined(MOZ_WIDGET_QT)
#include "QX11Info"
#define DISPLAY QX11Info().display
#endif

class QRect;
class QWidget;

class nsShmImage {
    NS_INLINE_DECL_REFCOUNTING(nsShmImage)

    typedef mozilla::ipc::SharedMemorySysV SharedMemorySysV;

public:
    typedef gfxASurface::gfxImageFormat Format;

    static PRBool UseShm();
    static already_AddRefed<nsShmImage>
        Create(const gfxIntSize& aSize, Visual* aVisual, unsigned int aDepth);
    static already_AddRefed<gfxASurface>
        EnsureShmImage(const gfxIntSize& aSize, Visual* aVisual, unsigned int aDepth,
                       nsRefPtr<nsShmImage>& aImage);

    ~nsShmImage() {
        if (mImage) {
            XSync(DISPLAY(), False);
            if (mXAttached) {
                XShmDetach(DISPLAY(), &mInfo);
            }
            XDestroyImage(mImage);
        }
    }

    already_AddRefed<gfxASurface> AsSurface();

#if defined(MOZ_WIDGET_GTK2)
    void Put(GdkWindow* aWindow, GdkRectangle* aRects, GdkRectangle* aEnd);
#elif defined(MOZ_WIDGET_QT)
    void Put(QWidget* aWindow, QRect& aRect);
#endif

    gfxIntSize Size() const { return mSize; }

private:
    nsShmImage()
        : mImage(nsnull)
        , mXAttached(PR_FALSE)
    { mInfo.shmid = SharedMemorySysV::NULLHandle(); }

    nsRefPtr<SharedMemorySysV>   mSegment;
    XImage*                      mImage;
    XShmSegmentInfo              mInfo;
    gfxIntSize                   mSize;
    Format                       mFormat;
    PRPackedBool                 mXAttached;
};

#endif 

#endif
