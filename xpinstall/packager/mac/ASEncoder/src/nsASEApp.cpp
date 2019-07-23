





































 
 
#ifndef _NS_ASEAPP_H_
	#include "nsASEApp.h"
#endif

#include <AppleEvents.h>
#include <Balloons.h>
#include <MacTypes.h>

#include "nsEventHandler.h"
#include "nsAppleSingleEncoder.h"
#include "nsAppleSingleDecoder.h"
#include "MoreFilesExtras.h"

Boolean gDone;

nsASEApp::nsASEApp()
{
	InitManagers();
	InitAEHandlers();
	mWindow = NULL;
	SetCompletionStatus(false);
	
	OSErr err = NavLoad();
	if (err!= noErr)
		FatalError(navLoadErr);
		
	MakeMenus();
}

nsASEApp::~nsASEApp()
{
	NavUnload();
}

void
nsASEApp::InitManagers(void)
{
	MaxApplZone();
	MoreMasters(); MoreMasters(); MoreMasters();
	
	InitGraf(&qd.thePort);
	InitFonts();			
	InitWindows();
	InitMenus();
	TEInit();		
	InitDialogs(NULL);
	
	InitCursor();
	FlushEvents(everyEvent, 0);	
}

#pragma mark -
#pragma mark *** Apple Event Handlers ***
#pragma mark -

pascal OSErr
EncodeEvent(AppleEvent *appEvent, AppleEvent *reply, SInt32 handlerRefCon)
{
	OSErr	err = noErr;
	FSSpec	param;
	Boolean	result = false, isDir = false;
	AEDesc	fileDesc;
	long	dummy;
	
	
	err = AEGetParamDesc(appEvent, keyDirectObject, typeFSS, &fileDesc);
	if (err != noErr)
		goto reply;
	BlockMoveData(*fileDesc.dataHandle, &param, sizeof(FSSpec));
	
	
	err = nsASEApp::GotRequiredParams(appEvent);
	if (err != noErr)
		goto reply;
		
	FSpGetDirectoryID(&param, &dummy, &isDir);
	
	
	if (isDir)
	{
		nsAppleSingleEncoder encoder;
		err = encoder.EncodeFolder(&param);
	}
	else
	{
		
		
		
		if (nsAppleSingleEncoder::HasResourceFork(&param))
		{
			
			nsAppleSingleEncoder encoder;
			err = encoder.Encode(&param);
		}
	}
	
	
	if (err == noErr)
	{
		
		result = true;
	}
	
reply:
	
	AEPutParamPtr(reply, keyDirectObject, typeBoolean, &result, sizeof(result));
	
	
	return noErr;
}

pascal OSErr
DecodeEvent(AppleEvent *appEvent, AppleEvent *reply, SInt32 handlerRefCon)
{
	OSErr	err = noErr;
	FSSpec	param, outFile;
	Boolean	result = false, isDir = false;
	AEDesc	fileDesc;
	long	dummy;
	
	
	err = AEGetParamDesc(appEvent, keyDirectObject, typeFSS, &fileDesc);
	if (err != noErr)
		goto reply;
	BlockMoveData(*fileDesc.dataHandle, &param, sizeof(FSSpec));
	
	
	err = nsASEApp::GotRequiredParams(appEvent);
	if (err != noErr)
		goto reply;
			
	FSpGetDirectoryID(&param, &dummy, &isDir);
	
	
	if (isDir)
	{
		nsAppleSingleDecoder decoder;
		err = decoder.DecodeFolder(&param);
	}
	else
	{	
		
		
		
		if (nsAppleSingleDecoder::IsAppleSingleFile(&param))
		{
			
			nsAppleSingleDecoder decoder;
			err = decoder.Decode(&param, &outFile);
		}
	}
	
	
	if (err == noErr)
	{
		
		result = true;
	}
	
reply:
	
	AEPutParamPtr(reply, keyDirectObject, typeBoolean, &result, sizeof(result));
	
	
	return noErr;
}

pascal OSErr
QuitEvent(AppleEvent *appEvent, AppleEvent *reply, SInt32 handlerRefCon)
{
	OSErr	err = noErr;
	
	nsASEApp::SetCompletionStatus(true);
	
	return err;
}

#pragma mark -

void
nsASEApp::InitAEHandlers()
{
	OSErr 					err = noErr;
	
	mEncodeUPP = NewAEEventHandlerProc((ProcPtr) EncodeEvent);
	err = AEInstallEventHandler(kASEncoderEventClass, kAEEncode,
								mEncodeUPP, 0L, false);
	if (err != noErr)
		::CautionAlert(aeInitErr, nil);
	
	mDecodeUPP = NewAEEventHandlerProc((ProcPtr) DecodeEvent);
	err = AEInstallEventHandler(kASEncoderEventClass, kAEDecode,
								mDecodeUPP, 0L, false);
	if (err != noErr)
		::CautionAlert(aeInitErr, nil);
		
	mQuitUPP = NewAEEventHandlerProc((ProcPtr) QuitEvent);
	err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
								mQuitUPP, 0L, false);
	if (err != noErr)
		::CautionAlert(aeInitErr, nil);
}
OSErr
nsASEApp::GotRequiredParams(AppleEvent *appEvent)
{
	OSErr		err = noErr;
	DescType	returnedType;
	Size		actualSize;

	err = AEGetAttributePtr(appEvent, keyMissedKeywordAttr, typeWildCard,
							&returnedType, NULL, 0, &actualSize);

	if (err == errAEDescNotFound)
		err = noErr;
	else if (err == noErr)
		err = errAEParamMissed;
		
	return err;
}

void
nsASEApp::MakeMenus()
{
    Handle 		mbarHdl;
	MenuHandle	menuHdl;

	mbarHdl = ::GetNewMBar(rMenuBar);
	::SetMenuBar(mbarHdl);
	
	if ((menuHdl = ::GetMenuHandle(rMenuApple))!=nil) 
	{
		::AppendResMenu(menuHdl, 'DRVR');
	}
		
	if ((menuHdl = GetMenuHandle(rMenuEdit))!=nil)
		::DisableItem(menuHdl, 0);
	
	::HMGetHelpMenuHandle(&menuHdl);
	::DisableItem(menuHdl, 0);

	::DrawMenuBar();
}

void
nsASEApp::SetCompletionStatus(Boolean aVal)
{
	gDone = aVal;
}

Boolean 
nsASEApp::GetCompletionStatus()
{
	return gDone;
}



void
nsASEApp::FatalError(short aErrID)
{
	::StopAlert(aErrID, nil);
	SetCompletionStatus(true);
}

OSErr
nsASEApp::Run()
{
	OSErr		err = noErr;
	EventRecord evt;
	nsEventHandler handler;
	
	while (!gDone)
	{
		if (::WaitNextEvent(everyEvent, &evt, 180, NULL))
		{
			handler.HandleNextEvent(&evt);
		}
	}
		
	return err;
}

int
main(void)
{
	nsASEApp app;
	
	app.Run();
	
	return 0;
}

