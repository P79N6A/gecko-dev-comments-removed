


















































































































#define __STDC_FORMAT_MACROS
#define _GNU_SOURCE 1
#define _DARWIN_C_SOURCE 1

#include <stddef.h>

#ifndef _WIN32
#include <inttypes.h>
#else
#define PRIxPTR "Ix"
typedef unsigned int uint32_t;

#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__OS2__)
#include <sys/types.h>
#include <unistd.h>
#include <setjmp.h>
#define INCL_DOS
#include <os2.h>
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

#elif defined __ia64
struct ia64_instr { uint32_t i[4]; };
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
StrW32Error(DWORD errcode)
{
  LPSTR errmsg;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM |
                 FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPSTR) &errmsg, 0, NULL);

  
  size_t n = strlen(errmsg)-1;
  while (errmsg[n] == '\r' || errmsg[n] == '\n') n--;
  errmsg[n+1] = '\0';
  return errmsg;
}
#define LastErrMsg() (StrW32Error(GetLastError()))



static SYSTEM_INFO _sinfo;
#undef PAGESIZE
#define PAGESIZE (_sinfo.dwAllocationGranularity)


static void *
ReserveRegion(uintptr_t request, bool accessible)
{
  return VirtualAlloc((void *)request, PAGESIZE,
                      accessible ? MEM_RESERVE|MEM_COMMIT : MEM_RESERVE,
                      accessible ? PAGE_EXECUTE_READWRITE : PAGE_NOACCESS);
}

static void
ReleaseRegion(void *page)
{
  VirtualFree(page, PAGESIZE, MEM_RELEASE);
}

static bool
ProbeRegion(uintptr_t page)
{
  if (page >= (uintptr_t)_sinfo.lpMaximumApplicationAddress &&
      page + PAGESIZE >= (uintptr_t)_sinfo.lpMaximumApplicationAddress) {
    return true;
  } else {
    return false;
  }
}

static bool
MakeRegionExecutable(void *)
{
  return false;
}

#undef MAP_FAILED
#define MAP_FAILED 0

#elif defined(__OS2__)


#undef PAGESIZE
#define PAGESIZE 0x1000
static unsigned long rc = 0;

char * LastErrMsg()
{
  char * errmsg = (char *)malloc(16);
  sprintf(errmsg, "rc= %ld", rc);
  rc = 0;
  return errmsg;
}

static void *
ReserveRegion(uintptr_t request, bool accessible)
{
  
  
  if (request) {
    return (void*)0xFFFD0000;
  }
  void * mem = 0;
  rc = DosAllocMem(&mem, PAGESIZE,
                   (accessible ? PAG_COMMIT : 0) | PAG_READ | PAG_WRITE);
  return rc ? 0 : mem;
}

static void
ReleaseRegion(void *page)
{
  return;
}

static bool
ProbeRegion(uintptr_t page)
{
  
  
  return false;
}

static bool
MakeRegionExecutable(void *page)
{
  rc = DosSetMem(page, PAGESIZE, PAG_READ | PAG_WRITE | PAG_EXECUTE);
  return rc ? true : false;
}

typedef struct _XCPT {
  EXCEPTIONREGISTRATIONRECORD regrec;
  jmp_buf                     jmpbuf;
} XCPT;

static unsigned long _System
ExceptionHandler(PEXCEPTIONREPORTRECORD pReport,
                 PEXCEPTIONREGISTRATIONRECORD pRegRec,
                 PCONTEXTRECORD pContext, PVOID pVoid)
{
  if (pReport->fHandlerFlags == 0) {
    longjmp(((XCPT*)pRegRec)->jmpbuf, pReport->ExceptionNum);
  }
  return XCPT_CONTINUE_SEARCH;
}

#undef MAP_FAILED
#define MAP_FAILED 0

#else 

#define LastErrMsg() (strerror(errno))

static unsigned long _pagesize;
#define PAGESIZE _pagesize

static void *
ReserveRegion(uintptr_t request, bool accessible)
{
  return mmap((caddr_t)request, PAGESIZE,
              accessible ? PROT_READ|PROT_WRITE : PROT_NONE,
              MAP_PRIVATE|MAP_ANON, -1, 0);
}

static void
ReleaseRegion(void *page)
{
  munmap((caddr_t)page, PAGESIZE);
}

