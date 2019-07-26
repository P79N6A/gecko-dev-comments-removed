


























#if !defined(ARMCPU_H)
# define ARMCPU_H

# if defined(OPUS_ARM_MAY_HAVE_EDSP)
#  define MAY_HAVE_EDSP(name) name ## _edsp
# else
#  define MAY_HAVE_EDSP(name) name ## _c
# endif

# if defined(OPUS_ARM_MAY_HAVE_MEDIA)
#  define MAY_HAVE_MEDIA(name) name ## _media
# else
#  define MAY_HAVE_MEDIA(name) MAY_HAVE_EDSP(name)
# endif

# if defined(OPUS_ARM_MAY_HAVE_NEON)
#  define MAY_HAVE_NEON(name) name ## _neon
# else
#  define MAY_HAVE_NEON(name) MAY_HAVE_MEDIA(name)
# endif

# if defined(OPUS_ARM_PRESUME_EDSP)
#  define PRESUME_EDSP(name) name ## _edsp
# else
#  define PRESUME_EDSP(name) name ## _c
# endif

# if defined(OPUS_ARM_PRESUME_MEDIA)
#  define PRESUME_MEDIA(name) name ## _media
# else
#  define PRESUME_MEDIA(name) PRESUME_EDSP(name)
# endif

# if defined(OPUS_ARM_PRESUME_NEON)
#  define PRESUME_NEON(name) name ## _neon
# else
#  define PRESUME_NEON(name) PRESUME_MEDIA(name)
# endif

# if defined(OPUS_HAVE_RTCD)
int opus_select_arch(void);
# endif

#endif
