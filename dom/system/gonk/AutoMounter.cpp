



#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/statfs.h>

#include <arpa/inet.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <android/log.h>
#include <cutils/properties.h>

#include "AutoMounter.h"
#include "nsVolumeService.h"
#include "AutoMounterSetting.h"
#include "base/message_loop.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Hal.h"
#include "mozilla/StaticPtr.h"
#include "MozMtpServer.h"
#include "MozMtpStorage.h"
#include "nsAutoPtr.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsMemory.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "OpenFileFinder.h"
#include "Volume.h"
#include "VolumeManager.h"
#include "nsIStatusReporter.h"

using namespace mozilla::hal;
USING_MTP_NAMESPACE



























#define USB_CONFIGURATION_SWITCH_NAME   NS_LITERAL_STRING("usb_configuration")

#define GB_SYS_UMS_ENABLE     "/sys/devices/virtual/usb_composite/usb_mass_storage/enable"
#define GB_SYS_USB_CONFIGURED "/sys/devices/virtual/switch/usb_configuration/state"

#define ICS_SYS_USB_FUNCTIONS "/sys/devices/virtual/android_usb/android0/functions"
#define ICS_SYS_UMS_DIRECTORY "/sys/devices/virtual/android_usb/android0/f_mass_storage"
#define ICS_SYS_MTP_DIRECTORY "/sys/devices/virtual/android_usb/android0/f_mtp"
#define ICS_SYS_USB_STATE     "/sys/devices/virtual/android_usb/android0/state"

#undef USE_DEBUG    // MozMtpDatabase.h also defines USE_DEBUG
#define USE_DEBUG 0

#undef LOG
#undef LOGW
#undef ERR
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO,  "AutoMounter", ## args)
#define LOGW(args...) __android_log_print(ANDROID_LOG_WARN,  "AutoMounter", ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "AutoMounter", ## args)

#undef DBG
#if USE_DEBUG
#define DBG(args...)  __android_log_print(ANDROID_LOG_DEBUG, "AutoMounter" , ## args)
#else
#define DBG(args...)
#endif

namespace mozilla {
namespace system {

#define SYS_USB_CONFIG          "sys.usb.config"
#define PERSIST_SYS_USB_CONFIG  "persist.sys.usb.config"

#define USB_FUNC_ADB    "adb"
#define USB_FUNC_MTP    "mtp"
#define USB_FUNC_NONE   "none"
#define USB_FUNC_RNDIS  "rndis"
#define USB_FUNC_UMS    "mass_storage"
#define USB_FUNC_DEFAULT    "default"

class AutoMounter;

static void SetAutoMounterStatus(int32_t aStatus);



inline const char* SwitchStateStr(const SwitchEvent& aEvent)
{
  return aEvent.status() == SWITCH_STATE_ON ? "plugged" : "unplugged";
}



static bool
IsUsbCablePluggedIn()
{
#if 0
  
  
  return GetCurrentSwitchEvent(SWITCH_USB) == SWITCH_STATE_ON;
#else
  
  if (access(ICS_SYS_USB_STATE, F_OK) == 0) {
    char usbState[20];
    if (ReadSysFile(ICS_SYS_USB_STATE, usbState, sizeof(usbState))) {
      DBG("IsUsbCablePluggedIn: state = '%s'", usbState);
      return strcmp(usbState, "CONFIGURED") == 0 ||
             strcmp(usbState, "CONNECTED") == 0;
    }
    ERR("Error reading file '%s': %s", ICS_SYS_USB_STATE, strerror(errno));
    return false;
  }
  bool configured;
  if (ReadSysFile(GB_SYS_USB_CONFIGURED, &configured)) {
    return configured;
  }
  ERR("Error reading file '%s': %s", GB_SYS_USB_CONFIGURED, strerror(errno));
  return false;
#endif
}

static bool
IsUsbConfigured()
{
  if (access(ICS_SYS_USB_STATE, F_OK) == 0) {
    char usbState[20];
    if (ReadSysFile(ICS_SYS_USB_STATE, usbState, sizeof(usbState))) {
      DBG("IsUsbConfigured: state = '%s'", usbState);
      return strcmp(usbState, "CONFIGURED") == 0;
    }
    ERR("Error reading file '%s': %s", ICS_SYS_USB_STATE, strerror(errno));
    return false;
  }
  bool configured;
  if (ReadSysFile(GB_SYS_USB_CONFIGURED, &configured)) {
    return configured;
  }
  ERR("Error reading file '%s': %s", GB_SYS_USB_CONFIGURED, strerror(errno));
  return false;
}





class AutoVolumeManagerStateObserver : public VolumeManager::StateObserver
{
public:
  virtual void Notify(const VolumeManager::StateChangedEvent& aEvent);
};



class AutoVolumeEventObserver : public Volume::EventObserver
{
public:
  virtual void Notify(Volume* const& aEvent);
};

class AutoMounterResponseCallback : public VolumeResponseCallback
{
public:
  AutoMounterResponseCallback()
    : mErrorCount(0)
  {
  }

protected:
  virtual void ResponseReceived(const VolumeCommand* aCommand);

private:
    const static int kMaxErrorCount = 3; 

