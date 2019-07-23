






































#ifndef __CWebBrowserCMAttachment__
#define __CWebBrowserCMAttachment__

#include "LCMAttachment.h"












class	CWebBrowserCMAttachment : public LCMAttachment {
public:

	enum { class_ID = FOUR_CHAR_CODE('WBCM') };
				
								CWebBrowserCMAttachment();
								CWebBrowserCMAttachment(LCommander*	inTarget,
										                PaneIDT		inTargetPaneID = PaneIDT_Unspecified);
								CWebBrowserCMAttachment(LStream* inStream);
	
	virtual						~CWebBrowserCMAttachment();
	
	
	virtual	void				ExecuteSelf(MessageT		inMessage,
										    void*			ioParam);
										    
	
	virtual void                SetCommandList(SInt16 mcmdResID);
	virtual void                DoContextMenuClick(const EventRecord&	inMacEvent);


};

#endif
