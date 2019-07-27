





#include "ProcessUtils.h"

#include "nsString.h"

#include <sys/prctl.h>

#ifdef MOZ_B2G_LOADER

#include <sys/types.h>
#include <unistd.h>

#include "nsAutoPtr.h"

#include "mozilla/Assertions.h"
#include "mozilla/ipc/PProcLoaderParent.h"
#include "mozilla/ipc/PProcLoaderChild.h"
#include "mozilla/ipc/Transport.h"
#include "mozilla/ipc/FileDescriptorUtils.h"
#include "mozilla/ipc/IOThreadChild.h"
#include "mozilla/dom/ContentProcess.h"
#include "base/file_descriptor_shuffle.h"
#include "mozilla/BackgroundHangMonitor.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/unused.h"
#include "base/process_util.h"
#include "base/eintr_wrapper.h"

#include "prenv.h"

#include "nsXULAppAPI.h" 

#include "nsAppRunner.h"

int content_process_main(int argc, char *argv[]);

typedef mozilla::Vector<int> FdArray;

#endif 

namespace mozilla {

namespace ipc {

void SetThisProcessName(const char *aName)
{
  prctl(PR_SET_NAME, (unsigned long)aName, 0uL, 0uL, 0uL);
}

#ifdef MOZ_B2G_LOADER






































using namespace base;
using namespace mozilla::dom;

static bool sProcLoaderClientOnDeinit = false;
static DebugOnly<bool> sProcLoaderClientInitialized = false;
static DebugOnly<bool> sProcLoaderClientGeckoInitialized = false;
static pid_t sProcLoaderPid = 0;
static int sProcLoaderChannelFd = -1;
static PProcLoaderParent *sProcLoaderParent = nullptr;
static MessageLoop *sProcLoaderLoop = nullptr;
static mozilla::UniquePtr<FdArray> sReservedFds;

static void ProcLoaderClientDeinit();







static const int kReservedFileDescriptors = 5;
static const int kBeginReserveFileDescriptor = STDERR_FILENO + 1;

class ProcLoaderParent : public PProcLoaderParent
{
private:
  nsAutoPtr<FileDescriptor> mChannelFd; 

public:
  ProcLoaderParent(FileDescriptor *aFd) : mChannelFd(aFd) {}

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool RecvLoadComplete(const int32_t &aPid,
                                const int32_t &aCookie) MOZ_OVERRIDE;

