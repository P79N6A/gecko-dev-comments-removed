
































#ifndef nptest_h_
#define nptest_h_

#include "mozilla-config.h"

#include "npapi.h"
#include "npfunctions.h"
#include "npruntime.h"
#include "prtypes.h"
#include <string>
#include <sstream>

typedef enum  {
  DM_DEFAULT,
  DM_SOLID_COLOR
} DrawMode;

typedef enum {
  FUNCTION_NONE,
  FUNCTION_NPP_GETURL,
  FUNCTION_NPP_GETURLNOTIFY,
  FUNCTION_NPP_POSTURL,
  FUNCTION_NPP_POSTURLNOTIFY,
  FUNCTION_NPP_NEWSTREAM,
  FUNCTION_NPP_WRITEREADY,
  FUNCTION_NPP_WRITE,
  FUNCTION_NPP_DESTROYSTREAM
} TestFunction;

typedef struct FunctionTable {
  TestFunction funcId;
  const char* funcName;
} FunctionTable;

typedef enum {
  POSTMODE_FRAME,
  POSTMODE_STREAM
} PostMode;

typedef struct TestNPObject : NPObject {
  NPP npp;
  DrawMode drawMode;
  PRUint32 drawColor; 
} TestNPObject;

typedef struct _PlatformData PlatformData;

typedef struct TestRange : NPByteRange {
  bool waiting;
} TestRange;

typedef struct InstanceData {
  NPP npp;
  NPWindow window;
  TestNPObject* scriptableObject;
  PlatformData* platformData;
  int32_t instanceCountWatchGeneration;
  bool lastReportedPrivateModeState;
  bool hasWidget;
  bool npnNewStream;
  uint32_t timerID1;
  uint32_t timerID2;
  int32_t lastMouseX;
  int32_t lastMouseY;
  int32_t writeCount;
  int32_t writeReadyCount;
  TestFunction testFunction;
  TestFunction functionToFail;
  NPError failureCode;
  PostMode postMode;
  std::string testUrl;
  std::string frame;
  std::ostringstream err;
  uint16_t streamMode;
  int32_t streamChunkSize;
  int32_t streamBufSize;
  int32_t fileBufSize;
  TestRange* testrange;
  void* streamBuf;
  void* fileBuf;
} InstanceData;

#endif 
