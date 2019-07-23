






































#include "MacInstallWizard.h"


#include <Math64.h>






static long sDSNeededK = 0;  
static long sDSAvailK = 0;   

void 
ShowSetupTypeWin(void)
{
	Str255 				next;
	Str255 				back;
	MenuHandle 			popupMenu;
	PopupPrivateData ** pvtDataHdl;
	unsigned char *		currMenuItem;
	short 				i;




	Rect 				viewRect;
	long				txtSize;
	Str255				instLocTitle, selectFolder;
	GrafPtr				oldPort;
	
	GetPort(&oldPort);
	
	if (gWPtr != NULL)
	{
		SetPort(gWPtr);
	
		gCurrWin = kSetupTypeID; 
		

		GetResourcedString(next, rInstList, sNextBtn);
		GetResourcedString(back, rInstList, sBackBtn);
	
		
		gControls->stw->instType = GetNewControl( rInstType, gWPtr);
		gControls->stw->instDescBox = GetNewControl( rInstDescBox, gWPtr);
		gControls->stw->destLocBox = GetNewControl( rDestLocBox, gWPtr);
		gControls->stw->destLoc = GetNewControl(rDestLoc, gWPtr);
		if (!gControls->stw->instType || !gControls->stw->instDescBox || 
			!gControls->stw->destLocBox || !gControls->stw->destLoc)
		{
			ErrorHandler(eMem, nil);
			return;
		}

		
		HLock((Handle)gControls->stw->instType);
		pvtDataHdl = (PopupPrivateData **) (*(gControls->stw->instType))->contrlData;
		HLock((Handle)pvtDataHdl);
		popupMenu = (MenuHandle) (**pvtDataHdl).mHandle;
		for (i=0; i<gControls->cfg->numSetupTypes; i++)
		{
			HLock(gControls->cfg->st[i].shortDesc);
			currMenuItem = CToPascal(*gControls->cfg->st[i].shortDesc);		
			HUnlock(gControls->cfg->st[i].shortDesc);
			InsertMenuItem( popupMenu, currMenuItem, i);
		}
		HUnlock((Handle)pvtDataHdl);
		HUnlock((Handle)gControls->stw->instType);
		SetControlMaximum(gControls->stw->instType, gControls->cfg->numSetupTypes);
		SetControlValue(gControls->stw->instType, gControls->opt->instChoice);
	
		
		HLock((Handle)gControls->stw->instDescBox);
		SetRect(&viewRect,  (*(gControls->stw->instDescBox))->contrlRect.left,
							(*(gControls->stw->instDescBox))->contrlRect.top,
							(*(gControls->stw->instDescBox))->contrlRect.right,
							(*(gControls->stw->instDescBox))->contrlRect.bottom);
		HUnlock((Handle)gControls->stw->instDescBox);	
		InsetRect(&viewRect, kTxtRectPad, kTxtRectPad);

		TextFont(systemFont);
		TextFace(normal);
		TextSize(12);	
		gControls->stw->instDescTxt = TENew( &viewRect, &viewRect);
		HLock(gControls->cfg->st[gControls->opt->instChoice - 1].longDesc);
		txtSize = strlen(*gControls->cfg->st[gControls->opt->instChoice - 1].longDesc);
		TEInsert( *gControls->cfg->st[gControls->opt->instChoice - 1].longDesc, txtSize, gControls->stw->instDescTxt);
		TESetAlignment( teFlushDefault, gControls->stw->instDescTxt);
		HUnlock(gControls->cfg->st[gControls->opt->instChoice - 1].longDesc);




















		GetResourcedString(selectFolder, rInstList, sSelectFolder);
		SetControlTitle(gControls->stw->destLoc, selectFolder);
		GetResourcedString(instLocTitle, rInstList, sInstLocTitle);
		SetControlTitle(gControls->stw->destLocBox, instLocTitle);	
	
		
		ShowControl(gControls->stw->instType);
		ShowControl(gControls->stw->destLoc);
		ShowNavButtons( back, next );
	
		DrawDiskNFolder(gControls->opt->vRefNum, gControls->opt->folder);
	}
		
	SetPort(oldPort);
}

