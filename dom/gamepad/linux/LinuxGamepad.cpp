










#include <algorithm>
#include <cstddef>

#include <glib.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "nscore.h"
#include "mozilla/dom/GamepadFunctions.h"
#include "udev.h"

namespace {

using namespace mozilla::dom::GamepadFunctions;
using mozilla::udev_lib;
using mozilla::udev_device;
using mozilla::udev_list_entry;
using mozilla::udev_enumerate;
using mozilla::udev_monitor;

static const float kMaxAxisValue = 32767.0;
static const char kJoystickPath[] = "/dev/input/js";



typedef struct {
  int index;
  guint source_id;
  int numAxes;
  int numButtons;
  char idstring[128];
  char devpath[PATH_MAX];
} Gamepad;

class LinuxGamepadService {
public:
  LinuxGamepadService() : mMonitor(nullptr),
                          mMonitorSourceID(0) {
  }

  void Startup();
  void Shutdown();

private:
  void AddDevice(struct udev_device* dev);
  void RemoveDevice(struct udev_device* dev);
  void ScanForDevices();
  void AddMonitor();
  void RemoveMonitor();
  bool is_gamepad(struct udev_device* dev);
  void ReadUdevChange();

  
  static gboolean OnGamepadData(GIOChannel *source,
                                GIOCondition condition,
                                gpointer data);

  
  static gboolean OnUdevMonitor(GIOChannel *source,
                                GIOCondition condition,
                                gpointer data);

  udev_lib mUdev;
  struct udev_monitor* mMonitor;
  guint mMonitorSourceID;
  
  nsAutoTArray<Gamepad,4> mGamepads;
};


LinuxGamepadService* gService = nullptr;

void
LinuxGamepadService::AddDevice(struct udev_device* dev)
{
  const char* devpath = mUdev.udev_device_get_devnode(dev);
  if (!devpath) {
    return;
  }

  
  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    if (strcmp(mGamepads[i].devpath, devpath) == 0) {
      return;
    }
  }

  Gamepad gamepad;
  snprintf(gamepad.devpath, sizeof(gamepad.devpath), "%s", devpath);
  GIOChannel* channel = g_io_channel_new_file(devpath, "r", nullptr);
  if (!channel) {
    return;
  }

  g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, nullptr);
  g_io_channel_set_encoding(channel, nullptr, nullptr);
  g_io_channel_set_buffered(channel, FALSE);
  int fd = g_io_channel_unix_get_fd(channel);
  char name[128];
  if (ioctl(fd, JSIOCGNAME(sizeof(name)), &name) == -1) {
    strcpy(name, "unknown");
  }
  const char* vendor_id =
    mUdev.udev_device_get_property_value(dev, "ID_VENDOR_ID");
  const char* model_id =
    mUdev.udev_device_get_property_value(dev, "ID_MODEL_ID");
  if (!vendor_id || !model_id) {
    struct udev_device* parent =
      mUdev.udev_device_get_parent_with_subsystem_devtype(dev,
                                                          "input",
                                                          nullptr);
    if (parent) {
      vendor_id = mUdev.udev_device_get_sysattr_value(parent, "id/vendor");
      model_id = mUdev.udev_device_get_sysattr_value(parent, "id/product");
    }
  }
  snprintf(gamepad.idstring, sizeof(gamepad.idstring),
           "%s-%s-%s",
           vendor_id ? vendor_id : "unknown",
           model_id ? model_id : "unknown",
           name);

  char numAxes = 0, numButtons = 0;
  ioctl(fd, JSIOCGAXES, &numAxes);
  gamepad.numAxes = numAxes;
  ioctl(fd, JSIOCGBUTTONS, &numButtons);
  gamepad.numButtons = numButtons;

  gamepad.index = AddGamepad(gamepad.idstring,
                             mozilla::dom::GamepadMappingType::_empty,
                             gamepad.numButtons,
                             gamepad.numAxes);

  gamepad.source_id =
    g_io_add_watch(channel,
                   GIOCondition(G_IO_IN | G_IO_ERR | G_IO_HUP),
                   OnGamepadData,
                   GINT_TO_POINTER(gamepad.index));
  g_io_channel_unref(channel);

  mGamepads.AppendElement(gamepad);
}

void
LinuxGamepadService::RemoveDevice(struct udev_device* dev)
{
  const char* devpath = mUdev.udev_device_get_devnode(dev);
  if (!devpath) {
    return;
  }

  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    if (strcmp(mGamepads[i].devpath, devpath) == 0) {
      g_source_remove(mGamepads[i].source_id);
      RemoveGamepad(mGamepads[i].index);
      mGamepads.RemoveElementAt(i);
      break;
    }
  }
}

