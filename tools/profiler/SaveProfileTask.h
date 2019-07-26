




#ifndef PROFILER_SAVETASK_H_
#define PROFILER_SAVETASK_H_

#include "platform.h"
#include "nsThreadUtils.h"
#include "nsIXULRuntime.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "jsfriendapi.h"
#include "nsIJSRuntimeService.h"
#include "nsIProfileSaveEvent.h"

#include <iostream>
#include <fstream>

#ifdef XP_WIN
 #include <windows.h>
 #define getpid GetCurrentProcessId
#else
 #include <unistd.h>
#endif





class SaveProfileTask : public nsRunnable {
public:
  SaveProfileTask() {}

  NS_IMETHOD Run();
};

class ProfileSaveEvent MOZ_FINAL : public nsIProfileSaveEvent {
public:
  typedef void (*AddSubProfileFunc)(const char* aProfile, void* aClosure);
  NS_DECL_ISUPPORTS

  ProfileSaveEvent(AddSubProfileFunc aFunc, void* aClosure)
    : mFunc(aFunc)
    , mClosure(aClosure)
  {}

  ~ProfileSaveEvent() {}

  NS_IMETHOD AddSubProfile(const char* aProfile);
private:
  AddSubProfileFunc mFunc;
  void* mClosure;
};

#endif

