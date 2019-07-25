
































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
  FUNCTION_NPP_DESTROYSTREAM,
  FUNCTION_NPP_WRITE_RPC
} TestFunction;

typedef enum {
  ACTIVATION_STATE_UNKNOWN,
  ACTIVATION_STATE_ACTIVATED,
  ACTIVATION_STATE_DEACTIVATED
} ActivationState;

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
  bool throwOnNextInvoke;
  bool runScriptOnPaint;
  uint32_t timerID[2];
  bool timerTestResult;
  bool asyncCallbackResult;
  bool invalidateDuringPaint;
  int32_t winX;
  int32_t winY;
  int32_t lastMouseX;
  int32_t lastMouseY;
  int32_t widthAtLastPaint;
  int32_t paintCount;
  int32_t writeCount;
  int32_t writeReadyCount;
  int32_t asyncTestPhase;
  TestFunction testFunction;
  TestFunction functionToFail;
  NPError failureCode;
  NPObject* callOnDestroy;
  PostMode postMode;
  std::string testUrl;
  std::string frame;
  std::string timerTestScriptCallback;
  std::string asyncTestScriptCallback;
  std::ostringstream err;
  uint16_t streamMode;
  int32_t streamChunkSize;
  int32_t streamBufSize;
  int32_t fileBufSize;
  TestRange* testrange;
  void* streamBuf;
  void* fileBuf;
  bool crashOnDestroy;
  bool cleanupWidget;
  ActivationState topLevelWindowActivationState;
  int32_t topLevelWindowActivationEventCount;
  ActivationState focusState;
  int32_t focusEventCount;
  int32_t eventModel;
  bool closeStream;
  std::string lastKeyText;
  bool wantsAllStreams;
} InstanceData;

void notifyDidPaint(InstanceData* instanceData);

#endif 
