





#import <CoreFoundation/CoreFoundation.h>
#import <IOKit/ps/IOPowerSources.h>
#import <IOKit/ps/IOPSKeys.h>

#include <mozilla/Hal.h>
#include <mozilla/dom/battery/Constants.h>
#include <mozilla/Services.h>

#include <nsIObserverService.h>
#include <nsIObserver.h>

#include <dlfcn.h>

#define IOKIT_FRAMEWORK_PATH "/System/Library/Frameworks/IOKit.framework/IOKit"

#ifndef kIOPSTimeRemainingUnknown
  #define kIOPSTimeRemainingUnknown ((CFTimeInterval)-1.0)
#endif
#ifndef kIOPSTimeRemainingUnlimited
  #define kIOPSTimeRemainingUnlimited ((CFTimeInterval)-2.0)
#endif

using namespace mozilla::dom::battery;

namespace mozilla {
namespace hal_impl {

typedef CFTimeInterval (*IOPSGetTimeRemainingEstimateFunc)(void);

class MacPowerInformationService
{
public:
  static MacPowerInformationService* GetInstance();
  static void Shutdown();
  static bool IsShuttingDown();

  void BeginListening();
  void StopListening();

  static void HandleChange(void *aContext);

  ~MacPowerInformationService();

private:
  MacPowerInformationService();

  
  CFRunLoopSourceRef mRunLoopSource;

  double mLevel;
  bool mCharging;
  double mRemainingTime;
  bool mShouldNotify;

  friend void GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo);

  static MacPowerInformationService* sInstance;
  static bool sShuttingDown;

  static void* sIOKitFramework;
  static IOPSGetTimeRemainingEstimateFunc sIOPSGetTimeRemainingEstimate;
};

void* MacPowerInformationService::sIOKitFramework;
IOPSGetTimeRemainingEstimateFunc MacPowerInformationService::sIOPSGetTimeRemainingEstimate;







void
EnableBatteryNotifications()
{
  if (!MacPowerInformationService::IsShuttingDown()) {
    MacPowerInformationService::GetInstance()->BeginListening();
  }
}

void
DisableBatteryNotifications()
{
  if (!MacPowerInformationService::IsShuttingDown()) {
    MacPowerInformationService::GetInstance()->StopListening();
  }
}

void
GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo)
{
  MacPowerInformationService* powerService = MacPowerInformationService::GetInstance();

  aBatteryInfo->level() = powerService->mLevel;
  aBatteryInfo->charging() = powerService->mCharging;
  aBatteryInfo->remainingTime() = powerService->mRemainingTime;
}

bool MacPowerInformationService::sShuttingDown = false;





MacPowerInformationService* MacPowerInformationService::sInstance = nullptr;

namespace {
struct SingletonDestroyer MOZ_FINAL : public nsIObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS(SingletonDestroyer, nsIObserver)

NS_IMETHODIMP
SingletonDestroyer::Observe(nsISupports*, const char* aTopic, const char16_t*)
{
  MOZ_ASSERT(!strcmp(aTopic, "xpcom-shutdown"));
  MacPowerInformationService::Shutdown();
  return NS_OK;
}
} 

 MacPowerInformationService*
MacPowerInformationService::GetInstance()
{
  if (sInstance) {
    return sInstance;
  }

  sInstance = new MacPowerInformationService();

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->AddObserver(new SingletonDestroyer(), "xpcom-shutdown", false);
  }

  return sInstance;
}

bool
MacPowerInformationService::IsShuttingDown()
{
  return sShuttingDown;
}

void
MacPowerInformationService::Shutdown()
{
  sShuttingDown = true;
  delete sInstance;
  sInstance = nullptr;
}

MacPowerInformationService::MacPowerInformationService()
  : mRunLoopSource(nullptr)
  , mLevel(kDefaultLevel)
  , mCharging(kDefaultCharging)
  , mRemainingTime(kDefaultRemainingTime)
  , mShouldNotify(false)
{
  
  
  sIOKitFramework = dlopen(IOKIT_FRAMEWORK_PATH, RTLD_LAZY | RTLD_LOCAL);
  if (sIOKitFramework) {
    sIOPSGetTimeRemainingEstimate =
      (IOPSGetTimeRemainingEstimateFunc)dlsym(sIOKitFramework, "IOPSGetTimeRemainingEstimate");
  } else {
    sIOPSGetTimeRemainingEstimate = nullptr;
  }
}

