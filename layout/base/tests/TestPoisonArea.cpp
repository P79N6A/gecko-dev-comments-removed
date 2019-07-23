


















































































































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

#else
#error "Need return instruction for this architecture"
#endif



#ifdef _WIN32

static LPSTR
StrW32Error(DWORD errcode)
{
  LPSTR errmsg;

#ifndef WINCE
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM |
                 FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPSTR) &errmsg, 0, NULL);

  
  size_t n = strlen(errmsg)-1;
  while (errmsg[n] == '\r' || errmsg[n] == '\n') n--;
  errmsg[n+1] = '\0';
#else
  
  
  
  errmsg = (LPSTR)LocalAlloc(LMEM_FIXED, 16);
  _snprintf(errmsg, 16, "code %u", errcode);
#endif

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

  
  uint32_t *p = (uint32_t *)result;
  uint32_t *limit = (uint32_t *)(((char *)result) + PAGESIZE);
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
    __try {
      unsigned char scratch;
      switch (test) {
      case 0: scratch = *(volatile unsigned char *)opaddr; break;
      case 1: ((void (*)())opaddr)(); break;
      case 2: *(volatile unsigned char *)opaddr = 0; break;
      default: abort();
      }

      
      if (should_succeed) {
        printf("TEST-PASS | %s %s\n", oplabel, pagelabel);
      } else {
        printf("TEST-UNEXPECTED-FAIL | %s %s\n", oplabel, pagelabel);
        failed = true;
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      
      DWORD code = GetExceptionCode();
      if (should_succeed) {
        printf("TEST-UNEXPECTED-FAIL | %s %s | exception code %x\n",
               oplabel, pagelabel, code);
        failed = true;
      } else {
        printf("TEST-PASS | %s %s | exception code %x\n",
               oplabel, pagelabel, code);
      }
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
      case 1: ((void (*)())opaddr)(); break;
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
          printf("TEST-UNEXPECTED-FAIL | %s %s\n", oplabel, pagelabel);
          failed = true;
        }
      } else if (WIFEXITED(status)) {
        printf("ERROR | %s %s | unexpected exit code %d\n",
               oplabel, pagelabel, WEXITSTATUS(status));
        exit(2);
      } else if (WIFSIGNALED(status)) {
        if (should_succeed) {
          printf("TEST-UNEXPECTED-FAIL | %s %s | %s\n",
                 oplabel, pagelabel, strsignal(WTERMSIG(status)));
          failed = true;
        } else {
          printf("TEST-PASS | %s %s | %s\n",
                 oplabel, pagelabel, strsignal(WTERMSIG(status)));
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
#else
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
