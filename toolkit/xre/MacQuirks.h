







































#ifndef MacQuirks_h__
#define MacQuirks_h__

#include <sys/types.h>
#include <sys/sysctl.h>
#include "CoreFoundation/CoreFoundation.h"
#include "CoreServices/CoreServices.h"
#include "Carbon/Carbon.h"

static void
TriggerQuirks()
{
  int mib[2];

  mib[0] = CTL_KERN;
  mib[1] = KERN_OSRELEASE;
  
  char release[sizeof("10.7.99")];
  size_t len = sizeof(release);
  
  int ret = sysctl(mib, 2, release, &len, NULL, 0);
  
  
  
  if (ret == 0 && NS_CompareVersions(release, "10.8.0") >= 0 && NS_CompareVersions(release, "11") < 0) {
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (mainBundle) {
      CFRetain(mainBundle);

      CFStringRef bundleID = CFBundleGetIdentifier(mainBundle);
      if (bundleID) {
        CFRetain(bundleID);

        CFMutableDictionaryRef dict = (CFMutableDictionaryRef)CFBundleGetInfoDictionary(mainBundle);
        CFDictionarySetValue(dict, CFSTR("CFBundleIdentifier"), CFSTR("org.mozilla.firefox"));

        
        
        
#ifdef __i386__
        ProcessSerialNumber psn;
        ::GetCurrentProcess(&psn);
#else
        SInt32 major;
        ::Gestalt(gestaltSystemVersionMajor, &major);
#endif

        
        dict = (CFMutableDictionaryRef)CFBundleGetInfoDictionary(mainBundle);
        CFDictionarySetValue(dict, CFSTR("CFBundleIdentifier"), bundleID);

        CFRelease(bundleID);
      }
      CFRelease(mainBundle);
    }
  }
}

#endif 
