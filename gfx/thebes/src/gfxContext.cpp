





































#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "cairo.h"

#include "gfxContext.h"

#include "gfxColor.h"
#include "gfxMatrix.h"
#include "gfxASurface.h"
#include "gfxPattern.h"



gfxContext::gfxContext(gfxASurface *surface) :
    mSurface(surface)
{
    mCairo = cairo_create(surface->CairoSurface());
}
gfxContext::~gfxContext()
{
    cairo_destroy(mCairo);
}

gfxASurface *
gfxContext::OriginalSurface()
{
    return mSurface;
}

already_AddRefed<gfxASurface>
gfxContext::CurrentSurface(gfxFloat *dx, gfxFloat *dy)
{
    cairo_surface_t *s = cairo_get_group_target(mCairo);
    if (s == mSurface->CairoSurface()) {
        if (dx && dy)
            cairo_surface_get_device_offset(s, dx, dy);
        gfxASurface *ret = mSurface;
        NS_ADDREF(ret);
        return ret;
    }

    if (dx && dy)
        cairo_surface_get_device_offset(s, dx, dy);
    return gfxASurface::Wrap(s);
}

void
gfxContext::Save()
{
    cairo_save(mCairo);
}

void
gfxContext::Restore()
{
    cairo_restore(mCairo);
}


void
gfxContext::NewPath()
{
    cairo_new_path(mCairo);
}

void
gfxContext::ClosePath()
{
    cairo_close_path(mCairo);
}

gfxPoint
gfxContext::CurrentPoint() const
{
    double x, y;
    cairo_get_current_point(mCairo, &x, &y);
    return gfxPoint(x, y);
}

void
gfxContext::Stroke()
{
    cairo_stroke_preserve(mCairo);
}

void
gfxContext::Fill()
{
    cairo_fill_preserve(mCairo);
}

void
gfxContext::MoveTo(const gfxPoint& pt)
{
    cairo_move_to(mCairo, pt.x, pt.y);
}

void
gfxContext::NewSubPath()
{
    cairo_new_sub_path(mCairo);
}

void
gfxContext::LineTo(const gfxPoint& pt)
{
    cairo_line_to(mCairo, pt.x, pt.y);
}

void
gfxContext::CurveTo(const gfxPoint& pt1, const gfxPoint& pt2, const gfxPoint& pt3)
{
    cairo_curve_to(mCairo, pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y);
}

void
gfxContext::Arc(const gfxPoint& center, gfxFloat radius,
                gfxFloat angle1, gfxFloat angle2)
{
    cairo_arc(mCairo, center.x, center.y, radius, angle1, angle2);
}

void
gfxContext::NegativeArc(const gfxPoint& center, gfxFloat radius,
                        gfxFloat angle1, gfxFloat angle2)
{
    cairo_arc_negative(mCairo, center.x, center.y, radius, angle1, angle2);
}

void
gfxContext::Line(const gfxPoint& start, const gfxPoint& end)
{
    MoveTo(start);
    LineTo(end);
}





void
gfxContext::Rectangle(const gfxRect& rect, PRBool snapToPixels)
{
    if (snapToPixels) {
        gfxRect snappedRect(rect);

        if (UserToDevicePixelSnapped(snappedRect)) {
            cairo_matrix_t mat;
            cairo_get_matrix(mCairo, &mat);
            cairo_identity_matrix(mCairo);
            Rectangle(snappedRect);
            cairo_set_matrix(mCairo, &mat);

            return;
        }
    }

    cairo_rectangle(mCairo, rect.pos.x, rect.pos.y, rect.size.width, rect.size.height);
}

void
gfxContext::Ellipse(const gfxPoint& center, const gfxSize& dimensions)
{
    
    if (dimensions.width == dimensions.height) {
        double radius = dimensions.width / 2.0;

        cairo_arc(mCairo, center.x, center.y, radius, 0, 2.0 * M_PI);
    } else {
        double x = center.x;
        double y = center.y;
        double w = dimensions.width;
        double h = dimensions.height;

        cairo_new_path(mCairo);
        cairo_move_to(mCairo, x + w/2.0, y);

        cairo_rel_curve_to(mCairo,
                           0, 0,
                           w / 2.0, 0,
                           w / 2.0, h / 2.0);
        cairo_rel_curve_to(mCairo,
                           0, 0,
                           0, h / 2.0,
                           - w / 2.0, h / 2.0);
        cairo_rel_curve_to(mCairo,
                           0, 0,
                           - w / 2.0, 0,
                           - w / 2.0, - h / 2.0);
        cairo_rel_curve_to(mCairo,
                           0, 0,
                           0, - h / 2.0,
                           w / 2.0, - h / 2.0);
    }
}

