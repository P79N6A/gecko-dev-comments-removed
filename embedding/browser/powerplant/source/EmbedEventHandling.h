






































#ifndef __EmbedEventHandling__
#define __EmbedEventHandling__

#include "LAttachment.h"
#include "LPeriodical.h"

class nsIWidget;
class nsIEventSink;









class	CEmbedEventAttachment : public LAttachment
{
public:

	enum { class_ID = FOUR_CHAR_CODE('GAWA') };
				
						        CEmbedEventAttachment();	
	virtual				        ~CEmbedEventAttachment();
	
	
	virtual	void                ExecuteSelf(MessageT		inMessage,
                                            void*			ioParam);
										              
protected:
    Boolean                     IsAlienGeckoWindow(WindowPtr inMacWindow);
  
    
    
    static void GetWindowEventSink ( WindowPtr aWindow, nsIEventSink** outSink ) ;
    static void GetTopWidget ( WindowPtr aWindow, nsIWidget** outWidget ) ;

    static WindowPtr            mLastAlienWindowClicked;
};









class CEmbedIdler : public LPeriodical
{
public:
                                CEmbedIdler();
    virtual                     ~CEmbedIdler();
  
  
	virtual	void		        SpendTime(const EventRecord& inMacEvent);
};









class CEmbedRepeater : public LPeriodical
{
public:
                                CEmbedRepeater();
    virtual                     ~CEmbedRepeater();
  
  
	virtual	void		        SpendTime(const EventRecord& inMacEvent);
};






void InitializeEmbedEventHandling(LApplication* theApplication);   

#endif
