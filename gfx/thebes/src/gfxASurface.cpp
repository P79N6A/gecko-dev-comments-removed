





































#include "gfxASurface.h"

#include "gfxImageSurface.h"

#include "cairo.h"

#ifdef CAIRO_HAS_WIN32_SURFACE
#include "gfxWindowsSurface.h"
#endif

#ifdef CAIRO_HAS_XLIB_SURFACE
#include "gfxXlibSurface.h"
#endif

#ifdef CAIRO_HAS_NQUARTZ_SURFACE
#include "gfxQuartzSurface.h"
#endif

#include <stdio.h>

static cairo_user_data_key_t gfxasurface_pointer_key;



nsrefcnt
gfxASurface::AddRef(void)
{
    NS_PRECONDITION(mSurface != nsnull, "gfxASurface::AddRef without mSurface");

    if (mHasFloatingRef) {
        
        mHasFloatingRef = PR_FALSE;
    } else {
        cairo_surface_reference(mSurface);
    }

    return (nsrefcnt) cairo_surface_get_reference_count(mSurface);
}

nsrefcnt
gfxASurface::Release(void)
{
    NS_PRECONDITION(!mHasFloatingRef, "gfxASurface::Release while floating ref still outstanding!");
    NS_PRECONDITION(mSurface != nsnull, "gfxASurface::Release without mSurface");
    
    
    
    nsrefcnt refcnt = (nsrefcnt) cairo_surface_get_reference_count(mSurface);
    cairo_surface_destroy(mSurface);

    

    return --refcnt;
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
#ifdef CAIRO_HAS_NQUARTZ_SURFACE
    else if (stype == CAIRO_SURFACE_TYPE_NQUARTZ) {
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
    SetSurfaceWrapper(surface, this);

    mSurface = surface;

    if (existingSurface) {
        mHasFloatingRef = PR_FALSE;
    } else {
        mHasFloatingRef = PR_TRUE;
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
