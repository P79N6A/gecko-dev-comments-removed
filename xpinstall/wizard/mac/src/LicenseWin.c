






































#include "MacInstallWizard.h"






void
ShowLicenseWin(void)
{
	Str255 		accept;
	Str255 		decline;
	Rect 		sbRect;
	int 		sbWidth;
	
	GrafPtr	oldPort;
	GetPort(&oldPort);
	
	SetPort(gWPtr);

	gCurrWin = kLicenseID; 
	
	
	GetResourcedString(accept, rInstList, sAcceptBtn);
	GetResourcedString(decline, rInstList, sDeclineBtn);
	
	gControls->lw->scrollBar = GetNewControl( rLicScrollBar, gWPtr);
	gControls->lw->licBox = GetNewControl( rLicBox, gWPtr);

	if(gControls->lw->scrollBar && gControls->lw->licBox)
	{
		HLock( (Handle) gControls->lw->scrollBar);
		sbRect = (*(gControls->lw->licBox))->contrlRect;
				
		sbWidth = (*(gControls->lw->scrollBar))->contrlRect.right -
				  (*(gControls->lw->scrollBar))->contrlRect.left;
		
		(*(gControls->lw->scrollBar))->contrlRect.right = sbRect.right + kScrollBarPad;
		(*(gControls->lw->scrollBar))->contrlRect.left = sbRect.right + kScrollBarPad - 
														 sbWidth;
		(*(gControls->lw->scrollBar))->contrlRect.top = sbRect.top - kScrollBarPad;
		(*(gControls->lw->scrollBar))->contrlRect.bottom = sbRect.bottom + kScrollBarPad;
		HUnlock( (Handle) gControls->lw->scrollBar);
	}
	else
	{
		ErrorHandler(eParam, nil);
		return;
	}
	InitLicTxt();

	ShowNavButtons( decline, accept);
	ShowControl( gControls->lw->scrollBar);
	ShowControl( gControls->lw->licBox);
	ShowTxt();
	InitScrollBar( gControls->lw->scrollBar);
	ShowLogo(false);
	
	SetPort(oldPort);
}

void
InitLicTxt(void)
{
	Rect	destRect, viewRect;
	FSSpec	licFile;
	long 	dirID, dataSize;
	short 	vRefNum, dataRef, resRef;
	unsigned char* 	cLicFName;
	Str255			pLicFName;
	OSErr	err;
	Handle 	text, stylHdl;
	
	ERR_CHECK(GetCWD(&dirID, &vRefNum));
	
	
	HLock(gControls->cfg->licFileName);
	if(**gControls->cfg->licFileName != nil)
	{
		cLicFName = CToPascal(*gControls->cfg->licFileName);
		
		ERR_CHECK(FSMakeFSSpec(vRefNum, dirID, cLicFName, &licFile));
		if (cLicFName)
			DisposePtr((char*)cLicFName);
	}
	else 
	{	
		GetResourcedString(pLicFName, rInstList, sLicenseFName);
		ERR_CHECK(FSMakeFSSpec(vRefNum, dirID, pLicFName, &licFile));
	}
	HUnlock(gControls->cfg->licFileName);
	
	
	ERR_CHECK(FSpOpenDF( &licFile, fsRdPerm, &dataRef));
	ERR_CHECK(GetEOF(dataRef, &dataSize));

	if (dataSize > 0)
	{
		if (!(text = NewHandle(dataSize)))
		{
			ErrorHandler(eMem, nil);
			return;
		}
		ERR_CHECK(FSRead(dataRef, &dataSize, *text));
	}
	else
		text = nil;
	ERR_CHECK(FSClose(dataRef));

	
	resRef = FSpOpenResFile( &licFile, fsRdPerm);
	ERR_CHECK(ResError());

	UseResFile(resRef);
	stylHdl = RGetResource('styl', 128);
	ERR_CHECK(ResError());
	
	if(stylHdl)
		DetachResource(stylHdl);
	else
		stylHdl = nil;
	CloseResFile(resRef);
	
	
	HLock( (Handle) gControls->lw->licBox);
	SetRect(&viewRect, 	(*(gControls->lw->licBox))->contrlRect.left, 
						(*(gControls->lw->licBox))->contrlRect.top, 
						(*(gControls->lw->licBox))->contrlRect.right, 
						(*(gControls->lw->licBox))->contrlRect.bottom);
	HUnlock( (Handle) gControls->lw->licBox);

	destRect.left = viewRect.left;
		viewRect.right = (*(gControls->lw->scrollBar))->contrlRect.left; 
	destRect.right = viewRect.right;
	destRect.top = viewRect.top;
	destRect.bottom = viewRect.bottom * kNumLicScrns;
	
	
	
	TextFont(applFont);
	TextFace(normal);
	TextSize(9);
	
	HLock(text);
	if (stylHdl)
	{
		gControls->lw->licTxt = TEStyleNew( &destRect, &viewRect );
		TEStyleInsert( *text, dataSize, (StScrpRec ** )stylHdl, 
						gControls->lw->licTxt);
	}
	else
	{
		gControls->lw->licTxt = TENew( &destRect, &viewRect);
		TEInsert( *text, dataSize, gControls->lw->licTxt);
	}
	HUnlock(text);
	
	TextFont(systemFont);
	TextSize(12);
	
	TESetAlignment(teFlushDefault, gControls->lw->licTxt);
}

