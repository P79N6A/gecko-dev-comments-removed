



























#if defined __linux__ && !defined _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <ffi.h>
#include <ffi_common.h>

#if !FFI_MMAP_EXEC_WRIT && !FFI_EXEC_TRAMPOLINE_TABLE
# if __gnu_linux__ && !defined(__ANDROID__)







#  define FFI_MMAP_EXEC_WRIT 1
#  define HAVE_MNTENT 1
# endif
# if defined(X86_WIN32) || defined(X86_WIN64) || defined(__OS2__)



#  define FFI_MMAP_EXEC_WRIT 1
# endif
#endif

#if FFI_MMAP_EXEC_WRIT && !defined FFI_MMAP_EXEC_SELINUX
# ifdef __linux__



#  define FFI_MMAP_EXEC_SELINUX 1
# endif
#endif

#if FFI_CLOSURES

# if FFI_EXEC_TRAMPOLINE_TABLE



# elif FFI_MMAP_EXEC_WRIT 

#define USE_LOCKS 1
#define USE_DL_PREFIX 1
#ifdef __GNUC__
#ifndef USE_BUILTIN_FFS
#define USE_BUILTIN_FFS 1
#endif
#endif


#define HAVE_MORECORE 0


#define HAVE_MREMAP 0


#define NO_MALLINFO 1



#define DEFAULT_MMAP_THRESHOLD MAX_SIZE_T


#define DEFAULT_GRANULARITY ((size_t)malloc_getpagesize)

#if FFI_CLOSURE_TEST




#define DEFAULT_TRIM_THRESHOLD ((size_t)malloc_getpagesize)
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#if !defined(X86_WIN32) && !defined(X86_WIN64)
#ifdef HAVE_MNTENT
#include <mntent.h>
#endif 
#include <sys/param.h>
#include <pthread.h>



#include <sys/mman.h>
#define LACKS_SYS_MMAN_H 1

#if FFI_MMAP_EXEC_SELINUX
#include <sys/statfs.h>
#include <stdlib.h>

static int selinux_enabled = -1;

static int
selinux_enabled_check (void)
{
  struct statfs sfs;
  FILE *f;
  char *buf = NULL;
  size_t len = 0;

  if (statfs ("/selinux", &sfs) >= 0
      && (unsigned int) sfs.f_type == 0xf97cff8cU)
    return 1;
  f = fopen ("/proc/mounts", "r");
  if (f == NULL)
    return 0;
  while (getline (&buf, &len, f) >= 0)
    {
      char *p = strchr (buf, ' ');
      if (p == NULL)
        break;
      p = strchr (p + 1, ' ');
      if (p == NULL)
        break;
      if (strncmp (p + 1, "selinuxfs ", 10) == 0)
        {
          free (buf);
          fclose (f);
          return 1;
        }
    }
  free (buf);
  fclose (f);
  return 0;
}

#define is_selinux_enabled() (selinux_enabled >= 0 ? selinux_enabled \
			      : (selinux_enabled = selinux_enabled_check ()))

#else

#define is_selinux_enabled() 0

#endif 


#ifdef FFI_MMAP_EXEC_EMUTRAMP_PAX
#include <stdlib.h>

static int emutramp_enabled = -1;

static int
emutramp_enabled_check (void)
{
  char *buf = NULL;
  size_t len = 0;
  FILE *f;
  int ret;
  f = fopen ("/proc/self/status", "r");
  if (f == NULL)
    return 0;
  ret = 0;

  while (getline (&buf, &len, f) != -1)
    if (!strncmp (buf, "PaX:", 4))
      {
        char emutramp;
        if (sscanf (buf, "%*s %*c%c", &emutramp) == 1)
          ret = (emutramp == 'E');
        break;
      }
  free (buf);
  fclose (f);
  return ret;
}

#define is_emutramp_enabled() (emutramp_enabled >= 0 ? emutramp_enabled \
                               : (emutramp_enabled = emutramp_enabled_check ()))
#endif 

#elif defined (__CYGWIN__) || defined(__INTERIX)

#include <sys/mman.h>


#define is_selinux_enabled() 0

#endif 

#ifndef FFI_MMAP_EXEC_EMUTRAMP_PAX
#define is_emutramp_enabled() 0
#endif 


static void *dlmalloc(size_t);
static void dlfree(void*);
static void *dlcalloc(size_t, size_t) MAYBE_UNUSED;
static void *dlrealloc(void *, size_t) MAYBE_UNUSED;
static void *dlmemalign(size_t, size_t) MAYBE_UNUSED;
static void *dlvalloc(size_t) MAYBE_UNUSED;
static int dlmallopt(int, int) MAYBE_UNUSED;
static size_t dlmalloc_footprint(void) MAYBE_UNUSED;
static size_t dlmalloc_max_footprint(void) MAYBE_UNUSED;
static void** dlindependent_calloc(size_t, size_t, void**) MAYBE_UNUSED;
static void** dlindependent_comalloc(size_t, size_t*, void**) MAYBE_UNUSED;
static void *dlpvalloc(size_t) MAYBE_UNUSED;
static int dlmalloc_trim(size_t) MAYBE_UNUSED;
static size_t dlmalloc_usable_size(void*) MAYBE_UNUSED;
static void dlmalloc_stats(void) MAYBE_UNUSED;

