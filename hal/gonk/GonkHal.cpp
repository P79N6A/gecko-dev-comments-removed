








































#include "hardware_legacy/uevent.h"
#include "Hal.h"
#include "mozilla/dom/battery/Constants.h"
#include "mozilla/FileUtils.h"
#include "nsAlgorithm.h"
#include "nsThreadUtils.h"
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>

using mozilla::hal::WindowIdentifier;

namespace mozilla {
namespace hal_impl {

void
Vibrate(const nsTArray<uint32>& pattern, const WindowIdentifier &)
{}

void
CancelVibrate(const WindowIdentifier &)
{}

namespace {

class BatteryUpdater : public nsRunnable {
public:
  NS_IMETHOD Run()
  {
    hal::BatteryInformation info;
    hal_impl::GetCurrentBatteryInformation(&info);
    hal::NotifyBatteryChange(info);
    return NS_OK;
  }
};

class UEventWatcher : public nsRunnable {
public:
  UEventWatcher()
    : mUpdater(new BatteryUpdater())
    , mRunning(false)
  {
  }

  NS_IMETHOD Run()
  {
    while (mRunning) {
      char buf[1024];
      int count = uevent_next_event(buf, sizeof(buf) - 1);
      if (!count) {
        NS_WARNING("uevent_next_event() returned 0!");
        continue;
      }

      buf[sizeof(buf) - 1] = 0;
      if (strstr(buf, "battery"))
        NS_DispatchToMainThread(mUpdater);
    }
    return NS_OK;
  }

  bool mRunning;

private:
  nsRefPtr<BatteryUpdater> mUpdater;
};

} 

static bool sUEventInitialized = false;
static UEventWatcher *sWatcher = NULL;
static nsIThread *sWatcherThread = NULL;

void
EnableBatteryNotifications()
{
  if (!sUEventInitialized)
    sUEventInitialized = uevent_init();
  if (!sUEventInitialized) {
    NS_WARNING("uevent_init() failed!");
    return;
  }

  if (!sWatcher)
    sWatcher = new UEventWatcher();
  NS_ADDREF(sWatcher);

  sWatcher->mRunning = true;
  nsresult rv = NS_NewThread(&sWatcherThread, sWatcher);
  if (NS_FAILED(rv))
    NS_WARNING("Failed to get new thread for uevent watching");
}

void
DisableBatteryNotifications()
{
  sWatcher->mRunning = false;
  sWatcherThread->Shutdown();
  NS_IF_RELEASE(sWatcherThread);
  delete sWatcher;
}

void
GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo)
{
  FILE *capacityFile = fopen("/sys/class/power_supply/battery/capacity", "r");
  double capacity = dom::battery::kDefaultLevel * 100;
  if (capacityFile)
    fscanf(capacityFile, "%lf", &capacity);
  fclose(capacityFile);

  FILE *chargingFile = fopen("/sys/class/power_supply/battery/charging_source", "r");
  int chargingSrc = 1;
  if (chargingFile)
    fscanf(chargingFile, "%d", &chargingSrc);
  fclose(chargingFile);

  aBatteryInfo->level() = capacity / 100;
  aBatteryInfo->charging() = chargingSrc == 1;
  aBatteryInfo->remainingTime() = dom::battery::kUnknownRemainingTime;
}

namespace {




const char *screenEnabledFilename = "/sys/power/state";
const char *screenBrightnessFilename = "/sys/class/backlight/pwm-backlight/brightness";

template<ssize_t n>
bool ReadFromFile(const char *filename, char (&buf)[n])
{
  int fd = open(filename, O_RDONLY);
  ScopedClose autoClose(fd);
  if (fd < 0) {
    HAL_LOG(("Unable to open file %s.", filename));
    return false;
  }

  ssize_t numRead = read(fd, buf, n);
  if (numRead < 0) {
    HAL_LOG(("Error reading from file %s.", filename));
    return false;
  }

  buf[PR_MIN(numRead, n - 1)] = '\0';
  return true;
}

void WriteToFile(const char *filename, const char *toWrite)
{
  int fd = open(filename, O_WRONLY);
  ScopedClose autoClose(fd);
  if (fd < 0) {
    HAL_LOG(("Unable to open file %s.", filename));
    return;
  }

  if (write(fd, toWrite, strlen(toWrite)) < 0) {
    HAL_LOG(("Unable to write to file %s.", filename));
    return;
  }
}




bool sScreenEnabled = true;

} 

bool
GetScreenEnabled()
{
  return sScreenEnabled;
}

void
SetScreenEnabled(bool enabled)
{
  WriteToFile(screenEnabledFilename, enabled ? "on" : "mem");
  sScreenEnabled = enabled;
}

double
GetScreenBrightness()
{
  char buf[32];
  ReadFromFile(screenBrightnessFilename, buf);

  errno = 0;
  unsigned long val = strtoul(buf, NULL, 10);
  if (errno) {
    HAL_LOG(("Cannot parse contents of %s; expected an unsigned "
             "int, but contains \"%s\".",
             screenBrightnessFilename, buf));
    return 1;
  }

  if (val > 255) {
    HAL_LOG(("Got out-of-range brightness %d, truncating to 1.0", val));
    val = 255;
  }

  return val / 255.0;
}

void
SetScreenBrightness(double brightness)
{
  
  
  if (!(0 <= brightness && brightness <= 1)) {
    HAL_LOG(("SetScreenBrightness: Dropping illegal brightness %f.",
             brightness));
    return;
  }

  
  
  int val = static_cast<int>(round(brightness * 255));
  char str[4];
  DebugOnly<int> numChars = snprintf(str, sizeof(str), "%d", val);
  MOZ_ASSERT(numChars < static_cast<int>(sizeof(str)));

  WriteToFile(screenBrightnessFilename, str);
}

} 
} 
