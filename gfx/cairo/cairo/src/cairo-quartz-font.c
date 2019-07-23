



































#include "cairoint.h"

#include <dlfcn.h>

#include "cairo-quartz.h"
#include "cairo-quartz-private.h"


static CGFontRef (*CGFontCreateWithFontNamePtr) (CFStringRef) = NULL;
static CGFontRef (*CGFontCreateWithNamePtr) (const char *) = NULL;


static int (*CGFontGetUnitsPerEmPtr) (CGFontRef) = NULL;
static bool (*CGFontGetGlyphAdvancesPtr) (CGFontRef, const CGGlyph[], size_t, int[]) = NULL;
static bool (*CGFontGetGlyphBBoxesPtr) (CGFontRef, const CGGlyph[], size_t, CGRect[]) = NULL;
static CGRect (*CGFontGetFontBBoxPtr) (CGFontRef) = NULL;


static void (*CGFontGetGlyphsForUnicharsPtr) (CGFontRef, const UniChar[], const CGGlyph[], size_t) = NULL;


static CGPathRef (*CGFontGetGlyphPathPtr) (CGFontRef fontRef, CGAffineTransform *textTransform, int unknown, CGGlyph glyph) = NULL;


typedef struct {
    int ascent;
    int descent;
    int leading;
} quartz_CGFontMetrics;
static quartz_CGFontMetrics* (*CGFontGetHMetricsPtr) (CGFontRef fontRef) = NULL;
static int (*CGFontGetAscentPtr) (CGFontRef fontRef) = NULL;
static int (*CGFontGetDescentPtr) (CGFontRef fontRef) = NULL;
static int (*CGFontGetLeadingPtr) (CGFontRef fontRef) = NULL;

static cairo_bool_t _cairo_quartz_font_symbol_lookup_done = FALSE;
static cairo_bool_t _cairo_quartz_font_symbols_present = FALSE;

static void
quartz_font_ensure_symbols(void)
{
    if (_cairo_quartz_font_symbol_lookup_done)
	return;

    
    CGFontGetGlyphBBoxesPtr = dlsym(RTLD_DEFAULT, "CGFontGetGlyphBBoxes");
    if (!CGFontGetGlyphBBoxesPtr)
	CGFontGetGlyphBBoxesPtr = dlsym(RTLD_DEFAULT, "CGFontGetGlyphBoundingBoxes");

    CGFontGetGlyphsForUnicharsPtr = dlsym(RTLD_DEFAULT, "CGFontGetGlyphsForUnichars");
    if (!CGFontGetGlyphsForUnicharsPtr)
	CGFontGetGlyphsForUnicharsPtr = dlsym(RTLD_DEFAULT, "CGFontGetGlyphsForUnicodes");

    CGFontGetFontBBoxPtr = dlsym(RTLD_DEFAULT, "CGFontGetFontBBox");

    
    CGFontCreateWithFontNamePtr = dlsym(RTLD_DEFAULT, "CGFontCreateWithFontName");
    CGFontCreateWithNamePtr = dlsym(RTLD_DEFAULT, "CGFontCreateWithName");

    
    CGFontGetUnitsPerEmPtr = dlsym(RTLD_DEFAULT, "CGFontGetUnitsPerEm");
    CGFontGetGlyphAdvancesPtr = dlsym(RTLD_DEFAULT, "CGFontGetGlyphAdvances");
    CGFontGetGlyphPathPtr = dlsym(RTLD_DEFAULT, "CGFontGetGlyphPath");

    CGFontGetHMetricsPtr = dlsym(RTLD_DEFAULT, "CGFontGetHMetrics");
    CGFontGetAscentPtr = dlsym(RTLD_DEFAULT, "CGFontGetAscent");
    CGFontGetDescentPtr = dlsym(RTLD_DEFAULT, "CGFontGetDescent");
    CGFontGetLeadingPtr = dlsym(RTLD_DEFAULT, "CGFontGetLeading");

    if ((CGFontCreateWithFontNamePtr || CGFontCreateWithNamePtr) &&
	CGFontGetGlyphBBoxesPtr &&
	CGFontGetGlyphsForUnicharsPtr &&
	CGFontGetUnitsPerEmPtr &&
	CGFontGetGlyphAdvancesPtr &&
	CGFontGetGlyphPathPtr &&
	(CGFontGetHMetricsPtr || (CGFontGetAscentPtr && CGFontGetDescentPtr && CGFontGetLeadingPtr)))
	_cairo_quartz_font_symbols_present = TRUE;

    _cairo_quartz_font_symbol_lookup_done = TRUE;
}

