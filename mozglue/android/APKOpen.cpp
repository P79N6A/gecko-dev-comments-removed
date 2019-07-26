










#include <jni.h>
#include <android/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/limits.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>
#include "dlfcn.h"
#include "APKOpen.h"
#include <sys/time.h>
#include <sys/resource.h>
#include "Zip.h"
#include "sqlite3.h"
#include "SQLiteBridge.h"
#include "NSSBridge.h"
#include "ElfLoader.h"
#include "application.ini.h"


#ifndef RUSAGE_THREAD
#define RUSAGE_THREAD 1
#endif

extern "C" {












  NS_EXPORT __attribute__((weak)) void *__dso_handle;
}

typedef int mozglueresult;

enum StartupEvent {
#define mozilla_StartupTimeline_Event(ev, z) ev,
#include "StartupTimeline.h"
#undef mozilla_StartupTimeline_Event
  MAX_STARTUP_EVENT_ID
};

using namespace mozilla;






static uint64_t TimeStamp_Now()
{
  struct timespec ts;
  int rv = clock_gettime(CLOCK_MONOTONIC, &ts);

  if (rv != 0) {
    return 0;
  }

  uint64_t baseNs = (uint64_t)ts.tv_sec * 1000000000;
  return baseNs + (uint64_t)ts.tv_nsec;
}

static struct mapping_info * lib_mapping = NULL;

NS_EXPORT const struct mapping_info *
getLibraryMapping()
{
  return lib_mapping;
}

void
JNI_Throw(JNIEnv* jenv, const char* classname, const char* msg)
{
    __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Throw\n");
    jclass cls = jenv->FindClass(classname);
    if (cls == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Couldn't find exception class (or exception pending) %s\n", classname);
        exit(FAILURE);
    }
    int rc = jenv->ThrowNew(cls, msg);
    if (rc < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Error throwing exception %s\n", msg);
        exit(FAILURE);
    }
    jenv->DeleteLocalRef(cls);
}

#define JNI_STUBS
#include "jni-stubs.inc"
#undef JNI_STUBS

static void * xul_handle = NULL;
#ifndef MOZ_FOLD_LIBS
static void * sqlite_handle = NULL;
static void * nspr_handle = NULL;
static void * plc_handle = NULL;
#else
#define sqlite_handle nss_handle
#define nspr_handle nss_handle
#define plc_handle nss_handle
#endif
static void * nss_handle = NULL;

template <typename T> inline void
xul_dlsym(const char *symbolName, T *value)
{
  *value = (T) (uintptr_t) __wrap_dlsym(xul_handle, symbolName);
}

static int mapping_count = 0;

#define MAX_MAPPING_INFO 32

extern "C" void
report_mapping(char *name, void *base, uint32_t len, uint32_t offset)
{
  if (mapping_count >= MAX_MAPPING_INFO)
    return;

  struct mapping_info *info = &lib_mapping[mapping_count++];
  info->name = strdup(name);
  info->base = (uintptr_t)base;
  info->len = len;
  info->offset = offset;
}

