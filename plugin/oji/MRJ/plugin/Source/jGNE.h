




































#pragma once









#ifndef __EVENTS__
#include <Events.h>
#endif

typedef Boolean (*EventFilterProcPtr) (EventRecord* event);

OSStatus InstallEventFilter(EventFilterProcPtr filter);
OSStatus RemoveEventFilter();
