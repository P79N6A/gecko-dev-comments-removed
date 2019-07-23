



























#ifdef USE_MMX

#if !defined(__amd64__) && !defined(__x86_64__)
pixman_private
Bool fbHaveMMX(void);
#else
#define fbHaveMMX() TRUE
#endif

#else
#define fbHaveMMX() FALSE
#endif

#ifdef USE_MMX

pixman_private
void fbComposeSetupMMX(void);

pixman_private
void fbCompositeIn_nx8x8mmx (pixman_operator_t	op,
			     PicturePtr pSrc,
			     PicturePtr pMask,
			     PicturePtr pDst,
			     INT16      xSrc,
			     INT16      ySrc,
			     INT16      xMask,
			     INT16      yMask,
			     INT16      xDst,
			     INT16      yDst,
			     CARD16     width,
			     CARD16     height);

pixman_private
void fbCompositeSolidMask_nx8888x0565Cmmx (pixman_operator_t      op,
					   PicturePtr pSrc,
					   PicturePtr pMask,
					   PicturePtr pDst,
					   INT16      xSrc,
					   INT16      ySrc,
					   INT16      xMask,
					   INT16      yMask,
					   INT16      xDst,
					   INT16      yDst,
					   CARD16     width,
					   CARD16     height);
pixman_private
void fbCompositeSrcAdd_8888x8888mmx (pixman_operator_t	op,
				     PicturePtr	pSrc,
				     PicturePtr	pMask,
				     PicturePtr	pDst,
				     INT16	xSrc,
				     INT16      ySrc,
				     INT16      xMask,
				     INT16      yMask,
				     INT16      xDst,
				     INT16      yDst,
				     CARD16     width,
				     CARD16     height);
pixman_private
void fbCompositeSolidMask_nx8888x8888Cmmx (pixman_operator_t	op,
					   PicturePtr	pSrc,
					   PicturePtr	pMask,
					   PicturePtr	pDst,
					   INT16	xSrc,
					   INT16	ySrc,
					   INT16	xMask,
					   INT16	yMask,
					   INT16	xDst,
					   INT16	yDst,
					   CARD16	width,
					   CARD16	height);
pixman_private
void fbCompositeSolidMask_nx8x8888mmx (pixman_operator_t      op,
				       PicturePtr pSrc,
				       PicturePtr pMask,
				       PicturePtr pDst,
				       INT16      xSrc,
				       INT16      ySrc,
				       INT16      xMask,
				       INT16      yMask,
				       INT16      xDst,
				       INT16      yDst,
				       CARD16     width,
				       CARD16     height);
pixman_private
void fbCompositeSolidMaskSrc_nx8x8888mmx (pixman_operator_t      op,
					  PicturePtr pSrc,
					  PicturePtr pMask,
					  PicturePtr pDst,
					  INT16      xSrc,
					  INT16      ySrc,
					  INT16      xMask,
					  INT16      yMask,
					  INT16      xDst,
					  INT16      yDst,
					  CARD16     width,
					  CARD16     height);

pixman_private
void fbCompositeSrcAdd_8888x8x8mmx (pixman_operator_t   op,
				    PicturePtr pSrc,
				    PicturePtr pMask,
				    PicturePtr pDst,
				    INT16      xSrc,
				    INT16      ySrc,
				    INT16      xMask,
				    INT16      yMask,
				    INT16      xDst,
				    INT16      yDst,
				    CARD16     width,
				    CARD16     height);

pixman_private
void fbCompositeIn_8x8mmx (pixman_operator_t	op,
			   PicturePtr pSrc,
			   PicturePtr pMask,
			   PicturePtr pDst,
			   INT16      xSrc,
			   INT16      ySrc,
			   INT16      xMask,
			   INT16      yMask,
			   INT16      xDst,
			   INT16      yDst,
			   CARD16     width,
			   CARD16     height);

pixman_private
void fbCompositeSrcAdd_8000x8000mmx (pixman_operator_t	op,
				     PicturePtr pSrc,
				     PicturePtr pMask,
				     PicturePtr pDst,
				     INT16      xSrc,
				     INT16      ySrc,
				     INT16      xMask,
				     INT16      yMask,
				     INT16      xDst,
				     INT16      yDst,
				     CARD16     width,
				     CARD16     height);
