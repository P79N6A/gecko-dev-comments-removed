











































 
 
#include "CTextInputEventHandler.h"
#include "nsCRT.h"
#include "nsAutoBuffer.h"

#pragma mark -

#pragma mark -





CBrowserShell* CTextInputEventHandler::GetGeckoTarget()
{
  return dynamic_cast<CBrowserShell*> (LCommander::GetTarget());  
}

#pragma mark -




OSStatus CTextInputEventHandler::GetScriptLang(EventRef inEvent, ScriptLanguageRecord& outSlr )
{
  OSStatus err = ::GetEventParameter(inEvent, kEventParamTextInputSendSLRec, typeIntlWritingCode, NULL, 
                          sizeof(outSlr), NULL, &outSlr);
  return err;
}




OSStatus CTextInputEventHandler::GetText(EventRef inEvent, nsString& outString)
{
  UInt32 neededSize;
  outString.Truncate(0);
  OSStatus err = ::GetEventParameter(inEvent, kEventParamTextInputSendText, typeUnicodeText, NULL, 0, &neededSize, NULL);
  if (noErr != err)
    return err;
    
  if (neededSize > 0) 
  {
    nsAutoBuffer<PRUnichar, 256> buf;
    if (! buf.EnsureElemCapacity(neededSize/sizeof(PRUnichar)))
      return eventParameterNotFoundErr;

    err = ::GetEventParameter(inEvent, kEventParamTextInputSendText, typeUnicodeText, NULL, 
                            neededSize, &neededSize, buf.get());
                            
    if (noErr == err) 
       outString.Assign(buf.get(), neededSize/sizeof(PRUnichar));
  }
  return err;
}

#pragma mark -





OSStatus CTextInputEventHandler::HandleUnicodeForKeyEvent(
  CBrowserShell* aBrowserShell, 
  EventHandlerCallRef inHandlerCallRef, 
  EventRef inEvent)
{
  NS_ENSURE_TRUE(aBrowserShell, eventNotHandledErr);
  EventRef keyboardEvent;
  OSStatus err = ::GetEventParameter(inEvent, kEventParamTextInputSendKeyboardEvent, typeEventRef, NULL, 
                          sizeof(keyboardEvent), NULL, &keyboardEvent);
  
  if (noErr != err)
    return eventParameterNotFoundErr;
  
  
  
  
  
  UInt32 keyModifiers;  
  err = ::GetEventParameter(keyboardEvent, kEventParamKeyModifiers, typeUInt32,
        NULL, sizeof(UInt32), NULL, &keyModifiers);
  if ((noErr == err) && (keyModifiers & (cmdKey | controlKey)))
    return eventNotHandledErr;
    
  EventRecord eventRecord;                        
                       
  if (! ::ConvertEventRefToEventRecord(keyboardEvent, &eventRecord))
    return eventParameterNotFoundErr;
  

  ScriptLanguageRecord slr;
  err = GetScriptLang(inEvent, slr);
  if (noErr != err)
    return eventParameterNotFoundErr;
  
  nsAutoString text;
  err = GetText(inEvent, text);
  if (noErr != err)
    return eventParameterNotFoundErr;
  
  
  err = aBrowserShell->HandleUnicodeForKeyEvent(text, slr.fScript, slr.fLanguage, &eventRecord);
  
  return err;
}





OSStatus CTextInputEventHandler::HandleUpdateActiveInputArea(
  CBrowserShell* aBrowserShell, 
  EventHandlerCallRef inHandlerCallRef, 
  EventRef inEvent)
{
  NS_ENSURE_TRUE(aBrowserShell, eventNotHandledErr);
  PRUint32 fixLength;
  OSStatus err = ::GetEventParameter(inEvent, kEventParamTextInputSendFixLen, typeLongInteger, NULL, 
                          sizeof(fixLength), NULL, &fixLength);
  if (noErr != err)
    return eventParameterNotFoundErr;

  ScriptLanguageRecord slr;
  err = GetScriptLang(inEvent, slr);
  if (noErr != err)
    return eventParameterNotFoundErr;
  
  nsAutoString text;
  err = GetText(inEvent, text);
   if (noErr != err)
    return eventParameterNotFoundErr;

  
  
  TextRangeArray* hiliteRng = nsnull;                        
  UInt32 rngSize=0;   
  err = ::GetEventParameter(inEvent, kEventParamTextInputSendHiliteRng, typeTextRangeArray, NULL, 
                          0, NULL, &rngSize);
  if (noErr == err)  
  {
    TextRangeArray* pt = (TextRangeArray*)::malloc(rngSize);
    NS_ASSERTION( (pt), "Cannot malloc for hiliteRng") ; 
    if (pt)
    { 
      hiliteRng = pt;
      err = ::GetEventParameter(inEvent, kEventParamTextInputSendHiliteRng, typeTextRangeArray, NULL, 
                              rngSize, &rngSize, hiliteRng);
      NS_ASSERTION( (noErr == err), "Cannot get hiliteRng") ; 
    }                          
  }                     
  
  
  err = aBrowserShell->HandleUpdateActiveInputArea(text, slr.fScript, slr.fLanguage,
                                          fixLength / sizeof(PRUnichar), hiliteRng);
  if (hiliteRng)
     ::free(hiliteRng);
  return err;
}



