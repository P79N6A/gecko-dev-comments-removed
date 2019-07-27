




#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef MOZ_VALGRIND
# include <valgrind/helgrind.h>
# include <valgrind/memcheck.h>
#else
# define VALGRIND_HG_MUTEX_LOCK_PRE(_mx,_istry)
# define VALGRIND_HG_MUTEX_LOCK_POST(_mx)
# define VALGRIND_HG_MUTEX_UNLOCK_PRE(_mx)
# define VALGRIND_HG_MUTEX_UNLOCK_POST(_mx)
# define VALGRIND_MAKE_MEM_DEFINED(_addr,_len)   ((void)0)
# define VALGRIND_MAKE_MEM_UNDEFINED(_addr,_len) ((void)0)
#endif

#include "prenv.h"
#include "mozilla/arm.h"
#include "mozilla/DebugOnly.h"
#include <stdint.h>
#include "PlatformMacros.h"

#include "platform.h"
#include <ostream>
#include <string>

#include "ProfileEntry.h"
#include "SyncProfile.h"
#include "AutoObjectMapper.h"
#include "UnwinderThread2.h"

#if !defined(SPS_OS_windows)
# include <sys/mman.h>
#endif

#if defined(SPS_OS_android) || defined(SPS_OS_linux)
# include <ucontext.h>
# include "LulMain.h"
#endif

#include "shared-libraries.h"










#define LOGLEVEL 2




#define MAX_NATIVE_FRAMES 256



#if defined(SPS_OS_windows) || defined(SPS_OS_darwin)








void uwt__init()
{
}

void uwt__stop()
{
}

void uwt__deinit()
{
}

void uwt__register_thread_for_profiling ( void* stackTop )
{
}

void uwt__unregister_thread_for_profiling()
{
}

LinkedUWTBuffer* utb__acquire_sync_buffer(void* stackTop)
{
  return nullptr;
}


UnwinderThreadBuffer* uwt__acquire_empty_buffer()
{
  return nullptr;
}

void
utb__finish_sync_buffer(ThreadProfile* aProfile,
                        UnwinderThreadBuffer* utb,
                        void*  ucV)
{
}

void
utb__release_sync_buffer(LinkedUWTBuffer* utb)
{
}

void
utb__end_sync_buffer_unwind(LinkedUWTBuffer* utb)
{
}


void
uwt__release_full_buffer(ThreadProfile* aProfile,
                         UnwinderThreadBuffer* utb,
                         void*  ucV )
{
}


void
utb__addEntry(UnwinderThreadBuffer* utb, ProfileEntry ent)
{
}




#else 






static void* unwind_thr_fn ( void* exit_nowV );
static pthread_t unwind_thr;
static int       unwind_thr_exit_now = 0; 




static void thread_register_for_profiling ( void* stackTop );


static void thread_unregister_for_profiling();



static void empty_buffer_queue();


static LinkedUWTBuffer* acquire_sync_buffer(void* stackTop);



static UnwinderThreadBuffer* acquire_empty_buffer();

static void finish_sync_buffer(ThreadProfile* aProfile,
                               UnwinderThreadBuffer* utb,
                               void*  ucV);


static void release_sync_buffer(LinkedUWTBuffer* utb);


static void end_sync_buffer_unwind(LinkedUWTBuffer* utb);









static void release_full_buffer(ThreadProfile* aProfile,
                                UnwinderThreadBuffer* utb,
                                void*  ucV );


static void utb_add_prof_ent(UnwinderThreadBuffer* utb, ProfileEntry ent);


static void do_MBAR();















static pthread_mutex_t sLULmutex = PTHREAD_MUTEX_INITIALIZER;
static lul::LUL*       sLUL      = nullptr;
static int             sLULcount = 0;


void uwt__init()
{
  
  MOZ_ASSERT(unwind_thr_exit_now == 0);
  int r = pthread_create( &unwind_thr, nullptr,
                          unwind_thr_fn, (void*)&unwind_thr_exit_now );
  MOZ_ALWAYS_TRUE(r == 0);
}

void uwt__stop()
{
  
  MOZ_ASSERT(unwind_thr_exit_now == 0);
  unwind_thr_exit_now = 1;
  do_MBAR();
  int r = pthread_join(unwind_thr, nullptr);
  MOZ_ALWAYS_TRUE(r == 0);
}

void uwt__deinit()
{
  empty_buffer_queue();
}

void uwt__register_thread_for_profiling(void* stackTop)
{
  thread_register_for_profiling(stackTop);
}

void uwt__unregister_thread_for_profiling()
{
  thread_unregister_for_profiling();
}

LinkedUWTBuffer* utb__acquire_sync_buffer(void* stackTop)
{
  return acquire_sync_buffer(stackTop);
}

void utb__finish_sync_buffer(ThreadProfile* profile,
                             UnwinderThreadBuffer* buff,
                             void*  ucV)
{
  finish_sync_buffer(profile, buff, ucV);
}

void utb__release_sync_buffer(LinkedUWTBuffer* buff)
{
  release_sync_buffer(buff);
}

void
utb__end_sync_buffer_unwind(LinkedUWTBuffer* utb)
{
  end_sync_buffer_unwind(utb);
}


UnwinderThreadBuffer* uwt__acquire_empty_buffer()
{
  return acquire_empty_buffer();
}


void
uwt__release_full_buffer(ThreadProfile* aProfile,
                         UnwinderThreadBuffer* utb,
                         void*  ucV )
{
  release_full_buffer( aProfile, utb, ucV );
}


void
utb__addEntry(UnwinderThreadBuffer* utb, ProfileEntry ent)
{
  utb_add_prof_ent(utb, ent);
}








static_assert(sizeof(uint32_t) == 4, "uint32_t size incorrect");
static_assert(sizeof(uint64_t) == 8, "uint64_t size incorrect");
static_assert(sizeof(uintptr_t) == sizeof(void*),
              "uintptr_t size incorrect");

typedef
  struct { 
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip; 
  }
  AMD64Regs;