void
gfxContext::Polygon(const gfxPoint *points, PRUint32 numPoints)
{
    if (numPoints == 0)
        return;

    cairo_move_to(mCairo, points[0].x, points[0].y);
    for (PRUint32 i = 1; i < numPoints; ++i) {
        cairo_line_to(mCairo, points[i].x, points[i].y);
    }
}

void
gfxContext::DrawSurface(gfxASurface *surface, const gfxSize& size)
{
    cairo_save(mCairo);
    cairo_set_source_surface(mCairo, surface->CairoSurface(), 0, 0);
    cairo_new_path(mCairo);

    
    Rectangle(gfxRect(gfxPoint(0.0, 0.0), size), PR_TRUE);

    cairo_fill(mCairo);
    cairo_restore(mCairo);
}


void
gfxContext::Translate(const gfxPoint& pt)
{
    cairo_translate(mCairo, pt.x, pt.y);
}

void
gfxContext::Scale(gfxFloat x, gfxFloat y)
{
    cairo_scale(mCairo, x, y);
}

void
gfxContext::Rotate(gfxFloat angle)
{
    cairo_rotate(mCairo, angle);
}

void
gfxContext::Multiply(const gfxMatrix& matrix)
{
    const cairo_matrix_t& mat = reinterpret_cast<const cairo_matrix_t&>(matrix);
    cairo_transform(mCairo, &mat);
}

void
gfxContext::SetMatrix(const gfxMatrix& matrix)
{
    const cairo_matrix_t& mat = reinterpret_cast<const cairo_matrix_t&>(matrix);
    cairo_set_matrix(mCairo, &mat);
}

void
gfxContext::IdentityMatrix()
{
    cairo_identity_matrix(mCairo);
}

gfxMatrix
gfxContext::CurrentMatrix() const
{
    cairo_matrix_t mat;
    cairo_get_matrix(mCairo, &mat);
    return gfxMatrix(*reinterpret_cast<gfxMatrix*>(&mat));
}

gfxPoint
gfxContext::DeviceToUser(const gfxPoint& point) const
{
    gfxPoint ret = point;
    cairo_device_to_user(mCairo, &ret.x, &ret.y);
    return ret;
}

gfxSize
gfxContext::DeviceToUser(const gfxSize& size) const
{
    gfxSize ret = size;
    cairo_device_to_user_distance(mCairo, &ret.width, &ret.height);
    return ret;
}

gfxRect
gfxContext::DeviceToUser(const gfxRect& rect) const
{
    gfxRect ret = rect;
    cairo_device_to_user(mCairo, &ret.pos.x, &ret.pos.y);
    cairo_device_to_user_distance(mCairo, &ret.size.width, &ret.size.height);
    return ret;
}

gfxPoint
gfxContext::UserToDevice(const gfxPoint& point) const
{
    gfxPoint ret = point;
    cairo_user_to_device(mCairo, &ret.x, &ret.y);
    return ret;
}

gfxSize
gfxContext::UserToDevice(const gfxSize& size) const
{
    gfxSize ret = size;
    cairo_user_to_device_distance(mCairo, &ret.width, &ret.height);
    return ret;
}

gfxRect
gfxContext::UserToDevice(const gfxRect& rect) const
{
    double xmin, ymin, xmax, ymax;
    xmin = rect.pos.x;
    ymin = rect.pos.y;
    xmax = rect.pos.x + rect.size.width;
    ymax = rect.pos.y + rect.size.height;

    double x[3], y[3];
    x[0] = xmin;  y[0] = ymax;
    x[1] = xmax;  y[1] = ymax;
    x[2] = xmax;  y[2] = ymin;

    cairo_user_to_device(mCairo, &xmin, &ymin);
    xmax = xmin;
    ymax = ymin;
    for (int i = 0; i < 3; i++) {
        cairo_user_to_device(mCairo, &x[i], &y[i]);
        xmin = PR_MIN(xmin, x[i]);
        xmax = PR_MAX(xmax, x[i]);
        ymin = PR_MIN(ymin, y[i]);
        ymax = PR_MAX(ymax, y[i]);
    }

    return gfxRect(xmin, ymin, xmax - xmin, ymax - ymin);
}