  virtual void OnChannelError() MOZ_OVERRIDE;
};

void
ProcLoaderParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

static void
_ProcLoaderParentDestroy(PProcLoaderParent *aLoader)
{
  aLoader->Close();
  delete aLoader;
  sProcLoaderClientOnDeinit = false;
}

bool
ProcLoaderParent::RecvLoadComplete(const int32_t &aPid,
                                   const int32_t &aCookie)
{
  ProcLoaderClientDeinit();
  return true;
}

static void
CloseFileDescriptors(FdArray& aFds)
{
  for (size_t i = 0; i < aFds.length(); i++) {
    unused << HANDLE_EINTR(close(aFds[i]));
  }
}

void
ProcLoaderParent::OnChannelError()
{
  if (sProcLoaderClientOnDeinit) {
    
    return;
  }
  NS_WARNING("ProcLoaderParent is in channel error");
  ProcLoaderClientDeinit();
}












static void
ProcLoaderClientInit(pid_t aPeerPid, int aChannelFd)
{
  MOZ_ASSERT(!sProcLoaderClientInitialized, "call ProcLoaderClientInit() more than once");
  MOZ_ASSERT(aPeerPid != 0 && aChannelFd != -1, "invalid argument");
  sProcLoaderPid = aPeerPid;
  sProcLoaderChannelFd = aChannelFd;
  sProcLoaderClientInitialized = true;
}




void
ProcLoaderClientGeckoInit()
{
  MOZ_ASSERT(sProcLoaderClientInitialized, "call ProcLoaderClientInit() at first");
  MOZ_ASSERT(!sProcLoaderClientGeckoInitialized,
             "call ProcLoaderClientGeckoInit() more than once");

  sProcLoaderClientGeckoInitialized = true;

  FileDescriptor *fd = new FileDescriptor(sProcLoaderChannelFd);
  close(sProcLoaderChannelFd);
  sProcLoaderChannelFd = -1;
  Transport *transport = OpenDescriptor(*fd, Transport::MODE_CLIENT);
  sProcLoaderParent = new ProcLoaderParent(fd);
  sProcLoaderParent->Open(transport,
                          sProcLoaderPid,
                          XRE_GetIOMessageLoop(),
                          ParentSide);
  sProcLoaderLoop = MessageLoop::current();
}




static void
ProcLoaderClientDeinit()
{
  MOZ_ASSERT(sProcLoaderClientGeckoInitialized && sProcLoaderClientInitialized);
  sProcLoaderClientGeckoInitialized = false;
  sProcLoaderClientInitialized = false;

  sProcLoaderClientOnDeinit = true;

  MOZ_ASSERT(sProcLoaderParent != nullptr);
  PProcLoaderParent *procLoaderParent = sProcLoaderParent;
  sProcLoaderParent = nullptr;
  sProcLoaderLoop = nullptr;

  MessageLoop::current()->
    PostTask(FROM_HERE,
             NewRunnableFunction(&_ProcLoaderParentDestroy,
                                 procLoaderParent));
}

struct AsyncSendLoadData
{
  nsTArray<nsCString> mArgv;
  nsTArray<nsCString> mEnv;
  nsTArray<FDRemap> mFdsremap;
  ChildPrivileges mPrivs;
  int mCookie;
};

static void
AsyncSendLoad(AsyncSendLoadData *aLoad)
{
  PProcLoaderParent *loader = sProcLoaderParent;
  DebugOnly<bool> ok =
    loader->SendLoad(aLoad->mArgv, aLoad->mEnv, aLoad->mFdsremap,
                     aLoad->mPrivs, aLoad->mCookie);
  MOZ_ASSERT(ok);
  delete aLoad;
}




bool
ProcLoaderLoad(const char *aArgv[],
               const char *aEnvp[],
               const file_handle_mapping_vector &aFdsRemap,
               const ChildPrivileges aPrivs,
               ProcessHandle *aProcessHandle)
{
  static int cookie=0;
  int i;

  if (sProcLoaderParent == nullptr || sProcLoaderPid == 0) {
    return false;
  }

  AsyncSendLoadData *load = new AsyncSendLoadData();
  nsTArray<nsCString> &argv = load->mArgv;
  for (i = 0; aArgv[i] != nullptr; i++) {
    argv.AppendElement(nsCString(aArgv[i]));
  }
  nsTArray<nsCString> &env = load->mEnv;
  for (i = 0; aEnvp[i] != nullptr; i++) {
    env.AppendElement(nsCString(aEnvp[i]));
  }
  nsTArray<FDRemap> &fdsremap = load->mFdsremap;
  for (file_handle_mapping_vector::const_iterator fdmap =
         aFdsRemap.begin();
       fdmap != aFdsRemap.end();
       fdmap++) {
    fdsremap.AppendElement(FDRemap(FileDescriptor(fdmap->first), fdmap->second));
  }
  load->mPrivs = aPrivs;
  load->mCookie = cookie++;

  *aProcessHandle = sProcLoaderPid;
  sProcLoaderPid = 0;

  sProcLoaderLoop->PostTask(FROM_HERE,
                            NewRunnableFunction(AsyncSendLoad, load));
  return true;
}


class ProcLoaderRunnerBase;

static bool sProcLoaderServing = false;
static ProcLoaderRunnerBase *sProcLoaderDispatchedTask = nullptr;

class ProcLoaderRunnerBase
{
public:
  virtual int DoWork() = 0;
  virtual ~ProcLoaderRunnerBase() {}
};


class ProcLoaderNoopRunner : public ProcLoaderRunnerBase {
public:
  virtual int DoWork();
};

int
ProcLoaderNoopRunner::DoWork() {
  return 0;
}




class ProcLoaderLoadRunner : public ProcLoaderRunnerBase {
private:
  const nsTArray<nsCString> mArgv;
  const nsTArray<nsCString> mEnv;
  const nsTArray<FDRemap> mFdsRemap;
  const ChildPrivileges mPrivs;