typedef
  struct {
    uint32_t r15;
    uint32_t r14;
    uint32_t r13;
    uint32_t r12;
    uint32_t r11;
    uint32_t r7;
  }
  ARMRegs;

typedef
  struct {
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
  }
  X86Regs;

#if defined(SPS_ARCH_amd64)
typedef  AMD64Regs  ArchRegs;
#elif defined(SPS_ARCH_arm)
typedef  ARMRegs  ArchRegs;
#elif defined(SPS_ARCH_x86)
typedef  X86Regs  ArchRegs;
#else
# error "Unknown plat"
#endif

#if defined(SPS_ARCH_amd64) || defined(SPS_ARCH_arm) || defined(SPS_ARCH_x86)
# define SPS_PAGE_SIZE 4096
#else
# error "Unknown plat"
#endif

typedef  enum { S_EMPTY, S_FILLING, S_EMPTYING, S_FULL }  State;

typedef  struct { uintptr_t val; }  SpinLock;




#define N_FIXED_PROF_ENTS 20





#define N_PROF_ENT_PAGES 100


#define N_PROF_ENTS_PER_PAGE (SPS_PAGE_SIZE / sizeof(ProfileEntry))




typedef
  struct { ProfileEntry ents[N_PROF_ENTS_PER_PAGE]; }
  ProfEntsPage;

#define ProfEntsPage_INVALID ((ProfEntsPage*)1)




struct _UnwinderThreadBuffer {
   State  state;
  






  
  uint64_t       seqNo;
  

  ThreadProfile* aProfile;
  
  ProfileEntry   entsFixed[N_FIXED_PROF_ENTS];
  ProfEntsPage*  entsPages[N_PROF_ENT_PAGES];
  uintptr_t      entsUsed;
  
  bool           haveNativeInfo;
  

  lul::UnwindRegs startRegs;
  lul::StackImage stackImg;
  void* stackMaxSafe; 
};



















typedef
  struct {
    pthread_t thrId;
    void*     stackTop;
    uint64_t  nSamples; 
  }
  StackLimit;


#define N_UNW_THR_BUFFERS 10
 static UnwinderThreadBuffer** g_buffers     = nullptr;
 static uint64_t               g_seqNo       = 0;
 static SpinLock               g_spinLock    = { 0 };








 static StackLimit* g_stackLimits     = nullptr;
 static size_t      g_stackLimitsUsed = 0;
 static size_t      g_stackLimitsSize = 0;


static uintptr_t g_stats_totalSamples = 0; 
static uintptr_t g_stats_noBuffAvail  = 0; 
static uintptr_t g_stats_thrUnregd    = 0; 
















typedef  struct { uint64_t pc; uint64_t sp; }  PCandSP;


static
void do_lul_unwind_Buffer(PCandSP** pairs,
                          unsigned int* nPairs,
                          UnwinderThreadBuffer* buff,
                          int buffNo );

static bool is_page_aligned(void* v)
{
  uintptr_t w = (uintptr_t) v;
  return (w & (SPS_PAGE_SIZE-1)) == 0  ? true  : false;
}




static bool do_CASW(uintptr_t* addr, uintptr_t expected, uintptr_t nyu)
{
#if defined(__GNUC__)
  return __sync_bool_compare_and_swap(addr, expected, nyu);
#else
# error "Unhandled compiler"
#endif
}





static void do_SPINLOOP_RELAX()
{
#if (defined(SPS_ARCH_amd64) || defined(SPS_ARCH_x86)) && defined(__GNUC__)
  __asm__ __volatile__("rep; nop");
#elif defined(SPS_PLAT_arm_android) && MOZILLA_ARM_ARCH >= 7
  __asm__ __volatile__("wfe");
#endif
}


static void do_SPINLOOP_NUDGE()
{
#if (defined(SPS_ARCH_amd64) || defined(SPS_ARCH_x86)) && defined(__GNUC__)
  
#elif defined(SPS_PLAT_arm_android) && MOZILLA_ARM_ARCH >= 7
  __asm__ __volatile__("sev");
#endif
}


static void do_MBAR()
{
#if defined(__GNUC__)
  __sync_synchronize();
#else
# error "Unhandled compiler"
#endif
}

static void spinLock_acquire(SpinLock* sl)
{
  uintptr_t* val = &sl->val;
  VALGRIND_HG_MUTEX_LOCK_PRE(sl, 0);
  while (1) {
    bool ok = do_CASW( val, 0, 1 );
    if (ok) break;
    do_SPINLOOP_RELAX();
  }
  do_MBAR();
  VALGRIND_HG_MUTEX_LOCK_POST(sl);
}

static void spinLock_release(SpinLock* sl)
{
  uintptr_t* val = &sl->val;
  VALGRIND_HG_MUTEX_UNLOCK_PRE(sl);
  do_MBAR();
  bool ok = do_CASW( val, 1, 0 );
  

  MOZ_ALWAYS_TRUE(ok);
  do_SPINLOOP_NUDGE();
  VALGRIND_HG_MUTEX_UNLOCK_POST(sl);
}

static void sleep_ms(unsigned int ms)
{
  struct timespec req;
  req.tv_sec = ((time_t)ms) / 1000;
  req.tv_nsec = 1000 * 1000 * (((unsigned long)ms) % 1000);
  nanosleep(&req, nullptr);
}


static void atomic_INC(uintptr_t* loc)
{
  while (1) {
    uintptr_t old = *loc;
    uintptr_t nyu = old + 1;
    bool ok = do_CASW( loc, old, nyu );
    if (ok) break;
  }
}


static void empty_buffer_queue()
{
  spinLock_acquire(&g_spinLock);

  UnwinderThreadBuffer** tmp_g_buffers = g_buffers;
  g_stackLimitsUsed = 0;
  g_seqNo = 0;
  g_buffers = nullptr;

  spinLock_release(&g_spinLock);

  
  free(tmp_g_buffers);

  
  
  
}




