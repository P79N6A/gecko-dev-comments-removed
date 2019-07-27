
















































































#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/NullPtr.h"


#define _GNU_SOURCE 1
#define _DARWIN_C_SOURCE 1

#include <stddef.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <sys/mman.h>
#ifndef MAP_ANON
#ifdef MAP_ANONYMOUS
#define MAP_ANON MAP_ANONYMOUS
#else
#error "Don't know how to get anonymous memory"
#endif
#endif
#endif

#define SIZxPTR ((int)(sizeof(uintptr_t)*2))







#if defined __i386__ || defined __x86_64__ ||   \
  defined __i386 || defined __x86_64 ||         \
  defined _M_IX86 || defined _M_AMD64
#define RETURN_INSTR 0xC3C3C3C3  /* ret; ret; ret; ret */

#elif defined __arm__ || defined _M_ARM
#define RETURN_INSTR 0xE12FFF1E /* bx lr */



#elif defined _ARCH_PPC || defined _ARCH_PWR || defined _ARCH_PWR2
#define RETURN_INSTR 0x4E800020 /* blr */

#elif defined __sparc || defined __sparcv9
#define RETURN_INSTR 0x81c3e008 /* retl */

#elif defined __alpha
#define RETURN_INSTR 0x6bfa8001 /* ret */

#elif defined __hppa
#define RETURN_INSTR 0xe840c002 /* bv,n r0(rp) */

#elif defined __mips
#define RETURN_INSTR 0x03e00008 /* jr ra */

#ifdef __MIPSEL


#define RETURN_INSTR_TYPE uint64_t
#endif

#elif defined __s390__
#define RETURN_INSTR 0x07fe0000 /* br %r14 */

#elif defined __aarch64__
#define RETURN_INSTR 0xd65f03c0 /* ret */

#elif defined __ia64
struct ia64_instr { uint32_t mI[4]; };
static const ia64_instr _return_instr =
  {{ 0x00000011, 0x00000001, 0x80000200, 0x00840008 }}; 

#define RETURN_INSTR _return_instr
#define RETURN_INSTR_TYPE ia64_instr

#else
#error "Need return instruction for this architecture"
#endif

#ifndef RETURN_INSTR_TYPE
#define RETURN_INSTR_TYPE uint32_t
#endif



#ifdef _WIN32

static LPSTR
StrW32Error(DWORD aErrcode)
{
  LPSTR errmsg;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM |
                 FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, aErrcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPSTR)&errmsg, 0, nullptr);

  
  size_t n = strlen(errmsg)-1;
  while (errmsg[n] == '\r' || errmsg[n] == '\n') {
    n--;
  }
  errmsg[n+1] = '\0';
  return errmsg;
}
#define LastErrMsg() (StrW32Error(GetLastError()))



static SYSTEM_INFO sInfo_;

static inline uint32_t
PageSize()
{
  return sInfo_.dwAllocationGranularity;
}

static void*
ReserveRegion(uintptr_t aRequest, bool aAccessible)
{
  return VirtualAlloc((void*)aRequest, PageSize(),
                      aAccessible ? MEM_RESERVE|MEM_COMMIT : MEM_RESERVE,
                      aAccessible ? PAGE_EXECUTE_READWRITE : PAGE_NOACCESS);
}

static void
ReleaseRegion(void* aPage)
{
  VirtualFree(aPage, PageSize(), MEM_RELEASE);
}

static bool
ProbeRegion(uintptr_t aPage)
{
  return aPage >= (uintptr_t)sInfo_.lpMaximumApplicationAddress &&
         aPage + PageSize() >= (uintptr_t)sInfo_.lpMaximumApplicationAddress;
}

static bool
MakeRegionExecutable(void*)
{
  return false;
}

#undef MAP_FAILED
#define MAP_FAILED 0

#else 

#define LastErrMsg() (strerror(errno))

static unsigned long gUnixPageSize;

static inline unsigned long
PageSize()
{
  return gUnixPageSize;
}

static void*
ReserveRegion(uintptr_t aRequest, bool aAccessible)
{
  return mmap(reinterpret_cast<void*>(aRequest), PageSize(),
              aAccessible ? PROT_READ|PROT_WRITE : PROT_NONE,
              MAP_PRIVATE|MAP_ANON, -1, 0);
}