#if !(defined(X86_WIN32) || defined(X86_WIN64) || defined(__OS2__)) || defined (__CYGWIN__) || defined(__INTERIX)

static void *dlmmap(void *, size_t, int, int, int, off_t);
static int dlmunmap(void *, size_t);
#endif 

#define mmap dlmmap
#define munmap dlmunmap

#include "dlmalloc.c"

#undef mmap
#undef munmap

#if !(defined(X86_WIN32) || defined(X86_WIN64) || defined(__OS2__)) || defined (__CYGWIN__) || defined(__INTERIX)


static pthread_mutex_t open_temp_exec_file_mutex = PTHREAD_MUTEX_INITIALIZER;



static int execfd = -1;


static size_t execsize = 0;


static int
open_temp_exec_file_name (char *name)
{
  int fd = mkstemp (name);

  if (fd != -1)
    unlink (name);

  return fd;
}


static int
open_temp_exec_file_dir (const char *dir)
{
  static const char suffix[] = "/ffiXXXXXX";
  size_t lendir = strlen (dir);
  char *tempname = __builtin_alloca (lendir + sizeof (suffix));

  if (!tempname)
    return -1;

  memcpy (tempname, dir, lendir);
  memcpy (tempname + lendir, suffix, sizeof (suffix));

  return open_temp_exec_file_name (tempname);
}



static int
open_temp_exec_file_env (const char *envvar)
{
  const char *value = getenv (envvar);

  if (!value)
    return -1;

  return open_temp_exec_file_dir (value);
}

#ifdef HAVE_MNTENT




static int
open_temp_exec_file_mnt (const char *mounts)
{
  static const char *last_mounts;
  static FILE *last_mntent;

  if (mounts != last_mounts)
    {
      if (last_mntent)
	endmntent (last_mntent);

      last_mounts = mounts;

      if (mounts)
	last_mntent = setmntent (mounts, "r");
      else
	last_mntent = NULL;
    }

  if (!last_mntent)
    return -1;

  for (;;)
    {
      int fd;
      struct mntent mnt;
      char buf[MAXPATHLEN * 3];

      if (getmntent_r (last_mntent, &mnt, buf, sizeof (buf)) == NULL)
	return -1;

      if (hasmntopt (&mnt, "ro")
	  || hasmntopt (&mnt, "noexec")
	  || access (mnt.mnt_dir, W_OK))
	continue;

      fd = open_temp_exec_file_dir (mnt.mnt_dir);

      if (fd != -1)
	return fd;
    }
}
#endif 



static struct
{
  int (*func)(const char *);
  const char *arg;
  int repeat;
} open_temp_exec_file_opts[] = {
  { open_temp_exec_file_env, "TMPDIR", 0 },
  { open_temp_exec_file_dir, "/tmp", 0 },
  { open_temp_exec_file_dir, "/var/tmp", 0 },
  { open_temp_exec_file_dir, "/dev/shm", 0 },
  { open_temp_exec_file_env, "HOME", 0 },
#ifdef HAVE_MNTENT
  { open_temp_exec_file_mnt, "/etc/mtab", 1 },
  { open_temp_exec_file_mnt, "/proc/mounts", 1 },
#endif 
};


static int open_temp_exec_file_opts_idx = 0;




static int
open_temp_exec_file_opts_next (void)
{
  if (open_temp_exec_file_opts[open_temp_exec_file_opts_idx].repeat)
    open_temp_exec_file_opts[open_temp_exec_file_opts_idx].func (NULL);

  open_temp_exec_file_opts_idx++;
  if (open_temp_exec_file_opts_idx
      == (sizeof (open_temp_exec_file_opts)
	  / sizeof (*open_temp_exec_file_opts)))
    {
      open_temp_exec_file_opts_idx = 0;
      return 1;
    }

  return 0;
}



static int
open_temp_exec_file (void)
{
  int fd;

  do
    {
      fd = open_temp_exec_file_opts[open_temp_exec_file_opts_idx].func
	(open_temp_exec_file_opts[open_temp_exec_file_opts_idx].arg);

      if (!open_temp_exec_file_opts[open_temp_exec_file_opts_idx].repeat
	  || fd == -1)
	{
	  if (open_temp_exec_file_opts_next ())
	    break;
	}
    }
  while (fd == -1);

  return fd;
}






