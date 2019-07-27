





#include "nsISupports.h"
#include "nsPicoService.h"
#include "nsPrintfCString.h"
#include "nsIWeakReferenceUtils.h"
#include "SharedBuffer.h"
#include "nsISimpleEnumerator.h"

#include "mozilla/dom/nsSynthVoiceRegistry.h"
#include "mozilla/dom/nsSpeechTask.h"

#include "nsIFile.h"
#include "nsThreadUtils.h"
#include "prenv.h"

#include "mozilla/DebugOnly.h"
#include <dlfcn.h>






#define PICO_MEM_SIZE 2500000



#define PICO_RETSTRINGSIZE 200


#define PICO_MAX_CHUNK_SIZE 128


#define PICO_VOICE_NAME "pico"



#define PICO_STEP_BUSY 201



#define PICO_RESET_SOFT 0x10


#define PICO_CHANNELS_NUM 1


#define PICO_SAMPLE_RATE 16000


#define GONK_PICO_LANG_PATH "/system/tts/lang_pico"

namespace mozilla {
namespace dom {

StaticRefPtr<nsPicoService> nsPicoService::sSingleton;

class PicoApi
{
public:

  PicoApi() : mInitialized(false) {}

  bool Init()
  {
    if (mInitialized) {
      return true;
    }

    void* handle = dlopen("libttspico.so", RTLD_LAZY);

    if (!handle) {
      NS_WARNING("Failed to open libttspico.so, pico cannot run");
      return false;
    }

    pico_initialize =
      (pico_Status (*)(void*, uint32_t, pico_System*))dlsym(
        handle, "pico_initialize");

    pico_terminate =
      (pico_Status (*)(pico_System*))dlsym(handle, "pico_terminate");

    pico_getSystemStatusMessage =
      (pico_Status (*)(pico_System, pico_Status, pico_Retstring))dlsym(
        handle, "pico_getSystemStatusMessage");;

    pico_loadResource =
      (pico_Status (*)(pico_System, const char*, pico_Resource*))dlsym(
        handle, "pico_loadResource");

    pico_unloadResource =
      (pico_Status (*)(pico_System, pico_Resource*))dlsym(
        handle, "pico_unloadResource");

    pico_getResourceName =
      (pico_Status (*)(pico_System, pico_Resource, pico_Retstring))dlsym(
        handle, "pico_getResourceName");

    pico_createVoiceDefinition =
      (pico_Status (*)(pico_System, const char*))dlsym(
        handle, "pico_createVoiceDefinition");

    pico_addResourceToVoiceDefinition =
      (pico_Status (*)(pico_System, const char*, const char*))dlsym(
        handle, "pico_addResourceToVoiceDefinition");

    pico_releaseVoiceDefinition =
      (pico_Status (*)(pico_System, const char*))dlsym(
        handle, "pico_releaseVoiceDefinition");

    pico_newEngine =
      (pico_Status (*)(pico_System, const char*, pico_Engine*))dlsym(
        handle, "pico_newEngine");

    pico_disposeEngine =
      (pico_Status (*)(pico_System, pico_Engine*))dlsym(
        handle, "pico_disposeEngine");

    pico_resetEngine =
      (pico_Status (*)(pico_Engine, int32_t))dlsym(handle, "pico_resetEngine");

    pico_putTextUtf8 =
      (pico_Status (*)(pico_Engine, const char*, const int16_t, int16_t*))dlsym(
        handle, "pico_putTextUtf8");

    pico_getData =
      (pico_Status (*)(pico_Engine, void*, int16_t, int16_t*, int16_t*))dlsym(
        handle, "pico_getData");

    mInitialized = true;
    return true;
  }

  typedef signed int pico_Status;
  typedef char pico_Retstring[PICO_RETSTRINGSIZE];

  pico_Status (* pico_initialize)(void*, uint32_t, pico_System*);
  pico_Status (* pico_terminate)(pico_System*);
  pico_Status (* pico_getSystemStatusMessage)(
    pico_System, pico_Status, pico_Retstring);

  pico_Status (* pico_loadResource)(pico_System, const char*, pico_Resource*);
  pico_Status (* pico_unloadResource)(pico_System, pico_Resource*);
  pico_Status (* pico_getResourceName)(
    pico_System, pico_Resource, pico_Retstring);
  pico_Status (* pico_createVoiceDefinition)(pico_System, const char*);
  pico_Status (* pico_addResourceToVoiceDefinition)(
    pico_System, const char*, const char*);
  pico_Status (* pico_releaseVoiceDefinition)(pico_System, const char*);
  pico_Status (* pico_newEngine)(pico_System, const char*, pico_Engine*);
  pico_Status (* pico_disposeEngine)(pico_System, pico_Engine*);

