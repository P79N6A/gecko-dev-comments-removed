















#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPermissionController.h>
#include <private/android_filesystem_config.h>
#include "GonkPermission.h"

#undef LOG
#include <android/log.h>
#define ALOGE(args...)  __android_log_print(ANDROID_LOG_ERROR, "gonkperm" , ## args)

using namespace android;
using namespace mozilla;

bool
GonkPermissionService::checkPermission(const String16& permission, int32_t pid,
                                     int32_t uid)
{
  if (0 == uid)
    return true;

  
  
  

  if (uid < AID_APP) {
    ALOGE("%s for pid=%d,uid=%d denied: not an app",
      String8(permission).string(), pid, uid);
    return false;
  }

  String8 perm8(permission);

  if (perm8 != "android.permission.CAMERA" &&
    perm8 != "android.permission.RECORD_AUDIO") {
    ALOGE("%s for pid=%d,uid=%d denied: unsupported permission",
      String8(permission).string(), pid, uid);
    return false;
  }

  
  
  
  PermissionGrant permGrant(perm8.string(), pid);
  if (nsTArray<PermissionGrant>::NoIndex != mGrantArray.IndexOf(permGrant)) {
    mGrantArray.RemoveElement(permGrant);
    return true;
  }

  char filename[32];
  snprintf(filename, sizeof(filename), "/proc/%d/status", pid);
  FILE *f = fopen(filename, "r");
  if (!f) {
    ALOGE("%s for pid=%d,uid=%d denied: unable to open %s",
      String8(permission).string(), pid, uid, filename);
    return false;
  }

  char line[80];
  while (fgets(line, sizeof(line), f)) {
    char *save;
    char *name = strtok_r(line, "\t", &save);
    if (!name)
      continue;

    if (strcmp(name, "Groups:"))
      continue;
    char *group;
    while ((group = strtok_r(NULL, " \n", &save))) {
      #define _STR(x) #x
      #define STR(x) _STR(x)
      if (!strcmp(group, STR(AID_SDCARD_RW))) {
        fclose(f);
        return true;
      }
    }
    break;
  }
  fclose(f);

  ALOGE("%s for pid=%d,uid=%d denied: missing group",
    String8(permission).string(), pid, uid);
  return false;
}

static GonkPermissionService* gGonkPermissionService = NULL;


void
GonkPermissionService::instantiate()
{
  defaultServiceManager()->addService(String16(getServiceName()),
    GetInstance());
}


GonkPermissionService*
GonkPermissionService::GetInstance()
{
  if (!gGonkPermissionService) {
    gGonkPermissionService = new GonkPermissionService();
  }
  return gGonkPermissionService;
}

void
GonkPermissionService::addGrantInfo(const char* permission, int32_t pid)
{
  mGrantArray.AppendElement(PermissionGrant(permission, pid));
}