static void thread_register_for_profiling(void* stackTop)
{
  pthread_t me = pthread_self();

  spinLock_acquire(&g_spinLock);

  
  int n_used;

  
  if (stackTop == nullptr) {
    n_used = g_stackLimitsUsed;
    spinLock_release(&g_spinLock);
    LOGF("BPUnw: [%d total] thread_register_for_profiling"
         "(me=%p, stacktop=NULL) (IGNORED)", n_used, (void*)me);
    return;
  }

  
  MOZ_ASSERT((void*)&n_used < stackTop);

  bool is_dup = false;
  for (size_t i = 0; i < g_stackLimitsUsed; i++) {
    if (g_stackLimits[i].thrId == me) {
      is_dup = true;
      break;
    }
  }

  if (is_dup) {
    

    n_used = g_stackLimitsUsed;
    spinLock_release(&g_spinLock);

    LOGF("BPUnw: [%d total] thread_register_for_profiling"
         "(me=%p, stacktop=%p) (DUPLICATE)", n_used, (void*)me, stackTop);
    return;
  }

  


























  MOZ_ASSERT(g_stackLimitsUsed <= g_stackLimitsSize);

  if (g_stackLimitsUsed == g_stackLimitsSize) {
    

    size_t old_size = g_stackLimitsSize;
    size_t new_size = old_size == 0 ? 4 : (2 * old_size);

    spinLock_release(&g_spinLock);
    StackLimit* new_arr  = (StackLimit*)malloc(new_size * sizeof(StackLimit));
    if (!new_arr)
      return;

    spinLock_acquire(&g_spinLock);

    if (old_size != g_stackLimitsSize) {
      


      spinLock_release(&g_spinLock);
      free(new_arr);
      thread_register_for_profiling(stackTop);
      return;
    }

    memcpy(new_arr, g_stackLimits, old_size * sizeof(StackLimit));
    if (g_stackLimits)
      free(g_stackLimits);

    g_stackLimits = new_arr;

    MOZ_ASSERT(g_stackLimitsSize < new_size);
    g_stackLimitsSize = new_size;
  }

  MOZ_ASSERT(g_stackLimitsUsed < g_stackLimitsSize);

  

  
  
  
  
  uintptr_t stackTopR = (uintptr_t)stackTop;
  stackTopR = (stackTopR & ~(uintptr_t)4095) + (uintptr_t)4095;

  g_stackLimits[g_stackLimitsUsed].thrId    = me;
  g_stackLimits[g_stackLimitsUsed].stackTop = (void*)stackTopR;
  g_stackLimits[g_stackLimitsUsed].nSamples = 0;
  g_stackLimitsUsed++;

  n_used = g_stackLimitsUsed;
  spinLock_release(&g_spinLock);

  LOGF("BPUnw: [%d total] thread_register_for_profiling"
       "(me=%p, stacktop=%p)", n_used, (void*)me, stackTop);
}



static void thread_unregister_for_profiling()
{
  spinLock_acquire(&g_spinLock);

  
  size_t n_used;

  size_t i;
  bool found = false;
  pthread_t me = pthread_self();
  for (i = 0; i < g_stackLimitsUsed; i++) {
    if (g_stackLimits[i].thrId == me)
      break;
  }
  if (i < g_stackLimitsUsed) {
    
    for (; i+1 < g_stackLimitsUsed; i++) {
      g_stackLimits[i] = g_stackLimits[i+1];
    }
    g_stackLimitsUsed--;
    found = true;
  }

  n_used = g_stackLimitsUsed;

  spinLock_release(&g_spinLock);
  LOGF("BPUnw: [%d total] thread_unregister_for_profiling(me=%p) %s", 
       (int)n_used, (void*)me, found ? "" : " (NOT REGISTERED) ");
}


__attribute__((unused))
static void show_registered_threads()
{
  size_t i;
  spinLock_acquire(&g_spinLock);
  for (i = 0; i < g_stackLimitsUsed; i++) {
    LOGF("[%d]  pthread_t=%p  nSamples=%lld",
         (int)i, (void*)g_stackLimits[i].thrId, 
                 (unsigned long long int)g_stackLimits[i].nSamples);
  }
  spinLock_release(&g_spinLock);
}




static void init_empty_buffer(UnwinderThreadBuffer* buff, void* stackTop)
{
  
  buff->aProfile            = nullptr;
  buff->entsUsed            = 0;
  buff->haveNativeInfo      = false;
  buff->stackImg.mLen       = 0;
  buff->stackImg.mStartAvma = 0;
  buff->stackMaxSafe        = stackTop; 

  for (size_t i = 0; i < N_PROF_ENT_PAGES; i++)
    buff->entsPages[i] = ProfEntsPage_INVALID;
}

struct SyncUnwinderThreadBuffer : public LinkedUWTBuffer
{
  UnwinderThreadBuffer* GetBuffer()
  {
    return &mBuff;
  }
  
  UnwinderThreadBuffer  mBuff;
};

static LinkedUWTBuffer* acquire_sync_buffer(void* stackTop)
{
  MOZ_ASSERT(stackTop);
  SyncUnwinderThreadBuffer* buff = new SyncUnwinderThreadBuffer();
  
  
  buff->GetBuffer()->state = S_FILLING;
  init_empty_buffer(buff->GetBuffer(), stackTop);
  return buff;
}


static UnwinderThreadBuffer* acquire_empty_buffer()
{
  






  size_t i;

  atomic_INC( &g_stats_totalSamples );

  







  spinLock_acquire(&g_spinLock);

  




  pthread_t me = pthread_self();
  MOZ_ASSERT(g_stackLimitsUsed <= g_stackLimitsSize);
  for (i = 0; i < g_stackLimitsUsed; i++) {
    if (g_stackLimits[i].thrId == me)
      break;
  }

  

  if (i == g_stackLimitsUsed) {
    spinLock_release(&g_spinLock);
    atomic_INC( &g_stats_thrUnregd );
    return nullptr;
  }

  
  MOZ_ASSERT(i < g_stackLimitsUsed);

  
  void* myStackTop = g_stackLimits[i].stackTop;
  g_stackLimits[i].nSamples++;

  
  if (g_buffers == nullptr) {
    

    spinLock_release(&g_spinLock);
    atomic_INC( &g_stats_noBuffAvail );
    return nullptr;
  }

  for (i = 0; i < N_UNW_THR_BUFFERS; i++) {
    if (g_buffers[i]->state == S_EMPTY)
      break;
  }
  MOZ_ASSERT(i <= N_UNW_THR_BUFFERS);

  if (i == N_UNW_THR_BUFFERS) {
    
    spinLock_release(&g_spinLock);
    atomic_INC( &g_stats_noBuffAvail );
    if (LOGLEVEL >= 3)
      LOG("BPUnw: handler:  no free buffers");
    return nullptr;
  }

  


  UnwinderThreadBuffer* buff = g_buffers[i];
  MOZ_ASSERT(buff->state == S_EMPTY);
  buff->state = S_FILLING;
  buff->seqNo = g_seqNo;
  g_seqNo++;

  
  spinLock_release(&g_spinLock);

  
  init_empty_buffer(buff, myStackTop);
  return buff;
}




