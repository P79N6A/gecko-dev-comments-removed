







































#pragma once

#include "nsIPluginInstance.h"

class EmbeddedFrame;
class MRJPluginInstance;

class EmbeddedFramePluginInstance : public nsIPluginInstance {
public:
	EmbeddedFramePluginInstance();
	virtual ~EmbeddedFramePluginInstance();
	
	NS_DECL_ISUPPORTS
	
    






	NS_IMETHOD
    Initialize(nsIPluginInstancePeer* peer);

    








    NS_IMETHOD
    GetPeer(nsIPluginInstancePeer* *resultingPeer);

    







    NS_IMETHOD
    Start(void)
    {
    	return NS_OK;
    }

    







    NS_IMETHOD
    Stop(void)
    {
    	return NS_OK;
    }

    







    NS_IMETHOD
    Destroy(void);

    







    NS_IMETHOD
    SetWindow(nsPluginWindow* window);

    








    NS_IMETHOD
    NewStream(nsIPluginStreamListener** listener)
    {
    	*listener = NULL;
    	return NS_ERROR_NOT_IMPLEMENTED;
    }

    







    NS_IMETHOD
    Print(nsPluginPrint* platformPrint)
    {
		return NS_ERROR_NOT_IMPLEMENTED;
    }

    






    NS_IMETHOD
    GetValue(nsPluginInstanceVariable variable, void *value)
    {
		return NS_ERROR_NOT_IMPLEMENTED;
    }

    
















    NS_IMETHOD
    HandleEvent(nsPluginEvent* event, PRBool* handled);

	void setFrame(EmbeddedFrame* frame);

private:
	nsIPluginInstancePeer* mPeer;
	MRJPluginInstance* mParentInstance;
	EmbeddedFrame* mFrame;
};
