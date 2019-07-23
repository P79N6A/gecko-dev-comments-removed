




































#include "gfxMatrix.h"
#include "cairo.h"

#define CAIRO_MATRIX(x) reinterpret_cast<cairo_matrix_t*>((x))
#define CONST_CAIRO_MATRIX(x) reinterpret_cast<const cairo_matrix_t*>((x))

const gfxMatrix&
gfxMatrix::Reset()
{
    cairo_matrix_init_identity(CAIRO_MATRIX(this));
    return *this;
}

const gfxMatrix&
gfxMatrix::Invert()
{
    cairo_matrix_invert(CAIRO_MATRIX(this));
    return *this;
}

const gfxMatrix&
gfxMatrix::Scale(gfxFloat x, gfxFloat y)
{
    cairo_matrix_scale(CAIRO_MATRIX(this), x, y);
    return *this;
}

const gfxMatrix&
gfxMatrix::Translate(const gfxPoint& pt)
{
    cairo_matrix_translate(CAIRO_MATRIX(this), pt.x, pt.y);
    return *this;
}

const gfxMatrix&
gfxMatrix::Rotate(gfxFloat radians)
{
    cairo_matrix_rotate(CAIRO_MATRIX(this), radians);
    return *this;
}

const gfxMatrix&
gfxMatrix::Multiply(const gfxMatrix& m)
{
    cairo_matrix_multiply(CAIRO_MATRIX(this), CAIRO_MATRIX(this), CONST_CAIRO_MATRIX(&m));
    return *this;
}

gfxPoint
gfxMatrix::Transform(const gfxPoint& point) const
{
    gfxPoint ret = point;
    cairo_matrix_transform_point(CONST_CAIRO_MATRIX(this), &ret.x, &ret.y);
    return ret;
}

gfxSize
gfxMatrix::Transform(const gfxSize& size) const
{
    gfxSize ret = size;
    cairo_matrix_transform_distance(CONST_CAIRO_MATRIX(this), &ret.width, &ret.height);
    return ret;
}

gfxRect
gfxMatrix::Transform(const gfxRect& rect) const
{
    return gfxRect(Transform(rect.pos), Transform(rect.size));
}

gfxRect
gfxMatrix::TransformBounds(const gfxRect& rect) const
{
    
    int i;
    double quad_x[4], quad_y[4];
    double min_x, max_x;
    double min_y, max_y;

    quad_x[0] = rect.pos.x;
    quad_y[0] = rect.pos.y;
    cairo_matrix_transform_point (CONST_CAIRO_MATRIX(this), &quad_x[0], &quad_y[0]);

    quad_x[1] = rect.pos.x + rect.size.width;
    quad_y[1] = rect.pos.y;
    cairo_matrix_transform_point (CONST_CAIRO_MATRIX(this), &quad_x[1], &quad_y[1]);

    quad_x[2] = rect.pos.x;
    quad_y[2] = rect.pos.y + rect.size.height;
    cairo_matrix_transform_point (CONST_CAIRO_MATRIX(this), &quad_x[2], &quad_y[2]);

    quad_x[3] = rect.pos.x + rect.size.width;
    quad_y[3] = rect.pos.y + rect.size.height;
    cairo_matrix_transform_point (CONST_CAIRO_MATRIX(this), &quad_x[3], &quad_y[3]);

    min_x = max_x = quad_x[0];
    min_y = max_y = quad_y[0];

    for (i = 1; i < 4; i++) {
        if (quad_x[i] < min_x)
            min_x = quad_x[i];
        if (quad_x[i] > max_x)
            max_x = quad_x[i];

        if (quad_y[i] < min_y)
            min_y = quad_y[i];
        if (quad_y[i] > max_y)
            max_y = quad_y[i];
    }

    
#if 0
    if (is_tight) {
        






        *is_tight =
            (quad_x[1] == quad_x[0] && quad_y[1] == quad_y[3] &&
             quad_x[2] == quad_x[3] && quad_y[2] == quad_y[0]) ||
            (quad_x[1] == quad_x[3] && quad_y[1] == quad_y[0] &&
             quad_x[2] == quad_x[0] && quad_y[2] == quad_y[3]);
    }
#endif

    return gfxRect(min_x, min_y, max_x - min_x, max_y - min_y);
}
