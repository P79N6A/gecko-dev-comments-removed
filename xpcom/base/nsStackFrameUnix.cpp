






































#include "nsStackFrameUnix.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nscore.h"





#if (__GLIBC_MINOR__ >= 1) && !defined(__USE_GNU)
#define __USE_GNU
#endif

#ifdef HAVE_LIBDL
#include <dlfcn.h>
#endif





#if defined(MOZ_DEMANGLE_SYMBOLS)
#include <cxxabi.h>
#include <stdlib.h> 
#endif 

void DemangleSymbol(const char * aSymbol, 
                    char * aBuffer,
                    int aBufLen)
{
    aBuffer[0] = '\0';

#if defined(MOZ_DEMANGLE_SYMBOLS)
    
    char * demangled = abi::__cxa_demangle(aSymbol,0,0,0);
    
    if (demangled)
    {
        strncpy(aBuffer,demangled,aBufLen);
        free(demangled);
    }
#endif 
}


#if defined(linux) && defined(__GNUC__) && (defined(__i386) || defined(PPC) || defined(__x86_64__)) 


void DumpStackToFile(FILE* aStream)
{
  

  
  void **bp;
#if defined(__i386) 
  __asm__( "movl %%ebp, %0" : "=g"(bp));
#elif defined(__x86_64__)
  __asm__( "movq %%rbp, %0" : "=g"(bp));
#else
  
  
  
  bp = (void**) __builtin_frame_address(0);
#endif

  int skip = 2;
  for ( ; (void**)*bp > bp; bp = (void**)*bp) {
    void *pc = *(bp+1);
    if (--skip <= 0) {
      Dl_info info;
      int ok = dladdr(pc, &info);
      if (!ok) {
        fprintf(aStream, "UNKNOWN %p\n", pc);
        continue;
      }

      PRUint32 foff = (char*)pc - (char*)info.dli_fbase;

      const char * symbol = info.dli_sname;
      int len;
      if (!symbol || !(len = strlen(symbol))) {
        fprintf(aStream, "UNKNOWN [%s +0x%08X]\n",
                info.dli_fname, foff);
        continue;
      }

      char demangled[4096] = "\0";

      DemangleSymbol(symbol, demangled, sizeof(demangled));

      if (strlen(demangled)) {
        symbol = demangled;
        len = strlen(symbol);
      }

      PRUint32 off = (char*)pc - (char*)info.dli_saddr;
      fprintf(aStream, "%s+0x%08X [%s +0x%08X]\n",
              symbol, off, info.dli_fname, foff);
    }
  }
}

#elif defined(__sun) && (defined(__sparc) || defined(sparc) || defined(__i386) || defined(i386))





#include <synch.h>
#include <ucontext.h>
#include <sys/frame.h>
#include <sys/regset.h>
#include <sys/stack.h>

static int    load_address ( void * pc, void * arg, FILE * aStream );
static int    write_address_file ( void * pc );
static struct bucket * newbucket ( void * pc );
static struct frame * cs_getmyframeptr ( void );
static void   cs_walk_stack ( void * (*read_func)(char * address),
                              struct frame * fp,
                              int (*operate_func)(void *, void *),
                              void * usrarg, FILE * aStream );
static void   cs_operate ( void (*operate_func)(void *, void *),
                           void * usrarg, FILE * aStream );

#ifndef STACK_BIAS
#define STACK_BIAS 0
#endif 

#define LOGSIZE 4096


typedef int demf_t(const char *, char *, size_t);

static demf_t *demf;

static int initialized = 0;

#if defined(sparc) || defined(__sparc)
#define FRAME_PTR_REGISTER REG_SP
#endif

#if defined(i386) || defined(__i386)
#define FRAME_PTR_REGISTER EBP
#endif

struct bucket {
    void * pc;
    int index;
    struct bucket * next;
};

struct mybuf {
    char * buffer;
    int chars_left;
};


static void myinit();

