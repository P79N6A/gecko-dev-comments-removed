



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
#include "AutoMounterSetting.h"
#include "base/message_loop.h"
#include "mozilla/FileUtils.h"
#include "mozilla/Hal.h"
#include "mozilla/StaticPtr.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "Volume.h"
#include "VolumeManager.h"

using namespace mozilla::hal;



























#define USB_CONFIGURATION_SWITCH_NAME   NS_LITERAL_STRING("usb_configuration")

#define GB_SYS_UMS_ENABLE     "/sys/devices/virtual/usb_composite/usb_mass_storage/enable"
#define GB_SYS_USB_CONFIGURED "/sys/devices/virtual/switch/usb_configuration/state"

#define ICS_SYS_USB_FUNCTIONS "/sys/devices/virtual/android_usb/android0/functions"
#define ICS_SYS_UMS_DIRECTORY "/sys/devices/virtual/android_usb/android0/f_mass_storage"
#define ICS_SYS_USB_STATE     "/sys/devices/virtual/android_usb/android0/state"

#define USE_DEBUG 0

#undef LOG
#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "AutoMounter" , ## args)
#define ERR(args...)  __android_log_print(ANDROID_LOG_ERROR, "AutoMounter" , ## args)

#if USE_DEBUG
#define DBG(args...)  __android_log_print(ANDROID_LOG_DEBUG, "AutoMounter" , ## args)
#else
#define DBG(args...)
#endif

namespace mozilla {
namespace system {

class AutoMounter;



static const nsDependentCString sAutoVolumeName[] = { NS_LITERAL_CSTRING("sdcard") };



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



class AutoMounter : public RefCounted<AutoMounter>
{
public:

  typedef nsTArray<RefPtr<Volume> > VolumeArray;

  AutoMounter()
    : mResponseCallback(new AutoMounterResponseCallback),
      mMode(AUTOMOUNTER_DISABLE)
  {
    VolumeManager::RegisterStateObserver(&mVolumeManagerStateObserver);
    Volume::RegisterObserver(&mVolumeEventObserver);

    for (size_t i = 0; i < NS_ARRAY_LENGTH(sAutoVolumeName); i++) {
      RefPtr<Volume> vol = VolumeManager::FindAddVolumeByName(sAutoVolumeName[i]);
      if (vol) {
        vol->RegisterObserver(&mVolumeEventObserver);
        mAutoVolume.AppendElement(vol);
      }
    }

    DBG("Calling UpdateState from constructor");
    UpdateState();
  }

  ~AutoMounter()
  {
    VolumeArray::index_type volIndex;
    VolumeArray::size_type  numVolumes = mAutoVolume.Length();
    for (volIndex = 0; volIndex < numVolumes; volIndex++) {
      mAutoVolume[volIndex]->UnregisterObserver(&mVolumeEventObserver);
    }
    Volume::UnregisterObserver(&mVolumeEventObserver);
    VolumeManager::UnregisterStateObserver(&mVolumeManagerStateObserver);
  }

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

private:

