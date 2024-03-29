













































#ifndef _DATATYPES_H
#define _DATATYPES_H

#include "integers.h"           
#include "alloc.h"

#include <stdarg.h>

#ifndef SRTP_KERNEL
# include <stdio.h>
# include <string.h>
# include <time.h>
# ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
# elif defined HAVE_WINSOCK2_H
#  include <winsock2.h>
# endif
#endif



#define DATATYPES_USE_MACROS  

typedef union {
  uint8_t  v8[2];
  uint16_t value;
} v16_t;

typedef union {
  uint8_t  v8[4];
  uint16_t v16[2];
  uint32_t value;
} v32_t;

typedef union {
  uint8_t  v8[8];
  uint16_t v16[4];
  uint32_t v32[2];
  uint64_t value;
} v64_t;

typedef union {
  uint8_t  v8[16];
  uint16_t v16[8];
  uint32_t v32[4];
  uint64_t v64[2];
} v128_t;





#define pow_2(X) ( (unsigned int)1 << (X) )   /* 2^X     */

#define pow_minus_one(X) ( (X) ? -1 : 1 )      /* (-1)^X  */







int
octet_get_weight(uint8_t octet);

char *
octet_bit_string(uint8_t x);

#define MAX_PRINT_STRING_LEN 1024

char *
octet_string_hex_string(const void *str, int length);

char *
v128_bit_string(v128_t *x);

char *
v128_hex_string(v128_t *x);

uint8_t
nibble_to_hex_char(uint8_t nibble);

char *
char_to_hex_string(char *x, int num_char);

uint8_t
hex_string_to_octet(char *s);














int
hex_string_to_octet_string(char *raw, char *hex, int len);

v128_t
hex_string_to_v128(char *s);

void
v128_copy_octet_string(v128_t *x, const uint8_t s[16]);

void
v128_left_shift(v128_t *x, int shift_index);

void
v128_right_shift(v128_t *x, int shift_index);










#define _v128_set_to_zero(x)     \
(                               \
  (x)->v32[0] = 0,              \
  (x)->v32[1] = 0,              \
  (x)->v32[2] = 0,              \
  (x)->v32[3] = 0               \
)

#define _v128_copy(x, y)          \
(                                \
  (x)->v32[0] = (y)->v32[0],     \
  (x)->v32[1] = (y)->v32[1],     \
  (x)->v32[2] = (y)->v32[2],     \
  (x)->v32[3] = (y)->v32[3]      \
)

#define _v128_xor(z, x, y)                       \
(                                               \
   (z)->v32[0] = (x)->v32[0] ^ (y)->v32[0],     \
   (z)->v32[1] = (x)->v32[1] ^ (y)->v32[1],     \
   (z)->v32[2] = (x)->v32[2] ^ (y)->v32[2],     \
   (z)->v32[3] = (x)->v32[3] ^ (y)->v32[3]      \
)

#define _v128_and(z, x, y)                       \
(                                               \
   (z)->v32[0] = (x)->v32[0] & (y)->v32[0],     \
   (z)->v32[1] = (x)->v32[1] & (y)->v32[1],     \
   (z)->v32[2] = (x)->v32[2] & (y)->v32[2],     \
   (z)->v32[3] = (x)->v32[3] & (y)->v32[3]      \
)

#define _v128_or(z, x, y)                        \
(                                               \
   (z)->v32[0] = (x)->v32[0] | (y)->v32[0],     \
   (z)->v32[1] = (x)->v32[1] | (y)->v32[1],     \
   (z)->v32[2] = (x)->v32[2] | (y)->v32[2],     \
   (z)->v32[3] = (x)->v32[3] | (y)->v32[3]      \
)

#define _v128_complement(x)        \
(                                  \
   (x)->v32[0] = ~(x)->v32[0],     \
   (x)->v32[1] = ~(x)->v32[1],     \
   (x)->v32[2] = ~(x)->v32[2],     \
   (x)->v32[3] = ~(x)->v32[3]      \
)


