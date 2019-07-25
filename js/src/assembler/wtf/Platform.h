

























#ifndef WTF_Platform_h
#define WTF_Platform_h













































#if defined(_MSC_VER)
#define WTF_COMPILER_MSVC 1
#if _MSC_VER < 1400
#define WTF_COMPILER_MSVC7 1
#endif
#endif


#if defined(__CC_ARM) || defined(__ARMCC__)
#define WTF_COMPILER_RVCT 1
#endif



#if defined(__GNUC__) && !WTF_COMPILER_RVCT
#define WTF_COMPILER_GCC 1
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif


#if defined(MINGW) || defined(__MINGW32__)
#define WTF_COMPILER_MINGW 1
#endif


#if defined(__WINSCW__)
#define WTF_COMPILER_WINSCW 1
#endif


#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#define WTF_COMPILER_SUNPRO 1
#endif








#if defined(__alpha__)
#define WTF_CPU_ALPHA 1
#endif


#if defined(__ia64__)
#define WTF_CPU_IA64 1
#endif


#if   defined(__ppc__)     \
   || defined(__PPC__)     \
   || defined(__powerpc__) \
   || defined(__powerpc)   \
   || defined(__POWERPC__) \
   || defined(_M_PPC)      \
   || defined(__PPC)
#define WTF_CPU_PPC 1
#define WTF_CPU_BIG_ENDIAN 1
#endif


#if   defined(__ppc64__) \
   || defined(__PPC64__)
#define WTF_CPU_PPC64 1
#define WTF_CPU_BIG_ENDIAN 1
#endif


#if defined(__SH4__)
#define WTF_CPU_SH4 1
#endif


#if defined(__sparc) && !defined(__arch64__) || defined(__sparcv8)
#define WTF_CPU_SPARC32 1
#define WTF_CPU_BIG_ENDIAN 1
#endif


#if defined(__sparc__) && defined(__arch64__) || defined (__sparcv9)
#define WTF_CPU_SPARC64 1
#define WTF_CPU_BIG_ENDIAN 1
#endif


#if WTF_CPU_SPARC32 || WTF_CPU_SPARC64
#define WTF_CPU_SPARC 1
#endif


#if   defined(__i386__) \
   || defined(i386)     \
   || defined(_M_IX86)  \
   || defined(_X86_)    \
   || defined(__THW_INTEL)
#define WTF_CPU_X86 1
#endif


#if   defined(__x86_64__) \
   || defined(_M_X64)
#define WTF_CPU_X86_64 1
#endif


#if   defined(arm) \
   || defined(__arm__)
#define WTF_CPU_ARM 1

#if defined(__ARMEB__)
#define WTF_CPU_BIG_ENDIAN 1

#elif !defined(__ARM_EABI__) \
   && !defined(__EABI__) \
   && !defined(__VFP_FP__) \
   && !defined(ANDROID)
#define WTF_CPU_MIDDLE_ENDIAN 1

#endif

#define WTF_ARM_ARCH_AT_LEAST(N) (WTF_CPU_ARM && WTF_ARM_ARCH_VERSION >= N)


#if   defined(__ARM_ARCH_4__) \
   || defined(__ARM_ARCH_4T__) \
   || defined(__MARM_ARMV4__) \
   || defined(_ARMV4I_)
#define WTF_ARM_ARCH_VERSION 4

#elif defined(__ARM_ARCH_5__) \
   || defined(__ARM_ARCH_5T__) \
   || defined(__ARM_ARCH_5E__) \
   || defined(__ARM_ARCH_5TE__) \
   || defined(__ARM_ARCH_5TEJ__) \
   || defined(__MARM_ARMV5__)
#define WTF_ARM_ARCH_VERSION 5

#elif defined(__ARM_ARCH_6__) \
   || defined(__ARM_ARCH_6J__) \
   || defined(__ARM_ARCH_6K__) \
   || defined(__ARM_ARCH_6Z__) \
   || defined(__ARM_ARCH_6ZK__) \
   || defined(__ARM_ARCH_6T2__) \
   || defined(__ARMV6__)
