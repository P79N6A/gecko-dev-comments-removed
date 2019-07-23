
























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-private.h"

void
arm_composite_add_8000_8000 (pixman_implementation_t * impl,
			     pixman_op_t               op,
			     pixman_image_t *          src_image,
			     pixman_image_t *          mask_image,
			     pixman_image_t *          dst_image,
			     int32_t                   src_x,
			     int32_t                   src_y,
			     int32_t                   mask_x,
			     int32_t                   mask_y,
			     int32_t                   dest_x,
			     int32_t                   dest_y,
			     int32_t                   width,
			     int32_t                   height);

void
arm_composite_over_8888_8888 (pixman_implementation_t * impl,
			      pixman_op_t               op,
			      pixman_image_t *          src_image,
			      pixman_image_t *          mask_image,
			      pixman_image_t *          dst_image,
			      int32_t                   src_x,
			      int32_t                   src_y,
			      int32_t                   mask_x,
			      int32_t                   mask_y,
			      int32_t                   dest_x,
			      int32_t                   dest_y,
			      int32_t                   width,
			      int32_t                   height);

void
arm_composite_over_8888_n_8888 (pixman_implementation_t * impl,
				pixman_op_t               op,
				pixman_image_t *          src_image,
				pixman_image_t *          mask_image,
				pixman_image_t *          dst_image,
				int32_t                   src_x,
				int32_t                   src_y,
				int32_t                   mask_x,
				int32_t                   mask_y,
				int32_t                   dest_x,
				int32_t                   dest_y,
				int32_t                   width,
				int32_t                   height);

void
arm_composite_over_n_8_8888 (pixman_implementation_t * impl,
			     pixman_op_t               op,
			     pixman_image_t *          src_image,
			     pixman_image_t *          mask_image,
			     pixman_image_t *          dst_image,
			     int32_t                   src_x,
			     int32_t                   src_y,
			     int32_t                   mask_x,
			     int32_t                   mask_y,
			     int32_t                   dest_x,
			     int32_t                   dest_y,
			     int32_t                   width,
			     int32_t                   height);
void
arm_composite_src_8888_0565 (pixman_implementation_t * impl,
			     pixman_op_t               op,
			     pixman_image_t *          src_image,
			     pixman_image_t *          mask_image,
			     pixman_image_t *          dst_image,
			     int32_t                   src_x,
			     int32_t                   src_y,
			     int32_t                   mask_x,
			     int32_t                   mask_y,
			     int32_t                   dest_x,
			     int32_t                   dest_y,
			     int32_t                   width,
			     int32_t                   height);
