
























#ifndef _PIXMAN_XSERVER_COMPAT_H_
#define _PIXMAN_XSERVER_COMPAT_H_



















#include "icint.h"






















#define RENDER 1









#define pDrawable pixels
#define fbGetDrawable(pDrawable, buf, outstride, outbpp, xoff, yoff) { \
    (buf) = (pDrawable)->data; \
    (outstride) = ((int) pDrawable->stride) / sizeof (pixman_bits_t); \
    (outbpp) = (pDrawable)->bpp; \
    (xoff) = 0; \
    (yoff) = 0; \
}


#define RepeatNone                          0
#define RepeatNormal                        1
#define RepeatPad                           2
#define RepeatReflect                       3

typedef pixman_vector_t PictVector;
typedef pixman_vector_t* PictVectorPtr;

#define miIndexedPtr FbIndexedPtr
#define miIndexToEnt24 FbIndexToEnt24
#define miIndexToEntY24 FbIndexToEntY24

#define MAX_FIXED_48_16	    ((xFixed_48_16) 0x7fffffff)
#define MIN_FIXED_48_16	    (-((xFixed_48_16) 1 << 31))




#include "renderedge.h"






pixman_private void
fbRasterizeEdges (pixman_bits_t		*buf,
		  int			bpp,
		  int			width,
		  int			stride,
		  RenderEdge		*l,
		  RenderEdge	 	*r,
		  pixman_fixed16_16_t	t,
		  pixman_fixed16_16_t	b);

#endif