typedef struct _cairo_quartz_font_face cairo_quartz_font_face_t;
typedef struct _cairo_quartz_scaled_font cairo_quartz_scaled_font_t;

struct _cairo_quartz_scaled_font {
    cairo_scaled_font_t base;
};

struct _cairo_quartz_font_face {
    cairo_font_face_t base;

    CGFontRef cgFont;
};





static void
_cairo_quartz_font_face_destroy (void *abstract_face)
{
    cairo_quartz_font_face_t *font_face = (cairo_quartz_font_face_t*) abstract_face;

    CGFontRelease (font_face->cgFont);
}

static cairo_status_t
_cairo_quartz_font_face_scaled_font_create (void *abstract_face,
					    const cairo_matrix_t *font_matrix,
					    const cairo_matrix_t *ctm,
					    const cairo_font_options_t *options,
					    cairo_scaled_font_t **font_out)
{
    cairo_quartz_font_face_t *font_face = abstract_face;
    cairo_quartz_scaled_font_t *font = NULL;
    cairo_status_t status;
    cairo_font_extents_t fs_metrics;
    double ems;
    CGRect bbox;

    quartz_font_ensure_symbols();
    if (!_cairo_quartz_font_symbols_present)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    font = malloc(sizeof(cairo_quartz_scaled_font_t));
    if (font == NULL)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    memset (font, 0, sizeof(cairo_quartz_scaled_font_t));

    status = _cairo_scaled_font_init (&font->base,
				      &font_face->base, font_matrix, ctm, options,
				      &cairo_quartz_scaled_font_backend);
    if (status)
	goto FINISH;

    ems = CGFontGetUnitsPerEmPtr (font_face->cgFont);

    
    if (CGFontGetFontBBoxPtr && CGFontGetAscentPtr) {
	fs_metrics.ascent = (CGFontGetAscentPtr (font_face->cgFont) / ems);
	fs_metrics.descent = - (CGFontGetDescentPtr (font_face->cgFont) / ems);
	fs_metrics.height = fs_metrics.ascent + fs_metrics.descent +
	    (CGFontGetLeadingPtr (font_face->cgFont) / ems);

	bbox = CGFontGetFontBBoxPtr (font_face->cgFont);
	fs_metrics.max_x_advance = CGRectGetMaxX(bbox) / ems;
	fs_metrics.max_y_advance = 0.0;
    } else {
	CGGlyph wGlyph;
	UniChar u;

	quartz_CGFontMetrics *m;
	m = CGFontGetHMetricsPtr (font_face->cgFont);

	fs_metrics.ascent = (m->ascent / ems);
	fs_metrics.descent = - (m->descent / ems);
	fs_metrics.height = fs_metrics.ascent + fs_metrics.descent + (m->leading / ems);

	
	u = (UniChar) 'W';
	CGFontGetGlyphsForUnicharsPtr (font_face->cgFont, &u, &wGlyph, 1);
	if (wGlyph && CGFontGetGlyphBBoxesPtr (font_face->cgFont, &wGlyph, 1, &bbox)) {
	    fs_metrics.max_x_advance = CGRectGetMaxX(bbox) / ems;
	    fs_metrics.max_y_advance = 0.0;
	} else {
	    fs_metrics.max_x_advance = 0.0;
	    fs_metrics.max_y_advance = 0.0;
	}
    }

    status = _cairo_scaled_font_set_metrics (&font->base, &fs_metrics);

FINISH:
    if (status != CAIRO_STATUS_SUCCESS) {
	free (font);
    } else {
	*font_out = (cairo_scaled_font_t*) font;
    }

    return status;
}