static mozglueresult
loadGeckoLibs(const char *apkName)
{
  chdir(getenv("GRE_HOME"));

  uint64_t t0 = TimeStamp_Now();
  struct rusage usage1_thread, usage1;
  getrusage(RUSAGE_THREAD, &usage1_thread);
  getrusage(RUSAGE_SELF, &usage1);
  
  RefPtr<Zip> zip = ZipCollection::GetZip(apkName);

  char *file = new char[strlen(apkName) + sizeof("!/libxul.so")];
  sprintf(file, "%s!/libxul.so", apkName);
  xul_handle = __wrap_dlopen(file, RTLD_GLOBAL | RTLD_LAZY);
  delete[] file;

  if (!xul_handle) {
    __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Couldn't get a handle to libxul!");
    return FAILURE;
  }

#define JNI_BINDINGS
#include "jni-stubs.inc"
#undef JNI_BINDINGS

  void (*XRE_StartupTimelineRecord)(int, uint64_t);
  xul_dlsym("XRE_StartupTimelineRecord", &XRE_StartupTimelineRecord);

  uint64_t t1 = TimeStamp_Now();
  struct rusage usage2_thread, usage2;
  getrusage(RUSAGE_THREAD, &usage2_thread);
  getrusage(RUSAGE_SELF, &usage2);

#define RUSAGE_TIMEDIFF(u1, u2, field) \
  ((u2.ru_ ## field.tv_sec - u1.ru_ ## field.tv_sec) * 1000 + \
   (u2.ru_ ## field.tv_usec - u1.ru_ ## field.tv_usec) / 1000)

  __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Loaded libs in %lldms total, %ldms(%ldms) user, %ldms(%ldms) system, %ld(%ld) faults",
                      (t1 - t0) / 1000000,
                      RUSAGE_TIMEDIFF(usage1_thread, usage2_thread, utime),
                      RUSAGE_TIMEDIFF(usage1, usage2, utime),
                      RUSAGE_TIMEDIFF(usage1_thread, usage2_thread, stime),
                      RUSAGE_TIMEDIFF(usage1, usage2, stime),
                      usage2_thread.ru_majflt - usage1_thread.ru_majflt,
                      usage2.ru_majflt - usage1.ru_majflt);

  XRE_StartupTimelineRecord(LINKER_INITIALIZED, t0);
  XRE_StartupTimelineRecord(LIBRARIES_LOADED, t1);
  return SUCCESS;
}

static mozglueresult loadNSSLibs(const char *apkName);

static mozglueresult
loadSQLiteLibs(const char *apkName)
{
  if (sqlite_handle)
    return SUCCESS;

#ifdef MOZ_FOLD_LIBS
  if (loadNSSLibs(apkName) != SUCCESS)
    return FAILURE;
#else
  chdir(getenv("GRE_HOME"));

  RefPtr<Zip> zip = ZipCollection::GetZip(apkName);
  if (!lib_mapping) {
    lib_mapping = (struct mapping_info *)calloc(MAX_MAPPING_INFO, sizeof(*lib_mapping));
  }

  char *file = new char[strlen(apkName) + sizeof("!/libmozsqlite3.so")];
  sprintf(file, "%s!/libmozsqlite3.so", apkName);
  sqlite_handle = __wrap_dlopen(file, RTLD_GLOBAL | RTLD_LAZY);
  delete [] file;

  if (!sqlite_handle) {
    __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Couldn't get a handle to libmozsqlite3!");
    return FAILURE;
  }
#endif

  setup_sqlite_functions(sqlite_handle);
  return SUCCESS;
}

static mozglueresult
loadNSSLibs(const char *apkName)
{
  if (nss_handle && nspr_handle && plc_handle)
    return SUCCESS;

  chdir(getenv("GRE_HOME"));

  RefPtr<Zip> zip = ZipCollection::GetZip(apkName);
  if (!lib_mapping) {
    lib_mapping = (struct mapping_info *)calloc(MAX_MAPPING_INFO, sizeof(*lib_mapping));
  }

  char *file = new char[strlen(apkName) + sizeof("!/libnss3.so")];
  sprintf(file, "%s!/libnss3.so", apkName);
  nss_handle = __wrap_dlopen(file, RTLD_GLOBAL | RTLD_LAZY);
  delete [] file;

#ifndef MOZ_FOLD_LIBS
  file = new char[strlen(apkName) + sizeof("!/libnspr4.so")];
  sprintf(file, "%s!/libnspr4.so", apkName);
  nspr_handle = __wrap_dlopen(file, RTLD_GLOBAL | RTLD_LAZY);
  delete [] file;

  file = new char[strlen(apkName) + sizeof("!/libplc4.so")];
  sprintf(file, "%s!/libplc4.so", apkName);
  plc_handle = __wrap_dlopen(file, RTLD_GLOBAL | RTLD_LAZY);
  delete [] file;
#endif

  if (!nss_handle) {
    __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Couldn't get a handle to libnss3!");
    return FAILURE;
  }

#ifndef MOZ_FOLD_LIBS
  if (!nspr_handle) {
    __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Couldn't get a handle to libnspr4!");
    return FAILURE;
  }

  if (!plc_handle) {
    __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Couldn't get a handle to libplc4!");
    return FAILURE;
  }
#endif

  return setup_nss_functions(nss_handle, nspr_handle, plc_handle);
}

extern "C" NS_EXPORT void JNICALL
Java_org_mozilla_gecko_mozglue_GeckoLoader_loadGeckoLibsNative(JNIEnv *jenv, jclass jGeckoAppShellClass, jstring jApkName)
{
  const char* str;
  
  
  str = jenv->GetStringUTFChars(jApkName, NULL);
  if (str == NULL)
    return;

  int res = loadGeckoLibs(str);
  if (res != SUCCESS) {
    JNI_Throw(jenv, "java/lang/Exception", "Error loading gecko libraries");
  }
  jenv->ReleaseStringUTFChars(jApkName, str);
}

extern "C" NS_EXPORT void JNICALL
Java_org_mozilla_gecko_mozglue_GeckoLoader_loadSQLiteLibsNative(JNIEnv *jenv, jclass jGeckoAppShellClass, jstring jApkName, jboolean jShouldExtract) {
  if (jShouldExtract) {
    putenv("MOZ_LINKER_EXTRACT=1");
  }

  const char* str;
  
  
  str = jenv->GetStringUTFChars(jApkName, NULL);
  if (str == NULL)
    return;

  __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Load sqlite start\n");
  mozglueresult rv = loadSQLiteLibs(str);
  if (rv != SUCCESS) {
      JNI_Throw(jenv, "java/lang/Exception", "Error loading sqlite libraries");
  }
  __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Load sqlite done\n");
  jenv->ReleaseStringUTFChars(jApkName, str);
}

extern "C" NS_EXPORT void JNICALL
Java_org_mozilla_gecko_mozglue_GeckoLoader_loadNSSLibsNative(JNIEnv *jenv, jclass jGeckoAppShellClass, jstring jApkName, jboolean jShouldExtract) {
  if (jShouldExtract) {
    putenv("MOZ_LINKER_EXTRACT=1");
  }

  const char* str;
  
  
  str = jenv->GetStringUTFChars(jApkName, NULL);
  if (str == NULL)
    return;

  __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Load nss start\n");
  mozglueresult rv = loadNSSLibs(str);
  if (rv != SUCCESS) {
    JNI_Throw(jenv, "java/lang/Exception", "Error loading nss libraries");
  }
  __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Load nss done\n");
  jenv->ReleaseStringUTFChars(jApkName, str);
}

typedef void (*GeckoStart_t)(void *, const nsXREAppData *);

extern "C" NS_EXPORT void JNICALL
Java_org_mozilla_gecko_mozglue_GeckoLoader_nativeRun(JNIEnv *jenv, jclass jc, jstring jargs)
{
  GeckoStart_t GeckoStart;
  xul_dlsym("GeckoStart", &GeckoStart);
  if (GeckoStart == NULL)
    return;
  
  
  int len = jenv->GetStringUTFLength(jargs);
  
  char *args = (char *) malloc(len + 1);
  jenv->GetStringUTFRegion(jargs, 0, len, args);
  args[len] = '\0';
  GeckoStart(args, &sAppData);
  free(args);
}

typedef int GeckoProcessType;

extern "C" NS_EXPORT mozglueresult
ChildProcessInit(int argc, char* argv[])
{
  int i;
  for (i = 0; i < (argc - 1); i++) {
    if (strcmp(argv[i], "-greomni"))
      continue;

    i = i + 1;
    break;
  }

  if (loadNSSLibs(argv[i]) != SUCCESS) {
    return FAILURE;
  }
  if (loadSQLiteLibs(argv[i]) != SUCCESS) {
    return FAILURE;
  }
  if (loadGeckoLibs(argv[i]) != SUCCESS) {
    return FAILURE;
  }

  GeckoProcessType (*fXRE_StringToChildProcessType)(char*);
  xul_dlsym("XRE_StringToChildProcessType", &fXRE_StringToChildProcessType);

  mozglueresult (*fXRE_InitChildProcess)(int, char**, GeckoProcessType);
  xul_dlsym("XRE_InitChildProcess", &fXRE_InitChildProcess);

  GeckoProcessType proctype = fXRE_StringToChildProcessType(argv[--argc]);

  return fXRE_InitChildProcess(argc, argv, proctype);
}

