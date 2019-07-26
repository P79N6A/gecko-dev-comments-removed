





#include "mozilla/nsMemoryInfoDumper.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ContentChild.h"
#include "nsIConsoleService.h"
#include "nsICycleCollectorListener.h"
#include "nsDirectoryServiceDefs.h"
#include "nsGZFileWriter.h"
#include "nsJSEnvironment.h"
#include "nsPrintfCString.h"

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#ifdef XP_LINUX
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef ANDROID
#include "android/log.h"
#endif

#ifdef LOG
#undef LOG
#endif

#ifdef ANDROID
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "Gecko:MemoryInfoDumper", ## __VA_ARGS__)
#else
#define LOG(...)
#endif

using namespace mozilla;
using namespace mozilla::dom;

namespace {

class DumpMemoryReportsRunnable : public nsRunnable
{
public:
  DumpMemoryReportsRunnable(const nsAString& aIdentifier,
                            bool aMinimizeMemoryUsage,
                            bool aDumpChildProcesses)

      : mIdentifier(aIdentifier)
      , mMinimizeMemoryUsage(aMinimizeMemoryUsage)
      , mDumpChildProcesses(aDumpChildProcesses)
  {}

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIMemoryInfoDumper> dumper = do_GetService("@mozilla.org/memory-info-dumper;1");
    dumper->DumpMemoryReportsToFile(
      mIdentifier, mMinimizeMemoryUsage, mDumpChildProcesses);
    return NS_OK;
  }

private:
  const nsString mIdentifier;
  const bool mMinimizeMemoryUsage;
  const bool mDumpChildProcesses;
};

class GCAndCCLogDumpRunnable : public nsRunnable
{
public:
  GCAndCCLogDumpRunnable(const nsAString& aIdentifier,
                         bool aDumpChildProcesses)
    : mIdentifier(aIdentifier)
    , mDumpChildProcesses(aDumpChildProcesses)
  {}

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIMemoryInfoDumper> dumper = do_GetService("@mozilla.org/memory-info-dumper;1");
    dumper->DumpGCAndCCLogsToFile(
      mIdentifier, mDumpChildProcesses);
    return NS_OK;
  }

private:
  const nsString mIdentifier;
  const bool mDumpChildProcesses;
};

} 

#ifdef XP_LINUX 
namespace {



























static int sDumpAboutMemorySignum;         
static int sDumpAboutMemoryAfterMMUSignum; 
static int sGCAndCCDumpSignum;             



static int sDumpAboutMemoryPipeWriteFd = -1;

void
DumpAboutMemorySignalHandler(int aSignum)
{
  
  

  if (sDumpAboutMemoryPipeWriteFd != -1) {
    uint8_t signum = static_cast<int>(aSignum);
    write(sDumpAboutMemoryPipeWriteFd, &signum, sizeof(signum));
  }
}





class FdWatcher : public MessageLoopForIO::Watcher
                , public nsIObserver
{
protected:
  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;
  int mFd;

public:
  FdWatcher()
    : mFd(-1)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  virtual ~FdWatcher()
  {
    
    MOZ_ASSERT(mFd == -1);
  }

  


  virtual int OpenFd() = 0;

  



  virtual void OnFileCanReadWithoutBlocking(int aFd) = 0;
  virtual void OnFileCanWriteWithoutBlocking(int Afd) {};

  NS_DECL_ISUPPORTS

  





  void Init()
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    os->AddObserver(this, "xpcom-shutdown",  false);

    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &FdWatcher::StartWatching));
  }

  
  
  
  virtual void StartWatching()
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

  
  
  virtual void StopWatching()
  {
    MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

    mReadWatcher.StopWatchingFileDescriptor();
    if (mFd != -1) {
      close(mFd);
      mFd = -1;
    }
  }

  NS_IMETHOD Observe(nsISupports* aSubject, const char* aTopic,
                     const PRUnichar* aData)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!strcmp(aTopic, "xpcom-shutdown"));

    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &FdWatcher::StopWatching));

    return NS_OK;
  }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(FdWatcher, nsIObserver);

class SignalPipeWatcher : public FdWatcher
{
public:
  static void Create()
  {
    nsRefPtr<SignalPipeWatcher> sw = new SignalPipeWatcher();
    sw->Init();
  }

  virtual ~SignalPipeWatcher()
  {
    MOZ_ASSERT(sDumpAboutMemoryPipeWriteFd == -1);
  }

