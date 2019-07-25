









































#if defined(linux)
#undef _POSIX_SOURCE
#undef _SVID_SOURCE
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <errno.h>
#if defined(linux)
#include <linux/rtc.h>
#include <pthread.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <execinfo.h>

#include "libmalloc.h"
#include "jprof.h"
#include <string.h>
#include <errno.h>
#include <dlfcn.h>

#include "mozilla/mozalloc_undef_macro_wrappers.h"


void *moz_xmalloc(size_t size)
{
    return malloc(size);
}

void moz_xfree(void *mem)
{
    free(mem);
}

#ifdef NTO
#include <sys/link.h>
extern r_debug _r_debug;
#else
#include <link.h>
#endif

#define USE_GLIBC_BACKTRACE 1

#define JPROF_STATIC

static int gLogFD = -1;
static pthread_t main_thread;

static void startSignalCounter(unsigned long millisec);
static int enableRTCSignals(bool enable);





static void DumpAddressMap();

struct JprofShutdown {
    JprofShutdown() {}
    ~JprofShutdown() {
        DumpAddressMap();
    }
};

static void RegisterJprofShutdown() {
    
    
    
    static JprofShutdown t;
}

#if defined(i386) || defined(_i386) || defined(__x86_64__)
JPROF_STATIC void CrawlStack(malloc_log_entry* me,
                             void* stack_top, void* top_instr_ptr)
{
#if USE_GLIBC_BACKTRACE
    
    
    void *array[500];
    int cnt, i;
    u_long numpcs = 0;
    bool tracing = false;

    
    
    cnt = backtrace(&array[0],sizeof(array)/sizeof(array[0]));

    
    
    array[3] = top_instr_ptr;
    for (i = 3; i < cnt; i++)
    {
        me->pcs[numpcs++] = (char *) array[i];
    }
    me->numpcs = numpcs;

#else
  
  void **bp;
#if defined(__i386)
  __asm__( "movl %%ebp, %0" : "=g"(bp));
#elif defined(__x86_64__)
  __asm__( "movq %%rbp, %0" : "=g"(bp));
#else
  
  
  
  bp = __builtin_frame_address(0);
#endif
  u_long numpcs = 0;
  bool tracing = false;

  me->pcs[numpcs++] = (char*) top_instr_ptr;

  while (numpcs < MAX_STACK_CRAWL) {
    void** nextbp = (void**) *bp++;
    void* pc = *bp;
    if (nextbp < bp) {
      break;
    }
    if (tracing) {
      
      me->pcs[numpcs++] = (char*) pc;
    }
    else if (pc == top_instr_ptr) {
      tracing = true;
    }
    bp = nextbp;
  }
  me->numpcs = numpcs;
#endif
}
#endif



static int rtcHz;
static int rtcFD = -1;
static bool circular = false;

#if defined(linux) || defined(NTO)
static void DumpAddressMap()
{
  
#if defined(linux)
  if (rtcHz) {
    enableRTCSignals(false);
  } else
#endif
  {
    startSignalCounter(0);
  }

  int mfd = open(M_MAPFILE, O_CREAT|O_WRONLY|O_TRUNC, 0666);
  if (mfd >= 0) {
    malloc_map_entry mme;
    link_map* map = _r_debug.r_map;
    while (NULL != map) {
      if (map->l_name && *map->l_name) {
	mme.nameLen = strlen(map->l_name);
	mme.address = map->l_addr;
	write(mfd, &mme, sizeof(mme));
	write(mfd, map->l_name, mme.nameLen);
#if 0
	write(1, map->l_name, mme.nameLen);
	write(1, "\n", 1);
#endif
      }
      map = map->l_next;
    }
    close(mfd);
  }
}
#endif

static bool was_paused = true;

JPROF_STATIC void JprofBufferDump();
JPROF_STATIC void JprofBufferClear();

static void ClearProfilingHook(int signum)
{
    if (circular) {
        JprofBufferClear();
        puts("Jprof: cleared circular buffer.");
    }
}