static void
ReleaseRegion(void* aPage)
{
  munmap(aPage, PageSize());
}

static bool
ProbeRegion(uintptr_t aPage)
{
  return !!madvise(reinterpret_cast<void*>(aPage), PageSize(), MADV_NORMAL);
}

static int
MakeRegionExecutable(void* aPage)
{
  return mprotect((caddr_t)aPage, PageSize(), PROT_READ|PROT_WRITE|PROT_EXEC);
}

#endif

static uintptr_t
ReservePoisonArea()
{
  if (sizeof(uintptr_t) == 8) {
    
    
    
    uintptr_t result = (((uintptr_t(0x7FFFFFFFu) << 31) << 1 |
                         uintptr_t(0xF0DEAFFFu)) &
                        ~uintptr_t(PageSize()-1));
    printf("INFO | poison area assumed at 0x%.*" PRIxPTR "\n", SIZxPTR, result);
    return result;
  }

  
  uintptr_t candidate = (0xF0DEAFFF & ~(PageSize() - 1));
  void* result = ReserveRegion(candidate, false);
  if (result == reinterpret_cast<void*>(candidate)) {
    
    printf("INFO | poison area allocated at 0x%.*" PRIxPTR
           " (preferred addr)\n", SIZxPTR, reinterpret_cast<uintptr_t>(result));
    return candidate;
  }

  
  
  if (ProbeRegion(candidate)) {
    
    if (result != MAP_FAILED) {
      ReleaseRegion(result);
    }
    printf("INFO | poison area assumed at 0x%.*" PRIxPTR
           " (preferred addr)\n", SIZxPTR, candidate);
    return candidate;
  }

  
  
  if (result != MAP_FAILED) {
    uintptr_t ures = reinterpret_cast<uintptr_t>(result);
    printf("INFO | poison area allocated at 0x%.*" PRIxPTR
           " (consolation prize)\n", SIZxPTR, ures);
    return ures;
  }

  
  
  result = ReserveRegion(0, false);
  if (result != MAP_FAILED) {
    uintptr_t ures = reinterpret_cast<uintptr_t>(result);
    printf("INFO | poison area allocated at 0x%.*" PRIxPTR
           " (fallback)\n", SIZxPTR, ures);
    return ures;
  }

  printf("ERROR | no usable poison area found\n");
  return 0;
}




static uintptr_t
ReservePositiveControl()
{

  void* result = ReserveRegion(0, false);
  if (result == MAP_FAILED) {
    printf("ERROR | allocating positive control | %s\n", LastErrMsg());
    return 0;
  }
  printf("INFO | positive control allocated at 0x%.*" PRIxPTR "\n",
         SIZxPTR, (uintptr_t)result);
  return (uintptr_t)result;
}




static uintptr_t
ReserveNegativeControl()
{
  void* result = ReserveRegion(0, true);
  if (result == MAP_FAILED) {
    printf("ERROR | allocating negative control | %s\n", LastErrMsg());
    return 0;
  }

  
  RETURN_INSTR_TYPE* p = reinterpret_cast<RETURN_INSTR_TYPE*>(result);
  RETURN_INSTR_TYPE* limit =
    reinterpret_cast<RETURN_INSTR_TYPE*>(
      reinterpret_cast<char*>(result) + PageSize());
  while (p < limit) {
    *p++ = RETURN_INSTR;
  }

  
  

  if (MakeRegionExecutable(result)) {
    printf("ERROR | making negative control executable | %s\n", LastErrMsg());
    return 0;
  }

  printf("INFO | negative control allocated at 0x%.*" PRIxPTR "\n",
         SIZxPTR, (uintptr_t)result);
  return (uintptr_t)result;
}

static void
JumpTo(uintptr_t aOpaddr)
{
#ifdef __ia64
  struct func_call
  {
    uintptr_t mFunc;
    uintptr_t mGp;
  } call = { aOpaddr, };
  ((void (*)())&call)();
#else
  ((void (*)())aOpaddr)();
#endif
}

