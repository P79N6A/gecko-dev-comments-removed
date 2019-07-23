






































#include "MacInstallWizard.h"






Boolean 	gDone = false;
WindowPtr 	gWPtr = NULL;
short		gCurrWin = 0;
InstWiz		*gControls = NULL;
InstINIRes  *gStrings = NULL;
Boolean     gInstallStarted = false;
ErrTableEnt *gErrTable = NULL;
short gErrTableSize = 0;







int gState;

void main(void)
{	
	OSErr err = noErr;
	
	Init();
	if (VerifyEnv() && !gDone)	
	{
		err = NavLoad();
		if (err!=noErr)
			SysBeep(10);	
			
		ShowWindow(gWPtr);
		MainEventLoop();
	}
}

Boolean
VerifyEnv(void)
{
	long	response;
	OSErr 	err = noErr;
	Boolean bEnvOK = true;
	
	
	err = Gestalt('sysv', &response);
	if (err != noErr)
	{
		
        ErrorHandler(err, nil);
		bEnvOK = false;
	}
	
	if (response < 0x00000850)
	{
		
		StopAlert(160, nil);
		bEnvOK = false;
	}
	
	
	return bEnvOK;
}

void Init(void)
{
	Str255		 	winTitle;
	OSErr			err = noErr;
    Str255			instMode;
    Ptr				pinstMode;
	
	gDone = false;
	InitManagers();
	InitControlsObject();	
	CleanTemp();

	ParseInstall();

	gWPtr = GetNewCWindow(rRootWin, NULL, (WindowPtr) -1);	
    GetIndString( instMode, rTitleStrList, sNSInstTitle);
    pinstMode = PascalToC(instMode);
#if MOZILLA == 0
    GetResourcedString(winTitle, rInstList, sNsTitle);
#else
    GetResourcedString(winTitle, rInstList, sMoTitle);
#endif
	SetWTitle( gWPtr, winTitle );	
	SetWRefCon(gWPtr, kMIWMagic);
	MakeMenus();

	ParseConfig(); 
	InitOptObject();
	
	ShowWelcomeWin();	
	SetThemeWindowBackground(gWPtr, kThemeBrushDialogBackgroundActive, true); 
	
	
	InitNewMenu();
}

OSErr
GetCWD(long *outDirID, short *outVRefNum)
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
			*outDirID = pInfo.processAppSpec->parID;
			*outVRefNum = pInfo.processAppSpec->vRefNum;
		}
	}
	
	return err;
}

void
InitOptObject(void)
{
	FSSpec 	tmp;
	OSErr	err=noErr;
	Boolean isDir;
	
	gControls->opt = (Options*)NewPtrClear(sizeof(Options));

	if (!gControls->opt)
	{
        ErrorHandler(eMem, nil);
		return;
	}
	
	
	gControls->opt->instChoice = 1;		
	gControls->opt->folder = (unsigned char *)NewPtrClear(64*sizeof(unsigned char));
	if (!gControls->opt->folder)
	{
        ErrorHandler(eMem, nil);
		return;
	}
	
	
	gControls->opt->siteChoice = 1;
	gControls->opt->saveBits = false;
	
	gControls->opt->vRefNum = -1;
	err = FSMakeFSSpec(gControls->opt->vRefNum, 0, "\p", &tmp);
	pstrcpy( gControls->opt->folder, tmp.name );
	err = FSpGetDirectoryID( &tmp, &gControls->opt->dirID, &isDir );

}

void
InitControlsObject(void)
{	
	gControls 		= (InstWiz *) 		NewPtrClear(sizeof(InstWiz));
	if (!gControls)
	{
        ErrorHandler(eMem, nil);
		return;
	}
	
	gControls->lw 	= (LicWin *) 		NewPtrClear(sizeof(LicWin));
	gControls->ww 	= (WelcWin *) 		NewPtrClear(sizeof(WelcWin));
	gControls->stw 	= (SetupTypeWin *) 	NewPtrClear(sizeof(SetupTypeWin));	
	gControls->cw 	= (CompWin *) 		NewPtrClear(sizeof(CompWin));
	gControls->aw 	= (CompWin *) 		NewPtrClear(sizeof(CompWin));
	gControls->tw 	= (TermWin*) 		NewPtrClear(sizeof(TermWin));

	if (!gControls->lw || !gControls->ww || !gControls->stw || 
		!gControls->cw || !gControls->tw)
	{
        ErrorHandler(eMem, nil);
	}
	
	gControls->state = eInstallNotStarted;
	
	return;
}

void InitManagers(void)
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

