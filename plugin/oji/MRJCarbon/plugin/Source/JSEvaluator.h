








































#pragma once

#include "nsIPluginStreamListener.h"

#ifndef JNI_H
#include "jni.h"
#endif

class MRJMonitor;
class MRJSession;
class MRJPluginInstance;

class JSEvaluator : public nsIPluginStreamListener {
public:
	NS_DECL_ISUPPORTS
	
	JSEvaluator(MRJPluginInstance* pluginInstance);
	virtual ~JSEvaluator();
	
    const char* eval(const char* script);
    
    const char* getResult()
    {
    	return mResult;
    }

	
	
    






    NS_IMETHOD
    OnStartBinding(nsIPluginStreamInfo* pluginInfo)
    {
    	return NS_OK;
    }

    









    NS_IMETHOD
    OnDataAvailable(nsIPluginStreamInfo* pluginInfo, nsIInputStream* input, PRUint32 length);

    NS_IMETHOD
    OnFileAvailable(nsIPluginStreamInfo* pluginInfo, const char* fileName)
    {
		return NS_ERROR_NOT_IMPLEMENTED;
	}
	
    










    NS_IMETHOD
    OnStopBinding(nsIPluginStreamInfo* pluginInfo, nsresult status);

	


    NS_IMETHOD
    GetStreamType(nsPluginStreamType *result)
    {
    	*result = nsPluginStreamType_Normal;
    	return NS_OK;
    }

private:
	MRJPluginInstance*		mPluginInstance;
	MRJSession*				mSession;
	MRJMonitor*				mJSMonitor;
	char*					mScript;
	char*					mResult;
};