  virtual int OpenFd()
  {
    MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

    sDumpAboutMemorySignum = SIGRTMIN;
    sDumpAboutMemoryAfterMMUSignum = SIGRTMIN + 1;
    sGCAndCCDumpSignum = SIGRTMIN + 2;

    
    
    int pipeFds[2];
    if (pipe(pipeFds)) {
      LOG("SignalPipeWatcher failed to create pipe.");
      return -1;
    }

    
    fcntl(pipeFds[0], F_SETFD, FD_CLOEXEC);
    fcntl(pipeFds[1], F_SETFD, FD_CLOEXEC);

    int readFd = pipeFds[0];
    sDumpAboutMemoryPipeWriteFd = pipeFds[1];

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    action.sa_handler = DumpAboutMemorySignalHandler;

    if (sigaction(sDumpAboutMemorySignum, &action, nullptr)) {
      LOG("SignalPipeWatcher failed to register about:memory "
          "dump signal handler.");
    }
    if (sigaction(sDumpAboutMemoryAfterMMUSignum, &action, nullptr)) {
      LOG("SignalPipeWatcher failed to register about:memory "
          "dump after MMU signal handler.");
    }
    if (sigaction(sGCAndCCDumpSignum, &action, nullptr)) {
      LOG("Failed to register GC+CC dump signal handler.");
    }

    return readFd;
  }

  virtual void StopWatching()
  {
    MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

    
    
    
    
    
    
    
    int pipeWriteFd = sDumpAboutMemoryPipeWriteFd;
    PR_ATOMIC_SET(&sDumpAboutMemoryPipeWriteFd, -1);
    close(pipeWriteFd);

    FdWatcher::StopWatching();
  }

  virtual void OnFileCanReadWithoutBlocking(int aFd)
  {
    MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

    uint8_t signum;
    ssize_t numReceived = read(aFd, &signum, sizeof(signum));
    if (numReceived != sizeof(signum)) {
      LOG("Error reading from buffer in "
          "SignalPipeWatcher::OnFileCanReadWithoutBlocking.");
      return;
    }

    if (signum == sDumpAboutMemorySignum ||
        signum == sDumpAboutMemoryAfterMMUSignum) {
      
      bool doMMUFirst = signum == sDumpAboutMemoryAfterMMUSignum;
      nsRefPtr<DumpMemoryReportsRunnable> runnable =
        new DumpMemoryReportsRunnable(
             EmptyString(),
            doMMUFirst,
             true);
      NS_DispatchToMainThread(runnable);
    }
    else if (signum == sGCAndCCDumpSignum) {
      
      nsRefPtr<GCAndCCLogDumpRunnable> runnable =
        new GCAndCCLogDumpRunnable(
             EmptyString(),
             true);
      NS_DispatchToMainThread(runnable);
    }
    else {
      LOG("SignalPipeWatcher got unexpected signum.");
    }
  }
};

class FifoWatcher : public FdWatcher
{
public:
  static void MaybeCreate()
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (XRE_GetProcessType() != GeckoProcessType_Default) {
      
      
      return;
    }

    if (!Preferences::GetBool("memory_info_dumper.watch_fifo.enabled", false)) {
      LOG("Fifo watcher disabled via pref.");
      return;
    }

    
    nsRefPtr<FifoWatcher> fw = new FifoWatcher();
    fw->Init();
  }

  virtual int OpenFd()
  {
    
    

    nsCOMPtr<nsIFile> file;
    nsAutoCString dirPath;
    nsresult rv = Preferences::GetCString(
      "memory_info_dumper.watch_fifo.directory", &dirPath);

    if (NS_SUCCEEDED(rv)) {
      rv = XRE_GetFileFromPath(dirPath.get(), getter_AddRefs(file));
      if (NS_FAILED(rv)) {
        LOG("FifoWatcher failed to open file \"%s\"", dirPath.get());
        return -1;
      }
    } else {
      rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(file));
      NS_ENSURE_SUCCESS(rv, -1);
    }

    rv = file->AppendNative(NS_LITERAL_CSTRING("debug_info_trigger"));
    NS_ENSURE_SUCCESS(rv, -1);

    nsAutoCString path;
    rv = file->GetNativePath(path);
    NS_ENSURE_SUCCESS(rv, -1);

    
    
    
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

  virtual void OnFileCanReadWithoutBlocking(int aFd)
  {
    MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

    char buf[1024];
    int nread;
    do {
      
      nread = read(aFd, buf, sizeof(buf));
    } while(nread == -1 && errno == EINTR);

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

    bool doMemoryReport = inputStr == NS_LITERAL_CSTRING("memory report");
    bool doMMUMemoryReport = inputStr == NS_LITERAL_CSTRING("minimize memory report");
    bool doGCCCDump = inputStr == NS_LITERAL_CSTRING("gc log");

    if (doMemoryReport || doMMUMemoryReport) {
      LOG("FifoWatcher dispatching memory report runnable.");
      nsRefPtr<DumpMemoryReportsRunnable> runnable =
        new DumpMemoryReportsRunnable(
           EmptyString(),
          doMMUMemoryReport,
           true);
      NS_DispatchToMainThread(runnable);
    } else if (doGCCCDump) {
      LOG("FifoWatcher dispatching GC/CC log runnable.");
      nsRefPtr<GCAndCCLogDumpRunnable> runnable =
        new GCAndCCLogDumpRunnable(
             EmptyString(),
             true);
      NS_DispatchToMainThread(runnable);
    } else {
      LOG("Got unexpected value from fifo; ignoring it.");
    }
  }
};

} 
#endif 

