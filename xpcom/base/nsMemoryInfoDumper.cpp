





#include "mozilla/nsMemoryInfoDumper.h"
#include "nsDumpUtils.h"

#include "mozilla/unused.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ContentChild.h"
#include "nsIConsoleService.h"
#include "nsICycleCollectorListener.h"
#include "nsIMemoryReporter.h"
#include "nsDirectoryServiceDefs.h"
#include "nsGZFileWriter.h"
#include "nsJSEnvironment.h"
#include "nsPrintfCString.h"
#include "nsISimpleEnumerator.h"
#include "nsServiceManagerUtils.h"
#include "nsIFile.h"

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#if defined(XP_LINUX) || defined(__FreeBSD__) || defined(XP_MACOSX)
#define MOZ_SUPPORTS_FIFO 1
#endif

#if defined(XP_LINUX) || defined(__FreeBSD__)
#define MOZ_SUPPORTS_RT_SIGNALS 1
#endif

#if defined(MOZ_SUPPORTS_RT_SIGNALS)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#if defined(MOZ_SUPPORTS_FIFO)
#include "mozilla/Preferences.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;

namespace {

class DumpMemoryInfoToTempDirRunnable : public nsRunnable
{
public:
  DumpMemoryInfoToTempDirRunnable(const nsAString& aIdentifier,
                                  bool aMinimizeMemoryUsage)
    : mIdentifier(aIdentifier)
    , mMinimizeMemoryUsage(aMinimizeMemoryUsage)
  {
  }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIMemoryInfoDumper> dumper =
      do_GetService("@mozilla.org/memory-info-dumper;1");
    dumper->DumpMemoryInfoToTempDir(mIdentifier, mMinimizeMemoryUsage);
    return NS_OK;
  }

private:
  const nsString mIdentifier;
  const bool mMinimizeMemoryUsage;
};

class GCAndCCLogDumpRunnable : public nsRunnable
{
public:
  GCAndCCLogDumpRunnable(const nsAString& aIdentifier,
                         bool aDumpAllTraces,
                         bool aDumpChildProcesses)
    : mIdentifier(aIdentifier)
    , mDumpAllTraces(aDumpAllTraces)
    , mDumpChildProcesses(aDumpChildProcesses)
  {
  }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIMemoryInfoDumper> dumper =
      do_GetService("@mozilla.org/memory-info-dumper;1");

    nsString ccLogPath, gcLogPath;
    dumper->DumpGCAndCCLogsToFile(mIdentifier, mDumpAllTraces,
                                  mDumpChildProcesses, gcLogPath, ccLogPath);
    return NS_OK;
  }

private:
  const nsString mIdentifier;
  const bool mDumpAllTraces;
  const bool mDumpChildProcesses;
};

} 

#if defined(MOZ_SUPPORTS_RT_SIGNALS) 
namespace {



























static uint8_t sDumpAboutMemorySignum;         
static uint8_t sDumpAboutMemoryAfterMMUSignum; 
static uint8_t sGCAndCCDumpSignum;             

void doMemoryReport(const uint8_t recvSig)
{
  
  bool doMMUFirst = recvSig == sDumpAboutMemoryAfterMMUSignum;
  LOG("SignalWatcher(sig %d) dispatching memory report runnable.", recvSig);
  nsRefPtr<DumpMemoryInfoToTempDirRunnable> runnable =
    new DumpMemoryInfoToTempDirRunnable( EmptyString(),
                                        doMMUFirst);
  NS_DispatchToMainThread(runnable);
}

void doGCCCDump(const uint8_t recvSig)
{
  LOG("SignalWatcher(sig %d) dispatching GC/CC log runnable.", recvSig);
  
  nsRefPtr<GCAndCCLogDumpRunnable> runnable =
    new GCAndCCLogDumpRunnable(
       EmptyString(),
       true,
       true);
  NS_DispatchToMainThread(runnable);
}

} 
#endif 

