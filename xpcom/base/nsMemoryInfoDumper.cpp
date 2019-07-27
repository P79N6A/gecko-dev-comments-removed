





#include "mozilla/JSONWriter.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/nsMemoryInfoDumper.h"
#include "nsDumpUtils.h"

#include "mozilla/unused.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ContentChild.h"
#include "nsIConsoleService.h"
#include "nsCycleCollector.h"
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
#ifndef getpid
#define getpid _getpid
#endif
#else
#include <unistd.h>
#endif

#ifdef XP_UNIX
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
                                  bool aAnonymize, bool aMinimizeMemoryUsage)
    : mIdentifier(aIdentifier)
    , mAnonymize(aAnonymize)
    , mMinimizeMemoryUsage(aMinimizeMemoryUsage)
  {
  }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIMemoryInfoDumper> dumper =
      do_GetService("@mozilla.org/memory-info-dumper;1");
    dumper->DumpMemoryInfoToTempDir(mIdentifier, mAnonymize,
                                    mMinimizeMemoryUsage);
    return NS_OK;
  }

private:
  const nsString mIdentifier;
  const bool mAnonymize;
  const bool mMinimizeMemoryUsage;
};

class GCAndCCLogDumpRunnable MOZ_FINAL
  : public nsRunnable
  , public nsIDumpGCAndCCLogsCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  GCAndCCLogDumpRunnable(const nsAString& aIdentifier,
                         bool aDumpAllTraces,
                         bool aDumpChildProcesses)
    : mIdentifier(aIdentifier)
    , mDumpAllTraces(aDumpAllTraces)
    , mDumpChildProcesses(aDumpChildProcesses)
  {
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    nsCOMPtr<nsIMemoryInfoDumper> dumper =
      do_GetService("@mozilla.org/memory-info-dumper;1");

    dumper->DumpGCAndCCLogsToFile(mIdentifier, mDumpAllTraces,
                                  mDumpChildProcesses, this);
    return NS_OK;
  }

  NS_IMETHOD OnDump(nsIFile* aGCLog, nsIFile* aCCLog, bool aIsParent) MOZ_OVERRIDE
  {
    return NS_OK;
  }

  NS_IMETHOD OnFinish() MOZ_OVERRIDE
  {
    return NS_OK;
  }

private:
  ~GCAndCCLogDumpRunnable() {}

  const nsString mIdentifier;
  const bool mDumpAllTraces;
  const bool mDumpChildProcesses;
};

NS_IMPL_ISUPPORTS_INHERITED(GCAndCCLogDumpRunnable, nsRunnable,
                            nsIDumpGCAndCCLogsCallback)

} 

