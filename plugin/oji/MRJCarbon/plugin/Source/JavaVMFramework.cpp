












































#include <jni.h>
#include <JavaControl.h>

#include <CFURL.h>
#include <CFBundle.h>
#include <CFString.h>
#include <MacErrors.h>

static CFBundleRef getBundle(CFStringRef frameworkPath)
{
	CFBundleRef bundle = NULL;
    
	
	
	CFURLRef bundleURL = CFURLCreateWithFileSystemPath(NULL, frameworkPath, kCFURLPOSIXPathStyle, true);
	if (bundleURL != NULL) {
        bundle = CFBundleCreate(NULL, bundleURL);
        if (bundle != NULL)
        	CFBundleLoadExecutable(bundle);
        CFRelease(bundleURL);
	}
	
	return bundle;
}

#if DEBUG
static void* getSystemFunction(CFStringRef functionName)
{
  static CFBundleRef systemBundle = getBundle(CFSTR("/System/Library/Frameworks/System.framework"));
  if (systemBundle) return CFBundleGetFunctionPointerForName(systemBundle, functionName);
  return NULL;
}



typedef int (*printf_proc_ptr) (const char* format, ...);
static printf_proc_ptr kprintf = (printf_proc_ptr) getSystemFunction(CFSTR("printf"));

#endif



static void* getJavaVMFunction(CFStringRef functionName)
{
  static CFBundleRef javaBundle = getBundle(CFSTR("/System/Library/Frameworks/JavaVM.framework"));
  if (javaBundle) return CFBundleGetFunctionPointerForName(javaBundle, functionName);
  return NULL;
}

typedef jint JNICALL (*JNI_GetDefaultJavaVMInitArgs_proc_ptr) (void *args);
static JNI_GetDefaultJavaVMInitArgs_proc_ptr _JNI_GetDefaultJavaVMInitArgs = (JNI_GetDefaultJavaVMInitArgs_proc_ptr) getJavaVMFunction(CFSTR("JNI_GetDefaultJavaVMInitArgs"));

_JNI_IMPORT_OR_EXPORT_ jint JNICALL
JNI_GetDefaultJavaVMInitArgs(void *args)
{
#if DEBUG
    kprintf("_JNI_GetDefaultJavaVMInitArgs = 0x%08X\n", _JNI_GetDefaultJavaVMInitArgs);
#endif
    if (_JNI_GetDefaultJavaVMInitArgs) return _JNI_GetDefaultJavaVMInitArgs(args);
    return -1;
}

typedef jint JNICALL (*JNI_CreateJavaVM_proc_ptr) (JavaVM **pvm, void **penv, void *args);
static JNI_CreateJavaVM_proc_ptr _JNI_CreateJavaVM = (JNI_CreateJavaVM_proc_ptr) getJavaVMFunction(CFSTR("JNI_CreateJavaVM"));

_JNI_IMPORT_OR_EXPORT_ jint JNICALL
JNI_CreateJavaVM(JavaVM **pvm, void **penv, void *args)
{
#if DEBUG
    kprintf("_JNI_CreateJavaVM = 0x%08X\n", _JNI_CreateJavaVM);
#endif
    if (_JNI_CreateJavaVM) return _JNI_CreateJavaVM(pvm, penv, args);
    return -1;
}
