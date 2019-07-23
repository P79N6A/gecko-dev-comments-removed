



































#include <stdlib.h>
#include <math.h>
#include "cairo-atsui.h"
#include "cairoint.h"
#include "cairo.h"
#include "cairo-quartz-private.h"





#ifndef FixedToFloat
#define fixed1              ((Fixed) 0x00010000L)
#define FixedToFloat(a)     ((float)(a) / fixed1)
#define FloatToFixed(a)     ((Fixed)((float)(a) * fixed1))
#endif




#ifdef __BIG_ENDIAN__
#define CG_BITMAP_BYTE_ORDER_FLAG 0
#else    

#define CG_BITMAP_BYTE_ORDER_FLAG kCGBitmapByteOrder32Host
#endif


CG_EXTERN CGRect CGRectApplyAffineTransform (CGRect, CGAffineTransform);

typedef struct _cairo_atsui_font_face cairo_atsui_font_face_t;
typedef struct _cairo_atsui_font cairo_atsui_font_t;
typedef struct _cairo_atsui_scaled_path cairo_atsui_scaled_path_t;

static cairo_status_t _cairo_atsui_font_create_scaled (cairo_font_face_t *font_face,
						       ATSUFontID font_id,
						       ATSUStyle style,
						       const cairo_matrix_t *font_matrix,
						       const cairo_matrix_t *ctm,
						       const cairo_font_options_t *options,
						       cairo_scaled_font_t **font_out);

struct _cairo_atsui_font {
    cairo_scaled_font_t base;

    ATSUStyle style;
    ATSUStyle unscaled_style;
    ATSUFontID fontID;
};

struct _cairo_atsui_font_face {
  cairo_font_face_t base;
  ATSUFontID font_id;
};

struct _cairo_atsui_scaled_path {
    cairo_path_fixed_t *path;
    cairo_matrix_t *scale;
};

static void
_cairo_atsui_font_face_destroy (void *abstract_face)
{
}

static cairo_status_t
_cairo_atsui_font_face_scaled_font_create (void	*abstract_face,
					   const cairo_matrix_t	*font_matrix,
					   const cairo_matrix_t	*ctm,
					   const cairo_font_options_t *options,
					   cairo_scaled_font_t **font)
{
    cairo_atsui_font_face_t *font_face = abstract_face;
    OSStatus err;
    ATSUAttributeTag styleTags[] = { kATSUFontTag };
    ATSUAttributeValuePtr styleValues[] = { &font_face->font_id };
    ByteCount styleSizes[] = {  sizeof(ATSUFontID) };
    ATSUStyle style;

    err = ATSUCreateStyle (&style);
    err = ATSUSetAttributes(style,
                            sizeof(styleTags) / sizeof(styleTags[0]),
                            styleTags, styleSizes, styleValues);

    return _cairo_atsui_font_create_scaled (&font_face->base, font_face->font_id, style,
					    font_matrix, ctm, options, font);
}

static const cairo_font_face_backend_t _cairo_atsui_font_face_backend = {
    CAIRO_FONT_TYPE_ATSUI,
    _cairo_atsui_font_face_destroy,
    _cairo_atsui_font_face_scaled_font_create
};

cairo_font_face_t *
cairo_atsui_font_face_create_for_atsu_font_id (ATSUFontID font_id)
{
  cairo_atsui_font_face_t *font_face;

  font_face = malloc (sizeof (cairo_atsui_font_face_t));
  if (!font_face) {
    _cairo_error (CAIRO_STATUS_NO_MEMORY);
    return (cairo_font_face_t *)&_cairo_font_face_nil;
  }

  font_face->font_id = font_id;

    _cairo_font_face_init (&font_face->base, &_cairo_atsui_font_face_backend);

    return &font_face->base;
}

