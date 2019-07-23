





































#ifndef nsAESpyglassSuiteHandler_h_
#define nsAESpyglassSuiteHandler_h_


#include "nsAEUtils.h"

class nsDocLoadObserver;

class AESpyglassSuiteHandler
{
public:
	enum {
		kSuiteSignature					= 'WWW!',
				
		kOpenURLEvent					= 'OURL',			
		kRegisterURLEchoEvent		= 'RGUE',			
		kUnregisterURLEchoEvent	= 'UNRU',			


		kRegisterViewerEvent 		= 'RGVW',			
		kUnregisterViewerEvent	= 'UNRV',			
		kShowFileEvent					= 'SHWF',			
		kParseAnchorEvent			= 'PRSA',			
		kSpyActivateEvent			= 'ACTV',			
		kSpyListWindowsEvent		= 'LSTW',			
		kGetWindowInfoEvent		= 'WNFO',			
		kRegisterWinCloseEvent	= 'RGWC',			
		kUnregisterWinCloseEvent	= 'UNRC',			
		kRegisterProtocolEvent		= 'RGPR',			
		kUnregisterProtocolEvent	= 'UNRP',			
		kCancelProgressEvent		= 'CNCL',			
		kFindURLEvent					= 'FURL',			



		kSpyglassSendSignature	= 'WWW?',
		kSendURLEchoEvent			= 'URLE'

	};

	
	enum {
		kParamSaveToFileDest		= 'INTO',		
		kParamOpenInWindow		= 'WIND',		
		kParamOpenFlags			= 'FLGS',		
		kParamPostData			= 'POST',		
		kParamPostType			= 'MIME',		
		kParamProgressApp			= 'PROG'		
	};
	
						AESpyglassSuiteHandler();
						~AESpyglassSuiteHandler();

	void					HandleSpyglassSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply);	

protected:

	void					HandleOpenURLEvent(const AppleEvent *appleEvent, AppleEvent *reply);
	
	void					HandleRegisterURLEchoEvent(const AppleEvent *appleEvent, AppleEvent *reply);
	void					HandleUnregisterURLEchoEvent(const AppleEvent *appleEvent, AppleEvent *reply);

protected:

    nsDocLoadObserver*  mDocObserver;
    
};



#endif