static void EndProfilingHook(int signum)
{
    if (circular)
        JprofBufferDump();

    DumpAddressMap();
    was_paused = true;
    puts("Jprof: profiling paused.");
}






#if defined(linux)
#define DUMB_LOCK()   pthread_mutex_lock(&mutex);
#define DUMB_UNLOCK() pthread_mutex_unlock(&mutex);
#else
#define DUMB_LOCK()   FIXME()
#define DUMB_UNLOCK() FIXME()
#endif


class DumbCircularBuffer
{
public:
    DumbCircularBuffer(size_t init_buffer_size) {
        used = 0;
        buffer_size = init_buffer_size;
        buffer = (unsigned char *) malloc(buffer_size);
        head = tail = buffer;

#if defined(linux)
        pthread_mutexattr_t mAttr;
        pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE_NP);
        pthread_mutex_init(&mutex, &mAttr);
        pthread_mutexattr_destroy(&mAttr);
#endif
    }
    ~DumbCircularBuffer() {
        free(buffer);
#if defined(linux)
        pthread_mutex_destroy (&mutex);
#endif
    }

    void clear() {
        DUMB_LOCK();
        head = tail;
        used = 0;
        DUMB_UNLOCK();
    }

    bool empty() {
        return head == tail;
    }

    size_t space_available() {
        size_t result;
        DUMB_LOCK();
        if (tail > head)
            result = buffer_size - (tail-head) - 1;
        else
            result = head-tail - 1;
        DUMB_UNLOCK();
        return result;
    }

    void drop(size_t size) {
        
        DUMB_LOCK();
        head += size;
        if (head >= &buffer[buffer_size])
            head -= buffer_size;
        used--;
        DUMB_UNLOCK();
    }

    bool insert(void *data, size_t size) {
        
        DUMB_LOCK();
        if (space_available() < size)
            return false;

        size_t max_without_wrap = &buffer[buffer_size] - tail;
        size_t initial = size > max_without_wrap ? max_without_wrap : size;
#if DEBUG_CIRCULAR
        fprintf(stderr,"insert(%d): max_without_wrap %d, size %d, initial %d\n",used,max_without_wrap,size,initial);
#endif
        memcpy(tail,data,initial);
        tail += initial;
        data = ((char *)data)+initial;
        size -= initial;
        if (size != 0) {
#if DEBUG_CIRCULAR
            fprintf(stderr,"wrapping by %d bytes\n",size);
#endif
            memcpy(buffer,data,size);
            tail = &(((unsigned char *)buffer)[size]);
        }
            
        used++;
        DUMB_UNLOCK();

        return true;
    }

    
    void lock() {
        DUMB_LOCK();
    }

    void unlock() {
        DUMB_UNLOCK();
    }

    
    unsigned char *head;
    unsigned char *tail;
    unsigned int used;
    unsigned char *buffer;
    size_t buffer_size;

private:
    pthread_mutex_t mutex;
};

class DumbCircularBuffer *JprofBuffer;

JPROF_STATIC void
JprofBufferInit(size_t size)
{
    JprofBuffer = new DumbCircularBuffer(size);
}

JPROF_STATIC void
JprofBufferClear()
{
    fprintf(stderr,"Told to clear JPROF circular buffer\n");
    JprofBuffer->clear();
}

JPROF_STATIC size_t
JprofEntrySizeof(malloc_log_entry *me)
{
    return offsetof(malloc_log_entry, pcs) + me->numpcs*sizeof(char*);
}

JPROF_STATIC void
JprofBufferAppend(malloc_log_entry *me)
{
    size_t size = JprofEntrySizeof(me);

    do {
        while (JprofBuffer->space_available() < size &&
               JprofBuffer->used > 0) {
#if DEBUG_CIRCULAR
            fprintf(stderr,"dropping entry: %d in use, %d free, need %d, size_to_free = %d\n",
                    JprofBuffer->used,JprofBuffer->space_available(),size,JprofEntrySizeof((malloc_log_entry *) JprofBuffer->head));
#endif
            JprofBuffer->drop(JprofEntrySizeof((malloc_log_entry *) JprofBuffer->head));
        }
        if (JprofBuffer->space_available() < size)
            return;

    } while (!JprofBuffer->insert(me,size));
}

