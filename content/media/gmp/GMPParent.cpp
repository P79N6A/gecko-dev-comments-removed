




#include "GMPParent.h"
#include "prlog.h"
#include "nsComponentManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIInputStream.h"
#include "nsILineInputStream.h"
#include "nsNetUtil.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsThreadUtils.h"
#include "nsIRunnable.h"
#include "mozIGeckoMediaPluginService.h"
#include "mozilla/unused.h"
#include "nsIObserverService.h"
#include "GMPTimerParent.h"
#include "runnable_utils.h"

#include "mozilla/dom/CrashReporterParent.h"
using mozilla::dom::CrashReporterParent;

#ifdef MOZ_CRASHREPORTER
using CrashReporter::AnnotationTable;
using CrashReporter::GetIDFromMinidump;
#endif

namespace mozilla {

#ifdef LOG
#undef LOG
#endif

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetGMPLog();

#define LOGD(msg) PR_LOG(GetGMPLog(), PR_LOG_DEBUG, msg)
#define LOG(level, msg) PR_LOG(GetGMPLog(), (level), msg)
#else
#define LOGD(msg)
#define LOG(level, msg)
#endif

#ifdef __CLASS__
#undef __CLASS__
#endif
#define __CLASS__ "GMPParent"

namespace gmp {

GMPParent::GMPParent()
  : mState(GMPStateNotLoaded)
  , mProcess(nullptr)
  , mDeleteProcessOnlyOnUnload(false)
  , mAbnormalShutdownInProgress(false)
{
}

GMPParent::~GMPParent()
{
  
  MOZ_ASSERT(NS_IsMainThread());
}

void
GMPParent::CheckThread()
{
  MOZ_ASSERT(mGMPThread == NS_GetCurrentThread());
}

nsresult
GMPParent::CloneFrom(const GMPParent* aOther)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());
  MOZ_ASSERT(aOther->mDirectory && aOther->mService, "null plugin directory");
  return Init(aOther->mService, aOther->mDirectory);
}

nsresult
GMPParent::Init(GeckoMediaPluginService *aService, nsIFile* aPluginDir)
{
  MOZ_ASSERT(aPluginDir);
  MOZ_ASSERT(aService);
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  mService = aService;
  mDirectory = aPluginDir;

  nsAutoString leafname;
  nsresult rv = aPluginDir->GetLeafName(leafname);
  if (NS_FAILED(rv)) {
    return rv;
  }
  LOGD(("%s::%s: %p for %s", __CLASS__, __FUNCTION__, this,
       NS_LossyConvertUTF16toASCII(leafname).get()));

  MOZ_ASSERT(leafname.Length() > 4);
  mName = Substring(leafname, 4);

  return ReadGMPMetaData();
}

void
GMPParent::Crash()
{
  if (mState != GMPStateNotLoaded) {
    unused << SendCrashPluginNow();
  }
}

nsresult
GMPParent::LoadProcess()
{
  MOZ_ASSERT(mDirectory, "Plugin directory cannot be NULL!");
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());
  MOZ_ASSERT(mState == GMPStateNotLoaded);

  nsAutoCString path;
  if (NS_FAILED(mDirectory->GetNativePath(path))) {
    return NS_ERROR_FAILURE;
  }
  LOGD(("%s::%s: %p for %s", __CLASS__, __FUNCTION__, this, path.get()));

  if (!mProcess) {
    mProcess = new GMPProcessParent(path.get());
    if (!mProcess->Launch(30 * 1000)) {
      mProcess->Delete();
      mProcess = nullptr;
      return NS_ERROR_FAILURE;
    }

    bool opened = Open(mProcess->GetChannel(), mProcess->GetChildProcessHandle());
    if (!opened) {
      mProcess->Delete();
      mProcess = nullptr;
      return NS_ERROR_FAILURE;
    }
    LOGD(("%s::%s: Created new process %p", __CLASS__, __FUNCTION__, (void *)mProcess));
  }

  mState = GMPStateLoaded;

  return NS_OK;
}

void
GMPParent::CloseIfUnused()
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  if ((mDeleteProcessOnlyOnUnload ||
       mState == GMPStateLoaded ||
       mState == GMPStateUnloading) &&
      mVideoDecoders.IsEmpty() &&
      mVideoEncoders.IsEmpty() &&
      mDecryptors.IsEmpty() &&
      mAudioDecoders.IsEmpty()) {
    Shutdown();
  }
}