void
ShowSetupDescTxt(void)
{
	Rect teRect; 
	
	if (gControls->stw->instDescTxt)
	{
		teRect = (**(gControls->stw->instDescTxt)).viewRect;
		TEUpdate(&teRect, gControls->stw->instDescTxt);
	}
	
	DrawDiskNFolder(gControls->opt->vRefNum, gControls->opt->folder);
}

pascal void
OurNavEventFunction(NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms,
					NavCallBackUserData callBackUD)
{
	WindowPtr  windowPtr;
  
  if (!callBackParms || !callBackParms->eventData.eventDataParms.event)
  	return;

	windowPtr = (WindowPtr)callBackParms->eventData.eventDataParms.event->message;
	if (!windowPtr)
		return;
		
	switch(callBackSelector)
	{
		case kNavCBEvent:
			switch(callBackParms->eventData.eventDataParms.event->what)
			{
				case updateEvt:
					if(((WindowPeek) windowPtr)->windowKind != kDialogWindowKind)
						HandleUpdateEvt((EventRecord *) callBackParms->eventData.eventDataParms.event);
					break;
			}
			break;
	} 
}

static Boolean bFirstFolderSelection = true;

void 
InSetupTypeContent(EventRecord* evt, WindowPtr wCurrPtr)
{	
	Point 				localPt;
	Rect				r;
	ControlPartCode		part;	
	short				cntlVal;
	long 				len;
	ControlHandle 		currCntl;
	int					instChoice;
	
	
	NavReplyRecord		reply;	
	NavDialogOptions	dlgOpts;
	NavEventUPP			eventProc;
	AEDesc				resultDesc, initDesc;
	FSSpec				folderSpec, tmp;
	OSErr				err;
	long				realDirID;

	GrafPtr				oldPort;
	GetPort(&oldPort);
	SetPort(wCurrPtr);
	
	localPt = evt->where;
	GlobalToLocal( &localPt);
	
	HLock((Handle)gControls->stw->instType);
	SetRect(&r, (**(gControls->stw->instType)).contrlRect.left,
				(**(gControls->stw->instType)).contrlRect.top,
				(**(gControls->stw->instType)).contrlRect.right,
				(**(gControls->stw->instType)).contrlRect.bottom);
	HUnlock((Handle)gControls->stw->instType);
	if (PtInRect(localPt, &r))
	{
		part = FindControl(localPt, gWPtr, &currCntl);
		part = TrackControl(currCntl, localPt, (ControlActionUPP) -1);
		
		gControls->opt->instChoice = GetControlValue(currCntl);
		instChoice = gControls->opt->instChoice - 1;
		
		SetRect(&r, (**(gControls->stw->instDescTxt)).viewRect.left,
					(**(gControls->stw->instDescTxt)).viewRect.top,
					(**(gControls->stw->instDescTxt)).viewRect.right,
					(**(gControls->stw->instDescTxt)).viewRect.bottom);
					
		HLock(gControls->cfg->st[instChoice].longDesc);
		len = strlen(*gControls->cfg->st[instChoice].longDesc);
		TESetText( *gControls->cfg->st[instChoice].longDesc, len, gControls->stw->instDescTxt);
		HUnlock(gControls->cfg->st[instChoice].longDesc);

		EraseRect( &r );
		TEUpdate( &r, gControls->stw->instDescTxt);
		
		ClearDiskSpaceMsgs();
		DrawDiskSpaceMsgs(gControls->opt->vRefNum);
		return;
	}
	
	HLockHi((Handle)gControls->stw->destLoc);
	SetRect(&r, (**(gControls->stw->destLoc)).contrlRect.left,
				(**(gControls->stw->destLoc)).contrlRect.top,
				(**(gControls->stw->destLoc)).contrlRect.right,
				(**(gControls->stw->destLoc)).contrlRect.bottom);
	HUnlock((Handle)gControls->stw->destLoc);
	if (PtInRect(localPt, &r))
	{
		part = FindControl(localPt, gWPtr, &currCntl);
		part = TrackControl(currCntl, localPt, (ControlActionUPP) -1);
		cntlVal = GetControlValue(currCntl);
		
		err = NavGetDefaultDialogOptions(&dlgOpts);
		GetResourcedString( dlgOpts.message, rInstList, sFolderDlgMsg );
		eventProc = NewNavEventUPP( OurNavEventFunction );
		
		if (!bFirstFolderSelection)
			GetParentID(gControls->opt->vRefNum, gControls->opt->dirID, "\p", &realDirID);
		else
		{
			realDirID = gControls->opt->dirID;
			bFirstFolderSelection = false;
		}
		FSMakeFSSpec(gControls->opt->vRefNum, realDirID, "\p", &tmp);
		ERR_CHECK(AECreateDesc(typeFSS, (void*) &tmp, sizeof(FSSpec), &initDesc));
		err = NavChooseFolder( &initDesc, &reply, &dlgOpts, eventProc, NULL, NULL );
		
		AEDisposeDesc(&initDesc);
		DisposeRoutineDescriptor(eventProc);
		
		if((reply.validRecord) && (err == noErr))
		{
			if((err = AECoerceDesc(&(reply.selection),typeFSS,&resultDesc)) == noErr)
			{
				BlockMoveData(*resultDesc.dataHandle,&tmp,sizeof(FSSpec));
				
				FSMakeFSSpec(tmp.vRefNum, tmp.parID, tmp.name, &folderSpec); 
				
				







				pstrcpy(gControls->opt->folder, folderSpec.name);
				gControls->opt->vRefNum = tmp.vRefNum;
				gControls->opt->dirID = tmp.parID;
				DrawDiskNFolder(folderSpec.vRefNum, folderSpec.name);
			}
            
			AEDisposeDesc(&resultDesc);
			NavDisposeReply(&reply);
		}
		
		return;
	}
			
	HLock((Handle)gControls->nextB);			
	r = (**(gControls->nextB)).contrlRect;
	HUnlock((Handle)gControls->nextB);
	if (PtInRect( localPt, &r))
	{
		part = TrackControl(gControls->nextB, evt->where, NULL);
		if (part)
		{
			
			if (gControls->cfg->numLegacyChecks > 0)
				if (LegacyFileCheck(gControls->opt->vRefNum, gControls->opt->dirID))
				{
					
					return;
				}
			
			
			if (gControls->opt->instChoice < gControls->cfg->numSetupTypes)
			{
    			if (!VerifyDiskSpace())
    			    return;
            }
            			    			
			ClearDiskSpaceMsgs();
			KillControls(gWPtr);
			
			if (gControls->opt->instChoice == gControls->cfg->numSetupTypes)
				ShowComponentsWin();
			else
				ShowTerminalWin();
			return;
		}
	}
	SetPort(oldPort);
}