#define WTF_ARM_ARCH_VERSION 6

#elif defined(__ARM_ARCH_7A__) \
   || defined(__ARM_ARCH_7R__)
#define WTF_ARM_ARCH_VERSION 7


#elif defined(__TARGET_ARCH_ARM)
#define WTF_ARM_ARCH_VERSION __TARGET_ARCH_ARM

#else
#define WTF_ARM_ARCH_VERSION 0

#endif


#if   defined(__ARM_ARCH_4T__)
#define WTF_THUMB_ARCH_VERSION 1

#elif defined(__ARM_ARCH_5T__) \
   || defined(__ARM_ARCH_5TE__) \
   || defined(__ARM_ARCH_5TEJ__)
#define WTF_THUMB_ARCH_VERSION 2

#elif defined(__ARM_ARCH_6J__) \
   || defined(__ARM_ARCH_6K__) \
   || defined(__ARM_ARCH_6Z__) \
   || defined(__ARM_ARCH_6ZK__) \
   || defined(__ARM_ARCH_6M__)
#define WTF_THUMB_ARCH_VERSION 3

#elif defined(__ARM_ARCH_6T2__) \
   || defined(__ARM_ARCH_7__) \
   || defined(__ARM_ARCH_7A__) \
   || defined(__ARM_ARCH_7R__) \
   || defined(__ARM_ARCH_7M__)
#define WTF_THUMB_ARCH_VERSION 4


#elif defined(__TARGET_ARCH_THUMB)
#define WTF_THUMB_ARCH_VERSION __TARGET_ARCH_THUMB

#else
#define WTF_THUMB_ARCH_VERSION 0
#endif





#if !defined(ARMV5_OR_LOWER) 
#define WTF_CPU_ARMV5_OR_LOWER 1
#endif





#if !defined(WTF_CPU_ARM_TRADITIONAL) && !defined(WTF_CPU_ARM_THUMB2)
#  if defined(thumb2) || defined(__thumb2__) \
  || ((defined(__thumb) || defined(__thumb__)) && WTF_THUMB_ARCH_VERSION == 4)
#    define WTF_CPU_ARM_TRADITIONAL 1
#    define WTF_CPU_ARM_THUMB2 0
#  elif WTF_ARM_ARCH_AT_LEAST(4)
#    define WTF_CPU_ARM_TRADITIONAL 1
#    define WTF_CPU_ARM_THUMB2 0
#  else
#    error "Not supported ARM architecture"
#  endif
#elif WTF_CPU_ARM_TRADITIONAL && WTF_CPU_ARM_THUMB2 
#  error "Cannot use both of WTF_CPU_ARM_TRADITIONAL and WTF_CPU_ARM_THUMB2 platforms"
#endif 

#endif 








#ifdef __APPLE__
#define WTF_PLATFORM_DARWIN 1
#include <AvailabilityMacros.h>
#if !defined(MAC_OS_X_VERSION_10_5) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5
#define BUILDING_ON_TIGER 1
#elif !defined(MAC_OS_X_VERSION_10_6) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_6
#define BUILDING_ON_LEOPARD 1
#elif !defined(MAC_OS_X_VERSION_10_7) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
#define BUILDING_ON_SNOW_LEOPARD 1
#endif
#if !defined(MAC_OS_X_VERSION_10_5) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
#define TARGETING_TIGER 1
#elif !defined(MAC_OS_X_VERSION_10_6) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_6
#define TARGETING_LEOPARD 1
#elif !defined(MAC_OS_X_VERSION_10_7) || MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_7
#define TARGETING_SNOW_LEOPARD 1
#endif
#include <TargetConditionals.h>
#endif




#if defined(WIN32) || defined(_WIN32)
#define WTF_PLATFORM_WIN_OS 1
#endif




