





#include "ThreadStackHelper.h"
#include "MainThreadUtils.h"
#include "nsJSPrincipals.h"
#include "nsScriptSecurityManager.h"
#include "jsfriendapi.h"
#include "prprf.h"
#ifdef MOZ_THREADSTACKHELPER_NATIVE
#include "shared-libraries.h"
#endif

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/IntegerPrintfMacros.h"
#include "mozilla/Move.h"
#include "mozilla/Scoped.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/MemoryChecking.h"

#ifdef MOZ_THREADSTACKHELPER_NATIVE
#include "google_breakpad/processor/call_stack.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "google_breakpad/processor/stack_frame_cpu.h"
#include "processor/basic_code_module.h"
#include "processor/basic_code_modules.h"
#endif

#if defined(MOZ_THREADSTACKHELPER_X86)
#include "processor/stackwalker_x86.h"
#elif defined(MOZ_THREADSTACKHELPER_X64)
#include "processor/stackwalker_amd64.h"
#elif defined(MOZ_THREADSTACKHELPER_ARM)
#include "processor/stackwalker_arm.h"
#endif

#if defined(MOZ_VALGRIND)
# include <valgrind/valgrind.h>
#endif

#include <string.h>
#include <vector>
#include <cstdlib>

#ifdef XP_LINUX
#ifdef ANDROID

# include "common/android/include/sys/ucontext.h"
#else
# include <ucontext.h>
#endif
#include <unistd.h>
#include <sys/syscall.h>
#endif

#if defined(XP_LINUX) || defined(XP_MACOSX)
#include <pthread.h>
#endif

#ifdef ANDROID
#ifndef SYS_gettid
#define SYS_gettid __NR_gettid
#endif
#if defined(__arm__) && !defined(__NR_rt_tgsigqueueinfo)

#define __NR_rt_tgsigqueueinfo (__NR_SYSCALL_BASE+363)
#endif
#ifndef SYS_rt_tgsigqueueinfo
#define SYS_rt_tgsigqueueinfo __NR_rt_tgsigqueueinfo
#endif
#endif

#ifdef MOZ_THREADSTACKHELPER_NATIVE
#if defined(MOZ_THREADSTACKHELPER_X86) || \
    defined(MOZ_THREADSTACKHELPER_X64) || \
    defined(MOZ_THREADSTACKHELPER_ARM)

#define MOZ_THREADSTACKHELPER_STACK_GROWS_DOWN
#else
#error "Unsupported architecture"
#endif
#endif 