void
InsertCompList(int instChoice)
{
	int compsDone, i, len;
	InstComp currComp;
	char compName[128];
	Boolean didOneComp = false;
	
	
	if (gControls->opt->instChoice < gControls->cfg->numSetupTypes)
	{
		compsDone = 0;
		TEInsert("\r", 1, gControls->stw->instDescTxt);
		for(i=0; i<kMaxComponents; i++)
		{
			if ( (gControls->cfg->st[instChoice].comp[i] == kInSetupType) &&
				 (!gControls->cfg->comp[i].invisible) &&
				 (compsDone < gControls->cfg->st[instChoice].numComps) )
			{
				currComp = gControls->cfg->comp[i];
				HLock(currComp.shortDesc);
				len = strlen(*currComp.shortDesc) + 4;
				memset(compName, 0, 128);
				if (didOneComp)
				    sprintf(compName, ", %s", *currComp.shortDesc);
				else
				{
				    sprintf(compName, "%s", *currComp.shortDesc);
				    didOneComp = true;
				}
				TEInsert(compName, len, gControls->stw->instDescTxt);
				HUnlock(currComp.shortDesc);
			}
			compsDone++;
		}
	}
}

void
DrawDiskNFolder(short vRefNum, unsigned char *folder)
{
	Str255			inFolderMsg, onDiskMsg, volName;
	char			*cstr;
	Rect			viewRect, dlb;
	TEHandle		pathInfo;
	short			bCmp, outVRefNum;
	OSErr			err = noErr;
	FSSpec			fsTarget;
	IconRef			icon;
	SInt16			label;
	unsigned long   free, total;
	
#define ICON_DIM 32

    dlb = (*gControls->stw->destLocBox)->contrlRect;
    SetRect(&viewRect, dlb.left+10, dlb.top+15, dlb.left+10+ICON_DIM, dlb.top+15+ICON_DIM);
   
	
	FSMakeFSSpec(gControls->opt->vRefNum, gControls->opt->dirID, "\p", &fsTarget);
	err = GetIconRefFromFile(&fsTarget, &icon, &label);
	if (err==noErr)
	{
		EraseRect(&viewRect);
		PlotIconRef(&viewRect, kAlignNone, kTransformNone, kIconServicesNormalUsageFlag, icon);
	}
	ReleaseIconRef(icon);	
	
	
    SetRect(&viewRect, dlb.left+10+ICON_DIM+12, dlb.top+15, dlb.left+220, dlb.bottom-5);
    
	
	if ((err = HGetVInfo(vRefNum, volName, &outVRefNum, &free, &total)) == noErr)
	{	
		
		TextFace(normal);
		TextSize(9);
		TextFont(applFont);
		EraseRect(&viewRect);
		pathInfo = TENew(&viewRect, &viewRect);
	
		if ( (bCmp = pstrcmp(folder, volName)) == 0)
		{
			GetResourcedString( inFolderMsg, rInstList, sInFolder);
			cstr = PascalToC(inFolderMsg);
			TEInsert(cstr, strlen(cstr), pathInfo); 
			DisposePtr(cstr);	
				cstr = "\r\"\0"; 	TEInsert(cstr, strlen(cstr), pathInfo);
	
			cstr = PascalToC(folder);
			TEInsert(cstr, strlen(cstr), pathInfo);
			DisposePtr(cstr);
				cstr = "\"\r\0"; 	TEInsert(cstr, strlen(cstr), pathInfo);
		}
		
		GetResourcedString( onDiskMsg,   rInstList, sOnDisk);
		cstr = PascalToC(onDiskMsg);
		TEInsert(cstr, strlen(cstr), pathInfo);
		DisposePtr(cstr);
			cstr = "\r\"\0"; 	TEInsert(cstr, strlen(cstr), pathInfo);
			
		cstr = PascalToC(volName);
		TEInsert(cstr, strlen(cstr), pathInfo);
		DisposePtr(cstr);
			cstr = "\"\0"; 	TEInsert(cstr, strlen(cstr), pathInfo);
			
		TEUpdate(&viewRect, pathInfo);
		
		TextFont(systemFont);
		TextSize(12);
	}
	
	
	TEDispose(pathInfo);

	DrawDiskSpaceMsgs(vRefNum);
}