    int   mErrorCount;
};



class AutoMounter
{
public:
  NS_INLINE_DECL_REFCOUNTING(AutoMounter)

  typedef nsTArray<RefPtr<Volume>> VolumeArray;

  AutoMounter()
    : mState(STATE_IDLE),
      mResponseCallback(new AutoMounterResponseCallback),
      mMode(AUTOMOUNTER_DISABLE)
  {
    VolumeManager::RegisterStateObserver(&mVolumeManagerStateObserver);
    Volume::RegisterVolumeObserver(&mVolumeEventObserver, "AutoMounter");

    
    
    
    CheckVolumeSettings();

    DBG("Calling UpdateState from constructor");
    UpdateState();
  }

  void CheckVolumeSettings()
  {
    if (VolumeManager::State() != VolumeManager::VOLUMES_READY) {
      DBG("CheckVolumeSettings: VolumeManager is NOT READY yet");
      return;
    }
    DBG("CheckVolumeSettings: VolumeManager is READY");

    
    
    

    VolumeManager::VolumeArray::size_type numVolumes = VolumeManager::NumVolumes();
    VolumeManager::VolumeArray::index_type i;
    for (i = 0; i < numVolumes; i++) {
      RefPtr<Volume> vol = VolumeManager::GetVolume(i);
      if (vol) {
        
        
        AutoMounterSetting::CheckVolumeSettings(vol->Name());

        
        
        
      }
    }
  }

  void UpdateState();
  void GetStatus(bool& umsAvail, bool& umsConfigured, bool& umsEnabled, bool& mtpAvail,
	         bool& mtpConfigured, bool& mtpEnabled, bool& rndisConfigured);

  nsresult Dump(nsACString& desc);

  void ConfigureUsbFunction(const char* aUsbFunc);

  bool StartMtpServer();
  void StopMtpServer();

  void StartUmsSharing();
  void StopUmsSharing();


  const char* ModeStr(int32_t aMode)
  {
    switch (aMode) {
      case AUTOMOUNTER_DISABLE:                 return "Disable";
      case AUTOMOUNTER_ENABLE_UMS:              return "Enable-UMS";
      case AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED:  return "DisableWhenUnplugged";
      case AUTOMOUNTER_ENABLE_MTP:              return "Enable-MTP";
    }
    return "??? Unknown ???";
  }

  bool IsModeEnabled(int32_t aMode)
  {
    return aMode == AUTOMOUNTER_ENABLE_MTP ||
           aMode == AUTOMOUNTER_ENABLE_UMS;
  }

  void SetMode(int32_t aMode)
  {
    if ((aMode == AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED) &&
        (mMode == AUTOMOUNTER_DISABLE)) {
      
      
      aMode = AUTOMOUNTER_DISABLE;
    }

    if (aMode == AUTOMOUNTER_DISABLE &&
        mMode == AUTOMOUNTER_ENABLE_UMS && IsUsbCablePluggedIn()) {
      
      
      
      
      
      
      
      
      LOG("Attempting to disable UMS. Deferring until USB cable is unplugged.");
      aMode = AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED;
    }

    if (aMode != mMode) {
      LOG("Changing mode from '%s' to '%s'", ModeStr(mMode), ModeStr(aMode));
      mMode = aMode;
      DBG("Calling UpdateState due to mode set to %d", mMode);
      UpdateState();
    }
  }

  void SetSharingMode(const nsACString& aVolumeName, bool aAllowSharing)
  {
    RefPtr<Volume> vol = VolumeManager::FindVolumeByName(aVolumeName);
    if (!vol) {
      return;
    }
    if (vol->IsSharingEnabled() == aAllowSharing) {
      return;
    }
    vol->SetUnmountRequested(false);
    vol->SetMountRequested(false);
    vol->SetSharingEnabled(aAllowSharing);
    DBG("Calling UpdateState due to volume %s sharing set to %d",
        vol->NameStr(), (int)aAllowSharing);
    UpdateState();
  }

  void FormatVolume(const nsACString& aVolumeName)
  {
    RefPtr<Volume> vol = VolumeManager::FindVolumeByName(aVolumeName);
    if (!vol) {
      return;
    }
    if (vol->IsFormatRequested()) {
      return;
    }
    vol->SetUnmountRequested(false);
    vol->SetMountRequested(false);
    vol->SetFormatRequested(true);
    DBG("Calling UpdateState due to volume %s formatting set to %d",
        vol->NameStr(), (int)vol->IsFormatRequested());
    UpdateState();
  }

  void MountVolume(const nsACString& aVolumeName)
  {
    RefPtr<Volume> vol = VolumeManager::FindVolumeByName(aVolumeName);
    if (!vol) {
      return;
    }
    vol->SetUnmountRequested(false);
    if (vol->IsMountRequested() || vol->mState == nsIVolume::STATE_MOUNTED) {
      return;
    }
    vol->SetMountRequested(true);
    DBG("Calling UpdateState due to volume %s mounting set to %d",
        vol->NameStr(), (int)vol->IsMountRequested());
    UpdateState();
  }