  pico_Status (* pico_resetEngine)(pico_Engine, int32_t);
  pico_Status (* pico_putTextUtf8)(
    pico_Engine, const char*, const int16_t, int16_t*);
  pico_Status (* pico_getData)(
    pico_Engine, void*, const int16_t, int16_t*, int16_t*);

private:

  bool mInitialized;

} sPicoApi;

#define PICO_ENSURE_SUCCESS_VOID(_funcName, _status)                      \
  if (_status < 0) {                                                      \
    PicoApi::pico_Retstring message;                                      \
    sPicoApi.pico_getSystemStatusMessage(                                 \
      nsPicoService::sSingleton->mPicoSystem, _status, message);          \
    NS_WARNING(                                                           \
      nsPrintfCString("Error running %s: %s", _funcName, message).get()); \
    return;                                                               \
  }

#define PICO_ENSURE_SUCCESS(_funcName, _status, _rv)                      \
  if (_status < 0) {                                                      \
    PicoApi::pico_Retstring message;                                      \
    sPicoApi.pico_getSystemStatusMessage(                                 \
      nsPicoService::sSingleton->mPicoSystem, _status, message);          \
    NS_WARNING(                                                           \
      nsPrintfCString("Error running %s: %s", _funcName, message).get()); \
    return _rv;                                                           \
  }

class PicoVoice
{
public:

  PicoVoice(const nsAString& aLanguage)
    : mLanguage(aLanguage) {}

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PicoVoice)

  
  nsString mLanguage;

  
  nsCString mTaFile;

  
  nsCString mSgFile;

private:
    ~PicoVoice() {}
};

class PicoCallbackRunnable : public nsRunnable,
                             public nsISpeechTaskCallback
{
  friend class PicoSynthDataRunnable;

public:
  PicoCallbackRunnable(const nsAString& aText, PicoVoice* aVoice,
                       float aRate, float aPitch, nsISpeechTask* aTask,
                       nsPicoService* aService)
    : mText(NS_ConvertUTF16toUTF8(aText))
    , mRate(aRate)
    , mPitch(aPitch)
    , mFirstData(true)
    , mTask(aTask)
    , mVoice(aVoice)
    , mService(aService) { }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSISPEECHTASKCALLBACK

  NS_IMETHOD Run() override;

  bool IsCurrentTask() { return mService->mCurrentTask == mTask; }

private:
  ~PicoCallbackRunnable() { }

  void DispatchSynthDataRunnable(already_AddRefed<SharedBuffer>&& aBuffer,
                                 size_t aBufferSize);

  nsCString mText;

  float mRate;

  float mPitch;

  bool mFirstData;

  
  
  nsISpeechTask* mTask;

  
  
  PicoVoice* mVoice;

  
  
  nsRefPtr<nsPicoService> mService;
};

NS_IMPL_ISUPPORTS_INHERITED(PicoCallbackRunnable, nsRunnable, nsISpeechTaskCallback)



NS_IMETHODIMP
PicoCallbackRunnable::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());
  PicoApi::pico_Status status = 0;

  if (mService->CurrentVoice() != mVoice) {
    mService->LoadEngine(mVoice);
  } else {
    status = sPicoApi.pico_resetEngine(mService->mPicoEngine, PICO_RESET_SOFT);
    PICO_ENSURE_SUCCESS("pico_unloadResource", status, NS_ERROR_FAILURE);
  }

  
  
  nsPrintfCString markedUpText(
    "<pitch level=\"%0.0f\"><speed level=\"%0.0f\">%s</speed></pitch>",
    std::min(std::max(50.0f, mPitch * 100), 200.0f),
    std::min(std::max(20.0f, mRate * 100), 500.0f),
    mText.get());

  const char* text = markedUpText.get();
  size_t buffer_size = 512, buffer_offset = 0;
  nsRefPtr<SharedBuffer> buffer = SharedBuffer::Create(buffer_size);
  int16_t text_offset = 0, bytes_recv = 0, bytes_sent = 0, out_data_type = 0;
  int16_t text_remaining = markedUpText.Length() + 1;

  
  while (IsCurrentTask()) {
    if (text_remaining) {
      status = sPicoApi.pico_putTextUtf8(mService->mPicoEngine,
                                         text + text_offset, text_remaining,
                                         &bytes_sent);
      PICO_ENSURE_SUCCESS("pico_putTextUtf8", status, NS_ERROR_FAILURE);
      
      text_remaining -= bytes_sent;
      text_offset += bytes_sent;
    } else {
      
      
      DispatchSynthDataRunnable(already_AddRefed<SharedBuffer>(), 0);
      break;
    }

    do {
      
      
      
      if (!IsCurrentTask()) {
        
        break;
      }

      if (buffer_size - buffer_offset < PICO_MAX_CHUNK_SIZE) {
        
        
        DispatchSynthDataRunnable(buffer.forget(), buffer_offset);
        buffer_offset = 0;
        buffer = SharedBuffer::Create(buffer_size);
      }

      status = sPicoApi.pico_getData(mService->mPicoEngine,
                                     (uint8_t*)buffer->Data() + buffer_offset,
                                     PICO_MAX_CHUNK_SIZE,
                                     &bytes_recv, &out_data_type);
      PICO_ENSURE_SUCCESS("pico_getData", status, NS_ERROR_FAILURE);
      buffer_offset += bytes_recv;
    } while (status == PICO_STEP_BUSY);
  }

  return NS_OK;
}

