



#ifndef gfx_src_DriverInitCrashDetection_h__
#define gfx_src_DriverInitCrashDetection_h__

#include "gfxCore.h"
#include "nsCOMPtr.h"
#include "nsIGfxInfo.h"
#include "nsIFile.h"

namespace mozilla {
namespace gfx {

enum class DriverInitStatus
{
  
  None,

  
  Attempting,

  
  Okay,

  
  Recovered
};

class DriverInitCrashDetection
{
public:
  DriverInitCrashDetection();
  ~DriverInitCrashDetection();

  bool DisableAcceleration() const {
    return sDisableAcceleration;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  enum class TelemetryState {
    
    
    
    Okay = 0,

    
    
    
    
    
    EnvironmentChanged = 1,

    
    
    
    
    RecoveredFromCrash = 2,

    
    
    
    
    DriverUseDisabled = 3
  };

private:
  bool InitLockFilePath();
  bool UpdateEnvironment();
  bool CheckAndUpdatePref(const char* aPrefName, const nsAString& aCurrentValue);
  bool CheckAndUpdateBoolPref(const char* aPrefName, bool aCurrentValue);
  bool FeatureEnabled(int aFeature);
  void AllowDriverInitAttempt();
  bool RecoverFromDriverInitCrash();
  void FlushPreferences();

  void RecordTelemetry(TelemetryState aState);

private:
  static bool sDisableAcceleration;
  static bool sEnvironmentHasBeenUpdated;

private:
  bool mIsChromeProcess;
  nsCOMPtr<nsIGfxInfo> mGfxInfo;
  nsCOMPtr<nsIFile> mLockFile;
};

} 
} 

#endif

