





































#define WIN32_LEAN_AND_MEAN

#if !defined(WINVER) || (WINVER < 0x0500)
# define WINVER 0x0500
#endif
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
# define _WIN32_WINNT 0x0500
#endif

#include "cairoint.h"

#include "cairo-clip-private.h"
#include "cairo-composite-rectangles-private.h"
#include "cairo-error-private.h"
#include "cairo-paginated-private.h"
#include "cairo-win32-private.h"
#include "cairo-scaled-font-subsets-private.h"
#include "cairo-surface-fallback-private.h"
#include "cairo-surface-clipper-private.h"
#include "cairo-gstate-private.h"
#include "cairo-private.h"
#include <wchar.h>
#include <windows.h>
#include <d3d9.h>

#if defined(__MINGW32__) && !defined(ETO_PDY)
# define ETO_PDY 0x2000
#endif

#undef DEBUG_COMPOSITE


#ifndef SHADEBLENDCAPS
#define SHADEBLENDCAPS  120
#endif
#ifndef SB_NONE
#define SB_NONE         0x00000000
#endif

#define PELS_72DPI  ((LONG)(72. / 0.0254))

























static const cairo_surface_backend_t cairo_win32_surface_backend;










cairo_status_t
_cairo_win32_print_gdi_error (const char *context)
{
    void *lpMsgBuf;
    DWORD last_error = GetLastError ();

    if (!FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER |
			 FORMAT_MESSAGE_FROM_SYSTEM,
			 NULL,
			 last_error,
			 MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
			 (LPWSTR) &lpMsgBuf,
			 0, NULL)) {
	fprintf (stderr, "%s: Unknown GDI error", context);
    } else {
	fwprintf (stderr, L"%s: %S", context, (wchar_t *)lpMsgBuf);

	LocalFree (lpMsgBuf);
    }
    fflush(stderr);

    




    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
}

uint32_t
_cairo_win32_flags_for_dc (HDC dc)
{
    uint32_t flags = 0;

    if (GetDeviceCaps(dc, TECHNOLOGY) == DT_RASDISPLAY) {
	flags |= CAIRO_WIN32_SURFACE_IS_DISPLAY;

	





	flags |= CAIRO_WIN32_SURFACE_CAN_BITBLT;
	flags |= CAIRO_WIN32_SURFACE_CAN_ALPHABLEND;
	flags |= CAIRO_WIN32_SURFACE_CAN_STRETCHBLT;
	flags |= CAIRO_WIN32_SURFACE_CAN_STRETCHDIB;
    } else {
	int cap;

	cap = GetDeviceCaps(dc, SHADEBLENDCAPS);
	if (cap != SB_NONE)
	    flags |= CAIRO_WIN32_SURFACE_CAN_ALPHABLEND;

	cap = GetDeviceCaps(dc, RASTERCAPS);
	if (cap & RC_BITBLT)
	    flags |= CAIRO_WIN32_SURFACE_CAN_BITBLT;
	if (cap & RC_STRETCHBLT)
	    flags |= CAIRO_WIN32_SURFACE_CAN_STRETCHBLT;
	if (cap & RC_STRETCHDIB)
	    flags |= CAIRO_WIN32_SURFACE_CAN_STRETCHDIB;
    }

    return flags;
}

static cairo_status_t
_create_dc_and_bitmap (cairo_win32_surface_t *surface,
		       HDC                    original_dc,
		       cairo_format_t         format,
		       int                    width,
		       int                    height,
		       unsigned char        **bits_out,
		       int                   *rowstride_out)
{
    cairo_status_t status;

    BITMAPINFO *bitmap_info = NULL;
    struct {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[2];
    } bmi_stack;
    void *bits;

    int num_palette = 0;	
    int i;

    surface->dc = NULL;
    surface->bitmap = NULL;
    surface->is_dib = FALSE;

    switch (format) {
    default:
    case CAIRO_FORMAT_INVALID:
	return _cairo_error (CAIRO_STATUS_INVALID_FORMAT);
    case CAIRO_FORMAT_ARGB32:
    case CAIRO_FORMAT_RGB24:
	num_palette = 0;
	break;

    case CAIRO_FORMAT_A8:
	num_palette = 256;
	break;

    case CAIRO_FORMAT_A1:
	num_palette = 2;
	break;
    }

    if (num_palette > 2) {
	bitmap_info = _cairo_malloc_ab_plus_c (num_palette, sizeof(RGBQUAD), sizeof(BITMAPINFOHEADER));
	if (!bitmap_info)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    } else {
	bitmap_info = (BITMAPINFO *)&bmi_stack;
    }

    bitmap_info->bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    bitmap_info->bmiHeader.biWidth = width == 0 ? 1 : width;
    bitmap_info->bmiHeader.biHeight = height == 0 ? -1 : - height; 
    bitmap_info->bmiHeader.biSizeImage = 0;
    bitmap_info->bmiHeader.biXPelsPerMeter = PELS_72DPI; 
    bitmap_info->bmiHeader.biYPelsPerMeter = PELS_72DPI; 
    bitmap_info->bmiHeader.biPlanes = 1;

    switch (format) {
    






    case CAIRO_FORMAT_RGB24:
    case CAIRO_FORMAT_ARGB32:
	bitmap_info->bmiHeader.biBitCount = 32;
	bitmap_info->bmiHeader.biCompression = BI_RGB;
	bitmap_info->bmiHeader.biClrUsed = 0;	
	bitmap_info->bmiHeader.biClrImportant = 0;
	break;

    case CAIRO_FORMAT_A8:
	bitmap_info->bmiHeader.biBitCount = 8;
	bitmap_info->bmiHeader.biCompression = BI_RGB;
	bitmap_info->bmiHeader.biClrUsed = 256;
	bitmap_info->bmiHeader.biClrImportant = 0;

	for (i = 0; i < 256; i++) {
	    bitmap_info->bmiColors[i].rgbBlue = i;
	    bitmap_info->bmiColors[i].rgbGreen = i;
	    bitmap_info->bmiColors[i].rgbRed = i;
	    bitmap_info->bmiColors[i].rgbReserved = 0;
	}

	break;

    case CAIRO_FORMAT_A1:
	bitmap_info->bmiHeader.biBitCount = 1;
	bitmap_info->bmiHeader.biCompression = BI_RGB;
	bitmap_info->bmiHeader.biClrUsed = 2;
	bitmap_info->bmiHeader.biClrImportant = 0;

	for (i = 0; i < 2; i++) {
	    bitmap_info->bmiColors[i].rgbBlue = i * 255;
	    bitmap_info->bmiColors[i].rgbGreen = i * 255;
	    bitmap_info->bmiColors[i].rgbRed = i * 255;
	    bitmap_info->bmiColors[i].rgbReserved = 0;
	}

	break;
    }

    surface->dc = CreateCompatibleDC (original_dc);
    if (!surface->dc)
	goto FAIL;

    surface->bitmap = CreateDIBSection (surface->dc,
			                bitmap_info,
			                DIB_RGB_COLORS,
			                &bits,
			                NULL, 0);
    if (!surface->bitmap)
	goto FAIL;

    surface->is_dib = TRUE;

    GdiFlush();

    surface->saved_dc_bitmap = SelectObject (surface->dc,
					     surface->bitmap);
    if (!surface->saved_dc_bitmap)
	goto FAIL;

    if (bitmap_info && num_palette > 2)
	free (bitmap_info);

    if (bits_out)
	*bits_out = bits;

    if (rowstride_out) {
	
	switch (format) {
	case CAIRO_FORMAT_ARGB32:
	case CAIRO_FORMAT_RGB24:
	    *rowstride_out = 4 * width;
	    break;

	case CAIRO_FORMAT_A8:
	    *rowstride_out = (width + 3) & ~3;
	    break;

	case CAIRO_FORMAT_A1:
	    *rowstride_out = ((width + 31) & ~31) / 8;
	    break;
	}
    }

    surface->flags = _cairo_win32_flags_for_dc (surface->dc);

    return CAIRO_STATUS_SUCCESS;

 FAIL:
    status = _cairo_win32_print_gdi_error ("_create_dc_and_bitmap");

    if (bitmap_info && num_palette > 2)
	free (bitmap_info);

    if (surface->saved_dc_bitmap) {
	SelectObject (surface->dc, surface->saved_dc_bitmap);
	surface->saved_dc_bitmap = NULL;
    }

    if (surface->bitmap) {
	DeleteObject (surface->bitmap);
	surface->bitmap = NULL;
    }

    if (surface->dc) {
 	DeleteDC (surface->dc);
	surface->dc = NULL;
    }

    return status;
}

static cairo_surface_t *
_cairo_win32_surface_create_for_dc (HDC             original_dc,
				    cairo_format_t  format,
				    int	            width,
				    int	            height)
{
    cairo_status_t status;
    cairo_win32_surface_t *surface;
    unsigned char *bits;
    int rowstride;

    if (! CAIRO_FORMAT_VALID (format))
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_INVALID_FORMAT));

    surface = malloc (sizeof (cairo_win32_surface_t));
    if (surface == NULL)
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));

    surface->clip_region = NULL;

    status = _create_dc_and_bitmap (surface, original_dc, format,
				    width, height,
				    &bits, &rowstride);
    if (status)
	goto FAIL;

    surface->image = cairo_image_surface_create_for_data (bits, format,
							  width, height, rowstride);
    status = surface->image->status;
    if (status)
	goto FAIL;

    surface->format = format;
    surface->d3d9surface = NULL;

    surface->clip_rect.x = 0;
    surface->clip_rect.y = 0;
    surface->clip_rect.width = width;
    surface->clip_rect.height = height;

    surface->initial_clip_rgn = NULL;
    surface->had_simple_clip = FALSE;

    surface->extents = surface->clip_rect;
    surface->font_subsets = NULL;

    _cairo_surface_init (&surface->base,
			 &cairo_win32_surface_backend,
			 NULL, 
			 _cairo_content_from_format (format));

    return &surface->base;

 FAIL:
    if (surface->bitmap) {
	SelectObject (surface->dc, surface->saved_dc_bitmap);
	DeleteObject (surface->bitmap);
	DeleteDC (surface->dc);
    }
    free (surface);

    return _cairo_surface_create_in_error (status);
}

static cairo_surface_t *
_cairo_win32_surface_create_similar_internal (void	    *abstract_src,
					      cairo_content_t content,
					      int	     width,
					      int	     height,
					      cairo_bool_t   force_dib)
{
    cairo_win32_surface_t *src = abstract_src;
    cairo_format_t format = _cairo_format_from_content (content);
    cairo_surface_t *new_surf = NULL;

    







    if (src->is_dib ||
	(content & CAIRO_CONTENT_ALPHA) ||
	src->base.backend->type == CAIRO_SURFACE_TYPE_WIN32_PRINTING)
    {
	force_dib = TRUE;
    }

    if (!force_dib) {
	
	new_surf = cairo_win32_surface_create_with_ddb (src->dc, CAIRO_FORMAT_RGB24, width, height);

	if (new_surf->status != CAIRO_STATUS_SUCCESS)
	    new_surf = NULL;
    }

    if (new_surf == NULL) {
	new_surf = _cairo_win32_surface_create_for_dc (src->dc, format, width, height);
    }

    return new_surf;
}

cairo_surface_t *
_cairo_win32_surface_create_similar (void	    *abstract_src,
				     cairo_content_t content,
				     int	     width,
				     int	     height)
{
    return _cairo_win32_surface_create_similar_internal (abstract_src, content, width, height, FALSE);
}