  void UnmountVolume(const nsACString& aVolumeName)
  {
    RefPtr<Volume> vol = VolumeManager::FindVolumeByName(aVolumeName);
    if (!vol) {
      return;
    }
    if (vol->IsUnmountRequested()) {
      return;
    }
    vol->SetMountRequested(false);
    vol->SetUnmountRequested(true);
    DBG("Calling UpdateState due to volume %s unmounting set to %d",
        vol->NameStr(), (int)vol->IsUnmountRequested());
    UpdateState();
  }

protected:
  ~AutoMounter()
  {
    Volume::UnregisterVolumeObserver(&mVolumeEventObserver, "AutoMounter");
    VolumeManager::UnregisterStateObserver(&mVolumeManagerStateObserver);
  }

private:

  enum STATE
  {
    
    STATE_IDLE,

    
    
    
    
    
    STATE_MTP_CONFIGURING,

    
    
    
    STATE_MTP_STARTED,

    
    
    STATE_MTP_CONNECTED,

    
    
    STATE_UMS_CONFIGURING,

    
    
    STATE_UMS_CONFIGURED,

    
    STATE_RNDIS_CONFIGURED,
  };

  const char *StateStr(STATE aState)
  {
    switch (aState) {
      case STATE_IDLE:              return "IDLE";
      case STATE_MTP_CONFIGURING:   return "MTP_CONFIGURING";
      case STATE_MTP_CONNECTED:     return "MTP_CONNECTED";
      case STATE_MTP_STARTED:       return "MTP_STARTED";
      case STATE_UMS_CONFIGURING:   return "UMS_CONFIGURING";
      case STATE_UMS_CONFIGURED:    return "UMS_CONFIGURED";
      case STATE_RNDIS_CONFIGURED:  return "RNDIS_CONFIGURED";
    }
    return "STATE_???";
  }

  void SetState(STATE aState)
  {
    const char *oldStateStr = StateStr(mState);
    mState = aState;
    const char *newStateStr = StateStr(mState);
    LOG("AutoMounter state changed from %s to %s", oldStateStr, newStateStr);
  }

  STATE                           mState;