#define _v128_is_eq(x, y)                                        \
  (((x)->v64[0] == (y)->v64[0]) && ((x)->v64[1] == (y)->v64[1]))


#ifdef NO_64BIT_MATH
#define _v128_xor_eq(z, x)         \
(                                  \
   (z)->v32[0] ^= (x)->v32[0],     \
   (z)->v32[1] ^= (x)->v32[1],     \
   (z)->v32[2] ^= (x)->v32[2],     \
   (z)->v32[3] ^= (x)->v32[3]      \
)
#else
#define _v128_xor_eq(z, x)         \
(                                  \
   (z)->v64[0] ^= (x)->v64[0],     \
   (z)->v64[1] ^= (x)->v64[1]      \
)
#endif










#define _v128_get_bit(x, bit)                     \
(                                                 \
  ((((x)->v32[(bit) >> 5]) >> ((bit) & 31)) & 1)  \
)

#define _v128_set_bit(x, bit)                                    \
(                                                                \
  (((x)->v32[(bit) >> 5]) |= ((uint32_t)1 << ((bit) & 31))) \
)

#define _v128_clear_bit(x, bit)                                   \
(                                                                 \
  (((x)->v32[(bit) >> 5]) &= ~((uint32_t)1 << ((bit) & 31))) \
)

#define _v128_set_bit_to(x, bit, value)   \
(                                         \
   (value) ? _v128_set_bit(x, bit) :      \
             _v128_clear_bit(x, bit)      \
)


#if 0

#ifdef WORDS_BIGENDIAN

#define _v128_add(z, x, y) {                    \
  uint64_t tmp;					\
    						\
  tmp = x->v32[3] + y->v32[3];                  \
  z->v32[3] = (uint32_t) tmp;			\
  						\
  tmp =  x->v32[2] + y->v32[2] + (tmp >> 32);	\
  z->v32[2] = (uint32_t) tmp;                   \
						\
  tmp =  x->v32[1] + y->v32[1] + (tmp >> 32);	\
  z->v32[1] = (uint32_t) tmp;			\
                                                \
  tmp =  x->v32[0] + y->v32[0] + (tmp >> 32);	\
  z->v32[0] = (uint32_t) tmp;			\
}

#else 

#define _v128_add(z, x, y) {                    \
  uint64_t tmp;					\
						\
  tmp = htonl(x->v32[3]) + htonl(y->v32[3]);	\
  z->v32[3] = ntohl((uint32_t) tmp);		\
  						\
  tmp =  htonl(x->v32[2]) + htonl(y->v32[2])	\
       + htonl(tmp >> 32);			\
  z->v32[2] = ntohl((uint32_t) tmp);		\
                                                \
  tmp =  htonl(x->v32[1]) + htonl(y->v32[1])	\
       + htonl(tmp >> 32);			\
  z->v32[1] = ntohl((uint32_t) tmp);		\
  						\
  tmp =  htonl(x->v32[0]) + htonl(y->v32[0])	\
       + htonl(tmp >> 32);			\
  z->v32[0] = ntohl((uint32_t) tmp);		\
}
#endif                       
#endif 


#ifdef DATATYPES_USE_MACROS  
   
#define v128_set_to_zero(z)       _v128_set_to_zero(z)
#define v128_copy(z, x)           _v128_copy(z, x)
#define v128_xor(z, x, y)         _v128_xor(z, x, y)
#define v128_and(z, x, y)         _v128_and(z, x, y)
#define v128_or(z, x, y)          _v128_or(z, x, y)
#define v128_complement(x)        _v128_complement(x) 
#define v128_is_eq(x, y)          _v128_is_eq(x, y)
#define v128_xor_eq(x, y)         _v128_xor_eq(x, y)
#define v128_get_bit(x, i)        _v128_get_bit(x, i)
#define v128_set_bit(x, i)        _v128_set_bit(x, i)
#define v128_clear_bit(x, i)      _v128_clear_bit(x, i)
#define v128_set_bit_to(x, i, y)  _v128_set_bit_to(x, i, y)

#else

