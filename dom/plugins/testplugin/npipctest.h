
































#ifndef nptest_h_
#define nptest_h_

#include "mozilla-config.h"

#include "npapi.h"
#include "npfunctions.h"
#include "npruntime.h"
#include "prtypes.h"

typedef enum  {
  DM_DEFAULT,
  DM_SOLID_COLOR
} DrawMode;

typedef struct _PlatformData PlatformData;

typedef struct InstanceData {
  NPP npp;
  NPWindow window;
  PlatformData* platformData;
  uint32_t instanceCountWatchGeneration;
  bool lastReportedPrivateModeState;
  bool hasWidget;
  uint32_t timerID1;
  uint32_t timerID2;
} InstanceData;

#endif 