void
DrawDiskSpaceMsgs(short vRefNum)
{
	XVolumeParam	pb;
	OSErr			err, reserr;
	short			msglen = 0;
	TEHandle		dsAvailH, dsNeededH;
	Rect			instDescBox, viewRect;
	Handle			instDescRectH;
	Str255			msg;
	Str15			kb;
	char 			*cstr, *cmsg, *ckb, *cfreeSpace, *cSpaceNeeded;
	
	pb.ioCompletion = NULL;
	pb.ioVolIndex = 0;
	pb.ioNamePtr = NULL;
	pb.ioVRefNum = vRefNum;
	
	ERR_CHECK( PBXGetVolInfoSync(&pb) );
	sDSAvailK = U32SetU(U64Divide(pb.ioVFreeBytes, U64SetU(1024L), nil));
    
	instDescRectH = NULL;
	instDescRectH = Get1Resource('RECT', rCompListBox);
	reserr = ResError();
	if (reserr!=noErr || !instDescRectH)
	{
		ErrorHandler(reserr, nil);
		return;
	}
	
	HLock(instDescRectH);
	instDescBox = (Rect) **((Rect**)instDescRectH);
	SetRect( &viewRect, instDescBox.left, instDescBox.bottom + 2, 
						instDescBox.right, instDescBox.bottom + 14 );
	HUnlock(instDescRectH);	
	DetachResource(instDescRectH);
	DisposeHandle(instDescRectH); 
							
	TextFace(normal);
	TextSize(9);
	TextFont(applFont);
	EraseRect(&viewRect);	
	dsAvailH = NULL;
	dsAvailH = TENew(&viewRect, &viewRect);
	if (!dsAvailH)
	{
		ErrorHandler(eMem, nil);
		return;
	}
	
	
	GetResourcedString( msg, rInstList, sDiskSpcAvail );
	cstr = PascalToC(msg);
	msglen = strlen(cstr);
	cmsg = (char*)malloc(msglen+255);
	strncpy(cmsg, cstr, msglen);
	cmsg[msglen] = '\0';
	
	
	cfreeSpace = ltoa(sDSAvailK);
	msglen += strlen(cfreeSpace);
	strcat( cmsg, cfreeSpace );
	cmsg[msglen] = '\0';
	
	
	GetResourcedString( kb, rInstList, sKilobytes );
	ckb = PascalToC(kb);
	msglen += strlen(ckb);
	strcat( cmsg, ckb );
	cmsg[msglen] = '\0';
	
	
	TEInsert( cmsg, strlen(cmsg), dsAvailH );
	TEUpdate( &viewRect, dsAvailH );
	
	
	if (cstr)
		DisposePtr((Ptr)cstr);
	if (cmsg)
		free(cmsg);
	if (ckb)
		DisposePtr((Ptr)ckb);
	
	SetRect( &viewRect, instDescBox.right - 150, instDescBox.bottom + 2,
						instDescBox.right, instDescBox.bottom + 14 );
	dsNeededH = NULL;
	dsNeededH = TENew( &viewRect, &viewRect );
	if (!dsNeededH)
	{
		ErrorHandler(eMem, nil);
		return;
	}
	
	
	GetResourcedString( msg, rInstList, sDiskSpcNeeded );
	cstr = PascalToC(msg);
	msglen = strlen(cstr);
	cmsg = (char*)malloc(msglen+255);
	strncpy(cmsg, cstr, msglen);
	cmsg[msglen] = '\0';
	
	
	cSpaceNeeded = DiskSpaceNeeded();
	msglen += strlen(cSpaceNeeded);
	strcat( cmsg, cSpaceNeeded );
	cmsg[msglen] = '\0';
	
	
	GetResourcedString( kb, rInstList, sKilobytes );
	ckb = PascalToC(kb);
	msglen += strlen(ckb);
	strcat( cmsg, ckb );
	cmsg[msglen] = '\0';
	
	
	TEInsert( cmsg, strlen(cmsg), dsNeededH );
	TEUpdate( &viewRect, dsNeededH );
	
	if (dsAvailH)
		TEDispose(dsAvailH);
	if (dsNeededH)
		TEDispose(dsNeededH);
	
	if (ckb)
		DisposePtr((Ptr)ckb);
	if (cSpaceNeeded)
		free(cSpaceNeeded);		
	if (cfreeSpace)
		free(cfreeSpace);
	if (cstr)
		DisposePtr((Ptr)cstr);
	if (cmsg)
		free(cmsg);
	TextFont(systemFont);
	TextSize(12);
}