void
GMPParent::AudioDecoderDestroyed(GMPAudioDecoderParent* aDecoder)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  MOZ_ALWAYS_TRUE(mAudioDecoders.RemoveElement(aDecoder));

  
  
  nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &GMPParent::CloseIfUnused);
  NS_DispatchToCurrentThread(event);
}

void
GMPParent::CloseActive(bool aDieWhenUnloaded)
{
  LOGD(("%s::%s: %p state %d", __CLASS__, __FUNCTION__, this, mState));
  if (aDieWhenUnloaded) {
    mDeleteProcessOnlyOnUnload = true; 
  }
  if (mState == GMPStateLoaded) {
    mState = GMPStateUnloading;
  }

  
  for (uint32_t i = mVideoDecoders.Length(); i > 0; i--) {
    mVideoDecoders[i - 1]->Shutdown();
  }

  
  for (uint32_t i = mVideoEncoders.Length(); i > 0; i--) {
    mVideoEncoders[i - 1]->Shutdown();
  }

  
  for (uint32_t i = mDecryptors.Length(); i > 0; i--) {
    mDecryptors[i - 1]->Shutdown();
  }

  
  for (uint32_t i = mAudioDecoders.Length(); i > 0; i--) {
    mAudioDecoders[i - 1]->Shutdown();
  }

  for (uint32_t i = mTimers.Length(); i > 0; i--) {
    mTimers[i - 1]->Shutdown();
  }

  
  
  
  CloseIfUnused();
}

void
GMPParent::Shutdown()
{
  LOGD(("%s::%s: %p", __CLASS__, __FUNCTION__, this));
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  if (mAbnormalShutdownInProgress) {
    return;
  }
  MOZ_ASSERT(mVideoDecoders.IsEmpty() && mVideoEncoders.IsEmpty());
  if (mState == GMPStateNotLoaded || mState == GMPStateClosing) {
    return;
  }

  mState = GMPStateClosing;
  DeleteProcess();
  
  
  if (!mDeleteProcessOnlyOnUnload) {
    
    nsRefPtr<GMPParent> self(this);
    mService->ReAddOnGMPThread(self);
  } 
  MOZ_ASSERT(mState == GMPStateNotLoaded);
}

void
GMPParent::DeleteProcess()
{
  LOGD(("%s::%s: %p", __CLASS__, __FUNCTION__, this));
  
  
  MOZ_ASSERT(mState == GMPStateClosing);
  Close();
  mProcess->Delete();
  LOGD(("%s::%s: Shut down process %p", __CLASS__, __FUNCTION__, (void *) mProcess));
  mProcess = nullptr;
  mState = GMPStateNotLoaded;
}

void
GMPParent::VideoDecoderDestroyed(GMPVideoDecoderParent* aDecoder)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  
  unused << NS_WARN_IF(!mVideoDecoders.RemoveElement(aDecoder));

  if (mVideoDecoders.IsEmpty() &&
      mVideoEncoders.IsEmpty()) {
    
    
    nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &GMPParent::CloseIfUnused);
    NS_DispatchToCurrentThread(event);
  }
}

void
GMPParent::VideoEncoderDestroyed(GMPVideoEncoderParent* aEncoder)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  
  unused << NS_WARN_IF(!mVideoEncoders.RemoveElement(aEncoder));

  if (mVideoDecoders.IsEmpty() &&
      mVideoEncoders.IsEmpty()) {
    
    
    nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &GMPParent::CloseIfUnused);
    NS_DispatchToCurrentThread(event);
  }
}

void
GMPParent::DecryptorDestroyed(GMPDecryptorParent* aSession)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  MOZ_ALWAYS_TRUE(mDecryptors.RemoveElement(aSession));

  
  
  if (mDecryptors.IsEmpty()) {
    nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(this, &GMPParent::CloseIfUnused);
    NS_DispatchToCurrentThread(event);
  }
}

nsresult
GMPParent::GetGMPDecryptor(GMPDecryptorParent** aGMPDP)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  if (!EnsureProcessLoaded()) {
    return NS_ERROR_FAILURE;
  }

  PGMPDecryptorParent* pdp = SendPGMPDecryptorConstructor();
  if (!pdp) {
    return NS_ERROR_FAILURE;
  }
  GMPDecryptorParent* dp = static_cast<GMPDecryptorParent*>(pdp);
  
  
  NS_ADDREF(dp);
  mDecryptors.AppendElement(dp);
  *aGMPDP = dp;

  return NS_OK;
}

GMPState
GMPParent::State() const
{
  return mState;
}


