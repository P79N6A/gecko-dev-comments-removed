







































#define CW_STRICT_DIALOGS 1


#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#include <AERegistry.h>
#include <Drag.h>
#include <Palettes.h>
#include <Resources.h>
#include <Scrap.h>
#include <TextUtils.h>
#include <Sound.h>


#include <DropInPanel.h>


#include "mac_xpidl_panel.h"

enum {
	kFactoryPrefsID = 128,
	kCW7ItemListID = 128,
	kCW8ItemListID = 129,

	kXPIDLModeItem = 1,
	kXPIDLWarningsItem,
	kXPIDLVerboseItem,

	kXPTLinkerOutputItem = 4
};



static RgnHandle	sDragRgn;
static Boolean		sHighlightOn;



static short	InitDialog(PanelParameterBlock *pb);
static void		TermDialog(PanelParameterBlock *pb);
static void		PutData(PanelParameterBlock *pb, Handle options);
static short	GetData(PanelParameterBlock *pb, Handle options, Boolean noisy);
static void		ByteSwapData(XPIDLSettingsHandle options);
static short	Filter(PanelParameterBlock *pb, EventRecord *event, short *itemHit);
static void		ItemHit(PanelParameterBlock *pb);
static void		Validate(Handle original, Handle current, Boolean *recompile, Boolean *relink, Boolean *reset);
static short	GetPref(AEKeyword keyword, AEDesc *prefsDesc, Handle settings);
static short	SetPref(AEKeyword keyword, const AEDesc *prefsDesc, Handle settings);
static short	GetFactory(Handle settings);
static short	UpdatePref(Handle settings);
static Boolean	ComparePrefs(Handle prefsHand1, Handle prefsHand2);
static Boolean	ComparePrefs(XPIDLSettings& prefs1, XPIDLSettings& prefs2);
static void		OutlineRect(const Rect* focusRect, Boolean outlineOn);
static OSErr	DragEnter(PanelParameterBlock *pb);
static void		DragWithin(PanelParameterBlock *pb);
static void		DragExit(PanelParameterBlock *pb);
static void		DragDrop(PanelParameterBlock *pb);

extern "C" {

pascal short	xpidl_panel(PanelParameterBlock *pb);

}





 
pascal short xpidl_panel(PanelParameterBlock *pb)
{
	short	result;
		
	result = noErr;
			
	switch (pb->request)
	{
	case reqInitPanel:
		
		break;

	case reqTermPanel:
		
		break;

	case reqFirstLoad:
		
		break;

	case reqInitDialog:
		
		result = InitDialog(pb);
		break;
	
	case reqTermDialog:
		
		TermDialog(pb);
		break;
	
	case reqPutData:
		
		PutData(pb, pb->currentPrefs);
		break;

	case reqGetData:
		
		result = GetData(pb, pb->currentPrefs, true);
		break;

	case reqByteSwapData:
		
		ByteSwapData((XPIDLSettingsHandle)pb->currentPrefs);
		break;
		
	case reqFilter:
		
		result = Filter(pb, pb->event, &pb->itemHit);
		break;
		
	case reqItemHit:
		
		ItemHit(pb);
		break;
		
	case reqDrawCustomItem:
		
		break;
		
	case reqActivateItem:
		break;
		
	case reqDeactivateItem:
		break;
		
	case reqHandleKey:
		break;
		
	case reqHandleClick:
		break;
		
	case reqFindStatus:
		break;
		
	case reqObeyCommand:
		break;
		
	case reqAEGetPref:
		
		result = GetPref(pb->prefsKeyword, &pb->prefsDesc, pb->currentPrefs);
		break;

	case reqAESetPref:
		
		result = SetPref(pb->prefsKeyword, &pb->prefsDesc, pb->currentPrefs);
		break;

	case reqValidate:
		
		Validate(pb->originalPrefs, pb->currentPrefs, &pb->recompile, &pb->relink, &pb->reset);
		break;

	case reqGetFactory:
		
		result = GetFactory(pb->factoryPrefs);
		break;

	case reqUpdatePref:
		
		result = UpdatePref(pb->currentPrefs);
		break;
		
	case reqDragEnter:
		
		result = DragEnter(pb);
		break;
	
	case reqDragWithin:
		
		DragWithin(pb);
		break;
	
	case reqDragExit:
		
		DragExit(pb);
		break;
	
	case reqDragDrop:
		
		DragDrop(pb);
		break;
	
	default:
		result = paramErr;
		break;
	}
		
	return (result);
}






