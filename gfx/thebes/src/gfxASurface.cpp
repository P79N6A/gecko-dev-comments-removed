





































#include "gfxASurface.h"

#include "gfxImageSurface.h"

#include "cairo.h"

#ifdef CAIRO_HAS_WIN32_SURFACE
#include "gfxWindowsSurface.h"
#endif

#ifdef CAIRO_HAS_XLIB_SURFACE
#include "gfxXlibSurface.h"

#undef Status
#endif

#ifdef CAIRO_HAS_QUARTZ_SURFACE
#include "gfxQuartzSurface.h"
#endif

#include <stdio.h>

static cairo_user_data_key_t gfxasurface_pointer_key;



nsrefcnt
gfxASurface::AddRef(void)
{
    NS_PRECONDITION(mSurface != nsnull, "gfxASurface::AddRef without mSurface");

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
    NS_PRECONDITION(mSurface != nsnull, "gfxASurface::Release without mSurface");

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
    else if (stype == CAIRO_SURFACE_TYPE_WIN32) {
        result = new gfxWindowsSurface(csurf);
    }
#endif
#ifdef CAIRO_HAS_XLIB_SURFACE
    else if (stype == CAIRO_SURFACE_TYPE_XLIB) {
        result = new gfxXlibSurface(csurf);
    }
#endif
#ifdef CAIRO_HAS_QUARTZ_SURFACE
    else if (stype == CAIRO_SURFACE_TYPE_QUARTZ) {
        result = new gfxQuartzSurface(csurf);
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
    }
}

gfxASurface::gfxSurfaceType
gfxASurface::GetType() const
{
    return (gfxSurfaceType)cairo_surface_get_type(mSurface);
}

gfxASurface::gfxContentType
gfxASurface::GetContentType() const
{
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
gfxASurface::Flush()
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
                                       (int) r.pos.x, (int) r.pos.y,
                                       (int) r.size.width, (int) r.size.height);
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

int
gfxASurface::Status()
{
    if (!mSurfaceValid)
        return -1;

    return cairo_surface_status(mSurface);
}


PRBool
gfxASurface::CheckSurfaceSize(const gfxIntSize& sz, PRInt32 limit)
{
    if (sz.width <= 0 || sz.height <= 0)
        return PR_FALSE;

    
    PRInt32 tmp = sz.width * sz.height;
    if (tmp / sz.height != sz.width)
        return PR_FALSE;

    
    tmp = tmp * 4;
    if (tmp / 4 != sz.width * sz.height)
        return PR_FALSE;

    
    if (limit &&
        (sz.width > limit || sz.height > limit))
        return PR_FALSE;

    return PR_TRUE;
}