static bool
ProbeRegion(uintptr_t page)
{
  if (madvise((caddr_t)page, PAGESIZE, MADV_NORMAL)) {
    return true;
  } else {
    return false;
  }
}

static int
MakeRegionExecutable(void *page)
{
  return mprotect((caddr_t)page, PAGESIZE, PROT_READ|PROT_WRITE|PROT_EXEC);
}

#endif

static uintptr_t
ReservePoisonArea()
{
  if (sizeof(uintptr_t) == 8) {
    
    
    
    uintptr_t result = (((uintptr_t(0x7FFFFFFFu) << 31) << 1 |
                         uintptr_t(0xF0DEAFFFu)) &
                        ~uintptr_t(PAGESIZE-1));
    printf("INFO | poison area assumed at 0x%.*"PRIxPTR"\n", SIZxPTR, result);
    return result;
  } else {
    
    uintptr_t candidate = (0xF0DEAFFF & ~(PAGESIZE-1));
    void *result = ReserveRegion(candidate, false);
    if (result == (void *)candidate) {
      
      printf("INFO | poison area allocated at 0x%.*"PRIxPTR
             " (preferred addr)\n", SIZxPTR, (uintptr_t)result);
      return candidate;
    }

    
    
    if (ProbeRegion(candidate)) {
      
      if (result != MAP_FAILED)
        ReleaseRegion(result);
      printf("INFO | poison area assumed at 0x%.*"PRIxPTR
             " (preferred addr)\n", SIZxPTR, candidate);
      return candidate;
    }

    
    
    if (result != MAP_FAILED) {
      printf("INFO | poison area allocated at 0x%.*"PRIxPTR
             " (consolation prize)\n", SIZxPTR, (uintptr_t)result);
      return (uintptr_t)result;
    }

    
    
    result = ReserveRegion(0, false);
    if (result != MAP_FAILED) {
      printf("INFO | poison area allocated at 0x%.*"PRIxPTR
             " (fallback)\n", SIZxPTR, (uintptr_t)result);
      return (uintptr_t)result;
    }

    printf("ERROR | no usable poison area found\n");
    return 0;
  }
}




static uintptr_t
ReservePositiveControl()
{

  void *result = ReserveRegion(0, false);
  if (result == MAP_FAILED) {
    printf("ERROR | allocating positive control | %s\n", LastErrMsg());
    return 0;
  }
  printf("INFO | positive control allocated at 0x%.*"PRIxPTR"\n",
         SIZxPTR, (uintptr_t)result);
  return (uintptr_t)result;
}




static uintptr_t
ReserveNegativeControl()
{
  void *result = ReserveRegion(0, true);
  if (result == MAP_FAILED) {
    printf("ERROR | allocating negative control | %s\n", LastErrMsg());
    return 0;
  }

  
  RETURN_INSTR_TYPE *p = (RETURN_INSTR_TYPE *)result;
  RETURN_INSTR_TYPE *limit = (RETURN_INSTR_TYPE *)(((char *)result) + PAGESIZE);
  while (p < limit)
    *p++ = RETURN_INSTR;

  
  

  if (MakeRegionExecutable(result)) {
    printf("ERROR | making negative control executable | %s\n", LastErrMsg());
    return 0;
  }

  printf("INFO | negative control allocated at 0x%.*"PRIxPTR"\n",
         SIZxPTR, (uintptr_t)result);
  return (uintptr_t)result;
}

static void
JumpTo(uintptr_t opaddr)
{
#ifdef __ia64
  struct func_call {
    uintptr_t func;
    uintptr_t gp;
  } call = { opaddr, };
  ((void (*)())&call)();
#else
  ((void (*)())opaddr)();
#endif
}