static void fill_buffer(ThreadProfile* aProfile,
                        UnwinderThreadBuffer* buff,
                        void*  ucV)
{
  MOZ_ASSERT(buff->state == S_FILLING);

  
  

  

  
  
  buff->aProfile = aProfile;

  
  buff->haveNativeInfo = ucV != nullptr;
  if (buff->haveNativeInfo) {
#   if defined(SPS_PLAT_amd64_linux)
    ucontext_t* uc = (ucontext_t*)ucV;
    mcontext_t* mc = &(uc->uc_mcontext);
    buff->startRegs.xip = lul::TaggedUWord(mc->gregs[REG_RIP]);
    buff->startRegs.xsp = lul::TaggedUWord(mc->gregs[REG_RSP]);
    buff->startRegs.xbp = lul::TaggedUWord(mc->gregs[REG_RBP]);
#   elif defined(SPS_PLAT_amd64_darwin)
    ucontext_t* uc = (ucontext_t*)ucV;
    struct __darwin_mcontext64* mc = uc->uc_mcontext;
    struct __darwin_x86_thread_state64* ss = &mc->__ss;
    buff->regs.rip = ss->__rip;
    buff->regs.rsp = ss->__rsp;
    buff->regs.rbp = ss->__rbp;
#   elif defined(SPS_PLAT_arm_android)
    ucontext_t* uc = (ucontext_t*)ucV;
    mcontext_t* mc = &(uc->uc_mcontext);
    buff->startRegs.r15 = lul::TaggedUWord(mc->arm_pc);
    buff->startRegs.r14 = lul::TaggedUWord(mc->arm_lr);
    buff->startRegs.r13 = lul::TaggedUWord(mc->arm_sp);
    buff->startRegs.r12 = lul::TaggedUWord(mc->arm_ip);
    buff->startRegs.r11 = lul::TaggedUWord(mc->arm_fp);
    buff->startRegs.r7  = lul::TaggedUWord(mc->arm_r7);
#   elif defined(SPS_PLAT_x86_linux) || defined(SPS_PLAT_x86_android)
    ucontext_t* uc = (ucontext_t*)ucV;
    mcontext_t* mc = &(uc->uc_mcontext);
    buff->startRegs.xip = lul::TaggedUWord(mc->gregs[REG_EIP]);
    buff->startRegs.xsp = lul::TaggedUWord(mc->gregs[REG_ESP]);
    buff->startRegs.xbp = lul::TaggedUWord(mc->gregs[REG_EBP]);
#   elif defined(SPS_PLAT_x86_darwin)
    ucontext_t* uc = (ucontext_t*)ucV;
    struct __darwin_mcontext32* mc = uc->uc_mcontext;
    struct __darwin_i386_thread_state* ss = &mc->__ss;
    buff->regs.eip = ss->__eip;
    buff->regs.esp = ss->__esp;
    buff->regs.ebp = ss->__ebp;
#   else
#     error "Unknown plat"
#   endif

    




    { 
#     if defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_amd64_darwin)
      uintptr_t rEDZONE_SIZE = 128;
      uintptr_t start = buff->startRegs.xsp.Value() - rEDZONE_SIZE;
#     elif defined(SPS_PLAT_arm_android)
      uintptr_t rEDZONE_SIZE = 0;
      uintptr_t start = buff->startRegs.r13.Value() - rEDZONE_SIZE;
#     elif defined(SPS_PLAT_x86_linux) || defined(SPS_PLAT_x86_darwin) \
           || defined(SPS_PLAT_x86_android)
      uintptr_t rEDZONE_SIZE = 0;
      uintptr_t start = buff->startRegs.xsp.Value() - rEDZONE_SIZE;
#     else
#       error "Unknown plat"
#     endif
      uintptr_t end   = (uintptr_t)buff->stackMaxSafe;
      uintptr_t ws    = sizeof(void*);
      start &= ~(ws-1);
      end   &= ~(ws-1);
      uintptr_t nToCopy = 0;
      if (start < end) {
        nToCopy = end - start;
        if (nToCopy > lul::N_STACK_BYTES)
          nToCopy = lul::N_STACK_BYTES;
      }
      MOZ_ASSERT(nToCopy <= lul::N_STACK_BYTES);
      buff->stackImg.mLen       = nToCopy;
      buff->stackImg.mStartAvma = start;
      if (nToCopy > 0) {
        memcpy(&buff->stackImg.mContents[0], (void*)start, nToCopy);
        (void)VALGRIND_MAKE_MEM_DEFINED(&buff->stackImg.mContents[0], nToCopy);
      }
    }
  } 
  
  
}




static void release_full_buffer(ThreadProfile* aProfile,
                                UnwinderThreadBuffer* buff,
                                void*  ucV )
{
  fill_buffer(aProfile, buff, ucV);
  

  spinLock_acquire(&g_spinLock);
  buff->state = S_FULL;
  spinLock_release(&g_spinLock);
}




