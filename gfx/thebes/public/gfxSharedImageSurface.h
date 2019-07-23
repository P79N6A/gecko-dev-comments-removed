




































#ifndef GFX_SHARED_IMAGESURFACE_H
#define GFX_SHARED_IMAGESURFACE_H

#ifdef MOZ_X11
#ifdef HAVE_XSHM

#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxPoint.h"

#include <stdlib.h>
#include <string.h>

#ifdef MOZ_WIDGET_QT
#include <QX11Info>
#endif

#ifdef MOZ_WIDGET_GTK2
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#endif

#ifdef MOZ_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#endif

#include <sys/ipc.h>
#include <sys/shm.h>

class THEBES_API gfxSharedImageSurface : public gfxASurface {
public:
  gfxSharedImageSurface();
  ~gfxSharedImageSurface();

  
  gfxImageFormat Format() const { return mFormat; }

  const gfxIntSize& GetSize() const { return mSize; }
  int Width() const { return mSize.width; }
  int Height() const { return mSize.height; }

  



  int Stride() const { return mStride; }

  



  unsigned char* Data() const { return mData; } 

  


  int GetDataSize() const { return mStride*mSize.height; }

  


  XImage *image() const { return mXShmImage; }

  


  already_AddRefed<gfxASurface> getASurface(void);

  










  bool Init(const gfxIntSize& aSize,
            gfxImageFormat aFormat = ImageFormatUnknown,
            int aDepth = 0,
            int aShmId = -1);

  


  int Depth() const { return mDepth; }

private:
  bool CreateInternal(int aShmid);
  long ComputeStride() const;
  inline bool ComputeDepth();
  inline bool ComputeFormat();

  unsigned int     mDepth;
  int              mShmId;

  gfxIntSize mSize;
  bool mOwnsData;

  unsigned char   *mData;
  gfxImageFormat   mFormat;
  long mStride;

  Display *mDisp;
  XShmSegmentInfo  mShmInfo;
  XImage          *mXShmImage;
};

#endif 
#endif 
#endif 