pixman_private
void fbCompositeSrc_8888RevNPx8888mmx (pixman_operator_t      op,
				       PicturePtr pSrc,
				       PicturePtr pMask,
				       PicturePtr pDst,
				       INT16      xSrc,
				       INT16      ySrc,
				       INT16      xMask,
				       INT16      yMask,
				       INT16      xDst,
				       INT16      yDst,
				       CARD16     width,
				       CARD16     height);
pixman_private
void fbCompositeSrc_8888x0565mmx (pixman_operator_t      op,
				  PicturePtr pSrc,
				  PicturePtr pMask,
				  PicturePtr pDst,
				  INT16      xSrc,
				  INT16      ySrc,
				  INT16      xMask,
				  INT16      yMask,
				  INT16      xDst,
				  INT16      yDst,
				  CARD16     width,
				  CARD16     height);
pixman_private
void fbCompositeSrc_8888RevNPx0565mmx (pixman_operator_t      op,
				       PicturePtr pSrc,
				       PicturePtr pMask,
				       PicturePtr pDst,
				       INT16      xSrc,
				       INT16      ySrc,
				       INT16      xMask,
				       INT16      yMask,
				       INT16      xDst,
				       INT16      yDst,
				       CARD16     width,
				       CARD16     height);
pixman_private
void fbCompositeSolid_nx8888mmx (pixman_operator_t		op,
				 PicturePtr	pSrc,
				 PicturePtr	pMask,
				 PicturePtr	pDst,
				 INT16		xSrc,
				 INT16		ySrc,
				 INT16		xMask,
				 INT16		yMask,
				 INT16		xDst,
				 INT16		yDst,
				 CARD16		width,
				 CARD16		height);
pixman_private
void fbCompositeSolid_nx0565mmx (pixman_operator_t		op,
				 PicturePtr	pSrc,
				 PicturePtr	pMask,
				 PicturePtr	pDst,
				 INT16		xSrc,
				 INT16		ySrc,
				 INT16		xMask,
				 INT16		yMask,
				 INT16		xDst,
				 INT16		yDst,
				 CARD16		width,
				 CARD16		height);
pixman_private
void fbCompositeSolidMask_nx8x0565mmx (pixman_operator_t      op,
				       PicturePtr pSrc,
				       PicturePtr pMask,
				       PicturePtr pDst,
				       INT16      xSrc,
				       INT16      ySrc,
				       INT16      xMask,
				       INT16      yMask,
				       INT16      xDst,
				       INT16      yDst,
				       CARD16     width,
				       CARD16     height);
pixman_private
void fbCompositeSrc_x888x8x8888mmx (pixman_operator_t	op,
				    PicturePtr  pSrc,
				    PicturePtr  pMask,
				    PicturePtr  pDst,
				    INT16	xSrc,
				    INT16	ySrc,
				    INT16       xMask,
				    INT16       yMask,
				    INT16       xDst,
				    INT16       yDst,
				    CARD16      width,
				    CARD16      height);
pixman_private
void fbCompositeSrc_8888x8x8888mmx (pixman_operator_t	op,
				    PicturePtr  pSrc,
				    PicturePtr  pMask,
				    PicturePtr  pDst,
				    INT16	xSrc,
				    INT16	ySrc,
				    INT16       xMask,
				    INT16       yMask,
				    INT16       xDst,
				    INT16       yDst,
				    CARD16      width,
				    CARD16      height);
pixman_private
void fbCompositeSrc_8888x8888mmx (pixman_operator_t      op,
				  PicturePtr pSrc,
				  PicturePtr pMask,
				  PicturePtr pDst,
				  INT16      xSrc,
				  INT16      ySrc,
				  INT16      xMask,
				  INT16      yMask,
				  INT16      xDst,
				  INT16      yDst,
				  CARD16     width,
				  CARD16     height);
pixman_private
void fbCompositeCopyAreammx (pixman_operator_t	op,
			     PicturePtr	pSrc,
			     PicturePtr	pMask,
			     PicturePtr	pDst,
			     INT16	xSrc,
			     INT16      ySrc,
			     INT16      xMask,
			     INT16      yMask,
			     INT16      xDst,
			     INT16      yDst,
			     CARD16     width,
			     CARD16     height);
#endif 