char *
DiskSpaceNeeded(void)
{
	char *cSpaceNeeded;
	short i;
	long spaceNeeded = 0;
	int instChoice = gControls->opt->instChoice - 1;
	
	


	for (i=0; i<kMaxComponents; i++)
	{	
		
		if (gControls->opt->instChoice == gControls->cfg->numSetupTypes)
		{
			if ((gControls->cfg->st[instChoice].comp[i] == kInSetupType) &&
				(gControls->cfg->comp[i].selected == true))
				spaceNeeded += gControls->cfg->comp[i].size;
		}
		
		
		else if (gControls->cfg->st[instChoice].comp[i] == kInSetupType)
		{
			spaceNeeded += gControls->cfg->comp[i].size;
		}
	}
	
	cSpaceNeeded = ltoa(spaceNeeded);
	sDSNeededK = spaceNeeded;
	
	return cSpaceNeeded;
}

void
ClearDiskSpaceMsgs(void)
{
	Rect instDescBox, viewRect;
	Handle instDescRectH;
	OSErr	reserr;
	GrafPtr	oldPort;
	
	GetPort(&oldPort);
	if (gWPtr)
		SetPort(gWPtr);
	
	instDescRectH = NULL;
	instDescRectH = Get1Resource('RECT', rCompListBox);
	reserr = ResError();
	if (reserr!=noErr || !instDescRectH)
	{
		ErrorHandler(reserr, nil);
		return;
	}

	HLock(instDescRectH);
	instDescBox = (Rect) **((Rect**)instDescRectH);
	SetRect( &viewRect, instDescBox.left, instDescBox.top, 
						instDescBox.right, instDescBox.bottom + 14 );
	HUnlock(instDescRectH);	
	DetachResource(instDescRectH);
	DisposeHandle(instDescRectH);
						
	EraseRect( &viewRect );
	InvalRect( &viewRect );
	
	SetPort(oldPort);
}