NS_IMPL_ISUPPORTS1(nsMemoryInfoDumper, nsIMemoryInfoDumper)

nsMemoryInfoDumper::nsMemoryInfoDumper()
{
}

nsMemoryInfoDumper::~nsMemoryInfoDumper()
{
}

 void
nsMemoryInfoDumper::Initialize()
{
#ifdef XP_LINUX
  SignalPipeWatcher::Create();
  FifoWatcher::MaybeCreate();
#endif
}

static void
EnsureNonEmptyIdentifier(nsAString& aIdentifier)
{
  if (!aIdentifier.IsEmpty()) {
    return;
  }

  
  
  
  
  aIdentifier.AppendInt(static_cast<int64_t>(PR_Now()) / 1000000);
}

NS_IMETHODIMP
nsMemoryInfoDumper::DumpMemoryReportsToFile(
    const nsAString& aIdentifier,
    bool aMinimizeMemoryUsage,
    bool aDumpChildProcesses)
{
  nsString identifier(aIdentifier);
  EnsureNonEmptyIdentifier(identifier);

  
  
  
  
  if (aDumpChildProcesses) {
    nsTArray<ContentParent*> children;
    ContentParent::GetAll(children);
    for (uint32_t i = 0; i < children.Length(); i++) {
      unused << children[i]->SendDumpMemoryReportsToFile(
          identifier, aMinimizeMemoryUsage, aDumpChildProcesses);
    }
  }

  if (aMinimizeMemoryUsage) {
    
    nsRefPtr<DumpMemoryReportsRunnable> callback =
      new DumpMemoryReportsRunnable(identifier,
           false,
           false);
    nsCOMPtr<nsIMemoryReporterManager> mgr =
      do_GetService("@mozilla.org/memory-reporter-manager;1");
    NS_ENSURE_TRUE(mgr, NS_ERROR_FAILURE);
    nsCOMPtr<nsICancelableRunnable> runnable;
    mgr->MinimizeMemoryUsage(callback, getter_AddRefs(runnable));
    return NS_OK;
  }

  return DumpMemoryReportsToFileImpl(identifier);
}

NS_IMETHODIMP
nsMemoryInfoDumper::DumpGCAndCCLogsToFile(
  const nsAString& aIdentifier,
  bool aDumpChildProcesses)
{
  nsString identifier(aIdentifier);
  EnsureNonEmptyIdentifier(identifier);

  if (aDumpChildProcesses) {
    nsTArray<ContentParent*> children;
    ContentParent::GetAll(children);
    for (uint32_t i = 0; i < children.Length(); i++) {
      unused << children[i]->SendDumpGCAndCCLogsToFile(
          identifier, aDumpChildProcesses);
    }
  }

  nsCOMPtr<nsICycleCollectorListener> logger =
    do_CreateInstance("@mozilla.org/cycle-collector-logger;1");
  logger->SetFilenameIdentifier(identifier);

  nsJSContext::CycleCollectNow(logger);
  return NS_OK;
}

