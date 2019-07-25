


























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pixman-private.h"
#include "pixman-combine32.h"
#include "pixman-private.h"

#define SCANLINE_BUFFER_LENGTH 8192

static void
general_composite_rect  (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         pixman_image_t *         src,
                         pixman_image_t *         mask,
                         pixman_image_t *         dest,
                         int32_t                  src_x,
                         int32_t                  src_y,
                         int32_t                  mask_x,
                         int32_t                  mask_y,
                         int32_t                  dest_x,
                         int32_t                  dest_y,
                         int32_t                  width,
                         int32_t                  height)
{
    uint8_t stack_scanline_buffer[SCANLINE_BUFFER_LENGTH * 3];
    uint8_t *scanline_buffer = stack_scanline_buffer;
    uint8_t *src_buffer, *mask_buffer, *dest_buffer;
    fetch_scanline_t fetch_src = NULL, fetch_mask = NULL, fetch_dest = NULL;
    pixman_combine_32_func_t compose;
    store_scanline_t store;
    source_image_class_t src_class, mask_class;
    pixman_bool_t component_alpha;
    uint32_t *bits;
    int32_t stride;
    int narrow, Bpp;
    int i;

    narrow =
	(src->common.flags & FAST_PATH_NARROW_FORMAT)		&&
	(!mask || mask->common.flags & FAST_PATH_NARROW_FORMAT)	&&
	(dest->common.flags & FAST_PATH_NARROW_FORMAT);
    Bpp = narrow ? 4 : 8;

    if (width * Bpp > SCANLINE_BUFFER_LENGTH)
    {
	scanline_buffer = pixman_malloc_abc (width, 3, Bpp);

	if (!scanline_buffer)
	    return;
    }

    src_buffer = scanline_buffer;
    mask_buffer = src_buffer + width * Bpp;
    dest_buffer = mask_buffer + width * Bpp;

    src_class = _pixman_image_classify (src,
                                        src_x, src_y,
                                        width, height);

    mask_class = SOURCE_IMAGE_CLASS_UNKNOWN;

    if (mask)
    {
	mask_class = _pixman_image_classify (mask,
	                                     src_x, src_y,
	                                     width, height);
    }

    if (op == PIXMAN_OP_CLEAR)
	fetch_src = NULL;
    else if (narrow)
	fetch_src = _pixman_image_get_scanline_32;
    else
	fetch_src = _pixman_image_get_scanline_64;

    if (!mask || op == PIXMAN_OP_CLEAR)
	fetch_mask = NULL;
    else if (narrow)
	fetch_mask = _pixman_image_get_scanline_32;
    else
	fetch_mask = _pixman_image_get_scanline_64;

    if (op == PIXMAN_OP_CLEAR || op == PIXMAN_OP_SRC)
	fetch_dest = NULL;
    else if (narrow)
	fetch_dest = _pixman_image_get_scanline_32;
    else
	fetch_dest = _pixman_image_get_scanline_64;

    if (narrow)
	store = _pixman_image_store_scanline_32;
    else
	store = _pixman_image_store_scanline_64;

    







    if ((dest->bits.format == PIXMAN_a8r8g8b8)	||
	(dest->bits.format == PIXMAN_x8r8g8b8	&&
	 (op == PIXMAN_OP_OVER		||
	  op == PIXMAN_OP_ADD		||
	  op == PIXMAN_OP_SRC		||
	  op == PIXMAN_OP_CLEAR		||
	  op == PIXMAN_OP_IN_REVERSE	||
	  op == PIXMAN_OP_OUT_REVERSE	||
	  op == PIXMAN_OP_DST)))
    {
	if (narrow &&
	    !dest->common.alpha_map &&
	    !dest->bits.write_func)
	{
	    store = NULL;
	}
    }

    if (!store)
    {
	bits = dest->bits.bits;
	stride = dest->bits.rowstride;
    }
    else
    {
	bits = NULL;
	stride = 0;
    }

    component_alpha =
        fetch_src                       &&
        fetch_mask                      &&
        mask                            &&
        mask->common.type == BITS       &&
        mask->common.component_alpha    &&
        PIXMAN_FORMAT_RGB (mask->bits.format);

    if (narrow)
    {
	if (component_alpha)
	    compose = _pixman_implementation_combine_32_ca;
	else
	    compose = _pixman_implementation_combine_32;
    }
    else
    {
	if (component_alpha)
	    compose = (pixman_combine_32_func_t)_pixman_implementation_combine_64_ca;
	else
	    compose = (pixman_combine_32_func_t)_pixman_implementation_combine_64;
    }

    if (!compose)
	return;

    if (!fetch_mask)
	mask_buffer = NULL;

    for (i = 0; i < height; ++i)
    {
	
	if (fetch_src)
	{
	    if (fetch_mask)
	    {
		

		fetch_mask (mask, mask_x, mask_y + i,
		            width, (void *)mask_buffer, 0);

		if (mask_class == SOURCE_IMAGE_CLASS_HORIZONTAL)
		    fetch_mask = NULL;
	    }

	    if (src_class == SOURCE_IMAGE_CLASS_HORIZONTAL)
	    {
		fetch_src (src, src_x, src_y + i,
		           width, (void *)src_buffer, 0);
		fetch_src = NULL;
	    }
	    else
	    {
		fetch_src (src, src_x, src_y + i,
		           width, (void *)src_buffer, (void *)mask_buffer);
	    }
	}
	else if (fetch_mask)
	{
	    fetch_mask (mask, mask_x, mask_y + i,
	                width, (void *)mask_buffer, 0);
	}

	if (store)
	{
	    
	    if (fetch_dest)
	    {
		fetch_dest (dest, dest_x, dest_y + i,
		            width, (void *)dest_buffer, 0);
	    }

	    
	    compose (imp->toplevel, op,
		     (void *)dest_buffer,
		     (void *)src_buffer,
		     (void *)mask_buffer,
		     width);

	    
	    store (&(dest->bits), dest_x, dest_y + i, width,
	           (void *)dest_buffer);
	}
	else
	{
	    
	    compose (imp->toplevel, op,
		     bits + (dest_y + i) * stride + dest_x,
	             (void *)src_buffer, (void *)mask_buffer, width);
	}
    }

    if (scanline_buffer != stack_scanline_buffer)
	free (scanline_buffer);
}

static const pixman_fast_path_t general_fast_path[] =
{
    { PIXMAN_OP_any, PIXMAN_any, 0, PIXMAN_any,	0, PIXMAN_any, 0, general_composite_rect },
    { PIXMAN_OP_NONE }
};

static pixman_bool_t
general_blt (pixman_implementation_t *imp,
             uint32_t *               src_bits,
             uint32_t *               dst_bits,
             int                      src_stride,
             int                      dst_stride,
             int                      src_bpp,
             int                      dst_bpp,
             int                      src_x,
             int                      src_y,
             int                      dst_x,
             int                      dst_y,
             int                      width,
             int                      height)
{
    

    return FALSE;
}

static pixman_bool_t
general_fill (pixman_implementation_t *imp,
              uint32_t *               bits,
              int                      stride,
              int                      bpp,
              int                      x,
              int                      y,
              int                      width,
              int                      height,
              uint32_t xor)
{
    return FALSE;
}

pixman_implementation_t *
_pixman_implementation_create_general (void)
{
    pixman_implementation_t *imp = _pixman_implementation_create (NULL, general_fast_path);

    _pixman_setup_combiner_functions_32 (imp);
    _pixman_setup_combiner_functions_64 (imp);

    imp->blt = general_blt;
    imp->fill = general_fill;

    return imp;
}