JPROF_STATIC void
JprofBufferDump()
{
    JprofBuffer->lock();
#if DEBUG_CIRCULAR
    fprintf(stderr,"dumping JP_CIRCULAR buffer, %d of %d bytes\n",
            JprofBuffer->tail > JprofBuffer->head ? 
              JprofBuffer->tail - JprofBuffer->head :
              JprofBuffer->buffer_size + JprofBuffer->tail - JprofBuffer->head,
            JprofBuffer->buffer_size);
#endif
    if (JprofBuffer->tail >= JprofBuffer->head) {
        write(gLogFD, JprofBuffer->head, JprofBuffer->tail - JprofBuffer->head);
    } else {
        write(gLogFD, JprofBuffer->head, &(JprofBuffer->buffer[JprofBuffer->buffer_size]) - JprofBuffer->head);
        write(gLogFD, JprofBuffer->buffer, JprofBuffer->tail - JprofBuffer->buffer);
    }
    JprofBuffer->clear();
    JprofBuffer->unlock();
}



JPROF_STATIC void
JprofLog(u_long aTime, void* stack_top, void* top_instr_ptr)
{
  
  static malloc_log_entry me;

  me.delTime = aTime;
  me.thread = syscall(SYS_gettid); 
  if (was_paused) {
      me.flags = JP_FIRST_AFTER_PAUSE;
      was_paused = 0;
  } else {
      me.flags = 0;
  }

  CrawlStack(&me, stack_top, top_instr_ptr);

#ifndef NTO
  if (circular) {
      JprofBufferAppend(&me);
  } else {
      write(gLogFD, &me, JprofEntrySizeof(&me));
  }
#else
      printf("Neutrino is missing the pcs member of malloc_log_entry!! \n");
#endif
}

static int realTime;






static void startSignalCounter(unsigned long millisec)
{
    struct itimerval tvalue;

    tvalue.it_interval.tv_sec = 0;
    tvalue.it_interval.tv_usec = 0;
    tvalue.it_value.tv_sec = millisec/1000;
    tvalue.it_value.tv_usec = (millisec%1000)*1000;

    if (realTime) {
	setitimer(ITIMER_REAL, &tvalue, NULL);
    } else {
    	setitimer(ITIMER_PROF, &tvalue, NULL);
    }
}

static long timerMiliSec = 50;

#if defined(linux)
static int setupRTCSignals(int hz, struct sigaction *sap)
{
     rtcFD = open("/dev/rtc", O_RDONLY);
    if (rtcFD < 0) {
        perror("JPROF_RTC setup: open(\"/dev/rtc\", O_RDONLY)");
        return 0;
    }

    if (sigaction(SIGIO, sap, NULL) == -1) {
        perror("JPROF_RTC setup: sigaction(SIGIO)");
        return 0;
    }

    if (ioctl(rtcFD, RTC_IRQP_SET, hz) == -1) {
        perror("JPROF_RTC setup: ioctl(/dev/rtc, RTC_IRQP_SET, $JPROF_RTC_HZ)");
        return 0;
    }

    if (ioctl(rtcFD, RTC_PIE_ON, 0) == -1) {
        perror("JPROF_RTC setup: ioctl(/dev/rtc, RTC_PIE_ON)");
        return 0;
    }

    if (fcntl(rtcFD, F_SETSIG, 0) == -1) {
        perror("JPROF_RTC setup: fcntl(/dev/rtc, F_SETSIG, 0)");
        return 0;
    }

    if (fcntl(rtcFD, F_SETOWN, getpid()) == -1) {
        perror("JPROF_RTC setup: fcntl(/dev/rtc, F_SETOWN, getpid())");
        return 0;
    }

    return 1;
}