namespace mozilla {

#define DUMP(o, s) \
  do { \
    nsresult rv = (o)->Write(s); \
    NS_ENSURE_SUCCESS(rv, rv); \
  } while (0)

static nsresult
DumpReport(nsIGZFileWriter *aWriter, bool aIsFirst,
  const nsACString &aProcess, const nsACString &aPath, int32_t aKind,
  int32_t aUnits, int64_t aAmount, const nsACString &aDescription)
{
  DUMP(aWriter, aIsFirst ? "[" : ",");

  
  
  
  if (!aProcess.IsEmpty()) {
    return NS_OK;
  }

  
  
  
  nsAutoCString processId;
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    
    processId.AssignLiteral("Main Process ");
  } else if (ContentChild *cc = ContentChild::GetSingleton()) {
    
    nsAutoString processName;
    cc->GetProcessName(processName);
    processId.Assign(NS_ConvertUTF16toUTF8(processName));
    if (!processId.IsEmpty()) {
      processId.AppendLiteral(" ");
    }
  }

  
  unsigned pid = getpid();
  processId.Append(nsPrintfCString("(pid %u)", pid));

  DUMP(aWriter, "\n    {\"process\": \"");
  DUMP(aWriter, processId);

  DUMP(aWriter, "\", \"path\": \"");
  nsCString path(aPath);
  path.ReplaceSubstring("\\", "\\\\");    
  path.ReplaceSubstring("\"", "\\\"");    
  DUMP(aWriter, path);

  DUMP(aWriter, "\", \"kind\": ");
  DUMP(aWriter, nsPrintfCString("%d", aKind));

  DUMP(aWriter, ", \"units\": ");
  DUMP(aWriter, nsPrintfCString("%d", aUnits));

  DUMP(aWriter, ", \"amount\": ");
  DUMP(aWriter, nsPrintfCString("%lld", aAmount));

  nsCString description(aDescription);
  description.ReplaceSubstring("\\", "\\\\");    
  description.ReplaceSubstring("\"", "\\\"");    
  description.ReplaceSubstring("\n", "\\n");     
  DUMP(aWriter, ", \"description\": \"");
  DUMP(aWriter, description);
  DUMP(aWriter, "\"}");

  return NS_OK;
}

class DumpMultiReporterCallback MOZ_FINAL : public nsIMemoryMultiReporterCallback
{
  public:
    NS_DECL_ISUPPORTS

      NS_IMETHOD Callback(const nsACString &aProcess, const nsACString &aPath,
          int32_t aKind, int32_t aUnits, int64_t aAmount,
          const nsACString &aDescription,
          nsISupports *aData)
      {
        nsCOMPtr<nsIGZFileWriter> writer = do_QueryInterface(aData);
        NS_ENSURE_TRUE(writer, NS_ERROR_FAILURE);

        
        
        
        return DumpReport(writer,  false, aProcess, aPath,
            aKind, aUnits, aAmount, aDescription);
        return NS_OK;
      }
};

NS_IMPL_ISUPPORTS1(
    DumpMultiReporterCallback
    , nsIMemoryMultiReporterCallback
    )

} 

static void
MakeFilename(const char *aPrefix, const nsAString &aIdentifier,
             const char *aSuffix, nsACString &aResult)
{
  aResult = nsPrintfCString("%s-%s-%d.%s",
                            aPrefix,
                            NS_ConvertUTF16toUTF8(aIdentifier).get(),
                            getpid(), aSuffix);
}

 nsresult
