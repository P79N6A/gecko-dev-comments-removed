


























#include "pixman-private.h"

#ifdef USE_VMX

pixman_bool_t pixman_have_vmx(void);

#else
#define pixman_have_vmx() FALSE
#endif

#ifdef USE_VMX

#define AVV(x...) {x}

void fbComposeSetupVMX (void);

#if 0
void fbCompositeIn_nx8x8vmx (pixman_operator_t	op,
			     pixman_image_t * pSrc,
			     pixman_image_t * pMask,
			     pixman_image_t * pDst,
			     INT16      xSrc,
			     INT16      ySrc,
			     INT16      xMask,
			     INT16      yMask,
			     INT16      xDst,
			     INT16      yDst,
			     CARD16     width,
			     CARD16     height);

void fbCompositeSolidMask_nx8888x0565Cvmx (pixman_operator_t      op,
					   pixman_image_t * pSrc,
					   pixman_image_t * pMask,
					   pixman_image_t * pDst,
					   INT16      xSrc,
					   INT16      ySrc,
					   INT16      xMask,
					   INT16      yMask,
					   INT16      xDst,
					   INT16      yDst,
					   CARD16     width,
					   CARD16     height);

void fbCompositeSrcAdd_8888x8888vmx (pixman_operator_t	op,
				     pixman_image_t *	pSrc,
				     pixman_image_t *	pMask,
				     pixman_image_t *	pDst,
				     INT16	xSrc,
				     INT16      ySrc,
				     INT16      xMask,
				     INT16      yMask,
				     INT16      xDst,
				     INT16      yDst,
				     CARD16     width,
				     CARD16     height);

void fbCompositeSolidMask_nx8888x8888Cvmx (pixman_operator_t	op,
					   pixman_image_t *	pSrc,
					   pixman_image_t *	pMask,
					   pixman_image_t *	pDst,
					   INT16	xSrc,
					   INT16	ySrc,
					   INT16	xMask,
					   INT16	yMask,
					   INT16	xDst,
					   INT16	yDst,
					   CARD16	width,
					   CARD16	height);

void fbCompositeSolidMask_nx8x8888vmx (pixman_operator_t      op,
				       pixman_image_t * pSrc,
				       pixman_image_t * pMask,
				       pixman_image_t * pDst,
				       INT16      xSrc,
				       INT16      ySrc,
				       INT16      xMask,
				       INT16      yMask,
				       INT16      xDst,
				       INT16      yDst,
				       CARD16     width,
				       CARD16     height);

void fbCompositeSolidMaskSrc_nx8x8888vmx (pixman_operator_t      op,
					  pixman_image_t * pSrc,
					  pixman_image_t * pMask,
					  pixman_image_t * pDst,
					  INT16      xSrc,
					  INT16      ySrc,
					  INT16      xMask,
					  INT16      yMask,
					  INT16      xDst,
					  INT16      yDst,
					  CARD16     width,
					  CARD16     height);

void fbCompositeSrcAdd_8888x8x8vmx (pixman_operator_t   op,
				    pixman_image_t * pSrc,
				    pixman_image_t * pMask,
				    pixman_image_t * pDst,
				    INT16      xSrc,
				    INT16      ySrc,
				    INT16      xMask,
				    INT16      yMask,
				    INT16      xDst,
				    INT16      yDst,
				    CARD16     width,
				    CARD16     height);

void fbCompositeIn_8x8vmx (pixman_operator_t	op,
			   pixman_image_t * pSrc,
			   pixman_image_t * pMask,
			   pixman_image_t * pDst,
			   INT16      xSrc,
			   INT16      ySrc,
			   INT16      xMask,
			   INT16      yMask,
			   INT16      xDst,
			   INT16      yDst,
			   CARD16     width,
			   CARD16     height);

void fbCompositeSrcAdd_8000x8000vmx (pixman_operator_t	op,
				     pixman_image_t * pSrc,
				     pixman_image_t * pMask,
				     pixman_image_t * pDst,
				     INT16      xSrc,
				     INT16      ySrc,
				     INT16      xMask,
				     INT16      yMask,
				     INT16      xDst,
				     INT16      yDst,
				     CARD16     width,
				     CARD16     height);

void fbCompositeSrc_8888RevNPx8888vmx (pixman_operator_t      op,
				       pixman_image_t * pSrc,
				       pixman_image_t * pMask,
				       pixman_image_t * pDst,
				       INT16      xSrc,
				       INT16      ySrc,
				       INT16      xMask,
				       INT16      yMask,
				       INT16      xDst,
				       INT16      yDst,
				       CARD16     width,
				       CARD16     height);