void
v128_set_to_zero(v128_t *x);

int
v128_is_eq(const v128_t *x, const v128_t *y);

void
v128_copy(v128_t *x, const v128_t *y);

void
v128_xor(v128_t *z, v128_t *x, v128_t *y);

void
v128_and(v128_t *z, v128_t *x, v128_t *y);

void
v128_or(v128_t *z, v128_t *x, v128_t *y); 

void
v128_complement(v128_t *x);

int
v128_get_bit(const v128_t *x, int i);

void
v128_set_bit(v128_t *x, int i) ;     

void
v128_clear_bit(v128_t *x, int i);    

void
v128_set_bit_to(v128_t *x, int i, int y);

#endif 






int
octet_string_is_eq(uint8_t *a, uint8_t *b, int len);

void
octet_string_set_to_zero(uint8_t *s, int len);


#ifndef SRTP_KERNEL_LINUX




#ifdef WORDS_BIGENDIAN

# define be32_to_cpu(x)	(x)
# define be64_to_cpu(x)	(x)
#elif defined(HAVE_BYTESWAP_H)

# include <byteswap.h>
# define be32_to_cpu(x)	bswap_32((x))
# define be64_to_cpu(x)	bswap_64((x))
#else

#if defined(__GNUC__) && defined(HAVE_X86)

static inline uint32_t be32_to_cpu(uint32_t v) {
   
   asm("bswap %0" : "=r" (v) : "0" (v));
   return v;
}
# else 
#  ifdef HAVE_NETINET_IN_H
#   include <netinet/in.h>
#  elif defined HAVE_WINSOCK2_H
#   include <winsock2.h>
#  else
#    error "Platform not recognized"
#  endif
#  define be32_to_cpu(x)	ntohl((x))
# endif 

static inline uint64_t be64_to_cpu(uint64_t v) {
# ifdef NO_64BIT_MATH
   
   v = make64(htonl(low32(v)),htonl(high32(v)));
# else
   
   v= (uint64_t)((be32_to_cpu((uint32_t)(v >> 32))) | (((uint64_t)be32_to_cpu((uint32_t)v)) << 32));
# endif
   return v;
}

#endif 

#endif 












#define bits_per_word  32
#define bytes_per_word 4

typedef struct {
  uint32_t length;   
  uint32_t *word;
} bitvector_t;


#define _bitvector_get_bit(v, bit_index)				\
(									\
 ((((v)->word[((bit_index) >> 5)]) >> ((bit_index) & 31)) & 1)		\
)


#define _bitvector_set_bit(v, bit_index)				\
(									\
 (((v)->word[((bit_index) >> 5)] |= ((uint32_t)1 << ((bit_index) & 31)))) \
)

#define _bitvector_clear_bit(v, bit_index)				\
(									\
 (((v)->word[((bit_index) >> 5)] &= ~((uint32_t)1 << ((bit_index) & 31)))) \
)

#define _bitvector_get_length(v)					\
(									\
 ((v)->length)								\
)

#ifdef DATATYPES_USE_MACROS  

#define bitvector_get_bit(v, bit_index) _bitvector_get_bit(v, bit_index)
#define bitvector_set_bit(v, bit_index) _bitvector_set_bit(v, bit_index)
#define bitvector_clear_bit(v, bit_index) _bitvector_clear_bit(v, bit_index)
#define bitvector_get_length(v) _bitvector_get_length(v)

#else

int
bitvector_get_bit(const bitvector_t *v, int bit_index);

void
bitvector_set_bit(bitvector_t *v, int bit_index);

void
bitvector_clear_bit(bitvector_t *v, int bit_index);

unsigned long
bitvector_get_length(const bitvector_t *v);

#endif

int
bitvector_alloc(bitvector_t *v, unsigned long length);

void
bitvector_dealloc(bitvector_t *v);

void
bitvector_set_to_zero(bitvector_t *x);

void
bitvector_left_shift(bitvector_t *x, int index);

char *
bitvector_bit_string(bitvector_t *x, char* buf, int len);

#endif 