nsIThread*
GMPParent::GMPThread()
{
  if (!mGMPThread) {
    nsCOMPtr<mozIGeckoMediaPluginService> mps = do_GetService("@mozilla.org/gecko-media-plugin-service;1");
    MOZ_ASSERT(mps);
    if (!mps) {
      return nullptr;
    }
    
    
    
    
    
    mps->GetThread(getter_AddRefs(mGMPThread));
    MOZ_ASSERT(mGMPThread);
  }

  return mGMPThread;
}

bool
GMPParent::SupportsAPI(const nsCString& aAPI, const nsCString& aTag)
{
  for (uint32_t i = 0; i < mCapabilities.Length(); i++) {
    if (!mCapabilities[i]->mAPIName.Equals(aAPI)) {
      continue;
    }
    nsTArray<nsCString>& tags = mCapabilities[i]->mAPITags;
    for (uint32_t j = 0; j < tags.Length(); j++) {
      if (tags[j].Equals(aTag)) {
        return true;
      }
    }
  }
  return false;
}

bool
GMPParent::EnsureProcessLoaded()
{
  if (mState == GMPStateLoaded) {
    return true;
  }
  if (mState == GMPStateClosing ||
      mState == GMPStateUnloading) {
    return false;
  }

  nsresult rv = LoadProcess();

  return NS_SUCCEEDED(rv);
}

nsresult
GMPParent::GetGMPAudioDecoder(GMPAudioDecoderParent** aGMPAD)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  if (!EnsureProcessLoaded()) {
    return NS_ERROR_FAILURE;
  }

  PGMPAudioDecoderParent* pvap = SendPGMPAudioDecoderConstructor();
  if (!pvap) {
    return NS_ERROR_FAILURE;
  }
  GMPAudioDecoderParent* vap = static_cast<GMPAudioDecoderParent*>(pvap);
  
  
  NS_ADDREF(vap);
  *aGMPAD = vap;
  mAudioDecoders.AppendElement(vap);

  return NS_OK;
}

nsresult
GMPParent::GetGMPVideoDecoder(GMPVideoDecoderParent** aGMPVD)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  if (!EnsureProcessLoaded()) {
    return NS_ERROR_FAILURE;
  }

  
  PGMPVideoDecoderParent* pvdp = SendPGMPVideoDecoderConstructor();
  if (!pvdp) {
    return NS_ERROR_FAILURE;
  }
  GMPVideoDecoderParent *vdp = static_cast<GMPVideoDecoderParent*>(pvdp);
  
  
  NS_ADDREF(vdp);
  *aGMPVD = vdp;
  mVideoDecoders.AppendElement(vdp);

  return NS_OK;
}

nsresult
GMPParent::GetGMPVideoEncoder(GMPVideoEncoderParent** aGMPVE)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  if (!EnsureProcessLoaded()) {
    return NS_ERROR_FAILURE;
  }

  
  PGMPVideoEncoderParent* pvep = SendPGMPVideoEncoderConstructor();
  if (!pvep) {
    return NS_ERROR_FAILURE;
  }
  GMPVideoEncoderParent *vep = static_cast<GMPVideoEncoderParent*>(pvep);
  
  
  NS_ADDREF(vep);
  *aGMPVE = vep;
  mVideoEncoders.AppendElement(vep);

  return NS_OK;
}

#ifdef MOZ_CRASHREPORTER
void
GMPParent::WriteExtraDataForMinidump(CrashReporter::AnnotationTable& notes)
{
  notes.Put(NS_LITERAL_CSTRING("GMPPlugin"), NS_LITERAL_CSTRING("1"));
  notes.Put(NS_LITERAL_CSTRING("PluginFilename"),
                               NS_ConvertUTF16toUTF8(mName));
  notes.Put(NS_LITERAL_CSTRING("PluginName"), mDisplayName);
  notes.Put(NS_LITERAL_CSTRING("PluginVersion"), mVersion);
}

void
GMPParent::GetCrashID(nsString& aResult)
{
  CrashReporterParent* cr = nullptr;
  if (ManagedPCrashReporterParent().Length() > 0) {
    cr = static_cast<CrashReporterParent*>(ManagedPCrashReporterParent()[0]);
  }
  if (NS_WARN_IF(!cr)) {
    return;
  }

  AnnotationTable notes(4);
  WriteExtraDataForMinidump(notes);
  nsCOMPtr<nsIFile> dumpFile;
  TakeMinidump(getter_AddRefs(dumpFile), nullptr);
  if (!dumpFile) {
    NS_WARNING("GMP crash without crash report");
    return;
  }
  GetIDFromMinidump(dumpFile, aResult);
  cr->GenerateCrashReportForMinidump(dumpFile, &notes);
}

