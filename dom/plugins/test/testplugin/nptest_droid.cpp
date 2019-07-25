































#include "nptest_platform.h"
#include "npapi.h"

struct _PlatformData {
};
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
  printf("NPERR_INCOMPATIBLE_VERSION_ERROR\n");
  return NPERR_INCOMPATIBLE_VERSION_ERROR;
}

void
pluginInstanceShutdown(InstanceData* instanceData)
{
  NPN_MemFree(instanceData->platformData);
  instanceData->platformData = 0;
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

static void
pluginDrawWindow(InstanceData* instanceData, void* event)
{
  return;
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