nsMemoryInfoDumper::OpenTempFile(const nsACString &aFilename, nsIFile* *aFile)
{
#ifdef ANDROID
  
  
  if (!*aFile) {
    char *env = PR_GetEnv("DOWNLOADS_DIRECTORY");
    if (env) {
      NS_NewNativeLocalFile(nsCString(env),  true, aFile);
    }
  }
#endif
  nsresult rv;
  if (!*aFile) {
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, aFile);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIFile> file(*aFile);

  rv = file->AppendNative(aFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0644);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

#ifdef MOZ_DMD
struct DMDWriteState
{
  static const size_t kBufSize = 4096;
  char mBuf[kBufSize];
  nsRefPtr<nsGZFileWriter> mGZWriter;

  DMDWriteState(nsGZFileWriter *aGZWriter)
    : mGZWriter(aGZWriter)
  {}
};

static void DMDWrite(void* aState, const char* aFmt, va_list ap)
{
  DMDWriteState *state = (DMDWriteState*)aState;
  vsnprintf(state->mBuf, state->kBufSize, aFmt, ap);
  unused << state->mGZWriter->Write(state->mBuf);
}
#endif

 nsresult
nsMemoryInfoDumper::DumpMemoryReportsToFileImpl(
  const nsAString& aIdentifier)
{
  MOZ_ASSERT(!aIdentifier.IsEmpty());

  
  
  
  
  
  
  
  
  
  

  
  
  nsCString mrFilename;
  MakeFilename("memory-report", aIdentifier, "json.gz", mrFilename);

  nsCOMPtr<nsIFile> mrTmpFile;
  nsresult rv;
  rv = OpenTempFile(NS_LITERAL_CSTRING("incomplete-") + mrFilename,
                    getter_AddRefs(mrTmpFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsGZFileWriter> writer = new nsGZFileWriter();
  rv = writer->Init(mrTmpFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
#ifdef MOZ_DMD
  dmd::ClearReports();
#endif

  

  
  
  
  
  
  DUMP(writer, "{\n  \"version\": 1,\n");

  DUMP(writer, "  \"hasMozMallocUsableSize\": ");

  nsCOMPtr<nsIMemoryReporterManager> mgr =
    do_GetService("@mozilla.org/memory-reporter-manager;1");
  NS_ENSURE_STATE(mgr);

  DUMP(writer, mgr->GetHasMozMallocUsableSize() ? "true" : "false");
  DUMP(writer, ",\n");
  DUMP(writer, "  \"reports\": ");

  
  bool isFirst = true;
  bool more;
  nsCOMPtr<nsISimpleEnumerator> e;
  mgr->EnumerateReporters(getter_AddRefs(e));
  while (NS_SUCCEEDED(e->HasMoreElements(&more)) && more) {
    nsCOMPtr<nsIMemoryReporter> r;
    e->GetNext(getter_AddRefs(r));

    nsCString process;
    rv = r->GetProcess(process);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString path;
    rv = r->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    int32_t kind;
    rv = r->GetKind(&kind);
    NS_ENSURE_SUCCESS(rv, rv);

    int32_t units;
    rv = r->GetUnits(&units);
    NS_ENSURE_SUCCESS(rv, rv);

    int64_t amount;
    rv = r->GetAmount(&amount);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString description;
    rv = r->GetDescription(description);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = DumpReport(writer, isFirst, process, path, kind, units, amount,
                    description);
    NS_ENSURE_SUCCESS(rv, rv);

    isFirst = false;
  }

  
  nsCOMPtr<nsISimpleEnumerator> e2;
  mgr->EnumerateMultiReporters(getter_AddRefs(e2));
  nsRefPtr<DumpMultiReporterCallback> cb = new DumpMultiReporterCallback();
  while (NS_SUCCEEDED(e2->HasMoreElements(&more)) && more) {
    nsCOMPtr<nsIMemoryMultiReporter> r;
    e2->GetNext(getter_AddRefs(r));
    r->CollectReports(cb, writer);
  }

  DUMP(writer, "\n  ]\n}");

#ifdef MOZ_DMD
  
  
  
  
  
  
  
  

  nsCString dmdFilename;
  MakeFilename("dmd", aIdentifier, "txt.gz", dmdFilename);

  nsCOMPtr<nsIFile> dmdFile;
  rv = OpenTempFile(dmdFilename, getter_AddRefs(dmdFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsGZFileWriter> dmdWriter = new nsGZFileWriter();
  rv = dmdWriter->Init(dmdFile);
  NS_ENSURE_SUCCESS(rv, rv);

  

  DMDWriteState state(dmdWriter);
  dmd::Writer w(DMDWrite, &state);
  dmd::Dump(w);

  rv = dmdWriter->Finish();
  NS_ENSURE_SUCCESS(rv, rv);
#endif  

  
  
  
  
  
  rv = writer->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  nsCOMPtr<nsIFile> mrFinalFile;
  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(mrFinalFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mrFinalFile->AppendNative(mrFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mrFinalFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString mrActualFinalFilename;
  rv = mrFinalFile->GetLeafName(mrActualFinalFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mrTmpFile->MoveTo( nullptr, mrActualFinalFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIConsoleService> cs =
    do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString path;
  mrTmpFile->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString msg = NS_LITERAL_STRING(
    "nsIMemoryInfoDumper dumped reports to ");
  msg.Append(path);
  return cs->LogStringMessage(msg.get());
}

#undef DUMP