cairo_status_t
_cairo_win32_surface_finish (void *abstract_surface)
{
    cairo_win32_surface_t *surface = abstract_surface;

    if (surface->image)
	cairo_surface_destroy (surface->image);

    
    if (surface->bitmap) {
	SelectObject (surface->dc, surface->saved_dc_bitmap);
	DeleteObject (surface->bitmap);
	DeleteDC (surface->dc);
    } else {
	_cairo_win32_restore_initial_clip (surface);
    }

    if (surface->d3d9surface) {
        IDirect3DSurface9_ReleaseDC (surface->d3d9surface, surface->dc);
        IDirect3DSurface9_Release (surface->d3d9surface);
    }

    if (surface->initial_clip_rgn)
	DeleteObject (surface->initial_clip_rgn);

    if (surface->font_subsets != NULL)
	_cairo_scaled_font_subsets_destroy (surface->font_subsets);

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_win32_surface_d3d9_lock_rect (cairo_win32_surface_t  *surface,
				   int                     x,
				   int                     y,
				   int                     width,
				   int                     height,
				   cairo_image_surface_t **local_out)
{
    cairo_image_surface_t *local;
    cairo_int_status_t status;

    RECT rectin = { x, y, x+width, y+height };
    D3DLOCKED_RECT rectout;
    HRESULT hr;
    hr = IDirect3DSurface9_ReleaseDC (surface->d3d9surface, surface->dc);
    hr = IDirect3DSurface9_LockRect (surface->d3d9surface,
	                             &rectout, &rectin, 0);
    surface->dc = 0; 
    if (hr) {
        IDirect3DSurface9_GetDC (surface->d3d9surface, &surface->dc);
        return CAIRO_INT_STATUS_UNSUPPORTED;
    }
    local = cairo_image_surface_create_for_data (rectout.pBits,
	                                         surface->format,
						 width, height,
						 rectout.Pitch);
    if (local == NULL) {
	IDirect3DSurface9_UnlockRect (surface->d3d9surface);
	IDirect3DSurface9_GetDC (surface->d3d9surface, &surface->dc);
        return CAIRO_INT_STATUS_UNSUPPORTED;
    }
    if (local->base.status) {
	IDirect3DSurface9_UnlockRect (surface->d3d9surface);
	IDirect3DSurface9_GetDC (surface->d3d9surface, &surface->dc);
        return local->base.status;
    }

    *local_out = local;

    return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
_cairo_win32_surface_get_subimage (cairo_win32_surface_t  *surface,
				   int                     x,
				   int                     y,
				   int                     width,
				   int                     height,
				   cairo_win32_surface_t **local_out)
{
    cairo_win32_surface_t *local;
    cairo_int_status_t status;
    cairo_content_t content = _cairo_content_from_format (surface->format);

    local =
	(cairo_win32_surface_t *) _cairo_win32_surface_create_similar_internal
	(surface, content, width, height, TRUE);
    if (local == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    if (local->base.status)
	return local->base.status;

    status = CAIRO_INT_STATUS_UNSUPPORTED;

    
    if ((surface->flags & CAIRO_WIN32_SURFACE_CAN_BITBLT) &&
	BitBlt (local->dc,
		0, 0,
		width, height,
		surface->dc,
		x, y,
		SRCCOPY))
    {
	status = CAIRO_STATUS_SUCCESS;
    }

    if (status) {
	





	RECT r;
	r.left = r.top = 0;
	r.right = width;
	r.bottom = height;
	FillRect(local->dc, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
    }

    *local_out = local;

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_win32_convert_ddb_to_dib (cairo_win32_surface_t *surface)
{
    cairo_win32_surface_t *new_surface;
    int width  = surface->extents.width  + surface->extents.x;
    int height = surface->extents.height + surface->extents.y;

    BOOL ok;
    HBITMAP oldbitmap;

    new_surface = (cairo_win32_surface_t*)
	_cairo_win32_surface_create_for_dc (surface->dc,
					    surface->format,
					    width,
					    height);

    if (new_surface->base.status)
	return;

    
    ok = BitBlt (new_surface->dc,
		 0, 0, width, height,
		 surface->dc,
		 0, 0, SRCCOPY);

    if (!ok)
	goto out;

    

    DeleteDC (new_surface->dc);
    new_surface->dc = NULL;

    oldbitmap = SelectObject (surface->dc, new_surface->bitmap);
    DeleteObject (oldbitmap);

    surface->image = new_surface->image;
    surface->is_dib = new_surface->is_dib;
    surface->bitmap = new_surface->bitmap;

    new_surface->bitmap = NULL;
    new_surface->image = NULL;

    
    surface->flags = _cairo_win32_flags_for_dc (surface->dc);

  out:
    cairo_surface_destroy ((cairo_surface_t*)new_surface);
}

static cairo_status_t
_cairo_win32_surface_acquire_source_image (void                    *abstract_surface,
					   cairo_image_surface_t  **image_out,
					   void                   **image_extra)
{
    cairo_win32_surface_t *surface = abstract_surface;
    cairo_status_t status;

    if (!surface->image && !surface->is_dib && surface->bitmap &&
	(surface->flags & CAIRO_WIN32_SURFACE_CAN_CONVERT_TO_DIB) != 0)
    {
	




	_cairo_win32_convert_ddb_to_dib (surface);
    }

    if (surface->image) {
	*image_out = (cairo_image_surface_t *)surface->image;
	*image_extra = NULL;
	return CAIRO_STATUS_SUCCESS;
    }

    if (surface->d3d9surface) {
	cairo_image_surface_t *local;
	status = _cairo_win32_surface_d3d9_lock_rect (abstract_surface, 0, 0,
						      surface->extents.width,
						      surface->extents.height, &local);
	if (status)
	    return status;

	*image_out = local;
	*image_extra = surface;
    } else {
	cairo_win32_surface_t *local;
	status = _cairo_win32_surface_get_subimage (abstract_surface, 0, 0,
						    surface->extents.width,
						    surface->extents.height, &local);
	if (status)
	    return status;

	*image_out = (cairo_image_surface_t *)local->image;
	*image_extra = local;
    }
    
    

    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_win32_surface_release_source_image (void                   *abstract_surface,
					   cairo_image_surface_t  *image,
					   void                   *image_extra)
{
    cairo_win32_surface_t *surface = abstract_surface;
    cairo_win32_surface_t *local = image_extra;

    if (local && local->d3d9surface) {
	IDirect3DSurface9_UnlockRect (local->d3d9surface);
	IDirect3DSurface9_GetDC (local->d3d9surface, &local->dc);
	cairo_surface_destroy ((cairo_surface_t *)image);
    } else {
	cairo_surface_destroy ((cairo_surface_t *)local);
    }
}

static cairo_status_t
_cairo_win32_surface_acquire_dest_image (void                    *abstract_surface,
					 cairo_rectangle_int_t   *interest_rect,
					 cairo_image_surface_t  **image_out,
					 cairo_rectangle_int_t   *image_rect,
					 void                   **image_extra)
{
    cairo_win32_surface_t *surface = abstract_surface;
    cairo_status_t status;

    if (surface->image) {
	GdiFlush();

	*image_out = (cairo_image_surface_t *) surface->image;
	*image_extra = NULL;
	*image_rect = surface->extents;
	return CAIRO_STATUS_SUCCESS;
    }

    if (surface->d3d9surface) {
	cairo_image_surface_t *local = NULL;
	status = _cairo_win32_surface_d3d9_lock_rect (abstract_surface,
						interest_rect->x,
						interest_rect->y,
						interest_rect->width,
						interest_rect->height, &local);

	if (status)
	    return status;

	*image_out = local;
	*image_extra = surface;
    } else {
	cairo_win32_surface_t *local = NULL;
	status = _cairo_win32_surface_get_subimage (abstract_surface,
						interest_rect->x,
						interest_rect->y,
						interest_rect->width,
						interest_rect->height, &local);

	if (status)
	    return status;

	*image_out = (cairo_image_surface_t *) local->image;
	*image_extra = local;
    }
    
    

    *image_rect = *interest_rect;
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_win32_surface_release_dest_image (void                    *abstract_surface,
					 cairo_rectangle_int_t   *interest_rect,
					 cairo_image_surface_t   *image,
					 cairo_rectangle_int_t   *image_rect,
					 void                    *image_extra)
{
    cairo_win32_surface_t *surface = abstract_surface;
    cairo_win32_surface_t *local = image_extra;

    if (!local)
	return;

    if (local->d3d9surface) {
	IDirect3DSurface9_UnlockRect (local->d3d9surface);
	IDirect3DSurface9_GetDC (local->d3d9surface, &local->dc);
	cairo_surface_destroy ((cairo_surface_t *)image);
    } else {

	

	_cairo_win32_surface_set_clip_region (surface, NULL);

	if (!BitBlt (surface->dc,
		     image_rect->x, image_rect->y,
		     image_rect->width, image_rect->height,
		     local->dc,
		     0, 0,
		     SRCCOPY))
	    _cairo_win32_print_gdi_error ("_cairo_win32_surface_release_dest_image");

	cairo_surface_destroy ((cairo_surface_t *)local);
    }

}

cairo_status_t
_cairo_win32_surface_set_clip_region (void           *abstract_surface,
				      cairo_region_t *region)
{
    cairo_win32_surface_t *surface = abstract_surface;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    if (surface->clip_region == region)
	return CAIRO_STATUS_SUCCESS;

    cairo_region_destroy (surface->clip_region);
    surface->clip_region = cairo_region_reference (region);

    





    
    status = _cairo_win32_restore_initial_clip (surface);

    
    if (region) {
	cairo_rectangle_int_t extents;
	int num_rects;
	RGNDATA *data;
	size_t data_size;
	RECT *rects;
	int i;
	HRGN gdi_region;

	

	cairo_region_get_extents (region, &extents);
	num_rects = cairo_region_num_rectangles (region);
	






	data_size = sizeof (RGNDATAHEADER) + num_rects * sizeof (RECT);
	data = malloc (data_size);
	if (!data)
	    return _cairo_error(CAIRO_STATUS_NO_MEMORY);
	rects = (RECT *)data->Buffer;

	data->rdh.dwSize = sizeof (RGNDATAHEADER);
	data->rdh.iType = RDH_RECTANGLES;
	data->rdh.nCount = num_rects;
	data->rdh.nRgnSize = num_rects * sizeof (RECT);
	data->rdh.rcBound.left = extents.x;
	data->rdh.rcBound.top = extents.y;
	data->rdh.rcBound.right = extents.x + extents.width;
	data->rdh.rcBound.bottom = extents.y + extents.height;

	for (i = 0; i < num_rects; i++) {
	    cairo_rectangle_int_t rect;

	    cairo_region_get_rectangle (region, i, &rect);

	    rects[i].left   = rect.x;
	    rects[i].top    = rect.y;
	    rects[i].right  = rect.x + rect.width;
	    rects[i].bottom = rect.y + rect.height;
	}

	gdi_region = ExtCreateRegion (NULL, data_size, data);
	free (data);

	if (!gdi_region)
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);

	
	if (ExtSelectClipRgn (surface->dc, gdi_region, RGN_AND) == ERROR)
	    status = _cairo_win32_print_gdi_error ("_cairo_win32_surface_set_clip_region");

	DeleteObject (gdi_region);
    }

    return status;
}

#if !defined(AC_SRC_OVER)
#define AC_SRC_OVER                 0x00
#pragma pack(1)
typedef struct {
    BYTE   BlendOp;
    BYTE   BlendFlags;
    BYTE   SourceConstantAlpha;
    BYTE   AlphaFormat;
}BLENDFUNCTION;
#pragma pack()
#endif


#ifndef AC_SRC_ALPHA
#define AC_SRC_ALPHA                0x01
#endif

typedef BOOL (WINAPI *cairo_alpha_blend_func_t) (HDC hdcDest,
						 int nXOriginDest,
						 int nYOriginDest,
						 int nWidthDest,
						 int hHeightDest,
						 HDC hdcSrc,
						 int nXOriginSrc,
						 int nYOriginSrc,
						 int nWidthSrc,
						 int nHeightSrc,
						 BLENDFUNCTION blendFunction);

static cairo_int_status_t
_composite_alpha_blend (cairo_win32_surface_t *dst,
			cairo_win32_surface_t *src,
			int                    alpha,
			int                    src_x,
			int                    src_y,
			int                    src_w,
			int                    src_h,
			int                    dst_x,
			int                    dst_y,
			int                    dst_w,
			int                    dst_h)
{
    static unsigned alpha_blend_checked = FALSE;
    static cairo_alpha_blend_func_t alpha_blend = NULL;

    BLENDFUNCTION blend_function;

    


    if (!alpha_blend_checked) {
	OSVERSIONINFO os;

	os.dwOSVersionInfoSize = sizeof (os);
	GetVersionEx (&os);

	

	if (VER_PLATFORM_WIN32_WINDOWS != os.dwPlatformId ||
	    os.dwMajorVersion != 4 || os.dwMinorVersion != 10)
	{
	    HMODULE msimg32_dll = LoadLibraryW (L"msimg32");

	    if (msimg32_dll != NULL)
		alpha_blend = (cairo_alpha_blend_func_t)GetProcAddress (msimg32_dll,
									"AlphaBlend");
	}

	alpha_blend_checked = TRUE;
    }

    if (alpha_blend == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;
    if (!(dst->flags & CAIRO_WIN32_SURFACE_CAN_ALPHABLEND))
	return CAIRO_INT_STATUS_UNSUPPORTED;
    if (src->format == CAIRO_FORMAT_RGB24 && dst->format == CAIRO_FORMAT_ARGB32)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    blend_function.BlendOp = AC_SRC_OVER;
    blend_function.BlendFlags = 0;
    blend_function.SourceConstantAlpha = alpha;
    blend_function.AlphaFormat = (src->format == CAIRO_FORMAT_ARGB32) ? AC_SRC_ALPHA : 0;

    if (!alpha_blend (dst->dc,
		      dst_x, dst_y,
		      dst_w, dst_h,
		      src->dc,
		      src_x, src_y,
		      src_w, src_h,
		      blend_function))
	return _cairo_win32_print_gdi_error ("_cairo_win32_surface_composite(AlphaBlend)");

    return CAIRO_STATUS_SUCCESS;
}


static void
make_opaque (cairo_image_surface_t *image, cairo_rectangle_int_t src_r)
{
    int x; int y;
    for (y = 0; y < src_r.height; y++) {
        for (x = 0; x < src_r.width; x++) {
            image->data[(src_r.y + y) * image->stride + (src_r.x + x)*4 + 3] = 0xff;
        }
    }
}

static cairo_int_status_t
_cairo_win32_surface_composite_inner (cairo_win32_surface_t *src,
				      cairo_image_surface_t *src_image,
				      cairo_win32_surface_t *dst,
				      cairo_rectangle_int_t src_extents,
				      cairo_rectangle_int_t src_r,
				      cairo_rectangle_int_t dst_r,
				      int alpha,
				      cairo_bool_t needs_alpha,
				      cairo_bool_t needs_scale)
{
    
    if (src_image) {
	if (needs_alpha || needs_scale)
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	if (dst->flags & CAIRO_WIN32_SURFACE_CAN_STRETCHBLT) {
	    BITMAPINFO bi;
	    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	    bi.bmiHeader.biWidth = src_image->width;
	    bi.bmiHeader.biHeight = - src_image->height;
	    bi.bmiHeader.biSizeImage = 0;
	    bi.bmiHeader.biXPelsPerMeter = PELS_72DPI;
	    bi.bmiHeader.biYPelsPerMeter = PELS_72DPI;
	    bi.bmiHeader.biPlanes = 1;
	    bi.bmiHeader.biBitCount = 32;
	    bi.bmiHeader.biCompression = BI_RGB;
	    bi.bmiHeader.biClrUsed = 0;
	    bi.bmiHeader.biClrImportant = 0;

	    






	    if (!StretchDIBits (dst->dc,
				
				dst_r.x, dst_r.y + dst_r.height - 1,
				dst_r.width, - (int) dst_r.height,
				
				src_r.x, src_extents.height - src_r.y + 1,
				src_r.width, - (int) src_r.height,
				src_image->data,
				&bi,
				DIB_RGB_COLORS,
				SRCCOPY))
		return _cairo_win32_print_gdi_error ("_cairo_win32_surface_composite(StretchDIBits)");
	}
    } else if (!needs_alpha) {
	if (src->format == CAIRO_FORMAT_RGB24 && dst->format == CAIRO_FORMAT_ARGB32) {
	    



	    GdiFlush();
	    make_opaque(src->image, src_r);
	}
	
	if (!needs_scale && (dst->flags & CAIRO_WIN32_SURFACE_CAN_BITBLT)) {
            if (!BitBlt (dst->dc,
			 dst_r.x, dst_r.y,
			 dst_r.width, dst_r.height,
			 src->dc,
			 src_r.x, src_r.y,
			 SRCCOPY))
		return _cairo_win32_print_gdi_error ("_cairo_win32_surface_composite(BitBlt)");
	} else if (dst->flags & CAIRO_WIN32_SURFACE_CAN_STRETCHBLT) {
	    
	    
	    BOOL success;
	    int oldmode = SetStretchBltMode(dst->dc, HALFTONE);
	    success = StretchBlt(dst->dc,
				 dst_r.x, dst_r.y,
				 dst_r.width, dst_r.height,
				 src->dc,
				 src_r.x, src_r.y,
				 src_r.width, src_r.height,
				 SRCCOPY);
	    SetStretchBltMode(dst->dc, oldmode);

	    if (!success)
		return _cairo_win32_print_gdi_error ("StretchBlt");
	}
    } else if (needs_alpha && !needs_scale) {
	RECT r = {0, 0, 5000, 5000};
        
  	return _composite_alpha_blend (dst, src, alpha,
				       src_r.x, src_r.y, src_r.width, src_r.height,
				       dst_r.x, dst_r.y, dst_r.width, dst_r.height);
    }

    return CAIRO_STATUS_SUCCESS;
}


#define MOD(a,b) ((a) < 0 ? ((b) - ((-(a) - 1) % (b))) - 1 : (a) % (b))

static cairo_int_status_t
_cairo_win32_surface_composite (cairo_operator_t	op,
				const cairo_pattern_t	*pattern,
				const cairo_pattern_t	*mask_pattern,
				void			*abstract_dst,
				int			src_x,
				int			src_y,
				int			mask_x,
				int			mask_y,
				int			dst_x,
				int			dst_y,
				unsigned int		width,
				unsigned int		height,
				cairo_region_t	       *clip_region)
{
    cairo_win32_surface_t *dst = abstract_dst;
    cairo_win32_surface_t *src;
    cairo_surface_pattern_t *src_surface_pattern;
    int alpha;
    double scalex, scaley;
    cairo_fixed_t x0_fixed, y0_fixed;
    cairo_int_status_t status;

    cairo_bool_t needs_alpha, needs_scale, needs_repeat, needs_pad;
    cairo_image_surface_t *src_image = NULL;

    cairo_format_t src_format;
    cairo_rectangle_int_t src_extents;

    cairo_rectangle_int_t src_r = { src_x, src_y, width, height };
    cairo_rectangle_int_t dst_r = { dst_x, dst_y, width, height };

#ifdef DEBUG_COMPOSITE
    fprintf (stderr, "+++ composite: %d %p %p %p [%d %d] [%d %d] [%d %d] %dx%d\n",
	     op, pattern, mask_pattern, abstract_dst, src_x, src_y, mask_x, mask_y, dst_x, dst_y, width, height);
#endif

    



    if ((dst->flags & (CAIRO_WIN32_SURFACE_CAN_BITBLT |
		       CAIRO_WIN32_SURFACE_CAN_ALPHABLEND |
		       CAIRO_WIN32_SURFACE_CAN_STRETCHBLT |
		       CAIRO_WIN32_SURFACE_CAN_STRETCHDIB))
	== 0)
    {
	goto UNSUPPORTED;
    }

    if (pattern->type != CAIRO_PATTERN_TYPE_SURFACE)
	goto UNSUPPORTED;

    if (pattern->extend != CAIRO_EXTEND_NONE &&
	pattern->extend != CAIRO_EXTEND_REPEAT &&
	pattern->extend != CAIRO_EXTEND_PAD)
	goto UNSUPPORTED;

    if (mask_pattern) {
	


	if (mask_pattern->type != CAIRO_PATTERN_TYPE_SOLID)
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	alpha = ((cairo_solid_pattern_t *)mask_pattern)->color.alpha_short >> 8;
    } else {
	alpha = 255;
    }

    src_surface_pattern = (cairo_surface_pattern_t *)pattern;
    src = (cairo_win32_surface_t *)src_surface_pattern->surface;

    if (src->base.type == CAIRO_SURFACE_TYPE_IMAGE &&
	dst->flags & (CAIRO_WIN32_SURFACE_CAN_STRETCHDIB))
    {
	







	src_image = (cairo_image_surface_t*) src;

	if (src_image->format != CAIRO_FORMAT_RGB24 ||
	    dst->format != CAIRO_FORMAT_RGB24 ||
	    alpha != 255 ||
	    (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_OVER) ||
	    src_image->stride != (src_image->width * 4))
	{
	    goto UNSUPPORTED;
	}

	src_format = src_image->format;
	src_extents.x = 0;
	src_extents.y = 0;
	src_extents.width = src_image->width;
	src_extents.height = src_image->height;
    } else if (src->base.backend != dst->base.backend) {
	goto UNSUPPORTED;
    } else {
	src_format = src->format;
	src_extents = src->extents;
    }


#ifdef DEBUG_COMPOSITE
    fprintf (stderr, "Before check: src size: (%d %d) xy [%d %d] -> dst [%d %d %d %d] {srcmat %f %f %f %f}\n",
	     src_extents.width, src_extents.height,
	     src_x, src_y,
	     dst_x, dst_y, width, height,
	     pattern->matrix.x0, pattern->matrix.y0, pattern->matrix.xx, pattern->matrix.yy);
#endif

    


    x0_fixed = _cairo_fixed_from_double(pattern->matrix.x0 / pattern->matrix.xx);
    y0_fixed = _cairo_fixed_from_double(pattern->matrix.y0 / pattern->matrix.yy);

    if (pattern->matrix.yx != 0.0 ||
	pattern->matrix.xy != 0.0 ||
	!_cairo_fixed_is_integer(x0_fixed) ||
	!_cairo_fixed_is_integer(y0_fixed))
    {
	goto UNSUPPORTED;
    }

    scalex = pattern->matrix.xx;
    scaley = pattern->matrix.yy;

    src_r.x += _cairo_fixed_integer_part(x0_fixed);
    src_r.y += _cairo_fixed_integer_part(y0_fixed);

    
    if (scalex == 0.0 || scaley == 0.0)
	return CAIRO_STATUS_SUCCESS;

    if (scalex != 1.0 || scaley != 1.0)
	goto UNSUPPORTED;

    




#ifdef DEBUG_COMPOSITE
    fprintf (stderr, "before: [%d %d %d %d] -> [%d %d %d %d]\n",
	     src_r.x, src_r.y, src_r.width, src_r.height,
	     dst_r.x, dst_r.y, dst_r.width, dst_r.height);
    fflush (stderr);
#endif

    






    needs_pad = FALSE;
    if (pattern->extend != CAIRO_EXTEND_REPEAT) {
	needs_repeat = FALSE;

	


	if (src_r.x > src_extents.width || src_r.y > src_extents.height ||
	    (src_r.x + src_r.width) < 0 || (src_r.y + src_r.height) < 0)
	{
	    if (op == CAIRO_OPERATOR_OVER)
		return CAIRO_STATUS_SUCCESS;
	    goto UNSUPPORTED;
	}

	if (src_r.x < 0) {
	    src_r.width += src_r.x;

	    dst_r.width += src_r.x;
	    dst_r.x -= src_r.x;

            src_r.x = 0;
            needs_pad = TRUE;
	}

	if (src_r.y < 0) {
	    src_r.height += src_r.y;

	    dst_r.height += src_r.y;
	    dst_r.y -= src_r.y;
	    
            src_r.y = 0;
            needs_pad = TRUE;
	}

	if (src_r.x + src_r.width > src_extents.width) {
	    src_r.width = src_extents.width - src_r.x;
	    dst_r.width = src_r.width;
            needs_pad = TRUE;
	}

	if (src_r.y + src_r.height > src_extents.height) {
	    src_r.height = src_extents.height - src_r.y;
	    dst_r.height = src_r.height;
            needs_pad = TRUE;
	}
    } else {
	needs_repeat = TRUE;
    }

    if (pattern->extend == CAIRO_EXTEND_PAD && needs_pad) {
        goto UNSUPPORTED;
    }

    































    


    if (op == CAIRO_OPERATOR_OVER) {
	if (alpha == 0)
	    return CAIRO_STATUS_SUCCESS;

	if (src_format == dst->format) {
	    if (alpha == 255 && src_format == CAIRO_FORMAT_RGB24) {
		needs_alpha = FALSE;
	    } else {
		needs_alpha = TRUE;
	    }
	} else if (src_format == CAIRO_FORMAT_ARGB32 &&
		   dst->format == CAIRO_FORMAT_RGB24)
	{
	    needs_alpha = TRUE;
	} else if (src_format == CAIRO_FORMAT_RGB24 &&
		   dst->format == CAIRO_FORMAT_ARGB32 &&
		   src->image)
	{
	    if (alpha == 255) {
		needs_alpha = FALSE;
	    } else {
		needs_alpha = TRUE;
	    }
	} else {
	    goto UNSUPPORTED;
	}
    } else if (alpha == 255 && op == CAIRO_OPERATOR_SOURCE) {
	if ((src_format == dst->format) ||
	    (src_format == CAIRO_FORMAT_ARGB32 && dst->format == CAIRO_FORMAT_RGB24) ||
	    (src_format == CAIRO_FORMAT_RGB24  && dst->format == CAIRO_FORMAT_ARGB32 && src->image))
	{
	    needs_alpha = FALSE;
	} else {
	    goto UNSUPPORTED;
	}
    } else {
	goto UNSUPPORTED;
    }

    if (scalex == 1.0 && scaley == 1.0) {
	needs_scale = FALSE;
    } else {
	
	needs_scale = TRUE;
    }

#ifdef DEBUG_COMPOSITE
    fprintf (stderr, "action: [%d %d %d %d] -> [%d %d %d %d]\n",
	     src_r.x, src_r.y, src_r.width, src_r.height,
	     dst_r.x, dst_r.y, dst_r.width, dst_r.height);
    fflush (stderr);
#endif

    status = _cairo_win32_surface_set_clip_region (dst, clip_region);
    if (status)
	return status;

    


    if (needs_repeat) {
	cairo_rectangle_int_t piece_src_r, piece_dst_r;
	uint32_t rendered_width = 0, rendered_height = 0;
	uint32_t to_render_height, to_render_width;
	int32_t piece_x, piece_y;
	int32_t src_start_x = MOD(src_r.x, src_extents.width);
	int32_t src_start_y = MOD(src_r.y, src_extents.height);

	if (needs_scale)
	    goto UNSUPPORTED;

	




	if ((src_image || src->image) && dst->image)
	    goto UNSUPPORTED;

	


	if (src->bitmap == NULL)
	    goto UNSUPPORTED;

	
	if (!src_image && !needs_alpha)
	{
	    HBRUSH brush;
	    HGDIOBJ old_brush;
	    POINT old_brush_origin;

	    
	    brush = CreatePatternBrush (src->bitmap);

	    


	    SetBrushOrgEx (dst->dc, dst_r.x - src_start_x,
			   dst_r.y - src_start_y, &old_brush_origin);

	    old_brush = SelectObject (dst->dc, brush);

	    PatBlt (dst->dc, dst_r.x, dst_r.y, dst_r.width, dst_r.height, PATCOPY);

	    
	    SetBrushOrgEx (dst->dc, old_brush_origin.x, old_brush_origin.y, NULL);
	    SelectObject (dst->dc, old_brush);
	    DeleteObject (brush);

	    return CAIRO_STATUS_SUCCESS;
	}

	

	



	if (dst_r.width / src_extents.width > 5 ||
	    dst_r.height / src_extents.height > 5)
	    goto UNSUPPORTED;

	for (rendered_height = 0;
	     rendered_height < dst_r.height;
	     rendered_height += to_render_height)
	{
	    piece_y = (src_start_y + rendered_height) % src_extents.height;
	    to_render_height = src_extents.height - piece_y;

	    if (rendered_height + to_render_height > dst_r.height)
		to_render_height = dst_r.height - rendered_height;

	    for (rendered_width = 0;
		 rendered_width < dst_r.width;
		 rendered_width += to_render_width)
	    {
		piece_x = (src_start_x + rendered_width) % src_extents.width;
		to_render_width = src_extents.width - piece_x;

		if (rendered_width + to_render_width > dst_r.width)
		    to_render_width = dst_r.width - rendered_width;

		piece_src_r.x = piece_x;
		piece_src_r.y = piece_y;
		piece_src_r.width = to_render_width;
		piece_src_r.height = to_render_height;

		piece_dst_r.x = dst_r.x + rendered_width;
		piece_dst_r.y = dst_r.y + rendered_height;
		piece_dst_r.width = to_render_width;
		piece_dst_r.height = to_render_height;

		status = _cairo_win32_surface_composite_inner (src, src_image, dst,
							       src_extents, piece_src_r, piece_dst_r,
							       alpha, needs_alpha, needs_scale);
		if (status != CAIRO_STATUS_SUCCESS) {
		    



		    if (rendered_width == 0 &&
			rendered_height == 0)
		    {
			goto UNSUPPORTED;
		    }

		    return status;
		}
	    }
	}
    } else {
	status = _cairo_win32_surface_composite_inner (src, src_image, dst,
						       src_extents, src_r, dst_r,
						       alpha, needs_alpha, needs_scale);
    }

    if (status == CAIRO_STATUS_SUCCESS)
	return status;

UNSUPPORTED:
    
    if (dst->image) {
	GdiFlush();

	return dst->image->backend->composite (op, pattern, mask_pattern,
					       dst->image,
					       src_x, src_y,
					       mask_x, mask_y,
					       dst_x, dst_y,
					       width, height,
					       clip_region);
    }

    return CAIRO_INT_STATUS_UNSUPPORTED;
}







static enum { DO_CLEAR, DO_SOURCE, DO_NOTHING, DO_UNSUPPORTED }
categorize_solid_dest_operator (cairo_operator_t op,
				unsigned short   alpha)
{
    enum { SOURCE_TRANSPARENT, SOURCE_LIGHT, SOURCE_SOLID, SOURCE_OTHER } source;

    if (alpha >= 0xff00)
	source = SOURCE_SOLID;
    else if (alpha < 0x100)
	source = SOURCE_TRANSPARENT;
    else
	source = SOURCE_OTHER;

    switch (op) {
    case CAIRO_OPERATOR_CLEAR:    
    case CAIRO_OPERATOR_OUT:      
	return DO_CLEAR;
	break;

    case CAIRO_OPERATOR_SOURCE:   
    case CAIRO_OPERATOR_IN:       
	return DO_SOURCE;
	break;

    case CAIRO_OPERATOR_OVER:     
    case CAIRO_OPERATOR_ATOP:     
	if (source == SOURCE_SOLID)
	    return DO_SOURCE;
	else if (source == SOURCE_TRANSPARENT)
	    return DO_NOTHING;
	else
	    return DO_UNSUPPORTED;
	break;

    case CAIRO_OPERATOR_DEST_OUT: 
    case CAIRO_OPERATOR_XOR:      
	if (source == SOURCE_SOLID)
	    return DO_CLEAR;
	else if (source == SOURCE_TRANSPARENT)
	    return DO_NOTHING;
	else
	    return DO_UNSUPPORTED;
    	break;

    case CAIRO_OPERATOR_DEST:     
    case CAIRO_OPERATOR_DEST_OVER:
    case CAIRO_OPERATOR_SATURATE: 
	return DO_NOTHING;
	break;

    case CAIRO_OPERATOR_DEST_IN:  
    case CAIRO_OPERATOR_DEST_ATOP:
	if (source == SOURCE_SOLID)
	    return DO_NOTHING;
	else if (source == SOURCE_TRANSPARENT)
	    return DO_CLEAR;
	else
	    return DO_UNSUPPORTED;
	break;

    case CAIRO_OPERATOR_ADD:	  
	if (source == SOURCE_TRANSPARENT)
	    return DO_NOTHING;
	else
	    return DO_UNSUPPORTED;
	break;
    }

    ASSERT_NOT_REACHED;
    return DO_UNSUPPORTED;
}

static cairo_status_t
_cairo_win32_surface_fill_rectangles_stretchdib (HDC                   dc,
                                                 const cairo_color_t   *color,
                                                 cairo_rectangle_int_t *rect,
                                                 int                   num_rects)
{
    BITMAPINFO bi;
    int pixel = ((color->alpha_short >> 8) << 24) |
                ((color->red_short >> 8) << 16) |
                ((color->green_short >> 8) << 8) |
                (color->blue_short >> 8);
    int i;

    


    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = 1;
    bi.bmiHeader.biHeight = 1;
    bi.bmiHeader.biSizeImage = 0;
    bi.bmiHeader.biXPelsPerMeter = PELS_72DPI;
    bi.bmiHeader.biYPelsPerMeter = PELS_72DPI;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;

    for (i = 0; i < num_rects; i++) {
        if (!StretchDIBits (dc,
                            
                            rect[i].x, rect[i].y, rect[i].width, rect[i].height,
                            
                            0, 0, 1, 1,
                            &pixel,
                            &bi,
                            DIB_RGB_COLORS,
                            SRCCOPY))
            return _cairo_win32_print_gdi_error ("_cairo_win32_surface_fill_rectangles_stretchdib(StretchDIBits)");
    }
    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_win32_surface_fill_rectangles (void			*abstract_surface,
				      cairo_operator_t		op,
				      const cairo_color_t	*color,
				      cairo_rectangle_int_t	*rects,
				      int			num_rects)
{
    cairo_win32_surface_t *surface = abstract_surface;
    cairo_status_t status;
    COLORREF new_color;
    HBRUSH new_brush;
    int i;

    status = _cairo_win32_surface_set_clip_region (surface, NULL);
    if (status)
        return status;

    if (surface->format == CAIRO_FORMAT_ARGB32 &&
        (surface->flags & CAIRO_WIN32_SURFACE_CAN_STRETCHDIB) &&
        (op == CAIRO_OPERATOR_SOURCE ||
         (op == CAIRO_OPERATOR_OVER && color->alpha_short >= 0xff00))) {
        return _cairo_win32_surface_fill_rectangles_stretchdib (surface->dc,
                                                                color, rects, num_rects);
    }
    if (surface->format != CAIRO_FORMAT_RGB24)
        return CAIRO_INT_STATUS_UNSUPPORTED;

    


    switch (categorize_solid_dest_operator (op, color->alpha_short)) {
    case DO_CLEAR:
	new_color = RGB (0, 0, 0);
	break;
    case DO_SOURCE:
	new_color = RGB (color->red_short >> 8, color->green_short >> 8, color->blue_short >> 8);
	break;
    case DO_NOTHING:
	return CAIRO_STATUS_SUCCESS;
    case DO_UNSUPPORTED:
    default:
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    new_brush = CreateSolidBrush (new_color);
    if (!new_brush)
	return _cairo_win32_print_gdi_error ("_cairo_win32_surface_fill_rectangles");

    for (i = 0; i < num_rects; i++) {
	RECT rect;

	rect.left = rects[i].x;
	rect.top = rects[i].y;
	rect.right = rects[i].x + rects[i].width;
	rect.bottom = rects[i].y + rects[i].height;

	if (!FillRect (surface->dc, &rect, new_brush))
	    goto FAIL;
    }

    DeleteObject (new_brush);

    return CAIRO_STATUS_SUCCESS;

 FAIL:
    status = _cairo_win32_print_gdi_error ("_cairo_win32_surface_fill_rectangles");

    DeleteObject (new_brush);

    return status;
}

cairo_bool_t
_cairo_win32_surface_get_extents (void		          *abstract_surface,
				  cairo_rectangle_int_t   *rectangle)
{
    cairo_win32_surface_t *surface = abstract_surface;

    *rectangle = surface->extents;
    return TRUE;
}

static cairo_status_t
_cairo_win32_surface_flush (void *abstract_surface)
{
    return _cairo_win32_surface_set_clip_region (abstract_surface, NULL);
}

#define STACK_GLYPH_SIZE 256

cairo_int_status_t
_cairo_win32_surface_show_glyphs_internal (void			*surface,
					   cairo_operator_t	 op,
					   const cairo_pattern_t	*source,
					   cairo_glyph_t		*glyphs,
					   int			 num_glyphs,
					   cairo_scaled_font_t	*scaled_font,
					   cairo_clip_t		*clip,
					   int			*remaining_glyphs,
					   cairo_bool_t           glyph_indexing)
{
#ifdef CAIRO_HAS_WIN32_FONT
    if (scaled_font->backend->type == CAIRO_FONT_TYPE_DWRITE) {
#ifdef CAIRO_HAS_DWRITE_FONT
        return _cairo_dwrite_show_glyphs_on_surface(surface, op, source, glyphs, num_glyphs, scaled_font, clip);
#endif
    } else {
	cairo_win32_surface_t *dst = surface;
        
	WORD glyph_buf_stack[STACK_GLYPH_SIZE];
	WORD *glyph_buf = glyph_buf_stack;
	int dxy_buf_stack[2 * STACK_GLYPH_SIZE];
	int *dxy_buf = dxy_buf_stack;

	BOOL win_result = 0;
	int i, j;

	cairo_solid_pattern_t *solid_pattern;
	COLORREF color;

	cairo_matrix_t device_to_logical;

	int start_x, start_y;
	double user_x, user_y;
	int logical_x, logical_y;
	unsigned int glyph_index_option;

	
	if (cairo_scaled_font_get_type (scaled_font) != CAIRO_FONT_TYPE_WIN32)
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	
	if (!_cairo_pattern_is_opaque_solid(source))
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	

	if ((op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_OVER) ||
	    (dst->format != CAIRO_FORMAT_RGB24))
	    return CAIRO_INT_STATUS_UNSUPPORTED;

	


	if (clip != NULL) {
	    if ((dst->flags & CAIRO_WIN32_SURFACE_FOR_PRINTING) == 0) {
		cairo_region_t *clip_region;
		cairo_status_t status;

		status = _cairo_clip_get_region (clip, &clip_region);
		assert (status != CAIRO_INT_STATUS_NOTHING_TO_DO);
		if (status)
		    return status;

		_cairo_win32_surface_set_clip_region (surface, clip_region);
	    }
	} else {
	    _cairo_win32_surface_set_clip_region (surface, NULL);
	}

	solid_pattern = (cairo_solid_pattern_t *)source;
	color = RGB(((int)solid_pattern->color.red_short) >> 8,
		    ((int)solid_pattern->color.green_short) >> 8,
		    ((int)solid_pattern->color.blue_short) >> 8);

	cairo_win32_scaled_font_get_device_to_logical(scaled_font, &device_to_logical);

	SaveDC(dst->dc);

	cairo_win32_scaled_font_select_font(scaled_font, dst->dc);
	SetTextColor(dst->dc, color);
	SetTextAlign(dst->dc, TA_BASELINE | TA_LEFT);
	SetBkMode(dst->dc, TRANSPARENT);

	if (num_glyphs > STACK_GLYPH_SIZE) {
	    glyph_buf = (WORD *) _cairo_malloc_ab (num_glyphs, sizeof(WORD));
	    dxy_buf = (int *) _cairo_malloc_abc (num_glyphs, sizeof(int), 2);
	}

	





	user_x = glyphs[0].x;
	user_y = glyphs[0].y;

	cairo_matrix_transform_point(&device_to_logical,
				     &user_x, &user_y);

	logical_x = _cairo_lround (user_x);
	logical_y = _cairo_lround (user_y);

	start_x = logical_x;
	start_y = logical_y;

	for (i = 0, j = 0; i < num_glyphs; ++i, j = 2 * i) {
	    glyph_buf[i] = (WORD) glyphs[i].index;
	    if (i == num_glyphs - 1) {
		dxy_buf[j] = 0;
		dxy_buf[j+1] = 0;
	    } else {
		double next_user_x = glyphs[i+1].x;
		double next_user_y = glyphs[i+1].y;
		int next_logical_x, next_logical_y;

		cairo_matrix_transform_point(&device_to_logical,
					     &next_user_x, &next_user_y);

		next_logical_x = _cairo_lround (next_user_x);
		next_logical_y = _cairo_lround (next_user_y);

		dxy_buf[j] = _cairo_lround (next_logical_x - logical_x);
		dxy_buf[j+1] = _cairo_lround (logical_y - next_logical_y);
		    

		logical_x = next_logical_x;
		logical_y = next_logical_y;
	    }
	}

	if (glyph_indexing)
	    glyph_index_option = ETO_GLYPH_INDEX;
	else
	    glyph_index_option = 0;

	win_result = ExtTextOutW(dst->dc,
				 start_x,
				 start_y,
				 glyph_index_option | ETO_PDY,
				 NULL,
				 glyph_buf,
				 num_glyphs,
				 dxy_buf);
	if (!win_result) {
	    _cairo_win32_print_gdi_error("_cairo_win32_surface_show_glyphs(ExtTextOutW failed)");
	}

	RestoreDC(dst->dc, -1);

	if (glyph_buf != glyph_buf_stack) {
	    free(glyph_buf);
	    free(dxy_buf);
	}
	return (win_result) ? CAIRO_STATUS_SUCCESS : CAIRO_INT_STATUS_UNSUPPORTED;
    }
#else
    return CAIRO_INT_STATUS_UNSUPPORTED;
#endif
}

#undef STACK_GLYPH_SIZE

cairo_int_status_t
_cairo_win32_surface_show_glyphs (void			*surface,
 				  cairo_operator_t	 op,
 				  const cairo_pattern_t *source,
 				  cairo_glyph_t	 	*glyphs,
 				  int			 num_glyphs,
 				  cairo_scaled_font_t	*scaled_font,
 				  cairo_clip_t          *clip,
 				  int		      	*remaining_glyphs)
{
    return _cairo_win32_surface_show_glyphs_internal (surface,
 						      op,
 						      source,
 						      glyphs,
 						      num_glyphs,
 						      scaled_font,
 						      clip,
 						      remaining_glyphs,
 						      TRUE);
}

static cairo_surface_t *
cairo_win32_surface_create_internal (HDC hdc, cairo_format_t format)
{
    cairo_win32_surface_t *surface;

    RECT rect;

    surface = malloc (sizeof (cairo_win32_surface_t));
    if (surface == NULL)
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));

    if (_cairo_win32_save_initial_clip (hdc, surface) != CAIRO_STATUS_SUCCESS) {
	free (surface);
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
    }

    surface->clip_region = NULL;
    surface->image = NULL;
    surface->format = format;

    surface->d3d9surface = NULL;
    surface->dc = hdc;
    surface->bitmap = NULL;
    surface->is_dib = FALSE;
    surface->saved_dc_bitmap = NULL;
    surface->brush = NULL;
    surface->old_brush = NULL;
    surface->font_subsets = NULL;

    GetClipBox(hdc, &rect);
    surface->extents.x = rect.left;
    surface->extents.y = rect.top;
    surface->extents.width = rect.right - rect.left;
    surface->extents.height = rect.bottom - rect.top;

    surface->flags = _cairo_win32_flags_for_dc (surface->dc);

    _cairo_surface_init (&surface->base,
			 &cairo_win32_surface_backend,
			 NULL, 
			 _cairo_content_from_format (format));

    return &surface->base;
}















cairo_surface_t *
cairo_win32_surface_create (HDC hdc)
{
    
    return cairo_win32_surface_create_internal(hdc, CAIRO_FORMAT_RGB24);
}















cairo_surface_t *
cairo_win32_surface_create_with_alpha (HDC hdc)
{
    return cairo_win32_surface_create_internal(hdc, CAIRO_FORMAT_ARGB32);
}















cairo_surface_t *
cairo_win32_surface_create_with_dib (cairo_format_t format,
				     int	    width,
				     int	    height)
{
    return _cairo_win32_surface_create_for_dc (NULL, format, width, height);
}
















cairo_surface_t *
cairo_win32_surface_create_with_ddb (HDC hdc,
				     cairo_format_t format,
				     int width,
				     int height)
{
    cairo_win32_surface_t *new_surf;
    HBITMAP ddb;
    HDC screen_dc, ddb_dc;
    HBITMAP saved_dc_bitmap;

    if (format != CAIRO_FORMAT_RGB24)
	return _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_INVALID_FORMAT));





    if (!hdc) {
	screen_dc = GetDC (NULL);
	hdc = screen_dc;
    } else {
	screen_dc = NULL;
    }

    ddb_dc = CreateCompatibleDC (hdc);
    if (ddb_dc == NULL) {
	new_surf = (cairo_win32_surface_t*) _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
	goto FINISH;
    }

    ddb = CreateCompatibleBitmap (hdc, width, height);
    if (ddb == NULL) {
	DeleteDC (ddb_dc);

	




	new_surf = (cairo_win32_surface_t*) _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
	goto FINISH;
    }

    saved_dc_bitmap = SelectObject (ddb_dc, ddb);

    new_surf = (cairo_win32_surface_t*) cairo_win32_surface_create (ddb_dc);
    new_surf->bitmap = ddb;
    new_surf->saved_dc_bitmap = saved_dc_bitmap;
    new_surf->is_dib = FALSE;

FINISH:
    if (screen_dc)
	ReleaseDC (NULL, screen_dc);

    return (cairo_surface_t*) new_surf;
}

cairo_public cairo_surface_t *
cairo_win32_surface_create_with_d3dsurface9 (IDirect3DSurface9 *surface)
{
    HDC dc;
    cairo_surface_t *win_surface;

    IDirect3DSurface9_AddRef (surface);
    IDirect3DSurface9_GetDC (surface, &dc);
    win_surface = cairo_win32_surface_create_internal(dc, CAIRO_FORMAT_RGB24);
    if (likely(win_surface->status == CAIRO_STATUS_SUCCESS)) {
	((cairo_win32_surface_t*)win_surface)->d3d9surface = surface;
    }
    return win_surface;

}










int
_cairo_surface_is_win32 (cairo_surface_t *surface)
{
    return surface->backend == &cairo_win32_surface_backend;
}












HDC
cairo_win32_surface_get_dc (cairo_surface_t *surface)
{
    cairo_win32_surface_t *winsurf;

    if (_cairo_surface_is_win32 (surface)){
	winsurf = (cairo_win32_surface_t *) surface;

	return winsurf->dc;
    }

    if (_cairo_surface_is_paginated (surface)) {
	cairo_surface_t *target;

	target = _cairo_paginated_surface_get_target (surface);

#ifndef CAIRO_OMIT_WIN32_PRINTING
	if (_cairo_surface_is_win32_printing (target)) {
	    winsurf = (cairo_win32_surface_t *) target;
	    return winsurf->dc;
	}
#endif
    }

    return NULL;
}


HDC
cairo_win32_get_dc_with_clip (cairo_t *cr)
{
    cairo_surface_t *surface = cr->gstate->target;
    cairo_clip_t clip;
    _cairo_clip_init_copy(&clip, &cr->gstate->clip);

    if (_cairo_surface_is_win32 (surface)){
	cairo_win32_surface_t *winsurf = (cairo_win32_surface_t *) surface;
	cairo_region_t *clip_region = NULL;
	cairo_status_t status;

	if (clip.path) {
	    status = _cairo_clip_get_region (&clip, &clip_region);
	    assert (status != CAIRO_INT_STATUS_NOTHING_TO_DO);
	    if (status) {
		_cairo_clip_fini(&clip);
		return NULL;
	    }
	}
	_cairo_win32_surface_set_clip_region (winsurf, clip_region);

	_cairo_clip_fini(&clip);
	return winsurf->dc;
    }

    if (_cairo_surface_is_paginated (surface)) {
	cairo_surface_t *target;

	target = _cairo_paginated_surface_get_target (surface);

#ifndef CAIRO_OMIT_WIN32_PRINTING
	if (_cairo_surface_is_win32_printing (target)) {
	    cairo_status_t status;
	    cairo_win32_surface_t *winsurf = (cairo_win32_surface_t *) target;

	    status = _cairo_surface_clipper_set_clip (&winsurf->clipper, &clip);

	    _cairo_clip_fini(&clip);

	    if (status)
		return NULL;

	    return winsurf->dc;
	}
#endif
    }

    _cairo_clip_fini(&clip);
    return NULL;
}
















cairo_surface_t *
cairo_win32_surface_get_image (cairo_surface_t *surface)
{
    if (!_cairo_surface_is_win32(surface))
	return NULL;

    return ((cairo_win32_surface_t*)surface)->image;
}

static cairo_bool_t
_cairo_win32_surface_is_similar (void *surface_a,
	                         void *surface_b)
{
    cairo_win32_surface_t *a = surface_a;
    cairo_win32_surface_t *b = surface_b;

    return a->dc == b->dc;
}

typedef struct _cairo_win32_surface_span_renderer {
    cairo_span_renderer_t base;

    cairo_operator_t op;
    const cairo_pattern_t *pattern;
    cairo_antialias_t antialias;

    uint8_t *mask_data;
    uint32_t mask_stride;

    cairo_image_surface_t *mask;
    cairo_win32_surface_t *dst;
    cairo_region_t *clip_region;

    cairo_composite_rectangles_t composite_rectangles;
} cairo_win32_surface_span_renderer_t;

static cairo_status_t
_cairo_win32_surface_span_renderer_render_rows (
    void				*abstract_renderer,
    int					 y,
    int					 height,
    const cairo_half_open_span_t	*spans,
    unsigned				 num_spans)
{
    cairo_win32_surface_span_renderer_t *renderer = abstract_renderer;
    while (height--)
	_cairo_image_surface_span_render_row (y++, spans, num_spans, renderer->mask_data, renderer->mask_stride);
    return CAIRO_STATUS_SUCCESS;
}

static void
_cairo_win32_surface_span_renderer_destroy (void *abstract_renderer)
{
    cairo_win32_surface_span_renderer_t *renderer = abstract_renderer;
    if (!renderer) return;

    if (renderer->mask != NULL)
	cairo_surface_destroy (&renderer->mask->base);

    free (renderer);
}

static cairo_status_t
_cairo_win32_surface_span_renderer_finish (void *abstract_renderer)
{
    cairo_win32_surface_span_renderer_t *renderer = abstract_renderer;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    if (renderer->pattern == NULL || renderer->mask == NULL)
	return CAIRO_STATUS_SUCCESS;

    status = cairo_surface_status (&renderer->mask->base);
    if (status == CAIRO_STATUS_SUCCESS) {
	cairo_composite_rectangles_t *rects = &renderer->composite_rectangles;
	cairo_win32_surface_t *dst = renderer->dst;
	cairo_pattern_t *mask_pattern = cairo_pattern_create_for_surface (&renderer->mask->base);
	
	if (dst->image) {
	    GdiFlush(); 

	    status = dst->image->backend->composite (renderer->op,
		    renderer->pattern, mask_pattern, dst->image,
		    rects->bounded.x, rects->bounded.y,
		    0, 0,		
		    rects->bounded.x, rects->bounded.y,
		    rects->bounded.width, rects->bounded.height,
		    renderer->clip_region);
	} else {
	    

	    status = _cairo_surface_fallback_composite (
		    renderer->op,
		    renderer->pattern, mask_pattern, &dst->base,
		    rects->bounded.x, rects->bounded.y,
		    0, 0,		
		    rects->bounded.x, rects->bounded.y,
		    rects->bounded.width, rects->bounded.height,
		    renderer->clip_region);
	}
	cairo_pattern_destroy (mask_pattern);

    }
    if (status != CAIRO_STATUS_SUCCESS)
	return _cairo_span_renderer_set_error (abstract_renderer,
					       status);
    return CAIRO_STATUS_SUCCESS;
}

static cairo_int_status_t
_cairo_win32_surface_paint (void			*abstract_surface,
			    cairo_operator_t		 op,
			    const cairo_pattern_t	*source,
			    cairo_clip_t		*clip)
{
    cairo_win32_surface_t *surface = abstract_surface;

    if (surface->image) {
	return _cairo_surface_paint (surface->image, op, source, clip);
    }
    return CAIRO_INT_STATUS_UNSUPPORTED;
}

static cairo_int_status_t
_cairo_win32_surface_mask (void				*abstract_surface,
			   cairo_operator_t		 op,
			   const cairo_pattern_t	*source,
			   const cairo_pattern_t	*mask,
			   cairo_clip_t			*clip)
{
    cairo_win32_surface_t *surface = abstract_surface;

    if (surface->image) {
	return _cairo_surface_mask (surface->image, op, source, mask, clip);
    }
    return CAIRO_INT_STATUS_UNSUPPORTED;
}

static cairo_int_status_t
_cairo_win32_surface_stroke (void			*abstract_surface,
			     cairo_operator_t		 op,
			     const cairo_pattern_t	*source,
			     cairo_path_fixed_t		*path,
			     const cairo_stroke_style_t	*style,
			     const cairo_matrix_t	*ctm,
			     const cairo_matrix_t	*ctm_inverse,
			     double			 tolerance,
			     cairo_antialias_t		 antialias,
			     cairo_clip_t		*clip)
{
    cairo_win32_surface_t *surface = abstract_surface;

    if (surface->image) {
	return _cairo_surface_stroke (surface->image, op, source,
					        path, style,
					        ctm, ctm_inverse,
					        tolerance, antialias,
					        clip);

    }
    return CAIRO_INT_STATUS_UNSUPPORTED;
}

static cairo_int_status_t
_cairo_win32_surface_fill (void				*abstract_surface,
			   cairo_operator_t		 op,
			   const cairo_pattern_t	*source,
			   cairo_path_fixed_t		*path,
			   cairo_fill_rule_t		 fill_rule,
			   double			 tolerance,
			   cairo_antialias_t		 antialias,
			   cairo_clip_t			*clip)
{
    cairo_win32_surface_t *surface = abstract_surface;

    if (surface->image) {
	return _cairo_surface_fill (surface->image, op, source,
					 path, fill_rule,
					 tolerance, antialias,
					 clip);
    }
    return CAIRO_INT_STATUS_UNSUPPORTED;
}


#include "cairoint.h"

#include "cairo-boxes-private.h"
#include "cairo-clip-private.h"
#include "cairo-composite-rectangles-private.h"
#include "cairo-error-private.h"
#include "cairo-region-private.h"
#include "cairo-spans-private.h"
#include "cairo-surface-fallback-private.h"

typedef struct {
    cairo_surface_t *dst;
    cairo_rectangle_int_t extents;
    cairo_image_surface_t *image;
    cairo_rectangle_int_t image_rect;
    void *image_extra;
} fallback_state_t;











static cairo_int_status_t
_fallback_init (fallback_state_t *state,
		cairo_surface_t  *dst,
		int               x,
		int               y,
		int               width,
		int               height)
{
    cairo_status_t status;

    state->extents.x = x;
    state->extents.y = y;
    state->extents.width = width;
    state->extents.height = height;

    state->dst = dst;

    status = _cairo_surface_acquire_dest_image (dst, &state->extents,
						&state->image, &state->image_rect,
						&state->image_extra);
    if (unlikely (status))
	return status;


    




    assert (state->image != NULL);

    return CAIRO_STATUS_SUCCESS;
}

static void
_fallback_fini (fallback_state_t *state)
{
    _cairo_surface_release_dest_image (state->dst, &state->extents,
				       state->image, &state->image_rect,
				       state->image_extra);
}

typedef cairo_status_t
(*cairo_draw_func_t) (void                          *closure,
		      cairo_operator_t               op,
		      const cairo_pattern_t         *src,
		      cairo_surface_t               *dst,
		      int                            dst_x,
		      int                            dst_y,
		      const cairo_rectangle_int_t   *extents,
		      cairo_region_t		    *clip_region);

static cairo_status_t
_create_composite_mask_pattern (cairo_surface_pattern_t       *mask_pattern,
				cairo_clip_t                  *clip,
				cairo_draw_func_t              draw_func,
				void                          *draw_closure,
				cairo_surface_t               *dst,
				const cairo_rectangle_int_t   *extents)
{
    cairo_surface_t *mask;
    cairo_region_t *clip_region = NULL, *fallback_region = NULL;
    cairo_status_t status;
    cairo_bool_t clip_surface = FALSE;

    if (clip != NULL) {
	status = _cairo_clip_get_region (clip, &clip_region);
	if (unlikely (_cairo_status_is_error (status) ||
		      status == CAIRO_INT_STATUS_NOTHING_TO_DO))
	{
	    return status;
	}

	clip_surface = status == CAIRO_INT_STATUS_UNSUPPORTED;
    }

    


    mask = _cairo_surface_create_similar_solid (dst,
						CAIRO_CONTENT_ALPHA,
						extents->width,
						extents->height,
						CAIRO_COLOR_TRANSPARENT,
						TRUE);
    if (unlikely (mask->status))
	return mask->status;

    if (clip_region && (extents->x || extents->y)) {
	fallback_region = cairo_region_copy (clip_region);
	status = fallback_region->status;
	if (unlikely (status))
	    goto CLEANUP_SURFACE;

	cairo_region_translate (fallback_region,
				-extents->x,
				-extents->y);
	clip_region = fallback_region;
    }

    status = draw_func (draw_closure, CAIRO_OPERATOR_ADD,
			&_cairo_pattern_white.base, mask,
			extents->x, extents->y,
			extents,
			clip_region);
    if (unlikely (status))
	goto CLEANUP_SURFACE;

    if (clip_surface)
	status = _cairo_clip_combine_with_surface (clip, mask, extents->x, extents->y);

    _cairo_pattern_init_for_surface (mask_pattern, mask);

 CLEANUP_SURFACE:
    if (fallback_region)
        cairo_region_destroy (fallback_region);
    cairo_surface_destroy (mask);

    return status;
}




static cairo_status_t
_clip_and_composite_with_mask (cairo_clip_t                  *clip,
			       cairo_operator_t               op,
			       const cairo_pattern_t         *src,
			       cairo_draw_func_t              draw_func,
			       void                          *draw_closure,
			       cairo_surface_t               *dst,
			       const cairo_rectangle_int_t   *extents)
{
    cairo_surface_pattern_t mask_pattern;
    cairo_status_t status;

    status = _create_composite_mask_pattern (&mask_pattern,
					     clip,
					     draw_func, draw_closure,
					     dst, extents);
    if (likely (status == CAIRO_STATUS_SUCCESS)) {
	status = _cairo_surface_composite (op,
					   src, &mask_pattern.base, dst,
					   extents->x,     extents->y,
					   0,              0,
					   extents->x,     extents->y,
					   extents->width, extents->height,
					   NULL);

	_cairo_pattern_fini (&mask_pattern.base);
    }

    return status;
}




static cairo_status_t
_clip_and_composite_combine (cairo_clip_t                  *clip,
			     cairo_operator_t               op,
			     const cairo_pattern_t         *src,
			     cairo_draw_func_t              draw_func,
			     void                          *draw_closure,
			     cairo_surface_t               *dst,
			     const cairo_rectangle_int_t   *extents)
{
    cairo_surface_t *intermediate;
    cairo_surface_pattern_t pattern;
    cairo_surface_pattern_t clip_pattern;
    cairo_surface_t *clip_surface;
    int clip_x, clip_y;
    cairo_status_t status;

    






    intermediate =
	_cairo_surface_create_similar_scratch (dst, dst->content,
					       extents->width,
					       extents->height);
    if (intermediate == NULL) {
	intermediate =
	    _cairo_image_surface_create_with_content (dst->content,
						      extents->width,
						      extents->width);
    }
    if (unlikely (intermediate->status))
	return intermediate->status;

    
    _cairo_pattern_init_for_surface (&pattern, dst);
    status = _cairo_surface_composite (CAIRO_OPERATOR_SOURCE,
				       &pattern.base, NULL, intermediate,
				       extents->x,     extents->y,
				       0,              0,
				       0,              0,
				       extents->width, extents->height,
				       NULL);
    _cairo_pattern_fini (&pattern.base);
    if (unlikely (status))
	goto CLEANUP_SURFACE;

    status = (*draw_func) (draw_closure, op,
			   src, intermediate,
			   extents->x, extents->y,
			   extents,
			   NULL);
    if (unlikely (status))
	goto CLEANUP_SURFACE;

    assert (clip->path != NULL);
    clip_surface = _cairo_clip_get_surface (clip, dst, &clip_x, &clip_y);
    if (unlikely (clip_surface->status))
	goto CLEANUP_SURFACE;

    _cairo_pattern_init_for_surface (&clip_pattern, clip_surface);

    
    status = _cairo_surface_composite (CAIRO_OPERATOR_DEST_IN,
				       &clip_pattern.base, NULL, intermediate,
				       extents->x - clip_x,
				       extents->y - clip_y,
				       0, 0,
				       0, 0,
				       extents->width, extents->height,
				       NULL);
    if (unlikely (status))
	goto CLEANUP_CLIP;

    
    status = _cairo_surface_composite (CAIRO_OPERATOR_DEST_OUT,
				       &clip_pattern.base, NULL, dst,
				       extents->x - clip_x,
				       extents->y - clip_y,
				       0, 0,
				       extents->x, extents->y,
				       extents->width, extents->height,
				       NULL);
    if (unlikely (status))
	goto CLEANUP_CLIP;

    
    _cairo_pattern_init_for_surface (&pattern, intermediate);
    status = _cairo_surface_composite (CAIRO_OPERATOR_ADD,
				       &pattern.base, NULL, dst,
				       0,              0,
				       0,              0,
				       extents->x,     extents->y,
				       extents->width, extents->height,
				       NULL);
    _cairo_pattern_fini (&pattern.base);

 CLEANUP_CLIP:
    _cairo_pattern_fini (&clip_pattern.base);
 CLEANUP_SURFACE:
    cairo_surface_destroy (intermediate);

    return status;
}




static cairo_status_t
_clip_and_composite_source (cairo_clip_t                  *clip,
			    const cairo_pattern_t         *src,
			    cairo_draw_func_t              draw_func,
			    void                          *draw_closure,
			    cairo_surface_t               *dst,
			    const cairo_rectangle_int_t   *extents)
{
    cairo_surface_pattern_t mask_pattern;
    cairo_region_t *clip_region = NULL;
    cairo_status_t status;

    if (clip != NULL) {
	status = _cairo_clip_get_region (clip, &clip_region);
	if (unlikely (_cairo_status_is_error (status) ||
		      status == CAIRO_INT_STATUS_NOTHING_TO_DO))
	{
	    return status;
	}
    }

    
    status = _create_composite_mask_pattern (&mask_pattern,
					     clip,
					     draw_func, draw_closure,
					     dst, extents);
    if (unlikely (status))
	return status;

    
    status = _cairo_surface_composite (CAIRO_OPERATOR_DEST_OUT,
				       &mask_pattern.base, NULL, dst,
				       0,              0,
				       0,              0,
				       extents->x,     extents->y,
				       extents->width, extents->height,
				       clip_region);

    if (unlikely (status))
	goto CLEANUP_MASK_PATTERN;

    
    status = _cairo_surface_composite (CAIRO_OPERATOR_ADD,
				       src, &mask_pattern.base, dst,
				       extents->x,     extents->y,
				       0,              0,
				       extents->x,     extents->y,
				       extents->width, extents->height,
				       clip_region);

 CLEANUP_MASK_PATTERN:
    _cairo_pattern_fini (&mask_pattern.base);
    return status;
}

static int
_cairo_rectangle_empty (const cairo_rectangle_int_t *rect)
{
    return rect->width == 0 || rect->height == 0;
}






















static cairo_status_t
_clip_and_composite (cairo_clip_t                  *clip,
		     cairo_operator_t               op,
		     const cairo_pattern_t         *src,
		     cairo_draw_func_t              draw_func,
		     void                          *draw_closure,
		     cairo_surface_t               *dst,
		     const cairo_rectangle_int_t   *extents)
{
    cairo_status_t status;

    if (_cairo_rectangle_empty (extents))
	
	return CAIRO_STATUS_SUCCESS;

    if (op == CAIRO_OPERATOR_CLEAR) {
	src = &_cairo_pattern_white.base;
	op = CAIRO_OPERATOR_DEST_OUT;
    }

    if (op == CAIRO_OPERATOR_SOURCE) {
	status = _clip_and_composite_source (clip,
					     src,
					     draw_func, draw_closure,
					     dst, extents);
    } else {
	cairo_bool_t clip_surface = FALSE;
	cairo_region_t *clip_region = NULL;

	if (clip != NULL) {
	    status = _cairo_clip_get_region (clip, &clip_region);
	    if (unlikely (_cairo_status_is_error (status) ||
			  status == CAIRO_INT_STATUS_NOTHING_TO_DO))
	    {
		return status;
	    }

	    clip_surface = status == CAIRO_INT_STATUS_UNSUPPORTED;
	}

	if (clip_surface) {
	    if (_cairo_operator_bounded_by_mask (op)) {
		status = _clip_and_composite_with_mask (clip, op,
							src,
							draw_func, draw_closure,
							dst, extents);
	    } else {
		status = _clip_and_composite_combine (clip, op,
						      src,
						      draw_func, draw_closure,
						      dst, extents);
	    }
	} else {
	    status = draw_func (draw_closure, op,
				src, dst,
				0, 0,
				extents,
				clip_region);
	}
    }

    return status;
}



static cairo_status_t
_composite_trap_region (cairo_clip_t            *clip,
			const cairo_pattern_t	*src,
			cairo_operator_t         op,
			cairo_surface_t         *dst,
			cairo_region_t          *trap_region,
			const cairo_rectangle_int_t   *extents)
{
    cairo_status_t status;
    cairo_surface_pattern_t mask_pattern;
    cairo_pattern_t *mask = NULL;
    int mask_x = 0, mask_y =0;

    if (clip != NULL) {
	cairo_surface_t *clip_surface = NULL;
	int clip_x, clip_y;

	clip_surface = _cairo_clip_get_surface (clip, dst, &clip_x, &clip_y);
	if (unlikely (clip_surface->status))
	    return clip_surface->status;

	if (op == CAIRO_OPERATOR_CLEAR) {
	    src = &_cairo_pattern_white.base;
	    op = CAIRO_OPERATOR_DEST_OUT;
	}

	_cairo_pattern_init_for_surface (&mask_pattern, clip_surface);
	mask_x = extents->x - clip_x;
	mask_y = extents->y - clip_y;
	mask = &mask_pattern.base;
    }

    status = _cairo_surface_composite (op, src, mask, dst,
				       extents->x, extents->y,
				       mask_x, mask_y,
				       extents->x, extents->y,
				       extents->width, extents->height,
				       trap_region);

    if (mask != NULL)
      _cairo_pattern_fini (mask);

    return status;
}

typedef struct {
    cairo_traps_t *traps;
    cairo_antialias_t antialias;
} cairo_composite_traps_info_t;

static cairo_status_t
_composite_traps_draw_func (void                          *closure,
			    cairo_operator_t               op,
			    const cairo_pattern_t         *src,
			    cairo_surface_t               *dst,
			    int                            dst_x,
			    int                            dst_y,
			    const cairo_rectangle_int_t   *extents,
			    cairo_region_t		  *clip_region)
{
    cairo_composite_traps_info_t *info = closure;
    cairo_status_t status;
    cairo_region_t *extents_region = NULL;

    if (dst_x != 0 || dst_y != 0)
	_cairo_traps_translate (info->traps, - dst_x, - dst_y);

    if (clip_region == NULL &&
        !_cairo_operator_bounded_by_source (op)) {
        extents_region = cairo_region_create_rectangle (extents);
        if (unlikely (extents_region->status))
            return extents_region->status;
        cairo_region_translate (extents_region, -dst_x, -dst_y);
        clip_region = extents_region;
    }

    status = _cairo_surface_composite_trapezoids (op,
                                                  src, dst, info->antialias,
                                                  extents->x,         extents->y,
                                                  extents->x - dst_x, extents->y - dst_y,
                                                  extents->width,     extents->height,
                                                  info->traps->traps,
                                                  info->traps->num_traps,
                                                  clip_region);

    if (extents_region)
        cairo_region_destroy (extents_region);

    return status;
}

enum {
    HAS_CLEAR_REGION = 0x1,
};

static cairo_status_t
_clip_and_composite_region (const cairo_pattern_t *src,
			    cairo_operator_t op,
			    cairo_surface_t *dst,
			    cairo_region_t *trap_region,
			    cairo_clip_t *clip,
			    cairo_rectangle_int_t *extents)
{
    cairo_region_t clear_region;
    unsigned int has_region = 0;
    cairo_status_t status;

    if (! _cairo_operator_bounded_by_mask (op) && clip == NULL) {
	



	_cairo_region_init_rectangle (&clear_region, extents);
	status = cairo_region_subtract (&clear_region, trap_region);
	if (unlikely (status))
	    return status;

	if (! cairo_region_is_empty (&clear_region))
	    has_region |= HAS_CLEAR_REGION;
    }

    if ((src->type == CAIRO_PATTERN_TYPE_SOLID || op == CAIRO_OPERATOR_CLEAR) &&
	clip == NULL)
    {
	const cairo_color_t *color;

	if (op == CAIRO_OPERATOR_CLEAR)
	    color = CAIRO_COLOR_TRANSPARENT;
	else
	    color = &((cairo_solid_pattern_t *)src)->color;

	
	status = _cairo_surface_fill_region (dst, op, color, trap_region);
    } else {
	














	status = _composite_trap_region (clip, src, op, dst,
					 trap_region, extents);
    }

    if (has_region & HAS_CLEAR_REGION) {
	if (status == CAIRO_STATUS_SUCCESS) {
	    status = _cairo_surface_fill_region (dst,
						 CAIRO_OPERATOR_CLEAR,
						 CAIRO_COLOR_TRANSPARENT,
						 &clear_region);
	}
	_cairo_region_fini (&clear_region);
    }

    return status;
}


static cairo_status_t
_fill_rectangles (cairo_surface_t *dst,
		  cairo_operator_t op,
		  const cairo_pattern_t *src,
		  cairo_traps_t *traps,
		  cairo_clip_t *clip)
{
    const cairo_color_t *color;
    cairo_rectangle_int_t stack_rects[CAIRO_STACK_ARRAY_LENGTH (cairo_rectangle_int_t)];
    cairo_rectangle_int_t *rects = stack_rects;
    cairo_status_t status;
    int i;

    if (! traps->is_rectilinear || ! traps->maybe_region)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    
    if (clip != NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    
    if (! _cairo_operator_bounded_by_mask (op))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (! (src->type == CAIRO_PATTERN_TYPE_SOLID || op == CAIRO_OPERATOR_CLEAR))
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (traps->has_intersections) {
	if (traps->is_rectangular) {
	    status = _cairo_bentley_ottmann_tessellate_rectangular_traps (traps, CAIRO_FILL_RULE_WINDING);
	} else {
	    status = _cairo_bentley_ottmann_tessellate_rectilinear_traps (traps, CAIRO_FILL_RULE_WINDING);
	}
	if (unlikely (status))
	    return status;
    }

    for (i = 0; i < traps->num_traps; i++) {
	if (! _cairo_fixed_is_integer (traps->traps[i].top)          ||
	    ! _cairo_fixed_is_integer (traps->traps[i].bottom)       ||
	    ! _cairo_fixed_is_integer (traps->traps[i].left.p1.x)    ||
	    ! _cairo_fixed_is_integer (traps->traps[i].right.p1.x))
	{
	    traps->maybe_region = FALSE;
	    return CAIRO_INT_STATUS_UNSUPPORTED;
	}
    }

    if (traps->num_traps > ARRAY_LENGTH (stack_rects)) {
	rects = _cairo_malloc_ab (traps->num_traps,
				  sizeof (cairo_rectangle_int_t));
	if (unlikely (rects == NULL))
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    for (i = 0; i < traps->num_traps; i++) {
	int x1 = _cairo_fixed_integer_part (traps->traps[i].left.p1.x);
	int y1 = _cairo_fixed_integer_part (traps->traps[i].top);
	int x2 = _cairo_fixed_integer_part (traps->traps[i].right.p1.x);
	int y2 = _cairo_fixed_integer_part (traps->traps[i].bottom);

	rects[i].x = x1;
	rects[i].y = y1;
	rects[i].width = x2 - x1;
	rects[i].height = y2 - y1;
    }

    if (op == CAIRO_OPERATOR_CLEAR)
	color = CAIRO_COLOR_TRANSPARENT;
    else
	color = &((cairo_solid_pattern_t *)src)->color;

    status =  _cairo_surface_fill_rectangles (dst, op, color, rects, i);

    if (rects != stack_rects)
	free (rects);

    return status;
}


static cairo_status_t
_composite_rectangle (cairo_surface_t *dst,
		      cairo_operator_t op,
		      const cairo_pattern_t *src,
		      cairo_traps_t *traps,
		      cairo_clip_t *clip)
{
    cairo_rectangle_int_t rect;

    if (clip != NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (traps->num_traps > 1 || ! traps->is_rectilinear || ! traps->maybe_region)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    if (! _cairo_fixed_is_integer (traps->traps[0].top)          ||
	! _cairo_fixed_is_integer (traps->traps[0].bottom)       ||
	! _cairo_fixed_is_integer (traps->traps[0].left.p1.x)    ||
	! _cairo_fixed_is_integer (traps->traps[0].right.p1.x))
    {
	traps->maybe_region = FALSE;
	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    rect.x = _cairo_fixed_integer_part (traps->traps[0].left.p1.x);
    rect.y = _cairo_fixed_integer_part (traps->traps[0].top);
    rect.width  = _cairo_fixed_integer_part (traps->traps[0].right.p1.x) - rect.x;
    rect.height = _cairo_fixed_integer_part (traps->traps[0].bottom) - rect.y;

    return _cairo_surface_composite (op, src, NULL, dst,
				     rect.x, rect.y,
				     0, 0,
				     rect.x, rect.y,
				     rect.width, rect.height,
				     NULL);
}


static cairo_status_t
_clip_and_composite_trapezoids (const cairo_pattern_t *src,
				cairo_operator_t op,
				cairo_surface_t *dst,
				cairo_traps_t *traps,
				cairo_antialias_t antialias,
				cairo_clip_t *clip,
				cairo_rectangle_int_t *extents)
{
    cairo_composite_traps_info_t traps_info;
    cairo_region_t *clip_region = NULL;
    cairo_bool_t clip_surface = FALSE;
    cairo_status_t status;

    if (traps->num_traps == 0 && _cairo_operator_bounded_by_mask (op))
	return CAIRO_STATUS_SUCCESS;

    if (clip != NULL) {
	status = _cairo_clip_get_region (clip, &clip_region);
	if (unlikely (_cairo_status_is_error (status)))
	    return status;
	if (unlikely (status == CAIRO_INT_STATUS_NOTHING_TO_DO))
	    return CAIRO_STATUS_SUCCESS;

	clip_surface = status == CAIRO_INT_STATUS_UNSUPPORTED;
    }

    



    if (! clip_surface ||
	(_cairo_operator_bounded_by_mask (op) && op != CAIRO_OPERATOR_SOURCE))
    {
	cairo_region_t *trap_region = NULL;

        if (_cairo_operator_bounded_by_source (op)) {
            status = _fill_rectangles (dst, op, src, traps, clip);
            if (status != CAIRO_INT_STATUS_UNSUPPORTED)
                return status;

            status = _composite_rectangle (dst, op, src, traps, clip);
            if (status != CAIRO_INT_STATUS_UNSUPPORTED)
                return status;
        }

	status = _cairo_traps_extract_region (traps, &trap_region);
	if (unlikely (_cairo_status_is_error (status)))
	    return status;

	if (trap_region != NULL) {
	    status = cairo_region_intersect_rectangle (trap_region, extents);
	    if (unlikely (status)) {
		cairo_region_destroy (trap_region);
		return status;
	    }

	    if (clip_region != NULL) {
		status = cairo_region_intersect (trap_region, clip_region);
		if (unlikely (status)) {
		    cairo_region_destroy (trap_region);
		    return status;
		}
	    }

	    if (_cairo_operator_bounded_by_mask (op)) {
		cairo_rectangle_int_t trap_extents;

		cairo_region_get_extents (trap_region, &trap_extents);
		if (! _cairo_rectangle_intersect (extents, &trap_extents)) {
		    cairo_region_destroy (trap_region);
		    return CAIRO_STATUS_SUCCESS;
		}
	    }

	    status = _clip_and_composite_region (src, op, dst,
						 trap_region,
						 clip_surface ? clip : NULL,
						 extents);
	    cairo_region_destroy (trap_region);

	    if (likely (status != CAIRO_INT_STATUS_UNSUPPORTED))
		return status;
	}
    }

    
    if (traps->has_intersections) {
	if (traps->is_rectangular)
	    status = _cairo_bentley_ottmann_tessellate_rectangular_traps (traps, CAIRO_FILL_RULE_WINDING);
	else if (traps->is_rectilinear)
	    status = _cairo_bentley_ottmann_tessellate_rectilinear_traps (traps, CAIRO_FILL_RULE_WINDING);
	else
	    status = _cairo_bentley_ottmann_tessellate_traps (traps, CAIRO_FILL_RULE_WINDING);
	if (unlikely (status))
	    return status;
    }

    


    traps_info.traps = traps;
    traps_info.antialias = antialias;

    return _clip_and_composite (clip, op, src,
				_composite_traps_draw_func,
				&traps_info, dst, extents);
}

typedef struct {
    cairo_polygon_t		*polygon;
    cairo_fill_rule_t		 fill_rule;
    cairo_antialias_t		 antialias;
} cairo_composite_spans_info_t;

static cairo_status_t
_composite_spans_draw_func (void                          *closure,
			    cairo_operator_t               op,
			    const cairo_pattern_t         *src,
			    cairo_surface_t               *dst,
			    int                            dst_x,
			    int                            dst_y,
			    const cairo_rectangle_int_t   *extents,
			    cairo_region_t		  *clip_region)
{
    cairo_composite_rectangles_t rects;
    cairo_composite_spans_info_t *info = closure;

    rects.source = *extents;
    rects.mask = *extents;
    rects.bounded = *extents;
    


    rects.bounded.x -= dst_x;
    rects.bounded.y -= dst_y;
    rects.unbounded = rects.bounded;
    rects.is_bounded = _cairo_operator_bounded_by_either (op);

    return _cairo_surface_composite_polygon (dst, op, src,
					     info->fill_rule,
					     info->antialias,
					     &rects,
					     info->polygon,
					     clip_region);
}

static cairo_status_t
_cairo_win32_surface_span_renderer_composite
                 (void                          *closure,
		  cairo_operator_t               op,
		  const cairo_pattern_t         *src,
                  cairo_image_surface_t         *mask,
		  cairo_win32_surface_t         *dst,
		  int                            dst_x,
		  int                            dst_y,
		  const cairo_rectangle_int_t   *extents,
		  cairo_region_t		*clip_region)
{
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    if (src == NULL || mask == NULL)
	return CAIRO_STATUS_SUCCESS;

    status = cairo_surface_status (&mask->base);
    if (status == CAIRO_STATUS_SUCCESS) {
	cairo_pattern_t *mask_pattern = cairo_pattern_create_for_surface (&mask->base);
	
	if (dst->image) {
	    GdiFlush(); 

	    status = dst->image->backend->composite (op,
		    src, mask_pattern, dst->image,
                    extents->x, extents->y,
		    0, 0,		
                    extents->x - dst_x, extents->y - dst_y,
                    extents->width, extents->height,
		    clip_region);
	} else {
	    

	    status = _cairo_surface_fallback_composite (
		    op,
		    src, mask_pattern, &dst->base,
                    extents->x, extents->y,
		    0, 0,		
                    extents->x - dst_x, extents->y - dst_y,
                    extents->width, extents->height,
		    clip_region);
	}
	cairo_pattern_destroy (mask_pattern);

    }
    return status;
}

typedef struct _cairo_image_surface_span_renderer {
    cairo_span_renderer_t base;

    uint8_t *mask_data;
    uint32_t mask_stride;
} cairo_image_surface_span_renderer_t;

cairo_status_t
_cairo_image_surface_span (void *abstract_renderer,
			   int y, int height,
			   const cairo_half_open_span_t *spans,
			   unsigned num_spans);

static cairo_status_t
_composite_spans (void                          *closure,
		  cairo_operator_t               op,
		  const cairo_pattern_t         *src,
		  cairo_surface_t               *dst,
		  int                            dst_x,
		  int                            dst_y,
		  const cairo_rectangle_int_t   *extents,
		  cairo_region_t		*clip_region)
{
    cairo_composite_spans_info_t *info = closure;
    cairo_image_surface_span_renderer_t renderer;
    cairo_scan_converter_t *converter;
    cairo_image_surface_t *mask;
    cairo_status_t status;

    converter = _cairo_tor_scan_converter_create (extents->x, extents->y,
						  extents->x + extents->width,
						  extents->y + extents->height,
						  info->fill_rule);
    status = converter->add_polygon (converter, info->polygon);
    if (unlikely (status))
	goto CLEANUP_CONVERTER;

    


    {
	mask = cairo_image_surface_create (CAIRO_FORMAT_A8,
				           extents->width,
                                           extents->height);

	if (cairo_surface_status(&mask->base)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto CLEANUP_CONVERTER;
	}
    }

    renderer.base.render_rows = _cairo_image_surface_span;
    renderer.mask_stride = cairo_image_surface_get_stride (mask);
    renderer.mask_data = cairo_image_surface_get_data (mask);
    renderer.mask_data -= extents->y * renderer.mask_stride + extents->x;

    status = converter->generate (converter, &renderer.base);
    if (unlikely (status))
	goto CLEANUP_RENDERER;

    _cairo_win32_surface_span_renderer_composite
                 (closure,
		  op,
		  src,
                  mask,
		  (cairo_win32_surface_t*)dst, 
		  dst_x,
		  dst_y,
		  extents,
		  clip_region);
#if 0
    {
	pixman_image_t *src;
	int src_x, src_y;

	src = _pixman_image_for_pattern (pattern, FALSE, extents, &src_x, &src_y);
	if (unlikely (src == NULL)) {
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    goto CLEANUP_RENDERER;
	}

	pixman_image_composite32 (_pixman_operator (op), src, mask, dst,
                                  extents->x + src_x, extents->y + src_y,
                                  0, 0, 
                                  extents->x - dst_x, extents->y - dst_y,
                                  extents->width, extents->height);
	pixman_image_unref (src);
    }
#endif
 CLEANUP_RENDERER:
    cairo_surface_destroy (mask);
 CLEANUP_CONVERTER:
    converter->destroy (converter);
    return status;
}



cairo_status_t
_cairo_win32_surface_fallback_paint (cairo_surface_t		*surface,
			       cairo_operator_t		 op,
			       const cairo_pattern_t	*source,
			       cairo_clip_t		*clip)
{
    cairo_composite_rectangles_t extents;
    cairo_rectangle_int_t rect;
    cairo_clip_path_t *clip_path = clip ? clip->path : NULL;
    cairo_box_t boxes_stack[32], *clip_boxes = boxes_stack;
    cairo_boxes_t  boxes;
    int num_boxes = ARRAY_LENGTH (boxes_stack);
    cairo_status_t status;
    cairo_traps_t traps;

    if (!_cairo_surface_get_extents (surface, &rect))
        ASSERT_NOT_REACHED;

    status = _cairo_composite_rectangles_init_for_paint (&extents,
							 &rect,
							 op, source,
							 clip);
    if (unlikely (status))
	return status;

    if (_cairo_clip_contains_extents (clip, &extents))
	clip = NULL;

    status = _cairo_clip_to_boxes (&clip, &extents, &clip_boxes, &num_boxes);
    if (unlikely (status))
	return status;

    




    if (clip != NULL && clip_path->prev == NULL &&
	_cairo_operator_bounded_by_mask (op))
    {
	return _cairo_surface_fill (surface, op, source,
				    &clip_path->path,
				    clip_path->fill_rule,
				    clip_path->tolerance,
				    clip_path->antialias,
				    NULL);
    }

    
    _cairo_boxes_init_for_array (&boxes, clip_boxes, num_boxes);
    status = _cairo_traps_init_boxes (&traps, &boxes);
    if (unlikely (status))
	goto CLEANUP_BOXES;

    status = _clip_and_composite_trapezoids (source, op, surface,
					     &traps, CAIRO_ANTIALIAS_DEFAULT,
					     clip,
                                             extents.is_bounded ? &extents.bounded : &extents.unbounded);
    _cairo_traps_fini (&traps);

CLEANUP_BOXES:
    if (clip_boxes != boxes_stack)
	free (clip_boxes);

    return status;
}

static cairo_status_t
_cairo_surface_mask_draw_func (void                        *closure,
			       cairo_operator_t             op,
			       const cairo_pattern_t       *src,
			       cairo_surface_t             *dst,
			       int                          dst_x,
			       int                          dst_y,
			       const cairo_rectangle_int_t *extents,
			       cairo_region_t		   *clip_region)
{
    cairo_pattern_t *mask = closure;
    cairo_status_t status;
    cairo_region_t *extents_region = NULL;

    if (clip_region == NULL &&
        !_cairo_operator_bounded_by_source (op)) {
        extents_region = cairo_region_create_rectangle (extents);
        if (unlikely (extents_region->status))
            return extents_region->status;
        cairo_region_translate (extents_region, -dst_x, -dst_y);
        clip_region = extents_region;
    }

    if (src) {
	status = _cairo_surface_composite (op,
                                           src, mask, dst,
                                           extents->x,         extents->y,
                                           extents->x,         extents->y,
                                           extents->x - dst_x, extents->y - dst_y,
                                           extents->width,     extents->height,
                                           clip_region);
    } else {
	status = _cairo_surface_composite (op,
                                           mask, NULL, dst,
                                           extents->x,         extents->y,
                                           0,                  0, 
                                           extents->x - dst_x, extents->y - dst_y,
                                           extents->width,     extents->height,
                                           clip_region);
    }

    if (extents_region)
        cairo_region_destroy (extents_region);

    return status;
}

cairo_status_t
_cairo_win32_surface_fallback_mask (cairo_surface_t		*surface,
			      cairo_operator_t		 op,
			      const cairo_pattern_t	*source,
			      const cairo_pattern_t	*mask,
			      cairo_clip_t		*clip)
{
    cairo_composite_rectangles_t extents;
    cairo_rectangle_int_t rect;
    cairo_status_t status;

    if (!_cairo_surface_get_extents (surface, &rect))
        ASSERT_NOT_REACHED;

    status = _cairo_composite_rectangles_init_for_mask (&extents,
							&rect,
							op, source, mask, clip);
    if (unlikely (status))
	return status;

    if (_cairo_clip_contains_extents (clip, &extents))
	clip = NULL;

    if (clip != NULL && extents.is_bounded) {
	status = _cairo_clip_rectangle (clip, &extents.bounded);
	if (unlikely (status))
	    return status;
    }

    return _clip_and_composite (clip, op, source,
				_cairo_surface_mask_draw_func,
				(void *) mask,
				surface,
                                extents.is_bounded ? &extents.bounded : &extents.unbounded);
}

cairo_status_t
_cairo_win32_surface_fallback_stroke (cairo_surface_t		*surface,
				cairo_operator_t	 op,
				const cairo_pattern_t	*source,
				cairo_path_fixed_t	*path,
				const cairo_stroke_style_t	*stroke_style,
				const cairo_matrix_t		*ctm,
				const cairo_matrix_t		*ctm_inverse,
				double			 tolerance,
				cairo_antialias_t	 antialias,
				cairo_clip_t		*clip)
{
    cairo_polygon_t polygon;
    cairo_traps_t traps;
    cairo_box_t boxes_stack[32], *clip_boxes = boxes_stack;
    int num_boxes = ARRAY_LENGTH (boxes_stack);
    cairo_composite_rectangles_t extents;
    cairo_rectangle_int_t rect;
    cairo_status_t status;

    if (!_cairo_surface_get_extents (surface, &rect))
        ASSERT_NOT_REACHED;

    status = _cairo_composite_rectangles_init_for_stroke (&extents,
							  &rect,
							  op, source,
							  path, stroke_style, ctm,
							  clip);
    if (unlikely (status))
	return status;

    if (_cairo_clip_contains_extents (clip, &extents))
	clip = NULL;

    status = _cairo_clip_to_boxes (&clip, &extents, &clip_boxes, &num_boxes);
    if (unlikely (status))
	return status;

    _cairo_polygon_init (&polygon);
    _cairo_polygon_limit (&polygon, clip_boxes, num_boxes);

    _cairo_traps_init (&traps);
    _cairo_traps_limit (&traps, clip_boxes, num_boxes);

    if (path->is_rectilinear) {
	status = _cairo_path_fixed_stroke_rectilinear_to_traps (path,
								stroke_style,
								ctm,
								&traps);
	if (likely (status == CAIRO_STATUS_SUCCESS))
	    goto DO_TRAPS;

	if (_cairo_status_is_error (status))
	    goto CLEANUP;
    }

    status = _cairo_path_fixed_stroke_to_polygon (path,
						  stroke_style,
						  ctm, ctm_inverse,
						  tolerance,
						  &polygon);
    if (unlikely (status))
	goto CLEANUP;

    if (polygon.num_edges == 0)
	goto DO_TRAPS;

    if (_cairo_operator_bounded_by_mask (op)) {
	_cairo_box_round_to_rectangle (&polygon.extents, &extents.mask);
	if (! _cairo_rectangle_intersect (&extents.bounded, &extents.mask))
	    goto CLEANUP;
    }

    if (antialias != CAIRO_ANTIALIAS_NONE) {
	cairo_composite_spans_info_t info;

	info.polygon = &polygon;
	info.fill_rule = CAIRO_FILL_RULE_WINDING;
	info.antialias = antialias;

	status = _clip_and_composite (clip, op, source,
				      _composite_spans,
				      &info, surface,
                                      extents.is_bounded ? &extents.bounded : &extents.unbounded);
	goto CLEANUP;
    }

    
    status = _cairo_bentley_ottmann_tessellate_polygon (&traps,
							&polygon,
							CAIRO_FILL_RULE_WINDING);
    if (unlikely (status))
	goto CLEANUP;

  DO_TRAPS:
    status = _clip_and_composite_trapezoids (source, op, surface,
					     &traps, antialias,
					     clip,
                                             extents.is_bounded ? &extents.bounded : &extents.unbounded);
  CLEANUP:
    _cairo_traps_fini (&traps);
    _cairo_polygon_fini (&polygon);
    if (clip_boxes != boxes_stack)
	free (clip_boxes);

    return status;
}

cairo_status_t
_cairo_win32_surface_fallback_fill (cairo_surface_t		*surface,
			      cairo_operator_t		 op,
			      const cairo_pattern_t	*source,
			      cairo_path_fixed_t	*path,
			      cairo_fill_rule_t		 fill_rule,
			      double			 tolerance,
			      cairo_antialias_t		 antialias,
			      cairo_clip_t		*clip)
{
    cairo_polygon_t polygon;
    cairo_traps_t traps;
    cairo_box_t boxes_stack[32], *clip_boxes = boxes_stack;
    int num_boxes = ARRAY_LENGTH (boxes_stack);
    cairo_bool_t is_rectilinear;
    cairo_composite_rectangles_t extents;
    cairo_rectangle_int_t rect;
    cairo_status_t status;

    if (!_cairo_surface_get_extents (surface, &rect))
        ASSERT_NOT_REACHED;

    status = _cairo_composite_rectangles_init_for_fill (&extents,
							&rect,
							op, source, path,
							clip);
    if (unlikely (status))
	return status;

    if (_cairo_clip_contains_extents (clip, &extents))
	clip = NULL;

    status = _cairo_clip_to_boxes (&clip, &extents, &clip_boxes, &num_boxes);
    if (unlikely (status))
	return status;

    _cairo_traps_init (&traps);
    _cairo_traps_limit (&traps, clip_boxes, num_boxes);

    _cairo_polygon_init (&polygon);
    _cairo_polygon_limit (&polygon, clip_boxes, num_boxes);

    if (_cairo_path_fixed_fill_is_empty (path))
	goto DO_TRAPS;

    is_rectilinear = _cairo_path_fixed_is_rectilinear_fill (path);
    if (is_rectilinear) {
	status = _cairo_path_fixed_fill_rectilinear_to_traps (path,
							      fill_rule,
							      &traps);
	if (likely (status == CAIRO_STATUS_SUCCESS))
	    goto DO_TRAPS;

	if (_cairo_status_is_error (status))
	    goto CLEANUP;
    }

    status = _cairo_path_fixed_fill_to_polygon (path, tolerance, &polygon);
    if (unlikely (status))
	goto CLEANUP;

    if (polygon.num_edges == 0)
	goto DO_TRAPS;

    if (_cairo_operator_bounded_by_mask (op)) {
	_cairo_box_round_to_rectangle (&polygon.extents, &extents.mask);
	if (! _cairo_rectangle_intersect (&extents.bounded, &extents.mask))
	    goto CLEANUP;
    }

    if (is_rectilinear) {
	status = _cairo_bentley_ottmann_tessellate_rectilinear_polygon (&traps,
									&polygon,
									fill_rule);
	if (likely (status == CAIRO_STATUS_SUCCESS))
	    goto DO_TRAPS;

	if (unlikely (_cairo_status_is_error (status)))
	    goto CLEANUP;
    }


    if (antialias != CAIRO_ANTIALIAS_NONE) {
	cairo_composite_spans_info_t info;

	info.polygon = &polygon;
	info.fill_rule = fill_rule;
	info.antialias = antialias;

	status = _clip_and_composite (clip, op, source,
				      _composite_spans,
				      &info, surface,
                                      extents.is_bounded ? &extents.bounded : &extents.unbounded);
	goto CLEANUP;
    }

    
    status = _cairo_bentley_ottmann_tessellate_polygon (&traps,
							&polygon,
							fill_rule);
    if (unlikely (status))
	goto CLEANUP;

  DO_TRAPS:
    status = _clip_and_composite_trapezoids (source, op, surface,
					     &traps, antialias,
					     clip,
                                             extents.is_bounded ? &extents.bounded : &extents.unbounded);
  CLEANUP:
    _cairo_traps_fini (&traps);
    _cairo_polygon_fini (&polygon);
    if (clip_boxes != boxes_stack)
	free (clip_boxes);

    return status;
}

static const cairo_surface_backend_t cairo_win32_surface_backend = {
    CAIRO_SURFACE_TYPE_WIN32,
    _cairo_win32_surface_create_similar,
    _cairo_win32_surface_finish,
    _cairo_win32_surface_acquire_source_image,
    _cairo_win32_surface_release_source_image,
    _cairo_win32_surface_acquire_dest_image,
    _cairo_win32_surface_release_dest_image,
    NULL, 
    _cairo_win32_surface_composite,
    _cairo_win32_surface_fill_rectangles,
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    _cairo_win32_surface_get_extents,
    NULL, 
    NULL, 
    _cairo_win32_surface_flush,
    NULL, 
    NULL, 
    NULL, 

    _cairo_win32_surface_fallback_paint,
    _cairo_win32_surface_fallback_mask,
    _cairo_win32_surface_fallback_stroke,
    _cairo_win32_surface_fallback_fill,
    _cairo_win32_surface_show_glyphs,

    NULL,  
    _cairo_win32_surface_is_similar,
};
















cairo_int_status_t
_cairo_win32_save_initial_clip (HDC hdc, cairo_win32_surface_t *surface)
{
    RECT rect;
    int clipBoxType;
    int gm;
    XFORM saved_xform;

    












    gm = GetGraphicsMode (hdc);
    if (gm == GM_ADVANCED) {
	GetWorldTransform (hdc, &saved_xform);
	ModifyWorldTransform (hdc, NULL, MWT_IDENTITY);
    }

    clipBoxType = GetClipBox (hdc, &rect);
    if (clipBoxType == ERROR) {
	_cairo_win32_print_gdi_error ("cairo_win32_surface_create");
	SetGraphicsMode (hdc, gm);
	
	return _cairo_error (CAIRO_STATUS_NO_MEMORY);
    }

    surface->clip_rect.x = rect.left;
    surface->clip_rect.y = rect.top;
    surface->clip_rect.width = rect.right - rect.left;
    surface->clip_rect.height = rect.bottom - rect.top;

    surface->initial_clip_rgn = NULL;
    surface->had_simple_clip = FALSE;

    if (clipBoxType == COMPLEXREGION) {
	surface->initial_clip_rgn = CreateRectRgn (0, 0, 0, 0);
	if (GetClipRgn (hdc, surface->initial_clip_rgn) <= 0) {
	    DeleteObject(surface->initial_clip_rgn);
	    surface->initial_clip_rgn = NULL;
	}
    } else if (clipBoxType == SIMPLEREGION) {
	surface->had_simple_clip = TRUE;
    }

    if (gm == GM_ADVANCED)
	SetWorldTransform (hdc, &saved_xform);

    return CAIRO_STATUS_SUCCESS;
}

cairo_int_status_t
_cairo_win32_restore_initial_clip (cairo_win32_surface_t *surface)
{
    cairo_int_status_t status = CAIRO_STATUS_SUCCESS;

    XFORM saved_xform;
    int gm = GetGraphicsMode (surface->dc);
    if (gm == GM_ADVANCED) {
	GetWorldTransform (surface->dc, &saved_xform);
	ModifyWorldTransform (surface->dc, NULL, MWT_IDENTITY);
    }

    
    SelectClipRgn (surface->dc, surface->initial_clip_rgn);

    if (surface->had_simple_clip) {
	
	IntersectClipRect (surface->dc,
			   surface->clip_rect.x,
			   surface->clip_rect.y,
			   surface->clip_rect.x + surface->clip_rect.width,
			   surface->clip_rect.y + surface->clip_rect.height);
    }

    if (gm == GM_ADVANCED)
	SetWorldTransform (surface->dc, &saved_xform);

    return status;
}

void
_cairo_win32_debug_dump_hrgn (HRGN rgn, char *header)
{
    RGNDATA *rd;
    unsigned int z;

    if (header)
	fprintf (stderr, "%s\n", header);

    if (rgn == NULL) {
	fprintf (stderr, " NULL\n");
    }

    z = GetRegionData(rgn, 0, NULL);
    rd = (RGNDATA*) malloc(z);
    z = GetRegionData(rgn, z, rd);

    fprintf (stderr, " %ld rects, bounds: %ld %ld %ld %ld\n",
	     rd->rdh.nCount,
	     rd->rdh.rcBound.left,
	     rd->rdh.rcBound.top,
	     rd->rdh.rcBound.right - rd->rdh.rcBound.left,
	     rd->rdh.rcBound.bottom - rd->rdh.rcBound.top);

    for (z = 0; z < rd->rdh.nCount; z++) {
	RECT r = ((RECT*)rd->Buffer)[z];
	fprintf (stderr, " [%d]: [%ld %ld %ld %ld]\n",
		 z, r.left, r.top, r.right - r.left, r.bottom - r.top);
    }

    free(rd);
    fflush (stderr);
}


















cairo_status_t
cairo_win32_surface_set_can_convert_to_dib (cairo_surface_t *asurface, cairo_bool_t can_convert)
{
    cairo_win32_surface_t *surface = (cairo_win32_surface_t*) asurface;
    if (surface->base.type != CAIRO_SURFACE_TYPE_WIN32)
	return CAIRO_STATUS_SURFACE_TYPE_MISMATCH;

    if (surface->bitmap) {
	if (can_convert)
	    surface->flags |= CAIRO_WIN32_SURFACE_CAN_CONVERT_TO_DIB;
	else
	    surface->flags &= ~CAIRO_WIN32_SURFACE_CAN_CONVERT_TO_DIB;
    }

    return CAIRO_STATUS_SUCCESS;
}














cairo_status_t
cairo_win32_surface_get_can_convert_to_dib (cairo_surface_t *asurface, cairo_bool_t *can_convert)
{
    cairo_win32_surface_t *surface = (cairo_win32_surface_t*) asurface;
    if (surface->base.type != CAIRO_SURFACE_TYPE_WIN32)
	return CAIRO_STATUS_SURFACE_TYPE_MISMATCH;

    *can_convert = ((surface->flags & CAIRO_WIN32_SURFACE_CAN_CONVERT_TO_DIB) != 0);
    return CAIRO_STATUS_SUCCESS;
}

int
cairo_win32_surface_get_width (cairo_surface_t *asurface)
{
    cairo_win32_surface_t *surface = (cairo_win32_surface_t*) asurface;
    if (surface->base.type != CAIRO_SURFACE_TYPE_WIN32)
	return CAIRO_STATUS_SURFACE_TYPE_MISMATCH;

    return surface->extents.width;
}

int
cairo_win32_surface_get_height (cairo_surface_t *asurface)
{
    cairo_win32_surface_t *surface = (cairo_win32_surface_t*) asurface;
    if (surface->base.type != CAIRO_SURFACE_TYPE_WIN32)
	return CAIRO_STATUS_SURFACE_TYPE_MISMATCH;

    return surface->extents.height;
}
