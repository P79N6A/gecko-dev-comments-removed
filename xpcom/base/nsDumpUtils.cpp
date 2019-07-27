





#include "nsDumpUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "prenv.h"
#include <errno.h>
#include "mozilla/Services.h"
#include "nsIObserverService.h"
#include "mozilla/ClearOnShutdown.h"

#ifdef XP_UNIX 
#include "mozilla/Preferences.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace mozilla;

























static Atomic<int> sDumpPipeWriteFd(-1);

const char* const FifoWatcher::kPrefName =
  "memory_info_dumper.watch_fifo.enabled";

static void
DumpSignalHandler(int aSignum)
{
  
  

  if (sDumpPipeWriteFd != -1) {
    uint8_t signum = static_cast<int>(aSignum);
    write(sDumpPipeWriteFd, &signum, sizeof(signum));
  }
}

NS_IMPL_ISUPPORTS(FdWatcher, nsIObserver);

void
FdWatcher::Init()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  os->AddObserver(this, "xpcom-shutdown",  false);

  XRE_GetIOMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableMethod(this, &FdWatcher::StartWatching));
}




void
FdWatcher::StartWatching()
{
  MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());
  MOZ_ASSERT(mFd == -1);

  mFd = OpenFd();
  if (mFd == -1) {
    LOG("FdWatcher: OpenFd failed.");
    return;
  }

  MessageLoopForIO::current()->WatchFileDescriptor(
    mFd,  true,
    MessageLoopForIO::WATCH_READ,
    &mReadWatcher, this);
}



void
FdWatcher::StopWatching()
{
  MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

  mReadWatcher.StopWatchingFileDescriptor();
  if (mFd != -1) {
    close(mFd);
    mFd = -1;
  }
}

StaticRefPtr<SignalPipeWatcher> SignalPipeWatcher::sSingleton;

 SignalPipeWatcher*
SignalPipeWatcher::GetSingleton()
{
  if (!sSingleton) {
    sSingleton = new SignalPipeWatcher();
    sSingleton->Init();
    ClearOnShutdown(&sSingleton);
  }
  return sSingleton;
}

void
SignalPipeWatcher::RegisterCallback(uint8_t aSignal,
                                    PipeCallback aCallback)
{
  MutexAutoLock lock(mSignalInfoLock);

  for (SignalInfoArray::index_type i = 0; i < mSignalInfo.Length(); ++i) {
    if (mSignalInfo[i].mSignal == aSignal) {
      LOG("Register Signal(%d) callback failed! (DUPLICATE)", aSignal);
      return;
    }
  }
  SignalInfo signalInfo = { aSignal, aCallback };
  mSignalInfo.AppendElement(signalInfo);
  RegisterSignalHandler(signalInfo.mSignal);
}

void
SignalPipeWatcher::RegisterSignalHandler(uint8_t aSignal)
{
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  sigemptyset(&action.sa_mask);
  action.sa_handler = DumpSignalHandler;

  if (aSignal) {
    if (sigaction(aSignal, &action, nullptr)) {
      LOG("SignalPipeWatcher failed to register sig %d.", aSignal);
    }
  } else {
    MutexAutoLock lock(mSignalInfoLock);
    for (SignalInfoArray::index_type i = 0; i < mSignalInfo.Length(); i++) {
      if (sigaction(mSignalInfo[i].mSignal, &action, nullptr)) {
        LOG("SignalPipeWatcher failed to register signal(%d) "
            "dump signal handler.", mSignalInfo[i].mSignal);
      }
    }
  }
}

SignalPipeWatcher::~SignalPipeWatcher()
{
  if (sDumpPipeWriteFd != -1) {
    StopWatching();
  }
}

int
SignalPipeWatcher::OpenFd()
{
  MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

  
  
  int pipeFds[2];
  if (pipe(pipeFds)) {
    LOG("SignalPipeWatcher failed to create pipe.");
    return -1;
  }

  
  fcntl(pipeFds[0], F_SETFD, FD_CLOEXEC);
  fcntl(pipeFds[1], F_SETFD, FD_CLOEXEC);

  int readFd = pipeFds[0];
  sDumpPipeWriteFd = pipeFds[1];

  RegisterSignalHandler();
  return readFd;
}

void
SignalPipeWatcher::StopWatching()
{
  MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

  
  
  
  
  
  
  
  int pipeWriteFd = sDumpPipeWriteFd.exchange(-1);
  close(pipeWriteFd);

  FdWatcher::StopWatching();
}

void
SignalPipeWatcher::OnFileCanReadWithoutBlocking(int aFd)
{
  MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

  uint8_t signum;
  ssize_t numReceived = read(aFd, &signum, sizeof(signum));
  if (numReceived != sizeof(signum)) {
    LOG("Error reading from buffer in "
        "SignalPipeWatcher::OnFileCanReadWithoutBlocking.");
    return;
  }

  {
    MutexAutoLock lock(mSignalInfoLock);
    for (SignalInfoArray::index_type i = 0; i < mSignalInfo.Length(); i++) {
      if (signum == mSignalInfo[i].mSignal) {
        mSignalInfo[i].mCallback(signum);
        return;
      }
    }
  }
  LOG("SignalPipeWatcher got unexpected signum.");
}

StaticRefPtr<FifoWatcher> FifoWatcher::sSingleton;

 FifoWatcher*