#if defined(__linux__) && !defined(ANDROID)
#define WTF_PLATFORM_LINUX 1
#endif




#ifdef __FreeBSD__
#define WTF_PLATFORM_FREEBSD 1
#endif




#ifdef __OpenBSD__
#define WTF_PLATFORM_OPENBSD 1
#endif




#if defined(sun) || defined(__sun)
#define WTF_PLATFORM_SOLARIS 1
#endif




#if defined(OS2) || defined(__OS2__)
#define WTF_PLATFORM_OS2 1
#endif

#if defined (__SYMBIAN32__)

#undef WTF_PLATFORM_WIN_OS
#undef WTF_PLATFORM_WIN
#define WTF_PLATFORM_SYMBIAN 1
#endif





#if defined(__NetBSD__)
#define WTF_PLATFORM_NETBSD 1
#endif




#if defined(__QNXNTO__)
#define WTF_PLATFORM_QNX 1
#endif




#if   WTF_PLATFORM_DARWIN     \
   || WTF_PLATFORM_FREEBSD    \
   || WTF_PLATFORM_SYMBIAN    \
   || WTF_PLATFORM_NETBSD     \
   || defined(unix)        \
   || defined(__unix)      \
   || defined(__unix__)    \
   || defined(_AIX)        \
   || defined(__HAIKU__)   \
   || defined(__QNXNTO__)  \
   || defined(ANDROID)
#define WTF_PLATFORM_UNIX 1
#endif










#if defined(BUILDING_CHROMIUM__)
#define WTF_PLATFORM_CHROMIUM 1
#elif defined(BUILDING_QT__)
#define WTF_PLATFORM_QT 1
#elif defined(BUILDING_WX__)
#define WTF_PLATFORM_WX 1
#elif defined(BUILDING_GTK__)
#define WTF_PLATFORM_GTK 1
#elif defined(BUILDING_HAIKU__)
#define WTF_PLATFORM_HAIKU 1
#elif WTF_PLATFORM_DARWIN
#define WTF_PLATFORM_MAC 1
#elif WTF_PLATFORM_WIN_OS
#define WTF_PLATFORM_WIN 1
#endif


#if (defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define WTF_PLATFORM_IPHONE 1
#endif


#if defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
#define WTF_PLATFORM_IPHONE 1
#define WTF_PLATFORM_IPHONE_SIMULATOR 1
#else
#define WTF_PLATFORM_IPHONE_SIMULATOR 0
#endif

#if !defined(WTF_PLATFORM_IPHONE)
#define WTF_PLATFORM_IPHONE 0
#endif


#if defined(ANDROID)
#define WTF_PLATFORM_ANDROID 1
#endif




#if WTF_PLATFORM_MAC || WTF_PLATFORM_IPHONE
#define WTF_PLATFORM_CG 1
#endif
#if WTF_PLATFORM_MAC && !WTF_PLATFORM_IPHONE
#define WTF_PLATFORM_CI 1
#endif


#if WTF_PLATFORM_CHROMIUM
#if WTF_PLATFORM_DARWIN
#define WTF_PLATFORM_CG 1
#define WTF_PLATFORM_CI 1
#define WTF_USE_ATSUI 1
#define WTF_USE_CORE_TEXT 1
#else
#define WTF_PLATFORM_SKIA 1
#endif
#endif

#if WTF_PLATFORM_GTK
#define WTF_PLATFORM_CAIRO 1
#endif


#if (WTF_PLATFORM_IPHONE || WTF_PLATFORM_MAC || WTF_PLATFORM_WIN || WTF_PLATFORM_OS2 || (WTF_PLATFORM_QT && WTF_PLATFORM_DARWIN && !ENABLE_SINGLE_THREADED)) && !defined(ENABLE_JSC_MULTIPLE_THREADS)
#define ENABLE_JSC_MULTIPLE_THREADS 1
#endif


#if WTF_PLATFORM_WIN_OS
#define WTF_USE_QUERY_PERFORMANCE_COUNTER  1
#endif