#define kMaxLongLen	12
char *
ltoa(long n)
{
	char	s[kMaxLongLen] = "";
	char *returnBuf;
	int i, j, sign;
	
	
	if ( (sign = n) < 0)
		n = -n;
	i = 0;
	
	
	do
	{
		s[i++] = n % 10 + '0';  
	}
	while( (n /= 10) > 0);	
	
	
	if (sign < 0)
	{
		s[i++] = '-';
	}

	s[i] = '\0';
	
	
	for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
	{
		char tmp = s[i];
		s[i] = s[j];
		s[j] = tmp;
	}

	returnBuf = (char *)malloc(strlen(s) + 1);
	strcpy(returnBuf, s);
	return returnBuf;
}

short
pstrcmp(unsigned char* s1, unsigned char* s2)
{
	long len;
	register short i;
	
	if ( *s1 != *s2)  
		return false;
	
	len = *s1;
	for (i=0; i<len; i++)
	{
		s1++;
		s2++;
		if (*s1 != *s2)
			return false;
	}
	
	return true;
}

unsigned char*
pstrcpy(unsigned char* dest, unsigned char* src)
{
	long len;
	register short i;
	unsigned char* origdest;
	
	if (!dest || !src)
		return nil;
	
	origdest = dest;
	len = *src;
	for (i=0; i<=len; i++)
	{
		*dest = *src;
		dest++;
		src++;
	}
		
	return origdest;
}
	
unsigned char*
pstrcat(unsigned char* dst, unsigned char* src)
{
	unsigned char 	*origdst;
	long			dlen, slen;
	register short	i;
	
	if (!dst || !src)
		return nil;
		
	origdst = dst;
	dlen = *dst;
	slen = *src;
	*dst = dlen+slen; 
	
	for (i=1; i<=slen; i++)
	{
		*(dst+dlen+i) = *(src+i);
	}
	
	return origdst;
}

void
GetAllVInfo( unsigned char **volName, short *count)
{
	QHdrPtr				vcbQ;
	VCB *				currVCB;
	register short		i;
	
	vcbQ = GetVCBQHdr();
	currVCB = (VCB *)vcbQ->qHead;
	i = 0;
	while(1)
	{
		volName[i] = currVCB->vcbVN;
		
		
		i++;  
		if (currVCB == (VCB *) vcbQ->qTail)
			break;
		currVCB = (VCB *)currVCB->qLink;
	}
	*count = i;
}