#pragma init (myinit)

static void
myinit()
{

    if (! initialized) {
#ifndef __GNUC__
        void *handle;
        const char *libdem = "libdemangle.so.1";

        
        if ((handle = dlopen(libdem, RTLD_LAZY)) != NULL) {
            demf = (demf_t *)dlsym(handle,
                           "cplus_demangle"); 
                



        }
#endif 
    }    
    initialized = 1;
}


static int
write_address_file(void * pc, FILE* aStream)
{
    static struct bucket table[2048];
    static mutex_t lock;
    struct bucket * ptr;

    unsigned int val = NS_PTR_TO_INT32(pc);

    ptr = table + ((val >> 2)&2047);

    mutex_lock(&lock);
    while (ptr->next) {
        if (ptr->next->pc == pc)
            break;
        ptr = ptr->next;
    }

    if (ptr->next) {
        mutex_unlock(&lock);
        return (ptr->next->index);
    } else {
        char buffer[4096], dembuff[4096];
        Dl_info info;
        const char *func = "??", *lib = "??";

        ptr->next = newbucket(pc);
        mutex_unlock(&lock);
 
        if (dladdr(pc, & info)) {
            if (info.dli_fname)
                lib =  info.dli_fname;
            if (info.dli_sname)
                func = info.dli_sname;
        }
 
#ifdef __GNUC__
        DemangleSymbol(func, dembuff, sizeof(dembuff));
#else
        if (!demf || demf(func, dembuff, sizeof (dembuff)))
            dembuff[0] = 0;
#endif 
        if (strlen(dembuff)) {
            func = dembuff;
        }
        fprintf(aStream, "%u %s:%s+0x%x\n",
                ptr->next->index,
                lib,
                func,
                (char *)pc - (char*)info.dli_saddr);
 
        return (ptr->next->index);
    }
}


static int
load_address(void * pc, void * arg, FILE * aStream)
{
    struct mybuf * buf = (struct mybuf *) arg;

    char name[80];
    int len;

    sprintf(name, " %u", write_address_file(pc, aStream));

    len = strlen(name);

    if (len >= buf->chars_left)
        return (1);

    strcat(buf->buffer, name);

    buf->chars_left -= len;

    return (0);
}


static struct bucket *
newbucket(void * pc)
{
    struct bucket * ptr = (struct bucket *) malloc(sizeof (*ptr));
    static int index; 
                     
    ptr->index = index++;
    ptr->next = NULL;
    ptr->pc = pc;    
    return (ptr);    
}


static struct frame *
csgetframeptr()
{
    ucontext_t u;
    struct frame *fp;

    (void) getcontext(&u);

    fp = (struct frame *)
        ((char *)u.uc_mcontext.gregs[FRAME_PTR_REGISTER] +
        STACK_BIAS);

    

    return ((struct frame *)((ulong_t)fp->fr_savfp + STACK_BIAS));
}


static void
cswalkstack(struct frame *fp, int (*operate_func)(void *, void *, FILE *),
    void *usrarg, FILE * aStream)
{

    while (fp != 0 && fp->fr_savpc != 0) {

        if (operate_func((void *)fp->fr_savpc, usrarg, aStream) != 0)
            break;
        




        fp = (struct frame *)((ulong_t)fp->fr_savfp +
            (fp->fr_savfp?(ulong_t)STACK_BIAS:0));
    }
}


static void
cs_operate(int (*operate_func)(void *, void *, FILE *), void * usrarg, FILE *aStream)
{
    cswalkstack(csgetframeptr(), operate_func, usrarg, aStream);
}

void DumpStackToFile(FILE* aStream)
{
    char buffer[LOGSIZE];
    struct mybuf mybuf;

    if (!initialized)
        myinit();

    mybuf.chars_left = LOGSIZE - strlen(buffer)-1;
    mybuf.buffer = buffer;
    cs_operate(load_address, &mybuf, aStream);
}
#endif
