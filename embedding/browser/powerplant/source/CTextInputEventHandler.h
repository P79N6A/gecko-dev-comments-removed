





































 
#ifndef CTextInputEventHandler_h__
#define CTextInputEventHandler_h__

#include <CarbonEvents.h>
#include <TextServices.h>

#include "nsIMacTextInputEventSink.h"

#include "CBrowserShell.h"

class CTextInputEventHandler
{
public:
  CTextInputEventHandler() {};
  virtual ~CTextInputEventHandler() {};
  
  virtual OSStatus HandleAll( EventHandlerCallRef inHandlerCallRef, 
                              EventRef inEvent);
protected:  
  virtual OSStatus HandleUnicodeForKeyEvent( CBrowserShell* aBrowserShell, 
                                             EventHandlerCallRef inHandlerCallRef, 
                                             EventRef inEvent);
  virtual OSStatus HandleUpdateActiveInputArea( CBrowserShell* sink, 
                                                EventHandlerCallRef inHandlerCallRef, 
                                                EventRef inEvent);
  virtual OSStatus HandleOffsetToPos( CBrowserShell* aBrowserShell, 
                                      EventHandlerCallRef inHandlerCallRef, 
                                      EventRef inEvent);
  virtual OSStatus HandlePosToOffset( CBrowserShell* aBrowserShell, 
                                      EventHandlerCallRef inHandlerCallRef, 
                                      EventRef inEvent);
  virtual OSStatus HandleGetSelectedText( CBrowserShell* aBrowserShell, 
                                      EventHandlerCallRef inHandlerCallRef, 
                                      EventRef inEvent);
  
  virtual CBrowserShell* GetGeckoTarget();
 
private:
  OSStatus GetText(EventRef inEvent, nsString& outString);
  OSStatus GetScriptLang(EventRef inEvent, ScriptLanguageRecord& outSlr);
};

void InitializeTextInputEventHandling();

#endif 

