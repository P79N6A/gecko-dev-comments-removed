















#if !defined(_internal_H)
# define _internal_H (1)
# include <stdlib.h>
# include <limits.h>
# if defined(HAVE_CONFIG_H)
#  include "config.h"
# endif
# include "theora/codec.h"
# include "theora/theora.h"
# include "ocintrin.h"

# if !defined(__GNUC_PREREQ)
#  if defined(__GNUC__)&&defined(__GNUC_MINOR__)
#   define __GNUC_PREREQ(_maj,_min) \
 ((__GNUC__<<16)+__GNUC_MINOR__>=((_maj)<<16)+(_min))
#  else
#   define __GNUC_PREREQ(_maj,_min) 0
#  endif
# endif

# if defined(_MSC_VER)

#  pragma warning(disable:4799)

#  pragma warning(disable:4554)
# endif

# if __GNUC_PREREQ(4,2)
#  pragma GCC diagnostic ignored "-Wparentheses"
# endif







# if defined(OC_X86_ASM)||defined(OC_ARM_ASM)
#  if defined(__GNUC__)
#   define OC_ALIGN8(expr) expr __attribute__((aligned(8)))
#   define OC_ALIGN16(expr) expr __attribute__((aligned(16)))
#  elif defined(_MSC_VER)
#   define OC_ALIGN8(expr) __declspec (align(8)) expr
#   define OC_ALIGN16(expr) __declspec (align(16)) expr
#  else
#   error "Alignment macros required for this platform."
#  endif
# endif
# if !defined(OC_ALIGN8)
#  define OC_ALIGN8(expr) expr
# endif
# if !defined(OC_ALIGN16)
#  define OC_ALIGN16(expr) expr
# endif




# define OC_VENDOR_STRING "Xiph.Org libtheora 1.2.0alpha 20100924 (Ptalarbvorm)"


# define TH_VERSION_MAJOR (3)
# define TH_VERSION_MINOR (2)
# define TH_VERSION_SUB   (1)
# define TH_VERSION_CHECK(_info,_maj,_min,_sub) \
 ((_info)->version_major>(_maj)||(_info)->version_major==(_maj)&& \
 ((_info)->version_minor>(_min)||(_info)->version_minor==(_min)&& \
 (_info)->version_subminor>=(_sub)))





extern const unsigned char OC_FZIG_ZAG[128];


extern const unsigned char OC_IZIG_ZAG[64];


extern const unsigned char OC_MB_MAP[2][2];


extern const unsigned char OC_MB_MAP_IDXS[TH_PF_NFORMATS][12];


extern const unsigned char OC_MB_MAP_NIDXS[TH_PF_NFORMATS];



int oc_ilog(unsigned _v);
void *oc_aligned_malloc(size_t _sz,size_t _align);
void oc_aligned_free(void *_ptr);
void **oc_malloc_2d(size_t _height,size_t _width,size_t _sz);
void **oc_calloc_2d(size_t _height,size_t _width,size_t _sz);
void oc_free_2d(void *_ptr);

void oc_ycbcr_buffer_flip(th_ycbcr_buffer _dst,
 const th_ycbcr_buffer _src);

#endif
