






































#ifndef nsAEDefs_h_
#define nsAEDefs_h_


#include <MacTypes.h>


typedef char	CStr255[256];		
typedef long TAEListIndex;    
typedef short TWindowKind;







enum {
	kAnyWindowKind = 99,
	
	
	kBrowserWindowKind = 100,
	kMailWindowKind,
	kMailComposeWindowKind,
	kComposerWindowKind,
	kAddressBookWindowKind,
	kOtherWindowKind
	
};


typedef enum
{
	eSaveUnspecified,
	eSaveYes,
	eSaveNo,
	eSaveAsk
} TAskSave;


#if DEBUG

#define AE_ASSERT(x,msg)	{ if (!(x)) { DebugStr("\p"msg); } }

#else

#define AE_ASSERT(x,msg)	((void) 0)

#endif




#endif 
