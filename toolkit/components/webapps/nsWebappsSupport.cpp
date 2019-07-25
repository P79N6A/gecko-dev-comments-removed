




































#include "AndroidBridge.h"
#include "nsCRTGlue.h"
#include "nsWebappsSupport.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(nsWebappsSupport, nsIWebappsSupport)

NS_IMETHODIMP
nsWebappsSupport::InstallApplication(const PRUnichar *aTitle, const PRUnichar *aURI, const PRUnichar *aIconURI, const PRUnichar *aIconData)
{
  ALOG("in nsWebappsSupport::InstallApplication()\n");
  AndroidBridge::AutoLocalJNIFrame jniFrame;
  JNIEnv *jEnv = GetJNIForThread();
  jclass jGeckoAppShellClass = GetGeckoAppShellClass();
  
  if (!jEnv || !jGeckoAppShellClass)
    return NS_ERROR_FAILURE;
    
  jmethodID jInstallWebApplication = jEnv->GetStaticMethodID(jGeckoAppShellClass, "installWebApplication", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  jstring jstrURI = jEnv->NewString(aURI, NS_strlen(aURI));
  jstring jstrTitle = jEnv->NewString(aTitle, NS_strlen(aTitle));
  jstring jstrIconData = jEnv->NewString(aIconData, NS_strlen(aIconData));
  
  if (!jstrURI || !jstrTitle || !jstrIconData)
    return NS_ERROR_FAILURE;
    
  jEnv->CallStaticVoidMethod(jGeckoAppShellClass, jInstallWebApplication, jstrURI, jstrTitle, jstrIconData);
  return NS_OK;
}




NS_IMETHODIMP
nsWebappsSupport::IsApplicationInstalled(const PRUnichar *aURI, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

