












































#include "jGNE.h"

#include <MixedMode.h>
#include <Memory.h>
#include <LowMem.h>
#include <TextUtils.h>




#pragma options align=mac68k

struct Jump {
	unsigned short		jmp;
	UniversalProcPtr	addr;
};
 
#pragma options align=reset

static void GNEFilter(EventRecord *event, Boolean* result);

static RoutineDescriptor theGNEFilterDescriptor = BUILD_ROUTINE_DESCRIPTOR(uppGetNextEventFilterProcInfo, GNEFilter);
static Jump* theGNEFilterJump;
static GetNextEventFilterUPP theOldGNEFilterUPP = NULL;
static EventFilterProcPtr  theEventFilter = NULL;

static Str63 theAppName;

OSStatus InstallEventFilter(EventFilterProcPtr filter)
{
	if (theEventFilter == NULL) {
		theEventFilter = filter;

		
		StringPtr currentAppName = LMGetCurApName();
		::BlockMoveData(currentAppName, theAppName, 1 + currentAppName[0]);

		
		if (theGNEFilterJump == NULL) {
			theGNEFilterJump = (Jump*) NewPtrSys(sizeof(Jump));
			if (theGNEFilterJump == NULL)
				return MemError();
			
			theGNEFilterJump->jmp = 0x4EF9;
			theGNEFilterJump->addr = &theGNEFilterDescriptor;
			
			
			theOldGNEFilterUPP = LMGetGNEFilter();
			LMSetGNEFilter(GetNextEventFilterUPP(theGNEFilterJump));
		} else {
			
			theOldGNEFilterUPP = theGNEFilterJump->addr;
			theGNEFilterJump->addr = &theGNEFilterDescriptor;
		}
		
		return noErr;
	}
	return paramErr;
}

OSStatus RemoveEventFilter()
{
	if (theEventFilter != NULL) {
		
		if (LMGetGNEFilter() == GetNextEventFilterUPP(theGNEFilterJump)) {
			
			LMSetGNEFilter(theOldGNEFilterUPP);
			DisposePtr(Ptr(theGNEFilterJump));
			theGNEFilterJump = NULL;
		} else {
			
			theGNEFilterJump->addr = theOldGNEFilterUPP;
		}
		theOldGNEFilterUPP = NULL;
		theEventFilter = NULL;
		return noErr;
	}
	return paramErr;
}

static void GNEFilter(EventRecord *event, Boolean* result)
{
	
	if (theOldGNEFilterUPP != NULL)
		CallGetNextEventFilterProc(theOldGNEFilterUPP, event, result);

	
	if (*result) {
		
		
		{
			
			static Boolean inFilter = false;
			if (! inFilter) {
				inFilter = true;
				Boolean filteredEvent = theEventFilter(event);
				if (filteredEvent) {
					
					event->what = nullEvent;
					*result = false;
				}
				inFilter = false;
			}
		}
	}
}