void CleanTemp(void)
{
    OSErr   err = noErr;
    short   vRefNum;
    long    dirID;
    FSSpec  viewerFSp;
    XPISpec *xpiList, *currXPI = 0, *nextXPI = 0;
#ifdef MIW_DEBUG
    Boolean isDir = false;
#endif
    
#ifndef MIW_DEBUG
    
    ERR_CHECK(FindFolder(kOnSystemDisk, kTemporaryFolderType, kCreateFolder, &vRefNum, &dirID));
    err = FSMakeFSSpec(vRefNum, dirID, kViewerFolder, &viewerFSp);
#else
    
    ERR_CHECK(GetCWD(&dirID, &vRefNum));
 	err = FSMakeFSSpec(vRefNum, 0, kTempFolder, &viewerFSp);
	if (err == fnfErr)
	    return; 
	err = FSpGetDirectoryID(&viewerFSp, &dirID, &isDir);
	if (err != noErr || !isDir)
	    return;
    err = FSMakeFSSpec(vRefNum, dirID, kViewerFolder, &viewerFSp);
#endif
    
    
    if (err == noErr)
    {
        ERR_CHECK(DeleteDirectory(viewerFSp.vRefNum, viewerFSp.parID, viewerFSp.name));
    }
    
    
    xpiList = (XPISpec *) NewPtrClear(sizeof(XPISpec));
    if (!xpiList)
        return;
    IterateDirectory(vRefNum, dirID, "\p", 1, CheckIfXPI, (void*)&xpiList);
    
    if (xpiList)
    {
        currXPI = xpiList;
        while(currXPI)
        {
            nextXPI = currXPI->next; 
            if (currXPI->FSp)
            {
                FSpDelete(currXPI->FSp);
                DisposePtr((Ptr)currXPI->FSp);
            }
            DisposePtr((Ptr)currXPI);
            currXPI = nextXPI;
        }
    }
}

pascal void CheckIfXPI(const CInfoPBRec * const cpbPtr, Boolean *quitFlag, void *dataPtr)
{
    OSErr err = noErr;
    char cFilename[256];    
    int len = 0;            
    FSSpecPtr currFSp;
    XPISpec *currXPI = 0, *newXPI = 0, **xpiList = 0;
    
    
    if (!cpbPtr || !dataPtr)
        return;    
     xpiList = (XPISpec **)dataPtr;
     
    
    if ((cpbPtr->hFileInfo.ioFlAttrib & ioDirMask) == 0)
    {
        if (!cpbPtr->hFileInfo.ioNamePtr)
            return;
        len = *(cpbPtr->hFileInfo.ioNamePtr);            
        strncpy(cFilename, (char*)(cpbPtr->hFileInfo.ioNamePtr + 1), len);
        
           
        if (0 == strncmp(".xpi", cFilename+len-4, 4))
        {
            currFSp = (FSSpecPtr) NewPtrClear(sizeof(FSSpec));
            if (!currFSp)
                return;
            err = FSMakeFSSpec(cpbPtr->hFileInfo.ioVRefNum, cpbPtr->hFileInfo.ioFlParID,
                               cpbPtr->hFileInfo.ioNamePtr, currFSp);
                               
            
            if (err == noErr)
            {
                currXPI = *xpiList;
                while (currXPI)
                { 
                    
                    if (!currXPI->FSp)
                    {
                        newXPI = currXPI;  
                        break;
                    }
                    
                    
                    if (currXPI->next)
                    {
                        currXPI = currXPI->next;
                        continue;
                    }
                     
                    else
                    {
                        newXPI = (XPISpec *) NewPtrClear(sizeof(XPISpec));
                        if (!newXPI)
                            return;  
                        currXPI->next = newXPI;
                        break;
                    }
                }
                newXPI->FSp = currFSp;
            }
            else
                DisposePtr((Ptr) currFSp);
        }
    }
    
    
    *quitFlag = false;
}

void MakeMenus(void)
{
    Handle 		mbarHdl;
	MenuHandle	menuHdl;
	OSErr		err;
	
	if ( !(mbarHdl = GetNewMBar( rMBar)) )
	{
        ErrorHandler(eMem, nil);
		return;
	}
	
	SetMenuBar(mbarHdl);
	
	if ( (menuHdl = GetMenuHandle(mApple)) != nil) 
	{
		AppendResMenu(menuHdl, 'DRVR');
	}
	else
        ErrorHandler(eMenuHdl, nil); 

	ERR_CHECK(HMGetHelpMenuHandle(&menuHdl));
	DisableItem(menuHdl, 1);

	DrawMenuBar();
}

static 	RgnHandle gMouseRgn;

void MainEventLoop(void)
{
	gMouseRgn = NewRgn();
	
	while (!gDone) 
	{	
        YieldToAnyThread();  
	    MainEventLoopPass();	
	}
	
	if (gMouseRgn)
		DisposeRgn(gMouseRgn);
	gMouseRgn = (RgnHandle) 0;
	Shutdown();
}
 


