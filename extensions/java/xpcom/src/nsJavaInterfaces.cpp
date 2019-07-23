



































#include "nsJavaInterfaces.h"
#include "nsJavaWrapper.h"
#include "nsJavaXPCOMBindingUtils.h"
#include "nsJavaXPTCStub.h"
#include "nsIComponentRegistrar.h"
#include "nsString.h"
#include "nsISimpleEnumerator.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIInputStream.h"
#include "nsEnumeratorUtils.h"
#include "nsAppFileLocProviderProxy.h"
#include "nsXULAppAPI.h"
#include "nsILocalFile.h"

#ifdef XP_MACOSX
#include "jawt.h"
#endif


extern "C" NS_EXPORT void JNICALL
MOZILLA_NATIVE(initialize) (JNIEnv* env, jobject)
{
  if (!InitializeJavaGlobals(env)) {
    jclass clazz =
        env->FindClass("org/mozilla/xpcom/XPCOMInitializationException");
    if (clazz) {
      env->ThrowNew(clazz, "Failed to initialize JavaXPCOM");
    }
  }
}

nsresult
InitEmbedding_Impl(JNIEnv* env, jobject aLibXULDirectory,
                   jobject aAppDirectory, jobject aAppDirProvider)
{
  nsresult rv;

  
  nsCOMPtr<nsILocalFile> libXULDir;
  if (aLibXULDirectory) {
    rv = File_to_nsILocalFile(env, aLibXULDirectory, getter_AddRefs(libXULDir));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  nsCOMPtr<nsILocalFile> appDir;
  if (aAppDirectory) {
    rv = File_to_nsILocalFile(env, aAppDirectory, getter_AddRefs(appDir));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIDirectoryServiceProvider> provider;
  if (aAppDirProvider) {
    rv = NS_NewAppFileLocProviderProxy(aAppDirProvider,
                                       getter_AddRefs(provider));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  return XRE_InitEmbedding(libXULDir, appDir, provider, nsnull, 0);
}

extern "C" NS_EXPORT void JNICALL
GRE_NATIVE(initEmbedding) (JNIEnv* env, jobject, jobject aLibXULDirectory,
                           jobject aAppDirectory, jobject aAppDirProvider)
{
  nsresult rv = InitEmbedding_Impl(env, aLibXULDirectory, aAppDirectory,
                                   aAppDirProvider);

  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "Failure in initEmbedding");
    FreeJavaGlobals(env);
  }
}

extern "C" NS_EXPORT void JNICALL
GRE_NATIVE(termEmbedding) (JNIEnv *env, jobject)
{
  
  
  FreeJavaGlobals(env);

  XRE_TermEmbedding();
}

nsresult
InitXPCOM_Impl(JNIEnv* env, jobject aMozBinDirectory,
               jobject aAppFileLocProvider, jobject* aResult)
{
  nsresult rv;

  
  nsCOMPtr<nsILocalFile> directory;
  if (aMozBinDirectory) {
    rv = File_to_nsILocalFile(env, aMozBinDirectory, getter_AddRefs(directory));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIDirectoryServiceProvider> provider;
  if (aAppFileLocProvider) {
    rv = NS_NewAppFileLocProviderProxy(aAppFileLocProvider,
                                       getter_AddRefs(provider));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIServiceManager> servMan;
  rv = NS_InitXPCOM2(getter_AddRefs(servMan), directory, provider);
  NS_ENSURE_SUCCESS(rv, rv);

  
  return GetNewOrUsedJavaObject(env, servMan, NS_GET_IID(nsIServiceManager),
                                nsnull, aResult);
}

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(initXPCOM) (JNIEnv* env, jobject, jobject aMozBinDirectory,
                         jobject aAppFileLocProvider)
{
  jobject servMan;
  nsresult rv = InitXPCOM_Impl(env, aMozBinDirectory, aAppFileLocProvider,
                               &servMan);
  if (NS_SUCCEEDED(rv))
    return servMan;

  ThrowException(env, rv, "Failure in initXPCOM");
  FreeJavaGlobals(env);
  return nsnull;
}

extern "C" NS_EXPORT void JNICALL
XPCOM_NATIVE(shutdownXPCOM) (JNIEnv *env, jobject, jobject aServMgr)
{
  nsresult rv;
  nsIServiceManager* servMgr = nsnull;
  if (aServMgr) {
    
    rv = GetNewOrUsedXPCOMObject(env, aServMgr, NS_GET_IID(nsIServiceManager),
                                 (nsISupports**) &servMgr);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get XPCOM obj for ServiceMgr.");

    
    
  }

  
  
  FreeJavaGlobals(env);

  rv = NS_ShutdownXPCOM(servMgr);
  if (NS_FAILED(rv))
    ThrowException(env, rv, "NS_ShutdownXPCOM failed");
}

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(newLocalFile) (JNIEnv *env, jobject, jstring aPath,
                            jboolean aFollowLinks)
{
  
  const PRUnichar* buf = nsnull;
  if (aPath) {
    buf = env->GetStringChars(aPath, nsnull);
    if (!buf)
      return nsnull;  
  }

  nsAutoString path_str(buf);
  env->ReleaseStringChars(aPath, buf);

  
  nsCOMPtr<nsILocalFile> file;
  nsresult rv = NS_NewLocalFile(path_str, aFollowLinks, getter_AddRefs(file));

  if (NS_SUCCEEDED(rv)) {
    jobject javaProxy;
    rv = GetNewOrUsedJavaObject(env, file, NS_GET_IID(nsILocalFile),
                                nsnull, &javaProxy);
    if (NS_SUCCEEDED(rv))
      return javaProxy;
  }

  ThrowException(env, rv, "Failure in newLocalFile");
  return nsnull;
}

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(getComponentManager) (JNIEnv *env, jobject)
{
  
  nsCOMPtr<nsIComponentManager> cm;
  nsresult rv = NS_GetComponentManager(getter_AddRefs(cm));

  if (NS_SUCCEEDED(rv)) {
    jobject javaProxy;
    rv = GetNewOrUsedJavaObject(env, cm, NS_GET_IID(nsIComponentManager),
                                nsnull, &javaProxy);
    if (NS_SUCCEEDED(rv))
      return javaProxy;
  }

  ThrowException(env, rv, "Failure in getComponentManager");
  return nsnull;
}

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(getComponentRegistrar) (JNIEnv *env, jobject)
{
  
  nsCOMPtr<nsIComponentRegistrar> cr;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(cr));

  if (NS_SUCCEEDED(rv)) {
    jobject javaProxy;
    rv = GetNewOrUsedJavaObject(env, cr, NS_GET_IID(nsIComponentRegistrar),
                                nsnull, &javaProxy);
    if (NS_SUCCEEDED(rv))
      return javaProxy;
  }

  ThrowException(env, rv, "Failure in getComponentRegistrar");
  return nsnull;
}

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(getServiceManager) (JNIEnv *env, jobject)
{
  
  nsCOMPtr<nsIServiceManager> sm;
  nsresult rv = NS_GetServiceManager(getter_AddRefs(sm));

  if (NS_SUCCEEDED(rv)) {
    jobject javaProxy;
    rv = GetNewOrUsedJavaObject(env, sm, NS_GET_IID(nsIServiceManager),
                                nsnull, &javaProxy);
    if (NS_SUCCEEDED(rv))
      return javaProxy;
  }

  ThrowException(env, rv, "Failure in getServiceManager");
  return nsnull;
}

extern "C" NS_EXPORT jobject JNICALL
GRE_NATIVE(lockProfileDirectory) (JNIEnv* env, jobject, jobject aDirectory)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (aDirectory) {
    nsCOMPtr<nsILocalFile> profileDir;
    rv = File_to_nsILocalFile(env, aDirectory, getter_AddRefs(profileDir));

    if (NS_SUCCEEDED(rv)) {
      nsISupports* lock;
      rv = XRE_LockProfileDirectory(profileDir, &lock);

      if (NS_SUCCEEDED(rv)) {
        jclass clazz =
            env->FindClass("org/mozilla/xpcom/ProfileLock");
        if (clazz) {
          jmethodID mid = env->GetMethodID(clazz, "<init>", "(J)V");
          if (mid) {
            return env->NewObject(clazz, mid, NS_REINTERPRET_CAST(jlong, lock));
          }
        }

        
        rv = NS_ERROR_FAILURE;
      }
    }
  }

  ThrowException(env, rv, "Failure in lockProfileDirectory");
  return nsnull;
}

extern "C" NS_EXPORT void JNICALL
GRE_NATIVE(notifyProfile) (JNIEnv *env, jobject)
{
  XRE_NotifyProfile();
}

#ifdef XP_MACOSX
extern PRUint64 GetPlatformHandle(JAWT_DrawingSurfaceInfo* dsi);
#endif

extern "C" NS_EXPORT jlong JNICALL
MOZILLA_NATIVE(getNativeHandleFromAWT) (JNIEnv* env, jobject clazz,
                                        jobject widget)
{
  PRUint64 handle = 0;

#ifdef XP_MACOSX
  JAWT awt;
  awt.version = JAWT_VERSION_1_4;
  jboolean result = JAWT_GetAWT(env, &awt);
  if (result == JNI_FALSE)
    return 0;
    
  JAWT_DrawingSurface* ds = awt.GetDrawingSurface(env, widget);
  if (ds != nsnull) {
    jint lock = ds->Lock(ds);
    if (!(lock & JAWT_LOCK_ERROR)) {
      JAWT_DrawingSurfaceInfo* dsi = ds->GetDrawingSurfaceInfo(ds);
      if (dsi) {
        handle = GetPlatformHandle(dsi);
        ds->FreeDrawingSurfaceInfo(dsi);
      }

      ds->Unlock(ds);
    }

    awt.FreeDrawingSurface(ds);
  }
#else
  NS_WARNING("getNativeHandleFromAWT JNI method not implemented");
#endif

  return handle;
}

extern "C" NS_EXPORT jlong JNICALL
JXUTILS_NATIVE(wrapJavaObject) (JNIEnv* env, jobject, jobject aJavaObject,
                                jstring aIID)
{
  nsresult rv;
  nsISupports* xpcomObject = nsnull;

  if (!aJavaObject || !aIID) {
    rv = NS_ERROR_NULL_POINTER;
  } else {
    const char* str = env->GetStringUTFChars(aIID, nsnull);
    if (!str) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
      nsID iid;
      if (iid.Parse(str)) {
        rv = GetNewOrUsedXPCOMObject(env, aJavaObject, iid, &xpcomObject);
      } else {
        rv = NS_ERROR_INVALID_ARG;
      }

      env->ReleaseStringUTFChars(aIID, str);
    }
  }

  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "Failed to create XPCOM proxy for Java object");
  }
  return NS_REINTERPRET_CAST(jlong, xpcomObject);
}

extern "C" NS_EXPORT jobject JNICALL
JXUTILS_NATIVE(wrapXPCOMObject) (JNIEnv* env, jobject, jlong aXPCOMObject,
                                 jstring aIID)
{
  nsresult rv;
  jobject javaObject = nsnull;
  nsISupports* xpcomObject = NS_REINTERPRET_CAST(nsISupports*, aXPCOMObject);

  if (!xpcomObject || !aIID) {
    rv = NS_ERROR_NULL_POINTER;
  } else {
    const char* str = env->GetStringUTFChars(aIID, nsnull);
    if (!str) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
      nsID iid;
      if (iid.Parse(str)) {
        
        rv = GetNewOrUsedJavaObject(env, xpcomObject, iid, nsnull, &javaObject);
      } else {
        rv = NS_ERROR_INVALID_ARG;
      }

      env->ReleaseStringUTFChars(aIID, str);
    }
  }

  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "Failed to create XPCOM proxy for Java object");
  }
  return javaObject;
}
