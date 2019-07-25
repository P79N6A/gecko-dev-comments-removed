



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
#include "nsAutoPtr.h"
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







static bool
ReadSysFile(const char *aFilename, char *aBuf, size_t aBufSize)
{
  int fd = open(aFilename, O_RDONLY);
  if (fd < 0) {
    ERR("Unable to open file '%s' for reading", aFilename);
    return false;
  }
  ScopedClose autoClose(fd);
  ssize_t bytesRead = read(fd, aBuf, aBufSize - 1);
  if (bytesRead < 0) {
    ERR("Unable to read from file '%s'", aFilename);
    return false;
  }
  if (aBuf[bytesRead - 1] == '\n') {
    bytesRead--;
  }
  aBuf[bytesRead] = '\0';
  return true;
}

static bool
ReadSysFile(const char *aFilename, bool *aVal)
{
  char valBuf[20];
  if (!ReadSysFile(aFilename, valBuf, sizeof(valBuf))) {
    return false;
  }
  int intVal;
  if (sscanf(valBuf, "%d", &intVal) != 1) {
    return false;
  }
  *aVal = (intVal != 0);
  return true;
}



inline const char *SwitchStateStr(const SwitchEvent &aEvent)
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
    return ReadSysFile(ICS_SYS_USB_STATE,
                       usbState, sizeof(usbState)) &&
           (strcmp(usbState, "CONFIGURED") == 0);
  }
  bool configured;
  return ReadSysFile(GB_SYS_USB_CONFIGURED, &configured) &&
         configured;
#endif
}



class VolumeManagerStateObserver : public VolumeManager::StateObserver
{
public:
  virtual void Notify(const VolumeManager::StateChangedEvent &aEvent);
};

class AutoMounterResponseCallback : public VolumeResponseCallback
{
public:
  AutoMounterResponseCallback()
    : mErrorCount(0)
  {
  }

protected:
  virtual void ResponseReceived(const VolumeCommand *aCommand);

private:
    const static int kMaxErrorCount = 3; 

    int   mErrorCount;
};



class AutoMounter : public RefCounted<AutoMounter>
{
public:
  AutoMounter()
    : mResponseCallback(new AutoMounterResponseCallback),
      mMode(AUTOMOUNTER_DISABLE)
  {
    VolumeManager::RegisterStateObserver(&mVolumeManagerStateObserver);
    DBG("Calling UpdateState from constructor");
    UpdateState();
  }

  ~AutoMounter()
  {
    VolumeManager::UnregisterStateObserver(&mVolumeManagerStateObserver);
  }

  void UpdateState();

  void SetMode(int32_t aMode)
  {
    if ((aMode == AUTOMOUNTER_DISABLE_WHEN_UNPLUGGED) &&
        (mMode == AUTOMOUNTER_DISABLE)) {
      
      
      aMode = AUTOMOUNTER_DISABLE;
    }
    mMode = aMode;
    DBG("Calling UpdateState due to mode set to %d", mMode);
    UpdateState();
  }

private:

  VolumeManagerStateObserver      mVolumeManagerStateObserver;
  RefPtr<VolumeResponseCallback>  mResponseCallback;
  int32_t                         mMode;
};

static RefPtr<AutoMounter> sAutoMounter;



void
VolumeManagerStateObserver::Notify(const VolumeManager::StateChangedEvent &)
{
  LOG("VolumeManager state changed event: %s", VolumeManager::StateStr());

  if (!sAutoMounter) {
    return;
  }
  DBG("Calling UpdateState due to VolumeManagerStateObserver");
  sAutoMounter->UpdateState();
}

void
AutoMounterResponseCallback::ResponseReceived(const VolumeCommand *aCommand)
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

  std::vector<RefPtr<Volume> > volumeArray;
  RefPtr<Volume> vol = VolumeManager::FindVolumeByName(NS_LITERAL_CSTRING("sdcard"));
  if (vol != NULL) {
    volumeArray.push_back(vol);
  }
  if (volumeArray.size() == 0) {
    
    LOG("UpdateState: No volumes found");
    return;
  }

  bool  umsAvail = false;
  bool  umsEnabled = false;

  if (access(ICS_SYS_USB_FUNCTIONS, F_OK) == 0) {
    umsAvail = (access(ICS_SYS_UMS_DIRECTORY, F_OK) == 0);
    char functionsStr[60];
    umsEnabled = umsAvail &&
                 ReadSysFile(ICS_SYS_USB_FUNCTIONS, functionsStr, sizeof(functionsStr)) &&
                 !!strstr(functionsStr, "mass_storage");
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

  VolumeManager::VolumeArray::iterator volIter;
  for (volIter = volumeArray.begin(); volIter != volumeArray.end(); volIter++) {
    RefPtr<Volume>  vol = *volIter;
    Volume::STATE   volState = vol->State();

    LOG("UpdateState: Volume %s is %s", vol->NameStr(), vol->StateStr());
    if (tryToShare) {
      
      switch (volState) {
        case Volume::STATE_MOUNTED: {
          
          
          DBG("UpdateState: Unmounting %s", vol->NameStr());
          vol->StartUnmount(mResponseCallback);
          return;
        }
        case Volume::STATE_IDLE: {
          
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
        case Volume::STATE_SHARED: {
          
          DBG("UpdateState: Unsharing %s", vol->NameStr());
          vol->StartUnshare(mResponseCallback);
          return;
        }
        case Volume::STATE_IDLE: {
          

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
SetAutoMounterModeIOThread(const int32_t &aMode)
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

  virtual void Notify(const SwitchEvent &aEvent)
  {
    DBG("UsbCable switch device: %d state: %s\n",
        aEvent.device(), SwitchStateStr(aEvent));
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(UsbCableEventIOThread));
  }
};

static RefPtr<UsbCableObserver> sUsbCableObserver;
static RefPtr<AutoMounterSetting> sAutoMounterSetting;

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