#if WTF_PLATFORM_QT
#define WTF_USE_QT4_UNICODE 1
#elif WTF_PLATFORM_GTK

#else
#define WTF_USE_ICU_UNICODE 1
#endif

#if WTF_PLATFORM_MAC && !WTF_PLATFORM_IPHONE
#define WTF_PLATFORM_CF 1
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_TIGER) && WTF_CPU_X86_64
#define WTF_USE_PLUGIN_HOST_PROCESS 1
#endif
#if !defined(ENABLE_MAC_JAVA_BRIDGE)
#define ENABLE_MAC_JAVA_BRIDGE 1
#endif
#if !defined(ENABLE_DASHBOARD_SUPPORT)
#define ENABLE_DASHBOARD_SUPPORT 1
#endif
#define HAVE_READLINE 1
#define HAVE_RUNLOOP_TIMER 1
#endif 

#if WTF_PLATFORM_CHROMIUM && WTF_PLATFORM_DARWIN
#define WTF_PLATFORM_CF 1
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1
#endif

#if WTF_PLATFORM_QT && WTF_PLATFORM_DARWIN
#define WTF_PLATFORM_CF 1
#endif

#if WTF_PLATFORM_IPHONE
#define ENABLE_CONTEXT_MENUS 0
#define ENABLE_DRAG_SUPPORT 0
#define ENABLE_FTPDIR 1
#define ENABLE_GEOLOCATION 1
#define ENABLE_ICONDATABASE 0
#define ENABLE_INSPECTOR 0
#define ENABLE_MAC_JAVA_BRIDGE 0
#define ENABLE_NETSCAPE_PLUGIN_API 0
#define ENABLE_ORIENTATION_EVENTS 1
#define ENABLE_REPAINT_THROTTLING 1
#define HAVE_READLINE 1
#define WTF_PLATFORM_CF 1
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1
#endif

#if WTF_PLATFORM_ANDROID
#define WTF_USE_PTHREADS 1
#define WTF_PLATFORM_SGL 1
#define USE_SYSTEM_MALLOC 1
#define ENABLE_MAC_JAVA_BRIDGE 1
#define LOG_DISABLED 1


#define ENABLE_TEXT_CARET 1
#define ENABLE_JAVASCRIPT_DEBUGGER 0
#endif

#if WTF_PLATFORM_WIN
#define WTF_USE_WININET 1
#endif

#if WTF_PLATFORM_WX
#define ENABLE_ASSEMBLER 1
#if WTF_PLATFORM_DARWIN
#define WTF_PLATFORM_CF 1
#endif
#endif

#if WTF_PLATFORM_GTK
#if HAVE_PTHREAD_H
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1
#endif
#endif

#if WTF_PLATFORM_HAIKU
#define HAVE_POSIX_MEMALIGN 1
#define WTF_USE_CURL 1
#define WTF_USE_PTHREADS 1
#define HAVE_PTHREAD_RWLOCK 1
#define USE_SYSTEM_MALLOC 1
#define ENABLE_NETSCAPE_PLUGIN_API 0
#endif

#if !defined(HAVE_ACCESSIBILITY)
#if WTF_PLATFORM_IPHONE || WTF_PLATFORM_MAC || WTF_PLATFORM_WIN || WTF_PLATFORM_GTK || WTF_PLATFORM_CHROMIUM
#define HAVE_ACCESSIBILITY 1
#endif
#endif 

#if WTF_PLATFORM_UNIX && !WTF_PLATFORM_SYMBIAN
#define HAVE_SIGNAL_H 1
#endif

#if !WTF_PLATFORM_WIN_OS && !WTF_PLATFORM_SOLARIS && !WTF_PLATFORM_QNX \
    && !WTF_PLATFORM_SYMBIAN && !WTF_PLATFORM_HAIKU && !WTF_COMPILER_RVCT \
    && !WTF_PLATFORM_ANDROID && !WTF_PLATFORM_OS2