static void
GMPNotifyObservers(nsAString& aData)
{
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    nsString temp(aData);
    obs->NotifyObservers(nullptr, "gmp-plugin-crash", temp.get());
  }
}
#endif
void
GMPParent::ActorDestroy(ActorDestroyReason aWhy)
{
  LOGD(("%s::%s: %p (%d)", __CLASS__, __FUNCTION__, this, (int) aWhy));
#ifdef MOZ_CRASHREPORTER
  if (AbnormalShutdown == aWhy) {
    nsString dumpID;
    GetCrashID(dumpID);
    nsString id;
    
    
    id.AppendInt(reinterpret_cast<uint64_t>(this));
    id.Append(NS_LITERAL_STRING(" "));
    AppendUTF8toUTF16(mDisplayName, id);
    id.Append(NS_LITERAL_STRING(" "));
    id.Append(dumpID);

    
    NS_DispatchToMainThread(WrapRunnableNM(&GMPNotifyObservers, id),
                            NS_DISPATCH_NORMAL);
  }
#endif
  
  mState = GMPStateClosing;
  mAbnormalShutdownInProgress = true;
  CloseActive(false);

  
  if (AbnormalShutdown == aWhy) {
    mState = GMPStateClosing;
    nsRefPtr<GMPParent> self(this);
    
    mService->ReAddOnGMPThread(self);
  }
}

mozilla::dom::PCrashReporterParent*
GMPParent::AllocPCrashReporterParent(const NativeThreadId& aThread)
{
#ifndef MOZ_CRASHREPORTER
  MOZ_ASSERT(false, "Should only be sent if crash reporting is enabled.");
#endif
  CrashReporterParent* cr = new CrashReporterParent();
  cr->SetChildData(aThread, GeckoProcessType_GMPlugin);
  return cr;
}

bool
GMPParent::DeallocPCrashReporterParent(PCrashReporterParent* aCrashReporter)
{
  delete aCrashReporter;
  return true;
}

PGMPVideoDecoderParent*
GMPParent::AllocPGMPVideoDecoderParent()
{
  GMPVideoDecoderParent* vdp = new GMPVideoDecoderParent(this);
  NS_ADDREF(vdp);
  return vdp;
}

bool
GMPParent::DeallocPGMPVideoDecoderParent(PGMPVideoDecoderParent* aActor)
{
  GMPVideoDecoderParent* vdp = static_cast<GMPVideoDecoderParent*>(aActor);
  NS_RELEASE(vdp);
  return true;
}

PGMPVideoEncoderParent*
GMPParent::AllocPGMPVideoEncoderParent()
{
  GMPVideoEncoderParent* vep = new GMPVideoEncoderParent(this);
  NS_ADDREF(vep);
  return vep;
}

bool
GMPParent::DeallocPGMPVideoEncoderParent(PGMPVideoEncoderParent* aActor)
{
  GMPVideoEncoderParent* vep = static_cast<GMPVideoEncoderParent*>(aActor);
  NS_RELEASE(vep);
  return true;
}

PGMPDecryptorParent*
GMPParent::AllocPGMPDecryptorParent()
{
  GMPDecryptorParent* ksp = new GMPDecryptorParent(this);
  NS_ADDREF(ksp);
  return ksp;
}

bool
GMPParent::DeallocPGMPDecryptorParent(PGMPDecryptorParent* aActor)
{
  GMPDecryptorParent* ksp = static_cast<GMPDecryptorParent*>(aActor);
  NS_RELEASE(ksp);
  return true;
}

PGMPAudioDecoderParent*
GMPParent::AllocPGMPAudioDecoderParent()
{
  GMPAudioDecoderParent* vdp = new GMPAudioDecoderParent(this);
  NS_ADDREF(vdp);
  return vdp;
}

bool
GMPParent::DeallocPGMPAudioDecoderParent(PGMPAudioDecoderParent* aActor)
{
  GMPAudioDecoderParent* vdp = static_cast<GMPAudioDecoderParent*>(aActor);
  NS_RELEASE(vdp);
  return true;
}

bool
GMPParent::RecvPGMPTimerConstructor(PGMPTimerParent* actor)
{
  return true;
}

PGMPTimerParent*
GMPParent::AllocPGMPTimerParent()
{
  GMPTimerParent* p = new GMPTimerParent(GMPThread());
  NS_ADDREF(p); 
  return p;
}

