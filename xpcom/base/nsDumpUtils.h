





#ifndef mozilla_nsDumpUtils_h
#define mozilla_nsDumpUtils_h

#include "nsIObserver.h"
#include "base/message_loop.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "mozilla/Mutex.h"
#include "mozilla/StaticPtr.h"
#include "nsTArray.h"

#ifdef LOG
#undef LOG
#endif

#ifdef ANDROID
#include "android/log.h"
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "Gecko:DumpUtils", ## __VA_ARGS__)
#else
#define LOG(...)
#endif

using namespace mozilla;

#if defined(XP_LINUX) || defined(__FreeBSD__) || defined(XP_MACOSX) 





class FdWatcher
  : public MessageLoopForIO::Watcher
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
  virtual void OnFileCanWriteWithoutBlocking(int aFd) {};

  NS_DECL_THREADSAFE_ISUPPORTS

  





  void Init();

  

  virtual void StartWatching();

  
  
  virtual void StopWatching();

  NS_IMETHOD Observe(nsISupports* aSubject, const char* aTopic,
                     const char16_t* aData)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!strcmp(aTopic, "xpcom-shutdown"));

    XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableMethod(this, &FdWatcher::StopWatching));

    return NS_OK;
  }
};

typedef void (*FifoCallback)(const nsCString& aInputStr);
struct FifoInfo
{
  nsCString mCommand;
  FifoCallback mCallback;
};
typedef nsTArray<FifoInfo> FifoInfoArray;

class FifoWatcher : public FdWatcher
{
public:
  


  static const char* const kPrefName;

  static FifoWatcher* GetSingleton();

  static bool MaybeCreate();

  void RegisterCallback(const nsCString& aCommand, FifoCallback aCallback);

  virtual ~FifoWatcher();

  virtual int OpenFd();

  virtual void OnFileCanReadWithoutBlocking(int aFd);

private:
  nsAutoCString mDirPath;

  static StaticRefPtr<FifoWatcher> sSingleton;

  FifoWatcher(nsCString aPath)
    : mDirPath(aPath)
    , mFifoInfoLock("FifoWatcher.mFifoInfoLock")
  {
  }

  mozilla::Mutex mFifoInfoLock; 
  FifoInfoArray mFifoInfo;
};

typedef void (*PipeCallback)(const uint8_t aRecvSig);
struct SignalInfo
{
  uint8_t mSignal;
  PipeCallback mCallback;
};
typedef nsTArray<SignalInfo> SignalInfoArray;

class SignalPipeWatcher : public FdWatcher
{
public:
  static SignalPipeWatcher* GetSingleton();

  void RegisterCallback(uint8_t aSignal, PipeCallback aCallback);

  void RegisterSignalHandler(uint8_t aSignal = 0);

  virtual ~SignalPipeWatcher();

  virtual int OpenFd();

  virtual void StopWatching();

  virtual void OnFileCanReadWithoutBlocking(int aFd);

private:
  static StaticRefPtr<SignalPipeWatcher> sSingleton;

  SignalPipeWatcher()
    : mSignalInfoLock("SignalPipeWatcher.mSignalInfoLock")
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  mozilla::Mutex mSignalInfoLock; 
  SignalInfoArray mSignalInfo;
};

#endif 


class nsDumpUtils
{
public:
  






  static nsresult OpenTempFile(const nsACString& aFilename,
                               nsIFile** aFile,
                               const nsACString& aFoldername = EmptyCString());
};

#endif
