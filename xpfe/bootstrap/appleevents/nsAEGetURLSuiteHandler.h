







































#ifndef nsAEGetURLSuiteHandler_h_
#define nsAEGetURLSuiteHandler_h_

#include "nsAEUtils.h"
#include "prtypes.h"


class AEGetURLSuiteHandler
{
public:
	enum {
		kSuiteSignature			= 'GURL',
		kGetURLEvent			= 'GURL',
		
		kInsideWindowParameter	= 'HWIN',
		kReferrerParameter		= 'refe'		
	};
	
						AEGetURLSuiteHandler();
						~AEGetURLSuiteHandler();

	void				HandleGetURLSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply);	

protected:

	void				HandleGetURLEvent(const AppleEvent *appleEvent, AppleEvent *reply);
};




#endif
