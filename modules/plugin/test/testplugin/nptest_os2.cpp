
































#include "nptest_platform.h"

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
pluginWidgetInit(InstanceData* instanceData, void* oldWindow)
{
}

int16_t
pluginHandleEvent(InstanceData* instanceData, void* event)
{
  return 0;
}