static const cairo_font_face_backend_t _cairo_quartz_font_face_backend = {
    CAIRO_FONT_TYPE_QUARTZ,
    _cairo_quartz_font_face_destroy,
    _cairo_quartz_font_face_scaled_font_create
};














cairo_font_face_t *
cairo_quartz_font_face_create_for_cgfont (CGFontRef font)
{
    cairo_quartz_font_face_t *font_face;

    quartz_font_ensure_symbols();

    font_face = malloc (sizeof (cairo_quartz_font_face_t));
    if (!font_face) {
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_font_face_t *)&_cairo_font_face_nil;
    }

    font_face->cgFont = CGFontRetain (font);

    _cairo_font_face_init (&font_face->base, &_cairo_quartz_font_face_backend);

    return &font_face->base;
}





static cairo_quartz_font_face_t *
_cairo_quartz_scaled_to_face (void *abstract_font)
{
    cairo_quartz_scaled_font_t *sfont = (cairo_quartz_scaled_font_t*) abstract_font;
    cairo_font_face_t *font_face = cairo_scaled_font_get_font_face (&sfont->base);
    if (!font_face || font_face->backend->type != CAIRO_FONT_TYPE_QUARTZ)
	return NULL;

    return (cairo_quartz_font_face_t*) font_face;
}

static cairo_status_t
_cairo_quartz_font_create_toy(cairo_toy_font_face_t *toy_face,
			      const cairo_matrix_t *font_matrix,
			      const cairo_matrix_t *ctm,
			      const cairo_font_options_t *options,
			      cairo_scaled_font_t **font_out)
{
    const char *family = toy_face->family;
    char *full_name = malloc(strlen(family) + 64); 
    CFStringRef cgFontName = NULL;
    CGFontRef cgFont = NULL;
    int loop;

    cairo_status_t status;
    cairo_font_face_t *face;
    cairo_scaled_font_t *scaled_font;

    quartz_font_ensure_symbols();
    if (!_cairo_quartz_font_symbols_present)
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);

    
    if (!strcmp(family, "serif") || !strcmp(family, "Times Roman"))
	family = "Times";
    else if (!strcmp(family, "sans-serif") || !strcmp(family, "sans"))
	family = "Helvetica";
    else if (!strcmp(family, "cursive"))
	family = "Apple Chancery";
    else if (!strcmp(family, "fantasy"))
	family = "Papyrus";
    else if (!strcmp(family, "monospace") || !strcmp(family, "mono"))
	family = "Courier";

    



    for (loop = 0; loop < 5; loop++) {
	if (loop == 4)
	    family = "Helvetica";

	strcpy (full_name, family);

	if (loop < 3 && (loop & 1) == 0) {
	    if (toy_face->weight == CAIRO_FONT_WEIGHT_BOLD)
		strcat (full_name, " Bold");
	}

	if (loop < 3 && (loop & 2) == 0) {
	    if (toy_face->slant == CAIRO_FONT_SLANT_ITALIC)
		strcat (full_name, " Italic");
	    else if (toy_face->slant == CAIRO_FONT_SLANT_OBLIQUE)
		strcat (full_name, " Oblique");
	}

	if (CGFontCreateWithFontNamePtr) {
	    cgFontName = CFStringCreateWithCString (NULL, full_name, kCFStringEncodingASCII);
	    cgFont = CGFontCreateWithFontNamePtr (cgFontName);
	    CFRelease (cgFontName);
	} else {
	    cgFont = CGFontCreateWithNamePtr (full_name);
	}

	if (cgFont)
	    break;
    }

    if (!cgFont) {
	
	return CAIRO_STATUS_NO_MEMORY;
    }

    face = cairo_quartz_font_face_create_for_cgfont (cgFont);
    CGFontRelease (cgFont);

    if (face->status)
	return face->status;

    status = _cairo_quartz_font_face_scaled_font_create (face,
							 font_matrix, ctm,
							 options,
							 &scaled_font);
    cairo_font_face_destroy (face);
    if (status)
	return status;

    *font_out = scaled_font;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_quartz_font_fini(void *abstract_font)
{
}