  AutoVolumeEventObserver         mVolumeEventObserver;
  AutoVolumeManagerStateObserver  mVolumeManagerStateObserver;
  RefPtr<VolumeResponseCallback>  mResponseCallback;
  int32_t                         mMode;
  MozMtpStorage::Array            mMozMtpStorage;
};

static StaticRefPtr<AutoMounter> sAutoMounter;
static StaticRefPtr<MozMtpServer> sMozMtpServer;


enum STATE_REPORTER_STATE
{
  REPORTER_UNREGISTERED,
  REPORTER_REGISTERED
};

static int status_reporter_progress = REPORTER_UNREGISTERED;
nsresult getState(nsACString &desc)
{
  sAutoMounter->Dump(desc);
  return NS_OK;
}
NS_STATUS_REPORTER_IMPLEMENT(AutoMounter, "AutoMounter", getState);



void
AutoVolumeManagerStateObserver::Notify(const VolumeManager::StateChangedEvent &)
{
  LOG("VolumeManager state changed event: %s", VolumeManager::StateStr());

  if (!sAutoMounter) {
    return;
  }

  
  
  
  
  sAutoMounter->CheckVolumeSettings();

  DBG("Calling UpdateState due to VolumeManagerStateObserver");
  sAutoMounter->UpdateState();
}

void
AutoVolumeEventObserver::Notify(Volume * const &)
{
  if (!sAutoMounter) {
    return;
  }
  DBG("Calling UpdateState due to VolumeEventStateObserver");
  sAutoMounter->UpdateState();
}

void
AutoMounterResponseCallback::ResponseReceived(const VolumeCommand* aCommand)
{

  if (WasSuccessful()) {
    DBG("Calling UpdateState due to Volume::OnSuccess");
    mErrorCount = 0;
    sAutoMounter->UpdateState();
    return;
  }
  ERR("Command '%s' failed: %d '%s'",
      aCommand->CmdStr(), ResponseCode(), ResponseStr().get());

  if (++mErrorCount < kMaxErrorCount) {
    DBG("Calling UpdateState due to VolumeResponseCallback::OnError");
    sAutoMounter->UpdateState();
  }
}

static bool
IsUsbFunctionEnabled(const char* aConfig, const char* aUsbFunc)
{
  nsAutoCString config(aConfig);
  nsCCharSeparatedTokenizer tokenizer(config, ',');

  while (tokenizer.hasMoreTokens()) {
    nsAutoCString token(tokenizer.nextToken());
    if (token.Equals(aUsbFunc)) {
      DBG("IsUsbFunctionEnabled('%s', '%s'): returning true", aConfig, aUsbFunc);
      return true;
    }
  }
  DBG("IsUsbFunctionEnabled('%s', '%s'): returning false", aConfig, aUsbFunc);
  return false;
}

static void
SetUsbFunction(const char* aUsbFunc)
{
  char oldSysUsbConfig[PROPERTY_VALUE_MAX];
  property_get(SYS_USB_CONFIG, oldSysUsbConfig, "");

  if (strcmp(oldSysUsbConfig, USB_FUNC_NONE) == 0) {
    
    
    
    oldSysUsbConfig[0] = '\0';
  }

  if (IsUsbFunctionEnabled(oldSysUsbConfig, aUsbFunc)) {
    
    DBG("SetUsbFunction('%s') - already set - nothing to do", aUsbFunc);
    return;
  }

  char newSysUsbConfig[PROPERTY_VALUE_MAX];

  if (strcmp(aUsbFunc, USB_FUNC_MTP) == 0) {
    
    strlcpy(newSysUsbConfig, USB_FUNC_MTP, sizeof(newSysUsbConfig));
  } else if (strcmp(aUsbFunc, USB_FUNC_UMS) == 0) {
    
    
    property_get(PERSIST_SYS_USB_CONFIG, newSysUsbConfig, "");
  } else if (strcmp(aUsbFunc, USB_FUNC_DEFAULT) == 0) {
    
    property_get(PERSIST_SYS_USB_CONFIG, newSysUsbConfig, "");
  } else {
    printf_stderr("AutoMounter::SetUsbFunction Unrecognized aUsbFunc '%s'\n", aUsbFunc);
    MOZ_ASSERT(0);
    return;
  }

  
  

  if (IsUsbFunctionEnabled(oldSysUsbConfig, USB_FUNC_ADB)) {
    
    if (!IsUsbFunctionEnabled(newSysUsbConfig, USB_FUNC_ADB)) {
      
      strlcat(newSysUsbConfig, ",", sizeof(newSysUsbConfig));
      strlcat(newSysUsbConfig, USB_FUNC_ADB, sizeof(newSysUsbConfig));
    }
  } else {
    
    if (IsUsbFunctionEnabled(newSysUsbConfig, USB_FUNC_ADB)) {
      
      if (strcmp(newSysUsbConfig, USB_FUNC_ADB) == 0) {
        newSysUsbConfig[0] = '\0';
      } else {
        nsAutoCString withoutAdb(newSysUsbConfig);
        withoutAdb.ReplaceSubstring( "," USB_FUNC_ADB, "");
        strlcpy(newSysUsbConfig, withoutAdb.get(), sizeof(newSysUsbConfig));
      }
    }
  }

  
  
  

  if (strcmp(oldSysUsbConfig, newSysUsbConfig) == 0) {
    DBG("SetUsbFunction('%s') %s is already set to '%s' - nothing to do",
        aUsbFunc, SYS_USB_CONFIG, newSysUsbConfig);
    return;
  }

  if (newSysUsbConfig[0] == '\0') {
    
    strlcpy(newSysUsbConfig, USB_FUNC_NONE, sizeof(newSysUsbConfig));
  }
  LOG("SetUsbFunction(%s) %s from '%s' to '%s'", aUsbFunc, SYS_USB_CONFIG,
      oldSysUsbConfig, newSysUsbConfig);
  property_set(SYS_USB_CONFIG, newSysUsbConfig);
}

bool
AutoMounter::StartMtpServer()
{
  if (sMozMtpServer) {
    
    return true;
  }
  LOG("Starting MtpServer");

  
  
  
#if 0
  LOG("Sleeping");
  PRTime now = PR_Now();
  PRTime stopTime = now + 5000000;
  while (PR_Now() < stopTime) {
    LOG("Sleeping...");
    sleep(1);
  }
  LOG("Sleep done");
#endif

  sMozMtpServer = new MozMtpServer();
  if (!sMozMtpServer->Init()) {
    sMozMtpServer = nullptr;
    return false;
  }

  VolumeArray::index_type volIndex;
  VolumeArray::size_type  numVolumes = VolumeManager::NumVolumes();
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume> vol = VolumeManager::GetVolume(volIndex);
    nsRefPtr<MozMtpStorage> storage = new MozMtpStorage(vol, sMozMtpServer);
    mMozMtpStorage.AppendElement(storage);
  }

