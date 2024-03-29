










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
#include <fcntl.h>
#include <unistd.h>
#include <zlib.h>
#include "dlfcn.h"
#include "APKOpen.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include "sqlite3.h"
#include "SQLiteBridge.h"
#include "NSSBridge.h"
#include "ElfLoader.h"
#include "application.ini.h"

#include "mozilla/TimeStamp.h"


#ifndef RUSAGE_THREAD
#define RUSAGE_THREAD 1
#endif

#ifndef RELEASE_BUILD







__attribute__((constructor))
void make_dumpable() {
  prctl(PR_SET_DUMPABLE, 1);
}
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

static struct mapping_info * lib_mapping = nullptr;

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
    if (cls == nullptr) {
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

namespace {
    JavaVM* sJavaVM;
}

void
abortThroughJava(const char* msg)
{
    struct sigaction sigact = {};
    if (SEGVHandler::__wrap_sigaction(SIGSEGV, nullptr, &sigact)) {
        return; 
    }

    Dl_info info = {};
    if ((sigact.sa_flags & SA_SIGINFO) &&
        __wrap_dladdr(reinterpret_cast<void*>(sigact.sa_sigaction), &info) &&
        info.dli_fname && strstr(info.dli_fname, "libxul.so")) {

        return; 
    }

    JNIEnv* env = nullptr;
    if (!sJavaVM || sJavaVM->AttachCurrentThreadAsDaemon(&env, nullptr) != JNI_OK) {
        return;
    }

    if (!env || env->PushLocalFrame(2) != JNI_OK) {
        return;
    }

    jclass loader = env->FindClass("org/mozilla/gecko/mozglue/GeckoLoader");
    if (!loader) {
        return;
    }

    jmethodID method = env->GetStaticMethodID(loader, "abort", "(Ljava/lang/String;)V");
    jstring str = env->NewStringUTF(msg);

    if (method && str) {
        env->CallStaticVoidMethod(loader, method, str);
    }

    env->PopLocalFrame(nullptr);
}

#define JNI_STUBS
#include "jni-stubs.inc"
#undef JNI_STUBS

static void * xul_handle = nullptr;
#ifndef MOZ_FOLD_LIBS
static void * sqlite_handle = nullptr;
static void * nspr_handle = nullptr;
static void * plc_handle = nullptr;
#else
#define sqlite_handle nss_handle
#define nspr_handle nss_handle
#define plc_handle nss_handle
#endif
static void * nss_handle = nullptr;

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
  TimeStamp t0 = TimeStamp::Now();
  struct rusage usage1_thread, usage1;
  getrusage(RUSAGE_THREAD, &usage1_thread);
  getrusage(RUSAGE_SELF, &usage1);
  
  char *file = new char[strlen(apkName) + sizeof("!/assets/" ANDROID_CPU_ARCH "/libxul.so")];
  sprintf(file, "%s!/assets/" ANDROID_CPU_ARCH "/libxul.so", apkName);
  xul_handle = __wrap_dlopen(file, RTLD_GLOBAL | RTLD_LAZY);
  delete[] file;

  if (!xul_handle) {
    __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Couldn't get a handle to libxul!");
    return FAILURE;
  }

#define JNI_BINDINGS
#include "jni-stubs.inc"
#undef JNI_BINDINGS

  void (*XRE_StartupTimelineRecord)(int, TimeStamp);
  xul_dlsym("XRE_StartupTimelineRecord", &XRE_StartupTimelineRecord);

  TimeStamp t1 = TimeStamp::Now();
  struct rusage usage2_thread, usage2;
  getrusage(RUSAGE_THREAD, &usage2_thread);
  getrusage(RUSAGE_SELF, &usage2);

#define RUSAGE_TIMEDIFF(u1, u2, field) \
  ((u2.ru_ ## field.tv_sec - u1.ru_ ## field.tv_sec) * 1000 + \
   (u2.ru_ ## field.tv_usec - u1.ru_ ## field.tv_usec) / 1000)

  __android_log_print(ANDROID_LOG_ERROR, "GeckoLibLoad", "Loaded libs in %fms total, %ldms(%ldms) user, %ldms(%ldms) system, %ld(%ld) faults",
                      (t1 - t0).ToMilliseconds(),
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
  if (!lib_mapping) {
    lib_mapping = (struct mapping_info *)calloc(MAX_MAPPING_INFO, sizeof(*lib_mapping));
  }

  char *file = new char[strlen(apkName) + sizeof("!/assets/" ANDROID_CPU_ARCH "/libmozsqlite3.so")];
  sprintf(file, "%s!/assets/" ANDROID_CPU_ARCH "/libmozsqlite3.so", apkName);
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

  if (!lib_mapping) {
    lib_mapping = (struct mapping_info *)calloc(MAX_MAPPING_INFO, sizeof(*lib_mapping));
  }

  char *file = new char[strlen(apkName) + sizeof("!/assets/" ANDROID_CPU_ARCH "/libnss3.so")];
  sprintf(file, "%s!/assets/" ANDROID_CPU_ARCH "/libnss3.so", apkName);
  nss_handle = __wrap_dlopen(file, RTLD_GLOBAL | RTLD_LAZY);
  delete [] file;

#ifndef MOZ_FOLD_LIBS
  file = new char[strlen(apkName) + sizeof("!/assets/" ANDROID_CPU_ARCH "/libnspr4.so")];
  sprintf(file, "%s!/assets/" ANDROID_CPU_ARCH "/libnspr4.so", apkName);
  nspr_handle = __wrap_dlopen(file, RTLD_GLOBAL | RTLD_LAZY);
  delete [] file;

  file = new char[strlen(apkName) + sizeof("!/assets/" ANDROID_CPU_ARCH "/libplc4.so")];
  sprintf(file, "%s!/assets/" ANDROID_CPU_ARCH "/libplc4.so", apkName);
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
  jenv->GetJavaVM(&sJavaVM);

  const char* str;
  
  
  str = jenv->GetStringUTFChars(jApkName, nullptr);
  if (str == nullptr)
    return;

  int res = loadGeckoLibs(str);
  if (res != SUCCESS) {
    JNI_Throw(jenv, "java/lang/Exception", "Error loading gecko libraries");
  }
  jenv->ReleaseStringUTFChars(jApkName, str);
}

extern "C" NS_EXPORT void JNICALL
Java_org_mozilla_gecko_mozglue_GeckoLoader_loadSQLiteLibsNative(JNIEnv *jenv, jclass jGeckoAppShellClass, jstring jApkName) {
  const char* str;
  
  
  str = jenv->GetStringUTFChars(jApkName, nullptr);
  if (str == nullptr)
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
Java_org_mozilla_gecko_mozglue_GeckoLoader_loadNSSLibsNative(JNIEnv *jenv, jclass jGeckoAppShellClass, jstring jApkName) {
  const char* str;
  
  
  str = jenv->GetStringUTFChars(jApkName, nullptr);
  if (str == nullptr)
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
  if (GeckoStart == nullptr)
    return;
  
  
  int len = jenv->GetStringUTFLength(jargs);
  
  char *args = (char *) malloc(len + 1);
  jenv->GetStringUTFRegion(jargs, 0, len, args);
  args[len] = '\0';
  ElfLoader::Singleton.ExpectShutdown(false);
  GeckoStart(args, &sAppData);
  ElfLoader::Singleton.ExpectShutdown(true);
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

  void (*fXRE_SetProcessType)(char*);
  xul_dlsym("XRE_SetProcessType", &fXRE_SetProcessType);

  mozglueresult (*fXRE_InitChildProcess)(int, char**, void*);
  xul_dlsym("XRE_InitChildProcess", &fXRE_InitChildProcess);

  fXRE_SetProcessType(argv[--argc]);

  return fXRE_InitChildProcess(argc, argv, nullptr);
}