void
ShowTxt(void)
{	    
	switch (gCurrWin)
	{
		case kLicenseID:
			if(gControls->lw->licTxt)
			{
			    RGBColor backColorOld;
			    Rect     textRect;
			    
			    
			    GetBackColor(&backColorOld);
			    
			    
			    BackColor(whiteColor);
			    
			    
			    textRect = (**(gControls->lw->licTxt)).viewRect;
			    EraseRect(&textRect);
				TEUpdate(&textRect, gControls->lw->licTxt);
				
				
				RGBBackColor(&backColorOld);
			}
			break;
		default:
			break;
	}		
}

void
ShowLogo(Boolean bEraseRect)
{
	short 		reserr;
	Rect 		derefd, logoRect;
	PicHandle 	logoPicH;
	Handle		logoRectH; 

	
    ControlHandle imgWellH = GetNewControl(rLogoImgWell, gWPtr);
    if (!imgWellH)
    {
        ErrorHandler(eMem, nil);
        return;
	}

	
	logoPicH = GetPicture(rNSLogo);  
	reserr = ResError();
	
	if (reserr == noErr)
	{
		
		if (logoPicH != nil)
		{		
			logoRectH = GetResource('RECT', rNSLogoBox);
			reserr = ResError();
			if (reserr == noErr && logoRectH)
			{
				HLock(logoRectH);
				derefd = (Rect) **((Rect**)logoRectH);
				SetRect(&logoRect, derefd.left, derefd.top, derefd.right, derefd.bottom);
				HUnlock(logoRectH);
				reserr = ResError();
				if (reserr == noErr)
				{
					if (bEraseRect)
					{
						EraseRect(&logoRect);
						InvalRect(&logoRect);
					}
					DrawPicture(logoPicH, &logoRect);
					ReleaseResource((Handle)logoPicH);
				}
				
				ReleaseResource((Handle)logoRectH);
			}
		}
	}
	
	if (reserr != noErr)
		ErrorHandler(reserr, nil);
}

void
InLicenseContent(EventRecord* evt, WindowPtr wCurrPtr)
{
	Point 			localPt;
	Rect			r;
	ControlPartCode	part;
	short 			code, value;
	ControlHandle	scrollBar;
	ControlActionUPP	scrollActionFunctionUPP;
	GrafPtr			oldPort;
	
	GetPort(&oldPort);
	SetPort(gWPtr);
	localPt = evt->where;
	GlobalToLocal( &localPt);
	
	code = FindControl(localPt, wCurrPtr, &scrollBar);
	switch (code)
	{
		case kControlUpButtonPart:
		case kControlDownButtonPart:
		case kControlPageUpPart:
		case kControlPageDownPart:
			scrollActionFunctionUPP = NewControlActionUPP(DoScrollProc);
			value = TrackControl(scrollBar, localPt, scrollActionFunctionUPP);
 			return;
			
		case kControlIndicatorPart:
			value = GetControlValue(scrollBar);
			code = TrackControl(scrollBar, localPt, nil);
			if (code) 
			{
				value -= GetControlValue(scrollBar);
				if (value) 
				{
					TEScroll(0, value * kScrollAmount, gControls->lw->licTxt);
                    ShowTxt();
				}
			}
			return;
	}
	
	HLock((Handle)gControls->backB);
	r = (**(gControls->backB)).contrlRect;
	HUnlock((Handle)gControls->backB);
	if (PtInRect( localPt, &r))
	{
		part = TrackControl(gControls->backB, evt->where, NULL);
		if (part)
			gDone = true;  
	}
	
	HLock((Handle)gControls->nextB);			
	r = (**(gControls->nextB)).contrlRect;
	HUnlock((Handle)gControls->nextB);
	if (PtInRect( localPt, &r))
	{
		part = TrackControl(gControls->nextB, evt->where, NULL);
		if (part)
		{
			KillControls(gWPtr);
			ShowSetupTypeWin();
			return;
		}
	}
	
	ShowTxt();
	
	SetPort(oldPort);
}

