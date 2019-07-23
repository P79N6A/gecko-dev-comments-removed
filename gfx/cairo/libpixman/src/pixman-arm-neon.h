

























#include "pixman-private.h"

#ifdef USE_ARM_NEON

pixman_bool_t pixman_have_arm_neon(void);

#else
#define pixman_have_arm_neon() FALSE
#endif

#ifdef USE_ARM_NEON

void
fbCompositeSrcAdd_8000x8000neon (pixman_op_t op,
                        pixman_image_t * pSrc,
                        pixman_image_t * pMask,
                        pixman_image_t * pDst,
                        int16_t      xSrc,
                        int16_t      ySrc,
                        int16_t      xMask,
                        int16_t      yMask,
                        int16_t      xDst,
                        int16_t      yDst,
                        uint16_t     width,
                        uint16_t     height);

void
fbCompositeSrc_8888x8888neon (pixman_op_t op,
			pixman_image_t * pSrc,
			pixman_image_t * pMask,
			pixman_image_t * pDst,
			int16_t      xSrc,
			int16_t      ySrc,
			int16_t      xMask,
			int16_t      yMask,
			int16_t      xDst,
			int16_t      yDst,
			uint16_t     width,
			uint16_t     height);

void
fbCompositeSrc_8888x8x8888neon (pixman_op_t op,
			pixman_image_t * pSrc,
			pixman_image_t * pMask,
			pixman_image_t * pDst,
			int16_t      xSrc,
			int16_t      ySrc,
			int16_t      xMask,
			int16_t      yMask,
			int16_t      xDst,
			int16_t      yDst,
			uint16_t     width,
			uint16_t     height);

void
fbCompositeSolidMask_nx8x0565neon (pixman_op_t op,
                        pixman_image_t * pSrc,
                        pixman_image_t * pMask,
                        pixman_image_t * pDst,
                        int16_t      xSrc,
                        int16_t      ySrc,
                        int16_t      xMask,
                        int16_t      yMask,
                        int16_t      xDst,
                        int16_t      yDst,
                        uint16_t     width,
                        uint16_t     height);

void
fbCompositeSolidMask_nx8x8888neon (pixman_op_t op,
			pixman_image_t * pSrc,
			pixman_image_t * pMask,
			pixman_image_t * pDst,
			int16_t      xSrc,
			int16_t      ySrc,
			int16_t      xMask,
			int16_t      yMask,
			int16_t      xDst,
			int16_t      yDst,
		 	uint16_t     width,
			uint16_t     height);

void
fbCompositeSrc_x888x0565neon (pixman_op_t op,
                        pixman_image_t * pSrc,
                        pixman_image_t * pMask,
                        pixman_image_t * pDst,
                        int16_t      xSrc,
                        int16_t      ySrc,
                        int16_t      xMask,
                        int16_t      yMask,
                        int16_t      xDst,
                        int16_t      yDst,
                        uint16_t     width,
                        uint16_t     height);

void
fbCompositeSrcAdd_8888x8x8neon (pixman_op_t op,
                        pixman_image_t * pSrc,
                        pixman_image_t * pMask,
                        pixman_image_t * pDst,
                        int16_t      xSrc,
                        int16_t      ySrc,
                        int16_t      xMask,
                        int16_t      yMask,
                        int16_t      xDst,
                        int16_t      yDst,
                        uint16_t     width,
                        uint16_t     height);

#endif 