void
PicoCallbackRunnable::DispatchSynthDataRunnable(
  already_AddRefed<SharedBuffer>&& aBuffer, size_t aBufferSize)
{
  class PicoSynthDataRunnable final : public nsRunnable
  {
  public:
    PicoSynthDataRunnable(already_AddRefed<SharedBuffer>& aBuffer,
                          size_t aBufferSize, bool aFirstData,
                          PicoCallbackRunnable* aCallback)
      : mBuffer(aBuffer)
      , mBufferSize(aBufferSize)
      , mFirstData(aFirstData)
      , mCallback(aCallback) {
    }

    NS_IMETHOD Run()
    {
      MOZ_ASSERT(NS_IsMainThread());

      if (!mCallback->IsCurrentTask()) {
        return NS_ERROR_NOT_AVAILABLE;
      }

      nsISpeechTask* task = mCallback->mTask;

      if (mFirstData) {
        task->Setup(mCallback, PICO_CHANNELS_NUM, PICO_SAMPLE_RATE, 2);
      }

      return task->SendAudioNative(
        mBufferSize ? static_cast<short*>(mBuffer->Data()) : nullptr, mBufferSize / 2);
    }

  private:
    nsRefPtr<SharedBuffer> mBuffer;

    size_t mBufferSize;

    bool mFirstData;

    nsRefPtr<PicoCallbackRunnable> mCallback;
  };

  nsCOMPtr<nsIRunnable> sendEvent =
    new PicoSynthDataRunnable(aBuffer, aBufferSize, mFirstData, this);
  NS_DispatchToMainThread(sendEvent);
  mFirstData = false;
}



NS_IMETHODIMP
PicoCallbackRunnable::OnPause()
{
  return NS_OK;
}

NS_IMETHODIMP
PicoCallbackRunnable::OnResume()
{
  return NS_OK;
}

NS_IMETHODIMP
PicoCallbackRunnable::OnCancel()
{
  mService->mCurrentTask = nullptr;
  return NS_OK;
}

NS_INTERFACE_MAP_BEGIN(nsPicoService)
  NS_INTERFACE_MAP_ENTRY(nsISpeechService)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPicoService)
NS_IMPL_RELEASE(nsPicoService)

nsPicoService::nsPicoService()
  : mInitialized(false)
  , mVoicesMonitor("nsPicoService::mVoices")
  , mCurrentTask(nullptr)
  , mPicoSystem(nullptr)
  , mPicoEngine(nullptr)
  , mSgResource(nullptr)
  , mTaResource(nullptr)
  , mPicoMemArea(nullptr)
{
}

nsPicoService::~nsPicoService()
{
  
  
  MonitorAutoLock autoLock(mVoicesMonitor);
  mVoices.Clear();

  if (mThread) {
    mThread->Shutdown();
  }

  UnloadEngine();
}



NS_IMETHODIMP
nsPicoService::Observe(nsISupports* aSubject, const char* aTopic,
                       const char16_t* aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE(!strcmp(aTopic, "profile-after-change"), NS_ERROR_UNEXPECTED);

  DebugOnly<nsresult> rv = NS_NewNamedThread("Pico Worker", getter_AddRefs(mThread));
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  return mThread->Dispatch(
    NS_NewRunnableMethod(this, &nsPicoService::Init), NS_DISPATCH_NORMAL);
}