static CGContextRef
CGBitmapContextCreateWithCairoImageSurface (const cairo_image_surface_t * surface)
{
    CGContextRef contextRef;
    CGColorSpaceRef colorSpace;
    int bits_per_comp, alpha;

    
    if (surface->depth == 1) {
	colorSpace = CGColorSpaceCreateDeviceGray ();
	bits_per_comp = 1;
	alpha = kCGImageAlphaNone;
    } else if (surface->depth == 8) {
	colorSpace = CGColorSpaceCreateDeviceGray ();
	bits_per_comp = 8;
	alpha = kCGImageAlphaNone;
    } else if (surface->depth == 24) {
	colorSpace = CGColorSpaceCreateDeviceRGB ();
	bits_per_comp = 8;
	alpha = kCGImageAlphaNoneSkipFirst | CG_BITMAP_BYTE_ORDER_FLAG;
    } else if (surface->depth == 32) {
	colorSpace = CGColorSpaceCreateDeviceRGB ();
	bits_per_comp = 8;
	alpha = kCGImageAlphaPremultipliedFirst | CG_BITMAP_BYTE_ORDER_FLAG;
    } else {
	
	return NULL;
    }
    
    contextRef = CGBitmapContextCreate (surface->data,
					surface->width,
					surface->height,
					bits_per_comp,
					surface->stride,
					colorSpace,
					alpha);
    CGColorSpaceRelease (colorSpace);

    return contextRef;
}

static CGAffineTransform
CGAffineTransformMakeWithCairoFontScale(const cairo_matrix_t *scale)
{
    return CGAffineTransformMake(scale->xx, scale->yx,
                                 scale->xy, scale->yy,
                                 0, 0);
}

static CGAffineTransform
CGAffineTransformMakeWithCairoScaleFactors(const cairo_matrix_t *scale)
{
    double xscale = 1.0;
    double yscale = 1.0;
    _cairo_matrix_compute_scale_factors(scale, &xscale, &yscale, 1);
    return CGAffineTransformMake(xscale, 0,
                                 0, yscale,
                                 0, 0);
}

static ATSUStyle
CreateSizedCopyOfStyle(ATSUStyle inStyle, const cairo_matrix_t *scale)
{
    ATSUStyle style;
    OSStatus err;

    
    Fixed theSize = FloatToFixed(1.0);
    CGAffineTransform theTransform = CGAffineTransformMakeWithCairoScaleFactors(scale);
    const ATSUAttributeTag theFontStyleTags[] = { kATSUSizeTag, kATSUFontMatrixTag };
    const ByteCount theFontStyleSizes[] = { sizeof(Fixed), sizeof(CGAffineTransform) };
    ATSUAttributeValuePtr theFontStyleValues[] = { &theSize, &theTransform };

    err = ATSUCreateAndCopyStyle(inStyle, &style);

    err = ATSUSetAttributes(style,
                            sizeof(theFontStyleTags) /
                            sizeof(ATSUAttributeTag), theFontStyleTags,
                            theFontStyleSizes, theFontStyleValues);

    return style;
}

static cairo_status_t
_cairo_atsui_font_set_metrics (cairo_atsui_font_t *font)
{
    ATSFontRef atsFont;
    ATSFontMetrics metrics;
    OSStatus err;

    atsFont = FMGetATSFontRefFromFont(font->fontID);

    if (atsFont) {
        err = ATSFontGetHorizontalMetrics(atsFont, kATSOptionFlagsDefault, &metrics);

        if (err == noErr) {
	    cairo_font_extents_t extents;

            extents.ascent = metrics.ascent;
            extents.descent = -metrics.descent;
            extents.height = metrics.capHeight;
            extents.max_x_advance = metrics.maxAdvanceWidth;

            
            extents.max_y_advance = 0.0;

	    _cairo_scaled_font_set_metrics (&font->base, &extents);

            return CAIRO_STATUS_SUCCESS;
        }
    }

    return CAIRO_STATUS_NULL_POINTER;
}