PRBool
gfxContext::UserToDevicePixelSnapped(gfxRect& rect) const
{
    
    cairo_matrix_t mat;
    cairo_get_matrix(mCairo, &mat);
    if (mat.xx != 1.0 || mat.yy != 1.0)
        return PR_FALSE;

    gfxPoint p1 = UserToDevice(rect.pos);
    gfxPoint p2 = UserToDevice(rect.pos + rect.size);

    gfxPoint p3 = UserToDevice(rect.pos + gfxSize(rect.size.width, 0.0));
    gfxPoint p4 = UserToDevice(rect.pos + gfxSize(0.0, rect.size.height));

    
    if (p1.x != p4.x ||
        p2.x != p3.x ||
        p1.y != p3.y ||
        p2.y != p4.y)
        return PR_FALSE;

    p1.Round();
    p2.Round();

    gfxPoint pd = p2 - p1;

    rect.pos = p1;
    rect.size = gfxSize(pd.x, pd.y);

    return PR_TRUE;
}

void
gfxContext::PixelSnappedRectangleAndSetPattern(const gfxRect& rect,
                                               gfxPattern *pattern)
{
    gfxRect r(rect);

    
    
    
    
    
    
    
    
    
    
    
    

    gfxMatrix mat = CurrentMatrix();
    if (UserToDevicePixelSnapped(r)) {
        IdentityMatrix();
    }

    Translate(r.pos);
    r.pos.x = r.pos.y = 0;
    Rectangle(r);
    SetPattern(pattern);

    SetMatrix(mat);
}

void
gfxContext::SetAntialiasMode(AntialiasMode mode)
{
    if (mode == MODE_ALIASED) {
        cairo_set_antialias(mCairo, CAIRO_ANTIALIAS_NONE);
    } else if (mode == MODE_COVERAGE) {
        cairo_set_antialias(mCairo, CAIRO_ANTIALIAS_DEFAULT);
    }
}

gfxContext::AntialiasMode
gfxContext::CurrentAntialiasMode() const
{
    cairo_antialias_t aa = cairo_get_antialias(mCairo);
    if (aa == CAIRO_ANTIALIAS_NONE)
        return MODE_ALIASED;
    return MODE_COVERAGE;
}

void
gfxContext::SetDash(gfxLineType ltype)
{
    static double dash[] = {5.0, 5.0};
    static double dot[] = {1.0, 1.0};

    switch (ltype) {
        case gfxLineDashed:
            SetDash(dash, 2, 0.0);
            break;
        case gfxLineDotted:
            SetDash(dot, 2, 0.0);
            break;
        case gfxLineSolid:
        default:
            SetDash(nsnull, 0, 0.0);
            break;
    }
}

void
gfxContext::SetDash(gfxFloat *dashes, int ndash, gfxFloat offset)
{
    cairo_set_dash(mCairo, dashes, ndash, offset);
}


void
gfxContext::SetLineWidth(gfxFloat width)
{
    cairo_set_line_width(mCairo, width);
}

gfxFloat
gfxContext::CurrentLineWidth() const
{
    return cairo_get_line_width(mCairo);
}

void
gfxContext::SetOperator(GraphicsOperator op)
{
    cairo_set_operator(mCairo, (cairo_operator_t)op);
}

gfxContext::GraphicsOperator
gfxContext::CurrentOperator() const
{
    return (GraphicsOperator)cairo_get_operator(mCairo);
}

void
gfxContext::SetLineCap(GraphicsLineCap cap)
{
    cairo_set_line_cap(mCairo, (cairo_line_cap_t)cap);
}

gfxContext::GraphicsLineCap
gfxContext::CurrentLineCap() const
{
    return (GraphicsLineCap)cairo_get_line_cap(mCairo);
}

void
gfxContext::SetLineJoin(GraphicsLineJoin join)
{
    cairo_set_line_join(mCairo, (cairo_line_join_t)join);
}

gfxContext::GraphicsLineJoin
gfxContext::CurrentLineJoin() const
{
    return (GraphicsLineJoin)cairo_get_line_join(mCairo);
}

void
gfxContext::SetMiterLimit(gfxFloat limit)
{
    cairo_set_miter_limit(mCairo, limit);
}