  sMozMtpServer->Run();
  return true;
}

void
AutoMounter::StopMtpServer()
{
  LOG("Stopping MtpServer");

  mMozMtpStorage.Clear();
  sMozMtpServer = nullptr;
}



void
AutoMounter::UpdateState()
{
  static bool inUpdateState = false;
  if (inUpdateState) {
    
    
    
    
    
    
    return;
  }
  AutoRestore<bool> inUpdateStateDetector(inUpdateState);
  inUpdateState = true;

  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  
  
  
  
  
  
  

  if (VolumeManager::State() != VolumeManager::VOLUMES_READY) {
    
    
    LOG("UpdateState: VolumeManager not ready yet");
    return;
  }

  if (mResponseCallback->IsPending()) {
    
    
    return;
  }

  
  
  
  
  
  
  bool umsAvail, umsConfigured, umsEnabled;
  bool mtpAvail, mtpConfigured, mtpEnabled;
  bool rndisConfigured;
  bool usbCablePluggedIn = IsUsbCablePluggedIn();
  GetStatus(umsAvail, umsConfigured, umsEnabled, mtpAvail,
            mtpConfigured, mtpEnabled, rndisConfigured);
  bool enabled = mtpEnabled || umsEnabled;

  if (mMode == AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED) {
    
    enabled = usbCablePluggedIn;
    if (!usbCablePluggedIn) {
      mMode = AUTOMOUNTER_DISABLE;
      mtpEnabled = false;
      umsEnabled = false;
    }
  }

  DBG("UpdateState: ums:A%dC%dE%d mtp:A%dC%dE%d rndis:%d mode:%d usb:%d mState:%s",
      umsAvail, umsConfigured, umsEnabled,
      mtpAvail, mtpConfigured, mtpEnabled, rndisConfigured,
      mMode, usbCablePluggedIn, StateStr(mState));

  switch (mState) {

    case STATE_IDLE:
      if (!usbCablePluggedIn) {
        
        
        break;
      }
      if (rndisConfigured) {
        
        SetState(STATE_RNDIS_CONFIGURED);
        break;
      }
      if (mtpEnabled) {
        if (mtpConfigured) {
          
          
          
          
          if (StartMtpServer()) {
            SetState(STATE_MTP_STARTED);
          } else {
            if (umsAvail) {
              
              LOG("UpdateState: StartMtpServer failed, switch to UMS");
              SetUsbFunction(USB_FUNC_UMS);
              SetState(STATE_UMS_CONFIGURING);
            } else {
              LOG("UpdateState: StartMtpServer failed, keep idle state");
              SetUsbFunction(USB_FUNC_DEFAULT);
            }
          }
        } else {
          
          
          SetUsbFunction(USB_FUNC_MTP);
          SetState(STATE_MTP_CONFIGURING);
        }
      } else if (umsConfigured) {
        
        SetState(STATE_UMS_CONFIGURED);
      } else if (umsAvail) {
        
        
        
        SetUsbFunction(USB_FUNC_UMS);
        SetState(STATE_UMS_CONFIGURING);
      }
      break;

    case STATE_MTP_CONFIGURING:
      
      
      
      if (mtpEnabled && mtpConfigured) {
        
        
        if (StartMtpServer()) {
          SetState(STATE_MTP_STARTED);
        } else {
          
          SetUsbFunction(USB_FUNC_UMS);
          SetState(STATE_UMS_CONFIGURING);
        }
        break;
      }
      if (rndisConfigured) {
        SetState(STATE_RNDIS_CONFIGURED);
        break;
      }
      break;

    case STATE_MTP_STARTED:
      if (usbCablePluggedIn && mtpConfigured && mtpEnabled) {
        
        break;
      }
      DBG("STATE_MTP_STARTED: About to StopMtpServer "
          "mtpConfigured = %d mtpEnabled = %d usbCablePluggedIn: %d",
          mtpConfigured, mtpEnabled, usbCablePluggedIn);
      StopMtpServer();
      if (rndisConfigured) {
        SetState(STATE_RNDIS_CONFIGURED);
        break;
      }
      if (umsAvail) {
        
        SetUsbFunction(USB_FUNC_UMS);
        SetState(STATE_UMS_CONFIGURING);
        break;
      }

      
      
      SetUsbFunction(USB_FUNC_DEFAULT);
      SetState(STATE_IDLE);
      break;

    case STATE_UMS_CONFIGURING:
      
      
      
      
      
      
      
      if (umsConfigured) {
        if (mtpEnabled) {
          
          SetState(STATE_MTP_CONFIGURING);
          SetUsbFunction(USB_FUNC_MTP);
          break;
        }
        SetState(STATE_UMS_CONFIGURED);
      }
      if (rndisConfigured) {
        SetState(STATE_RNDIS_CONFIGURED);
        break;
      }
      break;

    case STATE_UMS_CONFIGURED:
      if (usbCablePluggedIn) {
        if (mtpEnabled) {
          
          SetState(STATE_MTP_CONFIGURING);
          SetUsbFunction(USB_FUNC_MTP);
          break;
        }
        if (umsConfigured && umsEnabled) {
          
          break;
        }
      }
      if (rndisConfigured) {
        SetState(STATE_RNDIS_CONFIGURED);
        break;
      }
      SetState(STATE_IDLE);
      break;

    case STATE_RNDIS_CONFIGURED:
      if (usbCablePluggedIn && rndisConfigured) {
        
        break;
      }
      SetState(STATE_IDLE);
      break;

    default:
      SetState(STATE_IDLE);
      break;
  }

  bool tryToShare = umsEnabled && usbCablePluggedIn;
  LOG("UpdateState: ums:A%dC%dE%d mtp:A%dC%dE%d mode:%d usb:%d tryToShare:%d state:%s",
      umsAvail, umsConfigured, umsEnabled,
      mtpAvail, mtpConfigured, mtpEnabled,
      mMode, usbCablePluggedIn, tryToShare, StateStr(mState));

  bool filesOpen = false;
  static unsigned filesOpenDelayCount = 0;
  VolumeArray::index_type volIndex;
  VolumeArray::size_type  numVolumes = VolumeManager::NumVolumes();
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume>  vol = VolumeManager::GetVolume(volIndex);
    Volume::STATE   volState = vol->State();

    if (vol->State() == nsIVolume::STATE_MOUNTED) {
      LOG("UpdateState: Volume %s is %s and %s @ %s gen %d locked %d sharing %s",
          vol->NameStr(), vol->StateStr(),
          vol->MediaPresent() ? "inserted" : "missing",
          vol->MountPoint().get(), vol->MountGeneration(),
          (int)vol->IsMountLocked(),
          vol->CanBeShared() ? (vol->IsSharingEnabled() ?
                                 (vol->IsSharing() ? "en-y" : "en-n") : "dis") : "x");
      if (vol->IsSharing() && !usbCablePluggedIn) {
        
        
        
        
        
        
        
        
        
        
        vol->SetIsSharing(false);
      }
    } else {
      LOG("UpdateState: Volume %s is %s and %s", vol->NameStr(), vol->StateStr(),
          vol->MediaPresent() ? "inserted" : "missing");
    }
    if (!vol->MediaPresent()) {
      
      continue;
    }

    if (vol->State() == nsIVolume::STATE_CHECKMNT) {
      
      
      
      
      struct statfs fsbuf;
      int rc = MOZ_TEMP_FAILURE_RETRY(statfs(vol->MountPoint().get(), &fsbuf));
      if (rc == -1) {
        
        
        
        ERR("statfs failed for '%s': errno = %d (%s)", vol->NameStr(), errno, strerror(errno));
        continue;
      }
      static int delay = 250;
      if (fsbuf.f_blocks == 0) {
        if (delay <= 4000) {
          LOG("UpdateState: Volume '%s' is inaccessible, checking again in %d msec", vol->NameStr(), delay);
          MessageLoopForIO::current()->
            PostDelayedTask(FROM_HERE,
                            NewRunnableMethod(this, &AutoMounter::UpdateState),
                            delay);
          delay *= 2;
        } else {
          LOG("UpdateState: Volume '%s' is inaccessible, giving up", vol->NameStr());
        }
        continue;
      } else {
        delay = 250;
        vol->SetState(nsIVolume::STATE_MOUNTED);
      }
    }

    if ((tryToShare && vol->IsSharingEnabled()) ||
        vol->IsFormatRequested() ||
        vol->IsUnmountRequested()) {
      switch (volState) {
        
        case nsIVolume::STATE_MOUNTED: {
          if (vol->IsMountLocked()) {
            
            
            LOGW("UpdateState: Mounted volume %s is locked, not sharing or formatting",
                 vol->NameStr());
            break;
          }

          
          
          
          
          if (tryToShare && vol->IsSharingEnabled()) {
            vol->SetIsSharing(true);
          } else if (vol->IsFormatRequested()){
            vol->SetIsFormatting(true);
          } else if (vol->IsUnmountRequested()){
            vol->SetIsUnmounting(true);
          }

          
          
          OpenFileFinder::Info fileInfo;
          OpenFileFinder fileFinder(vol->MountPoint());
          if (fileFinder.First(&fileInfo)) {
            LOGW("The following files are open under '%s'",
                 vol->MountPoint().get());
            do {
              LOGW("  PID: %d file: '%s' app: '%s' comm: '%s' exe: '%s'\n",
                   fileInfo.mPid,
                   fileInfo.mFileName.get(),
                   fileInfo.mAppName.get(),
                   fileInfo.mComm.get(),
                   fileInfo.mExe.get());
            } while (fileFinder.Next(&fileInfo));
            LOGW("UpdateState: Mounted volume %s has open files, not sharing or formatting",
                 vol->NameStr());

            
            
            
            
            
            
            
            
            
            
            

            int delay = 1000;
            if (filesOpenDelayCount > 10) {
              delay = 5000;
            }
            MessageLoopForIO::current()->
              PostDelayedTask(FROM_HERE,
                              NewRunnableMethod(this, &AutoMounter::UpdateState),
                              delay);
            filesOpen = true;
            break;
          }

          
          
          LOG("UpdateState: Unmounting %s", vol->NameStr());
          vol->StartUnmount(mResponseCallback);
          return; 
        }
        case nsIVolume::STATE_IDLE:
        case nsIVolume::STATE_MOUNT_FAIL: {
          LOG("UpdateState: Volume %s is %s", vol->NameStr(), vol->StateStr());
          if (vol->IsFormatting() && !vol->IsFormatRequested()) {
            vol->SetFormatRequested(false);
            LOG("UpdateState: Mounting %s", vol->NameStr());
            vol->StartMount(mResponseCallback);
            break;
          }
          if (tryToShare && vol->IsSharingEnabled() && volState == nsIVolume::STATE_IDLE) {
            
            LOG("UpdateState: Sharing %s", vol->NameStr());
            vol->StartShare(mResponseCallback);
          } else if (vol->IsFormatRequested()){
            
            LOG("UpdateState: Formatting %s", vol->NameStr());
            vol->StartFormat(mResponseCallback);
          }
          return; 
        }
        default: {
          
          break;
        }
      }
    } else {
      
      switch (volState) {
        case nsIVolume::STATE_SHARED: {
          
          LOG("UpdateState: Unsharing %s", vol->NameStr());
          vol->StartUnshare(mResponseCallback);
          return; 
        }
        case nsIVolume::STATE_IDLE: {
          if (!vol->IsUnmountRequested()) {
            

            LOG("UpdateState: Mounting %s", vol->NameStr());
            vol->StartMount(mResponseCallback);
          }
          return; 
        }
        default: {
          
          break;
        }
      }
    }
  }