#if defined(MOZ_SUPPORTS_FIFO) 
namespace {

void
doMemoryReport(const nsCString& aInputStr)
{
  bool doMMUMemoryReport = aInputStr == NS_LITERAL_CSTRING("minimize memory report");
  LOG("FifoWatcher(command:%s) dispatching memory report runnable.", aInputStr.get());
  nsRefPtr<DumpMemoryInfoToTempDirRunnable> runnable =
    new DumpMemoryInfoToTempDirRunnable( EmptyString(),
                                        doMMUMemoryReport);
  NS_DispatchToMainThread(runnable);
}

void
doGCCCDump(const nsCString& aInputStr)
{
  bool doAllTracesGCCCDump = aInputStr == NS_LITERAL_CSTRING("gc log");
  LOG("FifoWatcher(command:%s) dispatching GC/CC log runnable.", aInputStr.get());
  nsRefPtr<GCAndCCLogDumpRunnable> runnable =
    new GCAndCCLogDumpRunnable( EmptyString(),
                               doAllTracesGCCCDump,
                                true);
  NS_DispatchToMainThread(runnable);
}

bool
SetupFifo()
{
  static bool fifoCallbacksRegistered = false;

  if (!FifoWatcher::MaybeCreate()) {
    return false;
  }

  MOZ_ASSERT(!fifoCallbacksRegistered,
             "FifoWatcher callbacks should be registered only once");

  FifoWatcher* fw = FifoWatcher::GetSingleton();
  
  fw->RegisterCallback(NS_LITERAL_CSTRING("memory report"),
                       doMemoryReport);
  fw->RegisterCallback(NS_LITERAL_CSTRING("minimize memory report"),
                       doMemoryReport);
  
  fw->RegisterCallback(NS_LITERAL_CSTRING("gc log"),
                       doGCCCDump);
  fw->RegisterCallback(NS_LITERAL_CSTRING("abbreviated gc log"),
                       doGCCCDump);

  fifoCallbacksRegistered = true;
  return true;
}

void
OnFifoEnabledChange(const char* , void* )
{
  LOG("%s changed", FifoWatcher::kPrefName);
  if (SetupFifo()) {
    Preferences::UnregisterCallback(OnFifoEnabledChange,
                                    FifoWatcher::kPrefName,
                                    nullptr);
  }
}

} 
#endif 

NS_IMPL_ISUPPORTS(nsMemoryInfoDumper, nsIMemoryInfoDumper)

nsMemoryInfoDumper::nsMemoryInfoDumper()
{
}

nsMemoryInfoDumper::~nsMemoryInfoDumper()
{
}

 void
