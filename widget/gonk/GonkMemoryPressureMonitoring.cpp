





#include "GonkMemoryPressureMonitoring.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Monitor.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsMemoryPressure.h"
#include "nsThreadUtils.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <android/log.h>

#define LOG(args...)  \
  __android_log_print(ANDROID_LOG_INFO, "GonkMemoryPressure" , ## args)

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

using namespace mozilla;

namespace {






























class MemoryPressureWatcher
  : public nsIRunnable
  , public nsIObserver
{
public:
  MemoryPressureWatcher()
    : mMonitor("MemoryPressureWatcher")
    , mShuttingDown(false)
  {
  }

  NS_DECL_THREADSAFE_ISUPPORTS

  nsresult Init()
  {
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    NS_ENSURE_STATE(os);

    
    os->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,  false);

    
    
    
    
    mPollMS = Preferences::GetUint("gonk.systemMemoryPressureRecoveryPollMS",
                                    5000);

    int pipes[2];
    NS_ENSURE_STATE(!pipe(pipes));
    mShutdownPipeRead = pipes[0];
    mShutdownPipeWrite = pipes[1];
    return NS_OK;
  }

  NS_IMETHOD Observe(nsISupports* aSubject, const char* aTopic,
                     const PRUnichar* aData)
  {
    MOZ_ASSERT(strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0);
    LOG("Observed XPCOM shutdown.");

    MonitorAutoLock lock(mMonitor);
    mShuttingDown = true;
    mMonitor.Notify();

    int rv;
    do {
      
      uint32_t dummy = 0;
      rv = write(mShutdownPipeWrite, &dummy, sizeof(dummy));
    } while(rv == -1 && errno == EINTR);

    return NS_OK;
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
      NS_ASSERTION(NuwaMarkCurrentThread != nullptr,
                   "NuwaMarkCurrentThread is undefined!");
      NuwaMarkCurrentThread(nullptr, nullptr);
    }
#endif

    int lowMemFd = open("/sys/kernel/mm/lowmemkiller/notify_trigger_active",
                        O_RDONLY | O_CLOEXEC);
    NS_ENSURE_STATE(lowMemFd != -1);
    ScopedClose autoClose(lowMemFd);

    nsresult rv = CheckForMemoryPressure(lowMemFd, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);

    while (true) {
      
      
      
      struct pollfd pollfds[2];
      pollfds[0].fd = lowMemFd;
      pollfds[0].events = POLLPRI;
      pollfds[1].fd = mShutdownPipeRead;
      pollfds[1].events = POLLIN;

      int pollRv;
      do {
        pollRv = poll(pollfds, NS_ARRAY_LENGTH(pollfds),  -1);
      } while (pollRv == -1 && errno == EINTR);

      if (pollfds[1].revents) {
        
        LOG("shutting down (1)");
        return NS_OK;
      }

      
      if (!(pollfds[0].revents & POLLPRI)) {
        LOG("Unexpected revents value after poll(): %d. "
            "Shutting down GonkMemoryPressureMonitoring.", pollfds[0].revents);
        return NS_ERROR_FAILURE;
      }

      
      
      
      

      
      
      rv = NS_DispatchMemoryPressure(MemPressure_New);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      
      
      bool memoryPressure;
      do {
        {
          MonitorAutoLock lock(mMonitor);

          
          
          
          
          
          
          
          if (mShuttingDown) {
            LOG("shutting down (2)");
            return NS_OK;
          }
          mMonitor.Wait(PR_MillisecondsToInterval(mPollMS));
        }

        LOG("Checking to see if memory pressure is over.");
        rv = CheckForMemoryPressure(lowMemFd, &memoryPressure);
        NS_ENSURE_SUCCESS(rv, rv);

        if (memoryPressure) {
          rv = NS_DispatchMemoryPressure(MemPressure_Ongoing);
          NS_ENSURE_SUCCESS(rv, rv);
          continue;
        }
      } while (false);

      LOG("Memory pressure is over.");
    }

    return NS_OK;
  }

private:
  






  nsresult CheckForMemoryPressure(int aLowMemFd, bool* aOut)
  {
    if (aOut) {
      *aOut = false;
    }

    lseek(aLowMemFd, 0, SEEK_SET);

    char buf[2];
    int nread;
    do {
      nread = read(aLowMemFd, buf, sizeof(buf));
    } while(nread == -1 && errno == EINTR);
    NS_ENSURE_STATE(nread == 2);

    
    
    if (aOut) {
      *aOut = buf[0] == '1' && buf[1] == '\n';
    }
    return NS_OK;
  }

  Monitor mMonitor;
  uint32_t mPollMS;
  bool mShuttingDown;

  ScopedClose mShutdownPipeRead;
  ScopedClose mShutdownPipeWrite;
};

NS_IMPL_ISUPPORTS2(MemoryPressureWatcher, nsIRunnable, nsIObserver);

} 

namespace mozilla {

void
InitGonkMemoryPressureMonitoring()
{
  
  nsRefPtr<MemoryPressureWatcher> memoryPressureWatcher =
    new MemoryPressureWatcher();
  NS_ENSURE_SUCCESS_VOID(memoryPressureWatcher->Init());

  nsCOMPtr<nsIThread> thread;
  NS_NewThread(getter_AddRefs(thread), memoryPressureWatcher);
}

} 