  int32_t status = AUTOMOUNTER_STATUS_DISABLED;
  if (filesOpen) {
    filesOpenDelayCount++;
    status = AUTOMOUNTER_STATUS_FILES_OPEN;
  } else if (enabled) {
    filesOpenDelayCount = 0;
    status = AUTOMOUNTER_STATUS_ENABLED;
  }
  SetAutoMounterStatus(status);
}



void AutoMounter::GetStatus(bool& umsAvail, bool& umsConfigured, bool& umsEnabled,
                            bool& mtpAvail, bool& mtpConfigured, bool& mtpEnabled,
                            bool& rndisConfigured)
{
  umsConfigured = false;
  umsEnabled = false;
  mtpAvail = false;
  mtpConfigured = false;
  mtpEnabled = false;
  rndisConfigured = false;

  if (access(ICS_SYS_USB_FUNCTIONS, F_OK) != 0) {
    return;
  }

  char functionsStr[60];
  if (!ReadSysFile(ICS_SYS_USB_FUNCTIONS, functionsStr, sizeof(functionsStr))) {
    ERR("Error reading file '%s': %s", ICS_SYS_USB_FUNCTIONS, strerror(errno));
    functionsStr[0] = '\0';
  }
  DBG("GetStatus: USB functions: '%s'", functionsStr);

  bool  usbConfigured = IsUsbConfigured();

  
  
  
  char persistSysUsbConfig[PROPERTY_VALUE_MAX];
  property_get(PERSIST_SYS_USB_CONFIG, persistSysUsbConfig, "");
  if (IsUsbFunctionEnabled(persistSysUsbConfig, USB_FUNC_UMS)) {
    umsAvail = (access(ICS_SYS_UMS_DIRECTORY, F_OK) == 0);
  }
  if (umsAvail) {
    umsConfigured = usbConfigured && strstr(functionsStr, USB_FUNC_UMS) != nullptr;
    umsEnabled = (mMode == AUTOMOUNTER_ENABLE_UMS) ||
                 ((mMode == AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED) && umsConfigured);
  } else {
    umsConfigured = false;
    umsEnabled = false;
  }

  mtpAvail = (access(ICS_SYS_MTP_DIRECTORY, F_OK) == 0);
  if (mtpAvail) {
    mtpConfigured = usbConfigured && strstr(functionsStr, USB_FUNC_MTP) != nullptr;
    mtpEnabled = (mMode == AUTOMOUNTER_ENABLE_MTP) ||
                 ((mMode == AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED) && mtpConfigured);
  } else {
    mtpConfigured = false;
    mtpEnabled = false;
  }

  rndisConfigured = strstr(functionsStr, USB_FUNC_RNDIS) != nullptr;
}


