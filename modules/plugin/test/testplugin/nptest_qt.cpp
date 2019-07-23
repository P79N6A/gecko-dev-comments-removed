
































#include "nptest_platform.h"

 using namespace std;

bool
pluginSupportsWindowMode()
{
  return false;
}

bool
pluginSupportsWindowlessMode()
{
  return true;
}

NPError
pluginInstanceInit(InstanceData* instanceData)
{
  return NPERR_NO_ERROR;
}

void
pluginInstanceShutdown(InstanceData* instanceData)
{
}

void
pluginDoSetWindow(InstanceData* instanceData, NPWindow* newWindow)
{
  instanceData->window = *newWindow;
}

void
pluginWidgetInit(InstanceData* instanceData, void* oldWindow)
{
}

int16_t
pluginHandleEvent(InstanceData* instanceData, void* event)
{
  return 0;
}

int32_t pluginGetEdge(InstanceData* instanceData, RectEdge edge)
{
  
  return NPTEST_INT32_ERROR;
}

int32_t pluginGetClipRegionRectCount(InstanceData* instanceData)
{
  
  return NPTEST_INT32_ERROR;
}

int32_t pluginGetClipRegionRectEdge(InstanceData* instanceData, 
    int32_t rectIndex, RectEdge edge)
{
  
  return NPTEST_INT32_ERROR;
}

void pluginDoInternalConsistencyCheck(InstanceData* instanceData, string& error)
{
}