Boolean
LegacyFileCheck(short vRefNum, long dirID)
{
	Boolean 	bRetry = false;
	int			i, diffLevel;
	StringPtr	pFilepath = 0, pMessage = 0, pSubfolder = 0;
	FSSpec		legacy, fsDest;
	OSErr		err = noErr;
	short		dlgRV = 0;
	char		cFilepath[1024];
	AlertStdAlertParamRec *alertdlg;
	
	for (i = 0; i < gControls->cfg->numLegacyChecks; i++)
	{
		
		HLock(gControls->cfg->checks[i].filename);
		if (!**gControls->cfg->checks[i].filename)
		{
			HUnlock(gControls->cfg->checks[i].filename);
			continue;
		}
		HLock(gControls->cfg->checks[i].subfolder);
		memset(cFilepath, 0, 1024);
		strcpy(cFilepath, ":");
		strcat(cFilepath, *gControls->cfg->checks[i].subfolder);
		strcat(cFilepath, ":");
		strcat(cFilepath, *gControls->cfg->checks[i].filename);
		HUnlock(gControls->cfg->checks[i].filename);
		pSubfolder = CToPascal(*gControls->cfg->checks[i].subfolder);
		HUnlock(gControls->cfg->checks[i].subfolder);
		pFilepath = CToPascal(cFilepath);
		
		err = FSMakeFSSpec(vRefNum, dirID, pFilepath, &legacy);
		if (pFilepath)
			DisposePtr((Ptr)pFilepath);
			
		
		if (err == noErr)
		{
			
			diffLevel = CompareVersion( gControls->cfg->checks[i].version, 
										&legacy );
			if (diffLevel > 0)
			{
				
				if (!gControls->cfg->checks[i].message || !(*gControls->cfg->checks[i].message))
					continue;
				HLock(gControls->cfg->checks[i].message);
				pMessage = CToPascal(*gControls->cfg->checks[i].message);
				HUnlock(gControls->cfg->checks[i].message);
				if (!pMessage)
					continue;
				
				
				alertdlg = (AlertStdAlertParamRec *)NewPtrClear(sizeof(AlertStdAlertParamRec));
				alertdlg->defaultButton = kAlertStdAlertOKButton;
				alertdlg->defaultText = (ConstStringPtr)NewPtrClear(kKeyMaxLen);
				alertdlg->cancelText = (ConstStringPtr)NewPtrClear(kKeyMaxLen);
			    GetResourcedString((unsigned char *)alertdlg->defaultText, rInstList, sDeleteBtn);
			    GetResourcedString((unsigned char *)alertdlg->cancelText, rInstList, sCancel);
				StandardAlert(kAlertCautionAlert, pMessage, nil, alertdlg, &dlgRV);
				if (dlgRV == 1) 
				{			
					
					err = FSMakeFSSpec(gControls->opt->vRefNum, gControls->opt->dirID, pSubfolder, &fsDest);
					if (err == noErr) 
						DeleteDirectoryContents(fsDest.vRefNum, fsDest.parID, fsDest.name);
				}
				else
					bRetry = true;
					
				if (pMessage)
					DisposePtr((Ptr) pMessage);
			}
		}
	}
	
	if (pSubfolder)
		DisposePtr((Ptr) pSubfolder);
		
	return bRetry;
}

int
CompareVersion(Handle newVersion, FSSpecPtr file)
{
	int			diffLevel = 0, intVal;
	OSErr		err = noErr;
	short		fileRef;
	Handle		versRsrc = nil;
	char		oldRel, oldRev, oldFix, oldInternalStage, oldDevStage, oldRev_n_Fix;
	char		*newRel, *newRev, newFix[2], *newInternalStage, newDevStage, *newFix_n_DevStage;
	Ptr			newVerCopy;
	
	
	if (!newVersion || !(*newVersion))
		return 6;
	
	
	if (!file)
		return -6;	
		
			
	fileRef = FSpOpenResFile(file, fsRdPerm);
	if (fileRef == -1)
		return -9;
		
	versRsrc = Get1Resource('vers', 1);
	if (versRsrc == nil)
	{
		CloseResFile(fileRef);
		return -10;
	}
	
	
	HLock(versRsrc);
	oldRel = *(*versRsrc);
	oldRev_n_Fix = *((*versRsrc)+1);
	oldDevStage = *((*versRsrc)+2);
	oldInternalStage = *((*versRsrc)+3);
	HUnlock(versRsrc);
	CloseResFile(fileRef);
	
	oldRev = (oldRev_n_Fix & 0xF0) >> 4;
	oldFix =  oldRev_n_Fix & 0x0F;
	
	
	HLock(newVersion);
	newVerCopy = NewPtrClear(strlen(*newVersion));
	BlockMove(*newVersion, newVerCopy, strlen(*newVersion));
	newRel = strtok(newVerCopy, ".");
	newRev = strtok(NULL, ".");
	newFix_n_DevStage = strtok(NULL, ".");
	newInternalStage = strtok(NULL, ".");
	HUnlock(newVersion);
	
	


	newDevStage = 0x80; 					
	if (NULL != strchr(newFix_n_DevStage, 'd')) 
		newDevStage = 0x20;					
	else if (NULL != strchr(newFix_n_DevStage, 'a'))
		newDevStage = 0x40;					
	else if (NULL != strchr(newFix_n_DevStage, 'b')) 
		newDevStage = 0x60;					
	 	
	newFix[0] = *newFix_n_DevStage;
	newFix[1] = 0;
	
	
	intVal = atoi(newRel);
	if (oldRel < intVal)
	{
		diffLevel = 5;
		goto au_revoir;
	}
	else if (oldRel > intVal)
	{
		diffLevel = -5;
		goto au_revoir;
	}
		
	intVal = atoi(newRev);
	if (oldRev < intVal)
	{
		diffLevel = 4;
		goto au_revoir;
	}
	else if (oldRev > intVal)
	{
		diffLevel = -4;
		goto au_revoir;
	}
		
	intVal = atoi(newFix);
	if (oldFix < intVal)
	{	
		diffLevel = 3;
		goto au_revoir;
	}
	else if (oldFix > intVal)
	{
		diffLevel = -3;
		goto au_revoir;
	}
	
	intVal = atoi(newInternalStage);
	if (oldInternalStage < intVal)
	{
		diffLevel = 2;
		goto au_revoir;
	}
	else if (oldInternalStage > intVal)
	{
		diffLevel = -2;
		goto au_revoir;
	}
	
	if (oldDevStage < newDevStage)
	{
		diffLevel = 1;
		goto au_revoir;
	}
	else if (oldDevStage > newDevStage)
	{
		diffLevel = -1;
		goto au_revoir;
	}
	
	
	diffLevel = 0;

au_revoir:
	if (newVerCopy)
		DisposePtr(newVerCopy);
			
	return diffLevel;
}

