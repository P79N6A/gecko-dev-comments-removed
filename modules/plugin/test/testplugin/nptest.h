
































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

typedef struct TestNPObject : NPObject {
  NPP npp;
  DrawMode drawMode;
  PRUint32 drawColor; 
} TestNPObject;

typedef struct _PlatformData PlatformData;

typedef struct InstanceData {
  NPP npp;
  NPWindow window;
  TestNPObject* scriptableObject;
  PlatformData* platformData;
  uint32_t instanceCountWatchGeneration;
  bool lastReportedPrivateModeState;
  bool hasWidget;
  uint32_t timerID1;
  uint32_t timerID2;
  int32_t lastMouseX;
  int32_t lastMouseY;
} InstanceData;

#endif 