NS_IMETHODIMP
nsPicoService::Speak(const nsAString& aText, const nsAString& aUri,
                     float aVolume, float aRate, float aPitch,
                     nsISpeechTask* aTask)
{
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_AVAILABLE);

  MonitorAutoLock autoLock(mVoicesMonitor);
  bool found = false;
  PicoVoice* voice = mVoices.GetWeak(aUri, &found);
  NS_ENSURE_TRUE(found, NS_ERROR_NOT_AVAILABLE);

  mCurrentTask = aTask;
  nsRefPtr<PicoCallbackRunnable> cb = new PicoCallbackRunnable(aText, voice, aRate, aPitch, aTask, this);
  return mThread->Dispatch(cb, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
nsPicoService::GetServiceType(SpeechServiceType* aServiceType)
{
  *aServiceType = nsISpeechService::SERVICETYPE_DIRECT_AUDIO;
  return NS_OK;
}

struct VoiceTraverserData
{
  nsPicoService* mService;
  nsSynthVoiceRegistry* mRegistry;
};



static PLDHashOperator
PicoAddVoiceTraverser(const nsAString& aUri,
                      nsRefPtr<PicoVoice>& aVoice,
                      void* aUserArg)
{
  
  if (aVoice->mTaFile.IsEmpty() || aVoice->mSgFile.IsEmpty()) {
    return PL_DHASH_REMOVE;
  }

  VoiceTraverserData* data = static_cast<VoiceTraverserData*>(aUserArg);

  nsAutoString name;
  name.AssignLiteral("Pico ");
  name.Append(aVoice->mLanguage);

  DebugOnly<nsresult> rv =
    data->mRegistry->AddVoice(
      data->mService, aUri, name, aVoice->mLanguage, true);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to add voice");

  return PL_DHASH_NEXT;
}

void
nsPicoService::Init()
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(!mInitialized);

  if (!sPicoApi.Init()) {
    NS_WARNING("Failed to initialize pico library");
    return;
  }

  
  nsAutoCString langPath(PR_GetEnv("PICO_LANG_PATH"));

  if (langPath.IsEmpty()) {
    langPath.AssignLiteral(GONK_PICO_LANG_PATH);
  }

  nsCOMPtr<nsIFile> voicesDir;
  NS_NewNativeLocalFile(langPath, true, getter_AddRefs(voicesDir));

  nsCOMPtr<nsISimpleEnumerator> dirIterator;
  nsresult rv = voicesDir->GetDirectoryEntries(getter_AddRefs(dirIterator));

  if (NS_FAILED(rv)) {
    NS_WARNING(nsPrintfCString("Failed to get contents of directory: %s", langPath.get()).get());
    return;
  }

  bool hasMoreElements = false;
  rv = dirIterator->HasMoreElements(&hasMoreElements);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  MonitorAutoLock autoLock(mVoicesMonitor);

  while (hasMoreElements && NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsISupports> supports;
    rv = dirIterator->GetNext(getter_AddRefs(supports));
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    nsCOMPtr<nsIFile> voiceFile = do_QueryInterface(supports);
    MOZ_ASSERT(voiceFile);

    nsAutoCString leafName;
    voiceFile->GetNativeLeafName(leafName);

    nsAutoString lang;

    if (GetVoiceFileLanguage(leafName, lang)) {
      nsAutoString uri;
      uri.AssignLiteral("urn:moz-tts:pico:");
      uri.Append(lang);

      bool found = false;
      PicoVoice* voice = mVoices.GetWeak(uri, &found);

      if (!found) {
        voice = new PicoVoice(lang);
        mVoices.Put(uri, voice);
      }

      
      
      
      if (StringEndsWith(leafName, NS_LITERAL_CSTRING("_ta.bin"))) {
        rv = voiceFile->GetPersistentDescriptor(voice->mTaFile);
        MOZ_ASSERT(NS_SUCCEEDED(rv));
      } else if (StringEndsWith(leafName, NS_LITERAL_CSTRING("_sg.bin"))) {
        rv = voiceFile->GetPersistentDescriptor(voice->mSgFile);
        MOZ_ASSERT(NS_SUCCEEDED(rv));
      }
    }

    rv = dirIterator->HasMoreElements(&hasMoreElements);
  }

  NS_DispatchToMainThread(NS_NewRunnableMethod(this, &nsPicoService::RegisterVoices));
}

void
nsPicoService::RegisterVoices()
{
  VoiceTraverserData data = { this, nsSynthVoiceRegistry::GetInstance() };
  mVoices.Enumerate(PicoAddVoiceTraverser, &data);

  mInitialized = true;
}

bool
nsPicoService::GetVoiceFileLanguage(const nsACString& aFileName, nsAString& aLang)
{
  nsACString::const_iterator start, end;
  aFileName.BeginReading(start);
  aFileName.EndReading(end);

  
  
  if (FindInReadable(NS_LITERAL_CSTRING("_"), start, end)) {
    end = start;
    aFileName.BeginReading(start);
    aLang.Assign(NS_ConvertUTF8toUTF16(Substring(start, end)));
    return true;
  }

  return false;
}