static short InitDialog(PanelParameterBlock *pb)
{
	OSErr	err;
	
	
	
	
	
	err = CWPanlAppendItems(pb, kCW8ItemListID);
	if (err != noErr)
		return (err);
	
	sDragRgn = NewRgn();
	
	return (err);
}






static void TermDialog(PanelParameterBlock *pb)
{
	DisposeRgn(sDragRgn);
}

inline Boolean hasLinkerOutput(short mode)
{
	return (mode == kXPIDLModeHeader || mode == kXPIDLModeTypelib);
}






static void PutData(PanelParameterBlock *pb, Handle options)
{
	
	UpdatePref(options);
	
	XPIDLSettings prefsData = **(XPIDLSettingsHandle) options;

	CWPanlSetItemValue(pb, kXPIDLModeItem, prefsData.mode);
	CWPanlSetItemValue(pb, kXPIDLWarningsItem, prefsData.warnings);
	CWPanlSetItemValue(pb, kXPIDLVerboseItem, prefsData.verbose);

	CWPanlEnableItem(pb, kXPTLinkerOutputItem, hasLinkerOutput(prefsData.mode));
	CWPanlSetItemText(pb, kXPTLinkerOutputItem, prefsData.output);
}






static short GetData(PanelParameterBlock *pb, Handle options, Boolean noisy)
{
	XPIDLSettings prefsData	= **(XPIDLSettingsHandle) options;
	long mode, warnings, verbose;
	
	CWPanlGetItemValue(pb, kXPIDLModeItem, &mode);
	CWPanlGetItemValue(pb, kXPIDLWarningsItem, &warnings);
	CWPanlGetItemValue(pb, kXPIDLVerboseItem, &verbose);
	
	prefsData.mode = (short) mode;
	prefsData.warnings = (Boolean) warnings;
	prefsData.verbose = (Boolean) verbose;
	
	CWPanlGetItemText(pb, kXPTLinkerOutputItem, prefsData.output, sizeof(Str32));
	
	** (XPIDLSettingsHandle) options = prefsData;
	
	return (noErr);
}

static void ByteSwapShort(short* x)
{
	union {
		short	s;
		char	c[2];
	}	from,to;

	from.s=*x;
	to.c[0]=from.c[1];
	to.c[1]=from.c[0];
	*x = to.s;
}






static void ByteSwapData(XPIDLSettingsHandle options)
{
	ByteSwapShort(&(**options).version);
	ByteSwapShort(&(**options).mode);
}





static short Filter(PanelParameterBlock *pb, EventRecord *event, short *itemHit)
{
#pragma unused(pb, event, itemHit)
	
	return (noErr);
}






static void ItemHit(PanelParameterBlock *pb)
{
	short	theItem	= pb->itemHit - pb->baseItems;
	long	oldValue;
	
	switch (theItem) {
	case kXPIDLModeItem:
		CWPanlGetItemValue(pb, theItem, &oldValue);
		CWPanlEnableItem(pb, kXPTLinkerOutputItem, hasLinkerOutput(oldValue));
		break;
		
	case kXPIDLWarningsItem:
	case kXPIDLVerboseItem:
		CWPanlGetItemValue(pb, theItem, &oldValue);
		break;
	}
	
	GetData(pb, pb->currentPrefs, false);

	pb->canRevert	= !ComparePrefs(pb->originalPrefs, pb->currentPrefs);
	pb->canFactory	= !ComparePrefs(pb->factoryPrefs,  pb->currentPrefs);
}