#define HAVE_TM_GMTOFF 1
#define HAVE_TM_ZONE 1
#define HAVE_TIMEGM 1
#endif     

#if WTF_PLATFORM_DARWIN

#define HAVE_ERRNO_H 1
#define HAVE_LANGINFO_H 1
#define HAVE_MMAP 1
#define HAVE_MERGESORT 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TIMEB_H 1

#if !defined(BUILDING_ON_TIGER) && !defined(BUILDING_ON_LEOPARD) && !WTF_PLATFORM_IPHONE && !WTF_PLATFORM_QT
#define HAVE_MADV_FREE_REUSE 1
#define HAVE_MADV_FREE 1
#define HAVE_PTHREAD_SETNAME_NP 1
#endif

#if WTF_PLATFORM_IPHONE
#define HAVE_MADV_FREE 1
#endif

#elif WTF_PLATFORM_WIN_OS

#define HAVE_SYS_TIMEB_H 1
#define HAVE_VIRTUALALLOC 1

#elif WTF_PLATFORM_SYMBIAN

#define HAVE_ERRNO_H 1
#define HAVE_MMAP 0
#define HAVE_SBRK 1

#define HAVE_SYS_TIME_H 1
#define HAVE_STRINGS_H 1

#if !WTF_COMPILER_RVCT
#define HAVE_SYS_PARAM_H 1
#endif

#elif WTF_PLATFORM_QNX

#define HAVE_ERRNO_H 1
#define HAVE_MMAP 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1

#elif WTF_PLATFORM_ANDROID

#define HAVE_ERRNO_H 1
#define HAVE_LANGINFO_H 0
#define HAVE_MMAP 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1

#elif WTF_PLATFORM_OS2

#define HAVE_MMAP 1
#define ENABLE_ASSEMBLER 1
#define HAVE_ERRNO_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TIMEB_H 1

#else



#define HAVE_ERRNO_H 1

#if !WTF_PLATFORM_HAIKU
#define HAVE_LANGINFO_H 1
#endif
#define HAVE_MMAP 1
#define HAVE_SBRK 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1

#endif





#if !defined(ENABLE_FAST_MALLOC_MATCH_VALIDATION)
#define ENABLE_FAST_MALLOC_MATCH_VALIDATION 0
#endif

#if !defined(ENABLE_ICONDATABASE)
#define ENABLE_ICONDATABASE 1
#endif

#if !defined(ENABLE_DATABASE)
#define ENABLE_DATABASE 1
#endif

#if !defined(ENABLE_JAVASCRIPT_DEBUGGER)
#define ENABLE_JAVASCRIPT_DEBUGGER 1
#endif

#if !defined(ENABLE_FTPDIR)
#define ENABLE_FTPDIR 1
#endif

#if !defined(ENABLE_CONTEXT_MENUS)
#define ENABLE_CONTEXT_MENUS 1
#endif

#if !defined(ENABLE_DRAG_SUPPORT)
#define ENABLE_DRAG_SUPPORT 1
#endif

#if !defined(ENABLE_DASHBOARD_SUPPORT)
#define ENABLE_DASHBOARD_SUPPORT 0
#endif

#if !defined(ENABLE_INSPECTOR)
#define ENABLE_INSPECTOR 1
#endif

#if !defined(ENABLE_MAC_JAVA_BRIDGE)
#define ENABLE_MAC_JAVA_BRIDGE 0
#endif

#if !defined(ENABLE_NETSCAPE_PLUGIN_API)
#define ENABLE_NETSCAPE_PLUGIN_API 1
#endif

#if !defined(WTF_USE_PLUGIN_HOST_PROCESS)
#define WTF_USE_PLUGIN_HOST_PROCESS 0
#endif

#if !defined(ENABLE_ORIENTATION_EVENTS)
#define ENABLE_ORIENTATION_EVENTS 0
#endif

#if !defined(ENABLE_OPCODE_STATS)
#define ENABLE_OPCODE_STATS 0
#endif

