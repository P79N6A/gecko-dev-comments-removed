
#ifndef __CURL_CURLBUILD_H
#define __CURL_CURLBUILD_H


























































#ifdef CURL_SIZEOF_LONG
#  error "CURL_SIZEOF_LONG shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SIZEOF_LONG_already_defined
#endif

#ifdef CURL_TYPEOF_CURL_SOCKLEN_T
#  error "CURL_TYPEOF_CURL_SOCKLEN_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_TYPEOF_CURL_SOCKLEN_T_already_defined
#endif

#ifdef CURL_SIZEOF_CURL_SOCKLEN_T
#  error "CURL_SIZEOF_CURL_SOCKLEN_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SIZEOF_CURL_SOCKLEN_T_already_defined
#endif

#ifdef CURL_TYPEOF_CURL_OFF_T
#  error "CURL_TYPEOF_CURL_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_TYPEOF_CURL_OFF_T_already_defined
#endif

#ifdef CURL_FORMAT_CURL_OFF_T
#  error "CURL_FORMAT_CURL_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_FORMAT_CURL_OFF_T_already_defined
#endif

#ifdef CURL_FORMAT_CURL_OFF_TU
#  error "CURL_FORMAT_CURL_OFF_TU shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_FORMAT_CURL_OFF_TU_already_defined
#endif

#ifdef CURL_FORMAT_OFF_T
#  error "CURL_FORMAT_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_FORMAT_OFF_T_already_defined
#endif

#ifdef CURL_SIZEOF_CURL_OFF_T
#  error "CURL_SIZEOF_CURL_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SIZEOF_CURL_OFF_T_already_defined
#endif

#ifdef CURL_SUFFIX_CURL_OFF_T
#  error "CURL_SUFFIX_CURL_OFF_T shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SUFFIX_CURL_OFF_T_already_defined
#endif

#ifdef CURL_SUFFIX_CURL_OFF_TU
#  error "CURL_SUFFIX_CURL_OFF_TU shall not be defined except in curlbuild.h"
   Error Compilation_aborted_CURL_SUFFIX_CURL_OFF_TU_already_defined
#endif








#ifdef CURL_PULL_WS2TCPIP_H
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif



#define CURL_PULL_SYS_TYPES_H 1
#ifdef CURL_PULL_SYS_TYPES_H
#  include <sys/types.h>
#endif




#ifdef CURL_PULL_STDINT_H
#  include <stdint.h>
#endif




#ifdef CURL_PULL_INTTYPES_H
#  include <inttypes.h>
#endif



#define CURL_PULL_SYS_SOCKET_H 1
#ifdef CURL_PULL_SYS_SOCKET_H
#  include <sys/socket.h>
#endif


#if defined(_M_X64) || (defined(__x86_64__) && !defined(__ILP32__))
#define CURL_SIZEOF_LONG 8
#else
#define CURL_SIZEOF_LONG 4
#endif


#define CURL_TYPEOF_CURL_SOCKLEN_T socklen_t


#define CURL_SIZEOF_CURL_SOCKLEN_T 4


typedef CURL_TYPEOF_CURL_SOCKLEN_T curl_socklen_t;


#if defined(_M_X64) || (defined(__x86_64__) && !defined(__ILP32__))
#define CURL_TYPEOF_CURL_OFF_T long
#else
#define CURL_TYPEOF_CURL_OFF_T int64_t
#endif


typedef CURL_TYPEOF_CURL_OFF_T curl_off_t;


#define CURL_FORMAT_CURL_OFF_T "ld"


#define CURL_FORMAT_CURL_OFF_TU "lu"


#define CURL_FORMAT_OFF_T "%ld"


#define CURL_SIZEOF_CURL_OFF_T 8


#define CURL_SUFFIX_CURL_OFF_T L


#define CURL_SUFFIX_CURL_OFF_TU UL

#endif 
