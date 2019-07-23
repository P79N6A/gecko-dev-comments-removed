




































#include "gfxTypes.h"
#include "gfxPattern.h"
#include "gfxASurface.h"

#include "cairo.h"

gfxPattern::gfxPattern(cairo_pattern_t *aPattern)
{
    mPattern = cairo_pattern_reference(aPattern);
}

gfxPattern::gfxPattern(const gfxRGBA& aColor)
{
    mPattern = cairo_pattern_create_rgba(aColor.r, aColor.g, aColor.b, aColor.a);
}


gfxPattern::gfxPattern(gfxASurface *surface)
{
    mPattern = cairo_pattern_create_for_surface(surface->CairoSurface());
}


gfxPattern::gfxPattern(gfxFloat x0, gfxFloat y0, gfxFloat x1, gfxFloat y1)
{
    mPattern = cairo_pattern_create_linear(x0, y0, x1, y1);
}


gfxPattern::gfxPattern(gfxFloat cx0, gfxFloat cy0, gfxFloat radius0,
                       gfxFloat cx1, gfxFloat cy1, gfxFloat radius1)
{
    mPattern = cairo_pattern_create_radial(cx0, cy0, radius0,
                                           cx1, cy1, radius1);
}

gfxPattern::~gfxPattern()
{
    cairo_pattern_destroy(mPattern);
}

cairo_pattern_t *
gfxPattern::CairoPattern()
{
    return mPattern;
}

void
gfxPattern::AddColorStop(gfxFloat offset, const gfxRGBA& c)
{
    cairo_pattern_add_color_stop_rgba(mPattern, offset, c.r, c.g, c.b, c.a);
}

void
gfxPattern::SetMatrix(const gfxMatrix& matrix)
{
    cairo_matrix_t mat = *reinterpret_cast<const cairo_matrix_t*>(&matrix);
    cairo_pattern_set_matrix(mPattern, &mat);
}

gfxMatrix
gfxPattern::GetMatrix() const
{
    cairo_matrix_t mat;
    cairo_pattern_get_matrix(mPattern, &mat);
    return gfxMatrix(*reinterpret_cast<gfxMatrix*>(&mat));
}

void
gfxPattern::SetExtend(GraphicsExtend extend)
{
    cairo_pattern_set_extend(mPattern, (cairo_extend_t)extend);
}

gfxPattern::GraphicsExtend
gfxPattern::Extend() const
{
    return (GraphicsExtend)cairo_pattern_get_extend(mPattern);
}

void
gfxPattern::SetFilter(int filter)
{
    cairo_pattern_set_filter(mPattern, (cairo_filter_t)filter);
}

int
gfxPattern::Filter() const
{
    return (int)cairo_pattern_get_filter(mPattern);
}

PRBool
gfxPattern::GetSolidColor(gfxRGBA& aColor)
{
    return cairo_pattern_get_rgba(mPattern,
                                  &aColor.r,
                                  &aColor.g,
                                  &aColor.b,
                                  &aColor.a) == CAIRO_STATUS_SUCCESS;
}

already_AddRefed<gfxASurface>
gfxPattern::GetSurface()
{
    cairo_surface_t *surf = nsnull;

    if (cairo_pattern_get_surface (mPattern, &surf) != CAIRO_STATUS_SUCCESS)
        return nsnull;


    return gfxASurface::Wrap(surf);
}
