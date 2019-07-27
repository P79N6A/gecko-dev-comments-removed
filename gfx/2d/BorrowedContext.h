




#ifndef _MOZILLA_GFX_BORROWED_CONTEXT_H
#define _MOZILLA_GFX_BORROWED_CONTEXT_H

#include "2D.h"

#ifdef MOZ_X11
#include <X11/extensions/Xrender.h>
#include <X11/Xlib.h>
#endif

struct _cairo;
typedef struct _cairo cairo_t;

namespace mozilla {

namespace gfx {







class BorrowedCairoContext
{
public:
  BorrowedCairoContext()
    : mCairo(nullptr)
    , mDT(nullptr)
  { }

  explicit BorrowedCairoContext(DrawTarget *aDT)
    : mDT(aDT)
  {
    mCairo = BorrowCairoContextFromDrawTarget(aDT);
  }

  
  
  
  cairo_t *Init(DrawTarget *aDT)
  {
    MOZ_ASSERT(!mDT, "Can't initialize twice!");
    mDT = aDT;
    return mCairo = BorrowCairoContextFromDrawTarget(aDT);
  }

  
  
  
  
  
  void Finish()
  {
    if (mCairo) {
      ReturnCairoContextToDrawTarget(mDT, mCairo);
      mCairo = nullptr;
    }
  }

  ~BorrowedCairoContext() {
    MOZ_ASSERT(!mCairo);
  }

  cairo_t *mCairo;
private:
  static cairo_t* BorrowCairoContextFromDrawTarget(DrawTarget *aDT);
  static void ReturnCairoContextToDrawTarget(DrawTarget *aDT, cairo_t *aCairo);
  DrawTarget *mDT;
};

#ifdef MOZ_X11






class BorrowedXlibDrawable
{
public:
  BorrowedXlibDrawable()
    : mDT(nullptr),
      mDisplay(nullptr),
      mDrawable(None),
      mScreen(nullptr),
      mVisual(nullptr),
      mXRenderFormat(nullptr)
  {}

  explicit BorrowedXlibDrawable(DrawTarget *aDT)
    : mDT(nullptr),
      mDisplay(nullptr),
      mDrawable(None),
      mScreen(nullptr),
      mVisual(nullptr),
      mXRenderFormat(nullptr)
  {
    Init(aDT);
  }

  
  
  
  bool Init(DrawTarget *aDT);

  
  
  
  
  
  void Finish();

  ~BorrowedXlibDrawable() {
    MOZ_ASSERT(!mDrawable);
  }

  Display *GetDisplay() const { return mDisplay; }
  Drawable GetDrawable() const { return mDrawable; }
  Screen *GetScreen() const { return mScreen; }
  Visual *GetVisual() const { return mVisual; }

  XRenderPictFormat* GetXRenderFormat() const { return mXRenderFormat; }

private:
  DrawTarget *mDT;
  Display *mDisplay;
  Drawable mDrawable;
  Screen *mScreen;
  Visual *mVisual;
  XRenderPictFormat *mXRenderFormat;
};
#endif

#ifdef XP_MACOSX






class BorrowedCGContext
{
public:
  BorrowedCGContext()
    : cg(nullptr)
    , mDT(nullptr)
  { }

  explicit BorrowedCGContext(DrawTarget *aDT)
    : mDT(aDT)
  {
    MOZ_ASSERT(aDT, "Caller should check for nullptr");
    cg = BorrowCGContextFromDrawTarget(aDT);
  }

  
  
  
  CGContextRef Init(DrawTarget *aDT)
  {
    MOZ_ASSERT(aDT, "Caller should check for nullptr");
    MOZ_ASSERT(!mDT, "Can't initialize twice!");
    mDT = aDT;
    cg = BorrowCGContextFromDrawTarget(aDT);
    return cg;
  }

  
  
  
  
  
  void Finish()
  {
    if (cg) {
      ReturnCGContextToDrawTarget(mDT, cg);
      cg = nullptr;
    }
  }

  ~BorrowedCGContext() {
    MOZ_ASSERT(!cg);
  }

  CGContextRef cg;
private:
  static CGContextRef BorrowCGContextFromDrawTarget(DrawTarget *aDT);
  static void ReturnCGContextToDrawTarget(DrawTarget *aDT, CGContextRef cg);
  DrawTarget *mDT;
};
#endif

} 
} 

#endif 
