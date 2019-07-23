














































#include "EventFilter.h"

#include <MixedMode.h>
#include <Memory.h>
#include <LowMem.h>
#include <Menus.h>
#include <Patches.h>
#include <Traps.h>

enum {
	uppWaitNextEventProcInfo = kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(Boolean)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(EventMask)))
		| STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(EventRecord*)))
		| STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(UInt32)))
		| STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(RgnHandle))),
	uppMenuSelectProcInfo = kPascalStackBased
		| RESULT_SIZE(SIZE_CODE(sizeof(long)))
		| STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(Point)))
};

static pascal Boolean NewWaitNextEvent(EventMask eventMask, EventRecord* event, UInt32 sleep, RgnHandle mouseRgn);
static pascal long NewMenuSelect(Point where);

enum {
	kIsToolboxTrap = (1 << 11)
};

inline TrapType getTrapType(UInt16 trapNum)
{
	return ((trapNum & kIsToolboxTrap) != 0 ? kToolboxTrapType : kOSTrapType);
}

static UniversalProcPtr SwapTrapAddress(UInt16 trapNum, UniversalProcPtr newTrapAddress)
{
	TrapType type = getTrapType(trapNum);
	UniversalProcPtr oldTrapAddress = NGetTrapAddress(trapNum, type);
	NSetTrapAddress(newTrapAddress, trapNum, type);
	return oldTrapAddress;
}

struct Patch {
	UInt16 trap;
	RoutineDescriptor descriptor;
	UniversalProcPtr original;

	void Install() { original = SwapTrapAddress(trap, &descriptor); }
	void Remove() { SwapTrapAddress(trap, original); original = NULL; }
};

static Patch WaitNextEventPatch = {
	_WaitNextEvent,
	BUILD_ROUTINE_DESCRIPTOR(uppWaitNextEventProcInfo, NewWaitNextEvent),
	NULL
};

static Patch MenuSelectPatch = {
	_MenuSelect,
	BUILD_ROUTINE_DESCRIPTOR(uppMenuSelectProcInfo, NewMenuSelect),
	NULL
};

static EventFilterProcPtr theEventFilter;
static MenuFilterProcPtr theMenuFilter;

OSStatus InstallEventFilters(EventFilterProcPtr eventFilter, MenuFilterProcPtr menuFilter)
{
	if (theEventFilter == NULL) {
		theEventFilter = eventFilter;
		theMenuFilter = menuFilter;

		
		WaitNextEventPatch.Install();
		
		
		MenuSelectPatch.Install();

		return noErr;
	}
	return paramErr;
}

OSStatus RemoveEventFilters()
{
	if (theEventFilter != NULL) {
		WaitNextEventPatch.Remove();
		MenuSelectPatch.Remove();
		
		theEventFilter = NULL;
		theMenuFilter = NULL;
		
		return noErr;
	}
	return paramErr;
}

static pascal Boolean NewWaitNextEvent(EventMask eventMask, EventRecord* event, UInt32 sleep, RgnHandle mouseRgn)
{
	Boolean gotEvent = CALL_FOUR_PARAMETER_UPP(WaitNextEventPatch.original, uppWaitNextEventProcInfo, eventMask, event, sleep, mouseRgn);
	if (true) {
		
		static Boolean inFilter = false;
		if (! inFilter) {
			inFilter = true;
			Boolean filteredEvent = theEventFilter(event);
			if (filteredEvent) {
				
				event->what = nullEvent;
				gotEvent = false;
			}
			inFilter = false;
		}
	}
	return gotEvent;
}

static pascal long NewMenuSelect(Point where)
{
	long menuSelection = CALL_ONE_PARAMETER_UPP(MenuSelectPatch.original, uppMenuSelectProcInfo, where);
	if (menuSelection != 0) {
		Boolean filteredEvent = theMenuFilter(menuSelection);
		if (filteredEvent) {
			
			menuSelection = 0;
		}
	}
	return menuSelection;
}