#define INVALID_GLYPH 0x00

static inline CGGlyph
_cairo_quartz_scaled_glyph_index (cairo_scaled_glyph_t *scaled_glyph) {
    unsigned long index = _cairo_scaled_glyph_index (scaled_glyph);
    if (index > 0xffff)
	return INVALID_GLYPH;
    return (CGGlyph) index;
}

static inline cairo_status_t
_cairo_matrix_to_unit_quartz_matrix (const cairo_matrix_t *m, CGAffineTransform *txout,
				     double *xout, double *yout)
{
    CGAffineTransform transform;
    double xscale, yscale;
    cairo_status_t status;

    status = _cairo_matrix_compute_scale_factors (m, &xscale, &yscale, 1);
    if (status)
	return status;

    transform = CGAffineTransformMake (m->xx, - m->yx,
				       - m->xy, m->yy,
				       0.0f, 0.0f);
    if (xout)
	*xout = xscale;
    if (yout)
	*yout = yscale;

    if (xscale)
	xscale = 1.0 / xscale;
    if (yscale)
	yscale = 1.0 / yscale;

    *txout = CGAffineTransformScale (transform, xscale, yscale);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_quartz_init_glyph_metrics (cairo_quartz_scaled_font_t *font,
				  cairo_scaled_glyph_t *scaled_glyph)
{
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

    cairo_quartz_font_face_t *font_face = _cairo_quartz_scaled_to_face(font);
    cairo_text_extents_t extents = {0, 0, 0, 0, 0, 0};
    CGAffineTransform textMatrix;
    CGGlyph glyph = _cairo_quartz_scaled_glyph_index (scaled_glyph);
    int advance;
    CGRect bbox;
    double emscale = CGFontGetUnitsPerEmPtr (font_face->cgFont);
    double xscale, yscale;
    double xmin, ymin, xmax, ymax;

    if (glyph == INVALID_GLYPH)
	goto FAIL;

    if (!CGFontGetGlyphAdvancesPtr (font_face->cgFont, &glyph, 1, &advance) ||
	!CGFontGetGlyphBBoxesPtr (font_face->cgFont, &glyph, 1, &bbox))
	goto FAIL;

    status = _cairo_matrix_compute_scale_factors (&font->base.scale,
						  &xscale, &yscale, 1);
    if (status)
	goto FAIL;

    bbox = CGRectMake (bbox.origin.x / emscale,
		       bbox.origin.y / emscale,
		       bbox.size.width / emscale,
		       bbox.size.height / emscale);

    
#if 0
    {
	CGAffineTransform textMatrix;
	textMatrix = CGAffineTransformMake (font->base.scale.xx,
					    -font->base.scale.yx,
					    -font->base.scale.xy,
					    font->base.scale.yy,
					    0.0f, 0.0f);

	bbox = CGRectApplyAffineTransform (bbox, textMatrix);
	bbox = CGRectIntegral (bbox);
	bbox = CGRectApplyAffineTransform (bbox, CGAffineTransformInvert (textMatrix));
    }
#endif

#if 0
    fprintf (stderr, "[0x%04x] bbox: %f %f %f %f\n", glyph,
	     bbox.origin.x / emscale, bbox.origin.y / emscale,
	     bbox.size.width / emscale, bbox.size.height / emscale);
#endif

    xmin = CGRectGetMinX(bbox);
    ymin = CGRectGetMinY(bbox);
    xmax = CGRectGetMaxX(bbox);
    ymax = CGRectGetMaxY(bbox);

    extents.x_bearing = xmin;
    extents.y_bearing = - ymax;
    extents.width = xmax - xmin;
    extents.height = ymax - ymin;

    extents.x_advance = (double) advance / emscale;
    extents.y_advance = 0.0;

#if 0
    fprintf (stderr, "[0x%04x] extents: bearings: %f %f dim: %f %f adv: %f\n\n", glyph,
	     extents.x_bearing, extents.y_bearing, extents.width, extents.height, extents.x_advance);
#endif

  FAIL:
    _cairo_scaled_glyph_set_metrics (scaled_glyph,
				     &font->base,
				     &extents);

    return status;
}

static void
_cairo_quartz_path_apply_func (void *info, const CGPathElement *el)
{
    cairo_path_fixed_t *path = (cairo_path_fixed_t *) info;

    switch (el->type) {
	case kCGPathElementMoveToPoint:
	    _cairo_path_fixed_move_to (path,
				       _cairo_fixed_from_double(el->points[0].x),
				       _cairo_fixed_from_double(el->points[0].y));
	    break;
	case kCGPathElementAddLineToPoint:
	    _cairo_path_fixed_line_to (path,
				       _cairo_fixed_from_double(el->points[0].x),
				       _cairo_fixed_from_double(el->points[0].y));
	    break;
	case kCGPathElementAddQuadCurveToPoint: {
	    cairo_fixed_t fx, fy;
	    double x, y;
	    if (!_cairo_path_fixed_get_current_point (path, &fx, &fy))
		fx = fy = 0;
	    x = _cairo_fixed_to_double (fx);
	    y = _cairo_fixed_to_double (fy);

	    _cairo_path_fixed_curve_to (path,
					_cairo_fixed_from_double((x + el->points[0].x * 2.0) / 3.0),
					_cairo_fixed_from_double((y + el->points[0].y * 2.0) / 3.0),
					_cairo_fixed_from_double((el->points[0].x * 2.0 + el->points[1].x) / 3.0),
					_cairo_fixed_from_double((el->points[0].y * 2.0 + el->points[1].y) / 3.0),
					_cairo_fixed_from_double(el->points[1].x),
					_cairo_fixed_from_double(el->points[1].y));
	}
	    break;
	case kCGPathElementAddCurveToPoint:
	    _cairo_path_fixed_curve_to (path,
					_cairo_fixed_from_double(el->points[0].x),
					_cairo_fixed_from_double(el->points[0].y),
					_cairo_fixed_from_double(el->points[1].x),
					_cairo_fixed_from_double(el->points[1].y),
					_cairo_fixed_from_double(el->points[2].x),
					_cairo_fixed_from_double(el->points[2].y));
	    break;
	case kCGPathElementCloseSubpath:
	    _cairo_path_fixed_close_path (path);
	    break;
    }
}

static cairo_int_status_t
_cairo_quartz_init_glyph_path (cairo_quartz_scaled_font_t *font,
			       cairo_scaled_glyph_t *scaled_glyph)
{
    cairo_quartz_font_face_t *font_face = _cairo_quartz_scaled_to_face(font);
    CGGlyph glyph = _cairo_quartz_scaled_glyph_index (scaled_glyph);
    CGAffineTransform textMatrix;
    CGPathRef glyphPath;
    cairo_path_fixed_t *path;

    if (glyph == INVALID_GLYPH) {
	_cairo_scaled_glyph_set_path (scaled_glyph, &font->base, _cairo_path_fixed_create());
	return CAIRO_STATUS_SUCCESS;
    }

    textMatrix = CGAffineTransformMake (font->base.scale.xx,
					-font->base.scale.yx,
					-font->base.scale.xy,
					font->base.scale.yy,
					font->base.scale.x0,
					font->base.scale.y0);

    textMatrix = CGAffineTransformConcat (textMatrix, CGAffineTransformMake (1.0, 0.0, 0.0, -1.0, 0.0, 0.0));

    glyphPath = CGFontGetGlyphPathPtr (font_face->cgFont, &textMatrix, 0, glyph);
    if (!glyphPath)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    path = _cairo_path_fixed_create ();
    if (!path) {
	CGPathRelease (glyphPath);
	return _cairo_error(CAIRO_STATUS_NO_MEMORY);
    }

    CGPathApply (glyphPath, path, _cairo_quartz_path_apply_func);

    CGPathRelease (glyphPath);

    _cairo_scaled_glyph_set_path (scaled_glyph, &font->base, path);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_quartz_init_glyph_surface (cairo_quartz_scaled_font_t *font,
				  cairo_scaled_glyph_t *scaled_glyph)
{
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

    cairo_quartz_font_face_t *font_face = _cairo_quartz_scaled_to_face(font);

    cairo_image_surface_t *surface = NULL;

    CGGlyph glyph = _cairo_quartz_scaled_glyph_index (scaled_glyph);

    int advance;
    CGRect bbox;
    double width, height;
    double xscale, yscale;
    double emscale = CGFontGetUnitsPerEmPtr (font_face->cgFont);

    CGColorSpaceRef gray;
    CGContextRef cgContext = NULL;
    CGAffineTransform textMatrix;
    CGRect glyphRect, glyphRectInt;
    CGPoint glyphOrigin;

    

    



    if (glyph == INVALID_GLYPH) {
	surface = (cairo_image_surface_t*) cairo_image_surface_create (CAIRO_FORMAT_A8, 2, 2);
	status = cairo_surface_status ((cairo_surface_t *) surface);
	if (status)
	    return status;

	_cairo_scaled_glyph_set_surface (scaled_glyph,
					 &font->base,
					 surface);
	return CAIRO_STATUS_SUCCESS;
    }

    if (!CGFontGetGlyphAdvancesPtr (font_face->cgFont, &glyph, 1, &advance) ||
	!CGFontGetGlyphBBoxesPtr (font_face->cgFont, &glyph, 1, &bbox))
    {
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    status = _cairo_matrix_compute_scale_factors (&font->base.scale,
						  &xscale, &yscale, 1);
    if (status)
	return status;

    textMatrix = CGAffineTransformMake (font->base.scale.xx,
					-font->base.scale.yx,
					-font->base.scale.xy,
					font->base.scale.yy,
					0.0f, 0.0f);
    glyphRect = CGRectMake (bbox.origin.x / emscale,
			    bbox.origin.y / emscale,
			    bbox.size.width / emscale,
			    bbox.size.height / emscale);

    glyphRect = CGRectApplyAffineTransform (glyphRect, textMatrix);

    


    glyphRectInt = CGRectIntegral (glyphRect);

#if 0
    fprintf (stderr, "glyphRect[o]: %f %f %f %f\n",
	     glyphRect.origin.x, glyphRect.origin.y, glyphRect.size.width, glyphRect.size.height);
    fprintf (stderr, "glyphRectInt: %f %f %f %f\n",
	     glyphRectInt.origin.x, glyphRectInt.origin.y, glyphRectInt.size.width, glyphRectInt.size.height);
#endif

    glyphOrigin = glyphRectInt.origin;

    

    width = glyphRectInt.size.width;
    height = glyphRectInt.size.height;

    

    surface = (cairo_image_surface_t*) cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
    if (surface->base.status)
	return surface->base.status;

    gray = CGColorSpaceCreateDeviceGray ();
    cgContext = CGBitmapContextCreate (surface->data,
				       surface->width,
				       surface->height,
				       8,
				       surface->stride,
				       gray,
				       kCGImageAlphaNone);
    CGColorSpaceRelease (gray);

    CGContextSetFont (cgContext, font_face->cgFont);
    CGContextSetFontSize (cgContext, 1.0);
    CGContextSetTextMatrix (cgContext, textMatrix);

    CGContextClearRect (cgContext, CGRectMake (0.0f, 0.0f, width, height));

    if (font->base.options.antialias == CAIRO_ANTIALIAS_NONE)
	CGContextSetShouldAntialias (cgContext, false);

    CGContextSetRGBFillColor (cgContext, 1.0, 1.0, 1.0, 1.0);
    CGContextShowGlyphsAtPoint (cgContext, - glyphOrigin.x, - glyphOrigin.y, &glyph, 1);

    CGContextRelease (cgContext);

    cairo_surface_set_device_offset (&surface->base,
				     - glyphOrigin.x,
				     height + glyphOrigin.y);

    _cairo_scaled_glyph_set_surface (scaled_glyph, &font->base, surface);

    return status;
}

static cairo_int_status_t
_cairo_quartz_font_scaled_glyph_init (void *abstract_font,
				      cairo_scaled_glyph_t *scaled_glyph,
				      cairo_scaled_glyph_info_t info)
{
    cairo_quartz_scaled_font_t *font = (cairo_quartz_scaled_font_t *) abstract_font;
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

    if (!status && (info & CAIRO_SCALED_GLYPH_INFO_METRICS))
	status = _cairo_quartz_init_glyph_metrics (font, scaled_glyph);

    if (!status && (info & CAIRO_SCALED_GLYPH_INFO_PATH))
	status = _cairo_quartz_init_glyph_path (font, scaled_glyph);

    if (!status && (info & CAIRO_SCALED_GLYPH_INFO_SURFACE))
	status = _cairo_quartz_init_glyph_surface (font, scaled_glyph);

    return status;
}

static unsigned long
_cairo_quartz_ucs4_to_index (void *abstract_font,
			     uint32_t ucs4)
{
    cairo_quartz_scaled_font_t *font = (cairo_quartz_scaled_font_t*) abstract_font;
    cairo_quartz_font_face_t *ffont = _cairo_quartz_scaled_to_face(font);
    UniChar u = (UniChar) ucs4;
    CGGlyph glyph;

    CGFontGetGlyphsForUnicharsPtr (ffont->cgFont, &u, &glyph, 1);

    return glyph;
}

const cairo_scaled_font_backend_t cairo_quartz_scaled_font_backend = {
    CAIRO_FONT_TYPE_QUARTZ,
    _cairo_quartz_font_create_toy,
    _cairo_quartz_font_fini,
    _cairo_quartz_font_scaled_glyph_init,
    NULL, 
    _cairo_quartz_ucs4_to_index,
    NULL, 
    NULL, 
    NULL, 
};





CGFontRef
_cairo_quartz_scaled_font_get_cg_font_ref (cairo_scaled_font_t *abstract_font)
{
    cairo_quartz_font_face_t *ffont = _cairo_quartz_scaled_to_face(abstract_font);

    return ffont->cgFont;
}



















cairo_font_face_t *
cairo_quartz_font_face_create_for_atsu_font_id (ATSUFontID font_id)
{
    ATSFontRef atsFont = FMGetATSFontRefFromFont (font_id);
    CGFontRef cgFont = CGFontCreateWithPlatformFont (&atsFont);
    cairo_font_face_t *ff;

    ff = cairo_quartz_font_face_create_for_cgfont (cgFont);

    CGFontRelease (cgFont);

    return ff;
}


cairo_font_face_t *cairo_atsui_font_face_create_for_atsu_font_id (ATSUFontID font_id);

cairo_font_face_t *
cairo_atsui_font_face_create_for_atsu_font_id (ATSUFontID font_id)
{
    return cairo_quartz_font_face_create_for_atsu_font_id (font_id);
}
