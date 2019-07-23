








































#include "EmbeddedFramePluginInstance.h"
#include "EmbeddedFrame.h"
#include "MRJPlugin.h"

#include "nsIPluginInstancePeer.h"
#include "nsIPluginTagInfo.h"

#include <stdio.h>

EmbeddedFramePluginInstance::EmbeddedFramePluginInstance()
	:	mPeer(NULL), mFrame(NULL)
{
}

EmbeddedFramePluginInstance::~EmbeddedFramePluginInstance()
{
	if (mFrame != NULL)
		delete mFrame;
}

NS_METHOD EmbeddedFramePluginInstance::Initialize(nsIPluginInstancePeer* peer)
{
	mPeer = peer;
	NS_ADDREF(mPeer);

	nsIPluginTagInfo* tagInfo = NULL;
	if (mPeer->QueryInterface(NS_GET_IID(nsIPluginTagInfo), (void**)&tagInfo) == NS_OK) {
		const char* frameValue = NULL;
		if (tagInfo->GetAttribute("JAVAFRAME", &frameValue) == NS_OK) {
			sscanf(frameValue, "%X", &mFrame);
		}
		if (mFrame != NULL)
			mFrame->setPluginInstance(this);
		NS_RELEASE(tagInfo);
	}

	return NS_OK;
}

NS_METHOD EmbeddedFramePluginInstance::GetPeer(nsIPluginInstancePeer* *resultingPeer)
{
	if (mPeer != NULL) {
		*resultingPeer = mPeer;
		mPeer->AddRef();
	}
	return NS_OK;
}

NS_METHOD EmbeddedFramePluginInstance::Destroy()
{
	NS_IF_RELEASE(mPeer);

	
	if (mFrame != NULL) {
		mFrame->showHide(false);
		
		mFrame = NULL;
	}

	return NS_OK;
}

NS_METHOD EmbeddedFramePluginInstance::SetWindow(nsPluginWindow* pluginWindow)
{
	if (mFrame != NULL) {
		if (pluginWindow != NULL)
			mFrame->setWindow(WindowRef(pluginWindow->window->port));
		else
			mFrame->setWindow(NULL);
	}
	return NS_OK;
}

NS_METHOD EmbeddedFramePluginInstance::HandleEvent(nsPluginEvent* pluginEvent, PRBool* eventHandled)
{
	if (mFrame != NULL)
		*eventHandled = mFrame->handleEvent(pluginEvent->event);
	return NS_OK;
}

void EmbeddedFramePluginInstance::setFrame(EmbeddedFrame* frame)
{
	mFrame = frame;
}

NS_IMPL_ISUPPORTS1(EmbeddedFramePluginInstance, nsIPluginInstance)
