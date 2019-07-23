





































 
 
#ifndef _NS_EVENTHANDLER_H_
	#include "nsEventHandler.h"
#endif

#include <LowMem.h>
#include <ToolUtils.h>


#include "nsASEApp.h"
#include "nsFileSelector.h"
#include "nsAppleSingleEncoder.h"
#include "nsAppleSingleDecoder.h"

nsEventHandler::nsEventHandler()
{

}

nsEventHandler::~nsEventHandler()
{

}

OSErr 
nsEventHandler::HandleNextEvent(EventRecord *aEvt) 
{ 
	OSErr err = noErr;
		
	mCurrEvent = aEvt;
	
	switch(aEvt->what)
	{
		case kHighLevelEvent:
			err = AEProcessAppleEvent(aEvt);
			if (err != errAEEventNotHandled)
				break;
				
		case mouseDown:
			err = HandleMouseDown();
			break;
			
		case mouseUp:
			break;

		case autoKey:
		case keyDown:
			err = HandleKeyDown();
			break;
			
		case updateEvt:
			err = HandleUpdateEvt();
			break;
			
		case activateEvt:
			err = HandleActivateEvt();
			break;
			
		case osEvt:
			err = HandleOSEvt();
			break;
		
		case nullEvent:
			break;
			
		default:
			break;
	}
	
	return err;
}

OSErr 
nsEventHandler::HandleMouseDown() 
{ 
	OSErr				err = noErr;
	WindowPtr			currWindow;
	SInt16				cntlPartCode;
	SInt32				menuChoice;
	
	cntlPartCode = FindWindow(mCurrEvent->where, &currWindow);
	switch (cntlPartCode)
	{
		case inMenuBar:
			menuChoice = MenuSelect(mCurrEvent->where);
			HandleMenuChoice(menuChoice);
			::InvalMenuBar();
			break;
			
		default:
			break;
	}
	
	return err;
}

OSErr 
nsEventHandler::HandleKeyDown() 
{ 
	OSErr	err = noErr;
	SInt8	charCode;
	SInt32	menuChoice = 0;
	
	charCode = mCurrEvent->message & charCodeMask;
	if ((mCurrEvent->modifiers & cmdKey) != 0)
	{
		switch (charCode)
		{
			case 'E':
			case 'e':
				menuChoice = rMenuFile;
				menuChoice <<= 16;
				menuChoice |= rMenuItemASEncode;
				break;
				
			case 'D':
			case 'd':
				menuChoice = rMenuFile;
				menuChoice <<= 16;
				menuChoice |= rMenuItemASDecode;
				break;
				
			case 'F':
			case 'f':
				menuChoice = rMenuFile;
				menuChoice <<= 16;
				menuChoice |= rMenuItemASEncodeFolder;
				break;
				
			case 'O':
			case 'o':
				menuChoice = rMenuFile;
				menuChoice <<= 16;
				menuChoice |= rMenuItemASDecodeFolder;
				break;
				
			case 'Q':
			case 'q':
				menuChoice = rMenuFile;
				menuChoice <<= 16;
				menuChoice |= rMenuItemQuit;
				break;
				
			default:
				menuChoice = 0;
				break;
		}	
		err = HandleMenuChoice(menuChoice);
	}
	
	return err;
}

OSErr 
nsEventHandler::HandleUpdateEvt() 
{ 
	return noErr;
}

OSErr 
nsEventHandler::HandleActivateEvt() 
{ 
	return noErr;
}

OSErr 
nsEventHandler::HandleOSEvt() 
{ 
	::HiliteMenu(0);
	
	return noErr;
}

OSErr 
nsEventHandler::HandleInContent() 
{ 
	return noErr;
}

OSErr
nsEventHandler::HandleMenuChoice(SInt32 aChoice)
{
	OSErr				err = noErr;
	long 				menuID = HiWord(aChoice);
	long 				menuItem = LoWord(aChoice);
	FSSpec				file, outFile;
	nsFileSelector 		selector;
	
	switch (menuID)
	{
		case rMenuFile:
			switch (menuItem)
			{
				case rMenuItemASEncode:			
					err = selector.SelectFile(&file);
					if (err == noErr)
					{
						if (nsAppleSingleEncoder::HasResourceFork(&file))
						{
							nsAppleSingleEncoder encoder;
							err = encoder.Encode(&file);
						}
					}
					break;
				
				case rMenuItemASDecode:			
					err = selector.SelectFile(&file);
					if (err == noErr)
					{
						if (nsAppleSingleDecoder::IsAppleSingleFile(&file))
						{
							nsAppleSingleDecoder decoder;
							err = decoder.Decode(&file, &outFile);
						}
					}
					break;
					
				case rMenuItemASEncodeFolder:	
					err = selector.SelectFolder(&file);
					if (err == noErr)
					{
						nsAppleSingleEncoder encoder;
						err = encoder.EncodeFolder(&file);
					}
					break;
					
				case rMenuItemASDecodeFolder:	
					err = selector.SelectFolder(&file);
					if (err == noErr)
					{
						nsAppleSingleDecoder decoder;
						err = decoder.DecodeFolder(&file);
					}
					break;
					
				case rMenuItemQuit:			 	
					nsASEApp::SetCompletionStatus(true);
					break;
			}
			break;
		
		case rMenuApple:
			if (menuItem == rMenuItemAbout)
				::Alert(rAboutBox, nil);
			break;
			
		default:
			break;
	}
	
	return err;
}