namespace mozilla {

void
ThreadStackHelper::Startup()
{
#if defined(XP_LINUX)
  MOZ_ASSERT(NS_IsMainThread());
  if (!sInitialized) {
    
    sFillStackSignum = SIGRTMIN + 4;
    if (sFillStackSignum > SIGRTMAX) {
      
      MOZ_ASSERT(false);
      return;
    }
    struct sigaction sigact = {};
    sigact.sa_sigaction = FillStackHandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO | SA_RESTART;
    MOZ_ALWAYS_TRUE(!::sigaction(sFillStackSignum, &sigact, nullptr));
  }
  sInitialized++;
#endif
}

void
ThreadStackHelper::Shutdown()
{
#if defined(XP_LINUX)
  MOZ_ASSERT(NS_IsMainThread());
  if (sInitialized == 1) {
    struct sigaction sigact = {};
    sigact.sa_handler = SIG_DFL;
    MOZ_ALWAYS_TRUE(!::sigaction(sFillStackSignum, &sigact, nullptr));
  }
  sInitialized--;
#endif
}

ThreadStackHelper::ThreadStackHelper()
  : mStackToFill(nullptr)
#ifdef MOZ_THREADSTACKHELPER_PSEUDO
  , mPseudoStack(mozilla_get_pseudo_stack())
#ifdef MOZ_THREADSTACKHELPER_NATIVE
  , mContextToFill(nullptr)
#endif
  , mMaxStackSize(Stack::sMaxInlineStorage)
  , mMaxBufferSize(0)
#endif
{
#if defined(XP_LINUX)
  MOZ_ALWAYS_TRUE(!::sem_init(&mSem, 0, 0));
  mThreadID = ::syscall(SYS_gettid);
#elif defined(XP_WIN)
  mInitialized = !!::DuplicateHandle(
    ::GetCurrentProcess(), ::GetCurrentThread(),
    ::GetCurrentProcess(), &mThreadID,
    THREAD_SUSPEND_RESUME
#ifdef MOZ_THREADSTACKHELPER_NATIVE
    | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION
#endif
    , FALSE, 0);
  MOZ_ASSERT(mInitialized);
#elif defined(XP_MACOSX)
  mThreadID = mach_thread_self();
#endif

#ifdef MOZ_THREADSTACKHELPER_NATIVE
  GetThreadStackBase();
#endif
}

ThreadStackHelper::~ThreadStackHelper()
{
#if defined(XP_LINUX)
  MOZ_ALWAYS_TRUE(!::sem_destroy(&mSem));
#elif defined(XP_WIN)
  if (mInitialized) {
    MOZ_ALWAYS_TRUE(!!::CloseHandle(mThreadID));
  }
#endif
}

#ifdef MOZ_THREADSTACKHELPER_NATIVE
void ThreadStackHelper::GetThreadStackBase()
{
  mThreadStackBase = 0;

#if defined(XP_LINUX)
  void* stackAddr;
  size_t stackSize;
  ::pthread_t pthr = ::pthread_self();
  ::pthread_attr_t pthr_attr;
  NS_ENSURE_TRUE_VOID(!::pthread_getattr_np(pthr, &pthr_attr));
  if (!::pthread_attr_getstack(&pthr_attr, &stackAddr, &stackSize)) {
#ifdef MOZ_THREADSTACKHELPER_STACK_GROWS_DOWN
    mThreadStackBase = intptr_t(stackAddr) + stackSize;
#else
    mThreadStackBase = intptr_t(stackAddr);
#endif
  }
  MOZ_ALWAYS_TRUE(!::pthread_attr_destroy(&pthr_attr));

#elif defined(XP_WIN)
  ::MEMORY_BASIC_INFORMATION meminfo = {};
  NS_ENSURE_TRUE_VOID(::VirtualQuery(&meminfo, &meminfo, sizeof(meminfo)));
#ifdef MOZ_THREADSTACKHELPER_STACK_GROWS_DOWN
  mThreadStackBase = intptr_t(meminfo.BaseAddress) + meminfo.RegionSize;
#else
  mThreadStackBase = intptr_t(meminfo.AllocationBase);
#endif

#elif defined(XP_MACOSX)
  ::pthread_t pthr = ::pthread_self();
  mThreadStackBase = intptr_t(::pthread_get_stackaddr_np(pthr));

#else
  #error "Unsupported platform"
#endif 
}
#endif 

namespace {
template<typename T>
class ScopedSetPtr
{
private:
  T*& mPtr;
public:
  ScopedSetPtr(T*& p, T* val) : mPtr(p) { mPtr = val; }
  ~ScopedSetPtr() { mPtr = nullptr; }
};
} 

void
ThreadStackHelper::GetStack(Stack& aStack)
{
  
  if (!PrepareStackBuffer(aStack)) {
    
    return;
  }

  ScopedSetPtr<Stack> stackPtr(mStackToFill, &aStack);

#if defined(XP_LINUX)
  if (!sInitialized) {
    MOZ_ASSERT(false);
    return;
  }
  siginfo_t uinfo = {};
  uinfo.si_signo = sFillStackSignum;
  uinfo.si_code = SI_QUEUE;
  uinfo.si_pid = getpid();
  uinfo.si_uid = getuid();
  uinfo.si_value.sival_ptr = this;
  if (::syscall(SYS_rt_tgsigqueueinfo, uinfo.si_pid,
                mThreadID, sFillStackSignum, &uinfo)) {
    
    
    return;
  }
  MOZ_ALWAYS_TRUE(!::sem_wait(&mSem));

#elif defined(XP_WIN)
  if (!mInitialized) {
    MOZ_ASSERT(false);
    return;
  }
  if (::SuspendThread(mThreadID) == DWORD(-1)) {
    MOZ_ASSERT(false);
    return;
  }

  FillStackBuffer();
  FillThreadContext();

  MOZ_ALWAYS_TRUE(::ResumeThread(mThreadID) != DWORD(-1));

#elif defined(XP_MACOSX)
# if defined(MOZ_VALGRIND) && defined(RUNNING_ON_VALGRIND)
  if (RUNNING_ON_VALGRIND) {
    

    return;
  }
# endif

  if (::thread_suspend(mThreadID) != KERN_SUCCESS) {
    MOZ_ASSERT(false);
    return;
  }

  FillStackBuffer();
  FillThreadContext();

  MOZ_ALWAYS_TRUE(::thread_resume(mThreadID) == KERN_SUCCESS);

#endif
}

#ifdef MOZ_THREADSTACKHELPER_NATIVE
class ThreadStackHelper::CodeModulesProvider
  : public google_breakpad::CodeModules
{
private:
  typedef google_breakpad::CodeModule CodeModule;
  typedef google_breakpad::BasicCodeModule BasicCodeModule;

  const SharedLibraryInfo mLibs;
  mutable ScopedDeletePtr<BasicCodeModule> mModule;

public:
  CodeModulesProvider() : mLibs(SharedLibraryInfo::GetInfoForSelf()) {}
  virtual ~CodeModulesProvider() {}

  virtual unsigned int module_count() const
  {
    return mLibs.GetSize();
  }

  virtual const CodeModule* GetModuleForAddress(uint64_t aAddress) const
  {
    MOZ_CRASH("Not implemented");
  }

  virtual const CodeModule* GetMainModule() const
  {
    return nullptr;
  }

  virtual const CodeModule* GetModuleAtSequence(unsigned int aSequence) const
  {
    MOZ_CRASH("Not implemented");
  }

  virtual const CodeModule* GetModuleAtIndex(unsigned int aIndex) const
  {
    const SharedLibrary& lib = mLibs.GetEntry(aIndex);
    mModule = new BasicCodeModule(lib.GetStart(), lib.GetEnd() - lib.GetStart(),
                                  lib.GetName(), lib.GetBreakpadId(),
                                  lib.GetName(), lib.GetBreakpadId(), "");
    
    return mModule;
  }

  virtual const CodeModules* Copy() const
  {
    MOZ_CRASH("Not implemented");
  }
};

class ThreadStackHelper::ThreadContext final
  : public google_breakpad::MemoryRegion
{
public:
#if defined(MOZ_THREADSTACKHELPER_X86)
  typedef MDRawContextX86 Context;
#elif defined(MOZ_THREADSTACKHELPER_X64)
  typedef MDRawContextAMD64 Context;
#elif defined(MOZ_THREADSTACKHELPER_ARM)
  typedef MDRawContextARM Context;
#endif
  
  static const size_t kMaxStackSize = 0x1000;
  
  static const unsigned int kMaxStackFrames = 32;
  
  bool mValid;
  
  Context mContext;
  
  UniquePtr<uint8_t[]> mStack;
  
  uintptr_t mStackBase;
  
  size_t mStackSize;
  
  const void* mStackEnd;

  ThreadContext()
    : mValid(false)
    , mStackBase(0)
    , mStackSize(0)
    , mStackEnd(nullptr) {}
  virtual ~ThreadContext() {}

  virtual uint64_t GetBase() const { return uint64_t(mStackBase); }
  virtual uint32_t GetSize() const { return mStackSize; }
  virtual bool GetMemoryAtAddress(uint64_t aAddress, uint8_t* aValue) const
  {
    return GetMemoryAtAddressInternal(aAddress, aValue);
  }
  virtual bool GetMemoryAtAddress(uint64_t aAddress, uint16_t* aValue) const
  {
    return GetMemoryAtAddressInternal(aAddress, aValue);
  }
  virtual bool GetMemoryAtAddress(uint64_t aAddress, uint32_t* aValue) const
  {
    return GetMemoryAtAddressInternal(aAddress, aValue);
  }
  virtual bool GetMemoryAtAddress(uint64_t aAddress, uint64_t* aValue) const
  {
    return GetMemoryAtAddressInternal(aAddress, aValue);
  }

private:
  template<typename T>
  bool GetMemoryAtAddressInternal(uint64_t aAddress, T* aValue) const
  {
    const intptr_t offset = intptr_t(aAddress) - intptr_t(GetBase());
    if (offset < 0 || uintptr_t(offset) > (GetSize() - sizeof(T))) {
      return false;
    }
    *aValue = *reinterpret_cast<const T*>(&mStack[offset]);
    return true;
  }
};
#endif 

void
ThreadStackHelper::GetNativeStack(Stack& aStack)
{
#ifdef MOZ_THREADSTACKHELPER_NATIVE
  ThreadContext context;
  context.mStack = MakeUnique<uint8_t[]>(ThreadContext::kMaxStackSize);

  ScopedSetPtr<ThreadContext> contextPtr(mContextToFill, &context);

  
  GetStack(aStack);
  NS_ENSURE_TRUE_VOID(context.mValid);

  CodeModulesProvider modulesProvider;
  google_breakpad::BasicCodeModules modules(&modulesProvider);
  google_breakpad::BasicSourceLineResolver resolver;
  google_breakpad::StackFrameSymbolizer symbolizer(nullptr, &resolver);

#if defined(MOZ_THREADSTACKHELPER_X86)
  google_breakpad::StackwalkerX86 stackWalker(
    nullptr, &context.mContext, &context, &modules, &symbolizer);
#elif defined(MOZ_THREADSTACKHELPER_X64)
  google_breakpad::StackwalkerAMD64 stackWalker(
    nullptr, &context.mContext, &context, &modules, &symbolizer);
#elif defined(MOZ_THREADSTACKHELPER_ARM)
  google_breakpad::StackwalkerARM stackWalker(
    nullptr, &context.mContext, -1, &context, &modules, &symbolizer);
#else
  #error "Unsupported architecture"
#endif

  google_breakpad::CallStack callStack;
  std::vector<const google_breakpad::CodeModule*> modules_without_symbols;

  google_breakpad::Stackwalker::set_max_frames(ThreadContext::kMaxStackFrames);
  google_breakpad::Stackwalker::
    set_max_frames_scanned(ThreadContext::kMaxStackFrames);

  NS_ENSURE_TRUE_VOID(stackWalker.Walk(&callStack, &modules_without_symbols));

  const std::vector<google_breakpad::StackFrame*>& frames(*callStack.frames());
  for (intptr_t i = frames.size() - 1; i >= 0; i--) {
    const google_breakpad::StackFrame& frame = *frames[i];
    if (!frame.module) {
      continue;
    }
    const string& module = frame.module->code_file();
#if defined(XP_LINUX) || defined(XP_MACOSX)
    const char PATH_SEP = '/';
#elif defined(XP_WIN)
    const char PATH_SEP = '\\';
#endif
    const char* const module_basename = strrchr(module.c_str(), PATH_SEP);
    const char* const module_name = module_basename ?
                                    module_basename + 1 : module.c_str();

    char buffer[0x100];
    size_t len = 0;
    if (!frame.function_name.empty()) {
      len = PR_snprintf(buffer, sizeof(buffer), "%s:%s",
                        module_name, frame.function_name.c_str());
    } else {
      len = PR_snprintf(buffer, sizeof(buffer), "%s:0x%p",
                        module_name, (intptr_t)
                        (frame.instruction - frame.module->base_address()));
    }
    if (len) {
      aStack.AppendViaBuffer(buffer, len);
    }
  }
#endif 
}

#ifdef XP_LINUX

int ThreadStackHelper::sInitialized;
int ThreadStackHelper::sFillStackSignum;

void
ThreadStackHelper::FillStackHandler(int aSignal, siginfo_t* aInfo,
                                    void* aContext)
{
  ThreadStackHelper* const helper =
    reinterpret_cast<ThreadStackHelper*>(aInfo->si_value.sival_ptr);
  helper->FillStackBuffer();
  helper->FillThreadContext(aContext);
  ::sem_post(&helper->mSem);
}

#endif 

bool
ThreadStackHelper::PrepareStackBuffer(Stack& aStack)
{
  
  aStack.clear();
#ifdef MOZ_THREADSTACKHELPER_PSEUDO
  




#ifdef MOZ_WIDGET_GONK
  if (!mPseudoStack) {
    return false;
  }
#endif
  MOZ_ASSERT(mPseudoStack);
  if (!aStack.reserve(mMaxStackSize) ||
      !aStack.reserve(aStack.capacity()) || 
      !aStack.EnsureBufferCapacity(mMaxBufferSize)) {
    return false;
  }
  return true;
#else
  return false;
#endif
}

#ifdef MOZ_THREADSTACKHELPER_PSEUDO

namespace {

bool
IsChromeJSScript(JSScript* aScript)
{
  
  

  nsIScriptSecurityManager* const secman =
    nsScriptSecurityManager::GetScriptSecurityManager();
  NS_ENSURE_TRUE(secman, false);

  JSPrincipals* const principals = JS_GetScriptPrincipals(aScript);
  return secman->IsSystemPrincipal(nsJSPrincipals::get(principals));
}



template <size_t LEN>
const char*
GetFullPathForScheme(const char* filename, const char (&scheme)[LEN]) {
  
  if (!strncmp(filename, scheme, LEN - 1)) {
    return filename + LEN - 1;
  }
  return nullptr;
}



template <size_t LEN>
const char*
GetPathAfterComponent(const char* filename, const char (&component)[LEN]) {
  const char* found = nullptr;
  const char* next = strstr(filename, component);
  while (next) {
    
    
    found = next + LEN - 1;
    
    next = strstr(found - 1, component);
  }
  return found;
}

} 

const char*
ThreadStackHelper::AppendJSEntry(const volatile StackEntry* aEntry,
                                 intptr_t& aAvailableBufferSize,
                                 const char* aPrevLabel)
{
  
  
  
  MOZ_ASSERT(aEntry->isJs());
  MOZ_ASSERT(aEntry->script());

  const char* label;
  if (IsChromeJSScript(aEntry->script())) {
    const char* filename = JS_GetScriptFilename(aEntry->script());
    const unsigned lineno = JS_PCToLineNumber(aEntry->script(), aEntry->pc());
    MOZ_ASSERT(filename);

    char buffer[128]; 

    
    
    const char* basename = GetPathAfterComponent(filename, " -> ");
    if (basename) {
      filename = basename;
    }

    basename = GetFullPathForScheme(filename, "chrome://");
    if (!basename) {
      basename = GetFullPathForScheme(filename, "resource://");
    }
    if (!basename) {
      
      
      basename = GetPathAfterComponent(filename, "/extensions/");
    }
    if (!basename) {
      
      basename = strrchr(filename, '/');
      basename = basename ? basename + 1 : filename;
      
      filename = strrchr(basename, '\\');
      if (filename) {
        basename = filename + 1;
      }
    }

    size_t len = PR_snprintf(buffer, sizeof(buffer), "%s:%u", basename, lineno);
    if (len < sizeof(buffer)) {
      if (mStackToFill->IsSameAsEntry(aPrevLabel, buffer)) {
        return aPrevLabel;
      }

      
      aAvailableBufferSize -= (len + 1);
      if (aAvailableBufferSize >= 0) {
        
        return mStackToFill->InfallibleAppendViaBuffer(buffer, len);
      }
      
    }
    
    label = "(chrome script)";
  } else {
    label = "(content script)";
  }

  if (mStackToFill->IsSameAsEntry(aPrevLabel, label)) {
    return aPrevLabel;
  }
  mStackToFill->infallibleAppend(label);
  return label;
}

#endif 

void
ThreadStackHelper::FillStackBuffer()
{
  MOZ_ASSERT(mStackToFill->empty());

#ifdef MOZ_THREADSTACKHELPER_PSEUDO
  size_t reservedSize = mStackToFill->capacity();
  size_t reservedBufferSize = mStackToFill->AvailableBufferSize();
  intptr_t availableBufferSize = intptr_t(reservedBufferSize);

  
  const volatile StackEntry* entry = mPseudoStack->mStack;
  const volatile StackEntry* end = entry + mPseudoStack->stackSize();
  
  const char* prevLabel = nullptr;
  for (; reservedSize-- && entry != end; entry++) {
    

    if (entry->isCopyLabel()) {
      continue;
    }
    if (entry->isJs()) {
      prevLabel = AppendJSEntry(entry, availableBufferSize, prevLabel);
      continue;
    }
#ifdef MOZ_THREADSTACKHELPER_NATIVE
    if (mContextToFill) {
      mContextToFill->mStackEnd = entry->stackAddress();
    }
#endif
    const char* const label = entry->label();
    if (mStackToFill->IsSameAsEntry(prevLabel, label)) {
      
      continue;
    }
    mStackToFill->infallibleAppend(label);
    prevLabel = label;
  }

  
  
  mMaxStackSize = mStackToFill->capacity() + (end - entry);

  
  
  if (availableBufferSize < 0) {
    mMaxBufferSize = reservedBufferSize - availableBufferSize;
  }
#endif
}

MOZ_ASAN_BLACKLIST void
ThreadStackHelper::FillThreadContext(void* aContext)
{
#ifdef MOZ_THREADSTACKHELPER_NATIVE
  if (!mContextToFill) {
    return;
  }

#if defined(XP_LINUX)
  const ucontext_t& context = *reinterpret_cast<ucontext_t*>(aContext);
#if defined(MOZ_THREADSTACKHELPER_X86)
  mContextToFill->mContext.context_flags = MD_CONTEXT_X86_FULL;
  mContextToFill->mContext.edi = context.uc_mcontext.gregs[REG_EDI];
  mContextToFill->mContext.esi = context.uc_mcontext.gregs[REG_ESI];
  mContextToFill->mContext.ebx = context.uc_mcontext.gregs[REG_EBX];
  mContextToFill->mContext.edx = context.uc_mcontext.gregs[REG_EDX];
  mContextToFill->mContext.ecx = context.uc_mcontext.gregs[REG_ECX];
  mContextToFill->mContext.eax = context.uc_mcontext.gregs[REG_EAX];
  mContextToFill->mContext.ebp = context.uc_mcontext.gregs[REG_EBP];
  mContextToFill->mContext.eip = context.uc_mcontext.gregs[REG_EIP];
  mContextToFill->mContext.eflags = context.uc_mcontext.gregs[REG_EFL];
  mContextToFill->mContext.esp = context.uc_mcontext.gregs[REG_ESP];
#elif defined(MOZ_THREADSTACKHELPER_X64)
  mContextToFill->mContext.context_flags = MD_CONTEXT_AMD64_FULL;
  mContextToFill->mContext.eflags = uint32_t(context.uc_mcontext.gregs[REG_EFL]);
  mContextToFill->mContext.rax = context.uc_mcontext.gregs[REG_RAX];
  mContextToFill->mContext.rcx = context.uc_mcontext.gregs[REG_RCX];
  mContextToFill->mContext.rdx = context.uc_mcontext.gregs[REG_RDX];
  mContextToFill->mContext.rbx = context.uc_mcontext.gregs[REG_RBX];
  mContextToFill->mContext.rsp = context.uc_mcontext.gregs[REG_RSP];
  mContextToFill->mContext.rbp = context.uc_mcontext.gregs[REG_RBP];
  mContextToFill->mContext.rsi = context.uc_mcontext.gregs[REG_RSI];
  mContextToFill->mContext.rdi = context.uc_mcontext.gregs[REG_RDI];
  memcpy(&mContextToFill->mContext.r8,
         &context.uc_mcontext.gregs[REG_R8], 8 * sizeof(int64_t));
  mContextToFill->mContext.rip = context.uc_mcontext.gregs[REG_RIP];
#elif defined(MOZ_THREADSTACKHELPER_ARM)
  mContextToFill->mContext.context_flags = MD_CONTEXT_ARM_FULL;
  memcpy(&mContextToFill->mContext.iregs[0],
         &context.uc_mcontext.arm_r0, 17 * sizeof(int32_t));
#else
  #error "Unsupported architecture"
#endif 

#elif defined(XP_WIN)
  
  
  static_assert(sizeof(ThreadContext::Context) == sizeof(::CONTEXT),
                "Context struct mismatch");
  static_assert(offsetof(ThreadContext::Context, context_flags) ==
                offsetof(::CONTEXT, ContextFlags),
                "Context struct mismatch");
  mContextToFill->mContext.context_flags = CONTEXT_FULL;
  NS_ENSURE_TRUE_VOID(::GetThreadContext(mThreadID,
      reinterpret_cast<::CONTEXT*>(&mContextToFill->mContext)));

#elif defined(XP_MACOSX)
#if defined(MOZ_THREADSTACKHELPER_X86)
  const thread_state_flavor_t flavor = x86_THREAD_STATE32;
  x86_thread_state32_t state = {};
  mach_msg_type_number_t count = x86_THREAD_STATE32_COUNT;
#elif defined(MOZ_THREADSTACKHELPER_X64)
  const thread_state_flavor_t flavor = x86_THREAD_STATE64;
  x86_thread_state64_t state = {};
  mach_msg_type_number_t count = x86_THREAD_STATE64_COUNT;
#elif defined(MOZ_THREADSTACKHELPER_ARM)
  const thread_state_flavor_t flavor = ARM_THREAD_STATE;
  arm_thread_state_t state = {};
  mach_msg_type_number_t count = ARM_THREAD_STATE_COUNT;
#endif
  NS_ENSURE_TRUE_VOID(KERN_SUCCESS == ::thread_get_state(
    mThreadID, flavor, reinterpret_cast<thread_state_t>(&state), &count));
#if __DARWIN_UNIX03
#define GET_REGISTER(s, r) ((s).__##r)
#else
#define GET_REGISTER(s, r) ((s).r)
#endif
#if defined(MOZ_THREADSTACKHELPER_X86)
  mContextToFill->mContext.context_flags = MD_CONTEXT_X86_FULL;
  mContextToFill->mContext.edi = GET_REGISTER(state, edi);
  mContextToFill->mContext.esi = GET_REGISTER(state, esi);
  mContextToFill->mContext.ebx = GET_REGISTER(state, ebx);
  mContextToFill->mContext.edx = GET_REGISTER(state, edx);
  mContextToFill->mContext.ecx = GET_REGISTER(state, ecx);
  mContextToFill->mContext.eax = GET_REGISTER(state, eax);
  mContextToFill->mContext.ebp = GET_REGISTER(state, ebp);
  mContextToFill->mContext.eip = GET_REGISTER(state, eip);
  mContextToFill->mContext.eflags = GET_REGISTER(state, eflags);
  mContextToFill->mContext.esp = GET_REGISTER(state, esp);
#elif defined(MOZ_THREADSTACKHELPER_X64)
  mContextToFill->mContext.context_flags = MD_CONTEXT_AMD64_FULL;
  mContextToFill->mContext.eflags = uint32_t(GET_REGISTER(state, rflags));
  mContextToFill->mContext.rax = GET_REGISTER(state, rax);
  mContextToFill->mContext.rcx = GET_REGISTER(state, rcx);
  mContextToFill->mContext.rdx = GET_REGISTER(state, rdx);
  mContextToFill->mContext.rbx = GET_REGISTER(state, rbx);
  mContextToFill->mContext.rsp = GET_REGISTER(state, rsp);
  mContextToFill->mContext.rbp = GET_REGISTER(state, rbp);
  mContextToFill->mContext.rsi = GET_REGISTER(state, rsi);
  mContextToFill->mContext.rdi = GET_REGISTER(state, rdi);
  memcpy(&mContextToFill->mContext.r8,
         &GET_REGISTER(state, r8), 8 * sizeof(int64_t));
  mContextToFill->mContext.rip = GET_REGISTER(state, rip);
#elif defined(MOZ_THREADSTACKHELPER_ARM)
  mContextToFill->mContext.context_flags = MD_CONTEXT_ARM_FULL;
  memcpy(mContextToFill->mContext.iregs,
         GET_REGISTER(state, r), 17 * sizeof(int32_t));
#else
  #error "Unsupported architecture"
#endif 
#undef GET_REGISTER

#else
  #error "Unsupported platform"
#endif 

  intptr_t sp = 0;
#if defined(MOZ_THREADSTACKHELPER_X86)
  sp = mContextToFill->mContext.esp;
#elif defined(MOZ_THREADSTACKHELPER_X64)
  sp = mContextToFill->mContext.rsp;
#elif defined(MOZ_THREADSTACKHELPER_ARM)
  sp = mContextToFill->mContext.iregs[13];
#else
  #error "Unsupported architecture"
#endif 
  NS_ENSURE_TRUE_VOID(sp);
  NS_ENSURE_TRUE_VOID(mThreadStackBase);

  size_t stackSize = std::min(intptr_t(ThreadContext::kMaxStackSize),
                              std::abs(sp - mThreadStackBase));

  if (mContextToFill->mStackEnd) {
    
    stackSize = std::min(intptr_t(stackSize),
      std::abs(sp - intptr_t(mContextToFill->mStackEnd)));
  }

#ifndef MOZ_THREADSTACKHELPER_STACK_GROWS_DOWN
  
  
  
  sp -= stackSize - sizeof(void*);
#endif

#ifndef MOZ_ASAN
  memcpy(mContextToFill->mStack.get(), reinterpret_cast<void*>(sp), stackSize);
  
  
  
  
  MOZ_MAKE_MEM_DEFINED(mContextToFill->mStack.get(), stackSize);
#else
  
  
  intptr_t* dst = reinterpret_cast<intptr_t*>(&mContextToFill->mStack[0]);
  const intptr_t* src = reinterpret_cast<intptr_t*>(sp);
  for (intptr_t len = stackSize; len > 0; len -= sizeof(*src)) {
    *(dst++) = *(src++);
  }
#endif

  mContextToFill->mStackBase = uintptr_t(sp);
  mContextToFill->mStackSize = stackSize;
  mContextToFill->mValid = true;
#endif 
}

} 