nsresult AutoMounter::Dump(nsACString& desc)
{
  DBG("GetState!");
  bool umsAvail, umsConfigured, umsEnabled;
  bool mtpAvail, mtpConfigured, mtpEnabled;
  bool rndisConfigured;
  GetStatus(umsAvail, umsConfigured, umsEnabled, mtpAvail,
            mtpConfigured, mtpEnabled, rndisConfigured);
  if (mMode == AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED) {
    
    if (!IsUsbCablePluggedIn()) {
      mMode = AUTOMOUNTER_DISABLE;
      mtpEnabled = false;
      umsEnabled = false;
    }
  }

  
  desc += "Current Mode:";
  desc += ModeStr(mMode);
  desc += "|";


  desc += "Current State:";
  desc += StateStr(mState);
  desc += "|";

  desc += "UMS Status:";
  if (umsAvail) {
    desc += "Available";
  } else {
    desc += "UnAvailable";
  }
  desc += ",";
  if (umsConfigured) {
    desc += "Configured";
  } else {
    desc += "Un-Configured";
  }
  desc += ",";
  if (umsEnabled) {
    desc += "Enabled";
  } else {
    desc += "Disabled";
  }
  desc += "|";


  desc += "MTP Status:";
  if (mtpAvail) {
    desc += "Available";
  } else {
    desc += "UnAvailable";
  }
  desc += ",";
  if (mtpConfigured) {
    desc += "Configured";
  } else {
    desc += "Un-Configured";
  }
  desc += ",";
  if (mtpEnabled) {
    desc += "Enabled";
  } else {
    desc += "Disabled";
  }


  
  VolumeArray::index_type volIndex;
  VolumeArray::size_type  numVolumes = VolumeManager::NumVolumes();
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume>  vol = VolumeManager::GetVolume(volIndex);

    desc += "|";
    desc += vol->NameStr();
    desc += ":";
    desc += vol->StateStr();
    desc += "@";
    desc += vol->MountPoint().get();

    if (!vol->MediaPresent()) {
      continue;
    }

    if (vol->CanBeShared()) {
      desc += ",CanBeShared";
    }
    if (vol->CanBeFormatted()) {
      desc += ",CanBeFormatted";
    }
    if (vol->CanBeMounted()) {
      desc += ",CanBeMounted";
    }
    if (vol->IsRemovable()) {
      desc += ",Removable";
    }
    if (vol->IsHotSwappable()) {
      desc += ",HotSwappable";
    }
  }

  return NS_OK;
}


