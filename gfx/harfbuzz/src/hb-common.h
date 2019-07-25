

























#ifndef HB_COMMON_H
#define HB_COMMON_H

# ifdef __cplusplus
#  define HB_BEGIN_DECLS	extern "C" {
#  define HB_END_DECLS		}
# else 
#  define HB_BEGIN_DECLS
#  define HB_END_DECLS
# endif 

HB_BEGIN_DECLS


#ifdef _MSC_VER
#define _HB__STR2__(x) #x
#define _HB__STR1__(x) _HB__STR2__(x)
#define _HB__LOC__ __FILE__ "("_HB__STR1__(__LINE__)") : Warning Msg: "
#pragma message(_HB__LOC__"Not using stdint.h; integer types may have wrong size")
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#ifndef __cplusplus
#define inline __inline
#endif
#else
#include <stdint.h>
#endif

typedef int hb_bool_t;

typedef uint32_t hb_tag_t;
#define HB_TAG(a,b,c,d) ((hb_tag_t)(((uint8_t)a<<24)|((uint8_t)b<<16)|((uint8_t)c<<8)|(uint8_t)d))
#define HB_TAG_STR(s)   (HB_TAG(((const char *) s)[0], \
				((const char *) s)[1], \
				((const char *) s)[2], \
				((const char *) s)[3]))
#define HB_TAG_NONE HB_TAG(0,0,0,0)

hb_tag_t hb_tag_from_string (const char *s);


typedef uint32_t hb_codepoint_t;
typedef int32_t hb_position_t;
typedef uint32_t hb_mask_t;

typedef void (*hb_destroy_func_t) (void *user_data);

typedef enum _hb_direction_t {
  HB_DIRECTION_LTR,
  HB_DIRECTION_RTL,
  HB_DIRECTION_TTB,
  HB_DIRECTION_BTT
} hb_direction_t;

#define HB_DIRECTION_IS_HORIZONTAL(dir)	((((unsigned int) (dir)) & ~1U) == 0)
#define HB_DIRECTION_IS_VERTICAL(dir)	((((unsigned int) (dir)) & ~1U) == 2)
#define HB_DIRECTION_IS_FORWARD(dir)	((((unsigned int) (dir)) & ~2U) == 0)
#define HB_DIRECTION_IS_BACKWARD(dir)	((((unsigned int) (dir)) & ~2U) == 1)
#define HB_DIRECTION_REVERSE(dir)	((hb_direction_t) (((unsigned int) (dir)) ^ 1))


typedef union _hb_var_int_t {
  uint32_t u32;
  int32_t i32;
  uint16_t u16[2];
  int16_t i16[2];
  uint8_t u8[4];
  int8_t i8[4];
} hb_var_int_t;


HB_END_DECLS

#endif 
