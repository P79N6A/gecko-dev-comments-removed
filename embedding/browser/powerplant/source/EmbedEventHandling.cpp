






































#include "EmbedEventHandling.h"
#include "CTextInputEventHandler.h"

#include "prthread.h"

#if defined(__MWERKS__) && defined(DEBUG) && !TARGET_CARBON
#include "SIOUX.h"
#endif

#include "nsIWidget.h"
#include "nsIEventSink.h"
#include "nsCOMPtr.h"



WindowPtr        CEmbedEventAttachment::mLastAlienWindowClicked;








CEmbedEventAttachment::CEmbedEventAttachment()
{
}

CEmbedEventAttachment::~CEmbedEventAttachment()
{
}








void
CEmbedEventAttachment::GetTopWidget ( WindowPtr aWindow, nsIWidget** outWidget )
{
  nsIWidget* topLevelWidget = nsnull;
	::GetWindowProperty ( aWindow, 'MOSS', 'GEKO', sizeof(nsIWidget*), nsnull, (void*)&topLevelWidget);
  if ( topLevelWidget ) {
    *outWidget = topLevelWidget;
    NS_ADDREF(*outWidget);
  }
}








void
CEmbedEventAttachment::GetWindowEventSink ( WindowPtr aWindow, nsIEventSink** outSink )
{
  *outSink = nsnull;
  
  nsCOMPtr<nsIWidget> topWidget;
  GetTopWidget ( aWindow, getter_AddRefs(topWidget) );
  nsCOMPtr<nsIEventSink> sink ( do_QueryInterface(topWidget) );
  if ( sink ) {
    *outSink = sink;
    NS_ADDREF(*outSink);
  }
}


void CEmbedEventAttachment::ExecuteSelf(MessageT	inMessage,
	                                    void*		ioParam)
{
	SetExecuteHost(true);

    EventRecord *inMacEvent;
    WindowPtr	macWindowP;
	
	
	
	
	
	
	
		
    if (inMessage == msg_AdjustCursor) {
        inMacEvent = static_cast<EventRecord*>(ioParam);
        ::MacFindWindow(inMacEvent->where, &macWindowP);
        if (IsAlienGeckoWindow(macWindowP)) {
            nsCOMPtr<nsIEventSink> sink;
            GetWindowEventSink(macWindowP, getter_AddRefs(sink));
            if ( sink ) {
              PRBool handled = PR_FALSE;
              sink->DispatchEvent(inMacEvent, &handled);
              SetExecuteHost(false);
            }
        }
		
	} else if (inMessage == msg_Event) {
        inMacEvent = static_cast<EventRecord*>(ioParam);

#if defined(__MWERKS__) && defined(DEBUG) && !TARGET_CARBON
        
        
        if ((inMacEvent->what == mouseDown ||
            inMacEvent->what == updateEvt ||
            inMacEvent->what == activateEvt ||
            inMacEvent->what == keyDown) &&
            SIOUXHandleOneEvent(inMacEvent))
        {
            SetExecuteHost(false);
            return;
        }
#endif
    	 
    	
    	
    	
    	
    	   	  
        switch (inMacEvent->what)
  	    {	
          case mouseDown:
          {
              mLastAlienWindowClicked = nil;
              
              
              
              
              WindowPtr frontWindow = ::FrontWindow();
              if (IsAlienGeckoWindow(frontWindow)) {
                 nsCOMPtr<nsIEventSink> sink;
                 GetWindowEventSink(frontWindow, getter_AddRefs(sink));
                 if (sink) {
                   PRBool handled = PR_FALSE;
                   sink->DispatchEvent(inMacEvent, &handled);
                   SInt16 thePart = ::MacFindWindow(inMacEvent->where, &macWindowP);
                   if ((thePart == inContent) && (frontWindow == macWindowP)) {
                     
                     
                     mLastAlienWindowClicked = macWindowP;
                     SetExecuteHost(false);
                  }
               }
            }
         }
         break;

  		    case mouseUp:
  		    {
  		        if (mLastAlienWindowClicked) {
                nsCOMPtr<nsIEventSink> sink;
                GetWindowEventSink(mLastAlienWindowClicked, getter_AddRefs(sink));
                if ( sink ) {
                  PRBool handled = PR_FALSE;
  		            sink->DispatchEvent(inMacEvent, &handled);
  		            mLastAlienWindowClicked = nil;
  		            SetExecuteHost(false);
  		          }
  		        }
  		    }
  			break;
  			  	
  		    case updateEvt:
  		    case activateEvt:
  		    {
  		      macWindowP = (WindowPtr)inMacEvent->message;
  			    if (IsAlienGeckoWindow(macWindowP)) {
                nsCOMPtr<nsIEventSink> sink;
                GetWindowEventSink(macWindowP, getter_AddRefs(sink));
                if ( sink ) {
                  PRBool handled = PR_FALSE;
  		            sink->DispatchEvent(inMacEvent, &handled);
  		            SetExecuteHost(false);
  		          }
  			    }
  			}
  			break;  			
  	    }
	}
}


Boolean CEmbedEventAttachment::IsAlienGeckoWindow(WindowPtr inMacWindow)
{
    PRBool isAlien = PR_FALSE;
    
    
    
    if (inMacWindow && !LWindow::FetchWindowObject(inMacWindow)) {
      nsCOMPtr<nsIEventSink> sink;
      GetWindowEventSink ( inMacWindow, getter_AddRefs(sink) );
      if ( sink )
        isAlien = PR_TRUE;   
    }
    
    return isAlien;
}








CEmbedIdler::CEmbedIdler()
{
}


CEmbedIdler::~CEmbedIdler()
{
}
  
void CEmbedIdler::SpendTime(const EventRecord&	inMacEvent)
{
    ::PR_Sleep(PR_INTERVAL_NO_WAIT);
}





void InitializeEmbedEventHandling(LApplication* theApplication)
{
    CEmbedEventAttachment *windowAttachment = new CEmbedEventAttachment;
    ThrowIfNil_(windowAttachment);
    theApplication->AddAttachment(windowAttachment);

    CEmbedIdler *embedIdler = new CEmbedIdler;
    ThrowIfNil_(embedIdler);
    embedIdler->StartIdling();
    
#if TARGET_CARBON
    InitializeTextInputEventHandling();
#endif

}
   
