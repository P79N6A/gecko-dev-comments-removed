





















































 
#ifndef __PDEUTILITIES__
#define __PDEUTILITIES__

#include <Carbon/Carbon.h>
#include <Print/PMPrintingDialogExtensions.h>

#include "PDECore.h"








extern void         MyDebugMessage (char *msg, SInt32 err);
extern CFTypeRef    MyCFAssign(CFTypeRef srcRef, CFTypeRef dstRef);

extern CFBundleRef  MyGetBundle();
extern void         MyFreeBundle();

extern CFStringRef  MyGetTitle();
extern void         MyFreeTitle();

extern OSStatus     MyEmbedControl (WindowRef, ControlRef, const ControlID*, ControlRef*);
extern OSStatus     MyGetTicket (PMPrintSession, CFStringRef, PMTicketRef*);

extern void         MyReleaseContext (MyContext);
extern OSStatus     MyInstallHelpEventHandler (WindowRef, EventHandlerRef*, EventHandlerUPP *);
extern OSStatus     MyRemoveHelpEventHandler (EventHandlerRef*, EventHandlerUPP *);


#endif