void
LinuxGamepadService::ScanForDevices()
{
  struct udev_enumerate* en = mUdev.udev_enumerate_new(mUdev.udev);
  mUdev.udev_enumerate_add_match_subsystem(en, "input");
  mUdev.udev_enumerate_scan_devices(en);

  struct udev_list_entry* dev_list_entry;
  for (dev_list_entry = mUdev.udev_enumerate_get_list_entry(en);
       dev_list_entry != nullptr;
       dev_list_entry = mUdev.udev_list_entry_get_next(dev_list_entry)) {
    const char* path = mUdev.udev_list_entry_get_name(dev_list_entry);
    struct udev_device* dev = mUdev.udev_device_new_from_syspath(mUdev.udev,
                                                                 path);
    if (is_gamepad(dev)) {
      AddDevice(dev);
    }

    mUdev.udev_device_unref(dev);
  }

  mUdev.udev_enumerate_unref(en);
}

void
LinuxGamepadService::AddMonitor()
{
  
  mMonitor =
    mUdev.udev_monitor_new_from_netlink(mUdev.udev, "udev");
  if (!mMonitor) {
    
    return;
  }
  mUdev.udev_monitor_filter_add_match_subsystem_devtype(mMonitor,
                                                        "input",
							nullptr);

  int monitor_fd = mUdev.udev_monitor_get_fd(mMonitor);
  GIOChannel* monitor_channel = g_io_channel_unix_new(monitor_fd);
  mMonitorSourceID =
    g_io_add_watch(monitor_channel,
                   GIOCondition(G_IO_IN | G_IO_ERR | G_IO_HUP),
                   OnUdevMonitor,
                   nullptr);
  g_io_channel_unref(monitor_channel);

  mUdev.udev_monitor_enable_receiving(mMonitor);
}

void
LinuxGamepadService::RemoveMonitor()
{
  if (mMonitorSourceID) {
    g_source_remove(mMonitorSourceID);
    mMonitorSourceID = 0;
  }
  if (mMonitor) {
    mUdev.udev_monitor_unref(mMonitor);
    mMonitor = nullptr;
  }
}

void
LinuxGamepadService::Startup()
{
  
  if (!mUdev)
    return;

  AddMonitor();
  ScanForDevices();
}

void
LinuxGamepadService::Shutdown()
{
  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    g_source_remove(mGamepads[i].source_id);
  }
  mGamepads.Clear();
  RemoveMonitor();
}

bool
LinuxGamepadService::is_gamepad(struct udev_device* dev)
{
  if (!mUdev.udev_device_get_property_value(dev, "ID_INPUT_JOYSTICK"))
    return false;

  const char* devpath = mUdev.udev_device_get_devnode(dev);
  if (!devpath) {
    return false;
  }
  if (strncmp(kJoystickPath, devpath, sizeof(kJoystickPath) - 1) != 0) {
    return false;
  }

  return true;
}

void
LinuxGamepadService::ReadUdevChange()
{
  struct udev_device* dev =
    mUdev.udev_monitor_receive_device(mMonitor);
  const char* action = mUdev.udev_device_get_action(dev);
  if (is_gamepad(dev)) {
    if (strcmp(action, "add") == 0) {
      AddDevice(dev);
    } else if (strcmp(action, "remove") == 0) {
      RemoveDevice(dev);
    }
  }
  mUdev.udev_device_unref(dev);
}


gboolean
LinuxGamepadService::OnGamepadData(GIOChannel* source,
                                   GIOCondition condition,
                                   gpointer data)
{
  int index = GPOINTER_TO_INT(data);
  
  if (condition & G_IO_ERR || condition & G_IO_HUP)
    return FALSE;

  while (true) {
    struct js_event event;
    gsize count;
    GError* err = nullptr;
    if (g_io_channel_read_chars(source,
				(gchar*)&event,
				sizeof(event),
				&count,
				&err) != G_IO_STATUS_NORMAL ||
	count == 0) {
      break;
    }

    
    if (event.type & JS_EVENT_INIT) {
      continue;
    }

    switch (event.type) {
    case JS_EVENT_BUTTON:
      NewButtonEvent(index, event.number, !!event.value);
      break;
    case JS_EVENT_AXIS:
      NewAxisMoveEvent(index, event.number,
                       ((float)event.value) / kMaxAxisValue);
      break;
    }
  }

  return TRUE;
}


gboolean
LinuxGamepadService::OnUdevMonitor(GIOChannel* source,
                                   GIOCondition condition,
                                   gpointer data)
{
  if (condition & G_IO_ERR || condition & G_IO_HUP)
    return FALSE;

  gService->ReadUdevChange();
  return TRUE;
}

} 

namespace mozilla {
namespace dom {

void StartGamepadMonitoring()
{
  if (gService) {
    return;
  }
  gService = new LinuxGamepadService();
  gService->Startup();
}

void StopGamepadMonitoring()
{
  if (!gService) {
    return;
  }
  gService->Shutdown();
  delete gService;
  gService = nullptr;
}

} 
} 