FifoWatcher::GetSingleton()
{
  if (!sSingleton) {
    nsAutoCString dirPath;
    Preferences::GetCString(
      "memory_info_dumper.watch_fifo.directory", &dirPath);
    sSingleton = new FifoWatcher(dirPath);
    sSingleton->Init();
    ClearOnShutdown(&sSingleton);
  }
  return sSingleton;
}

 bool
FifoWatcher::MaybeCreate()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    
    
    return false;
  }

  if (!Preferences::GetBool(kPrefName, false)) {
    LOG("Fifo watcher disabled via pref.");
    return false;
  }

  
  if (!sSingleton) {
    GetSingleton();
  }
  return true;
}

void
FifoWatcher::RegisterCallback(const nsCString& aCommand, FifoCallback aCallback)
{
  MutexAutoLock lock(mFifoInfoLock);

  for (FifoInfoArray::index_type i = 0; i < mFifoInfo.Length(); ++i) {
    if (mFifoInfo[i].mCommand.Equals(aCommand)) {
      LOG("Register command(%s) callback failed! (DUPLICATE)", aCommand.get());
      return;
    }
  }
  FifoInfo aFifoInfo = { aCommand, aCallback };
  mFifoInfo.AppendElement(aFifoInfo);
}

FifoWatcher::~FifoWatcher()
{
}

int
FifoWatcher::OpenFd()
{
  
  

  nsCOMPtr<nsIFile> file;

  nsresult rv;
  if (mDirPath.Length() > 0) {
    rv = XRE_GetFileFromPath(mDirPath.get(), getter_AddRefs(file));
    if (NS_FAILED(rv)) {
      LOG("FifoWatcher failed to open file \"%s\"", mDirPath.get());
      return -1;
    }
  } else {
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(file));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return -1;
    }
  }

  rv = file->AppendNative(NS_LITERAL_CSTRING("debug_info_trigger"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return -1;
  }

  nsAutoCString path;
  rv = file->GetNativePath(path);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return -1;
  }

  
  
  
  if (unlink(path.get())) {
    LOG("FifoWatcher::OpenFifo unlink failed; errno=%d.  "
        "Continuing despite error.", errno);
  }

  if (mkfifo(path.get(), 0766)) {
    LOG("FifoWatcher::OpenFifo mkfifo failed; errno=%d", errno);
    return -1;
  }

#ifdef ANDROID
  
  
  chmod(path.get(), 0666);
#endif

  int fd;
  do {
    
    
    
    
    fd = open(path.get(), O_RDONLY | O_NONBLOCK);
  } while (fd == -1 && errno == EINTR);

  if (fd == -1) {
    LOG("FifoWatcher::OpenFifo open failed; errno=%d", errno);
    return -1;
  }

  
  if (fcntl(fd, F_SETFL, 0)) {
    close(fd);
    return -1;
  }

  return fd;
}

void
FifoWatcher::OnFileCanReadWithoutBlocking(int aFd)
{
  MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

  char buf[1024];
  int nread;
  do {
    
    nread = read(aFd, buf, sizeof(buf));
  } while (nread == -1 && errno == EINTR);

  if (nread == -1) {
    
    
    
    LOG("FifoWatcher hit an error (%d) and is quitting.", errno);
    StopWatching();
    return;
  }

  if (nread == 0) {
    
    
    

    LOG("FifoWatcher closing and re-opening fifo.");
    StopWatching();
    StartWatching();
    return;
  }

  nsAutoCString inputStr;
  inputStr.Append(buf, nread);

  
  
  
  inputStr.Trim("\b\t\r\n");

  {
    MutexAutoLock lock(mFifoInfoLock);

    for (FifoInfoArray::index_type i = 0; i < mFifoInfo.Length(); i++) {
      const nsCString commandStr = mFifoInfo[i].mCommand;
      if (inputStr == commandStr.get()) {
        mFifoInfo[i].mCallback(inputStr);
        return;
      }
    }
  }
  LOG("Got unexpected value from fifo; ignoring it.");
}

#endif 




 nsresult
nsDumpUtils::OpenTempFile(const nsACString& aFilename, nsIFile** aFile,
                          const nsACString& aFoldername)
{
#ifdef ANDROID
  
  
  if (!*aFile) {
    char* env = PR_GetEnv("DOWNLOADS_DIRECTORY");
    if (env) {
      NS_NewNativeLocalFile(nsCString(env),  true, aFile);
    }
  }
#endif
  nsresult rv;
  if (!*aFile) {
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, aFile);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

#ifdef ANDROID
  
  
  
  
  if (aFoldername != EmptyCString()) {
    rv = (*aFile)->AppendNative(aFoldername);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    
    
    (*aFile)->Create(nsIFile::DIRECTORY_TYPE, 0777);

    nsAutoCString dirPath;
    rv = (*aFile)->GetNativePath(dirPath);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    while (chmod(dirPath.get(), 0777) == -1 && errno == EINTR) {
    }
  }
#endif

  nsCOMPtr<nsIFile> file(*aFile);

  rv = file->AppendNative(aFilename);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = file->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0666);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifdef ANDROID
  
  
  
  nsAutoCString path;
  rv = file->GetNativePath(path);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  while (chmod(path.get(), 0666) == -1 && errno == EINTR) {
  }
#endif

  return NS_OK;
}