static void *
dlmmap_locked (void *start, size_t length, int prot, int flags, off_t offset)
{
  void *ptr;

  if (execfd == -1)
    {
      open_temp_exec_file_opts_idx = 0;
    retry_open:
      execfd = open_temp_exec_file ();
      if (execfd == -1)
	return MFAIL;
    }

  offset = execsize;

  if (ftruncate (execfd, offset + length))
    return MFAIL;

  flags &= ~(MAP_PRIVATE | MAP_ANONYMOUS);
  flags |= MAP_SHARED;

  ptr = mmap (NULL, length, (prot & ~PROT_WRITE) | PROT_EXEC,
	      flags, execfd, offset);
  if (ptr == MFAIL)
    {
      if (!offset)
	{
	  close (execfd);
	  goto retry_open;
	}
      ftruncate (execfd, offset);
      return MFAIL;
    }
  else if (!offset
	   && open_temp_exec_file_opts[open_temp_exec_file_opts_idx].repeat)
    open_temp_exec_file_opts_next ();

  start = mmap (start, length, prot, flags, execfd, offset);

  if (start == MFAIL)
    {
      munmap (ptr, length);
      ftruncate (execfd, offset);
      return start;
    }

  mmap_exec_offset ((char *)start, length) = (char*)ptr - (char*)start;

  execsize += length;

  return start;
}



static void *
dlmmap (void *start, size_t length, int prot,
	int flags, int fd, off_t offset)
{
  void *ptr;

  assert (start == NULL && length % malloc_getpagesize == 0
	  && prot == (PROT_READ | PROT_WRITE)
	  && flags == (MAP_PRIVATE | MAP_ANONYMOUS)
	  && fd == -1 && offset == 0);

#if FFI_CLOSURE_TEST
  printf ("mapping in %zi\n", length);
#endif

  if (execfd == -1 && is_emutramp_enabled ())
    {
      ptr = mmap (start, length, prot & ~PROT_EXEC, flags, fd, offset);
      return ptr;
    }

  if (execfd == -1 && !is_selinux_enabled ())
    {
      ptr = mmap (start, length, prot | PROT_EXEC, flags, fd, offset);

      if (ptr != MFAIL || (errno != EPERM && errno != EACCES))
	
	return ptr;

      


    }

  if (execsize == 0 || execfd == -1)
    {
      pthread_mutex_lock (&open_temp_exec_file_mutex);
      ptr = dlmmap_locked (start, length, prot, flags, offset);
      pthread_mutex_unlock (&open_temp_exec_file_mutex);

      return ptr;
    }

  return dlmmap_locked (start, length, prot, flags, offset);
}



static int
dlmunmap (void *start, size_t length)
{
  





  msegmentptr seg = segment_holding (gm, start);
  void *code;

#if FFI_CLOSURE_TEST
  printf ("unmapping %zi\n", length);
#endif

  if (seg && (code = add_segment_exec_offset (start, seg)) != start)
    {
      int ret = munmap (code, length);
      if (ret)
	return ret;
    }

  return munmap (start, length);
}

#if FFI_CLOSURE_FREE_CODE

static msegmentptr
segment_holding_code (mstate m, char* addr)
{
  msegmentptr sp = &m->seg;
  for (;;) {
    if (addr >= add_segment_exec_offset (sp->base, sp)
	&& addr < add_segment_exec_offset (sp->base, sp) + sp->size)
      return sp;
    if ((sp = sp->next) == 0)
      return 0;
  }
}
#endif

#endif 




void *
ffi_closure_alloc (size_t size, void **code)
{
  void *ptr;

  if (!code)
    return NULL;

  ptr = dlmalloc (size);

  if (ptr)
    {
      msegmentptr seg = segment_holding (gm, ptr);

      *code = add_segment_exec_offset (ptr, seg);
    }

  return ptr;
}





void
ffi_closure_free (void *ptr)
{
#if FFI_CLOSURE_FREE_CODE
  msegmentptr seg = segment_holding_code (gm, ptr);

  if (seg)
    ptr = sub_segment_exec_offset (ptr, seg);
#endif

  dlfree (ptr);
}


#if FFI_CLOSURE_TEST


int main ()
{
  void *p[3];
#define GET(idx, len) do { p[idx] = dlmalloc (len); printf ("allocated %zi for p[%i]\n", (len), (idx)); } while (0)
#define PUT(idx) do { printf ("freeing p[%i]\n", (idx)); dlfree (p[idx]); } while (0)
  GET (0, malloc_getpagesize / 2);
  GET (1, 2 * malloc_getpagesize - 64 * sizeof (void*));
  PUT (1);
  GET (1, 2 * malloc_getpagesize);
  GET (2, malloc_getpagesize / 2);
  PUT (1);
  PUT (0);
  PUT (2);
  return 0;
}
#endif 
# else 




#include <stdlib.h>

void *
ffi_closure_alloc (size_t size, void **code)
{
  if (!code)
    return NULL;

  return *code = malloc (size);
}

void
ffi_closure_free (void *ptr)
{
  free (ptr);
}

# endif 
#endif 