#ifdef _WIN32
static BOOL
IsBadExecPtr(uintptr_t ptr)
{
  BOOL ret = false;

#ifdef _MSC_VER
  __try {
    JumpTo(ptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    ret = true;
  }
#else
  printf("INFO | exec test not supported on MinGW build\n");
  
  ret = IsBadReadPtr((const void*)ptr, 1);
#endif
  return ret;
}
#endif


static bool
TestPage(const char *pagelabel, uintptr_t pageaddr, int should_succeed)
{
  const char *oplabel;
  uintptr_t opaddr;

  bool failed = false;
  for (unsigned int test = 0; test < 3; test++) {
    switch (test) {
      
      
    case 0: oplabel = "reading"; opaddr = pageaddr + PAGESIZE/2 - 1; break;
    case 1: oplabel = "executing"; opaddr = pageaddr + PAGESIZE/2; break;
    case 2: oplabel = "writing"; opaddr = pageaddr + PAGESIZE/2 - 1; break;
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
      if (should_succeed) {
        printf("TEST-UNEXPECTED-FAIL | %s %s\n", oplabel, pagelabel);
        failed = true;
      } else {
        printf("TEST-PASS | %s %s\n", oplabel, pagelabel);
      }
    } else {
      
      if (should_succeed) {
        printf("TEST-PASS | %s %s\n", oplabel, pagelabel);
      } else {
        printf("TEST-UNEXPECTED-FAIL | %s %s\n", oplabel, pagelabel);
        failed = true;
      }
    }
#elif defined(__OS2__)
    XCPT xcpt;
    volatile int code = setjmp(xcpt.jmpbuf);

    if (!code) {
      xcpt.regrec.prev_structure = 0;
      xcpt.regrec.ExceptionHandler = ExceptionHandler;
      DosSetExceptionHandler(&xcpt.regrec);
      unsigned char scratch;
      switch (test) {
        case 0: scratch = *(volatile unsigned char *)opaddr; break;
        case 1: ((void (*)())opaddr)(); break;
        case 2: *(volatile unsigned char *)opaddr = 0; break;
        default: abort();
      }
    }

    if (code) {
      if (should_succeed) {
        printf("TEST-UNEXPECTED-FAIL | %s %s | exception code %x\n",
               oplabel, pagelabel, code);
        failed = true;
      } else {
        printf("TEST-PASS | %s %s | exception code %x\n",
               oplabel, pagelabel, code);
      }
    } else {
      if (should_succeed) {
        printf("TEST-PASS | %s %s\n", oplabel, pagelabel);
      } else {
        printf("TEST-UNEXPECTED-FAIL | %s %s\n", oplabel, pagelabel);
        failed = true;
      }
      DosUnsetExceptionHandler(&xcpt.regrec);
    }
#else
    pid_t pid = fork();
    if (pid == -1) {
      printf("ERROR | %s %s | fork=%s\n", oplabel, pagelabel,
             LastErrMsg());
      exit(2);
    } else if (pid == 0) {
      volatile unsigned char scratch;
      switch (test) {
      case 0: scratch = *(volatile unsigned char *)opaddr; break;
      case 1: JumpTo(opaddr); break;
      case 2: *(volatile unsigned char *)opaddr = 0; break;
      default: abort();
      }
      _exit(0);
    } else {
      int status;
      if (waitpid(pid, &status, 0) != pid) {
        printf("ERROR | %s %s | wait=%s\n", oplabel, pagelabel,
               LastErrMsg());
        exit(2);
      }

      if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        if (should_succeed) {
          printf("TEST-PASS | %s %s\n", oplabel, pagelabel);
        } else {
          printf("TEST-UNEXPECTED-FAIL | %s %s | unexpected successful exit\n",
                 oplabel, pagelabel);
          failed = true;
        }
      } else if (WIFEXITED(status)) {
        printf("ERROR | %s %s | unexpected exit code %d\n",
               oplabel, pagelabel, WEXITSTATUS(status));
        exit(2);
      } else if (WIFSIGNALED(status)) {
        if (should_succeed) {
          printf("TEST-UNEXPECTED-FAIL | %s %s | unexpected signal %d\n",
                 oplabel, pagelabel, WTERMSIG(status));
          failed = true;
        } else {
          printf("TEST-PASS | %s %s | signal %d (as expected)\n",
                 oplabel, pagelabel, WTERMSIG(status));
        }
      } else {
        printf("ERROR | %s %s | unexpected exit status %d\n",
               oplabel, pagelabel, status);
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
  GetSystemInfo(&_sinfo);
#elif !defined(__OS2__)
  _pagesize = sysconf(_SC_PAGESIZE);
#endif

  uintptr_t ncontrol = ReserveNegativeControl();
  uintptr_t pcontrol = ReservePositiveControl();
  uintptr_t poison = ReservePoisonArea();

  if (!ncontrol || !pcontrol || !poison)
    return 2;

  bool failed = false;
  failed |= TestPage("negative control", ncontrol, 1);
  failed |= TestPage("positive control", pcontrol, 0);
  failed |= TestPage("poison area", poison, 0);

  return failed ? 1 : 0;
}
