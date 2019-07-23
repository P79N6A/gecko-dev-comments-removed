






































#include "CPrintAttachment.h"
#include "CBrowserShell.h"


#include "nsIPrintingPromptService.h"
#include "nsIDOMWindow.h"
#include "nsServiceManagerUtils.h"
#include "nsIWebBrowserPrint.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebProgressListener.h"
#include "nsIWebBrowserChrome.h"
#include "nsIEmbeddingSiteWindow.h"






CPrintAttachment::CPrintAttachment(PaneIDT inBrowserPaneID,
                                   MessageT inMessage,
                                   Boolean inExecuteHost) :
    LAttachment(inMessage,inExecuteHost),
    mBrowserShell(nil), mBrowserShellPaneID(inBrowserPaneID)
{
}


CPrintAttachment::CPrintAttachment(LStream* inStream) :
    LAttachment(inStream),
    mBrowserShell(nil), mBrowserShellPaneID(PaneIDT_Unspecified)
{
	*inStream >> mBrowserShellPaneID;
}


CPrintAttachment::~CPrintAttachment()
{
}





void CPrintAttachment::SetOwnerHost(LAttachable* inOwnerHost)
{
    LAttachment::SetOwnerHost(inOwnerHost);
    
	if (mBrowserShell == nil) {
		if (mBrowserShellPaneID != PaneIDT_Unspecified) {
			LView*	container = GetTopmostView(dynamic_cast<LPane*>(mOwnerHost));
			if (container != nil) {
				LPane* targetPane = container->FindPaneByID(mBrowserShellPaneID);
				if (targetPane != nil)
					mBrowserShell = dynamic_cast<CBrowserShell*>(targetPane);
			}
		}
		else
			mBrowserShell = dynamic_cast<CBrowserShell*>(mOwnerHost);

	    Assert_(mBrowserShell != nil);		
	}
}

void CPrintAttachment::ExecuteSelf(MessageT inMessage, void *ioParam)
{
	mExecuteHost = true;

	if (inMessage == msg_CommandStatus) {
		SCommandStatus	*status = (SCommandStatus *)ioParam;
		if (status->command == cmd_Print) {
			*status->enabled = true;
			*status->usesMark = false;
			mExecuteHost = false; 
		}
		else if (status->command == cmd_PageSetup) {
			*status->enabled = true;
			*status->usesMark = false;
			mExecuteHost = false; 
		}
	}
	else if (inMessage == cmd_Print) {
	    DoPrint();
	    mExecuteHost = false; 
	}
	else if (inMessage == cmd_PageSetup) {
	    DoPageSetup();
	    mExecuteHost = false; 
	}
}





void CPrintAttachment::DoPrint()
{
    nsCOMPtr<nsIWebBrowser> wb;
    mBrowserShell->GetWebBrowser(getter_AddRefs(wb));
    ThrowIfNil_(wb);
    nsCOMPtr<nsIWebBrowserPrint> wbPrint(do_GetInterface(wb));
    ThrowIfNil_(wbPrint);
    nsCOMPtr<nsIPrintSettings> settings;
    mBrowserShell->GetPrintSettings(getter_AddRefs(settings));
    ThrowIfNil_(settings);
       
    nsresult rv = wbPrint->Print(settings, nsnull);
    if (rv != NS_ERROR_ABORT)
        ThrowIfError_(rv);
}


void CPrintAttachment::DoPageSetup()
{
    nsCOMPtr<nsIWebBrowser> wb;
    mBrowserShell->GetWebBrowser(getter_AddRefs(wb));
    ThrowIfNil_(wb);
    nsCOMPtr<nsIDOMWindow> domWindow;
    wb->GetContentDOMWindow(getter_AddRefs(domWindow));
    ThrowIfNil_(domWindow);
    nsCOMPtr<nsIPrintSettings> settings;
    mBrowserShell->GetPrintSettings(getter_AddRefs(settings));
    ThrowIfNil_(settings);
    
    nsCOMPtr<nsIPrintingPromptService> printingPromptService =
             do_GetService("@mozilla.org/embedcomp/printingprompt-service;1");
    ThrowIfNil_(printingPromptService);

    nsresult rv = printingPromptService->ShowPageSetup(domWindow, settings, nsnull);
    if (rv != NS_ERROR_ABORT)
        ThrowIfError_(rv);
}


LView* CPrintAttachment::GetTopmostView(LPane*	inStartPane)
{
										
										
										
	LView*	theView = dynamic_cast<LView*>(inStartPane);
	
	if (inStartPane != nil) {
										
		LView*	superView = inStartPane->GetSuperView();
		
		while (superView != nil) {		
			theView = superView;		
			superView = theView->GetSuperView();
		}
	}
	
	return theView;
}