void fbCompositeSrc_8888x0565vmx (pixman_operator_t      op,
				  pixman_image_t * pSrc,
				  pixman_image_t * pMask,
				  pixman_image_t * pDst,
				  INT16      xSrc,
				  INT16      ySrc,
				  INT16      xMask,
				  INT16      yMask,
				  INT16      xDst,
				  INT16      yDst,
				  CARD16     width,
				  CARD16     height);

void fbCompositeSrc_8888RevNPx0565vmx (pixman_operator_t      op,
				       pixman_image_t * pSrc,
				       pixman_image_t * pMask,
				       pixman_image_t * pDst,
				       INT16      xSrc,
				       INT16      ySrc,
				       INT16      xMask,
				       INT16      yMask,
				       INT16      xDst,
				       INT16      yDst,
				       CARD16     width,
				       CARD16     height);

void fbCompositeSolid_nx8888vmx (pixman_operator_t		op,
				 pixman_image_t *	pSrc,
				 pixman_image_t *	pMask,
				 pixman_image_t *	pDst,
				 INT16		xSrc,
				 INT16		ySrc,
				 INT16		xMask,
				 INT16		yMask,
				 INT16		xDst,
				 INT16		yDst,
				 CARD16		width,
				 CARD16		height);

void fbCompositeSolid_nx0565vmx (pixman_operator_t		op,
				 pixman_image_t *	pSrc,
				 pixman_image_t *	pMask,
				 pixman_image_t *	pDst,
				 INT16		xSrc,
				 INT16		ySrc,
				 INT16		xMask,
				 INT16		yMask,
				 INT16		xDst,
				 INT16		yDst,
				 CARD16		width,
				 CARD16		height);

void fbCompositeSolidMask_nx8x0565vmx (pixman_operator_t      op,
				       pixman_image_t * pSrc,
				       pixman_image_t * pMask,
				       pixman_image_t * pDst,
				       INT16      xSrc,
				       INT16      ySrc,
				       INT16      xMask,
				       INT16      yMask,
				       INT16      xDst,
				       INT16      yDst,
				       CARD16     width,
				       CARD16     height);

void fbCompositeSrc_x888x8x8888vmx (pixman_operator_t	op,
				    pixman_image_t *  pSrc,
				    pixman_image_t *  pMask,
				    pixman_image_t *  pDst,
				    INT16	xSrc,
				    INT16	ySrc,
				    INT16       xMask,
				    INT16       yMask,
				    INT16       xDst,
				    INT16       yDst,
				    CARD16      width,
				    CARD16      height);

void fbCompositeSrc_8888x8x8888vmx (pixman_operator_t	op,
				    pixman_image_t *  pSrc,
				    pixman_image_t *  pMask,
				    pixman_image_t *  pDst,
				    INT16	xSrc,
				    INT16	ySrc,
				    INT16       xMask,
				    INT16       yMask,
				    INT16       xDst,
				    INT16       yDst,
				    CARD16      width,
				    CARD16      height);

void fbCompositeSrc_8888x8888vmx (pixman_operator_t      op,
				  pixman_image_t * pSrc,
				  pixman_image_t * pMask,
				  pixman_image_t * pDst,
				  INT16      xSrc,
				  INT16      ySrc,
				  INT16      xMask,
				  INT16      yMask,
				  INT16      xDst,
				  INT16      yDst,
				  CARD16     width,
				  CARD16     height);

pixman_bool_t fbCopyAreavmx (FbPixels	*pSrc,
		    FbPixels	*pDst,
		    int		src_x,
		    int		src_y,
		    int		dst_x,
		    int		dst_y,
		    int		width,
		    int		height);

void fbCompositeCopyAreavmx (pixman_operator_t	op,
			     pixman_image_t *	pSrc,
			     pixman_image_t *	pMask,
			     pixman_image_t *	pDst,
			     INT16	xSrc,
			     INT16      ySrc,
			     INT16      xMask,
			     INT16      yMask,
			     INT16      xDst,
			     INT16      yDst,
			     CARD16     width,
			     CARD16     height);

pixman_bool_t fbSolidFillvmx (FbPixels	*pDraw,
		     int		x,
		     int		y,
		     int		width,
		     int		height,
		     FbBits		xor);
#endif
#endif 
