


































#ifndef __stl_config__system_h
#define __stl_config__system_h

#if defined (__sun)
#  include <stl/config/_solaris.h>
#  if defined (__GNUC__)
#    include <stl/config/_gcc.h>
#  elif defined (__SUNPRO_CC) || defined (__SUNPRO_C)
#    include <stl/config/_sunprocc.h>





#  elif defined (__APOGEE__)  
#    include <stl/config/_apcc.h>
#  elif defined (__FCC_VERSION) 
#    include <stl/config/_fujitsu.h>
#  endif
#elif defined (__hpux)
#  include <stl/config/_hpux.h>
#  if defined (__GNUC__)
#    include <stl/config/_gcc.h>
#  elif defined (__HP_aCC)
#    include <stl/config/_hpacc.h>
#  endif
#elif defined (__ANDROID__)

#  include <stl/config/_android.h>
#elif defined (linux) || defined (__linux__)
#  include <stl/config/_linux.h>
#  if defined (__BORLANDC__)
#    include <stl/config/_bc.h> 

#  elif defined (__INTEL_COMPILER)
#    include <stl/config/_icc.h>
#  elif defined (__GNUC__)
#    include <stl/config/_gcc.h>
#  endif





#elif defined (__FreeBSD__)
#  include <stl/config/_freebsd.h>
#  if defined (__GNUC__)
#    include <stl/config/_gcc.h>
#  endif
#elif defined (__OpenBSD__)
#  include <stl/config/_openbsd.h>
#  if defined (__GNUC__)
#    include <stl/config/_gcc.h>
#  endif
#elif defined (__sgi) 
#  define _STLP_PLATFORM "SGI Irix"
#  if defined (__GNUC__)
#    include <stl/config/_gcc.h>
#  else
#    include <stl/config/_sgi.h>
#  endif
#elif defined (__OS400__) 
#  define _STLP_PLATFORM "OS 400"
#  if defined (__GNUC__)
#    include <stl/config/_gcc.h>
#  else
#    include <stl/config/_as400.h>
#  endif
#elif defined (_AIX)
#  include <stl/config/_aix.h>
#  if defined (__xlC__) || defined (__IBMC__) || defined ( __IBMCPP__ )
     
#    include <stl/config/_ibm.h>
#  endif
#elif defined (_CRAY) 
#  define _STLP_PLATFORM "Cray"
#  include <config/_cray.h>
#elif defined (__DECCXX) || defined (__DECC)
#  define _STLP_PLATFORM "DECC"
#  ifdef __vms
#    include <stl/config/_dec_vms.h>
#  else
#    include <stl/config/_dec.h>
#  endif
#elif defined (macintosh) || defined (_MAC)
#  include <stl/config/_mac.h>
#  if defined (__MWERKS__)
#    include <stl/config/_mwerks.h>
#  endif
#elif defined (__APPLE__)
#  include <stl/config/_macosx.h>
#  ifdef __GNUC__
#    include <stl/config/_gcc.h>
#  endif
#elif defined (__CYGWIN__)
#  include <stl/config/_cygwin.h>
#  if defined (__GNUC__)
#    include <stl/config/_gcc.h>
#  endif
#elif defined (__MINGW32__)
#  define _STLP_PLATFORM "MinGW"
#  if defined (__GNUC__)
#    include <stl/config/_gcc.h>
#  endif
#  include <stl/config/_windows.h>
#elif defined (_WIN32) || defined (__WIN32) || defined (WIN32) || defined (__WIN32__) || \
      defined (__WIN16) || defined (WIN16) || defined (_WIN16)
#  if defined ( __BORLANDC__ )  
#    include <stl/config/_bc.h>
#  elif defined (__WATCOM_CPLUSPLUS__) || defined (__WATCOMC__)  
#    include <stl/config/_watcom.h>
#  elif defined (__COMO__) || defined (__COMO_VERSION_)
#    include <stl/config/_como.h>
#  elif defined (__DMC__)   
#    include <stl/config/_dm.h>
#  elif defined (__ICL) 
#    include <stl/config/_intel.h>
#  elif defined (__MWERKS__)
#    include <stl/config/_mwerks.h>
#  elif defined (_MSC_VER) && (_MSC_VER >= 1200) && defined (UNDER_CE)
     
#    include <stl/config/_evc.h>
#  elif defined (_MSC_VER)
    
#    include <stl/config/_msvc.h>
#  endif

#  include <stl/config/_windows.h>
#else
#  error Unknown platform !!
#endif

#if !defined (_STLP_COMPILER)


#  include <stl/config/stl_mycomp.h>
#endif

#endif 