nsMemoryInfoDumper::Initialize()
{
#if defined(MOZ_SUPPORTS_RT_SIGNALS)
  SignalPipeWatcher* sw = SignalPipeWatcher::GetSingleton();

  
  sDumpAboutMemorySignum = SIGRTMIN;
  sw->RegisterCallback(sDumpAboutMemorySignum, doMemoryReport);
  
  sDumpAboutMemoryAfterMMUSignum = SIGRTMIN + 1;
  sw->RegisterCallback(sDumpAboutMemoryAfterMMUSignum, doMemoryReport);
  
  sGCAndCCDumpSignum = SIGRTMIN + 2;
  sw->RegisterCallback(sGCAndCCDumpSignum, doGCCCDump);
#endif

#if defined(MOZ_SUPPORTS_FIFO)
  if (!SetupFifo()) {
    
    
    
    
    Preferences::RegisterCallback(OnFifoEnabledChange,
                                  FifoWatcher::kPrefName,
                                  nullptr);
  }
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
nsMemoryInfoDumper::DumpGCAndCCLogsToFile(const nsAString& aIdentifier,
                                          bool aDumpAllTraces,
                                          bool aDumpChildProcesses,
                                          nsAString& aGCLogPath,
                                          nsAString& aCCLogPath)
{
  nsString identifier(aIdentifier);
  EnsureNonEmptyIdentifier(identifier);

  if (aDumpChildProcesses) {
    nsTArray<ContentParent*> children;
    ContentParent::GetAll(children);
    for (uint32_t i = 0; i < children.Length(); i++) {
      unused << children[i]->SendDumpGCAndCCLogsToFile(
        identifier, aDumpAllTraces, aDumpChildProcesses);
    }
  }

  nsCOMPtr<nsICycleCollectorListener> logger =
    do_CreateInstance("@mozilla.org/cycle-collector-logger;1");
  logger->SetFilenameIdentifier(identifier);

  if (aDumpAllTraces) {
    nsCOMPtr<nsICycleCollectorListener> allTracesLogger;
    logger->AllTraces(getter_AddRefs(allTracesLogger));
    logger = allTracesLogger;
  }

  nsJSContext::CycleCollectNow(logger);

  logger->GetGcLogPath(aGCLogPath);
  logger->GetCcLogPath(aCCLogPath);

  return NS_OK;
}

namespace mozilla {

#define DUMP(o, s) \
  do { \
    nsresult rv = (o)->Write(s); \
    if (NS_WARN_IF(NS_FAILED(rv))) \
      return rv; \
  } while (0)

class DumpReportCallback MOZ_FINAL : public nsIHandleReportCallback
{
public:
  NS_DECL_ISUPPORTS

  DumpReportCallback(nsGZFileWriter* aWriter)
    : mIsFirst(true)
    , mWriter(aWriter)
  {
  }

  NS_IMETHOD Callback(const nsACString& aProcess, const nsACString& aPath,
                      int32_t aKind, int32_t aUnits, int64_t aAmount,
                      const nsACString& aDescription,
                      nsISupports* aData)
  {
    if (mIsFirst) {
      DUMP(mWriter, "[");
      mIsFirst = false;
    } else {
      DUMP(mWriter, ",");
    }

    nsAutoCString process;
    if (aProcess.IsEmpty()) {
      
      
      
      
      
      if (XRE_GetProcessType() == GeckoProcessType_Default) {
        
        process.AssignLiteral("Main Process");
      } else if (ContentChild* cc = ContentChild::GetSingleton()) {
        
        cc->GetProcessName(process);
      }
      ContentChild::AppendProcessId(process);

    } else {
      
      
      process = aProcess;
    }

    DUMP(mWriter, "\n    {\"process\": \"");
    DUMP(mWriter, process);

    DUMP(mWriter, "\", \"path\": \"");
    nsCString path(aPath);
    path.ReplaceSubstring("\\", "\\\\");    
    path.ReplaceSubstring("\"", "\\\"");    
    DUMP(mWriter, path);

    DUMP(mWriter, "\", \"kind\": ");
    DUMP(mWriter, nsPrintfCString("%d", aKind));

    DUMP(mWriter, ", \"units\": ");
    DUMP(mWriter, nsPrintfCString("%d", aUnits));

    DUMP(mWriter, ", \"amount\": ");
    DUMP(mWriter, nsPrintfCString("%lld", aAmount));

    nsCString description(aDescription);
    description.ReplaceSubstring("\\", "\\\\");    
    description.ReplaceSubstring("\"", "\\\"");    
    description.ReplaceSubstring("\n", "\\n");     
    DUMP(mWriter, ", \"description\": \"");
    DUMP(mWriter, description);
    DUMP(mWriter, "\"}");

    return NS_OK;
  }

private:
  bool mIsFirst;
  nsRefPtr<nsGZFileWriter> mWriter;
};

NS_IMPL_ISUPPORTS(DumpReportCallback, nsIHandleReportCallback)

} 

static void
MakeFilename(const char* aPrefix, const nsAString& aIdentifier,
             const char* aSuffix, nsACString& aResult)
{
  aResult = nsPrintfCString("%s-%s-%d.%s",
                            aPrefix,
                            NS_ConvertUTF16toUTF8(aIdentifier).get(),
                            getpid(), aSuffix);
}

#ifdef MOZ_DMD
struct DMDWriteState
{
  static const size_t kBufSize = 4096;
  char mBuf[kBufSize];
  nsRefPtr<nsGZFileWriter> mGZWriter;

  DMDWriteState(nsGZFileWriter* aGZWriter)
    : mGZWriter(aGZWriter)
  {
  }
};

static void
DMDWrite(void* aState, const char* aFmt, va_list ap)
{
  DMDWriteState* state = (DMDWriteState*)aState;
  vsnprintf(state->mBuf, state->kBufSize, aFmt, ap);
  unused << state->mGZWriter->Write(state->mBuf);
}
#endif

static nsresult
DumpHeader(nsIGZFileWriter* aWriter)
{
  
  
  
  
  
  DUMP(aWriter, "{\n  \"version\": 1,\n");

  DUMP(aWriter, "  \"hasMozMallocUsableSize\": ");

  nsCOMPtr<nsIMemoryReporterManager> mgr =
    do_GetService("@mozilla.org/memory-reporter-manager;1");
  if (NS_WARN_IF(!mgr)) {
    return NS_ERROR_UNEXPECTED;
  }

  DUMP(aWriter, mgr->GetHasMozMallocUsableSize() ? "true" : "false");
  DUMP(aWriter, ",\n");
  DUMP(aWriter, "  \"reports\": ");

  return NS_OK;
}

static nsresult
DumpFooter(nsIGZFileWriter* aWriter)
{
  DUMP(aWriter, "\n  ]\n}\n");

  return NS_OK;
}

class TempDirMemoryFinishCallback MOZ_FINAL : public nsIFinishReportingCallback
{
public:
  NS_DECL_ISUPPORTS

  TempDirMemoryFinishCallback(nsGZFileWriter* aWriter,
                              nsIFile* aTmpFile,
                              const nsCString& aFilename,
                              const nsString& aIdentifier)
    : mrWriter(aWriter)
    , mrTmpFile(aTmpFile)
    , mrFilename(aFilename)
    , mIdentifier(aIdentifier)
  {
  }

  NS_IMETHOD Callback(nsISupports* aData);

private:
  nsRefPtr<nsGZFileWriter> mrWriter;
  nsCOMPtr<nsIFile> mrTmpFile;
  nsCString mrFilename;
  nsString mIdentifier;
};

NS_IMPL_ISUPPORTS(TempDirMemoryFinishCallback, nsIFinishReportingCallback)

NS_IMETHODIMP
nsMemoryInfoDumper::DumpMemoryInfoToTempDir(const nsAString& aIdentifier,
                                            bool aMinimizeMemoryUsage)
{
  nsString identifier(aIdentifier);
  EnsureNonEmptyIdentifier(identifier);

#ifdef MOZ_DMD
  
  
  dmd::ClearReports();
#endif

  
  
  
  
  
  
  
  
  
  

  
  
  nsCString mrFilename;
  
  
  
  
  
  MakeFilename("unified-memory-report", identifier, "json.gz", mrFilename);

  nsCOMPtr<nsIFile> mrTmpFile;
  nsresult rv;
  
  
  
  rv = nsDumpUtils::OpenTempFile(NS_LITERAL_CSTRING("incomplete-") +
                                 mrFilename,
                                 getter_AddRefs(mrTmpFile),
                                 NS_LITERAL_CSTRING("memory-reports"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsRefPtr<nsGZFileWriter> mrWriter = new nsGZFileWriter();
  rv = mrWriter->Init(mrTmpFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  rv = DumpHeader(mrWriter);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  nsCOMPtr<nsIMemoryReporterManager> mgr =
    do_GetService("@mozilla.org/memory-reporter-manager;1");
  nsRefPtr<DumpReportCallback> dumpReport = new DumpReportCallback(mrWriter);
  nsRefPtr<nsIFinishReportingCallback> finishReport =
    new TempDirMemoryFinishCallback(mrWriter, mrTmpFile, mrFilename, identifier);
  rv = mgr->GetReportsExtended(dumpReport, nullptr,
                               finishReport, nullptr,
                               aMinimizeMemoryUsage,
                               identifier);
  return rv;
}

#ifdef MOZ_DMD
nsresult
nsMemoryInfoDumper::DumpDMD(const nsAString& aIdentifier)
{
  if (!dmd::IsEnabled()) {
    return NS_OK;
  }

  nsresult rv;

  
  
  nsCString dmdFilename;
  MakeFilename("dmd", aIdentifier, "txt.gz", dmdFilename);

  
  
  
  

  nsCOMPtr<nsIFile> dmdFile;
  rv = nsDumpUtils::OpenTempFile(dmdFilename,
                                 getter_AddRefs(dmdFile),
                                 NS_LITERAL_CSTRING("memory-reports"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsRefPtr<nsGZFileWriter> dmdWriter = new nsGZFileWriter();
  rv = dmdWriter->Init(dmdFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  

  DMDWriteState state(dmdWriter);
  dmd::Writer w(DMDWrite, &state);
  dmd::Dump(w);

  rv = dmdWriter->Finish();
  NS_WARN_IF(NS_FAILED(rv));
  return rv;
}
#endif  

NS_IMETHODIMP
TempDirMemoryFinishCallback::Callback(nsISupports* aData)
{
  nsresult rv = DumpFooter(mrWriter);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  
  
  
  
  rv = mrWriter->Finish();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  

  nsCOMPtr<nsIFile> mrFinalFile;
  rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(mrFinalFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

#ifdef ANDROID
  rv = mrFinalFile->AppendNative(NS_LITERAL_CSTRING("memory-reports"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
#endif

  rv = mrFinalFile->AppendNative(mrFilename);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mrFinalFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsAutoString mrActualFinalFilename;
  rv = mrFinalFile->GetLeafName(mrActualFinalFilename);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = mrTmpFile->MoveTo( nullptr, mrActualFinalFilename);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  

  nsCOMPtr<nsIConsoleService> cs =
    do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsString path;
  mrTmpFile->GetPath(path);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsString msg = NS_LITERAL_STRING("nsIMemoryInfoDumper dumped reports to ");
  msg.Append(path);
  return cs->LogStringMessage(msg.get());
}



class FinishReportingCallback MOZ_FINAL : public nsIFinishReportingCallback
{
public:
  NS_DECL_ISUPPORTS

  FinishReportingCallback(nsIFinishDumpingCallback* aFinishDumping,
                          nsISupports* aFinishDumpingData)
    : mFinishDumping(aFinishDumping)
    , mFinishDumpingData(aFinishDumpingData)
  {
  }

  NS_IMETHOD Callback(nsISupports* aData)
  {
    nsCOMPtr<nsIGZFileWriter> writer = do_QueryInterface(aData);
    NS_ENSURE_TRUE(writer, NS_ERROR_FAILURE);

    nsresult rv = DumpFooter(writer);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = writer->Finish();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mFinishDumping) {
      return NS_OK;
    }

    return mFinishDumping->Callback(mFinishDumpingData);
  }

private:
  nsCOMPtr<nsIFinishDumpingCallback> mFinishDumping;
  nsCOMPtr<nsISupports> mFinishDumpingData;
};

NS_IMPL_ISUPPORTS(FinishReportingCallback, nsIFinishReportingCallback)

NS_IMETHODIMP
nsMemoryInfoDumper::DumpMemoryReportsToNamedFile(
  const nsAString& aFilename,
  nsIFinishDumpingCallback* aFinishDumping,
  nsISupports* aFinishDumpingData)
{
  MOZ_ASSERT(!aFilename.IsEmpty());

  

  nsCOMPtr<nsIFile> mrFile;
  nsresult rv = NS_NewLocalFile(aFilename, false, getter_AddRefs(mrFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mrFile->InitWithPath(aFilename);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool exists;
  rv = mrFile->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    rv = mrFile->Create(nsIFile::NORMAL_FILE_TYPE, 0644);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  

  nsRefPtr<nsGZFileWriter> mrWriter = new nsGZFileWriter();
  rv = mrWriter->Init(mrFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  rv = DumpHeader(mrWriter);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  nsRefPtr<DumpReportCallback> dumpReport = new DumpReportCallback(mrWriter);
  nsRefPtr<FinishReportingCallback> finishReporting =
    new FinishReportingCallback(aFinishDumping, aFinishDumpingData);
  nsCOMPtr<nsIMemoryReporterManager> mgr =
    do_GetService("@mozilla.org/memory-reporter-manager;1");
  return mgr->GetReports(dumpReport, nullptr, finishReporting, mrWriter);
}

#undef DUMP
