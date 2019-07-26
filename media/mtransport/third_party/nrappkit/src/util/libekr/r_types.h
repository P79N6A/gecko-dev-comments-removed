


















































































#ifndef _r_types_h
#define _r_types_h



#ifdef R_PLATFORM_INT_TYPES
#include R_PLATFORM_INT_TYPES
#else
#include <stdint.h>
#endif

#ifndef R_DEFINED_INT2
#ifndef SIZEOF_INT
typedef short INT2;
#else
# if (SIZEOF_INT==2)
typedef int INT2;
# elif (SIZEOF_SHORT==2)
typedef short INT2;
# elif (SIZEOF_LONG==2)
typedef long INT2;
# else
# error no type for INT2
# endif
#endif
#else
typedef R_DEFINED_INT2 INT2;
#endif

#ifndef R_DEFINED_UINT2
#ifndef SIZEOF_UNSIGNED_INT
typedef unsigned short UINT2;
#else
# if (SIZEOF_UNSIGNED_INT==2)
typedef unsigned int UINT2;
# elif (SIZEOF_UNSIGNED_SHORT==2)
typedef unsigned short UINT2;
# elif (SIZEOF_UNSIGNED_LONG==2)
typedef unsigned long UINT2;
# else
# error no type for UINT2
# endif
#endif
#else
typedef R_DEFINED_UINT2 UINT2;
#endif

#ifndef R_DEFINED_INT4
#ifndef SIZEOF_INT
typedef int INT4;
#else
# if (SIZEOF_INT==4)
typedef int INT4;
# elif (SIZEOF_SHORT==4)
typedef short INT4;
# elif (SIZEOF_LONG==4)
typedef long INT4;
# else
# error no type for INT4
# endif
#endif
#else
typedef R_DEFINED_INT4 INT4;
#endif

#ifndef R_DEFINED_UINT4
#ifndef SIZEOF_UNSIGNED_INT
typedef unsigned int UINT4;
#else
# if (SIZEOF_UNSIGNED_INT==4)
typedef unsigned int UINT4;
# elif (SIZEOF_UNSIGNED_SHORT==4)
typedef unsigned short UINT4;
# elif (SIZEOF_UNSIGNED_LONG==4)
typedef unsigned long UINT4;
# else
# error no type for UINT4
# endif
#endif
#else
typedef R_DEFINED_UINT4 UINT4;
#endif

#ifndef R_DEFINED_INT8
#ifndef SIZEOF_INT
typedef long long INT8;
#else
# if (SIZEOF_INT==8)
typedef int INT8;
# elif (SIZEOF_SHORT==8)
typedef short INT8;
# elif (SIZEOF_LONG==8)
typedef long INT8;
# elif (SIZEOF_LONG_LONG==8)
typedef long long INT8;
# else
# error no type for INT8
# endif
#endif
#else
typedef R_DEFINED_INT8 INT8;
#endif

#ifndef R_DEFINED_UINT8
#ifndef SIZEOF_UNSIGNED_INT
typedef unsigned long long UINT8;
#else
# if (SIZEOF_UNSIGNED_INT==8)
typedef unsigned int UINT8;
# elif (SIZEOF_UNSIGNED_SHORT==8)
typedef unsigned short UINT8;
# elif (SIZEOF_UNSIGNED_LONG==8)
typedef unsigned long UINT8;
# elif (SIZEOF_UNSIGNED_LONG_LONG==8)
typedef unsigned long long UINT8;
# else
# error no type for UINT8
# endif
#endif
#else
typedef R_DEFINED_UINT8 UINT8;
#endif

#ifndef R_DEFINED_UCHAR
typedef unsigned char UCHAR;
#else
typedef R_DEFINED_UCHAR UCHAR;
#endif
#endif