void
EnableLicenseWin(void)
{
	GrafPtr	oldPort;
	GetPort(&oldPort);
	
	SetPort(gWPtr);
	
	EnableNavButtons();
	
	if(gControls->lw->licBox)
		HiliteControl(gControls->lw->licBox, kEnableControl);	
	if(gControls->lw->scrollBar)
		HiliteControl(gControls->lw->scrollBar, kEnableControl);			
	
	ShowTxt();
	SetPort(oldPort);
	
	
}

void
DisableLicenseWin(void)
{
	DisableNavButtons();
	
	



	if(gControls->lw->scrollBar)
		HiliteControl(gControls->lw->scrollBar, kDisableControl);
	
}

void 
InitScrollBar(ControlHandle sb)
{
	short		lines;
	short		max;
	short		height;
	TEPtr		te;
	TEHandle	currTE;
	
	switch(gCurrWin)
	{
		case kLicenseID:
			currTE = gControls->lw->licTxt;
			break;
		default:
			ErrorHandler(eUnknownDlgID, nil);
			break;
	}
	
	lines = TEGetHeight((**currTE).nLines,0,currTE) / kScrollAmount;
	te = *currTE;							

	height = te->viewRect.bottom - te->viewRect.top;
	max = lines - (height / kScrollAmount);
	if (height % kScrollAmount) max++;
	if ( max < 0 ) max = 0;

	SetControlMaximum(sb, max);
}

pascal void 
DoScrollProc(ControlHandle theControl, short part)
{
	short		amount;
	TEPtr		te;
	
	if ( part != 0 ) {
		switch (gCurrWin)
		{
			case kLicenseID:				
				te = *(gControls->lw->licTxt);
				break;
			default:
				ErrorHandler(eUnknownDlgID, nil);
				break;
		}
		
		switch ( part ) {
			case kControlUpButtonPart:
			case kControlDownButtonPart:		
				amount = 1;
				break;
			case kControlPageUpPart:			
			case kControlPageDownPart:
				amount = (te->viewRect.bottom - te->viewRect.top) / kScrollAmount;
				break;
		}
		if ( (part == kControlDownButtonPart) || (part == kControlPageDownPart) )
			amount = -amount;
		CalcChange(theControl, &amount);
		if (amount) {
			TEScroll(0, amount * kScrollAmount, &te);
            ShowTxt();
		}
	}
}

void 
CalcChange(ControlHandle theControl,	short *amount)
{
	short		value, max;
	
	value = GetControlValue(theControl);	
	max = GetControlMaximum(theControl);		
	*amount = value - *amount;
	if (*amount < 0)
		*amount = 0;
	else if (*amount > max)
		*amount = max;
	SetControlValue(theControl, *amount);
	*amount = value - *amount;			
}

void
ShowNavButtons(unsigned char* backTitle, unsigned char* nextTitle)
{
    Boolean bDefault = true;
    	
	gControls->backB = GetNewControl( rBackBtn, gWPtr);
	gControls->nextB = GetNewControl( rNextBtn, gWPtr);

	if( gControls->backB != NULL)
	{
		SetControlTitle( gControls->backB, backTitle); 
		ShowControl( gControls->backB);

		if (gCurrWin==kWelcomeID || gCurrWin==kSetupTypeID)
			HiliteControl(gControls->backB, kDisableControl);
	}
	
	if ( gControls->nextB != NULL)
	{
		SetControlTitle( gControls->nextB, nextTitle);
		ShowControl( gControls->nextB);

        SetControlData(gControls->nextB, kControlNoPart, 
            kControlPushButtonDefaultTag, sizeof(bDefault),(Ptr) &bDefault);
	}
	
    ShowCancelButton();
}

void
EnableNavButtons(void)
{
	if (gControls->backB && gCurrWin!=kWelcomeID)
		HiliteControl(gControls->backB, kEnableControl);
	if (gControls->nextB)
		HiliteControl(gControls->nextB, kEnableControl);

    if (gControls->cancelB)
		HiliteControl(gControls->cancelB, kEnableControl);

}

void
DisableNavButtons(void)
{	
	if (gControls->backB)
		HiliteControl(gControls->backB, kDisableControl);
	if(gControls->nextB)
		HiliteControl(gControls->nextB, kDisableControl);

    if (gControls->cancelB)
		HiliteControl(gControls->cancelB, kDisableControl);
}