  void ShuffleFds();

public:
  ProcLoaderLoadRunner(const InfallibleTArray<nsCString>& aArgv,
                       const InfallibleTArray<nsCString>& aEnv,
                       const InfallibleTArray<FDRemap>& aFdsRemap,
                       const ChildPrivileges aPrivs)
    : mArgv(aArgv)
    , mEnv(aEnv)
    , mFdsRemap(aFdsRemap)
    , mPrivs(aPrivs) {}

  int DoWork();
};

void
ProcLoaderLoadRunner::ShuffleFds()
{
  unsigned int i;

  MOZ_ASSERT(mFdsRemap.Length() <= kReservedFileDescriptors);

  InjectiveMultimap fd_shuffle1, fd_shuffle2;
  fd_shuffle1.reserve(mFdsRemap.Length());
  fd_shuffle2.reserve(mFdsRemap.Length());

  for (i = 0; i < mFdsRemap.Length(); i++) {
    const FDRemap *map = &mFdsRemap[i];
    int fd = map->fd().PlatformHandle();
    int tofd = map->mapto();

    fd_shuffle1.push_back(InjectionArc(fd, tofd, false));
    fd_shuffle2.push_back(InjectionArc(fd, tofd, false));

    
    for (int* toErase = sReservedFds->begin();
         toErase < sReservedFds->end();
         toErase++) {
      if (tofd == *toErase) {
        sReservedFds->erase(toErase);
        break;
      }
    }
  }

  DebugOnly<bool> ok = ShuffleFileDescriptors(&fd_shuffle1);

  
  
  MOZ_ASSERT(sReservedFds);
  CloseFileDescriptors(*sReservedFds);
  sReservedFds = nullptr;

  
  
}

int
ProcLoaderLoadRunner::DoWork()
{
  unsigned int i;

  ShuffleFds();

  unsigned int argc = mArgv.Length();
  char **argv = new char *[argc + 1];
  for (i = 0; i < argc; i++) {
    argv[i] = ::strdup(mArgv[i].get());
  }
  argv[argc] = nullptr;

  unsigned int envc = mEnv.Length();
  for (i = 0; i < envc; i++) {
    PR_SetEnv(mEnv[i].get());
  }

  SetCurrentProcessPrivileges(mPrivs);

  MOZ_ASSERT(content_process_main != nullptr,
             "content_process_main not found");
  
  int ret = content_process_main(argc, argv);

  for (i = 0; i < argc; i++) {
    free(argv[i]);
  }
  delete[] argv;

  return ret;
}


class ProcLoaderChild : public PProcLoaderChild
{
  pid_t mPeerPid;

public:
  ProcLoaderChild(pid_t aPeerPid) : mPeerPid(aPeerPid) {}

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool RecvLoad(const InfallibleTArray<nsCString>& aArgv,
                        const InfallibleTArray<nsCString>& aEnv,
                        const InfallibleTArray<FDRemap>& aFdsremap,
                        const uint32_t& aPrivs,
                        const int32_t& aCookie);