#if defined(MOZ_SUPPORTS_RT_SIGNALS) 
namespace {



























static uint8_t sDumpAboutMemorySignum;         
static uint8_t sDumpAboutMemoryAfterMMUSignum; 
static uint8_t sGCAndCCDumpSignum;             

void doMemoryReport(const uint8_t aRecvSig)
{
  
  bool minimize = aRecvSig == sDumpAboutMemoryAfterMMUSignum;
  LOG("SignalWatcher(sig %d) dispatching memory report runnable.", aRecvSig);
  nsRefPtr<DumpMemoryInfoToTempDirRunnable> runnable =
    new DumpMemoryInfoToTempDirRunnable( EmptyString(),
                                         false,
                                        minimize);
  NS_DispatchToMainThread(runnable);
}

void doGCCCDump(const uint8_t aRecvSig)
{
  LOG("SignalWatcher(sig %d) dispatching GC/CC log runnable.", aRecvSig);
  
  nsRefPtr<GCAndCCLogDumpRunnable> runnable =
    new GCAndCCLogDumpRunnable( EmptyString(),
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
  bool minimize = aInputStr.EqualsLiteral("minimize memory report");
  LOG("FifoWatcher(command:%s) dispatching memory report runnable.",
      aInputStr.get());
  nsRefPtr<DumpMemoryInfoToTempDirRunnable> runnable =
    new DumpMemoryInfoToTempDirRunnable( EmptyString(),
                                         false,
                                        minimize);
  NS_DispatchToMainThread(runnable);
}

void
doGCCCDump(const nsCString& aInputStr)
{
  bool doAllTracesGCCCDump = aInputStr.EqualsLiteral("gc log");
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




class nsDumpGCAndCCLogsCallbackHolder MOZ_FINAL
  : public nsIDumpGCAndCCLogsCallback
{
public:
  NS_DECL_ISUPPORTS

  explicit nsDumpGCAndCCLogsCallbackHolder(nsIDumpGCAndCCLogsCallback* aCallback)
    : mCallback(aCallback)
  {
  }

  NS_IMETHODIMP OnFinish()
  {
    return NS_ERROR_UNEXPECTED;
  }

  NS_IMETHODIMP OnDump(nsIFile* aGCLog, nsIFile* aCCLog, bool aIsParent)
  {
    return mCallback->OnDump(aGCLog, aCCLog, aIsParent);
  }

private:
  ~nsDumpGCAndCCLogsCallbackHolder()
  {
    unused << mCallback->OnFinish();
  }

  nsCOMPtr<nsIDumpGCAndCCLogsCallback> mCallback;
};

NS_IMPL_ISUPPORTS(nsDumpGCAndCCLogsCallbackHolder, nsIDumpGCAndCCLogsCallback)

NS_IMETHODIMP
nsMemoryInfoDumper::DumpGCAndCCLogsToFile(const nsAString& aIdentifier,
                                          bool aDumpAllTraces,
                                          bool aDumpChildProcesses,
                                          nsIDumpGCAndCCLogsCallback* aCallback)
{
  nsString identifier(aIdentifier);
  EnsureNonEmptyIdentifier(identifier);
  nsCOMPtr<nsIDumpGCAndCCLogsCallback> callbackHolder =
    new nsDumpGCAndCCLogsCallbackHolder(aCallback);

  if (aDumpChildProcesses) {
    nsTArray<ContentParent*> children;
    ContentParent::GetAll(children);
    for (uint32_t i = 0; i < children.Length(); i++) {
      ContentParent* cp = children[i];
      nsCOMPtr<nsICycleCollectorLogSink> logSink =
        nsCycleCollector_createLogSink();

      logSink->SetFilenameIdentifier(identifier);
      logSink->SetProcessIdentifier(cp->Pid());

      unused << cp->CycleCollectWithLogs(aDumpAllTraces, logSink,
                                         callbackHolder);
    }
  }

  nsCOMPtr<nsICycleCollectorListener> logger =
    do_CreateInstance("@mozilla.org/cycle-collector-logger;1");

  if (aDumpAllTraces) {
    nsCOMPtr<nsICycleCollectorListener> allTracesLogger;
    logger->AllTraces(getter_AddRefs(allTracesLogger));
    logger = allTracesLogger;
  }

  nsCOMPtr<nsICycleCollectorLogSink> logSink;
  logger->GetLogSink(getter_AddRefs(logSink));

  logSink->SetFilenameIdentifier(identifier);

  nsJSContext::CycleCollectNow(logger);

  nsCOMPtr<nsIFile> gcLog, ccLog;
  logSink->GetGcLog(getter_AddRefs(gcLog));
  logSink->GetCcLog(getter_AddRefs(ccLog));
  callbackHolder->OnDump(gcLog, ccLog,  true);

  return NS_OK;
}

NS_IMETHODIMP
nsMemoryInfoDumper::DumpGCAndCCLogsToSink(bool aDumpAllTraces,
                                          nsICycleCollectorLogSink* aSink)
{
  nsCOMPtr<nsICycleCollectorListener> logger =
    do_CreateInstance("@mozilla.org/cycle-collector-logger;1");

  if (aDumpAllTraces) {
    nsCOMPtr<nsICycleCollectorListener> allTracesLogger;
    logger->AllTraces(getter_AddRefs(allTracesLogger));
    logger = allTracesLogger;
  }

  logger->SetLogSink(aSink);

  nsJSContext::CycleCollectNow(logger);

  return NS_OK;
}

static void
MakeFilename(const char* aPrefix, const nsAString& aIdentifier,
             int aPid, const char* aSuffix, nsACString& aResult)
{
  aResult = nsPrintfCString("%s-%s-%d.%s",
                            aPrefix,
                            NS_ConvertUTF16toUTF8(aIdentifier).get(),
                            aPid, aSuffix);
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





class GZWriterWrapper : public JSONWriteFunc
{
public:
  explicit GZWriterWrapper(nsGZFileWriter* aGZWriter)
    : mGZWriter(aGZWriter)
  {}

  void Write(const char* aStr)
  {
    (void)mGZWriter->Write(aStr);
  }

  nsresult Finish() { return mGZWriter->Finish(); }

private:
  nsRefPtr<nsGZFileWriter> mGZWriter;
};




class HandleReportAndFinishReportingCallbacks MOZ_FINAL
  : public nsIHandleReportCallback, public nsIFinishReportingCallback
{
public:
  NS_DECL_ISUPPORTS

  HandleReportAndFinishReportingCallbacks(UniquePtr<JSONWriter> aWriter,
                                          nsIFinishDumpingCallback* aFinishDumping,
                                          nsISupports* aFinishDumpingData)
    : mWriter(Move(aWriter))
    , mFinishDumping(aFinishDumping)
    , mFinishDumpingData(aFinishDumpingData)
  {
  }

  
  NS_IMETHOD Callback(const nsACString& aProcess, const nsACString& aPath,
                      int32_t aKind, int32_t aUnits, int64_t aAmount,
                      const nsACString& aDescription,
                      nsISupports* aData)
  {
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

    mWriter->StartObjectElement();
    {
      mWriter->StringProperty("process", process.get());
      mWriter->StringProperty("path", PromiseFlatCString(aPath).get());
      mWriter->IntProperty("kind", aKind);
      mWriter->IntProperty("units", aUnits);
      mWriter->IntProperty("amount", aAmount);
      mWriter->StringProperty("description",
                              PromiseFlatCString(aDescription).get());
    }
    mWriter->EndObject();

    return NS_OK;
  }

  
  NS_IMETHOD Callback(nsISupports* aData)
  {
    mWriter->EndArray();  
    mWriter->End();

    
    
    
    
    
    nsresult rv = static_cast<GZWriterWrapper*>(mWriter->WriteFunc())->Finish();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mFinishDumping) {
      return NS_OK;
    }

    return mFinishDumping->Callback(mFinishDumpingData);
  }

private:
  ~HandleReportAndFinishReportingCallbacks() {}

  UniquePtr<JSONWriter> mWriter;
  nsCOMPtr<nsIFinishDumpingCallback> mFinishDumping;
  nsCOMPtr<nsISupports> mFinishDumpingData;
};

NS_IMPL_ISUPPORTS(HandleReportAndFinishReportingCallbacks,
                  nsIHandleReportCallback, nsIFinishReportingCallback)

class TempDirFinishCallback MOZ_FINAL : public nsIFinishDumpingCallback
{
public:
  NS_DECL_ISUPPORTS

  TempDirFinishCallback(nsIFile* aReportsTmpFile,
                        const nsCString& aReportsFinalFilename)
    : mReportsTmpFile(aReportsTmpFile)
    , mReportsFilename(aReportsFinalFilename)
  {
  }

  NS_IMETHOD Callback(nsISupports* aData)
  {
    
    

    nsCOMPtr<nsIFile> reportsFinalFile;
    nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR,
                                         getter_AddRefs(reportsFinalFile));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

  #ifdef ANDROID
    rv = reportsFinalFile->AppendNative(NS_LITERAL_CSTRING("memory-reports"));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  #endif

    rv = reportsFinalFile->AppendNative(mReportsFilename);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = reportsFinalFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsAutoString reportsFinalFilename;
    rv = reportsFinalFile->GetLeafName(reportsFinalFilename);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = mReportsTmpFile->MoveTo( nullptr,
                                 reportsFinalFilename);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    

    nsCOMPtr<nsIConsoleService> cs =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsString path;
    mReportsTmpFile->GetPath(path);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsString msg = NS_LITERAL_STRING("nsIMemoryInfoDumper dumped reports to ");
    msg.Append(path);
    return cs->LogStringMessage(msg.get());
  }

private:
  ~TempDirFinishCallback() {}

  nsCOMPtr<nsIFile> mReportsTmpFile;
  nsCString mReportsFilename;
};

NS_IMPL_ISUPPORTS(TempDirFinishCallback, nsIFinishDumpingCallback)

static nsresult
DumpMemoryInfoToFile(
  nsIFile* aReportsFile,
  nsIFinishDumpingCallback* aFinishDumping,
  nsISupports* aFinishDumpingData,
  bool aAnonymize,
  bool aMinimizeMemoryUsage,
  nsAString& aDMDIdentifier)
{
  nsRefPtr<nsGZFileWriter> gzWriter = new nsGZFileWriter();
  nsresult rv = gzWriter->Init(aReportsFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  auto jsonWriter =
    MakeUnique<JSONWriter>(MakeUnique<GZWriterWrapper>(gzWriter));

  nsCOMPtr<nsIMemoryReporterManager> mgr =
    do_GetService("@mozilla.org/memory-reporter-manager;1");

  
  
  jsonWriter->Start();
  {
    
    jsonWriter->IntProperty("version", 1);
    jsonWriter->BoolProperty("hasMozMallocUsableSize",
                             mgr->GetHasMozMallocUsableSize());
    jsonWriter->StartArrayProperty("reports");
  }

  nsRefPtr<HandleReportAndFinishReportingCallbacks>
    handleReportAndFinishReporting =
      new HandleReportAndFinishReportingCallbacks(Move(jsonWriter),
                                                  aFinishDumping,
                                                  aFinishDumpingData);
  rv = mgr->GetReportsExtended(handleReportAndFinishReporting, nullptr,
                               handleReportAndFinishReporting, nullptr,
                               aAnonymize,
                               aMinimizeMemoryUsage,
                               aDMDIdentifier);
  return rv;
}

NS_IMETHODIMP
nsMemoryInfoDumper::DumpMemoryReportsToNamedFile(
  const nsAString& aFilename,
  nsIFinishDumpingCallback* aFinishDumping,
  nsISupports* aFinishDumpingData,
  bool aAnonymize)
{
  MOZ_ASSERT(!aFilename.IsEmpty());

  

  nsCOMPtr<nsIFile> reportsFile;
  nsresult rv = NS_NewLocalFile(aFilename, false, getter_AddRefs(reportsFile));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  reportsFile->InitWithPath(aFilename);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  bool exists;
  rv = reportsFile->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    rv = reportsFile->Create(nsIFile::NORMAL_FILE_TYPE, 0644);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  nsString dmdIdent = EmptyString();
  return DumpMemoryInfoToFile(reportsFile, aFinishDumping, aFinishDumpingData,
                              aAnonymize,  false,
                              dmdIdent);
}

NS_IMETHODIMP
nsMemoryInfoDumper::DumpMemoryInfoToTempDir(const nsAString& aIdentifier,
                                            bool aAnonymize,
                                            bool aMinimizeMemoryUsage)
{
  nsString identifier(aIdentifier);
  EnsureNonEmptyIdentifier(identifier);

  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  nsCString reportsFinalFilename;
  MakeFilename("unified-memory-report", identifier, getpid(), "json.gz",
               reportsFinalFilename);

  nsCOMPtr<nsIFile> reportsTmpFile;
  nsresult rv;
  
  
  
  rv = nsDumpUtils::OpenTempFile(NS_LITERAL_CSTRING("incomplete-") +
                                 reportsFinalFilename,
                                 getter_AddRefs(reportsTmpFile),
                                 NS_LITERAL_CSTRING("memory-reports"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  nsRefPtr<TempDirFinishCallback> finishDumping =
    new TempDirFinishCallback(reportsTmpFile, reportsFinalFilename);

  return DumpMemoryInfoToFile(reportsTmpFile, finishDumping, nullptr,
                              aAnonymize, aMinimizeMemoryUsage, identifier);
}

#ifdef MOZ_DMD
nsresult
nsMemoryInfoDumper::OpenDMDFile(const nsAString& aIdentifier, int aPid,
                                FILE** aOutFile)
{
  if (!dmd::IsRunning()) {
    *aOutFile = nullptr;
    return NS_OK;
  }

  
  
  nsCString dmdFilename;
  MakeFilename("dmd", aIdentifier, aPid, "txt.gz", dmdFilename);

  
  
  
  

  nsresult rv;
  nsCOMPtr<nsIFile> dmdFile;
  rv = nsDumpUtils::OpenTempFile(dmdFilename,
                                 getter_AddRefs(dmdFile),
                                 NS_LITERAL_CSTRING("memory-reports"));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  rv = dmdFile->OpenANSIFileDesc("wb", aOutFile);
  NS_WARN_IF(NS_FAILED(rv));

  
  nsCString path;
  rv = dmdFile->GetNativePath(path);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  dmd::StatusMsg("opened %s for writing\n", path.get());

  return rv;
}

nsresult
nsMemoryInfoDumper::DumpDMDToFile(FILE* aFile)
{
  nsRefPtr<nsGZFileWriter> dmdWriter = new nsGZFileWriter();
  nsresult rv = dmdWriter->InitANSIFileDesc(aFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  DMDWriteState state(dmdWriter);
  dmd::Writer w(DMDWrite, &state);
  dmd::AnalyzeReports(w);

  rv = dmdWriter->Finish();
  NS_WARN_IF(NS_FAILED(rv));
  return rv;
}
#endif  

