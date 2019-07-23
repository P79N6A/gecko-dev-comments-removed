






















#include "pixmanint.h"

#define Mask(n)	((n) == 32 ? 0xffffffff : (unsigned) ((1 << (n))-1))

int
pixman_format_init (pixman_format_t *format, pixman_format_name_t name)
{
    switch (name) {
    case PIXMAN_FORMAT_NAME_ARGB32:
	pixman_format_init_masks (format, 32,
				  0xff000000,
				  0x00ff0000,
				  0x0000ff00,
				  0x000000ff);
	break;

    case PIXMAN_FORMAT_NAME_RGB24:
	pixman_format_init_masks (format, 32,
				  0x0,
				  0xff0000,
				  0x00ff00,
				  0x0000ff);
	break;

    case PIXMAN_FORMAT_NAME_A8:
	pixman_format_init_masks (format, 8,
		                  0xff,
				  0,
				  0,
				  0);
	break;

    case PIXMAN_FORMAT_NAME_A1:
	pixman_format_init_masks (format, 1,
	                          0x1,
				  0,
				  0,
				  0);
	break;

    case PIXMAN_FORMAT_NAME_RGB16_565:
	pixman_format_init_masks (format, 16,
				  0x0,
				  0xf800,
				  0x07e0,
				  0x001f);
	break;

    case PIXMAN_FORMAT_NAME_ABGR32:
	pixman_format_init_masks (format, 32,
				  0xff000000,
				  0x000000ff,
				  0x0000ff00,
				  0x00ff0000);
	break;

    case PIXMAN_FORMAT_NAME_BGR24:
	pixman_format_init_masks (format, 32,
				  0x0,
				  0x000000ff,
				  0x0000ff00,
				  0x00ff0000);
	break;

    default:
	return 0;
    }

    return 1;
}






void
pixman_format_init_masks (pixman_format_t *format,
	                    int bpp,
			    int alpha_mask,
			    int red_mask,
			    int green_mask,
			    int blue_mask)
{
    int type;
    int format_code;

    if (red_mask == 0 && green_mask == 0 && blue_mask == 0)
	type = PICT_TYPE_A;
    else if (red_mask > blue_mask)
	type = PICT_TYPE_ARGB;
    else
	type = PICT_TYPE_ABGR;

    format_code = PICT_FORMAT (bpp, type,
			       _FbOnes (alpha_mask),
			       _FbOnes (red_mask),
			       _FbOnes (green_mask),
			       _FbOnes (blue_mask));

    pixman_format_init_code (format, format_code);
}

void
pixman_format_init_code (pixman_format_t *format, int format_code)
{
    memset (format, 0, sizeof (pixman_format_t));




    format->format_code = format_code;

    switch (PICT_FORMAT_TYPE(format_code)) {
    case PICT_TYPE_ARGB:

	format->alphaMask = Mask(PICT_FORMAT_A(format_code));
	if (format->alphaMask)
	    format->alpha = (PICT_FORMAT_R(format_code) +
			     PICT_FORMAT_G(format_code) +
			     PICT_FORMAT_B(format_code));

	format->redMask = Mask(PICT_FORMAT_R(format_code));
	format->red = (PICT_FORMAT_G(format_code) +
		       PICT_FORMAT_B(format_code));

	format->greenMask = Mask(PICT_FORMAT_G(format_code));
	format->green = PICT_FORMAT_B(format_code);

	format->blueMask = Mask(PICT_FORMAT_B(format_code));
	format->blue = 0;
	break;

    case PICT_TYPE_ABGR:

	format->alphaMask = Mask(PICT_FORMAT_A(format_code));
	if (format->alphaMask)
	    format->alpha = (PICT_FORMAT_B(format_code) +
			     PICT_FORMAT_G(format_code) +
			     PICT_FORMAT_R(format_code));

	format->blueMask = Mask(PICT_FORMAT_B(format_code));
	format->blue = (PICT_FORMAT_G(format_code) +
			PICT_FORMAT_R(format_code));

	format->greenMask = Mask(PICT_FORMAT_G(format_code));
	format->green = PICT_FORMAT_R(format_code);

	format->redMask = Mask(PICT_FORMAT_R(format_code));
	format->red = 0;
	break;

    case PICT_TYPE_A:

	format->alpha = 0;
	format->alphaMask = Mask(PICT_FORMAT_A(format_code));

	
	break;
    }

    format->depth = _FbOnes ((format->alphaMask << format->alpha) |
			     (format->redMask << format->red) |
			     (format->blueMask << format->blue) |
			     (format->greenMask << format->green));
}

void
pixman_format_get_masks (pixman_format_t *format,
                         unsigned int *bpp,
                         unsigned int *alpha_mask,
                         unsigned int *red_mask,
                         unsigned int *green_mask,
                         unsigned int *blue_mask)
{
    *bpp = PICT_FORMAT_BPP (format->format_code);

    if (format->alphaMask)
	*alpha_mask = format->alphaMask << format->alpha;
    else
	*alpha_mask = 0;

    if (format->redMask)
	*red_mask = format->redMask << format->red;
    else
	*red_mask = 0;

    if (format->greenMask)
	*green_mask = format->greenMask << format->green;
    else
	*green_mask = 0;

    if (format->blueMask)
	*blue_mask = format->blueMask << format->blue;
    else
	*blue_mask = 0;
}
