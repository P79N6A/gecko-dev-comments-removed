



#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <android/log.h>

#include "AutoMounter.h"
#include "nsVolumeService.h"
#include "AutoMounterSetting.h"
#include "base/message_loop.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Hal.h"
#include "mozilla/StaticPtr.h"
#include "MozMtpServer.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "OpenFileFinder.h"
#include "Volume.h"
#include "VolumeManager.h"

using namespace mozilla::hal;
USING_MTP_NAMESPACE



























#define USB_CONFIGURATION_SWITCH_NAME   NS_LITERAL_STRING("usb_configuration")

#define GB_SYS_UMS_ENABLE     "/sys/devices/virtual/usb_composite/usb_mass_storage/enable"
#define GB_SYS_USB_CONFIGURED "/sys/devices/virtual/switch/usb_configuration/state"

#define ICS_SYS_USB_FUNCTIONS "/sys/devices/virtual/android_usb/android0/functions"
#define ICS_SYS_UMS_DIRECTORY "/sys/devices/virtual/android_usb/android0/f_mass_storage"
#define ICS_SYS_MTP_DIRECTORY "/sys/devices/virtual/android_usb/android0/f_mtp"
#define ICS_SYS_USB_STATE     "/sys/devices/virtual/android_usb/android0/state"

#define USE_DEBUG 0

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO,  "AutoMounter", ## args)
#define LOGW(args...) __android_log_print(ANDROID_LOG_WARN,  "AutoMounter", ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "AutoMounter", ## args)

#if USE_DEBUG
#define DBG(args...)  __android_log_print(ANDROID_LOG_DEBUG, "AutoMounter" , ## args)
#else
#define DBG(args...)
#endif

namespace mozilla {
namespace system {

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
#endif
}





class AutoVolumeManagerStateObserver : public VolumeManager::StateObserver
{
public:
  virtual void Notify(const VolumeManager::StateChangedEvent& aEvent);
};



class AutoVolumeEventObserver : public Volume::EventObserver
{
public:
  virtual void Notify(Volume * const & aEvent);
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
    : mResponseCallback(new AutoMounterResponseCallback),
      mMode(AUTOMOUNTER_DISABLE)
  {
    VolumeManager::RegisterStateObserver(&mVolumeManagerStateObserver);
    Volume::RegisterObserver(&mVolumeEventObserver);

    
    
    
    CheckVolumeSettings();

    DBG("Calling UpdateState from constructor");
    UpdateState();
  }

