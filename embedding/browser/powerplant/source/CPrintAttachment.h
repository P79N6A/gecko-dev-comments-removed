





































#ifndef __CPrintAttachment__
#define __CPrintAttachment__

#include <LAttachment.h>

class CBrowserShell;

class CPrintAttachment : public LAttachment
{
public:
	enum { class_ID = FOUR_CHAR_CODE('BRPR') };

                            CPrintAttachment(PaneIDT    inBrowserPaneID,
                                             MessageT   inMessage = msg_AnyMessage,
                                             Boolean    inExecuteHost = true);
                            
                            CPrintAttachment(LStream*   inStream);
                            
    virtual                 ~CPrintAttachment();

	
	virtual void	        SetOwnerHost(LAttachable *inOwnerHost);

	virtual	void            ExecuteSelf(MessageT		inMessage,
                                        void*			ioParam);
    
    virtual void            DoPrint();
    virtual void            DoPageSetup();

protected:
    static LView*           GetTopmostView(LPane*	inStartPane);
    
protected:
    CBrowserShell           *mBrowserShell;
    PaneIDT                 mBrowserShellPaneID;
};

#endif
