





















































#include "prthread.h"
#include "prgc.h"
#include "prprf.h"
#include "prinrval.h"
#include "prlock.h"
#include "prinit.h"
#include "prcvar.h"

#include "private/pprthred.h"

#include <stdio.h>
#include <memory.h>
#include <string.h>


PRIntn failed_already=0;
PRIntn debug_mode;

static char* progname;
static PRInt32 loops = 1000;
static int tix1, tix2, tix3;
static GCInfo* gcInfo;
static PRLock* stderrLock;

typedef struct Type1 Type1;
typedef struct Type2 Type2;

struct Type1 {
  Type2* atwo;
  Type1* next;
};

struct Type2 {
  void* buf;
};

static void PR_CALLBACK ScanType1(void *obj) {
  gcInfo->livePointer(((Type1 *)obj)->atwo);
  gcInfo->livePointer(((Type1 *)obj)->next);
}

static void PR_CALLBACK ScanType2(void *obj) {
  gcInfo->livePointer(((Type2 *)obj)->buf);
}

static GCType type1 = {
    ScanType1
};

static GCType type2 = {
    ScanType2

};

static GCType type3 = {
  0
};

Type1* NewType1(void) {
    Type1* p = (Type1*) PR_AllocMemory(sizeof(Type1), tix1, PR_ALLOC_DOUBLE);
    PR_ASSERT(p != NULL);
    return p;
}

Type2* NewType2(void) {
    Type2* p = (Type2*) PR_AllocMemory(sizeof(Type2), tix2, PR_ALLOC_DOUBLE);
    PR_ASSERT(p != NULL);
    return p;
}

void* NewBuffer(PRInt32 size) {
    void* p = PR_AllocMemory(size, tix3, PR_ALLOC_DOUBLE);
    PR_ASSERT(p != NULL);
    return p;
}


static void PR_CALLBACK AllocStuff(void *unused) {
  PRInt32 i;
  void* danglingRefs[50];
  PRIntervalTime start, end;
  char msg[100];

  start = PR_IntervalNow();
  for (i = 0; i < loops; i++) {
    void* p;
    if (i & 1) {
      Type1* t1 = NewType1();
      t1->atwo = NewType2();
      t1->next = NewType1();
      t1->atwo->buf = NewBuffer(100);
      p = t1;
    } else {
      Type2* t2 = NewType2();
      t2->buf = NewBuffer(i & 16383);
      p = t2;
    }
    if ((i % 10) == 0) {
      memmove(&danglingRefs[0], &danglingRefs[1], 49*sizeof(void*));
      danglingRefs[49] = p;
    }
  }
  end = PR_IntervalNow();
  if (debug_mode) PR_snprintf(msg, sizeof(msg), "Thread %p: %ld allocations took %ld ms",
	      PR_GetCurrentThread(), loops,
	      PR_IntervalToMilliseconds((PRIntervalTime) (end - start)));
  PR_Lock(stderrLock);
  fprintf(stderr, "%s\n", msg);
  PR_Unlock(stderrLock);
  }

static void usage(char *progname) {
  fprintf(stderr, "Usage: %s [-t threads] [-l loops]\n", progname);
  exit(-1);
}

static int realMain(int argc, char **argv, char *notused) {
  int i;
  int threads = 0;

  progname = strrchr(argv[0], '/');
  if (progname == 0) progname = argv[0];
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-t") == 0) {
      if (i == argc - 1) {
	usage(progname);
      }
      threads = atoi(argv[++i]);
      if (threads < 0) threads = 0;
      if (threads > 10000) threads = 10000;
      continue;
    }
    if (strcmp(argv[i], "-l") == 0) {
      if (i == argc - 1) {
	usage(progname);
      }
      loops = atoi(argv[++i]);
      continue;
    }
    usage(progname);
  }

  for (i = 0; i < threads; i++) {
    PRThread* thread;

    
    thread = PR_CreateThreadGCAble(PR_USER_THREAD,  
			     AllocStuff,  
			     NULL,  
			     PR_PRIORITY_NORMAL,  
			     PR_LOCAL_THREAD,  
			     PR_UNJOINABLE_THREAD,  
			     0);   
    if (thread == 0) {
      fprintf(stderr, "%s: no more threads (only %d were created)\n",
	      progname, i);
      break;
    }
  }
  AllocStuff(NULL);
  return 0;
}

static int padMain(int argc, char **argv) {
  char pad[512];
  return realMain(argc, argv, pad);
}

int main(int argc, char **argv) {
  int rv;

  debug_mode = 1;
  
  PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
  PR_SetThreadGCAble();

  PR_InitGC(0, 0, 0, PR_GLOBAL_THREAD);
  PR_STDIO_INIT();
  stderrLock = PR_NewLock();
  tix1 = PR_RegisterType(&type1);
  tix2 = PR_RegisterType(&type2);
  tix3 = PR_RegisterType(&type3);
  gcInfo = PR_GetGCInfo();
  rv = padMain(argc, argv);
  printf("PASS\n");
  PR_Cleanup();
  return rv;
}