  ~AutoMounter()
  {
    VolumeManager::VolumeArray::size_type numVolumes = VolumeManager::NumVolumes();
    VolumeManager::VolumeArray::index_type volIndex;
    for (volIndex = 0; volIndex < numVolumes; volIndex++) {
      RefPtr<Volume> vol = VolumeManager::GetVolume(volIndex);
      if (vol) {
        vol->UnregisterObserver(&mVolumeEventObserver);
      }
    }
    Volume::UnregisterObserver(&mVolumeEventObserver);
    VolumeManager::UnregisterStateObserver(&mVolumeManagerStateObserver);
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
        vol->RegisterObserver(&mVolumeEventObserver);
        
        
        AutoMounterSetting::CheckVolumeSettings(vol->Name());

        
        
        
      }
    }
  }

  void StartMtpServer();
  void StopMtpServer();

  void UpdateState();

  const char* ModeStr(int32_t aMode)
  {
    switch (aMode) {
      case AUTOMOUNTER_DISABLE:                 return "Disable";
      case AUTOMOUNTER_ENABLE:                  return "Enable";
      case AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED:  return "DisableWhenUnplugged";
    }
    return "??? Unknown ???";
  }

  void SetMode(int32_t aMode)
  {
    if ((aMode == AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED) &&
        (mMode == AUTOMOUNTER_DISABLE)) {
      
      
      aMode = AUTOMOUNTER_DISABLE;
    }

    if ((aMode == AUTOMOUNTER_DISABLE) &&
        (mMode == AUTOMOUNTER_ENABLE) && IsUsbCablePluggedIn()) {
      
      
      
      
      
      
      
      
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

private:

  AutoVolumeEventObserver         mVolumeEventObserver;
  AutoVolumeManagerStateObserver  mVolumeManagerStateObserver;
  RefPtr<VolumeResponseCallback>  mResponseCallback;
  int32_t                         mMode;
};

static StaticRefPtr<AutoMounter> sAutoMounter;
static StaticRefPtr<MozMtpServer> sMozMtpServer;



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

void
AutoMounter::StartMtpServer()
{
  if (sMozMtpServer) {
    
    return;
  }
  LOG("Starting MtpServer");
  sMozMtpServer = new MozMtpServer();
  sMozMtpServer->Run();
}

void
AutoMounter::StopMtpServer()
{
  LOG("Stopping MtpServer");
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

  bool  umsAvail = false;
  bool  umsEnabled = false;
  bool  mtpAvail = false;
  bool  mtpEnabled = false;

  if (access(ICS_SYS_USB_FUNCTIONS, F_OK) == 0) {
    char functionsStr[60];
    if (!ReadSysFile(ICS_SYS_USB_FUNCTIONS, functionsStr, sizeof(functionsStr))) {
      ERR("Error reading file '%s': %s", ICS_SYS_USB_FUNCTIONS, strerror(errno));
      functionsStr[0] = '\0';
    }
    umsAvail = (access(ICS_SYS_UMS_DIRECTORY, F_OK) == 0);
    if (umsAvail) {
      umsEnabled = strstr(functionsStr, "mass_storage") != nullptr;
    }
    mtpAvail = (access(ICS_SYS_MTP_DIRECTORY, F_OK) == 0);
    if (mtpAvail) {
      mtpEnabled = strstr(functionsStr, "mtp") != nullptr;
    }
  }

  bool usbCablePluggedIn = IsUsbCablePluggedIn();
  bool enabled = (mMode == AUTOMOUNTER_ENABLE);

  if (mMode == AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED) {
    enabled = usbCablePluggedIn;
    if (!usbCablePluggedIn) {
      mMode = AUTOMOUNTER_DISABLE;
    }
  }

  bool tryToShare = (((umsAvail && umsEnabled) || (mtpAvail && mtpEnabled))
                  && enabled && usbCablePluggedIn);
  LOG("UpdateState: ums:%d%d mtp:%d%d mode:%d usbCablePluggedIn:%d tryToShare:%d",
      umsAvail, umsEnabled, mtpAvail, mtpEnabled, mMode, usbCablePluggedIn, tryToShare);

  if (mtpAvail && mtpEnabled) {
    if (enabled && usbCablePluggedIn) {
      StartMtpServer();
    } else {
      StopMtpServer();
    }
    return;
  }

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
          vol->CanBeShared() ? (vol->IsSharingEnabled() ? (vol->IsSharing() ? "en-y" : "en-n") : "dis") : "x");
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
        case nsIVolume::STATE_IDLE: {
          LOG("UpdateState: Volume %s is nsIVolume::STATE_IDLE", vol->NameStr());
          if (vol->IsFormatting() && !vol->IsFormatRequested()) {
            vol->SetFormatRequested(false);
            LOG("UpdateState: Mounting %s", vol->NameStr());
            vol->StartMount(mResponseCallback);
            break;
          }
          if (tryToShare && vol->IsSharingEnabled()) {
            
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










class UsbCableObserver MOZ_FINAL : public SwitchObserver
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

static void
InitVolumeConfig()
{
  
  
  
  
  
  
  
  
  

  nsCOMPtr<nsIVolumeService> vs = do_GetService(NS_VOLUMESERVICE_CONTRACTID);
  NS_ENSURE_TRUE_VOID(vs);

  ScopedCloseFile fp;
  int n = 0;
  char line[255];
  char *command, *vol_name_cstr, *mount_point_cstr, *save_ptr;
  const char *filename = "/system/etc/volume.cfg";
  if (!(fp = fopen(filename, "r"))) {
    LOG("Unable to open volume configuration file '%s' - ignoring", filename);
    return;
  }
  while(fgets(line, sizeof(line), fp)) {
    const char *delim = " \t\n";
    n++;

    if (line[0] == '#')
      continue;
    if (!(command = strtok_r(line, delim, &save_ptr))) {
      
      continue;
    }
    if (!strcmp(command, "create")) {
      if (!(vol_name_cstr = strtok_r(nullptr, delim, &save_ptr))) {
        ERR("No vol_name in %s line %d",  filename, n);
        continue;
      }
      if (!(mount_point_cstr = strtok_r(nullptr, delim, &save_ptr))) {
        ERR("No mount point for volume '%s'. %s line %d", vol_name_cstr, filename, n);
        continue;
      }
      nsString  mount_point = NS_ConvertUTF8toUTF16(mount_point_cstr);
      nsString vol_name = NS_ConvertUTF8toUTF16(vol_name_cstr);
      nsresult rv;
      rv = vs->CreateFakeVolume(vol_name, mount_point);
      NS_ENSURE_SUCCESS_VOID(rv);
      rv = vs->SetFakeVolumeState(vol_name, nsIVolume::STATE_MOUNTED);
      NS_ENSURE_SUCCESS_VOID(rv);
    }
    else {
      ERR("Unrecognized command: '%s'", command);
    }
  }
}

void
InitAutoMounter()
{
  InitVolumeConfig();
  InitVolumeManager();
  sAutoMounterSetting = new AutoMounterSetting();

  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(InitAutoMounterIOThread));

  
  
  
  sUsbCableObserver = new UsbCableObserver();
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
    sAutoMounter->StopMtpServer();
  }
  sAutoMounterSetting = nullptr;
  sUsbCableObserver = nullptr;

  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(ShutdownAutoMounterIOThread));
}

} 
} 
