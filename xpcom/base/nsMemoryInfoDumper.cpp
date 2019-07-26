





#include "mozilla/nsMemoryInfoDumper.h"

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/FileUtils.h"
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
#endif

#ifdef ANDROID
#include <sys/stat.h>
#endif

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {
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



static int sDumpAboutMemoryPipeWriteFd;

void
DumpAboutMemorySignalHandler(int aSignum)
{
  
  

  if (sDumpAboutMemoryPipeWriteFd != 0) {
    uint8_t signum = static_cast<int>(aSignum);
    write(sDumpAboutMemoryPipeWriteFd, &signum, sizeof(signum));
  }
}

class SignalPipeWatcher : public MessageLoopForIO::Watcher
{
public:
  SignalPipeWatcher()
  {}

  ~SignalPipeWatcher()
  {
    
    
    
    int pipeWriteFd = sDumpAboutMemoryPipeWriteFd;
    PR_ATOMIC_SET(&sDumpAboutMemoryPipeWriteFd, 0);

    
    mReadWatcher.StopWatchingFileDescriptor();

    close(pipeWriteFd);
    close(mPipeReadFd);
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SignalPipeWatcher)

  bool Start()
  {
    MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

    sDumpAboutMemorySignum = SIGRTMIN;
    sDumpAboutMemoryAfterMMUSignum = SIGRTMIN + 1;
    sGCAndCCDumpSignum = SIGRTMIN + 2;

    
    
    int pipeFds[2];
    if (pipe(pipeFds)) {
      NS_WARNING("Failed to create pipe.");
      return false;
    }

    
    fcntl(pipeFds[0], F_SETFD, FD_CLOEXEC);
    fcntl(pipeFds[1], F_SETFD, FD_CLOEXEC);

    mPipeReadFd = pipeFds[0];
    sDumpAboutMemoryPipeWriteFd = pipeFds[1];

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    action.sa_handler = DumpAboutMemorySignalHandler;

    if (sigaction(sDumpAboutMemorySignum, &action, nullptr)) {
      NS_WARNING("Failed to register about:memory dump signal handler.");
    }
    if (sigaction(sDumpAboutMemoryAfterMMUSignum, &action, nullptr)) {
      NS_WARNING("Failed to register about:memory dump after MMU signal handler.");
    }
    if (sigaction(sGCAndCCDumpSignum, &action, nullptr)) {
      NS_WARNING("Failed to register GC+CC dump signal handler.");
    }

    
    return MessageLoopForIO::current()->WatchFileDescriptor(
      mPipeReadFd,  true,
      MessageLoopForIO::WATCH_READ,
      &mReadWatcher, this);
  }

  virtual void OnFileCanReadWithoutBlocking(int aFd)
  {
    MOZ_ASSERT(XRE_GetIOMessageLoop() == MessageLoopForIO::current());

    uint8_t signum;
    ssize_t numReceived = read(aFd, &signum, sizeof(signum));
    if (numReceived != sizeof(signum)) {
      NS_WARNING("Error reading from buffer in "
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
      NS_WARNING("Got unexpected signum.");
    }
  }

  virtual void OnFileCanWriteWithoutBlocking(int aFd)
  {}

private:
  int mPipeReadFd;
  MessageLoopForIO::FileDescriptorWatcher mReadWatcher;
};

StaticRefPtr<SignalPipeWatcher> sSignalPipeWatcher;

void
InitializeSignalWatcher()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!sSignalPipeWatcher);

  sSignalPipeWatcher = new SignalPipeWatcher();
  ClearOnShutdown(&sSignalPipeWatcher);

  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableMethod(sSignalPipeWatcher.get(),
                        &SignalPipeWatcher::Start));
}

} 
#endif 

} 

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
  InitializeSignalWatcher();
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

static nsresult
OpenTempFile(const nsACString &aFilename, nsIFile* *aFile)
{
  nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, aFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> file(*aFile);

  rv = file->AppendNative(aFilename);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0644);
  NS_ENSURE_SUCCESS(rv, rv);
#ifdef ANDROID
  {
    
    
    
    
    nsAutoCString path;
    rv = file->GetNativePath(path);
    if (NS_SUCCEEDED(rv)) {
      chmod(path.get(), 0644);
    }
  }
#endif
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

  rv = writer->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

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
