



#include "DriverInitCrashDetection.h"
#include "gfxPrefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsXULAppAPI.h"
#include "mozilla/Preferences.h"
#include "mozilla/gfx/Logging.h"

namespace mozilla {
namespace gfx {

bool DriverInitCrashDetection::sDisableAcceleration = false;
bool DriverInitCrashDetection::sEnvironmentHasBeenUpdated = false;

DriverInitCrashDetection::DriverInitCrashDetection()
 : mIsChromeProcess(XRE_GetProcessType() == GeckoProcessType_Default)
{
  
  
  
  if (mIsChromeProcess && !InitLockFilePath()) {
    return;
  }

  if (RecoverFromDriverInitCrash()) {
    if (!sDisableAcceleration) {
      
      
      gfxCriticalError(CriticalLog::DefaultOptions(false)) << "Recovered from graphics driver startup crash; acceleration disabled.";
      sDisableAcceleration = true;
    }
    return;
  }

  
  
  MOZ_ASSERT(!sDisableAcceleration);

  if (mIsChromeProcess &&
      (UpdateEnvironment() || sEnvironmentHasBeenUpdated))
  {
    
    
    
    
    AllowDriverInitAttempt();
    sEnvironmentHasBeenUpdated = true;
    return;
  }
}

DriverInitCrashDetection::~DriverInitCrashDetection()
{
  if (mLockFile) {
    mLockFile->Remove(false);
  }

  if (gfxPrefs::DriverInitStatus() == int32_t(DriverInitStatus::Attempting)) {
    
    
    gfxPrefs::SetDriverInitStatus(int32_t(DriverInitStatus::Okay));
    gfxCriticalError(CriticalLog::DefaultOptions(false)) << "Successfully verified new graphics environment.";
  }
}

bool
DriverInitCrashDetection::InitLockFilePath()
{
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR, getter_AddRefs(mLockFile));
  if (!mLockFile) {
    return false;
  }
  if (!NS_SUCCEEDED(mLockFile->AppendNative(NS_LITERAL_CSTRING("gfxinit.lock")))) {
    return false;
  }
  return true;
}

void
DriverInitCrashDetection::AllowDriverInitAttempt()
{
  
  FILE* fp;
  if (!NS_SUCCEEDED(mLockFile->OpenANSIFileDesc("w", &fp))) {
    return;
  }
  fclose(fp);

  gfxPrefs::SetDriverInitStatus(int32_t(DriverInitStatus::Attempting));

  
  FlushPreferences();
}

bool
DriverInitCrashDetection::RecoverFromDriverInitCrash()
{
  bool exists;
  if (mLockFile &&
      NS_SUCCEEDED(mLockFile->Exists(&exists)) &&
      exists)
  {
    
    
    
    
    gfxPrefs::SetDriverInitStatus(int32_t(DriverInitStatus::Recovered));
    UpdateEnvironment();
    FlushPreferences();
    return true;
  }
  if (gfxPrefs::DriverInitStatus() == int32_t(DriverInitStatus::Recovered)) {
    
    
    return true;
  }
  return false;
}

bool
DriverInitCrashDetection::UpdateEnvironment()
{
  mGfxInfo = do_GetService("@mozilla.org/gfx/info;1");

  bool changed = false;
  if (mGfxInfo) {
    nsString value;

    
    mGfxInfo->GetAdapterDriverVersion(value);
    changed |= CheckAndUpdatePref("gfx.driver-init.driverVersion", value);
    mGfxInfo->GetAdapterDeviceID(value);
    changed |= CheckAndUpdatePref("gfx.driver-init.deviceID", value);

    
#if defined(XP_WIN)
    bool d2dEnabled = gfxPrefs::Direct2DForceEnabled() ||
                      (!gfxPrefs::Direct2DDisabled() && FeatureEnabled(nsIGfxInfo::FEATURE_DIRECT2D));
    changed |= CheckAndUpdateBoolPref("gfx.driver-init.feature-d2d", d2dEnabled);

    bool d3d11Enabled = !gfxPrefs::LayersPreferD3D9();
    if (!FeatureEnabled(nsIGfxInfo::FEATURE_DIRECT3D_11_LAYERS)) {
      d3d11Enabled = false;
    }
    changed |= CheckAndUpdateBoolPref("gfx.driver-init.feature-d3d11", d3d11Enabled);
#endif
  }

  
  changed |= CheckAndUpdatePref("gfx.driver-init.appVersion", NS_LITERAL_STRING(MOZ_APP_VERSION));

  
  changed |= (gfxPrefs::DriverInitStatus() == int32_t(DriverInitStatus::None));

  mGfxInfo = nullptr;
  return changed;
}

bool
DriverInitCrashDetection::FeatureEnabled(int aFeature)
{
  int32_t status;
  if (!NS_SUCCEEDED(mGfxInfo->GetFeatureStatus(aFeature, &status))) {
    return false;
  }
  return status == nsIGfxInfo::FEATURE_STATUS_OK;
}

bool
DriverInitCrashDetection::CheckAndUpdateBoolPref(const char* aPrefName, bool aCurrentValue)
{
  bool oldValue;
  if (NS_SUCCEEDED(Preferences::GetBool(aPrefName, &oldValue)) &&
      oldValue == aCurrentValue)
  {
    return false;
  }
  Preferences::SetBool(aPrefName, aCurrentValue);
  return true;
}

bool
DriverInitCrashDetection::CheckAndUpdatePref(const char* aPrefName, const nsAString& aCurrentValue)
{
  nsAdoptingString oldValue = Preferences::GetString(aPrefName);
  if (oldValue == aCurrentValue) {
    return false;
  }
  Preferences::SetString(aPrefName, aCurrentValue);
  return true;
}

void
DriverInitCrashDetection::FlushPreferences()
{
  if (nsIPrefService* prefService = Preferences::GetService()) {
    prefService->SavePrefFile(nullptr);
  }
}

} 
} 