gfxFloat
gfxContext::CurrentMiterLimit() const
{
    return cairo_get_miter_limit(mCairo);
}

void
gfxContext::SetFillRule(FillRule rule)
{
    cairo_set_fill_rule(mCairo, (cairo_fill_rule_t)rule);
}

gfxContext::FillRule
gfxContext::CurrentFillRule() const
{
    return (FillRule)cairo_get_fill_rule(mCairo);
}


void
gfxContext::Clip(const gfxRect& rect)
{
    cairo_new_path(mCairo);
    cairo_rectangle(mCairo, rect.pos.x, rect.pos.y, rect.size.width, rect.size.height);
    cairo_clip(mCairo);
}

void
gfxContext::Clip()
{
    cairo_clip_preserve(mCairo);
}

void
gfxContext::ResetClip()
{
    cairo_reset_clip(mCairo);
}

void
gfxContext::UpdateSurfaceClip()
{
    NewPath();
    Rectangle(gfxRect(0,0,0,0));
    Fill();
}



void
gfxContext::SetColor(const gfxRGBA& c)
{
    cairo_set_source_rgba(mCairo, c.r, c.g, c.b, c.a);
}

PRBool
gfxContext::GetColor(gfxRGBA& c)
{
    return cairo_pattern_get_rgba(cairo_get_source(mCairo),
                                  &c.r,
                                  &c.g,
                                  &c.b,
                                  &c.a) == CAIRO_STATUS_SUCCESS;
}

void
gfxContext::SetSource(gfxASurface *surface, const gfxPoint& offset)
{
    cairo_set_source_surface(mCairo, surface->CairoSurface(), offset.x, offset.y);
}

void
gfxContext::SetPattern(gfxPattern *pattern)
{
    cairo_set_source(mCairo, pattern->CairoPattern());
}

already_AddRefed<gfxPattern>
gfxContext::GetPattern()
{
    cairo_pattern_t *pat = cairo_get_source(mCairo);
    NS_ASSERTION(pat, "I was told this couldn't be null");

    gfxPattern *wrapper = nsnull;
    if (pat)
        wrapper = new gfxPattern(pat);
    else
        wrapper = new gfxPattern(gfxRGBA(0,0,0,0));

    NS_ADDREF(wrapper);
    return wrapper;
}




void
gfxContext::Mask(gfxPattern *pattern)
{
    cairo_mask(mCairo, pattern->CairoPattern());
}

void
gfxContext::Mask(gfxASurface *surface, const gfxPoint& offset)
{
    cairo_mask_surface(mCairo, surface->CairoSurface(), offset.x, offset.y);
}

void
gfxContext::Paint(gfxFloat alpha)
{
    cairo_paint_with_alpha(mCairo, alpha);
}



void
gfxContext::PushGroup(gfxASurface::gfxContentType content)
{
    cairo_push_group_with_content(mCairo, (cairo_content_t) content);
}

already_AddRefed<gfxPattern>
gfxContext::PopGroup()
{
    cairo_pattern_t *pat = cairo_pop_group(mCairo);
    gfxPattern *wrapper = new gfxPattern(pat);
    NS_ADDREF(wrapper);
    return wrapper;
}

void
gfxContext::PopGroupToSource()
{
    cairo_pop_group_to_source(mCairo);
}

PRBool
gfxContext::PointInFill(const gfxPoint& pt)
{
    return cairo_in_fill(mCairo, pt.x, pt.y);
}

PRBool
gfxContext::PointInStroke(const gfxPoint& pt)
{
    return cairo_in_stroke(mCairo, pt.x, pt.y);
}

gfxRect
gfxContext::GetUserFillExtent()
{
    double xmin, ymin, xmax, ymax;
    cairo_fill_extents(mCairo, &xmin, &ymin, &xmax, &ymax);
    return gfxRect(xmin, ymin, xmax - xmin, ymax - ymin);
}

gfxRect
gfxContext::GetUserStrokeExtent()
{
    double xmin, ymin, xmax, ymax;
    cairo_stroke_extents(mCairo, &xmin, &ymin, &xmax, &ymax);
    return gfxRect(xmin, ymin, xmax - xmin, ymax - ymin);
}

already_AddRefed<gfxFlattenedPath>
gfxContext::GetFlattenedPath()
{
    gfxFlattenedPath *path =
        new gfxFlattenedPath(cairo_copy_path_flat(mCairo));
    NS_IF_ADDREF(path);
    return path;
}