static cairo_status_t
_cairo_atsui_font_create_scaled (cairo_font_face_t *font_face,
				 ATSUFontID font_id,
				 ATSUStyle style,
				 const cairo_matrix_t *font_matrix,
				 const cairo_matrix_t *ctm,
				 const cairo_font_options_t *options,
				 cairo_scaled_font_t **font_out)
{
    cairo_atsui_font_t *font = NULL;
    OSStatus err;
    cairo_status_t status;

    font = malloc(sizeof(cairo_atsui_font_t));
    if (font == NULL)
	return CAIRO_STATUS_NO_MEMORY;

    _cairo_scaled_font_init(&font->base, font_face, font_matrix, ctm, options,
			    &cairo_atsui_scaled_font_backend);

    font->style = CreateSizedCopyOfStyle(style, &font->base.scale);

    {
	Fixed theSize = FloatToFixed(1.0);
	const ATSUAttributeTag theFontStyleTags[] = { kATSUSizeTag };
	const ByteCount theFontStyleSizes[] = { sizeof(Fixed) };
	ATSUAttributeValuePtr theFontStyleValues[] = { &theSize };

	err = ATSUSetAttributes(style,
				sizeof(theFontStyleTags) /
				sizeof(ATSUAttributeTag), theFontStyleTags,
				theFontStyleSizes, theFontStyleValues);
	if (err != noErr) {
	    status = CAIRO_STATUS_NO_MEMORY;
	    goto FAIL;
	}
    }

    font->unscaled_style = style;
    font->fontID = font_id;

    *font_out = &font->base;

    status = _cairo_atsui_font_set_metrics (font);

  FAIL:
    if (status) {
	cairo_scaled_font_destroy (&font->base);
	return status;
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_atsui_font_create_toy(cairo_toy_font_face_t *toy_face,
			     const cairo_matrix_t *font_matrix,
			     const cairo_matrix_t *ctm,
			     const cairo_font_options_t *options,
			     cairo_scaled_font_t **font_out)
{
    ATSUStyle style;
    ATSUFontID fontID;
    OSStatus err;
    Boolean isItalic, isBold;
    const char *family = toy_face->family;
    const char *full_name;

    err = ATSUCreateStyle(&style);

    switch (toy_face->weight) {
    case CAIRO_FONT_WEIGHT_BOLD:
        isBold = true;
        break;
    case CAIRO_FONT_WEIGHT_NORMAL:
    default:
        isBold = false;
        break;
    }

    switch (toy_face->slant) {
    case CAIRO_FONT_SLANT_ITALIC:
        isItalic = true;
        break;
    case CAIRO_FONT_SLANT_OBLIQUE:
        isItalic = false;
        break;
    case CAIRO_FONT_SLANT_NORMAL:
    default:
        isItalic = false;
        break;
    }

    











    if (!strcmp(family, "serif") || !strcmp(family, "Times"))
	full_name = "Times Roman";
    else if (!strcmp(family, "sans-serif") || !strcmp(family, "sans"))
	full_name = "Helvetica";
    else if (!strcmp(family, "cursive"))
	full_name = "Apple Chancery";
    else if (!strcmp(family, "fantasy"))
	full_name = "American Typewriter";
    else if (!strcmp(family, "monospace") || !strcmp(family, "mono"))
	full_name = "Courier";
    else
	full_name = family;

    err = ATSUFindFontFromName(family, strlen(family),
                               kFontFullName,
                               kFontNoPlatformCode,
                               kFontRomanScript,
                               kFontNoLanguageCode, &fontID);

    if (err != noErr) {
	



	err = ATSUFindFontFromName(family, strlen(family),
				   kFontFamilyName,
				   kFontNoPlatformCode,
				   kFontRomanScript,
				   kFontNoLanguageCode, &fontID);
	if (err != noErr) {
	    
	    full_name = "Courier";
	    err = ATSUFindFontFromName(full_name, strlen(full_name),
				       kFontFullName,
				       kFontNoPlatformCode,
				       kFontRomanScript,
				       kFontNoLanguageCode, &fontID);
	}
    }

    {
	ATSUAttributeTag styleTags[] =
	    { kATSUQDItalicTag, kATSUQDBoldfaceTag, kATSUFontTag };
	ATSUAttributeValuePtr styleValues[] = { &isItalic, &isBold, &fontID };
	ByteCount styleSizes[] =
	    { sizeof(Boolean), sizeof(Boolean), sizeof(ATSUFontID) };

	err = ATSUSetAttributes(style,
				sizeof(styleTags) / sizeof(styleTags[0]),
				styleTags, styleSizes, styleValues);
    }

    return _cairo_atsui_font_create_scaled (&toy_face->base, fontID, style,
					    font_matrix, ctm, options, font_out);
}

static void
_cairo_atsui_font_fini(void *abstract_font)
{
    cairo_atsui_font_t *font = abstract_font;

    if (font == NULL)
        return;

    if (font->style)
        ATSUDisposeStyle(font->style);
    if (font->unscaled_style)
        ATSUDisposeStyle(font->unscaled_style);
}

static
OSStatus _move_to_for_metrics (const Float32Point *point, void *callback_data)
{
    CGMutablePathRef path = callback_data;

    CGPathMoveToPoint (path, &CGAffineTransformIdentity,
			   point->x, point->y);
    return noErr;
}

static
OSStatus _line_to_for_metrics(const Float32Point *point, void *callback_data)
{
    CGMutablePathRef path = callback_data;

    CGPathAddLineToPoint (path, &CGAffineTransformIdentity,
			   point->x, point->y);
    return noErr;
}

static
OSStatus _curve_to_for_metrics (const Float32Point *point1,
				const Float32Point *point2,
				const Float32Point *point3,
				void *callback_data)
{
    CGMutablePathRef path = callback_data;

    CGPathAddCurveToPoint (path, &CGAffineTransformIdentity,
			   point1->x, point1->y,
			   point2->x, point2->y,
			   point3->x, point3->y);
    return noErr;
}

static
OSStatus _close_path_for_metrics(void *callback_data)
{
    CGMutablePathRef path = callback_data;

    CGPathCloseSubpath (path);
    return noErr;
}

static cairo_status_t
_cairo_atsui_font_init_glyph_metrics (cairo_atsui_font_t *scaled_font,
				      cairo_scaled_glyph_t *scaled_glyph)
{
    cairo_text_extents_t extents;
    OSStatus err, callback_err;
    ATSGlyphScreenMetrics metricsH;
    static ATSCubicMoveToUPP moveProc = NULL;
    static ATSCubicLineToUPP lineProc = NULL;
    static ATSCubicCurveToUPP curveProc = NULL;
    static ATSCubicClosePathUPP closePathProc = NULL;
    CGMutablePathRef path;
    GlyphID theGlyph = _cairo_scaled_glyph_index (scaled_glyph);
    double xscale, yscale;
    CGRect rect;

    


    err = ATSUGlyphGetScreenMetrics (scaled_font->style,
				     1, &theGlyph, 0, false,
				     false, &metricsH);
    if (err != noErr)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    
    _cairo_matrix_compute_scale_factors (&scaled_font->base.scale,
					 &xscale, &yscale, 1);
    xscale = 1.0/xscale;
    yscale = 1.0/yscale;

    extents.x_advance = metricsH.deviceAdvance.x * xscale;
    extents.y_advance = 0;

    if (moveProc == NULL) {
        moveProc = NewATSCubicMoveToUPP (_move_to_for_metrics);
        lineProc = NewATSCubicLineToUPP (_line_to_for_metrics);
        curveProc = NewATSCubicCurveToUPP (_curve_to_for_metrics);
        closePathProc = NewATSCubicClosePathUPP (_close_path_for_metrics);
    }

    path = CGPathCreateMutable ();

    



    err = ATSUGlyphGetCubicPaths (scaled_font->style, theGlyph,
				  moveProc, lineProc, curveProc, closePathProc,
				  (void *)path, &callback_err);

    if (err != noErr) {
	


	CGPathRelease (path);
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    rect = CGPathGetBoundingBox (path);

    extents.x_bearing = rect.origin.x * xscale;
    extents.y_bearing = rect.origin.y * yscale;
    extents.width = rect.size.width * xscale;
    extents.height = rect.size.height * yscale;

    _cairo_scaled_glyph_set_metrics (scaled_glyph,
				     &scaled_font->base,
				     &extents);
    CGPathRelease (path);

    return CAIRO_STATUS_SUCCESS;
}

static OSStatus
_move_to (const Float32Point *point,
	  void *callback_data)
{
    cairo_atsui_scaled_path_t *scaled_path = callback_data;
    double x = point->x;
    double y = point->y;
    
    cairo_matrix_transform_point (scaled_path->scale, &x, &y);
    _cairo_path_fixed_close_path (scaled_path->path);
    _cairo_path_fixed_move_to (scaled_path->path,
			       _cairo_fixed_from_double (x),
			       _cairo_fixed_from_double (y));

    return noErr;
}

static OSStatus
_line_to (const Float32Point *point,
	  void *callback_data)
{
    cairo_atsui_scaled_path_t *scaled_path = callback_data;
    double x = point->x;
    double y = point->y;
    
    cairo_matrix_transform_point (scaled_path->scale, &x, &y);

    _cairo_path_fixed_line_to (scaled_path->path,
			       _cairo_fixed_from_double (x),
			       _cairo_fixed_from_double (y));

    return noErr;
}

static OSStatus
_curve_to (const Float32Point *point1,
	   const Float32Point *point2,
	   const Float32Point *point3,
	   void *callback_data)
{
    cairo_atsui_scaled_path_t *scaled_path = callback_data;
    double x1 = point1->x;
    double y1 = point1->y;
    double x2 = point2->x;
    double y2 = point2->y;
    double x3 = point3->x;
    double y3 = point3->y;
    
    cairo_matrix_transform_point (scaled_path->scale, &x1, &y1);
    cairo_matrix_transform_point (scaled_path->scale, &x2, &y2);
    cairo_matrix_transform_point (scaled_path->scale, &x3, &y3);

    _cairo_path_fixed_curve_to (scaled_path->path,
				_cairo_fixed_from_double (x1),
				_cairo_fixed_from_double (y1),
				_cairo_fixed_from_double (x2),
				_cairo_fixed_from_double (y2),
				_cairo_fixed_from_double (x3),
				_cairo_fixed_from_double (y3));

    return noErr;
}

static OSStatus
_close_path (void *callback_data)

{
    cairo_atsui_scaled_path_t *scaled_path = callback_data;

    _cairo_path_fixed_close_path (scaled_path->path);

    return noErr;
}

static cairo_status_t
_cairo_atsui_scaled_font_init_glyph_path (cairo_atsui_font_t *scaled_font,
					  cairo_scaled_glyph_t *scaled_glyph)
{
    static ATSCubicMoveToUPP moveProc = NULL;
    static ATSCubicLineToUPP lineProc = NULL;
    static ATSCubicCurveToUPP curveProc = NULL;
    static ATSCubicClosePathUPP closePathProc = NULL;
    OSStatus err;
    cairo_atsui_scaled_path_t scaled_path;
    cairo_matrix_t *font_to_device = &scaled_font->base.scale;
    cairo_matrix_t unscaled_font_to_device;
    double xscale;
    double yscale;
    
    
    _cairo_matrix_compute_scale_factors (font_to_device, &xscale, &yscale, 1);
    cairo_matrix_init (&unscaled_font_to_device, 
		      font_to_device->xx, 
		      font_to_device->yx, 
		      font_to_device->xy, 
		      font_to_device->yy, 0., 0.);
    cairo_matrix_scale (&unscaled_font_to_device, 1.0/xscale, 1.0/yscale);

    scaled_path.scale = &unscaled_font_to_device;
    scaled_path.path = _cairo_path_fixed_create ();
    if (!scaled_path.path)
	return CAIRO_STATUS_NO_MEMORY;

    if (moveProc == NULL) {
        moveProc = NewATSCubicMoveToUPP(_move_to);
        lineProc = NewATSCubicLineToUPP(_line_to);
        curveProc = NewATSCubicCurveToUPP(_curve_to);
        closePathProc = NewATSCubicClosePathUPP(_close_path);
    }

    err = ATSUGlyphGetCubicPaths(scaled_font->style,
				 _cairo_scaled_glyph_index (scaled_glyph),
				 moveProc,
				 lineProc,
				 curveProc,
				 closePathProc, (void *)&scaled_path, &err);

    _cairo_scaled_glyph_set_path (scaled_glyph, &scaled_font->base, 
				  scaled_path.path);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_atsui_scaled_font_init_glyph_surface (cairo_atsui_font_t *scaled_font,
					     cairo_scaled_glyph_t *scaled_glyph)
{
    OSStatus err;
    CGContextRef drawingContext;
    cairo_image_surface_t *surface;
    cairo_format_t format;

    ATSFontRef atsFont;
    CGFontRef cgFont;
    cairo_scaled_font_t base = scaled_font->base;
    cairo_font_extents_t extents = base.extents;

    GlyphID theGlyph = _cairo_scaled_glyph_index (scaled_glyph);
    ATSGlyphScreenMetrics metricsH;
    double left, bottom, width, height;
    double xscale, yscale;
    CGRect bbox;
    CGAffineTransform transform;


    



    height = extents.ascent + extents.descent + 2.0;
    bottom = -extents.descent - 1.0;

    




    err = ATSUGlyphGetScreenMetrics (scaled_font->style,
				     1, &theGlyph, 0, false, 
				     false, &metricsH);    
    left = metricsH.sideBearing.x - 1.0;
    width = metricsH.deviceAdvance.x 
	- metricsH.sideBearing.x 
	+ metricsH.otherSideBearing.x + 2.0;

    




    transform = CGAffineTransformMake (base.scale.xx, 
				      -base.scale.yx, 
				      -base.scale.xy, 
				      base.scale.yy, 
				      0., 0.);
    _cairo_matrix_compute_scale_factors (&base.scale, 
					&xscale, &yscale, 1);
    transform = CGAffineTransformScale (transform, 1.0/xscale, 1.0/yscale);

    


    bbox = CGRectApplyAffineTransform (CGRectMake (left, bottom, 
						   width, height), transform);
    


    bbox = CGRectIntegral (bbox);

    left = CGRectGetMinX (bbox);
    bottom = CGRectGetMinY (bbox);

    
    format = CAIRO_FORMAT_A8;

    
    surface = (cairo_image_surface_t *)cairo_image_surface_create (format, bbox.size.width, bbox.size.height);
    if (!surface)
	return CAIRO_STATUS_NO_MEMORY;

    drawingContext = CGBitmapContextCreateWithCairoImageSurface (surface);
    if (!drawingContext) {
	cairo_surface_destroy ((cairo_surface_t *)surface);
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }
    
    atsFont = FMGetATSFontRefFromFont (scaled_font->fontID);
    cgFont = CGFontCreateWithPlatformFont (&atsFont);

    CGContextSetFont (drawingContext, cgFont);

    if (base.options.antialias ==  CAIRO_ANTIALIAS_NONE) {
    	CGContextSetShouldAntialias (drawingContext, false);
    }

    
    CGContextSetRGBFillColor (drawingContext, 1.0, 1.0, 1.0, 1.0);

    CGContextSetFontSize (drawingContext, 1.0);
    CGContextTranslateCTM (drawingContext, -left, -bottom);
    CGContextScaleCTM (drawingContext, xscale, yscale);
    CGContextSetTextMatrix (drawingContext, transform);
    CGContextShowGlyphsAtPoint (drawingContext, 0, 0,  
				&theGlyph, 1);

    CGContextRelease (drawingContext);

    


    cairo_surface_set_device_offset ((cairo_surface_t *)surface, left, 
				    -bbox.size.height - bottom);
    _cairo_scaled_glyph_set_surface (scaled_glyph,
				     &base,
				     surface);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_atsui_font_scaled_glyph_init (void			*abstract_font,
				     cairo_scaled_glyph_t	*scaled_glyph,
				     cairo_scaled_glyph_info_t	 info)
{
    cairo_atsui_font_t *scaled_font = abstract_font;
    cairo_status_t status;

    if ((info & CAIRO_SCALED_GLYPH_INFO_METRICS) != 0) {
      status = _cairo_atsui_font_init_glyph_metrics (scaled_font, scaled_glyph);
      if (status)
	return status;
    }

    if ((info & CAIRO_SCALED_GLYPH_INFO_PATH) != 0) {
	status = _cairo_atsui_scaled_font_init_glyph_path (scaled_font, scaled_glyph);
	if (status)
	    return status;
    }

    if ((info & CAIRO_SCALED_GLYPH_INFO_SURFACE) != 0) {
	status = _cairo_atsui_scaled_font_init_glyph_surface (scaled_font, scaled_glyph);
	if (status)
	    return status;
    }

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_atsui_font_text_to_glyphs (void		*abstract_font,
				  double	 x,
				  double	 y,
				  const char	*utf8,
				  cairo_glyph_t **glyphs,
				  int		*num_glyphs)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;
    uint16_t *utf16;
    int n16;
    OSStatus err;
    ATSUTextLayout textLayout;
    ATSLayoutRecord *layoutRecords;
    cairo_atsui_font_t *font = abstract_font;
    ItemCount glyphCount;
    int i;

    status = _cairo_utf8_to_utf16 ((unsigned char *)utf8, -1, &utf16, &n16);
    if (status)
	return status;

    err = ATSUCreateTextLayout(&textLayout);

    err = ATSUSetTextPointerLocation(textLayout, utf16, 0, n16, n16);

    
    err = ATSUSetRunStyle(textLayout,
			  font->style, kATSUFromTextBeginning, kATSUToTextEnd);

    err = ATSUDirectGetLayoutDataArrayPtrFromTextLayout(textLayout,
							0,
							kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
							(void *)&layoutRecords,
							&glyphCount);

    *num_glyphs = glyphCount - 1;
    *glyphs =
	(cairo_glyph_t *) malloc(*num_glyphs * (sizeof (cairo_glyph_t)));
    if (*glyphs == NULL) {
	return CAIRO_STATUS_NO_MEMORY;
    }

    for (i = 0; i < *num_glyphs; i++) {
	(*glyphs)[i].index = layoutRecords[i].glyphID;
	(*glyphs)[i].x = x + FixedToFloat(layoutRecords[i].realPos);
	(*glyphs)[i].y = y;
    }

    free (utf16);

    ATSUDirectReleaseLayoutDataArrayPtr(NULL,
					kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
					(void *) &layoutRecords);
    ATSUDisposeTextLayout(textLayout);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_atsui_font_old_show_glyphs (void		       *abstract_font,
				   cairo_operator_t    	op,
				   cairo_pattern_t     *pattern,
				   cairo_surface_t     *generic_surface,
				   int                 	source_x,
				   int                 	source_y,
				   int			dest_x,
				   int			dest_y,
				   unsigned int		width,
				   unsigned int		height,
				   cairo_glyph_t       *glyphs,
				   int                 	num_glyphs)
{
    cairo_atsui_font_t *font = abstract_font;
    CGContextRef drawingContext;
    cairo_image_surface_t *destImageSurface;
    int i;
    void *extra = NULL;
    cairo_bool_t can_draw_directly;
    cairo_rectangle_int16_t rect;
    cairo_quartz_surface_t *surface = (cairo_quartz_surface_t *)generic_surface;

    ATSFontRef atsFont;
    CGFontRef cgFont;
    CGAffineTransform textTransform;

    if (!_cairo_surface_is_quartz (generic_surface))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    
    can_draw_directly = _cairo_pattern_is_opaque_solid (pattern) &&
	op == CAIRO_OPERATOR_OVER;

    if (!can_draw_directly) {
	rect.x = dest_x;
	rect.y = dest_y;
	rect.width = width;
	rect.height = height;

	_cairo_surface_acquire_dest_image(generic_surface,
					  &rect,
					  &destImageSurface,
					  &rect,
					  &extra);

	drawingContext = CGBitmapContextCreateWithCairoImageSurface (destImageSurface);
	if (!drawingContext) 
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	CGContextTranslateCTM(drawingContext, 0, destImageSurface->height);
	CGContextScaleCTM(drawingContext, 1.0f, -1.0f);
    } else {
	drawingContext = ((cairo_quartz_surface_t *)generic_surface)->context;
	CGContextSaveGState (drawingContext);
    }

    atsFont = FMGetATSFontRefFromFont(font->fontID);
    cgFont = CGFontCreateWithPlatformFont(&atsFont);

    CGContextSetFont(drawingContext, cgFont);

    if (font->base.options.antialias ==  CAIRO_ANTIALIAS_NONE) {
	CGContextSetShouldAntialias (drawingContext, false);
    }

    textTransform = CGAffineTransformMakeWithCairoFontScale(&font->base.scale);
    textTransform = CGAffineTransformScale(textTransform, 1.0f, -1.0f);

    CGContextSetFontSize(drawingContext, 1.0);
    CGContextSetTextMatrix(drawingContext, textTransform);

    if (pattern->type == CAIRO_PATTERN_TYPE_SOLID &&
	_cairo_pattern_is_opaque_solid(pattern))
    {
	cairo_solid_pattern_t *solid = (cairo_solid_pattern_t *)pattern;
	CGContextSetRGBFillColor(drawingContext,
				 solid->color.red,
				 solid->color.green,
				 solid->color.blue, 1.0f);
    } else {
	CGContextSetRGBFillColor(drawingContext, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    if (surface->clip_region) {
	pixman_box16_t *boxes = pixman_region_rects (surface->clip_region);
	int num_boxes = pixman_region_num_rects (surface->clip_region);
	CGRect stack_rects[10];
	CGRect *rects;
	int i;
	
	



	if (num_boxes > 10)
	    rects = malloc (sizeof (CGRect) * num_boxes);
	else
	    rects = stack_rects;
	
	for (i = 0; i < num_boxes; i++) {
	    rects[i].origin.x = boxes[i].x1;
	    rects[i].origin.y = boxes[i].y1;
	    rects[i].size.width = boxes[i].x2 - boxes[i].x1;
	    rects[i].size.height = boxes[i].y2 - boxes[i].y1;
	}
	
	CGContextClipToRects (drawingContext, rects, num_boxes);
	
	if (rects != stack_rects)
	    free(rects);
    }

    






    for (i = 0; i < num_glyphs; i++) {
        CGGlyph theGlyph = glyphs[i].index;

	
	
        CGContextShowGlyphsAtPoint(drawingContext,
				   _cairo_lround (glyphs[i].x),
				   _cairo_lround (glyphs[i].y),
                                   &theGlyph, 1);
    }

    if (!can_draw_directly) {
	CGContextRelease(drawingContext);

	_cairo_surface_release_dest_image(generic_surface,
					  &rect,
					  destImageSurface,
					  &rect,
					  extra);
    } else {
      CGContextRestoreGState (drawingContext);
    }

    return CAIRO_STATUS_SUCCESS;
}

cairo_bool_t
_cairo_scaled_font_is_atsui (cairo_scaled_font_t *sfont)
{
    return (sfont->backend == &cairo_atsui_scaled_font_backend);
}

ATSUStyle
_cairo_atsui_scaled_font_get_atsu_style (cairo_scaled_font_t *sfont)
{
    cairo_atsui_font_t *afont = (cairo_atsui_font_t *) sfont;

    return afont->style;
}

ATSUFontID
_cairo_atsui_scaled_font_get_atsu_font_id (cairo_scaled_font_t *sfont)
{
    cairo_atsui_font_t *afont = (cairo_atsui_font_t *) sfont;

    return afont->fontID;
}

const cairo_scaled_font_backend_t cairo_atsui_scaled_font_backend = {
    CAIRO_FONT_TYPE_ATSUI,
    _cairo_atsui_font_create_toy,
    _cairo_atsui_font_fini,
    _cairo_atsui_font_scaled_glyph_init,
    _cairo_atsui_font_text_to_glyphs,
    NULL, 
    _cairo_atsui_font_old_show_glyphs,
};