bool
GMPParent::DeallocPGMPTimerParent(PGMPTimerParent* aActor)
{
  GMPTimerParent* p = static_cast<GMPTimerParent*>(aActor);
  NS_RELEASE(p);
  return true;
}

nsresult
ParseNextRecord(nsILineInputStream* aLineInputStream,
                const nsCString& aPrefix,
                nsCString& aResult,
                bool& aMoreLines)
{
  nsAutoCString record;
  nsresult rv = aLineInputStream->ReadLine(record, &aMoreLines);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (record.Length() <= aPrefix.Length() ||
      !Substring(record, 0, aPrefix.Length()).Equals(aPrefix)) {
    return NS_ERROR_FAILURE;
  }

  aResult = Substring(record, aPrefix.Length());
  aResult.Trim("\b\t\r\n ");

  return NS_OK;
}

nsresult
GMPParent::ReadGMPMetaData()
{
  MOZ_ASSERT(mDirectory, "Plugin directory cannot be NULL!");
  MOZ_ASSERT(!mName.IsEmpty(), "Plugin mName cannot be empty!");

  nsCOMPtr<nsIFile> infoFile;
  nsresult rv = mDirectory->Clone(getter_AddRefs(infoFile));
  if (NS_FAILED(rv)) {
    return rv;
  }
  infoFile->AppendRelativePath(mName + NS_LITERAL_STRING(".info"));

  nsCOMPtr<nsIInputStream> inputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), infoFile);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(inputStream, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCString value;
  bool moreLines = false;

  
  nsCString prefix = NS_LITERAL_CSTRING("Name:");
  rv = ParseNextRecord(lineInputStream, prefix, value, moreLines);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (value.IsEmpty()) {
    
    return NS_ERROR_FAILURE;
  }
  mDisplayName = value;

  
  if (!moreLines) {
    return NS_ERROR_FAILURE;
  }
  prefix = NS_LITERAL_CSTRING("Description:");
  rv = ParseNextRecord(lineInputStream, prefix, value, moreLines);
  if (NS_FAILED(rv)) {
    return rv;
  }
  mDescription = value;

  
  if (!moreLines) {
    return NS_ERROR_FAILURE;
  }
  prefix = NS_LITERAL_CSTRING("Version:");
  rv = ParseNextRecord(lineInputStream, prefix, value, moreLines);
  if (NS_FAILED(rv)) {
    return rv;
  }
  mVersion = value;

  
  if (!moreLines) {
    return NS_ERROR_FAILURE;
  }
  prefix = NS_LITERAL_CSTRING("APIs:");
  rv = ParseNextRecord(lineInputStream, prefix, value, moreLines);
  if (NS_FAILED(rv)) {
    return rv;
  }
  nsCCharSeparatedTokenizer apiTokens(value, ',');
  while (apiTokens.hasMoreTokens()) {
    nsAutoCString api(apiTokens.nextToken());
    api.StripWhitespace();
    if (api.IsEmpty()) {
      continue;
    }

    int32_t tagsStart = api.FindChar('[');
    if (tagsStart == 0) {
      
      
      continue;
    }

    auto cap = new GMPCapability();
    if (tagsStart == -1) {
      
      cap->mAPIName.Assign(api);
    } else {
      auto tagsEnd = api.FindChar(']');
      if (tagsEnd == -1 || tagsEnd < tagsStart) {
        
        delete cap;
        continue;
      }

      cap->mAPIName.Assign(Substring(api, 0, tagsStart));

      if ((tagsEnd - tagsStart) > 1) {
        const nsDependentCSubstring ts(Substring(api, tagsStart + 1, tagsEnd - tagsStart - 1));
        nsCCharSeparatedTokenizer tagTokens(ts, ':');
        while (tagTokens.hasMoreTokens()) {
          const nsDependentCSubstring tag(tagTokens.nextToken());
          cap->mAPITags.AppendElement(tag);
        }
      }
    }

    mCapabilities.AppendElement(cap);
  }

  if (mCapabilities.IsEmpty()) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

bool
GMPParent::CanBeSharedCrossOrigin() const
{
  return mOrigin.IsEmpty();
}

bool
GMPParent::CanBeUsedFrom(const nsAString& aOrigin) const
{
  return (mOrigin.IsEmpty() && State() == GMPStateNotLoaded) ||
         mOrigin.Equals(aOrigin);
}

void
GMPParent::SetOrigin(const nsAString& aOrigin)
{
  MOZ_ASSERT(!aOrigin.IsEmpty());
  MOZ_ASSERT(CanBeUsedFrom(aOrigin));
  mOrigin = aOrigin;
}

} 
} 
