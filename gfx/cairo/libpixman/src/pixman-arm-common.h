
























#ifndef PIXMAN_ARM_COMMON_H
#define PIXMAN_ARM_COMMON_H




















#define PIXMAN_ARM_BIND_FAST_PATH_SRC_DST(cputype, name,                \
                                          src_type, src_cnt,            \
                                          dst_type, dst_cnt)            \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t   w,                   \
                                         int32_t   h,                   \
                                         dst_type *dst,                 \
                                         int32_t   dst_stride,          \
                                         src_type *src,                 \
                                         int32_t   src_stride);         \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_op_t              op,                \
                            pixman_image_t *         src_image,         \
                            pixman_image_t *         mask_image,        \
                            pixman_image_t *         dst_image,         \
                            int32_t                  src_x,             \
                            int32_t                  src_y,             \
                            int32_t                  mask_x,            \
                            int32_t                  mask_y,            \
                            int32_t                  dest_x,            \
                            int32_t                  dest_y,            \
                            int32_t                  width,             \
                            int32_t                  height)            \
{                                                                       \
    dst_type *dst_line;                                                 \
    src_type *src_line;                                                 \
    int32_t dst_stride, src_stride;                                     \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,           \
                           src_stride, src_line, src_cnt);              \
    PIXMAN_IMAGE_GET_LINE (dst_image, dest_x, dest_y, dst_type,         \
                           dst_stride, dst_line, dst_cnt);              \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src_line, src_stride);     \
}

#define PIXMAN_ARM_BIND_FAST_PATH_N_DST(cputype, name,                  \
                                        dst_type, dst_cnt)              \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         uint32_t   src);               \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_op_t              op,                \
                            pixman_image_t *         src_image,         \
                            pixman_image_t *         mask_image,        \
                            pixman_image_t *         dst_image,         \
                            int32_t                  src_x,             \
                            int32_t                  src_y,             \
                            int32_t                  mask_x,            \
                            int32_t                  mask_y,            \
                            int32_t                  dest_x,            \
                            int32_t                  dest_y,            \
                            int32_t                  width,             \
                            int32_t                  height)            \
{                                                                       \
    dst_type  *dst_line;                                                \
    int32_t    dst_stride;                                              \
    uint32_t   src;                                                     \
                                                                        \
    src = _pixman_image_get_solid (src_image, dst_image->bits.format);  \
                                                                        \
    if (src == 0)                                                       \
	return;                                                         \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dst_image, dest_x, dest_y, dst_type,         \
                           dst_stride, dst_line, dst_cnt);              \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src);                      \
}

#define PIXMAN_ARM_BIND_FAST_PATH_N_MASK_DST(cputype, name,             \
                                             mask_type, mask_cnt,       \
                                             dst_type, dst_cnt)         \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         uint32_t   src,                \
                                         int32_t    unused,             \
                                         mask_type *mask,               \
                                         int32_t    mask_stride);       \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_op_t              op,                \
                            pixman_image_t *         src_image,         \
                            pixman_image_t *         mask_image,        \
                            pixman_image_t *         dst_image,         \
                            int32_t                  src_x,             \
                            int32_t                  src_y,             \
                            int32_t                  mask_x,            \
                            int32_t                  mask_y,            \
                            int32_t                  dest_x,            \
                            int32_t                  dest_y,            \
                            int32_t                  width,             \
                            int32_t                  height)            \
{                                                                       \
    dst_type  *dst_line;                                                \
    mask_type *mask_line;                                               \
    int32_t    dst_stride, mask_stride;                                 \
    uint32_t   src;                                                     \
                                                                        \
    src = _pixman_image_get_solid (src_image, dst_image->bits.format);  \
                                                                        \
    if (src == 0)                                                       \
	return;                                                         \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dst_image, dest_x, dest_y, dst_type,         \
                           dst_stride, dst_line, dst_cnt);              \
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type,       \
                           mask_stride, mask_line, mask_cnt);           \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src, 0,                    \
                                             mask_line, mask_stride);   \
}

#define PIXMAN_ARM_BIND_FAST_PATH_SRC_N_DST(cputype, name,              \
                                            src_type, src_cnt,          \
                                            dst_type, dst_cnt)          \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         src_type  *src,                \
                                         int32_t    src_stride,         \
                                         uint32_t   mask);              \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_op_t              op,                \
                            pixman_image_t *         src_image,         \
                            pixman_image_t *         mask_image,        \
                            pixman_image_t *         dst_image,         \
                            int32_t                  src_x,             \
                            int32_t                  src_y,             \
                            int32_t                  mask_x,            \
                            int32_t                  mask_y,            \
                            int32_t                  dest_x,            \
                            int32_t                  dest_y,            \
                            int32_t                  width,             \
                            int32_t                  height)            \
{                                                                       \
    dst_type  *dst_line;                                                \
    src_type  *src_line;                                                \
    int32_t    dst_stride, src_stride;                                  \
    uint32_t   mask;                                                    \
                                                                        \
    mask = _pixman_image_get_solid (mask_image, dst_image->bits.format);\
                                                                        \
    if (mask == 0)                                                      \
	return;                                                         \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dst_image, dest_x, dest_y, dst_type,         \
                           dst_stride, dst_line, dst_cnt);              \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,           \
                           src_stride, src_line, src_cnt);              \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src_line, src_stride,      \
                                             mask);                     \
}

#define PIXMAN_ARM_BIND_FAST_PATH_SRC_MASK_DST(cputype, name,           \
                                               src_type, src_cnt,       \
                                               mask_type, mask_cnt,     \
                                               dst_type, dst_cnt)       \
void                                                                    \
pixman_composite_##name##_asm_##cputype (int32_t    w,                  \
                                         int32_t    h,                  \
                                         dst_type  *dst,                \
                                         int32_t    dst_stride,         \
                                         src_type  *src,                \
                                         int32_t    src_stride,         \
                                         mask_type *mask,               \
                                         int32_t    mask_stride);       \
                                                                        \
static void                                                             \
cputype##_composite_##name (pixman_implementation_t *imp,               \
                            pixman_op_t              op,                \
                            pixman_image_t *         src_image,         \
                            pixman_image_t *         mask_image,        \
                            pixman_image_t *         dst_image,         \
                            int32_t                  src_x,             \
                            int32_t                  src_y,             \
                            int32_t                  mask_x,            \
                            int32_t                  mask_y,            \
                            int32_t                  dest_x,            \
                            int32_t                  dest_y,            \
                            int32_t                  width,             \
                            int32_t                  height)            \
{                                                                       \
    dst_type  *dst_line;                                                \
    src_type  *src_line;                                                \
    mask_type *mask_line;                                               \
    int32_t    dst_stride, src_stride, mask_stride;                     \
                                                                        \
    PIXMAN_IMAGE_GET_LINE (dst_image, dest_x, dest_y, dst_type,         \
                           dst_stride, dst_line, dst_cnt);              \
    PIXMAN_IMAGE_GET_LINE (src_image, src_x, src_y, src_type,           \
                           src_stride, src_line, src_cnt);              \
    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type,       \
                           mask_stride, mask_line, mask_cnt);           \
                                                                        \
    pixman_composite_##name##_asm_##cputype (width, height,             \
                                             dst_line, dst_stride,      \
                                             src_line, src_stride,      \
                                             mask_line, mask_stride);   \
}

#endif