  virtual void OnChannelError();
};

void
ProcLoaderChild::ActorDestroy(ActorDestroyReason aWhy)
{
}

static void
_ProcLoaderChildDestroy(ProcLoaderChild *aChild)
{
  aChild->Close();
  delete aChild;
  MessageLoop::current()->Quit();
}

bool
ProcLoaderChild::RecvLoad(const InfallibleTArray<nsCString>& aArgv,
                          const InfallibleTArray<nsCString>& aEnv,
                          const InfallibleTArray<FDRemap>& aFdsRemap,
                          const uint32_t& aPrivs,
                          const int32_t& aCookie) {
  if (!sProcLoaderServing) {
    return true;
  }
  sProcLoaderServing = false;

  MOZ_ASSERT(sProcLoaderDispatchedTask == nullptr);
  ChildPrivileges privs = static_cast<ChildPrivileges>(aPrivs);
  sProcLoaderDispatchedTask =
    new ProcLoaderLoadRunner(aArgv, aEnv, aFdsRemap, privs);

  SendLoadComplete(mPeerPid, aCookie);

  MessageLoop::current()->PostTask(FROM_HERE,
                                   NewRunnableFunction(_ProcLoaderChildDestroy,
                                                       this));
  return true;
}

void
ProcLoaderChild::OnChannelError()
{
  if (!sProcLoaderServing) {
    return;
  }
  sProcLoaderServing = false;

  PProcLoaderChild::OnChannelError();

  MOZ_ASSERT(sProcLoaderDispatchedTask == nullptr);
  sProcLoaderDispatchedTask = new ProcLoaderNoopRunner();

  MessageLoop::current()->PostTask(FROM_HERE,
                                   NewRunnableFunction(_ProcLoaderChildDestroy,
                                                       this));
}




class ScopedLogging
{
public:
  ScopedLogging() { NS_LogInit(); }
  ~ScopedLogging() { NS_LogTerm(); }
};









static int
ProcLoaderServiceRun(pid_t aPeerPid, int aFd,
                     int aArgc, const char *aArgv[],
                     FdArray& aReservedFds)
{
  
  
  sReservedFds = MakeUnique<FdArray>(mozilla::Move(aReservedFds));

  ScopedLogging logging;

  char **_argv;
  _argv = new char *[aArgc + 1];
  for (int i = 0; i < aArgc; i++) {
    _argv[i] = ::strdup(aArgv[i]);
    MOZ_ASSERT(_argv[i] != nullptr);
  }
  _argv[aArgc] = nullptr;

  gArgv = _argv;
  gArgc = aArgc;

  {
    nsresult rv = XRE_InitCommandLine(aArgc, _argv);
    if (NS_FAILED(rv)) {
      MOZ_CRASH();
    }

    FileDescriptor fd(aFd);
    close(aFd);

    MOZ_ASSERT(!sProcLoaderServing);
    MessageLoop loop;

    nsAutoPtr<ContentProcess> process;
    process = new ContentProcess(aPeerPid);
    ChildThread *iothread = process->child_thread();

    Transport *transport = OpenDescriptor(fd, Transport::MODE_CLIENT);
    ProcLoaderChild *loaderChild = new ProcLoaderChild(aPeerPid);
    
    
    loaderChild->Open(transport, aPeerPid, iothread->message_loop());

    BackgroundHangMonitor::Prohibit();

    sProcLoaderServing = true;
    loop.Run();

    BackgroundHangMonitor::Allow();

    XRE_DeinitCommandLine();
  }

  MOZ_ASSERT(sProcLoaderDispatchedTask != nullptr);
  ProcLoaderRunnerBase *task = sProcLoaderDispatchedTask;
  sProcLoaderDispatchedTask = nullptr;
  int ret = task->DoWork();
  delete task;

  for (int i = 0; i < aArgc; i++) {
    free(_argv[i]);
  }
  delete[] _argv;

  return ret;
}

#endif 

} 
} 

#ifdef MOZ_B2G_LOADER
void
XRE_ProcLoaderClientInit(pid_t aPeerPid, int aChannelFd, FdArray& aReservedFds)
{
  
  
  mozilla::ipc::CloseFileDescriptors(aReservedFds);

  mozilla::ipc::ProcLoaderClientInit(aPeerPid, aChannelFd);
}

int
XRE_ProcLoaderServiceRun(pid_t aPeerPid, int aFd,
                         int aArgc, const char *aArgv[],
                         FdArray& aReservedFds)
{
  return mozilla::ipc::ProcLoaderServiceRun(aPeerPid, aFd,
                                            aArgc, aArgv,
                                            aReservedFds);
}
#endif 
