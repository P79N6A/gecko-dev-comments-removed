







































#pragma once

#ifndef __MAC_XPIDL_PANEL__
#define __MAC_XPIDL_PANEL__

#ifndef __TYPES__
#include <Types.h>
#endif

#pragma options align=mac68k


#define kXPIDLPanelName	"xpidl Settings"








enum {

	class_XPIDL					= 'XIDL',

	prefsPR_ProjectType			= 'PR01',	
	prefsPR_FileName			= 'PR02',	
	prefsLN_GenerateSymFile		= 'LN02',	
	
	
	enumeration_ProjectType		= 'PRPT',
	enum_Project_Application	= 'PRPA',	
	enum_Project_Library		= 'PRPL',	
	enum_Project_SharedLibrary	= 'PRPS',	
	enum_Project_CodeResource	= 'PRPC',	
	enum_Project_MPWTool		= 'PRPM'	
};

enum {
	kXPIDLModeHeader = 1,
	kXPIDLModeJava,
	kXPIDLModeTypelib,
	kXPIDLModeDoc
};





enum {
	kXPIDLSettingsVersion = 0x0100
};

struct XPIDLSettings {
	short		version;			
	short		mode;				
	Boolean		warnings;			
	Boolean		verbose;			
	Str32Field	output;				
};

typedef struct XPIDLSettings XPIDLSettings, **XPIDLSettingsHandle;

#pragma options align=reset

#endif	
