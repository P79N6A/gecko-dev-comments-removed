
#ifndef LIBAV_CONFIG_H
#define LIBAV_CONFIG_H
#define LIBAV_CONFIGURATION "--disable-programs --disable-everything"
#define LIBAV_LICENSE "LGPL version 2.1 or later"
#define AVCONV_DATADIR "/usr/local/share/avconv"
#define CC_IDENT "gcc 4.9.2 (Debian 4.9.2-10)"
#define restrict restrict
#define EXTERN_PREFIX ""
#define EXTERN_ASM
#define SLIBSUF ".so"
#define HAVE_ARMV5TE 0
#define HAVE_ARMV6 0
#define HAVE_ARMV6T2 0
#define HAVE_ARMV8 0
#define HAVE_NEON 0
#define HAVE_VFP 0
#define HAVE_VFPV3 0
#define HAVE_ALTIVEC 0
#define HAVE_DCBZL 1
#define HAVE_LDBRX 1
#define HAVE_PPC4XX 0
#define HAVE_AMD3DNOW 1
#define HAVE_AMD3DNOWEXT 1
#define HAVE_AVX 1
#define HAVE_AVX2 1
#define HAVE_FMA3 1
#define HAVE_FMA4 1
#define HAVE_MMX 1
#define HAVE_MMXEXT 1
#define HAVE_SSE 1
#define HAVE_SSE2 1
#define HAVE_SSE3 1
#define HAVE_SSE4 1
#define HAVE_SSE42 1
#define HAVE_SSSE3 1
#define HAVE_XOP 1
#define HAVE_CPUNOP 1
#define HAVE_I686 1
#define HAVE_LOONGSON 1
#define HAVE_VIS 1
#define HAVE_ARMV5TE_EXTERNAL 0
#define HAVE_ARMV6_EXTERNAL 0
#define HAVE_ARMV6T2_EXTERNAL 0
#define HAVE_ARMV8_EXTERNAL 0
#define HAVE_NEON_EXTERNAL 0
#define HAVE_VFP_EXTERNAL 0
#define HAVE_VFPV3_EXTERNAL 0
#define HAVE_ALTIVEC_EXTERNAL 0
#define HAVE_DCBZL_EXTERNAL 0
#define HAVE_LDBRX_EXTERNAL 0
#define HAVE_PPC4XX_EXTERNAL 0
#define HAVE_AMD3DNOW_EXTERNAL 1
#define HAVE_AMD3DNOWEXT_EXTERNAL 1
#define HAVE_AVX_EXTERNAL 1
#define HAVE_AVX2_EXTERNAL 1
#define HAVE_FMA3_EXTERNAL 1
#define HAVE_FMA4_EXTERNAL 1
#define HAVE_MMX_EXTERNAL 1
#define HAVE_MMXEXT_EXTERNAL 1
#define HAVE_SSE_EXTERNAL 1
#define HAVE_SSE2_EXTERNAL 1
#define HAVE_SSE3_EXTERNAL 1
#define HAVE_SSE4_EXTERNAL 1
#define HAVE_SSE42_EXTERNAL 1
#define HAVE_SSSE3_EXTERNAL 1
#define HAVE_XOP_EXTERNAL 1
#define HAVE_CPUNOP_EXTERNAL 0
#define HAVE_I686_EXTERNAL 0
#define HAVE_LOONGSON_EXTERNAL 0
#define HAVE_VIS_EXTERNAL 0
#define HAVE_ARMV5TE_INLINE 0
#define HAVE_ARMV6_INLINE 0
#define HAVE_ARMV6T2_INLINE 0
#define HAVE_ARMV8_INLINE 0
#define HAVE_NEON_INLINE 0
#define HAVE_VFP_INLINE 0
#define HAVE_VFPV3_INLINE 0
#define HAVE_ALTIVEC_INLINE 0
#define HAVE_DCBZL_INLINE 0
#define HAVE_LDBRX_INLINE 0
#define HAVE_PPC4XX_INLINE 0
#define HAVE_AMD3DNOW_INLINE 1
#define HAVE_AMD3DNOWEXT_INLINE 1
#define HAVE_AVX_INLINE 1
#define HAVE_AVX2_INLINE 1
#define HAVE_FMA3_INLINE 1
#define HAVE_FMA4_INLINE 1
#define HAVE_MMX_INLINE 1
#define HAVE_MMXEXT_INLINE 1
#define HAVE_SSE_INLINE 1
#define HAVE_SSE2_INLINE 1
#define HAVE_SSE3_INLINE 1
#define HAVE_SSE4_INLINE 1
#define HAVE_SSE42_INLINE 1
#define HAVE_SSSE3_INLINE 1
#define HAVE_XOP_INLINE 1
#define HAVE_CPUNOP_INLINE 0
#define HAVE_I686_INLINE 0
#define HAVE_LOONGSON_INLINE 0
#define HAVE_VIS_INLINE 0
#define HAVE_ALIGNED_STACK 1
#if defined(i386) || defined(__i386__) || defined(_M_IX86)
#define HAVE_FAST_64BIT 0
#define HAVE_FAST_CLZ 1
#define HAVE_FAST_CMOV 0
#else
#define HAVE_FAST_64BIT 1
#define HAVE_FAST_CLZ 1
#define HAVE_FAST_CMOV 1
#endif
#define HAVE_LOCAL_ALIGNED_8 1
#define HAVE_LOCAL_ALIGNED_16 1
#define HAVE_SIMD_ALIGN_16 1
#define HAVE_ATOMICS_GCC 1
#define HAVE_ATOMICS_SUNCC 0
#define HAVE_ATOMICS_WIN32 0
#define HAVE_ATOMIC_CAS_PTR 0
#define HAVE_MACHINE_RW_BARRIER 0
#define HAVE_MEMORYBARRIER 0
#define HAVE_MM_EMPTY 1
#define HAVE_RDTSC 0
#define HAVE_SYNC_VAL_COMPARE_AND_SWAP 1
#define HAVE_INLINE_ASM 1
#define HAVE_SYMVER 1
#define HAVE_YASM 1
#define HAVE_BIGENDIAN 0
#define HAVE_FAST_UNALIGNED 1
#define HAVE_ALSA_ASOUNDLIB_H 0
#define HAVE_ALTIVEC_H 0
#define HAVE_ARPA_INET_H 0
#define HAVE_CDIO_PARANOIA_H 0
#define HAVE_CDIO_PARANOIA_PARANOIA_H 0
#define HAVE_DEV_BKTR_IOCTL_BT848_H 0
#define HAVE_DEV_BKTR_IOCTL_METEOR_H 0
#define HAVE_DEV_IC_BT8XX_H 0
#define HAVE_DEV_VIDEO_BKTR_IOCTL_BT848_H 0
#define HAVE_DEV_VIDEO_METEOR_IOCTL_METEOR_H 0
#define HAVE_DIRECT_H 0
#define HAVE_DLFCN_H 1
#define HAVE_DXVA_H 0
#define HAVE_GSM_H 0
#define HAVE_IO_H 0
#define HAVE_MACH_MACH_TIME_H 0
#define HAVE_MACHINE_IOCTL_BT848_H 0
#define HAVE_MACHINE_IOCTL_METEOR_H 0
#define HAVE_MALLOC_H 1
#define HAVE_POLL_H 1
#define HAVE_SNDIO_H 0
#define HAVE_SOUNDCARD_H 0
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_SOUNDCARD_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_UN_H 1
#define HAVE_SYS_VIDEOIO_H 0
#define HAVE_UNISTD_H 1
#define HAVE_WINDOWS_H 0
#define HAVE_WINSOCK2_H 0
#define HAVE_INTRINSICS_NEON 0
#define HAVE_ATANF 1
#define HAVE_ATAN2F 1
#define HAVE_CBRTF 1
#define HAVE_COSF 1
#define HAVE_EXP2 1
#define HAVE_EXP2F 1
#define HAVE_EXPF 1
#define HAVE_ISINF 1
#define HAVE_ISNAN 1
#define HAVE_LDEXPF 1
#define HAVE_LLRINT 1
#define HAVE_LLRINTF 1
#define HAVE_LOG2 1
#define HAVE_LOG2F 1
#define HAVE_LOG10F 1
#define HAVE_LRINT 1
#define HAVE_LRINTF 1
#define HAVE_POWF 1
#define HAVE_RINT 1
#define HAVE_ROUND 1
#define HAVE_ROUNDF 1
#define HAVE_SINF 1
#define HAVE_TRUNC 1
#define HAVE_TRUNCF 1
#define HAVE_ALIGNED_MALLOC 0
#define HAVE_CLOSESOCKET 0
#define HAVE_COMMANDLINETOARGVW 0
#define HAVE_COTASKMEMFREE 0
#define HAVE_CRYPTGENRANDOM 0
#define HAVE_DLOPEN 1
#define HAVE_FCNTL 1
#define HAVE_FLT_LIM 1
#define HAVE_FORK 1
#define HAVE_GETADDRINFO 0
#define HAVE_GETHRTIME 0
#define HAVE_GETOPT 1
#define HAVE_GETPROCESSAFFINITYMASK 0
#define HAVE_GETPROCESSMEMORYINFO 0
#define HAVE_GETPROCESSTIMES 0
#define HAVE_GETRUSAGE 1
#define HAVE_GETSERVBYPORT 0
#define HAVE_GETSYSTEMTIMEASFILETIME 0
#define HAVE_GETTIMEOFDAY 1
#define HAVE_INET_ATON 0
#define HAVE_ISATTY 1
#define HAVE_JACK_PORT_GET_LATENCY_RANGE 0
#define HAVE_LOCALTIME_R 1
#define HAVE_MACH_ABSOLUTE_TIME 0
#define HAVE_MAPVIEWOFFILE 0
#define HAVE_MEMALIGN 1
#define HAVE_MKSTEMP 1
#define HAVE_MMAP 1
#define HAVE_MPROTECT 1
#define HAVE_NANOSLEEP 1
#define HAVE_POSIX_MEMALIGN 1
#define HAVE_SCHED_GETAFFINITY 1
#define HAVE_SETCONSOLETEXTATTRIBUTE 0
#define HAVE_SETMODE 0
#define HAVE_SETRLIMIT 1
#define HAVE_SLEEP 0
#define HAVE_STRERROR_R 1
#define HAVE_STRPTIME 1
#define HAVE_SYSCONF 1
#define HAVE_SYSCTL 1
#define HAVE_USLEEP 1
#define HAVE_VIRTUALALLOC 0
#define HAVE_PTHREADS 0
#define HAVE_W32THREADS 0
#define HAVE_AS_DN_DIRECTIVE 0
#define HAVE_AS_FUNC 0
#define HAVE_AS_OBJECT_ARCH 0
#define HAVE_ASM_MOD_Q 0
#define HAVE_ATTRIBUTE_MAY_ALIAS 1
#define HAVE_ATTRIBUTE_PACKED 1
#define HAVE_EBP_AVAILABLE 1
#define HAVE_EBX_AVAILABLE 1
#define HAVE_GNU_AS 0
#define HAVE_IBM_ASM 0
#define HAVE_INLINE_ASM_LABELS 1
#define HAVE_PRAGMA_DEPRECATED 1
#define HAVE_SYMVER_ASM_LABEL 0
#define HAVE_SYMVER_GNU_ASM 1
#define HAVE_VFP_ARGS 0
#define HAVE_XFORM_ASM 0
#define HAVE_XMM_CLOBBERS 1
#define HAVE_SOCKLEN_T 0
#define HAVE_STRUCT_ADDRINFO 0
#define HAVE_STRUCT_GROUP_SOURCE_REQ 0
#define HAVE_STRUCT_IP_MREQ_SOURCE 0
#define HAVE_STRUCT_IPV6_MREQ 0
#define HAVE_STRUCT_POLLFD 0
#define HAVE_STRUCT_RUSAGE_RU_MAXRSS 1
#define HAVE_STRUCT_SOCKADDR_IN6 0
#define HAVE_STRUCT_SOCKADDR_SA_LEN 0
#define HAVE_STRUCT_SOCKADDR_STORAGE 0
#define HAVE_STRUCT_V4L2_FRMIVALENUM_DISCRETE 1
#define HAVE_ATOMICS_NATIVE 1
#define HAVE_DOS_PATHS 0
#define HAVE_DXVA2_LIB 0
#define HAVE_LIBC_MSVCRT 0
#define HAVE_LIBDC1394_1 0
#define HAVE_LIBDC1394_2 0
#define HAVE_SDL 0
#define HAVE_THREADS 0
#define HAVE_VDPAU_X11 0
#define HAVE_XLIB 1
#endif 