#ifdef _WIN32
static BOOL
IsBadExecPtr(uintptr_t aPtr)
{
  BOOL ret = false;

#if defined(_MSC_VER) && !defined(__clang__)
  __try {
    JumpTo(aPtr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    ret = true;
  }
#else
  printf("INFO | exec test not supported on MinGW or clang-cl builds\n");
  
  ret = IsBadReadPtr((const void*)aPtr, 1);
#endif
  return ret;
}
#endif


static bool
TestPage(const char* aPageLabel, uintptr_t aPageAddr, int aShouldSucceed)
{
  const char* oplabel;
  uintptr_t opaddr;

  bool failed = false;
  for (unsigned int test = 0; test < 3; test++) {
    switch (test) {
      
      
    case 0: oplabel = "reading"; opaddr = aPageAddr + PageSize()/2 - 1; break;
    case 1: oplabel = "executing"; opaddr = aPageAddr + PageSize()/2; break;
    case 2: oplabel = "writing"; opaddr = aPageAddr + PageSize()/2 - 1; break;
    default: abort();
    }

#ifdef _WIN32
    BOOL badptr;

    switch (test) {
    case 0: badptr = IsBadReadPtr((const void*)opaddr, 1); break;
    case 1: badptr = IsBadExecPtr(opaddr); break;
    case 2: badptr = IsBadWritePtr((void*)opaddr, 1); break;
    default: abort();
    }

    if (badptr) {
      if (aShouldSucceed) {
        printf("TEST-UNEXPECTED-FAIL | %s %s\n", oplabel, aPageLabel);
        failed = true;
      } else {
        printf("TEST-PASS | %s %s\n", oplabel, aPageLabel);
      }
    } else {
      
      if (aShouldSucceed) {
        printf("TEST-PASS | %s %s\n", oplabel, aPageLabel);
      } else {
        printf("TEST-UNEXPECTED-FAIL | %s %s\n", oplabel, aPageLabel);
        failed = true;
      }
    }
#else
    pid_t pid = fork();
    if (pid == -1) {
      printf("ERROR | %s %s | fork=%s\n", oplabel, aPageLabel,
             LastErrMsg());
      exit(2);
    } else if (pid == 0) {
      volatile unsigned char scratch;
      switch (test) {
      case 0: scratch = *(volatile unsigned char*)opaddr; break;
      case 1: JumpTo(opaddr); break;
      case 2: *(volatile unsigned char*)opaddr = 0; break;
      default: abort();
      }
      (void)scratch;
      _exit(0);
    } else {
      int status;
      if (waitpid(pid, &status, 0) != pid) {
        printf("ERROR | %s %s | wait=%s\n", oplabel, aPageLabel,
               LastErrMsg());
        exit(2);
      }

      if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        if (aShouldSucceed) {
          printf("TEST-PASS | %s %s\n", oplabel, aPageLabel);
        } else {
          printf("TEST-UNEXPECTED-FAIL | %s %s | unexpected successful exit\n",
                 oplabel, aPageLabel);
          failed = true;
        }
      } else if (WIFEXITED(status)) {
        printf("ERROR | %s %s | unexpected exit code %d\n",
               oplabel, aPageLabel, WEXITSTATUS(status));
        exit(2);
      } else if (WIFSIGNALED(status)) {
        if (aShouldSucceed) {
          printf("TEST-UNEXPECTED-FAIL | %s %s | unexpected signal %d\n",
                 oplabel, aPageLabel, WTERMSIG(status));
          failed = true;
        } else {
          printf("TEST-PASS | %s %s | signal %d (as expected)\n",
                 oplabel, aPageLabel, WTERMSIG(status));
        }
      } else {
        printf("ERROR | %s %s | unexpected exit status %d\n",
               oplabel, aPageLabel, status);
        exit(2);
      }
    }
#endif
  }
  return failed;
}

int
main()
{
#ifdef _WIN32
  GetSystemInfo(&sInfo_);
#else
  gUnixPageSize = sysconf(_SC_PAGESIZE);
#endif

  uintptr_t ncontrol = ReserveNegativeControl();
  uintptr_t pcontrol = ReservePositiveControl();
  uintptr_t poison = ReservePoisonArea();

  if (!ncontrol || !pcontrol || !poison) {
    return 2;
  }

  bool failed = false;
  failed |= TestPage("negative control", ncontrol, 1);
  failed |= TestPage("positive control", pcontrol, 0);
  failed |= TestPage("poison area", poison, 0);

  return failed ? 1 : 0;
}
