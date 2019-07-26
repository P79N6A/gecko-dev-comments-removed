
















#ifndef __UPOSIXDEFS_H__
#define __UPOSIXDEFS_H__












#ifdef _XOPEN_SOURCE
    
#else
    









#   define _XOPEN_SOURCE 600
#endif








#if !defined(_XOPEN_SOURCE_EXTENDED)
#   define _XOPEN_SOURCE_EXTENDED 1
#endif









#if (U_PLATFORM == U_PF_AIX && !defined(__GNUC__)) || (U_PLATFORM == U_PF_SOLARIS && defined(__GNUC__))
#   if _XOPEN_SOURCE_EXTENDED && !defined(U_HAVE_STD_STRING)
#   define U_HAVE_STD_STRING 0
#   endif
#endif

#endif  
