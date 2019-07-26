
































#ifndef nptest_platform_h_
#define nptest_platform_h_

#include "nptest.h"




bool    pluginSupportsWindowMode();






bool    pluginSupportsWindowlessMode();




bool    pluginSupportsAsyncBitmapDrawing();




inline bool    pluginSupportsAsyncDXGIDrawing()
{
#ifdef XP_WIN
  return true;
#else
  return false;
#endif
}





NPError pluginInstanceInit(InstanceData* instanceData);




void    pluginInstanceShutdown(InstanceData* instanceData);




void    pluginDoSetWindow(InstanceData* instanceData, NPWindow* newWindow);





void    pluginWidgetInit(InstanceData* instanceData, void* oldWindow);





int16_t pluginHandleEvent(InstanceData* instanceData, void* event);

#ifdef XP_WIN
void    pluginDrawAsyncDxgiColor(InstanceData* instanceData);
#endif

enum RectEdge {
  EDGE_LEFT = 0,
  EDGE_TOP = 1,
  EDGE_RIGHT = 2,
  EDGE_BOTTOM = 3
};

enum {
  NPTEST_INT32_ERROR = 0x7FFFFFFF
};








int32_t pluginGetEdge(InstanceData* instanceData, RectEdge edge);






int32_t pluginGetClipRegionRectCount(InstanceData* instanceData);








int32_t pluginGetClipRegionRectEdge(InstanceData* instanceData, 
    int32_t rectIndex, RectEdge edge);






void pluginDoInternalConsistencyCheck(InstanceData* instanceData, std::string& error);





std::string pluginGetClipboardText(InstanceData* instanceData);







bool pluginCrashInNestedLoop(InstanceData* instanceData);










bool pluginDestroySharedGfxStuff(InstanceData* instanceData);

#endif 
