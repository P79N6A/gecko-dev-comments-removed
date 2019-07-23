





































 
 
#ifndef _NS_FILESELECTOR_H_
	#include "nsFileSelector.h"
#endif

#include "nsAppleSingleDecoder.h"

nsFileSelector::nsFileSelector()
{
	mFile = NULL;
}

nsFileSelector::~nsFileSelector()
{

}

pascal void
OurNavEventFunction(NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms,
					NavCallBackUserData callBackUD)
{
	WindowPtr  windowPtr;
                     
	windowPtr = (WindowPtr) callBackParms->eventData.eventDataParms.event->message;
	if (!windowPtr)
		return;
		
	switch(callBackSelector)
	{
		case kNavCBEvent:
			switch(callBackParms->eventData.eventDataParms.event->what)
			{
				case updateEvt:
					if(((WindowPeek) windowPtr)->windowKind != kDialogWindowKind)
						
						
					break;
			}
			break;
	} 
}

OSErr
nsFileSelector::SelectFile(FSSpecPtr aOutFile)
{
	OSErr	err = noErr;
	NavReplyRecord		reply;	
	NavDialogOptions	dlgOpts;
	NavEventUPP			eventProc;
	AEDesc				resultDesc, initDesc;
	FSSpec				tmp;
	short				cwdVRefNum;
	long				cwdDirID, len;
	
	mFile = aOutFile;
	
	err = NavGetDefaultDialogOptions(&dlgOpts);
	len = strlen("Please select a file");
	nsAppleSingleDecoder::PLstrncpy(dlgOpts.message, "\pPlease select a file", len);
	eventProc = NewNavEventProc( (ProcPtr) OurNavEventFunction );
	
	ERR_CHECK( GetCWD(&cwdDirID, &cwdVRefNum) );
	ERR_CHECK( FSMakeFSSpec(cwdVRefNum, cwdDirID, NULL, &tmp) );
	ERR_CHECK( AECreateDesc(typeFSS, (void*) &tmp, sizeof(FSSpec), &initDesc) );
	
	err = NavChooseFile( &initDesc, &reply, &dlgOpts, eventProc, NULL, NULL, NULL, NULL );	
		
	AEDisposeDesc(&initDesc);
	DisposeRoutineDescriptor(eventProc);
		
	if((reply.validRecord) && (err == noErr))
	{
		if((err = AECoerceDesc(&(reply.selection),typeFSS,&resultDesc)) == noErr)
		{
			BlockMoveData(*resultDesc.dataHandle,&tmp,sizeof(FSSpec));
			
			FSMakeFSSpec(tmp.vRefNum, tmp.parID, tmp.name, aOutFile); 
		}
            
		AEDisposeDesc(&resultDesc);
		NavDisposeReply(&reply);
	}
	
	return err;
}

OSErr
nsFileSelector::SelectFolder(FSSpecPtr aOutFolder)
{
	OSErr	err = noErr;
	NavReplyRecord		reply;	
	NavDialogOptions	dlgOpts;
	NavEventUPP			eventProc;
	AEDesc				resultDesc, initDesc;
	FSSpec				tmp;
	short				cwdVRefNum;
	long				cwdDirID, len;
	
	mFile = aOutFolder;
	
	err = NavGetDefaultDialogOptions(&dlgOpts);
	len = strlen("Please select a folder");
	nsAppleSingleDecoder::PLstrncpy(dlgOpts.message, "\pPlease select a folder", len);
	eventProc = NewNavEventProc( (ProcPtr) OurNavEventFunction );
	
	ERR_CHECK( GetCWD(&cwdDirID, &cwdVRefNum) );
	ERR_CHECK( FSMakeFSSpec(cwdVRefNum, cwdDirID, NULL, &tmp) );
	ERR_CHECK( AECreateDesc(typeFSS, (void*) &tmp, sizeof(FSSpec), &initDesc) );
	
	err = NavChooseFolder( &initDesc, &reply, &dlgOpts, eventProc, NULL, NULL );
		
	AEDisposeDesc(&initDesc);
	DisposeRoutineDescriptor(eventProc);
		
	if((reply.validRecord) && (err == noErr))
	{
		if((err = AECoerceDesc(&(reply.selection),typeFSS,&resultDesc)) == noErr)
		{
			BlockMoveData(*resultDesc.dataHandle,&tmp,sizeof(FSSpec));
			
			FSMakeFSSpec(tmp.vRefNum, tmp.parID, tmp.name, aOutFolder); 
		}
            
		AEDisposeDesc(&resultDesc);
		NavDisposeReply(&reply);
	}
	
	return err;
}

OSErr
nsFileSelector::GetCWD(long *aOutDirID, short *aOutVRefNum)
{
	OSErr 				err = noErr;
	ProcessSerialNumber	psn;
	ProcessInfoRec		pInfo;
	FSSpec				tmp;
	
	
	if (!(err = GetCurrentProcess(&psn))) 
	{
		pInfo.processName = nil;
		pInfo.processAppSpec = &tmp;
		pInfo.processInfoLength = (sizeof(ProcessInfoRec));
		
		if(!(err = GetProcessInformation(&psn, &pInfo)))
		{
			*aOutDirID = pInfo.processAppSpec->parID;
			*aOutVRefNum = pInfo.processAppSpec->vRefNum;
		}
	}
	
	return err;
}