static int enableRTCSignals(bool enable)
{
    static bool enabled = false;
    if (enabled == enable) {
        return 0;
    }
    enabled = enable;
    
    int flags = fcntl(rtcFD, F_GETFL);
    if (flags < 0) {
        perror("JPROF_RTC setup: fcntl(/dev/rtc, F_GETFL)");
        return 0;
    }

    if (enable) {
        flags |= FASYNC;
    } else {
        flags &= ~FASYNC;
    }

    if (fcntl(rtcFD, F_SETFL, flags) == -1) {
        if (enable) {
            perror("JPROF_RTC setup: fcntl(/dev/rtc, F_SETFL, flags | FASYNC)");
        } else {
            perror("JPROF_RTC setup: fcntl(/dev/rtc, F_SETFL, flags & ~FASYNC)");
        }            
        return 0;
    }

    return 1;
}
#endif

JPROF_STATIC void StackHook(
int signum,
siginfo_t *info,
void *ucontext)
{
    static struct timeval tFirst;
    static int first=1;
    size_t millisec = 0;

#if defined(linux)
    if (rtcHz && pthread_self() != main_thread) {
      
      return;
    }
#endif

    if(first && !(first=0)) {
        puts("Jprof: received first signal");
#if defined(linux)
        if (rtcHz) {
            enableRTCSignals(true);
        } else
#endif
        {
            gettimeofday(&tFirst, 0);
            millisec = 0;
        }
    } else {
#if defined(linux)
        if (rtcHz) {
            enableRTCSignals(true);
        } else
#endif
        {
            struct timeval tNow;
            gettimeofday(&tNow, 0);
            double usec = 1e6*(tNow.tv_sec - tFirst.tv_sec);
            usec += (tNow.tv_usec - tFirst.tv_usec);
            millisec = static_cast<size_t>(usec*1e-3);
        }
    }

    gregset_t &gregs = ((ucontext_t*)ucontext)->uc_mcontext.gregs;
#ifdef __x86_64__
    JprofLog(millisec, (void*)gregs[REG_RSP], (void*)gregs[REG_RIP]);
#else
    JprofLog(millisec, (void*)gregs[REG_ESP], (void*)gregs[REG_EIP]);
#endif

    if (!rtcHz)
        startSignalCounter(timerMiliSec);
}