Boolean
VerifyDiskSpace(void)
{
    char dsNeededStr[255], dsAvailStr[255];
    short alertRV;
    Str255 pMessage, pStr;
    AlertStdAlertParamRec *alertdlg;
    
    if (sDSNeededK > sDSAvailK)
    {
        sprintf(dsNeededStr, "%d", sDSNeededK);
        sprintf(dsAvailStr, "%d", sDSAvailK);

        GetResourcedString(pMessage, rInstList, sSpaceMsg1);
        pstrcat(pMessage, CToPascal(dsAvailStr));
        pstrcat(pMessage, CToPascal("KB \r"));
        GetResourcedString(pStr, rInstList, sSpaceMsg2);
        pstrcat(pStr, CToPascal(dsNeededStr));
        pstrcat(pStr, CToPascal("KB \r\r"));
        pstrcat(pMessage, pStr);
        GetResourcedString(pStr, rInstList, sSpaceMsg3);
        pstrcat(pMessage, pStr);
        alertdlg = (AlertStdAlertParamRec *)NewPtrClear(sizeof(AlertStdAlertParamRec));
        alertdlg->defaultButton = kAlertStdAlertCancelButton;
        alertdlg->defaultText = (ConstStringPtr)NewPtrClear(kKeyMaxLen);
        alertdlg->cancelText = (ConstStringPtr)NewPtrClear(kKeyMaxLen);
        GetResourcedString((unsigned char *)alertdlg->defaultText, rInstList, sOKBtn);
        GetResourcedString((unsigned char *)alertdlg->cancelText, rInstList, sQuitBtn);
        StandardAlert(kAlertCautionAlert, pMessage, nil, alertdlg, &alertRV);
        if (alertRV == 2)
        {
            gDone = true;
        }
        return false;
    }    
    
    return true;
}

void
EnableSetupTypeWin(void)
{
    EnableNavButtons();
	
    


    if (gControls->backB)
        HiliteControl(gControls->backB, kDisableControl);
        
    if (gControls->stw->instType)
        HiliteControl(gControls->stw->instType, kEnableControl);
    if (gControls->stw->destLoc)
        HiliteControl(gControls->stw->destLoc, kEnableControl);
}

void
DisableSetupTypeWin(void)
{
	DisableNavButtons();
	
	if (gControls->stw->instType)
		HiliteControl(gControls->stw->instType, kDisableControl);
	if (gControls->stw->destLoc)
		HiliteControl(gControls->stw->destLoc, kDisableControl);
}