#define ENABLE_SAMPLING_COUNTERS 0
#define ENABLE_SAMPLING_FLAGS 0
#define ENABLE_OPCODE_SAMPLING 0
#define ENABLE_CODEBLOCK_SAMPLING 0
#if ENABLE_CODEBLOCK_SAMPLING && !ENABLE_OPCODE_SAMPLING
#error "CODEBLOCK_SAMPLING requires OPCODE_SAMPLING"
#endif
#if ENABLE_OPCODE_SAMPLING || ENABLE_SAMPLING_FLAGS
#define ENABLE_SAMPLING_THREAD 1
#endif

#if !defined(ENABLE_GEOLOCATION)
#define ENABLE_GEOLOCATION 0
#endif

#if !defined(ENABLE_NOTIFICATIONS)
#define ENABLE_NOTIFICATIONS 0
#endif

#if !defined(ENABLE_TEXT_CARET)
#define ENABLE_TEXT_CARET 1
#endif

#if !defined(ENABLE_ON_FIRST_TEXTAREA_FOCUS_SELECT_ALL)
#define ENABLE_ON_FIRST_TEXTAREA_FOCUS_SELECT_ALL 0
#endif

#if !defined(WTF_USE_JSVALUE64) && !defined(WTF_USE_JSVALUE32) && !defined(WTF_USE_JSVALUE32_64)
#if (WTF_CPU_X86_64 && (WTF_PLATFORM_UNIX || WTF_PLATFORM_WIN_OS)) || WTF_CPU_IA64 || WTF_CPU_ALPHA
#define WTF_USE_JSVALUE64 1
#elif WTF_CPU_ARM || WTF_CPU_PPC64
#define WTF_USE_JSVALUE32 1
#elif WTF_PLATFORM_WIN_OS && WTF_COMPILER_MINGW


#define WTF_USE_JSVALUE32 1
#else
#define WTF_USE_JSVALUE32_64 1
#endif
#endif 

#if !defined(ENABLE_REPAINT_THROTTLING)
#define ENABLE_REPAINT_THROTTLING 0
#endif

#if !defined(ENABLE_JIT)


#if WTF_CPU_X86_64 && WTF_PLATFORM_MAC
    #define ENABLE_JIT 1

#elif WTF_CPU_X86 && WTF_PLATFORM_MAC
    #define ENABLE_JIT 1
    #define WTF_USE_JIT_STUB_ARGUMENT_VA_LIST 1
#elif WTF_CPU_ARM_THUMB2 && WTF_PLATFORM_IPHONE
    #define ENABLE_JIT 1

#elif WTF_CPU_X86 && WTF_PLATFORM_OS2
    #define ENABLE_JIT 1

#elif WTF_CPU_X86 && WTF_PLATFORM_WIN
    #define ENABLE_JIT 1
#elif WTF_CPU_SPARC
    #define ENABLE_JIT 1
#endif

#if WTF_PLATFORM_QT
#if WTF_CPU_X86_64 && WTF_PLATFORM_DARWIN
    #define ENABLE_JIT 1
#elif WTF_CPU_X86 && WTF_PLATFORM_DARWIN
    #define ENABLE_JIT 1
    #define WTF_USE_JIT_STUB_ARGUMENT_VA_LIST 1
#elif WTF_CPU_X86 && WTF_PLATFORM_WIN_OS && WTF_COMPILER_MINGW && GCC_VERSION >= 40100
    #define ENABLE_JIT 1
    #define WTF_USE_JIT_STUB_ARGUMENT_VA_LIST 1
#elif WTF_CPU_X86 && WTF_PLATFORM_WIN_OS && WTF_COMPILER_MSVC
    #define ENABLE_JIT 1
    #define WTF_USE_JIT_STUB_ARGUMENT_REGISTER 1
#elif WTF_CPU_X86 && WTF_PLATFORM_LINUX && GCC_VERSION >= 40100
    #define ENABLE_JIT 1
    #define WTF_USE_JIT_STUB_ARGUMENT_VA_LIST 1