static void Validate(Handle original, Handle current, Boolean *recompile, Boolean *relink, Boolean *reset)
{
#pragma unused(original, current)
	XPIDLSettings& origSettings = **(XPIDLSettingsHandle) original;
	XPIDLSettings& currentSettings = **(XPIDLSettingsHandle) current;
	
	*recompile	= currentSettings.mode != origSettings.mode;
	*relink		= *recompile && hasLinkerOutput(currentSettings.mode);
	*reset		= false;
}





static short GetPref(AEKeyword keyword, AEDesc *prefsDesc, Handle settings)
{
#if 0	
	XPIDLSettings	prefsData	= ** (XPIDLSettingsHandle) settings;
	DescType	anEnum;
	OSErr		err;

	switch (keyword)  {
	case prefsLN_GenerateSymFile:
		err = AECreateDesc(typeBoolean, &prefsData.linksym, sizeof(Boolean), prefsDesc);
		break;
		
	case prefsPR_ProjectType:
		switch (prefsData.projtype)
		{
		case kProjTypeApplication:	anEnum = enum_Project_Application;		break;
		case kProjTypeLibrary:		anEnum = enum_Project_Library;			break;
		case kProjTypeSharedLib:	anEnum = enum_Project_SharedLibrary;	break;
		case kProjTypeCodeResource:	anEnum = enum_Project_CodeResource;		break;
		case kProjTypeMPWTool:		anEnum = enum_Project_MPWTool;			break;
		default:					return (paramErr);
		}
		err = AECreateDesc(typeEnumeration, &anEnum, sizeof(anEnum), prefsDesc);
		break;
		
	case prefsPR_FileName:
		err = AECreateDesc(typeChar, prefsData.outfile+1, StrLength(prefsData.outfile), prefsDesc);
		break;

	default:
		err = errAECantHandleClass;
		break;
	}
	
	return (err);
#else
	return (errAECantHandleClass);
#endif
}






static short SetPref(AEKeyword keyword, const AEDesc *prefsDesc, Handle settings)
{
#if 0
	XPIDLSettings	prefsData	= ** (XPIDLSettingsHandle) settings;
	AEDesc			toDesc	= { typeNull, NULL };
	OSErr			err		= noErr;
	Handle			dataHand;
	Size			textLength;
	DescType		anEnum;
	
	switch (keyword)
	{
	case prefsLN_GenerateSymFile:
		if (prefsDesc->descriptorType == typeBoolean)
		{
			dataHand = prefsDesc->dataHandle;
		}
		else
		{
			err = AECoerceDesc(prefsDesc, typeBoolean, &toDesc);
			if (err == noErr)
				dataHand = toDesc.dataHandle;
		}
		if (err == noErr)
		{
			prefsData.linksym = ** (Boolean **) dataHand;
		}
		break;
		
	case prefsPR_ProjectType:
		if (prefsDesc->descriptorType != typeEnumeration)
		{
			err = errAETypeError;
			break;
		}

		anEnum = ** (DescType **) prefsDesc->dataHandle;
		
		switch (anEnum)
		{
		case enum_Project_Application:		prefsData.projtype = kProjTypeApplication;	break;
		case enum_Project_Library:			prefsData.projtype = kProjTypeLibrary;		break;
		case enum_Project_SharedLibrary:	prefsData.projtype = kProjTypeSharedLib;	break;
		case enum_Project_CodeResource:		prefsData.projtype = kProjTypeCodeResource;	break;
		case enum_Project_MPWTool:			prefsData.projtype = kProjTypeMPWTool;		break;
		default:							return (errAECoercionFail);
		}
		break;
		
	case prefsPR_FileName:
		if (prefsDesc->descriptorType == typeChar)
		{
			dataHand = prefsDesc->dataHandle;
		}
		else
		{
			err = AECoerceDesc(prefsDesc, typeChar, &toDesc);
			if (err == noErr)
				dataHand = toDesc.dataHandle;
		}
		if (err == noErr)
		{
			textLength = GetHandleSize(dataHand);
			if (textLength > sizeof(prefsData.outfile) - 1)
				textLength = sizeof(prefsData.outfile) - 1;
			BlockMoveData(*dataHand, prefsData.outfile+1, textLength);
			prefsData.outfile[0] = textLength;
		}
		break;

	default:
		err = errAECantHandleClass;
		break;
	}
	
	if (err == noErr)
	{
		** (XPIDLSettingsHandle) settings = prefsData;
	}
	
	AEDisposeDesc(&toDesc);
	
	return (err);
#else
	return (errAECantHandleClass);
#endif
}






