







































#include "nsLiveconnect.h"
#include "MRJMonitor.h"
#include "MRJPlugin.h"
#include "nsIPluginManager.h"

#include <string.h>

extern nsIPluginManager* thePluginManager;

const InterfaceInfo nsLiveconnect::sInterfaces[] = {
    { NS_ILIVECONNECT_IID, INTERFACE_OFFSET(nsLiveconnect, nsILiveconnect) },
    { NS_IPLUGINSTREAMLISTENER_IID, INTERFACE_OFFSET(nsLiveconnect, nsIPluginStreamListener) },
};
const UInt32 nsLiveconnect::kInterfaceCount = sizeof(sInterfaces) / sizeof(InterfaceInfo);

nsLiveconnect::nsLiveconnect()
    :   SupportsMixin(this, sInterfaces, kInterfaceCount),
        mJavaScriptMonitor(NULL), mScript(NULL), mResult(NULL)
{
}

nsLiveconnect::~nsLiveconnect()
{
    delete mJavaScriptMonitor;
}

static char* u2c(const jchar *ustr, jsize length)
{
    char* result = new char[length + 1];
    if (result != NULL) {
        char* cstr = result;
        while (length-- > 0) {
            *cstr++ = (char) *ustr++;
        }
        *cstr = '\0';
    }
    return result;
}

const char* kJavaScriptPrefix = "javascript:";

NS_METHOD   
nsLiveconnect::Eval(JNIEnv *env, jsobject obj, const jchar *script, jsize length, void* principalsArray[], 
                    int numPrincipals, nsISupports *securitySupports, jobject *outResult)
{
    MRJPluginInstance* pluginInstance = (MRJPluginInstance*) obj;
    nsIPluginStreamListener* listener = this;

    if (!mJavaScriptMonitor) {
        mJavaScriptMonitor = new MRJMonitor(pluginInstance->getSession());
        if (!mJavaScriptMonitor)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    mJavaScriptMonitor->enter();

    while (mScript != NULL) {
        
        mJavaScriptMonitor->wait();
    }

    nsresult rv = NS_OK;    
    
    char* cscript = u2c(script, length);
    if (!cscript) {
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        mScript = new char[strlen(kJavaScriptPrefix) + length + 1];
        if (!mScript) {
            rv = NS_ERROR_OUT_OF_MEMORY;
        } else {
            strcpy(mScript, kJavaScriptPrefix);
            strcat(mScript, cscript);
            delete[] cscript;
            rv = thePluginManager->GetURL((nsIPluginInstance*)pluginInstance, mScript, NULL, listener);

            
            mJavaScriptMonitor->wait();

            
            *outResult = NULL;

            
            if (mResult != NULL) {
                *outResult = env->NewStringUTF(mResult);
                delete[] mResult;
                mResult = NULL;
            }

            delete[] mScript;
            mScript = NULL;
        }
    }

    mJavaScriptMonitor->notifyAll();

    mJavaScriptMonitor->exit();

    return rv;
}

NS_METHOD nsLiveconnect::OnDataAvailable(nsIPluginStreamInfo* pluginInfo, nsIInputStream* input, PRUint32 length)
{
    
    mResult = new char[length + 1];
    if (mResult != NULL) {
        if (input->Read(mResult, length, &length) == NS_OK) {
            
            
            mResult[length] = '\0';
        }
    }
    return NS_OK;
}

NS_METHOD nsLiveconnect::OnStopBinding(nsIPluginStreamInfo* pluginInfo, nsresult status)
{
    
    mJavaScriptMonitor->notifyAll();
    return NS_OK;
}