  AutoVolumeEventObserver         mVolumeEventObserver;
  AutoVolumeManagerStateObserver  mVolumeManagerStateObserver;
  RefPtr<VolumeResponseCallback>  mResponseCallback;
  int32_t                         mMode;
  VolumeArray                     mAutoVolume;
};

static StaticRefPtr<AutoMounter> sAutoMounter;



void
AutoVolumeManagerStateObserver::Notify(const VolumeManager::StateChangedEvent &)
{
  LOG("VolumeManager state changed event: %s", VolumeManager::StateStr());

  if (!sAutoMounter) {
    return;
  }
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
AutoMounter::UpdateState()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  
  
  
  
  
  
  

  if (VolumeManager::State() != VolumeManager::VOLUMES_READY) {
    
    
    LOG("UpdateState: VolumeManager not ready yet");
    return;
  }

  if (mResponseCallback->IsPending()) {
    
    
    return;
  }

  if (mAutoVolume.Length() == 0) {
    
    LOG("UpdateState: No volumes found");
    return;
  }

  bool  umsAvail = false;
  bool  umsEnabled = false;

  if (access(ICS_SYS_USB_FUNCTIONS, F_OK) == 0) {
    umsAvail = (access(ICS_SYS_UMS_DIRECTORY, F_OK) == 0);
    if (umsAvail) {
      char functionsStr[60];
      if (ReadSysFile(ICS_SYS_USB_FUNCTIONS, functionsStr, sizeof(functionsStr))) {
        umsEnabled = strstr(functionsStr, "mass_storage") != NULL;
      } else {
        ERR("Error reading file '%s': %s", ICS_SYS_USB_FUNCTIONS, strerror(errno));
        umsEnabled = false;
      }
    } else {
      umsEnabled = false;
    }
  } else {
    umsAvail = ReadSysFile(GB_SYS_UMS_ENABLE, &umsEnabled);
  }

  bool usbCablePluggedIn = IsUsbCablePluggedIn();
  bool enabled = (mMode == AUTOMOUNTER_ENABLE);

  if (mMode == AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED) {
    enabled = usbCablePluggedIn;
    if (!usbCablePluggedIn) {
      mMode = AUTOMOUNTER_DISABLE;
    }
  }

  bool tryToShare = (umsAvail && umsEnabled && enabled && usbCablePluggedIn);
  LOG("UpdateState: umsAvail:%d umsEnabled:%d mode:%d usbCablePluggedIn:%d tryToShare:%d",
      umsAvail, umsEnabled, mMode, usbCablePluggedIn, tryToShare);

  VolumeArray::index_type volIndex;
  VolumeArray::size_type  numVolumes = mAutoVolume.Length();
  for (volIndex = 0; volIndex < numVolumes; volIndex++) {
    RefPtr<Volume>  vol = mAutoVolume[volIndex];
    Volume::STATE   volState = vol->State();

    if (vol->State() == nsIVolume::STATE_MOUNTED) {
      LOG("UpdateState: Volume %s is %s and %s @ %s gen %d locked %d",
          vol->NameStr(), vol->StateStr(),
          vol->MediaPresent() ? "inserted" : "missing",
          vol->MountPoint().get(), vol->MountGeneration(),
          (int)vol->IsMountLocked());
    } else {
      LOG("UpdateState: Volume %s is %s and %s", vol->NameStr(), vol->StateStr(),
          vol->MediaPresent() ? "inserted" : "missing");
    }
    if (!vol->MediaPresent()) {
      
      continue;
    }

    if (tryToShare) {
      
      switch (volState) {
        case nsIVolume::STATE_MOUNTED: {
          if (vol->IsMountLocked()) {
            
            
            DBG("UpdateState: Mounted volume %s is locked, leaving",
                vol->NameStr());
            break;
          }
          
          
          DBG("UpdateState: Unmounting %s", vol->NameStr());
          vol->StartUnmount(mResponseCallback);
          return; 
        }
        case nsIVolume::STATE_IDLE: {
          
          DBG("UpdateState: Sharing %s", vol->NameStr());
          vol->StartShare(mResponseCallback);
          return; 
        }
        default: {
          
          break;
        }
      }
    } else {
      
      switch (volState) {
        case nsIVolume::STATE_SHARED: {
          
          DBG("UpdateState: Unsharing %s", vol->NameStr());
          vol->StartUnshare(mResponseCallback);
          return; 
        }
        case nsIVolume::STATE_IDLE: {
          

          DBG("UpdateState: Mounting %s", vol->NameStr());
          vol->StartMount(mResponseCallback);
          return; 
        }
        default: {
          
          break;
        }
      }
    }
  }
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

  sAutoMounter = NULL;
  ShutdownVolumeManager();
}

static void
SetAutoMounterModeIOThread(const int32_t& aMode)
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(sAutoMounter);

  sAutoMounter->SetMode(aMode);
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










class UsbCableObserver : public SwitchObserver,
                         public RefCounted<UsbCableObserver>
{
public:
  UsbCableObserver()
  {
    RegisterSwitchObserver(SWITCH_USB, this);
  }

  ~UsbCableObserver()
  {
    UnregisterSwitchObserver(SWITCH_USB, this);
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
}

void
SetAutoMounterMode(int32_t aMode)
{
  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(SetAutoMounterModeIOThread, aMode));
}

void
ShutdownAutoMounter()
{
  sAutoMounterSetting = NULL;
  sUsbCableObserver = NULL;

  XRE_GetIOMessageLoop()->PostTask(
      FROM_HERE,
      NewRunnableFunction(ShutdownAutoMounterIOThread));
}

} 
} 