static short GetFactory(Handle settings)
{
	Handle	factory;
	Size	size;
	OSErr	err;
	
	factory = Get1Resource('pref', kFactoryPrefsID);
	if (factory == NULL) {
		err = ResError();
		if (err == noErr)
			err = resNotFound;
		return (err);
	}
	
	size = GetHandleSize(factory);
	SetHandleSize(settings, size);
	err = MemError();
	
	if (err == noErr) {
		BlockMoveData(*factory, *settings, size);
	}
	
	return (err);
}




static short UpdatePref(Handle settings)
{
	if (GetHandleSize(settings) != sizeof(XPIDLSettings))
		GetFactory(settings);

	return (noErr);
}





static Boolean ComparePrefs(Handle prefsHand1, Handle prefsHand2)
{
	XPIDLSettings& prefs1	= **(XPIDLSettingsHandle) prefsHand1;
	XPIDLSettings& prefs2	= **(XPIDLSettingsHandle) prefsHand2;
	
	return ((prefs1.mode  == prefs2.mode) && 
			(prefs1.warnings == prefs2.warnings) && 
			(prefs1.verbose == prefs2.verbose) &&
			(EqualString(prefs1.output, prefs2.output, true, true)));
}

static Boolean ComparePrefs(XPIDLSettings& prefs1, XPIDLSettings& prefs2)
{
	return ((prefs1.mode  == prefs2.mode) && 
			(prefs1.warnings == prefs2.warnings) && 
			(prefs1.verbose == prefs2.verbose) &&
			(EqualString(prefs1.output, prefs2.output, true, true)));
}





static void	OutlineRect(const Rect* focusRect, Boolean outlineOn)
{
	ColorSpec	savedForeColor, backColor;
	PenState	savedPen;
	
	GetPenState(&savedPen);
	PenNormal();
	
	if (!outlineOn)
	{
		SaveFore(&savedForeColor);
		SaveBack(&backColor);
		RestoreFore(&backColor);
	}
	
	PenSize(2, 2);
	FrameRect(focusRect);
	
	SetPenState(&savedPen);
	
	if (!outlineOn)
	{
		RestoreFore(&savedForeColor);
	}
}





static OSErr	DragEnter(PanelParameterBlock *pb)
{
#if 0
	short			theItem	= pb->itemHit - pb->baseItems;
	unsigned short	itemCount;
	Rect			itemRect;
	OSErr			err;
#endif
	
	
	return (paramErr);
}





static void	DragWithin(PanelParameterBlock *pb)
{
#pragma unused(pb)

	
	

}





static void	DragExit(PanelParameterBlock *pb)
{
	OSErr	err;
	

	
	if (sHighlightOn) {
		err = HideDragHilite(pb->dragref);
		if (err == noErr)
			sHighlightOn = false;
	}
}





static void	DragDrop(PanelParameterBlock *pb)
{
#if 0
	Rect	itemRect;
#endif
	

	
	DragExit(pb);
}
