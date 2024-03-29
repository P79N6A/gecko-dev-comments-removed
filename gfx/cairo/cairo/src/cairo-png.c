





































#include "cairoint.h"

#include "cairo-error-private.h"
#include "cairo-output-stream-private.h"

#include <stdio.h>
#include <errno.h>
#include <png.h>



















struct png_read_closure_t {
    cairo_read_func_t		 read_func;
    void			*closure;
    cairo_output_stream_t	*png_data;
};



static void
unpremultiply_data (png_structp png, png_row_infop row_info, png_bytep data)
{
    unsigned int i;

    for (i = 0; i < row_info->rowbytes; i += 4) {
        uint8_t *b = &data[i];
        uint32_t pixel;
        uint8_t  alpha;

	memcpy (&pixel, b, sizeof (uint32_t));
	alpha = (pixel & 0xff000000) >> 24;
        if (alpha == 0) {
	    b[0] = b[1] = b[2] = b[3] = 0;
	} else {
            b[0] = (((pixel & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
            b[1] = (((pixel & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
            b[2] = (((pixel & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
	    b[3] = alpha;
	}
    }
}


static void
convert_data_to_bytes (png_structp png, png_row_infop row_info, png_bytep data)
{
    unsigned int i;

    for (i = 0; i < row_info->rowbytes; i += 4) {
        uint8_t *b = &data[i];
        uint32_t pixel;

	memcpy (&pixel, b, sizeof (uint32_t));

	b[0] = (pixel & 0xff0000) >> 16;
	b[1] = (pixel & 0x00ff00) >>  8;
	b[2] = (pixel & 0x0000ff) >>  0;
	b[3] = 0;
    }
}





static void
png_simple_error_callback (png_structp png,
	                   png_const_charp error_msg)
{
    cairo_status_t *error = png_get_error_ptr (png);

    
    if (*error == CAIRO_STATUS_SUCCESS)
	*error = _cairo_error (CAIRO_STATUS_NO_MEMORY);

#ifdef PNG_SETJMP_SUPPORTED
    longjmp (png_jmpbuf (png), 1);
#endif

    
}

static void
png_simple_warning_callback (png_structp png,
	                     png_const_charp error_msg)
{
    cairo_status_t *error = png_get_error_ptr (png);

    
    if (*error == CAIRO_STATUS_SUCCESS)
	*error = _cairo_error (CAIRO_STATUS_NO_MEMORY);

    
}




static void
png_simple_output_flush_fn (png_structp png_ptr)
{
}

static cairo_status_t
write_png (cairo_surface_t	*surface,
	   png_rw_ptr		write_func,
	   void			*closure)
{
    int i;
    cairo_status_t status;
    cairo_image_surface_t *image;
    cairo_image_surface_t * volatile clone;
    void *image_extra;
    png_struct *png;
    png_info *info;
    png_byte **volatile rows = NULL;
    png_color_16 white;
    int png_color_type;
    int depth;

    status = _cairo_surface_acquire_source_image (surface,
						  &image,
						  &image_extra);

    if (status == CAIRO_INT_STATUS_UNSUPPORTED)
	return _cairo_error (CAIRO_STATUS_SURFACE_TYPE_MISMATCH);
    else if (unlikely (status))
        return status;

    
    if (image->width == 0 || image->height == 0) {
	status = _cairo_error (CAIRO_STATUS_WRITE_ERROR);
	goto BAIL1;
    }

    


    clone = _cairo_image_surface_coerce (image);
    status = clone->base.status;
    if (unlikely (status))
        goto BAIL1;

    rows = _cairo_malloc_ab (clone->height, sizeof (png_byte*));
    if (unlikely (rows == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto BAIL2;
    }

    for (i = 0; i < clone->height; i++)
	rows[i] = (png_byte *) clone->data + i * clone->stride;

    png = png_create_write_struct (PNG_LIBPNG_VER_STRING, &status,
	                           png_simple_error_callback,
	                           png_simple_warning_callback);
    if (unlikely (png == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto BAIL3;
    }

    info = png_create_info_struct (png);
    if (unlikely (info == NULL)) {
	status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	goto BAIL4;
    }

#ifdef PNG_SETJMP_SUPPORTED
    if (setjmp (png_jmpbuf (png)))
	goto BAIL4;
#endif

    png_set_write_fn (png, closure, write_func, png_simple_output_flush_fn);

    switch (clone->format) {
    case CAIRO_FORMAT_ARGB32:
	depth = 8;
	if (_cairo_image_analyze_transparency (clone) == CAIRO_IMAGE_IS_OPAQUE)
	    png_color_type = PNG_COLOR_TYPE_RGB;
	else
	    png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	break;
    case CAIRO_FORMAT_RGB24:
	depth = 8;
	png_color_type = PNG_COLOR_TYPE_RGB;
	break;
    case CAIRO_FORMAT_A8:
	depth = 8;
	png_color_type = PNG_COLOR_TYPE_GRAY;
	break;
    case CAIRO_FORMAT_A1:
	depth = 1;
	png_color_type = PNG_COLOR_TYPE_GRAY;
#ifndef WORDS_BIGENDIAN
	png_set_packswap (png);
#endif
	break;
    case CAIRO_FORMAT_INVALID:
    case CAIRO_FORMAT_RGB16_565:
    default:
	status = _cairo_error (CAIRO_STATUS_INVALID_FORMAT);
	goto BAIL4;
    }

    png_set_IHDR (png, info,
		  clone->width,
		  clone->height, depth,
		  png_color_type,
		  PNG_INTERLACE_NONE,
		  PNG_COMPRESSION_TYPE_DEFAULT,
		  PNG_FILTER_TYPE_DEFAULT);

    white.gray = (1 << depth) - 1;
    white.red = white.blue = white.green = white.gray;
    png_set_bKGD (png, info, &white);

    if (0) { 
	png_time pt;

	png_convert_from_time_t (&pt, time (NULL));
	png_set_tIME (png, info, &pt);
    }

    



    png_write_info (png, info);

    if (png_color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
	png_set_write_user_transform_fn (png, unpremultiply_data);
    } else if (png_color_type == PNG_COLOR_TYPE_RGB) {
	png_set_write_user_transform_fn (png, convert_data_to_bytes);
	png_set_filler (png, 0, PNG_FILLER_AFTER);
    }

    png_write_image (png, rows);
    png_write_end (png, info);

BAIL4:
    png_destroy_write_struct (&png, &info);
BAIL3:
    free (rows);
BAIL2:
    cairo_surface_destroy (&clone->base);
BAIL1:
    _cairo_surface_release_source_image (surface, image, image_extra);

    return status;
}

static void
stdio_write_func (png_structp png, png_bytep data, png_size_t size)
{
    FILE *fp;

    fp = png_get_io_ptr (png);
    while (size) {
	size_t ret = fwrite (data, 1, size, fp);
	size -= ret;
	data += ret;
	if (size && ferror (fp)) {
	    cairo_status_t *error = png_get_error_ptr (png);
	    if (*error == CAIRO_STATUS_SUCCESS)
		*error = _cairo_error (CAIRO_STATUS_WRITE_ERROR);
	    png_error (png, NULL);
	}
    }
}
















cairo_status_t
cairo_surface_write_to_png (cairo_surface_t	*surface,
			    const char		*filename)
{
    FILE *fp;
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return _cairo_error (CAIRO_STATUS_SURFACE_FINISHED);

    fp = fopen (filename, "wb");
    if (fp == NULL) {
	switch (errno) {
	case ENOMEM:
	    return _cairo_error (CAIRO_STATUS_NO_MEMORY);
	default:
	    return _cairo_error (CAIRO_STATUS_WRITE_ERROR);
	}
    }

    status = write_png (surface, stdio_write_func, fp);

    if (fclose (fp) && status == CAIRO_STATUS_SUCCESS)
	status = _cairo_error (CAIRO_STATUS_WRITE_ERROR);

    return status;
}

struct png_write_closure_t {
    cairo_write_func_t		 write_func;
    void			*closure;
};

static void
stream_write_func (png_structp png, png_bytep data, png_size_t size)
{
    cairo_status_t status;
    struct png_write_closure_t *png_closure;

    png_closure = png_get_io_ptr (png);
    status = png_closure->write_func (png_closure->closure, data, size);
    if (unlikely (status)) {
	cairo_status_t *error = png_get_error_ptr (png);
	if (*error == CAIRO_STATUS_SUCCESS)
	    *error = status;
	png_error (png, NULL);
    }
}















cairo_status_t
cairo_surface_write_to_png_stream (cairo_surface_t	*surface,
				   cairo_write_func_t	write_func,
				   void			*closure)
{
    struct png_write_closure_t png_closure;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return _cairo_error (CAIRO_STATUS_SURFACE_FINISHED);

    png_closure.write_func = write_func;
    png_closure.closure = closure;

    return write_png (surface, stream_write_func, &png_closure);
}
slim_hidden_def (cairo_surface_write_to_png_stream);

static inline int
multiply_alpha (int alpha, int color)
{
    int temp = (alpha * color) + 0x80;
    return ((temp + (temp >> 8)) >> 8);
}


static void
premultiply_data (png_structp   png,
                  png_row_infop row_info,
                  png_bytep     data)
{
    unsigned int i;

    for (i = 0; i < row_info->rowbytes; i += 4) {
	uint8_t *base  = &data[i];
	uint8_t  alpha = base[3];
	uint32_t p;

	if (alpha == 0) {
	    p = 0;
	} else {
	    uint8_t  red   = base[0];
	    uint8_t  green = base[1];
	    uint8_t  blue  = base[2];

	    if (alpha != 0xff) {
		red   = multiply_alpha (alpha, red);
		green = multiply_alpha (alpha, green);
		blue  = multiply_alpha (alpha, blue);
	    }
	    p = (alpha << 24) | (red << 16) | (green << 8) | (blue << 0);
	}
	memcpy (base, &p, sizeof (uint32_t));
    }
}


static void
convert_bytes_to_data (png_structp png, png_row_infop row_info, png_bytep data)
{
    unsigned int i;

    for (i = 0; i < row_info->rowbytes; i += 4) {
	uint8_t *base  = &data[i];
	uint8_t  red   = base[0];
	uint8_t  green = base[1];
	uint8_t  blue  = base[2];
	uint32_t pixel;

	pixel = (0xff << 24) | (red << 16) | (green << 8) | (blue << 0);
	memcpy (base, &pixel, sizeof (uint32_t));
    }
}

static cairo_status_t
stdio_read_func (void *closure, unsigned char *data, unsigned int size)
{
    FILE *file = closure;

    while (size) {
	size_t ret;

	ret = fread (data, 1, size, file);
	size -= ret;
	data += ret;

	if (size && (feof (file) || ferror (file)))
	    return _cairo_error (CAIRO_STATUS_READ_ERROR);
    }

    return CAIRO_STATUS_SUCCESS;
}

static void
stream_read_func (png_structp png, png_bytep data, png_size_t size)
{
    cairo_status_t status;
    struct png_read_closure_t *png_closure;

    png_closure = png_get_io_ptr (png);
    status = png_closure->read_func (png_closure->closure, data, size);
    if (unlikely (status)) {
	cairo_status_t *error = png_get_error_ptr (png);
	if (*error == CAIRO_STATUS_SUCCESS)
	    *error = status;
	png_error (png, NULL);
    }

    _cairo_output_stream_write (png_closure->png_data, data, size);
}

static cairo_surface_t *
read_png (struct png_read_closure_t *png_closure)
{
    cairo_surface_t *surface;
    png_struct *png = NULL;
    png_info *info;
    png_byte *data = NULL;
    png_byte **row_pointers = NULL;
    png_uint_32 png_width, png_height;
    int depth, color_type, interlace, stride;
    unsigned int i;
    cairo_format_t format;
    cairo_status_t status;
    unsigned char *mime_data;
    unsigned long mime_data_length;

    png_closure->png_data = _cairo_memory_stream_create ();

    
    png = png_create_read_struct (PNG_LIBPNG_VER_STRING,
                                  &status,
	                          png_simple_error_callback,
	                          png_simple_warning_callback);
    if (unlikely (png == NULL)) {
	surface = _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
	goto BAIL;
    }

    info = png_create_info_struct (png);
    if (unlikely (info == NULL)) {
	surface = _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
	goto BAIL;
    }

    png_set_read_fn (png, png_closure, stream_read_func);

    status = CAIRO_STATUS_SUCCESS;
#ifdef PNG_SETJMP_SUPPORTED
    if (setjmp (png_jmpbuf (png))) {
	surface = _cairo_surface_create_in_error (status);
	goto BAIL;
    }
#endif

    png_read_info (png, info);

    png_get_IHDR (png, info,
                  &png_width, &png_height, &depth,
                  &color_type, &interlace, NULL, NULL);
    if (unlikely (status)) { 
	surface = _cairo_surface_create_in_error (status);
	goto BAIL;
    }

    
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb (png);

    
    if (color_type == PNG_COLOR_TYPE_GRAY) {
#if PNG_LIBPNG_VER >= 10209
        png_set_expand_gray_1_2_4_to_8 (png);
#else
        png_set_gray_1_2_4_to_8 (png);
#endif
    }

    
    if (png_get_valid (png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha (png);

    if (depth == 16)
        png_set_strip_16 (png);

    if (depth < 8)
        png_set_packing (png);

    
    if (color_type == PNG_COLOR_TYPE_GRAY ||
	color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
	png_set_gray_to_rgb (png);
    }

    if (interlace != PNG_INTERLACE_NONE)
        png_set_interlace_handling (png);

    png_set_filler (png, 0xff, PNG_FILLER_AFTER);

    
    png_read_update_info (png, info);
    png_get_IHDR (png, info,
                  &png_width, &png_height, &depth,
                  &color_type, &interlace, NULL, NULL);
    if (depth != 8 ||
	! (color_type == PNG_COLOR_TYPE_RGB ||
	   color_type == PNG_COLOR_TYPE_RGB_ALPHA))
    {
	surface = _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_READ_ERROR));
	goto BAIL;
    }

    switch (color_type) {
	default:
	    ASSERT_NOT_REACHED;
	    

	case PNG_COLOR_TYPE_RGB_ALPHA:
	    format = CAIRO_FORMAT_ARGB32;
	    png_set_read_user_transform_fn (png, premultiply_data);
	    break;

	case PNG_COLOR_TYPE_RGB:
	    format = CAIRO_FORMAT_RGB24;
	    png_set_read_user_transform_fn (png, convert_bytes_to_data);
	    break;
    }

    stride = cairo_format_stride_for_width (format, png_width);
    if (stride < 0) {
	surface = _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_INVALID_STRIDE));
	goto BAIL;
    }

    data = _cairo_malloc_ab (png_height, stride);
    if (unlikely (data == NULL)) {
	surface = _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
	goto BAIL;
    }

    row_pointers = _cairo_malloc_ab (png_height, sizeof (char *));
    if (unlikely (row_pointers == NULL)) {
	surface = _cairo_surface_create_in_error (_cairo_error (CAIRO_STATUS_NO_MEMORY));
	goto BAIL;
    }

    for (i = 0; i < png_height; i++)
        row_pointers[i] = &data[i * stride];

    png_read_image (png, row_pointers);
    png_read_end (png, info);

    if (unlikely (status)) { 
	surface = _cairo_surface_create_in_error (status);
	goto BAIL;
    }

    surface = cairo_image_surface_create_for_data (data, format,
						   png_width, png_height,
						   stride);
    if (surface->status)
	goto BAIL;

    _cairo_image_surface_assume_ownership_of_data ((cairo_image_surface_t*)surface);
    data = NULL;

    _cairo_debug_check_image_surface_is_defined (surface);

    status = _cairo_memory_stream_destroy (png_closure->png_data,
					   &mime_data,
					   &mime_data_length);
    png_closure->png_data = NULL;
    if (unlikely (status)) {
	cairo_surface_destroy (surface);
	surface = _cairo_surface_create_in_error (status);
	goto BAIL;
    }

    status = cairo_surface_set_mime_data (surface,
					  CAIRO_MIME_TYPE_PNG,
					  mime_data,
					  mime_data_length,
					  free,
					  mime_data);
    if (unlikely (status)) {
	free (mime_data);
	cairo_surface_destroy (surface);
	surface = _cairo_surface_create_in_error (status);
	goto BAIL;
    }

 BAIL:
    if (row_pointers != NULL)
	free (row_pointers);
    if (data != NULL)
	free (data);
    if (png != NULL)
	png_destroy_read_struct (&png, &info, NULL);
    if (png_closure->png_data != NULL) {
	cairo_status_t status_ignored;

	status_ignored = _cairo_output_stream_destroy (png_closure->png_data);
    }

    return surface;
}





















cairo_surface_t *
cairo_image_surface_create_from_png (const char *filename)
{
    struct png_read_closure_t png_closure;
    cairo_surface_t *surface;

    png_closure.closure = fopen (filename, "rb");
    if (png_closure.closure == NULL) {
	cairo_status_t status;
	switch (errno) {
	case ENOMEM:
	    status = _cairo_error (CAIRO_STATUS_NO_MEMORY);
	    break;
	case ENOENT:
	    status = _cairo_error (CAIRO_STATUS_FILE_NOT_FOUND);
	    break;
	default:
	    status = _cairo_error (CAIRO_STATUS_READ_ERROR);
	    break;
	}
	return _cairo_surface_create_in_error (status);
    }

    png_closure.read_func = stdio_read_func;

    surface = read_png (&png_closure);

    fclose (png_closure.closure);

    return surface;
}






















cairo_surface_t *
cairo_image_surface_create_from_png_stream (cairo_read_func_t	read_func,
					    void		*closure)
{
    struct png_read_closure_t png_closure;

    png_closure.read_func = read_func;
    png_closure.closure = closure;

    return read_png (&png_closure);
}