static ProfEntsPage* mmap_anon_ProfEntsPage()
{
# if defined(SPS_OS_darwin)
  void* v = ::mmap(nullptr, sizeof(ProfEntsPage), PROT_READ | PROT_WRITE, 
                   MAP_PRIVATE | MAP_ANON,      -1, 0);
# else
  void* v = ::mmap(nullptr, sizeof(ProfEntsPage), PROT_READ | PROT_WRITE, 
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
# endif
  if (v == MAP_FAILED) {
    return ProfEntsPage_INVALID;
  } else {
    return (ProfEntsPage*)v;
  }
}



static void munmap_ProfEntsPage(ProfEntsPage* pep)
{
  MOZ_ALWAYS_TRUE(is_page_aligned(pep));
  ::munmap(pep, sizeof(ProfEntsPage));
}



void
utb_add_prof_ent(UnwinderThreadBuffer* utb, ProfileEntry ent)
{
  uintptr_t limit
    = N_FIXED_PROF_ENTS + (N_PROF_ENTS_PER_PAGE * N_PROF_ENT_PAGES);
  if (utb->entsUsed == limit) {
    
    LOG("BPUnw: utb__addEntry: NO SPACE for ProfileEntry; ignoring.");
    return;
  }
  MOZ_ASSERT(utb->entsUsed < limit);

  
  if (utb->entsUsed < N_FIXED_PROF_ENTS) {
    utb->entsFixed[utb->entsUsed] = ent;
    utb->entsUsed++;
    return;
  }

  
  uintptr_t i     = utb->entsUsed;
  uintptr_t j     = i - N_FIXED_PROF_ENTS;
  uintptr_t j_div = j / N_PROF_ENTS_PER_PAGE; 
  uintptr_t j_mod = j % N_PROF_ENTS_PER_PAGE; 
  ProfEntsPage* pep = utb->entsPages[j_div];
  if (pep == ProfEntsPage_INVALID) {
    pep = mmap_anon_ProfEntsPage();
    if (pep == ProfEntsPage_INVALID) {
      
      LOG("BPUnw: utb__addEntry: MMAP FAILED for ProfileEntry; ignoring.");
      return;
    }
    utb->entsPages[j_div] = pep;
  }
  pep->ents[j_mod] = ent;
  utb->entsUsed++;
}



static ProfileEntry utb_get_profent(UnwinderThreadBuffer* buff, uintptr_t i)
{
  MOZ_ASSERT(i < buff->entsUsed);
  if (i < N_FIXED_PROF_ENTS) {
    return buff->entsFixed[i];
  } else {
    uintptr_t j     = i - N_FIXED_PROF_ENTS;
    uintptr_t j_div = j / N_PROF_ENTS_PER_PAGE; 
    uintptr_t j_mod = j % N_PROF_ENTS_PER_PAGE; 
    MOZ_ASSERT(buff->entsPages[j_div] != ProfEntsPage_INVALID);
    return buff->entsPages[j_div]->ents[j_mod];
  }
}


static void process_buffer(UnwinderThreadBuffer* buff, int oldest_ix);

static void process_sync_buffer(ProfileEntry& ent)
{
  UnwinderThreadBuffer* buff = (UnwinderThreadBuffer*)ent.get_tagPtr();
  buff->state = S_EMPTYING;
  process_buffer(buff, -1);
}





static void process_buffer(UnwinderThreadBuffer* buff, int oldest_ix)
{
  

  buff->aProfile->BeginUnwind();

  






















  


  const uintptr_t infUW = ~(uintptr_t)0; 
  bool  need_native_unw = false;
  uintptr_t ix_first_hP = infUW; 
  uintptr_t ix_last_hQ  = infUW; 

  uintptr_t k;
  for (k = 0; k < buff->entsUsed; k++) {
    ProfileEntry ent = utb_get_profent(buff, k);
    if (ent.is_ent_hint('N')) {
      need_native_unw = true;
    }
    else if (ent.is_ent_hint('P') && ix_first_hP == ~(uintptr_t)0) {
      ix_first_hP = k;
    }
    else if (ent.is_ent_hint('Q')) {
      ix_last_hQ = k;
    }
  }

  if (0) LOGF("BPUnw: ix_first_hP %llu  ix_last_hQ %llu  need_native_unw %llu",
              (unsigned long long int)ix_first_hP,
              (unsigned long long int)ix_last_hQ,
              (unsigned long long int)need_native_unw);

  


  MOZ_ASSERT( (ix_first_hP == infUW && ix_last_hQ == infUW) ||
              (ix_first_hP != infUW && ix_last_hQ != infUW) );
  bool have_P = ix_first_hP != infUW;
  if (have_P) {
    MOZ_ASSERT(ix_first_hP < ix_last_hQ);
    MOZ_ASSERT(ix_last_hQ <= buff->entsUsed);
  }

  

  if (!need_native_unw && !have_P) {
    for (k = 0; k < buff->entsUsed; k++) {
      ProfileEntry ent = utb_get_profent(buff, k);
      
      if (ent.is_ent_hint('F')) { continue; }
      
      if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
      
      if (ent.is_ent('B')) {
        process_sync_buffer(ent);
        continue;
      }
      
      buff->aProfile->addTag( ent );
    }
  }
  else 
  if (need_native_unw && !have_P) {
    for (k = 0; k < buff->entsUsed; k++) {
      ProfileEntry ent = utb_get_profent(buff, k);
      
      if (ent.is_ent_hint('N')) {
        MOZ_ASSERT(buff->haveNativeInfo);
        PCandSP* pairs = nullptr;
        unsigned int nPairs = 0;
        do_lul_unwind_Buffer(&pairs, &nPairs, buff, oldest_ix);
        buff->aProfile->addTag( ProfileEntry('s', "(root)") );
        for (unsigned int i = 0; i < nPairs; i++) {
          


          if (pairs[i].pc == 0 && pairs[i].sp == 0)
            continue;
          buff->aProfile
              ->addTag( ProfileEntry('l', reinterpret_cast<void*>(pairs[i].pc)) );
        }
        if (pairs)
          free(pairs);
        continue;
      }
      
      if (ent.is_ent_hint('F')) { continue; }
      
      if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
      
      if (ent.is_ent('B')) {
        process_sync_buffer(ent);
        continue;
      }
      
      buff->aProfile->addTag( ent );
    }
  }
  else 
  if (!need_native_unw && have_P) {
    




    for (k = 0; k < buff->entsUsed; k++) {
      ProfileEntry ent = utb_get_profent(buff, k);
      
      if (k == ix_first_hP) {
        buff->aProfile->addTag( ProfileEntry('s', "(root)") );
      }
      
      if (ent.is_ent_hint('F')) { continue; }
      
      if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
      
      if (ent.is_ent('B')) {
        process_sync_buffer(ent);
        continue;
      }
      
      buff->aProfile->addTag( ent );
    }
  }
  else 
  if (need_native_unw && have_P)
  {
    





    MOZ_ASSERT(buff->haveNativeInfo);

    
    PCandSP* pairs = nullptr;
    unsigned int n_pairs = 0;
    do_lul_unwind_Buffer(&pairs, &n_pairs, buff, oldest_ix);

    
    for (k = 0; k < ix_first_hP; k++) {
      ProfileEntry ent = utb_get_profent(buff, k);
      
      if (ent.is_ent_hint('F')) { continue; }
      
      if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
      
      if (ent.is_ent('B')) {
        process_sync_buffer(ent);
        continue;
      }
      
      buff->aProfile->addTag( ent );
    }

    
    buff->aProfile->addTag( ProfileEntry('s', "(root)") );
    unsigned int next_N = 0; 
    unsigned int next_P = ix_first_hP; 
    bool last_was_P = false;
    if (0) LOGF("at mergeloop: n_pairs %llu ix_last_hQ %llu",
                (unsigned long long int)n_pairs,
                (unsigned long long int)ix_last_hQ);
    


    while (next_N < n_pairs && pairs[next_N].pc == 0 && pairs[next_N].sp == 0)
      next_N++;

    while (true) {
      if (next_P <= ix_last_hQ) {
        
        MOZ_ASSERT(utb_get_profent(buff, next_P).is_ent_hint('P'));
      }
      if (next_N >= n_pairs && next_P > ix_last_hQ) {
        
        break;
      }
      







      bool use_P = true;
      if (next_N >= n_pairs) {
        
        use_P = true;
        if (0) LOG("  P  <=  no remaining N entries");
      }
      else if (next_P > ix_last_hQ) {
        
        use_P = false;
        if (0) LOG("  N  <=  no remaining P entries");
      }
      else {
        
        
        uint64_t sp_cur_P = 0;
        unsigned int m = next_P + 1;
        while (1) {
          


          MOZ_ASSERT(m < buff->entsUsed);
          ProfileEntry ent = utb_get_profent(buff, m);
          if (ent.is_ent_hint('Q'))
            break;
          if (ent.is_ent('S')) {
            sp_cur_P = reinterpret_cast<uint64_t>(ent.get_tagPtr());
            break;
          }
          m++;
        }
        if (last_was_P && sp_cur_P == 0) {
          if (0) LOG("  P  <=  last_was_P && sp_cur_P == 0");
          use_P = true;
        } else {
          uint64_t sp_cur_N = pairs[next_N].sp;
          use_P = (sp_cur_P > sp_cur_N);
          if (0) LOGF("  %s  <=  sps P %p N %p",
                      use_P ? "P" : "N", (void*)(intptr_t)sp_cur_P, 
                                         (void*)(intptr_t)sp_cur_N);
        }
      }
      
      if (use_P) {
        unsigned int m = next_P + 1;
        while (true) {
          MOZ_ASSERT(m < buff->entsUsed);
          ProfileEntry ent = utb_get_profent(buff, m);
          if (ent.is_ent_hint('Q')) {
            next_P = m + 1;
            break;
          }
          
          MOZ_ASSERT(!ent.is_ent_hint('F'));
          
          if (ent.is_ent_hint() || ent.is_ent('S')) { m++; continue; }
          
          buff->aProfile->addTag( ent );
          m++;
        }
      } else {
        buff->aProfile
            ->addTag( ProfileEntry('l', reinterpret_cast<void*>(pairs[next_N].pc)) );
        next_N++;
      }
      
      last_was_P = use_P;
    }

    MOZ_ASSERT(next_P == ix_last_hQ + 1);
    MOZ_ASSERT(next_N == n_pairs);
    

    
    for (k = ix_last_hQ+1; k < buff->entsUsed; k++) {
      ProfileEntry ent = utb_get_profent(buff, k);
      
      if (ent.is_ent_hint('F')) { continue; }
      
      if (ent.is_ent_hint() || ent.is_ent('S')) { continue; }
      
      buff->aProfile->addTag( ent );
    }

    
    if (pairs)
      free(pairs);
  }

#if 0
  bool show = true;
  if (show) LOG("----------------");
  for (k = 0; k < buff->entsUsed; k++) {
    ProfileEntry ent = utb_get_profent(buff, k);
    if (show) ent.log();
    if (ent.is_ent_hint('N')) {
      
      MOZ_ASSERT(buff->haveNativeInfo);
      PCandSP* pairs = nullptr;
      unsigned int nPairs = 0;
      do_lul_unwind_Buffer(&pairs, &nPairs, buff, oldest_ix);
      buff->aProfile->addTag( ProfileEntry('s', "(root)") );
      for (unsigned int i = 0; i < nPairs; i++) {
        buff->aProfile
            ->addTag( ProfileEntry('l', reinterpret_cast<void*>(pairs[i].pc)) );
      }
      if (pairs)
        free(pairs);
    } else {
      
      buff->aProfile->addTag( ent );
    }
  }
#endif

  buff->aProfile->EndUnwind();
}





void
read_procmaps(lul::LUL* aLUL)
{
  MOZ_ASSERT(aLUL->CountMappings() == 0);

# if defined(SPS_OS_linux) || defined(SPS_OS_android) || defined(SPS_OS_darwin)
  SharedLibraryInfo info = SharedLibraryInfo::GetInfoForSelf();

  for (size_t i = 0; i < info.GetSize(); i++) {
    const SharedLibrary& lib = info.GetEntry(i);

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
    
    AutoObjectMapperFaultyLib mapper(aLUL->mLog);
#else
    
    AutoObjectMapperPOSIX mapper(aLUL->mLog);
#endif

    
    
    void*  image = nullptr;
    size_t size  = 0;
    bool ok = mapper.Map(&image, &size, lib.GetName());
    if (ok && image && size > 0) {
      aLUL->NotifyAfterMap(lib.GetStart(), lib.GetEnd()-lib.GetStart(),
                           lib.GetName().c_str(), image);
    } else if (!ok && lib.GetName() == "") {
      
      
      
      
      
      
      
      
      
      
      aLUL->NotifyExecutableArea(lib.GetStart(), lib.GetEnd()-lib.GetStart());
    }

    
    
  }

# else
#  error "Unknown platform"
# endif
}


static void
logging_sink_for_LUL(const char* str) {
  
  size_t n = strlen(str);
  if (n > 0 && str[n-1] == '\n') {
    char* tmp = strdup(str);
    tmp[n-1] = 0;
    LOG(tmp);
    free(tmp);
  } else {
    LOG(str);
  }
}


static void* unwind_thr_fn(void* exit_nowV)
{
  
  
  
  
  
  
  
  
  
  
  LOG("unwind_thr_fn: START");

  
  
  
  
  
  bool doLulTest = false;

  mozilla::DebugOnly<int> r = pthread_mutex_lock(&sLULmutex);
  MOZ_ASSERT(!r);

  if (!sLUL) {
    
    sLUL = new lul::LUL(logging_sink_for_LUL);
    MOZ_ASSERT(sLUL);
    MOZ_ASSERT(sLULcount == 0);
    
    sLUL->RegisterUnwinderThread();
    
    read_procmaps(sLUL);
    
    if (PR_GetEnv("MOZ_PROFILER_LUL_TEST")) {
      doLulTest = true;
    }
  } else {
    
    
    MOZ_ASSERT(sLULcount > 0);
    
    sLUL->RegisterUnwinderThread();
  }

  sLULcount++;

  r = pthread_mutex_unlock(&sLULmutex);
  MOZ_ASSERT(!r);

  
  
  
  
  
  if (doLulTest) {
    int nTests = 0, nTestsPassed = 0;
    RunLulUnitTests(&nTests, &nTestsPassed, sLUL);
  }

  
  
  
  

  
  

  
  
  spinLock_acquire(&g_spinLock);
  if (g_buffers == nullptr) {
    
    
    
    spinLock_release(&g_spinLock);
    UnwinderThreadBuffer** buffers
      = (UnwinderThreadBuffer**)malloc(N_UNW_THR_BUFFERS
                                        * sizeof(UnwinderThreadBuffer*));
    MOZ_ASSERT(buffers);
    int i;
    for (i = 0; i < N_UNW_THR_BUFFERS; i++) {
      


      buffers[i] = (UnwinderThreadBuffer*)
                   calloc(sizeof(UnwinderThreadBuffer), 1);
      MOZ_ASSERT(buffers[i]);
      buffers[i]->state = S_EMPTY;
    }
    
    spinLock_acquire(&g_spinLock);
    if (g_buffers == nullptr) {
      g_buffers = buffers;
      spinLock_release(&g_spinLock);
    } else {
      

      spinLock_release(&g_spinLock);
      for (i = 0; i < N_UNW_THR_BUFFERS; i++) {
        free(buffers[i]);
      }
      free(buffers);
    }
  } else {
    
    spinLock_release(&g_spinLock);
  }

  














  int* exit_now = (int*)exit_nowV;
  int ms_to_sleep_if_empty = 1;

  const int longest_sleep_ms = 1000;
  bool show_sleep_message = true;

  while (1) {

    if (*exit_now != 0) {
      *exit_now = 0;
      break;
    }

    spinLock_acquire(&g_spinLock);

    
    uint64_t oldest_seqNo = ~0ULL; 
    int      oldest_ix    = -1;
    int      i;
    for (i = 0; i < N_UNW_THR_BUFFERS; i++) {
      UnwinderThreadBuffer* buff = g_buffers[i];
      if (buff->state != S_FULL) continue;
      if (buff->seqNo < oldest_seqNo) {
        oldest_seqNo = buff->seqNo;
        oldest_ix    = i;
      }
    }
    if (oldest_ix == -1) {
      
      MOZ_ASSERT(oldest_seqNo == ~0ULL);
      spinLock_release(&g_spinLock);
      if (ms_to_sleep_if_empty > 100 && LOGLEVEL >= 2) {
        if (show_sleep_message)
          LOGF("BPUnw: unwinder: sleep for %d ms", ms_to_sleep_if_empty);
        


        if (ms_to_sleep_if_empty == longest_sleep_ms)
          show_sleep_message = false;
      }
      sleep_ms(ms_to_sleep_if_empty);
      if (ms_to_sleep_if_empty < 20) {
        ms_to_sleep_if_empty += 2;
      } else {
        ms_to_sleep_if_empty = (15 * ms_to_sleep_if_empty) / 10;
        if (ms_to_sleep_if_empty > longest_sleep_ms)
          ms_to_sleep_if_empty = longest_sleep_ms;
      }
      continue;
    }

    

    UnwinderThreadBuffer* buff = g_buffers[oldest_ix];
    MOZ_ASSERT(buff->state == S_FULL);
    buff->state = S_EMPTYING;
    spinLock_release(&g_spinLock);

    



    if (0) LOGF("BPUnw: unwinder: seqNo %llu: emptying buf %d\n",
                (unsigned long long int)oldest_seqNo, oldest_ix);

    process_buffer(buff, oldest_ix);

    


    for (i = 0; i < N_PROF_ENT_PAGES; i++) {
      if (buff->entsPages[i] == ProfEntsPage_INVALID)
        continue;
      munmap_ProfEntsPage(buff->entsPages[i]);
      buff->entsPages[i] = ProfEntsPage_INVALID;
    }

    (void)VALGRIND_MAKE_MEM_UNDEFINED(&buff->stackImg.mContents[0],
                                      lul::N_STACK_BYTES);
    spinLock_acquire(&g_spinLock);
    MOZ_ASSERT(buff->state == S_EMPTYING);
    buff->state = S_EMPTY;
    spinLock_release(&g_spinLock);
    ms_to_sleep_if_empty = 1;
    show_sleep_message = true;
  }

  
  
  r = pthread_mutex_lock(&sLULmutex);
  MOZ_ASSERT(!r);

  MOZ_ASSERT(sLULcount > 0);
  if (sLULcount == 1) {
    
    
    sLUL->NotifyBeforeUnmapAll();

    delete sLUL;
    sLUL = nullptr;
  }

  sLULcount--;

  r = pthread_mutex_unlock(&sLULmutex);
  MOZ_ASSERT(!r);

  LOG("unwind_thr_fn: STOP");
  return nullptr;
}

static void finish_sync_buffer(ThreadProfile* profile,
                               UnwinderThreadBuffer* buff,
                               void*  ucV)
{
  SyncProfile* syncProfile = profile->AsSyncProfile();
  MOZ_ASSERT(syncProfile);
  SyncUnwinderThreadBuffer* utb = static_cast<SyncUnwinderThreadBuffer*>(
                                                   syncProfile->GetUWTBuffer());
  fill_buffer(profile, utb->GetBuffer(), ucV);
  utb->GetBuffer()->state = S_FULL;
  PseudoStack* stack = profile->GetPseudoStack();
  stack->addLinkedUWTBuffer(utb);
}

static void release_sync_buffer(LinkedUWTBuffer* buff)
{
  SyncUnwinderThreadBuffer* data = static_cast<SyncUnwinderThreadBuffer*>(buff);
  
  MOZ_ASSERT(data->GetBuffer()->state == S_EMPTY ||
             data->GetBuffer()->state == S_FILLING);
  delete data;
}

static void end_sync_buffer_unwind(LinkedUWTBuffer* buff)
{
  SyncUnwinderThreadBuffer* data = static_cast<SyncUnwinderThreadBuffer*>(buff);
  data->GetBuffer()->state = S_EMPTY;
}










static void stats_notify_frame(int n_context, int n_cfi, int n_scanned)
{
  
  static unsigned int nf_total    = 0; 
  static unsigned int nf_CONTEXT  = 0;
  static unsigned int nf_CFI      = 0;
  static unsigned int nf_SCANNED  = 0;

  nf_CONTEXT += n_context;
  nf_CFI     += n_cfi;
  nf_SCANNED += n_scanned;
  nf_total   += (n_context + n_cfi + n_scanned);

  if (nf_total >= 5000) {
    LOGF("BPUnw frame stats: TOTAL %5u"
         "    CTX %4u    CFI %4u    SCAN %4u",
         nf_total, nf_CONTEXT, nf_CFI, nf_SCANNED);
    nf_total    = 0;
    nf_CONTEXT  = 0;
    nf_CFI      = 0;
    nf_SCANNED  = 0;
  }
}

static
void do_lul_unwind_Buffer(PCandSP** pairs,
                          unsigned int* nPairs,
                          UnwinderThreadBuffer* buff,
                          int buffNo )
{
# if defined(SPS_ARCH_amd64) || defined(SPS_ARCH_x86)
  lul::UnwindRegs startRegs = buff->startRegs;
  if (0) {
    LOGF("Initial RIP = 0x%llx", (unsigned long long int)startRegs.xip.Value());
    LOGF("Initial RSP = 0x%llx", (unsigned long long int)startRegs.xsp.Value());
    LOGF("Initial RBP = 0x%llx", (unsigned long long int)startRegs.xbp.Value());
  }

# elif defined(SPS_ARCH_arm)
  lul::UnwindRegs startRegs = buff->startRegs;
  if (0) {
    LOGF("Initial R15 = 0x%llx", (unsigned long long int)startRegs.r15.Value());
    LOGF("Initial R13 = 0x%llx", (unsigned long long int)startRegs.r13.Value());
  }

# else
#   error "Unknown plat"
# endif

  
  
  
# if defined(SPS_OS_linux)
  
  
  
  
# elif defined(SPS_OS_android)
  
  
# elif defined(SPS_OS_darwin)
  
# else
#   error "Unknown plat"
# endif

  
  
  size_t scannedFramesAllowed
    = std::min(std::max(0, sUnwindStackScan), MAX_NATIVE_FRAMES);

  
  
  uintptr_t framePCs[MAX_NATIVE_FRAMES];
  uintptr_t frameSPs[MAX_NATIVE_FRAMES];
  size_t framesAvail = mozilla::ArrayLength(framePCs);
  size_t framesUsed  = 0;
  size_t scannedFramesAcquired = 0;
  sLUL->Unwind( &framePCs[0], &frameSPs[0], 
                &framesUsed, &scannedFramesAcquired,
                framesAvail, scannedFramesAllowed,
                &startRegs, &buff->stackImg );

  if (LOGLEVEL >= 2)
    stats_notify_frame( 1,
                        framesUsed - 1 - scannedFramesAcquired,
                        scannedFramesAcquired);

  
  
  *pairs  = (PCandSP*)calloc(framesUsed, sizeof(PCandSP));
  *nPairs = framesUsed;
  if (*pairs == nullptr) {
    *nPairs = 0;
    return;
  }

  if (framesUsed > 0) {
    for (unsigned int frame_index = 0; 
         frame_index < framesUsed; ++frame_index) {
      (*pairs)[framesUsed-1-frame_index].pc = framePCs[frame_index];
      (*pairs)[framesUsed-1-frame_index].sp = frameSPs[frame_index];
    }
  }

  if (LOGLEVEL >= 3) {
    LOGF("BPUnw: unwinder: seqNo %llu, buf %d: got %u frames",
         (unsigned long long int)buff->seqNo, buffNo,
         (unsigned int)framesUsed);
  }

  if (LOGLEVEL >= 2) {
    if (0 == (g_stats_totalSamples % 1000))
      LOGF("BPUnw: %llu total samples, %llu failed (buffer unavail), "
                   "%llu failed (thread unreg'd), ",
           (unsigned long long int)g_stats_totalSamples,
           (unsigned long long int)g_stats_noBuffAvail,
           (unsigned long long int)g_stats_thrUnregd);
  }
}

#endif 
