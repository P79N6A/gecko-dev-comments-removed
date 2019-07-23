










































#if defined(WIN32) || defined(OS2)
  



  typedef int int32_t;
  typedef unsigned int uint32_t;
#elif defined(_AIX) || defined(__sun) || defined(__osf__) || defined(IRIX) || defined(HPUX)
  



  #include <inttypes.h>

  #ifndef __cplusplus
    typedef int bool;
  #endif
#elif defined(bsdi) || defined(FREEBSD) || defined(OPENBSD)
  



  #include <sys/types.h>

  




  #if defined(bsdi) || defined(OPENBSD)
  typedef u_int32_t uint32_t;

  #if !defined(__cplusplus)
    typedef int bool;
  #endif
  #else
  


    #include <inttypes.h>
    #include <stdbool.h>
  #endif
#elif defined(BEOS)
  #include <inttypes.h>
#else
  




  #include <stdint.h>

  #ifndef __cplusplus
    #if !defined(__GNUC__) || (__GNUC__ > 2 || __GNUC_MINOR__ > 95)
      #include <stdbool.h>
    #else
      



      #define bool int
    #endif
  #endif
#endif
