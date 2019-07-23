
































#ifndef nptest_platform_h_
#define nptest_platform_h_

#include "nptest.h"

NPError pluginInstanceInit(InstanceData* instanceData);
int16_t pluginHandleEvent(InstanceData* instanceData, void* event);

void    pluginDraw(InstanceData* instanceData);

#endif 
