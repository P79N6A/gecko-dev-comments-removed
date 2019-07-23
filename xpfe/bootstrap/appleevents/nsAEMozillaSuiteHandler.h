







































#ifndef nsAEMozillaSuiteHandler_h_
#define nsAEMozillaSuiteHandler_h_

#include "nsAEUtils.h"


class AEMozillaSuiteHandler
{
public:
	enum {
		kSuiteSignature			= 'MOZZ',
		kDoJavaScriptEvent		= 'jscr'
	};
	
						AEMozillaSuiteHandler();
						~AEMozillaSuiteHandler();

	void					HandleMozillaSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply);		

protected:


};



#endif