NS_EXPORT_(void) setupProfilingStuff(void)
{
    static int gFirstTime = 1;
    char filename[2048]; 

    if(gFirstTime && !(gFirstTime=0)) {
	int  startTimer = 1;
	int  doNotStart = 1;
	int  firstDelay = 0;
        int  append = O_TRUNC;
        char *tst  = getenv("JPROF_FLAGS");

	




















        circular = false;

	if(tst) {
	    if(strstr(tst, "JP_DEFER"))
	    {
		doNotStart = 0;
		startTimer = 0;
	    }
	    if(strstr(tst, "JP_START")) doNotStart = 0;
	    if(strstr(tst, "JP_REALTIME")) realTime = 1;
	    if(strstr(tst, "JP_APPEND")) append = O_APPEND;

	    char *delay = strstr(tst,"JP_PERIOD=");
	    if(delay) {
                double tmp = strtod(delay+strlen("JP_PERIOD="), NULL);
                if (tmp>=1e-3) {
		    timerMiliSec = static_cast<unsigned long>(1000 * tmp);
                } else {
                    fprintf(stderr,
                            "JP_PERIOD of %g less than 0.001 (1ms), using 1ms\n",
                            tmp);
                    timerMiliSec = 1;
                }
	    }

	    char *circular_op = strstr(tst,"JP_CIRCULAR=");
	    if(circular_op) {
                size_t size = atol(circular_op+strlen("JP_CIRCULAR="));
                if (size < 1000) {
                    fprintf(stderr,
                            "JP_CIRCULAR of %d less than 1000, using 10000\n",
                            size);
                    size = 10000;
                }
                JprofBufferInit(size);
                fprintf(stderr,"JP_CIRCULAR buffer of %d bytes\n",size);
                circular = true;
	    }

	    char *first = strstr(tst, "JP_FIRST=");
	    if(first) {
                firstDelay = atol(first+strlen("JP_FIRST="));
	    }

            char *rtc = strstr(tst, "JP_RTC_HZ=");
            if (rtc) {
#if defined(linux)
                rtcHz = atol(rtc+strlen("JP_RTC_HZ="));
                timerMiliSec = 0; 
                realTime = 1; 

#define IS_POWER_OF_TWO(x) (((x) & ((x) - 1)) == 0)

                if (!IS_POWER_OF_TWO(rtcHz) || rtcHz < 2) {
                    fprintf(stderr, "JP_RTC_HZ must be power of two and >= 2, "
                            "but %d was provided; using default of 2048\n",
                            rtcHz);
                    rtcHz = 2048;
                }
#else
                fputs("JP_RTC_HZ found, but RTC profiling only supported on "
                      "Linux!\n", stderr);
                  
#endif
            }
            char *f = strstr(tst,"JP_FILENAME=");
            if (f)
                f = f + strlen("JP_FILENAME=");
            else
                f = M_LOGFILE;

            char *is_slave = getenv("JPROF_SLAVE");
            if (!is_slave)
                setenv("JPROF_SLAVE","", 0);

            int pid = syscall(SYS_gettid); 
            if (is_slave)
                snprintf(filename,sizeof(filename),"%s-%d",f,pid);
            else
                snprintf(filename,sizeof(filename),"%s",f);

            
	}

	if(!doNotStart) {

	    if(gLogFD<0) {
		gLogFD = open(filename, O_CREAT | O_WRONLY | append, 0666);
		if(gLogFD<0) {
		    fprintf(stderr, "Unable to create " M_LOGFILE);
		    perror(":");
		} else {
		    struct sigaction action;
		    sigset_t mset;

		    
		    RegisterJprofShutdown();

		    main_thread = pthread_self();
                    
                    

                    
                    
		    sigemptyset(&mset);
		    action.sa_handler = NULL;
		    action.sa_sigaction = StackHook;
		    action.sa_mask  = mset;
		    action.sa_flags = SA_RESTART | SA_SIGINFO;
#if defined(linux)
                    if (rtcHz) {
                        if (!setupRTCSignals(rtcHz, &action)) {
                            fputs("jprof: Error initializing RTC, NOT "
                                  "profiling\n", stderr);
                            return;
                        }
                    }

                    if (!rtcHz || firstDelay != 0)
#endif
                    {
                        if (realTime) {
                            sigaction(SIGALRM, &action, NULL);
                        }
                    }
                    
                    sigaction(SIGPROF, &action, NULL);

		    
		    
		    
		    

		    struct sigaction stop_action;
		    stop_action.sa_handler = EndProfilingHook;
		    stop_action.sa_mask  = mset;
		    stop_action.sa_flags = SA_RESTART;
		    sigaction(SIGUSR1, &stop_action, NULL);

		    

		    stop_action.sa_handler = ClearProfilingHook;
		    stop_action.sa_mask  = mset;
		    stop_action.sa_flags = SA_RESTART;
		    sigaction(SIGUSR2, &stop_action, NULL);

                    printf("Jprof: Initialized signal handler and set "
                           "timer for %lu %s, %d s "
                           "initial delay\n",
                           rtcHz ? rtcHz : timerMiliSec, 
                           rtcHz ? "Hz" : "ms",
                           firstDelay);

		    if(startTimer) {
#if defined(linux)
                        



                        if (rtcHz && firstDelay == 0) {
                            puts("Jprof: enabled RTC signals");
                            enableRTCSignals(true);
                        } else
#endif
                        {
                            puts("Jprof: started timer");
                            startSignalCounter(firstDelay*1000 + timerMiliSec);
                        }
		    }
		}
	    }
	}
    } else {
        printf("setupProfilingStuff() called multiple times\n");
    }
}
