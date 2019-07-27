




#include "OpenSLESProvider.h"
#include "prlog.h"
#include "nsDebug.h"
#include "mozilla/NullPtr.h"

#include <dlfcn.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>


#undef LOG
#undef LOG_ENABLED
#if defined(PR_LOGGING)
PRLogModuleInfo *gOpenSLESProviderLog;
#define LOG(args) PR_LOG(gOpenSLESProviderLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gLoadManagerLog, 5)
#else
#define LOG(args)
#define LOG_ENABLED() (false)
#endif

namespace mozilla {

OpenSLESProvider::OpenSLESProvider()
  : mLock("OpenSLESProvider.mLock"),
    mSLEngine(nullptr),
    mSLEngineUsers(0),
    mIsRealized(false),
    mOpenSLESLib(nullptr)
{
#if defined(PR_LOGGING)
  if (!gOpenSLESProviderLog)
    gOpenSLESProviderLog = PR_NewLogModule("OpenSLESProvider");
  LOG(("OpenSLESProvider being initialized"));
#endif
}

OpenSLESProvider::~OpenSLESProvider()
{
  if (mOpenSLESLib) {
    LOG(("OpenSLES Engine was not properly Destroyed"));
    (void)dlclose(mOpenSLESLib);
  }
}


OpenSLESProvider& OpenSLESProvider::getInstance()
{
  
  
  static OpenSLESProvider instance;
  return instance;
}


SLresult OpenSLESProvider::Get(SLObjectItf * aObjectm,
                               SLuint32 aOptionCount,
                               const SLEngineOption *aOptions)
{
  OpenSLESProvider& provider = OpenSLESProvider::getInstance();
  return provider.GetEngine(aObjectm, aOptionCount, aOptions);
}

SLresult OpenSLESProvider::GetEngine(SLObjectItf * aObjectm,
                                     SLuint32 aOptionCount,
                                     const SLEngineOption *aOptions)
{
  MutexAutoLock lock(mLock);
  LOG(("Getting OpenSLES engine"));
  
  if (mSLEngine != nullptr) {
    *aObjectm = mSLEngine;
    mSLEngineUsers++;
    LOG(("Returning existing engine, %d users", mSLEngineUsers));
    return SL_RESULT_SUCCESS;
  } else {
    int res = ConstructEngine(aObjectm, aOptionCount, aOptions);
    if (res == SL_RESULT_SUCCESS) {
      
      mSLEngine = *aObjectm;
      mSLEngineUsers++;
      LOG(("Returning new engine"));
    } else {
      LOG(("Error getting engine: %d", res));
    }
    return res;
  }
}

SLresult OpenSLESProvider::ConstructEngine(SLObjectItf * aObjectm,
                                           SLuint32 aOptionCount,
                                           const SLEngineOption *aOptions)
{
  mLock.AssertCurrentThreadOwns();

  if (!mOpenSLESLib) {
    mOpenSLESLib = dlopen("libOpenSLES.so", RTLD_LAZY);
    if (!mOpenSLESLib) {
      LOG(("Failed to dlopen OpenSLES library"));
      return SL_RESULT_MEMORY_FAILURE;
    }
  }

  typedef SLresult (*slCreateEngine_t)(SLObjectItf *,
                                       SLuint32,
                                       const SLEngineOption *,
                                       SLuint32,
                                       const SLInterfaceID *,
                                       const SLboolean *);

  slCreateEngine_t f_slCreateEngine =
    (slCreateEngine_t)dlsym(mOpenSLESLib, "slCreateEngine");
  int result = f_slCreateEngine(aObjectm, aOptionCount, aOptions, 0, NULL, NULL);
  return result;
}


void OpenSLESProvider::Destroy(SLObjectItf * aObjectm)
{
  OpenSLESProvider& provider = OpenSLESProvider::getInstance();
  provider.DestroyEngine(aObjectm);
}

void OpenSLESProvider::DestroyEngine(SLObjectItf * aObjectm)
{
  MutexAutoLock lock(mLock);
  NS_ASSERTION(mOpenSLESLib, "OpenSLES destroy called but library is not open");

  mSLEngineUsers--;
  LOG(("Freeing engine, %d users left", mSLEngineUsers));
  if (mSLEngineUsers) {
    return;
  }

  (*(*aObjectm))->Destroy(*aObjectm);
  
  
  *aObjectm = nullptr;

  (void)dlclose(mOpenSLESLib);
  mOpenSLESLib = nullptr;
  mIsRealized = false;
}


SLresult OpenSLESProvider::Realize(SLObjectItf aObjectm)
{
  OpenSLESProvider& provider = OpenSLESProvider::getInstance();
  return provider.RealizeEngine(aObjectm);
}

SLresult OpenSLESProvider::RealizeEngine(SLObjectItf aObjectm)
{
  MutexAutoLock lock(mLock);
  NS_ASSERTION(mOpenSLESLib, "OpenSLES realize called but library is not open");
  NS_ASSERTION(aObjectm != nullptr, "OpenSLES realize engine with empty ObjectItf");

  if (mIsRealized) {
    LOG(("Not realizing already realized engine"));
    return SL_RESULT_SUCCESS;
  } else {
    SLresult res = (*aObjectm)->Realize(aObjectm, SL_BOOLEAN_FALSE);
    if (res != SL_RESULT_SUCCESS) {
      LOG(("Error realizing OpenSLES engine: %d", res));
    } else {
      LOG(("Realized OpenSLES engine"));
      mIsRealized = true;
    }
    return res;
  }
}

} 

extern "C" {
SLresult mozilla_get_sles_engine(SLObjectItf * aObjectm,
                                 SLuint32 aOptionCount,
                                 const SLEngineOption *aOptions)
{
  return mozilla::OpenSLESProvider::Get(aObjectm, aOptionCount, aOptions);
}

void mozilla_destroy_sles_engine(SLObjectItf * aObjectm)
{
  mozilla::OpenSLESProvider::Destroy(aObjectm);
}

SLresult mozilla_realize_sles_engine(SLObjectItf aObjectm)
{
  return mozilla::OpenSLESProvider::Realize(aObjectm);
}

}