static void
InitAutoMounterIOThread()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(!sAutoMounter);

  sAutoMounter = new AutoMounter();
}

static void
ShutdownAutoMounterIOThread()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  sAutoMounter = nullptr;
  ShutdownVolumeManager();
}

static void
SetAutoMounterModeIOThread(const int32_t& aMode)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(sAutoMounter);

  sAutoMounter->SetMode(aMode);
}

static void
SetAutoMounterSharingModeIOThread(const nsCString& aVolumeName, const bool& aAllowSharing)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(sAutoMounter);

  sAutoMounter->SetSharingMode(aVolumeName, aAllowSharing);
}

static void
AutoMounterFormatVolumeIOThread(const nsCString& aVolumeName)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(sAutoMounter);

  sAutoMounter->FormatVolume(aVolumeName);
}

static void
AutoMounterMountVolumeIOThread(const nsCString& aVolumeName)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(sAutoMounter);

  sAutoMounter->MountVolume(aVolumeName);
}

static void
AutoMounterUnmountVolumeIOThread(const nsCString& aVolumeName)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(sAutoMounter);

  sAutoMounter->UnmountVolume(aVolumeName);
}

static void
UsbCableEventIOThread()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  if (!sAutoMounter) {
    return;
  }
  DBG("Calling UpdateState due to USBCableEvent");
  sAutoMounter->UpdateState();
}










class UsbCableObserver final : public SwitchObserver
{
  ~UsbCableObserver()
  {
    UnregisterSwitchObserver(SWITCH_USB, this);
  }

public:
  NS_INLINE_DECL_REFCOUNTING(UsbCableObserver)

  UsbCableObserver()
  {
    RegisterSwitchObserver(SWITCH_USB, this);
  }

  virtual void Notify(const SwitchEvent& aEvent)
  {
    DBG("UsbCable switch device: %d state: %s\n",
        aEvent.device(), SwitchStateStr(aEvent));
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(UsbCableEventIOThread));
  }
};

static StaticRefPtr<UsbCableObserver> sUsbCableObserver;
static StaticRefPtr<AutoMounterSetting> sAutoMounterSetting;

void
InitAutoMounter()
{
  InitVolumeManager();
  sAutoMounterSetting = new AutoMounterSetting();

  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(InitAutoMounterIOThread));

  
  
  
  sUsbCableObserver = new UsbCableObserver();

  
  if(status_reporter_progress == REPORTER_UNREGISTERED) {
    NS_RegisterStatusReporter(new NS_STATUS_REPORTER_NAME(AutoMounter));
  }
  status_reporter_progress = REPORTER_REGISTERED;
}

int32_t
GetAutoMounterStatus()
{
  if (sAutoMounterSetting) {
    return sAutoMounterSetting->GetStatus();
  }
  return AUTOMOUNTER_STATUS_DISABLED;
}


void
SetAutoMounterStatus(int32_t aStatus)
{
  if (sAutoMounterSetting) {
    sAutoMounterSetting->SetStatus(aStatus);
  }
}

void
SetAutoMounterMode(int32_t aMode)
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(SetAutoMounterModeIOThread, aMode));
}

void
SetAutoMounterSharingMode(const nsCString& aVolumeName, bool aAllowSharing)
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(SetAutoMounterSharingModeIOThread,
                          aVolumeName, aAllowSharing));
}

void
AutoMounterFormatVolume(const nsCString& aVolumeName)
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(AutoMounterFormatVolumeIOThread,
                          aVolumeName));
}

void
AutoMounterMountVolume(const nsCString& aVolumeName)
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(AutoMounterMountVolumeIOThread,
                          aVolumeName));
}

void
AutoMounterUnmountVolume(const nsCString& aVolumeName)
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(AutoMounterUnmountVolumeIOThread,
                          aVolumeName));
}

void
ShutdownAutoMounter()
{
  if (sAutoMounter) {
    DBG("ShutdownAutoMounter: About to StopMtpServer");
    sAutoMounter->StopMtpServer();
    
    if(status_reporter_progress == REPORTER_REGISTERED) {
      NS_UnregisterStatusReporter(new NS_STATUS_REPORTER_NAME(AutoMounter));
    }
    status_reporter_progress = REPORTER_UNREGISTERED;
  }
  sAutoMounterSetting = nullptr;
  sUsbCableObserver = nullptr;

  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(ShutdownAutoMounterIOThread));
}

} 
} 
