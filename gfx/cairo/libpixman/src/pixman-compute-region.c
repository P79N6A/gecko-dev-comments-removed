






















#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include "pixman-private.h"

#define BOUND(v)	(int16_t) ((v) < INT16_MIN ? INT16_MIN : (v) > INT16_MAX ? INT16_MAX : (v))

static inline pixman_bool_t
miClipPictureReg (pixman_region16_t *	pRegion,
		  pixman_region16_t *	pClip,
		  int		dx,
		  int		dy)
{
    if (pixman_region_n_rects(pRegion) == 1 &&
	pixman_region_n_rects(pClip) == 1)
    {
	pixman_box16_t *  pRbox = pixman_region_rectangles(pRegion, NULL);
	pixman_box16_t *  pCbox = pixman_region_rectangles(pClip, NULL);
	int	v;
	
	if (pRbox->x1 < (v = pCbox->x1 + dx))
	    pRbox->x1 = BOUND(v);
	if (pRbox->x2 > (v = pCbox->x2 + dx))
	    pRbox->x2 = BOUND(v);
	if (pRbox->y1 < (v = pCbox->y1 + dy))
	    pRbox->y1 = BOUND(v);
	if (pRbox->y2 > (v = pCbox->y2 + dy))
	    pRbox->y2 = BOUND(v);
	if (pRbox->x1 >= pRbox->x2 ||
	    pRbox->y1 >= pRbox->y2)
	{
	    pixman_region_init (pRegion);
	}
    }
    else if (!pixman_region_not_empty (pClip))
	return FALSE;
    else
    {
	if (dx || dy)
	    pixman_region_translate (pRegion, -dx, -dy);
	if (!pixman_region_intersect (pRegion, pRegion, pClip))
	    return FALSE;
	if (dx || dy)
	    pixman_region_translate(pRegion, dx, dy);
    }
    return pixman_region_not_empty(pRegion);
}


static inline pixman_bool_t
miClipPictureSrc (pixman_region16_t *	pRegion,
		  pixman_image_t *	pPicture,
		  int		dx,
		  int		dy)
{
    
    if (pPicture->common.transform || pPicture->type != BITS)
	return TRUE;

    if (pPicture->common.repeat)
    {
	










	if (pPicture->common.has_client_clip)
	{
	    pixman_region_translate ( pRegion, dx, dy);
	    
	    if (!pixman_region_intersect (pRegion, pRegion, 
					  (pixman_region16_t *) pPicture->common.src_clip))
		return FALSE;
	    
	    pixman_region_translate ( pRegion, -dx, -dy);
	}
	    
	return TRUE;
    }
    else
    {
	return miClipPictureReg (pRegion,
				 pPicture->common.src_clip,
				 dx,
				 dy);
    }
}






pixman_bool_t
pixman_compute_composite_region (pixman_region16_t *	pRegion,
				 pixman_image_t *	pSrc,
				 pixman_image_t *	pMask,
				 pixman_image_t *	pDst,
				 int16_t		xSrc,
				 int16_t		ySrc,
				 int16_t		xMask,
				 int16_t		yMask,
				 int16_t		xDst,
				 int16_t		yDst,
				 uint16_t	width,
				 uint16_t	height)
{
    int		v;
    
    pRegion->extents.x1 = xDst;
    v = xDst + width;
    pRegion->extents.x2 = BOUND(v);
    pRegion->extents.y1 = yDst;
    v = yDst + height;
    pRegion->extents.y2 = BOUND(v);
    pRegion->data = 0;
    
    if (pRegion->extents.x1 >= pRegion->extents.x2 ||
	pRegion->extents.y1 >= pRegion->extents.y2)
    {
	pixman_region_init (pRegion);
	return FALSE;
    }
    
    if (!miClipPictureReg (pRegion, &pDst->common.clip_region, 0, 0))
    {
	pixman_region_fini (pRegion);
	return FALSE;
    }
    if (pDst->common.alpha_map)
    {
	if (!miClipPictureReg (pRegion, &pDst->common.alpha_map->common.clip_region,
			       -pDst->common.alpha_origin.x,
			       -pDst->common.alpha_origin.y))
	{
	    pixman_region_fini (pRegion);
	    return FALSE;
	}
    }
    
    if (!miClipPictureSrc (pRegion, pSrc, xDst - xSrc, yDst - ySrc))
    {
	pixman_region_fini (pRegion);
	return FALSE;
    }
    if (pSrc->common.alpha_map)
    {
	if (!miClipPictureSrc (pRegion, (pixman_image_t *)pSrc->common.alpha_map,
			       xDst - (xSrc + pSrc->common.alpha_origin.x),
			       yDst - (ySrc + pSrc->common.alpha_origin.y)))
	{
	    pixman_region_fini (pRegion);
	    return FALSE;
	}
    }
    
    if (pMask)
    {
	if (!miClipPictureSrc (pRegion, pMask, xDst - xMask, yDst - yMask))
	{
	    pixman_region_fini (pRegion);
	    return FALSE;
	}	
	if (pMask->common.alpha_map)
	{
	    if (!miClipPictureSrc (pRegion, (pixman_image_t *)pMask->common.alpha_map,
				   xDst - (xMask + pMask->common.alpha_origin.x),
				   yDst - (yMask + pMask->common.alpha_origin.y)))
	    {
		pixman_region_fini (pRegion);
		return FALSE;
	    }
	}
    }
    
    return TRUE;
}