#elif WTF_CPU_ARM_TRADITIONAL && WTF_PLATFORM_LINUX
    #define ENABLE_JIT 1
#endif
#endif 

#endif 

#if ENABLE_JIT
#ifndef ENABLE_JIT_OPTIMIZE_CALL
#define ENABLE_JIT_OPTIMIZE_CALL 1
#endif
#ifndef ENABLE_JIT_OPTIMIZE_NATIVE_CALL
#define ENABLE_JIT_OPTIMIZE_NATIVE_CALL 1
#endif
#ifndef ENABLE_JIT_OPTIMIZE_PROPERTY_ACCESS
#define ENABLE_JIT_OPTIMIZE_PROPERTY_ACCESS 1
#endif
#ifndef ENABLE_JIT_OPTIMIZE_METHOD_CALLS
#define ENABLE_JIT_OPTIMIZE_METHOD_CALLS 1
#endif
#endif

#if WTF_CPU_X86 && WTF_COMPILER_MSVC
#define JSC_HOST_CALL __fastcall
#elif WTF_CPU_X86 && WTF_COMPILER_GCC
#define JSC_HOST_CALL __attribute__ ((fastcall))
#else
#define JSC_HOST_CALL
#endif

#if WTF_COMPILER_GCC && !ENABLE_JIT
#define HAVE_COMPUTED_GOTO 1
#endif

#if ENABLE_JIT && defined(COVERAGE)
    #define WTF_USE_INTERPRETER 0
#else
    #define WTF_USE_INTERPRETER 1
#endif


#if !defined(ENABLE_YARR_JIT)


#if (WTF_CPU_X86 \
 || WTF_CPU_X86_64 \
 || WTF_CPU_SPARC \
 || WTF_CPU_ARM_TRADITIONAL \
 || WTF_CPU_ARM_THUMB2 \
 || WTF_CPU_X86)
#define ENABLE_YARR_JIT 1
#else
#define ENABLE_YARR_JIT 0
#endif

#endif 

#if (ENABLE_JIT || ENABLE_YARR_JIT)
#define ENABLE_ASSEMBLER 1
#endif


#if WTF_PLATFORM_IPHONE
#define ENABLE_ASSEMBLER_WX_EXCLUSIVE 1
#else
#define ENABLE_ASSEMBLER_WX_EXCLUSIVE 0
#endif

#if !defined(ENABLE_PAN_SCROLLING) && WTF_PLATFORM_WIN_OS
#define ENABLE_PAN_SCROLLING 1
#endif



#if WTF_PLATFORM_QT
#define WTF_USE_QXMLSTREAM 1
#define WTF_USE_QXMLQUERY 1
#endif

#if !WTF_PLATFORM_QT
#define WTF_USE_FONT_FAST_PATH 1
#endif


#if WTF_PLATFORM_MAC
#if !defined(BUILDING_ON_TIGER)
#define WTF_USE_ACCELERATED_COMPOSITING 1
#endif
#endif

#if WTF_PLATFORM_IPHONE
#define WTF_USE_ACCELERATED_COMPOSITING 1
#endif












#if WTF_COMPILER_GCC
#define WARN_UNUSED_RETURN __attribute__ ((warn_unused_result))
#else
#define WARN_UNUSED_RETURN
#endif

#if !ENABLE_NETSCAPE_PLUGIN_API || (ENABLE_NETSCAPE_PLUGIN_API && ((WTF_PLATFORM_UNIX && (WTF_PLATFORM_QT || WTF_PLATFORM_WX)) || WTF_PLATFORM_GTK))
#define ENABLE_PLUGIN_PACKAGE_SIMPLE_HASH 1
#endif


#define WTF_PLATFORM_CFNETWORK Error USE_macro_should_be_used_with_CFNETWORK

#define ENABLE_JSC_ZOMBIES 0

#endif 
