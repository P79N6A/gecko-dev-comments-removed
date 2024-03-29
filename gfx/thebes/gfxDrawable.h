




#ifndef GFX_DRAWABLE_H
#define GFX_DRAWABLE_H

#include "nsAutoPtr.h"
#include "gfxRect.h"
#include "gfxMatrix.h"
#include "GraphicsFilter.h"
#include "mozilla/gfx/2D.h"

class gfxContext;
class gfxPattern;






class gfxDrawable {
    NS_INLINE_DECL_REFCOUNTING(gfxDrawable)
public:
    explicit gfxDrawable(const mozilla::gfx::IntSize aSize)
     : mSize(aSize) {}

    






    virtual bool Draw(gfxContext* aContext,
                        const gfxRect& aFillRect,
                        bool aRepeat,
                        const GraphicsFilter& aFilter,
                        gfxFloat aOpacity = 1.0,
                        const gfxMatrix& aTransform = gfxMatrix()) = 0;
    virtual bool DrawWithSamplingRect(gfxContext* aContext,
                                      const gfxRect& aFillRect,
                                      const gfxRect& aSamplingRect,
                                      bool aRepeat,
                                      const GraphicsFilter& aFilter,
                                      gfxFloat aOpacity = 1.0)
    {
        return false;
    }

    virtual mozilla::gfx::IntSize Size() { return mSize; }

protected:
    
    virtual ~gfxDrawable() {}

    const mozilla::gfx::IntSize mSize;
};





class gfxSurfaceDrawable : public gfxDrawable {
public:
    gfxSurfaceDrawable(mozilla::gfx::SourceSurface* aSurface, const mozilla::gfx::IntSize aSize,
                       const gfxMatrix aTransform = gfxMatrix());
    virtual ~gfxSurfaceDrawable() {}

    virtual bool Draw(gfxContext* aContext,
                        const gfxRect& aFillRect,
                        bool aRepeat,
                        const GraphicsFilter& aFilter,
                        gfxFloat aOpacity = 1.0,
                        const gfxMatrix& aTransform = gfxMatrix());
    virtual bool DrawWithSamplingRect(gfxContext* aContext,
                                      const gfxRect& aFillRect,
                                      const gfxRect& aSamplingRect,
                                      bool aRepeat,
                                      const GraphicsFilter& aFilter,
                                      gfxFloat aOpacity = 1.0);
    
protected:
    void DrawInternal(gfxContext* aContext,
                      const gfxRect& aFillRect,
                      const mozilla::gfx::IntRect& aSamplingRect,
                      bool aRepeat,
                      const GraphicsFilter& aFilter,
                      gfxFloat aOpacity,
                      const gfxMatrix& aTransform = gfxMatrix());

    mozilla::RefPtr<mozilla::gfx::SourceSurface> mSourceSurface;
    const gfxMatrix mTransform;
};





class gfxDrawingCallback {
    NS_INLINE_DECL_REFCOUNTING(gfxDrawingCallback)
protected:
    
    virtual ~gfxDrawingCallback() {}

public:
    






    virtual bool operator()(gfxContext* aContext,
                              const gfxRect& aFillRect,
                              const GraphicsFilter& aFilter,
                              const gfxMatrix& aTransform = gfxMatrix()) = 0;

};





class gfxCallbackDrawable : public gfxDrawable {
public:
    gfxCallbackDrawable(gfxDrawingCallback* aCallback, const mozilla::gfx::IntSize aSize);
    virtual ~gfxCallbackDrawable() {}

    virtual bool Draw(gfxContext* aContext,
                        const gfxRect& aFillRect,
                        bool aRepeat,
                        const GraphicsFilter& aFilter,
                        gfxFloat aOpacity = 1.0,
                        const gfxMatrix& aTransform = gfxMatrix());

protected:
    already_AddRefed<gfxSurfaceDrawable> MakeSurfaceDrawable(const GraphicsFilter aFilter = GraphicsFilter::FILTER_FAST);

    nsRefPtr<gfxDrawingCallback> mCallback;
    nsRefPtr<gfxSurfaceDrawable> mSurfaceDrawable;
};





class gfxPatternDrawable : public gfxDrawable {
public:
    gfxPatternDrawable(gfxPattern* aPattern,
                       const mozilla::gfx::IntSize aSize);
    virtual ~gfxPatternDrawable();

    virtual bool Draw(gfxContext* aContext,
                        const gfxRect& aFillRect,
                        bool aRepeat,
                        const GraphicsFilter& aFilter,
                        gfxFloat aOpacity = 1.0,
                        const gfxMatrix& aTransform = gfxMatrix());

protected:
    already_AddRefed<gfxCallbackDrawable> MakeCallbackDrawable();

    nsRefPtr<gfxPattern> mPattern;
};

#endif 
