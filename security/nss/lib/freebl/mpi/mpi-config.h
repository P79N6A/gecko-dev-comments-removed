







































#ifndef MPI_CONFIG_H_
#define MPI_CONFIG_H_










#ifndef MP_IOFUNC
#define MP_IOFUNC     0  /* include mp_print() ?                */
#endif

#ifndef MP_MODARITH
#define MP_MODARITH   1  /* include modular arithmetic ?        */
#endif

#ifndef MP_NUMTH
#define MP_NUMTH      1  /* include number theoretic functions? */
#endif

#ifndef MP_LOGTAB
#define MP_LOGTAB     1  /* use table of logs instead of log()? */
#endif

#ifndef MP_MEMSET
#define MP_MEMSET     1  /* use memset() to zero buffers?       */
#endif

#ifndef MP_MEMCPY
#define MP_MEMCPY     1  /* use memcpy() to copy buffers?       */
#endif

#ifndef MP_CRYPTO
#define MP_CRYPTO     1  /* erase memory on free?               */
#endif

#ifndef MP_ARGCHK





#ifdef DEBUG
#define MP_ARGCHK     2  /* how to check input arguments        */
#else
#define MP_ARGCHK     1  /* how to check input arguments        */
#endif
#endif

#ifndef MP_DEBUG
#define MP_DEBUG      0  /* print diagnostic output?            */
#endif

#ifndef MP_DEFPREC
#define MP_DEFPREC    64 /* default precision, in digits        */
#endif

#ifndef MP_MACRO
#define MP_MACRO      1  /* use macros for frequent calls?      */
#endif

#ifndef MP_SQUARE
#define MP_SQUARE     1  /* use separate squaring code?         */
#endif

#endif 