int BreathFunc()
{
    static int ticks = 0;
    
    ticks++;
    if ( ( ticks % 4 ) == 0 ) {
        ticks = 0;
        MainEventLoopPass();
        if ( gDone == true ) {     
            if (gMouseRgn)
		        DisposeRgn(gMouseRgn);
	        gMouseRgn = (RgnHandle) 0;
	        Shutdown();
	    } 
    }
    return 1;
}

void  MainEventLoopPass()
{
    EventRecord evt;
	Boolean		notHandled = true;

    if (!gDone)	 
    {
		if(WaitNextEvent(everyEvent, &evt, 1, gMouseRgn))
		{
			if (gMouseRgn)
				SetRectRgn(gMouseRgn, evt.where.h, evt.where.v, evt.where.h + 1, evt.where.v + 1);
					
			HandleNextEvent(&evt);
		}
	}
}
 
void ErrorHandler(short errCode, Str255 msg)
{



    Str255      pErrorStr;
    Str255      pMessage, errMsg;
    char        *cErrNo = 0;
    StringPtr   pErrNo = 0;
    AlertStdAlertParamRec *alertdlg;

    
    static Boolean bErrHandled = false;
    if (bErrHandled)
        return;
    else
        bErrHandled = true;
        
    
    if( errCode == eInstRead )
    {
        GetIndString(pErrorStr, rStringList, errCode);
        ParamText(pErrorStr, "\p", "\p", "\p");
        StopAlert(rAlrtError, nil);
        SysBeep(10);
        gDone = true;
        return;
    }
	
    GetResourcedString(pMessage, rErrorList, eErr1);
    GetResourcedString(pErrorStr, rErrorList, eErr2);
    
    cErrNo = ltoa(errCode);
    pErrNo = CToPascal(cErrNo);
    
    if (errCode > 0)    
    {
        GetResourcedString(pErrorStr, rErrorList, errCode);
        pstrcat(pMessage, pErrNo);
        pstrcat(pMessage, "\p: ");
        pstrcat(pMessage, pErrorStr);
    }
    else
    {
        GetResourcedString(pMessage, rErrorList, eErr3);
        if ( LookupErrorMsg( errCode, errMsg ) == true )
          pstrcat(pMessage, errMsg);
        else 
          pstrcat(pMessage, pErrNo);
        if ( msg[0] != 0 ) {
          pstrcat(pMessage, "\p : ");
          pstrcat(pMessage, msg);
        }
    }  
        
    alertdlg = (AlertStdAlertParamRec *)NewPtrClear(sizeof(AlertStdAlertParamRec));
    alertdlg->defaultButton = kAlertStdAlertOKButton;
    alertdlg->defaultText = (ConstStringPtr)NewPtrClear(kKeyMaxLen);
    GetResourcedString((unsigned char *)alertdlg->defaultText, rInstList, sOKBtn);
    StandardAlert(kAlertStopAlert, pMessage, nil, alertdlg, 0);
	  SysBeep(10);
	
    if (cErrNo)
        free(cErrNo);
    if (pErrNo)
        DisposePtr((Ptr) pErrNo); 
        
	gDone = true;
}

Boolean
LookupErrorMsg( short code, Str255 msg )
{
    int i;
    Boolean retval = false;
    msg[0] = 1; msg[1] = ' ';
    
    for ( i = 0; i < gErrTableSize; i++ ) {
      if ( gErrTable[i].num == code ) {
          pstrcat( msg, gErrTable[i].msg );
          retval = true;
          break;
        }   
    }
    return( retval );
}

