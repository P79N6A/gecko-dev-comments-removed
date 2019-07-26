













































#ifndef INTEGERS_H
#define INTEGERS_H

#include "config.h"	

#ifdef SRTP_KERNEL

#include "kernel_compat.h"

#else 


#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef INTEGER_TYPES_H

#include INTEGER_TYPES_H

#if !defined(HAVE_UINT64_T)
#define NO_64BIT_MATH 1
#endif
#else
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#endif 
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_INT_TYPES_H
# include <sys/int_types.h>    
#endif
#ifdef HAVE_MACHINE_TYPES_H
# include <machine/types.h>
#endif


#if !defined(HAVE_UINT64_T)
# if SIZEOF_UNSIGNED_LONG == 8
typedef unsigned long		uint64_t;
# elif SIZEOF_UNSIGNED_LONG_LONG == 8
typedef unsigned long long	uint64_t;
# else
#  define NO_64BIT_MATH 1
# endif
#endif



#ifndef HAVE_UINT8_T
typedef unsigned char		uint8_t;
#endif
#ifndef HAVE_UINT16_T
typedef unsigned short int	uint16_t;
#endif
#ifndef HAVE_UINT32_T
typedef unsigned int		uint32_t;
#endif

#ifdef NO_64BIT_MATH
typedef double uint64_t;

extern uint64_t make64(uint32_t high, uint32_t low);
extern uint32_t high32(uint64_t value);
extern uint32_t low32(uint64_t value);
#endif

#endif 




#ifdef ALIGNMENT_32BIT_REQUIRED

#ifdef WORDS_BIGENDIAN
#define PUT_32(addr,value) \
    { \
        ((unsigned char *) (addr))[0] = (value >> 24); \
        ((unsigned char *) (addr))[1] = (value >> 16) & 0xff; \
        ((unsigned char *) (addr))[2] = (value >> 8) & 0xff; \
        ((unsigned char *) (addr))[3] = (value)      & 0xff; \
    }
#define GET_32(addr) ((((unsigned char *) (addr))[0] << 24) |  \
                      (((unsigned char *) (addr))[1] << 16) |  \
                      (((unsigned char *) (addr))[2] << 8)  |  \
                      (((unsigned char *) (addr))[3])) 
#else
#define PUT_32(addr,value) \
    { \
        ((unsigned char *) (addr))[3] = (value >> 24); \
        ((unsigned char *) (addr))[2] = (value >> 16) & 0xff; \
        ((unsigned char *) (addr))[1] = (value >> 8) & 0xff; \
        ((unsigned char *) (addr))[0] = (value)      & 0xff; \
    }
#define GET_32(addr) ((((unsigned char *) (addr))[3] << 24) |  \
                      (((unsigned char *) (addr))[2] << 16) |  \
                      (((unsigned char *) (addr))[1] << 8)  |  \
                      (((unsigned char *) (addr))[0])) 
#endif 
#else
#define PUT_32(addr,value) *(((uint32_t *) (addr)) = (value)
#define GET_32(addr) (*(((uint32_t *) (addr)))
#endif

#endif 