MacPowerInformationService::~MacPowerInformationService()
{
  MOZ_ASSERT(!mRunLoopSource,
               "The observers have not been correctly removed! "
               "(StopListening should have been called)");

  if (sIOKitFramework) {
    dlclose(sIOKitFramework);
  }
}

void
MacPowerInformationService::BeginListening()
{
  
  MOZ_ASSERT(!mRunLoopSource, "IOPS Notification Loop Source already set up. "
                              "(StopListening should have been called)");

  mRunLoopSource = ::IOPSNotificationCreateRunLoopSource(HandleChange, this);
  if (mRunLoopSource) {
    ::CFRunLoopAddSource(::CFRunLoopGetCurrent(), mRunLoopSource,
                         kCFRunLoopDefaultMode);

    
    
    HandleChange(this);
    mShouldNotify = true;
  }
}

void
MacPowerInformationService::StopListening()
{
  MOZ_ASSERT(mRunLoopSource, "IOPS Notification Loop Source not set up. "
                             "(StopListening without BeginListening)");

  ::CFRunLoopRemoveSource(::CFRunLoopGetCurrent(), mRunLoopSource,
                          kCFRunLoopDefaultMode);
  mRunLoopSource = nullptr;
}

void
MacPowerInformationService::HandleChange(void* aContext) {
  MacPowerInformationService* power =
    static_cast<MacPowerInformationService*>(aContext);

  CFTypeRef data = ::IOPSCopyPowerSourcesInfo();
  if (!data) {
    ::CFRelease(data);
    return;
  }

  
  CFArrayRef list = ::IOPSCopyPowerSourcesList(data);
  if (!list) {
    ::CFRelease(list);
    return;
  }

  
  
  double level = kDefaultLevel;
  double charging = kDefaultCharging;
  double remainingTime = kDefaultRemainingTime;

  
  
  for (CFIndex i = 0; i < ::CFArrayGetCount(list); ++i) {
    CFTypeRef source = ::CFArrayGetValueAtIndex(list, i);
    CFDictionaryRef currPowerSourceDesc = ::IOPSGetPowerSourceDescription(data, source);
    if (!currPowerSourceDesc) {
      continue;
    }

    
    int currentCapacity = 0;
    const void* cfRef = ::CFDictionaryGetValue(currPowerSourceDesc, CFSTR(kIOPSCurrentCapacityKey));
    ::CFNumberGetValue((CFNumberRef)cfRef, kCFNumberSInt32Type, &currentCapacity);

    
    int maxCapacity = 0;
    cfRef = ::CFDictionaryGetValue(currPowerSourceDesc, CFSTR(kIOPSMaxCapacityKey));
    ::CFNumberGetValue((CFNumberRef)cfRef, kCFNumberSInt32Type, &maxCapacity);

    if (maxCapacity > 0) {
      level = static_cast<double>(currentCapacity)/static_cast<double>(maxCapacity);
    }

    
    
    
    if(::CFDictionaryGetValueIfPresent(currPowerSourceDesc, CFSTR(kIOPSIsChargingKey), &cfRef)) {
      charging = ::CFBooleanGetValue((CFBooleanRef)cfRef);

      
      
      if (charging) {
        
        
        remainingTime = level == 1.0 ? kDefaultRemainingTime : kUnknownRemainingTime;

        if (::CFDictionaryGetValueIfPresent(currPowerSourceDesc,
                CFSTR(kIOPSTimeToFullChargeKey), &cfRef)) {
          int timeToCharge;
          ::CFNumberGetValue((CFNumberRef)cfRef, kCFNumberIntType, &timeToCharge);
          if (timeToCharge != kIOPSTimeRemainingUnknown) {
            remainingTime = timeToCharge*60;
          }
        }
      } else if (sIOPSGetTimeRemainingEstimate) { 
        
        CFTimeInterval estimate = sIOPSGetTimeRemainingEstimate();
        if (estimate == kIOPSTimeRemainingUnlimited || estimate == kIOPSTimeRemainingUnknown) {
          remainingTime = kUnknownRemainingTime;
        } else {
          remainingTime = estimate;
        }
      }
    }

    break;
  }

  bool isNewData = level != power->mLevel || charging != power->mCharging ||
                   remainingTime != power->mRemainingTime;

  power->mRemainingTime = remainingTime;
  power->mCharging = charging;
  power->mLevel = level;

  
  if (power->mShouldNotify && isNewData) {
    hal::NotifyBatteryChange(hal::BatteryInformation(power->mLevel,
                                                     power->mCharging,
                                                     power->mRemainingTime));
  }

  ::CFRelease(data);
  ::CFRelease(list);
}

} 
} 