void Shutdown(void)
{
	WindowPtr	frontWin;
	long 		MIWMagic = 0;

	NavUnload();
	
#if 0


    if (gControls->cfg)
    {
        
        if (gControls->cfg->targetSubfolder)
            DisposePtr((Ptr) gControls->cfg->targetSubfolder);
        if (gControls->cfg->globalURL)
            DisposePtr((Ptr) gControls->cfg->globalURL);
            
        
        if (gControls->cfg->licFileName)
            DisposePtr((Ptr) gControls->cfg->licFileName);        
            
        
        for (i = 0; i < kNumWelcMsgs; i++)
        {
            if (gControls->cfg->welcMsg[i])
                DisposePtr((Ptr) gControls->cfg->welcMsg[i]);  
        }      
        if (gControls->cfg->readmeFile)
            DisposePtr((Ptr) gControls->cfg->readmeFile);    
        if (gControls->cfg->readmeApp)
            DisposePtr((Ptr) gControls->cfg->readmeApp);
            
        
        if (gControls->cfg->selCompMsg)
            DisposePtr((Ptr) gControls->cfg->selCompMsg);
        if (gControls->cfg->selAddMsg)
            DisposePtr((Ptr) gControls->cfg->selAddMsg);

                    
        if (gControls->cfg->startMsg)
            DisposePtr((Ptr) gControls->cfg->startMsg);
        if (gControls->cfg->saveBitsMsg)
            DisposePtr((Ptr) gControls->cfg->saveBitsMsg);
                        
        
        if (gControls->cfg->coreFile)
            DisposePtr((Ptr) gControls->cfg->coreFile);  
        if (gControls->cfg->coreDir)
            DisposePtr((Ptr) gControls->cfg->coreDir);  
        if (gControls->cfg->noAds)
            DisposePtr((Ptr) gControls->cfg->noAds);  
        if (gControls->cfg->silent)
            DisposePtr((Ptr) gControls->cfg->silent);  
        if (gControls->cfg->execution)
            DisposePtr((Ptr) gControls->cfg->execution);  
        if (gControls->cfg->confirmInstall)
            DisposePtr((Ptr) gControls->cfg->confirmInstall);
            
        DisposePtr((Ptr)gControls->cfg);
    }
    	

	if (gControls->opt && gControls->opt->folder)
	{
		DisposePtr((Ptr) gControls->opt->folder);
		DisposePtr((Ptr) gControls->opt);
	}
		
	

	if (gControls->nextB)
		DisposeControl(gControls->nextB);  
	if (gControls->backB)
		DisposeControl(gControls->backB);
	
	if (gControls->lw)
		DisposePtr( (char*) gControls->lw);
	if (gControls->ww)
		DisposePtr( (char*) gControls->ww);
	if (gControls->stw)
		DisposePtr( (char*) gControls->stw);
	if (gControls->cw)
		DisposePtr( (char*) gControls->cw);
	if (gControls->tw)
		DisposePtr( (char*) gControls->tw);
	
	if (gControls)
		DisposePtr( (char*) gControls);
		
#endif 
			
	frontWin = FrontWindow();
	MIWMagic = GetWRefCon(frontWin);
	if (MIWMagic != kMIWMagic)
		if (gWPtr)
			BringToFront(gWPtr);

	if (gWPtr)
	{
		HideWindow(gWPtr);
		DisposeWindow(gWPtr);
	}
	ExitToShell();
}


void InitNewMenu()
{
    MenuHandle		instMenu=0;
    MenuRef			fileMenu, editMenu;
    Str255			menuText;

    instMenu = GetMenuHandle(mApple);
#if MOZILLA == 0
    	GetResourcedString(menuText, rInstMenuList, sMenuAboutNs);
#else
    	GetResourcedString(menuText, rInstMenuList, sMenuAboutMo);
#endif
    SetMenuItemText(instMenu, iAbout, menuText);
    
    GetResourcedString(menuText, rInstMenuList, sMenuFile);
    fileMenu = NewMenu(mFile, menuText);
    InsertMenu(fileMenu, mFile);
    GetResourcedString(menuText, rInstMenuList, sMenuEdit);
    editMenu = NewMenu(mEdit, menuText);
    InsertMenu(editMenu, mEdit);
    DrawMenuBar();

    GetResourcedString(menuText, rInstMenuList, sMenuQuit);
    AppendMenu(fileMenu, menuText);
    GetResourcedString(menuText, rInstMenuList, sMenuQuitHot);
    SetItemCmd(fileMenu, iQuit, menuText[1]);
    
    GetResourcedString(menuText, rInstMenuList, sMenuUndo);
    AppendMenu(editMenu, menuText);
    GetResourcedString(menuText, rInstMenuList, sMenuUndoHot);
    SetItemCmd(editMenu, iUndo, menuText[1]);
    pstrcpy(menuText, CToPascal("-"));
    AppendMenu(editMenu, menuText);
    GetResourcedString(menuText, rInstMenuList, sMenuCut);
    AppendMenu(editMenu, menuText);
    GetResourcedString(menuText, rInstMenuList, sMenuCutHot);
    SetItemCmd(editMenu, iCut, menuText[1]);
    GetResourcedString(menuText, rInstMenuList, sMenuCopy);
    AppendMenu(editMenu, menuText);
    GetResourcedString(menuText, rInstMenuList, sMenuCopyHot);
    SetItemCmd(editMenu, iCopy, menuText[1]);
    GetResourcedString(menuText, rInstMenuList, sMenuPaste);
    AppendMenu(editMenu, menuText);
    GetResourcedString(menuText, rInstMenuList, sMenuPasteHot);
    SetItemCmd(editMenu, iPaste, menuText[1]);
    GetResourcedString(menuText, rInstMenuList, sMenuClear);
    AppendMenu(editMenu, menuText);
    GetResourcedString(menuText, rInstMenuList, sMenuClearHot);
    SetItemCmd(editMenu, iClear, menuText[1]);
}