void
nsPicoService::LoadEngine(PicoVoice* aVoice)
{
  PicoApi::pico_Status status = 0;

  if (mPicoSystem) {
    UnloadEngine();
  }

  if (!mPicoMemArea) {
    mPicoMemArea = new uint8_t[PICO_MEM_SIZE];
  }

  status = sPicoApi.pico_initialize(mPicoMemArea, PICO_MEM_SIZE, &mPicoSystem);
  PICO_ENSURE_SUCCESS_VOID("pico_initialize", status);

  status = sPicoApi.pico_loadResource(mPicoSystem, aVoice->mTaFile.get(), &mTaResource);
  PICO_ENSURE_SUCCESS_VOID("pico_loadResource", status);

  status = sPicoApi.pico_loadResource(mPicoSystem, aVoice->mSgFile.get(), &mSgResource);
  PICO_ENSURE_SUCCESS_VOID("pico_loadResource", status);

  status = sPicoApi.pico_createVoiceDefinition(mPicoSystem, PICO_VOICE_NAME);
  PICO_ENSURE_SUCCESS_VOID("pico_createVoiceDefinition", status);

  char taName[PICO_RETSTRINGSIZE];
  status = sPicoApi.pico_getResourceName(mPicoSystem, mTaResource, taName);
  PICO_ENSURE_SUCCESS_VOID("pico_getResourceName", status);

  status = sPicoApi.pico_addResourceToVoiceDefinition(
    mPicoSystem, PICO_VOICE_NAME, taName);
  PICO_ENSURE_SUCCESS_VOID("pico_addResourceToVoiceDefinition", status);

  char sgName[PICO_RETSTRINGSIZE];
  status = sPicoApi.pico_getResourceName(mPicoSystem, mSgResource, sgName);
  PICO_ENSURE_SUCCESS_VOID("pico_getResourceName", status);

  status = sPicoApi.pico_addResourceToVoiceDefinition(
    mPicoSystem, PICO_VOICE_NAME, sgName);
  PICO_ENSURE_SUCCESS_VOID("pico_addResourceToVoiceDefinition", status);

  status = sPicoApi.pico_newEngine(mPicoSystem, PICO_VOICE_NAME, &mPicoEngine);
  PICO_ENSURE_SUCCESS_VOID("pico_newEngine", status);

  if (sSingleton) {
    sSingleton->mCurrentVoice = aVoice;
  }
}

void
nsPicoService::UnloadEngine()
{
  PicoApi::pico_Status status = 0;

  if (mPicoEngine) {
    status = sPicoApi.pico_disposeEngine(mPicoSystem, &mPicoEngine);
    PICO_ENSURE_SUCCESS_VOID("pico_disposeEngine", status);
    status = sPicoApi.pico_releaseVoiceDefinition(mPicoSystem, PICO_VOICE_NAME);
    PICO_ENSURE_SUCCESS_VOID("pico_releaseVoiceDefinition", status);
    mPicoEngine = nullptr;
  }

  if (mSgResource) {
    status = sPicoApi.pico_unloadResource(mPicoSystem, &mSgResource);
    PICO_ENSURE_SUCCESS_VOID("pico_unloadResource", status);
    mSgResource = nullptr;
  }

  if (mTaResource) {
    status = sPicoApi.pico_unloadResource(mPicoSystem, &mTaResource);
    PICO_ENSURE_SUCCESS_VOID("pico_unloadResource", status);
    mTaResource = nullptr;
  }

  if (mPicoSystem) {
    status = sPicoApi.pico_terminate(&mPicoSystem);
    PICO_ENSURE_SUCCESS_VOID("pico_terminate", status);
    mPicoSystem = nullptr;
  }
}

PicoVoice*
nsPicoService::CurrentVoice()
{
  MOZ_ASSERT(!NS_IsMainThread());

  return mCurrentVoice;
}



nsPicoService*
nsPicoService::GetInstance()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    MOZ_ASSERT(false, "nsPicoService can only be started on main gecko process");
    return nullptr;
  }

  if (!sSingleton) {
    sSingleton = new nsPicoService();
  }

  return sSingleton;
}

already_AddRefed<nsPicoService>
nsPicoService::GetInstanceForService()
{
  nsRefPtr<nsPicoService> picoService = GetInstance();
  return picoService.forget();
}

void
nsPicoService::Shutdown()
{
  if (!sSingleton) {
    return;
  }

  sSingleton->mCurrentTask = nullptr;

  sSingleton = nullptr;
}

} 
} 
