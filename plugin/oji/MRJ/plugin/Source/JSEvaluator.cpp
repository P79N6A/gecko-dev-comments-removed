







































#include "JSEvaluator.h"
#include "MRJPlugin.h"
#include "MRJSession.h"
#include "MRJMonitor.h"
#include "nsIPluginManager.h"

#include <string.h>

extern nsIPluginManager* thePluginManager;

JSEvaluator::JSEvaluator(MRJPluginInstance* pluginInstance)
	:	mPluginInstance(pluginInstance)
{
	NS_ADDREF(pluginInstance);
	mSession = pluginInstance->getSession();
	mJSMonitor = new MRJMonitor(mSession);
}

JSEvaluator::~JSEvaluator()
{
	NS_IF_RELEASE(mPluginInstance);
	if (mJSMonitor != NULL)
		delete mJSMonitor;
}

NS_IMPL_ISUPPORTS1(JSEvaluator, nsIPluginStreamListener)

const char* JSEvaluator::eval(const char* script)
{
	JNIEnv* env = mSession->getCurrentEnv();
	nsIPluginStreamListener* listener = this;

	mJSMonitor->enter();

	while (mScript != NULL) {
		
		mJSMonitor->wait();
	}
	
	
	const char* kJavaScriptPrefix = "javascript:";
	mScript = new char[strlen(kJavaScriptPrefix) + strlen(script) + 1];
	if (mScript != NULL) {
		strcpy(mScript, kJavaScriptPrefix);
		strcat(mScript, script);

		
		nsresult result = thePluginManager->GetURL((nsIPluginInstance*)mPluginInstance, mScript, NULL, (nsIPluginStreamListener*)this);
		
		
		if (mResult != NULL) {
			delete[] mResult;
			mResult = NULL;
		}

		
		mJSMonitor->wait();
		
		
		delete[] mScript;
		mScript = NULL;
	}
	
	mJSMonitor->notifyAll();
	
	mJSMonitor->exit();
	
	return mResult;
}

NS_METHOD JSEvaluator::OnDataAvailable(nsIPluginStreamInfo* pluginInfo, nsIInputStream* input, PRUint32 length)
{
	
	mResult = new char[length + 1];
	if (mResult != NULL) {
		if (input->Read(mResult, length, &length) == NS_OK) {
			
			
			mResult[length] = '\0';
		}
	}
	return NS_OK;
}

NS_METHOD JSEvaluator::OnStopBinding(nsIPluginStreamInfo* pluginInfo, nsresult status)
{
	
	mJSMonitor->notifyAll();
	return NS_OK;
}
