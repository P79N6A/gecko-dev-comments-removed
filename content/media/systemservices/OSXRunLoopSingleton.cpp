




#include "OSXRunLoopSingleton.h"
#include <mozilla/StaticMutex.h>
#include <mozilla/NullPtr.h>

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/AudioHardware.h>
#include <CoreAudio/HostTime.h>
#include <CoreFoundation/CoreFoundation.h>

static bool gRunLoopSet = false;
static mozilla::StaticMutex gMutex;

void mozilla_set_coreaudio_notification_runloop_if_needed()
{
  mozilla::StaticMutexAutoLock lock(gMutex);
  if (gRunLoopSet) {
    return;
  }

  


  AudioObjectPropertyAddress runloop_address = {
    kAudioHardwarePropertyRunLoop,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMaster
  };

  CFRunLoopRef run_loop = nullptr;

  OSStatus r;
  r = AudioObjectSetPropertyData(kAudioObjectSystemObject,
                                 &runloop_address,
                                 0, NULL, sizeof(CFRunLoopRef), &run_loop);
  if (r != noErr) {
    NS_WARNING("Could not make global CoreAudio notifications use their own thread.");
  }

  gRunLoopSet = true;
}