OSStatus CTextInputEventHandler::HandleGetSelectedText(
  CBrowserShell* aBrowserShell, 
  EventHandlerCallRef inHandlerCallRef, 
  EventRef inEvent)
{
  NS_ENSURE_TRUE(aBrowserShell, eventNotHandledErr);

  nsAutoString outString;
  OSStatus err = aBrowserShell->HandleGetSelectedText(outString);   
  if (noErr != err)
    return eventParameterNotFoundErr;
  
  err = ::SetEventParameter(inEvent, kEventParamTextInputReplyText, typeUnicodeText,
                          outString.Length()*sizeof(PRUnichar), outString.get());
  return err; 
}





OSStatus CTextInputEventHandler::HandleOffsetToPos(
  CBrowserShell* aBrowserShell, 
  EventHandlerCallRef inHandlerCallRef, 
  EventRef inEvent)
{
  NS_ENSURE_TRUE(aBrowserShell, eventNotHandledErr);
  PRUint32 offset;
  OSStatus err = ::GetEventParameter(inEvent, kEventParamTextInputSendTextOffset, typeLongInteger, NULL, 
                          sizeof(offset), NULL, &offset);
  if (noErr != err)
    return eventParameterNotFoundErr;

  Point thePoint;
  
  err = aBrowserShell->HandleOffsetToPos(offset, &thePoint.h, &thePoint.v);   
  
  if (noErr != err)
    return eventParameterNotFoundErr;
  
  err = ::SetEventParameter(inEvent, kEventParamTextInputReplyPoint, typeQDPoint,
                          sizeof(thePoint), &thePoint);
  return err; 
}





OSStatus CTextInputEventHandler::HandlePosToOffset(
  CBrowserShell* aBrowserShell, 
  EventHandlerCallRef inHandlerCallRef, 
  EventRef inEvent)
{
  NS_ENSURE_TRUE(aBrowserShell, eventNotHandledErr);
  PRInt32 offset;
  Point thePoint;
  short regionClass;

  OSStatus err = ::GetEventParameter(inEvent, kEventParamTextInputSendCurrentPoint, typeQDPoint, NULL, 
                                   sizeof(thePoint), NULL, &thePoint);
  if (noErr != err)
    return eventParameterNotFoundErr;

  
  err = aBrowserShell->HandlePosToOffset(thePoint.h, thePoint.v, &offset, &regionClass);
  
  if (noErr != err)
    return eventParameterNotFoundErr;

  err = ::SetEventParameter(inEvent, kEventParamTextInputReplyRegionClass, typeShortInteger,
                          sizeof(regionClass), &regionClass);
  if (noErr != err)
    return eventParameterNotFoundErr;
  
  err = ::SetEventParameter(inEvent, kEventParamTextInputReplyTextOffset, typeLongInteger,
                          sizeof(offset), &offset);
  return err;   
}





OSStatus CTextInputEventHandler::HandleAll(EventHandlerCallRef inHandlerCallRef, EventRef inEvent)
{
  CBrowserShell* aBrowserShell = GetGeckoTarget();
  
  
  if (!aBrowserShell)
    return eventNotHandledErr;
  
  UInt32 eventClass = ::GetEventClass(inEvent);
  if (eventClass != kEventClassTextInput)
    return eventNotHandledErr;

  UInt32 eventKind = ::GetEventKind(inEvent);
  if ((kEventTextInputUpdateActiveInputArea != eventKind) &&
                  (kEventTextInputUnicodeForKeyEvent!= eventKind) &&
                  (kEventTextInputOffsetToPos != eventKind) &&
                  (kEventTextInputPosToOffset != eventKind) &&
                  (kEventTextInputGetSelectedText != eventKind))
    return eventNotHandledErr;
    
  switch(eventKind)
  {
    case kEventTextInputUpdateActiveInputArea:
      return HandleUpdateActiveInputArea(aBrowserShell, inHandlerCallRef, inEvent);
    case kEventTextInputUnicodeForKeyEvent:
      return HandleUnicodeForKeyEvent(aBrowserShell, inHandlerCallRef, inEvent);
    case kEventTextInputOffsetToPos:
      return HandleOffsetToPos(aBrowserShell, inHandlerCallRef, inEvent);
    case kEventTextInputPosToOffset:
      return HandlePosToOffset(aBrowserShell, inHandlerCallRef, inEvent);
    case kEventTextInputGetSelectedText:
      return HandleGetSelectedText(aBrowserShell, inHandlerCallRef, inEvent);
  }
  return eventNotHandledErr;
}


#pragma mark -




static pascal OSStatus TextInputHandler(EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void *inUserData)
{
   CTextInputEventHandler* realHandler = (CTextInputEventHandler*)inUserData;
   return realHandler->HandleAll(inHandlerCallRef, inEvent);
}




void InitializeTextInputEventHandling()
{
  static CTextInputEventHandler Singleton;
  EventTypeSpec eventTypes[5] = {
    {kEventClassTextInput, kEventTextInputUpdateActiveInputArea },
    {kEventClassTextInput, kEventTextInputUnicodeForKeyEvent },
    {kEventClassTextInput, kEventTextInputOffsetToPos },
    {kEventClassTextInput, kEventTextInputPosToOffset },
    {kEventClassTextInput, kEventTextInputGetSelectedText }
  };  
  
  EventHandlerUPP textInputUPP = NewEventHandlerUPP(TextInputHandler); 
  OSStatus err = InstallApplicationEventHandler( textInputUPP, 5, eventTypes, &Singleton, NULL);
  NS_ASSERTION(err==noErr, "Cannot install carbon event");
}
