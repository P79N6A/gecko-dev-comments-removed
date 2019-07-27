



#ifndef BASE_THREADING_THREAD_RESTRICTIONS_H_
#define BASE_THREADING_THREAD_RESTRICTIONS_H_

#include "base/base_export.h"
#include "base/basictypes.h"


#if (!defined(NDEBUG) || defined(DCHECK_ALWAYS_ON))
#define ENABLE_THREAD_RESTRICTIONS 1
#else
#define ENABLE_THREAD_RESTRICTIONS 0
#endif

class BrowserProcessImpl;
class HistogramSynchronizer;
class NativeBackendKWallet;
class ScopedAllowWaitForLegacyWebViewApi;

namespace cc {
class CompletionEvent;
}
namespace chromeos {
class BlockingMethodCaller;
namespace system {
class StatisticsProviderImpl;
}
}
namespace chrome_browser_net {
class Predictor;
}
namespace content {
class BrowserGpuChannelHostFactory;
class BrowserGpuMemoryBufferManager;
class BrowserShutdownProfileDumper;
class BrowserTestBase;
class GpuChannelHost;
class NestedMessagePumpAndroid;
class RenderWidgetResizeHelper;
class ScopedAllowWaitForAndroidLayoutTests;
class TextInputClientMac;
}
namespace dbus {
class Bus;
}
namespace disk_cache {
class BackendImpl;
class InFlightIO;
}
namespace mojo {
namespace common {
class WatcherThreadManager;
}
}
namespace net {
namespace internal {
class AddressTrackerLinux;
}
}

namespace remoting {
class AutoThread;
}

namespace base {

namespace android {
class JavaHandlerThread;
}

class SequencedWorkerPool;
class SimpleThread;
class Thread;
class ThreadTestHelper;


























class BASE_EXPORT ThreadRestrictions {
 public:
  
  
  class BASE_EXPORT ScopedAllowIO {
   public:
    ScopedAllowIO() { previous_value_ = SetIOAllowed(true); }
    ~ScopedAllowIO() { SetIOAllowed(previous_value_); }
   private:
    
    bool previous_value_;

    DISALLOW_COPY_AND_ASSIGN(ScopedAllowIO);
  };

  
  
  class BASE_EXPORT ScopedAllowSingleton {
   public:
    ScopedAllowSingleton() { previous_value_ = SetSingletonAllowed(true); }
    ~ScopedAllowSingleton() { SetSingletonAllowed(previous_value_); }
   private:
    
    
    bool previous_value_;

    DISALLOW_COPY_AND_ASSIGN(ScopedAllowSingleton);
  };

#if ENABLE_THREAD_RESTRICTIONS
  
  
  
  static bool SetIOAllowed(bool allowed);

  
  
  
  static void AssertIOAllowed();

  
  
  static bool SetSingletonAllowed(bool allowed);

  
  
  static void AssertSingletonAllowed();

  
  
  static void DisallowWaiting();

  
  static void AssertWaitAllowed();
#else
  
  
  static bool SetIOAllowed(bool allowed) { return true; }
  static void AssertIOAllowed() {}
  static bool SetSingletonAllowed(bool allowed) { return true; }
  static void AssertSingletonAllowed() {}
  static void DisallowWaiting() {}
  static void AssertWaitAllowed() {}
#endif

 private:
  
  
  friend class content::BrowserShutdownProfileDumper;
  friend class content::BrowserTestBase;
  friend class content::NestedMessagePumpAndroid;
  friend class content::RenderWidgetResizeHelper;
  friend class content::ScopedAllowWaitForAndroidLayoutTests;
  friend class ::HistogramSynchronizer;
  friend class ::ScopedAllowWaitForLegacyWebViewApi;
  friend class cc::CompletionEvent;
  friend class mojo::common::WatcherThreadManager;
  friend class remoting::AutoThread;
  friend class MessagePumpDefault;
  friend class SequencedWorkerPool;
  friend class SimpleThread;
  friend class Thread;
  friend class ThreadTestHelper;
  friend class PlatformThread;
  friend class android::JavaHandlerThread;

  
  
  friend class ::chromeos::BlockingMethodCaller;  
  friend class ::chromeos::system::StatisticsProviderImpl;  
  friend class chrome_browser_net::Predictor;     
  friend class
      content::BrowserGpuChannelHostFactory;      
  friend class
      content::BrowserGpuMemoryBufferManager;     
  friend class content::GpuChannelHost;           
  friend class content::TextInputClientMac;       
  friend class dbus::Bus;                         
  friend class disk_cache::BackendImpl;           
  friend class disk_cache::InFlightIO;            
  friend class net::internal::AddressTrackerLinux;  
  friend class ::BrowserProcessImpl;              
  friend class ::NativeBackendKWallet;            
  

#if ENABLE_THREAD_RESTRICTIONS
  static bool SetWaitAllowed(bool allowed);
#else
  static bool SetWaitAllowed(bool allowed) { return true; }
#endif

  
  
  
  
  class BASE_EXPORT ScopedAllowWait {
   public:
    ScopedAllowWait() { previous_value_ = SetWaitAllowed(true); }
    ~ScopedAllowWait() { SetWaitAllowed(previous_value_); }
   private:
    
    
    bool previous_value_;

    DISALLOW_COPY_AND_ASSIGN(ScopedAllowWait);
  };

  DISALLOW_IMPLICIT_CONSTRUCTORS(ThreadRestrictions);
};

}  

#endif  
