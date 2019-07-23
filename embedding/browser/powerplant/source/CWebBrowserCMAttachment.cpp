






































#include "CWebBrowserCMAttachment.h"













CWebBrowserCMAttachment::CWebBrowserCMAttachment()
{
}

CWebBrowserCMAttachment::CWebBrowserCMAttachment(LCommander*    inTarget,
	                                             PaneIDT		inTargetPaneID)
	: LCMAttachment(inTarget, inTargetPaneID)
{
}

CWebBrowserCMAttachment::CWebBrowserCMAttachment(LStream*	inStream)
	: LCMAttachment(inStream)
{
}

CWebBrowserCMAttachment::~CWebBrowserCMAttachment()
{
}


void CWebBrowserCMAttachment::ExecuteSelf(MessageT	inMessage,
	                                      void*		ioParam)
{
	SetExecuteHost(true);
	
		
		
	if (not mCMMInitialized) {
		return;
	}

		
		
	if (inMessage == msg_AdjustCursor) {
	
			
		if (((static_cast<EventRecord*>(ioParam))->modifiers & controlKey) != 0) {
		
				
				
			UCursor::SetTheCursor(mCMMCursorID);
			SetExecuteHost(false);
		}
		
	} else if (inMessage == msg_Event) {
	
	} else if (inMessage == msg_Click) {

			
		if (::IsShowContextualMenuClick(
				&(static_cast<SMouseDownEvent*>(ioParam))->macEvent)) {

			
			
			
			LCommander*	target = FindCommandTarget();
			if ((target != nil) && LCommander::SwitchTarget(target)) {
				LPane*	ownerAsPane = dynamic_cast<LPane*>(mOwnerHost);				
				if (ownerAsPane != nil) {
					ownerAsPane->UpdatePort();
				}
			}
		}
	}
}


void CWebBrowserCMAttachment::SetCommandList(SInt16 mcmdResID)
{
    Handle mcmdRes = ::GetResource(ResType_MenuCommands, mcmdResID);
    ThrowIfResFail_(mcmdRes);
    ::DetachResource(mcmdRes);  
    LHandleStream inStream(mcmdRes);
    
    mCommandList.RemoveItemsAt(mCommandList.GetCount(), LArray::index_First);
    
	UInt16 numCommands;
	inStream >> numCommands;
	
	for (SInt16 i = 1; i <= numCommands; i++) {
		CommandT theCommand;
		inStream >> theCommand;
		mCommandList.AddItem(theCommand);
	}
}


void CWebBrowserCMAttachment::DoContextMenuClick(const EventRecord&	inMacEvent)
{
    DoCMMClick(inMacEvent);
}

