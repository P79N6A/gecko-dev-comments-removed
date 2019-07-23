
































#ifndef nptest_platform_h_
#define nptest_platform_h_

#include "nptest.h"




bool    pluginSupportsWindowMode();






bool    pluginSupportsWindowlessMode();





NPError pluginInstanceInit(InstanceData* instanceData);




void    pluginInstanceShutdown(InstanceData* instanceData);





void    pluginWidgetInit(InstanceData* instanceData, void* oldWindow);





int16_t pluginHandleEvent(InstanceData* instanceData, void* event);

#endif 
