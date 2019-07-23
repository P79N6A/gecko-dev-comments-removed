




































#pragma once









#ifndef __EVENTS__
#include <Events.h>
#endif

typedef Boolean (*EventFilterProcPtr) (EventRecord* event);
typedef Boolean (*MenuFilterProcPtr) (long menuSelection);

OSStatus InstallEventFilters(EventFilterProcPtr eventFilter, MenuFilterProcPtr menuFilter);
OSStatus RemoveEventFilters();
